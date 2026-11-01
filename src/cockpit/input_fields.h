#ifndef CAN_BRIDGE_COCKPIT_INPUT_FIELDS_H_
#define CAN_BRIDGE_COCKPIT_INPUT_FIELDS_H_

#include <cstdint>
#include <vector>

namespace can_bridge {

// Sizes follow the legacy simulator layout. Where the legacy arrays reserve
// slot 0 to match the hardware documentation, so do these.
inline constexpr int kMfiCount = 3;          // Left, center, right MFI.
inline constexpr int kMfiFkSlots = 13;       // Upper keys 1..12.
inline constexpr int kMfiMfkSlots = 29;      // Lower/side keys 1..28.
inline constexpr int kMfpuCount = 2;         // Left, right MFPU.
inline constexpr int kMfpuButtonSlots = 74;  // Keys 0..73.
inline constexpr int kB8PanelCount = 2;      // Left, right B8-50KR.
inline constexpr int kSo2010ButtonCount = 12;
inline constexpr int kOverheadSwitchSlots = 112;

// Positions of multi-unit devices, used as FieldKey::unit.
inline constexpr int kMfiLeft = 0;
inline constexpr int kMfiCenter = 1;
inline constexpr int kMfiRight = 2;
inline constexpr int kSideLeft = 0;  // MFPU and B8-50KR.
inline constexpr int kSideRight = 1;

// Keys of the B8-50KR radio control panel (Б8-50КР).
enum class B8Button : uint8_t {
  kTriangle1,  // "Треугольник" upper.
  kTriangle2,  // "Треугольник" middle.
  kTriangle3,  // "Треугольник" lower.
  kPsh1,       // "ПШ 1".
  kPsh2,
  kPsh3,
  kApr,    // "АПр".
  kRi,     // "РИ".
  kReset,  // "Сброс".
  kEnter,  // "Ввод".
  kCa,     // "CA" (no hardware code; cleared together with the others).
  kDigit0,
  kDigit1,
  kDigit2,
  kDigit3,
  kDigit4,
  kDigit5,
  kDigit6,
  kDigit7,
  kDigit8,
  kDigit9,
  kCount,
};
inline constexpr int kB8ButtonCount = static_cast<int>(B8Button::kCount);

// Rotary selectors of the B8-50KR panel.
enum class B8Selector : uint8_t {
  kSet = 0,     // "Набор".
  kVolume = 1,  // "Громк".
};

// Every input value the bridge can publish to the simulator.
enum class FieldKind : uint8_t {
  // Level fields. The bridge owns them and rewrites all of them every publish
  // cycle (legacy behaviour).
  kMfiFkButton,        // unit: MFI, index: key slot.
  kMfiMfkButton,       // unit: MFI, index: key slot.
  kMfiBrightnessStep,  // Pulse. unit: MFI.
  kMfiInnerKnobStep,   // Pulse ("ИНДИК" inner ring). unit: MFI.
  kMfiOuterKnobStep,   // Pulse ("ИНДИК" outer ring). unit: MFI.
  kMfiIndikButton,     // unit: MFI.
  kMfpuButton,         // unit: side, index: key slot.
  kIsrpBrightnessMinus,
  kIsrpBrightnessPlus,
  kIsrpFastCorrection,
  kIsrpHdgPress,
  kIsrpBaroPress,
  kIsrpBaroRoll,  // Counter of encoder steps.
  kIsrpHdgRoll,   // Counter of encoder steps.
  kIsrpKontur,    // Backlight level, percent.
  kSo2010Button,  // Pulse. index: key 0..11.
  kSo2010EnteredCode,

  // Direct fields. They are written to the simulator only when an event
  // changes them, because the simulator may modify them as well (for
  // example consume a key press). This matches the legacy behaviour.
  kB8Button,            // unit: side, index: B8Button.
  kB8SelectorPosition,  // unit: side, index: B8Selector.
  kB8SelectorRoll,      // Delta added to the simulator counter.
  kOverheadSwitch,      // index: switch slot.
};

// True for fields that are written to the simulator on change only.
bool IsDirectField(FieldKind kind);

// True for level fields that hold a value for exactly one publish cycle.
bool IsPulseField(FieldKind kind);

struct FieldKey {
  FieldKind kind = FieldKind::kMfiFkButton;
  uint8_t unit = 0;
  uint8_t index = 0;

  uint32_t Packed() const {
    return (static_cast<uint32_t>(kind) << 16) |
           (static_cast<uint32_t>(unit) << 8) | index;
  }
  bool operator==(const FieldKey& other) const = default;
};

enum class WriteOp : uint8_t {
  kAssign,  // field = value.
  kAdd,     // field += value. Commutative, so never conflicts.
};

struct FieldWrite {
  FieldKey key;
  WriteOp op = WriteOp::kAssign;
  int32_t value = 0;

  bool operator==(const FieldWrite& other) const = default;
};

FieldWrite AssignField(FieldKind kind, int unit, int index, int32_t value);
FieldWrite AddToField(FieldKind kind, int unit, int index, int32_t delta);

// Everything one CAN frame changes. Applied to the simulator atomically.
using InputEvent = std::vector<FieldWrite>;

}  // namespace can_bridge

#endif  // CAN_BRIDGE_COCKPIT_INPUT_FIELDS_H_
