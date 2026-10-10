// Частичная защита, которую игра называет "немного" (уклонился, отклонил, отразил), должна
// срезать урон до двух третей, а не обнулять его. Рефакторинг #2764 переписал `dam = dam * 10 / 15`
// как `dam *= 10 / 15`, а целочисленное деление тут даёт ноль -- и "немного уклонился" четыре
// умения подряд отдавали как полный промах (жалоба Вайнарда, 30.09.2026).

#include <gtest/gtest.h>

#include "gameplay/fight/common.h"

namespace slight_defence_damage_test {

TEST(SlightDefenceDamage, KeepsTwoThirdsOfTheHit) {
	EXPECT_EQ(SlightDefenceDamage(150), 100);
	EXPECT_EQ(SlightDefenceDamage(15), 10);
	EXPECT_EQ(SlightDefenceDamage(3), 2);
}

TEST(SlightDefenceDamage, NeverSwallowsTheWholeHit) {
	// Главное свойство: пока удар нёс хоть что-то ощутимое, он что-то и донесёт. Ноль здесь
	// означал бы промах, а промах у частичной защиты -- это и был баг.
	for (int dam = 2; dam <= 1000; ++dam) {
		EXPECT_GT(SlightDefenceDamage(dam), 0) << "урон " << dam << " обнулился";
		EXPECT_LE(SlightDefenceDamage(dam), dam) << "урон " << dam << " вырос";
	}
}

TEST(SlightDefenceDamage, SingleHitPointIsEatenOnPurpose) {
	// Удар в одну единицу округляется в ноль. Так считала и формула до рефакторинга
	// (`*dam = *dam * 10 / 15`), и так решено оставить: единица урона ни на что не влияет,
	// отдельную оговорку ради неё не вводим. Чинить тут нечего.
	EXPECT_EQ(SlightDefenceDamage(1), 0);
}

TEST(SlightDefenceDamage, WeakerThanPartialDefence) {
	// Лестница защит: "немного" слабее "частично" (половина) -- иначе смысл ступеней теряется.
	for (const int dam : {10, 37, 150, 999}) {
		EXPECT_GT(SlightDefenceDamage(dam), dam / 2) << "урон " << dam;
	}
}

TEST(SlightDefenceDamage, ZeroStaysZero) {
	EXPECT_EQ(SlightDefenceDamage(0), 0);
}

}   // namespace slight_defence_damage_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
