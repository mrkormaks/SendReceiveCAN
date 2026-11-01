#ifndef CAN_BRIDGE_CAN_CAN_FRAME_H_
#define CAN_BRIDGE_CAN_CAN_FRAME_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <ostream>
#include <string>

namespace can_bridge {

// Bits of CanFrame::flags.
inline constexpr uint8_t kCanFlagRemote = 1u << 0;    // Remote request.
inline constexpr uint8_t kCanFlagExtended = 1u << 1;  // 29-bit identifier.
// Status report from the controller instead of a bus message. The bits below
// describe what happened.
inline constexpr uint8_t kCanFlagError = 1u << 2;
inline constexpr uint8_t kCanFlagErrorPassive = 1u << 3;
inline constexpr uint8_t kCanFlagBusOff = 1u << 4;
inline constexpr uint8_t kCanFlagControllerOverrun = 1u << 5;
inline constexpr uint8_t kCanFlagRxBufferOverrun = 1u << 6;

// Classic CAN 2.0 frame. Bytes past `length` are always zero, so decoders can
// read data[0..7] without checking the length first.
struct CanFrame {
  static constexpr size_t kMaxLength = 8;

  uint32_t id = 0;
  uint8_t length = 0;
  uint8_t flags = 0;
  std::array<uint8_t, kMaxLength> data = {};

  bool is_error() const { return (flags & kCanFlagError) != 0; }
  bool is_remote() const { return (flags & kCanFlagRemote) != 0; }

  bool operator==(const CanFrame& other) const = default;
};

// Builds a data frame. Payload bytes beyond kMaxLength are ignored.
CanFrame MakeCanFrame(uint32_t id, std::initializer_list<uint8_t> payload);

// Human-readable form for logs, e.g. "id=0x8e [2a 14]".
std::string DescribeCanFrame(const CanFrame& frame);

std::ostream& operator<<(std::ostream& stream, const CanFrame& frame);

}  // namespace can_bridge

#endif  // CAN_BRIDGE_CAN_CAN_FRAME_H_
