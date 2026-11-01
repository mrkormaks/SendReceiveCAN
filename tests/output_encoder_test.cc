#include "protocol/output_encoder.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace can_bridge::protocol {
namespace {

TEST(OutputEncoderTest, SendsEverythingFirstThenOnlyChanges) {
  OutputEncoder encoder(OutputEncoderOptions{});
  IndicatorValues values;
  values.compass_heading_deg = 90.0;
  EXPECT_EQ(encoder.Update(values).size(), 4u);
  EXPECT_TRUE(encoder.Update(values).empty());

  values.compass_heading_deg = 90.7;  // Same whole degree: same payload.
  EXPECT_TRUE(encoder.Update(values).empty());

  values.compass_heading_deg = 91.0;
  const std::vector<CanFrame> frames = encoder.Update(values);
  ASSERT_EQ(frames.size(), 1u);
  EXPECT_EQ(frames[0].id, 80u);
}

TEST(OutputEncoderTest, SmallSlipBallMovesAreSent) {
  OutputEncoder encoder(OutputEncoderOptions{});
  IndicatorValues values;
  encoder.Update(values);

  values.slip_ball = 0.03;  // One display step; the legacy code needed 1.0.
  const std::vector<CanFrame> frames = encoder.Update(values);
  ASSERT_EQ(frames.size(), 1u);
  EXPECT_EQ(frames[0], MakeCanFrame(88, {88, 51}));
}

TEST(OutputEncoderTest, FullRefreshResendsEverything) {
  OutputEncoder encoder(OutputEncoderOptions{});
  const IndicatorValues values;
  encoder.Update(values);
  encoder.RequestFullRefresh();
  EXPECT_EQ(encoder.Update(values).size(), 4u);
  EXPECT_TRUE(encoder.Update(values).empty());
}

TEST(OutputEncoderTest, So2010Frames) {
  EXPECT_EQ(OutputEncoder::EncodeSo2010(3, 1234),
            MakeCanFrame(60, {60, 3, 1, 2, 3, 4}));
  EXPECT_EQ(OutputEncoder::EncodeSo2010(10, 1234),
            MakeCanFrame(60, {60, 10, 0, 1, 0, 0}));
  EXPECT_EQ(OutputEncoder::EncodeSo2010(1, 70),
            MakeCanFrame(60, {60, 1, 0, 0, 7, 0}));
}

TEST(OutputEncoderTest, AdfFrames) {
  EXPECT_EQ(OutputEncoder::EncodeAdfFrequency(1215.0),
            MakeCanFrame(51, {51, 0x04, 0xBF, 0}));
  EXPECT_EQ(OutputEncoder::EncodeAdfFrequency(1215.5),
            MakeCanFrame(51, {51, 0x04, 0xBF, 5}));
  EXPECT_EQ(OutputEncoder::EncodeAdfFrequency(65535.0),
            MakeCanFrame(51, {51, 0xFF, 0xFF, 0xFF}));
  EXPECT_EQ(OutputEncoder::EncodeAdfFrequency(-3.0),
            MakeCanFrame(51, {51, 0, 0, 0}));
  EXPECT_EQ(OutputEncoder::EncodeAdfFrequency(
                std::numeric_limits<double>::quiet_NaN()),
            MakeCanFrame(51, {51, 0, 0, 0}));
}

TEST(OutputEncoderTest, CompassFrames) {
  EXPECT_EQ(OutputEncoder::EncodeCompass(254.9, 255),
            MakeCanFrame(80, {80, 0, 254}));
  EXPECT_EQ(OutputEncoder::EncodeCompass(300.0, 255),
            MakeCanFrame(80, {80, 1, 45}));
  EXPECT_EQ(OutputEncoder::EncodeCompass(300.0, 256),
            MakeCanFrame(80, {80, 1, 44}));
  EXPECT_EQ(OutputEncoder::EncodeCompass(360.0, 255),
            MakeCanFrame(80, {80, 0, 0}));
  EXPECT_EQ(OutputEncoder::EncodeCompass(-10.0, 255),
            MakeCanFrame(80, {80, 1, 95}));  // 350 degrees.
  EXPECT_EQ(OutputEncoder::EncodeCompass(INFINITY, 255),
            MakeCanFrame(80, {80, 0, 0}));
}

TEST(OutputEncoderTest, SlipBallFrames) {
  EXPECT_EQ(OutputEncoder::EncodeSlipBall(-1.0), MakeCanFrame(88, {88, 0}));
  EXPECT_EQ(OutputEncoder::EncodeSlipBall(0.0), MakeCanFrame(88, {88, 50}));
  EXPECT_EQ(OutputEncoder::EncodeSlipBall(1.0), MakeCanFrame(88, {88, 100}));
  EXPECT_EQ(OutputEncoder::EncodeSlipBall(-5.0), MakeCanFrame(88, {88, 0}));
  EXPECT_EQ(OutputEncoder::EncodeSlipBall(10.0), MakeCanFrame(88, {88, 255}));
}

}  // namespace
}  // namespace can_bridge::protocol
