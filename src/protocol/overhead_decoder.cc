#include "protocol/overhead_decoder.h"

#include <array>

#include "protocol/protocol_constants.h"

namespace can_bridge::protocol {
namespace {

struct OverheadRow {
  uint8_t device;
  uint8_t first_slot;
  // Bits that correspond to installed switches. Other slots are never written,
  // exactly as in the legacy code.
  uint8_t low_mask;   // data[2] -> slots first_slot + 0..7.
  uint8_t high_mask;  // data[1] -> slots first_slot + 8..15.
};

constexpr std::array<OverheadRow, 7> kRows = {{
    {71, 0, 0xFF, 0xFF},
    {72, 16, 0xFF, 0xFF},
    {73, 32, 0xFF, 0x1F},
    {74, 48, 0x3F, 0x1F},
    {75, 64, 0x1F, 0xFF},
    {76, 80, 0xFF, 0xFF},
    {77, 96, 0xFF, 0x3F},
}};

const OverheadRow* FindRow(uint8_t device) {
  for (const OverheadRow& row : kRows) {
    if (row.device == device) return &row;
  }
  return nullptr;
}

void PushSwitches(int first_slot, uint8_t mask, uint8_t bits,
                  InputEvent* event) {
  for (int bit = 0; bit < 8; ++bit) {
    if ((mask & (1u << bit)) == 0) continue;
    const bool on = (bits & (1u << bit)) != 0;
    event->push_back(AssignField(FieldKind::kOverheadSwitch, 0,
                                 first_slot + bit, on ? 1 : 0));
  }
}

}  // namespace

DecodeResult OverheadDecoder::Decode(const CanFrame& frame) {
  DecodeResult result;
  // A row report needs data[0..2]; shorter frames would read as "all off".
  if (frame.is_error() || frame.is_remote() || frame.length < 3) return result;

  const OverheadRow* row = FindRow(frame.data[0]);
  if (row == nullptr) return result;

  PushSwitches(row->first_slot, row->low_mask, frame.data[2], &result.event);
  PushSwitches(row->first_slot + 8, row->high_mask, frame.data[1],
               &result.event);
  result.ack = MakeCanFrame(kAckIdOffset + row->device,
                            {frame.data[0], frame.data[1], frame.data[2]});
  return result;
}

std::vector<CanFrame> OverheadDecoder::MakePollRequests() {
  std::vector<CanFrame> requests;
  for (int row = 1; row <= kOverheadLastRow - kOverheadFirstRow + 1; ++row) {
    requests.push_back(
        MakeCanFrame(kOverheadPollBaseId + row, {kOverheadPollPayload}));
  }
  return requests;
}

}  // namespace can_bridge::protocol
