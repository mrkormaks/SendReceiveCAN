#ifndef CAN_BRIDGE_PROTOCOL_ROTARY_SWITCH_H_
#define CAN_BRIDGE_PROTOCOL_ROTARY_SWITCH_H_

#include <cstdint>
#include <optional>

namespace can_bridge::protocol {

// Decodes the direction of an endless three-phase rotary switch.
//
// The switch closes one of three contacts at a time and reports them as
// one-hot bits 0..2 (values 1, 2, 4). Stepping 1 -> 2 -> 4 -> 1 is clockwise.
// Unlike the legacy code, the first sample only initialises the tracker and
// samples with zero or several bits set (contact bounce) are ignored, so they
// can no longer produce phantom steps.
class RotarySwitchTracker {
 public:
  // Returns +1 (clockwise), -1 (counter-clockwise) or 0.
  int Update(uint8_t phase_bits);

 private:
  std::optional<int> last_phase_;
};

}  // namespace can_bridge::protocol

#endif  // CAN_BRIDGE_PROTOCOL_ROTARY_SWITCH_H_
