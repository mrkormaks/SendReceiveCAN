#ifndef CAN_BRIDGE_CAN_ADVANTECH_ADVANTECH_ABI_H_
#define CAN_BRIDGE_CAN_ADVANTECH_ADVANTECH_ABI_H_

#include <cstdint>

// Binary interface of the Windows (WDM) driver for Advantech PCI/PCIe CAN
// boards, limited to what this project uses: device path, I/O control codes
// and record layouts. The driver is installed with the board; nothing else
// is needed to build or run the bridge.
namespace can_bridge::advantech_abi {

// Devices are opened as "\\.\<name>", e.g. "\\.\can1", in overlapped mode.
// ReadFile/WriteFile transfer arrays of Frame; their byte-count arguments and
// results are numbers of frames, not bytes.

constexpr uint32_t MakeControlCode(uint32_t function) {
  constexpr uint32_t kDeviceTypeUnknown = 0x22;  // FILE_DEVICE_UNKNOWN.
  constexpr uint32_t kMethodBuffered = 0;
  constexpr uint32_t kAnyAccess = 0;
  return (kDeviceTypeUnknown << 16) | (kAnyAccess << 14) | (function << 2) |
         kMethodBuffered;
}

// DeviceIoControl codes; the input buffer is a Request.
inline constexpr uint32_t kControlCommand = MakeControlCode(0x950);
inline constexpr uint32_t kControlConfigure = MakeControlCode(0x951);

// Request::command values for kControlCommand.
enum class Command : int32_t {
  kStart = 1,         // Leave reset mode, join the bus.
  kStop = 2,          // Enter reset mode.
  kClearBuffers = 4,  // Drop frames buffered by the driver.
};

// Request::target values for kControlConfigure.
enum class ConfigTarget : int32_t {
  kAcceptanceMask = 1,  // value1 = mask.
  kAcceptanceCode = 2,  // value1 = code.
  kBitRate = 3,         // value1 = bit rate in kbit/s.
  kTimeouts = 13,       // value1 = write timeout, value2 = read timeout (ms).
};

// Frame::flags bits.
inline constexpr int32_t kFlagRemote = 1 << 0;
inline constexpr int32_t kFlagControllerOverrun = 1 << 1;
inline constexpr int32_t kFlagExtended = 1 << 2;
inline constexpr int32_t kFlagErrorPassive = 1 << 4;
inline constexpr int32_t kFlagBusOff = 1 << 5;
inline constexpr int32_t kFlagRxBufferOverrun = 1 << 7;

// Identifier of status records that report controller errors.
inline constexpr uint32_t kErrorFrameId = 0xFFFFFFFF;

#pragma pack(push, 1)
struct Frame {
  int32_t flags;
  int32_t object_number;  // Unused by this driver mode.
  uint32_t id;
  int16_t length;
  uint8_t data[8];
};

struct Request {
  int32_t command;
  int32_t target;
  uint32_t value1;
  uint32_t value2;
  int32_t error;
  uint32_t result;
};
#pragma pack(pop)

static_assert(sizeof(Frame) == 22, "driver frame layout");
static_assert(sizeof(Request) == 24, "driver request layout");

}  // namespace can_bridge::advantech_abi

#endif  // CAN_BRIDGE_CAN_ADVANTECH_ADVANTECH_ABI_H_
