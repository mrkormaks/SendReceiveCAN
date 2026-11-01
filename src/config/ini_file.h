#ifndef CAN_BRIDGE_CONFIG_INI_FILE_H_
#define CAN_BRIDGE_CONFIG_INI_FILE_H_

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/status.h"

namespace can_bridge {

// Minimal INI reader: "[section]" headers, "key = value" pairs and full-line
// comments starting with ';' or '#'. Section and key names are
// case-insensitive. Portable replacement for GetPrivateProfileString, which
// silently looks in the Windows directory when given a relative path.
class IniFile {
 public:
  static Status Parse(std::string_view text, IniFile* ini);
  static Status Load(const std::filesystem::path& path, IniFile* ini);

  std::optional<std::string> Get(std::string_view section,
                                 std::string_view key) const;

  // "section.key" names of all entries, for detecting unknown settings.
  std::vector<std::string> Keys() const;

 private:
  // Keys are "section.key", lower-case.
  std::map<std::string, std::string> values_;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_CONFIG_INI_FILE_H_
