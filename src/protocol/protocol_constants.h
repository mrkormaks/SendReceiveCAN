#ifndef CAN_BRIDGE_PROTOCOL_PROTOCOL_CONSTANTS_H_
#define CAN_BRIDGE_PROTOCOL_PROTOCOL_CONSTANTS_H_

#include <cstdint>

// Constants of the cockpit controllers' CAN protocol.
//
// Every frame carries the device code in data[0]; frames are dispatched by
// that byte, not by the CAN identifier. For keys, data[1] is the key code and
// 0 (or any unknown code) means "all keys of this device released". Knob
// frames are 3 bytes long for a rotation step (data[1]: 0 = clockwise,
// 1 = counter-clockwise) and 4 bytes long for the knob's push button
// (data[3]: 1 = pressed, 0 = released).
namespace can_bridge::protocol {

// Panel bus (port 1): inputs.
inline constexpr uint8_t kMfiLeftOuterKnob = 20;  // + "ИНДИК" button.
inline constexpr uint8_t kMfiLeftInnerKnob = 21;
inline constexpr uint8_t kMfiLeftBrightness = 22;
inline constexpr uint8_t kMfiCenterOuterKnob = 23;  // + "ИНДИК" button.
inline constexpr uint8_t kMfiCenterInnerKnob = 24;  // + ISRP "Яркость +".
inline constexpr uint8_t kMfiCenterBrightness = 25;  // + ISRP "FE".
inline constexpr uint8_t kMfiRightOuterKnob = 26;    // + "ИНДИК" button.
inline constexpr uint8_t kMfiRightInnerKnob = 27;
inline constexpr uint8_t kMfiRightBrightness = 28;  // + ISRP "Яркость -".
inline constexpr uint8_t kIsrpBaroKnob = 29;
inline constexpr uint8_t kIsrpHdgKnob = 30;
inline constexpr uint8_t kB8LeftKeys = 42;
inline constexpr uint8_t kB8RightKeys = 43;
inline constexpr uint8_t kSo2010 = 60;
inline constexpr uint8_t kMfiCenterKeys = 61;
inline constexpr uint8_t kMfiRightKeys = 62;
inline constexpr uint8_t kMfiLeftKeys = 63;
inline constexpr uint8_t kMfpuRightMainKeys = 64;
inline constexpr uint8_t kMfpuRightExtraKeys = 65;
inline constexpr uint8_t kMfpuLeftMainKeys = 66;
inline constexpr uint8_t kMfpuLeftExtraKeys = 67;
inline constexpr uint8_t kB8RightSelectors = 81;
inline constexpr uint8_t kB8LeftSelectors = 82;

// Panel bus (port 1): outputs. The CAN identifier equals the device code.
inline constexpr uint8_t kAdfIndicator = 51;       // АРК-40 frequency.
inline constexpr uint8_t kSo2010Indicator = 60;    // СО-2010 display.
inline constexpr uint8_t kCompassIndicator = 80;   // Compass heading.
inline constexpr uint8_t kSlipBallIndicator = 88;  // "Шарик".

// Overhead panel bus (port 2): seven rows of toggle switches.
inline constexpr uint8_t kOverheadFirstRow = 71;
inline constexpr uint8_t kOverheadLastRow = 77;
// Requests the current switch state of a row: id = kOverheadPollBaseId + row
// number (1..7), one data byte kOverheadPollPayload.
inline constexpr uint32_t kOverheadPollBaseId = 120;
inline constexpr uint8_t kOverheadPollPayload = 111;

// Acknowledgements are sent with id = kAckIdOffset + device code.
inline constexpr uint32_t kAckIdOffset = 100;

// SO-2010 key that carries the entered code in data[2..5].
inline constexpr uint8_t kSo2010CodeKey = 4;
// SO-2010 message that shows a fixed text instead of the code.
inline constexpr uint8_t kSo2010FixedTextMessage = 10;

}  // namespace can_bridge::protocol

#endif  // CAN_BRIDGE_PROTOCOL_PROTOCOL_CONSTANTS_H_
