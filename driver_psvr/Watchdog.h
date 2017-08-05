#pragma once

#include <openvr_driver.h>
#include <thread>

class CWatchdogDriver_PSVR : public vr::IVRWatchdogProvider
{
public:
	CWatchdogDriver_PSVR()
	{
		m_pWatchdogThread = nullptr;
	}

	virtual vr::EVRInitError Init(vr::IVRDriverContext *pDriverContext);
	virtual void Cleanup();

private:
	std::thread *m_pWatchdogThread;
};
