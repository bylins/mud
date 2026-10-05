// Part of Bylins http://www.mud.ru
// issue #4005: флаги аффекта (EAffFlag) живут в четырёх плоскостях по 30 бит. Плоскость 0 занята
// целиком, поэтому следующий флаг заведут в плоскости 1 -- эти тесты держат путь такого флага:
// набор, сериализацию поля battleflag в сейве игрока и отбор per-instance пометок в воронке.

#include "gameplay/affects/affect_data.h"
#include "engine/structs/flag_data.h"

#include <gtest/gtest.h>

#include <string>

namespace {

// Синтетический флаг из плоскости 1: в упакованном виде это (плоскость << 30) | (1 << бит).
// Настоящего такого флага пока нет -- первый заведут здесь же, когда понадобится.
constexpr auto kPlaneOneFlag = static_cast<EAffFlag>(kIntOne | (1u << 3));

}  // namespace

TEST(AffectFlagPlanes, FlagBeyondPlaneZeroIsStored) {
	const AffectFlags flags{EAffFlag::kAfDispellable, kPlaneOneFlag};
	EXPECT_TRUE(flags.get(kPlaneOneFlag));
	EXPECT_TRUE(flags.get(EAffFlag::kAfDispellable));
	EXPECT_EQ(flags.get_plane(1), 1u << 3);
	// Плоскость 0 про чужой флаг ничего не знает -- именно поэтому одиночного слова на пути мало.
	EXPECT_EQ(flags.get_plane(0), static_cast<unsigned>(EAffFlag::kAfDispellable));
}

TEST(AffectFlagPlanes, TasciiRoundTripKeepsPlaneOne) {
	const AffectFlags flags{EAffFlag::kAfCurable, EAffFlag::kAfCharmBond, kPlaneOneFlag};
	const std::string text = flags.tascii(kFlagPlanes);
	AffectFlags restored;
	restored.from_string(text.c_str());
	EXPECT_EQ(restored, flags);
}

TEST(AffectFlagPlanes, LegacyNumericFieldStillReads) {
	// Старый сейв писал поле battleffag одним числом -- плоскость 0 в упакованном виде.
	const std::string legacy = std::to_string(static_cast<unsigned>(EAffFlag::kAfDispellable));
	AffectFlags restored;
	restored.from_string(legacy.c_str());
	EXPECT_TRUE(restored.get(EAffFlag::kAfDispellable));
	EXPECT_EQ(restored.count(), 1u);
}

TEST(AffectFlagPlanes, PerInstanceFlagsKeepMarksAndDropProperties) {
	const AffectFlags caller{EAffFlag::kAfCharmBond, EAffFlag::kAfFromSet,
							 EAffFlag::kAfDispellable, kPlaneOneFlag};
	const AffectFlags kept = PerInstanceAffectFlags(caller);
	EXPECT_TRUE(kept.get(EAffFlag::kAfCharmBond));
	EXPECT_TRUE(kept.get(EAffFlag::kAfFromSet));
	// Свойства аффекта воронка берёт из affects.xml, а не у вызывающего.
	EXPECT_FALSE(kept.get(EAffFlag::kAfDispellable));
	EXPECT_FALSE(kept.get(kPlaneOneFlag));
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

TEST(AffectFlagPlanes, AsciiFormParsesSingleFlag) {
	AffectFlags restored;
	restored.from_string("l0");   // 'l' = бит 11 плоскости 0 = kAfFailed
	EXPECT_TRUE(restored.get(EAffFlag::kAfFailed));
	EXPECT_EQ(restored.count(), 1u);
}
