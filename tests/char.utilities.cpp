#include "char.utilities.hpp"

#include <char.hpp>
#include <handler.h>

namespace test_utils
{
auto create_character()
{
	const auto result = std::make_shared<CHAR_DATA>();
	CREATE(result->player_specials, 1);
	return result;
}

auto add_poison(const CHAR_DATA::shared_ptr& character)
{
	AFFECT_DATA<EApplyLocation> poison;
	poison.type = SPELL_POISON;
	poison.modifier = 0;
	poison.location = APPLY_STR;
	poison.duration = pc_duration(character.get(), 10*2, 0, 0, 0, 0);
	poison.bitvector = to_underlying(EAffectFlag::AFF_POISON);
	poison.battleflag = AF_SAME_TIME;
	affect_join(character.get(), poison, false, false, false, false);
	return character;
}

auto add_sleep(const CHAR_DATA::shared_ptr& character)
{
	AFFECT_DATA<EApplyLocation> sleep;
	sleep.type = SPELL_SLEEP;
	sleep.modifier = 0;
	sleep.location = APPLY_AC;
	sleep.duration = pc_duration(character.get(), 10*2, 0, 0, 0, 0);
	sleep.bitvector = to_underlying(EAffectFlag::AFF_SLEEP);
	sleep.battleflag = AF_SAME_TIME;
	affect_join(character.get(), sleep, false, false, false, false);
	return character;
}

auto add_detect_invis(const CHAR_DATA::shared_ptr& character)
{
	AFFECT_DATA<EApplyLocation> detect_invis;
	detect_invis.type = SPELL_DETECT_INVIS;
	detect_invis.modifier = 0;
	detect_invis.location = APPLY_AC;
	detect_invis.duration = pc_duration(character.get(), 10*2, 0, 0, 0, 0);
	detect_invis.bitvector = to_underlying(EAffectFlag::AFF_DETECT_INVIS);
	detect_invis.battleflag = AF_SAME_TIME;
	affect_join(character.get(), detect_invis, false, false, false, false);
	return character;
}

auto add_detect_align(const CHAR_DATA::shared_ptr& character)
{
	AFFECT_DATA<EApplyLocation> detect_align;
	detect_align.type = SPELL_DETECT_ALIGN;
	detect_align.modifier = 0;
	detect_align.location = APPLY_AC;
	detect_align.duration = pc_duration(character.get(), 10*2, 0, 0, 0, 0);
	detect_align.bitvector = to_underlying(EAffectFlag::AFF_DETECT_ALIGN);
	detect_align.battleflag = AF_SAME_TIME;
	affect_join(character.get(), detect_align, false, false, false, false);
	return character;
}

auto create_character_with_one_removable_affect()
{
	auto character = create_character();
	return add_poison(character);
}

auto create_character_with_two_removable_affects()
{
	auto character = create_character_with_one_removable_affect();
	return add_sleep(character);
}

auto create_character_with_two_removable_and_two_not_removable_affects()
{
	auto character = create_character_with_two_removable_affects();
	add_detect_invis(character);
	return add_detect_align(character);
}

void CharacterBuilder::create_new() { m_result = create_character(); }
void CharacterBuilder::create_character_with_one_removable_affect() { m_result = test_utils::create_character_with_one_removable_affect(); }
void CharacterBuilder::create_character_with_two_removable_affects() { m_result = test_utils::create_character_with_two_removable_affects(); }
void CharacterBuilder::create_character_with_two_removable_and_two_not_removable_affects() { m_result = test_utils::create_character_with_two_removable_and_two_not_removable_affects(); }

void CharacterBuilder::add_poison() { test_utils::add_poison(m_result); }
void CharacterBuilder::add_sleep() { test_utils::add_sleep(m_result); }
void CharacterBuilder::add_detect_invis() { test_utils::add_detect_invis(m_result); }
void CharacterBuilder::add_detect_align() { test_utils::add_detect_align(m_result); }
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
