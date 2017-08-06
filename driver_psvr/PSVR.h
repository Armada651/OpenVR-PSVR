#pragma once

#include <stdint.h>

namespace psvr
{

static const uint16_t EDIDVendor = 0xD94D;
static const uint16_t EDIDProduct = 0xB403;
static const wchar_t* EDIDString = L"SNYB403";

static const uint16_t USBVendor = 0x054C;
static const uint16_t USBProduct = 0x09AF;

static const uint16_t InterfaceSensor = 4;
static const uint16_t InterfaceControl = 5;
static const uint16_t InterfaceControl2 = 8;

static const uint16_t EndpointControl = 4;

enum StatusMask
{
	NoStatus = 0,
	HeadsetOn = (1 << 0),
	Worn = (1 << 1),
	Cinematic = (1 << 2),
	UnknownA = (1 << 3),
	Headphones = (1 << 4),
	Mute = (1 << 5),
	CEC = (1 << 6),
	UnknownC = (1 << 7),
};

struct DeviceStatusReport
{
	uint8_t ReportID;
	StatusMask Status;
	uint32_t Volume;
	uint16_t UnknownA;
	uint8_t BridgeOutputID;
	uint8_t UnknownB;
};

static const uint8_t DeviceStatusReportID = 0xF0;

struct SetVRTrackingReport
{
	uint32_t UnknownA;
	uint32_t UnknownB;
};

static const uint32_t UnknownEnableVRTracking = 0xFFFFFF00;
static const int32_t SetVRTrackingReportID = 0x11;

struct SetHeadsetStateReport
{
	uint32_t HeadsetOn;
};

static const int32_t SetHeadsetStateReportID = 0x17;

struct SetVRModeReport
{
	uint32_t Enabled;
};

static const int32_t SetVRModeReportID = 0x23;

struct Report
{
	uint8_t ReportID;
	uint8_t CommandStatus;
	uint8_t DataStart;
	uint8_t DataLength;
	union {
		SetVRTrackingReport SetVRTracking;
		SetHeadsetStateReport SetHeadsetState;
		SetVRModeReport SetVRMode;
		uint8_t Array[100];
	};
};

static const size_t ReportHeaderSize = 4;

}
