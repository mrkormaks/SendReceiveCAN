#ifndef CAN_BRIDGE_APP_CAN_CHANNEL_H_
#define CAN_BRIDGE_APP_CAN_CHANNEL_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "can/can_frame.h"
#include "can/can_port.h"
#include "common/bounded_queue.h"
#include "common/logger.h"

namespace can_bridge {

struct CanChannelOptions {
  // Frames requested from the driver per read call.
  size_t read_batch_size = 64;
  // Frames handed to the driver per write call.
  size_t write_batch_size = 16;
  size_t tx_queue_capacity = 1024;
  std::chrono::milliseconds reconnect_delay_min{250};
  std::chrono::milliseconds reconnect_delay_max{5000};
};

struct ChannelStats {
  uint64_t rx_frames = 0;
  uint64_t tx_frames = 0;
  uint64_t tx_dropped = 0;   // Discarded because the queue overflowed.
  uint64_t tx_timeouts = 0;  // Write calls that did not send everything.
  uint64_t error_frames = 0;
  uint64_t ignored_frames = 0;  // Remote requests and empty frames.
  uint64_t io_errors = 0;
  uint64_t connects = 0;
};

// Owns one CanPort and drives it from a dedicated thread.
//
// The thread reads frames in batches, hands every data frame to the frame
// handler, transmits queued frames and recovers from failures by reopening the
// port with exponential back-off. Because only this thread touches the port,
// reads and writes never race on driver state.
class CanChannel {
 public:
  // Called on the channel thread for every received data frame.
  using FrameHandler = std::function<void(const CanFrame&)>;
  // Called on the channel thread after every successful (re)connect.
  using ConnectHandler = std::function<void()>;

  CanChannel(std::unique_ptr<CanPort> port, CanChannelOptions options,
             Logger* logger);
  ~CanChannel();

  CanChannel(const CanChannel&) = delete;
  CanChannel& operator=(const CanChannel&) = delete;

  // Handlers must be set before Start().
  void set_frame_handler(FrameHandler handler) {
    frame_handler_ = std::move(handler);
  }
  void set_connect_handler(ConnectHandler handler) {
    connect_handler_ = std::move(handler);
  }

  void Start();
  // Stops the thread and closes the port. Safe to call more than once.
  void Stop();

  // Queues a frame for transmission. Thread-safe and non-blocking. Returns
  // false if an older queued frame had to be dropped to make room. Frames
  // still queued when the port (re)connects are discarded as stale.
  bool Send(const CanFrame& frame);

  // True while the port is open and working.
  bool healthy() const { return healthy_.load(); }
  const std::string& name() const { return port_->name(); }
  ChannelStats stats() const;

 private:
  void Run();
  bool Connect();
  // Closes the port after a failure and backs off; Run() then reconnects.
  void Fail(const std::string& reason);
  void Receive();
  // Returns false if the frame indicates that the port must be reset.
  bool HandleFrame(const CanFrame& frame);
  void Transmit();
  // Sleeps unless Stop() is called. Returns false if stopping.
  bool WaitUnlessStopped(std::chrono::milliseconds duration);

  std::unique_ptr<CanPort> port_;
  const CanChannelOptions options_;
  Logger* const logger_;
  FrameHandler frame_handler_;
  ConnectHandler connect_handler_;

  BoundedQueue<CanFrame> tx_queue_;
  // Accessed by the channel thread only.
  std::vector<CanFrame> rx_buffer_;
  std::vector<CanFrame> pending_tx_;
  std::chrono::milliseconds reconnect_delay_;
  std::chrono::steady_clock::time_point connected_at_;
  RateLimiter error_log_limiter_{std::chrono::seconds(1)};
  RateLimiter tx_timeout_log_limiter_{std::chrono::seconds(5)};
  RateLimiter overrun_log_limiter_{std::chrono::seconds(5)};

  std::thread thread_;
  std::mutex stop_mutex_;
  std::condition_variable stop_cv_;
  bool stop_requested_ = false;  // Guarded by stop_mutex_.
  std::atomic<bool> stopping_{false};
  std::atomic<bool> healthy_{false};

  std::atomic<uint64_t> rx_frames_{0};
  std::atomic<uint64_t> tx_frames_{0};
  std::atomic<uint64_t> tx_timeouts_{0};
  std::atomic<uint64_t> error_frames_{0};
  std::atomic<uint64_t> ignored_frames_{0};
  std::atomic<uint64_t> io_errors_{0};
  std::atomic<uint64_t> connects_{0};
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_APP_CAN_CHANNEL_H_
