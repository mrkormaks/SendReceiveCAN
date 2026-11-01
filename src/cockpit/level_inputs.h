#ifndef CAN_BRIDGE_COCKPIT_LEVEL_INPUTS_H_
#define CAN_BRIDGE_COCKPIT_LEVEL_INPUTS_H_

#include <array>

#include "cockpit/input_fields.h"

namespace can_bridge {

// Inputs of one multi-function indicator (МФИ).
struct MfiLevels {
  std::array<bool, kMfiFkSlots> fk_buttons = {};
  std::array<bool, kMfiMfkSlots> mfk_buttons = {};
  // Pulses: +1/-1 for one publish cycle per encoder step.
  int brightness_step = 0;
  int inner_knob_step = 0;
  int outer_knob_step = 0;
  bool indik_button = false;
  // Not reported by the hardware; published as false like before.
  bool brightness_button = false;

  bool operator==(const MfiLevels& other) const = default;
};

// Controls of the ISRP-26 standby instrument (ИСРП-26).
struct IsrpLevels {
  bool brightness_minus = false;
  bool brightness_plus = false;
  bool fast_correction = false;
  bool hdg_press = false;
  bool baro_press = false;
  // Raw encoder steps since start; the simulator receives steps / 2.
  int baro_roll_steps = 0;
  int hdg_roll_steps = 0;
  int kontur_percent = 75;

  bool operator==(const IsrpLevels& other) const = default;
};

// Keys of the SO-2010 transponder control panel (СО-2010).
struct So2010Levels {
  // Pulses: the hardware reports presses only, so each press is visible for
  // exactly one publish cycle.
  std::array<bool, kSo2010ButtonCount> buttons = {};
  int entered_code_a = 0;

  bool operator==(const So2010Levels& other) const = default;
};

// State of all level fields (see FieldKind) for one publish cycle.
struct LevelInputs {
  std::array<MfiLevels, kMfiCount> mfi = {};
  std::array<std::array<bool, kMfpuButtonSlots>, kMfpuCount> mfpu_buttons = {};
  IsrpLevels isrp;
  So2010Levels so2010;

  bool operator==(const LevelInputs& other) const = default;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_COCKPIT_LEVEL_INPUTS_H_
