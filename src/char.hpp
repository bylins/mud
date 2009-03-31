// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_HPP_INCLUDED
#define CHAR_HPP_INCLUDED

#include <bitset>
#include <map>
#include <boost/shared_ptr.hpp>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

/* These data contain information about a players time data */
struct time_data
{
	time_t birth;		/* This represents the characters age                */
	time_t logon;		/* Time of the last logon (used to calculate played) */
	int played;		/* This is the total accumulated time played in secs */
};

/* general player-related info, usually PC's and NPC's */
struct char_player_data
{
	char *name;		/* PC / NPC s name (kill ...  )         */
	char *short_descr;	/* for NPC 'actions'                    */
	char *long_descr;	/* for 'look'               */
	char *description;	/* Extra descriptions                   */
	char *title;		/* PC / NPC's title                     */
	byte sex;		/* PC / NPC's sex                       */
	byte chclass;		/* PC / NPC's class             */
	byte level;		/* PC / NPC's level                     */
	struct time_data time;			/* PC's AGE in days                 */
	ubyte weight;		/* PC / NPC's weight                    */
	ubyte height;		/* PC / NPC's height                    */

	char *PNames[6];
	ubyte Religion;
	ubyte Kin;
	ubyte Race;		/* PC / NPC's race*/
};

/* Char's additional abilities. Used only while work */
struct char_played_ability_data
{
	int str_add;
	int intel_add;
	int wis_add;
	int dex_add;
	int con_add;
	int cha_add;
	int weight_add;
	int height_add;
	int size_add;
	int age_add;
	int hit_add;
	int move_add;
	int hitreg;
	int movereg;
	int manareg;
	sbyte slot_add[10];
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
	sh_int apply_saving_throw[4];		/* Saving throw (Bonuses)  */
	sh_int apply_resistance_throw[7];	/* Сопротивление (резисты) к магии, ядам и крит. ударам */
	ubyte mresist;
	ubyte aresist;
};

/* Char's abilities. */
struct char_ability_data
{
	ubyte SplKnw[MAX_SPELLS + 1];	/* array of SPELL_KNOW_TYPE         */
	ubyte SplMem[MAX_SPELLS + 1];	/* array of MEMed SPELLS            */
	bitset<MAX_FEATS> Feats;
	sbyte str;
	sbyte intel;
	sbyte wis;
	sbyte dex;
	sbyte con;
	sbyte cha;
	sbyte size;
	sbyte hitroll;
	sbyte damroll;
	sbyte armor;
};

/* Char's points. */
struct char_point_data
{
	sh_int hit;
	sh_int max_hit;		/* Max hit for PC/NPC                      */
	sh_int move;
	sh_int max_move;	/* Max move for PC/NPC                     */
	int gold;			/* Money carried                           */
	long bank_gold;		/* Gold the char has in a bank account    */
	long exp;			/* The experience of the player            */
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
	int alignment;		/* +-1000 for alignments                */
	long idnum;			/* player's idnum; -1 for mobiles   */
	FLAG_DATA act;		/* act flag for NPC's; player flag for PC's */

	FLAG_DATA affected_by;
	/* Bitvector for spells/skills affected by */
};

/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data
{
	CHAR_DATA *fighting;	/* Opponent */

	byte position;		/* Standing, fighting, sleeping, etc. */

	int carry_weight;		/* Carried weight */
	int carry_items;		/* Number of items carried   */
	int timer;			/* Timer for update  */

	struct char_special_data_saved saved;			/* constants saved in plrfile  */
};

/* Specials used by NPCs, not PCs */
struct mob_special_data
{
	byte last_direction;	/* The last direction the monster went     */
	int attack_type;		/* The Attack Type Bitvector for NPC's     */
	byte default_pos;	/* Default position for NPC                */
	memory_rec *memory;	/* List of attackers to remember          */
	byte damnodice;		/* The number of damage dice's             */
	byte damsizedice;	/* The size of the damage dice's           */
	int dest[MAX_DEST];
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
};

// очередь запоминания заклинаний
struct spell_mem_queue
{
	struct spell_mem_queue_item *queue;
	int stored;		// накоплено манны
	int total;			// полное время мема всей очереди
};

/* Structure used for extra_attack - bash, kick, diasrm, chopoff, etc */
struct extra_attack_type
{
	int used_skill;
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
	int wimp_level;		/* Below this # of hit points, flee!  */
	int invis_level;		/* level of invisibility      */
	room_vnum load_room;	/* Which room to place char in      */
	FLAG_DATA pref;		/* preference flags for PC's.    */
	int bad_pws;		/* number of bad password attemps   */
	int conditions[3];		/* Drunk, full, thirsty        */

	int DrunkState;
	int olc_zone;
	int unique;
	int Remorts;
	int NameGod;
	long NameIDGod;
	long GodsLike;
	time_t LastLogon; //by kilnik

	char EMail[128];
	char LastIP[128];

	char remember[MAX_REMEMBER_TELLS][MAX_RAW_INPUT_LENGTH];
	int lasttell;

	int stringLength;
	int stringWidth;
};


#define START_STATS_TOTAL 5 // кол-во сохраняемых стартовых статов в файле

struct player_special_data
{
	struct player_special_data_saved saved;

	char *poofin;		/* Description on arrival of a god. */
	char *poofout;		/* Description upon a god's exit.   */
	struct alias_data *aliases;	/* Character's aliases    */
	long last_tell;		/* idnum of last tell from      */
	time_t may_rent;		/* PK control                       */
	int agressor;		/* Agression room(it is also a flag) */
	time_t agro_time;		/* Last agression time (it is also a flag) */
	struct _im_rskill_tag *rskill;	/* Известные рецепты */
	struct char_portal_type *portals;	/* порталы теперь живут тут */
	int *logs;		// уровни подробности каналов log

	char *LastAllTell;
	char *Exchange_filter;
	struct ignore_data *ignores;
	char *Karma; /* Записи о поощрениях, наказаниях персонажа*/

	struct logon_data * logons; /*Записи о входах чара*/

// Punishments structs
	struct punish_data pmute;
	struct punish_data pdumb;
	struct punish_data phell;
	struct punish_data pname;
	struct punish_data pfreeze;
	struct punish_data pgcurse;
	struct punish_data punreg;

	char *clanStatus; // строка для отображения приписки по кто
	// TODO: однозначно переписать
	boost::shared_ptr<class Clan> clan; // собсна клан, если он есть
	boost::shared_ptr<class ClanMember> clan_member; // поле мембера в клане

	struct board_data *board; // последние прочитанные мессаги на досках
	int start_stats[START_STATS_TOTAL]; // сгенеренные при старте чара статы
};

class Player;
typedef boost::shared_ptr<Player> PlayerPtr;
typedef std::map < int/* номер скилла */, int/* значение скилла */ > CharSkillsType;

/**
* Общий класс для игроков/мобов.
*/
class Character
{
// новое
public:
	Character();
	~Character();

	void create_player();
	void create_mob_guard();

	// это все как обычно временно... =)
	friend void save_char(CHAR_DATA *ch);

	int get_skill(int skill_num);
	void set_skill(int skill_num, int percent);
	void clear_skills();
	int get_skills_count();

	int get_equipped_skill(int skill_num);
	int get_trained_skill(int skill_num);

private:
	static int normolize_skill(int percent);

public:
	PlayerPtr player;

private:
	CharSkillsType skills; // список изученных скиллов

// старое
public:
	mob_rnum nr;		/* Mob's rnum                   */
	room_rnum in_room;	/* Location (real room number)   */
	int wait;			/* wait for how many loops         */
	int punctual_wait;		/* wait for how many loops (punctual style) */
	char *last_comm;		/* последний приказ чармису перед окончанием лага */

	struct char_player_data player_data;		/* Normal data                   */
	struct char_played_ability_data add_abils;		/* Abilities that add to main */
	struct char_ability_data real_abils;		/* Abilities without modifiers   */
	struct char_point_data points;		/* Points                       */
	struct char_special_data char_specials;		/* PC/NPC specials     */
	struct mob_special_data mob_specials;		/* NPC specials        */

	struct player_special_data *player_specials;	/* PC specials        */

	AFFECT_DATA *affected;	/* affected by what spells       */
	struct timed_type *timed;	/* use which timed skill/spells  */
	struct timed_type *timed_feat;	/* use which timed feats  */
	OBJ_DATA *equipment[NUM_WEARS];	/* Equipment array               */

	OBJ_DATA *carrying;	/* Head of list                  */
	DESCRIPTOR_DATA *desc;	/* NULL for mobiles              */

	long id;			/* used by DG triggers             */
	struct trig_proto_list *proto_script;	/* list of default triggers      */
	struct script_data *script;	/* script info for the object      */
	struct script_memory *memory;	/* for mob memory triggers         */

	CHAR_DATA *next_in_room;	/* For room->people - list         */
	CHAR_DATA *next;	/* For either monster or ppl-list  */
	CHAR_DATA *next_fighting;	/* For fighting list               */

	struct follow_type *followers;	/* List of chars followers       */
	CHAR_DATA *master;	/* Who is char following?        */

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

	FLAG_DATA BattleAffects;

	CHAR_DATA *Protecting;
	CHAR_DATA *Touching;

	int Poisoner;

	struct extra_attack_type extra_attack;
	struct cast_attack_type cast_attack;

	int *ing_list;		//загружаемые в труп ингредиенты
	load_list *dl_list;	// загружаемые в труп предметы
};

//Polud тестовый класс для хранения параметров различных рас мобов
struct ingredient
{
	int imtype;
	std::string imname;
	int prob[50]; // вероятность загрузки для каждого уровня моба 
};

class MobRace{
public:
	MobRace();
	~MobRace();
	std::string race_name;
	std::vector<ingredient> ingrlist;
};

typedef boost::shared_ptr<MobRace> MobRacePtr;
typedef std::map<int, MobRacePtr> MobRaceListType;

//-Polud

#endif // CHAR_HPP_INCLUDED
