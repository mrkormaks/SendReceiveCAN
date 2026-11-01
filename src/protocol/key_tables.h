#ifndef CAN_BRIDGE_PROTOCOL_KEY_TABLES_H_
#define CAN_BRIDGE_PROTOCOL_KEY_TABLES_H_

#include <cstdint>
#include <optional>
#include <span>

#include "cockpit/input_fields.h"

// Mapping from hardware key codes (data[1] of key frames) to the slots of the
// simulator arrays. The tables reproduce the legacy implementation exactly.
namespace can_bridge::protocol {

struct KeyMapping {
  uint8_t code;
  uint8_t slot;

  bool operator==(const KeyMapping& other) const = default;
};

struct MfiKeyMapping {
  uint8_t code;
  bool upper_row;  // True: FK key (upper row), false: MFK key.
  uint8_t slot;

  bool operator==(const MfiKeyMapping& other) const = default;
};

// MFI keys; identical for all three indicators.
std::span<const MfiKeyMapping> MfiKeys();
std::optional<MfiKeyMapping> FindMfiKey(uint8_t code);

// MFPU keys reported with the "main" device code (66 left, 64 right) and the
// "extra" device code (67 left, 65 right). Identical for both sides.
std::span<const KeyMapping> MfpuMainKeys();
std::span<const KeyMapping> MfpuExtraKeys();
std::optional<uint8_t> FindMfpuKeySlot(std::span<const KeyMapping> keys,
                                       uint8_t code);

// B8-50KR keys. The left and right panels report digits 5 and 8 with swapped
// codes (10/11); this is what the legacy code did and is kept as is.
std::optional<B8Button> FindB8Key(int side, uint8_t code);

}  // namespace can_bridge::protocol

#endif  // CAN_BRIDGE_PROTOCOL_KEY_TABLES_H_
