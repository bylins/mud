// issue #4002: продление пакета призванного. Флаг kAfCharmBond носит не только привязка, но и
// прибавки существа, поэтому продлевать надо весь пакет -- иначе прибавки истекают, а индикатор
// «скоро уйдёт» горит по ним же.

#include "char.utilities.hpp"

#include "gameplay/affects/affect_data.h"

#include <gtest/gtest.h>

namespace charm_bond_extend_test {

void AddBondNode(CharData *ch, EAffect affect_type, int duration, std::initializer_list<EAffFlag> flags) {
	auto af = std::make_shared<Affect<EApply>>();
	af->affect_type = affect_type;
	af->duration = duration;
	af->location = EApply::kNone;
	af->modifier = 0;
	af->battleflag = flags;
	ch->affected.push_back(af);
}

int DurationOf(CharData *ch, EAffect affect_type) {
	for (const auto &af : ch->affected) {
		if (af->affect_type == affect_type) {
			return af->duration;
		}
	}
	return -100;
}

TEST(CharmBondExtend, ExtendsEveryNodeOfThePackage) {
	test_utils::CharacterBuilder builder;
	builder.create_new();
	auto character = builder.get();

	AddBondNode(character.get(), EAffect::kCharmed, 100, {kAfCharmBond});      // сама привязка
	AddBondNode(character.get(), EAffect::kUndefined, 100, {kAfCharmBond});    // прибавка существа
	AddBondNode(character.get(), EAffect::kBless, 50, {kAfDispellable});       // чужой аффект

	ExtendCharmBond(character.get(), 150);

	EXPECT_EQ(150, DurationOf(character.get(), EAffect::kCharmed));
	EXPECT_EQ(150, DurationOf(character.get(), EAffect::kUndefined));
	EXPECT_EQ(50, DurationOf(character.get(), EAffect::kBless));   // не из пакета -- не трогаем
}

TEST(CharmBondExtend, NeverShortensAndKeepsPermanent) {
	test_utils::CharacterBuilder builder;
	builder.create_new();
	auto character = builder.get();

	AddBondNode(character.get(), EAffect::kCharmed, 200, {kAfCharmBond});
	AddBondNode(character.get(), EAffect::kSanctuary, -1, {kAfCharmBond});   // вечный узел пакета

	ExtendCharmBond(character.get(), 150);

	EXPECT_EQ(200, DurationOf(character.get(), EAffect::kCharmed));    // укорачивать нельзя
	EXPECT_EQ(-1, DurationOf(character.get(), EAffect::kSanctuary));   // вечный остаётся вечным
}

}   // namespace charm_bond_extend_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
