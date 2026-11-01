#include "protocol/output_encoder.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "protocol/protocol_constants.h"

namespace can_bridge::protocol {

std::vector<CanFrame> OutputEncoder::Update(const IndicatorValues& values) {
  std::array<CanFrame, kOutputCount> frames;
  frames[kSo2010] = EncodeSo2010(values.so2010_message, values.so2010_code);
  frames[kAdf] = EncodeAdfFrequency(values.adf_frequency_khz);
  frames[kCompass] =
      EncodeCompass(values.compass_heading_deg, options_.compass_byte_base);
  frames[kSlipBall] = EncodeSlipBall(values.slip_ball);

  const bool refresh = refresh_requested_.exchange(false);
  std::vector<CanFrame> to_send;
  for (int i = 0; i < kOutputCount; ++i) {
    if (refresh || last_sent_[i] != frames[i]) {
      to_send.push_back(frames[i]);
      last_sent_[i] = frames[i];
    }
  }
  return to_send;
}

CanFrame OutputEncoder::EncodeSo2010(int message, int code) {
  CanFrame frame = MakeCanFrame(
      kSo2010Indicator,
      {kSo2010Indicator, static_cast<uint8_t>(message), 0, 0, 0, 0});
  if (message == kSo2010FixedTextMessage) {
    frame.data[3] = 1;  // The fixed text expects "0 1" in the digit bytes.
    return frame;
  }
  code = std::clamp(code, 0, 9999);
  frame.data[2] = static_cast<uint8_t>(code / 1000);
  frame.data[3] = static_cast<uint8_t>(code / 100 % 10);
  frame.data[4] = static_cast<uint8_t>(code / 10 % 10);
  frame.data[5] = static_cast<uint8_t>(code % 10);
  return frame;
}

CanFrame OutputEncoder::EncodeAdfFrequency(double frequency_khz) {
  CanFrame frame = MakeCanFrame(kAdfIndicator, {kAdfIndicator, 0, 0, 0});
  if (!(frequency_khz >= 0.0)) frequency_khz = 0.0;  // Also catches NaN.
  if (frequency_khz >= 65535.0) {
    // 0xFFFF kHz is the "no frequency" marker of the display.
    frame.data[1] = frame.data[2] = frame.data[3] = 0xFF;
    return frame;
  }
  const auto whole_khz = static_cast<uint16_t>(frequency_khz);
  frame.data[1] = static_cast<uint8_t>(whole_khz >> 8);
  frame.data[2] = static_cast<uint8_t>(whole_khz & 0xFF);
  // The display shows a ".5" suffix for any fractional frequency.
  frame.data[3] = frequency_khz - whole_khz > 0.00005 ? 5 : 0;
  return frame;
}

CanFrame OutputEncoder::EncodeCompass(double heading_deg, int byte_base) {
  if (!std::isfinite(heading_deg)) heading_deg = 0.0;
  double normalized = std::fmod(heading_deg, 360.0);
  if (normalized < 0.0) normalized += 360.0;
  const int heading = std::clamp(static_cast<int>(normalized), 0, 359);
  const int high = heading / byte_base;
  const int low = heading - high * byte_base;
  return MakeCanFrame(kCompassIndicator,
                      {kCompassIndicator, static_cast<uint8_t>(high),
                       static_cast<uint8_t>(low)});
}

CanFrame OutputEncoder::EncodeSlipBall(double position) {
  if (!std::isfinite(position)) position = 0.0;
  // -1..1 maps to 0..100; truncation matches the legacy conversion.
  const double raw = std::clamp((position + 1.0) * 50.0, 0.0, 255.0);
  return MakeCanFrame(kSlipBallIndicator,
                      {kSlipBallIndicator, static_cast<uint8_t>(raw)});
}

}  // namespace can_bridge::protocol
