// Флаг kGreatHelper ("помогает издали") -- первый флаг моба в третьей плоскости EMobFlag.
//
// Имена флагов лежат в action_bits[] отдельным массивом, где плоскости разделены "\n", а
// индекс имени считается от начала массива. Любой пропуск в перечислении без заполнителя
// в массиве сдвигает все имена после него -- так уже было с битами 14-19 второй плоскости
// (issue.mob-flag-names), и в "stat" печатались UNDEF вместо последних флагов. Эти тесты
// держат соответствие бит <-> имя на границах плоскостей.
//
// Биты 14-19 второй плоскости выглядят свободными, но выставлены на живых мобах мира --
// остатки снятых когда-то флагов. Поэтому новый флаг и заведен в третьей плоскости.

#include "engine/entities/entities_constants.h"
#include "engine/structs/bitset_flags.h"
#include "gameplay/core/constants.h"

#include <gtest/gtest.h>

#include <string>

namespace {

std::string NamesOf(std::initializer_list<EMobFlag> flags) {
	const BitsetFlags<EMobFlag> bits{flags};
	// print_flag = 4: печатать и служебные имена, которые начинаются со звездочки.
	return bits.sprintbits(action_bits, ",", 4);
}

}  // namespace

TEST(MobFlagGreatHelper, LivesInPlaneThree) {
	const BitsetFlags<EMobFlag> bits{EMobFlag::kGreatHelper};
	EXPECT_EQ(bits.get_plane(3), 1u << 0);
	EXPECT_EQ(bits.get_plane(2), 0u);
	EXPECT_TRUE(bits.get(EMobFlag::kGreatHelper));
}

TEST(MobFlagGreatHelper, HasItsOwnName) {
	EXPECT_EQ(NamesOf({EMobFlag::kGreatHelper}), "помогает издали");
}

// Соседи по границам плоскостей: если заполнители в action_bits разъедутся, сдвинется
// именно здесь.
TEST(MobFlagGreatHelper, NeighbourNamesDidNotShift) {
	EXPECT_EQ(NamesOf({EMobFlag::kHelper}), "помогает");
	EXPECT_EQ(NamesOf({EMobFlag::kResurrected}), "оживлен");
	EXPECT_EQ(NamesOf({EMobFlag::kNoResurrection}), "!оживить");
	EXPECT_EQ(NamesOf({EMobFlag::kIgnoresFormation}), "игн.строй");
}

TEST(MobFlagGreatHelper, SurvivesAsciiRoundTrip) {
	const BitsetFlags<EMobFlag> bits{EMobFlag::kHelper, EMobFlag::kGreatHelper, EMobFlag::kStayZone};
	const std::string text = bits.tascii(kFlagPlanes);

	BitsetFlags<EMobFlag> restored;
	restored.from_string(text.c_str());
	EXPECT_EQ(restored, bits);
	EXPECT_TRUE(restored.get(EMobFlag::kGreatHelper));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
