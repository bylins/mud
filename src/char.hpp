// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#ifndef CHAR_HPP_INCLUDED
#define CHAR_HPP_INCLUDED

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
	char *passwd;	/* character's password      */
	char *name;		/* PC / NPC s name (kill ...  )         */
	char *short_descr;	/* for NPC 'actions'                    */
	char *long_descr;	/* for 'look'               */
	char *description;	/* Extra descriptions                   */
	char *title;		/* PC / NPC's title                     */
	byte sex;		/* PC / NPC's sex                       */
	byte chclass;		/* PC / NPC's class             */
	byte level;		/* PC / NPC's level                     */
	int hometown;		/* PC s Hometown (zone)                 */
	struct time_data time;			/* PC's AGE in days                 */
	ubyte weight;		/* PC / NPC's weight                    */
	ubyte height;		/* PC / NPC's height                    */

	char *PNames[6];
	ubyte Religion;
	ubyte Kin;
	ubyte Race;
	ubyte Side;
	ubyte Lows;
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
	long pk_counter;		/* pk counter list */
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
	CHAR_DATA *hunting;	/* Char hunted by this char */

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

/* Structure used for questing */
struct quest_data
{
	int count;
	int *quests;
};

struct mob_kill_data
{
	int count;
	int *howmany;
	int *vnum;
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

typedef std::map < int/* номер скилла */, int/* значение скилла */ > CharSkillsType;

/**
* Общий класс для игроков/мобов.
*/
class Character
{
public:
// новое
	Character();
	~Character();
// поля
	CharSkillsType skills; // список изученных скиллов
// методы
	int get_skill(int skill_num);
	void set_skill(int skill_num, int percent);

// старое
	int pfilepos;		/* playerfile pos                */
	mob_rnum nr;		/* Mob's rnum                   */
	room_rnum in_room;	/* Location (real room number)   */
	room_rnum was_in_room;	/* location for linkdead people  */
	int wait;			/* wait for how many loops         */
	int punctual_wait;		/* wait for how many loops (punctual style) */
	char *last_comm;		/* последний приказ чармису перед окончанием лага */

	struct char_player_data player;		/* Normal data                   */
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

	struct quest_data Questing;
	struct mob_kill_data MobKill;

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
/* ====================================================================== */

#endif // CHAR_HPP_INCLUDED
