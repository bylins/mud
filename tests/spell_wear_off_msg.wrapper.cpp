#include <gtest/gtest.h>

#include <spells.h>

namespace
{
	const spell_wear_off_msg_t test_array = {
		"message 1",
		"message 2",
		nullptr,
		"",
		"message 3"
	};

	constexpr std::size_t BUFFER_LENGTH = 128;
}

TEST(spell_wear_off_msg_wrapper, GettingExisingIndex)
{
	ASSERT_STREQ(test_array[0], "message 1");
	ASSERT_STREQ(test_array[1], "message 2");
	ASSERT_STREQ(test_array[3], "");
	ASSERT_STREQ(test_array[4], "message 3");
}

TEST(spell_wear_off_msg_wrapper, GettingOutOfBoundIndex)
{
	for (auto index = test_array.size(); index < test_array.size() + 2; ++index)
	{
		char expected[BUFFER_LENGTH] = {0};
		::snprintf(expected, BUFFER_LENGTH, spell_wear_off_msg_t::DEFAULT_MESSAGE, index);

		ASSERT_STREQ(test_array[index], expected);
	}
}

TEST(spell_wear_off_msg_wrapper, GettingExistingIndexWithNullValue)
{
	const std::size_t index = 2;
	char expected[BUFFER_LENGTH] = {0};
	::snprintf(expected, BUFFER_LENGTH, spell_wear_off_msg_t::DEFAULT_MESSAGE, index);

	ASSERT_STREQ(test_array[index], expected);
}
