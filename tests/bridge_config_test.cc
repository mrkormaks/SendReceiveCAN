#include "config/bridge_config.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace can_bridge {
namespace {

Status ParseText(const std::string& text, BridgeConfig* config,
                 std::vector<std::string>* warnings) {
  IniFile ini;
  Status status = IniFile::Parse(text, &ini);
  if (!status.ok()) return status;
  return ParseBridgeConfig(ini, config, warnings);
}

TEST(BridgeConfigTest, DefaultsMatchLegacyProgram) {
  BridgeConfig config;
  std::vector<std::string> warnings;
  ASSERT_TRUE(ParseText("", &config, &warnings).ok());
  EXPECT_EQ(config.panel_port.device, "can1");
  EXPECT_EQ(config.overhead_port.device, "can2");
  EXPECT_EQ(config.panel_port.bitrate_kbps, 500);
  EXPECT_EQ(config.panel_port.fault_flag, 2);
  EXPECT_EQ(config.overhead_port.fault_flag, 1);
  EXPECT_EQ(config.publish_period_ms, 20);
  EXPECT_EQ(config.output_period_ms, 100);
  EXPECT_EQ(config.kontur_initial_percent, 75);
  EXPECT_EQ(config.compass_byte_base, 255);
  EXPECT_EQ(config.hdg_knob_mode, protocol::HdgKnobMode::kLeftMfiOuterKnob);
  EXPECT_EQ(config.avn_shared_memory, "STRUCT_AVN_410");
  EXPECT_EQ(config.sut_shared_memory, "Struct_SUT");
  EXPECT_TRUE(warnings.empty());
}

TEST(BridgeConfigTest, ReadsAllSettings) {
  BridgeConfig config;
  std::vector<std::string> warnings;
  ASSERT_TRUE(ParseText("[panel_port]\n"
                        "device = can3\n"
                        "bitrate_kbps = 250\n"
                        "fault_flag = 1\n"
                        "[overhead_port]\n"
                        "device = can4\n"
                        "fault_flag = 0\n"
                        "[timing]\n"
                        "publish_period_ms = 10\n"
                        "output_period_ms = 50\n"
                        "output_refresh_period_ms = 1000\n"
                        "read_timeout_ms = 5\n"
                        "write_timeout_ms = 30\n"
                        "[cockpit]\n"
                        "hdg_knob = isrp\n"
                        "kontur_initial_percent = 40\n"
                        "compass_byte_base = 256\n"
                        "[simulator]\n"
                        "avn_shared_memory = AVN\n"
                        "sut_shared_memory = SUT\n"
                        "[log]\n"
                        "level = warning\n"
                        "stats_period_s = 60\n",
                        &config, &warnings)
                  .ok());
  EXPECT_EQ(config.panel_port.device, "can3");
  EXPECT_EQ(config.panel_port.bitrate_kbps, 250);
  EXPECT_EQ(config.panel_port.fault_flag, 1);
  EXPECT_EQ(config.overhead_port.device, "can4");
  EXPECT_EQ(config.overhead_port.fault_flag, 0);
  EXPECT_EQ(config.publish_period_ms, 10);
  EXPECT_EQ(config.output_period_ms, 50);
  EXPECT_EQ(config.output_refresh_period_ms, 1000);
  EXPECT_EQ(config.read_timeout_ms, 5);
  EXPECT_EQ(config.write_timeout_ms, 30);
  EXPECT_EQ(config.hdg_knob_mode, protocol::HdgKnobMode::kIsrpHdg);
  EXPECT_EQ(config.kontur_initial_percent, 40);
  EXPECT_EQ(config.compass_byte_base, 256);
  EXPECT_EQ(config.avn_shared_memory, "AVN");
  EXPECT_EQ(config.sut_shared_memory, "SUT");
  EXPECT_EQ(config.log_level, LogLevel::kWarning);
  EXPECT_EQ(config.stats_period_s, 60);
  EXPECT_TRUE(warnings.empty());
}

TEST(BridgeConfigTest, WarnsAboutUnknownKeys) {
  BridgeConfig config;
  std::vector<std::string> warnings;
  ASSERT_TRUE(
      ParseText("[timing]\npublish_periot_ms = 10\n", &config, &warnings).ok());
  ASSERT_EQ(warnings.size(), 1u);
  EXPECT_NE(warnings[0].find("timing.publish_periot_ms"), std::string::npos);
}

TEST(BridgeConfigTest, RejectsInvalidValues) {
  constexpr const char* kInvalidConfigs[] = {
      "[panel_port]\nbitrate_kbps = 300\n",
      "[panel_port]\nbitrate_kbps = fast\n",
      "[panel_port]\nfault_flag = 3\n",
      "[timing]\npublish_period_ms = 0\n",
      "[timing]\noutput_period_ms = 100ms\n",
      "[cockpit]\nhdg_knob = right_mfi\n",
      "[cockpit]\ncompass_byte_base = 254\n",
      "[cockpit]\nkontur_initial_percent = 101\n",
      "[log]\nlevel = verbose\n",
      "[overhead_port]\ndevice = can1\n",
      "[overhead_port]\nfault_flag = 2\n",
      "[simulator]\navn_shared_memory =\n",
  };
  for (const char* text : kInvalidConfigs) {
    BridgeConfig config;
    std::vector<std::string> warnings;
    EXPECT_FALSE(ParseText(text, &config, &warnings).ok()) << text;
  }
}

TEST(BridgeConfigTest, ExampleFileDocumentsTheDefaults) {
#ifdef CAN_BRIDGE_SOURCE_DIR
  IniFile ini;
  ASSERT_TRUE(
      IniFile::Load(CAN_BRIDGE_SOURCE_DIR "/config/can_bridge.ini", &ini).ok());
  BridgeConfig config;
  std::vector<std::string> warnings;
  ASSERT_TRUE(ParseBridgeConfig(ini, &config, &warnings).ok());
  EXPECT_TRUE(warnings.empty());

  const BridgeConfig defaults;
  EXPECT_EQ(config.panel_port.device, defaults.panel_port.device);
  EXPECT_EQ(config.panel_port.bitrate_kbps, defaults.panel_port.bitrate_kbps);
  EXPECT_EQ(config.panel_port.fault_flag, defaults.panel_port.fault_flag);
  EXPECT_EQ(config.overhead_port.device, defaults.overhead_port.device);
  EXPECT_EQ(config.overhead_port.fault_flag, defaults.overhead_port.fault_flag);
  EXPECT_EQ(config.publish_period_ms, defaults.publish_period_ms);
  EXPECT_EQ(config.output_period_ms, defaults.output_period_ms);
  EXPECT_EQ(config.output_refresh_period_ms, defaults.output_refresh_period_ms);
  EXPECT_EQ(config.read_timeout_ms, defaults.read_timeout_ms);
  EXPECT_EQ(config.write_timeout_ms, defaults.write_timeout_ms);
  EXPECT_EQ(config.hdg_knob_mode, defaults.hdg_knob_mode);
  EXPECT_EQ(config.kontur_initial_percent, defaults.kontur_initial_percent);
  EXPECT_EQ(config.compass_byte_base, defaults.compass_byte_base);
  EXPECT_EQ(config.avn_shared_memory, defaults.avn_shared_memory);
  EXPECT_EQ(config.sut_shared_memory, defaults.sut_shared_memory);
  EXPECT_EQ(config.log_level, defaults.log_level);
  EXPECT_EQ(config.stats_period_s, defaults.stats_period_s);
#else
  GTEST_SKIP() << "source directory is not available (cross build)";
#endif
}

}  // namespace
}  // namespace can_bridge
