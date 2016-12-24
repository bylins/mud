#include "char.utilities.hpp"

#include <utils.h>
#include <char.hpp>
#include <handler.h>
#include <act.other.hpp>

namespace test_utils
{
void CharacterBuilder::create_new()
{
	const auto result = std::make_shared<character_t>();
	result->player_specials = std::make_shared<player_special_data>();
	result->set_class(CLASS_DRUID);
	result->set_level(1);
	m_result = result;
}

void CharacterBuilder::create_new_with_class(const short player_class)
{
	create_new();
	set_class(player_class);
}

void CharacterBuilder::create_character_with_one_removable_affect()
{
	create_new();
	add_poison();
}

void CharacterBuilder::create_character_with_two_removable_affects()
{
	create_character_with_one_removable_affect();
	add_sleep();
}

void CharacterBuilder::create_character_with_two_removable_and_two_not_removable_affects()
{
	create_character_with_two_removable_affects();
	add_detect_invis();
	add_detect_align();
}

void CharacterBuilder::add_poison()
{
	check_character_existance();

	AFFECT_DATA<EApplyLocation> poison;
	poison.type = SPELL_POISON;
	poison.modifier = 0;
	poison.location = APPLY_STR;
	poison.duration = pc_duration(m_result.get(), 10 * 2, 0, 0, 0, 0);
	poison.bitvector = to_underlying(EAffectFlag::AFF_POISON);
	poison.battleflag = AF_SAME_TIME;
	affect_join(m_result.get(), poison, false, false, false, false);
}

void CharacterBuilder::add_sleep()
{
	check_character_existance();

	AFFECT_DATA<EApplyLocation> sleep;
	sleep.type = SPELL_SLEEP;
	sleep.modifier = 0;
	sleep.location = APPLY_AC;
	sleep.duration = pc_duration(m_result.get(), 10 * 2, 0, 0, 0, 0);
	sleep.bitvector = to_underlying(EAffectFlag::AFF_SLEEP);
	sleep.battleflag = AF_SAME_TIME;
	affect_join(m_result.get(), sleep, false, false, false, false);
}

void CharacterBuilder::add_detect_invis()
{
	check_character_existance();

	AFFECT_DATA<EApplyLocation> detect_invis;
	detect_invis.type = SPELL_DETECT_INVIS;
	detect_invis.modifier = 0;
	detect_invis.location = APPLY_AC;
	detect_invis.duration = pc_duration(m_result.get(), 10 * 2, 0, 0, 0, 0);
	detect_invis.bitvector = to_underlying(EAffectFlag::AFF_DETECT_INVIS);
	detect_invis.battleflag = AF_SAME_TIME;
	affect_join(m_result.get(), detect_invis, false, false, false, false);
}

void CharacterBuilder::add_detect_align()
{
	check_character_existance();

	AFFECT_DATA<EApplyLocation> detect_align;
	detect_align.type = SPELL_DETECT_ALIGN;
	detect_align.modifier = 0;
	detect_align.location = APPLY_AC;
	detect_align.duration = pc_duration(m_result.get(), 10 * 2, 0, 0, 0, 0);
	detect_align.bitvector = to_underlying(EAffectFlag::AFF_DETECT_ALIGN);
	detect_align.battleflag = AF_SAME_TIME;
	affect_join(m_result.get(), detect_align, false, false, false, false);
}

void CharacterBuilder::set_level(const int level)
{
	m_result->set_level(level);
}

void CharacterBuilder::set_class(const short player_class)
{
	m_result->set_class(player_class);
}

void CharacterBuilder::make_group(CharacterBuilder& character_builder)
{
	check_character_existance();
	check_character_existance(character_builder.get());

	auto character = character_builder.get();

	m_result->add_follower_silently(character.get());

	perform_group(m_result.get(), m_result.get());
	perform_group(m_result.get(), character.get());
}

void CharacterBuilder::check_character_existance() const
{
	check_character_existance(m_result);
}

void CharacterBuilder::check_character_existance(result_t character)
{
	if (!character)
	{
		throw std::runtime_error("Character wasn't created.");
	}
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
