#ifndef CAN_BRIDGE_CONFIG_BRIDGE_CONFIG_H_
#define CAN_BRIDGE_CONFIG_BRIDGE_CONFIG_H_

#include <string>
#include <vector>

#include "common/logger.h"
#include "common/status.h"
#include "config/ini_file.h"
#include "protocol/panel_decoder.h"

namespace can_bridge {

struct PortConfig {
  std::string device;  // Driver device name, e.g. "can1".
  int bitrate_kbps = 500;
  // Which SUT fault flag reports this port: 1 = DevOtk1, 2 = DevOtk2,
  // 0 = do not report.
  int fault_flag = 0;
};

// All settings with their defaults. The defaults reproduce the legacy
// program, so the bridge works without a configuration file.
struct BridgeConfig {
  PortConfig panel_port = {"can1", 500, 2};
  PortConfig overhead_port = {"can2", 500, 1};

  int read_timeout_ms = 10;
  int write_timeout_ms = 20;
  int publish_period_ms = 20;
  int output_period_ms = 100;
  // Periodic resend of all indicator frames; 0 disables it.
  int output_refresh_period_ms = 0;

  protocol::HdgKnobMode hdg_knob_mode =
      protocol::HdgKnobMode::kLeftMfiOuterKnob;
  int kontur_initial_percent = 75;
  int compass_byte_base = 255;

  std::string avn_shared_memory = "STRUCT_AVN_410";
  std::string sut_shared_memory = "Struct_SUT";

  LogLevel log_level = LogLevel::kInfo;
  // Period of statistics lines in the log; 0 disables them.
  int stats_period_s = 0;
};

// Reads settings present in `ini` on top of the defaults in `*config`.
// Unknown keys are reported in `*warnings`; invalid values are errors.
Status ParseBridgeConfig(const IniFile& ini, BridgeConfig* config,
                         std::vector<std::string>* warnings);

}  // namespace can_bridge

#endif  // CAN_BRIDGE_CONFIG_BRIDGE_CONFIG_H_
