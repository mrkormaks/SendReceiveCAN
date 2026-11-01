#ifndef CAN_BRIDGE_COMMON_STATUS_H_
#define CAN_BRIDGE_COMMON_STATUS_H_

#include <string>
#include <utility>

namespace can_bridge {

// Result of a fallible operation. The project does not use exceptions
// (Google C++ style), so functions that can fail return a Status.
class Status {
 public:
  // Constructs an OK status.
  Status() = default;

  static Status Ok() { return {}; }
  static Status Error(std::string message) {
    return Status(std::move(message));
  }

  bool ok() const { return ok_; }
  const std::string& message() const { return message_; }

 private:
  explicit Status(std::string message)
      : ok_(false), message_(std::move(message)) {}

  bool ok_ = true;
  std::string message_;
};

}  // namespace can_bridge

// Returns the status from the enclosing function if it is not OK.
#define CAN_BRIDGE_RETURN_IF_ERROR(expr)                     \
  do {                                                       \
    ::can_bridge::Status can_bridge_status_ = (expr);        \
    if (!can_bridge_status_.ok()) return can_bridge_status_; \
  } while (false)

#endif  // CAN_BRIDGE_COMMON_STATUS_H_
