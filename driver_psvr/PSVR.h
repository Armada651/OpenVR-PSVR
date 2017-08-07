#pragma once

#include <stdint.h>

namespace psvr
{

#pragma pack(push, 1)

static const uint16_t EDIDVendor = 0xD94D;
static const uint16_t EDIDProduct = 0xB403;
static const wchar_t* EDIDString = L"SNYB403";

static const uint16_t USBVendor = 0x054C;
static const uint16_t USBProduct = 0x09AF;

static const uint16_t InterfaceSensor = 4;
static const uint16_t InterfaceControl = 5;
static const uint16_t InterfaceControl2 = 8;

static const uint16_t EndpointSensor = 3;
static const uint16_t EndpointControl = 4;

enum StatusMask
{
	NoStatus = 0,
	HeadsetOn = (1 << 0),
	Worn = (1 << 1),
	Cinematic = (1 << 2),
	UnknownA = (1 << 3),
	Headphones = (1 << 4),
	Muted = (1 << 5),
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

enum HeadsetButtons
{
	VolUp = 0x2,
	VolDown = 0x4,
	Mute = 0x8
};

enum HeadsetMode
{
	CinematicMode = 0x0,
	VRMode = 0x40
};

struct IMUReport
{
	int32_t Timestamp;
	int16_t Gyroscope[3];
	int16_t Accelerometer[3];
};

struct SensorReport
{
	uint8_t Buttons;		// 00
	uint8_t UnknownA;		// 01
	uint8_t Volume;			// 02
	uint8_t UnknownB[5];	// 03
	uint8_t Status;			// 08
	uint8_t UnknownC[6];	// 09
	uint8_t Mode;			// 15
	IMUReport IMU[2];		// 16
	uint8_t Calibration;	// 48
	uint8_t Ready;			// 49
	uint8_t UnknownE[3];	// 50
	uint8_t VoltageRef;		// 53
	uint8_t Voltage;		// 54
	int16_t Proximity;		// 55
	uint8_t UnknownF[4];	// 57
	int16_t SamplingPeriod;	// 61
	uint8_t PacketSequence; // 63
};

#pragma pack(pop)

}
