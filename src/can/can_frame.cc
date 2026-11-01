#include "can/can_frame.h"

#include <cstdio>

namespace can_bridge {

CanFrame MakeCanFrame(uint32_t id, std::initializer_list<uint8_t> payload) {
  CanFrame frame;
  frame.id = id;
  for (uint8_t byte : payload) {
    if (frame.length == CanFrame::kMaxLength) break;
    frame.data[frame.length++] = byte;
  }
  return frame;
}

std::string DescribeCanFrame(const CanFrame& frame) {
  char buffer[96];
  int written = std::snprintf(buffer, sizeof(buffer), "id=0x%x%s [",
                              static_cast<unsigned>(frame.id),
                              frame.is_error() ? " (error)" : "");
  std::string result(buffer, static_cast<size_t>(written));
  for (size_t i = 0; i < frame.length && i < CanFrame::kMaxLength; ++i) {
    written = std::snprintf(buffer, sizeof(buffer), i == 0 ? "%02x" : " %02x",
                            static_cast<unsigned>(frame.data[i]));
    result.append(buffer, static_cast<size_t>(written));
  }
  result += ']';
  return result;
}

std::ostream& operator<<(std::ostream& stream, const CanFrame& frame) {
  return stream << DescribeCanFrame(frame);
}

}  // namespace can_bridge
