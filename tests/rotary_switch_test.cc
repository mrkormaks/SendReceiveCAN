#include "protocol/rotary_switch.h"

#include <gtest/gtest.h>

namespace can_bridge::protocol {
namespace {

TEST(RotarySwitchTrackerTest, FirstSampleOnlyInitializes) {
  RotarySwitchTracker tracker;
  EXPECT_EQ(tracker.Update(0x02), 0);
}

TEST(RotarySwitchTrackerTest, ClockwiseSequence) {
  RotarySwitchTracker tracker;
  tracker.Update(0x01);
  EXPECT_EQ(tracker.Update(0x02), 1);
  EXPECT_EQ(tracker.Update(0x04), 1);
  EXPECT_EQ(tracker.Update(0x01), 1);  // Wraps around.
}

TEST(RotarySwitchTrackerTest, CounterClockwiseSequence) {
  RotarySwitchTracker tracker;
  tracker.Update(0x01);
  EXPECT_EQ(tracker.Update(0x04), -1);  // Wraps around.
  EXPECT_EQ(tracker.Update(0x02), -1);
  EXPECT_EQ(tracker.Update(0x01), -1);
}

TEST(RotarySwitchTrackerTest, RepeatedPhaseIsNoStep) {
  RotarySwitchTracker tracker;
  tracker.Update(0x04);
  EXPECT_EQ(tracker.Update(0x04), 0);
}

TEST(RotarySwitchTrackerTest, IgnoresBounceAndUpperBits) {
  RotarySwitchTracker tracker;
  tracker.Update(0x01);
  EXPECT_EQ(tracker.Update(0x00), 0);  // Between contacts.
  EXPECT_EQ(tracker.Update(0x03), 0);  // Two contacts at once.
  // The upper bits carry the lower selector's position.
  EXPECT_EQ(tracker.Update(0x08 | 0x02), 1);
}

}  // namespace
}  // namespace can_bridge::protocol
