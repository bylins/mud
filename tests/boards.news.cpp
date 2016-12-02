#include <gtest/gtest.h>

#include <boards.h>

const char* NEWS_BOARD_FILE_NAME = "data/boards/news.catcat";

class Boards_News : public ::testing::Test
{
protected:
	virtual void SetUp() override
	{
		// before implementing this test, we need to get rid of using global variables.
	}

private:
	Boards::Board::shared_ptr m_board;
};