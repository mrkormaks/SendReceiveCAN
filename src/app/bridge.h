#ifndef CAN_BRIDGE_APP_BRIDGE_H_
#define CAN_BRIDGE_APP_BRIDGE_H_

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include "app/can_channel.h"
#include "can/can_port.h"
#include "cockpit/input_scheduler.h"
#include "common/logger.h"
#include "config/bridge_config.h"
#include "protocol/output_encoder.h"
#include "protocol/overhead_decoder.h"
#include "protocol/panel_decoder.h"
#include "sim/simulator.h"

namespace can_bridge {

// Connects the two CAN buses with the simulator.
//
//   panel bus    --> PanelDecoder    --+
//                                      +--> InputScheduler --> Simulator
//   overhead bus --> OverheadDecoder --+
//   simulator --> OutputEncoder --> panel bus
//
// Each bus runs on its own CanChannel thread. The thread calling Run()
// publishes inputs every publish period, sends indicator updates every output
// period and reports port health through the simulator's fault flags.
class Bridge {
 public:
  Bridge(const BridgeConfig& config, Simulator* simulator,
         std::unique_ptr<CanPort> panel_port,
         std::unique_ptr<CanPort> overhead_port, Logger* logger);
  ~Bridge();

  Bridge(const Bridge&) = delete;
  Bridge& operator=(const Bridge&) = delete;

  // Runs until `stop_requested` becomes true or the simulator asks to close.
  void Run(const std::atomic<bool>& stop_requested);

  // Building blocks of Run(); public so tests can drive them step by step.
  void Start();
  void Stop();
  void PublishCycle(std::chrono::steady_clock::time_point now);
  void OutputCycle();

 private:
  // State of one SUT fault flag.
  struct FaultReport {
    int flag = 0;
    std::optional<bool> reported;
    std::optional<std::chrono::steady_clock::time_point> unhealthy_since;
  };

  void ReportHealth(const CanChannel& channel, FaultReport* report,
                    std::chrono::steady_clock::time_point now);
  void LogStats();

  const BridgeConfig config_;
  Simulator* const simulator_;
  Logger* const logger_;

  InputScheduler scheduler_;
  protocol::PanelDecoder panel_decoder_;
  protocol::OverheadDecoder overhead_decoder_;
  protocol::OutputEncoder output_encoder_;
  // Declared last: the channel threads use the members above.
  CanChannel panel_channel_;
  CanChannel overhead_channel_;

  FaultReport panel_fault_;
  FaultReport overhead_fault_;
  uint64_t reported_dropped_events_ = 0;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_APP_BRIDGE_H_
