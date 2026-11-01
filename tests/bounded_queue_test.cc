#include "common/bounded_queue.h"

#include <gtest/gtest.h>

#include <vector>

namespace can_bridge {
namespace {

TEST(BoundedQueueTest, KeepsOrder) {
  BoundedQueue<int> queue(10);
  EXPECT_TRUE(queue.Push(1));
  EXPECT_TRUE(queue.Push(2));
  EXPECT_TRUE(queue.Push(3));

  std::vector<int> out;
  EXPECT_EQ(queue.PopBatch(2, &out), 2u);
  EXPECT_EQ(out, (std::vector<int>{1, 2}));
  EXPECT_EQ(queue.PopBatch(10, &out), 1u);
  EXPECT_EQ(out, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(queue.size(), 0u);
}

TEST(BoundedQueueTest, DropsOldestWhenFull) {
  BoundedQueue<int> queue(2);
  EXPECT_TRUE(queue.Push(1));
  EXPECT_TRUE(queue.Push(2));
  EXPECT_FALSE(queue.Push(3));
  EXPECT_EQ(queue.dropped_count(), 1u);

  std::vector<int> out;
  queue.PopBatch(10, &out);
  EXPECT_EQ(out, (std::vector<int>{2, 3}));
}

TEST(BoundedQueueTest, ClearReturnsRemovedCount) {
  BoundedQueue<int> queue(10);
  queue.Push(1);
  queue.Push(2);
  EXPECT_EQ(queue.Clear(), 2u);
  EXPECT_EQ(queue.size(), 0u);
  EXPECT_EQ(queue.dropped_count(), 0u);
}

TEST(BoundedQueueTest, ZeroCapacityStillHoldsOneElement) {
  BoundedQueue<int> queue(0);
  queue.Push(7);
  std::vector<int> out;
  queue.PopBatch(10, &out);
  EXPECT_EQ(out, (std::vector<int>{7}));
}

}  // namespace
}  // namespace can_bridge
