// Имена апплаев в триггерах: подстановки вроде %actor.applyvalue(*множитель.опыта)% пишут имя
// через точку, а в apply_types оно записано словами ("*множитель опыта"). Посимвольный поиск
// спотыкался на первом же разделителе, подстановка уходила пустой, и условие в триггере молча
// оказывалось ложным. Тест держит сцепку таблицы движка с тем, как её зовут из мира.

#include "gameplay/affects/affect_contants.h"
#include "utils/utils_parse.h"

#include <gtest/gtest.h>

TEST(Dg_ApplyNames, DottedName_ResolvesToApply) {
	// Ровно те два имени, на которых ругались наградные свитки опыта.
	EXPECT_EQ(EApply::kExpBonus, search_block_by_words("*множитель.опыта", apply_types));
	EXPECT_EQ(EApply::kExpPercent, search_block_by_words("*дополнительный.бонус.опыта", apply_types));
	EXPECT_EQ(EApply::kSkillsBonus, search_block_by_words("бонус.ко.всем.умениям", apply_types));
}

TEST(Dg_ApplyNames, SpacedName_StillResolves) {
	EXPECT_EQ(EApply::kExpBonus, search_block_by_words("*множитель опыта", apply_types));
	EXPECT_EQ(EApply::kSkillsBonus, search_block_by_words("бонус ко всем умениям", apply_types));
}

TEST(Dg_ApplyNames, AbbreviatedName_StillResolves) {
	// Так имя пишет dgaffect в тех же свитках: одно слово вместо двух.
	EXPECT_EQ(EApply::kExpBonus, search_block_by_words("*множитель", apply_types));
}

TEST(Dg_ApplyNames, UnknownName_ReturnsMinusOne) {
	EXPECT_EQ(-1, search_block_by_words("множитель.опыта", apply_types));  // без звёздочки -- другое имя
	EXPECT_EQ(-1, search_block_by_words("несуществующий.апплай", apply_types));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
