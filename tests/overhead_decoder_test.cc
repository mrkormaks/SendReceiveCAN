#include "protocol/overhead_decoder.h"

#include <gtest/gtest.h>

#include <map>

namespace can_bridge::protocol {
namespace {

// Slot -> state for all switch writes of an event.
std::map<int, bool> Switches(const InputEvent& event) {
  std::map<int, bool> switches;
  for (const FieldWrite& write : event) {
    EXPECT_EQ(write.key.kind, FieldKind::kOverheadSwitch);
    EXPECT_EQ(write.op, WriteOp::kAssign);
    switches[write.key.index] = write.value != 0;
  }
  return switches;
}

TEST(OverheadDecoderTest, FirstRowUsesAllSixteenSlots) {
  OverheadDecoder decoder;
  // data[2] bit 0 -> slot 0 ("СОК"), data[1] bit 7 -> slot 15 ("ВЕНТ ГО").
  const DecodeResult result =
      decoder.Decode(MakeCanFrame(0x47, {71, 0x80, 0x01}));
  const std::map<int, bool> switches = Switches(result.event);
  EXPECT_EQ(switches.size(), 16u);
  EXPECT_TRUE(switches.at(0));
  EXPECT_TRUE(switches.at(15));
  EXPECT_FALSE(switches.at(1));
  EXPECT_FALSE(switches.at(8));
  EXPECT_EQ(result.ack, MakeCanFrame(171, {71, 0x80, 0x01}));
}

TEST(OverheadDecoderTest, PartialRowsWriteOnlyInstalledSwitches) {
  OverheadDecoder decoder;
  // Row 74: data[2] bits 0..5 -> slots 48..53, data[1] bits 0..4 -> 56..60.
  const std::map<int, bool> switches =
      Switches(decoder.Decode(MakeCanFrame(1, {74, 0xFF, 0xFF})).event);
  EXPECT_EQ(switches.size(), 11u);
  EXPECT_EQ(switches.begin()->first, 48);
  EXPECT_EQ(switches.rbegin()->first, 60);
  EXPECT_EQ(switches.count(54), 0u);
  EXPECT_EQ(switches.count(55), 0u);

  // Row 77: up to slot 109 ("ВЕНТИЛЯТОР").
  const std::map<int, bool> last_row =
      Switches(decoder.Decode(MakeCanFrame(1, {77, 0xFF, 0xFF})).event);
  EXPECT_EQ(last_row.rbegin()->first, 109);
}

TEST(OverheadDecoderTest, IgnoresShortAndUnknownFrames) {
  OverheadDecoder decoder;
  EXPECT_TRUE(decoder.Decode(MakeCanFrame(1, {71, 0xFF})).event.empty());
  EXPECT_FALSE(decoder.Decode(MakeCanFrame(1, {71, 0xFF})).ack.has_value());
  EXPECT_TRUE(decoder.Decode(MakeCanFrame(1, {70, 0, 0})).event.empty());
  EXPECT_TRUE(decoder.Decode(MakeCanFrame(1, {78, 0, 0})).event.empty());
}

TEST(OverheadDecoderTest, PollRequestsCoverAllRows) {
  const std::vector<CanFrame> requests = OverheadDecoder::MakePollRequests();
  ASSERT_EQ(requests.size(), 7u);
  for (size_t i = 0; i < requests.size(); ++i) {
    EXPECT_EQ(requests[i], MakeCanFrame(121 + static_cast<uint32_t>(i), {111}));
  }
}

}  // namespace
}  // namespace can_bridge::protocol
