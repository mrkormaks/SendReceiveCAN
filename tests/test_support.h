#ifndef CAN_BRIDGE_TESTS_TEST_SUPPORT_H_
#define CAN_BRIDGE_TESTS_TEST_SUPPORT_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "can/can_port.h"
#include "common/logger.h"
#include "sim/simulator.h"

namespace can_bridge::testing {

// Polls `condition` until it holds or `timeout` expires.
inline bool WaitUntil(
    const std::function<bool()>& condition,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(3000)) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (condition()) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return condition();
}

// Logger that keeps all messages in memory.
class MemoryLogger : public Logger {
 public:
  MemoryLogger() : Logger(LogLevel::kDebug) {}

  std::vector<std::string> messages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_;
  }
  bool Contains(const std::string& text) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const std::string& message : messages_) {
      if (message.find(text) != std::string::npos) return true;
    }
    return false;
  }

 protected:
  void Write(LogLevel /*level*/, const std::string& message) override {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push_back(message);
  }

 private:
  mutable std::mutex mutex_;
  std::vector<std::string> messages_;
};

// In-memory CanPort. Test controls are thread-safe.
class FakeCanPort : public CanPort {
 public:
  explicit FakeCanPort(std::string name) : name_(std::move(name)) {}

  // --- Test controls ---------------------------------------------------------
  void Inject(const CanFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    inbox_.push_back(frame);
    inbox_cv_.notify_all();
  }
  void FailOpens(int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    failing_opens_ = count;
  }
  void FailNextRead() {
    std::lock_guard<std::mutex> lock(mutex_);
    fail_next_read_ = true;
    inbox_cv_.notify_all();
  }
  void SetWriteTimeouts(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    write_timeouts_ = enabled;
  }
  std::vector<CanFrame> written() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return written_;
  }
  int open_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return open_count_;
  }
  int write_attempts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return write_attempts_;
  }

  // --- CanPort ---------------------------------------------------------------
  const std::string& name() const override { return name_; }

  Status Open() override {
    std::lock_guard<std::mutex> lock(mutex_);
    if (failing_opens_ > 0) {
      --failing_opens_;
      return Status::Error("device not found");
    }
    ++open_count_;
    open_ = true;
    return Status::Ok();
  }
  void Close() override {
    std::lock_guard<std::mutex> lock(mutex_);
    open_ = false;
  }
  bool is_open() const override {
    std::lock_guard<std::mutex> lock(mutex_);
    return open_;
  }

  IoResult Read(std::span<CanFrame> frames, size_t* count) override {
    *count = 0;
    std::unique_lock<std::mutex> lock(mutex_);
    inbox_cv_.wait_for(lock, std::chrono::milliseconds(2),
                       [this] { return !inbox_.empty() || fail_next_read_; });
    if (fail_next_read_) {
      fail_next_read_ = false;
      return IoResult::kError;
    }
    while (!inbox_.empty() && *count < frames.size()) {
      frames[(*count)++] = inbox_.front();
      inbox_.pop_front();
    }
    return *count > 0 ? IoResult::kOk : IoResult::kTimeout;
  }

  IoResult Write(std::span<const CanFrame> frames, size_t* written) override {
    std::lock_guard<std::mutex> lock(mutex_);
    ++write_attempts_;
    if (write_timeouts_) {
      *written = 0;
      return IoResult::kTimeout;
    }
    written_.insert(written_.end(), frames.begin(), frames.end());
    *written = frames.size();
    return IoResult::kOk;
  }

  std::string last_error() const override { return "fake failure"; }

 private:
  const std::string name_;
  mutable std::mutex mutex_;
  std::condition_variable inbox_cv_;
  std::deque<CanFrame> inbox_;
  std::vector<CanFrame> written_;
  int failing_opens_ = 0;
  bool fail_next_read_ = false;
  bool write_timeouts_ = false;
  bool open_ = false;
  int open_count_ = 0;
  int write_attempts_ = 0;
};

// Simulator that records everything. Used from the test thread only.
class FakeSimulator : public Simulator {
 public:
  void Publish(const PublishBatch& batch) override { batches.push_back(batch); }
  protocol::IndicatorValues ReadIndicators() const override {
    return indicators;
  }
  void SetDeviceFault(int flag, bool faulty) override {
    faults.emplace_back(flag, faulty);
  }
  bool IsCloseRequested() const override { return close_requested.load(); }

  std::vector<PublishBatch> batches;
  protocol::IndicatorValues indicators;
  std::vector<std::pair<int, bool>> faults;
  std::atomic<bool> close_requested{false};
};

}  // namespace can_bridge::testing

#endif  // CAN_BRIDGE_TESTS_TEST_SUPPORT_H_
