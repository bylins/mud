#include <gtest/gtest.h>

#include <telnet.h>
#include <msdp.senders.hpp>
#include <msdp.constants.hpp>

#include <memory>

class TestReporter : public ::msdp::AbstractReporter
{
public:
	TestReporter(const ::msdp::Variable::shared_ptr& value): m_value(value) {}

	virtual void get(::msdp::Variable::shared_ptr& response) override { response = m_value;  }

private:
	::msdp::Variable::shared_ptr m_value;
};

TEST(MSDP_Builder, NullValue)
{
	std::stringstream ss;
	::msdp::StreamSender sender(ss);

	::msdp::Variable::shared_ptr value;
	sender.send(std::make_shared<TestReporter>(value));

	EXPECT_TRUE(ss.str().empty());
}

TEST(MSDP_Builder, Variable)
{
	std::stringstream ss;
	::msdp::StreamSender sender(ss);

	::msdp::Variable::shared_ptr value = std::make_shared<::msdp::Variable>("name",
		std::make_shared<::msdp::StringValue>("value"));
	sender.send(std::make_shared<TestReporter>(value));

	const char EXPECTED[] =
	{
		char(IAC), char(SB), ::msdp::constants::TELOPT_MSDP,
		::msdp::MSDP_VAR, 'n', 'a', 'm', 'e',
		::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e',
		char(IAC), char(SE)
	};

	const std::string expected(EXPECTED, sizeof(EXPECTED));
	const auto actual = ss.str();
	EXPECT_EQ(expected, actual);
}

TEST(MSDP_Builder, EmptyArray)
{
	std::stringstream ss;
	::msdp::StreamSender sender(ss);

	::msdp::Variable::shared_ptr value = std::make_shared<::msdp::Variable>("array",
		std::make_shared<::msdp::ArrayValue>());
	sender.send(std::make_shared<TestReporter>(value));

	const char EXPECTED[] =
	{
		char(IAC), char(SB), ::msdp::constants::TELOPT_MSDP,
			::msdp::MSDP_VAR, 'a', 'r', 'r', 'a', 'y',
			::msdp::MSDP_VAL,
				::msdp::MSDP_ARRAY_OPEN,
				::msdp::MSDP_ARRAY_CLOSE,
		char(IAC), char(SE)
	};

	const std::string expected(EXPECTED, sizeof(EXPECTED));
	const auto actual = ss.str();
	EXPECT_EQ(expected, actual);
}

TEST(MSDP_Builder, NotEmptyArray_SingleElement)
{
	std::stringstream ss;
	::msdp::StreamSender sender(ss);

	const auto string = std::make_shared<::msdp::StringValue>("value");
	const auto array = std::make_shared<::msdp::ArrayValue>();
	array->add(string);
	::msdp::Variable::shared_ptr value = std::make_shared<::msdp::Variable>("array", array);
	sender.send(std::make_shared<TestReporter>(value));

	const char EXPECTED[] =
	{
		char(IAC), char(SB), ::msdp::constants::TELOPT_MSDP,
			::msdp::MSDP_VAR, 'a', 'r', 'r', 'a', 'y',
			::msdp::MSDP_VAL,
				::msdp::MSDP_ARRAY_OPEN,
					::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e',
				::msdp::MSDP_ARRAY_CLOSE,
		char(IAC), char(SE)
	};

	const std::string expected(EXPECTED, sizeof(EXPECTED));
	const auto actual = ss.str();
	EXPECT_EQ(expected, actual);
}

TEST(MSDP_Builder, NotEmptyArray)
{
	std::stringstream ss;
	::msdp::StreamSender sender(ss);

	const auto string1 = std::make_shared<::msdp::StringValue>("value1");
	const auto string2 = std::make_shared<::msdp::StringValue>("value2");
	
	const auto array = std::make_shared<::msdp::ArrayValue>();
	array->add(string1);
	array->add(string2);

	::msdp::Variable::shared_ptr value = std::make_shared<::msdp::Variable>("array", array);
	sender.send(std::make_shared<TestReporter>(value));

	const char EXPECTED[] =
	{
		char(IAC), char(SB), ::msdp::constants::TELOPT_MSDP,
			::msdp::MSDP_VAR, 'a', 'r', 'r', 'a', 'y',
			::msdp::MSDP_VAL,
				::msdp::MSDP_ARRAY_OPEN,
					::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '1',
					::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '2',
					::msdp::MSDP_ARRAY_CLOSE,
		char(IAC), char(SE)
	};

	const std::string expected(EXPECTED, sizeof(EXPECTED));
	const auto actual = ss.str();
	EXPECT_EQ(expected, actual);
}

TEST(MSDP_Builder, EmptyTable)
{
	std::stringstream ss;
	::msdp::StreamSender sender(ss);

	::msdp::Variable::shared_ptr value = std::make_shared<::msdp::Variable>("table",
		std::make_shared<::msdp::TableValue>());
	sender.send(std::make_shared<TestReporter>(value));

	const char EXPECTED[] =
	{
		char(IAC), char(SB), ::msdp::constants::TELOPT_MSDP,
			::msdp::MSDP_VAR, 't', 'a', 'b', 'l', 'e',
			::msdp::MSDP_VAL,
				::msdp::MSDP_TABLE_OPEN,
				::msdp::MSDP_TABLE_CLOSE,
		char(IAC), char(SE)
	};

	const std::string expected(EXPECTED, sizeof(EXPECTED));
	const auto actual = ss.str();
	EXPECT_EQ(expected, actual);
}

TEST(MSDP_Builder, NotEmptyTable_SingleElement)
{
	std::stringstream ss;
	::msdp::StreamSender sender(ss);

	const auto variable = std::make_shared<::msdp::Variable>("name",
		std::make_shared<::msdp::StringValue>("value"));
	const auto table = std::make_shared<::msdp::TableValue>();
	table->add(variable);

	::msdp::Variable::shared_ptr value = std::make_shared<::msdp::Variable>("table", table);
	sender.send(std::make_shared<TestReporter>(value));

	const char EXPECTED[] =
	{
		char(IAC), char(SB), ::msdp::constants::TELOPT_MSDP,
			::msdp::MSDP_VAR, 't', 'a', 'b', 'l', 'e',
			::msdp::MSDP_VAL,
				::msdp::MSDP_TABLE_OPEN,
					::msdp::MSDP_VAR, 'n', 'a', 'm', 'e',
					::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e',
				::msdp::MSDP_TABLE_CLOSE,
		char(IAC), char(SE)
	};

	const std::string expected(EXPECTED, sizeof(EXPECTED));
	const auto actual = ss.str();
	EXPECT_EQ(expected, actual);
}

TEST(MSDP_Builder, NotEmptyTable)
{
	std::stringstream ss;
	::msdp::StreamSender sender(ss);

	const auto table = std::make_shared<::msdp::TableValue>();
	const auto variable1 = std::make_shared<::msdp::Variable>("name1",
		std::make_shared<::msdp::StringValue>("value1"));
	table->add(variable1);
	const auto variable2 = std::make_shared<::msdp::Variable>("name2",
		std::make_shared<::msdp::StringValue>("value2"));
	table->add(variable2);

	::msdp::Variable::shared_ptr value = std::make_shared<::msdp::Variable>("table", table);
	sender.send(std::make_shared<TestReporter>(value));

	const char EXPECTED[] =
	{
		char(IAC), char(SB), ::msdp::constants::TELOPT_MSDP,
			::msdp::MSDP_VAR, 't', 'a', 'b', 'l', 'e',
			::msdp::MSDP_VAL,
				::msdp::MSDP_TABLE_OPEN,
					::msdp::MSDP_VAR, 'n', 'a', 'm', 'e', '1',
					::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '1',
					::msdp::MSDP_VAR, 'n', 'a', 'm', 'e', '2',
					::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '2',
					::msdp::MSDP_TABLE_CLOSE,
		char(IAC), char(SE)
	};

	const std::string expected(EXPECTED, sizeof(EXPECTED));
	const auto actual = ss.str();
	EXPECT_EQ(expected, actual);
}

TEST(MSDP_Builder, ArrayOfArrays)
{
	std::stringstream ss;
	::msdp::StreamSender sender(ss);

	const auto string1 = std::make_shared<::msdp::StringValue>("value1");
	const auto string2 = std::make_shared<::msdp::StringValue>("value2");

	const auto array = std::make_shared<::msdp::ArrayValue>();
	
	const auto nested_array1 = std::make_shared<::msdp::ArrayValue>();
	nested_array1->add(string1);
	nested_array1->add(string2);
	const auto nested_array2 = std::make_shared<::msdp::ArrayValue>();
	nested_array2->add(string2);
	nested_array2->add(string1);

	array->add(nested_array1);
	array->add(nested_array2);

	::msdp::Variable::shared_ptr value = std::make_shared<::msdp::Variable>("array", array);
	sender.send(std::make_shared<TestReporter>(value));

	const char EXPECTED[] =
	{
		char(IAC), char(SB), ::msdp::constants::TELOPT_MSDP,
			::msdp::MSDP_VAR, 'a', 'r', 'r', 'a', 'y',
			::msdp::MSDP_VAL,
				::msdp::MSDP_ARRAY_OPEN,
					::msdp::MSDP_VAL,
						::msdp::MSDP_ARRAY_OPEN,
							::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '1',
							::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '2',
						::msdp::MSDP_ARRAY_CLOSE,
					::msdp::MSDP_VAL,
						::msdp::MSDP_ARRAY_OPEN,
							::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '2',
							::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '1',
						::msdp::MSDP_ARRAY_CLOSE,
				::msdp::MSDP_ARRAY_CLOSE,
		char(IAC), char(SE)
	};

	const std::string expected(EXPECTED, sizeof(EXPECTED));
	const auto actual = ss.str();
	EXPECT_EQ(expected, actual);
}

TEST(MSDP_Builder, ArrayOfTables)
{
	std::stringstream ss;
	::msdp::StreamSender sender(ss);

	const auto variable1 = std::make_shared<::msdp::Variable>("name1",
		std::make_shared<::msdp::StringValue>("value1"));
	const auto variable2 = std::make_shared<::msdp::Variable>("name2",
		std::make_shared<::msdp::StringValue>("value2"));

	const auto array = std::make_shared<::msdp::ArrayValue>();

	const auto nested_table1 = std::make_shared<::msdp::TableValue>();
	nested_table1->add(variable1);
	nested_table1->add(variable2);
	const auto nested_table2 = std::make_shared<::msdp::TableValue>();
	nested_table2->add(variable2);
	nested_table2->add(variable1);

	array->add(nested_table1);
	array->add(nested_table2);

	::msdp::Variable::shared_ptr value = std::make_shared<::msdp::Variable>("array", array);
	sender.send(std::make_shared<TestReporter>(value));

	const char EXPECTED[] =
	{
		char(IAC), char(SB), ::msdp::constants::TELOPT_MSDP,
			::msdp::MSDP_VAR, 'a', 'r', 'r', 'a', 'y',
			::msdp::MSDP_VAL,
				::msdp::MSDP_ARRAY_OPEN,
					::msdp::MSDP_VAL,
						::msdp::MSDP_TABLE_OPEN,
							::msdp::MSDP_VAR, 'n', 'a', 'm', 'e', '1',
							::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '1',
							::msdp::MSDP_VAR, 'n', 'a', 'm', 'e', '2',
							::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '2',
						::msdp::MSDP_TABLE_CLOSE,
					::msdp::MSDP_VAL,
						::msdp::MSDP_TABLE_OPEN,
							::msdp::MSDP_VAR, 'n', 'a', 'm', 'e', '2',
							::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '2',
							::msdp::MSDP_VAR, 'n', 'a', 'm', 'e', '1',
							::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', '1',
						::msdp::MSDP_TABLE_CLOSE,
				::msdp::MSDP_ARRAY_CLOSE,
		char(IAC), char(SE)
	};

	const std::string expected(EXPECTED, sizeof(EXPECTED));
	const auto actual = ss.str();
	EXPECT_EQ(expected, actual);
}

/*
TODO: Implement following unit tests:
  * building response with table of arrays
  * building response with table of tables
  * building of mixed structure
*/

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
