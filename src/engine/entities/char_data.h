// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_HPP_INCLUDED
#define CHAR_HPP_INCLUDED

#include "player_i.h"
#include "gameplay/mechanics/alignment.h"
#include "administration/punishments.h"
#include "gameplay/fight/pk.h"
#include "gameplay/magic/magic_temp_spells.h"
#include "gameplay/mechanics/obj_sets.h"
#include "gameplay/mechanics/dead_load.h"
#include "engine/db/db.h"
#include "entities_constants.h"
#include "engine/structs/bitset_flags.h"
#include "room_data.h"
#include "obj_data.h"
#include "engine/scripting/dg_scripts.h"
#include "gameplay/communication/ignores.h"
#include "gameplay/crafting/im.h"
#include "gameplay/skills/skills.h"
#include "gameplay/economics/currency_storage.h"
#include "gameplay/mechanics/rune_stones.h"
#include "utils/utils.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/mem_queue.h"
#include "gameplay/ai/mob_memory.h"
#include "engine/network/logon.h"
#include "gameplay/statistics/char_stat.h"

#include <unordered_map>
#include <bitset>
#include <list>
#include <map>

// These data contain information about a players time data
struct time_data {
	time_t birth;        // This represents the characters age
	time_t logon;        // Time of the last logon (used to calculate played)
	int played;        // This is the total accumulated time played in secs
};

// general player-related info, usually PC's and NPC's
struct char_player_data {
	std::string long_descr;    // for 'look'
	std::string description;    // Extra descriptions
	std::string title;        // PC / NPC's title
	EGender sex;        // PC / NPC's sex
	struct time_data time;            // PC's AGE in days
	ubyte weight;        // PC / NPC's weight
	ubyte height;        // PC / NPC's height

	std::array<std::string, 6> PNames;
	ubyte Religion;
	ubyte Race;        // PC / NPC's race
};

enum class EBriefShieldsMode : int {
	kOff = 0,
	kBrief = 1,
	kCompact = 2,
	kCompressed = 3
};
const int MAX_ADD_SLOTS = 10;  // кол-во +слотов со шмоток

// Char's additional abilities. Used only while work
struct char_played_ability_data {
 public:
	int weight_add;
	int height_add;
	int size_add;
	int age_add;
	int hit_add;
	int move_add;
	int hitreg;
	int movereg;
	int manareg;
	std::array<sbyte, MAX_ADD_SLOTS> obj_slot{};
	int armour;
	int ac_add;
	int hr_add;
	int dr_add;
	int absorb;
	int morale;
	int cast_success;
	int initiative_add;
	int poison_add;
	int bind_add;        // issue.affects-improve: EApply::kBind accumulator (no-flee); not persisted
	int skill_reduce_add;
	int pray_add;
	int percent_exp_add;
	int percent_physdam_add;
	int percent_spellpower_add;
	int percent_spell_blink_phys;
	int percent_spell_blink_mag;
	std::array<int, to_underlying(ESaving::kLast) + 1> apply_saving_throw{};	// Saving throw (Bonuses)
	std::array<int, EResist::kLastResist + 1> apply_resistance{};					// Сопротивления повреждениям
	int mresist;
	int aresist;
	int presist; // По просьбе <сумасшедшего> (зачеркнуто) безбашенного билдера поглощение физ.урона в %

};

// Char's abilities.
struct char_ability_data {
	std::array<ubyte, to_underlying(ESpell::kLast) + 1> SplKnw{}; // array of SPELL_KNOW_TYPE
	std::array<ubyte, to_underlying(ESpell::kLast) + 1> SplMem{}; // array of MEMed SPELLS
	std::bitset<to_underlying(EFeat::kLast) + 1> Feats{};
	sbyte size{};
	int hitroll{};
	int damroll{};
	short armor{};
};

// Char's points.
struct char_point_data {
	int hit;
	int move;

	int max_move;    // Max move for PC/NPC
	int max_hit;        // Max hit for PC/NPC
};

/*
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
	int alignment;        // +-1000 for alignments
	FlagData act;        // act flag for NPC's; player flag for PC's
	BitsetFlags<EAffect> affected_by;
	// Bitvector for spells/skills affected by
};

// Special playing constants shared by PCs and NPCs which aren't in pfile
struct char_special_data {
	EPosition position;        // Standing, fighting, sleeping, etc.

	int carry_weight;        // Carried weight
	int carry_items;        // Number of items carried
	int timer;            // Timer for update
	time_t who_last; // таймстамп последнего использования команды кто

	struct char_special_data_saved saved;            // constants saved in plrfile
};

// Specials used by NPCs, not PCs
struct mob_special_data {
	byte last_direction;    // The last direction the monster went
	int attack_type;        // The Attack Type Bitvector for NPC's
	EPosition default_pos;    // Default position for NPC
	mob_ai::MemoryRecord *memory;    // List of attackers to remember
	byte damnodice;        // The number of damage dice's
	byte damsizedice;    // The size of the damage dice's
	std::array<int, kMaxDest> dest;
	int dest_dir;
	int dest_pos;
	int dest_count;
	int activity;
	FlagData npc_flags;
	byte extra_attack;
	byte like_work;
	int MaxFactor;
	int GoldNoDs;
	int GoldSiDs;
	int HorseState;
	int LastRoom;
	char *Questor;
	int speed;
	int hire_price;
	ESpell capable_spell;
	bool have_spell;
};

// Structure used for extra_attack - bash, kick, diasrm, chopoff, etc
struct extra_attack_type {
	EExtraAttack used_attack{};
	CharData *victim{nullptr};
};

struct CastAttack {
	ESpell spell_id{ESpell::kUndefined};
	ESpell spell_subst{ESpell::kUndefined};
	CharData *tch{nullptr};
	ObjData *tobj{nullptr};
	RoomData *troom{nullptr};
};

struct player_special_data_saved {
	player_special_data_saved();

	int wimp_level;        // Below this # of hit points, flee!
	int invis_level;        // level of invisibility
	RoomVnum load_room;    // Which room to place char in
	FlagData pref;        // preference flags for PC's.
	int bad_pws;        // number of bad password attemps
	std::array<int, 3> conditions{};        // Drunk, full, thirsty

	int DrunkState;
	int olc_zone;
	int NameGod;
	long NameIDGod;
	long GodsLike;

	char EMail[128];
	char LastIP[128];

	int stringLength;
	int stringWidth;

	CharStat personal_statistics_;
	long ntfyExchangePrice;
	int HiredCost;
	EBriefShieldsMode brief_shields_mode;
	unsigned int who_mana; // количество энергии для использования команды кто
	unsigned long int telegram_id;// идентификатор телеграма
	time_t lastGloryRespecTime; // дата последнего респека славой
};

struct player_special_data {
	using shared_ptr = std::shared_ptr<player_special_data>;
	using ignores_t = std::list<ignore_data::shared_ptr>;

	player_special_data();

	player_special_data_saved saved;

	char *poofin;        // Description on arrival of a god.
	char *poofout;        // Description upon a god's exit.
	struct alias_data *aliases;    // Character's aliases
	time_t may_rent;        // PK control
	int agressor;        // Agression room(it is also a flag)
	time_t agro_time;        // Last agression time (it is also a flag)
	im_rskill *rskill;    // Известные рецепты
  	CharacterRunestoneRoster runestones; // рунные камни для врат и не только
	int *logs;        // уровни подробности каналов log

	char *Exchange_filter;
	ignores_t ignores;
	char *Karma; // Записи о поощрениях, наказаниях персонажа

  	network::LogonRecords logons;

	punishments::CharPunishments punishments;  // mute/dumb/hell/name/freeze/gcurse/unreg

	char *clanStatus; // строка для отображения приписки по кто
	std::shared_ptr<class Clan> clan; // собсна клан, если он есть
	std::shared_ptr<class ClanMember> clan_member; // поле мембера в клане

	static player_special_data::shared_ptr s_for_mobiles;
};

#include "gameplay/skills/character_skills.h"

class ProtectedCharData;    // to break up cyclic dependencies

class CharacterRNum_ChangeObserver {
 public:
	using shared_ptr = std::shared_ptr<CharacterRNum_ChangeObserver>;

	CharacterRNum_ChangeObserver() = default;
	virtual ~CharacterRNum_ChangeObserver() = default;

	virtual void notify(ProtectedCharData &character, MobRnum old_rnum) = 0;
};

class ProtectedCharData : public PlayerI {
 public:
	ProtectedCharData();
	ProtectedCharData(const ProtectedCharData &rhv);
	ProtectedCharData &operator=(const ProtectedCharData &rhv);

	auto get_rnum() const { return m_rnum; }
	void set_rnum(MobRnum rnum);

	void subscribe_for_rnum_changes(const CharacterRNum_ChangeObserver::shared_ptr &observer) {
		m_rnum_change_observers.insert(observer);
	}
	void unsubscribe_from_rnum_changes(const CharacterRNum_ChangeObserver::shared_ptr &observer) {
		m_rnum_change_observers.erase(observer);
	}

 private:
	MobRnum m_rnum;        // Mob's rnum

	std::unordered_set<CharacterRNum_ChangeObserver::shared_ptr> m_rnum_change_observers;
};

// * Общий класс для игроков/мобов.
class CharData;   // for FightTargets below

// CharData's combat-relationship targets, grouped: each must be cleared when its target is
// extracted. Reached through get/set_protecting, get/set_touching and Get/SetEnemy.
struct FightTargets {
	CharData *enemy{nullptr};         // current fight target
	CharData *protecting{nullptr};    // target of 'прикрыть' (cover)
	CharData *intercepting{nullptr};  // target of 'перехватить' (intercept)
};

class CharData : public ProtectedCharData {
// новое
 public:
	using ptr_t = CharData *;
	using shared_ptr = std::shared_ptr<CharData>;
	using char_affects_list_t = std::list<Affect<EApply>::shared_ptr>;
	using role_t = std::bitset<9>;
	using followers_list_t = std::list<CharData *>;

	CharData();
	~CharData() override;

	friend void do_mtransform(CharData *ch, char *argument, int cmd, int subcmd);
	friend void medit_mobile_copy(CharData *dst, CharData *src);

	void SetFeat(EFeat feat_id) { real_abils.Feats.set(to_underlying(feat_id)); affect_total(this); };
	void UnsetFeat(EFeat feat_id) { real_abils.Feats.reset(to_underlying(feat_id)); affect_total(this); };
	bool HaveFeat(EFeat feat_id) const { return real_abils.Feats.test(to_underlying(feat_id)); };

	void clear_skills();
	int get_skills_count() const;
	const CharacterSkills::Map &GetCharSkills() const { return skills_.data(); }
	CharacterSkills &Skills() { return skills_; }
	const CharacterSkills &Skills() const { return skills_; }
	// Sets a skill's cooldown and registers the char in the global cooldown list (the one cooldown
	// op that is a char-level concern, hence here and not on CharacterSkills).
	void setSkillCooldown(ESkill skill_id, unsigned cooldown);
	int get_skill_bonus() const;
	void set_skill_bonus(int);
	int GetAddSkill(ESkill skill_id) const;
	void SetAddSkill(ESkill skill_id, int value);

	int get_obj_slot(int slot_num);
	void add_obj_slot(int slot_num, int count);
	// ресет нпс
	void restore_npc();
	CharData *get_touching() const;
	void set_touching(CharData *vict);

	CharData *get_protecting() const;
	std::vector<CharData *> who_protecting; // кто прикрыл
	void set_protecting(CharData *vict);
	void remove_protecting();

	EExtraAttack get_extra_attack_mode() const;
	CharData *GetExtraVictim() const;
	void SetExtraAttack(EExtraAttack Attack, CharData *vict);

	CharData *GetEnemy() const;
	void SetEnemy(CharData *enemy);

	void SetCast(ESpell spell_id, ESpell spell_subst, CharData *tch, ObjData *tobj, RoomData *troom);
	ESpell GetCastSpell() const;
	ESpell GetCastSubst() const;
	CharData *GetCastChar() const;
	ObjData *GetCastObj() const;

	bool purged() const;

	const std::string &get_name() const;
	void set_name(const char *name);
	const std::string &GetCharAliases() const { return name_; }
	void SetCharAliases(const char *name);
	void SetCharAliases(const std::string &name) { name_ = name; }
	void SetCharAliases(std::string &&name) { name_ = name; }
	const std::string &get_npc_name() const { return short_descr_; }
	void set_npc_name(const char *name);
	void set_npc_name(const std::string &name) { short_descr_ = name; }
	void set_npc_name(std::string &&name) { short_descr_ = name; }
	const std::string &get_name_str() const;
	const char *get_pad(unsigned pad) const;
	void set_pad(unsigned pad, const char *s);
	const char *get_long_descr() const;
	void set_long_descr(const char *);
	const char *get_description() const;
	void set_description(const char *);
	ECharClass GetClass() const;
	void set_class(ECharClass chclass);

	int GetLevel() const;
	int get_level_add() const;
	int get_remort() const;
	int get_remort_add() const;
	long get_exp() const;
	long get_uid() const;
	void set_level(int level);
	void set_level_add(int level);
	void set_uid(int uid);
	void set_exp(long exp);
	void set_remort(int num);
	void set_remort_add(short num);

	time_t get_last_logon() const;
	void set_last_logon(time_t num);

	time_t get_last_exchange() const;
	void set_last_exchange(time_t num);

	EGender get_sex() const;
	void set_sex(EGender sex);
	ubyte get_weight() const;
	void set_weight(ubyte);
	ubyte get_height() const;
	void set_height(ubyte);
	ubyte get_religion() const;
	void set_religion(ubyte);
	ubyte get_race() const;
	void set_race(ubyte);
	int get_hit() const;
	void set_hit(int);
	int get_max_hit() const;
	void set_max_hit(int);
	int get_hit_add() const;
	void set_hit_add(int);
	int get_real_max_hit() const;
	int get_hitreg() const;
	void set_hitreg(int);
	int get_move() const;
	void set_move(int);
	int get_max_move() const;
	void set_max_move(int);
	int get_move_add() const;
	void set_move_add(int);
	int get_real_max_move() const;
	int get_movereg() const;
	void set_movereg(int);

	// issue.currency-storage: the per-owner currency container (single mutation chokepoint).
	currencies::CurrencyStorage &currency_storage() { return currency_storage_; }
	[[nodiscard]] const currencies::CurrencyStorage &currency_storage() const { return currency_storage_; }

	int calc_morale() const;

	int get_str() const;
	int GetInbornStr() const;
	void set_str(int);
	void inc_str(int);
	void time_set_glory_stats(time_t);
	int get_dex() const;
	int GetInbornDex() const;
	void set_dex(int);
	void inc_dex(int);
	int get_con() const;
	int GetInbornCon() const;
	void set_con(int);
	void inc_con(int);
	int get_wis() const;
	int GetInbornWis() const;
	void set_wis(int);
	void inc_wis(int);
	int get_int() const;
	int GetInbornInt() const;
	void set_int(int);
	void inc_int(int);
	int get_cha() const;
	int GetInbornCha() const;
	void set_cha(int);
	void inc_cha(int);

	void clear_add_apply_affects();
	int get_str_add() const;
	void set_str_add(int);
	int get_dex_add() const;
	void set_dex_add(int);
	int get_con_add() const;
	void set_con_add(int);
	int get_wis_add() const;
	void set_wis_add(int);
	int get_int_add() const;
	void set_int_add(int);
	int get_cha_add() const;
	void set_cha_add(int);

	/**
	** Returns true if character is mob and located in used zone.
	**/
	bool in_used_zone() const;

	std::string GetTitle() const;
	std::string get_pretitle() const;
	std::string GetTitleAndName();
	std::string GetNameWithTitleOrRace();
	std::string race_or_title();
	bool isAffected(EAffect flag) const;

	void set_who_mana(unsigned int);
	void set_who_last(time_t);
	unsigned int get_who_mana();
	time_t get_who_last();

	/// роли (mob only)
	bool get_role(unsigned num) const;
	void set_role(unsigned num, bool flag);
	const CharData::role_t &get_role_bits() const;

	void mark_attacked(CharData *attacker);

	// issue.mob-flag-affect-materialization: returns true on the tick the out-of-combat restore fires,
	// so the caller can re-materialize the mob's dispelled intrinsic buffs.
	bool inc_restore_timer(int num);
	obj_sets::activ_sum &obj_bonus();
	[[nodiscard]] const obj_sets::activ_sum &obj_bonus() const { return obj_bonus_; }

	void set_souls(int souls);
	void inc_souls();
	void dec_souls();
	int get_souls();
	void set_type_charmice(int type);
	int get_type_charmice();
	unsigned get_wait() const { return m_wait; }
	void set_wait(const unsigned _);
	void wait_dec() { m_wait -= 0 < m_wait ? 1 : 0; }
	void zero_wait() { m_wait = 0; }

	void reset() override;

	bool has_any_affect(const affects_list_t &affects);
	size_t remove_random_affects(size_t count);

	const role_t &get_role() const { return role_; }
	void set_role(const role_t &new_role) { role_ = new_role; }
	void msdp_report(const std::string &name);

	const followers_list_t &get_followers_list() const { return followers; }
	const player_special_data::ignores_t &get_ignores() const;
	void add_ignore(const ignore_data::shared_ptr& ignore);
	void clear_ignores();

	template<typename T>
	void set_affect(T affect) { char_specials.saved.affected_by.set(affect); }
	template<typename T>
	void remove_affect(T affect) { char_specials.saved.affected_by.unset(affect); }

	void set_purged(const bool _ = true) {
		purged_ = _;
		script->set_purged(_);
	}

	void cleanup_script();

	bool IsNpc() const { return is_npc_; }
	void SetNpcAttribute(bool _) { is_npc_ = _; }
	bool IsPlayer() const { return !IsNpc(); }

 private:
	const auto &get_player_specials() const { return player_specials; }
	auto &get_player_specials() { return player_specials; }

	std::string GetClanTitleAddition();
	std::string GetTitleAndNameWithoutClan() const;
	void zero_init();
	void restore_mob();

	void purge();

	CharacterSkills skills_;    // learned skills (level + cooldown)
	FightTargets fight_targets_;   // enemy / cover / intercept targets (cleared on target extraction)

	struct extra_attack_type extra_attack_; // атаки типа баша, пинка и т.п.
	struct CastAttack cast_attack_;   // каст заклинания
	bool purged_;  // true - чар очищен и ждет вызова delete для оболочки

	std::string name_;  // имя чара или алиасы моба
	std::string short_descr_;  // имя моба (им.падеж)
	ECharClass chclass_;  // профессия чара/класс моба
	bool is_npc_;
	int level_;  // уровень
	int level_add_;  // плюс на уровень
	long uid_;  // id чара (не тот, что для тригов), у мобов -1
	long exp_;  // экспа
	int remorts_;  // реморты
	int remorts_add_;  // плюсы на реморт
	time_t last_logon_;  // время последнего входа в игру //by kilnik
	time_t last_exchange_;  // последний вызов базара
	currencies::CurrencyStorage currency_storage_;  // деньги на руках
	int str_;  // родная сила
	int str_add_;  // плюсы на силу
	int dex_;  // родная ловкость
	int dex_add_;  // плюсы на ловкость
	int con_;  // родное тело
	int con_add_;  // плюсы на тело
	int wis_;  // родная мудра
	int wis_add_;  // плюсы на мудру
	int int_;  // родная инта
	int int_add_;  // плюсы на инту
	int cha_;  // родная харизма
	int cha_add_;  // плюсы на харизму
	role_t role_;  // аналог класса у моба
	bool was_attacked_{false};  // для боссов: атаковал ли босса игрок (взводит таймер рефреша после боя)
	// для боссов: таймер (в секундах), включающийся по окончанию боя
	// через который происходит сброс списка атакующих и рефреш моба
	int restore_timer_;
	obj_sets::activ_sum obj_bonus_;					// всякие хитрые бонусы с сетов (здесь, чтобы чармисов не обделить)
	int count_score;								// для режимов - количество набранных очков
	int souls;										// души, онли чернок
	int skill_bonus_;								// бонус ко всем умениям
	int type_charmice_;

	std::unordered_map<ESkill, int> skills_add_; 	// Бонусы к отдельным умениям

 public:
	bool isInSameRoom(const CharData *ch) const { return (this->in_room == ch->in_room); };
	RoomRnum in_room;    // Location (real room number)

 private:

	unsigned m_wait;            // wait for how many loops
	CharData *m_master;        // Who is char following?

 public:
	int punctual_wait;        // wait for how many loops (punctual style)
	std::string last_comm;        // последний приказ чармису перед окончанием лага

	struct char_player_data player_data;        // Normal data

	// Работа с полной строкой титула - титул+предтилул
  	void SetTitleStr(std::string_view title) { player_data.title = title; };
  	const std::string &GetTitleStr() const { return player_data.title; };

	struct char_played_ability_data add_abils;        // Abilities that add to main
	struct char_ability_data real_abils;        // Abilities without modifiers
	struct char_point_data points;        // Points

	struct char_special_data char_specials;        // PC/NPC specials

	EPosition GetPosition() const { return char_specials.position; };
  	void SetPosition(const EPosition position) { char_specials.position = position; };
	/* Character's flags actions */
	// Костыльная функция, которая пока нужна, потому что загрузчик файлов не часть чардаты и не friend class.
	void SetFlagsFromString(const std::string &string) { char_specials.saved.act.from_string(string.c_str()); };
  	void PrintFlagsToAscii(char *sink, size_t sink_size) const { char_specials.saved.act.tascii(FlagData::kPlanesNumber, sink, sink_size); };
  	void CopyFlagsFrom(CharData *source) { char_specials.saved.act = source->char_specials.saved.act ; };
	void SetFlag(const EMobFlag flag) { if (IsNpc()) { char_specials.saved.act.set(flag); }; };
  	void UnsetFlag(const EMobFlag flag) { if (IsNpc()) { char_specials.saved.act.unset(flag); }; };
  	[[nodiscard]] bool IsFlagged(const EMobFlag flag) const { return (IsNpc() && char_specials.saved.act.get(flag)); };
  	void SetFlag(const ENpcFlag flag) { if (IsNpc()) { mob_specials.npc_flags.set(flag); }; };
  	void UnsetFlag(const ENpcFlag flag) { if (IsNpc()) { mob_specials.npc_flags.unset(flag); }; };
  	[[nodiscard]] bool IsFlagged(const ENpcFlag flag) const { return (IsNpc() && mob_specials.npc_flags.get(flag)); };
  	void SetFlag(const EPlrFlag flag) { if (!IsNpc()) { char_specials.saved.act.set(flag); }; };
  	void UnsetFlag(const EPlrFlag flag) { if (!IsNpc()) { char_specials.saved.act.unset(flag); }; };
  	[[nodiscard]] bool IsFlagged(const EPlrFlag flag) const { return (!IsNpc() && char_specials.saved.act.get(flag)); };

	struct mob_special_data mob_specials;        // NPC specials

	player_special_data::shared_ptr player_specials;    // PC specials
  	void ClearStatistics() { player_specials->saved.personal_statistics_.Clear(); };
  	void ClearThisRemortStatistics() { player_specials->saved.personal_statistics_.ClearThisRemort(); };
  	void IncreaseStatistic(CharStat::ECategory category, ullong increment);
  	void ClearStatisticElement(CharStat::ECategory category);
  	ullong GetStatistic(CharStat::ECategory category) const;

  	void SetFlag(const EPrf flag) { if (!IsNpc()) { player_specials->saved.pref.set(flag); }; };
  	void UnsetFlag(const EPrf flag) { if (!IsNpc()) { player_specials->saved.pref.unset(flag); }; };
  	[[nodiscard]] bool IsFlagged(const EPrf flag) const { return (!IsNpc() && player_specials->saved.pref.get(flag)); };
	[[nodiscard]] EBriefShieldsMode GetBriefShieldsMode() const {
		if (IsNpc() || !IsFlagged(EPrf::kBriefShields)) {
			return EBriefShieldsMode::kOff;
		}

		return player_specials->saved.brief_shields_mode;
	};
	void SetBriefShieldsMode(const EBriefShieldsMode mode) {
		if (IsNpc()) {
			return;
		}

		player_specials->saved.brief_shields_mode = mode;
		if (mode == EBriefShieldsMode::kOff) {
			UnsetFlag(EPrf::kBriefShields);
		} else {
			SetFlag(EPrf::kBriefShields);
		}
	};

	int GetCarryingWeight() const { return char_specials.carry_weight; };
  	int GetCarryingQuantity() const { return char_specials.carry_items; };

	char_affects_list_t affected;    // affected by what spells
	std::unordered_map<ESkill, time_t> timed_skill;    // use which timed skill/spells
	std::unordered_map<EFeat, time_t> timed_feat;    // use which timed feats
	ObjData *equipment[EEquipPos::kNumEquipPos];    // Equipment array

	ObjData *carrying;    // Head of list
	DescriptorData *desc;    // NULL for mobiles
	ObjData::triggers_list_ptr proto_script;    // list of default triggers
	Script::shared_ptr script;    // script info for the object

	//отладочные сообщения имморталу/тестеру/кодеру

	// очередь изучаемых заклинаний
	SpellMemQueue mem_queue;

	int caster_level;
	int damage_level;
	std::unordered_map<long, PkMemory> pk_map;

	int track_dirs;
	bool check_aggressive;
	int extract_timer;

	BitsetFlags<ECharExtraFlag> Temporary;

	int initiative;
	int battle_counter;
	int round_counter;

	FlagData battle_affects;

	dead_load::OnDeadLoadList dl_list;    // загружаемые в труп предметы
	bool agrobd;        // показывает, агробд или нет

	std::map<ESpell, TemporarySpell> temp_spells;
	std::list<MobVnum> summon_helpers;
	std::vector<int> kill_list; //used only for MTRIG_KILL
 public:
	// FOLLOWERS
	std::list<CharData *> followers;
	CharData::ptr_t get_master() const { return m_master; }
	void set_master(CharData::ptr_t master);
	bool has_master() const { return nullptr != m_master; }
};

inline const player_special_data::ignores_t &CharData::get_ignores() const {
	const auto &ps = get_player_specials();
	return ps->ignores;
}

inline void CharData::add_ignore(const ignore_data::shared_ptr& ignore) {
	const auto &ps = get_player_specials();
	ps->ignores.push_back(ignore);
}

inline void CharData::clear_ignores() {
	get_player_specials()->ignores.clear();
}

inline int GET_INVIS_LEV(const CharData *ch) {
	if (ch->player_specials && ch->player_specials->saved.invis_level)
		return ch->player_specials->saved.invis_level;
	else
		return 0;
}
inline int GET_INVIS_LEV(const CharData::shared_ptr &ch) { return GET_INVIS_LEV(ch.get()); }

inline void SET_INVIS_LEV(const CharData *ch, const int level) {
	if (ch->player_specials) {
		ch->player_specials->saved.invis_level = level;
	}
}
inline void SET_INVIS_LEV(const CharData::shared_ptr &ch, const int level) { SET_INVIS_LEV(ch.get(), level); }

// issue.utils-cleaning: IsGood/IsEvil/IsNeutral moved to gameplay/mechanics/alignment.h
// (re-exported via the #include above so existing callers are unaffected).

inline void SetWaitState(CharData *ch, const unsigned cycle) {
	if (ch->get_wait() < cycle) {
		ch->set_wait(cycle);
	}
}

// Lag the character by `lag` battle rounds. Thin wrapper over SetWaitState that spells the
// "rounds, not raw pulses" intent at the call site -- preferred over the ubiquitous
// SetWaitState(ch, N * kBattleRound) idiom that previously dotted the codebase.
inline void SetBattleLag(CharData *ch, const unsigned lag) {
	SetWaitState(ch, lag * kBattleRound);
}

inline BitsetFlags<EAffect> &AFF_FLAGS(CharData *ch) { return ch->char_specials.saved.affected_by; }
inline const BitsetFlags<EAffect> &AFF_FLAGS(const CharData *ch) { return ch->char_specials.saved.affected_by; }
inline const BitsetFlags<EAffect> &AFF_FLAGS(const CharData::shared_ptr &ch) { return ch->char_specials.saved.affected_by; }

// Бывшие макросы GET_SPELL_MEM / GET_SPELL_TYPE из utils.h. Возвращают ссылку,
// т.к. используются и как lvalue (GET_SPELL_MEM(ch, sp)++ и т.п.).
constexpr ubyte &GET_SPELL_MEM(CharData *ch, ESpell spell) { return ch->real_abils.SplMem[to_underlying(spell)]; }
constexpr ubyte GET_SPELL_MEM(const CharData *ch, ESpell spell) { return ch->real_abils.SplMem[to_underlying(spell)]; }
inline ubyte &GET_SPELL_MEM(const CharData::shared_ptr &ch, ESpell spell) { return GET_SPELL_MEM(ch.get(), spell); }

constexpr ubyte &GET_SPELL_TYPE(CharData *ch, ESpell spell) { return ch->real_abils.SplKnw[to_underlying(spell)]; }
constexpr ubyte GET_SPELL_TYPE(const CharData *ch, ESpell spell) { return ch->real_abils.SplKnw[to_underlying(spell)]; }
inline ubyte &GET_SPELL_TYPE(const CharData::shared_ptr &ch, ESpell spell) { return GET_SPELL_TYPE(ch.get(), spell); }

inline bool AFF_FLAGGED(const CharData *ch, const EAffect flag) {
	return AFF_FLAGS(ch).get(flag);
}

// issue.affect-migration / TEMPORARY: a flag-only stealth/utility check. Many MOBS apply skills
// like hide/sneak/camouflage/light-walk as a bare EAffect flag, with NO full affect struct, so
// these sites cannot yet use IsAffected (which needs the struct). Thin AFF_FLAGGED wrapper whose
// distinct name marks every such site: once mobs carry real affect structs, grep IsAffectedFlagOnly
// and replace each occurrence with IsAffected.
inline bool IsAffectedFlagOnly(const CharData *ch, const EAffect flag) {
	return AFF_FLAGGED(ch, flag);
}

inline bool AFF_FLAGGED(const CharData::shared_ptr &ch, const EAffect flag) {
	return AFF_FLAGS(ch).get(flag);
//		|| ch->isAffected(flag); //обойдемся без морфа
}

bool AWAKE(const CharData *ch);
inline bool AWAKE(const CharData::shared_ptr &ch) { return AWAKE(ch.get()); }

bool IsMale(const CharData *ch);
inline bool IsMale(const CharData::shared_ptr &ch) { return IsMale(ch.get()); }
bool IsFemale(const CharData *ch);
inline bool IsFemale(const CharData::shared_ptr &ch) { return IsFemale(ch.get()); }
bool IsPoly(const CharData *ch);

int GetRealBaseStat(const CharData *ch, EBaseStat stat_id);

int ClampBaseStat(const CharData *ch, EBaseStat stat_id, int stat_value);

inline int ClampBaseStat(const CharData::shared_ptr &ch, const EBaseStat stat_id, const int val) {
	return ClampBaseStat(ch.get(), stat_id, val);
}

inline auto GetRealStr(const CharData *ch) {
	return ClampBaseStat(ch, EBaseStat::kStr, ch->get_str() + ch->get_str_add());
};
inline auto GetRealDex(const CharData *ch) {
	return ClampBaseStat(ch, EBaseStat::kDex, ch->get_dex() + ch->get_dex_add());
}
inline auto GetRealCon(const CharData *ch) {
	return ClampBaseStat(ch, EBaseStat::kCon, ch->get_con() + ch->get_con_add());
};
inline auto GetRealWis(const CharData *ch) {
	return ClampBaseStat(ch, EBaseStat::kWis, ch->get_wis() + ch->get_wis_add());
};
inline auto GetRealInt(const CharData *ch) {
	return ClampBaseStat(ch, EBaseStat::kInt, ch->get_int() + ch->get_int_add());
};
inline auto GetRealCha(const CharData *ch) {
	return ClampBaseStat(ch, EBaseStat::kCha, ch->get_cha() + ch->get_cha_add());
};

inline auto GetSave(CharData *ch, ESaving save) {
	return ch->add_abils.apply_saving_throw[to_underlying(save)];
}

inline void SetSave(CharData *ch, ESaving save, int mod) {
	ch->add_abils.apply_saving_throw[to_underlying(save)] = mod;
}

/*
 *  Это все, разумеется, безобразно. Уровни-реморты должны возвращаться какие есть, а всякие таблицы принимать любой
 *  уровень и возвращать вращумтельное значение. Но имеем, что имеем, потому пока так.
 */
int GetRealLevel(const CharData *ch);
int GetRealLevel(const std::shared_ptr<CharData> &ch);

#endif // CHAR_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
