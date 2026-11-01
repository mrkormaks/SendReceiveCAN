#include "config/ini_file.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include "common/str_cat.h"

namespace can_bridge {
namespace {

std::string_view Trim(std::string_view text) {
  const auto is_space = [](char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
  };
  while (!text.empty() && is_space(text.front())) text.remove_prefix(1);
  while (!text.empty() && is_space(text.back())) text.remove_suffix(1);
  return text;
}

std::string ToLower(std::string_view text) {
  std::string result(text);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

std::string MakeKey(std::string_view section, std::string_view key) {
  return ToLower(section) + "." + ToLower(key);
}

}  // namespace

Status IniFile::Parse(std::string_view text, IniFile* ini) {
  ini->values_.clear();
  // Skip a UTF-8 byte order mark written by some editors.
  if (text.substr(0, 3) == "\xEF\xBB\xBF") text.remove_prefix(3);

  std::string section;
  int line_number = 0;
  while (!text.empty()) {
    ++line_number;
    const size_t end = text.find('\n');
    std::string_view line =
        Trim(text.substr(0, end == std::string_view::npos ? text.size() : end));
    text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);

    if (line.empty() || line.front() == ';' || line.front() == '#') continue;

    if (line.front() == '[') {
      if (line.back() != ']') {
        return Status::Error(
            StrCat("line ", line_number, ": unterminated section header"));
      }
      section = std::string(Trim(line.substr(1, line.size() - 2)));
      if (section.empty()) {
        return Status::Error(StrCat("line ", line_number, ": empty section"));
      }
      continue;
    }

    const size_t equals = line.find('=');
    if (equals == std::string_view::npos) {
      return Status::Error(
          StrCat("line ", line_number, ": expected 'key = value'"));
    }
    const std::string_view key = Trim(line.substr(0, equals));
    if (key.empty()) {
      return Status::Error(StrCat("line ", line_number, ": empty key"));
    }
    if (section.empty()) {
      return Status::Error(
          StrCat("line ", line_number, ": key outside of a section"));
    }
    ini->values_[MakeKey(section, key)] =
        std::string(Trim(line.substr(equals + 1)));
  }
  return Status::Ok();
}

Status IniFile::Load(const std::filesystem::path& path, IniFile* ini) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return Status::Error(StrCat("cannot open ", path.string()));
  }
  std::ostringstream content;
  content << file.rdbuf();
  const Status status = Parse(content.str(), ini);
  if (!status.ok()) {
    return Status::Error(StrCat(path.string(), ": ", status.message()));
  }
  return Status::Ok();
}

std::optional<std::string> IniFile::Get(std::string_view section,
                                        std::string_view key) const {
  const auto it = values_.find(MakeKey(section, key));
  if (it == values_.end()) return std::nullopt;
  return it->second;
}

std::vector<std::string> IniFile::Keys() const {
  std::vector<std::string> keys;
  keys.reserve(values_.size());
  for (const auto& [key, value] : values_) keys.push_back(key);
  return keys;
}

}  // namespace can_bridge
