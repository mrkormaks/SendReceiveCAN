#ifndef CAN_BRIDGE_SIM_LEGACY_SIMULATOR_H_
#define CAN_BRIDGE_SIM_LEGACY_SIMULATOR_H_

#include "sim/legacy_sim_headers.h"
#include "sim/simulator.h"

namespace can_bridge {

// Simulator adapter for the legacy shared-memory layout (SHAVN_410 and
// SHSUT). It only translates field names and units; where the memory comes
// from (Win32 file mapping, a test fixture) is up to the caller.
class LegacySimulator : public Simulator {
 public:
  // Both blocks must outlive the adapter.
  LegacySimulator(SHAVN_410* avn, SHSUT* sut) : avn_(avn), sut_(sut) {}

  void Publish(const PublishBatch& batch) override;
  protocol::IndicatorValues ReadIndicators() const override;
  void SetDeviceFault(int flag, bool faulty) override;
  bool IsCloseRequested() const override;

 private:
  void WriteLevels(const LevelInputs& levels);
  void ApplyDirectWrite(const FieldWrite& write);

  SHAVN_410* avn_;
  SHSUT* sut_;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_SIM_LEGACY_SIMULATOR_H_
