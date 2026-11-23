// Имена рантаймовых меток персонажа (ECharExtraFlag) для вывода в "stat".
//
// Имена привязаны к enum switch'ем без default, а не плоским массивом const char* с индексом
// по значению: такой массив живет ровно до первого добавленного значения -- имена после него
// съезжают молча, и в "stat" печатается сосед. Так уже было с action_bits[] и битами 14-19
// второй плоскости EMobFlag (issue.mob-flag-names). Эти тесты держат соответствие метка <->
// имя, а компилятор ловит метку, которой имя забыли дать.
//
// kResurrected -- последняя метка и единственная, которую ставит заклинание, а не умение;
// в stat она и нужна прежде всего, потому что из MOB-флагов она ушла.

#include "engine/entities/entities_constants.h"
#include "engine/structs/bitset_flags.h"

#include <gtest/gtest.h>

#include <string>

namespace {

std::string ExtraNamesOf(std::initializer_list<ECharExtraFlag> flags) {
	return NamesOfCharExtraFlags(BitsetFlags<ECharExtraFlag>{flags});
}

}  // namespace

TEST(CharExtraFlagNames, EveryFlagHasItsOwnName) {
	EXPECT_EQ(ExtraNamesOf({ECharExtraFlag::kFailHide}), "!спрятался");
	EXPECT_EQ(ExtraNamesOf({ECharExtraFlag::kFailSneak}), "!подкрался");
	EXPECT_EQ(ExtraNamesOf({ECharExtraFlag::kFailCamouflage}), "!замаскировался");
	EXPECT_EQ(ExtraNamesOf({ECharExtraFlag::kGrpKillCount}), "учтен в добивании");
	EXPECT_EQ(ExtraNamesOf({ECharExtraFlag::kToldHide}), "сказано:скрытность");
	EXPECT_EQ(ExtraNamesOf({ECharExtraFlag::kToldSneak}), "сказано:подкрадывание");
	EXPECT_EQ(ExtraNamesOf({ECharExtraFlag::kToldCamouflage}), "сказано:маскировка");
	EXPECT_EQ(ExtraNamesOf({ECharExtraFlag::kGoesToHelp}), "идет на помощь");
	EXPECT_EQ(ExtraNamesOf({ECharExtraFlag::kResurrected}), "оживлен");
}

// Порядок в строке -- по значениям enum, а не по порядку в наборе.
TEST(CharExtraFlagNames, SeveralMarksGoInEnumOrder) {
	EXPECT_EQ(ExtraNamesOf({ECharExtraFlag::kResurrected, ECharExtraFlag::kFailSneak}),
			  "!подкрался,оживлен");
}

// Слово "ничего" подставляет stat, а не сама функция: тут пустая строка.
TEST(CharExtraFlagNames, EmptySetGivesEmptyString) {
	EXPECT_TRUE(NamesOfCharExtraFlags(BitsetFlags<ECharExtraFlag>{}).empty());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
