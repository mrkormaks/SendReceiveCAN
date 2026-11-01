#include "cockpit/input_fields.h"

namespace can_bridge {

bool IsDirectField(FieldKind kind) {
  switch (kind) {
    case FieldKind::kB8Button:
    case FieldKind::kB8SelectorPosition:
    case FieldKind::kB8SelectorRoll:
    case FieldKind::kOverheadSwitch:
      return true;
    default:
      return false;
  }
}

bool IsPulseField(FieldKind kind) {
  switch (kind) {
    case FieldKind::kMfiBrightnessStep:
    case FieldKind::kMfiInnerKnobStep:
    case FieldKind::kMfiOuterKnobStep:
    case FieldKind::kSo2010Button:
      return true;
    default:
      return false;
  }
}

FieldWrite AssignField(FieldKind kind, int unit, int index, int32_t value) {
  return FieldWrite{
      {kind, static_cast<uint8_t>(unit), static_cast<uint8_t>(index)},
      WriteOp::kAssign,
      value};
}

FieldWrite AddToField(FieldKind kind, int unit, int index, int32_t delta) {
  return FieldWrite{
      {kind, static_cast<uint8_t>(unit), static_cast<uint8_t>(index)},
      WriteOp::kAdd,
      delta};
}

}  // namespace can_bridge
