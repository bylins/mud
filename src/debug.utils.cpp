#include "debug.utils.hpp"

#include <iostream>

namespace debug
{
	void LogQueue::push(const std::string& s)
	{
		m_queue.push_front(s);
		if (QUEUE_SIZE < m_queue.size())
		{
			m_queue.pop_back();
		}
	}

	std::ostream& LogQueue::print(std::ostream& os) const
	{
		int number = 0;
		for (const auto& line : m_queue)
		{
			os << ++number << ". " << line << std::endl;
		}

		return os;
	}

	void LogQueue::print_queue(std::ostream& os, const std::string& key) const
	{
		os << "Queue '" << key << "':";
		if (0 == size())
		{
			os << " <empty>";
		}
		os << std::endl;
		os << *this;
	}

	log_queues_t& log_queues()
	{
		static log_queues_t log_queues;

		return log_queues;
	}

	LogQueue& log_queue(const std::string& key)
	{
		auto& queues = log_queues();

		return queues[key];
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
