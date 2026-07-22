// $RCSfile: char.cpp,v $     $Date: 2012/01/27 04:04:43 $     $Revision: 1.85 $
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "gameplay/mechanics/condition.h"
#include "utils/grammar/gender.h"
#include "gameplay/mechanics/minions.h"
#include "administration/privilege.h"
#include "char_player.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/mechanics/follow.h"
#include "utils/cache.h"
#include "gameplay/fight/fight.h"
#include "gameplay/clans/house.h"
#include "engine/network/msdp/msdp_constants.h"
#include "utils/backtrace.h"
#include "engine/db/global_objects.h"
#include "char_data.h"
#include "gameplay/statistics/money_drop.h"
#include "gameplay/economics/currencies.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/affects/affect_handler.h"
#include "gameplay/mechanics/illumination.h"
#include "engine/ui/alias.h"
#include "gameplay/core/remort.h"

#include <fmt/format.h>
#include <random>

std::string PlayerI::empty_const_str;
MapSystem::Options PlayerI::empty_map_options;

namespace {

// * На перспективу - втыкать во все методы character.
void check_purged(const CharData *ch, const char *fnc) {
	// Не спамим SYSLOG на каждое использование "очищенного" (purged) чара:
	// при шторме use-after-purge (моб умер посреди боевого раунда, а боевой
	// код продолжил дёргать его статы) набегали десятки одинаковых строк +
	// дорогих backtrace и вешали пульс на 100+ мс. Единичный случай пропускаем,
	// а после 3 срабатываний подряд один раз оповещаем богов через mudlog и
	// снимаем один backtrace.
	static int in_a_row = 0;
	if (ch->purged()) {
		if (++in_a_row == 3) {
			mudlog(fmt::format("SYSERR: Using purged character ({}).", fnc), BRF, kLvlGod, SYSLOG, true);
			debug::backtrace(runtime_config.logs(SYSLOG).handle());
		}
	} else {
		in_a_row = 0;
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
	script(new Script()) {
	this->zero_init();
	caching::character_cache.Add(this);
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
	if (this->IsNpc()) {
		return 0 != zone_table[world[in_room]->zone_rn].used;
	}
	return false;
}

//вычисление штрафов за голод и жажду
//P_DAMROLL, P_HITROLL, P_CAST, P_MEM_GAIN, P_MOVE_GAIN, P_HIT_GAIN

void CharData::reset() {
	int i;

	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		GET_EQ(this, i) = nullptr;
	}
	memset((void *) &add_abils, 0, sizeof(add_abils));

	followers.clear();
	m_master = nullptr;
	in_room = kNowhere;
	carrying = nullptr;
	if (this->get_protecting()) {
		remove_protecting();
	}
	set_touching(nullptr);
	battle_affects = clear_flags;
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

bool CharData::has_any_affect(const affects_list_t &affects) {
	for (const auto &affect : affects) {
		if (AFF_FLAGGED(this, affect)) {
			return true;
		}
	}

	return false;
}

size_t CharData::remove_random_affects(const size_t count) {
	// issue.affect-migration: dedup adjacent same-affect slots by IDENTITY (affect_type), not by the
	// casting spell -- migrated affects share Affect::type == kUndefined, so keying on type collapsed
	// them all into one "type" and listed only a single one as removable.
	Affect<EApply>::shared_ptr last_pushed;
	std::deque<char_affects_list_t::iterator> removable_affects;
	for (auto affect_i = affected.begin(); affect_i != affected.end(); ++affect_i) {
		const auto &affect = *affect_i;

		if ((!last_pushed || !SameAffectIdentity(affect, last_pushed)) && affect->removable()) {
			removable_affects.push_back(affect_i);
			last_pushed = affect;
		}
	}

	const auto to_remove = std::min(count, removable_affects.size());
	if (to_remove > 0) {
		std::shuffle(removable_affects.begin(), removable_affects.end(), std::mt19937(std::random_device()()));
		for (auto counter = 0u; counter < to_remove; ++counter) {
			const auto affect_i = removable_affects[counter];
			RemoveAffect(this, affect_i);    //count тут не сработает, удаляются все аффекты а не первый
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
	fight_targets_.protecting = nullptr;
	fight_targets_.intercepting = nullptr;
	fight_targets_.enemy = nullptr;
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
	was_attacked_ = false;
	restore_timer_ = 0;
	// char_data
	set_rnum(kNobody);
	in_room = 0;
	m_wait = 0u;
	punctual_wait = 0;
	last_comm.clear();
	player_specials = nullptr;
	carrying = nullptr;
	desc = nullptr;
	followers.clear();
	m_master = nullptr;
	caster_level = 0;
	damage_level = 0;
	track_dirs = 0;
	check_aggressive = false;
	extract_timer = 0;
	initiative = 0;
	battle_counter = 0;
	round_counter = 0;
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

	int i, id = -1;
	struct alias_data *a;

	if (!this->IsNpc() && !get_name().empty()) {
		id = GetPlayerTablePosByName(GET_NAME(this));
		if (id >= 0) {
			player_table[id].level = GetRealLevel(this);
			player_table[id].remorts = remort::GetRealRemort(this);
			player_table[id].activity = number(0, kObjectSaveActivity - 1);
		}
	}

	if (!this->IsNpc() || (this->IsNpc() && this->get_rnum() == -1)) {
		if (this->IsNpc() && this->mob_specials.Questor)
			free(this->mob_specials.Questor);
		this->summon_helpers.clear();
	} else if ((i = this->get_rnum())
		>= 0) {    // otherwise, free strings only if the string is not pointing at proto

		if (this->mob_specials.Questor && this->mob_specials.Questor != mob_proto[i].mob_specials.Questor)
			free(this->mob_specials.Questor);
	}
	this->affected.clear();
	this->timed_skill.clear();
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
		ClearRunestones(this);

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

	followers.clear();
}

void CharData::setSkillCooldown(ESkill skill_id, unsigned cooldown) {
	if (skills_.SetCooldown(skill_id, cooldown)) {
		chardata_cooldown_list.insert(this);
	}
}

void CharData::clear_skills() {
	skills_.Clear();
}

int CharData::get_skills_count() const {
	return skills_.Count();
}

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
	fight_targets_.intercepting = vict;
}

CharData *CharData::get_touching() const {
	return fight_targets_.intercepting;
}

void CharData::set_protecting(CharData *vict) {
	if (fight_targets_.protecting) {
		remove_protecting();
	}
	fight_targets_.protecting = vict;
	vict->who_protecting.push_back(this);
}

void CharData::remove_protecting() {
	if (fight_targets_.protecting) {
		auto predicate = [this](auto p) { return (this  ==  p); };
		auto it = std::find_if(fight_targets_.protecting->who_protecting.begin(), fight_targets_.protecting->who_protecting.end(), predicate);
		if (it != fight_targets_.protecting->who_protecting.end()) {
			fight_targets_.protecting->who_protecting.erase(it);
		}
	}
	fight_targets_.protecting = nullptr;
}

CharData *CharData::get_protecting() const {
	return fight_targets_.protecting;
}

void CharData::SetEnemy(CharData *enemy) {
	fight_targets_.enemy = enemy;
}

CharData *CharData::GetEnemy() const {
	return fight_targets_.enemy;
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
bool AWAKE(const CharData *ch) {
	return ch->GetPosition() > EPosition::kSleep
		&& !AFF_FLAGGED(ch, EAffect::kSleep);
}

bool IsMale(const CharData *ch) {
	return ch->get_sex() == EGender::kMale;
}

bool IsFemale(const CharData *ch) {
	return ch->get_sex() == EGender::kFemale;
}

bool IsPoly(const CharData *ch) {
	return ch->get_sex() == EGender::kPoly;
}

// * Внутри цикла чар нигде не пуржится и сам список соответственно не меняется.
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

// * Удача (мораль) для расчетов в скилах и вывода чару по счет все.
int CharData::calc_morale() const {
	return GetRealCha(this) / 2 + GET_MORALE(this);
}
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

std::string CharData::GetTitleAndNameWithoutClan() const {
	std::string result = get_name();

	auto pre_title = get_pretitle();
	if (!pre_title.empty()) {
		result = fmt::format("{} {}", pre_title, get_name());
	}

	auto title = GetTitle();
	if (!title.empty() && GetLevel() >= kMinTitleLev) {
		return fmt::format("{}, {}", result, title);
	}

	return result;
}

std::string CharData::GetClanTitleAddition() {
	if (CLAN(this) && !privilege::IsImmortal(this)) {
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
		return fmt::format("{} {}", MUD::RaceMessages().GetMessage(GET_RACE(this), this->get_sex()), title);
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

void CharData::cleanup_script() {
	script->cleanup();
}

const CharData::role_t &CharData::get_role_bits() const {
	return role_;
}

// отмечает, что непися атаковал игрок: взводит флаг пост-боевого таймера рефреша
// issue.mob-flag-affect-materialization: now marks ANY NPC a player engages (was: bosses only), so the
// out-of-combat restore can re-materialize buffs the player dispelled during the fight.
void CharData::mark_attacked(CharData *attacker) {
	if (!this->IsNpc() || attacker->IsNpc()) {
		return;
	}
	was_attacked_ = true;
}

// обновление босса вне боя по прошествии MOB_RESTORE_TIMER секунд
void CharData::restore_mob() {
	restore_timer_ = 0;
	was_attacked_ = false;

	this->set_hit(this->get_real_max_hit());
	this->set_move(this->get_real_max_move());
	update_pos(this);

	for (auto spell_id = ESpell::kFirst; spell_id <= ESpell::kLast; ++spell_id) {
		GET_SPELL_MEM(this, spell_id) = GET_SPELL_MEM(&mob_proto[this->get_rnum()], spell_id);
	}
	this->caster_level = (&mob_proto[this->get_rnum()])->caster_level;
}
//
void CharData::restore_npc() {
	if(!this->IsNpc()) return;
	
	was_attacked_ = false;
	auto proto = (&mob_proto[this->get_rnum()]);
	// ресторим хпшки / мувы
		
	this->set_hit(1 + proto->get_hit());
	this->set_max_hit(this->get_hit() + floorf(proto->get_max_hit()));
	
	this->set_move(proto->get_real_max_move());
	update_pos(this);
	// ресторим хиты / дамы / ас / армор
	this->set_weight(proto->get_weight());
	this->set_height(proto->get_height());
	this->real_abils.size = proto->real_abils.size;
	this->real_abils.hitroll = proto->real_abils.hitroll;
	this->real_abils.armor = proto->real_abils.armor;
	this->real_abils.damroll = proto->real_abils.damroll;
	this->add_abils.armour = proto->add_abils.armour;
	this->add_abils.initiative_add = proto->add_abils.initiative_add;
	this->add_abils.morale = proto->add_abils.morale;
	// ресторим резисты ФР/МР/АР
	this->add_abils.aresist = proto->add_abils.aresist;
	this->add_abils.mresist = proto->add_abils.mresist;
	this->add_abils.presist = proto->add_abils.presist;
	// ресторим имена
	this->player_data.PNames[grammar::ECase::kNom] = proto->player_data.PNames[grammar::ECase::kNom];
	this->player_data.PNames[grammar::ECase::kGen] = proto->player_data.PNames[grammar::ECase::kGen];
	this->player_data.PNames[grammar::ECase::kDat] = proto->player_data.PNames[grammar::ECase::kDat];
	this->player_data.PNames[grammar::ECase::kAcc] = proto->player_data.PNames[grammar::ECase::kAcc];
	this->player_data.PNames[grammar::ECase::kIns] = proto->player_data.PNames[grammar::ECase::kIns];
	this->player_data.PNames[grammar::ECase::kPre] = proto->player_data.PNames[grammar::ECase::kPre];
	this->SetCharAliases(proto->GetCharAliases());
	this->set_npc_name(proto->get_name());
    // кубики // екстра атаки
	this->mob_specials.damnodice = proto->mob_specials.damnodice;
	this->mob_specials.damsizedice = proto->mob_specials.damsizedice;
	this->mob_specials.extra_attack = proto->mob_specials.extra_attack;
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
			SetSkill(this, skill.GetId(), GetSkill(proto, skill.GetId()));
		}
	}

	for (const auto &feat : MUD::Feats()) {
		if (!proto->HaveFeat(feat.GetId())) {
				this->UnsetFeat(feat.GetId());
		}
	}
	this->caster_level = proto->caster_level;
}

void CharData::set_master(CharData::ptr_t master) {
	if (follow::MakesLoop(this, master)) {
		follow::ReportLoopError(this, master);
		return;
	}
	m_master = master;
}

void CharData::set_wait(const unsigned _) {

	if (_ > 0) {
/*		if (!this->IsNpc()) {
			debug::backtrace(runtime_config.logs(SYSLOG).handle());
			mudlog(fmt::format("ставим вайт для {} {}", GET_NAME(this), _));
		}
*/
		chardata_wait_list.insert(this);
		m_wait = _;
	}
}

// инкремент и проверка таймера на рестор босса,
// который находится вне боя и до этого был кем-то бит
// (т.к. имеет не нулевой список атакеров)
bool CharData::inc_restore_timer(int num) {
	if (!was_attacked_ || GetEnemy()) {   // only NPCs a player fought (mark_attacked), now out of combat
		return false;
	}
	restore_timer_ += num;
	if (restore_timer_ <= num) {
		return false;   // out of combat, but not long enough yet (fires on the 2nd tick, as before)
	}
	// Long enough out of combat: full stat/HP/moves/spell restore for EVERY mob (was: bosses only).
	// restore_mob() resets restore_timer_ + was_attacked_. The caller then re-materializes any dispelled
	// intrinsic buffs (see game_limits).
	restore_mob();
	return true;
}

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
	brief_shields_mode(EBriefShieldsMode::kBrief),
	who_mana(0),
	telegram_id(0),
	lastGloryRespecTime(0) {
	memset(EMail, 0, sizeof(EMail));
	memset(LastIP, 0, sizeof(LastIP));
}

// dummy spec area for mobs
player_special_data::shared_ptr player_special_data::s_for_mobiles = std::make_shared<player_special_data>();

int ClampBaseStat(const CharData *ch, const EBaseStat stat_id, const int stat_value) {
	if (ch->IsNpc() || privilege::IsGod(ch))
		return std::clamp(stat_value, kLeastBaseStat, kMobBaseStatCap);
	else
		return std::clamp(stat_value, kLeastBaseStat, std::max(kLeastBaseStat, MUD::Class(ch->GetClass()).GetBaseStatCap(stat_id)));
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

// issue.runestones: CharData::AddRunestone / RemoveRunestone (with messages) moved to
// CharacterRunestoneRoster in gameplay/mechanics/rune_stones.cpp; char_data.h now forwards inline.

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

	// GetRealLevel is a LEVEL accessor: characters whose stored level is in the immortal
	// range keep their real level (no mortal clamp). This must gate on the level itself,
	// NOT on privilege membership -- otherwise a high-level character who is not listed in
	// privilege.xml gets clamped to kLvlImmortal-1 while GetLevel() stays high, and the
	// level-up loops that test GetRealLevel but mutate GetLevel() spin forever (issue.advance-crash-bug).
	if (ch->GetLevel() >= kLvlImmortal) {
		return ch->GetLevel();
	}

	return std::clamp(ch->GetLevel() + ch->get_level_add(), 1, kLvlImmortal - 1);
}

int GetRealLevel(const std::shared_ptr<CharData> &ch) {
	return GetRealLevel(ch.get());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
