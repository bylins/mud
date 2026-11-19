// Код возврата боевого триггера предмета (OTRIG_FIGHT) -- маска того, чего боец в этом раунде не
// делает: fight_otrigger собирает её по всем надетым и несомым предметам, а process_player_attack
// по ней решает, пускать ли отложенный каст и доп. атаки.
//
// Разбор #4141: и script_driver, и Lua-движок возвращают 1, когда триггер не сказал `return`
// явно. Пока kNoCastMagic стоял на нулевом бите, любой батл-триггер без `return 0` молча лишал
// владельца предмета возможности колдовать в бою. Поэтому нулевой бит теперь не занят, и этот
// тест держит его свободным.

#include "engine/scripting/dg_scripts.h"
#include "utils/utils.h"

#include <gtest/gtest.h>

namespace {

// то, что отдаёт триггер, в котором `return` не написан
constexpr Bitvector kDefaultTriggerReturn = 1;

}  // namespace

TEST(FightOtriggerReturnCodes, DefaultReturnForbidsNothing) {
	EXPECT_FALSE(IS_SET(kDefaultTriggerReturn, kNoCastMagic));
	EXPECT_FALSE(IS_SET(kDefaultTriggerReturn, kNoExtraAttack));
	EXPECT_FALSE(IS_SET(kDefaultTriggerReturn, kNoLeftHandAttack));
	EXPECT_FALSE(IS_SET(kDefaultTriggerReturn, kNoRightHandAttack));
}

TEST(FightOtriggerReturnCodes, ExplicitZeroForbidsNothing) {
	EXPECT_EQ(kNormalRound, 0u);
	EXPECT_FALSE(IS_SET(kNormalRound, kNoCastMagic));
	EXPECT_FALSE(IS_SET(kNormalRound, kNoExtraAttack));
	EXPECT_FALSE(IS_SET(kNormalRound, kNoLeftHandAttack));
	EXPECT_FALSE(IS_SET(kNormalRound, kNoRightHandAttack));
}

TEST(FightOtriggerReturnCodes, EachFlagIsItsOwnBit) {
	const Bitvector all = kNoCastMagic | kNoExtraAttack | kNoLeftHandAttack | kNoRightHandAttack;
	EXPECT_TRUE(IS_SET(all, kNoCastMagic));
	EXPECT_TRUE(IS_SET(all, kNoExtraAttack));
	EXPECT_TRUE(IS_SET(all, kNoLeftHandAttack));
	EXPECT_TRUE(IS_SET(all, kNoRightHandAttack));

	// ни один запрет не пересекается с другим и не задевает нулевой бит
	EXPECT_EQ(kNoCastMagic & kNoExtraAttack, 0u);
	EXPECT_EQ(kNoCastMagic & kNoLeftHandAttack, 0u);
	EXPECT_EQ(kNoCastMagic & kNoRightHandAttack, 0u);
	EXPECT_EQ(kNoExtraAttack & kNoLeftHandAttack, 0u);
	EXPECT_EQ(kNoExtraAttack & kNoRightHandAttack, 0u);
	EXPECT_EQ(kNoLeftHandAttack & kNoRightHandAttack, 0u);
	EXPECT_EQ(all & kDefaultTriggerReturn, 0u);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
