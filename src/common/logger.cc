#include "common/logger.h"

#include <cctype>
#include <ctime>
#include <iomanip>

namespace can_bridge {
namespace {

char LevelLetter(LogLevel level) {
  switch (level) {
    case LogLevel::kDebug:
      return 'D';
    case LogLevel::kInfo:
      return 'I';
    case LogLevel::kWarning:
      return 'W';
    case LogLevel::kError:
      return 'E';
  }
  return '?';
}

std::tm ToLocalTime(std::time_t time) {
  std::tm result{};
#ifdef _WIN32
  localtime_s(&result, &time);
#else
  localtime_r(&time, &result);
#endif
  return result;
}

bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(b[i]))) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::optional<LogLevel> ParseLogLevel(std::string_view text) {
  if (EqualsIgnoreCase(text, "debug")) return LogLevel::kDebug;
  if (EqualsIgnoreCase(text, "info")) return LogLevel::kInfo;
  if (EqualsIgnoreCase(text, "warning")) return LogLevel::kWarning;
  if (EqualsIgnoreCase(text, "error")) return LogLevel::kError;
  return std::nullopt;
}

void StreamLogger::Write(LogLevel level, const std::string& message) {
  const auto now = std::chrono::system_clock::now();
  const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch()) %
                      1000;
  const std::tm local = ToLocalTime(std::chrono::system_clock::to_time_t(now));

  std::lock_guard<std::mutex> lock(mutex_);
  *stream_ << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3)
           << std::setfill('0') << millis.count() << ' ' << LevelLetter(level)
           << ' ' << message << '\n';
  stream_->flush();
}

bool RateLimiter::Allow(uint64_t* suppressed) {
  const auto now = std::chrono::steady_clock::now();
  if (last_report_.has_value() && now - *last_report_ < interval_) {
    ++suppressed_;
    return false;
  }
  last_report_ = now;
  *suppressed = suppressed_;
  suppressed_ = 0;
  return true;
}

}  // namespace can_bridge
