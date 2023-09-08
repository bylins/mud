// $RCSfile: char.cpp,v $     $Date: 2012/01/27 04:04:43 $     $Revision: 1.85 $
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

//#include "world_characters.h"
//#include "game_fight/pk.h"
#include "handler.h"
#include "administration/privilege.h"
#include "char_player.h"
#include "player_races.h"
//#include "game_mechanics/celebrates.h"
#include "cache.h"
#include "game_fight/fight.h"
#include "house.h"
#include "msdp/msdp_constants.h"
#include "backtrace.h"
#include "structs/global_objects.h"
#include "liquid.h"

#include <third_party_libs/fmt/include/fmt/format.h>
#include <random>

std::string PlayerI::empty_const_str;
MapSystem::Options PlayerI::empty_map_options;

namespace {

// * На перспективу - втыкать во все методы character.
void check_purged(const CharData *ch, const char *fnc) {
	if (ch->purged()) {
		log("SYSERR: Using purged character (%s).", fnc);
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
	}
}

} // namespace


ProtectedCharData::ProtectedCharData() : m_rnum(kNobody) {
}

ProtectedCharData::ProtectedCharData(const ProtectedCharData &rhv) : m_rnum(kNobody) {
	*this = rhv;
}

void ProtectedCharData::set_rnum(const MobRnum rnum) {
	if (rnum != m_rnum) {
		const auto old_rnum = m_rnum;

		m_rnum = rnum;

		for (const auto &observer : m_rnum_change_observers) {
			observer->notify(*this, old_rnum);
		}
	}
}

ProtectedCharData &ProtectedCharData::operator=(const ProtectedCharData &rhv) {
	if (this != &rhv) {
		set_rnum(rhv.m_rnum);
	}

	return *this;
}

CharData::CharData() :
	chclass_(ECharClass::kUndefined),
	role_(MOB_ROLE_TOTAL_NUM),
	in_room(Rooms::UNDEFINED_ROOM_VNUM),
	m_wait(~0u),
	m_master(nullptr),
	proto_script(new ObjData::triggers_list_t()),
	script(new Script()),
	followers(nullptr) {
	this->zero_init();
	current_morph_ = GetNormalMorphNew(this);
	caching::character_cache.Add(this);
	this->set_skill(ESkill::kGlobalCooldown, 1);
}

CharData::~CharData() {
	this->purge();
}
void CharData::set_type_charmice(int type) {
	this->type_charmice_ = type;
}

int CharData::get_type_charmice() {
	return this->type_charmice_;

}
void CharData::set_souls(int souls) {
	this->souls = souls;
}

void CharData::inc_souls() {
	this->souls++;
}

void CharData::dec_souls() {
	this->souls--;
}

int CharData::get_souls() {
	return this->souls;
}

bool CharData::in_used_zone() const {
	if (IS_MOB(this)) {
		return 0 != zone_table[world[in_room]->zone_rn].used;
	}
	return false;
}

//вычисление штрафов за голод и жажду
//P_DAMROLL, P_HITROLL, P_CAST, P_MEM_GAIN, P_MOVE_GAIN, P_HIT_GAIN
float CharData::get_cond_penalty(int type) const {
	if (this->IsNpc()) return 1;
	if (!(GET_COND_M(this, FULL) || GET_COND_M(this, THIRST))) return 1;

	auto penalty{0.0};
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
	penalty = 100.0 - std::clamp(penalty, 0.0, 100.0);
	penalty /= 100.0;
	return penalty;
}

void CharData::reset() {
	int i;

	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		GET_EQ(this, i) = nullptr;
	}
	memset((void *) &add_abils, 0, sizeof(add_abils));

	followers = nullptr;
	m_master = nullptr;
	in_room = kNowhere;
	carrying = nullptr;
	next_fighting = nullptr;
	remove_protecting();
	set_touching(nullptr);
	battle_affects = clear_flags;
	poisoner = 0;
	SetEnemy(nullptr);
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

	this->caster_level = 0;
	this->damage_level = 0;

	PlayerI::reset();
}

void CharData::set_abstinent() {
	int duration = CalcDuration(this, 2, std::max(0, GET_DRUNK_STATE(this) - kDrunked), 4, 2, 5);

	if (CanUseFeat(this, EFeat::kDrunkard)) {
		duration /= 2;
	}

	Affect<EApply> af;
	af.type = ESpell::kAbstinent;
	af.bitvector = to_underlying(EAffect::kAbstinent);
	af.duration = duration;

	af.location = EApply::kAc;
	af.modifier = 20;
	ImposeAffect(this, af, false, false, false, false);

	af.location = EApply::kHitroll;
	af.modifier = -2;
	ImposeAffect(this, af, false, false, false, false);

	af.location = EApply::kDamroll;
	af.modifier = -2;
	ImposeAffect(this, af, false, false, false, false);
}

void CharData::affect_remove(const char_affects_list_t::iterator &affect_i) {
	int was_lgt = AFF_FLAGGED(this, EAffect::kSingleLight) ? kLightYes : kLightNo;
	long was_hlgt = AFF_FLAGGED(this, EAffect::kHolyLight) ? kLightYes : kLightNo;
	long was_hdrk = AFF_FLAGGED(this, EAffect::kHolyDark) ? kLightYes : kLightNo;

	if (affected.empty()) {
		log("SYSERR: affect_remove(%s) when no affects...", GET_NAME(this));
		return;
	}

	const auto af = *affect_i;
	affect_modify(this, af->location, af->modifier, static_cast<EAffect>(af->bitvector), false);
	if (af->type == ESpell::kAbstinent) {
		if (player_specials) {
			GET_DRUNK_STATE(this) = GET_COND(this, DRUNK) = std::min(GET_COND(this, DRUNK), kDrunked - 1);
		} else {
			log("SYSERR: player_specials is not set.");
		}
	}
	if (af->type == ESpell::kDrunked && af->duration == 0) {
		set_abstinent();
	}

	affected.erase(affect_i);

	affect_total(this);
	CheckLight(this, kLightUndef, was_lgt, was_hlgt, was_hdrk, 1);
}

bool CharData::has_any_affect(const affects_list_t &affects) {
	for (const auto &affect : affects) {
		if (AFF_FLAGGED(this, affect)) {
			return true;
		}
	}

	return false;
}

size_t CharData::remove_random_affects(const size_t count) {
	auto last_type{ESpell::kUndefined};
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
		RemoveAffectFromChar(this, affect_i->get()->type);
	}

	return to_remove;
}

/**
* Обнуление всех полей Character (аналог конструктора),
* вынесено в отдельную функцию, чтобы дергать из purge().
*/
void CharData::zero_init() {
	set_sex(EGender::kMale);
	set_race(0);
	protecting_ = nullptr;
	touching_ = nullptr;
	enemy_ = nullptr;
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
	type_charmice_ = 0;
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
	caster_level = 0;
	damage_level = 0;
	pk_list = nullptr;
	track_dirs = 0;
	check_aggressive = false;
	extract_timer = 0;
	initiative = 0;
	battle_counter = 0;
	round_counter = 0;
	poisoner = 0;
	dl_list = nullptr;
	agrobd = false;

	memset(&extra_attack_, 0, sizeof(extra_attack_type));
	memset(&cast_attack_, 0, sizeof(CastAttack));
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

	mem_queue.Clear();

	memset(&Temporary, 0, sizeof(FlagData));
	memset(&battle_affects, 0, sizeof(FlagData));
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
void CharData::purge() {
	caching::character_cache.Remove(this);

	if (!get_name().empty()) {
		log("[FREE CHAR] (%s)", GET_NAME(this));
	}
	if (this->who_protecting()) {
//		std::stringstream ss;
//		ss << "Чар " << GET_PAD(this ,0) <<  " выходит из игры его прикрывал " << GET_PAD(this->who_protecting(), 0) << std::endl; 
		if (this == this->who_protecting()->get_protecting()) {
//			ss << "Совпал прикрывающий и упавший в лд снимаю флаг прикрышки" << std::endl;
			this->who_protecting()->remove_protecting();
		}
//		mudlog(ss.str(), CMP, kLvlImmortal, SYSLOG, true);
	}
	int i, id = -1;
	struct alias_data *a;

	if (!this->IsNpc() && !get_name().empty()) {
		id = get_ptable_by_name(GET_NAME(this));
		if (id >= 0) {
			player_table[id].level = GetRealLevel(this);
			player_table[id].remorts = GetRealRemort(this);
			player_table[id].activity = number(0, OBJECT_SAVE_ACTIVITY - 1);
		}
	}

	if (!this->IsNpc() || (this->IsNpc() && GET_MOB_RNUM(this) == -1)) {
		if (this->IsNpc() && this->mob_specials.Questor)
			free(this->mob_specials.Questor);
		pk_free_list(this);
		this->summon_helpers.clear();
	} else if ((i = GET_MOB_RNUM(this))
		>= 0) {    // otherwise, free strings only if the string is not pointing at proto

		if (this->mob_specials.Questor && this->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
			free(this->mob_specials.Questor);
	}

	while (!this->affected.empty()) {
		affect_remove(affected.begin());
	}

	while (this->timed) {
		ExpireTimedSkill(this, this->timed);
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

		if (EXCHANGE_FILTER(this)) {
			free(EXCHANGE_FILTER(this));
		}
		EXCHANGE_FILTER(this) = nullptr;

		clear_ignores();

		if (GET_CLAN_STATUS(this)) {
			free(GET_CLAN_STATUS(this));
		}

		LOGON_LIST(this).clear();

		this->player_specials.reset();

		if (this->IsNpc()) {
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

/*
 * Умение с учетом всех бонусов и штрафов (экипировка, таланты, яд).
 */
int CharData::GetSkill(const ESkill skill_id) const {
	int skill = GetMorphSkill(skill_id) + GetEquippedSkill(skill_id);

	if (skill) {
		skill += GetAddSkill(skill_id);
	}

	if (AFF_FLAGGED(this, EAffect::kSkillReduce)) {
		skill -= skill * GET_POISON(this) / 100;
	}

	if (ROOM_FLAGGED(this->in_room, ERoomFlag::kDominationArena)) {
		return std::clamp(skill, 0, CalcSkillRemortCap(this));
	} else {
		return skill;
	}
}

/*
 * Умение с учетом талантов, но без экипировки.
 */
int CharData::GetSkillWithoutEquip(const ESkill skill_id) const {
	auto skill = GetTrainedSkill(skill_id);
	if (skill) {
		skill += GetAddSkill(skill_id);
	}
	return skill;
}

/*
 * Бонусы умения от экипировки. Учитываются, только
 * если у персонажа уже изучено данное умение.
 */
int CharData::GetEquippedSkill(const ESkill skill_id) const {
	int skill = 0;
	bool is_native = this->IsNpc() || MUD::Class(chclass_).skills[skill_id].IsValid();
	for (const auto item : equipment) {
		if (item && is_native) {
				skill += item->get_skill(skill_id);
		}
	}
	if (is_native) {
		skill += obj_bonus_.get_skill(skill_id);
	}
	if(GetMorphSkill(skill_id) > 0) {
		skill += get_skill_bonus();
	}
	
	return skill;
}

/*
 * Уровень умения без учета каких-либо бонусов.
 */
int CharData::GetTrainedSkill(const ESkill skill_num) const {
	if (privilege::CheckSkills(this)) {
		auto it = skills.find(skill_num);
		if (it != skills.end()) {
			return std::clamp(it->second.skillLevel, 0, MUD::Skill(skill_num).cap);
		}
	}
	return 0;
}
/*
 * Умение с учетом формы превращения и т.п.
 */
int CharData::GetMorphSkill(const ESkill skill_id) const {
	if (ROOM_FLAGGED(this->in_room, ERoomFlag::kDominationArena)) {
		if (MUD::Class(chclass_).skills[skill_id].IsAvailable()) {
			return 100;
		}
	}
	if (privilege::CheckSkills(this)) {
		return std::clamp(current_morph_->get_trained_skill(skill_id), 0, MUD::Skill(skill_id).cap);
	}
	return 0;
}

// * Нулевой скилл мы не сетим, а при обнулении уже имеющегося удалем эту запись.
void CharData::set_skill(const ESkill skill_id, int percent) {
	if (MUD::Skills().IsInvalid(skill_id)) {
		// Только лишний спам в логе.
		//log("SYSERROR: некорректный номер скилла %d в set_skill.", to_underlying(skill_id));
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

void CharData::SetSkillAfterRemort(short remort) {
	for (auto & it : skills) {
		int maxSkillLevel = CalcSkillHardCap(this, it.first);

		if (GetMorphSkill(it.first) > maxSkillLevel) {
			set_skill(it.first, maxSkillLevel);
		};
	}
}

void CharData::set_morphed_skill(const ESkill skill_num, int percent) {
	current_morph_->set_skill(skill_num, percent);
};

void CharData::clear_skills() {
	skills.clear();
}

int CharData::get_skills_count() const {
	return static_cast<int>(skills.size());
}

void CharacterSkillDataType::decreaseCooldown(unsigned value) {
	if (cooldown > value) {
		cooldown -= value;
	} else {
		cooldown = 0u;
	};
};

void CharData::setSkillCooldown(ESkill skillID, unsigned cooldown) {
	auto skillData = skills.find(skillID);
	if (skillData != skills.end()) {
		skillData->second.cooldown = cooldown;
	}
};

unsigned CharData::getSkillCooldown(ESkill skillID) {
	auto skillData = skills.find(skillID);
	if (skillData != skills.end()) {
		return skillData->second.cooldown;
	}
	return 0;
};

int CharData::getSkillCooldownInPulses(ESkill skillID) {
	//return static_cast<int>(std::ceil(skillData->second.cooldown/(0.0 + kBattleRound)));
	return static_cast<int>(std::ceil(getSkillCooldown(skillID) / (0.0 + kBattleRound)));
};

/* Понадобится - тогда и раскомментим...
void CharacterData::decreaseSkillCooldown(ESkill skillID, unsigned value) {
	auto skillData = skills.find(skillID);
	if (skillData != skills.end()) {
		skillData->second.decreaseCooldown(value);
	}
};
*/
void CharData::decreaseSkillsCooldowns(unsigned value) {
	for (auto &skillData : skills) {
		skillData.second.decreaseCooldown(value);
	}
};

bool CharData::haveSkillCooldown(ESkill skillID) {
	auto skillData = skills.find(skillID);
	if (skillData != skills.end()) {
		return (skillData->second.cooldown > 0);
	}
	return false;
};

bool CharData::HasCooldown(ESkill skillID) {
	if (skills[ESkill::kGlobalCooldown].cooldown > 0) {
		return true;
	}
	return haveSkillCooldown(skillID);
};

int CharData::get_obj_slot(int slot_num) {
	if (slot_num >= 0 && slot_num < MAX_ADD_SLOTS) {
		return add_abils.obj_slot[slot_num];
	}
	return 0;
}

void CharData::add_obj_slot(int slot_num, int count) {
	if (slot_num >= 0 && slot_num < MAX_ADD_SLOTS) {
		add_abils.obj_slot[slot_num] += count;
	}
}

void CharData::set_touching(CharData *vict) {
	touching_ = vict;
}

CharData *CharData::get_touching() const {
	return touching_;
}

void CharData::set_protecting(CharData *vict) {
	protecting_ = vict;
	vict->who_protecting_ = this;
}

void CharData::remove_protecting() {
	protecting_ = nullptr;
}

CharData *CharData::get_protecting() const {
	return protecting_;
}

CharData *CharData::who_protecting() const {
	return who_protecting_;
}

void CharData::SetEnemy(CharData *enemy) {
	enemy_ = enemy;
}

CharData *CharData::GetEnemy() const {
	return enemy_;
}

void CharData::SetExtraAttack(EExtraAttack Attack, CharData *vict) {
	extra_attack_.used_attack = Attack;
	extra_attack_.victim = vict;
}

EExtraAttack CharData::get_extra_attack_mode() const {
	return extra_attack_.used_attack;
}

CharData *CharData::GetExtraVictim() const {
	return extra_attack_.victim;
}

void CharData::SetCast(ESpell spell_id, ESpell spell_subst, CharData *tch, ObjData *tobj, RoomData *troom) {
	cast_attack_.spell_id = spell_id;
	cast_attack_.spell_subst = spell_subst;
	cast_attack_.tch = tch;
	cast_attack_.tobj = tobj;
	cast_attack_.troom = troom;
}

ESpell CharData::GetCastSpell() const {
	return cast_attack_.spell_id;
}

ESpell CharData::GetCastSubst() const {
	return cast_attack_.spell_subst;
}

CharData *CharData::GetCastChar() const {
	return cast_attack_.tch;
}

ObjData *CharData::GetCastObj() const {
	return cast_attack_.tobj;
}

bool IS_CHARMICE(const CharData *ch) {
	return ch->IsNpc()
		&& (AFF_FLAGGED(ch, EAffect::kHelper)
			|| AFF_FLAGGED(ch, EAffect::kCharmed));
}

bool MORT_CAN_SEE(const CharData *sub, const CharData *obj) {
	return HERE(obj)
		&& INVIS_OK(sub, obj)
		&& (!is_dark((obj)->in_room)
			|| AFF_FLAGGED((sub), EAffect::kInfravision));
}

bool MAY_SEE(const CharData *ch, const CharData *sub, const CharData *obj) {
	return !(GET_INVIS_LEV(ch) > 30)
		&& !AFF_FLAGGED(sub, EAffect::kBlind)
		&& (!is_dark(sub->in_room)
			|| AFF_FLAGGED(sub, EAffect::kInfravision))
		&& (!AFF_FLAGGED(obj, EAffect::kInvisible)
			|| AFF_FLAGGED(sub, EAffect::kDetectInvisible));
}

bool IS_HORSE(const CharData *ch) {
	return ch->IsNpc()
		&& ch->has_master()
		&& AFF_FLAGGED(ch, EAffect::kHorse);
}

bool IS_MORTIFIER(const CharData *ch) {
	return ch->IsNpc()
		&& ch->has_master()
		&& MOB_FLAGGED(ch, EMobFlag::kCorpse);
}

bool MAY_ATTACK(const CharData *sub) {
	return (!AFF_FLAGGED((sub), EAffect::kCharmed)
		&& !IS_HORSE((sub))
		&& !AFF_FLAGGED((sub), EAffect::kStopFight)
		&& !AFF_FLAGGED((sub), EAffect::kMagicStopFight)
		&& !AFF_FLAGGED((sub), EAffect::kHold)
		&& !AFF_FLAGGED((sub), EAffect::kSleep)
		&& !MOB_FLAGGED((sub), EMobFlag::kNoFight)
		&& sub->get_wait() <= 0
		&& !sub->GetEnemy()
		&& GET_POS(sub) >= EPosition::kRest);
}

bool AWAKE(const CharData *ch) {
	return GET_POS(ch) > EPosition::kSleep
		&& !AFF_FLAGGED(ch, EAffect::kSleep);
}

bool CLEAR_MIND(const CharData *ch) {
	return (!GET_AF_BATTLE(ch, kEafOverwhelm) && !GET_AF_BATTLE(ch, kEafHammer));
}

//Вы уверены,что функцияам расчете опыта самое место в классе персонажа?
bool OK_GAIN_EXP(const CharData *ch, const CharData *victim) {
	return !NAME_BAD(ch)
		&& (NAME_FINE(ch)
			|| !(GetRealLevel(ch) == kNameLevel))
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)
		&& victim->IsNpc()
		&& (GET_EXP(victim) > 0)
		&& (!victim->IsNpc()
			|| !ch->IsNpc()
			|| AFF_FLAGGED(ch, EAffect::kCharmed))
		&& !IS_HORSE(victim)
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena);
}

bool IS_MALE(const CharData *ch) {
	return GET_SEX(ch) == EGender::kMale;
}

bool IS_FEMALE(const CharData *ch) {
	return GET_SEX(ch) == EGender::kFemale;
}

bool IS_NOSEXY(const CharData *ch) {
	return GET_SEX(ch) == EGender::kNeutral;
}

bool IS_POLY(const CharData *ch) {
	return GET_SEX(ch) == EGender::kPoly;
}

bool IMM_CAN_SEE(const CharData *sub, const CharData *obj) {
	return MORT_CAN_SEE(sub, obj)
		|| (!sub->IsNpc()
			&& PRF_FLAGGED(sub, EPrf::kHolylight));
}

bool CAN_SEE(const CharData *sub, const CharData *obj) {
	return SELF(sub, obj)
		|| ((GetRealLevel(sub) >= (obj->IsNpc() ? 0 : GET_INVIS_LEV(obj)))
			&& IMM_CAN_SEE(sub, obj));
}

// * Внутри цикла чар нигде не пуржится и сам список соответственно не меняется.
void change_fighting(CharData *ch, int need_stop) {
//	utils::CExecutionTimer time;
//	log("change_fighting start %f vnum %d", time.delta().count(), GET_MOB_VNUM(ch));
	//Loop for all entities is necessary for unprotecting
//	for (const auto &k : character_list) {
	for (const auto &k : world[ch->in_room]->people) {

		if (k->get_protecting() == ch) {
			k->remove_protecting();
			CLR_AF_BATTLE(k, kEafProtect);
		}
//		log("change_fighting protecting %f", time.delta().count());
		if (k->get_touching() == ch) {
			k->set_touching(0);
			CLR_AF_BATTLE(k, kEafProtect);
		}
//		log("change_fighting touching %f", time.delta().count());
		if (k->GetExtraVictim() == ch) {
			k->SetExtraAttack(kExtraAttackUnused, 0);
		}

		if (k->GetCastChar() == ch) {
			k->SetCast(ESpell::kUndefined, ESpell::kUndefined, 0, 0, 0);
		}
//		log("change_fighting set cast %f", time.delta().count());

		if (k->GetEnemy() == ch && IN_ROOM(k) != kNowhere) {
//			log("change_fighting Change victim %f", time.delta().count());
			bool found = false;
			for (const auto j : world[ch->in_room]->people) {
				if (j->GetEnemy() == k) {
					act("Вы переключили внимание на $N3.", false, k, 0, j, kToChar);
					act("$n переключил$u на вас!", false, k, 0, j, kToVict);
					k->SetEnemy(j);
					found = true;
//					log("change_fighting Change victim %f", time.delta().count());
					break;
				}
			}

			if (!found && need_stop) {
//				log("change_fighting stop fighting %f", time.delta().count());
				stop_fighting(k, false);
			}
		}
	}
//	log("change_fighting stop %f", time.delta().count());
}

int CharData::get_serial_num() {
	return serial_num_;
}

void CharData::set_serial_num(int num) {
	serial_num_ = num;
}

bool CharData::purged() const {
	return purged_;
}

const std::string &CharData::get_name_str() const {
	if (this->IsNpc()) {
		return short_descr_;
	}
	return name_;
}

const std::string &CharData::get_name() const {
	return this->IsNpc() ? get_npc_name() : GetCharAliases();
}

void CharData::set_name(const char *name) {
	if (this->IsNpc()) {
		set_npc_name(name);
	} else {
		SetCharAliases(name);
	}
}

void CharData::SetCharAliases(const char *name) {
	if (name) {
		name_ = name;
	} else {
		name_.clear();
	}
}

void CharData::set_npc_name(const char *name) {
	if (name) {
		short_descr_ = name;
	} else {
		short_descr_.clear();
	}
}

const char *CharData::get_pad(unsigned pad) const {
	if (pad < player_data.PNames.size()) {
		return player_data.PNames[pad].c_str();
	}
	return 0;
}

void CharData::set_pad(unsigned pad, const char *s) {
	if (pad >= player_data.PNames.size()) {
		return;
	}

	this->player_data.PNames[pad] = std::string(s);
}

const char *CharData::get_long_descr() const {
	return player_data.long_descr.c_str();
}

void CharData::set_long_descr(const char *s) {
	player_data.long_descr = std::string(s);
}

const char *CharData::get_description() const {
	return player_data.description.c_str();
}

void CharData::set_description(const char *s) {
	player_data.description = std::string(s);
}

ECharClass CharData::GetClass() const {
	return chclass_;
}

void CharData::set_class(ECharClass chclass) {
	// Range includes player classes and NPC classes (and does not consider gaps between them).
	// Почему классы не пронумеровать подряд - загадка...
	// && chclass != ECharClass::kNpcBase && chclass != ECharClass::kMob
	if (chclass < ECharClass::kFirst || chclass > ECharClass::kNpcLast) {
		log("WARNING: chclass=%d (%s:%d %s)", to_underlying(chclass), __FILE__, __LINE__, __func__);
	}
	chclass_ = chclass;
}

int CharData::GetLevel() const {
	return level_;
}

int CharData::get_level_add() const {
	return level_add_;
}

void CharData::set_level(int level) {
	if (this->IsNpc()) {
		level_ = std::clamp(level, kMinCharLevel, kMaxMobLevel);
	} else {
		level_ = std::clamp(level, kMinCharLevel, kLvlImplementator);
	}
}

void CharData::set_level_add(int level) {
	// level_ + level_add_ не должно быть больше максимального уровня
	// все проверки находятся в GetRealLevel()
	level_add_ = level;
}

long CharData::get_idnum() const {
	return idnum_;
}

void CharData::set_idnum(long idnum) {
	idnum_ = idnum;
}

int CharData::get_uid() const {
	return uid_;
}

void CharData::set_uid(int uid) {
	uid_ = uid;
}

long CharData::get_exp() const {
	return exp_;
}

void CharData::set_exp(long exp) {
	if (exp < 0) {
		log("WARNING: exp=%ld name=[%s] (%s:%d %s)", exp, get_name().c_str(), __FILE__, __LINE__, __func__);
	}
	exp_ = std::max(0L, exp);
	msdp_report(msdp::constants::EXPERIENCE);
}

int CharData::get_remort() const {
	return remorts_;
}

int CharData::get_remort_add() const {
	return remorts_add_;
}

void CharData::set_remort(int num) {
	remorts_ = std::max(0, num);
}

void CharData::set_remort_add(short num) {
	remorts_add_ = num;
}

time_t CharData::get_last_logon() const {
	return last_logon_;
}

void CharData::set_last_logon(time_t num) {
	last_logon_ = num;
}

time_t CharData::get_last_exchange() const {
	return last_exchange_;
}

void CharData::set_last_exchange(time_t num) {
	last_exchange_ = num;
}

EGender CharData::get_sex() const {
	return player_data.sex;
}

void CharData::set_sex(const EGender sex) {
/*	if (to_underlying(sex) >= 0
		&& to_underlying(sex) < EGender::kLast) {*/
	if (sex < EGender::kLast) {
		player_data.sex = sex;
	}
}

ubyte CharData::get_weight() const {
	return player_data.weight;
}

void CharData::set_weight(const ubyte v) {
	player_data.weight = v;
}

ubyte CharData::get_height() const {
	return player_data.height;
}

void CharData::set_height(const ubyte v) {
	player_data.height = v;
}

ubyte CharData::get_religion() const {
	return player_data.Religion;
}

void CharData::set_religion(const ubyte v) {
	if (v < 2) {
		player_data.Religion = v;
	}
}

ubyte CharData::get_kin() const {
	return player_data.Kin;
}

void CharData::set_kin(const ubyte v) {
	if (v < kNumKins) {
		player_data.Kin = v;
	}
}

ubyte CharData::get_race() const {
	return player_data.Race;
}

void CharData::set_race(const ubyte v) {
	player_data.Race = v;
}

int CharData::get_hit() const {
	return points.hit;
}

void CharData::set_hit(const int v) {
	if (v >= -10)
		points.hit = v;
}

int CharData::get_max_hit() const {
	return points.max_hit;
}

void CharData::set_max_hit(const int v) {
	if (v >= 0) {
		points.max_hit = v;
	}
	msdp_report(msdp::constants::MAX_HIT);
}

sh_int CharData::get_move() const {
	return points.move;
}

void CharData::set_move(const sh_int v) {
	if (v >= 0)
		points.move = v;
}

sh_int CharData::get_max_move() const {
	return points.max_move;
}

void CharData::set_max_move(const sh_int v) {
	if (v >= 0) {
		points.max_move = v;
	}
	msdp_report(msdp::constants::MAX_MOVE);
}

long CharData::get_ruble() {
	return ruble;
}

void CharData::set_ruble(int ruble) {
	this->ruble = ruble;
}

long CharData::get_gold() const {
	return gold_;
}

long CharData::get_bank() const {
	return bank_gold_;
}

long CharData::get_total_gold() const {
	return get_gold() + get_bank();
}

/**
 * Добавление денег на руках, плюсуются только положительные числа.
 * \param need_log здесь и далее - логировать или нет изменения счета (=true)
 * \param clan_tax - проверять и снимать клан-налог или нет (=false)
 */
void CharData::add_gold(long num, bool need_log, bool clan_tax) {
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
void CharData::add_bank(long num, bool need_log) {
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
void CharData::set_gold(long num, bool need_log) {
	if (get_gold() == num) {
		// чтобы с логированием не заморачиваться
		return;
	}
	num = std::clamp(num, 0L, kMaxMoneyKept);

	if (need_log && !this->IsNpc()) {
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
void CharData::set_bank(long num, bool need_log) {
	if (get_bank() == num) {
		// чтобы с логированием не заморачиваться
		return;
	}
	num = std::clamp(num, 0L, kMaxMoneyKept);

	if (need_log && !this->IsNpc()) {
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
long CharData::remove_gold(long num, bool need_log) {
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
long CharData::remove_bank(long num, bool need_log) {
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
long CharData::remove_both_gold(long num, bool need_log) {
	long rest = remove_bank(num, need_log);
	return remove_gold(rest, need_log);
}

// * Удача (мораль) для расчетов в скилах и вывода чару по счет все.
int CharData::calc_morale() const {
	return GetRealCha(this) / 2 + GET_MORALE(this);
//	return cha_app[GetRealCha(this)].morale + GET_MORALE(this);
}
///////////////////////////////////////////////////////////////////////////////
int CharData::get_str() const {
	check_purged(this, "get_str");
	return current_morph_->GetStr();
}

int CharData::GetInbornStr() const {
	return str_;
}

void CharData::set_str(int param) {
	str_ = std::max(1, param);
}

void CharData::inc_str(int param) {
	str_ = std::max(1, str_ + param);
}

int CharData::get_str_add() const {
	return str_add_;
}

void CharData::set_str_add(int param) {
	str_add_ = param;
}
///////////////////////////////////////////////////////////////////////////////
int CharData::get_dex() const {
	check_purged(this, "get_dex");
	return current_morph_->GetDex();
}

int CharData::GetInbornDex() const {
	return dex_;
}

void CharData::set_dex(int param) {
	dex_ = std::max(1, param);
}

void CharData::inc_dex(int param) {
	dex_ = std::max(1, dex_ + param);
}

int CharData::get_dex_add() const {
	return dex_add_;
}

void CharData::set_dex_add(int param) {
	dex_add_ = param;
}
///////////////////////////////////////////////////////////////////////////////
int CharData::get_con() const {
	check_purged(this, "get_con");
	return current_morph_->GetCon();
}

int CharData::GetInbornCon() const {
	return con_;
}

void CharData::set_con(int param) {
	con_ = std::max(1, param);
}
void CharData::inc_con(int param) {
	con_ = std::max(1, con_ + param);
}

int CharData::get_con_add() const {
	return con_add_;
}

void CharData::set_con_add(int param) {
	con_add_ = param;
}
//////////////////////////////////////

int CharData::get_int() const {
	check_purged(this, "get_int");
	return current_morph_->GetIntel();
}

int CharData::GetInbornInt() const {
	return int_;
}

void CharData::set_int(int param) {
	int_ = std::max(1, param);
}

void CharData::inc_int(int param) {
	int_ = std::max(1, int_ + param);
}

int CharData::get_int_add() const {
	return int_add_;
}

void CharData::set_int_add(int param) {
	int_add_ = param;
}
////////////////////////////////////////
int CharData::get_wis() const {
	check_purged(this, "get_wis");
	return current_morph_->GetWis();
}

int CharData::GetInbornWis() const {
	return wis_;
}

void CharData::set_wis(int param) {
	wis_ = std::max(1, param);
}

void CharData::set_who_mana(unsigned int param) {
	player_specials->saved.who_mana = param;
}

void CharData::set_who_last(time_t param) {
	char_specials.who_last = param;
}

unsigned int CharData::get_who_mana() {
	return player_specials->saved.who_mana;
}

time_t CharData::get_who_last() {
	return char_specials.who_last;
}

void CharData::inc_wis(int param) {
	wis_ = std::max(1, wis_ + param);
}

int CharData::get_wis_add() const {
	return wis_add_;
}

void CharData::set_wis_add(int param) {
	wis_add_ = param;
}
///////////////////////////////////////////////////////////////////////////////
int CharData::get_cha() const {
	check_purged(this, "get_cha");
	return current_morph_->GetCha();
}

int CharData::GetInbornCha() const {
	return cha_;
}

void CharData::set_cha(int param) {
	cha_ = std::max(1, param);
}
void CharData::inc_cha(int param) {
	cha_ = std::max(1, cha_ + param);
}

int CharData::get_cha_add() const {
	return cha_add_;
}
void CharData::set_cha_add(int param) {
	cha_add_ = param;
}
int CharData::get_skill_bonus() const {
	return skill_bonus_;
}
void CharData::set_skill_bonus(int param) {
	skill_bonus_ = param;
}

int CharData::GetAddSkill(ESkill skill_id) const {
	auto it = skills_add_.find(skill_id);
	if (it != skills_add_.end()) {
		return it->second;
	}
	return 0;
}

void CharData::SetAddSkill(ESkill skill_id, int value) {
	skills_add_[skill_id] += value;
}


///////////////////////////////////////////////////////////////////////////////

void CharData::clear_add_apply_affects() {
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
	skills_add_.clear();
}
///////////////////////////////////////////////////////////////////////////////
int CharData::get_zone_group() const {
	const auto rnum = get_rnum();
	if (this->IsNpc()
		&& rnum >= 0
		&& mob_index[rnum].zone >= 0) {
		const auto zone = mob_index[rnum].zone;
		return std::max(1, zone_table[zone].group);
	}

	return 1;
}

//===================================
//Polud формы и все что с ними связано
//===================================

bool CharData::know_morph(const std::string &morph_id) const {
	return std::find(morphs_.begin(), morphs_.end(), morph_id) != morphs_.end();
}

void CharData::add_morph(const std::string &morph_id) {
	morphs_.push_back(morph_id);
};

void CharData::clear_morphs() {
	morphs_.clear();
};

const CharData::morphs_list_t &CharData::get_morphs() {
	return morphs_;
};
// обрезает строку и выдергивает из нее предтитул
std::string CharData::get_title() {
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

std::string CharData::get_pretitle() {
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

std::string CharData::get_race_name() {
	return PlayerRace::GetRaceNameByNum(GET_KIN(this), GET_RACE(this), GET_SEX(this));
}

std::string CharData::get_morph_desc() const {
	return current_morph_->GetMorphDesc();
};

std::string CharData::get_morphed_name() const {
	return current_morph_->GetMorphDesc() + " - " + this->get_name();
};

std::string CharData::get_morphed_title() const {
	return current_morph_->GetMorphTitle();
};

std::string CharData::only_title_noclan() {
	std::string result = this->get_name();
	std::string title = this->get_title();
	std::string pre_title = this->get_pretitle();

	if (!pre_title.empty())
		result = pre_title + " " + result;

	if (!title.empty() && this->GetLevel() >= MIN_TITLE_LEV)
		result = result + ", " + title;

	return result;
}

std::string CharData::clan_for_title() {
	std::string result = std::string();

	bool imm = IS_IMMORTAL(this) || PRF_FLAGGED(this, EPrf::kCoderinfo);

	if (CLAN(this) && !imm)
		result = result + "(" + GET_CLAN_STATUS(this) + ")";

	return result;
}

std::string CharData::only_title() {
	std::string result = this->clan_for_title();
	if (!result.empty())
		result = this->only_title_noclan() + " " + result;
	else
		result = this->only_title_noclan();

	return result;
}

std::string CharData::noclan_title() {
	std::string race = this->get_race_name();
	std::string result = this->only_title_noclan();

	if (result == this->get_name()) {
		result = race + " " + result;
	}

	return result;
}

std::string CharData::race_or_title() {
	std::string result = this->clan_for_title();

	if (!result.empty())
		result = this->noclan_title() + " " + result;
	else
		result = this->noclan_title();

	return result;
}

size_t CharData::get_morphs_count() const {
	return morphs_.size();
};

std::string CharData::get_cover_desc() {
	return current_morph_->CoverDesc();
}

void CharData::set_morph(MorphPtr morph) {
	morph->SetChar(this);
	morph->InitSkills(this->GetSkill(ESkill::kMorph));
	morph->InitAbils();
	this->current_morph_ = morph;
};

void CharData::reset_morph() {
	int value = this->GetMorphSkill(ESkill::kMorph);
	auto msg_to_char = fmt::format(fmt::runtime(current_morph_->GetMessageToRoom()), "человеком") + "\r\n";
	SendMsgToChar(msg_to_char, this);
	auto msg_to_room = fmt::format(fmt::runtime(current_morph_->GetMessageToRoom()), "человеком");
	act(msg_to_room, true, this, 0, 0, kToRoom);
	this->current_morph_ = GetNormalMorphNew(this);
	this->set_morphed_skill(ESkill::kMorph, (std::min(kZeroRemortSkillCap + GetRealRemort(this) * 5, value)));
//	REMOVE_BIT(AFF_FLAGS(this, AFF_MORPH), AFF_MORPH);
};

bool CharData::is_morphed() const {
	return current_morph_->Name() != "Обычная" || AFF_FLAGGED(this, EAffect::kMorphing);
};

void CharData::set_normal_morph() {
	current_morph_ = GetNormalMorphNew(this);
}

bool CharData::isAffected(const EAffect flag) const {
	return current_morph_->isAffected(flag);
}

const IMorph::affects_list_t &CharData::GetMorphAffects() {
	return current_morph_->GetAffects();
}

//===================================
//-Polud
//===================================

bool CharData::get_role(unsigned num) const {
	bool result = false;
	if (num < role_.size()) {
		result = role_.test(num);
	} else {
		log("SYSERROR: num=%u (%s:%d)", num, __FILE__, __LINE__);
	}
	return result;
}

void CharData::set_role(unsigned num, bool flag) {
	if (num < role_.size()) {
		role_.set(num, flag);
	} else {
		log("SYSERROR: num=%u (%s:%d)", num, __FILE__, __LINE__);
	}
}

void CharData::msdp_report(const std::string &name) {
	if (nullptr != desc) {
		desc->msdp_report(name);
	}
}

void CharData::removeGroupFlags() {
	AFF_FLAGS(this).unset(EAffect::kGroup);
	PRF_FLAGS(this).unset(EPrf::kSkirmisher);
}

void CharData::add_follower(CharData *ch) {

	if (ch->IsNpc() && MOB_FLAGGED(ch, EMobFlag::kNoGroup))
		return;
	add_follower_silently(ch);

	if (!IS_HORSE(ch)) {
		act("Вы начали следовать за $N4.", false, ch, 0, this, kToChar);
		act("$n начал$g следовать за вами.", true, ch, 0, this, kToVict);
		act("$n начал$g следовать за $N4.", true, ch, 0, this, kToNotVict | kToArenaListen);
	}
}

CharData::followers_list_t CharData::get_followers_list() const {
	CharData::followers_list_t result;
	auto pos = followers;
	while (pos) {
		const auto follower = pos->follower;
		result.push_back(follower);
		pos = pos->next;
	}
	return result;
}

bool CharData::low_charm() const {
	for (const auto &aff : affected) {
		if (aff->type == ESpell::kCharm
			&& aff->duration <= 1) {
			return true;
		}
	}
	return false;
}

void CharData::cleanup_script() {
	script->cleanup();
}

void CharData::add_follower_silently(CharData *ch) {
	struct FollowerType *k;

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

	k->follower = ch;
	k->next = followers;
	followers = k;
}

const CharData::role_t &CharData::get_role_bits() const {
	return role_;
}

// добавляет указанного ch чара в список атакующих босса с параметром type
// или обновляет его данные в этом списке
void CharData::add_attacker(CharData *ch, unsigned type, int num) {
	if (!this->IsNpc() || ch->IsNpc() || !get_role(MOB_ROLE_BOSS)) {
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
int CharData::get_attacker(CharData *ch, unsigned type) const {
	if (!this->IsNpc() || ch->IsNpc() || !get_role(MOB_ROLE_BOSS)) {
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
std::pair<int /* uid */, int /* rounds */> CharData::get_max_damager_in_room() const {
	std::pair<int, int> damager(-1, 0);

	if (!this->IsNpc() || !get_role(MOB_ROLE_BOSS)) {
		return damager;
	}

	int max_dmg = 0;
	for (const auto i : world[this->in_room]->people) {
		if (!i->IsNpc() && i->desc) {
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
void CharData::restore_mob() {
	restore_timer_ = 0;
	attackers_.clear();

	GET_HIT(this) = GET_REAL_MAX_HIT(this);
	GET_MOVE(this) = GET_REAL_MAX_MOVE(this);
	update_pos(this);

	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		GET_SPELL_MEM(this, spell_id) = GET_SPELL_MEM(&mob_proto[GET_MOB_RNUM(this)], spell_id);
	}
	this->caster_level = (&mob_proto[GET_MOB_RNUM(this)])->caster_level;
}
//
void CharData::restore_npc() {
	if(!this->IsNpc()) return;
	
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
	this->SetCharAliases(GET_PC_NAME(proto));
	this->set_npc_name(GET_NAME(proto));
    // кубики // екстра атаки
	this->mob_specials.damnodice = proto->mob_specials.damnodice;
	this->mob_specials.damsizedice = proto->mob_specials.damsizedice;
	this->mob_specials.extra_attack = proto->mob_specials.extra_attack;
	// this->mob_specials.damnodice = 1;
	// this->mob_specials.damsizedice = 1;
	// this->mob_specials.ExtraAttack = 0;
	//флаги
	MOB_FLAGS(this) = MOB_FLAGS(proto);
	this->set_touching(nullptr);
	this->remove_protecting();
	// ресторим статы
	proto->set_normal_morph();
	this->set_str(GetRealStr(proto));
	this->set_int(GetRealInt(proto));
	this->set_wis(GetRealWis(proto));
	this->set_dex(GetRealDex(proto));
	this->set_con(GetRealCon(proto));
	this->set_cha(GetRealCha(proto));
	// ресторим мем	
	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		GET_SPELL_MEM(this, spell_id) = GET_SPELL_MEM(proto, spell_id);
	}

	for (const auto &skill : MUD::Skills()) {
		if (skill.IsValid()) {
			this->set_skill(skill.GetId(), GET_SKILL(proto, skill.GetId()));
		}
	}

	for (const auto &feat : MUD::Feats()) {
		if (!proto->HaveFeat(feat.GetId())) {
				this->UnsetFeat(feat.GetId());
		}
	}
	this->caster_level = proto->caster_level;
}

void CharData::report_loop_error(const CharData::ptr_t master) const {
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

	ss << "\nТекущий стек будет распечатан в SYSLOG.";
	debug::backtrace(runtime_config.logs(SYSLOG).handle());
	mudlog(ss.str().c_str(), LGH, kLvlImmortal, SYSLOG, false);
}

void CharData::print_leaders_chain(std::ostream &ss) const {
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

void CharData::set_master(CharData::ptr_t master) {
	if (makes_loop(master)) {
		report_loop_error(master);
		return;
	}
	m_master = master;
}

bool CharData::makes_loop(CharData::ptr_t master) const {
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
void CharData::inc_restore_timer(int num) {
	if (get_role(MOB_ROLE_BOSS) && !attackers_.empty() && !GetEnemy()) {
		restore_timer_ += num;
		if (restore_timer_ > num) {
			restore_mob();
		}
	}
}

//метод передачи отладочного сообщения:
//имморталу, тестеру или кодеру
//остальные параметры - функция printf
void CharData::send_to_TC(bool to_impl, bool to_tester, bool to_coder, const char *msg, ...) {
	bool needSend = false;
	// проверка на ситуацию "чармис стоит, хозяина уже нет с нами"
	if (IS_CHARMICE(this) && !this->has_master()) {
		sprintf(buf, "[WARNING] CharacterData::send_to_TC. Чармис без хозяина: %s", this->get_name().c_str());
		mudlog(buf, CMP, kLvlGod, SYSLOG, true);
		return;
	}
	if ((IS_CHARMICE(this) && this->get_master()->IsNpc()) //если это чармис у нпц
		|| (this->IsNpc() && !IS_CHARMICE(this))) //просто непись
		return;

	if (to_impl &&
		(IS_IMPL(this) || (IS_CHARMICE(this) && IS_IMPL(this->get_master()))))
		needSend = true;
	if (!needSend && to_coder &&
		(PRF_FLAGGED(this, EPrf::kCoderinfo) || (IS_CHARMICE(this) && (PRF_FLAGGED(this->get_master(), EPrf::kCoderinfo)))))
		needSend = true;
	if (!needSend && to_tester &&
		(PRF_FLAGGED(this, EPrf::kTester) || (IS_CHARMICE(this) && (PRF_FLAGGED(this->get_master(), EPrf::kTester)))))
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
		mudlog(buf, BRF, kLvlGod, SYSLOG, true);
		return;
	}
	// проверка на нпц была ранее. Шлем хозяину чармиса или самому тестеру
	SendMsgToChar(tmpbuf, IS_CHARMICE(this) ? this->get_master() : this);
}

bool CharData::have_mind() const {
	if (!AFF_FLAGGED(this, EAffect::kCharmed) && !IS_HORSE(this))
		return true;
	return false;
}

bool CharData::has_horse(bool same_room) const {
	struct FollowerType *f;

	if (this->IsNpc()) {
		return false;
	}

	for (f = this->followers; f; f = f->next) {
		if (f->follower->IsNpc() && AFF_FLAGGED(f->follower, EAffect::kHorse)
			&& (!same_room || this->in_room == IN_ROOM(f->follower))) {
			return true;
		}
	}
	return false;
}
// персонаж на лошади?
bool CharData::IsOnHorse() const {
	return AFF_FLAGGED(this, EAffect::kHorse) && this->has_horse(true);
}

bool CharData::IsHorsePrevents() {
	if (this->IsOnHorse()) {
		act("Вам мешает $N.", false, this, 0, this->get_horse(), kToChar);
		return true;
	}
	return false;
}

bool CharData::DropFromHorse() {
	CharData *plr;

	// вызвали для лошади
	if (IS_HORSE(this) && this->get_master()->IsOnHorse()) {
		plr = this->get_master();
		act("$N сбросил$G вас со своей спины.", false, plr, 0, this, kToChar);
	} else	if (this->IsOnHorse()) {// вызвали для седока
		plr = this;
		act("Вы упали с $N1.", false, plr, 0, this->get_horse(), kToChar);
	} else //не лошадь и не всадник
		return false;
	sprintf(buf, "%s свалил%s со своего скакуна.", GET_PAD(plr, 0), GET_CH_SUF_2(plr));
	act(buf, false, plr, 0, 0, kToRoom | kToArenaListen);
	AFF_FLAGS(plr).unset(EAffect::kHorse);
	SetWaitState(plr, 3 * kBattleRound);
	if (GET_POS(plr) > EPosition::kSit) {
		GET_POS(plr) = EPosition::kSit;
	}
	return true;
}

void CharData::dismount() {
	if (!this->IsOnHorse() || this->get_horse() == nullptr)
		return;
	if (!this->IsNpc() && this->has_horse(true)) {
		AFF_FLAGS(this).unset(EAffect::kHorse);
	}
	act("Вы слезли со спины $N1.", false, this, 0, this->get_horse(), kToChar);
	act("$n соскочил$g с $N1.", false, this, 0, this->get_horse(), kToRoom | kToArenaListen);
}

CharData *CharData::get_horse() {
	struct FollowerType *f;

	if (this->IsNpc())
		return nullptr;

	for (f = this->followers; f; f = f->next) {
		if (f->follower->IsNpc() && AFF_FLAGGED(f->follower, EAffect::kHorse)) {
			return (f->follower);
		}
	}
	return nullptr;
};

obj_sets::activ_sum &CharData::obj_bonus() {
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

int ClampBaseStat(const CharData *ch, const EBaseStat stat_id, const int stat_value) {
	return ch->IsNpc()
		   ? std::clamp(stat_value, kLeastBaseStat, kMobBaseStatCap)
		   : std::clamp(stat_value, kLeastBaseStat, MUD::Class(ch->GetClass()).GetBaseStatCap(stat_id));
}

int GetRealBaseStat(const CharData *ch, EBaseStat stat_id) {
	static const std::unordered_map<EBaseStat, int (*)(const CharData *ch)> stat_getters =
		{
			{EBaseStat::kStr, &GetRealStr},
			{EBaseStat::kDex, &GetRealDex},
			{EBaseStat::kCon, &GetRealCon},
			{EBaseStat::kInt, &GetRealInt},
			{EBaseStat::kWis, &GetRealWis},
			{EBaseStat::kCha, &GetRealCha}
		};

	try {
		return stat_getters.at(stat_id)(ch);
	} catch (std::out_of_range &) {
		return 1;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
