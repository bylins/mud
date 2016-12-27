#include <gtest/gtest.h>

#include "bonus.command.parser.hpp"

const int DEFAULT_DURATION = 12;
const Bonus::EBonusType DEFAULT_TYPE = Bonus::BONUS_DAMAGE;

TEST(Bonus_Command_Parser_Negative, NoArguments)
{
	Bonus::ArgumentsParser parser("", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_ERROR, parser.result());
	EXPECT_EQ(false, parser.error_message().empty());
	EXPECT_EQ(true, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser_Negative, OnlyType_Weapon)
{
	Bonus::ArgumentsParser parser("оружейный", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_ERROR, parser.result());
	EXPECT_EQ(false, parser.error_message().empty());
	EXPECT_EQ(true, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser_Negative, OnlyType_Experience)
{
	Bonus::ArgumentsParser parser("опыт", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_ERROR, parser.result());
	EXPECT_EQ(false, parser.error_message().empty());
	EXPECT_EQ(true, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser_Negative, OnlyType_Damage)
{
	Bonus::ArgumentsParser parser("урон", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_ERROR, parser.result());
	EXPECT_EQ(false, parser.error_message().empty());
	EXPECT_EQ(true, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser_Negative, OnlyDuration_OutOfRange)
{
	Bonus::ArgumentsParser parser("100500", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_ERROR, parser.result());
	EXPECT_EQ(false, parser.error_message().empty());
	EXPECT_EQ(true, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser_Negative, OnlyDuration_InRange)
{
	Bonus::ArgumentsParser parser("30", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_ERROR, parser.result());
	EXPECT_EQ(false, parser.error_message().empty());
	EXPECT_EQ(true, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser_Negative, DurationOutOfRange_Negative)
{
	Bonus::ArgumentsParser parser("двойной оружейный -1", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_ERROR, parser.result());
	EXPECT_EQ(false, parser.error_message().empty());
	EXPECT_EQ(true, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser_Negative, DurationOutOfRange_Big)
{
	Bonus::ArgumentsParser parser("тройной оружейный 100500", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_ERROR, parser.result());
	EXPECT_EQ(false, parser.error_message().empty());
	EXPECT_EQ(true, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser_Negative, DurationOutOfRange_LowerBound)
{
	Bonus::ArgumentsParser parser("двойной оружейный 0", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_ERROR, parser.result());
	EXPECT_EQ(false, parser.error_message().empty());
	EXPECT_EQ(true, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser_Negative, DurationOutOfRange_UpperBound)
{
	Bonus::ArgumentsParser parser("двойной оружейный 61", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_ERROR, parser.result());
	EXPECT_EQ(false, parser.error_message().empty());
	EXPECT_EQ(true, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser, OnlyMultiplier_Double)
{
	Bonus::ArgumentsParser parser("двойной", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_START, parser.result());
	EXPECT_EQ(2, parser.multiplier());
	EXPECT_EQ(DEFAULT_TYPE, parser.type());
	EXPECT_EQ(DEFAULT_DURATION, parser.time());
	EXPECT_EQ(true, parser.error_message().empty());
	EXPECT_EQ(false, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser, OnlyMultiplier_Triple)
{
	Bonus::ArgumentsParser parser("тройной", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_START, parser.result());
	EXPECT_EQ(3, parser.multiplier());
	EXPECT_EQ(DEFAULT_TYPE, parser.type());
	EXPECT_EQ(DEFAULT_DURATION, parser.time());
	EXPECT_EQ(true, parser.error_message().empty());
	EXPECT_EQ(false, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser, OnlyMultiplier_DoubleWeapon)
{
	Bonus::ArgumentsParser parser("двойной оружейный", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_START, parser.result());
	EXPECT_EQ(2, parser.multiplier());
	EXPECT_EQ(Bonus::BONUS_WEAPON_EXP, parser.type());
	EXPECT_EQ(DEFAULT_DURATION, parser.time());
	EXPECT_EQ(true, parser.error_message().empty());
	EXPECT_EQ(false, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser, OnlyMultiplier_TripleExperience)
{
	Bonus::ArgumentsParser parser("тройной опыт", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_START, parser.result());
	EXPECT_EQ(3, parser.multiplier());
	EXPECT_EQ(Bonus::BONUS_EXP, parser.type());
	EXPECT_EQ(DEFAULT_DURATION, parser.time());
	EXPECT_EQ(true, parser.error_message().empty());
	EXPECT_EQ(false, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser, OnlyMultiplier_TripleDamage)
{
	Bonus::ArgumentsParser parser("тройной урон", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_START, parser.result());
	EXPECT_EQ(3, parser.multiplier());
	EXPECT_EQ(Bonus::BONUS_DAMAGE, parser.type());
	EXPECT_EQ(DEFAULT_DURATION, parser.time());
	EXPECT_EQ(true, parser.error_message().empty());
	EXPECT_EQ(false, parser.broadcast_message().empty());
}

TEST(Bonus_Command_Parser, Double_Weapon_1)
{
	Bonus::ArgumentsParser parser("двойной оружейный 1", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_START, parser.result());
	EXPECT_EQ(true, parser.error_message().empty());
	EXPECT_EQ(false, parser.broadcast_message().empty());
	EXPECT_EQ(1, parser.time());
	EXPECT_EQ(2, parser.multiplier());
	EXPECT_EQ(Bonus::BONUS_WEAPON_EXP, parser.type());
}

TEST(Bonus_Command_Parser, Double_Experience_5)
{
	Bonus::ArgumentsParser parser("двойной опыт 5", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_START, parser.result());
	EXPECT_EQ(true, parser.error_message().empty());
	EXPECT_EQ(false, parser.broadcast_message().empty());
	EXPECT_EQ(5, parser.time());
	EXPECT_EQ(2, parser.multiplier());
	EXPECT_EQ(Bonus::BONUS_EXP, parser.type());
}

TEST(Bonus_Command_Parser, Triple_Damage_60)
{
	Bonus::ArgumentsParser parser("тройной урон 60", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_START, parser.result());
	EXPECT_EQ(true, parser.error_message().empty());
	EXPECT_EQ(false, parser.broadcast_message().empty());
	EXPECT_EQ(60, parser.time());
	EXPECT_EQ(3, parser.multiplier());
	EXPECT_EQ(Bonus::BONUS_DAMAGE, parser.type());
}

TEST(Bonus_Command_Parser, Cancel)
{
	Bonus::ArgumentsParser parser("отменить", DEFAULT_TYPE, DEFAULT_DURATION);

	parser.parse();

	ASSERT_EQ(Bonus::ArgumentsParser::ER_STOP, parser.result());
	EXPECT_EQ(true, parser.error_message().empty());
	EXPECT_EQ(false, parser.broadcast_message().empty());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
