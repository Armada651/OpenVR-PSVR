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
static const uint8_t DeviceStatusReportEndpoint = 0x84;

}
