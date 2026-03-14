#ifndef __DEBUG_UTILS_HPP__
#define __DEBUG_UTILS_HPP__

#include <string>
#include <list>
#include <unordered_map>

namespace debug {
class LogQueue {
 public:
	constexpr static std::size_t QUEUE_SIZE = 100;

	void push(const std::string &s);
	std::ostream &print(std::ostream &os) const;
	void print_queue(std::ostream &os, const std::string &key) const;
	auto size() const { return m_queue.size(); }
	auto begin() const { return m_queue.begin(); }
	auto end() const { return m_queue.end(); }

 private:
	using queue_t = std::list<std::string>;

	queue_t m_queue;
};

inline std::ostream &operator<<(std::ostream &os, const LogQueue &queue) { return queue.print(os); }

using log_queues_t = std::unordered_map<std::string, LogQueue>;

log_queues_t &log_queues();
LogQueue &log_queue(const std::string &key);
}

#endif // __DEBUG_UTILS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
