#include "ServerDriver.h"
#include "DriverLog.h"
#include "DeviceDriver.h"

#include <openvr_driver.h>
#include <libusb.h>

using namespace vr;

EVRInitError CServerDriver_PSVR::Init(vr::IVRDriverContext *pDriverContext)
{
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());
	if (libusb_init(NULL) < 0)
	{
		DriverLog("Watchdog: Unable to initialize libusb\n");
		return VRInitError_Driver_Failed;
	}

	m_pPSVRHmdLatest = new CPSVRDeviceDriver();
	vr::VRServerDriverHost()->TrackedDeviceAdded(m_pPSVRHmdLatest->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, m_pPSVRHmdLatest);
	return VRInitError_None;
}

void CServerDriver_PSVR::Cleanup()
{
	libusb_exit(NULL);
	CleanupDriverLog();
	delete m_pPSVRHmdLatest;
	m_pPSVRHmdLatest = NULL;
}


void CServerDriver_PSVR::RunFrame()
{
	if (m_pPSVRHmdLatest)
	{
		m_pPSVRHmdLatest->RunFrame();
	}
}
