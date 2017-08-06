#include "Watchdog.h"
#include "DriverLog.h"
#include "PSVR.h"

#include <openvr_driver.h>
#include <libusb.h>
#include <vector>
#include <thread>
#include <chrono>

using namespace vr;

bool g_bExiting = false;

void WatchdogThreadFunction()
{
	libusb_device_handle* handle = libusb_open_device_with_vid_pid(NULL, psvr::USBVendor, psvr::USBProduct);
	if (!handle)
	{
		DriverLog("Watchdog: PSVR headset not found");
		return;
	}

	libusb_claim_interface(handle, psvr::InterfaceControl);

	psvr::StatusMask lastStatus = psvr::NoStatus;
	while (!g_bExiting)
	{
		psvr::DeviceStatusReport report;
		int bytes = 0;
		int res = libusb_interrupt_transfer(handle, psvr::DeviceStatusReportEndpoint, (uint8_t*)&report, sizeof(report), &bytes, 0);

		if (res < 0)
		{
			DriverLog("Watchdog: Interrupt transfer failed (%d).", res);
			break;
		}

		if (bytes != sizeof(report) || report.ReportID != psvr::DeviceStatusReportID)
			continue;
		
		if (!(lastStatus & psvr::HeadsetOn) && report.Status & psvr::HeadsetOn)
			vr::VRWatchdogHost()->WatchdogWakeUp();
		lastStatus = report.Status;
	}

	libusb_release_interface(handle, psvr::InterfaceControl);
	libusb_close(handle);
}

EVRInitError CWatchdogDriver_PSVR::Init(vr::IVRDriverContext *pDriverContext)
{
	VR_INIT_WATCHDOG_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());
	if (libusb_init(NULL) < 0)
	{
		DriverLog("Watchdog: Unable to initialize libusb\n");
		return VRInitError_Driver_Failed;
	}

	g_bExiting = false;
	m_pWatchdogThread = new std::thread(WatchdogThreadFunction);
	if (!m_pWatchdogThread)
	{
		DriverLog("Watchdog: Unable to create watchdog thread\n");
		return VRInitError_Driver_Failed;
	}

	return VRInitError_None;
}


void CWatchdogDriver_PSVR::Cleanup()
{
	g_bExiting = true;
	if (m_pWatchdogThread)
	{
		m_pWatchdogThread->join();
		delete m_pWatchdogThread;
		m_pWatchdogThread = nullptr;
	}

	libusb_exit(NULL);
	CleanupDriverLog();
}
