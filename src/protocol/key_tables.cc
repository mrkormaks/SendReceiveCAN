#include "protocol/key_tables.h"

#include <array>

namespace can_bridge::protocol {
namespace {

constexpr std::array<MfiKeyMapping, 40> kMfiKeys = {{
    // Lower row, right to left.
    {1, false, 20},
    {2, false, 19},
    {3, false, 18},
    {4, false, 17},
    {5, false, 16},
    {6, false, 15},
    {7, false, 14},
    {8, false, 13},
    {9, false, 12},
    {10, false, 11},
    {11, false, 10},
    {12, false, 9},
    // Upper row (FK), right to left.
    {13, true, 12},
    {14, true, 11},
    {15, true, 10},
    {16, true, 9},
    {17, true, 8},
    {18, true, 7},
    {19, true, 6},
    {20, true, 5},
    {21, true, 4},
    {22, true, 3},
    {23, true, 2},
    {24, true, 1},
    // Side keys.
    {33, false, 21},
    {34, false, 22},
    {35, false, 23},
    {36, false, 24},
    {37, false, 25},
    {38, false, 26},
    {39, false, 27},
    {40, false, 28},
    {42, false, 7},
    {43, false, 8},
    {45, false, 4},
    {46, false, 5},
    {47, false, 6},
    {48, false, 1},
    {49, false, 2},
    {50, false, 3},
}};

constexpr std::array<KeyMapping, 56> kMfpuMainKeys = {{
    {1, 0},   {2, 1},   {3, 2},   {4, 3},   {5, 4},   {6, 5},   {7, 20},
    {8, 12},  {9, 28},  {10, 38}, {11, 48}, {12, 58}, {13, 68}, {14, 59},
    {15, 49}, {16, 39}, {17, 69}, {18, 60}, {19, 50}, {20, 40}, {21, 29},
    {22, 13}, {23, 21}, {24, 30}, {25, 70}, {26, 61}, {27, 51}, {28, 41},
    {29, 71}, {30, 62}, {31, 52}, {32, 42}, {33, 14}, {34, 22}, {35, 31},
    {36, 32}, {37, 23}, {38, 15}, {39, 33}, {40, 24}, {41, 16}, {42, 67},
    {43, 57}, {44, 17}, {45, 36}, {46, 18}, {47, 37}, {48, 46}, {49, 47},
    {50, 26}, {51, 9},  {52, 10}, {53, 11}, {54, 6},  {55, 7},  {56, 8},
}};

constexpr std::array<KeyMapping, 18> kMfpuExtraKeys = {{
    {1, 72},
    {2, 63},
    {3, 53},
    {4, 43},
    {5, 64},
    {6, 54},
    {7, 44},
    {8, 34},
    {9, 65},
    {10, 55},
    {11, 45},
    {12, 35},
    {13, 73},
    {14, 66},
    {15, 56},
    {16, 25},
    {17, 19},
    {18, 27},
}};

struct B8KeyMapping {
  uint8_t code;
  B8Button button;
};

constexpr std::array<B8KeyMapping, 20> kB8LeftKeys = {{
    {1, B8Button::kTriangle1}, {2, B8Button::kTriangle2},
    {3, B8Button::kTriangle3}, {20, B8Button::kPsh1},
    {19, B8Button::kPsh2},     {18, B8Button::kPsh3},
    {4, B8Button::kApr},       {5, B8Button::kRi},
    {17, B8Button::kReset},    {15, B8Button::kEnter},
    {16, B8Button::kDigit0},   {6, B8Button::kDigit1},
    {9, B8Button::kDigit2},    {12, B8Button::kDigit3},
    {7, B8Button::kDigit4},    {11, B8Button::kDigit5},
    {13, B8Button::kDigit6},   {8, B8Button::kDigit7},
    {10, B8Button::kDigit8},   {14, B8Button::kDigit9},
}};

constexpr std::array<B8KeyMapping, 20> kB8RightKeys = {{
    {1, B8Button::kTriangle1}, {2, B8Button::kTriangle2},
    {3, B8Button::kTriangle3}, {20, B8Button::kPsh1},
    {19, B8Button::kPsh2},     {18, B8Button::kPsh3},
    {4, B8Button::kApr},       {5, B8Button::kRi},
    {17, B8Button::kReset},    {15, B8Button::kEnter},
    {16, B8Button::kDigit0},   {6, B8Button::kDigit1},
    {9, B8Button::kDigit2},    {12, B8Button::kDigit3},
    {7, B8Button::kDigit4},    {10, B8Button::kDigit5},
    {13, B8Button::kDigit6},   {8, B8Button::kDigit7},
    {11, B8Button::kDigit8},   {14, B8Button::kDigit9},
}};

}  // namespace

std::span<const MfiKeyMapping> MfiKeys() { return kMfiKeys; }

std::optional<MfiKeyMapping> FindMfiKey(uint8_t code) {
  for (const MfiKeyMapping& key : kMfiKeys) {
    if (key.code == code) return key;
  }
  return std::nullopt;
}

std::span<const KeyMapping> MfpuMainKeys() { return kMfpuMainKeys; }

std::span<const KeyMapping> MfpuExtraKeys() { return kMfpuExtraKeys; }

std::optional<uint8_t> FindMfpuKeySlot(std::span<const KeyMapping> keys,
                                       uint8_t code) {
  for (const KeyMapping& key : keys) {
    if (key.code == code) return key.slot;
  }
  return std::nullopt;
}

std::optional<B8Button> FindB8Key(int side, uint8_t code) {
  const auto& keys = side == kSideLeft ? kB8LeftKeys : kB8RightKeys;
  for (const B8KeyMapping& key : keys) {
    if (key.code == code) return key.button;
  }
  return std::nullopt;
}

}  // namespace can_bridge::protocol
