#ifndef CAN_BRIDGE_SIM_SHARED_MEMORY_REGION_H_
#define CAN_BRIDGE_SIM_SHARED_MEMORY_REGION_H_

#include <cstddef>
#include <string>

#include "common/status.h"

namespace can_bridge {

// Named Win32 shared memory block (file mapping backed by the page file).
class SharedMemoryRegion {
 public:
  SharedMemoryRegion() = default;
  ~SharedMemoryRegion();

  SharedMemoryRegion(const SharedMemoryRegion&) = delete;
  SharedMemoryRegion& operator=(const SharedMemoryRegion&) = delete;

  // Maps the block `name`, creating it when the simulator has not done so
  // yet, so the start order of the processes does not matter. Fails if an
  // existing block is smaller than `size` (mismatched structure versions).
  Status OpenOrCreate(const std::string& name, size_t size);

  void* data() const { return view_; }
  bool created() const { return created_; }

  template <typename T>
  T* As() const {
    return static_cast<T*>(view_);
  }

 private:
  void Close();

  void* mapping_ = nullptr;  // HANDLE.
  void* view_ = nullptr;
  bool created_ = false;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_SIM_SHARED_MEMORY_REGION_H_
