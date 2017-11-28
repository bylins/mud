#include <gtest/gtest.h>

#include <telnet.h>
#include <msdp.parser.hpp>

TEST(MSDP_Parser, EmptyRequest)
{
	const char* request = "";

	size_t conversation_length = 0;
	::msdp::parsed_request_t result;

	EXPECT_EQ(true, !::msdp::parse_request(request, 0, conversation_length, result));
	EXPECT_EQ(0u, conversation_length);
}

TEST(MSDP_Parser, SimpleVariable_Tailless_TailNotRequired)
{
	const char request[] = {::msdp::MSDP_VAR, 'n', 'a', 'm', 'e', ::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e'};

	size_t conversation_length = 0;
	::msdp::parsed_request_t result;

	EXPECT_EQ(true, ::msdp::parse_request(request, sizeof(request), conversation_length, result, false));
	EXPECT_EQ(sizeof(request), conversation_length);
}

TEST(MSDP_Parser, SimpleVariable_Tailless_TailRequired)
{
	const char request[] = { ::msdp::MSDP_VAR, 'n', 'a', 'm', 'e', ::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e' };

	size_t conversation_length = 0;
	::msdp::parsed_request_t result;

	EXPECT_EQ(true, !::msdp::parse_request(request, sizeof(request), conversation_length, result, true));
	EXPECT_EQ(0u, conversation_length);
}

TEST(MSDP_Parser, SimpleVariable_WithTail_TailNotRequired)
{
	const char request[] = { ::msdp::MSDP_VAR, 'n', 'a', 'm', 'e', ::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', char(IAC), char(SE) };

	size_t conversation_length = 0;
	::msdp::parsed_request_t result;

	EXPECT_EQ(true, ::msdp::parse_request(request, sizeof(request), conversation_length, result, false));
	EXPECT_EQ(sizeof(request) - ::msdp::TAIL_LENGTH, conversation_length);
}

TEST(MSDP_Parser, SimpleVariable_WithTail_TailRequired)
{
	const char request[] = { ::msdp::MSDP_VAR, 'n', 'a', 'm', 'e', ::msdp::MSDP_VAL, 'v', 'a', 'l', 'u', 'e', char(IAC), char(SE) };

	size_t conversation_length = 0;
	::msdp::parsed_request_t result;

	EXPECT_EQ(true, ::msdp::parse_request(request, sizeof(request), conversation_length, result, true));
	EXPECT_EQ(sizeof(request), conversation_length);
}

TEST(MSDP_Parser, REPORT_ROOM)
{
	const char request[] = "\x01" "REPORT\x02" "ROOM\xff\xf0";

	size_t conversation_length = 0;
	::msdp::parsed_request_t result;

	EXPECT_EQ(true, ::msdp::parse_request(request, sizeof(request) - 1, conversation_length, result, true));
	EXPECT_EQ(sizeof(request) - 1, conversation_length);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
