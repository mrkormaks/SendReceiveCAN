#ifndef CAN_BRIDGE_CAN_CAN_PORT_H_
#define CAN_BRIDGE_CAN_CAN_PORT_H_

#include <cstddef>
#include <span>
#include <string>

#include "can/can_frame.h"
#include "common/status.h"

namespace can_bridge {

enum class IoResult {
  kOk,       // At least one frame was transferred (all frames for Write).
  kTimeout,  // Nothing (Read) or not everything (Write) was transferred.
  kError,    // The port is unusable and must be reopened.
};

// One CAN controller channel. Implementations are not thread-safe: a port is
// owned and driven by a single thread (see CanChannel).
class CanPort {
 public:
  virtual ~CanPort() = default;

  // Name used in logs, e.g. "can1".
  virtual const std::string& name() const = 0;

  // Opens and configures the channel and starts the controller. May be
  // called again after Close() to recover from errors.
  virtual Status Open() = 0;
  virtual void Close() = 0;
  virtual bool is_open() const = 0;

  // Waits up to the configured read timeout for incoming frames and stores
  // them at the beginning of `frames`. `*count` receives the number of frames
  // read; kOk implies `*count > 0`.
  virtual IoResult Read(std::span<CanFrame> frames, size_t* count) = 0;

  // Transmits `frames` in order. `*written` receives the number of frames that
  // were sent; kTimeout means that the tail was not sent and may be retried.
  virtual IoResult Write(std::span<const CanFrame> frames, size_t* written) = 0;

  // Description of the most recent failure, for diagnostics.
  virtual std::string last_error() const = 0;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_CAN_CAN_PORT_H_
