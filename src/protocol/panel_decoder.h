#ifndef CAN_BRIDGE_PROTOCOL_PANEL_DECODER_H_
#define CAN_BRIDGE_PROTOCOL_PANEL_DECODER_H_

#include <array>

#include "protocol/frame_decoder.h"
#include "protocol/rotary_switch.h"

namespace can_bridge::protocol {

// What the ISRP-26 HDG knob (device code 30) controls.
enum class HdgKnobMode {
  // Legacy behaviour: rotation is reported as the outer "ИНДИК" ring of the
  // left MFI, the push button is ignored.
  kLeftMfiOuterKnob,
  // Rotation and push button drive the ISRP-26 HDG fields, as the commented
  // out code in the legacy implementation intended.
  kIsrpHdg,
};

struct PanelDecoderOptions {
  HdgKnobMode hdg_knob_mode = HdgKnobMode::kLeftMfiOuterKnob;
};

// Decodes the instrument panel bus: MFI, MFPU, B8-50KR, ISRP-26 and SO-2010.
class PanelDecoder : public FrameDecoder {
 public:
  explicit PanelDecoder(PanelDecoderOptions options) : options_(options) {}

  DecodeResult Decode(const CanFrame& frame) override;

 private:
  void DecodeMfiOuterKnob(int mfi, const CanFrame& frame, InputEvent* event);
  void DecodeMfiKeys(int mfi, const CanFrame& frame, InputEvent* event);
  void DecodeMfpuKeys(int side, bool extra_group, const CanFrame& frame,
                      InputEvent* event);
  void DecodeB8Keys(int side, const CanFrame& frame, InputEvent* event);
  void DecodeB8Selectors(int side, const CanFrame& frame, InputEvent* event);
  void DecodeHdgKnob(const CanFrame& frame, InputEvent* event);
  void DecodeSo2010(const CanFrame& frame, InputEvent* event);

  const PanelDecoderOptions options_;
  // [side][selector]: upper (endless) part of the B8-50KR selectors.
  std::array<std::array<RotarySwitchTracker, 2>, kB8PanelCount> rotation_;
};

}  // namespace can_bridge::protocol

#endif  // CAN_BRIDGE_PROTOCOL_PANEL_DECODER_H_
