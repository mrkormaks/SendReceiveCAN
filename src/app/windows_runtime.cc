#include "app/windows_runtime.h"

#include <windows.h>
// <timeapi.h> needs <windows.h> first.
#include <timeapi.h>

#include <vector>

namespace can_bridge {
namespace {

std::atomic<bool> stop_requested_flag{false};
HANDLE shutdown_complete_event = nullptr;

// Upper bound for the cleanup Windows allows after a close event (about 5 s).
constexpr DWORD kShutdownWaitMs = 3000;

BOOL WINAPI HandleConsoleEvent(DWORD event) {
  switch (event) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      stop_requested_flag.store(true);
      return TRUE;
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      stop_requested_flag.store(true);
      if (shutdown_complete_event != nullptr) {
        WaitForSingleObject(shutdown_complete_event, kShutdownWaitMs);
      }
      return TRUE;
    default:
      return FALSE;
  }
}

}  // namespace

std::filesystem::path ExecutableDirectory() {
  std::vector<wchar_t> buffer(MAX_PATH);
  while (true) {
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                            static_cast<DWORD>(buffer.size()));
    if (length == 0) return std::filesystem::current_path();
    if (length < buffer.size()) {
      return std::filesystem::path(std::wstring(buffer.data(), length))
          .parent_path();
    }
    buffer.resize(buffer.size() * 2);
  }
}

SingleInstanceLock::SingleInstanceLock(const std::string& name) {
  mutex_ = CreateMutexA(nullptr, TRUE, name.c_str());
  acquired_ = mutex_ != nullptr && GetLastError() != ERROR_ALREADY_EXISTS;
}

SingleInstanceLock::~SingleInstanceLock() {
  if (mutex_ == nullptr) return;
  if (acquired_) ReleaseMutex(mutex_);
  CloseHandle(mutex_);
}

ConsoleStopHandler::ConsoleStopHandler() {
  shutdown_complete_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  SetConsoleCtrlHandler(HandleConsoleEvent, TRUE);
}

ConsoleStopHandler::~ConsoleStopHandler() {
  NotifyShutdownComplete();
  SetConsoleCtrlHandler(HandleConsoleEvent, FALSE);
}

const std::atomic<bool>& ConsoleStopHandler::stop_requested() const {
  return stop_requested_flag;
}

void ConsoleStopHandler::NotifyShutdownComplete() {
  if (shutdown_complete_event != nullptr) SetEvent(shutdown_complete_event);
}

TimerResolutionScope::TimerResolutionScope(unsigned milliseconds)
    : milliseconds_(milliseconds),
      active_(timeBeginPeriod(milliseconds) == TIMERR_NOERROR) {}

TimerResolutionScope::~TimerResolutionScope() {
  if (active_) timeEndPeriod(milliseconds_);
}

void RaiseProcessPriority() {
  SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
}

}  // namespace can_bridge
