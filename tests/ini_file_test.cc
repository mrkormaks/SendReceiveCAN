#include "config/ini_file.h"

#include <gtest/gtest.h>

namespace can_bridge {
namespace {

TEST(IniFileTest, ParsesSectionsKeysAndComments) {
  IniFile ini;
  ASSERT_TRUE(IniFile::Parse("; comment\n"
                             "[Panel_Port]\n"
                             "  Device = can1  \n"
                             "# another comment\n"
                             "bitrate_kbps=500\n"
                             "\n"
                             "[log]\n"
                             "level = debug\n",
                             &ini)
                  .ok());
  EXPECT_EQ(ini.Get("panel_port", "device"), "can1");
  EXPECT_EQ(ini.Get("PANEL_PORT", "BITRATE_KBPS"), "500");
  EXPECT_EQ(ini.Get("log", "level"), "debug");
  EXPECT_FALSE(ini.Get("log", "missing").has_value());
  EXPECT_EQ(ini.Keys().size(), 3u);
}

TEST(IniFileTest, HandlesWindowsLineEndingsAndBom) {
  IniFile ini;
  ASSERT_TRUE(IniFile::Parse("\xEF\xBB\xBF[a]\r\nkey = value\r\n", &ini).ok());
  EXPECT_EQ(ini.Get("a", "key"), "value");
}

TEST(IniFileTest, EmptyValueIsAllowed) {
  IniFile ini;
  ASSERT_TRUE(IniFile::Parse("[a]\nkey =\n", &ini).ok());
  EXPECT_EQ(ini.Get("a", "key"), "");
}

TEST(IniFileTest, RejectsMalformedLines) {
  IniFile ini;
  EXPECT_FALSE(IniFile::Parse("key = value\n", &ini).ok());
  EXPECT_FALSE(IniFile::Parse("[a\nkey = value\n", &ini).ok());
  EXPECT_FALSE(IniFile::Parse("[]\n", &ini).ok());
  EXPECT_FALSE(IniFile::Parse("[a]\njust text\n", &ini).ok());
  EXPECT_FALSE(IniFile::Parse("[a]\n= value\n", &ini).ok());

  const Status status = IniFile::Parse("[a]\nok = 1\nbroken\n", &ini);
  EXPECT_NE(status.message().find("line 3"), std::string::npos);
}

TEST(IniFileTest, LoadReportsMissingFile) {
  IniFile ini;
  EXPECT_FALSE(IniFile::Load("/nonexistent/can_bridge.ini", &ini).ok());
}

}  // namespace
}  // namespace can_bridge
