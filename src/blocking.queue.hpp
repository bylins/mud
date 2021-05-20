#ifndef __BLOCKING_QUEUE_HPP__
#define __BLOCKING_QUEUE_HPP__

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

template <typename MessageType>
class BlockingQueue
{
public:
	BlockingQueue(const std::size_t size);
	~BlockingQueue();

	bool push(const MessageType& message);
	bool pop(MessageType& result);
	bool empty() const;
	bool full() const;

	void destroy();

private:
	using messages_t = std::queue<MessageType>;

	std::size_t m_max_size;

	mutable std::mutex m_messages_mutex;
	std::condition_variable m_new_message;
	std::condition_variable m_message_processed;
	messages_t m_messages;
	std::atomic_bool m_destroying;
};

template <typename MessageType>
BlockingQueue<MessageType>::BlockingQueue(const std::size_t size):
	m_max_size(size),
	m_destroying(false)
{
}

template <typename MessageType>
BlockingQueue<MessageType>::~BlockingQueue()
{
	destroy();
}

template <typename MessageType>
bool BlockingQueue<MessageType>::empty() const
{
	std::unique_lock<std::mutex> lock(m_messages_mutex);

	return m_messages.empty();
}

template <typename MessageType>
bool BlockingQueue<MessageType>::full() const
{
	std::unique_lock<std::mutex> lock(m_messages_mutex);

	return m_messages.size() == m_max_size;
}

template <typename MessageType>
void BlockingQueue<MessageType>::destroy()
{
	m_destroying = true;

	m_new_message.notify_all();
	m_message_processed.notify_all();
}

template <typename MessageType>
bool BlockingQueue<MessageType>::push(const MessageType& message)
{
	std::unique_lock<std::mutex> lock(m_messages_mutex);
	while (m_messages.size() >= m_max_size
		&& !m_destroying)
	{
		m_message_processed.wait(lock);
	}

	if (m_destroying)
	{
		return false;
	}

	m_messages.push(message);
	m_new_message.notify_one();
	return true;
}

template <typename MessageType>
bool BlockingQueue<MessageType>::pop(MessageType& result)
{
	std::unique_lock<std::mutex> lock(m_messages_mutex);
	while (m_messages.empty()
		&& !m_destroying)
	{
		m_new_message.wait(lock);
	}

	if (m_messages.empty())
	{
		return false;
	}

	result = m_messages.front();
	m_messages.pop();

	m_message_processed.notify_one();

	return true;
}

#endif // __BLOCKING_QUEUE_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
