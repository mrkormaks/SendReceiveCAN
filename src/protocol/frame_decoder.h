#ifndef CAN_BRIDGE_PROTOCOL_FRAME_DECODER_H_
#define CAN_BRIDGE_PROTOCOL_FRAME_DECODER_H_

#include <optional>

#include "can/can_frame.h"
#include "cockpit/input_fields.h"

namespace can_bridge::protocol {

struct DecodeResult {
  // Input changes caused by the frame; empty if the frame is not an input.
  InputEvent event;
  // Confirmation the controller expects for this frame, if any.
  std::optional<CanFrame> ack;
};

// Converts frames of one CAN bus into input events. Decoders may keep state
// (e.g. encoder phases), so each bus has its own instance used by one thread.
class FrameDecoder {
 public:
  virtual ~FrameDecoder() = default;
  virtual DecodeResult Decode(const CanFrame& frame) = 0;
};

}  // namespace can_bridge::protocol

#endif  // CAN_BRIDGE_PROTOCOL_FRAME_DECODER_H_
