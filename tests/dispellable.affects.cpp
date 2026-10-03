// issue #3988: отбор аффектов для поля .dispel у dg-триггеров. Проверяем, что снимается ровно то,
// что снимает магия, а волшебство надетого и пакет призванного существа остаются на месте.

#include "char.utilities.hpp"

#include "gameplay/affects/affect_data.h"

#include <gtest/gtest.h>

namespace dispellable_affects_test {

void AddAffect(CharData *ch, EAffect affect_type, std::initializer_list<EAffFlag> flags) {
	auto af = std::make_shared<Affect<EApply>>();
	af->modifier = 0;
	af->location = EApply::kNone;
	af->duration = 10;
	af->affect_type = affect_type;
	af->battleflag = flags;
	ch->affected.push_back(af);
}

bool HasAffect(CharData *ch, EAffect affect_type) {
	for (const auto &af : ch->affected) {
		if (af->affect_type == affect_type) {
			return true;
		}
	}
	return false;
}

TEST(DispellableAffects, KeepsEquipmentAndCharmPackage) {
	test_utils::CharacterBuilder builder;
	builder.create_new();
	auto character = builder.get();

	AddAffect(character.get(), EAffect::kBless, {kAfDispellable});
	AddAffect(character.get(), EAffect::kPoisoned, {kAfCurable});
	AddAffect(character.get(), EAffect::kDetectInvisible, {kAfDispellable, kAfFromEquipment});
	AddAffect(character.get(), EAffect::kSanctuary, {kAfDispellable, kAfFromEquipment, kAfFromSet});
	AddAffect(character.get(), EAffect::kBlink, {kAfDispellable, kAfCharmBond});
	AddAffect(character.get(), EAffect::kHold, {});

	EXPECT_TRUE(RemoveDispellableAffects(character.get()));

	EXPECT_FALSE(HasAffect(character.get(), EAffect::kBless));
	EXPECT_FALSE(HasAffect(character.get(), EAffect::kPoisoned));
	EXPECT_TRUE(HasAffect(character.get(), EAffect::kDetectInvisible));
	EXPECT_TRUE(HasAffect(character.get(), EAffect::kSanctuary));
	EXPECT_TRUE(HasAffect(character.get(), EAffect::kBlink));
	EXPECT_TRUE(HasAffect(character.get(), EAffect::kHold));
	EXPECT_EQ(4u, character->affected.size());
}

TEST(DispellableAffects, ReportsNothingRemoved) {
	test_utils::CharacterBuilder builder;
	builder.create_new();
	auto character = builder.get();

	AddAffect(character.get(), EAffect::kDetectInvisible, {kAfDispellable, kAfFromEquipment});
	AddAffect(character.get(), EAffect::kHold, {});

	EXPECT_FALSE(RemoveDispellableAffects(character.get()));
	EXPECT_EQ(2u, character->affected.size());
}

}   // namespace dispellable_affects_test

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
