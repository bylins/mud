// $RCSfile: char.cpp,v $     $Date: 2012/01/27 04:04:43 $     $Revision: 1.85 $
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "engine/core/handler.h"
#include "administration/privilege.h"
#include "char_player.h"
#include "gameplay/mechanics/player_races.h"
#include "utils/cache.h"
#include "gameplay/fight/fight.h"
#include "gameplay/clans/house.h"
#include "engine/network/msdp/msdp_constants.h"
#include "utils/backtrace.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/liquid.h"
#include "char_data.h"
#include "gameplay/statistics/money_drop.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/illumination.h"
#include "engine/ui/alias.h"

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
	role_(MOB_ROLE_COUNT),
	in_room(Rooms::UNDEFINED_ROOM_VNUM),
	m_wait(~0u),
	m_master(nullptr),
	proto_script(new ObjData::triggers_list_t()),
	script(new Script()),
	followers(nullptr) {
	this->zero_init();
	caching::character_cache.Add(this);
	skills[ESkill::kGlobalCooldown].skillLevel = 0; //добавим позицию в map
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
	if (this->get_protecting()) {
		remove_protecting();
	}
	set_touching(nullptr);
	battle_affects = clear_flags;
	poisoner = 0;
	SetEnemy(nullptr);
	char_specials.position = EPosition::kStand;
	mob_specials.default_pos = EPosition::kStand;
	char_specials.carry_weight = 0;
	char_specials.carry_items = 0;

	if (this->get_hit() <= 0) {
		this->set_hit(1);
	}
	if (this->get_move() <= 0) {
		this->set_move(1);
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

CharData::char_affects_list_t::iterator CharData::AffectRemove(const char_affects_list_t::iterator &affect_i) {
	if (affected.empty()) {
		log("SYSERR: affect_remove(%s) when no affects...", GET_NAME(this));
		return affected.end();
	}
	const auto af = *affect_i;
	if (af->type == ESpell::kAbstinent) {
		if (player_specials) {
			GET_DRUNK_STATE(this) = GET_COND(this, DRUNK) = std::min(GET_COND(this, DRUNK), kDrunked - 1);
		} else {
			log("SYSERR: player_specials is not set.");
		}
	}
	return affected.erase(affect_i);
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
	if (to_remove > 0) {
		std::shuffle(removable_affects.begin(), removable_affects.end(), std::mt19937(std::random_device()()));
		for (auto counter = 0u; counter < to_remove; ++counter) {
			const auto affect_i = removable_affects[counter];
			AffectRemove(affect_i);    //count тут не сработает, удаляются все аффекты а не первый
		}
		affect_total(this);
	}
	return to_remove;
}

/**
* Обнуление всех полей Character (аналог конструктора),
* вынесено в отдельную функцию, чтобы дергать из purge().
*/
void CharData::zero_init() {
	set_sex(EGender::kMale);
	is_npc_ = false;
	set_race(0);
	protecting_ = nullptr;
	touching_ = nullptr;
	enemy_ = nullptr;
	purged_ = false;
	// на плеер-таблицу
	chclass_ = ECharClass::kUndefined;
	level_ = 0;
	level_add_ = 0,
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
	m_wait = 0u;
	punctual_wait = 0;
	last_comm.clear();
	player_specials = nullptr;
	timed = nullptr;
	timed_feat = nullptr;
	carrying = nullptr;
	desc = nullptr;
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
	agrobd = false;
	extra_attack_ = {};
	cast_attack_ = {};
	add_abils = {};
	real_abils = {};
	char_specials = {};
	mob_specials = {};
	mem_queue.Clear();
	battle_affects.clear();
	Temporary.clear();
	char_specials.position = EPosition::kStand;
	mob_specials.default_pos = EPosition::kStand;
	memset(&points, 0, sizeof(char_point_data));
	souls = 0;

//	memset(&extra_attack_, 0, sizeof(extra_attack_type));
//	memset(&cast_attack_, 0, sizeof(CastAttack));
	//memset(&player_data, 0, sizeof(char_player_data));
	//player_data char_player_data();
//	memset(&add_abils, 0, sizeof(char_played_ability_data));
//	memset(&real_abils, 0, sizeof(char_ability_data));
//	memset(&char_specials, 0, sizeof(char_special_data));
//	memset(&mob_specials, 0, sizeof(mob_special_data));
//	memset(&Temporary, 0, sizeof(FlagData));
//	memset(&battle_affects, 0, sizeof(FlagData));

	for (auto & i : equipment) {
		i = nullptr;
	}
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

//	if (!get_name().empty()) {
//		log("[FREE CHAR] (%s)", GET_NAME(this));
//	}
	int i, id = -1;
	struct alias_data *a;

	if (!this->IsNpc() && !get_name().empty()) {
		id = GetPlayerTablePosByName(GET_NAME(this));
		if (id >= 0) {
			player_table[id].level = GetRealLevel(this);
			player_table[id].remorts = GetRealRemort(this);
			player_table[id].activity = number(0, kObjectSaveActivity - 1);
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
	this->affected.clear();
	while (this->timed) {
		ExpireTimedSkill(this, this->timed);
	}

	celebrates::RemoveFromMobLists(this->get_uid());

	const bool keep_player_specials = player_specials == player_special_data::s_for_mobiles ? true : false;
	if (this->player_specials && !keep_player_specials) {
		while ((a = GET_ALIASES(this)) != nullptr) {
			GET_ALIASES(this) = (GET_ALIASES(this))->next;
			FreeAlias(a);
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
		this->ClearRunestones();
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

int CharData::GetSkillBonus(const ESkill skill_id) const {
	if (ROOM_FLAGGED(this->in_room, ERoomFlag::kDominationArena)) {
		if (MUD::Class(chclass_).skills[skill_id].IsAvailable()) {
			return 100;
		}
	}
	if (privilege::CheckSkills(this)) {
		return std::clamp(GetTrainedSkill(skill_id), 0, MUD::Skill(skill_id).cap);
	}
	return 0;

}
/*
 * Умение с учетом всех бонусов и штрафов (экипировка, таланты, яд).
 */
int CharData::GetSkill(const ESkill skill_id) const {
	int skill = GetSkillBonus(skill_id);

	if (skill > 0) {
		skill += GetAddSkill(skill_id) + GetEquippedSkill(skill_id);
	}

	if (AFF_FLAGGED(this, EAffect::kSkillReduce)) {
		skill -= skill * GET_SKILL_REDUCE(this) / 100;
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
	if (GetSkillBonus(skill_id) > 0) {
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

void CharData::SetSkillAfterRemort() {
	for (auto & it : skills) {
		int maxSkillLevel = CalcSkillHardCap(this, it.first);

		if (GetSkillBonus(it.first) > maxSkillLevel) {
			set_skill(it.first, maxSkillLevel);
		};
	}
}

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
		chardata_cooldown_list.insert(this);
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

bool CharData::HaveDecreaseCooldowns() {
	bool has_cooldown = false;
	for (auto &skillData : skills) {
		skillData.second.decreaseCooldown(1);
		if (skillData.second.cooldown > 0) {
			has_cooldown = true;
		}
	}
	return has_cooldown;
}

void CharData::ZeroCooldowns() {
	for (auto &skillData : skills) {
		skillData.second.cooldown = 0u;
	}
}

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
	if (protecting_) {
		remove_protecting();
	}
	protecting_ = vict;
	vict->who_protecting.push_back(this);
}

void CharData::remove_protecting() {

	if (protecting_) {
		auto predicate = [this](auto p) { return (this  ==  p); };
		auto it = std::find_if(protecting_->who_protecting.begin(), protecting_->who_protecting.end(), predicate);
		if (it != protecting_->who_protecting.end()) {
			protecting_->who_protecting.erase(it);
			SendMsgToChar(this, "Вы перестали прикрывать %s.\r\n", 
				GET_PAD(protecting_, 3));
			SendMsgToChar(get_protecting(), "%s перестал%s прикрывать вас.\r\n", GET_NAME(this), GET_CH_SUF_1(this));
		}
	}
	protecting_ = nullptr;
}

CharData *CharData::get_protecting() const {
	return protecting_;
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

// \todo Да-да, функциями типа "кто кого видит" - самое мместо в модуле персонажа. Вычистить это все отсюда.
bool IS_CHARMICE(const CharData *ch) {
	return ch->IsNpc()
		&& (AFF_FLAGGED(ch, EAffect::kHelper)
			|| AFF_FLAGGED(ch, EAffect::kCharmed));
}

bool CharData::IsLeader() {
	if (this->IsNpc()) {
		return false;
	}
	if (this->get_master() != this) {
		return false;
	}
	for (FollowerType *f = this->followers; f; f = f->next) {
		if (!f->follower->IsNpc()) {
			return true;
		}
	}
	return false;
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
		&& ch->IsFlagged(EMobFlag::kCorpse);
}

bool MAY_ATTACK(const CharData *sub) {
	return (!AFF_FLAGGED((sub), EAffect::kCharmed)
		&& !IS_HORSE((sub))
		&& !AFF_FLAGGED((sub), EAffect::kStopFight)
		&& !AFF_FLAGGED((sub), EAffect::kMagicStopFight)
		&& !AFF_FLAGGED((sub), EAffect::kHold)
		&& !AFF_FLAGGED((sub), EAffect::kSleep)
		&& !(sub)->IsFlagged(EMobFlag::kNoFight)
		&& sub->get_wait() <= 0
		&& !sub->GetEnemy()
		&& sub->GetPosition() >= EPosition::kRest);
}

bool AWAKE(const CharData *ch) {
	return ch->GetPosition() > EPosition::kSleep
		&& !AFF_FLAGGED(ch, EAffect::kSleep);
}

bool CLEAR_MIND(const CharData *ch) {
	return (!ch->battle_affects.get(kEafOverwhelm) && !ch->battle_affects.get(kEafHammer));
}

//Вы уверены,что функцияам расчете опыта самое место в классе персонажа?
bool OK_GAIN_EXP(const CharData *ch, const CharData *victim) {
	return !NAME_BAD(ch)
		&& (NAME_FINE(ch)
			|| !(GetRealLevel(ch) == kNameLevel))
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)
		&& victim->IsNpc()
		&& (victim->get_exp() > 0)
		&& (!victim->IsNpc()
			|| !ch->IsNpc()
			|| AFF_FLAGGED(ch, EAffect::kCharmed))
		&& !IS_HORSE(victim)
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena);
}

bool IS_MALE(const CharData *ch) {
	return ch->get_sex() == EGender::kMale;
}

bool IS_FEMALE(const CharData *ch) {
	return ch->get_sex() == EGender::kFemale;
}

bool IS_NOSEXY(const CharData *ch) {
	return ch->get_sex() == EGender::kNeutral;
}

bool IS_POLY(const CharData *ch) {
	return ch->get_sex() == EGender::kPoly;
}

bool IMM_CAN_SEE(const CharData *sub, const CharData *obj) {
	return MORT_CAN_SEE(sub, obj)
		|| (!sub->IsNpc()
			&& sub->IsFlagged(EPrf::kHolylight));
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
		}
//		log("change_fighting protecting %f", time.delta().count());
		if (k->get_touching() == ch) {
			k->set_touching(0);
		}
//		log("change_fighting touching %f", time.delta().count());
		if (k->GetExtraVictim() == ch) {
			k->SetExtraAttack(kExtraAttackUnused, 0);
		}

		if (k->GetCastChar() == ch) {
			k->SetCast(ESpell::kUndefined, ESpell::kUndefined, 0, 0, 0);
		}
//		log("change_fighting set cast %f", time.delta().count());

		if (k->GetEnemy() == ch && k->in_room != kNowhere) {
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

long CharData::get_uid() const {
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

int CharData::get_hit_add() const {
	return add_abils.hit_add;
}

void CharData::set_hit_add(const int v) {
	add_abils.hit_add = v;
}

int CharData::get_real_max_hit() const {
	return points.max_hit + add_abils.hit_add;
}

int CharData::get_hitreg() const {
	return add_abils.hitreg;
}

void CharData::set_hitreg(const int v) {
	add_abils.hitreg = v;
}

int CharData::get_move() const {
	return points.move;
}

void CharData::set_move(const int v) {
	if (v >= 0)
		points.move = v;
}

int CharData::get_max_move() const {
	return points.max_move;
}

void CharData::set_max_move(const int v) {
	if (v >= 0) {
		points.max_move = v;
	}
	msdp_report(msdp::constants::MAX_MOVE);
}

int CharData::get_move_add() const {
	return add_abils.move_add;
}

void CharData::set_move_add(const int v) {
	add_abils.move_add = v;
}

int CharData::get_real_max_move() const {
	return points.max_move + add_abils.move_add;
}

int CharData::get_movereg() const {
	return add_abils.movereg;
}

void CharData::set_movereg(const int v) {
	add_abils.movereg = v;
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
		if (this->in_room > 0) {
			MoneyDropStat::add(zone_table[world[this->in_room]->zone_rn].vnum, change);
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
	return GetInbornStr();
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
	return GetInbornDex();
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
	return GetInbornCon();
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
	return GetInbornInt();
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
	return GetInbornWis();
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
	return GetInbornCha();
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
	add_abils = {};
//	memset(&add_abils, 0, sizeof(char_played_ability_data));
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

// обрезает строку и выдергивает из нее предтитул
std::string CharData::GetTitle() const {
	std::string tmp = this->player_data.title;
	size_t pos = tmp.find('/');
	if (pos == std::string::npos) {
		return {};
	}
	tmp = tmp.substr(0, pos);
	pos = tmp.find(';');

	return pos == std::string::npos ? tmp : tmp.substr(0, pos);
};

std::string CharData::get_pretitle() const {
	std::string tmp = std::string(this->player_data.title);
	size_t pos = tmp.find('/');
	if (pos == std::string::npos) {
		return {};
	}
	tmp = tmp.substr(0, pos);
	pos = tmp.find(';');
	return pos == std::string::npos
		   ? std::string()
		   : tmp.substr(pos + 1, tmp.length() - (pos + 1));
}

std::string CharData::get_race_name() const {
	return PlayerRace::GetRaceNameByNum(GET_KIN(this), GET_RACE(this), this->get_sex());
}

std::string CharData::GetTitleAndNameWithoutClan() const {
	std::string result = get_name();

	auto pre_title = get_pretitle();
	if (!pre_title.empty()) {
		result = fmt::format("{} {}", pre_title, get_name());
	}

	auto title = GetTitle();
	if (!title.empty() && GetLevel() >= MIN_TITLE_LEV) {
		return fmt::format("{}, {}", result, title);
	}

	return result;
}

std::string CharData::GetClanTitleAddition() {
	if (CLAN(this) && !IS_IMMORTAL(this)) {
		return fmt::format("({})", GET_CLAN_STATUS(this));
	}

	return {};
}

std::string CharData::GetTitleAndName() {
	std::string clan_title = GetClanTitleAddition();
	if (!clan_title.empty()) {
		return fmt::format("{} {}", GetTitleAndNameWithoutClan(), clan_title);
	 } else {
		return GetTitleAndNameWithoutClan();
	}
}

std::string CharData::GetNameWithTitleOrRace() {
	std::string title = GetTitleAndNameWithoutClan();
	if (title == get_name()) {
		return fmt::format("{} {}", get_race_name(), title);
	}

	return title;
}

std::string CharData::race_or_title() {
	std::string clan_title = GetClanTitleAddition();
	if (!clan_title.empty()) {
		return fmt::format("{} {}", GetNameWithTitleOrRace(), clan_title);
	} else {
		return GetNameWithTitleOrRace();
	}
}

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
	this->UnsetFlag(EPrf::kSkirmisher);
}

void CharData::add_follower(CharData *ch) {

	if (ch->IsNpc() && ch->IsFlagged(EMobFlag::kNoGroup))
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
	if (!this->IsNpc() || ch->IsNpc() || !get_role(static_cast<unsigned>(EMobClass::kBoss))) {
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
	if (!this->IsNpc() || ch->IsNpc() || !get_role(static_cast<unsigned>(EMobClass::kBoss))) {
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

	if (!this->IsNpc() || !get_role(static_cast<unsigned>(EMobClass::kBoss))) {
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

	this->set_hit(this->get_real_max_hit());
	this->set_move(this->get_real_max_move());
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
		
	this->set_hit(1 + proto->get_hit());
	this->set_max_hit(this->get_hit() + floorf(proto->get_max_hit()));
	
	this->set_move(proto->get_real_max_move());
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
	this->player_data.PNames[ECase::kNom] = proto->player_data.PNames[ECase::kNom];
	this->player_data.PNames[ECase::kGen] = proto->player_data.PNames[ECase::kGen];
	this->player_data.PNames[ECase::kDat] = proto->player_data.PNames[ECase::kDat];
	this->player_data.PNames[ECase::kAcc] = proto->player_data.PNames[ECase::kAcc];
	this->player_data.PNames[ECase::kIns] = proto->player_data.PNames[ECase::kIns];
	this->player_data.PNames[ECase::kPre] = proto->player_data.PNames[ECase::kPre];
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
	this->char_specials.saved.act = proto->char_specials.saved.act;
	this->set_touching(nullptr);
	if (this->get_protecting()) {
		this->remove_protecting();
	}
	// ресторим статы
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
			this->set_skill(skill.GetId(), proto->GetSkill(skill.GetId()));
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

void CharData::set_wait(const unsigned _) {

	if (_ > 0) {
//	debug::backtrace(runtime_config.logs(SYSLOG).handle());
//		log("ставим вайт для %s (%d)", GET_NAME(this), GET_MOB_VNUM(this));
		chardata_wait_list.insert(this);
		m_wait = _;
	}
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
		if (get_role(static_cast<unsigned>(EMobClass::kBoss)) && !attackers_.empty() && !GetEnemy()) {
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
		(this->IsFlagged(EPrf::kCoderinfo) || (IS_CHARMICE(this) && (this->get_master()->IsFlagged(EPrf::kCoderinfo)))))
		needSend = true;
	if (!needSend && to_tester &&
		(this->IsFlagged(EPrf::kTester) || (IS_CHARMICE(this) && (this->get_master()->IsFlagged(EPrf::kTester)))))
		needSend = true;
	if (!needSend)
		return;

	va_list args;
	char tmpbuf[kMaxStringLength];

	va_start(args, msg);
	vsnprintf(tmpbuf, sizeof(tmpbuf), msg, args);
	va_end(args);

	if (tmpbuf[0] == '\0') {
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
			&& (!same_room || this->in_room == f->follower->in_room)) {
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

#include "utils/backtrace.h"

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
	if (plr->GetPosition() > EPosition::kSit) {
		plr->SetPosition(EPosition::kSit);
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

bool CharData::HasWeapon() {
	if ((GET_EQ(this, EEquipPos::kWield)
	  && GET_EQ(this, EEquipPos::kWield)->get_type() != EObjType::kLightSource)
	  || (GET_EQ(this, EEquipPos::kHold)
	  && GET_EQ(this, EEquipPos::kHold)->get_type() != EObjType::kLightSource)
	  || (GET_EQ(this, EEquipPos::kBoths)
	  && GET_EQ(this, EEquipPos::kBoths)->get_type() != EObjType::kLightSource)) {
		return true;
	}
	return false;
}

player_special_data::player_special_data() :
	poofin(nullptr),
	poofout(nullptr),
	aliases(nullptr),
	may_rent(0),
	agressor(0),
	agro_time(0),
	rskill(nullptr),
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
	if (ch->IsNpc() || IS_GOD(ch))
		return std::clamp(stat_value, kLeastBaseStat, kMobBaseStatCap);
	else
		return std::clamp(stat_value, kLeastBaseStat, MUD::Class(ch->GetClass()).GetBaseStatCap(stat_id));
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

void CharData::AddRunestone(const Runestone &stone) {
	if (player_specials->runestones.IsFull(this)) {
		SendMsgToChar
			("В вашей памяти не осталось места для новых рунных меток. Сперва забудьте какую-нибудь.\r\n", this);
		return;
	}

	if (player_specials->runestones.AddRunestone(stone)) {
		auto msg = fmt::format(
			"Вы осмотрели надпись и крепко запомнили начертанное огненными рунами слово '&R{}&n'.\r\n",
			stone.GetName());
		SendMsgToChar(msg, this);
	} else {
		SendMsgToChar("Руны всё время странно искажаются и вам не удаётся их запомнить.\r\n", this);
	}
	player_specials->runestones.DeleteIrrelevant(this);
};

void CharData::RemoveRunestone(const Runestone &stone) {
	if (player_specials->runestones.RemoveRunestone(stone)) {
		auto msg = fmt::format("Вы полностью забыли, как выглядит рунная метка '&R{}&n'.\r\n", stone.GetName());
		SendMsgToChar(msg, this);
	} else {
		SendMsgToChar("Чтобы забыть что-нибудь ненужное, следует сперва изучить что-нибудь ненужное...", this);
	}
};

void CharData::IncreaseStatistic(CharStat::ECategory category, ullong increment) {
	player_specials->saved.personal_statistics_.Increase(category, increment);
};

void CharData::ClearStatisticElement(CharStat::ECategory category) {
	player_specials->saved.personal_statistics_.ClearElement(category);
};

ullong CharData::GetStatistic(CharStat::ECategory category) const {
	return player_specials->saved.personal_statistics_.GetValue(category);
};

int GetRealLevel(const CharData *ch) {

	if (ch->IsNpc()) {
		return std::clamp(ch->GetLevel() + ch->get_level_add(), 0, kMaxMobLevel);
	}

	if (IS_IMMORTAL(ch)) {
		return ch->GetLevel();
	}

	return std::clamp(ch->GetLevel() + ch->get_level_add(), 1, kLvlImmortal - 1);
}

int GetRealLevel(const std::shared_ptr<CharData> &ch) {
	return GetRealLevel(ch.get());
}

int GetRealRemort(const CharData *ch) {
	return std::clamp(ch->get_remort() + ch->get_remort_add(), 0, kMaxRemort);
}

int GetRealRemort(const std::shared_ptr<CharData> &ch) {
	return GetRealRemort(ch.get());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
