#pragma once

#include "PSVR.h"
#include "MadgwickAHRS.h"

#ifdef _WINDOWS
#include <dxgi.h>
#endif

#include <openvr_driver.h>
#include <thread>

struct libusb_device_handle;

class CPSVRDeviceDriver : public vr::ITrackedDeviceServerDriver, public vr::IVRDisplayComponent
{
public:
	// keys for use with the settings API
	const char * const k_pch_PSVR_Section = "driver_psvr";
	const char * const k_pch_PSVR_SerialNumber_String = "serialNumber";
	const char * const k_pch_PSVR_ModelNumber_String = "modelNumber";
	const char * const k_pch_PSVR_WindowX_Int32 = "windowX";
	const char * const k_pch_PSVR_WindowY_Int32 = "windowY";
	const char * const k_pch_PSVR_WindowWidth_Int32 = "windowWidth";
	const char * const k_pch_PSVR_WindowHeight_Int32 = "windowHeight";
	const char * const k_pch_PSVR_RenderWidth_Int32 = "renderWidth";
	const char * const k_pch_PSVR_RenderHeight_Int32 = "renderHeight";
	const char * const k_pch_PSVR_SecondsFromVsyncToPhotons_Float = "secondsFromVsyncToPhotons";
	const char * const k_pch_PSVR_DisplayFrequency_Float = "displayFrequency";

	CPSVRDeviceDriver();
	virtual ~CPSVRDeviceDriver() { }

	virtual vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId);
	virtual void Deactivate();
	virtual void EnterStandby();

	void *GetComponent(const char *pchComponentNameAndVersion)
	{
		if (!_stricmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version))
		{
			return (vr::IVRDisplayComponent*)this;
		}

		// override this to add a component to a driver
		return NULL;
	}

	/** debug request from a client */
	virtual void DebugRequest(const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize)
	{
		if (unResponseBufferSize >= 1)
			pchResponseBuffer[0] = 0;
	}

	virtual void GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight);
	virtual bool IsDisplayOnDesktop();
	virtual bool IsDisplayRealDisplay() { return true; }

	virtual void GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight);
	virtual void GetEyeOutputViewport(vr::EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight);
	virtual void GetProjectionRaw(vr::EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom);
	virtual vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eEye, float fU, float fV);
	virtual vr::DriverPose_t GetPose();
	virtual void RunFrame() { }

	std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
	libusb_device_handle* m_pDeviceHandle;
	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	std::string m_sSerialNumber;
	std::string m_sModelNumber;

	int32_t m_nWindowX;
	int32_t m_nWindowY;
	int32_t m_nWindowWidth;
	int32_t m_nWindowHeight;
	int32_t m_nRenderWidth;
	int32_t m_nRenderHeight;
	float m_flSecondsFromVsyncToPhotons;
	float m_flDisplayFrequency;
	float m_flIPD;

	inline vr::HmdQuaternion_t HmdQuaternion_Init(double w, double x, double y, double z) const;
	inline void HmdMatrix_SetIdentity(vr::HmdMatrix34_t *pMatrix) const;

#ifdef _WINDOWS
	IDXGIOutput* m_pDisplayOutput;
	bool CPSVRDeviceDriver::FindHmdDisplay(const wchar_t* pSerialNumber, IDXGIOutput** ppOutputDesc) const;
#endif

	bool SendReport(psvr::Report& report);

	std::thread m_hPoseThread;
	static void PoseThread(CPSVRDeviceDriver* device);
};
