#include "app/can_channel.h"

#include <gtest/gtest.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "test_support.h"

namespace can_bridge {
namespace {

using testing::FakeCanPort;
using testing::MemoryLogger;
using testing::WaitUntil;

class CanChannelTest : public ::testing::Test {
 protected:
  CanChannelTest() {
    auto port = std::make_unique<FakeCanPort>("can_test");
    port_ = port.get();
    CanChannelOptions options;
    options.reconnect_delay_min = std::chrono::milliseconds(1);
    options.reconnect_delay_max = std::chrono::milliseconds(5);
    channel_ = std::make_unique<CanChannel>(std::move(port), options, &logger_);
    channel_->set_frame_handler([this](const CanFrame& frame) {
      std::lock_guard<std::mutex> lock(mutex_);
      received_.push_back(frame);
    });
    channel_->set_connect_handler([this] { connects_.fetch_add(1); });
  }

  ~CanChannelTest() override { channel_->Stop(); }

  size_t received_count() {
    std::lock_guard<std::mutex> lock(mutex_);
    return received_.size();
  }

  MemoryLogger logger_;
  FakeCanPort* port_ = nullptr;
  std::unique_ptr<CanChannel> channel_;
  std::mutex mutex_;
  std::vector<CanFrame> received_;
  std::atomic<int> connects_{0};
};

TEST_F(CanChannelTest, DeliversDataFramesAndSkipsOthers) {
  channel_->Start();
  ASSERT_TRUE(WaitUntil([&] { return channel_->healthy(); }));

  CanFrame remote = MakeCanFrame(1, {1});
  remote.flags = kCanFlagRemote;
  port_->Inject(MakeCanFrame(1, {42, 1}));
  port_->Inject(remote);
  port_->Inject(MakeCanFrame(2, {}));
  port_->Inject(MakeCanFrame(3, {63, 0}));

  ASSERT_TRUE(WaitUntil([&] { return channel_->stats().ignored_frames == 2; }));
  ASSERT_TRUE(WaitUntil([&] { return received_count() == 2; }));
  std::lock_guard<std::mutex> lock(mutex_);
  EXPECT_EQ(received_[0], MakeCanFrame(1, {42, 1}));
  EXPECT_EQ(received_[1], MakeCanFrame(3, {63, 0}));
  EXPECT_EQ(channel_->stats().rx_frames, 2u);
}

TEST_F(CanChannelTest, TransmitsQueuedFramesInOrder) {
  channel_->Start();
  ASSERT_TRUE(WaitUntil([&] { return connects_.load() == 1; }));
  for (uint8_t i = 0; i < 40; ++i) channel_->Send(MakeCanFrame(i, {i}));
  ASSERT_TRUE(WaitUntil([&] { return port_->written().size() == 40; }));
  const std::vector<CanFrame> written = port_->written();
  for (uint8_t i = 0; i < 40; ++i) EXPECT_EQ(written[i].id, i);
  EXPECT_EQ(channel_->stats().tx_frames, 40u);
}

TEST_F(CanChannelTest, RetriesOpeningUntilTheDeviceAppears) {
  port_->FailOpens(3);
  channel_->Start();
  ASSERT_TRUE(WaitUntil([&] { return connects_.load() == 1; }));
  EXPECT_TRUE(channel_->healthy());
  EXPECT_EQ(port_->open_count(), 1);
  EXPECT_TRUE(logger_.Contains("cannot open port"));
}

TEST_F(CanChannelTest, ReopensAfterReadError) {
  channel_->Start();
  ASSERT_TRUE(WaitUntil([&] { return connects_.load() == 1; }));
  port_->FailNextRead();
  ASSERT_TRUE(WaitUntil([&] { return connects_.load() == 2; }));
  EXPECT_TRUE(channel_->healthy());
  EXPECT_EQ(channel_->stats().io_errors, 1u);

  port_->Inject(MakeCanFrame(1, {42, 1}));
  EXPECT_TRUE(WaitUntil([&] { return received_count() == 1; }));
}

TEST_F(CanChannelTest, ReopensAfterBusOff) {
  channel_->Start();
  ASSERT_TRUE(WaitUntil([&] { return connects_.load() == 1; }));
  CanFrame bus_off;
  bus_off.id = 0xFFFFFFFF;
  bus_off.flags = kCanFlagError | kCanFlagBusOff;
  port_->Inject(bus_off);
  ASSERT_TRUE(WaitUntil([&] { return connects_.load() == 2; }));
  EXPECT_EQ(channel_->stats().error_frames, 1u);
  EXPECT_TRUE(logger_.Contains("bus-off"));
}

TEST_F(CanChannelTest, RetriesFramesAfterWriteTimeouts) {
  port_->SetWriteTimeouts(true);
  channel_->Start();
  ASSERT_TRUE(WaitUntil([&] { return connects_.load() == 1; }));
  channel_->Send(MakeCanFrame(7, {1}));
  ASSERT_TRUE(WaitUntil([&] { return port_->write_attempts() >= 3; }));
  EXPECT_TRUE(port_->written().empty());
  EXPECT_TRUE(channel_->healthy());  // A timeout is not a port failure.

  port_->SetWriteTimeouts(false);
  ASSERT_TRUE(WaitUntil([&] { return port_->written().size() == 1; }));
  EXPECT_EQ(port_->written()[0], MakeCanFrame(7, {1}));
  EXPECT_GE(channel_->stats().tx_timeouts, 3u);
}

TEST_F(CanChannelTest, DiscardsFramesQueuedWhileDisconnected) {
  port_->FailOpens(2);
  channel_->set_connect_handler([this] {
    connects_.fetch_add(1);
    channel_->Send(MakeCanFrame(2, {2}));  // What a fresh connection needs.
  });
  channel_->Send(MakeCanFrame(1, {1}));  // Stale once the port opens.
  channel_->Start();

  ASSERT_TRUE(WaitUntil([&] { return !port_->written().empty(); }));
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  EXPECT_EQ(port_->written(), (std::vector<CanFrame>{MakeCanFrame(2, {2})}));
}

TEST_F(CanChannelTest, StopIsIdempotentAndClosesThePort) {
  channel_->Start();
  ASSERT_TRUE(WaitUntil([&] { return channel_->healthy(); }));
  channel_->Stop();
  channel_->Stop();
  EXPECT_FALSE(port_->is_open());
  EXPECT_FALSE(channel_->healthy());
}

}  // namespace
}  // namespace can_bridge
