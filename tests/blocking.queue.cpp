#include "engine/structs/blocking_queue.h"

#include <gtest/gtest.h>

#include <thread>

class BlockingQueue_F: public ::testing::Test
{
public:
	static constexpr int QUEUE_SIZE = 32;

	BlockingQueue_F();

	void supply(const int period);
	void consume(const int period);

protected:
	BlockingQueue<int> m_queue;
	std::stringstream m_input;
	std::stringstream m_output;
};

BlockingQueue_F::BlockingQueue_F() :
	m_queue(QUEUE_SIZE)
{
}

void BlockingQueue_F::supply(const int period)
{
	for (int i = 0; i != 12 * QUEUE_SIZE; ++i)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(period));

		if (!m_queue.push(i))
		{
			break;
		}

		m_input << i;
	}
}

void BlockingQueue_F::consume(const int period)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(period));

	int value = 0;
	for (int i = 0; i != 12 * QUEUE_SIZE; ++i)
	{
		if (!m_queue.pop(value))
		{
			break;
		}

		m_output << value;

		std::this_thread::sleep_for(std::chrono::milliseconds(period));
	}
}

constexpr int BlockingQueue_F::QUEUE_SIZE;

TEST_F(BlockingQueue_F, FastSupply)
{
	std::thread supply(&BlockingQueue_F::supply, this, 0);
	std::thread consume(&BlockingQueue_F::consume, this, 2);

	supply.join();
	consume.join();

	EXPECT_EQ(m_input.str(), m_output.str());
}

TEST_F(BlockingQueue_F, SlowSupply)
{
	std::thread supply(&BlockingQueue_F::supply, this, 2);
	std::thread consume(&BlockingQueue_F::consume, this, 0);

	supply.join();
	consume.join();

	EXPECT_EQ(m_input.str(), m_output.str());
}

TEST_F(BlockingQueue_F, EvenSupply)
{
	std::thread supply(&BlockingQueue_F::supply, this, 2);
	std::thread consume(&BlockingQueue_F::consume, this, 2);

	supply.join();
	consume.join();

	EXPECT_EQ(m_input.str(), m_output.str());
}

TEST_F(BlockingQueue_F, SupplyDestroy)
{
	std::thread supply(&BlockingQueue_F::supply, this, 2);
	std::this_thread::sleep_for(std::chrono::milliseconds(12));

	m_queue.destroy();

	supply.join();
}

TEST_F(BlockingQueue_F, ConsumeDestroy)
{
	std::thread consume(&BlockingQueue_F::consume, this, 2);
	std::this_thread::sleep_for(std::chrono::milliseconds(12));

	m_queue.destroy();

	consume.join();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
