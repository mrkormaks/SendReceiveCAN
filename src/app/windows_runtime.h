#ifndef CAN_BRIDGE_APP_WINDOWS_RUNTIME_H_
#define CAN_BRIDGE_APP_WINDOWS_RUNTIME_H_

#include <atomic>
#include <filesystem>
#include <string>

namespace can_bridge {

// Directory that contains the running executable.
std::filesystem::path ExecutableDirectory();

// Prevents two bridges from fighting over the same CAN ports and shared
// memory. Holds a named mutex for its lifetime.
class SingleInstanceLock {
 public:
  explicit SingleInstanceLock(const std::string& name);
  ~SingleInstanceLock();

  SingleInstanceLock(const SingleInstanceLock&) = delete;
  SingleInstanceLock& operator=(const SingleInstanceLock&) = delete;

  bool acquired() const { return acquired_; }

 private:
  void* mutex_ = nullptr;  // HANDLE.
  bool acquired_ = false;
};

// Turns Ctrl+C, Ctrl+Break and console close/logoff/shutdown into a stop
// request. On close/logoff/shutdown Windows terminates the process once the
// handler returns, so the handler waits (briefly) for NotifyShutdownComplete.
class ConsoleStopHandler {
 public:
  ConsoleStopHandler();
  ~ConsoleStopHandler();

  ConsoleStopHandler(const ConsoleStopHandler&) = delete;
  ConsoleStopHandler& operator=(const ConsoleStopHandler&) = delete;

  const std::atomic<bool>& stop_requested() const;
  void NotifyShutdownComplete();
};

// Raises the system timer resolution so that 20 ms cycles are accurate
// (the default granularity is 15.6 ms).
class TimerResolutionScope {
 public:
  explicit TimerResolutionScope(unsigned milliseconds);
  ~TimerResolutionScope();

  TimerResolutionScope(const TimerResolutionScope&) = delete;
  TimerResolutionScope& operator=(const TimerResolutionScope&) = delete;

 private:
  unsigned milliseconds_;
  bool active_;
};

// Same priority class as the legacy program used.
void RaiseProcessPriority();

}  // namespace can_bridge

#endif  // CAN_BRIDGE_APP_WINDOWS_RUNTIME_H_
