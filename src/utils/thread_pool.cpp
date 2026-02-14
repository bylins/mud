// Part of Bylins http://www.mud.ru
// Thread pool implementation

#include "thread_pool.h"

namespace utils {

ThreadPool::ThreadPool(size_t num_threads)
	: m_stop(false), m_active_tasks(0) {

	if (num_threads == 0) {
		num_threads = std::thread::hardware_concurrency();
		if (num_threads == 0) {
			num_threads = 1;  // Fallback if hardware_concurrency fails
		}
	}

	m_workers.reserve(num_threads);
	for (size_t i = 0; i < num_threads; ++i) {
		m_workers.emplace_back([this] { WorkerThread(); });
	}
}

ThreadPool::~ThreadPool() {
	{
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		m_stop = true;
	}

	m_condition.notify_all();

	for (std::thread& worker : m_workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
}

void ThreadPool::WorkerThread() {
	while (true) {
		std::function<void()> task;

		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			m_condition.wait(lock, [this] {
				return m_stop || !m_tasks.empty();
			});

			if (m_stop && m_tasks.empty()) {
				return;
			}

			if (!m_tasks.empty()) {
				task = std::move(m_tasks.front());
				m_tasks.pop();
			}
		}

		if (task) {
			task();

			// Decrement active tasks and notify WaitAll
			m_active_tasks--;
			if (m_active_tasks == 0) {
				std::lock_guard<std::mutex> lock(m_done_mutex);
				m_all_done.notify_all();
			}
		}
	}
}

void ThreadPool::WaitAll() {
	std::unique_lock<std::mutex> lock(m_done_mutex);
	m_all_done.wait(lock, [this] {
		return m_active_tasks == 0;
	});
}

size_t ThreadPool::PendingTasks() const {
	std::lock_guard<std::mutex> lock(m_queue_mutex);
	return m_tasks.size();
}

} // namespace utils

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
