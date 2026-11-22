// Имена рантаймовых меток персонажа (ECharExtraFlag) лежат в char_extra_bits[] отдельным
// массивом, и индекс имени считается от начала массива -- ровно как у action_bits[] для
// EMobFlag. Значит та же грабля: стоит завести новую метку в середине перечисления и не
// вписать имя на то же место, как все имена после нее съедут, а "stat" начнет печатать
// соседа или UNDEF. Эти тесты держат соответствие метка <-> имя.
//
// kResurrected -- последняя метка и единственная, которую ставит заклинание, а не умение;
// в stat она и нужна прежде всего, потому что из MOB-флагов она ушла (issue: флаг стал
// рантаймовым).

#include "engine/entities/entities_constants.h"
#include "engine/structs/bitset_flags.h"
#include "gameplay/core/constants.h"

#include <gtest/gtest.h>

#include <string>

namespace {

// Имя с префиксом: юнити-сборка кладет этот файл и mob.flag.great.helper.cpp в одну единицу
// трансляции, и два одноименных помощника в анонимных пространствах стали бы перегрузками.
std::string ExtraNamesOf(std::initializer_list<ECharExtraFlag> flags) {
	const BitsetFlags<ECharExtraFlag> bits{flags};
	return bits.sprintbits(char_extra_bits, ",", 4);
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

// Если в перечисление допишут метку, не вписав имя, последнее имя станет "\n" и тест упадет
// именно здесь -- раньше, чем это увидят в игре.
TEST(CharExtraFlagNames, LastFlagIsStillNamed) {
	EXPECT_EQ(static_cast<int>(ECharExtraFlag::kCount), 9);
	EXPECT_EQ(ExtraNamesOf({static_cast<ECharExtraFlag>(static_cast<int>(ECharExtraFlag::kCount) - 1)}),
			  "оживлен");
}

// На пустом наборе строковая форма подставляет общее слово "ничего" из каталога сообщений,
// которого в юнит-тестах нет, поэтому проверяем буферную форму: она отдает признак "флаги
// были" отдельно от текста. Заодно это сторожит обход всех четырех плоскостей -- без
// разделителей "\n" на каждую он уходил за край массива имен.
TEST(CharExtraFlagNames, EmptySetReportsNoFlags) {
	const BitsetFlags<ECharExtraFlag> empty;
	char buffer[256];
	EXPECT_FALSE(empty.sprintbits(char_extra_bits, buffer, sizeof(buffer), ",", 4));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
