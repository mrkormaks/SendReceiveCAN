#ifndef CAN_BRIDGE_CAN_ADVANTECH_ADVANTECH_CAN_PORT_H_
#define CAN_BRIDGE_CAN_ADVANTECH_ADVANTECH_CAN_PORT_H_

#include <memory>
#include <string>

#include "can/can_port.h"

namespace can_bridge {

struct AdvantechCanPortOptions {
  std::string device;  // e.g. "can1".
  int bitrate_kbps = 500;
  int read_timeout_ms = 10;
  int write_timeout_ms = 20;
};

// CanPort on top of the Advantech CAN board driver (Windows only).
//
// Reads, writes and control requests use separate OVERLAPPED structures and
// events, and every wait is bounded: if the driver ever ignores its own
// timeouts, the request is cancelled instead of blocking the bridge.
class AdvantechCanPort : public CanPort {
 public:
  explicit AdvantechCanPort(AdvantechCanPortOptions options);
  ~AdvantechCanPort() override;

  AdvantechCanPort(const AdvantechCanPort&) = delete;
  AdvantechCanPort& operator=(const AdvantechCanPort&) = delete;

  const std::string& name() const override { return options_.device; }
  Status Open() override;
  void Close() override;
  bool is_open() const override;
  IoResult Read(std::span<CanFrame> frames, size_t* count) override;
  IoResult Write(std::span<const CanFrame> frames, size_t* written) override;
  std::string last_error() const override { return last_error_; }

 private:
  struct Impl;  // Keeps <windows.h> out of this header.

  const AdvantechCanPortOptions options_;
  std::unique_ptr<Impl> impl_;
  std::string last_error_;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_CAN_ADVANTECH_ADVANTECH_CAN_PORT_H_
