/* ************************************************************************
*   File: utils.h                                       Part of Bylins    *
*  Usage: header file: utility macros and prototypes of utility funcs     *                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _UTILS_H_
#define _UTILS_H_

#include "conf.h"
#include <string>
#include <list>
#include <map>
#include <new>
#include <boost/random.hpp>

using std::string;
using std::list;

/* external declarations and prototypes **********************************/

extern struct weather_data weather_info;
extern char AltToKoi[];
extern char KoiToAlt[];
extern char WinToKoi[];
extern char KoiToWin[];
extern char KoiToWin2[];
extern char AltToLat[];

//#define log        basic_mud_log

/* public functions in utils.cpp */
CHAR_DATA *find_char(long n);
char *title_noname(struct char_data *ch);
char *rustime(const struct tm *timeptr);
char *str_dup(const char *source);
char *str_add(char *dst, const char *src);
int str_cmp(const char *arg1, const char *arg2);
int str_cmp(const std::string &arg1, const char *arg2);
int str_cmp(const char *arg1, const std::string &arg2);
int str_cmp(const std::string &arg1, const std::string &arg2);
int strn_cmp(const char *arg1, const char *arg2, int n);
int strn_cmp(const std::string &arg1, const char *arg2, int n);
int strn_cmp(const char *arg1, const std::string &arg2, int n);
int strn_cmp(const std::string &arg1, const std::string &arg2, int n);
void write_time(FILE *file);
void log(const char *format, ...) __attribute__ ((format(printf, 1, 2)));
void olc_log(const char *format, ...);
void imm_log(const char *format, ...);
void pers_log(CHAR_DATA *ch, const char *format, ...);
void ip_log(const char *ip);
int touch(const char *path);
void mudlog(const char *str, int type, int level, int channel, int file);
int number(int from, int to);
int dice(int number, int size);
int sprintbit(bitvector_t vektor, const char *names[], char *result);
int sprintbitwd(bitvector_t vektor, const char *names[], char *result, char * div);
void sprintbits(FLAG_DATA flags, const char *names[], char *result, char *div);
void sprinttype(int type, const char *names[], char *result);
int get_line(FILE * fl, char *buf);
int get_filename(const char *orig_name, char *filename, int mode);
TIME_INFO_DATA *age(CHAR_DATA * ch);
int num_pc_in_room(ROOM_DATA * room);
void core_dump_real(const char *, int);
int replace_str(char **string, char *pattern, char *replacement, int rep_all, int max_size);
void format_text(char **ptr_string, int mode, DESCRIPTOR_DATA * d, int maxlen);
int check_moves(CHAR_DATA * ch, int how_moves);
void to_koi(char *str, int);
void from_koi(char *str, int);
int real_sector(int room);
char *format_act(const char *orig, CHAR_DATA * ch, OBJ_DATA * obj, const void *vict_obj);
int roundup(float fl);
int valid_email(const char *address);
void skip_spaces(char **string);
void skip_dots(char **string);
int get_skill(CHAR_DATA *ch, int skill);

extern std::list<FILE *> opened_files;

#define core_dump()     core_dump_real(__FILE__, __LINE__)

/* random functions in random.cpp */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);

class NormalRand {
	public:
	NormalRand() { rng.seed(static_cast<unsigned> (time(0))); };
	int number(int from, int to)
	{
		boost::uniform_int<> dist(from, to);
		boost::variate_generator<boost::mt19937&, boost::uniform_int<> >  dice(rng, dist);
		return dice();
	};
	boost::mt19937 rng;
};
extern NormalRand rnd;

extern const char *ACTNULL;

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL) i = ACTNULL; else i = (expression);

#define MIN_TITLE_LEV   25

#define ONE_DAY      24*60
#define SEVEN_DAYS   7*24*60

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

#define SF_EMPTY       (1 << 0)
#define SF_FOLLOWERDIE (1 << 1)
#define SF_MASTERDIE   (1 << 2)
#define SF_CHARMLOST   (1 << 3)

#define EXP_IMPL 2000000000
#define MAX_AUCTION_LOT            3
#define MAX_AUCTION_TACT_BUY       5
#define MAX_AUCTION_TACT_PRESENT   (MAX_AUCTION_TACT_BUY + 3)
#define AUCTION_PULSES             30
#define CHAR_DRUNKED               10
#define CHAR_MORTALLY_DRUNKED      16

int MAX(int a, int b);
int MIN(int a, int b);
// all argument min/max macros definition
#define MMIN(a,b) ((a<b)?a:b)
#define MMAX(a,b) ((a<b)?b:a)

char * CAP(char *txt);

#define KtoW(c) ((ubyte)(c) < 128 ? (c) : KoiToWin[(ubyte)(c)-128])
#define KtoW2(c) ((ubyte)(c) < 128 ? (c) : KoiToWin2[(ubyte)(c)-128])
#define KtoA(c) ((ubyte)(c) < 128 ? (c) : KoiToAlt[(ubyte)(c)-128])
#define WtoK(c) ((ubyte)(c) < 128 ? (c) : WinToKoi[(ubyte)(c)-128])
#define AtoK(c) ((ubyte)(c) < 128 ? (c) : AltToKoi[(ubyte)(c)-128])
#define AtoL(c) ((ubyte)(c) < 128 ? (c) : AltToLat[(ubyte)(c)-128])

/* in magic.cpp */
bool circle_follow(CHAR_DATA * ch, CHAR_DATA * victim);

/* in act.informative.cpp */
void look_at_room(CHAR_DATA * ch, int mode);

/* in act.movmement.cpp */
int do_simple_move(CHAR_DATA * ch, int dir, int following, CHAR_DATA * leader);
int perform_move(CHAR_DATA * ch, int dir, int following, int checkmob, CHAR_DATA * leader);

/* in limits.cpp */
int mana_gain(CHAR_DATA * ch);
int hit_gain(CHAR_DATA * ch);
int move_gain(CHAR_DATA * ch);
void advance_level(CHAR_DATA * ch);
void gain_exp(CHAR_DATA * ch, int gain, int clan_exp = 0);
void gain_exp_regardless(CHAR_DATA * ch, int gain);
void gain_condition(CHAR_DATA * ch, int condition, int value);
void check_idling(CHAR_DATA * ch);
void point_update(void);
void update_pos(CHAR_DATA * victim);


/* various constants *****************************************************/

/* defines for mudlog() */
#define OFF 0
#define CMP 1
#define BRF 2
#define NRM 3
#define LGH 4
#define DEF 5

/* get_filename() */
#define CRASH_FILE        0
#define ETEXT_FILE        1
#define ALIAS_FILE        2
#define SCRIPT_VARS_FILE  3
#define PLAYERS_FILE      4
// тут были пк-файлы
#define PQUESTS_FILE      6
#define PMKILL_FILE       7
#define TEXT_CRASH_FILE   8
#define TIME_CRASH_FILE   9
#define PERS_DEPOT_FILE  10
#define SHARE_DEPOT_FILE 11

/* breadth-first searching */
#define BFS_ERROR        -1
#define BFS_ALREADY_THERE  -2
#define BFS_NO_PATH         -3
#define NUM_PADS             6
/*
 * XXX: These constants should be configurable. See act.informative.c
 * and utils.cpp for other places to change.
 */
/* mud-life time */
#define HOURS_PER_DAY          24
#define DAYS_PER_MONTH         30
#define MONTHS_PER_YEAR        12
#define SECS_PER_PLAYER_AFFECT 2
#define SECS_PER_ROOM_AFFECT 2
#define TIME_KOEFF             2
#define MOB_MEM_KOEFF          SECS_PER_MUD_HOUR
#define SECS_PER_MUD_HOUR     60
#define SECS_PER_MUD_DAY      (HOURS_PER_DAY*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH    (DAYS_PER_MONTH*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR     (MONTHS_PER_YEAR*SECS_PER_MUD_MONTH)
#define NEWMOONSTART               27
#define NEWMOONSTOP                1
#define HALFMOONSTART              7
#define FULLMOONSTART              13
#define FULLMOONSTOP               15
#define LASTHALFMOONSTART          21
#define MOON_CYCLE               28
#define WEEK_CYCLE               7
#define POLY_WEEK_CYCLE          9

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN  60
#define SECS_PER_REAL_HOUR (60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY  (24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR (365*SECS_PER_REAL_DAY)

#define IS_IMMORTAL(ch)     (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT)
#define IS_GOD(ch)          (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_GOD)
#define IS_GRGOD(ch)        (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_GRGOD)
#define IS_IMPL(ch)         (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMPL)
#define IS_HIGHGOD(ch)      (IS_IMPL(ch) && (GET_GOD_FLAG(ch,GF_HIGHGOD)))

#define IS_BITS(mask, bitno) (IS_SET(mask,(1 << bitno)))
#define IS_CASTER(ch)        (IS_BITS(MASK_CASTER,GET_CLASS(ch)))
#define IS_MAGE(ch)          (IS_BITS(MASK_MAGES, GET_CLASS(ch)))


extern SPECIAL(receptionist);
extern SPECIAL(postmaster);


#define IS_SHOPKEEPER(ch) (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == shop_keeper)
#define IS_RENTKEEPER(ch) (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == receptionist)
#define IS_POSTKEEPER(ch) (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == postmaster)

/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (a_lcc(c))
#define UPPER(c)   (a_ucc(c))
#define LATIN(c)   (a_lat(c))

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')
#define IF_STR(st) ((st) ? (st) : "\0")

/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
   if ((number) * sizeof(type) <= 0)   \
      log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__); \
   if (!((result) = (type *) calloc ((number), sizeof(type)))) \
      { perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
      { perror("SYSERR: realloc failure"); abort(); } } while(0)

#define NEWCREATE(result, constructor) do {\
   if (!((result) = new(std::nothrow) constructor)) \
      { perror("SYSERR: new operator failure"); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)   \
   if ((item) == (head))      \
      head = (item)->next;    \
   else {            \
      temp = head;         \
      while (temp && (temp->next != (item))) \
    temp = temp->next;     \
      if (temp)            \
         temp->next = (item)->next; \
   }              \


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)  ((flag & 0x3FFFFFFF) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit & 0x3FFFFFFF))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit & 0x3FFFFFFF))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit & 0x3FFFFFFF))

/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 0
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
   (*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)  (var)
#endif

#define MOB_FLAGS(ch,flag)  (GET_FLAG((ch)->char_specials.saved.act,flag))
#define PLR_FLAGS(ch,flag)  (GET_FLAG((ch)->char_specials.saved.act,flag))
#define PRF_FLAGS(ch,flag)  (GET_FLAG((ch)->player_specials->saved.pref, flag))
#define AFF_FLAGS(ch,flag)  (GET_FLAG((ch)->char_specials.saved.affected_by, flag))
#define NPC_FLAGS(ch,flag)  (GET_FLAG((ch)->mob_specials.npc_flags, flag))
#define ROOM_AFF_FLAGS(room,flag)  (GET_FLAG((room)->affected_by, flag))
#define EXTRA_FLAGS(ch,flag)(GET_FLAG((ch)->Temporary, flag))
#define ROOM_FLAGS(loc,flag)(GET_FLAG(world[(loc)]->room_flags, flag))
#define DESC_FLAGS(d)   ((d)->options)
#define SPELL_ROUTINES(spl) (spell_info[spl].routines)

/* See http://www.circlemud.org/~greerga/todo.009 to eliminate MOB_ISNPC. */
#define IS_NPC(ch)           (IS_SET(MOB_FLAGS(ch, MOB_ISNPC), MOB_ISNPC))
#define IS_MOB(ch)          (IS_NPC(ch) && GET_MOB_RNUM(ch) >= 0)

#define MOB_FLAGGED(ch, flag)   (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch,flag), (flag)))
#define PLR_FLAGGED(ch, flag)   (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch,flag), (flag)))
#define AFF_FLAGGED(ch, flag)   (IS_SET(AFF_FLAGS(ch,flag), (flag)))
#define PRF_FLAGGED(ch, flag)   (IS_SET(PRF_FLAGS(ch,flag), (flag)))
#define NPC_FLAGGED(ch, flag)   (IS_SET(NPC_FLAGS(ch,flag), (flag)))
#define EXTRA_FLAGGED(ch, flag) (IS_SET(EXTRA_FLAGS(ch,flag), (flag)))
#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS((loc),(flag)), (flag)))
#define ROOM_AFFECTED(loc, flag) (IS_SET(ROOM_AFF_FLAGS((world[(loc)]),(flag)), (flag)))
#define EXIT_FLAGGED(exit, flag)     (IS_SET((exit)->exit_info, (flag)))
#define OBJVAL_FLAGGED(obj, flag)    (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define OBJWEAR_FLAGGED(obj, flag)   (IS_SET((obj)->obj_flags.wear_flags, (flag)))
#define DESC_FLAGGED(d, flag) (IS_SET(DESC_FLAGS(d), (flag)))
#define OBJ_FLAGGED(obj, flag)       (IS_SET(GET_OBJ_EXTRA(obj,flag), (flag)))
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))
#define IS_FLY(ch)                   (AFF_FLAGGED(ch,AFF_FLY))
#define SET_EXTRA(ch,skill,vict)   {(ch)->extra_attack.used_skill = skill; \
                                     (ch)->extra_attack.victim     = vict;}
#define GET_EXTRA_SKILL(ch)          ((ch)->extra_attack.used_skill)
#define GET_EXTRA_VICTIM(ch)         ((ch)->extra_attack.victim)
#define SET_CAST(ch,snum,subst,dch,dobj,droom)   {(ch)->cast_attack.spellnum  = snum; \
					(ch)->cast_attack.spell_subst  = subst; \
                                       (ch)->cast_attack.tch       = dch; \
                                       (ch)->cast_attack.tobj      = dobj; \
				       (ch)->cast_attack.troom     = droom;}
#define GET_CAST_SPELL(ch)         ((ch)->cast_attack.spellnum)
#define GET_CAST_SUBST(ch)         ((ch)->cast_attack.spell_subst)
#define GET_CAST_CHAR(ch)          ((ch)->cast_attack.tch)
#define GET_CAST_OBJ(ch)           ((ch)->cast_attack.tobj)



/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED(ch, skill))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch, flag), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch, flag), (flag))) & (flag))


/* room utils ************************************************************/


#define SECT(room)   (world[(room)]->sector_type)
#define GET_ROOM_SKY(room) (world[room]->weather.duration > 0 ? \
                            world[room]->weather.sky : weather_info.sky)

#define IS_MOONLIGHT(room) ((GET_ROOM_SKY(room) == SKY_LIGHTNING && \
                             weather_info.moon_day >= FULLMOONSTART && \
                             weather_info.moon_day <= FULLMOONSTOP))

#define IS_TIMEDARK(room)  ((world[room]->gdark > world[room]->glight) || \
                            (!(world[room]->light+world[room]->fires+world[room]->glight) && \
                              (ROOM_FLAGGED(room, ROOM_DARK) || \
                              (SECT(room) != SECT_INSIDE && \
                               SECT(room) != SECT_CITY   && \
                               ( weather_info.sunlight == SUN_SET || \
                                 weather_info.sunlight == SUN_DARK )) ) ) )

#define IS_DEFAULTDARK(room) (ROOM_FLAGGED(room, ROOM_DARK) || \
                              (SECT(room) != SECT_INSIDE && \
                               SECT(room) != SECT_CITY   && \
                               ( weather_info.sunlight == SUN_SET || \
                                 weather_info.sunlight == SUN_DARK )) )


#define IS_DARK(room)      ((world[room]->gdark > world[room]->glight) || \
                            (!(world[room]->gdark < world[room]->glight) && \
                            !(world[room]->light+world[room]->fires) && \
                              !ROOM_AFFECTED(room, AFF_ROOM_LIGHT) && \
				(ROOM_FLAGGED(room, ROOM_DARK) || \
                              (SECT(room) != SECT_INSIDE && \
                               SECT(room) != SECT_CITY   && \
                               ( weather_info.sunlight == SUN_SET || \
                                 (weather_info.sunlight == SUN_DARK && \
                                  !IS_MOONLIGHT(room)) )) ) ) )

#define IS_DARKTHIS(room)      ((world[room]->gdark > world[room]->glight) || \
                                (!(world[room]->gdark < world[room]->glight) && \
                                 !(world[room]->light+world[room]->fires) && \
                                  (ROOM_FLAGGED(room, ROOM_DARK) || \
                                  (SECT(room) != SECT_INSIDE && \
                                   SECT(room) != SECT_CITY   && \
                                   ( weather_info.sunlight == SUN_DARK && \
                                     !IS_MOONLIGHT(room) )) ) ) )

#define IS_DARKSIDE(room)      ((world[room]->gdark > world[room]->glight) || \
                                (!(world[room]->gdark < world[room]->glight) && \
                                 !(world[room]->light+world[room]->fires) && \
                                  (ROOM_FLAGGED(room, ROOM_DARK) || \
                                  (SECT(room) != SECT_INSIDE && \
                                   SECT(room) != SECT_CITY   && \
                                   ( weather_info.sunlight == SUN_SET  || \
                                     weather_info.sunlight == SUN_RISE || \
                                     (weather_info.sunlight == SUN_DARK && \
                                      !IS_MOONLIGHT(room) )) ) ) )


#define IS_LIGHT(room)     (!IS_DARK(room))

#define VALID_RNUM(rnum)   ((rnum) >= 0 && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
   ((room_vnum)(VALID_RNUM(rnum) ? world[(rnum)]->number : NOWHERE))
#define GET_ROOM_SPEC(room) (VALID_RNUM(room) ? world[(room)]->func : NULL)

/* char utils ************************************************************/

#define IS_MANA_CASTER(ch) (GET_CLASS(ch)==CLASS_DRUID)
#define IN_ROOM(ch)  ((ch)->in_room)
#define GET_WAS_IN(ch)  ((ch)->was_in_room)
#define GET_AGE(ch)     (age(ch)->year)
#define GET_REAL_AGE(ch) (age(ch)->year + GET_AGE_ADD(ch))

#define GET_PC_NAME(ch) ((ch)->player.name)
#define GET_NAME(ch)    (IS_NPC(ch) ? \
          (ch)->player.short_descr : GET_PC_NAME(ch))
#define GET_HELPER(ch)  ((ch)->helpers)
#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_LEVEL(ch)   ((ch)->player.level)
#define GET_MAX_MANA(ch)      MIN(9998,mana[(int) GET_REAL_WIS(ch)]+GET_REMORT(ch)*500)
 /*#define GET_MANA_COST(ch,spellnum)      (mana_cost_cs[(int)GET_LEVEL(ch)][spell_create[spellnum].runes.krug-1]) */
#define GET_MANA_COST(ch,spellnum)      mag_manacost(ch,spellnum)
#define GET_MANA_STORED(ch)   ((ch)->MemQueue.stored)
#define GET_MEM_COMPLETED(ch) ((ch)->MemQueue.stored)
#define GET_MEM_CURRENT(ch)   (MEMQUEUE_EMPTY(ch)?0:mag_manacost(ch,(ch)->MemQueue.queue->spellnum))
#define GET_MEM_TOTAL(ch)     ((ch)->MemQueue.total)
#define MEMQUEUE_EMPTY(ch)    ((ch)->MemQueue.queue == NULL)
#define INITIATIVE(ch)        ((ch)->Initiative)
#define BATTLECNTR(ch)        ((ch)->BattleCounter)
#define EXTRACT_TIMER(ch)     ((ch)->ExtractTimer)
#define CHECK_AGRO(ch)        ((ch)->CheckAggressive)
#define WAITLESS(ch)          (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))
#define PUNCTUAL_WAITLESS(ch)          (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))
#define CLR_MEMORY(ch)  (memset((ch)->Memory,0,MAX_SPELLS+1))
#define FORGET_ALL(ch) {MemQ_flush(ch);memset((ch)->real_abils.SplMem,0,MAX_SPELLS+1);}
#define GET_PASSWD(ch)   ((ch)->player.passwd)
#define GET_PFILEPOS(ch) ((ch)->pfilepos)
#define IS_KILLER(ch)    ((ch)->points.pk_counter)
#define IS_CODER(ch)    (GET_LEVEL(ch) < LVL_IMMORT && PRF_FLAGGED(ch, PRF_CODERINFO))
#define IS_COLORED(ch)    (pk_count (ch))
#define GET_LASTTELL(ch)    ((ch)->player_specials->saved.lasttell)
#define GET_TELL(ch,i)   ((ch)->player_specials->saved.remember)[i]
#define MAX_PORTALS(ch)  ((GET_LEVEL(ch)/3)+GET_REMORT(ch))

/**** Adding by me */
#define GET_AF_BATTLE(ch,flag) (IS_SET(GET_FLAG((ch)->BattleAffects, flag),flag))
#define SET_AF_BATTLE(ch,flag) (SET_BIT(GET_FLAG((ch)->BattleAffects,flag),flag))
#define CLR_AF_BATTLE(ch,flag) (REMOVE_BIT(GET_FLAG((ch)->BattleAffects, flag),flag))
#define NUL_AF_BATTLE(ch)      (GET_FLAG((ch)->BattleAffects, 0) = \
                                GET_FLAG((ch)->BattleAffects, INT_ONE) = \
                                GET_FLAG((ch)->BattleAffects, INT_TWO) = \
                                GET_FLAG((ch)->BattleAffects, INT_THREE) = 0)
#define GET_GLORY(ch)          ((ch)->player_specials->saved.glory)
#define GET_REMORT(ch)         ((ch)->player_specials->saved.Remorts)
#define GET_EMAIL(ch)          ((ch)->player_specials->saved.EMail)
#define GET_LASTIP(ch)         ((ch)->player_specials->saved.LastIP)
#define GET_GOD_FLAG(ch,flag)  (IS_SET((ch)->player_specials->saved.GodsLike,flag))
#define SET_GOD_FLAG(ch,flag)  (SET_BIT((ch)->player_specials->saved.GodsLike,flag))
#define CLR_GOD_FLAG(ch,flag)  (REMOVE_BIT((ch)->player_specials->saved.GodsLike,flag))
#define GET_UNIQUE(ch)         ((ch)->player_specials->saved.unique)
#define LAST_LOGON(ch)         ((ch)->player_specials->saved.LastLogon)

#define NAME_GOD(ch)  ((ch)->player_specials->saved.NameGod)
#define NAME_ID_GOD(ch)  ((ch)->player_specials->saved.NameIDGod)

#define STRING_LENGTH(ch)  ((ch)->player_specials->saved.stringLength)
#define STRING_WIDTH(ch)  ((ch)->player_specials->saved.stringWidth)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define POSI(val)      ((val < 50) ? ((val > 0) ? val : 1) : 50)
#define VPOSI(val,min,max)      ((val < max) ? ((val > min) ? val : min) : max)
#define GET_CLASS(ch)   ((ch)->player.chclass)
#define GET_KIN(ch)     ((ch)->player.Kin)
#define GET_HOME(ch) ((ch)->player.hometown)
#define GET_HEIGHT(ch)  ((ch)->player.height)
#define GET_HEIGHT_ADD(ch) ((ch)->add_abils.height_add)
#define GET_REAL_HEIGHT(ch) (GET_HEIGHT(ch) + GET_HEIGHT_ADD(ch))
#define GET_WEIGHT(ch)  ((ch)->player.weight)
#define GET_WEIGHT_ADD(ch) ((ch)->add_abils.weight_add)
#define GET_REAL_WEIGHT(ch) (GET_WEIGHT(ch) + GET_WEIGHT_ADD(ch))
#define GET_SEX(ch)  ((ch)->player.sex)

#define IS_MALE(ch)     (GET_SEX(ch) == SEX_MALE)
#define IS_FEMALE(ch)   (GET_SEX(ch) == SEX_FEMALE)
#define IS_NOSEXY(ch)   (GET_SEX(ch) == SEX_NEUTRAL)
#define IS_POLY(ch)     (GET_SEX(ch) == SEX_POLY)

#define GET_LOWS(ch) ((ch)->player.Lows)
#define GET_RELIGION(ch) ((ch)->player.Religion)
#define GET_RACE(ch) ((ch)->player.Race)
#define GET_SIDE(ch) ((ch)->player.Side)
#define GET_PAD(ch,i)    ((ch)->player.PNames[i])
#define GET_DRUNK_STATE(ch) ((ch)->player_specials->saved.DrunkState)

#define GET_STR(ch)     ((ch)->real_abils.str)
#define GET_STR_ADD(ch) ((ch)->add_abils.str_add)
#define GET_REAL_STR(ch) (POSI(GET_STR(ch) + GET_STR_ADD(ch)))
#define GET_DEX(ch)     ((ch)->real_abils.dex)
#define GET_DEX_ADD(ch) ((ch)->add_abils.dex_add)
#define GET_REAL_DEX(ch) (POSI(GET_DEX(ch)+GET_DEX_ADD(ch)))
#define GET_INT(ch)     ((ch)->real_abils.intel)
#define GET_INT_ADD(ch) ((ch)->add_abils.intel_add)
#define GET_REAL_INT(ch) (POSI(GET_INT(ch) + GET_INT_ADD(ch)))
#define GET_WIS(ch)     ((ch)->real_abils.wis)
#define GET_WIS_ADD(ch) ((ch)->add_abils.wis_add)
#define GET_REAL_WIS(ch) (POSI(GET_WIS(ch) + GET_WIS_ADD(ch)))
#define GET_CON(ch)     ((ch)->real_abils.con)
#define GET_CON_ADD(ch) ((ch)->add_abils.con_add)
#define GET_REAL_CON(ch) (POSI(GET_CON(ch) + GET_CON_ADD(ch)))
#define GET_CHA(ch)     ((ch)->real_abils.cha)
#define GET_CHA_ADD(ch) ((ch)->add_abils.cha_add)
#define GET_REAL_CHA(ch) (POSI(GET_CHA(ch) + GET_CHA_ADD(ch)))
#define GET_SIZE(ch)    ((ch)->real_abils.size)
#define GET_SIZE_ADD(ch)  ((ch)->add_abils.size_add)
#define GET_REAL_SIZE(ch) (VPOSI(GET_SIZE(ch) + GET_SIZE_ADD(ch), 1, 100))
#define GET_POS_SIZE(ch)  (POSI(GET_REAL_SIZE(ch) >> 1))
#define GET_HR(ch)         ((ch)->real_abils.hitroll)
#define GET_HR_ADD(ch)    ((ch)->add_abils.hr_add)
#define GET_REAL_HR(ch)   (VPOSI(GET_HR(ch)+GET_HR_ADD(ch), -50, 50))
#define GET_DR(ch)         ((ch)->real_abils.damroll)
#define GET_DR_ADD(ch)    ((ch)->add_abils.dr_add)
#define GET_REAL_DR(ch)   (VPOSI(GET_DR(ch)+GET_DR_ADD(ch), -50, 50))
#define GET_AC(ch)         ((ch)->real_abils.armor)
#define GET_AC_ADD(ch)    ((ch)->add_abils.ac_add)
#define GET_REAL_AC(ch)      (GET_AC(ch)+GET_AC_ADD(ch))
#define GET_MORALE(ch)       ((ch)->add_abils.morale_add)
#define GET_INITIATIVE(ch)   ((ch)->add_abils.initiative_add)
#define GET_POISON(ch)      ((ch)->add_abils.poison_add)
#define GET_CAST_SUCCESS(ch) ((ch)->add_abils.cast_success)
#define GET_PRAY(ch)         ((ch)->add_abils.pray_add)

#define GET_EXP(ch)            ((ch)->points.exp)
#define GET_HIT(ch)            ((ch)->points.hit)
#define GET_MAX_HIT(ch)       ((ch)->points.max_hit)
#define GET_REAL_MAX_HIT(ch)  (GET_MAX_HIT(ch) + GET_HIT_ADD(ch))
#define GET_MOVE(ch)       ((ch)->points.move)
#define GET_MAX_MOVE(ch)      ((ch)->points.max_move)
#define GET_REAL_MAX_MOVE(ch) (GET_MAX_MOVE(ch) + GET_MOVE_ADD(ch))
#define GET_GOLD(ch)       ((ch)->points.gold)
#define GET_BANK_GOLD(ch)     ((ch)->points.bank_gold)


#define GET_MANAREG(ch)   ((ch)->add_abils.manareg)
#define GET_HITREG(ch)    ((ch)->add_abils.hitreg)
#define GET_MOVEREG(ch)   ((ch)->add_abils.movereg)
#define GET_ARMOUR(ch)    ((ch)->add_abils.armour)
#define GET_ABSORBE(ch)   ((ch)->add_abils.absorb)
#define GET_AGE_ADD(ch)   ((ch)->add_abils.age_add)
#define GET_HIT_ADD(ch)   ((ch)->add_abils.hit_add)
#define GET_MOVE_ADD(ch)  ((ch)->add_abils.move_add)
#define GET_SLOT(ch,i)    ((ch)->add_abils.slot_add[i])
#define GET_SAVE(ch,i)    ((ch)->add_abils.apply_saving_throw[i])
#define GET_RESIST(ch,i)  ((ch)->add_abils.apply_resistance_throw[i])
#define GET_AR(ch)        ((ch)->add_abils.aresist)
#define GET_MR(ch)        ((ch)->add_abils.mresist)
#define GET_CASTER(ch)    ((ch)->CasterLevel)
#define GET_DAMAGE(ch)    ((ch)->DamageLevel)
#define GET_LIKES(ch)     ((ch)->mob_specials.LikeWork)

#define GET_POS(ch)        ((ch)->char_specials.position)
#define GET_IDNUM(ch)     ((ch)->char_specials.saved.idnum)
#define GET_ID(x)         ((x)->id)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define FIGHTING(ch)   ((ch)->char_specials.fighting)
#define HUNTING(ch)        ((ch)->char_specials.hunting)
#define PROTECTING(ch)    ((ch)->Protecting)
#define TOUCHING(ch)   ((ch)->Touching)

/*Макросы доступа к полям параметров комнат*/
#define GET_ROOM_BASE_POISON(room) ((room)->base_property.poison)
#define GET_ROOM_ADD_POISON(room) ((room)->add_property.poison)
#define GET_ROOM_POISON(room) (GET_ROOM_BASE_POISON(room)+GET_ROOM_ADD_POISON(room))

/* Получение кубиков урона - работает только для мобов! */
#define GET_NDD(ch) ((ch)->mob_specials.damnodice)
#define GET_SDD(ch) ((ch)->mob_specials.damsizedice)

#define ALIG_NEUT 0
#define ALIG_GOOD 1
#define ALIG_EVIL 2
#define ALIG_EVIL_LESS     -300
#define ALIG_GOOD_MORE     300

#define GET_ALIGNMENT(ch)     ((ch)->char_specials.saved.alignment)
#define CALC_ALIGNMENT(ch)    ((GET_ALIGNMENT(ch) <= ALIG_EVIL_LESS) ? ALIG_EVIL : \
                               (GET_ALIGNMENT(ch) >= ALIG_GOOD_MORE) ? ALIG_GOOD : ALIG_NEUT)

#define NAME_LEVEL 5
#define NAME_FINE(ch)          (NAME_GOD(ch)>1000)
#define NAME_BAD(ch)           (NAME_GOD(ch)<1000 && NAME_GOD(ch))
/*
#define OK_GAIN_EXP(ch,victim) (!NAME_BAD(ch) && \
                                (NAME_FINE(ch) || !(GET_LEVEL(ch)==NAME_LEVEL)) && \
                                !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) && \
                                ((IS_NPC(ch) ? \
                                  (IS_NPC(victim) ? 1 : 0) : \
                                  (IS_NPC(victim) && CALC_ALIGNMENT(victim) != ALIG_GOOD ? 1 : 0) \
                                 ) \
                                ) \
                               )
*/
#define OK_GAIN_EXP(ch,victim) (!NAME_BAD(ch) &&                                     \
                                (NAME_FINE(ch) || !(GET_LEVEL(ch)==NAME_LEVEL)) &&   \
                                !ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) &&            \
            (IS_NPC(victim) && (GET_EXP(victim) > 0)) &&         \
                                (!IS_NPC(victim)||                                   \
                                 CALC_ALIGNMENT(victim)==ALIG_GOOD ? 0 :             \
                                 (IS_NPC(ch)&&!AFF_FLAGGED(ch,AFF_CHARM)?0:1)        \
                                )                                                    \
                               )


#define MAX_EXP_PERCENT   80

#define GET_COND(ch, i)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.conditions[(i)]))
#define GET_LOADROOM(ch)   CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room))
#define GET_INVIS_LEV(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.invis_level))
#define GET_WIMP_LEV(ch)   CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.wimp_level))
#define GET_BAD_PWS(ch)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bad_pws))
#define GET_TALK(ch, i)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.talks[i]))
#define POOFIN(ch)              CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)     CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define RENTABLE(ch)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->may_rent))
#define AGRESSOR(ch)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->agressor))
#define AGRO(ch)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->agro_time))

#define EXCHANGE_FILTER(ch)     CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->Exchange_filter))
#define IGNORE_LIST(ch)     CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->ignores))

#define GET_BET(ch)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->bet))
#define GET_BET_SLOT(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->bet_slot))

#define GET_LAST_OLC_TARG(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
#define GET_LAST_ALL_TELL(ch)   CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->LastAllTell))
#define GET_RSKILL(ch)          CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->rskill))
#define GET_PORTALS(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->portals))
#define GET_LOGS(ch)            CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->logs))

// Punishments structs
#define MUTE_REASON(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pmute.reason))
#define DUMB_REASON(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pdumb.reason))
#define HELL_REASON(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->phell.reason))
#define FREEZE_REASON(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pfreeze.reason))
#define GCURSE_REASON(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pgcurse.reason))
#define NAME_REASON(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pname.reason))
#define UNREG_REASON(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->punreg.reason))

#define GET_MUTE_LEV(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pmute.level))
#define GET_DUMB_LEV(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pdumb.level))
#define GET_HELL_LEV(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->phell.level))
#define GET_FREEZE_LEV(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pfreeze.level))
#define GET_GCURSE_LEV(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pgcurse.level))
#define GET_NAME_LEV(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pname.level))
#define GET_UNREG_LEV(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->punreg.level))

#define MUTE_GODID(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pmute.godid))
#define DUMB_GODID(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pdumb.godid))
#define HELL_GODID(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->phell.godid))
#define FREEZE_GODID(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pfreeze.godid))
#define GCURSE_GODID(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pgcurse.godid))
#define NAME_GODID(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pname.godid))
#define UNREG_GODID(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->punreg.godid))

#define GCURSE_DURATION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pgcurse.duration))
#define MUTE_DURATION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pmute.duration))
#define DUMB_DURATION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pdumb.duration))
#define FREEZE_DURATION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pfreeze.duration))
#define HELL_DURATION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->phell.duration))
#define NAME_DURATION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pname.duration))
#define UNREG_DURATION(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->punreg.duration))

#define KARMA(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->Karma))
#define LOGON_LIST(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->logons))

#define CLAN(ch)              CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->clan))
#define CLAN_MEMBER(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->clan_member))
#define GET_CLAN_STATUS(ch)   CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->clanStatus))
#define GET_BOARD_DATE(ch, i) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->board_date[i]))
#define GET_START_STAT(ch, i) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->start_stats[i]))

#define SET_SKILL(ch, i, pct) ((ch)->real_abils.Skills[i] = pct)
#define GET_SPELL_TYPE(ch, i) ((ch)->real_abils.SplKnw[i])
#define GET_SPELL_MEM(ch, i)  ((ch)->real_abils.SplMem[i])
#define SET_SPELL(ch, i, pct) ((ch)->real_abils.SplMem[i] = pct)
#define SET_FEAT(ch, feat) ((ch)->real_abils.Feats.set(feat))
#define UNSET_FEAT(ch, feat) ((ch)->real_abils.Feats.reset(feat))
#define HAVE_FEAT(ch, feat) ((ch)->real_abils.Feats.test(feat))
#define	NUM_LEV_FEAT(ch) (MIN (MAX_ACC_FEAT, (GET_LEVEL(ch) / LEV_ACC_FEAT) + 1))
#define FEAT_SLOT(ch, feat) (feat_info[feat].min_level[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] / LEV_ACC_FEAT)

// Min cast level getting
#define MIN_CAST_LEV(sp, ch) (MMAX(0,MOD_CAST_LEV(sp,ch)))
#define MOD_CAST_LEV(sp, ch) (BASE_CAST_LEV(sp, ch) - (MMAX(GET_REMORT(ch) - MIN_CAST_REM(sp,ch),0) / 3))
#define BASE_CAST_LEV(sp, ch) ((sp).min_level[(int) GET_CLASS (ch)][(int) GET_KIN (ch)])

#define MIN_CAST_REM(sp, ch) ((sp).min_remort[(int) GET_CLASS (ch)][(int) GET_KIN (ch)])

#define GET_EQ(ch, i)      ((ch)->equipment[i])

#define GET_MOB_SPEC(ch)   (IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)
#define GET_MOB_RNUM(mob)  ((mob)->nr)
#define GET_MOB_VNUM(mob)  (IS_MOB(mob) ? \
             mob_index[GET_MOB_RNUM(mob)].vnum : -1)

#define GET_DEFAULT_POS(ch)   ((ch)->mob_specials.default_pos)
#define MEMORY(ch)          ((ch)->mob_specials.memory)
#define GET_DEST(ch)        (((ch)->mob_specials.dest_count ? \
                              (ch)->mob_specials.dest[(ch)->mob_specials.dest_pos] : \
                              NOWHERE))
#define GET_ACTIVITY(ch)    ((ch)->mob_specials.activity)
#define GET_GOLD_NoDs(ch)   ((ch)->mob_specials.GoldNoDs)
#define GET_GOLD_SiDs(ch)   ((ch)->mob_specials.GoldSiDs)
#define GET_HORSESTATE(ch)  ((ch)->mob_specials.HorseState)
#define GET_LASTROOM(ch)    ((ch)->mob_specials.LastRoom)


#define STRENGTH_APPLY_INDEX(ch) \
        ( GET_REAL_STR(ch) )

#define CAN_CARRY_W(ch) ((str_app[STRENGTH_APPLY_INDEX(ch)].carry_w * (HAVE_FEAT(ch, PORTER_FEAT) ? 110 : 100))/100)
#define CAN_CARRY_N(ch) (5 + (GET_REAL_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1) \
			 + (HAVE_FEAT(ch, JUGGLER_FEAT) ? (GET_LEVEL(ch) >> 1) : 0))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING && !AFF_FLAGGED(ch,AFF_SLEEP))
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

#define IS_GOOD(ch)          (GET_ALIGNMENT(ch) >= ALIG_GOOD_MORE)
#define IS_EVIL(ch)          (GET_ALIGNMENT(ch) <= ALIG_EVIL_LESS)
#define IS_NEUTRAL(ch)        (!IS_GOOD(ch) && !IS_EVIL(ch))
/*
#define SAME_ALIGN(ch,vict)  ((IS_GOOD(ch) && IS_GOOD(vict)) ||\
                              (IS_EVIL(ch) && IS_EVIL(vict)) ||\
               (IS_NEUTRAL(ch) && IS_NEUTRAL(vict)))
*/
#define ALIGN_DELTA  10
#define SAME_ALIGN(ch,vict)  (GET_ALIGNMENT(ch)>GET_ALIGNMENT(vict)?\
                              (GET_ALIGNMENT(ch)-GET_ALIGNMENT(vict))<=ALIGN_DELTA:\
                              (GET_ALIGNMENT(vict)-GET_ALIGNMENT(ch))<=ALIGN_DELTA\
                             )
#define GET_CH_SUF_1(ch) (IS_NOSEXY(ch) ? "о" :\
                          IS_MALE(ch) ? ""  :\
                          IS_FEMALE(ch) ? "а" : "и")
#define GET_CH_SUF_2(ch) (IS_NOSEXY(ch) ? "ось" :\
                          IS_MALE(ch) ? "ся"  :\
                          IS_FEMALE(ch) ? "ась" : "ись")
#define GET_CH_SUF_3(ch) (IS_NOSEXY(ch) ? "ое" :\
                          IS_MALE(ch) ? "ый"  :\
                          IS_FEMALE(ch) ? "ая" : "ые")
#define GET_CH_SUF_4(ch) (IS_NOSEXY(ch) ? "ло" :\
                          IS_MALE(ch) ? ""  :\
                          IS_FEMALE(ch) ? "ла" : "ли")
#define GET_CH_SUF_5(ch) (IS_NOSEXY(ch) ? "ло" :\
                          IS_MALE(ch) ? "ел"  :\
                          IS_FEMALE(ch) ? "ла" : "ли")
#define GET_CH_SUF_6(ch) (IS_NOSEXY(ch) ? "о" :\
                          IS_MALE(ch) ? ""  :\
                          IS_FEMALE(ch) ? "а" : "ы")

#define GET_CH_VIS_SUF_1(ch,och) (!CAN_SEE(och,ch) ? "" :\
                          IS_NOSEXY(ch) ? "о" :\
                          IS_MALE(ch) ? ""  :\
                          IS_FEMALE(ch) ? "а" : "и")
#define GET_CH_VIS_SUF_2(ch,och) (!CAN_SEE(och,ch) ? "ся" :\
                          IS_NOSEXY(ch) ? "ось" :\
                          IS_MALE(ch) ? "ся"  :\
                          IS_FEMALE(ch) ? "ась" : "ись")
#define GET_CH_VIS_SUF_3(ch,och) (!CAN_SEE(och,ch) ? "ый" :\
                          IS_NOSEXY(ch) ? "ое" :\
                          IS_MALE(ch) ? "ый"  :\
                          IS_FEMALE(ch) ? "ая" : "ые")
#define GET_CH_VIS_SUF_4(ch,och) (!CAN_SEE(och,ch) ? "" :\
                          IS_NOSEXY(ch) ? "ло" :\
                          IS_MALE(ch) ? ""  :\
                          IS_FEMALE(ch) ? "ла" : "ли")
#define GET_CH_VIS_SUF_5(ch,och) (!CAN_SEE(och,ch) ? "ел" :\
                          IS_NOSEXY(ch) ? "ло" :\
                          IS_MALE(ch) ? "ел"  :\
                          IS_FEMALE(ch) ? "ла" : "ли")
#define GET_CH_VIS_SUF_6(ch,och) (!CAN_SEE(och,ch) ? "" :\
                          IS_NOSEXY(ch) ? "о" :\
                          IS_MALE(ch) ? ""  :\
                          IS_FEMALE(ch) ? "а" : "ы")


#define GET_OBJ_SEX(obj) ((obj)->obj_flags.Obj_sex)
#define IS_OBJ_NOSEXY(obj)    (GET_OBJ_SEX(obj) == SEX_NEUTRAL)
#define IS_OBJ_MALE(obj)   (GET_OBJ_SEX(obj) == SEX_MALE)
#define IS_OBJ_FEMALE(obj)    (GET_OBJ_SEX(obj) == SEX_FEMALE)

#define GET_OBJ_MIW(obj) ((obj)->max_in_world)

#define GET_OBJ_SUF_1(obj) (IS_OBJ_NOSEXY(obj) ? "о" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "а" : "и")
#define GET_OBJ_SUF_2(obj) (IS_OBJ_NOSEXY(obj) ? "ось" :\
                            IS_OBJ_MALE(obj) ? "ся"  :\
                            IS_OBJ_FEMALE(obj) ? "ась" : "ись")
#define GET_OBJ_SUF_3(obj) (IS_OBJ_NOSEXY(obj) ? "ое" :\
                            IS_OBJ_MALE(obj) ? "ый"  :\
                            IS_OBJ_FEMALE(obj) ? "ая" : "ые")
#define GET_OBJ_SUF_4(obj) (IS_OBJ_NOSEXY(obj) ? "ло" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "ла" : "ли")
#define GET_OBJ_SUF_5(obj) (IS_OBJ_NOSEXY(obj) ? "ло" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "ла" : "ли")
#define GET_OBJ_SUF_6(obj) (IS_OBJ_NOSEXY(obj) ? "о" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "а" : "ы")
#define GET_OBJ_SUF_7(obj) (IS_OBJ_NOSEXY(obj) ? "е" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "а" : "и")


#define GET_OBJ_VIS_SUF_1(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "о" :\
                            IS_OBJ_NOSEXY(obj) ? "о" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "а" : "и")
#define GET_OBJ_VIS_SUF_2(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "ось" :\
                            IS_OBJ_NOSEXY(obj) ? "ось" :\
                            IS_OBJ_MALE(obj) ? "ся"  :\
                            IS_OBJ_FEMALE(obj) ? "ась" : "ись")
#define GET_OBJ_VIS_SUF_3(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "ый" :\
                            IS_OBJ_NOSEXY(obj) ? "ое" :\
                            IS_OBJ_MALE(obj) ? "ый"  :\
                            IS_OBJ_FEMALE(obj) ? "ая" : "ые")
#define GET_OBJ_VIS_SUF_4(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "ло" :\
                            IS_OBJ_NOSEXY(obj) ? "ло" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "ла" : "ли")
#define GET_OBJ_VIS_SUF_5(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "ло" :\
                            IS_OBJ_NOSEXY(obj) ? "ло" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "ла" : "ли")
#define GET_OBJ_VIS_SUF_6(obj,ch) (!CAN_SEE_OBJ(ch,obj) ? "о" :\
                            IS_OBJ_NOSEXY(obj) ? "о" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "а" : "ы")

#define GET_OBJ_POLY_1(ch, obj) ((GET_OBJ_SEX(obj) == SEX_POLY) ? "ят" : "ит")
#define GET_OBJ_VIS_POLY_1(ch, obj) (!CAN_SEE_OBJ(ch,obj) ? "ит" : (GET_OBJ_SEX(obj) == SEX_POLY) ? "ят" : "ит")

/* These three deprecated. */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
#define PUNCTUAL_WAIT_STATE(ch, cycle) do { GET_PUNCTUAL_WAIT_STATE(ch) = (cycle); } while(0)
#define CHECK_WAIT(ch)        ((ch)->wait > 0)
#define CHECK_PUNCTUAL_WAIT(ch)        ((ch)->punctual_wait > 0)
#define GET_WAIT(ch)          GET_WAIT_STATE(ch)
#define GET_PUNCTUAL_WAIT(ch)          GET_PUNCTUAL_WAIT_STATE(ch)
#define GET_MOB_HOLD(ch)      (AFF_FLAGGED((ch),AFF_HOLD) ? 1 : 0)
#define GET_MOB_SIELENCE(ch)  (AFF_FLAGGED((ch),AFF_SIELENCE) ? 1 : 0)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch)    ((ch)->wait)
#define GET_PUNCTUAL_WAIT_STATE(ch)    ((ch)->punctual_wait)


/* descriptor-based utils ************************************************/

/* Hrm, not many.  We should make more. -gg 3/4/99 */
#define STATE(d)  ((d)->connected)


/* object utils **********************************************************/
#define GET_OBJ_UID(obj)	((obj)->uid)

#define GET_OBJ_ALIAS(obj)      ((obj)->short_description)
#define GET_OBJ_PNAME(obj,pad)  ((obj)->PNames[pad])
#define GET_OBJ_DESC(obj)       ((obj)->description)
#define GET_OBJ_SPELL(obj)      ((obj)->obj_flags.Obj_spell)
#define GET_OBJ_LEVEL(obj)      ((obj)->obj_flags.Obj_level)
#define GET_OBJ_AFFECTS(obj)    ((obj)->obj_flags.affects)
#define GET_OBJ_ANTI(obj)       ((obj)->obj_flags.anti_flag)
#define GET_OBJ_NO(obj)         ((obj)->obj_flags.no_flag)
#define GET_OBJ_ACT(obj)        ((obj)->action_description)
#define GET_OBJ_POS(obj)        ((obj)->obj_flags.worn_on)
#define GET_OBJ_TYPE(obj)  ((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)  ((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)  ((obj)->obj_flags.cost_per_day_off)
#define GET_OBJ_RENTEQ(obj)   ((obj)->obj_flags.cost_per_day_on)
#define GET_OBJ_EXTRA(obj,flag)  (GET_FLAG((obj)->obj_flags.extra_flags,flag))
#define GET_OBJ_AFF(obj,flag) (GET_FLAG((obj)->obj_flags.affects,flag))
#define GET_OBJ_WEAR(obj)  ((obj)->obj_flags.wear_flags)
#define GET_OBJ_OWNER(obj)      ((obj)->obj_flags.Obj_owner)
#define GET_OBJ_MAKER(obj)      ((obj)->obj_flags.Obj_maker)
#define GET_OBJ_PARENT(obj)      ((obj)->obj_flags.Obj_parent)
#define GET_OBJ_VAL(obj, val) ((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEIGHT(obj)   ((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj) ((obj)->obj_flags.Obj_timer)
#define GET_OBJ_DESTROY(obj) ((obj)->obj_flags.Obj_destroyer)
#define GET_OBJ_SKILL(obj) ((obj)->obj_flags.Obj_skill)
#define GET_OBJ_CUR(obj)    ((obj)->obj_flags.Obj_cur)
#define GET_OBJ_MAX(obj)    ((obj)->obj_flags.Obj_max)
#define GET_OBJ_MATER(obj)  ((obj)->obj_flags.Obj_mater)
#define GET_OBJ_ZONE(obj)   ((obj)->obj_flags.Obj_zone)
#define GET_OBJ_RNUM(obj)  ((obj)->item_number)
#define OBJ_GET_LASTROOM(obj) ((obj)->room_was_in)
#define GET_OBJ_VNUM(obj)  (GET_OBJ_RNUM(obj) >= 0 ? \
             obj_index[GET_OBJ_RNUM(obj)].vnum : -1)
#define OBJ_WHERE(obj) ((obj)->worn_by    ? IN_ROOM(obj->worn_by) : \
                        (obj)->carried_by ? IN_ROOM(obj->carried_by) : (obj)->in_room)
#define IS_OBJ_STAT(obj,stat) (IS_SET(GET_FLAG((obj)->obj_flags.extra_flags, \
                                                  stat), stat))
#define IS_OBJ_ANTI(obj,stat) (IS_SET(GET_FLAG((obj)->obj_flags.anti_flag, \
                                                  stat), stat))
#define IS_OBJ_NO(obj,stat)       (IS_SET(GET_FLAG((obj)->obj_flags.no_flag, \
                                                  stat), stat))
#define IS_OBJ_AFF(obj,stat)    (IS_SET(GET_FLAG((obj)->obj_flags.affects, \
                                                  stat), stat))

#define IS_CORPSE(obj)     (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
               GET_OBJ_VAL((obj), 3) == 1)

#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? \
   (obj_index[(obj)->item_number].func) : NULL)

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))
#define CAN_WEAR_ANY(obj) (((obj)->obj_flags.wear_flags>0) && ((obj)->obj_flags.wear_flags != ITEM_WEAR_TAKE) )

/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
   (((major) << 16) + ((minor) << 8) + (patchlevel))

#define HSHR(ch) (GET_SEX(ch) ? (IS_MALE(ch) ? "его": (IS_FEMALE(ch) ? "ее" : "их")) :"его")
#define HSSH(ch) (GET_SEX(ch) ? (IS_MALE(ch) ? "он": (IS_FEMALE(ch) ? "она" : "они")) :"оно")
#define HMHR(ch) (GET_SEX(ch) ? (IS_MALE(ch) ? "ему": (IS_FEMALE(ch) ? "ей" : "им")) :"ему")
#define HYOU(ch) (GET_SEX(ch) ? (IS_MALE(ch) ? "Ваш": (IS_FEMALE(ch) ? "Ваша" : (IS_NOSEXY(ch) ? "Ваше": "Ваши"))) :"Ваш")

#define OSHR(ch) (GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch)==SEX_MALE ? "его": (GET_OBJ_SEX(ch) == SEX_FEMALE ? "ее" : "их")) :"его")
#define OSSH(ch) (GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch)==SEX_MALE ? "он": (GET_OBJ_SEX(ch) == SEX_FEMALE ? "она" : "они")) :"оно")
#define OMHR(ch) (GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch)==SEX_MALE ? "ему": (GET_OBJ_SEX(ch) == SEX_FEMALE ? "ей" : "им")) :"ему")
#define OYOU(ch) (GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch)==SEX_MALE ? "Ваш": (GET_OBJ_SEX(ch) == SEX_FEMALE ? "Ваша" : "Ваши")) :"Ваше")


/* Various macros building up to CAN_SEE */
#define MAY_SEE(sub,obj) (!AFF_FLAGGED((sub),AFF_BLIND) && \
                          (!IS_DARK(IN_ROOM(sub)) || AFF_FLAGGED((sub),AFF_INFRAVISION)) && \
           (!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED((sub),AFF_DETECT_INVIS)))

#define MAY_ATTACK(sub)  (!AFF_FLAGGED((sub),AFF_CHARM)     && \
                          !IS_HORSE((sub))                  && \
           !AFF_FLAGGED((sub),AFF_STOPFIGHT)      && \
           !AFF_FLAGGED((sub),AFF_MAGICSTOPFIGHT) && \
           !AFF_FLAGGED((sub),AFF_HOLD)           && \
           !MOB_FLAGGED((sub),MOB_NOFIGHT)        && \
           GET_WAIT(sub) <= 0                     && \
           !FIGHTING(sub)                         && \
           GET_POS(sub) >= POS_RESTING)

#define LIGHT_OK(sub)   (!AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT((sub)->in_room) || AFF_FLAGGED((sub), AFF_INFRAVISION)))

#define INVIS_OK(sub, obj) \
 (!AFF_FLAGGED((sub), AFF_BLIND) && \
  ((!AFF_FLAGGED((obj),AFF_INVISIBLE) || \
    AFF_FLAGGED((sub),AFF_DETECT_INVIS) \
   ) && \
   ((!AFF_FLAGGED((obj), AFF_HIDE) && !AFF_FLAGGED((obj), AFF_CAMOUFLAGE)) || \
    AFF_FLAGGED((sub), AFF_SENSE_LIFE) \
   ) \
  ) \
 )

#define HERE(ch)  ((IS_NPC(ch) || (ch)->desc || RENTABLE(ch)))

#define MORT_CAN_SEE(sub, obj) (HERE(obj) && \
                                INVIS_OK(sub,obj) && \
                                (IS_LIGHT((obj)->in_room) || \
                                 AFF_FLAGGED((sub), AFF_INFRAVISION) \
                                ) \
                               )

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
    IMM_CAN_SEE(sub, obj)))

/* Can subject see character "obj" without light */
#define MORT_CAN_SEE_CHAR(sub, obj) (HERE(obj) && \
                                     INVIS_OK(sub,obj) \
                )

#define IMM_CAN_SEE_CHAR(sub, obj) \
        (MORT_CAN_SEE_CHAR(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define CAN_SEE_CHAR(sub, obj) (IS_CODER(sub) || SELF(sub, obj) || \
        ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
         IMM_CAN_SEE_CHAR(sub, obj)))


/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) \
  (!IS_OBJ_STAT((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */

#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) && \
   (!obj->worn_by    || CAN_SEE(sub, obj->worn_by)))
/*
#define MORT_CAN_SEE_OBJ(sub, obj) \
  (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))
 */

#define MORT_CAN_SEE_OBJ(sub, obj) \
  (INVIS_OK_OBJ(sub, obj) && !AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT(IN_ROOM(sub)) || OBJ_FLAGGED(obj, ITEM_GLOW) || \
    (IS_CORPSE(obj) && AFF_FLAGGED(sub, AFF_INFRAVISION))))

#define CAN_SEE_OBJ(sub, obj) \
   (obj->worn_by    == sub || \
    obj->carried_by == sub || \
    (obj->in_obj && (obj->in_obj->worn_by == sub || obj->in_obj->carried_by == sub)) || \
    MORT_CAN_SEE_OBJ(sub, obj) || \
    (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

#define OK_BOTH(ch,obj)  (GET_OBJ_WEIGHT(obj) <= \
                          str_app[STRENGTH_APPLY_INDEX(ch)].wield_w + str_app[STRENGTH_APPLY_INDEX(ch)].hold_w)

#define OK_WIELD(ch,obj) (GET_OBJ_WEIGHT(obj) <= \
                          str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)

#define OK_HELD(ch,obj)  (GET_OBJ_WEIGHT(obj) <= \
                          str_app[STRENGTH_APPLY_INDEX(ch)].hold_w)

#define OK_SHIELD(ch,obj)  (GET_OBJ_WEIGHT(obj) <= \
                          (2 * str_app[STRENGTH_APPLY_INDEX(ch)].hold_w))

#define GET_PAD_PERS(pad) ((pad) == 5 ? "ком-то" :\
                           (pad) == 4 ? "кем-то" :\
                           (pad) == 3 ? "кого-то" :\
                           (pad) == 2 ? "кому-то" :\
                           (pad) == 1 ? "кого-то" : "кто-то")

#define PERS(ch,vict,pad) (CAN_SEE(vict, ch) ? GET_PAD(ch,pad) : GET_PAD_PERS(pad))

#define OBJS(obj,vict) (CAN_SEE_OBJ((vict), (obj)) ? \
                      (obj)->short_description  : "что-то")

#define GET_PAD_OBJ(pad)  ((pad) == 5 ? "чем-то" :\
                           (pad) == 4 ? "чем-то" :\
                           (pad) == 3 ? "что-то" :\
                           (pad) == 2 ? "чему-то" :\
                           (pad) == 1 ? "чего-то" : "что-то")

#define OBJ_PAD(obj,pad)  ((obj)->PNames[pad])

#define OBJN(obj,vict,pad) (CAN_SEE_OBJ((vict), (obj)) ? \
                           ((obj)->PNames[pad]) ? (obj)->PNames[pad] : (obj)->short_description \
                           : GET_PAD_OBJ(pad))

#define EXITDATA(room,door) ((room >= 0 && room <= top_of_world) ? \
                             world[room]->dir_option[door] : NULL)

#define EXIT(ch, door)  (world[(ch)->in_room]->dir_option[door])

#define CAN_GO(ch, door) (ch?((EXIT(ch,door) && \
          (EXIT(ch,door)->to_room != NOWHERE) && \
          !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))):0)


#define CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : class_abbrevs[(int)GET_CLASS(ch)])

#define KIN_ABBR(ch) (IS_NPC(ch) ? "--" : kin_abbrevs[(int)GET_KIN(ch)])

#define IS_MAGIC_USER(ch)  (!IS_NPC(ch) && \
            (IS_BITS(MASK_MAGES, (int) GET_CLASS(ch))))
#define IS_CLERIC(ch)      (!IS_NPC(ch) && \
            ((int) GET_CLASS(ch) == CLASS_CLERIC))
#define IS_THIEF(ch)    (!IS_NPC(ch) && \
            ((int) GET_CLASS(ch) == CLASS_THIEF))
#define IS_ASSASINE(ch)    (!IS_NPC(ch) && \
            ((int) GET_CLASS(ch) == CLASS_ASSASINE))
#define IS_WARRIOR(ch)     (!IS_NPC(ch) && \
            (GET_CLASS(ch) == CLASS_WARRIOR))
#define IS_PALADINE(ch)    (!IS_NPC(ch) && \
            (GET_CLASS(ch) == CLASS_PALADINE))
#define IS_RANGER(ch)      (!IS_NPC(ch) && \
            (GET_CLASS(ch) == CLASS_RANGER))
#define IS_GUARD(ch)    (!IS_NPC(ch) && \
            ((int) GET_CLASS(ch) == CLASS_GUARD))
#define IS_SMITH(ch)    (!IS_NPC(ch) && \
            (GET_CLASS(ch) == CLASS_SMITH))
#define IS_MERCHANT(ch)    (!IS_NPC(ch) && \
            (GET_CLASS(ch) == CLASS_MERCHANT))
#define IS_DRUID(ch)    (!IS_NPC(ch) && \
            ((int) GET_CLASS(ch) == CLASS_DRUID))
#define IS_BATTLEMAGE(ch)  (!IS_NPC(ch) && \
            ((int) GET_CLASS(ch) == CLASS_BATTLEMAGE))
#define IS_CHARMMAGE(ch)   (!IS_NPC(ch) && \
            ((int) GET_CLASS(ch) == CLASS_CHARMMAGE))
#define IS_DEFENDERMAGE(ch)   (!IS_NPC(ch) && \
            ((int) GET_CLASS(ch) == CLASS_DEFENDERMAGE))
#define IS_NECROMANCER(ch) (!IS_NPC(ch) && \
            ((int) GET_CLASS(ch) == CLASS_NECROMANCER))

#define IS_CHARMICE(ch)    (IS_NPC(ch) && (AFF_FLAGGED(ch,AFF_HELPER) || AFF_FLAGGED(ch,AFF_CHARM)))

#define IS_UNDEAD(ch) (IS_NPC(ch) && \
		(MOB_FLAGGED(ch, MOB_RESURRECTED) || (GET_CLASS(ch) == CLASS_UNDEAD)))

#define LIKE_ROOM(ch) ((IS_CLERIC(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_CLERIC)) || \
                       (IS_MAGIC_USER(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_MAGE)) || \
                       (IS_WARRIOR(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_WARRIOR)) || \
                       (IS_THIEF(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_THIEF)) || \
                       (IS_ASSASINE(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_ASSASINE)) || \
                       (IS_GUARD(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_GUARD)) || \
                       (IS_PALADINE(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_PALADINE)) || \
                       (IS_RANGER(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_RANGER)) || \
                       (IS_SMITH(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_SMITH)) || \
                       (IS_MERCHANT(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_MERCHANT)) || \
                       (IS_DRUID(ch) && ROOM_FLAGGED((ch)->in_room, ROOM_DRUID)))

#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS))
#define IS_HORSE(ch) (IS_NPC(ch) && (ch->master) && AFF_FLAGGED(ch, AFF_HORSE))
int on_horse(CHAR_DATA * ch);
int has_horse(CHAR_DATA * ch, int same_room);
CHAR_DATA *get_horse(CHAR_DATA * ch);
void horse_drop(CHAR_DATA * ch);
void make_horse(CHAR_DATA * horse, CHAR_DATA * ch);
void check_horse(CHAR_DATA * ch);

int same_group(CHAR_DATA * ch, CHAR_DATA * tch);

int is_post(room_rnum room);
int is_rent(room_rnum room);

char *noclan_title(CHAR_DATA * ch);
char *only_title(CHAR_DATA * ch);
char *race_or_title(CHAR_DATA * ch);
char *race_or_title_enl(CHAR_DATA * ch);
int pc_duration(CHAR_DATA * ch, int cnst, int level, int level_divisor, int min, int max);

/* Modifier functions */
int god_spell_modifier(CHAR_DATA * ch, int spellnum, int type, int value);
int day_spell_modifier(CHAR_DATA * ch, int spellnum, int type, int value);
int weather_spell_modifier(CHAR_DATA * ch, int spellnum, int type, int value);
int complex_spell_modifier(CHAR_DATA * ch, int spellnum, int type, int value);

int god_skill_modifier(CHAR_DATA * ch, int skillnum, int type, int value);
int day_skill_modifier(CHAR_DATA * ch, int skillnum, int type, int value);
int weather_skill_modifier(CHAR_DATA * ch, int skillnum, int type, int value);
int complex_skill_modifier(CHAR_DATA * ch, int skillnum, int type, int value);
void can_carry_obj(CHAR_DATA * ch, OBJ_DATA * obj);
bool ignores(CHAR_DATA *, CHAR_DATA *, unsigned int);

/* PADS for something ****************************************************/
char *desc_count(int how_many, int of_what);
#define WHAT_DAY	0
#define WHAT_HOUR	1
#define WHAT_YEAR	2
#define WHAT_POINT	3
#define WHAT_MINa	4
#define WHAT_MINu	5
#define WHAT_MONEYa	6
#define WHAT_MONEYu	7
#define WHAT_THINGa	8
#define WHAT_THINGu	9
#define WHAT_LEVEL	10
#define WHAT_MOVEa	11
#define WHAT_MOVEu	12
#define WHAT_ONEa	13
#define WHAT_ONEu	14
#define WHAT_SEC	15
#define WHAT_DEGREE	16
#define WHAT_ROW	17
#define WHAT_OBJECT	18
#define WHAT_REMORT	19
#define WHAT_WEEK	20
#define WHAT_MONTH	21

#undef AW_HIDE // конфликтует с winuser.h
/* some awaking cases */
#define AW_HIDE       (1 << 0)
#define AW_INVIS      (1 << 1)
#define AW_CAMOUFLAGE (1 << 2)
#define AW_SNEAK      (1 << 3)

#define ACHECK_AFFECTS (1 << 0)
#define ACHECK_LIGHT   (1 << 1)
#define ACHECK_HUMMING (1 << 2)
#define ACHECK_GLOWING (1 << 3)
#define ACHECK_WEIGHT  (1 << 4)

int check_awake(CHAR_DATA * ch, int what);
int awake_hide(CHAR_DATA * ch);
int awake_invis(CHAR_DATA * ch);
int awake_camouflage(CHAR_DATA * ch);
int awake_sneak(CHAR_DATA * ch);
int awaking(CHAR_DATA * ch, int mode);
std::string time_format(int timer);

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2
#endif

#define SENDOK(ch)   (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
               (to_sleeping || AWAKE(ch)) && \
                     !PLR_FLAGGED((ch), PLR_WRITING))



inline bool a_isspace(unsigned char c)
{
	return (strchr(" \f\n\r\t\v", c) != NULL);
}

// Далеко не все из следующих функций используются в коде, но пусть будут (переписано с асма AL'ом)
inline bool a_isascii(unsigned char c)
{
    return c >= 32;
}

inline bool a_isprint(unsigned char c)
{
    return c >= 32;
}

inline bool a_islower(unsigned char c)
{
    return (c>='a' && c<='z') || (c>=192 && c<=223) || c == 163;
}

inline bool a_isupper(unsigned char c)
{
    return (c>='A' && c<='Z') || c>=224 || c == 179;
}

inline bool a_isdigit(unsigned char c)
{
    return c>='0' && c<='9';
}

inline bool a_isalpha(unsigned char c)
{
    return (c>='a' && c<='z') || (c>='A' && c<='Z') || c>=192 || c == 163 || c == 179;
}

inline bool a_isalnum(unsigned char c)
{
    return (c>='0' && c<='9')
	|| (c>='a' && c<='z')
	|| (c>='A' && c<='Z') || c>=192 || c == 163 || c == 179;
}

inline bool a_isxdigit(unsigned char c)
{
    return (c>='0' && c<='9')
	|| (c>='a' && c<='f')
	|| (c>='A' && c<='F');
}

inline char a_ucc(unsigned char c)
{
    if (c >= 'a' && c <= 'z') return c - 'a' + 'A';
    if (c >= 192 && c <= 223) return c + 32;
	if (c == 163) return c + 16;
    return c;
}

inline char a_lcc(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    if (c >= 224) return c - 32;
	if (c == 179) return c - 16;
    return c;
}

enum separator_mode {
	A_ISSPACE,
	A_ISASCII,
	A_ISPRINT,
	A_ISLOWER,
	A_ISUPPER,
	A_ISDIGIT,
	A_ISALPHA,
	A_ISALNUM,
	A_ISXDIGIT
};

class pred_separator
{
	bool (*pred)(unsigned char);
	bool l_not;
public:
	explicit
	pred_separator(separator_mode __mode, bool __l_not = false) : l_not(__l_not)
	{
		switch (__mode) {
		case A_ISSPACE:
			pred = a_isspace;
			break;
		case A_ISASCII:
			pred = a_isascii;
			break;
		case A_ISPRINT:
			pred = a_isprint;
			break;
		case A_ISLOWER:
			pred = a_islower;
			break;
		case A_ISUPPER:
			pred = a_isupper;
			break;
		case A_ISDIGIT:
			pred = a_isdigit;
			break;
		case A_ISALPHA:
			pred = a_isalpha;
			break;
		case A_ISALNUM:
			pred = a_isalnum;
			break;
		case A_ISXDIGIT:
			pred = a_isxdigit;
			break;
		}
	}

	explicit
	pred_separator() : pred(a_isspace), l_not(false) {}

	void reset() {}

	bool operator()(std::string::const_iterator& next, std::string::const_iterator end, std::string& tok)
	{
		tok = std::string();

		if (l_not)
			for (; next != end && !pred(*next); ++next) {}
		else
			for (; next != end && pred(*next); ++next) {}

		if (next == end)
			return false;

		if (l_not)
			for (; next != end && pred(*next); ++next)
				tok += *next;
		else
			for (; next != end && !pred(*next); ++next)
				tok += *next;

		return true;
	}
};

#endif
