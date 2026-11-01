#ifndef CAN_BRIDGE_PROTOCOL_OVERHEAD_DECODER_H_
#define CAN_BRIDGE_PROTOCOL_OVERHEAD_DECODER_H_

#include <vector>

#include "protocol/frame_decoder.h"

namespace can_bridge::protocol {

// Decodes the overhead panel bus: seven rows of toggle switches (codes 71..77).
// Row frames carry switch states in data[2] (bits -> slots +0..7) and data[1]
// (bits -> slots +8..15); each row occupies 16 slots of the simulator array.
class OverheadDecoder : public FrameDecoder {
 public:
  DecodeResult Decode(const CanFrame& frame) override;

  // Frames that make the controller report all rows. Sent after every
  // (re)connect so the simulator always starts from the real switch state.
  static std::vector<CanFrame> MakePollRequests();
};

}  // namespace can_bridge::protocol

#endif  // CAN_BRIDGE_PROTOCOL_OVERHEAD_DECODER_H_
