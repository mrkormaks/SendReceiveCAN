#include "app/bridge.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <thread>

#include "test_support.h"

namespace can_bridge {
namespace {

using testing::FakeCanPort;
using testing::FakeSimulator;
using testing::MemoryLogger;
using testing::WaitUntil;
using Clock = std::chrono::steady_clock;

bool Contains(const std::vector<CanFrame>& frames, const CanFrame& frame) {
  return std::find(frames.begin(), frames.end(), frame) != frames.end();
}

class BridgeTest : public ::testing::Test {
 protected:
  void CreateBridge(const BridgeConfig& config = BridgeConfig{}) {
    auto panel = std::make_unique<FakeCanPort>("panel");
    auto overhead = std::make_unique<FakeCanPort>("overhead");
    panel_ = panel.get();
    overhead_ = overhead.get();
    bridge_ = std::make_unique<Bridge>(config, &simulator_, std::move(panel),
                                       std::move(overhead), &logger_);
  }

  // Publishes until `condition` holds for the latest batch.
  bool PublishUntil(const std::function<bool(const PublishBatch&)>& condition) {
    return WaitUntil([&] {
      bridge_->PublishCycle(Clock::now());
      return condition(simulator_.batches.back());
    });
  }

  MemoryLogger logger_;
  FakeSimulator simulator_;
  FakeCanPort* panel_ = nullptr;
  FakeCanPort* overhead_ = nullptr;
  std::unique_ptr<Bridge> bridge_;
};

TEST_F(BridgeTest, PanelKeyReachesSimulatorAndIsAcknowledged) {
  CreateBridge();
  bridge_->Start();
  panel_->Inject(MakeCanFrame(0x42, {66, 7}));  // Left MFPU, slot 20.

  EXPECT_TRUE(PublishUntil([](const PublishBatch& batch) {
    return batch.levels.mfpu_buttons[kSideLeft][20];
  }));
  EXPECT_TRUE(WaitUntil(
      [&] { return Contains(panel_->written(), MakeCanFrame(166, {66, 7})); }));
}

TEST_F(BridgeTest, OverheadIsPolledAndSwitchesArePublished) {
  CreateBridge();
  bridge_->Start();
  ASSERT_TRUE(WaitUntil([&] {
    return Contains(overhead_->written(), MakeCanFrame(127, {111}));
  }));

  overhead_->Inject(MakeCanFrame(0x47, {71, 0x00, 0x01}));
  bool switched_on = false;
  EXPECT_TRUE(PublishUntil([&](const PublishBatch& batch) {
    for (const FieldWrite& write : batch.direct_writes) {
      if (write.key.kind == FieldKind::kOverheadSwitch &&
          write.key.index == 0 && write.value == 1) {
        switched_on = true;
      }
    }
    return switched_on;
  }));
  EXPECT_TRUE(WaitUntil([&] {
    return Contains(overhead_->written(), MakeCanFrame(171, {71, 0x00, 0x01}));
  }));
}

TEST_F(BridgeTest, IndicatorsAreSentOnChange) {
  CreateBridge();
  bridge_->Start();
  simulator_.indicators.compass_heading_deg = 100.0;
  // Frames queued before the port connects are discarded and the connection
  // requests a full refresh, so keep cycling until the frame shows up.
  ASSERT_TRUE(WaitUntil([&] {
    bridge_->OutputCycle();
    return Contains(panel_->written(), MakeCanFrame(80, {80, 0, 100}));
  }));
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  const size_t before = panel_->written().size();

  simulator_.indicators.compass_heading_deg = 101.0;
  bridge_->OutputCycle();
  bridge_->OutputCycle();
  ASSERT_TRUE(WaitUntil([&] { return panel_->written().size() > before; }));
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  // Exactly one new frame: the compass.
  const std::vector<CanFrame> written = panel_->written();
  ASSERT_EQ(written.size(), before + 1);
  EXPECT_EQ(written.back(), MakeCanFrame(80, {80, 0, 101}));
}

TEST_F(BridgeTest, ReportsPortHealthThroughFaultFlags) {
  CreateBridge();
  panel_->FailOpens(1000000);
  bridge_->Start();
  ASSERT_TRUE(WaitUntil([&] { return overhead_->open_count() == 1; }));

  const Clock::time_point start = Clock::now();
  bridge_->PublishCycle(start);
  // Healthy overhead port: its flag (DevOtk1) is cleared once.
  EXPECT_EQ(simulator_.faults, (std::vector<std::pair<int, bool>>{{1, false}}));

  // The panel port has been failing for less than the debounce time.
  bridge_->PublishCycle(start + std::chrono::milliseconds(500));
  EXPECT_EQ(simulator_.faults.size(), 1u);

  bridge_->PublishCycle(start + std::chrono::seconds(2));
  EXPECT_EQ(simulator_.faults,
            (std::vector<std::pair<int, bool>>{{1, false}, {2, true}}));
  bridge_->PublishCycle(start + std::chrono::seconds(3));
  EXPECT_EQ(simulator_.faults.size(), 2u);  // Reported once.
}

TEST_F(BridgeTest, RunStopsWhenTheSimulatorAsksToClose) {
  BridgeConfig config;
  config.publish_period_ms = 5;
  config.output_period_ms = 10;
  CreateBridge(config);
  std::atomic<bool> stop{false};
  std::thread runner([&] { bridge_->Run(stop); });

  ASSERT_TRUE(WaitUntil([&] { return panel_->written().size() >= 4; }));
  simulator_.close_requested.store(true);
  runner.join();
  EXPECT_TRUE(logger_.Contains("requested shutdown"));
  EXPECT_FALSE(panel_->is_open());
}

TEST_F(BridgeTest, RunStopsOnStopRequest) {
  CreateBridge();
  std::atomic<bool> stop{false};
  std::thread runner([&] { bridge_->Run(stop); });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  stop.store(true);
  runner.join();
  EXPECT_FALSE(overhead_->is_open());
}

}  // namespace
}  // namespace can_bridge
