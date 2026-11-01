#ifndef CAN_BRIDGE_PROTOCOL_OUTPUT_ENCODER_H_
#define CAN_BRIDGE_PROTOCOL_OUTPUT_ENCODER_H_

#include <array>
#include <atomic>
#include <optional>
#include <vector>

#include "can/can_frame.h"

namespace can_bridge::protocol {

// Values the simulator wants to see on the cockpit indicators.
struct IndicatorValues {
  double compass_heading_deg = 0.0;
  double slip_ball = 0.0;  // "Шарик", -1..1 (left..right).
  double adf_frequency_khz = 0.0;
  int so2010_message = 0;
  int so2010_code = 0;
};

struct OutputEncoderOptions {
  // The compass frame splits the heading into data[1] * base + data[2]. The
  // legacy code used 255; standard byte splitting would be 256. Keep 255
  // unless the controller firmware says otherwise.
  int compass_byte_base = 255;
};

// Builds indicator frames and decides when they have to be sent.
//
// A frame is sent when its encoded payload differs from the last one sent
// (send-on-change). Comparing payloads instead of raw doubles avoids both
// redundant traffic and the legacy thresholds that hid small changes (the
// slip ball only moved after a change of 1.0, half of its range).
class OutputEncoder {
 public:
  explicit OutputEncoder(OutputEncoderOptions options) : options_(options) {}

  OutputEncoder(const OutputEncoder&) = delete;
  OutputEncoder& operator=(const OutputEncoder&) = delete;

  // Returns the frames that have to be sent for `values`.
  std::vector<CanFrame> Update(const IndicatorValues& values);

  // Makes the next Update() send every frame, e.g. after the controller
  // reconnected and may have lost its display state. Thread-safe.
  void RequestFullRefresh() { refresh_requested_.store(true); }

  static CanFrame EncodeSo2010(int message, int code);
  static CanFrame EncodeAdfFrequency(double frequency_khz);
  static CanFrame EncodeCompass(double heading_deg, int byte_base);
  static CanFrame EncodeSlipBall(double position);

 private:
  enum Output { kSo2010 = 0, kAdf, kCompass, kSlipBall, kOutputCount };

  const OutputEncoderOptions options_;
  std::atomic<bool> refresh_requested_{true};
  std::array<std::optional<CanFrame>, kOutputCount> last_sent_;
};

}  // namespace can_bridge::protocol

#endif  // CAN_BRIDGE_PROTOCOL_OUTPUT_ENCODER_H_
