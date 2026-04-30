#include "character_builder.h"

#include "utils/utils.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/db/global_objects.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/affects/affect_contants.h"
#include "gameplay/classes/pc_classes_info.h"
#include "gameplay/magic/spells.h"
#include "gameplay/mechanics/groups.h"

namespace simulator {

void CharacterBuilder::create_new()
{
	const auto result = std::make_shared<character_t>();
	result->player_specials = std::make_shared<player_special_data>();
	result->set_class(ECharClass::kFirst);
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

	auto poison = std::make_shared<Affect<EApply>>();
	poison->type = ESpell::kPoison;
	poison->modifier = 0;
	poison->location = EApply::kStr;
	poison->duration = CalcDuration(m_result.get(), 10 * 2, 0, 0, 0, 0);
	poison->bitvector = to_underlying(EAffect::kPoisoned);
	poison->battleflag = kAfSameTime;
	// Add directly to avoid affect_total() which requires global state
	m_result->affected.push_front(poison);
}

void CharacterBuilder::add_sleep()
{
	check_character_existance();

	auto sleep = std::make_shared<Affect<EApply>>();
	sleep->type = ESpell::kSleep;
	sleep->modifier = 0;
	sleep->location = EApply::kAc;
	sleep->duration = CalcDuration(m_result.get(), 10 * 2, 0, 0, 0, 0);
	sleep->bitvector = to_underlying(EAffect::kSleep);
	sleep->battleflag = kAfSameTime;
	// Add directly to avoid affect_total() which requires global state
	m_result->affected.push_front(sleep);
}

void CharacterBuilder::add_detect_invis()
{
	check_character_existance();

	auto detect_invis = std::make_shared<Affect<EApply>>();
	detect_invis->type = ESpell::kDetectInvis;
	detect_invis->modifier = 0;
	detect_invis->location = EApply::kAc;
	detect_invis->duration = CalcDuration(m_result.get(), 10 * 2, 0, 0, 0, 0);
	detect_invis->bitvector = to_underlying(EAffect::kDetectInvisible);
	detect_invis->battleflag = kAfSameTime;
	// Add directly to avoid affect_total() which requires global state
	m_result->affected.push_front(detect_invis);
}

void CharacterBuilder::add_detect_align()
{
	check_character_existance();

	auto detect_align = std::make_shared<Affect<EApply>>();
	detect_align->type = ESpell::kDetectAlign;
	detect_align->modifier = 0;
	detect_align->location = EApply::kAc;
	detect_align->duration = CalcDuration(m_result.get(), 10 * 2, 0, 0, 0, 0);
	detect_align->bitvector = to_underlying(EAffect::kDetectAlign);
	detect_align->battleflag = kAfSameTime;
	// Add directly to avoid affect_total() which requires global state
	m_result->affected.push_front(detect_align);
}

void CharacterBuilder::set_level(const int level)
{
	m_result->set_level(level);
}

void CharacterBuilder::set_class(const short player_class)
{
	m_result->set_class(static_cast<ECharClass>(player_class));
}

void CharacterBuilder::set_str(const int value)
{
	check_character_existance();
	m_result->set_str(value);
}

void CharacterBuilder::set_dex(const int value)
{
	check_character_existance();
	m_result->set_dex(value);
}

void CharacterBuilder::set_con(const int value)
{
	check_character_existance();
	m_result->set_con(value);
}

void CharacterBuilder::set_int(const int value)
{
	check_character_existance();
	m_result->set_int(value);
}

void CharacterBuilder::set_wis(const int value)
{
	check_character_existance();
	m_result->set_wis(value);
}

void CharacterBuilder::set_cha(const int value)
{
	check_character_existance();
	m_result->set_cha(value);
}

void CharacterBuilder::set_hit(const int value)
{
	check_character_existance();
	m_result->set_hit(value);
}

void CharacterBuilder::set_max_hit(const int value)
{
	check_character_existance();
	m_result->set_max_hit(value);
}

void CharacterBuilder::set_name(const std::string& name)
{
	check_character_existance();
	// PC alias used by GET_NAME() in messages, combat events, JSONL.
	m_result->set_name(name.c_str());
}

void CharacterBuilder::make_basic_player(const short player_class, const int level)
{
	create_new();
	set_class(player_class);
	set_level(level);
	set_str(25);
	set_dex(25);
	set_con(25);
	set_int(25);
	set_wis(25);
	set_cha(25);
	grant_class_skills_and_feats();
}

void CharacterBuilder::grant_class_skills_and_feats()
{
	check_character_existance();
	const auto cls = m_result->GetClass();
	const auto level = m_result->GetLevel();
	const int remort = m_result->get_remort();
	const auto &class_info = MUD::Class(cls);

	// Все скиллы класса, доступные на этом уровне/реморте, выставляем на 200
	// (максимум). Это симулятор: имитируем "хорошо прокачанного" персонажа,
	// чтобы баланс измерялся по самому классу, а не по тому что игрок его не
	// докачал.
	for (const auto &skill : class_info.skills) {
		if (level >= skill.GetMinLevel() && remort >= skill.GetMinRemort()) {
			m_result->set_skill(skill.GetId(), 200);
		}
	}

	// То же для inborn-фитов: даём всё, что положено классу на этом уровне.
	// Не-inborn фиты в реальной игре игрок выбирает вручную из слотов; для
	// симулятора берём все доступные, чтобы покрыть лучший случай.
	for (const auto &feat : class_info.feats) {
		if (level >= feat.GetMinLevel() && remort >= feat.GetMinRemort()) {
			m_result->SetFeat(feat.GetId());
		}
	}

	// Все class-заклинания, доступные на этом уровне, помечаем как известные
	// (SPELL_TYPE |= kKnow) и заливаем большой запас в память. Иначе DoCast
	// упрётся в проверку "не помню заклинания" / "нет слотов круга" и
	// просто откажется кастовать.
	for (const auto &spell : class_info.spells) {
		if (level >= spell.GetMinLevel() && remort >= spell.GetMinRemort()) {
			SET_BIT(GET_SPELL_TYPE(m_result, spell.GetId()), ESpellType::kKnow);
			SET_SPELL_MEM(m_result, spell.GetId(), 1000);
		}
	}
}

void CharacterBuilder::place_in_room(const RoomRnum room)
{
	check_character_existance();
	PlaceCharToRoom(m_result.get(), room);
}

void CharacterBuilder::make_group(CharacterBuilder& character_builder)
{
	check_character_existance();
	check_character_existance(character_builder.get());

	auto character = character_builder.get();

	m_result->add_follower_silently(character.get());

	group::perform_group(m_result.get(), m_result.get());
	group::perform_group(m_result.get(), character.get());
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

}  // namespace simulator

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
