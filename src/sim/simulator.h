#ifndef CAN_BRIDGE_SIM_SIMULATOR_H_
#define CAN_BRIDGE_SIM_SIMULATOR_H_

#include "cockpit/input_scheduler.h"
#include "protocol/output_encoder.h"

namespace can_bridge {

// The simulator side of the bridge. All methods are called from the bridge's
// main loop thread.
class Simulator {
 public:
  virtual ~Simulator() = default;

  // Hands over the inputs of one publish cycle.
  virtual void Publish(const PublishBatch& batch) = 0;

  // Current values for the cockpit indicators.
  virtual protocol::IndicatorValues ReadIndicators() const = 0;

  // Reports the health of a CAN port through fault flag `flag` (1 or 2).
  virtual void SetDeviceFault(int flag, bool faulty) = 0;

  // True when the simulator asks the bridge to exit.
  virtual bool IsCloseRequested() const = 0;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_SIM_SIMULATOR_H_
