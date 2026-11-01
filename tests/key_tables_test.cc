#include "protocol/key_tables.h"

#include <gtest/gtest.h>

#include <set>

namespace can_bridge::protocol {
namespace {

TEST(KeyTablesTest, MfiKeysAreUniqueAndInRange) {
  std::set<int> codes;
  std::set<std::pair<bool, int>> slots;
  for (const MfiKeyMapping& key : MfiKeys()) {
    EXPECT_TRUE(codes.insert(key.code).second) << int{key.code};
    EXPECT_TRUE(slots.insert({key.upper_row, key.slot}).second);
    EXPECT_GE(key.slot, 1);
    EXPECT_LT(key.slot, key.upper_row ? kMfiFkSlots : kMfiMfkSlots);
  }
  EXPECT_EQ(MfiKeys().size(), 40u);
}

TEST(KeyTablesTest, MfiKeysCoverAllSlots) {
  int fk = 0;
  int mfk = 0;
  for (const MfiKeyMapping& key : MfiKeys()) (key.upper_row ? fk : mfk)++;
  EXPECT_EQ(fk, kMfiFkSlots - 1);
  EXPECT_EQ(mfk, kMfiMfkSlots - 1);
}

TEST(KeyTablesTest, MfpuGroupsAreDisjointAndCoverAllSlots) {
  std::set<int> slots;
  for (const KeyMapping& key : MfpuMainKeys()) {
    EXPECT_TRUE(slots.insert(key.slot).second) << int{key.slot};
  }
  for (const KeyMapping& key : MfpuExtraKeys()) {
    EXPECT_TRUE(slots.insert(key.slot).second) << int{key.slot};
  }
  EXPECT_EQ(slots.size(), static_cast<size_t>(kMfpuButtonSlots));
  EXPECT_EQ(*slots.rbegin(), kMfpuButtonSlots - 1);
}

TEST(KeyTablesTest, SpotChecksAgainstLegacyCode) {
  EXPECT_EQ(FindMfpuKeySlot(MfpuMainKeys(), 7), 20);
  EXPECT_EQ(FindMfpuKeySlot(MfpuMainKeys(), 56), 8);
  EXPECT_EQ(FindMfpuKeySlot(MfpuExtraKeys(), 13), 73);
  EXPECT_FALSE(FindMfpuKeySlot(MfpuMainKeys(), 0).has_value());
  EXPECT_FALSE(FindMfpuKeySlot(MfpuExtraKeys(), 19).has_value());

  EXPECT_EQ(FindMfiKey(24), (MfiKeyMapping{24, true, 1}));
  EXPECT_FALSE(FindMfiKey(41).has_value());
  EXPECT_FALSE(FindMfiKey(44).has_value());
}

TEST(KeyTablesTest, B8PanelsDifferOnlyInDigits5And8) {
  for (int code = 0; code < 32; ++code) {
    const auto left = FindB8Key(kSideLeft, static_cast<uint8_t>(code));
    const auto right = FindB8Key(kSideRight, static_cast<uint8_t>(code));
    if (code == 10 || code == 11) continue;
    EXPECT_EQ(left, right) << code;
  }
  EXPECT_EQ(FindB8Key(kSideLeft, 11), B8Button::kDigit5);
  EXPECT_EQ(FindB8Key(kSideLeft, 10), B8Button::kDigit8);
  EXPECT_EQ(FindB8Key(kSideRight, 10), B8Button::kDigit5);
  EXPECT_EQ(FindB8Key(kSideRight, 11), B8Button::kDigit8);
  EXPECT_FALSE(FindB8Key(kSideLeft, 0).has_value());
}

}  // namespace
}  // namespace can_bridge::protocol
