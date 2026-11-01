#include "app/bridge.h"

#include <algorithm>
#include <thread>
#include <utility>

namespace can_bridge {
namespace {

using Clock = std::chrono::steady_clock;

// Input events waiting for publication; far above any human input rate.
constexpr size_t kMaxPendingInputEvents = 4096;
// A port must be failing this long before the simulator is told about it,
// so short reconnects do not flicker the fault indication.
constexpr std::chrono::seconds kFaultDebounce{1};

// Advances a periodic deadline. After a stall the missed cycles are skipped
// instead of being executed in a burst.
Clock::time_point NextDeadline(Clock::time_point deadline,
                               Clock::duration period, Clock::time_point now) {
  deadline += period;
  if (deadline <= now) deadline = now + period;
  return deadline;
}

void LogChannelStats(Logger* logger, const CanChannel& channel) {
  const ChannelStats stats = channel.stats();
  logger->Info(
      channel.name(), ": rx=", stats.rx_frames, " tx=", stats.tx_frames,
      " tx_dropped=", stats.tx_dropped, " tx_timeouts=", stats.tx_timeouts,
      " error_frames=", stats.error_frames, " ignored=", stats.ignored_frames,
      " io_errors=", stats.io_errors, " connects=", stats.connects);
}

}  // namespace

Bridge::Bridge(const BridgeConfig& config, Simulator* simulator,
               std::unique_ptr<CanPort> panel_port,
               std::unique_ptr<CanPort> overhead_port, Logger* logger)
    : config_(config),
      simulator_(simulator),
      logger_(logger),
      scheduler_(config.kontur_initial_percent, kMaxPendingInputEvents),
      panel_decoder_(protocol::PanelDecoderOptions{config.hdg_knob_mode}),
      output_encoder_(protocol::OutputEncoderOptions{config.compass_byte_base}),
      panel_channel_(std::move(panel_port), CanChannelOptions{}, logger),
      overhead_channel_(std::move(overhead_port), CanChannelOptions{}, logger) {
  panel_fault_.flag = config.panel_port.fault_flag;
  overhead_fault_.flag = config.overhead_port.fault_flag;

  panel_channel_.set_frame_handler([this](const CanFrame& frame) {
    protocol::DecodeResult result = panel_decoder_.Decode(frame);
    if (result.ack.has_value()) panel_channel_.Send(*result.ack);
    scheduler_.Enqueue(std::move(result.event));
  });
  // The indicators may have restarted together with the controller.
  panel_channel_.set_connect_handler(
      [this] { output_encoder_.RequestFullRefresh(); });

  overhead_channel_.set_frame_handler([this](const CanFrame& frame) {
    protocol::DecodeResult result = overhead_decoder_.Decode(frame);
    if (result.ack.has_value()) overhead_channel_.Send(*result.ack);
    scheduler_.Enqueue(std::move(result.event));
  });
  // Switches may have moved while the bus was down: ask for all rows.
  overhead_channel_.set_connect_handler([this] {
    for (const CanFrame& request :
         protocol::OverheadDecoder::MakePollRequests()) {
      overhead_channel_.Send(request);
    }
  });
}

Bridge::~Bridge() { Stop(); }

void Bridge::Start() {
  panel_channel_.Start();
  overhead_channel_.Start();
}

void Bridge::Stop() {
  panel_channel_.Stop();
  overhead_channel_.Stop();
}

void Bridge::Run(const std::atomic<bool>& stop_requested) {
  const auto publish_period =
      std::chrono::milliseconds(config_.publish_period_ms);
  const auto output_period =
      std::chrono::milliseconds(config_.output_period_ms);
  const auto refresh_period =
      std::chrono::milliseconds(config_.output_refresh_period_ms);
  const auto stats_period = std::chrono::seconds(config_.stats_period_s);

  Start();
  Clock::time_point now = Clock::now();
  Clock::time_point next_publish = now;
  Clock::time_point next_output = now;
  Clock::time_point next_refresh = now + refresh_period;
  Clock::time_point next_stats = now + stats_period;

  while (!stop_requested.load()) {
    if (simulator_->IsCloseRequested()) {
      logger_->Info("The simulator requested shutdown");
      break;
    }
    now = Clock::now();
    if (now >= next_publish) {
      PublishCycle(now);
      next_publish = NextDeadline(next_publish, publish_period, now);
    }
    if (now >= next_output) {
      OutputCycle();
      next_output = NextDeadline(next_output, output_period, now);
    }
    if (config_.output_refresh_period_ms > 0 && now >= next_refresh) {
      output_encoder_.RequestFullRefresh();
      next_refresh = now + refresh_period;
    }
    if (config_.stats_period_s > 0 && now >= next_stats) {
      LogStats();
      next_stats = now + stats_period;
    }
    std::this_thread::sleep_until(std::min(next_publish, next_output));
  }
  Stop();
}

void Bridge::PublishCycle(Clock::time_point now) {
  simulator_->Publish(scheduler_.NextBatch());
  ReportHealth(panel_channel_, &panel_fault_, now);
  ReportHealth(overhead_channel_, &overhead_fault_, now);

  const uint64_t dropped = scheduler_.dropped_events();
  if (dropped != reported_dropped_events_) {
    logger_->Warning("Input queue overflow: ",
                     dropped - reported_dropped_events_, " events dropped");
    reported_dropped_events_ = dropped;
  }
}

void Bridge::OutputCycle() {
  for (const CanFrame& frame :
       output_encoder_.Update(simulator_->ReadIndicators())) {
    panel_channel_.Send(frame);
  }
}

void Bridge::ReportHealth(const CanChannel& channel, FaultReport* report,
                          Clock::time_point now) {
  if (report->flag == 0) return;

  if (channel.healthy()) {
    report->unhealthy_since.reset();
    if (!report->reported.has_value() || *report->reported) {
      if (report->reported.has_value()) {
        logger_->Info(channel.name(), ": port recovered, fault flag cleared");
      }
      simulator_->SetDeviceFault(report->flag, false);
      report->reported = false;
    }
    return;
  }

  if (!report->unhealthy_since.has_value()) report->unhealthy_since = now;
  const bool already_reported =
      report->reported.has_value() && *report->reported;
  if (!already_reported && now - *report->unhealthy_since >= kFaultDebounce) {
    logger_->Error(channel.name(), ": port is down, setting fault flag DevOtk",
                   report->flag);
    simulator_->SetDeviceFault(report->flag, true);
    report->reported = true;
  }
}

void Bridge::LogStats() {
  LogChannelStats(logger_, panel_channel_);
  LogChannelStats(logger_, overhead_channel_);
  logger_->Info("inputs: pending=", scheduler_.pending_events(),
                " dropped=", scheduler_.dropped_events());
}

}  // namespace can_bridge
