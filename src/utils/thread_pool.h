// Part of Bylins http://www.mud.ru
// Thread pool for parallel task execution

#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

namespace utils {

/**
 * Thread pool for executing tasks in parallel.
 *
 * Usage:
 *   ThreadPool pool(4);  // Create pool with 4 threads
 *   auto future = pool.Enqueue([](){ do_work(); });
 *   future.wait();  // Wait for task completion
 *   pool.WaitAll();  // Wait for all tasks to complete
 */
class ThreadPool {
public:
	/**
	 * Create thread pool with specified number of worker threads.
	 * @param num_threads Number of worker threads (default: hardware_concurrency)
	 */
	explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());

	/**
	 * Destructor - waits for all tasks to complete and joins all threads.
	 */
	~ThreadPool();

	// Non-copyable, non-movable
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	/**
	 * Enqueue a task for execution.
	 * @param task Callable object to execute
	 * @return Future that can be used to wait for task completion and get result
	 */
	template<typename Func>
	auto Enqueue(Func&& task) -> std::future<std::invoke_result_t<Func>>;

	/**
	 * Wait for all currently enqueued tasks to complete.
	 * Does not prevent new tasks from being added.
	 */
	void WaitAll();

	/**
	 * Get number of worker threads.
	 */
	size_t NumThreads() const { return m_workers.size(); }

	/**
	 * Get number of pending tasks in queue.
	 */
	size_t PendingTasks() const;

private:
	// Worker thread function
	void WorkerThread();

	// Worker threads
	std::vector<std::thread> m_workers;

	// Task queue
	std::queue<std::function<void()>> m_tasks;

	// Synchronization
	mutable std::mutex m_queue_mutex;
	std::condition_variable m_condition;
	std::atomic<bool> m_stop;

	// Track active tasks for WaitAll
	std::atomic<size_t> m_active_tasks;
	std::condition_variable m_all_done;
	std::mutex m_done_mutex;
};

// Template implementation
template<typename Func>
auto ThreadPool::Enqueue(Func&& task) -> std::future<std::invoke_result_t<Func>> {
	using return_type = std::invoke_result_t<Func>;

	auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(std::forward<Func>(task));
	std::future<return_type> result = task_ptr->get_future();

	{
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		if (m_stop) {
			throw std::runtime_error("Enqueue on stopped ThreadPool");
		}
		m_active_tasks++;
		m_tasks.emplace([task_ptr]() { (*task_ptr)(); });
	}

	m_condition.notify_one();
	return result;
}

/**
 * Distribute items into N batches (round-robin).
 * @param items Vector of items to distribute
 * @param num_batches Number of batches to create
 * @return Vector of batches, each batch is a vector of items
 */
template<typename T>
std::vector<std::vector<T>> DistributeBatches(const std::vector<T>& items, size_t num_batches) {
	if (num_batches == 0) {
		num_batches = 1;
	}

	std::vector<std::vector<T>> batches(num_batches);

	// Round-robin distribution for load balancing
	for (size_t i = 0; i < items.size(); ++i) {
		batches[i % num_batches].push_back(items[i]);
	}

	return batches;
}

} // namespace utils

#endif // THREAD_POOL_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
