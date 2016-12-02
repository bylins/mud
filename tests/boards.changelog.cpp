#include <gtest/gtest.h>

#include <boards.changelog.loaders.hpp>
#include <boards.types.hpp>

#include <fstream>

const char* CHANGELOG_FILE_NAME = "data/boards/changelog.sample";

constexpr int EXPECTED_NUMBER_OF_COMMITS = 7;

constexpr int EXPECTED_NUMBER_OF_THE_FIRST_COMMIT = 0;
const char* EXPECTED_AUTHOR_OF_THE_LAST_COMMIT = "First";
constexpr time_t EXPECTED_DATE_OF_THE_FIRST_COMMIT = 1113343283ll;	// Tue Apr 12 17:01:23 2005

class Boards_Changelog : public ::testing::Test
{
protected:
	virtual void SetUp() override;

	const auto& board() const { return m_coder_board; }

private:
	Boards::Board::shared_ptr m_coder_board;
	Boards::ChangeLogLoader::shared_ptr m_changelog_loader;
};

void Boards_Changelog::SetUp()
{
	std::ifstream ifs;
	ifs.open(CHANGELOG_FILE_NAME);
	ASSERT_EQ(true, ifs.good()) << "Couldn't open sample changelog file.";

	m_coder_board = std::make_shared<Boards::Board>(Boards::CODER_BOARD);
	m_changelog_loader = Boards::ChangeLogLoader::create("git", m_coder_board);
	ASSERT_EQ(true, m_changelog_loader->load(ifs)) << "Couldn't load sample changelog data.";

	ASSERT_EQ(EXPECTED_NUMBER_OF_COMMITS, m_coder_board->messages_count())
		<< "Unexpected number of commit messages in sample changelog file.";
}

TEST_F(Boards_Changelog, GettingByIndex_0)
{
	const auto message0 = board()->get_message(0);
	EXPECT_EQ(true, bool(message0)) << "Could not get message with index 0.";
	EXPECT_EQ(EXPECTED_NUMBER_OF_THE_FIRST_COMMIT, message0->num);
	EXPECT_EQ(std::string(EXPECTED_AUTHOR_OF_THE_LAST_COMMIT), message0->author);
	EXPECT_EQ(EXPECTED_DATE_OF_THE_FIRST_COMMIT, message0->date);
}

TEST_F(Boards_Changelog, CheckIndexes)
{
	for (auto i = 0; i < board()->messages_count(); ++i)
	{
		const auto message = board()->get_message(i);

		EXPECT_EQ(true, bool(message))
			<< "Message #" << i << " was not obtained.";

		EXPECT_EQ(i, message->num);
	}
}

TEST_F(Boards_Changelog, CheckOrder)
{
	const auto first_message = board()->get_message(0);
	EXPECT_EQ(true, bool(first_message))
		<< "Couldn't get first message.";
	auto newer_date = first_message->date;

	for (auto i = 1; i < board()->messages_count(); ++i)
	{
		const auto message = board()->get_message(i);

		EXPECT_NE(false, bool(message))
			<< "Message #" << i << " was not obtained.";

		const auto older_date = message->date;
		ASSERT_GE(newer_date, older_date)
			<< "Changelog messages are not in descending order.";
		newer_date = older_date;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
