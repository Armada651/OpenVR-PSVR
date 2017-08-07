#include "DeviceDriver.h"
#include "DriverLog.h"
#include "PSVR.h"

#ifdef _WINDOWS
#include <dxgi.h>
#endif

#include <openvr_driver.h>
#include <libusb.h>

using namespace vr;

#ifndef M_SQRT2_2
#define M_SQRT2_2 0.70710678118654752440084436210484
#endif

#ifndef M_PI_180
#define M_PI_180 0.01745329251994329576923690768489
#endif

CPSVRDeviceDriver::CPSVRDeviceDriver()
{
	m_pDeviceHandle = nullptr;
	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

	DriverLog("Using settings values\n");
	m_flIPD = vr::VRSettings()->GetFloat(k_pch_SteamVR_Section, k_pch_SteamVR_IPD_Float);

	char buf[1024];
	vr::VRSettings()->GetString(k_pch_PSVR_Section, k_pch_PSVR_SerialNumber_String, buf, sizeof(buf));
	m_sSerialNumber = buf;

	vr::VRSettings()->GetString(k_pch_PSVR_Section, k_pch_PSVR_ModelNumber_String, buf, sizeof(buf));
	m_sModelNumber = buf;

	m_nWindowX = vr::VRSettings()->GetInt32(k_pch_PSVR_Section, k_pch_PSVR_WindowX_Int32);
	m_nWindowY = vr::VRSettings()->GetInt32(k_pch_PSVR_Section, k_pch_PSVR_WindowY_Int32);
	m_nWindowWidth = vr::VRSettings()->GetInt32(k_pch_PSVR_Section, k_pch_PSVR_WindowWidth_Int32);
	m_nWindowHeight = vr::VRSettings()->GetInt32(k_pch_PSVR_Section, k_pch_PSVR_WindowHeight_Int32);
	m_nRenderWidth = vr::VRSettings()->GetInt32(k_pch_PSVR_Section, k_pch_PSVR_RenderWidth_Int32);
	m_nRenderHeight = vr::VRSettings()->GetInt32(k_pch_PSVR_Section, k_pch_PSVR_RenderHeight_Int32);
	m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(k_pch_PSVR_Section, k_pch_PSVR_SecondsFromVsyncToPhotons_Float);
	m_flDisplayFrequency = vr::VRSettings()->GetFloat(k_pch_PSVR_Section, k_pch_PSVR_DisplayFrequency_Float);

	DriverLog("driver_null: Serial Number: %s\n", m_sSerialNumber.c_str());
	DriverLog("driver_null: Model Number: %s\n", m_sModelNumber.c_str());
	DriverLog("driver_null: Window: %d %d %d %d\n", m_nWindowX, m_nWindowY, m_nWindowWidth, m_nWindowHeight);
	DriverLog("driver_null: Render Target: %d %d\n", m_nRenderWidth, m_nRenderHeight);
	DriverLog("driver_null: Seconds from Vsync to Photons: %f\n", m_flSecondsFromVsyncToPhotons);
	DriverLog("driver_null: Display Frequency: %f\n", m_flDisplayFrequency);
	DriverLog("driver_null: IPD: %f\n", m_flIPD);
}

EVRInitError CPSVRDeviceDriver::Activate(vr::TrackedDeviceIndex_t unObjectId)
{
	m_unObjectId = unObjectId;
	m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);
	m_pDeviceHandle = libusb_open_device_with_vid_pid(NULL, psvr::USBVendor, psvr::USBProduct);
	if (!m_pDeviceHandle)
		return VRInitError_Init_HmdNotFound;

	FindHmdDisplay(psvr::EDIDString, &m_pDisplayOutput);

	libusb_claim_interface(m_pDeviceHandle, psvr::InterfaceControl);
	libusb_claim_interface(m_pDeviceHandle, psvr::InterfaceSensor);

	{
		psvr::Report report = {};
		report.ReportID = psvr::SetHeadsetStateReportID;
		report.SetHeadsetState.HeadsetOn = true;
		report.DataLength = sizeof(psvr::SetHeadsetStateReport);
		SendReport(report);
	}

	{
		psvr::Report report = {};
		report.ReportID = psvr::SetVRModeReportID;
		report.SetVRMode.Enabled = true;
		report.DataLength = sizeof(psvr::SetVRModeReport);
		SendReport(report);
	}

	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str());
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, m_sModelNumber.c_str());
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserIpdMeters_Float, m_flIPD);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.f);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_DisplayFrequency_Float, m_flDisplayFrequency);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_SecondsFromVsyncToPhotons_Float, m_flSecondsFromVsyncToPhotons);

	// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
	vr::VRProperties()->SetUint64Property(m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2);

	// avoid "not fullscreen" warnings from vrmonitor
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_IsOnDesktop_Bool, false);

	// Icons can be configured in code or automatically configured by an external file "drivername\resources\driver.vrresources".
	// Icon properties NOT configured in code (post Activate) are then auto-configured by the optional presence of a driver's "drivername\resources\driver.vrresources".
	// In this manner a driver can configure their icons in a flexible data driven fashion by using an external file.
	//
	// The structure of the driver.vrresources file allows a driver to specialize their icons based on their HW.
	// Keys matching the value in "Prop_ModelNumber_String" are considered first, since the driver may have model specific icons.
	// An absence of a matching "Prop_ModelNumber_String" then considers the ETrackedDeviceClass ("HMD", "Controller", "GenericTracker", "TrackingReference")
	// since the driver may have specialized icons based on those device class names.
	//
	// An absence of either then falls back to the "system.vrresources" where generic device class icons are then supplied.
	//
	// Please refer to "bin\drivers\psvr\resources\driver.vrresources" which contains this sample configuration.
	//
	// "Alias" is a reserved key and specifies chaining to another json block.
	//
	// In this sample configuration file (overly complex FOR EXAMPLE PURPOSES ONLY)....
	//
	// "Model-v2.0" chains through the alias to "Model-v1.0" which chains through the alias to "Model-v Defaults".
	//
	// Keys NOT found in "Model-v2.0" would then chase through the "Alias" to be resolved in "Model-v1.0" and either resolve their or continue through the alias.
	// Thus "Prop_NamedIconPathDeviceAlertLow_String" in each model's block represent a specialization specific for that "model".
	// Keys in "Model-v Defaults" are an example of mapping to the same states, and here all map to "Prop_NamedIconPathDeviceOff_String".
	//
	bool bSetupIconUsingExternalResourceFile = true;
	if (!bSetupIconUsingExternalResourceFile)
	{
		// Setup properties directly in code.
		// Path values are of the form {drivername}\icons\some_icon_filename.png
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceOff_String, "{psvr}/icons/headset_psvr_status_off.png");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearching_String, "{psvr}/icons/headset_psvr_status_searching.gif");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{psvr}/icons/headset_psvr_status_searching_alert.gif");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReady_String, "{psvr}/icons/headset_psvr_status_ready.png");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{psvr}/icons/headset_psvr_status_ready_alert.png");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceNotReady_String, "{psvr}/icons/headset_psvr_status_error.png");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceStandby_String, "{psvr}/icons/headset_psvr_status_standby.png");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceAlertLow_String, "{psvr}/icons/headset_psvr_status_ready_low.png");

		vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, vr::Prop_EdidVendorID_Int32, psvr::EDIDVendor);
		vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, vr::Prop_EdidProductID_Int32, psvr::EDIDProduct);
	}

	m_hPoseThread = std::thread(PoseThread, this);
	return VRInitError_None;
}

void CPSVRDeviceDriver::Deactivate()
{
	{
		psvr::Report report = {};
		report.ReportID = psvr::SetVRModeReportID;
		report.SetVRMode.Enabled = false;
		report.DataLength = sizeof(psvr::SetVRModeReport);
		SendReport(report);
	}

	{
		psvr::Report report = {};
		report.ReportID = psvr::SetHeadsetStateReportID;
		report.SetHeadsetState.HeadsetOn = false;
		report.DataLength = sizeof(psvr::SetHeadsetStateReport);
		SendReport(report);
	}

	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	if (m_hPoseThread.joinable())
		m_hPoseThread.join();

	libusb_release_interface(m_pDeviceHandle, psvr::InterfaceSensor);
	libusb_release_interface(m_pDeviceHandle, psvr::InterfaceControl);
	libusb_close(m_pDeviceHandle);
	m_pDeviceHandle = nullptr;
}

void CPSVRDeviceDriver::EnterStandby()
{
}

void CPSVRDeviceDriver::GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight)
{
	*pnX = m_nWindowX;
	*pnY = m_nWindowY;
	*pnWidth = m_nWindowWidth;
	*pnHeight = m_nWindowHeight;

#ifdef _WINDOWS
	DXGI_OUTPUT_DESC desc;
	if (m_pDisplayOutput && SUCCEEDED(m_pDisplayOutput->GetDesc(&desc))) {
		*pnX = desc.DesktopCoordinates.left;
		*pnY = desc.DesktopCoordinates.top;
		*pnWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
		*pnHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
	}
#endif
}

bool CPSVRDeviceDriver::IsDisplayOnDesktop()
{
#ifdef _WINDOWS
	DXGI_OUTPUT_DESC desc;
	if (m_pDisplayOutput && SUCCEEDED(m_pDisplayOutput->GetDesc(&desc)))
		return !!desc.AttachedToDesktop;
#endif
	// The headset is either in Direct Mode or disconnected.
	return false;
}

void CPSVRDeviceDriver::GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight)
{
	*pnWidth = m_nRenderWidth;
	*pnHeight = m_nRenderHeight;
}

void CPSVRDeviceDriver::GetEyeOutputViewport(EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight)
{
	*pnY = 0;
	*pnWidth = m_nWindowWidth / 2;
	*pnHeight = m_nWindowHeight;

	if (eEye == Eye_Left)
	{
		*pnX = 0;
	}
	else
	{
		*pnX = m_nWindowWidth / 2;
	}
}

void CPSVRDeviceDriver::GetProjectionRaw(EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom)
{
	float halfFov = (float)tan((100.0 * M_PI_180) / 2.0);

	*pfLeft = -halfFov;
	*pfRight = halfFov;
	*pfTop = -halfFov;
	*pfBottom = halfFov;
}

DistortionCoordinates_t CPSVRDeviceDriver::ComputeDistortion(EVREye eEye, float fU, float fV)
{
	DistortionCoordinates_t coordinates;
	coordinates.rfBlue[0] = fU;
	coordinates.rfBlue[1] = fV;
	coordinates.rfGreen[0] = fU;
	coordinates.rfGreen[1] = fV;
	coordinates.rfRed[0] = fU;
	coordinates.rfRed[1] = fV;
	return coordinates;
}

DriverPose_t CPSVRDeviceDriver::GetPose()
{
	psvr::SensorReport report = {};
	int bytes = 0;
	int res = libusb_interrupt_transfer(m_pDeviceHandle, psvr::EndpointSensor | LIBUSB_ENDPOINT_IN, (uint8_t*)&report, sizeof(report), &bytes, 0);

	DriverPose_t pose = { 0 };
	pose.poseIsValid = true;
	pose.result = TrackingResult_Running_OK;
	pose.deviceIsConnected = true;

	float gyro[] = {
		(report.IMU[0].Gyroscope[0] / 131.2f) * M_PI_180,
		(report.IMU[0].Gyroscope[1] / 131.2f) * M_PI_180,
		(report.IMU[0].Gyroscope[2] / 131.2f) * M_PI_180
	};

	float accel[] = {
		(report.IMU[0].Accelerometer[0] >> 4) / 1024.f,
		(report.IMU[0].Accelerometer[1] >> 4) / 1024.f,
		(report.IMU[0].Accelerometer[2] >> 4) / 1024.f
	};

	Madgwick::AHRSupdateIMU(gyro[0], gyro[1], gyro[2], accel[0], accel[1], accel[2]);

	pose.qWorldFromDriverRotation = HmdQuaternion_Init(-0.5, 0.5, 0.5, 0.5);
	pose.qDriverFromHeadRotation = HmdQuaternion_Init(0, M_SQRT2_2, M_SQRT2_2, 0);
	pose.qRotation = HmdQuaternion_Init(Madgwick::q0, Madgwick::q1, Madgwick::q2, Madgwick::q3);

	return pose;
}


void CPSVRDeviceDriver::PoseThread(CPSVRDeviceDriver* device)
{
	while (device->m_unObjectId != vr::k_unTrackedDeviceIndexInvalid)
	{
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(device->m_unObjectId, device->GetPose(), sizeof(DriverPose_t));
	}
}

inline HmdQuaternion_t CPSVRDeviceDriver::HmdQuaternion_Init(double w, double x, double y, double z) const
{
	HmdQuaternion_t quat;
	quat.w = w;
	quat.x = x;
	quat.y = y;
	quat.z = z;
	return quat;
}

inline void CPSVRDeviceDriver::HmdMatrix_SetIdentity(HmdMatrix34_t *pMatrix) const
{
	pMatrix->m[0][0] = 1.f;
	pMatrix->m[0][1] = 0.f;
	pMatrix->m[0][2] = 0.f;
	pMatrix->m[0][3] = 0.f;
	pMatrix->m[1][0] = 0.f;
	pMatrix->m[1][1] = 1.f;
	pMatrix->m[1][2] = 0.f;
	pMatrix->m[1][3] = 0.f;
	pMatrix->m[2][0] = 0.f;
	pMatrix->m[2][1] = 0.f;
	pMatrix->m[2][2] = 1.f;
	pMatrix->m[2][3] = 0.f;
}

#ifdef _WINDOWS
bool CPSVRDeviceDriver::FindHmdDisplay(const wchar_t* pSerialNumber, IDXGIOutput** ppOutputDesc) const
{
	IDXGIFactory1 * pFactory;
	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory)))
		return false;

	DISPLAY_DEVICE display;
	memset(&display, 0, sizeof(DISPLAY_DEVICE));
	display.cb = sizeof(display);

	bool found = false;
	DXGI_OUTPUT_DESC desc;
	IDXGIAdapter* pAdapter;
	IDXGIOutput* pOutput;

	// Search all adapters for the headset display
	for (uint32_t i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND && !found; ++i)
	{
		for (uint32_t j = 0; pAdapter->EnumOutputs(j, &pOutput) != DXGI_ERROR_NOT_FOUND && !found; ++j)
		{
			if (SUCCEEDED(pOutput->GetDesc(&desc)) && EnumDisplayDevices(desc.DeviceName, 0, &display, 0))
			{
				// Check if this display belongs to the headset by looking for the serial number in the DeviceID
				if (wcsstr(display.DeviceID, pSerialNumber))
				{
					// Keep the reference count as we will return this interface
					found = true;
					*ppOutputDesc = pOutput;
				}
				else
				{
					// Decrement reference count to avoid leaking this interface
					pOutput->Release();
				}
			}
		}
		pAdapter->Release();
	}
	pFactory->Release();

	return found;
}
#endif

bool CPSVRDeviceDriver::SendReport(psvr::Report& report)
{
	report.DataStart = 0xAA; // Ensure this is always 0xAA
	int res = libusb_interrupt_transfer(m_pDeviceHandle, psvr::EndpointControl | LIBUSB_ENDPOINT_OUT, (uint8_t*)&report, report.DataLength + psvr::ReportHeaderSize, nullptr, 0);
	if (res < 0)
	{
		DriverLog("DeviceDriver: Failed to send command report (%d).\n", res);
		return false;
	}

	res = libusb_interrupt_transfer(m_pDeviceHandle, psvr::EndpointControl | LIBUSB_ENDPOINT_IN, (uint8_t*)&report, sizeof(report), nullptr, 0);
	if (res < 0)
	{
		DriverLog("DeviceDriver: Failed to read command response (%d).\n", res);
		return false;
	}

	if (report.CommandStatus != 0)
	{
		DriverLog("DeviceDriver: Report %x status failed (%d).\n", report.ReportID, report.CommandStatus);
		return false;
	}

	return true;
}
