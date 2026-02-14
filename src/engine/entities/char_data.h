// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_HPP_INCLUDED
#define CHAR_HPP_INCLUDED

#include "player_i.h"
#include "administration/punishments.h"
#include "gameplay/mechanics/obj_sets.h"
#include "gameplay/mechanics/dead_load.h"
#include "engine/db/db.h"
#include "entities_constants.h"
#include "room_data.h"
#include "gameplay/communication/ignores.h"
#include "gameplay/crafting/im.h"
#include "gameplay/skills/skills.h"
#include "gameplay/skills/townportal.h"
#include "utils/utils.h"
#include "engine/core/conf.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/mem_queue.h"
#include "gameplay/ai/mob_memory.h"
#include "engine/network/logon.h"
#include "gameplay/statistics/char_stat.h"

#include <unordered_map>
#include <bitset>
#include <list>
#include <map>

// pernalty types
enum { P_DAMROLL, P_HITROLL, P_CAST, P_MEM_GAIN, P_MOVE_GAIN, P_HIT_GAIN, P_AC };

enum { DRUNK, FULL, THIRST };

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
	ubyte Kin;
	ubyte Race;        // PC / NPC's race
};

struct TemporarySpell {
	ESpell spell{ESpell::kUndefined};
	time_t set_time{0};
	time_t duration{0};
};
// кол-во +слотов со шмоток
const int MAX_ADD_SLOTS = 10;

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
	FlagData affected_by;
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

// Punishments structs
  	punishments::Punish pmute;
  	punishments::Punish pdumb;
  	punishments::Punish phell;
  	punishments::Punish pname;
  	punishments::Punish pfreeze;
  	punishments::Punish pgcurse;
  	punishments::Punish punreg;

	char *clanStatus; // строка для отображения приписки по кто
	// TODO: однозначно переписать
	std::shared_ptr<class Clan> clan; // собсна клан, если он есть
	std::shared_ptr<class ClanMember> clan_member; // поле мембера в клане

	static player_special_data::shared_ptr s_for_mobiles;
};

struct attacker_node {
	attacker_node() : damage(0), rounds(0) {};
	// влитый дамаг
	int damage;
	// кол-во раундов в бою
	int rounds;
};

enum {
	ATTACKER_DAMAGE,
	ATTACKER_ROUNDS
};

struct CharacterSkillDataType {
	int skillLevel;
	unsigned cooldown;

	void decreaseCooldown(unsigned value);
};

using CharSkillsType = std::map<ESkill, CharacterSkillDataType>;

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

	void SetFeat(EFeat feat_id) { real_abils.Feats.set(to_underlying(feat_id)); };
	void UnsetFeat(EFeat feat_id) { real_abils.Feats.reset(to_underlying(feat_id)); };
	bool HaveFeat(EFeat feat_id) const { return real_abils.Feats.test(to_underlying(feat_id)); };

	void set_skill(ESkill skill_id, int percent);
	void SetSkillAfterRemort();
	void clear_skills();
	int GetSkill(ESkill skill_id) const;
	int GetSkillWithoutEquip(ESkill skill_id) const;
	int get_skills_count() const;
	int GetEquippedSkill(ESkill skill_id) const;
	int get_skill_bonus() const;
	void set_skill_bonus(int);
	int GetAddSkill(ESkill skill_id) const;
	void SetAddSkill(ESkill skill_id, int value);

	int get_obj_slot(int slot_num);
	void add_obj_slot(int slot_num, int count);
	// ресет нпс
	void restore_npc();
	////////////////////////////////////////////////////////////////////////////
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

	// TODO: касты можно сделать и красивее (+ troom не используется, CastSpell/cast_subst/cast_obj только по разу)
	void SetCast(ESpell spell_id, ESpell spell_subst, CharData *tch, ObjData *tobj, RoomData *troom);
	ESpell GetCastSpell() const;
	ESpell GetCastSubst() const;
	CharData *GetCastChar() const;
	ObjData *GetCastObj() const;

	////////////////////////////////////////////////////////////////////////////

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
	ubyte get_kin() const;
	void set_kin(ubyte);
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

	////////////////////////////////////////////////////////////////////////////
	long get_gold() const;
	long get_bank() const;
	long get_total_gold() const;

	void add_gold(long gold, bool log = true, bool clan_tax = false);
	void add_bank(long gold, bool log = true);

	void set_gold(long num, bool log = true);
	void set_bank(long num, bool log = true);

	long remove_gold(long num, bool log = true);
	long remove_bank(long num, bool log = true);
	long remove_both_gold(long num, bool log = true);
	////////////////////////////////////////////////////////////////////////////

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

	////////////////////////////////////////////////////////////////////////////
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
	////////////////////////////////////////////////////////////////////////////

	/**
	 * \return поле group из зон файла данного моба
	 * <= 1 - обычная зона (соло), >= 2 - групповая зона на указанное кол-во человек
	 */
	int get_zone_group() const;

	/**
	** Returns true if character is mob and located in used zone.
	**/
	bool in_used_zone() const;

	/**
	 * Возвращает коэффициент штрафа за состояние
	**/
	float get_cond_penalty(int type) const;
	std::string GetTitle() const;
	std::string get_pretitle() const;
	std::string get_race_name() const;
	std::string GetTitleAndName();
	std::string GetNameWithTitleOrRace();
	std::string race_or_title();
	int GetSkillBonus(const ESkill skill_id) const;
	int GetTrainedSkill(ESkill skill_num) const;
	bool isAffected(EAffect flag) const;

	void set_who_mana(unsigned int);
	void set_who_last(time_t);
	unsigned int get_who_mana();
	time_t get_who_last();

	/// роли (mob only)
	bool get_role(unsigned num) const;
	void set_role(unsigned num, bool flag);
	const CharData::role_t &get_role_bits() const;

	void add_attacker(CharData *ch, unsigned type, int num);
	int get_attacker(CharData *ch, unsigned type) const;
	std::pair<int /* uid */, int /* rounds */> get_max_damager_in_room() const;

	void inc_restore_timer(int num);
	obj_sets::activ_sum &obj_bonus();

	void set_ruble(int ruble);
	long get_ruble();

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
	void setSkillCooldown(ESkill skillID, unsigned cooldown);
	void decreaseSkillsCooldowns(unsigned value);
	bool haveSkillCooldown(ESkill skillID);
	bool HasCooldown(ESkill skillID);
	bool HaveDecreaseCooldowns();
	int getSkillCooldownInPulses(ESkill skillID);
	void ZeroCooldowns();
	unsigned getSkillCooldown(ESkill skillID);

	void reset() override;

	void set_abstinent();
	char_affects_list_t::iterator AffectRemove(const char_affects_list_t::iterator &affect_i);
	bool has_any_affect(const affects_list_t &affects);
	size_t remove_random_affects(size_t count);

	const role_t &get_role() const { return role_; }
	void set_role(const role_t &new_role) { role_ = new_role; }
	void msdp_report(const std::string &name);

	void removeGroupFlags();
	void add_follower(CharData *ch);
	/** Do NOT call this before having checked if a circle of followers
	* will arise. CH will follow leader
	* \param silent - для смены лидера группы без лишнего спама (по дефолту 0)
	*/
	void add_follower_silently(CharData *ch);
	followers_list_t get_followers_list() const;
	const player_special_data::ignores_t &get_ignores() const;
	void add_ignore(const ignore_data::shared_ptr& ignore);
	void clear_ignores();

	template<typename T>
	void set_affect(T affect) { char_specials.saved.affected_by.set(affect); }
	template<typename T>
	void remove_affect(T affect) { char_specials.saved.affected_by.unset(affect); }
	bool low_charm() const;

	void set_purged(const bool _ = true) {
		purged_ = _;
		script->set_purged(_);
	}

	void cleanup_script();

	bool IsNpc() const { return is_npc_; }
	void SetNpcAttribute(bool _) { is_npc_ = _; }
	bool IsPlayer() const { return !IsNpc(); }
	bool have_mind() const;
	bool HasWeapon();

 private:
	const auto &get_player_specials() const { return player_specials; }
	auto &get_player_specials() { return player_specials; }

	std::string GetClanTitleAddition();
	std::string GetTitleAndNameWithoutClan() const;
	void zero_init();
	void restore_mob();

	void purge();

	CharSkillsType skills;    // список изученных скиллов
	////////////////////////////////////////////////////////////////////////////
	CharData *protecting_{nullptr}; // цель для 'прикрыть'
	CharData *touching_;   // цель для 'перехватить'
	CharData *enemy_;

	struct extra_attack_type extra_attack_; // атаки типа баша, пинка и т.п.
	struct CastAttack cast_attack_;   // каст заклинания
	////////////////////////////////////////////////////////////////////////////
	// true - чар очищен и ждет вызова delete для оболочки
	bool purged_;

// * TODO: пока сюда скидывается, чтобы поля private были
	// имя чара или алиасы моба
	std::string name_;
	// имя моба (им.падеж)
	std::string short_descr_;
	// профессия чара/класс моба
	ECharClass chclass_;
	bool is_npc_;
	// уровень
	int level_;
	// плюс на уровень
	int level_add_;
	// id чара (не тот, что для тригов), у мобов -1
	long uid_;
	// экспа
	long exp_;
	// реморты
	int remorts_;
	// плюсы на реморт
	int remorts_add_;
	// время последнего входа в игру //by kilnik
	time_t last_logon_;
	// последний вызов базара
	time_t last_exchange_;
	// деньги на руках
	long gold_;
	// деньги в банке
	long bank_gold_;
	// рубли
	long ruble;
	// родная сила
	int str_;
	// плюсы на силу
	int str_add_;
	// родная ловкость
	int dex_;
	// плюсы на ловкость
	int dex_add_;
	// родное тело
	int con_;
	// плюсы на тело
	int con_add_;
	// родная мудра
	int wis_;
	// плюсы на мудру
	int wis_add_;
	// родная инта
	int int_;
	// плюсы на инту
	int int_add_;
	// родная харизма
	int cha_;
	// плюсы на харизму
	int cha_add_;
	// аналог класса у моба
	role_t role_;
	// для боссов: список атакующих (и им сочувствующих), uid->attacker
	std::unordered_map<int, attacker_node> attackers_;
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
	void report_loop_error(CharData::ptr_t master) const;
	void print_leaders_chain(std::ostream &ss) const;

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
  	void PrintFlagsToAscii(char *sink) const { char_specials.saved.act.tascii(FlagData::kPlanesNumber, sink); };
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
  	void ClearRunestones() { player_specials->runestones.Clear(); };
  	void AddRunestone(const Runestone &stone);
  	void RemoveRunestone(const Runestone &stone);
  	void DeleteIrrelevantRunestones() { player_specials->runestones.DeleteIrrelevant(this); };
	void PageRunestonesToChar() { player_specials->runestones.PageToChar(this); };
  	bool IsRunestoneKnown(const Runestone &stone) { return player_specials->runestones.Contains(stone); };
  	void ClearStatistics() { player_specials->saved.personal_statistics_.Clear(); };
  	void ClearThisRemortStatistics() { player_specials->saved.personal_statistics_.ClearThisRemort(); };
  	void IncreaseStatistic(CharStat::ECategory category, ullong increment);
  	void ClearStatisticElement(CharStat::ECategory category);
  	ullong GetStatistic(CharStat::ECategory category) const;

  	void SetFlag(const EPrf flag) { if (!IsNpc()) { player_specials->saved.pref.set(flag); }; };
  	void UnsetFlag(const EPrf flag) { if (!IsNpc()) { player_specials->saved.pref.unset(flag); }; };
  	[[nodiscard]] bool IsFlagged(const EPrf flag) const { return (!IsNpc() && player_specials->saved.pref.get(flag)); };

	int GetCarryingWeight() const { return char_specials.carry_weight; };
  	int GetCarryingQuantity() const { return char_specials.carry_items; };

	char_affects_list_t affected;    // affected by what spells
	struct TimedSkill *timed;    // use which timed skill/spells
	struct TimedFeat *timed_feat;    // use which timed feats
	ObjData *equipment[EEquipPos::kNumEquipPos];    // Equipment array

	ObjData *carrying;    // Head of list
	DescriptorData *desc;    // NULL for mobiles
	ObjData::triggers_list_ptr proto_script;    // list of default triggers
	Script::shared_ptr script;    // script info for the object

	//отладочные сообщения имморталу/тестеру/кодеру
	void send_to_TC(bool to_impl, bool to_tester, bool to_coder, const char *msg, ...);

	// очередь изучаемых заклинаний
	SpellMemQueue mem_queue;

	int caster_level;
	int damage_level;
	struct PK_Memory_type *pk_list;

	int track_dirs;
	bool check_aggressive;
	int extract_timer;

	FlagData Temporary;

	int initiative;
	int battle_counter;
	int round_counter;

	FlagData battle_affects;

	int poisoner;

	dead_load::OnDeadLoadList dl_list;    // загружаемые в труп предметы
	bool agrobd;        // показывает, агробд или нет

	std::map<ESpell, TemporarySpell> temp_spells;
	std::list<MobVnum> summon_helpers;
	std::vector<int> kill_list; //used only for MTRIG_KILL
 public:
	// FOLLOWERS
	struct FollowerType *followers;
	CharData::ptr_t get_master() const { return m_master; }
	void set_master(CharData::ptr_t master);
	bool has_master() const { return nullptr != m_master; }
	bool makes_loop(CharData::ptr_t master) const;
	// MOUNTS
	bool IsOnHorse() const;
	bool has_horse(bool same_room) const;
	CharData *get_horse();
	bool DropFromHorse();
	bool IsHorsePrevents();
	void dismount();
	bool IsLeader();
};

# define MAX_FIRSTAID_REMOVE 17
inline ESpell GetRemovableSpellId(int num) {
	static const ESpell spell[MAX_FIRSTAID_REMOVE] = {ESpell::kAbstinent, ESpell::kPoison, ESpell::kMadness,
		ESpell::kWeaknes, ESpell::kSlowdown, ESpell::kMindless, ESpell::kColdWind,
		ESpell::kFever, ESpell::kCurse, ESpell::kDeafness, ESpell::kSilence,
		ESpell::kBlindness, ESpell::kSleep, ESpell::kHold, ESpell::kVacuum, ESpell::kHaemorrhage, ESpell::kBattle};
	if (num < MAX_FIRSTAID_REMOVE) {
		return spell[num];
	} else {
		return ESpell::kUndefined;
	}
}
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

inline void SetWaitState(CharData *ch, const unsigned cycle) {
	if (ch->get_wait() < cycle) {
		ch->set_wait(cycle);
	}
}

inline FlagData &AFF_FLAGS(CharData *ch) { return ch->char_specials.saved.affected_by; }
inline const FlagData &AFF_FLAGS(const CharData *ch) { return ch->char_specials.saved.affected_by; }
inline const FlagData &AFF_FLAGS(const CharData::shared_ptr &ch) { return ch->char_specials.saved.affected_by; }

inline bool AFF_FLAGGED(const CharData *ch, const EAffect flag) {
	return AFF_FLAGS(ch).get(flag);
}

inline bool AFF_FLAGGED(const CharData::shared_ptr &ch, const EAffect flag) {
	return AFF_FLAGS(ch).get(flag);
//		|| ch->isAffected(flag); //обойдемся без морфа
}

bool IS_CHARMICE(const CharData *ch);
inline bool IS_CHARMICE(const CharData::shared_ptr &ch) { return IS_CHARMICE(ch.get()); }

inline bool IS_FLY(const CharData *ch) {
	return AFF_FLAGGED(ch, EAffect::kFly);
}

inline bool INVIS_OK(const CharData *sub, const CharData *obj) {
	return !AFF_FLAGGED(sub, EAffect::kBlind)
		&& ((!AFF_FLAGGED(obj, EAffect::kInvisible)
			|| AFF_FLAGGED(sub, EAffect::kDetectInvisible))
			&& ((!AFF_FLAGGED(obj, EAffect::kHide)
				&& !AFF_FLAGGED(obj, EAffect::kDisguise))
				|| AFF_FLAGGED(sub, EAffect::kDetectLife)));
}

inline bool INVIS_OK(const CharData *sub, const CharData::shared_ptr &obj) { return INVIS_OK(sub, obj.get()); }

bool MORT_CAN_SEE(const CharData *sub, const CharData *obj);
bool IMM_CAN_SEE(const CharData *sub, const CharData *obj);

inline bool SELF(const CharData *sub, const CharData *obj) { return sub == obj; }
inline bool SELF(const CharData *sub, const CharData::shared_ptr &obj) { return sub == obj.get(); }

/// Can subject see character "obj"?
bool CAN_SEE(const CharData *sub, const CharData *obj);
inline bool CAN_SEE(const CharData *sub, const CharData::shared_ptr &obj) { return CAN_SEE(sub, obj.get()); }
inline bool CAN_SEE(const CharData::shared_ptr &sub, const CharData *obj) { return CAN_SEE(sub.get(), obj); }
inline bool CAN_SEE(const CharData::shared_ptr &sub, const CharData::shared_ptr &obj) {
	return CAN_SEE(sub.get(),
				   obj.get());
}

bool MAY_SEE(const CharData *ch, const CharData *sub, const CharData *obj);

bool IS_HORSE(const CharData *ch);
inline bool IS_HORSE(const CharData::shared_ptr &ch) { return IS_HORSE(ch.get()); }
bool IS_MORTIFIER(const CharData *ch);

bool MAY_ATTACK(const CharData *sub);
inline bool MAY_ATTACK(const CharData::shared_ptr &sub) { return MAY_ATTACK(sub.get()); }

bool AWAKE(const CharData *ch);
inline bool AWAKE(const CharData::shared_ptr &ch) { return AWAKE(ch.get()); }

bool CLEAR_MIND(const CharData *ch);
inline bool CLEAR_MIND(const CharData::shared_ptr &ch) { return CLEAR_MIND(ch.get()); }

inline bool CAN_START_MTRIG(const CharData *ch) {
	return !AFF_FLAGGED(ch, EAffect::kCharmed);
}

bool OK_GAIN_EXP(const CharData *ch, const CharData *victim);

bool IS_MALE(const CharData *ch);
inline bool IS_MALE(const CharData::shared_ptr &ch) { return IS_MALE(ch.get()); }
bool IS_FEMALE(const CharData *ch);
inline bool IS_FEMALE(const CharData::shared_ptr &ch) { return IS_FEMALE(ch.get()); }
bool IS_NOSEXY(const CharData *ch);
inline bool IS_NOSEXY(const CharData::shared_ptr &ch) { return IS_NOSEXY(ch.get()); }
bool IS_POLY(const CharData *ch);

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

inline bool IS_UNDEAD(CharData *ch) {
	return ch->IsNpc()
			&& (ch->IsFlagged(EMobFlag::kResurrected)
					|| GET_RACE(ch) == ENpcRace::kZombie
					|| GET_RACE(ch) == ENpcRace::kGhost);
}

void change_fighting(CharData *ch, int need_stop);

/*
 *  Это все, разумеется, безобразно. Уровни-реморты должны возвращаться какие есть, а всякие таблицы принимать любой
 *  уровень и возвращать вращумтельное значение. Но имеем, что имеем, потому пока так.
 */
int GetRealLevel(const CharData *ch);
int GetRealLevel(const std::shared_ptr<CharData> &ch);
int GetRealRemort(const CharData *ch);
int GetRealRemort(const std::shared_ptr<CharData> &ch);

#endif // CHAR_HPP_INCLUDED


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
