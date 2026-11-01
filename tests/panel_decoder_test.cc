#include "protocol/panel_decoder.h"

#include <gtest/gtest.h>

#include <algorithm>

#include "protocol/protocol_constants.h"

namespace can_bridge::protocol {
namespace {

using ::testing::Test;

class PanelDecoderTest : public Test {
 protected:
  DecodeResult Decode(std::initializer_list<uint8_t> payload) {
    return decoder_.Decode(MakeCanFrame(0x10, payload));
  }

  PanelDecoder decoder_{PanelDecoderOptions{}};
};

FieldWrite Assign(FieldKind kind, int unit, int index, int value) {
  return AssignField(kind, unit, index, value);
}

FieldWrite Add(FieldKind kind, int unit, int index, int delta) {
  return AddToField(kind, unit, index, delta);
}

TEST_F(PanelDecoderTest, MfiKeyPressAndAck) {
  const DecodeResult result = Decode({kMfiLeftKeys, 24});
  EXPECT_EQ(result.event,
            (InputEvent{Assign(FieldKind::kMfiFkButton, kMfiLeft, 1, 1)}));
  EXPECT_EQ(result.ack, MakeCanFrame(163, {63, 24}));

  EXPECT_EQ(Decode({kMfiCenterKeys, 48}).event,
            (InputEvent{Assign(FieldKind::kMfiMfkButton, kMfiCenter, 1, 1)}));
  EXPECT_EQ(Decode({kMfiRightKeys, 1}).event,
            (InputEvent{Assign(FieldKind::kMfiMfkButton, kMfiRight, 20, 1)}));
}

TEST_F(PanelDecoderTest, MfiReleaseClearsAllKeysOfThatIndicator) {
  const DecodeResult result = Decode({kMfiRightKeys, 0});
  EXPECT_EQ(result.event.size(),
            static_cast<size_t>(kMfiFkSlots - 1 + kMfiMfkSlots - 1));
  for (const FieldWrite& write : result.event) {
    EXPECT_EQ(write.key.unit, kMfiRight);
    EXPECT_EQ(write.value, 0);
    EXPECT_GE(write.key.index, 1);
  }
  EXPECT_EQ(result.ack, MakeCanFrame(162, {62, 0}));
}

TEST_F(PanelDecoderTest, MfpuKeysAndRelease) {
  EXPECT_EQ(Decode({kMfpuLeftMainKeys, 7}).event,
            (InputEvent{Assign(FieldKind::kMfpuButton, kSideLeft, 20, 1)}));
  EXPECT_EQ(Decode({kMfpuRightExtraKeys, 13}).event,
            (InputEvent{Assign(FieldKind::kMfpuButton, kSideRight, 73, 1)}));

  const DecodeResult release = Decode({kMfpuRightMainKeys, 0});
  EXPECT_EQ(release.event.size(), 56u);
  EXPECT_TRUE(std::all_of(
      release.event.begin(), release.event.end(), [](const FieldWrite& write) {
        return write.key.unit == kSideRight && write.value == 0;
      }));
  EXPECT_EQ(release.ack, MakeCanFrame(164, {64, 0}));
  EXPECT_EQ(Decode({kMfpuLeftExtraKeys, 0}).event.size(), 18u);
}

TEST_F(PanelDecoderTest, B8KeysUsePerPanelTables) {
  EXPECT_EQ(Decode({kB8LeftKeys, 11}).event,
            (InputEvent{Assign(FieldKind::kB8Button, kSideLeft,
                               static_cast<int>(B8Button::kDigit5), 1)}));
  EXPECT_EQ(Decode({kB8RightKeys, 11}).event,
            (InputEvent{Assign(FieldKind::kB8Button, kSideRight,
                               static_cast<int>(B8Button::kDigit8), 1)}));
  const DecodeResult release = Decode({kB8LeftKeys, 0});
  EXPECT_EQ(release.event.size(), static_cast<size_t>(kB8ButtonCount));
  EXPECT_EQ(release.ack, MakeCanFrame(142, {42, 0}));
}

TEST_F(PanelDecoderTest, B8SelectorsPositionAndRotation) {
  constexpr int kSet = static_cast<int>(B8Selector::kSet);
  constexpr int kVolume = static_cast<int>(B8Selector::kVolume);

  // Set selector in position 2 (bit 5), phase 1; volume in position 0
  // (bit 3), phase 1.
  DecodeResult result = Decode({kB8LeftSelectors, 0x20 | 0x01, 0x08 | 0x01, 0});
  EXPECT_EQ(result.event,
            (InputEvent{
                Assign(FieldKind::kB8SelectorPosition, kSideLeft, kSet, 2),
                Assign(FieldKind::kB8SelectorPosition, kSideLeft, kVolume, 0),
            }));
  EXPECT_FALSE(result.ack.has_value());

  // Set selector turned clockwise, volume counter-clockwise; "РНУ 3" contact.
  result = Decode({kB8LeftSelectors, 0x20 | 0x02, 0x08 | 0x04, 1});
  EXPECT_EQ(result.event,
            (InputEvent{
                Assign(FieldKind::kB8SelectorPosition, kSideLeft, kSet, 2),
                Assign(FieldKind::kB8SelectorPosition, kSideLeft, kVolume, 5),
                Add(FieldKind::kB8SelectorRoll, kSideLeft, kSet, 1),
                Add(FieldKind::kB8SelectorRoll, kSideLeft, kVolume, -1),
            }));

  // The right panel has its own trackers: its first frame never steps.
  result = Decode({kB8RightSelectors, 0x02, 0x04, 0});
  EXPECT_TRUE(result.event.empty());
}

TEST_F(PanelDecoderTest, MfiKnobs) {
  EXPECT_EQ(Decode({kMfiLeftOuterKnob, 0, 1}).event,
            (InputEvent{Assign(FieldKind::kMfiOuterKnobStep, kMfiLeft, 0, 1)}));
  EXPECT_EQ(
      Decode({kMfiRightOuterKnob, 1, 1}).event,
      (InputEvent{Assign(FieldKind::kMfiOuterKnobStep, kMfiRight, 0, -1)}));
  EXPECT_EQ(Decode({kMfiCenterOuterKnob, 0, 0, 1}).event,
            (InputEvent{Assign(FieldKind::kMfiIndikButton, kMfiCenter, 0, 1)}));
  EXPECT_EQ(
      Decode({kMfiRightInnerKnob, 1}).event,
      (InputEvent{Assign(FieldKind::kMfiInnerKnobStep, kMfiRight, 0, -1)}));
  EXPECT_EQ(
      Decode({kMfiLeftBrightness, 0}).event,
      (InputEvent{Assign(FieldKind::kMfiBrightnessStep, kMfiLeft, 0, 1)}));
  // Unknown direction code: nothing happens.
  EXPECT_TRUE(Decode({kMfiLeftBrightness, 7}).event.empty());
}

TEST_F(PanelDecoderTest, SharedKnobFramesCarryIsrpButtons) {
  EXPECT_EQ(
      Decode({kMfiCenterInnerKnob, 0, 1}).event,
      (InputEvent{Assign(FieldKind::kMfiInnerKnobStep, kMfiCenter, 0, 1)}));
  EXPECT_EQ(Decode({kMfiCenterInnerKnob, 0, 0, 1}).event,
            (InputEvent{Assign(FieldKind::kIsrpBrightnessPlus, 0, 0, 1)}));
  EXPECT_EQ(Decode({kMfiRightBrightness, 0, 0, 1}).event,
            (InputEvent{Assign(FieldKind::kIsrpBrightnessMinus, 0, 0, 1)}));
  EXPECT_EQ(Decode({kMfiCenterBrightness, 0, 0, 0}).event,
            (InputEvent{Assign(FieldKind::kIsrpFastCorrection, 0, 0, 0)}));
}

TEST_F(PanelDecoderTest, CenterBrightnessAdjustsKonturInverted) {
  EXPECT_EQ(
      Decode({kMfiCenterBrightness, 0, 1}).event,
      (InputEvent{Add(FieldKind::kIsrpKontur, 0, 0, 10),
                  Assign(FieldKind::kMfiBrightnessStep, kMfiCenter, 0, -1)}));
  EXPECT_EQ(
      Decode({kMfiCenterBrightness, 1, 1}).event,
      (InputEvent{Add(FieldKind::kIsrpKontur, 0, 0, -10),
                  Assign(FieldKind::kMfiBrightnessStep, kMfiCenter, 0, 1)}));
}

TEST_F(PanelDecoderTest, BaroKnob) {
  EXPECT_EQ(Decode({kIsrpBaroKnob, 0, 1}).event,
            (InputEvent{Add(FieldKind::kIsrpBaroRoll, 0, 0, 1)}));
  EXPECT_EQ(Decode({kIsrpBaroKnob, 1, 1}).event,
            (InputEvent{Add(FieldKind::kIsrpBaroRoll, 0, 0, -1)}));
  EXPECT_EQ(Decode({kIsrpBaroKnob, 0, 0, 1}).event,
            (InputEvent{Assign(FieldKind::kIsrpBaroPress, 0, 0, 1)}));
}

TEST_F(PanelDecoderTest, HdgKnobLegacyModeDrivesLeftMfi) {
  EXPECT_EQ(
      Decode({kIsrpHdgKnob, 1, 1}).event,
      (InputEvent{Assign(FieldKind::kMfiOuterKnobStep, kMfiLeft, 0, -1)}));
  EXPECT_TRUE(Decode({kIsrpHdgKnob, 0, 0, 1}).event.empty());
}

TEST(PanelDecoderHdgTest, IsrpModeDrivesHdgFields) {
  PanelDecoder decoder(PanelDecoderOptions{HdgKnobMode::kIsrpHdg});
  EXPECT_EQ(decoder.Decode(MakeCanFrame(1, {kIsrpHdgKnob, 0, 1})).event,
            (InputEvent{AddToField(FieldKind::kIsrpHdgRoll, 0, 0, 1)}));
  EXPECT_EQ(decoder.Decode(MakeCanFrame(1, {kIsrpHdgKnob, 0, 0, 1})).event,
            (InputEvent{AssignField(FieldKind::kIsrpHdgPress, 0, 0, 1)}));
}

TEST_F(PanelDecoderTest, So2010Keys) {
  DecodeResult result = Decode({kSo2010, 1});
  EXPECT_EQ(result.event,
            (InputEvent{Assign(FieldKind::kSo2010Button, 0, 0, 1)}));
  EXPECT_FALSE(result.ack.has_value());

  EXPECT_EQ(Decode({kSo2010, 4, 1, 2, 0, 7}).event,
            (InputEvent{Assign(FieldKind::kSo2010Button, 0, 3, 1),
                        Assign(FieldKind::kSo2010EnteredCode, 0, 0, 1207)}));
  EXPECT_EQ(Decode({kSo2010, 4}).event,
            (InputEvent{Assign(FieldKind::kSo2010Button, 0, 3, 1),
                        Assign(FieldKind::kSo2010EnteredCode, 0, 0, 0)}));
  EXPECT_TRUE(Decode({kSo2010, 23}).event.empty());
  EXPECT_TRUE(Decode({kSo2010, 13}).event.empty());
}

TEST_F(PanelDecoderTest, IgnoresEmptyErrorRemoteAndUnknownFrames) {
  EXPECT_TRUE(Decode({}).event.empty());
  EXPECT_FALSE(Decode({}).ack.has_value());

  CanFrame error = MakeCanFrame(0xFFFFFFFF, {kMfiLeftKeys, 1});
  error.flags = kCanFlagError;
  EXPECT_TRUE(decoder_.Decode(error).event.empty());
  EXPECT_FALSE(decoder_.Decode(error).ack.has_value());

  CanFrame remote = MakeCanFrame(0x10, {kMfiLeftKeys, 1});
  remote.flags = kCanFlagRemote;
  EXPECT_TRUE(decoder_.Decode(remote).event.empty());

  const DecodeResult unknown = Decode({99, 1, 2});
  EXPECT_TRUE(unknown.event.empty());
  EXPECT_FALSE(unknown.ack.has_value());
}

}  // namespace
}  // namespace can_bridge::protocol
