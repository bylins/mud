// $RCSfile: char.cpp,v $     $Date: 2012/01/27 04:04:43 $     $Revision: 1.85 $
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "world_characters.h"
#include "fightsystem/pk.h"
#include "handler.h"
#include "privilege.h"
#include "char_player.h"
#include "player_races.h"
#include "game_mechanics/celebrates.h"
#include "cache.h"
#include "fightsystem/fight.h"
#include "house.h"
#include "msdp/msdp_constants.h"
#include "backtrace.h"
#include "zone.h"
#include "structs/global_objects.h"

#include <boost/format.hpp>
#include <random>

std::string PlayerI::empty_const_str;
MapSystem::Options PlayerI::empty_map_options;

namespace {

// * На перспективу - втыкать во все методы character.
void check_purged(const CharacterData *ch, const char *fnc) {
	if (ch->purged()) {
		log("SYSERR: Using purged character (%s).", fnc);
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
	}
}

} // namespace


ProtectedCharacterData::ProtectedCharacterData() : m_rnum(kNobody) {
}

ProtectedCharacterData::ProtectedCharacterData(const ProtectedCharacterData &rhv) : m_rnum(kNobody) {
	*this = rhv;
}

void ProtectedCharacterData::set_rnum(const MobRnum rnum) {
	if (rnum != m_rnum) {
		const auto old_rnum = m_rnum;

		m_rnum = rnum;

		for (const auto &observer : m_rnum_change_observers) {
			observer->notify(*this, old_rnum);
		}
	}
}

ProtectedCharacterData &ProtectedCharacterData::operator=(const ProtectedCharacterData &rhv) {
	if (this != &rhv) {
		set_rnum(rhv.m_rnum);
	}

	return *this;
}

CharacterData::CharacterData() :
	chclass_(ECharClass::kUndefined),
	role_(MOB_ROLE_TOTAL_NUM),
	in_room(Rooms::UNDEFINED_ROOM_VNUM),
	m_wait(~0u),
	m_master(nullptr),
	proto_script(new ObjectData::triggers_list_t()),
	script(new Script()),
	followers(nullptr) {
	this->zero_init();
	current_morph_ = GetNormalMorphNew(this);
	caching::character_cache.add(this);
	this->set_skill(ESkill::kGlobalCooldown, 1);
}

CharacterData::~CharacterData() {
	this->purge();
}

void CharacterData::set_souls(int souls) {
	this->souls = souls;
}

void CharacterData::inc_souls() {
	this->souls++;
}

void CharacterData::dec_souls() {
	this->souls--;
}

int CharacterData::get_souls() {
	return this->souls;
}

bool CharacterData::in_used_zone() const {
	if (IS_MOB(this)) {
		return 0 != zone_table[world[in_room]->zone_rn].used;
	}
	return false;
}

//вычисление штрафов за голод и жажду
//P_DAMROLL, P_HITROLL, P_CAST, P_MEM_GAIN, P_MOVE_GAIN, P_HIT_GAIN
float CharacterData::get_cond_penalty(int type) const {
	if (IS_NPC(this)) return 1;
	if (!(GET_COND_M(this, FULL) || GET_COND_M(this, THIRST))) return 1;

	float penalty = 0;

	if (GET_COND_M(this, FULL)) {
		int tmp = GET_COND_K(this, FULL); // 0 - 1
		switch (type) {
			case P_DAMROLL://-50%
				penalty += tmp / 2;
				break;
			case P_HITROLL://-25%
				penalty += tmp / 4;
				break;
			case P_CAST://-25%
				penalty += tmp / 4;
				break;
			case P_MEM_GAIN://-25%
				penalty += tmp / 4;
				break;
			case P_MOVE_GAIN://-50%
				penalty += tmp / 2;
				break;
			case P_HIT_GAIN://-50%
				penalty += tmp / 2;
				break;
			case P_AC://-50%
				penalty += tmp / 2;
				break;
			default: break;
		}
	}

	if (GET_COND_M(this, THIRST)) {
		int tmp = GET_COND_K(this, THIRST); // 0 - 1
		switch (type) {
			case P_DAMROLL://-25%
				penalty += tmp / 4;
				break;
			case P_HITROLL://-50%
				penalty += tmp / 2;
				break;
			case P_CAST://-50%
				penalty += tmp / 2;
				break;
			case P_MEM_GAIN://-50%
				penalty += tmp / 2;
				break;
			case P_MOVE_GAIN://-25%
				penalty += tmp / 4;
				break;
			case P_AC://-25%
				penalty += tmp / 4;
				break;
			default: break;
		}
	}
	penalty = 100 - MIN(MAX(0, penalty), 100);
	penalty /= 100.0;
	return penalty;
}

void CharacterData::reset() {
	int i;

	for (i = 0; i < NUM_WEARS; i++) {
		GET_EQ(this, i) = nullptr;
	}
	memset((void *) &add_abils, 0, sizeof(add_abils));

	followers = nullptr;
	m_master = nullptr;
	in_room = kNowhere;
	carrying = nullptr;
	next_fighting = nullptr;
	set_protecting(nullptr);
	set_touching(nullptr);
	BattleAffects = clear_flags;
	Poisoner = 0;
	set_fighting(nullptr);
	char_specials.position = EPosition::kStand;
	mob_specials.default_pos = EPosition::kStand;
	char_specials.carry_weight = 0;
	char_specials.carry_items = 0;

	if (GET_HIT(this) <= 0) {
		GET_HIT(this) = 1;
	}
	if (GET_MOVE(this) <= 0) {
		GET_MOVE(this) = 1;
	}

	GET_CASTER(this) = 0;
	GET_DAMAGE(this) = 0;

	PlayerI::reset();
}

void CharacterData::set_abstinent() {
	int duration = pc_duration(this, 2, MAX(0, GET_DRUNK_STATE(this) - CHAR_DRUNKED), 4, 2, 5);

	if (can_use_feat(this, DRUNKARD_FEAT)) {
		duration /= 2;
	}

	Affect<EApplyLocation> af;
	af.type = SPELL_ABSTINENT;
	af.bitvector = to_underlying(EAffectFlag::AFF_ABSTINENT);
	af.duration = duration;

	af.location = APPLY_AC;
	af.modifier = 20;
	affect_join(this, af, false, false, false, false);

	af.location = APPLY_HITROLL;
	af.modifier = -2;
	affect_join(this, af, false, false, false, false);

	af.location = APPLY_DAMROLL;
	af.modifier = -2;
	affect_join(this, af, false, false, false, false);
}

void CharacterData::affect_remove(const char_affects_list_t::iterator &affect_i) {
	int was_lgt = AFF_FLAGGED(this, EAffectFlag::AFF_SINGLELIGHT) ? LIGHT_YES : LIGHT_NO;
	long was_hlgt = AFF_FLAGGED(this, EAffectFlag::AFF_HOLYLIGHT) ? LIGHT_YES : LIGHT_NO;
	long was_hdrk = AFF_FLAGGED(this, EAffectFlag::AFF_HOLYDARK) ? LIGHT_YES : LIGHT_NO;

	if (affected.empty()) {
		log("SYSERR: affect_remove(%s) when no affects...", GET_NAME(this));
		return;
	}

	const auto af = *affect_i;
	affect_modify(this, af->location, af->modifier, static_cast<EAffectFlag>(af->bitvector), false);
	if (af->type == SPELL_ABSTINENT) {
		if (player_specials) {
			GET_DRUNK_STATE(this) = GET_COND(this, DRUNK) = MIN(GET_COND(this, DRUNK), CHAR_DRUNKED - 1);
		} else {
			log("SYSERR: player_specials is not set.");
		}
	}
	if (af->type == SPELL_DRUNKED && af->duration == 0) {
		set_abstinent();
	}

	affected.erase(affect_i);

	affect_total(this);
	check_light(this, LIGHT_UNDEF, was_lgt, was_hlgt, was_hdrk, 1);
}

bool CharacterData::has_any_affect(const affects_list_t &affects) {
	for (const auto &affect : affects) {
		if (AFF_FLAGGED(this, affect)) {
			return true;
		}
	}

	return false;
}

size_t CharacterData::remove_random_affects(const size_t count) {
	int last_type = -1;
	std::deque<char_affects_list_t::iterator> removable_affects;
	for (auto affect_i = affected.begin(); affect_i != affected.end(); ++affect_i) {
		const auto &affect = *affect_i;

		if (affect->type != last_type && affect->removable()) {
			removable_affects.push_back(affect_i);
			last_type = affect->type;
		}
	}

	const auto to_remove = std::min(count, removable_affects.size());
	std::shuffle(removable_affects.begin(), removable_affects.end(), std::mt19937(std::random_device()()));
	for (auto counter = 0u; counter < to_remove; ++counter) {
		const auto affect_i = removable_affects[counter];
		affect_from_char(this, affect_i->get()->type);
	}

	return to_remove;
}

/**
* Обнуление всех полей Character (аналог конструктора),
* вынесено в отдельную функцию, чтобы дергать из purge().
*/
void CharacterData::zero_init() {
	set_sex(ESex::kMale);
	set_race(0);
	protecting_ = nullptr;
	touching_ = nullptr;
	fighting_ = nullptr;
	serial_num_ = 0;
	purged_ = false;
	// на плеер-таблицу
	chclass_ = ECharClass::kUndefined;
	level_ = 0;
	level_add_ = 0,
	idnum_ = 0;
	uid_ = 0;
	exp_ = 0;
	remorts_ = 0;
	remorts_add_ = 0;
	last_logon_ = 0;
	gold_ = 0;
	bank_gold_ = 0;
	ruble = 0;
	str_ = 0;
	str_add_ = 0;
	dex_ = 0;
	dex_add_ = 0;
	con_ = 0;
	con_add_ = 0;
	wis_ = 0;
	wis_add_ = 0;
	int_ = 0;
	int_add_ = 0;
	cha_ = 0;
	cha_add_ = 0;
	skill_bonus_ = 0;
	role_.reset();
	attackers_.clear();
	restore_timer_ = 0;
	// char_data
	set_rnum(kNobody);
	in_room = 0;
	set_wait(0u);
	punctual_wait = 0;
	last_comm = nullptr;
	player_specials = nullptr;
	timed = nullptr;
	timed_feat = nullptr;
	carrying = nullptr;
	desc = nullptr;
	id = 0;
	next_fighting = nullptr;
	followers = nullptr;
	m_master = nullptr;
	CasterLevel = 0;
	DamageLevel = 0;
	pk_list = nullptr;
	helpers = nullptr;
	track_dirs = 0;
	CheckAggressive = 0;
	ExtractTimer = 0;
	Initiative = 0;
	BattleCounter = 0;
	round_counter = 0;
	Poisoner = 0;
	dl_list = nullptr;
	agrobd = false;

	memset(&extra_attack_, 0, sizeof(extra_attack_type));
	memset(&cast_attack_, 0, sizeof(cast_attack_type));
	//memset(&player_data, 0, sizeof(char_player_data));
	//player_data char_player_data();
	memset(&add_abils, 0, sizeof(char_played_ability_data));
	memset(&real_abils, 0, sizeof(char_ability_data));
	memset(&points, 0, sizeof(char_point_data));
	memset(&char_specials, 0, sizeof(char_special_data));
	memset(&mob_specials, 0, sizeof(mob_special_data));

	for (auto & i : equipment) {
		i = nullptr;
	}

	memset(&MemQueue, 0, sizeof(spell_mem_queue));
	memset(&Temporary, 0, sizeof(FlagData));
	memset(&BattleAffects, 0, sizeof(FlagData));
	char_specials.position = EPosition::kStand;
	mob_specials.default_pos = EPosition::kStand;
	souls = 0;
}

/**
 * Освобождение выделенной в Character памяти, вынесено из деструктора,
 * т.к. есть необходимость дергать отдельно от delete.
 * \param destructor - true вызов произошел из дестркутора и обнулять/добавлять
 * в purged_list не нужно, по дефолту = false.
 *
 * Система в целом: там, где уже все обработано и почищено, и предполагается вызов
 * финального delete - вместо него вызывается метод purge(), в котором идет аналог
 * деструктора с полной очисткой объекта, но сама оболочка при этом сохраняется,
 * плюс инится флаг purged_, после чего по коду можно проверять старый указатель на
 * объект через метод purged() и, соответственно, не использовать его, если объект
 * по факту был удален. Таким образом мы не нарываемся на невалидные указатели, если
 * по ходу функции были спуржены чар/моб/шмотка. Гарантии ес-сно только в пределах
 * вызовов до выхода в обработку heartbeat(), где раз в минуту удаляются оболочки.
 */
void CharacterData::purge() {
	caching::character_cache.remove(this);

	if (!get_name().empty()) {
		log("[FREE CHAR] (%s)", GET_NAME(this));
	}

	int i, id = -1;
	struct alias_data *a;

	if (!IS_NPC(this) && !get_name().empty()) {
		id = get_ptable_by_name(GET_NAME(this));
		if (id >= 0) {
			player_table[id].level = GET_REAL_LEVEL(this);
			player_table[id].remorts = GET_REAL_REMORT(this);
			player_table[id].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
		}
	}

	if (!IS_NPC(this) || (IS_NPC(this) && GET_MOB_RNUM(this) == -1)) {
		if (IS_NPC(this) && this->mob_specials.Questor)
			free(this->mob_specials.Questor);

		pk_free_list(this);

		while (this->helpers) {
			REMOVE_FROM_LIST(this->helpers, this->helpers, [](auto list) -> auto & { return list->next; });
		}
	} else if ((i = GET_MOB_RNUM(this))
		>= 0) {    // otherwise, free strings only if the string is not pointing at proto

		if (this->mob_specials.Questor && this->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
			free(this->mob_specials.Questor);
	}

	while (!this->affected.empty()) {
		affect_remove(affected.begin());
	}

	while (this->timed) {
		timed_from_char(this, this->timed);
	}

	Celebrates::remove_from_mob_lists(this->id);

	const bool keep_player_specials = player_specials == player_special_data::s_for_mobiles ? true : false;
	if (this->player_specials && !keep_player_specials) {
		while ((a = GET_ALIASES(this)) != nullptr) {
			GET_ALIASES(this) = (GET_ALIASES(this))->next;
			free_alias(a);
		}
		if (this->player_specials->poofin)
			free(this->player_specials->poofin);
		if (this->player_specials->poofout)
			free(this->player_specials->poofout);
		// рецепты
		while (GET_RSKILL(this) != nullptr) {
			im_rskill *r;
			r = GET_RSKILL(this)->link;
			free(GET_RSKILL(this));
			GET_RSKILL(this) = r;
		}
		// порталы
		while (GET_PORTALS(this) != nullptr) {
			struct CharacterPortal *prt_next;
			prt_next = GET_PORTALS(this)->next;
			free(GET_PORTALS(this));
			GET_PORTALS(this) = prt_next;
		}
// Cleanup punish reasons
		if (MUTE_REASON(this))
			free(MUTE_REASON(this));
		if (DUMB_REASON(this))
			free(DUMB_REASON(this));
		if (HELL_REASON(this))
			free(HELL_REASON(this));
		if (FREEZE_REASON(this))
			free(FREEZE_REASON(this));
		if (NAME_REASON(this))
			free(NAME_REASON(this));
// End reasons cleanup

		if (KARMA(this))
			free(KARMA(this));

		free(GET_LOGS(this));
// shapirus: подчистим за криворукуми кодерами memory leak,
// вызванный неосвобождением фильтра базара...
		if (EXCHANGE_FILTER(this)) {
			free(EXCHANGE_FILTER(this));
		}
		EXCHANGE_FILTER(this) = nullptr;    // на всякий случай

		clear_ignores();

		if (GET_CLAN_STATUS(this)) {
			free(GET_CLAN_STATUS(this));
		}

		LOGON_LIST(this).clear();

		this->player_specials.reset();

		if (IS_NPC(this)) {
			log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(this), GET_MOB_VNUM(this));
		}
	}
	name_.clear();
	short_descr_.clear();

	auto follower = followers;
	while (follower) {
		const auto next_one = follower->next;
		free(follower);
		follower = next_one;
	}
}

// * Скилл с учетом всех плюсов и минусов от шмоток/яда.
int CharacterData::get_skill(const ESkill skill_num) const {
	int skill = get_trained_skill(skill_num) + get_equipped_skill(skill_num);
	if (AFF_FLAGGED(this, EAffectFlag::AFF_SKILLS_REDUCE)) {
		skill -= skill * GET_POISON(this) / 100;
	}
	return std::clamp(skill, 0, MUD::Skills()[skill_num].cap);
}

//  Скилл со шмоток.
// мобам и тем классам, у которых скилл является родным, учитываем скилл с каждой шмотки полностью,
// всем остальным -- не более 5% с шмотки
int CharacterData::get_equipped_skill(const ESkill skill_num) const {
	int skill = 0;
	bool is_native = IS_NPC(this) || MUD::Classes()[chclass_].IsKnown(skill_num);
	for (auto i : equipment) {
		if (i) {
			if (is_native) {
				skill += i->get_skill(skill_num);
			}
			// На новый год включаем
			/*else
			{
				skill += (MIN(5, equipment[i]->get_skill(skill_num)));
			}*/
		}
	}
	if (is_native) {
		skill += obj_bonus_.get_skill(skill_num);
	}
	if(get_trained_skill(skill_num) > 0) {
		skill += get_skill_bonus();
	}
	
	return skill;
}

// * Родной тренированный скилл чара.
int CharacterData::get_inborn_skill(const ESkill skill_num) {
	if (Privilege::check_skills(this)) {
		auto it = skills.find(skill_num);
		if (it != skills.end()) {
			//return normalize_skill(it->second.skillLevel, skill_num);
			return std::clamp(it->second.skillLevel, 0, MUD::Skills()[skill_num].cap);
		}
	}
	return 0;
}

int CharacterData::get_trained_skill(const ESkill skill_num) const {
	if (AFF_FLAGGED(this, EAffectFlag::AFF_DOMINATION)) {
		if (MUD::Classes()[chclass_].IsKnown(skill_num)) {
			return 100;
		}
	}
	if (Privilege::check_skills(this)) {
		//return normalize_skill(current_morph_->get_trained_skill(skill_num), skill_num);
		return std::clamp(current_morph_->get_trained_skill(skill_num), 0, MUD::Skills()[skill_num].cap);
	}
	return 0;
}

// * Нулевой скилл мы не сетим, а при обнулении уже имеющегося удалем эту запись.
void CharacterData::set_skill(const ESkill skill_id, int percent) {
	if (MUD::Skills().IsInvalid(skill_id)) {
		log("SYSERROR: некорректный номер скилла %d в set_skill.", to_underlying(skill_id));
		return;
	}
	auto it = skills.find(skill_id);
	if (it != skills.end()) {
		if (percent) {
			it->second.skillLevel = percent;
		} else {
			skills.erase(it);
		}
	} else if (percent) {
		skills[skill_id].skillLevel = percent;
	}
}

void CharacterData::set_skill(short remort) {
	int skill;
	int maxSkillLevel = kSkillCapOnZeroRemort + remort * kSkillCapBonusPerRemort;
	for (auto & it : skills) {
		skill = get_trained_skill(it.first) + get_equipped_skill(it.first);
		if (skill > maxSkillLevel) {
			it.second.skillLevel = maxSkillLevel;
		};
	}
}

void CharacterData::set_morphed_skill(const ESkill skill_num, int percent) {
	current_morph_->set_skill(skill_num, percent);
};

void CharacterData::clear_skills() {
	skills.clear();
}

int CharacterData::get_skills_count() const {
	return static_cast<int>(skills.size());
}

void CharacterSkillDataType::decreaseCooldown(unsigned value) {
	if (cooldown > value) {
		cooldown -= value;
	} else {
		cooldown = 0u;
	};
};

void CharacterData::setSkillCooldown(ESkill skillID, unsigned cooldown) {
	auto skillData = skills.find(skillID);
	if (skillData != skills.end()) {
		skillData->second.cooldown = cooldown;
	}
};

unsigned CharacterData::getSkillCooldown(ESkill skillID) {
	auto skillData = skills.find(skillID);
	if (skillData != skills.end()) {
		return skillData->second.cooldown;
	}
	return 0;
};

int CharacterData::getSkillCooldownInPulses(ESkill skillID) {
	//return static_cast<int>(std::ceil(skillData->second.cooldown/(0.0 + kPulseViolence)));
	return static_cast<int>(std::ceil(getSkillCooldown(skillID) / (0.0 + kPulseViolence)));
};

/* Понадобится - тогда и раскомментим...
void CharacterData::decreaseSkillCooldown(ESkill skillID, unsigned value) {
	auto skillData = skills.find(skillID);
	if (skillData != skills.end()) {
		skillData->second.decreaseCooldown(value);
	}
};
*/
void CharacterData::decreaseSkillsCooldowns(unsigned value) {
	for (auto &skillData : skills) {
		skillData.second.decreaseCooldown(value);
	}
};

bool CharacterData::haveSkillCooldown(ESkill skillID) {
	auto skillData = skills.find(skillID);
	if (skillData != skills.end()) {
		return (skillData->second.cooldown > 0);
	}
	return false;
};

bool CharacterData::haveCooldown(ESkill skillID) {
	if (skills[ESkill::kGlobalCooldown].cooldown > 0) {
		return true;
	}
	return haveSkillCooldown(skillID);
};

int CharacterData::get_obj_slot(int slot_num) {
	if (slot_num >= 0 && slot_num < MAX_ADD_SLOTS) {
		return add_abils.obj_slot[slot_num];
	}
	return 0;
}

void CharacterData::add_obj_slot(int slot_num, int count) {
	if (slot_num >= 0 && slot_num < MAX_ADD_SLOTS) {
		add_abils.obj_slot[slot_num] += count;
	}
}

void CharacterData::set_touching(CharacterData *vict) {
	touching_ = vict;
}

CharacterData *CharacterData::get_touching() const {
	return touching_;
}

void CharacterData::set_protecting(CharacterData *vict) {
	protecting_ = vict;
}

CharacterData *CharacterData::get_protecting() const {
	return protecting_;
}

void CharacterData::set_fighting(CharacterData *vict) {
	fighting_ = vict;
}

CharacterData *CharacterData::get_fighting() const {
	return fighting_;
}

void CharacterData::set_extra_attack(EExtraAttack Attack, CharacterData *vict) {
	extra_attack_.used_attack = Attack;
	extra_attack_.victim = vict;
}

EExtraAttack CharacterData::get_extra_attack_mode() const {
	return extra_attack_.used_attack;
}

CharacterData *CharacterData::get_extra_victim() const {
	return extra_attack_.victim;
}

void CharacterData::set_cast(int spellnum, int spell_subst, CharacterData *tch, ObjectData *tobj, RoomData *troom) {
	cast_attack_.spellnum = spellnum;
	cast_attack_.spell_subst = spell_subst;
	cast_attack_.tch = tch;
	cast_attack_.tobj = tobj;
	cast_attack_.troom = troom;
}

int CharacterData::get_cast_spell() const {
	return cast_attack_.spellnum;
}

int CharacterData::get_cast_subst() const {
	return cast_attack_.spell_subst;
}

CharacterData *CharacterData::get_cast_char() const {
	return cast_attack_.tch;
}

ObjectData *CharacterData::get_cast_obj() const {
	return cast_attack_.tobj;
}

bool IS_CHARMICE(const CharacterData *ch) {
	return IS_NPC(ch)
		&& (AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM));
}

bool MORT_CAN_SEE(const CharacterData *sub, const CharacterData *obj) {
	return HERE(obj)
		&& INVIS_OK(sub, obj)
		&& (IS_LIGHT((obj)->in_room)
			|| AFF_FLAGGED((sub), EAffectFlag::AFF_INFRAVISION));
}

bool MAY_SEE(const CharacterData *ch, const CharacterData *sub, const CharacterData *obj) {
	return !(GET_INVIS_LEV(ch) > 30)
		&& !AFF_FLAGGED(sub, EAffectFlag::AFF_BLIND)
		&& (!IS_DARK(sub->in_room)
			|| AFF_FLAGGED(sub, EAffectFlag::AFF_INFRAVISION))
		&& (!AFF_FLAGGED(obj, EAffectFlag::AFF_INVISIBLE)
			|| AFF_FLAGGED(sub, EAffectFlag::AFF_DETECT_INVIS));
}

bool IS_HORSE(const CharacterData *ch) {
	return IS_NPC(ch)
		&& ch->has_master()
		&& AFF_FLAGGED(ch, EAffectFlag::AFF_HORSE);
}

bool IS_MORTIFIER(const CharacterData *ch) {
	return IS_NPC(ch)
		&& ch->has_master()
		&& MOB_FLAGGED(ch, MOB_CORPSE);
}

bool MAY_ATTACK(const CharacterData *sub) {
	return (!AFF_FLAGGED((sub), EAffectFlag::AFF_CHARM)
		&& !IS_HORSE((sub))
		&& !AFF_FLAGGED((sub), EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED((sub), EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED((sub), EAffectFlag::AFF_HOLD)
		&& !AFF_FLAGGED((sub), EAffectFlag::AFF_SLEEP)
		&& !MOB_FLAGGED((sub), MOB_NOFIGHT)
		&& GET_WAIT(sub) <= 0
		&& !sub->get_fighting()
		&& GET_POS(sub) >= EPosition::kRest);
}

bool AWAKE(const CharacterData *ch) {
	return GET_POS(ch) > EPosition::kSleep
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP);
}

//Вы уверены,что функцияам расчете опыта самое место в классе персонажа?
bool OK_GAIN_EXP(const CharacterData *ch, const CharacterData *victim) {
	return !NAME_BAD(ch)
		&& (NAME_FINE(ch)
			|| !(GET_REAL_LEVEL(ch) == NAME_LEVEL))
		&& !ROOM_FLAGGED(ch->in_room, ROOM_ARENA)
		&& IS_NPC(victim)
		&& (GET_EXP(victim) > 0)
		&& (!IS_NPC(victim)
			|| !IS_NPC(ch)
			|| AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		&& !IS_HORSE(victim)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_DOMINATION);
}

bool IS_MALE(const CharacterData *ch) {
	return GET_SEX(ch) == ESex::kMale;
}

bool IS_FEMALE(const CharacterData *ch) {
	return GET_SEX(ch) == ESex::kFemale;
}

bool IS_NOSEXY(const CharacterData *ch) {
	return GET_SEX(ch) == ESex::kNeutral;
}

bool IS_POLY(const CharacterData *ch) {
	return GET_SEX(ch) == ESex::kPoly;
}

bool IMM_CAN_SEE(const CharacterData *sub, const CharacterData *obj) {
	return MORT_CAN_SEE(sub, obj)
		|| (!IS_NPC(sub)
			&& PRF_FLAGGED(sub, PRF_HOLYLIGHT));
}

bool CAN_SEE(const CharacterData *sub, const CharacterData *obj) {
	return SELF(sub, obj)
		|| ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj)))
			&& IMM_CAN_SEE(sub, obj));
}

// * Внутри цикла чар нигде не пуржится и сам список соответственно не меняется.
void change_fighting(CharacterData *ch, int need_stop) {
	//Loop for all entities is necessary for unprotecting
	for (const auto &k : character_list) {
		if (k->get_protecting() == ch) {
			k->set_protecting(0);
			CLR_AF_BATTLE(k, EAF_PROTECT);
		}

		if (k->get_touching() == ch) {
			k->set_touching(0);
			CLR_AF_BATTLE(k, EAF_PROTECT);
		}

		if (k->get_extra_victim() == ch) {
			k->set_extra_attack(kExtraAttackUnused, 0);
		}

		if (k->get_cast_char() == ch) {
			k->set_cast(0, 0, 0, 0, 0);
		}

		if (k->get_fighting() == ch && IN_ROOM(k) != kNowhere) {
			log("[Change fighting] Change victim");

			bool found = false;
			for (const auto j : world[ch->in_room]->people) {
				if (j->get_fighting() == k.get()) {
					act("Вы переключили внимание на $N3.", false, k.get(), 0, j, TO_CHAR);
					act("$n переключил$u на вас!", false, k.get(), 0, j, TO_VICT);
					k->set_fighting(j);
					found = true;

					break;
				}
			}

			if (!found
				&& need_stop) {
				stop_fighting(k.get(), false);
			}
		}
	}
}

int CharacterData::get_serial_num() {
	return serial_num_;
}

void CharacterData::set_serial_num(int num) {
	serial_num_ = num;
}

bool CharacterData::purged() const {
	return purged_;
}

const std::string &CharacterData::get_name_str() const {
	if (IS_NPC(this)) {
		return short_descr_;
	}
	return name_;
}

const std::string &CharacterData::get_name() const {
	return IS_NPC(this) ? get_npc_name() : get_pc_name();
}

void CharacterData::set_name(const char *name) {
	if (IS_NPC(this)) {
		set_npc_name(name);
	} else {
		set_pc_name(name);
	}
}

void CharacterData::set_pc_name(const char *name) {
	if (name) {
		name_ = name;
	} else {
		name_.clear();
	}
}

void CharacterData::set_npc_name(const char *name) {
	if (name) {
		short_descr_ = name;
	} else {
		short_descr_.clear();
	}
}

const char *CharacterData::get_pad(unsigned pad) const {
	if (pad < player_data.PNames.size()) {
		return player_data.PNames[pad].c_str();
	}
	return 0;
}

void CharacterData::set_pad(unsigned pad, const char *s) {
	if (pad >= player_data.PNames.size()) {
		return;
	}

	this->player_data.PNames[pad] = std::string(s);
}

const char *CharacterData::get_long_descr() const {
	return player_data.long_descr.c_str();
}

void CharacterData::set_long_descr(const char *s) {
	player_data.long_descr = std::string(s);
}

const char *CharacterData::get_description() const {
	return player_data.description.c_str();
}

void CharacterData::set_description(const char *s) {
	player_data.description = std::string(s);
}

ECharClass CharacterData::get_class() const {
	return chclass_;
}

void CharacterData::set_class(ECharClass chclass) {
	// Range includes player classes and NPC classes (and does not consider gaps between them).
	// Почему классы не пронумеровать подряд - загадка...
	if ((chclass < ECharClass::kFirst || chclass > ECharClass::kLast)
		&& chclass != ECharClass::kNPCBase && chclass != ECharClass::kMob) {
		log("WARNING: chclass=%d (%s:%d %s)", chclass, __FILE__, __LINE__, __func__);
	}
	chclass_ = chclass;
}

short CharacterData::get_level() const {
	return level_;
}

short CharacterData::get_level_add() const {
	return level_add_;
}

void CharacterData::set_level(short level) {
	if (IS_NPC(this)) {
		level_ = std::clamp(level, kMinCharLevel, kMaxMobLevel);
	} else {
		level_ = std::clamp(level, kMinCharLevel, kLevelImplementator);
	}
}

void CharacterData::set_level_add(short level_add) {
	// level_ + level_add_ не должно быть больше максимального уровня
	// все проверки находятся в GET_REAL_LEVEL()
	level_add_ = level_add;
}

long CharacterData::get_idnum() const {
	return idnum_;
}

void CharacterData::set_idnum(long idnum) {
	idnum_ = idnum;
}

int CharacterData::get_uid() const {
	return uid_;
}

void CharacterData::set_uid(int uid) {
	uid_ = uid;
}

long CharacterData::get_exp() const {
	return exp_;
}

void CharacterData::set_exp(long exp) {
	if (exp < 0) {
		log("WARNING: exp=%ld name=[%s] (%s:%d %s)", exp, get_name().c_str(), __FILE__, __LINE__, __func__);
	}
	exp_ = MAX(0, exp);
	msdp_report(msdp::constants::EXPERIENCE);
}

short CharacterData::get_remort() const {
	return remorts_;
}

short CharacterData::get_remort_add() const {
	return remorts_add_;
}

void CharacterData::set_remort(short num) {
	remorts_ = MAX(0, num);
}

void CharacterData::set_remort_add(short num) {
	remorts_add_ = num;
}

time_t CharacterData::get_last_logon() const {
	return last_logon_;
}

void CharacterData::set_last_logon(time_t num) {
	last_logon_ = num;
}

time_t CharacterData::get_last_exchange() const {
	return last_exchange_;
}

void CharacterData::set_last_exchange(time_t num) {
	last_exchange_ = num;
}

ESex CharacterData::get_sex() const {
	return player_data.sex;
}

void CharacterData::set_sex(const ESex sex) {
/*	if (to_underlying(sex) >= 0
		&& to_underlying(sex) < ESex::kLast) {*/
	if (sex < ESex::kLast) {
		player_data.sex = sex;
	}
}

ubyte CharacterData::get_weight() const {
	return player_data.weight;
}

void CharacterData::set_weight(const ubyte v) {
	player_data.weight = v;
}

ubyte CharacterData::get_height() const {
	return player_data.height;
}

void CharacterData::set_height(const ubyte v) {
	player_data.height = v;
}

ubyte CharacterData::get_religion() const {
	return player_data.Religion;
}

void CharacterData::set_religion(const ubyte v) {
	if (v < 2) {
		player_data.Religion = v;
	}
}

ubyte CharacterData::get_kin() const {
	return player_data.Kin;
}

void CharacterData::set_kin(const ubyte v) {
	if (v < kNumKins) {
		player_data.Kin = v;
	}
}

ubyte CharacterData::get_race() const {
	return player_data.Race;
}

void CharacterData::set_race(const ubyte v) {
	player_data.Race = v;
}

int CharacterData::get_hit() const {
	return points.hit;
}

void CharacterData::set_hit(const int v) {
	if (v >= -10)
		points.hit = v;
}

int CharacterData::get_max_hit() const {
	return points.max_hit;
}

void CharacterData::set_max_hit(const int v) {
	if (v >= 0) {
		points.max_hit = v;
	}
	msdp_report(msdp::constants::MAX_HIT);
}

sh_int CharacterData::get_move() const {
	return points.move;
}

void CharacterData::set_move(const sh_int v) {
	if (v >= 0)
		points.move = v;
}

sh_int CharacterData::get_max_move() const {
	return points.max_move;
}

void CharacterData::set_max_move(const sh_int v) {
	if (v >= 0) {
		points.max_move = v;
	}
	msdp_report(msdp::constants::MAX_MOVE);
}

long CharacterData::get_ruble() {
	return ruble;
}

void CharacterData::set_ruble(int ruble) {
	this->ruble = ruble;
}

long CharacterData::get_gold() const {
	return gold_;
}

long CharacterData::get_bank() const {
	return bank_gold_;
}

long CharacterData::get_total_gold() const {
	return get_gold() + get_bank();
}

/**
 * Добавление денег на руках, плюсуются только положительные числа.
 * \param need_log здесь и далее - логировать или нет изменения счета (=true)
 * \param clan_tax - проверять и снимать клан-налог или нет (=false)
 */
void CharacterData::add_gold(long num, bool need_log, bool clan_tax) {
	if (num < 0) {
		log("SYSERROR: num=%ld (%s:%d %s)", num, __FILE__, __LINE__, __func__);
		return;
	}
	if (clan_tax) {
		num -= ClanSystem::do_gold_tax(this, num);
	}
	set_gold(get_gold() + num, need_log);
}

// * см. add_gold()
void CharacterData::add_bank(long num, bool need_log) {
	if (num < 0) {
		log("SYSERROR: num=%ld (%s:%d %s)", num, __FILE__, __LINE__, __func__);
		return;
	}
	set_bank(get_bank() + num, need_log);
}

/**
 * Сет денег на руках, отрицательные числа просто обнуляют счет с
 * логированием бывшей суммы.
 */
void CharacterData::set_gold(long num, bool need_log) {
	if (get_gold() == num) {
		// чтобы с логированием не заморачиваться
		return;
	}
	num = MAX(0, MIN(kMaxMoneyKept, num));

	if (need_log && !IS_NPC(this)) {
		long change = num - get_gold();
		if (change > 0) {
			log("Gold: %s add %ld", get_name().c_str(), change);
		} else {
			log("Gold: %s remove %ld", get_name().c_str(), -change);
		}
		if (IN_ROOM(this) > 0) {
			MoneyDropStat::add(zone_table[world[IN_ROOM(this)]->zone_rn].vnum, change);
		}
	}

	gold_ = num;
	msdp_report(msdp::constants::GOLD);
}

// * см. set_gold()
void CharacterData::set_bank(long num, bool need_log) {
	if (get_bank() == num) {
		// чтобы с логированием не заморачиваться
		return;
	}
	num = MAX(0, MIN(kMaxMoneyKept, num));

	if (need_log && !IS_NPC(this)) {
		long change = num - get_bank();
		if (change > 0) {
			log("Gold: %s add %ld", get_name().c_str(), change);
		} else {
			log("Gold: %s remove %ld", get_name().c_str(), -change);
		}
	}

	bank_gold_ = num;
	msdp_report(msdp::constants::GOLD);
}

/**
 * Снятие находящихся на руках денег.
 * \param num - положительное число
 * \return - кол-во кун, которое не удалось снять с рук (нехватило денег)
 */
long CharacterData::remove_gold(long num, bool need_log) {
	if (num < 0) {
		log("SYSERROR: num=%ld (%s:%d %s)", num, __FILE__, __LINE__, __func__);
		return num;
	}
	if (num == 0) {
		return num;
	}

	long rest = 0;
	if (get_gold() >= num) {
		set_gold(get_gold() - num, need_log);
	} else {
		rest = num - get_gold();
		set_gold(0, need_log);
	}

	return rest;
}

// * см. remove_gold()
long CharacterData::remove_bank(long num, bool need_log) {
	if (num < 0) {
		log("SYSERROR: num=%ld (%s:%d %s)", num, __FILE__, __LINE__, __func__);
		return num;
	}
	if (num == 0) {
		return num;
	}

	long rest = 0;
	if (get_bank() >= num) {
		set_bank(get_bank() - num, need_log);
	} else {
		rest = num - get_bank();
		set_bank(0, need_log);
	}

	return rest;
}

/**
 * Попытка снятия денег с банка и, в случае остатка, с рук.
 * \return - кол-во кун, которое не удалось снять (нехватило денег в банке и на руках)
 */
long CharacterData::remove_both_gold(long num, bool need_log) {
	long rest = remove_bank(num, need_log);
	return remove_gold(rest, need_log);
}

// * Удача (мораль) для расчетов в скилах и вывода чару по счет все.
int CharacterData::calc_morale() const {
	return GET_REAL_CHA(this) / 2 + GET_MORALE(this);
//	return cha_app[GET_REAL_CHA(this)].morale + GET_MORALE(this);
}
///////////////////////////////////////////////////////////////////////////////
int CharacterData::get_str() const {
	check_purged(this, "get_str");
	return current_morph_->GetStr();
}

int CharacterData::get_inborn_str() const {
	return str_;
}

void CharacterData::set_str(int param) {
	str_ = MAX(1, param);
}

void CharacterData::inc_str(int param) {
	str_ = MAX(1, str_ + param);
}

int CharacterData::get_str_add() const {
	return str_add_;
}

void CharacterData::set_str_add(int param) {
	str_add_ = param;
}
///////////////////////////////////////////////////////////////////////////////
int CharacterData::get_dex() const {
	check_purged(this, "get_dex");
	return current_morph_->GetDex();
}

int CharacterData::get_inborn_dex() const {
	return dex_;
}

void CharacterData::set_dex(int param) {
	dex_ = MAX(1, param);
}

void CharacterData::inc_dex(int param) {
	dex_ = MAX(1, dex_ + param);
}

int CharacterData::get_dex_add() const {
	return dex_add_;
}

void CharacterData::set_dex_add(int param) {
	dex_add_ = param;
}
///////////////////////////////////////////////////////////////////////////////
int CharacterData::get_con() const {
	check_purged(this, "get_con");
	return current_morph_->GetCon();
}

int CharacterData::get_inborn_con() const {
	return con_;
}

void CharacterData::set_con(int param) {
	con_ = MAX(1, param);
}
void CharacterData::inc_con(int param) {
	con_ = MAX(1, con_ + param);
}

int CharacterData::get_con_add() const {
	return con_add_;
}

void CharacterData::set_con_add(int param) {
	con_add_ = param;
}
//////////////////////////////////////

int CharacterData::get_int() const {
	check_purged(this, "get_int");
	return current_morph_->GetIntel();
}

int CharacterData::get_inborn_int() const {
	return int_;
}

void CharacterData::set_int(int param) {
	int_ = MAX(1, param);
}

void CharacterData::inc_int(int param) {
	int_ = MAX(1, int_ + param);
}

int CharacterData::get_int_add() const {
	return int_add_;
}

void CharacterData::set_int_add(int param) {
	int_add_ = param;
}
////////////////////////////////////////
int CharacterData::get_wis() const {
	check_purged(this, "get_wis");
	return current_morph_->GetWis();
}

int CharacterData::get_inborn_wis() const {
	return wis_;
}

void CharacterData::set_wis(int param) {
	wis_ = MAX(1, param);
}

void CharacterData::set_who_mana(unsigned int param) {
	player_specials->saved.who_mana = param;
}

void CharacterData::set_who_last(time_t param) {
	char_specials.who_last = param;
}

unsigned int CharacterData::get_who_mana() {
	return player_specials->saved.who_mana;
}

time_t CharacterData::get_who_last() {
	return char_specials.who_last;
}

void CharacterData::inc_wis(int param) {
	wis_ = MAX(1, wis_ + param);
}

int CharacterData::get_wis_add() const {
	return wis_add_;
}

void CharacterData::set_wis_add(int param) {
	wis_add_ = param;
}
///////////////////////////////////////////////////////////////////////////////
int CharacterData::get_cha() const {
	check_purged(this, "get_cha");
	return current_morph_->GetCha();
}

int CharacterData::get_inborn_cha() const {
	return cha_;
}

void CharacterData::set_cha(int param) {
	cha_ = MAX(1, param);
}
void CharacterData::inc_cha(int param) {
	cha_ = MAX(1, cha_ + param);
}

int CharacterData::get_cha_add() const {
	return cha_add_;
}
void CharacterData::set_cha_add(int param) {
	cha_add_ = param;
}
int CharacterData::get_skill_bonus() const {
	return skill_bonus_;
}
void CharacterData::set_skill_bonus(int param) {
	skill_bonus_ = param;
}

///////////////////////////////////////////////////////////////////////////////

void CharacterData::clear_add_apply_affects() {
	// Clear all affect, because recalc one
	memset(&add_abils, 0, sizeof(char_played_ability_data));
	set_remort_add(0);
	set_level_add(0);
	set_str_add(0);
	set_dex_add(0);
	set_con_add(0);
	set_int_add(0);
	set_wis_add(0);
	set_cha_add(0);
	set_skill_bonus(0);
}
///////////////////////////////////////////////////////////////////////////////
int CharacterData::get_zone_group() const {
	const auto rnum = get_rnum();
	if (IS_NPC(this)
		&& rnum >= 0
		&& mob_index[rnum].zone >= 0) {
		const auto zone = mob_index[rnum].zone;
		return MAX(1, zone_table[zone].group);
	}

	return 1;
}

//===================================
//Polud формы и все что с ними связано
//===================================

bool CharacterData::know_morph(const std::string &morph_id) const {
	return std::find(morphs_.begin(), morphs_.end(), morph_id) != morphs_.end();
}

void CharacterData::add_morph(const std::string &morph_id) {
	morphs_.push_back(morph_id);
};

void CharacterData::clear_morphs() {
	morphs_.clear();
};

const CharacterData::morphs_list_t &CharacterData::get_morphs() {
	return morphs_;
};
// обрезает строку и выдергивает из нее предтитул
std::string CharacterData::get_title() {
	std::string tmp = this->player_data.title;
	size_t pos = tmp.find('/');
	if (pos == std::string::npos) {
		return std::string();
	}
	tmp = tmp.substr(0, pos);
	pos = tmp.find(';');

	return pos == std::string::npos
		   ? tmp
		   : tmp.substr(0, pos);
};

std::string CharacterData::get_pretitle() {
	std::string tmp = std::string(this->player_data.title);
	size_t pos = tmp.find('/');
	if (pos == std::string::npos) {
		return std::string();
	}
	tmp = tmp.substr(0, pos);
	pos = tmp.find(';');
	return pos == std::string::npos
		   ? std::string()
		   : tmp.substr(pos + 1, tmp.length() - (pos + 1));
}

std::string CharacterData::get_race_name() {
	return PlayerRace::GetRaceNameByNum(GET_KIN(this), GET_RACE(this), GET_SEX(this));
}

std::string CharacterData::get_morph_desc() const {
	return current_morph_->GetMorphDesc();
};

std::string CharacterData::get_morphed_name() const {
	return current_morph_->GetMorphDesc() + " - " + this->get_name();
};

std::string CharacterData::get_morphed_title() const {
	return current_morph_->GetMorphTitle();
};

std::string CharacterData::only_title_noclan() {
	std::string result = this->get_name();
	std::string title = this->get_title();
	std::string pre_title = this->get_pretitle();

	if (!pre_title.empty())
		result = pre_title + " " + result;

	if (!title.empty() && this->get_level() >= MIN_TITLE_LEV)
		result = result + ", " + title;

	return result;
}

std::string CharacterData::clan_for_title() {
	std::string result = std::string();

	bool imm = IS_IMMORTAL(this) || PRF_FLAGGED(this, PRF_CODERINFO);

	if (CLAN(this) && !imm)
		result = result + "(" + GET_CLAN_STATUS(this) + ")";

	return result;
}

std::string CharacterData::only_title() {
	std::string result = this->clan_for_title();
	if (!result.empty())
		result = this->only_title_noclan() + " " + result;
	else
		result = this->only_title_noclan();

	return result;
}

std::string CharacterData::noclan_title() {
	std::string race = this->get_race_name();
	std::string result = this->only_title_noclan();

	if (result == this->get_name()) {
		result = race + " " + result;
	}

	return result;
}

std::string CharacterData::race_or_title() {
	std::string result = this->clan_for_title();

	if (!result.empty())
		result = this->noclan_title() + " " + result;
	else
		result = this->noclan_title();

	return result;
}

size_t CharacterData::get_morphs_count() const {
	return morphs_.size();
};

std::string CharacterData::get_cover_desc() {
	return current_morph_->CoverDesc();
}

void CharacterData::set_morph(MorphPtr morph) {
	morph->SetChar(this);
	morph->InitSkills(this->get_skill(ESkill::kMorph));
	morph->InitAbils();
	this->current_morph_ = morph;
};

void CharacterData::reset_morph() {
	int value = this->get_trained_skill(ESkill::kMorph);
	send_to_char(str(boost::format(current_morph_->GetMessageToChar()) % "человеком") + "\r\n", this);
	act(str(boost::format(current_morph_->GetMessageToRoom()) % "человеком").c_str(), true, this, 0, 0, TO_ROOM);
	this->current_morph_ = GetNormalMorphNew(this);
	this->set_morphed_skill(ESkill::kMorph, (MIN(kSkillCapOnZeroRemort + GET_REAL_REMORT(this) * 5, value)));
//	REMOVE_BIT(AFF_FLAGS(this, AFF_MORPH), AFF_MORPH);
};

bool CharacterData::is_morphed() const {
	return current_morph_->Name() != "Обычная" || AFF_FLAGGED(this, EAffectFlag::AFF_MORPH);
};

void CharacterData::set_normal_morph() {
	current_morph_ = GetNormalMorphNew(this);
}

bool CharacterData::isAffected(const EAffectFlag flag) const {
	return current_morph_->isAffected(flag);
}

const IMorph::affects_list_t &CharacterData::GetMorphAffects() {
	return current_morph_->GetAffects();
}

//===================================
//-Polud
//===================================

bool CharacterData::get_role(unsigned num) const {
	bool result = false;
	if (num < role_.size()) {
		result = role_.test(num);
	} else {
		log("SYSERROR: num=%u (%s:%d)", num, __FILE__, __LINE__);
	}
	return result;
}

void CharacterData::set_role(unsigned num, bool flag) {
	if (num < role_.size()) {
		role_.set(num, flag);
	} else {
		log("SYSERROR: num=%u (%s:%d)", num, __FILE__, __LINE__);
	}
}

void CharacterData::msdp_report(const std::string &name) {
	if (nullptr != desc) {
		desc->msdp_report(name);
	}
}

void CharacterData::removeGroupFlags() {
	AFF_FLAGS(this).unset(EAffectFlag::AFF_GROUP);
	PRF_FLAGS(this).unset(PRF_SKIRMISHER);
}

void CharacterData::add_follower(CharacterData *ch) {

	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOGROUP))
		return;
	add_follower_silently(ch);

	if (!IS_HORSE(ch)) {
		act("Вы начали следовать за $N4.", false, ch, 0, this, TO_CHAR);
		act("$n начал$g следовать за вами.", true, ch, 0, this, TO_VICT);
		act("$n начал$g следовать за $N4.", true, ch, 0, this, TO_NOTVICT | TO_ARENA_LISTEN);
	}
}

CharacterData::followers_list_t CharacterData::get_followers_list() const {
	CharacterData::followers_list_t result;
	auto pos = followers;
	while (pos) {
		const auto follower = pos->ch;
		result.push_back(follower);
		pos = pos->next;
	}
	return result;
}

bool CharacterData::low_charm() const {
	for (const auto &aff : affected) {
		if (aff->type == SPELL_CHARM
			&& aff->duration <= 1) {
			return true;
		}
	}
	return false;
}

void CharacterData::cleanup_script() {
	script->cleanup();
}

void CharacterData::add_follower_silently(CharacterData *ch) {
	struct Follower *k;

	if (ch->has_master()) {
		log("SYSERR: add_follower_implementation(%s->%s) when master existing(%s)...",
			GET_NAME(ch), get_name().c_str(), GET_NAME(ch->get_master()));
		return;
	}

	if (ch == this) {
		return;
	}

	ch->set_master(this);

	CREATE(k, 1);

	k->ch = ch;
	k->next = followers;
	followers = k;
}

const CharacterData::role_t &CharacterData::get_role_bits() const {
	return role_;
}

// добавляет указанного ch чара в список атакующих босса с параметром type
// или обновляет его данные в этом списке
void CharacterData::add_attacker(CharacterData *ch, unsigned type, int num) {
	if (!IS_NPC(this) || IS_NPC(ch) || !get_role(MOB_ROLE_BOSS)) {
		return;
	}

	int uid = ch->get_uid();
	if (IS_CHARMICE(ch) && ch->has_master()) {
		uid = ch->get_master()->get_uid();
	}

	auto i = attackers_.find(uid);
	if (i != attackers_.end()) {
		switch (type) {
			case ATTACKER_DAMAGE: i->second.damage += num;
				break;
			case ATTACKER_ROUNDS: i->second.rounds += num;
				break;
		}
	} else {
		attacker_node tmp_node;
		switch (type) {
			case ATTACKER_DAMAGE: tmp_node.damage = num;
				break;
			case ATTACKER_ROUNDS: tmp_node.rounds = num;
				break;
		}
		attackers_.insert(std::make_pair(uid, tmp_node));
	}
}

// возвращает количественный параметр по флагу type указанного ch чара
// из списка атакующих данного босса
int CharacterData::get_attacker(CharacterData *ch, unsigned type) const {
	if (!IS_NPC(this) || IS_NPC(ch) || !get_role(MOB_ROLE_BOSS)) {
		return -1;
	}
	auto i = attackers_.find(ch->get_uid());
	if (i != attackers_.end()) {
		switch (type) {
			case ATTACKER_DAMAGE: return i->second.damage;
			case ATTACKER_ROUNDS: return i->second.rounds;
		}
	}
	return 0;
}

// поиск в списке атакующих нанесшего максимальный урон, который при этом
// находится в данный момент в этой же комнате с боссом и онлайн
std::pair<int /* uid */, int /* rounds */> CharacterData::get_max_damager_in_room() const {
	std::pair<int, int> damager(-1, 0);

	if (!IS_NPC(this) || !get_role(MOB_ROLE_BOSS)) {
		return damager;
	}

	int max_dmg = 0;
	for (const auto i : world[this->in_room]->people) {
		if (!IS_NPC(i) && i->desc) {
			auto it = attackers_.find(i->get_uid());
			if (it != attackers_.end()) {
				if (it->second.damage > max_dmg) {
					max_dmg = it->second.damage;
					damager.first = it->first;
					damager.second = it->second.rounds;
				}
			}
		}
	}

	return damager;
}

// обновление босса вне боя по прошествии MOB_RESTORE_TIMER секунд
void CharacterData::restore_mob() {
	restore_timer_ = 0;
	attackers_.clear();

	GET_HIT(this) = GET_REAL_MAX_HIT(this);
	GET_MOVE(this) = GET_REAL_MAX_MOVE(this);
	update_pos(this);

	for (int i = 0; i <= SPELLS_COUNT; ++i) {
		GET_SPELL_MEM(this, i) = GET_SPELL_MEM(&mob_proto[GET_MOB_RNUM(this)], i);
	}
	GET_CASTER(this) = GET_CASTER(&mob_proto[GET_MOB_RNUM(this)]);
}
// Кудояр 
void CharacterData::restore_npc() {
	if(!IS_NPC(this)) return;
	
	attackers_.clear();
	auto proto = (&mob_proto[GET_MOB_RNUM(this)]);
	// ресторим хпшки / мувы
		
	GET_HIT(this) = 1 + GET_HIT(proto);
	GET_MAX_HIT(this) = GET_HIT(this) + floorf(GET_MAX_HIT(proto));
	
	GET_MOVE(this) = GET_REAL_MAX_MOVE(proto);
	update_pos(this);
	// ресторим хиты / дамы / ас / армор
	GET_WEIGHT(this) = GET_WEIGHT(proto);
	GET_HEIGHT(this) = GET_HEIGHT(proto);
	GET_SIZE(this) = GET_SIZE(proto);
	GET_HR(this) = GET_HR(proto);
	GET_AC(this) = GET_AC(proto);
	GET_DR(this) = GET_DR(proto);
	GET_ARMOUR(this) = GET_ARMOUR(proto);
	GET_INITIATIVE(this) = GET_INITIATIVE(proto);
	GET_MORALE(this) = GET_MORALE(proto);
	// ресторим резисты ФР/МР/АР
	GET_AR(this) = GET_AR(proto);
	GET_MR(this) = GET_MR(proto);
	GET_PR(this) = GET_PR(proto);
	// ресторим имена
	this->player_data.PNames[0] = proto->player_data.PNames[0];
	this->player_data.PNames[1] = proto->player_data.PNames[1];
	this->player_data.PNames[2] = proto->player_data.PNames[2];
	this->player_data.PNames[3] = proto->player_data.PNames[3];
	this->player_data.PNames[4] = proto->player_data.PNames[4];
	this->player_data.PNames[5] = proto->player_data.PNames[5];
	this->set_pc_name(GET_PC_NAME(proto));
	this->set_npc_name(GET_NAME(proto));
    // кубики // екстра атаки
	this->mob_specials.damnodice = proto->mob_specials.damnodice;
	this->mob_specials.damsizedice = proto->mob_specials.damsizedice;
	this->mob_specials.ExtraAttack = proto->mob_specials.ExtraAttack;
	// this->mob_specials.damnodice = 1;
	// this->mob_specials.damsizedice = 1;
	// this->mob_specials.ExtraAttack = 0;
	//флаги
	MOB_FLAGS(this) = MOB_FLAGS(proto);
	this->set_touching(nullptr);
	this->set_protecting(nullptr);
	// ресторим статы
	proto->set_normal_morph();
	this->set_str(GET_REAL_STR(proto));
	this->set_int(GET_REAL_INT(proto));
	this->set_wis(GET_REAL_WIS(proto));
	this->set_dex(GET_REAL_DEX(proto));
	this->set_con(GET_REAL_CON(proto));
	this->set_cha(GET_REAL_CHA(proto));
	// ресторим мем	
	for (int i = 0; i <= SPELLS_COUNT; ++i) {
		GET_SPELL_MEM(this, i) = GET_SPELL_MEM(proto, i);
	}
	// рестор для скилов
	for (const auto i : AVAILABLE_SKILLS) { 
		this->set_skill(i, GET_SKILL(proto, i));
	}
	// рестор для фитов
	for (int i = 1; i < kMaxFeats; i++) {
		if (!HAVE_FEAT(proto, i)) {
				UNSET_FEAT(this, i);
			}
	}
	GET_CASTER(this) = GET_CASTER(proto);
}

void CharacterData::report_loop_error(const CharacterData::ptr_t master) const {
	std::stringstream ss;
	ss << "Обнаружена ошибка логики: попытка сделать цикл в цепочке последователей.\nТекущая цепочка лидеров: ";
	master->print_leaders_chain(ss);
	ss << "\nПопытка сделать персонажа [" << master->get_name() << "] лидером персонажа [" << get_name() << "]";
	mudlog(ss.str().c_str(), DEF, -1, ERRLOG, true);

	std::stringstream additional_info;
	additional_info << "Потенциальный лидер: name=[" << master->get_name() << "]"
					<< "; адрес структуры: " << master << "; текущий лидер: ";
	if (master->has_master()) {
		additional_info << "name=[" << master->get_master()->get_name() << "]"
						<< "; адрес структуры: " << master->get_master() << "";
	} else {
		additional_info << "<отсутствует>";
	}
	additional_info << "\nПоследователь: name=[" << get_name() << "]"
					<< "; адрес структуры: " << this << "; текущий лидер: ";
	if (has_master()) {
		additional_info << "name=[" << get_master()->get_name() << "]"
						<< "; адрес структуры: " << get_master() << "";
	} else {
		additional_info << "<отсутствует>";
	}
	mudlog(additional_info.str().c_str(), DEF, -1, ERRLOG, true);

	ss << "\nТекущий стек будет распечатан в ERRLOG.";
	debug::backtrace(runtime_config.logs(ERRLOG).handle());
	mudlog(ss.str().c_str(), DEF, kLevelImplementator, ERRLOG, false);
}

void CharacterData::print_leaders_chain(std::ostream &ss) const {
	if (!has_master()) {
		ss << "<пуста>";
		return;
	}

	bool first = true;
	for (auto master = get_master(); master; master = master->get_master()) {
		ss << (first ? "" : " -> ") << "[" << master->get_name() << "]";
		first = false;
	}
}

void CharacterData::set_master(CharacterData::ptr_t master) {
	if (makes_loop(master)) {
		report_loop_error(master);
		return;
	}
	m_master = master;
}

bool CharacterData::makes_loop(CharacterData::ptr_t master) const {
	while (master) {
		if (master == this) {
			return true;
		}
		master = master->get_master();
	}

	return false;
}

// инкремент и проверка таймера на рестор босса,
// который находится вне боя и до этого был кем-то бит
// (т.к. имеет не нулевой список атакеров)
void CharacterData::inc_restore_timer(int num) {
	if (get_role(MOB_ROLE_BOSS) && !attackers_.empty() && !get_fighting()) {
		restore_timer_ += num;
		if (restore_timer_ > num) {
			restore_mob();
		}
	}
}

//метод передачи отладочного сообщения:
//имморталу, тестеру или кодеру
//остальные параметры - функция printf
void CharacterData::send_to_TC(bool to_impl, bool to_tester, bool to_coder, const char *msg, ...) {
	bool needSend = false;
	// проверка на ситуацию "чармис стоит, хозяина уже нет с нами"
	if (IS_CHARMICE(this) && !this->has_master()) {
		sprintf(buf, "[WARNING] CharacterData::send_to_TC. Чармис без хозяина: %s", this->get_name().c_str());
		mudlog(buf, CMP, kLevelGod, SYSLOG, true);
		return;
	}
	if ((IS_CHARMICE(this) && this->get_master()->is_npc()) //если это чармис у нпц
		|| (IS_NPC(this) && !IS_CHARMICE(this))) //просто непись
		return;

	if (to_impl &&
		(IS_IMPL(this) || (IS_CHARMICE(this) && IS_IMPL(this->get_master()))))
		needSend = true;
	if (!needSend && to_coder &&
		(PRF_FLAGGED(this, PRF_CODERINFO) || (IS_CHARMICE(this) && (PRF_FLAGGED(this->get_master(), PRF_CODERINFO)))))
		needSend = true;
	if (!needSend && to_tester &&
		(PRF_FLAGGED(this, PRF_TESTER) || (IS_CHARMICE(this) && (PRF_FLAGGED(this->get_master(), PRF_TESTER)))))
		needSend = true;
	if (!needSend)
		return;

	va_list args;
	char tmpbuf[kMaxStringLength];

	va_start(args, msg);
	vsnprintf(tmpbuf, sizeof(tmpbuf), msg, args);
	va_end(args);

	if (!tmpbuf) {
		sprintf(buf, "[WARNING] CharacterData::send_to_TC. Передано пустое сообщение");
		mudlog(buf, BRF, kLevelGod, SYSLOG, true);
		return;
	}
	// проверка на нпц была ранее. Шлем хозяину чармиса или самому тестеру
	send_to_char(tmpbuf, IS_CHARMICE(this) ? this->get_master() : this);
}

bool CharacterData::have_mind() const {
	if (!AFF_FLAGGED(this, EAffectFlag::AFF_CHARM) && !IS_HORSE(this))
		return true;
	return false;
}

bool CharacterData::has_horse(bool same_room) const {
	struct Follower *f;

	if (IS_NPC(this)) {
		return false;
	}

	for (f = this->followers; f; f = f->next) {
		if (IS_NPC(f->ch) && AFF_FLAGGED(f->ch, EAffectFlag::AFF_HORSE)
			&& (!same_room || this->in_room == IN_ROOM(f->ch))) {
			return true;
		}
	}
	return false;
}
// персонаж на лошади?
bool CharacterData::ahorse() const {
	return AFF_FLAGGED(this, EAffectFlag::AFF_HORSE) && this->has_horse(true);
}

bool CharacterData::isHorsePrevents() {
	if (this->ahorse()) {
		act("Вам мешает $N.", false, this, 0, this->get_horse(), TO_CHAR);
		return true;
	}
	return false;
};

bool CharacterData::drop_from_horse() {
	CharacterData *plr = nullptr;
	// вызвали для лошади
	if (IS_HORSE(this) && this->get_master()->ahorse()) {
		plr = this->get_master();
		act("$N сбросил$G вас со своей спины.", false, plr, 0, this, TO_CHAR);
	}
	// вызвали для седока
	if (this->ahorse()) {
		plr = this;
		act("Вы упали с $N1.", false, plr, 0, this->get_horse(), TO_CHAR);
	}
	if (plr == nullptr || !plr->ahorse())
		return false;
	sprintf(buf, "%s свалил%s со своего скакуна.", GET_PAD(plr, 0), GET_CH_SUF_2(plr));
	act(buf, false, plr, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	AFF_FLAGS(plr).unset(EAffectFlag::AFF_HORSE);
	WAIT_STATE(this, 3 * kPulseViolence);
	if (GET_POS(plr) > EPosition::kSit)
		GET_POS(plr) = EPosition::kSit;
	return true;
};

void CharacterData::dismount() {
	if (!this->ahorse() || this->get_horse() == nullptr)
		return;
	if (!IS_NPC(this) && this->has_horse(true)) {
		AFF_FLAGS(this).unset(EAffectFlag::AFF_HORSE);
	}
	act("Вы слезли со спины $N1.", false, this, 0, this->get_horse(), TO_CHAR);
	act("$n соскочил$g с $N1.", false, this, 0, this->get_horse(), TO_ROOM | TO_ARENA_LISTEN);
}

CharacterData *CharacterData::get_horse() {
	struct Follower *f;

	if (IS_NPC(this))
		return nullptr;

	for (f = this->followers; f; f = f->next) {
		if (IS_NPC(f->ch) && AFF_FLAGGED(f->ch, EAffectFlag::AFF_HORSE)) {
			return (f->ch);
		}
	}
	return nullptr;
};

obj_sets::activ_sum &CharacterData::obj_bonus() {
	return obj_bonus_;
}

player_special_data::player_special_data() :
	poofin(nullptr),
	poofout(nullptr),
	aliases(nullptr),
	may_rent(0),
	agressor(0),
	agro_time(0),
	rskill(0),
	portals(0),
	logs(nullptr),
	Exchange_filter(nullptr),
	Karma(nullptr),
	clanStatus(nullptr) {
}

player_special_data_saved::player_special_data_saved() :
	wimp_level(0),
	invis_level(0),
	load_room(0),
	bad_pws(0),
	DrunkState(0),
	olc_zone(0),
	NameGod(0),
	NameIDGod(0),
	GodsLike(0),
	stringLength(0),
	stringWidth(0),
	Rip_arena(0),
	rip_arena_dom(0),
	kill_arena_dom(0),
	Rip_mob(0),
	Rip_pk(0),
	Rip_dt(0),
	Rip_other(0),
	Win_arena(0),
	Rip_mob_this(0),
	Rip_pk_this(0),
	Rip_dt_this(0),
	Rip_other_this(0),
	Exp_arena(0),
	Exp_mob(0),
	Exp_pk(0),
	Exp_dt(0),
	Exp_other(0),
	Exp_mob_this(0),
	Exp_pk_this(0),
	Exp_dt_this(0),
	Exp_other_this(0),
	ntfyExchangePrice(0),
	HiredCost(0),
	who_mana(0),
	telegram_id(0),
	lastGloryRespecTime(0) {
	memset(EMail, 0, sizeof(EMail));
	memset(LastIP, 0, sizeof(LastIP));
}

// dummy spec area for mobs
player_special_data::shared_ptr player_special_data::s_for_mobiles = std::make_shared<player_special_data>();

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
