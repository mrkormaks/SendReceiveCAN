#include "protocol/panel_decoder.h"

#include <optional>

#include "protocol/key_tables.h"
#include "protocol/protocol_constants.h"

namespace can_bridge::protocol {
namespace {

// Knob frames up to 3 bytes long report a rotation step, longer ones the
// knob's push button.
bool IsRotation(const CanFrame& frame) { return frame.length <= 3; }

// data[1] of a rotation frame: 0 = clockwise, 1 = counter-clockwise.
std::optional<int> RotationStep(const CanFrame& frame) {
  switch (frame.data[1]) {
    case 0:
      return 1;
    case 1:
      return -1;
    default:
      return std::nullopt;
  }
}

// data[3] of a push button frame: 1 = pressed, 0 = released.
std::optional<bool> PushButtonState(const CanFrame& frame) {
  switch (frame.data[3]) {
    case 0:
      return false;
    case 1:
      return true;
    default:
      return std::nullopt;
  }
}

void PushStep(FieldKind kind, int unit, const CanFrame& frame,
              InputEvent* event) {
  if (const std::optional<int> step = RotationStep(frame)) {
    event->push_back(AssignField(kind, unit, 0, *step));
  }
}

void PushButton(FieldKind kind, int unit, const CanFrame& frame,
                InputEvent* event) {
  if (const std::optional<bool> pressed = PushButtonState(frame)) {
    event->push_back(AssignField(kind, unit, 0, *pressed ? 1 : 0));
  }
}

// Position of the lower (detented) part of a B8-50KR selector: one-hot in
// bits 3..7. If several bits are set, the highest one wins (legacy rule).
std::optional<int> SelectorPosition(uint8_t bits) {
  std::optional<int> position;
  for (int bit = 3; bit < 8; ++bit) {
    if ((bits & (1u << bit)) != 0) position = bit - 3;
  }
  return position;
}

bool NeedsAck(uint8_t device) {
  switch (device) {
    case kB8LeftKeys:
    case kB8RightKeys:
    case kMfiCenterKeys:
    case kMfiRightKeys:
    case kMfiLeftKeys:
    case kMfpuRightMainKeys:
    case kMfpuRightExtraKeys:
    case kMfpuLeftMainKeys:
    case kMfpuLeftExtraKeys:
      return true;
    default:
      return false;
  }
}

}  // namespace

DecodeResult PanelDecoder::Decode(const CanFrame& frame) {
  DecodeResult result;
  if (frame.length == 0 || frame.is_error() || frame.is_remote()) {
    return result;
  }

  InputEvent* event = &result.event;
  const uint8_t device = frame.data[0];
  switch (device) {
    case kMfiLeftOuterKnob:
      DecodeMfiOuterKnob(kMfiLeft, frame, event);
      break;
    case kMfiCenterOuterKnob:
      DecodeMfiOuterKnob(kMfiCenter, frame, event);
      break;
    case kMfiRightOuterKnob:
      DecodeMfiOuterKnob(kMfiRight, frame, event);
      break;

    case kMfiLeftInnerKnob:
      PushStep(FieldKind::kMfiInnerKnobStep, kMfiLeft, frame, event);
      break;
    case kMfiCenterInnerKnob:
      if (IsRotation(frame)) {
        PushStep(FieldKind::kMfiInnerKnobStep, kMfiCenter, frame, event);
      } else {
        PushButton(FieldKind::kIsrpBrightnessPlus, 0, frame, event);
      }
      break;
    case kMfiRightInnerKnob:
      PushStep(FieldKind::kMfiInnerKnobStep, kMfiRight, frame, event);
      break;

    case kMfiLeftBrightness:
      PushStep(FieldKind::kMfiBrightnessStep, kMfiLeft, frame, event);
      break;
    case kMfiCenterBrightness:
      if (IsRotation(frame)) {
        // This knob also sets the "КОНТУР" backlight in 10 % steps and, as
        // in the legacy code, reports brightness in the opposite direction.
        if (const std::optional<int> step = RotationStep(frame)) {
          event->push_back(
              AddToField(FieldKind::kIsrpKontur, 0, 0, *step * 10));
          event->push_back(AssignField(FieldKind::kMfiBrightnessStep,
                                       kMfiCenter, 0, -*step));
        }
      } else {
        PushButton(FieldKind::kIsrpFastCorrection, 0, frame, event);
      }
      break;
    case kMfiRightBrightness:
      if (IsRotation(frame)) {
        PushStep(FieldKind::kMfiBrightnessStep, kMfiRight, frame, event);
      } else {
        PushButton(FieldKind::kIsrpBrightnessMinus, 0, frame, event);
      }
      break;

    case kIsrpBaroKnob:
      if (IsRotation(frame)) {
        if (const std::optional<int> step = RotationStep(frame)) {
          event->push_back(AddToField(FieldKind::kIsrpBaroRoll, 0, 0, *step));
        }
      } else {
        PushButton(FieldKind::kIsrpBaroPress, 0, frame, event);
      }
      break;
    case kIsrpHdgKnob:
      DecodeHdgKnob(frame, event);
      break;

    case kB8LeftKeys:
      DecodeB8Keys(kSideLeft, frame, event);
      break;
    case kB8RightKeys:
      DecodeB8Keys(kSideRight, frame, event);
      break;
    case kB8LeftSelectors:
      DecodeB8Selectors(kSideLeft, frame, event);
      break;
    case kB8RightSelectors:
      DecodeB8Selectors(kSideRight, frame, event);
      break;

    case kMfiLeftKeys:
      DecodeMfiKeys(kMfiLeft, frame, event);
      break;
    case kMfiCenterKeys:
      DecodeMfiKeys(kMfiCenter, frame, event);
      break;
    case kMfiRightKeys:
      DecodeMfiKeys(kMfiRight, frame, event);
      break;

    case kMfpuLeftMainKeys:
      DecodeMfpuKeys(kSideLeft, /*extra_group=*/false, frame, event);
      break;
    case kMfpuLeftExtraKeys:
      DecodeMfpuKeys(kSideLeft, /*extra_group=*/true, frame, event);
      break;
    case kMfpuRightMainKeys:
      DecodeMfpuKeys(kSideRight, /*extra_group=*/false, frame, event);
      break;
    case kMfpuRightExtraKeys:
      DecodeMfpuKeys(kSideRight, /*extra_group=*/true, frame, event);
      break;

    case kSo2010:
      DecodeSo2010(frame, event);
      break;

    default:
      break;
  }

  if (NeedsAck(device)) {
    result.ack = MakeCanFrame(kAckIdOffset + device, {device, frame.data[1]});
  }
  return result;
}

void PanelDecoder::DecodeMfiOuterKnob(int mfi, const CanFrame& frame,
                                      InputEvent* event) {
  if (IsRotation(frame)) {
    PushStep(FieldKind::kMfiOuterKnobStep, mfi, frame, event);
  } else {
    PushButton(FieldKind::kMfiIndikButton, mfi, frame, event);
  }
}

void PanelDecoder::DecodeHdgKnob(const CanFrame& frame, InputEvent* event) {
  switch (options_.hdg_knob_mode) {
    case HdgKnobMode::kLeftMfiOuterKnob:
      if (IsRotation(frame)) {
        PushStep(FieldKind::kMfiOuterKnobStep, kMfiLeft, frame, event);
      }
      break;
    case HdgKnobMode::kIsrpHdg:
      if (IsRotation(frame)) {
        if (const std::optional<int> step = RotationStep(frame)) {
          event->push_back(AddToField(FieldKind::kIsrpHdgRoll, 0, 0, *step));
        }
      } else {
        PushButton(FieldKind::kIsrpHdgPress, 0, frame, event);
      }
      break;
  }
}

void PanelDecoder::DecodeMfiKeys(int mfi, const CanFrame& frame,
                                 InputEvent* event) {
  if (const std::optional<MfiKeyMapping> key = FindMfiKey(frame.data[1])) {
    const FieldKind kind =
        key->upper_row ? FieldKind::kMfiFkButton : FieldKind::kMfiMfkButton;
    event->push_back(AssignField(kind, mfi, key->slot, 1));
    return;
  }
  // Release: every key of this indicator goes up.
  for (int slot = 1; slot < kMfiFkSlots; ++slot) {
    event->push_back(AssignField(FieldKind::kMfiFkButton, mfi, slot, 0));
  }
  for (int slot = 1; slot < kMfiMfkSlots; ++slot) {
    event->push_back(AssignField(FieldKind::kMfiMfkButton, mfi, slot, 0));
  }
}

void PanelDecoder::DecodeMfpuKeys(int side, bool extra_group,
                                  const CanFrame& frame, InputEvent* event) {
  const std::span<const KeyMapping> keys =
      extra_group ? MfpuExtraKeys() : MfpuMainKeys();
  if (const std::optional<uint8_t> slot =
          FindMfpuKeySlot(keys, frame.data[1])) {
    event->push_back(AssignField(FieldKind::kMfpuButton, side, *slot, 1));
    return;
  }
  // Release: every key of this group goes up.
  for (const KeyMapping& key : keys) {
    event->push_back(AssignField(FieldKind::kMfpuButton, side, key.slot, 0));
  }
}

void PanelDecoder::DecodeB8Keys(int side, const CanFrame& frame,
                                InputEvent* event) {
  if (const std::optional<B8Button> button = FindB8Key(side, frame.data[1])) {
    event->push_back(
        AssignField(FieldKind::kB8Button, side, static_cast<int>(*button), 1));
    return;
  }
  // Release: every key of this panel goes up.
  for (int button = 0; button < kB8ButtonCount; ++button) {
    event->push_back(AssignField(FieldKind::kB8Button, side, button, 0));
  }
}

void PanelDecoder::DecodeB8Selectors(int side, const CanFrame& frame,
                                     InputEvent* event) {
  const uint8_t set_bits = frame.data[1];
  const uint8_t volume_bits = frame.data[2];
  constexpr int kSet = static_cast<int>(B8Selector::kSet);
  constexpr int kVolume = static_cast<int>(B8Selector::kVolume);

  if (const std::optional<int> position = SelectorPosition(set_bits)) {
    event->push_back(
        AssignField(FieldKind::kB8SelectorPosition, side, kSet, *position));
  }
  std::optional<int> volume_position = SelectorPosition(volume_bits);
  if (frame.data[3] != 0) {
    volume_position = 5;  // Position "РНУ 3" has its own contact.
  }
  if (volume_position.has_value()) {
    event->push_back(AssignField(FieldKind::kB8SelectorPosition, side, kVolume,
                                 *volume_position));
  }

  // Upper (endless) part of the selectors: three-phase code in bits 0..2.
  if (const int step = rotation_[side][kSet].Update(set_bits); step != 0) {
    event->push_back(AddToField(FieldKind::kB8SelectorRoll, side, kSet, step));
  }
  if (const int step = rotation_[side][kVolume].Update(volume_bits);
      step != 0) {
    event->push_back(
        AddToField(FieldKind::kB8SelectorRoll, side, kVolume, step));
  }
}

void PanelDecoder::DecodeSo2010(const CanFrame& frame, InputEvent* event) {
  const uint8_t key = frame.data[1];
  // Other codes (e.g. 23 = brightness) are handled by the controller itself.
  if (key < 1 || key > kSo2010ButtonCount) return;

  event->push_back(AssignField(FieldKind::kSo2010Button, 0, key - 1, 1));
  if (key == kSo2010CodeKey) {
    // Four decimal digits in data[2..5]; a short frame clears the code.
    int code = 0;
    if (frame.length > 2) {
      code = frame.data[2] * 1000 + frame.data[3] * 100 + frame.data[4] * 10 +
             frame.data[5];
    }
    event->push_back(AssignField(FieldKind::kSo2010EnteredCode, 0, 0, code));
  }
}

}  // namespace can_bridge::protocol
