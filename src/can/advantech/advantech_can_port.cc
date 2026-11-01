#include "can/advantech/advantech_can_port.h"

#include <windows.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "can/advantech/advantech_abi.h"
#include "common/str_cat.h"

namespace can_bridge {
namespace {

namespace abi = advantech_abi;

// Control requests complete immediately in a healthy driver.
constexpr DWORD kControlTimeoutMs = 1000;
// Time granted on top of the driver's own I/O timeouts before a request is
// considered stuck and cancelled.
constexpr DWORD kWatchdogMarginMs = 1000;

std::string Win32ErrorText(DWORD error) {
  char* buffer = nullptr;
  const DWORD length = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error, 0, reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
  std::string text =
      length > 0 ? std::string(buffer, length) : std::string("unknown error");
  if (buffer != nullptr) LocalFree(buffer);
  while (!text.empty() && (text.back() == '\n' || text.back() == '\r' ||
                           text.back() == ' ' || text.back() == '.')) {
    text.pop_back();
  }
  return StrCat(text, " (error ", error, ")");
}

bool IsTimeoutError(DWORD error) {
  return error == ERROR_OPERATION_ABORTED || error == ERROR_TIMEOUT ||
         error == ERROR_SEM_TIMEOUT || error == WAIT_TIMEOUT;
}

std::wstring DevicePath(const std::string& device) {
  std::wstring wide(device.size(), L'\0');
  const int length = MultiByteToWideChar(
      CP_UTF8, 0, device.data(), static_cast<int>(device.size()), wide.data(),
      static_cast<int>(wide.size()));
  wide.resize(static_cast<size_t>(std::max(length, 0)));
  return L"\\\\.\\" + wide;
}

// OVERLAPPED structure with its own manual-reset event. Every kind of request
// has a separate slot, so requests never share an OVERLAPPED.
class AsyncSlot {
 public:
  AsyncSlot() : event_(CreateEventW(nullptr, TRUE, FALSE, nullptr)) {}
  ~AsyncSlot() {
    if (event_ != nullptr) CloseHandle(event_);
  }
  AsyncSlot(const AsyncSlot&) = delete;
  AsyncSlot& operator=(const AsyncSlot&) = delete;

  bool valid() const { return event_ != nullptr; }
  HANDLE event() const { return event_; }
  OVERLAPPED* overlapped() { return &overlapped_; }

  // Resets the structure for a new request.
  OVERLAPPED* Prepare() {
    overlapped_ = OVERLAPPED{};
    overlapped_.hEvent = event_;
    ResetEvent(event_);
    return &overlapped_;
  }

 private:
  HANDLE event_;
  OVERLAPPED overlapped_{};
};

enum class Completion { kDone, kTimedOut, kFailed };

// Waits for a request started on `slot`. `started` is the return value of the
// call that started it. The request is cancelled if it takes longer than
// `timeout_ms`; in every case it has finished when this function returns, so
// its buffers may be reused.
Completion Await(HANDLE device, BOOL started, AsyncSlot* slot, DWORD timeout_ms,
                 DWORD* transferred, DWORD* error) {
  *transferred = 0;
  *error = ERROR_SUCCESS;
  if (!started) {
    const DWORD start_error = GetLastError();
    if (start_error != ERROR_IO_PENDING) {
      *error = start_error;
      return IsTimeoutError(start_error) ? Completion::kTimedOut
                                         : Completion::kFailed;
    }
    if (WaitForSingleObject(slot->event(), timeout_ms) != WAIT_OBJECT_0) {
      CancelIoEx(device, slot->overlapped());
    }
  }
  if (!GetOverlappedResult(device, slot->overlapped(), transferred, TRUE)) {
    *error = GetLastError();
    return IsTimeoutError(*error) ? Completion::kTimedOut : Completion::kFailed;
  }
  return Completion::kDone;
}

abi::Request MakeCommand(abi::Command command) {
  abi::Request request{};
  request.command = static_cast<int32_t>(command);
  return request;
}

abi::Request MakeConfig(abi::ConfigTarget target, uint32_t value1,
                        uint32_t value2) {
  abi::Request request{};
  request.target = static_cast<int32_t>(target);
  request.value1 = value1;
  request.value2 = value2;
  return request;
}

CanFrame ToCanFrame(const abi::Frame& source) {
  CanFrame frame;
  frame.id = source.id;
  const int length = std::clamp<int>(source.length, 0, 8);
  frame.length = static_cast<uint8_t>(length);
  std::copy_n(source.data, length, frame.data.begin());
  if ((source.flags & abi::kFlagRemote) != 0) frame.flags |= kCanFlagRemote;
  if ((source.flags & abi::kFlagExtended) != 0) {
    frame.flags |= kCanFlagExtended;
  }
  if (source.id == abi::kErrorFrameId) {
    frame.flags |= kCanFlagError;
    if ((source.flags & abi::kFlagErrorPassive) != 0) {
      frame.flags |= kCanFlagErrorPassive;
    }
    if ((source.flags & abi::kFlagBusOff) != 0) frame.flags |= kCanFlagBusOff;
    if ((source.flags & abi::kFlagControllerOverrun) != 0) {
      frame.flags |= kCanFlagControllerOverrun;
    }
    if ((source.flags & abi::kFlagRxBufferOverrun) != 0) {
      frame.flags |= kCanFlagRxBufferOverrun;
    }
  }
  return frame;
}

abi::Frame ToDriverFrame(const CanFrame& source) {
  abi::Frame frame{};
  frame.id = source.id;
  const uint8_t length = std::min<uint8_t>(source.length, 8);
  frame.length = length;
  std::copy_n(source.data.begin(), length, frame.data);
  if (source.is_remote()) frame.flags |= abi::kFlagRemote;
  if ((source.flags & kCanFlagExtended) != 0) frame.flags |= abi::kFlagExtended;
  return frame;
}

}  // namespace

struct AdvantechCanPort::Impl {
  HANDLE device = INVALID_HANDLE_VALUE;
  AsyncSlot read_slot;
  AsyncSlot write_slot;
  AsyncSlot control_slot;
  std::vector<abi::Frame> read_buffer;
  std::vector<abi::Frame> write_buffer;

  Status Control(uint32_t code, abi::Request request) {
    control_slot.Prepare();
    const BOOL started =
        DeviceIoControl(device, code, &request, sizeof(request), nullptr, 0,
                        nullptr, control_slot.overlapped());
    DWORD transferred = 0;
    DWORD error = ERROR_SUCCESS;
    switch (Await(device, started, &control_slot, kControlTimeoutMs,
                  &transferred, &error)) {
      case Completion::kDone:
        return Status::Ok();
      case Completion::kTimedOut:
        return Status::Error("the driver did not answer");
      case Completion::kFailed:
        break;
    }
    return Status::Error(Win32ErrorText(error));
  }
};

AdvantechCanPort::AdvantechCanPort(AdvantechCanPortOptions options)
    : options_(std::move(options)), impl_(std::make_unique<Impl>()) {}

AdvantechCanPort::~AdvantechCanPort() { Close(); }

bool AdvantechCanPort::is_open() const {
  return impl_->device != INVALID_HANDLE_VALUE;
}

Status AdvantechCanPort::Open() {
  Close();
  if (!impl_->read_slot.valid() || !impl_->write_slot.valid() ||
      !impl_->control_slot.valid()) {
    last_error_ = "cannot create I/O events";
    return Status::Error(last_error_);
  }

  const HANDLE device =
      CreateFileW(DevicePath(options_.device).c_str(),
                  GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
  if (device == INVALID_HANDLE_VALUE) {
    last_error_ = Win32ErrorText(GetLastError());
    return Status::Error(StrCat("cannot open device: ", last_error_));
  }
  impl_->device = device;

  struct Step {
    const char* description;
    uint32_t code;
    abi::Request request;
  };
  const Step steps[] = {
      {"enter reset mode", abi::kControlCommand,
       MakeCommand(abi::Command::kStop)},
      {"clear buffers", abi::kControlCommand,
       MakeCommand(abi::Command::kClearBuffers)},
      {"set bit rate", abi::kControlConfigure,
       MakeConfig(abi::ConfigTarget::kBitRate,
                  static_cast<uint32_t>(options_.bitrate_kbps), 0)},
      // Accept every identifier: frames are dispatched by data[0].
      {"set acceptance mask", abi::kControlConfigure,
       MakeConfig(abi::ConfigTarget::kAcceptanceMask, 0xFFFFFFFF, 0)},
      {"set acceptance code", abi::kControlConfigure,
       MakeConfig(abi::ConfigTarget::kAcceptanceCode, 0xFFFFFFFF, 0)},
      {"set timeouts", abi::kControlConfigure,
       MakeConfig(abi::ConfigTarget::kTimeouts,
                  static_cast<uint32_t>(options_.write_timeout_ms),
                  static_cast<uint32_t>(options_.read_timeout_ms))},
      {"start controller", abi::kControlCommand,
       MakeCommand(abi::Command::kStart)},
  };
  for (const Step& step : steps) {
    const Status status = impl_->Control(step.code, step.request);
    if (!status.ok()) {
      Close();
      last_error_ = StrCat(step.description, ": ", status.message());
      return Status::Error(last_error_);
    }
  }
  last_error_.clear();
  return Status::Ok();
}

void AdvantechCanPort::Close() {
  if (impl_->device == INVALID_HANDLE_VALUE) return;
  CancelIoEx(impl_->device, nullptr);
  CloseHandle(impl_->device);
  impl_->device = INVALID_HANDLE_VALUE;
}

IoResult AdvantechCanPort::Read(std::span<CanFrame> frames, size_t* count) {
  *count = 0;
  if (!is_open()) {
    last_error_ = "port is not open";
    return IoResult::kError;
  }
  if (frames.empty()) return IoResult::kTimeout;

  std::vector<abi::Frame>& buffer = impl_->read_buffer;
  buffer.resize(frames.size());
  impl_->read_slot.Prepare();
  const BOOL started =
      ReadFile(impl_->device, buffer.data(), static_cast<DWORD>(buffer.size()),
               nullptr, impl_->read_slot.overlapped());
  DWORD received = 0;
  DWORD error = ERROR_SUCCESS;
  const Completion completion =
      Await(impl_->device, started, &impl_->read_slot,
            static_cast<DWORD>(options_.read_timeout_ms) + kWatchdogMarginMs,
            &received, &error);
  if (completion == Completion::kFailed) {
    last_error_ = StrCat("read: ", Win32ErrorText(error));
    return IoResult::kError;
  }

  received = std::min<DWORD>(received, static_cast<DWORD>(buffer.size()));
  for (DWORD i = 0; i < received; ++i) frames[i] = ToCanFrame(buffer[i]);
  *count = received;
  return received > 0 ? IoResult::kOk : IoResult::kTimeout;
}

IoResult AdvantechCanPort::Write(std::span<const CanFrame> frames,
                                 size_t* written) {
  *written = 0;
  if (!is_open()) {
    last_error_ = "port is not open";
    return IoResult::kError;
  }
  if (frames.empty()) return IoResult::kOk;

  std::vector<abi::Frame>& buffer = impl_->write_buffer;
  buffer.resize(frames.size());
  std::transform(frames.begin(), frames.end(), buffer.begin(), ToDriverFrame);
  impl_->write_slot.Prepare();
  const BOOL started =
      WriteFile(impl_->device, buffer.data(), static_cast<DWORD>(buffer.size()),
                nullptr, impl_->write_slot.overlapped());
  DWORD sent = 0;
  DWORD error = ERROR_SUCCESS;
  const DWORD timeout_ms = static_cast<DWORD>(options_.write_timeout_ms) *
                               static_cast<DWORD>(buffer.size()) +
                           kWatchdogMarginMs;
  const Completion completion = Await(
      impl_->device, started, &impl_->write_slot, timeout_ms, &sent, &error);
  if (completion == Completion::kFailed) {
    last_error_ = StrCat("write: ", Win32ErrorText(error));
    return IoResult::kError;
  }

  *written = std::min<size_t>(sent, buffer.size());
  return *written == buffer.size() ? IoResult::kOk : IoResult::kTimeout;
}

}  // namespace can_bridge
