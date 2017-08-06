#pragma once

#include <openvr_driver.h>
#include "DeviceDriver.h"

class CServerDriver_PSVR : public vr::IServerTrackedDeviceProvider
{
public:
	CServerDriver_PSVR()
		: m_pPSVRHmdLatest(nullptr)
	{
	}

	virtual vr::EVRInitError Init(vr::IVRDriverContext *pDriverContext);
	virtual void Cleanup();
	virtual const char * const *GetInterfaceVersions() { return vr::k_InterfaceVersions; }
	virtual void RunFrame();
	virtual bool ShouldBlockStandbyMode() { return false; }
	virtual void EnterStandby() {}
	virtual void LeaveStandby() {}

private:
	CPSVRDeviceDriver *m_pPSVRHmdLatest;
};
