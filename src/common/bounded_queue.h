#ifndef CAN_BRIDGE_COMMON_BOUNDED_QUEUE_H_
#define CAN_BRIDGE_COMMON_BOUNDED_QUEUE_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <utility>
#include <vector>

namespace can_bridge {

// Thread-safe FIFO with a fixed capacity. When the queue is full, the oldest
// element is discarded so that producers never block and fresh data wins.
template <typename T>
class BoundedQueue {
 public:
  explicit BoundedQueue(size_t capacity)
      : capacity_(std::max<size_t>(1, capacity)) {}

  BoundedQueue(const BoundedQueue&) = delete;
  BoundedQueue& operator=(const BoundedQueue&) = delete;

  // Appends `value`. Returns false if an old element had to be dropped.
  bool Push(T value) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool dropped = false;
    if (items_.size() >= capacity_) {
      items_.pop_front();
      ++dropped_count_;
      dropped = true;
    }
    items_.push_back(std::move(value));
    return !dropped;
  }

  // Moves up to `max_items` elements to the end of `*out`. Returns the number
  // of moved elements. Never blocks.
  size_t PopBatch(size_t max_items, std::vector<T>* out) {
    std::lock_guard<std::mutex> lock(mutex_);
    const size_t count = std::min(max_items, items_.size());
    for (size_t i = 0; i < count; ++i) {
      out->push_back(std::move(items_.front()));
      items_.pop_front();
    }
    return count;
  }

  // Removes all elements and returns how many there were.
  size_t Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    const size_t count = items_.size();
    items_.clear();
    return count;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return items_.size();
  }

  uint64_t dropped_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return dropped_count_;
  }

 private:
  const size_t capacity_;
  mutable std::mutex mutex_;
  std::deque<T> items_;
  uint64_t dropped_count_ = 0;
};

}  // namespace can_bridge

#endif  // CAN_BRIDGE_COMMON_BOUNDED_QUEUE_H_
