#include "cockpit/input_scheduler.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace can_bridge {
namespace {

int Combine(int current, const FieldWrite& write) {
  return write.op == WriteOp::kAdd ? current + write.value : write.value;
}

bool CombineFlag(bool current, const FieldWrite& write) {
  return Combine(current ? 1 : 0, write) != 0;
}

}  // namespace

InputScheduler::InputScheduler(int initial_kontur_percent,
                               size_t max_pending_events)
    : max_pending_events_(std::max<size_t>(1, max_pending_events)) {
  levels_.isrp.kontur_percent = std::clamp(initial_kontur_percent, 0, 100);
}

void InputScheduler::Enqueue(InputEvent event) {
  if (event.empty()) return;
  std::lock_guard<std::mutex> lock(mutex_);
  if (pending_.size() >= max_pending_events_) {
    pending_.pop_front();
    ++dropped_events_;
  }
  pending_.push_back(std::move(event));
}

PublishBatch InputScheduler::NextBatch() {
  std::lock_guard<std::mutex> lock(mutex_);
  ResetPulses();

  PublishBatch batch;
  // Fields that must not change any more in this cycle: fields assigned by
  // applied events and all fields of deferred events.
  std::unordered_set<uint32_t> blocked;
  std::deque<InputEvent> deferred;

  while (!pending_.empty()) {
    InputEvent event = std::move(pending_.front());
    pending_.pop_front();

    const bool conflicts =
        std::any_of(event.begin(), event.end(), [&](const FieldWrite& write) {
          return blocked.count(write.key.Packed()) != 0;
        });
    if (conflicts) {
      for (const FieldWrite& write : event) blocked.insert(write.key.Packed());
      deferred.push_back(std::move(event));
      continue;
    }

    for (const FieldWrite& write : event) {
      ApplyWrite(write, &batch);
      if (write.op == WriteOp::kAssign) blocked.insert(write.key.Packed());
    }
  }

  pending_ = std::move(deferred);
  batch.levels = levels_;
  return batch;
}

size_t InputScheduler::pending_events() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pending_.size();
}

uint64_t InputScheduler::dropped_events() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dropped_events_;
}

void InputScheduler::ResetPulses() {
  for (MfiLevels& mfi : levels_.mfi) {
    mfi.brightness_step = 0;
    mfi.inner_knob_step = 0;
    mfi.outer_knob_step = 0;
  }
  levels_.so2010.buttons.fill(false);
}

void InputScheduler::ApplyWrite(const FieldWrite& write, PublishBatch* batch) {
  const FieldKind kind = write.key.kind;
  if (IsDirectField(kind)) {
    batch->direct_writes.push_back(write);
    return;
  }

  const int unit = write.key.unit;
  const int index = write.key.index;
  const bool valid_mfi = unit < kMfiCount;
  IsrpLevels& isrp = levels_.isrp;

  switch (kind) {
    case FieldKind::kMfiFkButton:
      if (valid_mfi && index < kMfiFkSlots) {
        bool& key = levels_.mfi[unit].fk_buttons[index];
        key = CombineFlag(key, write);
      }
      break;
    case FieldKind::kMfiMfkButton:
      if (valid_mfi && index < kMfiMfkSlots) {
        bool& key = levels_.mfi[unit].mfk_buttons[index];
        key = CombineFlag(key, write);
      }
      break;
    case FieldKind::kMfiBrightnessStep:
      if (valid_mfi) {
        int& step = levels_.mfi[unit].brightness_step;
        step = Combine(step, write);
      }
      break;
    case FieldKind::kMfiInnerKnobStep:
      if (valid_mfi) {
        int& step = levels_.mfi[unit].inner_knob_step;
        step = Combine(step, write);
      }
      break;
    case FieldKind::kMfiOuterKnobStep:
      if (valid_mfi) {
        int& step = levels_.mfi[unit].outer_knob_step;
        step = Combine(step, write);
      }
      break;
    case FieldKind::kMfiIndikButton:
      if (valid_mfi) {
        bool& key = levels_.mfi[unit].indik_button;
        key = CombineFlag(key, write);
      }
      break;
    case FieldKind::kMfpuButton:
      if (unit < kMfpuCount && index < kMfpuButtonSlots) {
        bool& key = levels_.mfpu_buttons[unit][index];
        key = CombineFlag(key, write);
      }
      break;
    case FieldKind::kIsrpBrightnessMinus:
      isrp.brightness_minus = CombineFlag(isrp.brightness_minus, write);
      break;
    case FieldKind::kIsrpBrightnessPlus:
      isrp.brightness_plus = CombineFlag(isrp.brightness_plus, write);
      break;
    case FieldKind::kIsrpFastCorrection:
      isrp.fast_correction = CombineFlag(isrp.fast_correction, write);
      break;
    case FieldKind::kIsrpHdgPress:
      isrp.hdg_press = CombineFlag(isrp.hdg_press, write);
      break;
    case FieldKind::kIsrpBaroPress:
      isrp.baro_press = CombineFlag(isrp.baro_press, write);
      break;
    case FieldKind::kIsrpBaroRoll:
      isrp.baro_roll_steps = Combine(isrp.baro_roll_steps, write);
      break;
    case FieldKind::kIsrpHdgRoll:
      isrp.hdg_roll_steps = Combine(isrp.hdg_roll_steps, write);
      break;
    case FieldKind::kIsrpKontur:
      isrp.kontur_percent =
          std::clamp(Combine(isrp.kontur_percent, write), 0, 100);
      break;
    case FieldKind::kSo2010Button:
      if (index < kSo2010ButtonCount) {
        bool& key = levels_.so2010.buttons[index];
        key = CombineFlag(key, write);
      }
      break;
    case FieldKind::kSo2010EnteredCode:
      levels_.so2010.entered_code_a =
          Combine(levels_.so2010.entered_code_a, write);
      break;
    case FieldKind::kB8Button:
    case FieldKind::kB8SelectorPosition:
    case FieldKind::kB8SelectorRoll:
    case FieldKind::kOverheadSwitch:
      break;  // Direct fields are handled above.
  }
}

}  // namespace can_bridge
