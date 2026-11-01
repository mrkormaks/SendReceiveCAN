#ifndef CAN_BRIDGE_COMMON_STR_CAT_H_
#define CAN_BRIDGE_COMMON_STR_CAT_H_

#include <sstream>
#include <string>

namespace can_bridge {

// Concatenates the textual representation of all arguments.
// Note: uint8_t values are printed as characters by iostreams; cast them to
// int before passing them in.
template <typename... Args>
std::string StrCat(const Args&... args) {
  std::ostringstream stream;
  (stream << ... << args);
  return stream.str();
}

}  // namespace can_bridge

#endif  // CAN_BRIDGE_COMMON_STR_CAT_H_
