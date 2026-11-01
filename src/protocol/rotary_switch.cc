#include "protocol/rotary_switch.h"

namespace can_bridge::protocol {
namespace {

std::optional<int> PhaseIndex(uint8_t phase_bits) {
  switch (phase_bits & 0x07) {
    case 0x01:
      return 0;
    case 0x02:
      return 1;
    case 0x04:
      return 2;
    default:
      return std::nullopt;
  }
}

}  // namespace

int RotarySwitchTracker::Update(uint8_t phase_bits) {
  const std::optional<int> phase = PhaseIndex(phase_bits);
  if (!phase.has_value()) return 0;

  int step = 0;
  if (last_phase_.has_value()) {
    // On a three-phase ring every other phase is a neighbour: one position
    // forward is clockwise, two positions forward is one step back.
    switch ((*phase - *last_phase_ + 3) % 3) {
      case 1:
        step = 1;
        break;
      case 2:
        step = -1;
        break;
      default:
        break;
    }
  }
  last_phase_ = phase;
  return step;
}

}  // namespace can_bridge::protocol
