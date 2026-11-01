#include "cockpit/input_scheduler.h"

#include <gtest/gtest.h>

namespace can_bridge {
namespace {

constexpr int kSlot = 5;

InputEvent PressMfpu(int slot) {
  return {AssignField(FieldKind::kMfpuButton, kSideLeft, slot, 1)};
}

InputEvent ReleaseMfpu(std::initializer_list<int> slots) {
  InputEvent event;
  for (int slot : slots) {
    event.push_back(AssignField(FieldKind::kMfpuButton, kSideLeft, slot, 0));
  }
  return event;
}

InputEvent OuterStep(int mfi, int step) {
  return {AssignField(FieldKind::kMfiOuterKnobStep, mfi, 0, step)};
}

TEST(InputSchedulerTest, ShortPressIsVisibleForOneCycle) {
  InputScheduler scheduler(75, 100);
  scheduler.Enqueue(PressMfpu(kSlot));
  scheduler.Enqueue(ReleaseMfpu({kSlot}));

  EXPECT_TRUE(scheduler.NextBatch().levels.mfpu_buttons[kSideLeft][kSlot]);
  EXPECT_FALSE(scheduler.NextBatch().levels.mfpu_buttons[kSideLeft][kSlot]);
  EXPECT_EQ(scheduler.pending_events(), 0u);
}

TEST(InputSchedulerTest, LevelsPersistAndPulsesLastOneCycle) {
  InputScheduler scheduler(75, 100);
  scheduler.Enqueue(PressMfpu(kSlot));
  scheduler.Enqueue(OuterStep(kMfiLeft, 1));

  PublishBatch batch = scheduler.NextBatch();
  EXPECT_TRUE(batch.levels.mfpu_buttons[kSideLeft][kSlot]);
  EXPECT_EQ(batch.levels.mfi[kMfiLeft].outer_knob_step, 1);

  batch = scheduler.NextBatch();
  EXPECT_TRUE(batch.levels.mfpu_buttons[kSideLeft][kSlot]);
  EXPECT_EQ(batch.levels.mfi[kMfiLeft].outer_knob_step, 0);
}

TEST(InputSchedulerTest, EveryEncoderStepGetsItsOwnCycle) {
  InputScheduler scheduler(75, 100);
  for (int i = 0; i < 3; ++i) scheduler.Enqueue(OuterStep(kMfiCenter, -1));

  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(scheduler.NextBatch().levels.mfi[kMfiCenter].outer_knob_step, -1);
  }
  EXPECT_EQ(scheduler.NextBatch().levels.mfi[kMfiCenter].outer_knob_step, 0);
}

TEST(InputSchedulerTest, UnrelatedEventsAreNotHeldBack) {
  InputScheduler scheduler(75, 100);
  scheduler.Enqueue(OuterStep(kMfiLeft, 1));
  scheduler.Enqueue(OuterStep(kMfiLeft, 1));  // Deferred.
  scheduler.Enqueue(PressMfpu(kSlot));        // Not related: applied now.

  const PublishBatch batch = scheduler.NextBatch();
  EXPECT_EQ(batch.levels.mfi[kMfiLeft].outer_knob_step, 1);
  EXPECT_TRUE(batch.levels.mfpu_buttons[kSideLeft][kSlot]);
  EXPECT_EQ(scheduler.pending_events(), 1u);
}

TEST(InputSchedulerTest, DeferralKeepsPerFieldOrder) {
  InputScheduler scheduler(75, 100);
  scheduler.Enqueue(PressMfpu(1));
  scheduler.Enqueue(ReleaseMfpu({1, 2}));  // Conflicts with the press.
  scheduler.Enqueue(PressMfpu(2));         // Must stay after the release.

  PublishBatch batch = scheduler.NextBatch();
  EXPECT_TRUE(batch.levels.mfpu_buttons[kSideLeft][1]);
  EXPECT_FALSE(batch.levels.mfpu_buttons[kSideLeft][2]);

  batch = scheduler.NextBatch();
  EXPECT_FALSE(batch.levels.mfpu_buttons[kSideLeft][1]);
  EXPECT_FALSE(batch.levels.mfpu_buttons[kSideLeft][2]);

  batch = scheduler.NextBatch();
  EXPECT_TRUE(batch.levels.mfpu_buttons[kSideLeft][2]);
}

TEST(InputSchedulerTest, CountersAccumulateWithinOneCycle) {
  InputScheduler scheduler(75, 100);
  for (int i = 0; i < 3; ++i) {
    scheduler.Enqueue({AddToField(FieldKind::kIsrpBaroRoll, 0, 0, 1)});
  }
  scheduler.Enqueue({AddToField(FieldKind::kIsrpBaroRoll, 0, 0, -1)});
  EXPECT_EQ(scheduler.NextBatch().levels.isrp.baro_roll_steps, 2);
}

TEST(InputSchedulerTest, KonturIsClampedAndStartsFromConfig) {
  InputScheduler scheduler(95, 100);
  EXPECT_EQ(scheduler.NextBatch().levels.isrp.kontur_percent, 95);
  scheduler.Enqueue({AddToField(FieldKind::kIsrpKontur, 0, 0, 10)});
  EXPECT_EQ(scheduler.NextBatch().levels.isrp.kontur_percent, 100);
  for (int i = 0; i < 12; ++i) {
    scheduler.Enqueue({AddToField(FieldKind::kIsrpKontur, 0, 0, -10)});
  }
  EXPECT_EQ(scheduler.NextBatch().levels.isrp.kontur_percent, 0);
}

TEST(InputSchedulerTest, So2010KeysArePulses) {
  InputScheduler scheduler(75, 100);
  scheduler.Enqueue({AssignField(FieldKind::kSo2010Button, 0, 3, 1),
                     AssignField(FieldKind::kSo2010EnteredCode, 0, 0, 1234)});
  PublishBatch batch = scheduler.NextBatch();
  EXPECT_TRUE(batch.levels.so2010.buttons[3]);
  EXPECT_EQ(batch.levels.so2010.entered_code_a, 1234);

  batch = scheduler.NextBatch();
  EXPECT_FALSE(batch.levels.so2010.buttons[3]);
  EXPECT_EQ(batch.levels.so2010.entered_code_a, 1234);
}

TEST(InputSchedulerTest, DirectWritesArePassedOnInOrder) {
  InputScheduler scheduler(75, 100);
  const FieldWrite on = AssignField(FieldKind::kOverheadSwitch, 0, 7, 1);
  const FieldWrite off = AssignField(FieldKind::kOverheadSwitch, 0, 7, 0);
  const FieldWrite other = AssignField(FieldKind::kOverheadSwitch, 0, 8, 1);
  scheduler.Enqueue({on});
  scheduler.Enqueue({off});
  scheduler.Enqueue({other});

  EXPECT_EQ(scheduler.NextBatch().direct_writes,
            (std::vector<FieldWrite>{on, other}));
  EXPECT_EQ(scheduler.NextBatch().direct_writes,
            (std::vector<FieldWrite>{off}));
  EXPECT_TRUE(scheduler.NextBatch().direct_writes.empty());
}

TEST(InputSchedulerTest, OverflowDropsOldestEvents) {
  InputScheduler scheduler(75, 2);
  scheduler.Enqueue(OuterStep(kMfiLeft, 1));
  scheduler.Enqueue(OuterStep(kMfiLeft, -1));
  scheduler.Enqueue(OuterStep(kMfiLeft, -1));
  EXPECT_EQ(scheduler.dropped_events(), 1u);
  EXPECT_EQ(scheduler.NextBatch().levels.mfi[kMfiLeft].outer_knob_step, -1);
}

TEST(InputSchedulerTest, IgnoresEmptyEvents) {
  InputScheduler scheduler(75, 100);
  scheduler.Enqueue({});
  EXPECT_EQ(scheduler.pending_events(), 0u);
}

}  // namespace
}  // namespace can_bridge
