// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_HPP_INCLUDED
#define CHAR_HPP_INCLUDED

#include "player_i.hpp"
#include "morph.hpp"
#include "obj_sets.hpp"
#include "db.h"
#include "room.hpp"
#include "ignores.hpp"
#include "im.h"
#include "skills.h"
#include "utils.h"
#include "structs.h"
#include "conf.h"

#include <boost/dynamic_bitset.hpp>

#include <unordered_map>
#include <bitset>
#include <list>
#include <map>

// These data contain information about a players time data

struct time_data
{
	time_t birth;		// This represents the characters age
	time_t logon;		// Time of the last logon (used to calculate played)
	int played;		// This is the total accumulated time played in secs
};

// general player-related info, usually PC's and NPC's
struct char_player_data
{
	std::string long_descr;	// for 'look'
	std::string description;	// Extra descriptions
	std::string title;		// PC / NPC's title
	ESex sex;		// PC / NPC's sex
	struct time_data time;			// PC's AGE in days
	ubyte weight;		// PC / NPC's weight
	ubyte height;		// PC / NPC's height

	std::array<std::string, 6> PNames;
	ubyte Religion;
	ubyte Kin;
	ubyte Race;		// PC / NPC's race
};

struct temporary_spell_data
{
	int spell;
	time_t set_time;
	time_t duration;
};
// кол-во +слотов со шмоток
const int MAX_ADD_SLOTS = 10;
// типы резистов
enum { FIRE_RESISTANCE = 0, AIR_RESISTANCE, WATER_RESISTANCE, EARTH_RESISTANCE, VITALITY_RESISTANCE, MIND_RESISTANCE, IMMUNITY_RESISTANCE, DARK_RESISTANCE, MAX_NUMBER_RESISTANCE };

// Char's additional abilities. Used only while work
struct char_played_ability_data
{
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
	std::array<sbyte, MAX_ADD_SLOTS> obj_slot;
	int armour;
	int ac_add;
	int hr_add;
	int dr_add;
	int absorb;
	int morale_add;
	int cast_success;
	int initiative_add;
	int poison_add;
	int pray_add;
	std::array<sh_int, 4> apply_saving_throw;		// Saving throw (Bonuses)
	std::array<sh_int, MAX_NUMBER_RESISTANCE> apply_resistance_throw;	// Сопротивление (резисты) к магии, ядам и крит. ударам
	ubyte mresist;
	ubyte aresist;
	ubyte presist;	// added by WorM(Видолюб) по просьбе <сумасшедшего> (зачеркнуто) безбашенного билдера поглощение физ.урона в %
	
};

// Char's abilities.
struct char_ability_data
{
	std::array<ubyte, MAX_SPELLS + 1> SplKnw; // array of SPELL_KNOW_TYPE
	std::array<ubyte, MAX_SPELLS + 1> SplMem; // array of MEMed SPELLS
	bitset<MAX_FEATS> Feats;
	sbyte size;
	sbyte hitroll;
	int damroll;
	short armor;
};

// Char's points.
struct char_point_data
{
	int hit;	
	sh_int move;
	
	sh_int max_move;	// Max move for PC/NPC
	int max_hit;		// Max hit for PC/NPC
};

/*
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved
{
	int alignment;		// +-1000 for alignments
	FLAG_DATA act;		// act flag for NPC's; player flag for PC's
	FLAG_DATA affected_by;
	// Bitvector for spells/skills affected by
};

// Special playing constants shared by PCs and NPCs which aren't in pfile
struct char_special_data
{
	byte position;		// Standing, fighting, sleeping, etc.

	int carry_weight;		// Carried weight
	int carry_items;		// Number of items carried
	int timer;			// Timer for update
	time_t who_last; // таймстамп последнего использования команды кто

	struct char_special_data_saved saved;			// constants saved in plrfile
};

// Specials used by NPCs, not PCs
struct mob_special_data
{
	byte last_direction;	// The last direction the monster went
	int attack_type;		// The Attack Type Bitvector for NPC's
	byte default_pos;	// Default position for NPC
	memory_rec *memory;	// List of attackers to remember
	byte damnodice;		// The number of damage dice's
	byte damsizedice;	// The size of the damage dice's
	std::array<int, MAX_DEST> dest;
	int dest_dir;
	int dest_pos;
	int dest_count;
	int activity;
	FLAG_DATA npc_flags;
	byte ExtraAttack;
	byte LikeWork;
	byte MaxFactor;
	int GoldNoDs;
	int GoldSiDs;
	int HorseState;
	int LastRoom;
	char *Questor;
	int speed;
	int hire_price;// added by WorM (Видолюб) 2010.06.04 Цена найма чармиса
	int capable_spell;// added by WorM (Видолюб) Закл в мобе
};

// очередь запоминания заклинаний
struct spell_mem_queue
{
	struct spell_mem_queue_item *queue;
	int stored;		// накоплено манны
	int total;			// полное время мема всей очереди
};

// Structure used for extra_attack - bash, kick, diasrm, chopoff, etc
struct extra_attack_type
{
	ExtraAttackEnumType used_attack;
	CHAR_DATA *victim;
};

struct cast_attack_type
{
	int spellnum;
	int spell_subst;
	CHAR_DATA *tch;
	OBJ_DATA *tobj;
	ROOM_DATA *troom;
};

struct player_special_data_saved
{
	player_special_data_saved();

	int wimp_level;		// Below this # of hit points, flee!
	int invis_level;		// level of invisibility
	room_vnum load_room;	// Which room to place char in
	FLAG_DATA pref;		// preference flags for PC's.
	int bad_pws;		// number of bad password attemps
	std::array<int, 3> conditions;		// Drunk, full, thirsty

	int DrunkState;
	int olc_zone;
	int NameGod;
	long NameIDGod;
	long GodsLike;

	char EMail[128];
	char LastIP[128];

	int stringLength;
	int stringWidth;

	// 29.11.09 переменные для подсчета количества рипов (с) Василиса
	int Rip_arena; //рипы на арене
	int Rip_mob; // рипы от мобов всего
	int Rip_pk; // рипы от чаров всего
	int Rip_dt; // дт всего
	int Rip_other; // рипы от триггеров и прочее всего
	int Win_arena; //убито игроком на арене
	int Rip_mob_this; // рипы от мобов на этом морте
	int Rip_pk_this; // рипы от чаров на этом морте
	int Rip_dt_this; // дт на этом морте
	int Rip_other_this; // рипы от триггеров и прочее на этом морте
	//edited by WorM переделал на unsigned long long чтоб экспа в минуса не уходила
	unsigned long long Exp_arena; //потеряно экспы за рипы на арене
	unsigned long long Exp_mob; //потеряно экспы  рипы от мобов всего
	unsigned long long Exp_pk; //потеряно экспы  рипы от чаров всего
	unsigned long long Exp_dt; //потеряно экспы  дт всего
	unsigned long long Exp_other; //потеряно экспы  рипы от триггеров и прочее всего
	unsigned long long Exp_mob_this; //потеряно экспы  рипы от мобов на этом морте
	unsigned long long Exp_pk_this; //потеряно экспы  рипы от чаров на этом морте
	unsigned long long Exp_dt_this; //потеряно экспы  дт на этом морте
	unsigned long long Exp_other_this; //потеряно экспы  рипы от триггеров и прочее на этом морте
	//конец правки (с) Василиса
	//Polud храним цену, начиная с которой нужно присылать оффлайн-уведомления с базара
	long ntfyExchangePrice;
	int HiredCost;// added by WorM (Видолюб) 2010.06.04 сумма потраченная на найм(возвращается при креше)
	unsigned int who_mana; // количество энергии для использования команды кто
};


struct player_special_data
{
	using shared_ptr = std::shared_ptr<player_special_data>;
	using ignores_t = std::list<ignore_data::shared_ptr>;

	player_special_data();

	player_special_data_saved saved;

	char *poofin;		// Description on arrival of a god.
	char *poofout;		// Description upon a god's exit.
	struct alias_data *aliases;	// Character's aliases
	time_t may_rent;		// PK control
	int agressor;		// Agression room(it is also a flag)
	time_t agro_time;		// Last agression time (it is also a flag)
	im_rskill *rskill;	// Известные рецепты
	struct char_portal_type *portals;	// порталы теперь живут тут
	int *logs;		// уровни подробности каналов log

	char *Exchange_filter;
	ignores_t ignores;
	char *Karma; // Записи о поощрениях, наказаниях персонажа

	std::vector<logon_data> logons; //Записи о входах чара

// Punishments structs
	punish_data pmute;
	punish_data pdumb;
	punish_data phell;
	punish_data pname;
	punish_data pfreeze;
	punish_data pgcurse;
	punish_data punreg;

	char *clanStatus; // строка для отображения приписки по кто
	// TODO: однозначно переписать
	std::shared_ptr<class Clan> clan; // собсна клан, если он есть
	std::shared_ptr<class ClanMember> clan_member; // поле мембера в клане

	static player_special_data::shared_ptr s_for_mobiles;
};

enum
{
	MOB_ROLE_BOSS,
	MOB_ROLE_MINION,
	MOB_ROLE_TANK,
	MOB_ROLE_MELEE_DMG,
	MOB_ROLE_ARCHER,
	MOB_ROLE_ROGUE,
	MOB_ROLE_MAGE_DMG,
	MOB_ROLE_MAGE_BUFF,
	MOB_ROLE_HEALER,
	MOB_ROLE_TOTAL_NUM
};

struct attacker_node
{
	attacker_node() : damage(0), rounds(0) {};
	// влитый дамаг
	int damage;
	// кол-во раундов в бою
	int rounds;
};

enum
{
	ATTACKER_DAMAGE,
	ATTACKER_ROUNDS
};

typedef std::map<ESkill/* номер скилла */, int/* значение скилла */> CharSkillsType;
//typedef __gnu_cxx::hash_map < int/* номер скилла */, int/* значение скилла */ > CharSkillsType;

class ProtectedCharacterData;	// to break up cyclic dependencies

class CharacterRNum_ChangeObserver
{
public:
	using shared_ptr = std::shared_ptr<CharacterRNum_ChangeObserver>;

	CharacterRNum_ChangeObserver() {}
	virtual ~CharacterRNum_ChangeObserver() {}

	virtual void notify(ProtectedCharacterData& character, const mob_rnum old_rnum) = 0;
};

class ProtectedCharacterData : public PlayerI
{
public:
	ProtectedCharacterData();
	ProtectedCharacterData(const ProtectedCharacterData& rhv);
	ProtectedCharacterData& operator=(const ProtectedCharacterData& rhv);

	auto get_rnum() const { return m_rnum; }
	void set_rnum(const mob_rnum rnum);

	void subscribe_for_rnum_changes(const CharacterRNum_ChangeObserver::shared_ptr& observer) { m_rnum_change_observers.insert(observer); }
	void unsubscribe_from_rnum_changes(const CharacterRNum_ChangeObserver::shared_ptr& observer) { m_rnum_change_observers.erase(observer); }

private:
	mob_rnum m_rnum;		// Mob's rnum

	std::unordered_set<CharacterRNum_ChangeObserver::shared_ptr> m_rnum_change_observers;
};

// * Общий класс для игроков/мобов.
class CHAR_DATA : public ProtectedCharacterData
{
// новое
public:
	using ptr_t = CHAR_DATA*;
	using shared_ptr = std::shared_ptr<CHAR_DATA>;
	using char_affects_list_t = std::list<AFFECT_DATA<EApplyLocation>::shared_ptr>;
	using morphs_list_t = std::list<std::string>;
	using role_t = boost::dynamic_bitset<std::size_t>;
	using followers_list_t = std::list<CHAR_DATA*>;

	CHAR_DATA();
	virtual ~CHAR_DATA();

	friend void do_mtransform(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
	friend void medit_mobile_copy(CHAR_DATA * dst, CHAR_DATA * src);

	void set_skill(const ESkill skill_num, int percent);
	void set_skill(short remort);
	void clear_skills();
	int get_skill(const ESkill skill_num) const;
	int get_skills_count() const;
	void crop_skills();
	int get_equipped_skill(const ESkill skill_num) const;
	int get_trained_skill(const ESkill skill_num) const;

	int get_obj_slot(int slot_num);
	void add_obj_slot(int slot_num, int count);

	////////////////////////////////////////////////////////////////////////////
	CHAR_DATA * get_touching() const;
	void set_touching(CHAR_DATA *vict);

	CHAR_DATA * get_protecting() const;
	void set_protecting(CHAR_DATA *vict);

	ExtraAttackEnumType get_extra_attack_mode() const;
	CHAR_DATA * get_extra_victim() const;
	void set_extra_attack(ExtraAttackEnumType Attack, CHAR_DATA *vict);

	CHAR_DATA * get_fighting() const;
	void set_fighting(CHAR_DATA *vict);

	// TODO: касты можно сделать и красивее (+ troom не используется, cast_spell/cast_subst/cast_obj только по разу)
	void set_cast(int spellnum, int spell_subst, CHAR_DATA *tch, OBJ_DATA *tobj, ROOM_DATA *troom);
	int get_cast_spell() const;
	int get_cast_subst() const;
	CHAR_DATA * get_cast_char() const;
	OBJ_DATA * get_cast_obj() const;

	////////////////////////////////////////////////////////////////////////////

	int get_serial_num();
	void set_serial_num(int num);

	bool purged() const;

	const std::string& get_name() const;
	void set_name(const char *name);
	const std::string& get_pc_name() const { return name_; }
	void set_pc_name(const char *name);
	void set_pc_name(const std::string& name) {name_ = name; }
	void set_pc_name(std::string&& name) {name_ = name; }
	const std::string& get_npc_name() const { return short_descr_; }
	void set_npc_name(const char *name);
	void set_npc_name(const std::string& name) {short_descr_ = name; }
	void set_npc_name(std::string&& name) {short_descr_ = name; }
	const std::string & get_name_str() const;
	const char* get_pad(unsigned pad) const;
	void set_pad(unsigned pad, const char* s);
	const char* get_long_descr() const;
	void set_long_descr(const char*);
	const char* get_description() const;
	void set_description(const char*);
	short get_class() const;
	void set_class(short chclass);

	bool is_druid() const { return chclass_ == CLASS_DRUID; }

	short get_level() const;
	void set_level(short level);

	long get_idnum() const;
	void set_idnum(long idnum);

	int get_uid() const;
	void set_uid(int uid);

	long get_exp() const;
	void set_exp(long exp);

	short get_remort() const;
	void set_remort(short num);

	time_t get_last_logon() const;
	void set_last_logon(time_t num);

	time_t get_last_exchange() const;
	void set_last_exchange(time_t num);

	ESex get_sex() const;
	void set_sex(const ESex sex);
	ubyte get_weight() const;
	void set_weight(const ubyte );
	ubyte get_height() const;
	void set_height(const ubyte );
	ubyte get_religion() const;
	void set_religion(const ubyte );
	ubyte get_kin() const;
	void set_kin(const ubyte );
	ubyte get_race() const;
	void set_race(const ubyte );
	int get_hit() const;
	void set_hit(const int );
	int get_max_hit() const;
	void set_max_hit(const int );
	sh_int get_move() const;
	void set_move(const sh_int );
	sh_int get_max_move() const;
	void set_max_move(const sh_int );

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
	int get_inborn_str() const;
	void set_str(int);
	void inc_str(int);
	int get_dex() const;
	int get_inborn_dex() const;
	void set_dex(int);
	void inc_dex(int);
	int get_con() const;
	int get_inborn_con() const;
	void set_con(int);
	void inc_con(int);
	int get_wis() const;
	int get_inborn_wis() const;
	void set_wis(int);
	void inc_wis(int);
	int get_int() const;
	int get_inborn_int() const;
	void set_int(int);
	void inc_int(int);
	int get_cha() const;
	int get_inborn_cha() const;
	void set_cha(int);
	void inc_cha(int);

	////////////////////////////////////////////////////////////////////////////
	void clear_add_affects();
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

	bool know_morph(const std::string& morph_id) const;
	void add_morph(const std::string& morph_id);
	void clear_morphs();
	void set_morph(MorphPtr morph);
	void reset_morph();
	size_t get_morphs_count() const;
	const morphs_list_t& get_morphs();
	bool is_morphed() const;
	void set_normal_morph();

	std::string get_title();
	std::string get_morphed_name() const;
	std::string get_pretitle();
	std::string get_race_name();
	std::string only_title();
	std::string noclan_title();
	std::string race_or_title();
	std::string get_morphed_title() const;
	std::string get_cover_desc();
	std::string get_morph_desc() const;
	int get_inborn_skill(const ESkill skill_num);
	void set_morphed_skill(const ESkill skill_num, int percent);
	bool isAffected(const EAffectFlag flag) const;
	const IMorph::affects_list_t& GetMorphAffects();

	void set_who_mana(unsigned int);
	void set_who_last(time_t);
	unsigned int get_who_mana();
	time_t get_who_last();

	/// роли (mob only)
	bool get_role(unsigned num) const;
	void set_role(unsigned num, bool flag);
	const CHAR_DATA::role_t& get_role_bits() const;

	void add_attacker(CHAR_DATA *ch, unsigned type, int num);
	int get_attacker(CHAR_DATA *ch, unsigned type) const;
	std::pair<int /* uid */, int /* rounds */> get_max_damager_in_room() const;

	void inc_restore_timer(int num);
	obj_sets::activ_sum& obj_bonus();

	void set_ruble(int ruble);
	long get_ruble();

	void set_souls(int souls);
	void inc_souls();
	void dec_souls();
	int get_souls();
	
	unsigned get_wait() const { return m_wait; }
	void set_wait(const unsigned _) { m_wait = _; }
	void wait_dec() { m_wait -= 0 < m_wait ? 1 : 0; }
	void wait_dec(const unsigned value) { m_wait -= value <= m_wait ? value : m_wait; }

	virtual void reset();

	void set_abstinent();
	void affect_remove(const char_affects_list_t::iterator& affect_i);
	bool has_any_affect(const affects_list_t& affects);
	size_t remove_random_affects(const size_t count);

	const auto& get_role() const { return role_; }
	void set_role(const role_t& new_role) { role_ = new_role; }
	void msdp_report(const std::string& name);

	void add_follower(CHAR_DATA* ch);
	/** Do NOT call this before having checked if a circle of followers
	* will arise. CH will follow leader
	* \param silent - для смены лидера группы без лишнего спама (по дефолту 0)
	*/
	void add_follower_silently(CHAR_DATA* ch);
	followers_list_t get_followers_list() const;
	const player_special_data::ignores_t& get_ignores() const;
	void add_ignore(const ignore_data::shared_ptr ignore);
	void clear_ignores();

	template <typename T>
	void set_affect(T affect) { char_specials.saved.affected_by.set(affect); }
	template <typename T>
	void remove_affect(T affect) { char_specials.saved.affected_by.unset(affect); }
	bool low_charm() const;

	void set_purged(const bool _ = true) { purged_ = _; script->set_purged(_); }

	void cleanup_script();

	bool is_npc() const { return char_specials.saved.act.get(MOB_ISNPC); }

private:
	const auto& get_player_specials() const { return player_specials; }
	auto& get_player_specials() { return player_specials; }

	std::string clan_for_title();
	std::string only_title_noclan();
	void zero_init();
	void restore_mob();

	void purge();

	CharSkillsType skills;  // список изученных скиллов
	////////////////////////////////////////////////////////////////////////////
	CHAR_DATA *protecting_; // цель для 'прикрыть'
	CHAR_DATA *touching_;   // цель для 'перехватить'
	CHAR_DATA *fighting_;   // противник

	struct extra_attack_type extra_attack_; // атаки типа баша, пинка и т.п.
	struct cast_attack_type cast_attack_;   // каст заклинания
	////////////////////////////////////////////////////////////////////////////
	int serial_num_; // порядковый номер в списке чаров (для name_list)
	// true - чар очищен и ждет вызова delete для оболочки
	bool purged_;

// * TODO: пока сюда скидывается, чтобы поля private были
	// имя чара или алиасы моба
	std::string name_;
	// имя моба (им.падеж)
	std::string short_descr_;
	// профессия чара/класс моба
	short chclass_;
	// уровень
	short level_;
	// id чара (не тот, что для тригов), у мобов -1
	long idnum_;
	// uid (бывший unique) чара
	int uid_;
	//#
	// экспа
	long exp_;
	// реморты
	short remorts_;
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
	//изученные формы
	morphs_list_t morphs_;
	//текущая форма
	MorphPtr current_morph_;
	// аналог класса у моба
	role_t role_;
	// для боссов: список атакующих (и им сочувствующих), uid->attacker
	std::unordered_map<int, attacker_node> attackers_;
	// для боссов: таймер (в секундах), включающийся по окончанию боя
	// через который происходит сброс списка атакующих и рефреш моба
	int restore_timer_;
	// всякие хитрые бонусы с сетов (здесь, чтобы чармисов не обделить)
	obj_sets::activ_sum obj_bonus_;
	// для режимов
	// количество набранных очков
	int count_score;
	// души, онли чернок
	int souls;

public:
	room_rnum in_room;	// Location (real room number)

private:
	void report_loop_error(const CHAR_DATA::ptr_t master) const;
	void print_leaders_chain(std::ostream& ss) const;

	unsigned m_wait;			// wait for how many loops
	CHAR_DATA* m_master;	// Who is char following?

public:
	int punctual_wait;		// wait for how many loops (punctual style)
	char *last_comm;		// последний приказ чармису перед окончанием лага

	struct char_player_data player_data;		// Normal data
	struct char_played_ability_data add_abils;		// Abilities that add to main
	struct char_ability_data real_abils;		// Abilities without modifiers
	struct char_point_data points;		// Points
	struct char_special_data char_specials;		// PC/NPC specials
	struct mob_special_data mob_specials;		// NPC specials

	player_special_data::shared_ptr player_specials;	// PC specials

	char_affects_list_t affected;	// affected by what spells
	struct timed_type *timed;	// use which timed skill/spells
	struct timed_type *timed_feat;	// use which timed feats
	OBJ_DATA *equipment[NUM_WEARS];	// Equipment array

	OBJ_DATA *carrying;	// Head of list
	DESCRIPTOR_DATA* desc;	// NULL for mobiles
	long id;			// used by DG triggers
	OBJ_DATA::triggers_list_ptr proto_script;	// list of default triggers
	SCRIPT_DATA::shared_ptr script;	// script info for the object

	CHAR_DATA *next_fighting;	// For fighting list

	struct follow_type *followers;	// List of chars followers

	CHAR_DATA::ptr_t get_master() const { return m_master; }
	void set_master(CHAR_DATA::ptr_t master);
	bool has_master() const { return nullptr != m_master; }
	bool makes_loop(const CHAR_DATA::ptr_t master) const;

	struct spell_mem_queue MemQueue;		// очередь изучаемых заклинаний

	int CasterLevel;
	int DamageLevel;
	struct PK_Memory_type *pk_list;
	struct helper_data_type *helpers;
	int track_dirs;
	int CheckAggressive;
	int ExtractTimer;

	FLAG_DATA Temporary;

	int Initiative;
	int BattleCounter;
	int round_counter;

	FLAG_DATA BattleAffects;

	int Poisoner;

	int *ing_list;		//загружаемые в труп ингредиенты
	load_list *dl_list;	// загружаемые в труп предметы
	bool agrobd;		// показывает, агробд или нет

	std::map<int, temporary_spell_data> temp_spells;
};

inline const player_special_data::ignores_t& CHAR_DATA::get_ignores() const
{
	const auto& ps = get_player_specials();
	return ps->ignores;
}

inline void CHAR_DATA::add_ignore(const ignore_data::shared_ptr ignore)
{
	const auto& ps = get_player_specials();
	ps->ignores.push_back(ignore);
}

inline void CHAR_DATA::clear_ignores()
{
	get_player_specials()->ignores.clear();
}

inline int GET_INVIS_LEV(const CHAR_DATA* ch) 
{ 
	if (ch->player_specials->saved.invis_level)
		return ch->player_specials->saved.invis_level; 
	else
		return 0;
}
inline int GET_INVIS_LEV(const CHAR_DATA::shared_ptr& ch) { return GET_INVIS_LEV(ch.get()); }

inline void SET_INVIS_LEV(const CHAR_DATA* ch, const int level) { ch->player_specials->saved.invis_level = level; }
inline void SET_INVIS_LEV(const CHAR_DATA::shared_ptr& ch, const int level) { SET_INVIS_LEV(ch.get(), level); }

inline void WAIT_STATE(CHAR_DATA* ch, const unsigned cycle)
{
	if (ch->get_wait() < cycle)
	{
		ch->set_wait(cycle);
	}
}

inline FLAG_DATA& AFF_FLAGS(CHAR_DATA* ch) { return ch->char_specials.saved.affected_by; }
inline const FLAG_DATA& AFF_FLAGS(const CHAR_DATA* ch) { return ch->char_specials.saved.affected_by; }
inline const FLAG_DATA& AFF_FLAGS(const CHAR_DATA::shared_ptr& ch) { return ch->char_specials.saved.affected_by; }

inline bool AFF_FLAGGED(const CHAR_DATA* ch, const EAffectFlag flag)
{
	return AFF_FLAGS(ch).get(flag)
		|| ch->isAffected(flag);
}

inline bool AFF_FLAGGED(const CHAR_DATA::shared_ptr& ch, const EAffectFlag flag)
{
	return AFF_FLAGS(ch).get(flag)
		|| ch->isAffected(flag);
}

bool IS_CHARMICE(const CHAR_DATA* ch);
inline bool IS_CHARMICE(const CHAR_DATA::shared_ptr& ch) { return IS_CHARMICE(ch.get()); }

inline bool IS_FLY(const CHAR_DATA* ch)
{
	return AFF_FLAGGED(ch, EAffectFlag::AFF_FLY);
}

inline bool INVIS_OK(const CHAR_DATA* sub, const CHAR_DATA* obj)
{
	return !AFF_FLAGGED(sub, EAffectFlag::AFF_BLIND)
		&& ((!AFF_FLAGGED(obj, EAffectFlag::AFF_INVISIBLE)
				|| AFF_FLAGGED(sub, EAffectFlag::AFF_DETECT_INVIS))
			&& ((!AFF_FLAGGED(obj, EAffectFlag::AFF_HIDE)
					&& !AFF_FLAGGED(obj, EAffectFlag::AFF_CAMOUFLAGE))
				|| AFF_FLAGGED(sub, EAffectFlag::AFF_SENSE_LIFE)));
}

inline bool INVIS_OK(const CHAR_DATA* sub, const CHAR_DATA::shared_ptr& obj) { return INVIS_OK(sub, obj.get()); }

bool MORT_CAN_SEE(const CHAR_DATA* sub, const CHAR_DATA* obj);
bool IMM_CAN_SEE(const CHAR_DATA* sub, const CHAR_DATA* obj);

inline bool SELF(const CHAR_DATA* sub, const CHAR_DATA* obj) { return sub == obj; }
inline bool SELF(const CHAR_DATA* sub, const CHAR_DATA::shared_ptr& obj) { return sub == obj.get(); }

/// Can subject see character "obj"?
bool CAN_SEE(const CHAR_DATA* sub, const CHAR_DATA* obj);
inline bool CAN_SEE(const CHAR_DATA* sub, const CHAR_DATA::shared_ptr& obj) { return CAN_SEE(sub, obj.get()); }
inline bool CAN_SEE(const CHAR_DATA::shared_ptr& sub, const CHAR_DATA* obj) { return CAN_SEE(sub.get(), obj); }
inline bool CAN_SEE(const CHAR_DATA::shared_ptr& sub, const CHAR_DATA::shared_ptr& obj) { return CAN_SEE(sub.get(), obj.get()); }

bool MAY_SEE(const CHAR_DATA* ch, const CHAR_DATA* sub, const CHAR_DATA* obj);

bool IS_HORSE(const CHAR_DATA* ch);
inline bool IS_HORSE(const CHAR_DATA::shared_ptr& ch) { return IS_HORSE(ch.get()); }
bool IS_MORTIFIER(const CHAR_DATA* ch);

bool MAY_ATTACK(const CHAR_DATA* sub);
inline bool MAY_ATTACK(const CHAR_DATA::shared_ptr& sub) { return MAY_ATTACK(sub.get()); }

inline bool GET_MOB_HOLD(const CHAR_DATA* ch)
{
	return AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD);
}

bool AWAKE(const CHAR_DATA* ch);
inline bool AWAKE(const CHAR_DATA::shared_ptr& ch) { return AWAKE(ch.get()); }

// Polud условие для проверки перед запуском всех mob-триггеров КРОМЕ death, random и global
//пока здесь только чарм, как и было раньше
inline bool CAN_START_MTRIG(const CHAR_DATA *ch)
{
	return !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM);
}
//-Polud

bool OK_GAIN_EXP(const CHAR_DATA* ch, const CHAR_DATA* victim);

bool IS_MALE(const CHAR_DATA* ch);
inline bool IS_MALE(const CHAR_DATA::shared_ptr& ch) { return IS_MALE(ch.get()); }
bool IS_FEMALE(const CHAR_DATA* ch);
inline bool IS_FEMALE(const CHAR_DATA::shared_ptr& ch) { return IS_FEMALE(ch.get()); }
bool IS_NOSEXY(const CHAR_DATA* ch);
inline bool IS_NOSEXY(const CHAR_DATA::shared_ptr& ch) { return IS_NOSEXY(ch.get()); }
bool IS_POLY(const CHAR_DATA* ch);

inline int VPOSI_MOB(const CHAR_DATA *ch, const int stat_id, const int val)
{
	const int character_class = ch->get_class();
	return ch->is_npc()
		? VPOSI(val, 1, 100)
		: VPOSI(val, 1, class_stats_limit[character_class][stat_id]);
}
inline int VPOSI_MOB(const CHAR_DATA::shared_ptr& ch, const int stat_id, const int val) { return VPOSI_MOB(ch.get(), stat_id, val); }

inline auto GET_REAL_DEX(const CHAR_DATA* ch)
{
	return VPOSI_MOB(ch, 1, ch->get_dex() + ch->get_dex_add());
}

void change_fighting(CHAR_DATA * ch, int need_stop);

#endif // CHAR_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
