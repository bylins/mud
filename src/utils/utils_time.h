#if !defined __TIME_UTILS_HPP__
#define __TIME_UTILS_HPP__

#include <chrono>
#include <functional>
#include <memory>
#include <list>
#include <string>

#define LOAD_LOG_FOLDER "log/"
#define LOAD_LOG_FILE "profiler.log"

namespace utils {
class CExecutionTimer {
 public:
	using call_t = std::function<void()>;
	using clock_t = std::chrono::high_resolution_clock;
	using duration_t = std::chrono::duration<double>;

	CExecutionTimer() { start(); }
	void restart() { start(); }
	duration_t delta() const;
	const auto &start_time() const { return m_start_time; }

 private:
	void start() { m_start_time = clock_t::now(); }
	clock_t::time_point m_start_time;
};

inline CExecutionTimer::duration_t CExecutionTimer::delta() const {
	const auto stop_time = clock_t::now();
	return std::chrono::duration_cast<duration_t>(stop_time - m_start_time);
}

class CSteppedProfiler {
 public:
	class CExecutionStepProfiler {
	 public:
		CExecutionStepProfiler(const std::string &name) : m_name(name), m_stopped(false) {}
		const auto &name() const { return m_name; }
		void stop();
		const auto &duration() const { return m_duration; }
		bool operator<(const CExecutionStepProfiler &right) const { return duration() > right.duration(); }

	 private:
		std::string m_name;
		CExecutionTimer::duration_t m_duration;
		CExecutionTimer m_timer;
		bool m_stopped;
	};

	using step_t = std::shared_ptr<CExecutionStepProfiler>;

	CSteppedProfiler(const std::string &scope_name, const double time_probe = 0) : m_scope_name(scope_name), m_time_probe(time_probe) {}
	~CSteppedProfiler();
	void next_step(const std::string &step_name);

	static bool step_t__less(const step_t &left, const step_t &right) { return left->operator<(*right); }

 private:
	void report() const;

	const std::string m_scope_name;
	const double m_time_probe;
	std::list<step_t> m_steps;
	CExecutionTimer m_timer;
};

}

#endif // __TIME_UTILS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
