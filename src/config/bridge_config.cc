#include "config/bridge_config.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <optional>
#include <set>

#include "common/str_cat.h"

namespace can_bridge {
namespace {

constexpr std::array<int, 9> kSupportedBitrates = {10,  20,  50,  100, 125,
                                                   250, 500, 800, 1000};

// Reads typed values and remembers which keys were consumed.
class ConfigReader {
 public:
  explicit ConfigReader(const IniFile& ini) : ini_(ini) {}

  Status ReadInt(const char* section, const char* key, int min_value,
                 int max_value, int* value) {
    const std::optional<std::string> text = Lookup(section, key);
    if (!text.has_value()) return Status::Ok();
    int parsed = 0;
    const char* begin = text->data();
    const char* end = begin + text->size();
    const auto [ptr, error] = std::from_chars(begin, end, parsed);
    if (error != std::errc() || ptr != end) {
      return Status::Error(
          StrCat("[", section, "] ", key, ": '", *text, "' is not a number"));
    }
    if (parsed < min_value || parsed > max_value) {
      return Status::Error(StrCat("[", section, "] ", key, ": ", parsed,
                                  " is outside ", min_value, "..", max_value));
    }
    *value = parsed;
    return Status::Ok();
  }

  Status ReadString(const char* section, const char* key, std::string* value) {
    const std::optional<std::string> text = Lookup(section, key);
    if (!text.has_value()) return Status::Ok();
    if (text->empty()) {
      return Status::Error(StrCat("[", section, "] ", key, ": empty value"));
    }
    *value = *text;
    return Status::Ok();
  }

  std::vector<std::string> UnknownKeys() const {
    std::vector<std::string> unknown;
    for (const std::string& key : ini_.Keys()) {
      if (known_.count(key) == 0) unknown.push_back(key);
    }
    return unknown;
  }

 private:
  std::optional<std::string> Lookup(const char* section, const char* key) {
    std::string name = StrCat(section, ".", key);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    known_.insert(name);
    return ini_.Get(section, key);
  }

  const IniFile& ini_;
  std::set<std::string> known_;
};

Status ReadPort(ConfigReader* reader, const char* section, PortConfig* port) {
  CAN_BRIDGE_RETURN_IF_ERROR(
      reader->ReadString(section, "device", &port->device));
  CAN_BRIDGE_RETURN_IF_ERROR(
      reader->ReadInt(section, "bitrate_kbps", 1, 1000, &port->bitrate_kbps));
  if (std::find(kSupportedBitrates.begin(), kSupportedBitrates.end(),
                port->bitrate_kbps) == kSupportedBitrates.end()) {
    return Status::Error(StrCat("[", section,
                                "] bitrate_kbps: ", port->bitrate_kbps,
                                " is not supported by the driver"));
  }
  return reader->ReadInt(section, "fault_flag", 0, 2, &port->fault_flag);
}

}  // namespace

Status ParseBridgeConfig(const IniFile& ini, BridgeConfig* config,
                         std::vector<std::string>* warnings) {
  ConfigReader reader(ini);
  CAN_BRIDGE_RETURN_IF_ERROR(
      ReadPort(&reader, "panel_port", &config->panel_port));
  CAN_BRIDGE_RETURN_IF_ERROR(
      ReadPort(&reader, "overhead_port", &config->overhead_port));

  CAN_BRIDGE_RETURN_IF_ERROR(reader.ReadInt("timing", "read_timeout_ms", 1,
                                            1000, &config->read_timeout_ms));
  CAN_BRIDGE_RETURN_IF_ERROR(reader.ReadInt("timing", "write_timeout_ms", 1,
                                            1000, &config->write_timeout_ms));
  CAN_BRIDGE_RETURN_IF_ERROR(reader.ReadInt("timing", "publish_period_ms", 5,
                                            1000, &config->publish_period_ms));
  CAN_BRIDGE_RETURN_IF_ERROR(reader.ReadInt("timing", "output_period_ms", 10,
                                            10000, &config->output_period_ms));
  CAN_BRIDGE_RETURN_IF_ERROR(
      reader.ReadInt("timing", "output_refresh_period_ms", 0, 3600000,
                     &config->output_refresh_period_ms));

  CAN_BRIDGE_RETURN_IF_ERROR(reader.ReadInt("cockpit", "kontur_initial_percent",
                                            0, 100,
                                            &config->kontur_initial_percent));
  CAN_BRIDGE_RETURN_IF_ERROR(reader.ReadInt("cockpit", "compass_byte_base", 255,
                                            256, &config->compass_byte_base));

  CAN_BRIDGE_RETURN_IF_ERROR(reader.ReadString("simulator", "avn_shared_memory",
                                               &config->avn_shared_memory));
  CAN_BRIDGE_RETURN_IF_ERROR(reader.ReadString("simulator", "sut_shared_memory",
                                               &config->sut_shared_memory));

  CAN_BRIDGE_RETURN_IF_ERROR(reader.ReadInt("log", "stats_period_s", 0, 86400,
                                            &config->stats_period_s));

  std::string hdg_knob;
  CAN_BRIDGE_RETURN_IF_ERROR(
      reader.ReadString("cockpit", "hdg_knob", &hdg_knob));
  if (hdg_knob == "left_mfi") {
    config->hdg_knob_mode = protocol::HdgKnobMode::kLeftMfiOuterKnob;
  } else if (hdg_knob == "isrp") {
    config->hdg_knob_mode = protocol::HdgKnobMode::kIsrpHdg;
  } else if (!hdg_knob.empty()) {
    return Status::Error(StrCat("[cockpit] hdg_knob: '", hdg_knob,
                                "' must be 'left_mfi' or 'isrp'"));
  }

  std::string log_level;
  CAN_BRIDGE_RETURN_IF_ERROR(reader.ReadString("log", "level", &log_level));
  if (!log_level.empty()) {
    const std::optional<LogLevel> level = ParseLogLevel(log_level);
    if (!level.has_value()) {
      return Status::Error(StrCat("[log] level: '", log_level,
                                  "' must be debug, info, warning or error"));
    }
    config->log_level = *level;
  }

  if (config->panel_port.device == config->overhead_port.device) {
    return Status::Error("panel_port and overhead_port use the same device");
  }
  if (config->panel_port.fault_flag != 0 &&
      config->panel_port.fault_flag == config->overhead_port.fault_flag) {
    return Status::Error(
        "panel_port and overhead_port use the same fault_flag");
  }

  for (const std::string& key : reader.UnknownKeys()) {
    warnings->push_back(StrCat("unknown setting '", key, "' ignored"));
  }
  return Status::Ok();
}

}  // namespace can_bridge
