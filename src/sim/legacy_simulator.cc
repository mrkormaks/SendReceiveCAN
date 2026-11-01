#include "sim/legacy_simulator.h"

#include <cstddef>
#include <type_traits>

namespace can_bridge {
namespace {

// Assigns with an explicit conversion, so the code works whatever arithmetic
// type the simulator uses for a field (bool, int, double, ...).
template <typename Field, typename Value>
void Store(Field& field, Value value) {
  field = static_cast<Field>(value);
}

template <typename Field>
void AddTo(Field& field, int delta) {
  field = static_cast<Field>(field + delta);
}

// Number of elements of an array (or reference to array) along `Dim`.
template <typename Array, unsigned Dim = 0>
constexpr size_t kExtent = std::extent_v<std::remove_reference_t<Array>, Dim>;

// Reads a field another process may change at any time.
template <typename T>
T VolatileLoad(const T& field) {
  return *static_cast<const volatile T*>(&field);
}

template <typename Panel>
void WriteB8Button(Panel& panel, B8Button button, bool pressed) {
  switch (button) {
    case B8Button::kTriangle1:
      Store(panel.line1treug, pressed);
      break;
    case B8Button::kTriangle2:
      Store(panel.line2treug, pressed);
      break;
    case B8Button::kTriangle3:
      Store(panel.line3treug, pressed);
      break;
    case B8Button::kPsh1:
      Store(panel.button_psh_1, pressed);
      break;
    case B8Button::kPsh2:
      Store(panel.button_psh_2, pressed);
      break;
    case B8Button::kPsh3:
      Store(panel.button_psh_3, pressed);
      break;
    case B8Button::kApr:
      Store(panel.button_APr, pressed);
      break;
    case B8Button::kRi:
      Store(panel.button_RI, pressed);
      break;
    case B8Button::kReset:
      Store(panel.button_sbros, pressed);
      break;
    case B8Button::kEnter:
      Store(panel.button_vvod, pressed);
      break;
    case B8Button::kCa:
      Store(panel.button_CA, pressed);
      break;
    case B8Button::kDigit0:
      Store(panel.button_0, pressed);
      break;
    case B8Button::kDigit1:
      Store(panel.button_1, pressed);
      break;
    case B8Button::kDigit2:
      Store(panel.button_2, pressed);
      break;
    case B8Button::kDigit3:
      Store(panel.button_3, pressed);
      break;
    case B8Button::kDigit4:
      Store(panel.button_4, pressed);
      break;
    case B8Button::kDigit5:
      Store(panel.button_5, pressed);
      break;
    case B8Button::kDigit6:
      Store(panel.button_6, pressed);
      break;
    case B8Button::kDigit7:
      Store(panel.button_7, pressed);
      break;
    case B8Button::kDigit8:
      Store(panel.button_8, pressed);
      break;
    case B8Button::kDigit9:
      Store(panel.button_9, pressed);
      break;
    case B8Button::kCount:
      break;
  }
}

template <typename Panel>
void WriteB8Selector(Panel& panel, const FieldWrite& write) {
  const bool is_set = write.key.index == static_cast<int>(B8Selector::kSet);
  if (write.key.kind == FieldKind::kB8SelectorPosition) {
    if (is_set) {
      Store(panel.nabor, write.value);
    } else {
      Store(panel.gromk, write.value);
    }
  } else {
    // The legacy code incremented the counters in shared memory, so the
    // simulator may reset them; keep adding to whatever is there.
    if (is_set) {
      AddTo(panel.nabor_roll, write.value);
    } else {
      AddTo(panel.gromk_roll, write.value);
    }
  }
}

}  // namespace

void LegacySimulator::Publish(const PublishBatch& batch) {
  WriteLevels(batch.levels);
  for (const FieldWrite& write : batch.direct_writes) ApplyDirectWrite(write);
}

void LegacySimulator::WriteLevels(const LevelInputs& levels) {
  auto& mfi_in = avn_->IBKO.MFI_IN;
  static_assert(kExtent<decltype(mfi_in.Buttons_FK), 0> >= kMfiFkSlots &&
                kExtent<decltype(mfi_in.Buttons_FK), 1> >= kMfiCount);
  static_assert(kExtent<decltype(mfi_in.Buttons_MFK), 0> >= kMfiMfkSlots &&
                kExtent<decltype(mfi_in.Buttons_MFK), 1> >= kMfiCount);
  for (int mfi = 0; mfi < kMfiCount; ++mfi) {
    const MfiLevels& source = levels.mfi[mfi];
    for (int slot = 0; slot < kMfiFkSlots; ++slot) {
      Store(mfi_in.Buttons_FK[slot][mfi], source.fk_buttons[slot]);
    }
    for (int slot = 0; slot < kMfiMfkSlots; ++slot) {
      Store(mfi_in.Buttons_MFK[slot][mfi], source.mfk_buttons[slot]);
    }
    Store(mfi_in.brightness[mfi], source.brightness_step);
    Store(mfi_in.brightness_button[mfi], source.brightness_button);
    Store(mfi_in.indik_internal[mfi], source.inner_knob_step);
    Store(mfi_in.indik_external[mfi], source.outer_knob_step);
    Store(mfi_in.indik_button[mfi], source.indik_button);
  }

  auto& mfpu_buttons = avn_->IBKO.MFPU_IN.Buttons;
  static_assert(kExtent<decltype(mfpu_buttons), 0> >= kMfpuButtonSlots &&
                kExtent<decltype(mfpu_buttons), 1> >= kMfpuCount);
  for (int side = 0; side < kMfpuCount; ++side) {
    for (int slot = 0; slot < kMfpuButtonSlots; ++slot) {
      Store(mfpu_buttons[slot][side], levels.mfpu_buttons[side][slot]);
    }
  }

  // The encoders produce two steps per detent.
  const IsrpLevels& isrp = levels.isrp;
  const double baro_roll = isrp.baro_roll_steps / 2.0;
  const double hdg_roll = isrp.hdg_roll_steps / 2.0;
  auto& cabin_isrp = avn_->CAB.ISRP26;
  Store(cabin_isrp.Brightness_minus, isrp.brightness_minus);
  Store(cabin_isrp.Brightness_plus, isrp.brightness_plus);
  Store(cabin_isrp.Fast_Korrection, isrp.fast_correction);
  Store(cabin_isrp.HDG_press, isrp.hdg_press);
  Store(cabin_isrp.BARO_press, isrp.baro_press);
  Store(cabin_isrp.BARO_roll, baro_roll);
  Store(cabin_isrp.HDG_roll, hdg_roll);
  auto& instrument_isrp = avn_->COM.PRIB.ISRP26;
  Store(instrument_isrp.BARO_press, isrp.baro_press);
  Store(instrument_isrp.HDG_press, isrp.hdg_press);
  Store(instrument_isrp.BARO_roll, baro_roll);
  Store(instrument_isrp.HDG_roll, hdg_roll);
  Store(avn_->CAB.Kontur1, isrp.kontur_percent);

  auto& from_can = avn_->CAN.fromCan;
  static_assert(kExtent<decltype(from_can.buttons)> >= kSo2010ButtonCount);
  for (int key = 0; key < kSo2010ButtonCount; ++key) {
    Store(from_can.buttons[key], levels.so2010.buttons[key]);
  }
  Store(from_can.enteredCodeA, levels.so2010.entered_code_a);
}

void LegacySimulator::ApplyDirectWrite(const FieldWrite& write) {
  const int unit = write.key.unit;
  const int index = write.key.index;
  switch (write.key.kind) {
    case FieldKind::kB8Button: {
      const auto button = static_cast<B8Button>(index);
      const bool pressed = write.value != 0;
      if (unit == kSideLeft) {
        WriteB8Button(avn_->IBKO.B8_LEFT_IN, button, pressed);
      } else {
        WriteB8Button(avn_->IBKO.B8_RIGHT_IN, button, pressed);
      }
      break;
    }
    case FieldKind::kB8SelectorPosition:
    case FieldKind::kB8SelectorRoll:
      if (unit == kSideLeft) {
        WriteB8Selector(avn_->IBKO.B8_LEFT_IN, write);
      } else {
        WriteB8Selector(avn_->IBKO.B8_RIGHT_IN, write);
      }
      break;
    case FieldKind::kOverheadSwitch: {
      auto& switches = avn_->PCIE_1680_E1_1;
      if (static_cast<size_t>(index) < kExtent<decltype(switches)>) {
        Store(switches[index], write.value != 0);
      }
      break;
    }
    default:
      break;  // Level fields are written by WriteLevels().
  }
}

protocol::IndicatorValues LegacySimulator::ReadIndicators() const {
  const auto& to_can = avn_->CAN.toCan;
  protocol::IndicatorValues values;
  values.compass_heading_deg =
      static_cast<double>(VolatileLoad(to_can.KompasKurs));
  values.slip_ball = static_cast<double>(VolatileLoad(to_can.Sharik));
  values.adf_frequency_khz = static_cast<double>(VolatileLoad(to_can.ark_Freq));
  values.so2010_message = static_cast<int>(VolatileLoad(to_can.message));
  values.so2010_code = static_cast<int>(VolatileLoad(to_can.displayedCode));
  return values;
}

void LegacySimulator::SetDeviceFault(int flag, bool faulty) {
  switch (flag) {
    case 1:
      Store(sut_->RMI_IN.DevOtk1, faulty);
      break;
    case 2:
      Store(sut_->RMI_IN.DevOtk2, faulty);
      break;
    default:
      break;
  }
}

bool LegacySimulator::IsCloseRequested() const {
  return static_cast<bool>(VolatileLoad(sut_->Close));
}

}  // namespace can_bridge
