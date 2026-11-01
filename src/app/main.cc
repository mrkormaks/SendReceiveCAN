// CAN bridge between the cockpit controllers and the simulator (Windows).

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/bridge.h"
#include "app/windows_runtime.h"
#include "can/advantech/advantech_can_port.h"
#include "common/logger.h"
#include "config/bridge_config.h"
#include "config/ini_file.h"
#include "sim/legacy_simulator.h"
#include "sim/shared_memory_region.h"

#ifndef CAN_BRIDGE_VERSION
#define CAN_BRIDGE_VERSION "dev"
#endif

namespace can_bridge {
namespace {

constexpr char kDefaultConfigName[] = "can_bridge.ini";
constexpr char kInstanceLockName[] = "Local\\SendReceiveCAN.can_bridge";

enum ExitCode {
  kExitOk = 0,
  kExitRuntimeError = 1,
  kExitUsageError = 2,
};

struct CommandLine {
  std::optional<std::filesystem::path> config_path;
  bool show_help = false;
  bool show_version = false;
};

void PrintUsage() {
  std::cout << "Usage: can_bridge [--config <file>] [--version] [--help]\n"
               "\n"
               "Without --config, settings are read from "
            << kDefaultConfigName
            << " next to the executable; if that file does not exist,\n"
               "built-in defaults are used.\n";
}

std::optional<CommandLine> ParseCommandLine(int argc, char** argv) {
  CommandLine command_line;
  for (int i = 1; i < argc; ++i) {
    const std::string_view argument = argv[i];
    if (argument == "--help" || argument == "-h") {
      command_line.show_help = true;
    } else if (argument == "--version") {
      command_line.show_version = true;
    } else if (argument == "--config" && i + 1 < argc) {
      command_line.config_path = std::filesystem::path(argv[++i]);
    } else {
      std::cerr << "Unknown argument: " << argument << "\n";
      return std::nullopt;
    }
  }
  return command_line;
}

// Loads the configuration. A missing default file is fine (defaults are
// used); a missing explicit file or an invalid one is an error.
bool LoadConfig(const CommandLine& command_line, Logger* logger,
                BridgeConfig* config) {
  const std::filesystem::path path = command_line.config_path.value_or(
      ExecutableDirectory() / kDefaultConfigName);
  if (!command_line.config_path.has_value() && !std::filesystem::exists(path)) {
    logger->Warning("No ", path.string(), ", using built-in defaults");
    return true;
  }

  IniFile ini;
  Status status = IniFile::Load(path, &ini);
  std::vector<std::string> warnings;
  if (status.ok()) status = ParseBridgeConfig(ini, config, &warnings);
  if (!status.ok()) {
    logger->Error("Invalid configuration: ", status.message());
    return false;
  }
  for (const std::string& warning : warnings) {
    logger->Warning(path.string(), ": ", warning);
  }
  logger->Info("Configuration: ", path.string());
  return true;
}

std::unique_ptr<CanPort> MakePort(const PortConfig& port,
                                  const BridgeConfig& config) {
  AdvantechCanPortOptions options;
  options.device = port.device;
  options.bitrate_kbps = port.bitrate_kbps;
  options.read_timeout_ms = config.read_timeout_ms;
  options.write_timeout_ms = config.write_timeout_ms;
  return std::make_unique<AdvantechCanPort>(options);
}

// Unused only in stub-header builds, see Run().
[[maybe_unused]] int RunBridge(const CommandLine& command_line) {
  StreamLogger logger(&std::clog, LogLevel::kInfo);
  logger.Info("can_bridge ", CAN_BRIDGE_VERSION, " starting");

  BridgeConfig config;
  if (!LoadConfig(command_line, &logger, &config)) return kExitUsageError;
  logger.set_min_level(config.log_level);

  const SingleInstanceLock instance_lock(kInstanceLockName);
  if (!instance_lock.acquired()) {
    logger.Error("Another can_bridge instance is already running");
    return kExitRuntimeError;
  }

  ConsoleStopHandler stop_handler;
  const TimerResolutionScope timer_resolution(1);
  RaiseProcessPriority();

  SharedMemoryRegion avn_memory;
  SharedMemoryRegion sut_memory;
  Status status =
      avn_memory.OpenOrCreate(config.avn_shared_memory, sizeof(SHAVN_410));
  if (status.ok()) {
    status = sut_memory.OpenOrCreate(config.sut_shared_memory, sizeof(SHSUT));
  }
  if (!status.ok()) {
    logger.Error("Shared memory: ", status.message());
    return kExitRuntimeError;
  }
  LegacySimulator simulator(avn_memory.As<SHAVN_410>(), sut_memory.As<SHSUT>());

  logger.Info("Panel bus: ", config.panel_port.device, " @ ",
              config.panel_port.bitrate_kbps,
              " kbit/s; overhead bus: ", config.overhead_port.device, " @ ",
              config.overhead_port.bitrate_kbps, " kbit/s");
  {
    Bridge bridge(config, &simulator, MakePort(config.panel_port, config),
                  MakePort(config.overhead_port, config), &logger);
    bridge.Run(stop_handler.stop_requested());
  }
  logger.Info("can_bridge stopped");
  stop_handler.NotifyShutdownComplete();
  return kExitOk;
}

int Run(int argc, char** argv) {
  const std::optional<CommandLine> command_line = ParseCommandLine(argc, argv);
  if (!command_line.has_value()) {
    PrintUsage();
    return kExitUsageError;
  }
  if (command_line->show_help) {
    PrintUsage();
    return kExitOk;
  }
  if (command_line->show_version) {
    std::cout << "can_bridge " << CAN_BRIDGE_VERSION << "\n";
    return kExitOk;
  }
#ifdef CAN_BRIDGE_STUB_SIM_HEADERS
  // Built for compile checks only: the stub layout does not match the real
  // simulator, so writing to its shared memory would corrupt it.
  std::cerr << "This executable was built against the stub simulator headers "
               "from tests/. Rebuild with CAN_BRIDGE_SIM_INCLUDE_DIR pointing "
               "at the real simulator headers.\n";
  return kExitRuntimeError;
#else
  return RunBridge(*command_line);
#endif
}

}  // namespace
}  // namespace can_bridge

int main(int argc, char** argv) { return can_bridge::Run(argc, argv); }
