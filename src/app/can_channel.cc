#include "app/can_channel.h"

#include <algorithm>
#include <utility>

namespace can_bridge {
namespace {

// A connection that survived this long is considered stable again, so the
// next failure starts with the shortest reconnect delay.
constexpr std::chrono::seconds kStableConnection{10};
// An empty read should block for the driver's read timeout. Reads that
// return sooner are followed by a short pause.
constexpr std::chrono::milliseconds kMinIdleRead{1};

std::string SuppressedSuffix(uint64_t suppressed) {
  if (suppressed == 0) return "";
  return StrCat(" (", suppressed, " similar messages suppressed)");
}

}  // namespace

CanChannel::CanChannel(std::unique_ptr<CanPort> port, CanChannelOptions options,
                       Logger* logger)
    : port_(std::move(port)),
      options_(options),
      logger_(logger),
      tx_queue_(options.tx_queue_capacity),
      rx_buffer_(std::max<size_t>(1, options.read_batch_size)),
      reconnect_delay_(options.reconnect_delay_min) {}

CanChannel::~CanChannel() { Stop(); }

void CanChannel::Start() {
  if (thread_.joinable()) return;
  {
    std::lock_guard<std::mutex> lock(stop_mutex_);
    stop_requested_ = false;
  }
  stopping_.store(false);
  thread_ = std::thread(&CanChannel::Run, this);
}

void CanChannel::Stop() {
  {
    std::lock_guard<std::mutex> lock(stop_mutex_);
    stop_requested_ = true;
  }
  stopping_.store(true);
  stop_cv_.notify_all();
  if (thread_.joinable()) thread_.join();
}

bool CanChannel::Send(const CanFrame& frame) { return tx_queue_.Push(frame); }

ChannelStats CanChannel::stats() const {
  ChannelStats stats;
  stats.rx_frames = rx_frames_.load();
  stats.tx_frames = tx_frames_.load();
  stats.tx_dropped = tx_queue_.dropped_count();
  stats.tx_timeouts = tx_timeouts_.load();
  stats.error_frames = error_frames_.load();
  stats.ignored_frames = ignored_frames_.load();
  stats.io_errors = io_errors_.load();
  stats.connects = connects_.load();
  return stats;
}

void CanChannel::Run() {
  while (!stopping_.load()) {
    if (!port_->is_open() && !Connect()) {
      if (!WaitUnlessStopped(reconnect_delay_)) break;
      reconnect_delay_ =
          std::min(reconnect_delay_ * 2, options_.reconnect_delay_max);
      continue;
    }
    Receive();
    if (port_->is_open()) Transmit();
  }
  port_->Close();
  healthy_.store(false);
}

bool CanChannel::Connect() {
  const Status status = port_->Open();
  if (!status.ok()) {
    healthy_.store(false);
    uint64_t suppressed = 0;
    if (error_log_limiter_.Allow(&suppressed)) {
      logger_->Error(name(), ": cannot open port: ", status.message(),
                     SuppressedSuffix(suppressed));
    }
    return false;
  }
  connected_at_ = std::chrono::steady_clock::now();
  connects_.fetch_add(1);
  healthy_.store(true);
  // Frames queued while the port was down are stale (old indicator values,
  // acknowledgements the controller no longer waits for). The connect
  // handler queues whatever the fresh connection needs.
  const size_t stale = tx_queue_.Clear();
  logger_->Info(name(), ": port opened",
                stale > 0 ? StrCat(", ", stale, " stale frames discarded")
                          : std::string());
  if (connect_handler_) connect_handler_();
  return true;
}

void CanChannel::Fail(const std::string& reason) {
  io_errors_.fetch_add(1);
  healthy_.store(false);
  uint64_t suppressed = 0;
  if (error_log_limiter_.Allow(&suppressed)) {
    logger_->Error(name(), ": ", reason, "; reopening the port",
                   SuppressedSuffix(suppressed));
  }
  port_->Close();
  pending_tx_.clear();

  if (std::chrono::steady_clock::now() - connected_at_ > kStableConnection) {
    reconnect_delay_ = options_.reconnect_delay_min;
  }
  // Back off before reopening, so a port that opens but fails immediately
  // does not spin.
  WaitUnlessStopped(reconnect_delay_);
  reconnect_delay_ =
      std::min(reconnect_delay_ * 2, options_.reconnect_delay_max);
}

void CanChannel::Receive() {
  size_t count = 0;
  const auto started = std::chrono::steady_clock::now();
  const IoResult result = port_->Read(rx_buffer_, &count);
  if (result == IoResult::kError) {
    Fail(StrCat("read failed: ", port_->last_error()));
    return;
  }
  if (result == IoResult::kTimeout &&
      std::chrono::steady_clock::now() - started < kMinIdleRead) {
    // The driver returned without waiting for its read timeout; pace the
    // loop so that an idle bus does not burn a CPU core.
    WaitUnlessStopped(kMinIdleRead);
    return;
  }
  count = std::min(count, rx_buffer_.size());
  for (size_t i = 0; i < count; ++i) {
    if (!HandleFrame(rx_buffer_[i])) {
      Fail("controller went bus-off");
      return;
    }
  }
}

bool CanChannel::HandleFrame(const CanFrame& frame) {
  if (frame.is_error()) {
    error_frames_.fetch_add(1);
    if ((frame.flags & kCanFlagBusOff) != 0) return false;
    uint64_t suppressed = 0;
    if ((frame.flags & (kCanFlagControllerOverrun | kCanFlagRxBufferOverrun)) !=
            0 &&
        overrun_log_limiter_.Allow(&suppressed)) {
      logger_->Warning(name(), ": receive overrun, frames were lost",
                       SuppressedSuffix(suppressed));
    } else if ((frame.flags & kCanFlagErrorPassive) != 0 &&
               overrun_log_limiter_.Allow(&suppressed)) {
      logger_->Warning(name(), ": controller is error-passive (bus problems)",
                       SuppressedSuffix(suppressed));
    }
    return true;
  }
  if (frame.is_remote() || frame.length == 0) {
    ignored_frames_.fetch_add(1);
    return true;
  }
  rx_frames_.fetch_add(1);
  if (frame_handler_) frame_handler_(frame);
  return true;
}

void CanChannel::Transmit() {
  if (pending_tx_.empty()) {
    tx_queue_.PopBatch(options_.write_batch_size, &pending_tx_);
  }
  if (pending_tx_.empty()) return;

  size_t written = 0;
  const IoResult result = port_->Write(pending_tx_, &written);
  written = std::min(written, pending_tx_.size());
  tx_frames_.fetch_add(written);
  pending_tx_.erase(pending_tx_.begin(),
                    pending_tx_.begin() + static_cast<std::ptrdiff_t>(written));

  if (result == IoResult::kError) {
    Fail(StrCat("write failed: ", port_->last_error()));
  } else if (result == IoResult::kTimeout) {
    // Typically nobody acknowledges on the bus (cable, power). The unsent
    // frames stay at the head of the queue and are retried.
    tx_timeouts_.fetch_add(1);
    uint64_t suppressed = 0;
    if (tx_timeout_log_limiter_.Allow(&suppressed)) {
      logger_->Warning(name(), ": transmit timed out, retrying",
                       SuppressedSuffix(suppressed));
    }
  }
}

bool CanChannel::WaitUnlessStopped(std::chrono::milliseconds duration) {
  std::unique_lock<std::mutex> lock(stop_mutex_);
  return !stop_cv_.wait_for(lock, duration, [this] { return stop_requested_; });
}

}  // namespace can_bridge
