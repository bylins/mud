#include <gtest/gtest.h>

#include "engine/structs/structs.h"
#include "utils/utils.h"
#include "utils/utils_string.h"

#define PIECE_A_OLD	"   Вели"
#define PIECE_A_NEW	"   Вели"
#define PIECE_B_OLD	"редчайшими"
#define PIECE_B_NEW	"bla-bla-bla"
#define PIECE_C_OLD	"зверски\nизрубленного "
#define PIECE_C_NEW	"bla-bla-bla"
#define PIECE_D_OLD	"самобранкой."
#define PIECE_D_NEW	"tail"
#define SAMPLE_PIECE PIECE_A_OLD "чественные мраморные колонны уходят ввысь, подпирая самое небо.  На стенах,\n" \
	"отделанных " PIECE_B_OLD " сортами мрамора красуются барельефы, изображающие бытие\n" \
	"обитателей замка - охоту на Жар - Птицу, воскрешение Змея Горыныча, " PIECE_C_OLD \
	"Ильей Муромцем после очередной попойки, пленение Змея Тугарина и\n" \
	"многое другое.В центре зала - огромный дубовый стол, покрытый скатертью - " PIECE_D_OLD
#define SAMPLE_PIECE_R1 PIECE_A_NEW "чественные мраморные колонны уходят ввысь, подпирая самое небо.  На стенах,\n" \
	"отделанных " PIECE_B_OLD " сортами мрамора красуются барельефы, изображающие бытие\n" \
	"обитателей замка - охоту на Жар - Птицу, воскрешение Змея Горыныча, " PIECE_C_OLD \
	"Ильей Муромцем после очередной попойки, пленение Змея Тугарина и\n" \
	"многое другое.В центре зала - огромный дубовый стол, покрытый скатертью - " PIECE_D_OLD
#define SAMPLE_PIECE_R2 PIECE_A_NEW "чественные мраморные колонны уходят ввысь, подпирая самое небо.  На стенах,\n" \
	"отделанных " PIECE_B_NEW " сортами мрамора красуются барельефы, изображающие бытие\n" \
	"обитателей замка - охоту на Жар - Птицу, воскрешение Змея Горыныча, " PIECE_C_OLD \
	"Ильей Муромцем после очередной попойки, пленение Змея Тугарина и\n" \
	"многое другое.В центре зала - огромный дубовый стол, покрытый скатертью - " PIECE_D_OLD
#define SAMPLE_PIECE_R3 PIECE_A_NEW "чественные мраморные колонны уходят ввысь, подпирая самое небо.  На стенах,\n" \
	"отделанных " PIECE_B_NEW " сортами мрамора красуются барельефы, изображающие бытие\n" \
	"обитателей замка - охоту на Жар - Птицу, воскрешение Змея Горыныча, " PIECE_C_NEW \
	"Ильей Муромцем после очередной попойки, пленение Змея Тугарина и\n" \
	"многое другое.В центре зала - огромный дубовый стол, покрытый скатертью - " PIECE_D_OLD
#define SAMPLE_PIECE_R4 PIECE_A_NEW "чественные мраморные колонны уходят ввысь, подпирая самое небо.  На стенах,\n" \
	"отделанных " PIECE_B_NEW " сортами мрамора красуются барельефы, изображающие бытие\n" \
	"обитателей замка - охоту на Жар - Птицу, воскрешение Змея Горыныча, " PIECE_C_NEW \
	"Ильей Муромцем после очередной попойки, пленение Змея Тугарина и\n" \
	"многое другое.В центре зала - огромный дубовый стол, покрытый скатертью - " PIECE_D_NEW

#define SAMPLE_NUMBERS_PIECE "123456789012345678901"
#define SAMPLE_NUMBERS_PATTERN_PIECE_FROM_1 "1"
#define SAMPLE_NUMBERS_PATTERN_PIECE_TO_1 "a"
#define SAMPLE_NUMBERS_PIECE_R1 "a234567890a234567890a"
#define SAMPLE_NUMBERS_PATTERN_PIECE_FROM_2 "2"
#define SAMPLE_NUMBERS_PATTERN_PIECE_TO_2 "bbbb"

const char* SAMPLE_STRING = SAMPLE_PIECE;
const char* SAMPLE_STRING_R1 = SAMPLE_PIECE_R1;
const char* SAMPLE_STRING_R2 = SAMPLE_PIECE_R2;
const char* SAMPLE_STRING_R3 = SAMPLE_PIECE_R3;
const char* SAMPLE_STRING_R4 = SAMPLE_PIECE_R4;
const char* DOUBLE_SAMPLE_STRING = SAMPLE_PIECE SAMPLE_PIECE;

const char* SAMPLE_NUMBERS_STRING = SAMPLE_NUMBERS_PIECE;

const char* SAMPLE_NUMBERS_PATTERN_FROM_1 = SAMPLE_NUMBERS_PATTERN_PIECE_FROM_1;
const char* SAMPLE_NUMBERS_PATTERN_TO_1 = SAMPLE_NUMBERS_PATTERN_PIECE_TO_1;
const int SAMPLE_NUMBERS_SUBSTRINGS_COUNT = 3;
const char* SAMPLE_NUMBERS_STRING_R1 = SAMPLE_NUMBERS_PIECE_R1;

const char* SAMPLE_NUMBERS_PATTERN_FROM_2 = SAMPLE_NUMBERS_PATTERN_PIECE_FROM_2;
const char* SAMPLE_NUMBERS_PATTERN_TO_2 = SAMPLE_NUMBERS_PATTERN_PIECE_TO_2;
const char* SAMPLE_NUMBERS_STRING_R2 = SAMPLE_NUMBERS_PIECE;

utils::AbstractStringWriter::shared_ptr create_delegated_string_writer(char*& delegated_string)
{
	const auto result = utils::AbstractStringWriter::shared_ptr(new utils::DelegatedStringWriter(delegated_string));
	return result;
}

utils::AbstractStringWriter::shared_ptr create_std_string_writer()
{
	const auto result = utils::AbstractStringWriter::shared_ptr(new utils::StdStringWriter());
	return result;
}

TEST(Utils_Editor_DelegatedStringWriter, Set)
{
	char* string = nullptr;
	const auto writer = create_delegated_string_writer(string);
	writer->set_string(SAMPLE_STRING);

	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_STRING));
	EXPECT_EQ(0, strcmp(string, SAMPLE_STRING));

	// Explicitly free memory since DelegatedStringWriter destructor no longer does it
	free(string);
}

TEST(Utils_Editor_StdStringWriter, Set)
{
	const auto writer = create_std_string_writer();
	writer->set_string(SAMPLE_STRING);

	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_STRING));
}

TEST(Utils_Editor_DelegatedStringWriter, AppendToEmpty)
{
	char* string = nullptr;
	const auto writer = create_delegated_string_writer(string);
	writer->append_string(SAMPLE_STRING);

	const auto equality = strcmp(writer->get_string(), SAMPLE_STRING);
	EXPECT_EQ(0, equality);

	// Explicitly free memory since DelegatedStringWriter destructor no longer does it
	free(string);
}

TEST(Utils_Editor_StdStringWriter, AppendToEmpty)
{
	const auto writer = create_std_string_writer();
	writer->append_string(SAMPLE_STRING);

	const auto equality = strcmp(writer->get_string(), SAMPLE_STRING);
	EXPECT_EQ(0, equality);
}

TEST(Utils_Editor_DelegatedStringWriter, Append)
{
	char* string = nullptr;
	const auto writer = create_delegated_string_writer(string);
	writer->set_string(SAMPLE_STRING);
	writer->append_string(SAMPLE_STRING);

	EXPECT_EQ(0, strcmp(writer->get_string(), DOUBLE_SAMPLE_STRING));

	// Explicitly free memory since DelegatedStringWriter destructor no longer does it
	free(string);
}

TEST(Utils_Editor_StdStringWriter, Append)
{
	const auto writer = create_std_string_writer();
	writer->set_string(SAMPLE_STRING);
	writer->append_string(SAMPLE_STRING);

	EXPECT_EQ(0, strcmp(writer->get_string(), DOUBLE_SAMPLE_STRING));
}

TEST(Utils_Editor_DelegatedStringWriter, Length)
{
	char* string = nullptr;
	const auto writer = create_delegated_string_writer(string);
	writer->set_string(SAMPLE_STRING);

	EXPECT_EQ(strlen(SAMPLE_STRING), writer->length());

	// Explicitly free memory since DelegatedStringWriter destructor no longer does it
	free(string);
}

TEST(Utils_Editor_StdStringWriter, Length)
{
	const auto writer = create_std_string_writer();
	writer->set_string(SAMPLE_STRING);

	EXPECT_EQ(strlen(SAMPLE_STRING), writer->length());
}

TEST(Utils_Editor_DelegatedStringWriter, ReplaceOne)
{
	char* string = nullptr;
	const auto writer = create_delegated_string_writer(string);
	writer->set_string(SAMPLE_STRING);

	const auto replace_result1 = replace_str(writer, PIECE_A_OLD, PIECE_A_NEW, 0, kMaxStringLength);
	EXPECT_EQ(1, replace_result1);
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_PIECE_R1));

	const auto replace_result2 = replace_str(writer, PIECE_B_OLD, PIECE_B_NEW, 0, kMaxStringLength);
	EXPECT_EQ(1, replace_result2);
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_PIECE_R2));

	const auto replace_result3 = replace_str(writer, PIECE_C_OLD, PIECE_C_NEW, 0, kMaxStringLength);
	EXPECT_EQ(1, replace_result3);
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_PIECE_R3));

	const auto replace_result4 = replace_str(writer, PIECE_D_OLD, PIECE_D_NEW, 0, kMaxStringLength);
	EXPECT_EQ(1, replace_result4);
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_PIECE_R4));

	// Explicitly free memory since DelegatedStringWriter destructor no longer does it
	free(string);
}

TEST(Utils_Editor_StdStringWriter, ReplaceOne)
{
	const auto writer = create_std_string_writer();
	writer->set_string(SAMPLE_STRING);

	const auto replace_result1 = replace_str(writer, PIECE_A_OLD, PIECE_A_NEW, 0, kMaxStringLength);
	EXPECT_EQ(1, replace_result1);
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_PIECE_R1));

	const auto replace_result2 = replace_str(writer, PIECE_B_OLD, PIECE_B_NEW, 0, kMaxStringLength);
	EXPECT_EQ(1, replace_result2);
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_PIECE_R2));

	const auto replace_result3 = replace_str(writer, PIECE_C_OLD, PIECE_C_NEW, 0, kMaxStringLength);
	EXPECT_EQ(1, replace_result3);
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_PIECE_R3));

	const auto replace_result4 = replace_str(writer, PIECE_D_OLD, PIECE_D_NEW, 0, kMaxStringLength);
	EXPECT_EQ(1, replace_result4);
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_PIECE_R4));
}

TEST(Utils_Editor_DelegatedStringWriter, ReplaceMany)
{
	char* string = nullptr;
	const auto writer = create_delegated_string_writer(string);
	writer->set_string(SAMPLE_NUMBERS_STRING);
	const auto result = replace_str(writer, SAMPLE_NUMBERS_PATTERN_FROM_1, SAMPLE_NUMBERS_PATTERN_TO_1, 1, kMaxStringLength);

	EXPECT_EQ(SAMPLE_NUMBERS_SUBSTRINGS_COUNT, result);
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_NUMBERS_PIECE_R1));

	// Explicitly free memory since DelegatedStringWriter destructor no longer does it
	free(string);
}

TEST(Utils_Editor_StdStringWriter, ReplaceMany)
{
	const auto writer = create_std_string_writer();
	writer->set_string(SAMPLE_NUMBERS_STRING);
	const auto result = replace_str(writer, SAMPLE_NUMBERS_PATTERN_FROM_1, SAMPLE_NUMBERS_PATTERN_TO_1, 1, kMaxStringLength);

	EXPECT_EQ(SAMPLE_NUMBERS_SUBSTRINGS_COUNT, result);
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_NUMBERS_PIECE_R1));
}

TEST(Utils_Editor_DelegatedStringWriter, ReplaceManyOverflow)
{
	char* string = nullptr;
	const auto writer = create_delegated_string_writer(string);
	writer->set_string(SAMPLE_NUMBERS_STRING);

	const auto expected_result_length = strlen(SAMPLE_NUMBERS_PIECE);
	const auto result = replace_str(writer, SAMPLE_NUMBERS_PATTERN_FROM_2, SAMPLE_NUMBERS_PATTERN_TO_2, 1, static_cast<int>(expected_result_length));
	EXPECT_EQ(-1, result);
	EXPECT_EQ(expected_result_length, writer->length());
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_NUMBERS_PIECE));

	// Explicitly free memory since DelegatedStringWriter destructor no longer does it
	free(string);
}

TEST(Utils_Editor_StdStringWriter, ReplaceManyOverflow)
{
	const auto writer = create_std_string_writer();
	writer->set_string(SAMPLE_NUMBERS_STRING);

	const auto expected_result_length = strlen(SAMPLE_NUMBERS_PIECE);
	const auto result = replace_str(writer, SAMPLE_NUMBERS_PATTERN_FROM_2, SAMPLE_NUMBERS_PATTERN_TO_2, 1, static_cast<int>(expected_result_length));
	EXPECT_EQ(-1, result);
	EXPECT_EQ(expected_result_length, writer->length());
	EXPECT_EQ(0, strcmp(writer->get_string(), SAMPLE_NUMBERS_PIECE));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
