// Класс оружия хранится в spec_param прототипа и печатается в "познать" строкой
// 'Принадлежит к классу "..."'.
//
// Проверка там отсекала только ESkill::kUndefined (-1). А у оружия, которому класс не
// задали вовсе, spec_param равен нулю -- и ноль это ESkill::kGlobalCooldown, служебное
// умение с именем "!cooldown". Игрок читал 'Принадлежит к классу "!cooldown"', а
// заготовленное сообщение про неизвестный тип оружия не показывалось никогда.
//
// Тесты держат границу: классом оружия считается только настоящий класс.

#include "gameplay/skills/skills.h"

#include <gtest/gtest.h>

TEST(WeaponClass, UnsetSpecParamIsNotAWeaponClass) {
	// Именно этот случай и утекал игроку: пустое поле читается как ноль.
	EXPECT_EQ(static_cast<int>(ESkill::kGlobalCooldown), 0);
	EXPECT_FALSE(IsWeaponSkill(static_cast<ESkill>(0)));
	EXPECT_FALSE(IsWeaponSkill(ESkill::kGlobalCooldown));
}

TEST(WeaponClass, UndefinedIsNotAWeaponClass) {
	EXPECT_FALSE(IsWeaponSkill(ESkill::kUndefined));
	EXPECT_FALSE(IsWeaponSkill(ESkill::kAny));
	EXPECT_FALSE(IsWeaponSkill(ESkill::kReligion));
}

TEST(WeaponClass, EveryRealWeaponClassIsAccepted) {
	for (const auto skill : {ESkill::kClubs, ESkill::kAxes, ESkill::kLongBlades,
							 ESkill::kShortBlades, ESkill::kNonstandart, ESkill::kTwohands,
							 ESkill::kPicks, ESkill::kSpades, ESkill::kBows}) {
		EXPECT_TRUE(IsWeaponSkill(skill)) << "умение " << static_cast<int>(skill);
	}
}

// Соседи по нумерации: kTrack стоит прямо перед kClubs, kSideAttack -- сразу за kSpades.
// Если группу однажды расширят числовым диапазоном вместо перечисления, границы поедут
// именно здесь.
TEST(WeaponClass, NeighbouringSkillsAreNotWeaponClasses) {
	EXPECT_FALSE(IsWeaponSkill(ESkill::kTrack));
	EXPECT_FALSE(IsWeaponSkill(ESkill::kSideAttack));
	EXPECT_FALSE(IsWeaponSkill(ESkill::kHide));
	EXPECT_FALSE(IsWeaponSkill(ESkill::kRepair));
	EXPECT_FALSE(IsWeaponSkill(ESkill::kAddshot));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
