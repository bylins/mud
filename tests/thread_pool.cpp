// Part of Bylins http://www.mud.ru
// Unit tests for ThreadPool

#include "utils/thread_pool.h"

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <vector>

using namespace utils;

// Test: Create and destroy thread pool
TEST(ThreadPool, ConstructorDestructor) {
	EXPECT_NO_THROW({
		ThreadPool pool(4);
	});
}

// Test: Enqueue and execute simple tasks
TEST(ThreadPool, EnqueueSimpleTasks) {
	ThreadPool pool(4);
	std::atomic<int> counter{0};

	// Enqueue 100 tasks
	std::vector<std::future<void>> futures;
	for (int i = 0; i < 100; ++i) {
		futures.push_back(pool.Enqueue([&counter]() {
			counter++;
		}));
	}

	// Wait for all tasks
	for (auto& future : futures) {
		future.wait();
	}

	EXPECT_EQ(100, counter.load());
}

// Test: WaitAll functionality
TEST(ThreadPool, WaitAll) {
	ThreadPool pool(4);
	std::atomic<int> counter{0};

	// Enqueue 100 tasks
	for (int i = 0; i < 100; ++i) {
		pool.Enqueue([&counter]() {
			counter++;
		});
	}

	// WaitAll should block until all tasks complete
	pool.WaitAll();

	EXPECT_EQ(100, counter.load());
}

// Test: Tasks execute in parallel
TEST(ThreadPool, ParallelExecution) {
	ThreadPool pool(4);
	std::atomic<int> active_tasks{0};
	std::atomic<int> max_concurrent{0};

	// Enqueue tasks that run simultaneously
	std::vector<std::future<void>> futures;
	for (int i = 0; i < 10; ++i) {
		futures.push_back(pool.Enqueue([&active_tasks, &max_concurrent]() {
			int current = ++active_tasks;

			// Update max concurrent tasks
			int expected = max_concurrent.load();
			while (expected < current && !max_concurrent.compare_exchange_weak(expected, current)) {
				expected = max_concurrent.load();
			}

			// Simulate work
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			--active_tasks;
		}));
	}

	for (auto& future : futures) {
		future.wait();
	}

	// With 4 threads and 10 tasks, we should see at least 2 tasks running concurrently
	EXPECT_GE(max_concurrent.load(), 2);
}

// Test: NumThreads returns correct value
TEST(ThreadPool, NumThreads) {
	ThreadPool pool(8);
	EXPECT_EQ(8, pool.NumThreads());
}

// Test: DistributeBatches function
TEST(ThreadPool, DistributeBatches) {
	std::vector<int> items = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

	// Distribute into 4 batches
	auto batches = DistributeBatches(items, 4);

	EXPECT_EQ(4, batches.size());

	// Check round-robin distribution
	EXPECT_EQ(3, batches[0].size());  // Items 1, 5, 9
	EXPECT_EQ(3, batches[1].size());  // Items 2, 6, 10
	EXPECT_EQ(2, batches[2].size());  // Items 3, 7
	EXPECT_EQ(2, batches[3].size());  // Items 4, 8

	// Verify all items are present
	std::vector<int> all_items;
	for (const auto& batch : batches) {
		all_items.insert(all_items.end(), batch.begin(), batch.end());
	}
	std::sort(all_items.begin(), all_items.end());
	EXPECT_EQ(items, all_items);
}

// Test: DistributeBatches with empty input
TEST(ThreadPool, DistributeBatchesEmpty) {
	std::vector<int> items;
	auto batches = DistributeBatches(items, 4);

	EXPECT_EQ(4, batches.size());
	for (const auto& batch : batches) {
		EXPECT_TRUE(batch.empty());
	}
}

// Test: DistributeBatches with more batches than items
TEST(ThreadPool, DistributeBatchesMoreBatchesThanItems) {
	std::vector<int> items = {1, 2, 3};
	auto batches = DistributeBatches(items, 10);

	EXPECT_EQ(10, batches.size());

	// First 3 batches should have 1 item each
	for (size_t i = 0; i < 3; ++i) {
		EXPECT_EQ(1, batches[i].size());
	}

	// Remaining batches should be empty
	for (size_t i = 3; i < 10; ++i) {
		EXPECT_TRUE(batches[i].empty());
	}
}

// Test: Exception in task propagates through future
TEST(ThreadPool, ExceptionInTask_FutureRethrows) {
	ThreadPool pool(2);

	auto future = pool.Enqueue([]() -> int {
		throw std::runtime_error("task error");
		return 42;
	});

	EXPECT_THROW(future.get(), std::runtime_error);
}

// Test: ThreadPool(0) falls back to hardware_concurrency (at least 1 thread)
TEST(ThreadPool, ZeroThreads_FallsBackToHardwareConcurrency) {
	ThreadPool pool(0);
	EXPECT_GE(pool.NumThreads(), 1u);
}

// Test: WaitAll on empty pool returns immediately (no pending tasks)
TEST(ThreadPool, WaitAll_EmptyPool_ReturnsImmediately) {
	ThreadPool pool(4);
	// Should not block
	pool.WaitAll();
	EXPECT_EQ(0u, pool.PendingTasks());
}

// Test: Stress - 10,000 tasks all complete correctly
TEST(ThreadPool, StressTest_ManyTasks_AllComplete) {
	ThreadPool pool(8);
	std::atomic<int> counter{0};

	std::vector<std::future<void>> futures;
	futures.reserve(10000);
	for (int i = 0; i < 10000; ++i) {
		futures.push_back(pool.Enqueue([&counter]() {
			counter.fetch_add(1, std::memory_order_relaxed);
		}));
	}

	pool.WaitAll();
	EXPECT_EQ(10000, counter.load());
}

// Test: DistributeBatches with a single item
TEST(ThreadPool, DistributeBatches_SingleItem) {
	std::vector<int> items = {42};
	auto batches = DistributeBatches(items, 4);

	EXPECT_EQ(4u, batches.size());
	EXPECT_EQ(1u, batches[0].size());
	EXPECT_EQ(42, batches[0][0]);
	for (size_t i = 1; i < 4; ++i) {
		EXPECT_TRUE(batches[i].empty());
	}
}

// Test: DistributeBatches with a single batch - all items in one batch
TEST(ThreadPool, DistributeBatches_SingleBatch_AllItemsInOne) {
	std::vector<int> items = {1, 2, 3, 4, 5};
	auto batches = DistributeBatches(items, 1);

	EXPECT_EQ(1u, batches.size());
	EXPECT_EQ(5u, batches[0].size());
	EXPECT_EQ(items, batches[0]);
}

// Test: DistributeBatches preserves order within each batch
TEST(ThreadPool, DistributeBatches_OrderPreservation) {
	// items: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
	// 3 batches (round-robin): [0,3,6,9], [1,4,7], [2,5,8]
	std::vector<int> items = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	auto batches = DistributeBatches(items, 3);

	EXPECT_EQ(3u, batches.size());

	// Batch 0: items at positions 0, 3, 6, 9 -> values 0, 3, 6, 9
	ASSERT_EQ(4u, batches[0].size());
	EXPECT_EQ(0, batches[0][0]);
	EXPECT_EQ(3, batches[0][1]);
	EXPECT_EQ(6, batches[0][2]);
	EXPECT_EQ(9, batches[0][3]);

	// Batch 1: items at positions 1, 4, 7 -> values 1, 4, 7
	ASSERT_EQ(3u, batches[1].size());
	EXPECT_EQ(1, batches[1][0]);
	EXPECT_EQ(4, batches[1][1]);
	EXPECT_EQ(7, batches[1][2]);

	// Batch 2: items at positions 2, 5, 8 -> values 2, 5, 8
	ASSERT_EQ(3u, batches[2].size());
	EXPECT_EQ(2, batches[2][0]);
	EXPECT_EQ(5, batches[2][1]);
	EXPECT_EQ(8, batches[2][2]);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
