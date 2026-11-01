#ifndef CAN_BRIDGE_COCKPIT_INPUT_SCHEDULER_H_
#define CAN_BRIDGE_COCKPIT_INPUT_SCHEDULER_H_

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "cockpit/input_fields.h"
#include "cockpit/level_inputs.h"

namespace can_bridge {

// What the simulator receives in one publish cycle.
struct PublishBatch {
  // Complete state of all level fields.
  LevelInputs levels;
  // Changes of direct fields, in arrival order.
  std::vector<FieldWrite> direct_writes;
};

// Turns the stream of input events into per-cycle simulator updates without
// losing short-lived states.
//
// The simulator samples its inputs periodically, so every state has to stay
// visible for at least one publish cycle: a key that is pressed and released
// between two cycles must still be seen as pressed once, and two encoder
// steps must produce two pulses. Therefore an event is deferred to a later
// cycle when it assigns a field that an earlier event already assigned in the
// current cycle. Deferral is ordered: once an event is deferred, every later
// event touching any of its fields is deferred as well, so per-field order is
// always preserved. Unrelated events are not held back.
//
// Enqueue() may be called from any thread; NextBatch() from one thread.
class InputScheduler {
 public:
  InputScheduler(int initial_kontur_percent, size_t max_pending_events);

  InputScheduler(const InputScheduler&) = delete;
  InputScheduler& operator=(const InputScheduler&) = delete;

  // Queues an event. When the queue is full, the oldest event is dropped.
  void Enqueue(InputEvent event);

  // Applies as many queued events as possible and returns the result.
  PublishBatch NextBatch();

  size_t pending_events() const;
  uint64_t dropped_events() const;

 private:
  // Applies one write to `levels_` or records it in `batch`.
  void ApplyWrite(const FieldWrite& write, PublishBatch* batch);
  void ResetPulses();

  const size_t max_pending_events_;
  mutable std::mutex mutex_;
  std::deque<InputEvent> pending_;
  LevelInputs levels_;
  uint64_t dropped_events_ = 0;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_COCKPIT_INPUT_SCHEDULER_H_
