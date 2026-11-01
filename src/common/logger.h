#ifndef CAN_BRIDGE_COMMON_LOGGER_H_
#define CAN_BRIDGE_COMMON_LOGGER_H_

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "common/str_cat.h"

namespace can_bridge {

enum class LogLevel { kDebug = 0, kInfo = 1, kWarning = 2, kError = 3 };

// Parses "debug", "info", "warning" or "error" (case-insensitive).
std::optional<LogLevel> ParseLogLevel(std::string_view text);

// Thread-safe logging interface with level filtering.
class Logger {
 public:
  explicit Logger(LogLevel min_level = LogLevel::kInfo)
      : min_level_(min_level) {}
  virtual ~Logger() = default;

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void set_min_level(LogLevel level) { min_level_.store(level); }
  bool IsEnabled(LogLevel level) const { return level >= min_level_.load(); }

  template <typename... Args>
  void Log(LogLevel level, const Args&... args) {
    if (IsEnabled(level)) Write(level, StrCat(args...));
  }
  template <typename... Args>
  void Debug(const Args&... args) {
    Log(LogLevel::kDebug, args...);
  }
  template <typename... Args>
  void Info(const Args&... args) {
    Log(LogLevel::kInfo, args...);
  }
  template <typename... Args>
  void Warning(const Args&... args) {
    Log(LogLevel::kWarning, args...);
  }
  template <typename... Args>
  void Error(const Args&... args) {
    Log(LogLevel::kError, args...);
  }

 protected:
  // Called only for enabled levels. Implementations must be thread-safe.
  virtual void Write(LogLevel level, const std::string& message) = 0;

 private:
  std::atomic<LogLevel> min_level_;
};

// Writes "YYYY-MM-DD hh:mm:ss.mmm L message" lines to a stream.
class StreamLogger : public Logger {
 public:
  StreamLogger(std::ostream* stream, LogLevel min_level)
      : Logger(min_level), stream_(stream) {}

 protected:
  void Write(LogLevel level, const std::string& message) override;

 private:
  std::mutex mutex_;
  std::ostream* stream_;
};

// Limits how often a repeating event is reported. Not thread-safe: each
// instance is meant to be owned by a single thread.
class RateLimiter {
 public:
  explicit RateLimiter(std::chrono::steady_clock::duration interval)
      : interval_(interval) {}

  // Returns true if the event may be reported now. When it returns true,
  // `*suppressed` receives the number of events swallowed since the previous
  // report.
  bool Allow(uint64_t* suppressed);

 private:
  std::chrono::steady_clock::duration interval_;
  std::optional<std::chrono::steady_clock::time_point> last_report_;
  uint64_t suppressed_ = 0;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_COMMON_LOGGER_H_
