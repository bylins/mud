#include <utils.string.hpp>

#include <gtest/gtest.h>

struct
{
	const char* data;
	const char* result;
	const char* description;
} test_data[] = 
{
	{"", "", "empty string"},
	{"simple string", "simple string", "simple string"},
	{"&a&b&c", "", "just colors"},
	{"&asimple&b &cstring", "simple string", "mixed"},
	{"simple string&", "simple string&", "amp at the tail"},
	{"&&&&&&", "", "amps"}
};

TEST(Utils_String, ProcedureNullTerminated_Null)
{
	EXPECT_NO_FATAL_FAILURE(utils::remove_colors(nullptr));
}

TEST(Utils_String, ProcedureNullTerminated)
{
	for (const auto& test : test_data)
	{
		utils::shared_string_ptr string(strdup(test.data), free);
		EXPECT_NO_FATAL_FAILURE(utils::remove_colors(string.get()));
		EXPECT_EQ(0, strcmp(test.result, string.get()))
			<< "Failed test case '" << test.description << "'";
	}
}

TEST(Utils_String, ProcedureStdString)
{
	for (const auto& test : test_data)
	{
		std::string string = test.data;
		EXPECT_NO_FATAL_FAILURE(utils::remove_colors(string));
		EXPECT_EQ(test.result, string)
			<< "Failed test case '" << test.description << "'";
	}
}

TEST(Utils_String, FunctionNullTerminated_Null)
{
	EXPECT_EQ(nullptr, utils::get_string_without_colors(nullptr));
}

TEST(Utils_String, FunctionNullTerminated)
{
	for (const auto& test : test_data)
	{
		EXPECT_EQ(0, strcmp(test.result, utils::get_string_without_colors(test.data).get()))
			<< "Failed test case '" << test.description << "'";
	}
}

TEST(Utils_String, FunctionStdString)
{
	for (const auto& test : test_data)
	{
		std::string string = test.data;
		EXPECT_EQ(test.result, utils::get_string_without_colors(string))
			<< "Failed test case '" << test.description << "'";
	}
}

