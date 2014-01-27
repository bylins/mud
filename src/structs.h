/* ************************************************************************
*   File: structs.h                                     Part of Bylins    *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include "conf.h"
#include <vector>
#include <list>
#include <bitset>
#include <string>
#include <fstream>
#include <map>
#include <iterator>
#include <cstdint>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/array.hpp>

using std::map;
using std::iterator;
using std::string;

namespace ExtMoney
{

// золотые гривны
const unsigned TORC_GOLD = 0;
// серебряные гривны
const unsigned TORC_SILVER = 1;
// бронзовые гривны
const unsigned TORC_BRONZE = 2;
// терминатор всегда в конце
const unsigned TOTAL_TYPES = 3;

} // namespace ExtMoney

#define MAX_ALIAS_LENGTH 100
//-Polos.insert_wanted_gem

using std::list;
using std::bitset;

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD   0x030010	// Major/Minor/Patchlevel - MMmmPP

/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD 3.0 to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ   1		// TRUE/FALSE aren't defined yet.

#define MAX_DEST         50


// * Structure types.
/*
typedef struct area_data      AREA_DATA;
typedef struct ban_data    BAN_DATA;
typedef struct    buf_type    BUFFER;

typedef struct help_data      HELP_DATA;
typedef struct kill_data      KILL_DATA;
typedef struct mem_data    MEM_DATA;
typedef struct note_data      NOTE_DATA;

typedef struct mob_index_data    MOB_INDEX_DATA;
typedef struct obj_index_data    OBJ_INDEX_DATA;
typedef struct room_index_data      ROOM_INDEX_DATA;

typedef struct pc_data        PC_DATA;
typedef struct  gen_data      GEN_DATA;
typedef struct reset_data     RESET_DATA;

typedef struct weather_data      WEATHER_DATA;*/

// done
typedef struct flag_data FLAG_DATA;
typedef struct room_data ROOM_DATA;
typedef struct index_data INDEX_DATA;
typedef struct script_data SCRIPT_DATA;

typedef struct exit_data EXIT_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct affect_data AFFECT_DATA;

typedef class Character CHAR_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct trig_data TRIG_DATA;

// preamble ************************************************************

#define NOHOUSE    -1		// nil reference for non house
#define NOWHERE    0		// nil reference for room-database
#define NOTHING      -1		// nil reference for objects
#define NOBODY    -1		// nil reference for mobiles

#define SPECIAL(name) \
   int (name)(CHAR_DATA *ch, void *me, int cmd, char *argument)

// misc editor defines *************************************************

// format modes for format_text
#define FORMAT_INDENT      (1 << 0)

#define KT_ALT        1
#define KT_WIN        2
#define KT_WINZ       3
#define KT_WINZ6      4
#define KT_UTF8       5
#define KT_LAST       6
#define KT_SELECTMENU 255

// room-related defines ************************************************

#define HOLES_TIME 1

// The cardinal directions: used as index to room_data.dir_option[]
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5


// Room flags: used in room_data.room_flags //
// WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") //
#define ROOM_DARK    (1 << 0)	// Dark         //
#define ROOM_DEATH      (1 << 1)	// Death trap      //
#define ROOM_NOMOB      (1 << 2)	// MOBs not allowed      //
#define ROOM_INDOORS    (1 << 3)	// Indoors         //
#define ROOM_PEACEFUL      (1 << 4)	// Violence not allowed  //
#define ROOM_SOUNDPROOF    (1 << 5)	// Shouts, gossip blocked   //
#define ROOM_NOTRACK    (1 << 6)	// Track won't go through   //
#define ROOM_NOMAGIC    (1 << 7)	// Magic not allowed     //
#define ROOM_TUNNEL         (1 << 8)	// room for only 1 pers //
#define ROOM_NOTELEPORTIN   (1 << 9)	// В комнату не попасть телепортацией //
#define ROOM_GODROOM    (1 << 10)	// LVL_GOD+ only allowed //
#define ROOM_HOUSE          (1 << 11)	// (R) Room is a house  //
#define ROOM_HOUSE_CRASH   (1 << 12)	// (R) House needs saving   //
#define ROOM_ATRIUM         (1 << 13)	// (R) The door to a house //
#define ROOM_OLC         (1 << 14)	// (R) Modifyable/!compress   //
#define ROOM_BFS_MARK      (1 << 15)	// (R) breath-first srch mrk   //
#define ROOM_MAGE           (1 << 16)
#define ROOM_CLERIC         (1 << 17)
#define ROOM_THIEF          (1 << 18)
#define ROOM_WARRIOR        (1 << 19)
#define ROOM_ASSASINE       (1 << 20)
#define ROOM_GUARD          (1 << 21)
#define ROOM_PALADINE       (1 << 22)
#define ROOM_RANGER         (1 << 23)
#define ROOM_POLY           (1 << 24)
#define ROOM_MONO           (1 << 25)
#define ROOM_SMITH          (1 << 26)
#define ROOM_MERCHANT       (1 << 27)
#define ROOM_DRUID          (1 << 28)
#define ROOM_ARENA          (1 << 29)

#define ROOM_NOSUMMON       (INT_ONE | (1 << 0))
#define ROOM_NOTELEPORTOUT  (INT_ONE | (1 << 1))	// Из комнаты не выбраться телепортацией //
#define ROOM_NOHORSE        (INT_ONE | (1 << 2))
#define ROOM_NOWEATHER      (INT_ONE | (1 << 3))
#define ROOM_SLOWDEATH      (INT_ONE | (1 << 4))
#define ROOM_ICEDEATH       (INT_ONE | (1 << 5))
#define ROOM_NORELOCATEIN   (INT_ONE | (1 << 6))
#define ROOM_ARENARECV      (INT_ONE | (1 << 7))	// комната в которой слышно сообщения арены
#define ROOM_ARENASEND      (INT_ONE | (1 << 8))	// комната из которой отправляются сообщения арены
#define ROOM_NOBATTLE    (INT_ONE | (1 << 9)) //в клетке нельзя начать бой

#define ROOM_NOITEM         (INT_TWO|(1<<0))	// Передача вещей в комнате запрещена
#define ROOM_RUSICHI        (INT_TWO|(1<<1))
#define ROOM_VIKINGI        (INT_TWO|(1<<2))
#define ROOM_STEPNYAKI      (INT_TWO|(1<<3))

// Флаги комнатных аффектов НЕ сохраняются в файлах и возникают только от заклов //
#define AFF_ROOM_LIGHT       (1 << 0) // Аффект освещения комнаты  - SPELL_ROOM_LIGHT //
#define AFF_ROOM_FOG         (2 << 0) // Комната затуманена для SPELL_POISONED_FOG //
#define AFF_ROOM_RUNE_LABEL	 (4 << 0) // Комната помечена SPELL_MAGIC_LABEL //
#define AFF_ROOM_FORBIDDEN	 (8 << 0) // Комната помечена SPELL_FORBIDDEN //

// Exit info: used in room_data.dir_option.exit_info //
#define EX_ISDOOR    (1 << 0)	// Exit is a door     //
#define EX_CLOSED    (1 << 1)	// The door is closed //
#define EX_LOCKED    (1 << 2)	// The door is locked //
#define EX_PICKPROOF    (1 << 3)	// Lock can't be picked  //
#define EX_HIDDEN       (1 << 4)
#define EX_BROKEN       (1 << 5) //Polud замок двери сломан

#define AF_BATTLEDEC (1 << 0)
#define AF_DEADKEEP  (1 << 1)
#define AF_PULSEDEC  (1 << 2)
#define AF_SAME_TIME (1 << 3) // тикает раз в две секунды или во время раунда в бою (чтобы не между раундами)

// Sector types: used in room_data.sector_type //
#define SECT_INSIDE          0	// Indoors        //
#define SECT_CITY            1	// In a city         //
#define SECT_FIELD           2	// In a field     //
#define SECT_FOREST          3	// In a forest    //
#define SECT_HILLS           4	// In the hills      //
#define SECT_MOUNTAIN        5	// On a mountain     //
#define SECT_WATER_SWIM      6	// Swimmable water      //
#define SECT_WATER_NOSWIM    7	// Water - need a boat  //
#define SECT_FLYING          8	// Wheee!         //
#define SECT_UNDERWATER      9	// Underwater     //
#define SECT_SECRET          10
#define SECT_STONEROAD       11
#define SECT_ROAD            12
#define SECT_WILDROAD        13
// надо не забывать менять NUM_ROOM_SECTORS в olc.h

// Added values for weather changes //
#define SECT_FIELD_SNOW      20
#define SECT_FIELD_RAIN      21
#define SECT_FOREST_SNOW     22
#define SECT_FOREST_RAIN     23
#define SECT_HILLS_SNOW      24
#define SECT_HILLS_RAIN      25
#define SECT_MOUNTAIN_SNOW   26
#define SECT_THIN_ICE        27
#define SECT_NORMAL_ICE      28
#define SECT_THICK_ICE       29


#define WEATHER_QUICKCOOL     (1 << 0)
#define WEATHER_QUICKHOT      (1 << 1)
#define WEATHER_LIGHTRAIN     (1 << 2)
#define WEATHER_MEDIUMRAIN    (1 << 3)
#define WEATHER_BIGRAIN       (1 << 4)
#define WEATHER_GRAD       (1 << 5)
#define WEATHER_LIGHTSNOW     (1 << 6)
#define WEATHER_MEDIUMSNOW    (1 << 7)
#define WEATHER_BIGSNOW       (1 << 8)
#define WEATHER_LIGHTWIND     (1 << 9)
#define WEATHER_MEDIUMWIND    (1 << 10)
#define WEATHER_BIGWIND       (1 << 11)

#define MAX_REMORT            50

// char and mob-related defines ***************************************

// PC classes //
const int CLASS_UNDEFINED = -1;

enum
{
	CLASS_CLERIC,
	CLASS_BATTLEMAGE,
	CLASS_THIEF,
	CLASS_WARRIOR,
	CLASS_ASSASINE,
	CLASS_GUARD,
	CLASS_CHARMMAGE,
	CLASS_DEFENDERMAGE,
	CLASS_NECROMANCER,
	CLASS_PALADINE,
	CLASS_RANGER,
	CLASS_SMITH,
	CLASS_MERCHANT,
	CLASS_DRUID,
	NUM_CLASSES
};

// mobile class
#define CLASS_MOB      20

#define MASK_BATTLEMAGE   (1 << CLASS_BATTLEMAGE)
#define MASK_CLERIC       (1 << CLASS_CLERIC)
#define MASK_THIEF        (1 << CLASS_THIEF)
#define MASK_WARRIOR      (1 << CLASS_WARRIOR)
#define MASK_ASSASINE     (1 << CLASS_ASSASINE)
#define MASK_GUARD        (1 << CLASS_GUARD)
#define MASK_DEFENDERMAGE (1 << CLASS_DEFENDERMAGE)
#define MASK_CHARMMAGE    (1 << CLASS_CHARMMAGE)
#define MASK_NECROMANCER  (1 << CLASS_NECROMANCER)
#define MASK_PALADINE     (1 << CLASS_PALADINE)
#define MASK_RANGER       (1 << CLASS_RANGER)
#define MASK_SMITH        (1 << CLASS_SMITH)
#define MASK_MERCHANT     (1 << CLASS_MERCHANT)
#define MASK_DRUID        (1 << CLASS_DRUID)

#define MASK_MAGES        (MASK_BATTLEMAGE | MASK_DEFENDERMAGE | MASK_CHARMMAGE | MASK_NECROMANCER)
#define MASK_CASTER       (MASK_BATTLEMAGE | MASK_DEFENDERMAGE | MASK_CHARMMAGE | MASK_NECROMANCER | MASK_CLERIC | MASK_DRUID)

// PC religions //
#define RELIGION_POLY    0
#define RELIGION_MONO    1

#define MASK_RELIGION_POLY        (1 << RELIGION_POLY)
#define MASK_RELIGION_MONO        (1 << RELIGION_MONO)

// PC races //
// * Все расы персонажей-игроков теперь описываются в playerraces.xml

// PC Kin
#define NUM_KIN            3

// NPC classes
#define CLASS_BASIC_NPC    100
//#define CLASS_UNDEAD       101 к классам не относится, перенесено в расы (типы) мобов
//#define CLASS_HUMAN        102
//#define CLASS_ANIMAL       103
#define CLASS_HERO_WARRIOR 104
#define CLASS_HERO_MAGIC   105
#define CLASS_NPC_BATLEMAGE 106
#define CLASS_LAST_NPC     107

// NPC races
#define NPC_RACE_BASIC			100
#define NPC_RACE_HUMAN			101
#define NPC_RACE_HUMAN_ANIMAL	102
#define NPC_RACE_BIRD			103
#define NPC_RACE_ANIMAL			104
#define NPC_RACE_REPTILE		105
#define NPC_RACE_FISH			106
#define NPC_RACE_INSECT			107
#define NPC_RACE_PLANT			108
#define NPC_RACE_THING			109
#define NPC_RACE_ZOMBIE			110
#define NPC_RACE_GHOST			111
#define NPC_RACE_EVIL_SPIRIT	112
#define NPC_RACE_SPIRIT			113
#define NPC_RACE_MAGIC_CREATURE	114
#define NPC_RACE_LAST			115

// Sex
#define SEX_NEUTRAL         0
#define SEX_MALE            1
#define SEX_FEMALE          2
#define SEX_POLY            3

#define NUM_SEXES           4

#define MASK_SEX_NEUTRAL  (1 << SEX_NEUTRAL)
#define MASK_SEX_MALE     (1 << SEX_MALE)
#define MASK_SEX_FEMALE   (1 << SEX_FEMALE)
#define MASK_SEX_POLY     (1 << SEX_POLY)

// GODs FLAGS
#define GF_GODSLIKE   (1 << 0)
#define GF_GODSCURSE  (1 << 1)
#define GF_HIGHGOD    (1 << 2)
#define GF_REMORT     (1 << 3)
#define GF_DEMIGOD    (1 << 4)	// Морталы с привилегиями богов //
#define GF_PERSLOG    (1 << 5)	// Ведением отдельного лога команд персонажа //

// Positions
#define POS_DEAD       0	// dead        //
#define POS_MORTALLYW  1	// mortally wounded  //
#define POS_INCAP      2	// incapacitated  //
#define POS_STUNNED    3	// stunned     //
#define POS_SLEEPING   4	// sleeping    //
#define POS_RESTING    5	// resting     //
#define POS_SITTING    6	// sitting     //
#define POS_FIGHTING   7	// fighting    //
#define POS_STANDING   8	// standing    //


// Player flags: used by char_data.char_specials.act
#define PLR_KILLER       (1 << 0)	// Player is a player-killer     //
#define PLR_THIEF        (1 << 1)	// Player is a player-thief      //
#define PLR_FROZEN       (1 << 2)	// Player is frozen        //
#define PLR_DONTSET      (1 << 3)	// Don't EVER set (ISNPC bit)  //
#define PLR_WRITING      (1 << 4)	// Player writing (board/mail/olc)  //
#define PLR_MAILING      (1 << 5)	// Player is writing mail     //
#define PLR_CRASH        (1 << 6)	// Player needs to be crash-saved   //
#define PLR_SITEOK       (1 << 7)	// Player has been site-cleared  //
#define PLR_MUTE         (1 << 8)	// Player not allowed to shout/goss/auct  //
#define PLR_NOTITLE      (1 << 9)	// Player not allowed to set title  //
#define PLR_DELETED      (1 << 10)	// Player deleted - space reusable  //
#define PLR_LOADROOM     (1 << 11)	// Player uses nonstandard loadroom  (не используется) //
// свободно
#define PLR_NODELETE     (1 << 13)	// Player shouldn't be deleted //
#define PLR_INVSTART     (1 << 14)	// Player should enter game wizinvis //
#define PLR_CRYO         (1 << 15)	// Player is cryo-saved (purge prog)   //
#define PLR_HELLED       (1 << 16)	// Player is in Hell //
#define PLR_NAMED        (1 << 17)	// Player is in Names Room //
#define PLR_REGISTERED   (1 << 18)
#define PLR_DUMB         (1 << 19)	// Player is not allowed to tell/emote/social //
// свободно
#define PLR_DELETE       (1 << 28)	// RESERVED - ONLY INTERNALLY (MOB_DELETE) //
#define PLR_FREE         (1 << 29)	// RESERVED - ONLY INTERBALLY (MOB_FREE)//


// Mobile flags: used by char_data.char_specials.act
#define MOB_SPEC             (1 << 0)	// Mob has a callable spec-proc  //
#define MOB_SENTINEL         (1 << 1)	// Mob should not move     //
#define MOB_SCAVENGER        (1 << 2)	// Mob picks up stuff on the ground //
#define MOB_ISNPC            (1 << 3)	// (R) Automatically set on all Mobs   //
#define MOB_AWARE      (1 << 4)	// Mob can't be backstabbed      //
#define MOB_AGGRESSIVE       (1 << 5)	// Mob hits players in the room  //
#define MOB_STAY_ZONE        (1 << 6)	// Mob shouldn't wander out of zone //
#define MOB_WIMPY            (1 << 7)	// Mob flees if severely injured //
#define MOB_AGGR_DAY      (1 << 8)	// //
#define MOB_AGGR_NIGHT       (1 << 9)	// //
#define MOB_AGGR_FULLMOON    (1 << 10)	// //
#define MOB_MEMORY        (1 << 11)	// remember attackers if attacked   //
#define MOB_HELPER        (1 << 12)	// attack PCs fighting other NPCs   //
#define MOB_NOCHARM       (1 << 13)	// Mob can't be charmed    //
#define MOB_NOSUMMON      (1 << 14)	// Mob can't be summoned      //
#define MOB_NOSLEEP       (1 << 15)	// Mob can't be slept      //
#define MOB_NOBASH        (1 << 16)	// Mob can't be bashed (e.g. trees) //
#define MOB_NOBLIND       (1 << 17)	// Mob can't be blinded    //
#define MOB_MOUNTING         (1 << 18)
#define MOB_NOHOLD           (1 << 19)
#define MOB_NOSIELENCE       (1 << 20)
#define MOB_AGGRMONO         (1 << 21)
#define MOB_AGGRPOLY         (1 << 22)
#define MOB_NOFEAR           (1 << 23)
#define MOB_NOGROUP          (1 << 24)
#define MOB_CORPSE           (1 << 25)
#define MOB_LOOTER           (1 << 26)
#define MOB_PROTECT          (1 << 27)
#define MOB_DELETE           (1 << 28)	// RESERVED - ONLY INTERNALLY //
#define MOB_FREE             (1 << 29)	// RESERVED - ONLY INTERBALLY //

#define MOB_SWIMMING         (INT_ONE | (1 << 0))
#define MOB_FLYING           (INT_ONE | (1 << 1))
#define MOB_ONLYSWIMMING     (INT_ONE | (1 << 2))
#define MOB_AGGR_WINTER      (INT_ONE | (1 << 3))
#define MOB_AGGR_SPRING      (INT_ONE | (1 << 4))
#define MOB_AGGR_SUMMER      (INT_ONE | (1 << 5))
#define MOB_AGGR_AUTUMN      (INT_ONE | (1 << 6))
#define MOB_LIKE_DAY         (INT_ONE | (1 << 7))
#define MOB_LIKE_NIGHT       (INT_ONE | (1 << 8))
#define MOB_LIKE_FULLMOON    (INT_ONE | (1 << 9))
#define MOB_LIKE_WINTER      (INT_ONE | (1 << 10))
#define MOB_LIKE_SPRING      (INT_ONE | (1 << 11))
#define MOB_LIKE_SUMMER      (INT_ONE | (1 << 12))
#define MOB_LIKE_AUTUMN      (INT_ONE | (1 << 13))
#define MOB_NOFIGHT          (INT_ONE | (1 << 14))
#define MOB_EADECREASE       (INT_ONE | (1 << 15))
#define MOB_HORDE            (INT_ONE | (1 << 16))
#define MOB_CLONE            (INT_ONE | (1 << 17))
#define MOB_NOTKILLPUNCTUAL  (INT_ONE | (1 << 18))
#define MOB_NOTRIP           (INT_ONE | (1 << 19))
#define MOB_ANGEL            (INT_ONE | (1 << 20))
#define MOB_GUARDIAN         (INT_ONE | (1 << 21)) //Polud моб-стражник, ставится программно, берется из файла guards.xml
#define MOB_IGNORE_FORBIDDEN (INT_ONE | (1 << 22)) // игнорирует печать
#define MOB_NO_BATTLE_EXP    (INT_ONE | (1 << 23)) // не дает экспу за удары

#define MOB_FIREBREATH    (INT_TWO | (1 << 0))
#define MOB_GASBREATH     (INT_TWO | (1 << 1))
#define MOB_FROSTBREATH   (INT_TWO | (1 << 2))
#define MOB_ACIDBREATH    (INT_TWO | (1 << 3))
#define MOB_LIGHTBREATH   (INT_TWO | (1 << 4))
#define MOB_NOTRAIN       (INT_TWO | (1 << 5))
#define MOB_NOREST        (INT_TWO | (1 << 6))
#define MOB_AREA_ATTACK   (INT_TWO | (1 << 7))
#define MOB_NOSTUPOR      (INT_TWO | (1 << 8))
#define MOB_NOHELPS       (INT_TWO | (1 << 9))
#define MOB_OPENDOOR      (INT_TWO | (1 << 10))
#define MOB_IGNORNOMOB    (INT_TWO | (1 << 11))
#define MOB_IGNORPEACE    (INT_TWO | (1 << 12))
#define MOB_RESURRECTED   (INT_TWO | (1 << 13))	// поднят через !поднять труп! или !оживить труп! Ставится только програмно//
#define MOB_RUSICH         (INT_TWO | (1 << 14))
#define MOB_VIKING         (INT_TWO | (1 << 15))
#define MOB_STEPNYAK       (INT_TWO | (1 << 16))
#define MOB_AGGR_RUSICHI   (INT_TWO | (1 << 17))
#define MOB_AGGR_VIKINGI   (INT_TWO | (1 << 18))
#define MOB_AGGR_STEPNYAKI (INT_TWO | (1 << 19))
#define MOB_NORESURRECTION (INT_TWO | (1 << 20))


#define NPC_NORTH         (1 << 0)
#define NPC_EAST          (1 << 1)
#define NPC_SOUTH         (1 << 2)
#define NPC_WEST          (1 << 3)
#define NPC_UP            (1 << 4)
#define NPC_DOWN          (1 << 5)
#define NPC_POISON        (1 << 6)
#define NPC_INVIS         (1 << 7)
#define NPC_SNEAK         (1 << 8)
#define NPC_CAMOUFLAGE    (1 << 9)
#define NPC_MOVEFLY       (1 << 11)
#define NPC_MOVECREEP     (1 << 12)
#define NPC_MOVEJUMP      (1 << 13)
#define NPC_MOVESWIM      (1 << 14)
#define NPC_MOVERUN       (1 << 15)
#define NPC_AIRCREATURE   (1 << 20)
#define NPC_WATERCREATURE (1 << 21)
#define NPC_EARTHCREATURE (1 << 22)
#define NPC_FIRECREATURE  (1 << 23)
#define NPC_HELPED        (1 << 24)

#define NPC_STEALING      (INT_ONE | (1 << 0))
#define NPC_WIELDING      (INT_ONE | (1 << 1))
#define NPC_ARMORING      (INT_ONE | (1 << 2))
#define NPC_USELIGHT      (INT_ONE | (1 << 3))

// Descriptor flags //
#define DESC_CANZLIB (1 << 0)	// Client says compression capable.   //

// Preference flags: used by char_data.player_specials.pref //
#define PRF_BRIEF       (1 << 0)	// Room descs won't normally be shown //
#define PRF_COMPACT     (1 << 1)	// No extra CRLF pair before prompts  //
#define PRF_NOHOLLER     (1 << 2)	// Не слышит команду "орать"   //
#define PRF_NOTELL       (1 << 3)	// Не слышит команду "сказать" //
#define PRF_DISPHP       (1 << 4)	// Display hit points in prompt   //
#define PRF_DISPMANA (1 << 5)	// Display mana points in prompt   //
#define PRF_DISPMOVE (1 << 6)	// Display move points in prompt   //
#define PRF_AUTOEXIT (1 << 7)	// Display exits in a room      //
#define PRF_NOHASSLE (1 << 8)	// Aggr mobs won't attack    //
#define PRF_SUMMONABLE  (1 << 9)	// Can be summoned         //
#define PRF_QUEST         (1 << 10)	// On quest                       //
#define PRF_NOREPEAT (1 << 11)	// No repetition of comm commands  //
#define PRF_HOLYLIGHT   (1 << 12)	// Can see in dark        //
#define PRF_COLOR_1      (1 << 13)	// Color (low bit)       //
#define PRF_COLOR_2      (1 << 14)	// Color (high bit)         //
#define PRF_NOWIZ     (1 << 15)	// Can't hear wizline       //
#define PRF_LOG1      (1 << 16)	// On-line System Log (low bit)   //
#define PRF_LOG2      (1 << 17)	// On-line System Log (high bit)  //
#define PRF_NOAUCT       (1 << 18)	// Can't hear auction channel     //
#define PRF_NOGOSS       (1 << 19)	// Не слышит команду "болтать" //
#define PRF_DISPFIGHT   (1 << 20)	// Видит свое состояние в бою      //
#define PRF_ROOMFLAGS   (1 << 21)	// Can see room flags (ROOM_x)  //
#define PRF_DISPEXP     (1 << 22)
#define PRF_DISPEXITS   (1 << 23)
#define PRF_DISPLEVEL   (1 << 24)
#define PRF_DISPGOLD    (1 << 25)
#define PRF_DISPTICK    (1 << 26)
#define PRF_PUNCTUAL    (1 << 27)
#define PRF_AWAKE       (1 << 28)
#define PRF_CODERINFO   (1 << 29)

#define PRF_AUTOMEM     (INT_ONE | 1 << 0)
#define PRF_NOSHOUT     (INT_ONE | 1 << 1)	// Не слышит команду "кричать"  //
#define PRF_GOAHEAD     (INT_ONE | 1 << 2)	// Добавление IAC GA после промпта //
#define PRF_SHOWGROUP   (INT_ONE | 1 << 3)	// Показ полного состава группы //
#define PRF_AUTOASSIST  (INT_ONE | 1 << 4)	// Автоматическое вступление в бой //
#define PRF_AUTOLOOT    (INT_ONE | 1 << 5)	// Autoloot //
#define PRF_AUTOSPLIT   (INT_ONE | 1 << 6)	// Autosplit //
#define PRF_AUTOMONEY   (INT_ONE | 1 << 7)	// Automoney //
#define PRF_NOARENA     (INT_ONE | 1 << 8)	// Не слышит арену //
#define PRF_NOEXCHANGE  (INT_ONE | 1 << 9)	// Не слышит базар //
#define PRF_NOCLONES	(INT_ONE | 1 << 10)	// Не видит в группе чужих клонов //
#define PRF_NOINVISTELL	(INT_ONE | 1 << 11)	// Не хочет, чтобы телял "кто-то" //
#define PRF_POWERATTACK	(INT_ONE | 1 << 12)	// мощная атака //
#define PRF_GREATPOWERATTACK  (INT_ONE | 1 << 13) // улучшеная мощная атака //
#define PRF_AIMINGATTACK      (INT_ONE | 1 << 14) // прицельная атака //
#define PRF_GREATAIMINGATTACK (INT_ONE | 1 << 15) // улучшеная прицельная атака //
#define PRF_NEWS_MODE   (INT_ONE | 1 << 16) // вариант чтения новостей мада и дружины
#define PRF_BOARD_MODE  (INT_ONE | 1 << 17) // уведомления о новых мессагах на досках
#define PRF_DECAY_MODE  (INT_ONE | 1 << 18) // канал хранилища, рассыпание шмота
#define PRF_TAKE_MODE   (INT_ONE | 1 << 19) // канал хранилища, положили/взяли
#define PRF_PKL_MODE    (INT_ONE | 1 << 20) // уведомления о добавлении/убирании в пкл
#define PRF_POLIT_MODE  (INT_ONE | 1 << 21) // уведомления об изменении политики, своей и чужой
#define PRF_IRON_WIND   (INT_ONE | 1 << 22) // включен скилл "железный ветер"
#define PRF_PKFORMAT_MODE (INT_ONE | 1 << 23) // формат пкл/дрл
#define PRF_WORKMATE_MODE (INT_ONE | 1 << 24) // показ входов/выходов соклановцев
#define PRF_OFFTOP_MODE (INT_ONE | 1 << 25) // вкл/выкл канала оффтопа
#define PRF_ANTIDC_MODE (INT_ONE | 1 << 26) // режим защиты от дисконекта в бою
#define PRF_NOINGR_MODE (INT_ONE | 1 << 27) // не показывать продажу/покупку ингров в канале базара
#define PRF_NOINGR_LOOT (INT_ONE | 1 << 28) // не лутить ингры в режиме автограбежа
#define PRF_DISP_TIMED     (INT_ONE | 1 << 29) // показ задержек для характерных профам умений и способностей

#define PRF_IGVA_PRONA    (INT_TWO | 1 << 0)  // для стоп-списка оффтоп
#define PRF_EXECUTOR      (INT_TWO | 1 << 1)  // палач
#define PRF_DRAW_MAP      (INT_TWO | 1 << 2)  // отрисовка карты при осмотре клетки
#define PRF_CAN_REMORT    (INT_TWO | 1 << 3)  // разрешение на реморт через жертвование гривн
#define PRF_ENTER_ZONE    (INT_TWO | 1 << 4)  // вывод названия/среднего уровня при входе в зону
#define PRF_MISPRINT      (INT_TWO | 1 << 5)  // показ непрочитанных сообщений на доске опечаток при входе
#define PRF_BRIEF_SHIELDS (INT_TWO | 1 << 6)  // краткий режим сообщений при срабатывании маг.щитов
#define PRF_AUTO_NOSUMMON (INT_TWO | 1 << 7)  // автоматическое включение режима защиты от призыва ('реж призыв') после удачного суммона/пенты
// при добавлении не забываем про preference_bits[]

// Affect bits: used in char_data.char_specials.saved.affected_by //
// WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") //
#define AFF_BLIND             (1 << 0)	// (R) Char is blind    //
#define AFF_INVISIBLE         (1 << 1)	// Char is invisible    //
#define AFF_DETECT_ALIGN      (1 << 2)	// Char is sensitive to align //
#define AFF_DETECT_INVIS      (1 << 3)	// Char can see invis chars  //
#define AFF_DETECT_MAGIC      (1 << 4)	// Char is sensitive to magic //
#define AFF_SENSE_LIFE        (1 << 5)	// Char can sense hidden life //
#define AFF_WATERWALK         (1 << 6)	// Char can walk on water  //
#define AFF_SANCTUARY         (1 << 7)	// Char protected by sanct.   //
#define AFF_GROUP             (1 << 8)	// (R) Char is grouped  //
#define AFF_CURSE             (1 << 9)	// Char is cursed    //
#define AFF_INFRAVISION       (1 << 10)	// Char can see in dark //
#define AFF_POISON            (1 << 11)	// (R) Char is poisoned //
#define AFF_PROTECT_EVIL      (1 << 12)	// Char protected from evil  //
#define AFF_PROTECT_GOOD      (1 << 13)	// Char protected from good  //
#define AFF_SLEEP             (1 << 14)	// (R) Char magically asleep  //
#define AFF_NOTRACK           (1 << 15)	// Char can't be tracked   //
#define AFF_TETHERED          (1 << 16)	// Room for future expansion  //
#define AFF_BLESS             (1 << 17)	// Room for future expansion  //
#define AFF_SNEAK             (1 << 18)	// Char can move quietly   //
#define AFF_HIDE              (1 << 19)	// Char is hidden    //
#define AFF_COURAGE           (1 << 20)	// Room for future expansion  //
#define AFF_CHARM             (1 << 21)	// Char is charmed      //
#define AFF_HOLD              (1 << 22)
#define AFF_FLY               (1 << 23)
#define AFF_SIELENCE          (1 << 24)
#define AFF_AWARNESS          (1 << 25)
#define AFF_BLINK             (1 << 26)
#define AFF_HORSE             (1 << 27)	// NPC - is horse, PC - is horsed //
#define AFF_NOFLEE            (1 << 28)
#define AFF_SINGLELIGHT       (1 << 29)

#define AFF_HOLYLIGHT         (INT_ONE | (1 << 0))
#define AFF_HOLYDARK          (INT_ONE | (1 << 1))
#define AFF_DETECT_POISON     (INT_ONE | (1 << 2))
#define AFF_DRUNKED           (INT_ONE | (1 << 3))
#define AFF_ABSTINENT         (INT_ONE | (1 << 4))
#define AFF_STOPRIGHT         (INT_ONE | (1 << 5))
#define AFF_STOPLEFT          (INT_ONE | (1 << 6))
#define AFF_STOPFIGHT         (INT_ONE | (1 << 7))
#define AFF_HAEMORRAGIA       (INT_ONE | (1 << 8))
#define AFF_CAMOUFLAGE        (INT_ONE | (1 << 9))
#define AFF_WATERBREATH       (INT_ONE | (1 << 10))
#define AFF_SLOW              (INT_ONE | (1 << 11))
#define AFF_HASTE             (INT_ONE | (1 << 12))
#define AFF_SHIELD            (INT_ONE | (1 << 13))
#define AFF_AIRSHIELD         (INT_ONE | (1 << 14))
#define AFF_FIRESHIELD        (INT_ONE | (1 << 15))
#define AFF_ICESHIELD         (INT_ONE | (1 << 16))
#define AFF_MAGICGLASS        (INT_ONE | (1 << 17))
#define AFF_STAIRS            (INT_ONE | (1 << 18))
#define AFF_STONEHAND         (INT_ONE | (1 << 19))
#define AFF_PRISMATICAURA     (INT_ONE | (1 << 20))
#define AFF_HELPER            (INT_ONE | (1 << 21))
#define AFF_EVILESS           (INT_ONE | (1 << 22))
#define AFF_AIRAURA           (INT_ONE | (1 << 23))
#define AFF_FIREAURA          (INT_ONE | (1 << 24))
#define AFF_ICEAURA           (INT_ONE | (1 << 25))
#define AFF_DEAFNESS          (INT_ONE | (1 << 26))
#define AFF_CRYING            (INT_ONE | (1 << 27))
#define AFF_PEACEFUL          (INT_ONE | (1 << 28))
#define AFF_MAGICSTOPFIGHT    (INT_ONE | (1 << 29))

#define AFF_BERSERK           (INT_TWO | (1 << 0))
#define AFF_LIGHT_WALK        (INT_TWO | (1 << 1))
#define AFF_BROKEN_CHAINS     (INT_TWO | (1 << 2))
#define AFF_CLOUD_OF_ARROWS   (INT_TWO | (1 << 3))
#define AFF_SHADOW_CLOAK      (INT_TWO | (1 << 4))
#define AFF_GLITTERDUST       (INT_TWO | (1 << 5))
#define AFF_AFFRIGHT          (INT_TWO | (1 << 6))
#define AFF_SCOPOLIA_POISON   (INT_TWO | (1 << 7))
#define AFF_DATURA_POISON     (INT_TWO | (1 << 8))
#define AFF_SKILLS_REDUCE     (INT_TWO | (1 << 9))
#define AFF_NOT_SWITCH        (INT_TWO | (1 << 10))
#define AFF_BELENA_POISON     (INT_TWO | (1 << 11))
#define AFF_NOTELEPORT        (INT_TWO | (1 << 12))
#define AFF_LACKY             (INT_TWO | (1 << 13))
#define AFF_BANDAGE           (INT_TWO | (1 << 14))
#define AFF_NO_BANDAGE        (INT_TWO | (1 << 15))
#define AFF_MORPH             (INT_TWO | (1 << 16))
#define AFF_STRANGLED         (INT_TWO | (1 << 17))
#define AFF_RECALL_SPELLS     (INT_TWO | (1 << 18))
#define AFF_NOOB_REGEN        (INT_TWO | (1 << 19))

// shapirus: modes of ignoring
#define IGNORE_TELL	(1 << 0)
#define IGNORE_SAY	(1 << 1)
#define IGNORE_CLAN	(1 << 2)
#define IGNORE_ALLIANCE	(1 << 3)
#define IGNORE_GOSSIP	(1 << 4)
#define IGNORE_SHOUT	(1 << 5)
#define IGNORE_HOLLER	(1 << 6)
#define IGNORE_GROUP	(1 << 7)
#define IGNORE_WHISPER	(1 << 8)
#define IGNORE_ASK	(1 << 9)
#define IGNORE_EMOTE	(1 << 10)
#define IGNORE_OFFTOP	(1 << 11)

// Modes of connectedness: used by descriptor_data.state //
//ОБЕЗАТЕЛЬНО ДОБАВИТЬ В connected_types[]!!!!//
#define CON_PLAYING       0 // Playing - Nominal state //
#define CON_CLOSE         1 // Disconnecting     //
#define CON_GET_NAME      2 // By what name ..?     //
#define CON_NAME_CNFRM    3 // Did I get that right, x?   //
#define CON_PASSWORD      4 // Password:         //
#define CON_NEWPASSWD     5 // Give me a password for x   //
#define CON_CNFPASSWD     6 // Please retype password: //
#define CON_QSEX          7 // Sex?           //
#define CON_QCLASS        8 // Class?         //
#define CON_RMOTD         9 // PRESS RETURN after MOTD //
#define CON_MENU         10 // Your choice: (main menu)   //
#define CON_EXDESC       11 // Enter a new description:   //
#define CON_CHPWD_GETOLD 12 // Changing passwd: get old   //
#define CON_CHPWD_GETNEW 13 // Changing passwd: get new   //
#define CON_CHPWD_VRFY   14 // Verify new password     //
#define CON_DELCNF1      15 // Delete confirmation 1   //
#define CON_DELCNF2      16 // Delete confirmation 2   //
#define CON_DISCONNECT   17 // In-game disconnection   //
#define CON_OEDIT        18 //. OLC mode - object edit     . //
#define CON_REDIT        19 //. OLC mode - room edit       . //
#define CON_ZEDIT        20 //. OLC mode - zone info edit  . //
#define CON_MEDIT        21 //. OLC mode - mobile edit     . //
#define CON_TRIGEDIT     22 //. OLC mode - trigger edit    . //
#define CON_NAME2        23
#define CON_NAME3        24
#define CON_NAME4        25
#define CON_NAME5        26
#define CON_NAME6        27
#define CON_RELIGION     28
#define CON_RACE         29
#define CON_LOWS         30
#define CON_GET_KEYTABLE 31
#define CON_GET_EMAIL    32
#define CON_ROLL_STATS   33
#define CON_MREDIT       34 // OLC mode - make recept edit //
#define CON_QKIN         35
#define CON_QCLASSV      36
#define CON_QCLASSS      37
#define CON_MAP_MENU     38
#define CON_COLOR        39
#define CON_WRITEBOARD   40 // написание на доску
#define CON_CLANEDIT     41 // команда house
#define CON_NEW_CHAR     42
#define CON_SPEND_GLORY  43 // вливание славы через команду у чара
#define CON_RESET_STATS  44 // реролл статов при входе в игру
#define CON_BIRTHPLACE   45 // выбираем где начать игру
#define CON_WRITE_MOD    46 // пишет клановое сообщение дня
#define CON_GLORY_CONST  47 // вливает славу2
#define CON_NAMED_STUFF  48 // редактирует именной стаф
#define CON_RESET_KIN    49 // выбор расы после смены/удаления оной (или иного способа испоганивания значения)
#define CON_RESET_RACE   50 // выбор РОДА посла смены/сброса оного
#define CON_CONSOLE      51 // Интерактивная скриптовая консоль
#define CON_TORC_EXCH    52 // обмен гривен
#define CON_MENU_STATS   53 // оплата сброса стартовых статов из главного меню
#define CON_SEDIT        54 // sedit - редактирование сетов
// не забываем отражать новые состояния в connected_types -- Krodo

// Character equipment positions: used as index for char_data.equipment[] //
// NOTE: Don't confuse these constants with the ITEM_ bitvectors
//       which control the valid places you can wear a piece of equipment
#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD     16
#define WEAR_HOLD      17
#define WEAR_BOTHS     18
#define NUM_WEARS      19	// This must be the # of eq positions!! //


// object-related defines ******************************************* //


// Item types: used by obj_data.obj_flags.type_flag //
#define ITEM_LIGHT      1	// Item is a light source  //
#define ITEM_SCROLL     2	// Item is a scroll     //
#define ITEM_WAND       3	// Item is a wand    //
#define ITEM_STAFF      4	// Item is a staff      //
#define ITEM_WEAPON     5	// Item is a weapon     //
#define ITEM_FIREWEAPON 6	// Unimplemented     //
#define ITEM_MISSILE    7	// Unimplemented     //
#define ITEM_TREASURE   8	// Item is a treasure, not gold  //
#define ITEM_ARMOR      9	// Item is armor     //
#define ITEM_POTION    10	// Item is a potion     //
#define ITEM_WORN      11	// Unimplemented     //
#define ITEM_OTHER     12	// Misc object       //
#define ITEM_TRASH     13	// Trash - shopkeeps won't buy   //
#define ITEM_TRAP      14	// Unimplemented     //
#define ITEM_CONTAINER 15	// Item is a container     //
#define ITEM_NOTE      16	// Item is note      //
#define ITEM_DRINKCON  17	// Item is a drink container  //
#define ITEM_KEY       18	// Item is a key     //
#define ITEM_FOOD      19	// Item is food         //
#define ITEM_MONEY     20	// Item is money (gold)    //
#define ITEM_PEN       21	// Item is a pen     //
#define ITEM_BOAT      22	// Item is a boat    //
#define ITEM_FOUNTAIN  23	// Item is a fountain      //
#define ITEM_BOOK      24	// Item is book //
#define ITEM_INGRADIENT 25	// Item is magical ingradient //
#define ITEM_MING      26	// Магический ингредиент //
#define ITEM_MATERIAL  27	// Материал для крафтовых умений //
#define ITEM_BANDAGE   28  	// бинты для перевязки
#define ITEM_ARMOR_LIGHT  29 	// легкий тип брони
#define ITEM_ARMOR_MEDIAN 30 	// средний тип брони
#define ITEM_ARMOR_HEAVY  31 	// тяжелый тип брони
#define ITEM_ENCHANT      32	// зачарование предмета

// +newbook.patch (Alisher)
// Типы магических книг //
#define BOOK_SPELL		0	// Книга заклинания //
#define BOOK_SKILL		1	// Книга умения //
#define BOOK_UPGRD		2	// Увеличение умения //
#define BOOK_RECPT		3	// Книга рецепта //
#define BOOK_FEAT			4	// Книга способности (feats) //
// -newbook.patch (Alisher)


// Take/Wear flags: used by obj_data.obj_flags.wear_flags //
#define ITEM_WEAR_TAKE     (1 << 0)	// Item can be takes      //
#define ITEM_WEAR_FINGER   (1 << 1)	// Can be worn on finger  //
#define ITEM_WEAR_NECK     (1 << 2)	// Can be worn around neck   //
#define ITEM_WEAR_BODY     (1 << 3)	// Can be worn on body    //
#define ITEM_WEAR_HEAD     (1 << 4)	// Can be worn on head    //
#define ITEM_WEAR_LEGS     (1 << 5)	// Can be worn on legs //
#define ITEM_WEAR_FEET     (1 << 6)	// Can be worn on feet //
#define ITEM_WEAR_HANDS    (1 << 7)	// Can be worn on hands   //
#define ITEM_WEAR_ARMS     (1 << 8)	// Can be worn on arms //
#define ITEM_WEAR_SHIELD   (1 << 9)	// Can be used as a shield   //
#define ITEM_WEAR_ABOUT    (1 << 10)	// Can be worn about body    //
#define ITEM_WEAR_WAIST    (1 << 11)	// Can be worn around waist  //
#define ITEM_WEAR_WRIST    (1 << 12)	// Can be worn on wrist   //
#define ITEM_WEAR_WIELD    (1 << 13)	// Can be wielded      //
#define ITEM_WEAR_HOLD     (1 << 14)	// Can be held      //
#define ITEM_WEAR_BOTHS     (1 << 15)


// Extra object flags: used by obj_data.obj_flags.extra_flags //
#define ITEM_GLOW          (1 << 0)	// Item is glowing      //
#define ITEM_HUM           (1 << 1)	// Item is humming      //
#define ITEM_NORENT        (1 << 2)	// Item cannot be rented   //
#define ITEM_NODONATE      (1 << 3)	// Item cannot be donated  //
#define ITEM_NOINVIS    (1 << 4)	// Item cannot be made invis  //
#define ITEM_INVISIBLE     (1 << 5)	// Item is invisible    //
#define ITEM_MAGIC         (1 << 6)	// Item is magical      //
#define ITEM_NODROP        (1 << 7)	// Item is cursed: can't drop //
#define ITEM_BLESS         (1 << 8)	// Item is blessed      //
#define ITEM_NOSELL        (1 << 9)	// Not usable by good people  //
#define ITEM_DECAY         (1 << 10)	// Not usable by evil people  //
#define ITEM_ZONEDECAY     (1 << 11)	// Not usable by neutral people  //
#define ITEM_NODISARM      (1 << 12)	// Not usable by mages     //
#define ITEM_NODECAY       (1 << 13)
#define ITEM_POISONED      (1 << 14)
#define ITEM_SHARPEN       (1 << 15)
#define ITEM_ARMORED       (1 << 16)
#define ITEM_DAY        (1 << 17)
#define ITEM_NIGHT         (1 << 18)
#define ITEM_FULLMOON      (1 << 19)
#define ITEM_WINTER        (1 << 20)
#define ITEM_SPRING        (1 << 21)
#define ITEM_SUMMER        (1 << 22)
#define ITEM_AUTUMN        (1 << 23)
#define ITEM_SWIMMING      (1 << 24)
#define ITEM_FLYING        (1 << 25)
#define ITEM_THROWING      (1 << 26)
#define ITEM_TICKTIMER     (1 << 27)
#define ITEM_FIRE          (1 << 28)	// ...горит                   //
#define ITEM_REPOP_DECAY   (1 << 29)	// рассыпется при репопе зоны //
#define ITEM_NOLOCATE      (INT_ONE | (1 << 0))	// нельзя отлокейтить //
#define ITEM_TIMEDLVL      (INT_ONE | (1 << 1))	// для маг.предметов уровень уменьшается со временем //
#define ITEM_NOALTER       (INT_ONE | (1 << 2))	// свойства предмета не могут быть изменены магией //
#define ITEM_WITH1SLOT     (INT_ONE | (1 << 3))	// в предмет можно вплавить 1 камень //
#define ITEM_WITH2SLOTS    (INT_ONE | (1 << 4))	// в предмет можно вплавить 2 камня //
#define ITEM_WITH3SLOTS    (INT_ONE | (1 << 5))	// в предмет можно вплавить 3 камня (овер) //
#define ITEM_SETSTUFF      (INT_ONE | (1 << 6)) // Item is set object //
#define ITEM_NO_FAIL       (INT_ONE | (1 << 7)) // не фейлится при изучении (в случае книги)
#define ITEM_NAMED         (INT_ONE | (1 << 8)) // именной предмет
#define ITEM_BLOODY        (INT_ONE | (1 << 9)) // окровавленная вещь (снятая с трупа)
#define ITEM_1INLAID       (INT_ONE | (1 << 10)) // TODO: не используется, см convert_obj_values()
#define ITEM_2INLAID       (INT_ONE | (1 << 11)) // -//-
#define ITEM_3INLAID       (INT_ONE | (1 << 12)) // -//-

#define ITEM_NO_MONO       (1 << 0)
#define ITEM_NO_POLY       (1 << 1)
#define ITEM_NO_NEUTRAL    (1 << 2)
#define ITEM_NO_MAGIC_USER (1 << 3)
#define ITEM_NO_CLERIC     (1 << 4)
#define ITEM_NO_THIEF      (1 << 5)
#define ITEM_NO_WARRIOR    (1 << 6)
#define ITEM_NO_ASSASINE   (1 << 7)
#define ITEM_NO_GUARD      (1 << 8)
#define ITEM_NO_PALADINE   (1 << 9)
#define ITEM_NO_RANGER     (1 << 10)
#define ITEM_NO_SMITH      (1 << 11)
#define ITEM_NO_MERCHANT   (1 << 12)
#define ITEM_NO_DRUID      (1 << 13)
#define ITEM_NO_BATTLEMAGE (1 << 14)
#define ITEM_NO_CHARMMAGE  (1 << 15)
#define ITEM_NO_DEFENDERMAGE  (1 << 16)
#define ITEM_NO_NECROMANCER   (1 << 17)

#define ITEM_NO_KILLER     (INT_ONE | 1 << 0)
#define ITEM_NO_COLORED    (INT_ONE | 1 << 1)	// нельзя цветным //
//#define ITEM_NO_KILLERONLY (INT_ONE | 1 << 2)// // только для душиков //

#define ITEM_NO_SEVERANE   (INT_TWO | 1 << 0)
#define ITEM_NO_POLANE     (INT_TWO | 1 << 1)
#define ITEM_NO_KRIVICHI   (INT_TWO | 1 << 2)
#define ITEM_NO_VATICHI    (INT_TWO | 1 << 3)
#define ITEM_NO_VELANE     (INT_TWO | 1 << 4)
#define ITEM_NO_DREVLANE   (INT_TWO | 1 << 5)
#define ITEM_NO_MALE       (INT_TWO | 1 << 6)
#define ITEM_NO_FEMALE     (INT_TWO | 1 << 7)
#define ITEM_NO_CHARMICE   (INT_TWO | 1 << 8)
#define ITEM_NO_POLOVCI    (INT_TWO | 1 << 9)
#define ITEM_NO_PECHENEGI  (INT_TWO | 1 << 10)
#define ITEM_NO_MONGOLI    (INT_TWO | 1 << 11)
#define ITEM_NO_YIGURI     (INT_TWO | 1 << 12)
#define ITEM_NO_KANGARI    (INT_TWO | 1 << 13)
#define ITEM_NO_XAZARI     (INT_TWO | 1 << 14)
#define ITEM_NO_SVEI       (INT_TWO | 1 << 15)
#define ITEM_NO_DATCHANE   (INT_TWO | 1 << 16)
#define ITEM_NO_GETTI      (INT_TWO | 1 << 17)
#define ITEM_NO_UTTI       (INT_TWO | 1 << 18)
#define ITEM_NO_XALEIGI    (INT_TWO | 1 << 19)
#define ITEM_NO_NORVEZCI   (INT_TWO | 1 << 20)

#define ITEM_NO_RUSICHI     (INT_THREE | 1 << 0)
#define ITEM_NO_STEPNYAKI   (INT_THREE | 1 << 1)
#define ITEM_NO_VIKINGI     (INT_THREE | 1 << 2)

#define ITEM_AN_MONO       (1 << 0)
#define ITEM_AN_POLY       (1 << 1)
#define ITEM_AN_NEUTRAL    (1 << 2)
#define ITEM_AN_MAGIC_USER (1 << 3)
#define ITEM_AN_CLERIC     (1 << 4)
#define ITEM_AN_THIEF      (1 << 5)
#define ITEM_AN_WARRIOR    (1 << 6)
#define ITEM_AN_ASSASINE   (1 << 7)
#define ITEM_AN_GUARD      (1 << 8)
#define ITEM_AN_PALADINE   (1 << 9)
#define ITEM_AN_RANGER     (1 << 10)
#define ITEM_AN_SMITH      (1 << 11)
#define ITEM_AN_MERCHANT   (1 << 12)
#define ITEM_AN_DRUID      (1 << 13)
#define ITEM_AN_BATTLEMAGE (1 << 14)
#define ITEM_AN_CHARMMAGE  (1 << 15)
#define ITEM_AN_DEFENDERMAGE  (1 << 16)
#define ITEM_AN_NECROMANCER   (1 << 17)

#define ITEM_AN_KILLER     (INT_ONE | (1 << 0))
#define ITEM_AN_COLORED    (INT_ONE | (1 << 1))	// нельзя цветным //
//#define ITEM_AN_KILLERONLY (INT_ONE | (1 << 2))// // только для душиков //

#define ITEM_AN_SEVERANE   (INT_TWO | 1 << 0)
#define ITEM_AN_POLANE     (INT_TWO | 1 << 1)
#define ITEM_AN_KRIVICHI   (INT_TWO | 1 << 2)
#define ITEM_AN_VATICHI    (INT_TWO | 1 << 3)
#define ITEM_AN_VELANE     (INT_TWO | 1 << 4)
#define ITEM_AN_DREVLANE   (INT_TWO | 1 << 5)
#define ITEM_AN_MALE       (INT_TWO | 1 << 6)
#define ITEM_AN_FEMALE     (INT_TWO | 1 << 7)
#define ITEM_AN_CHARMICE   (INT_TWO | 1 << 8)
#define ITEM_AN_POLOVCI    (INT_TWO | 1 << 9)
#define ITEM_AN_PECHENEGI  (INT_TWO | 1 << 10)
#define ITEM_AN_MONGOLI    (INT_TWO | 1 << 11)
#define ITEM_AN_YIGURI     (INT_TWO | 1 << 12)
#define ITEM_AN_KANGARI    (INT_TWO | 1 << 13)
#define ITEM_AN_XAZARI     (INT_TWO | 1 << 14)
#define ITEM_AN_SVEI       (INT_TWO | 1 << 15)
#define ITEM_AN_DATCHANE   (INT_TWO | 1 << 16)
#define ITEM_AN_GETTI      (INT_TWO | 1 << 17)
#define ITEM_AN_UTTI       (INT_TWO | 1 << 18)
#define ITEM_AN_XALEIGI    (INT_TWO | 1 << 19)
#define ITEM_AN_NORVEZCI   (INT_TWO | 1 << 20)

#define ITEM_AN_RUSICHI     (INT_THREE | 1 << 0)
#define ITEM_AN_STEPNYAKI   (INT_THREE | 1 << 1)
#define ITEM_AN_VIKINGI     (INT_THREE | 1 << 2)

// Modifier constants used with obj affects ('A' fields) //
#define APPLY_NONE              0	// No effect         //
#define APPLY_STR               1	// Apply to strength    //
#define APPLY_DEX               2	// Apply to dexterity      //
#define APPLY_INT               3	// Apply to constitution   //
#define APPLY_WIS               4	// Apply to wisdom      //
#define APPLY_CON               5	// Apply to constitution   //
#define APPLY_CHA		6	// Apply to charisma    //
#define APPLY_CLASS             7	// Reserved       //
#define APPLY_LEVEL             8	// Reserved       //
#define APPLY_AGE               9	// Apply to age         //
#define APPLY_CHAR_WEIGHT      10	// Apply to weight      //
#define APPLY_CHAR_HEIGHT      11	// Apply to height      //
#define APPLY_MANAREG          12	// Apply to max mana    //
#define APPLY_HIT              13	// Apply to max hit points //
#define APPLY_MOVE             14	// Apply to max move points   //
#define APPLY_GOLD             15	// Reserved       //
#define APPLY_EXP              16	// Reserved       //
#define APPLY_AC               17	// Apply to Armor Class    //
#define APPLY_HITROLL          18	// Apply to hitroll     //
#define APPLY_DAMROLL          19	// Apply to damage roll    //
#define APPLY_SAVING_WILL      20	// Apply to save throw: paralz   //
#define APPLY_RESIST_FIRE      21	// Apply to RESIST throw: fire  //
#define APPLY_RESIST_AIR       22	// Apply to RESIST throw: air   //
#define APPLY_SAVING_CRITICAL  23	// Apply to save throw: breath   //
#define APPLY_SAVING_STABILITY 24	// Apply to save throw: spells   //
#define APPLY_HITREG           25
#define APPLY_MOVEREG          26
#define APPLY_C1               27
#define APPLY_C2               28
#define APPLY_C3               29
#define APPLY_C4               30
#define APPLY_C5               31
#define APPLY_C6               32
#define APPLY_C7               33
#define APPLY_C8               34
#define APPLY_C9               35
#define APPLY_SIZE             36
#define APPLY_ARMOUR           37
#define APPLY_POISON           38
#define APPLY_SAVING_REFLEX    39
#define APPLY_CAST_SUCCESS     40
#define APPLY_MORALE           41
#define APPLY_INITIATIVE       42
#define APPLY_RELIGION         43
#define APPLY_ABSORBE          44
#define APPLY_LIKES	       45
#define APPLY_RESIST_WATER     46	// Apply to RESIST throw: water  //
#define APPLY_RESIST_EARTH     47	// Apply to RESIST throw: earth  //
#define APPLY_RESIST_VITALITY  48	// Apply to RESIST throw: light, dark, critical damage  //
#define APPLY_RESIST_MIND      49	// Apply to RESIST throw: mind magic  //
#define APPLY_RESIST_IMMUNITY  50	// Apply to RESIST throw: poison, disease etc.  //
#define APPLY_AR	           51	// Apply to Magic affect resist //
#define APPLY_MR	           52	// Apply to Magic damage resist //
#define APPLY_ACONITUM_POISON  53
#define APPLY_SCOPOLIA_POISON  54
#define APPLY_BELENA_POISON    55
#define APPLY_DATURA_POISON    56
#define APPLY_HIT_GLORY        57
#define NUM_APPLIES	           58

// APPLY - эффекты для комнат //
#define APPLY_ROOM_NONE        0
#define APPLY_ROOM_POISON      1 	// Изменяет в комнате уровень ядности //
#define APPLY_ROOM_FLAME       2 	// Изменяет в комнате уровень огня (для потомков) //
#define NUM_ROOM_APPLIES       3

#define MAT_NONE               0
#define MAT_BULAT              1
#define MAT_BRONZE             2
#define MAT_IRON               3
#define MAT_STEEL              4
#define MAT_SWORDSSTEEL        5
#define MAT_COLOR              6
#define MAT_CRYSTALL           7
#define MAT_WOOD               8
#define MAT_SUPERWOOD          9
#define MAT_FARFOR             10
#define MAT_GLASS              11
#define MAT_ROCK               12
#define MAT_BONE               13
#define MAT_MATERIA            14
#define MAT_SKIN               15
#define MAT_ORGANIC            16
#define MAT_PAPER              17
#define MAT_DIAMOND            18


#define TRACK_NPC              (1 << 0)
#define TRACK_HIDE             (1 << 1)

// Container flags - value[1] //
#define CONT_CLOSEABLE      (1 << 0)	// Container can be closed //
#define CONT_PICKPROOF      (1 << 1)	// Container is pickproof  //
#define CONT_CLOSED         (1 << 2)	// Container is closed     //
#define CONT_LOCKED         (1 << 3)	// Container is locked     //
#define CONT_BROKEN         (1 << 4)	// Container is locked     //

// other miscellaneous defines ****************************************** //

enum { DRUNK, FULL, THIRST};

// Sun state for weather_data //
#define SUN_DARK  0
#define SUN_RISE  1
#define SUN_LIGHT 2
#define SUN_SET      3

// Moon change type //
#define MOON_INCREASE 0
#define MOON_DECREASE 1

// Sky conditions for weather_data //
#define SKY_CLOUDLESS   0
#define SKY_CLOUDY       1
#define SKY_RAINING      2
#define SKY_LIGHTNING   3

#define EXTRA_FAILHIDE       (1 << 0)
#define EXTRA_FAILSNEAK      (1 << 1)
#define EXTRA_FAILCAMOUFLAGE (1 << 2)
// для избежания повторных записей моба в списки SetsDrop
#define EXTRA_GRP_KILL_COUNT (1 << 3)

// other #defined constants ********************************************* //

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 *
 * LVL_IMPL should always be the HIGHEST possible immortal level, and
 * LVL_IMMORT should always be the LOWEST immortal level.  The number of
 * mortal levels will always be LVL_IMMORT - 1.
 */
const short LVL_IMPL = 34;
const short LVL_GRGOD = 33;
const short LVL_BUILDER = 33;
const short LVL_GOD = 32;
const short LVL_IMMORT = 31;

// Level of the 'freeze' command //
#define LVL_FREEZE   LVL_GRGOD

#define NUM_OF_DIRS  6		// number of directions in a room (nsewud) //
#define MAGIC_NUMBER (0x06)	// Arbitrary number that won't be in a string //

#define OPT_USEC  40000	// 25 passes per second //
#define PASSES_PER_SEC  (1000000 / OPT_USEC)
#define RL_SEC    * PASSES_PER_SEC

#define PULSE_ZONE      (1 RL_SEC)
#define PULSE_MOBILE    (10 RL_SEC)
#define PULSE_VIOLENCE  (2 RL_SEC)
#define ZONES_RESET	1	// number of zones to reset at one time //
#define PULSE_LOGROTATE (10 RL_SEC)

// Variables for the output buffering system //
#define MAX_SOCK_BUF            (24 * 1024)	// Size of kernel's sock buf   //
#define MAX_PROMPT_LENGTH       256	// Max length of prompt        //
#define GARBAGE_SPACE         32	// Space for **OVERFLOW** etc  //
#define SMALL_BUFSIZE         1024	// Static output buffer size   //
// Max amount of output that can be buffered //
#define LARGE_BUFSIZE            (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)
// Keep last 5 commands
const int HISTORY_SIZE = 5;
#define MAX_STRING_LENGTH     16384
#define MAX_EXTEND_LENGTH     0xFFFF
#define MAX_INPUT_LENGTH      256	// Max length per *line* of input //
#define MAX_RAW_INPUT_LENGTH  512	// Max size of *raw* input //
#define MAX_MESSAGES          600
#define MAX_NAME_LENGTH       20
#define MIN_NAME_LENGTH       4
#define HOST_LENGTH           30
#define EXDSCR_LENGTH         512
#define MAX_SPELLS            350
#define MAX_AFFECT            32
#define MAX_OBJ_AFFECT        6
#define MAX_TIMED_SKILLS      16
#define MAX_FEATS             128 // Максимальное количество фитов //
#define MAX_TIMED_FEATS       16 // Макс. количество фитов с таймером //
#define MAX_HITS              32000 // Максимальное количество хитов и дамага //
// Количество запомненных предложений для воззваний //
#define MAX_REMEMBER_PRAY     20
// Количество запомненных предложений для эфира //
#define MAX_REMEMBER_GOSSIP   15
// планка на кол-во денег у чара на руках и в банке (раздельно)
const long MAX_MONEY_KEPT = 1000000000;

// *********************************************************************
// * Structures                                                        *
// *********************************************************************


typedef signed char
sbyte;
typedef unsigned char
ubyte;
typedef short int
sh_int;
typedef short int
ush_int;
#if !defined(__cplusplus)	// Anyone know a portable method?
typedef char
bool;
#endif

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)	// Hm, sysdep.h?
typedef char
byte;
#endif

typedef int
room_vnum;			// A room's vnum type //
typedef int
obj_vnum;			// An object's vnum type //
typedef int
mob_vnum;			// A mob's vnum type //
typedef int
zone_vnum;			// A virtual zone number.  //

typedef int
room_rnum;			// A room's real (internal) number type //
typedef int
obj_rnum;			// An object's real (internal) num type //
typedef int
mob_rnum;			// A mobile's real (internal) num type //
typedef int
zone_rnum;			// A zone's real (array index) number. //


// ************ WARNING ******************************************* //
// This structure describe new bitvector structure                  //
typedef uint32_t bitvector_t;

#define INT_ZERRO (0 << 30)
#define INT_ONE   (1 << 30)
#define INT_TWO   (2 << 30)
#define INT_THREE (3 << 30)
#define GET_FLAG(value,flag) (value.flags[(static_cast<uint32_t>(flag)) >> 30])

struct flag_data
{
	flag_data& operator+=(const flag_data &r)
	{
		for (int i = 0; i < 4; ++i)
		{
			flags[i] |= r.flags[i];
		}
		return *this;
	}
	bool operator!=(const flag_data &r) const
	{
		for (int i = 0; i < 4; ++i)
		{
			if (flags[i] != r.flags[i])
			{
				return true;
			}
		}
		return false;
	}
	bool operator==(const flag_data &r) const
	{
		return !(*this != r);
	}
	bool empty() const
	{
		for (int i = 0; i < 4; ++i)
		{
			if (flags[i] != 0)
			{
				return false;
			}
		}
		return true;
	}

	uint32_t flags[4];
};

extern const FLAG_DATA clear_flags;

class unique_bit_flag_data : public flag_data
{
public:
	unique_bit_flag_data() : flag_data(clear_flags) {}

	unique_bit_flag_data(const flag_data& __base) : flag_data(__base) {}

	int
	get_plane(int __plane) const
	{
		return *(flags + __plane) | __plane << 30;
	}

	unique_bit_flag_data&
	set_plane(uint32_t __plane)
	{
		int num = __plane < static_cast<uint32_t>(INT_ONE)   ? 0 :
				  __plane < static_cast<uint32_t>(INT_TWO)   ? 1 :
				  __plane < static_cast<uint32_t>(INT_THREE) ? 2 : 3;
		*(flags + num) |= 0x3FFFFFFF & __plane;
		return *this;
	}
};

inline int
flag_data_by_num(const int& num)
{
	return num < 0   ? 0 :
		   num < 30  ? (1 << num) :
		   num < 60  ? (INT_ONE | (1 << (num - 30))) :
		   num < 90  ? (INT_TWO | (1 << (num - 60))) :
		   num < 120 ? (INT_THREE | (1 << (num - 90))) : 0;
}

inline bool
operator==(const unique_bit_flag_data& __lop, const unique_bit_flag_data& __rop)
{
	return *__lop.flags & *__rop.flags || *(__lop.flags + 1) & *(__rop.flags + 1) ||
		   *(__lop.flags + 2) & *(__rop.flags + 2) || *(__lop.flags + 3) & *(__rop.flags + 3);
}

inline bool
operator!=(const unique_bit_flag_data& __lop, const unique_bit_flag_data& __rop)
{
	return !(__lop == __rop);
}

inline bool
operator<(const unique_bit_flag_data& __lop, const unique_bit_flag_data& __rop)
{
	return __lop != __rop &&
		   (*__lop.flags < *__rop.flags || *(__lop.flags + 1) < *(__rop.flags + 1) ||
			*(__lop.flags + 2) < *(__rop.flags + 2) || *(__lop.flags + 3) < *(__rop.flags + 3));
}

inline bool
operator>(const unique_bit_flag_data& __lop, const unique_bit_flag_data& __rop)
{
	return __lop != __rop &&
		   (*__lop.flags > *__rop.flags || *(__lop.flags + 1) > *(__rop.flags + 1) ||
			*(__lop.flags + 2) > *(__rop.flags + 2) || *(__lop.flags + 3) > *(__rop.flags + 3));
}

inline bool
operator<=(const unique_bit_flag_data& __lop, const unique_bit_flag_data& __rop)
{
	return __lop == __rop ||
		   (*__lop.flags < *__rop.flags || *(__lop.flags + 1) < *(__rop.flags + 1) ||
			*(__lop.flags + 2) < *(__rop.flags + 2) || *(__lop.flags + 3) < *(__rop.flags + 3));
}

inline bool
operator>=(const unique_bit_flag_data& __lop, const unique_bit_flag_data& __rop)
{
	return __lop == __rop ||
		   (*__lop.flags > *__rop.flags || *(__lop.flags + 1) > *(__rop.flags + 1) ||
			*(__lop.flags + 2) > *(__rop.flags + 2) || *(__lop.flags + 3) > *(__rop.flags + 3));
}

// Extra description: used in objects, mobiles, and rooms //
struct extra_descr_data
{
	char *keyword;		// Keyword in look/examine          //
	char *description;	// What to see                      //
	EXTRA_DESCR_DATA *next;	// Next in list                     //
};

// header block for rent files.  BEWARE: Changing it will ruin rent files  //
struct save_rent_info
{
	save_rent_info() : time(0), rentcode(0), net_cost_per_diem(0), gold(0),
		account(0), nitems(0), oitems(0), spare1(0), spare2(0), spare3(0),
		spare4(0), spare5(0), spare6(0), spare7(0) {};

	int32_t time;
	int32_t rentcode;
	int32_t net_cost_per_diem;
	int32_t gold;
	int32_t account;
	int32_t nitems;
	int32_t oitems;
	int32_t spare1;
	int32_t spare2;
	int32_t spare3;
	int32_t spare4;
	int32_t spare5;
	int32_t spare6;
	int32_t spare7;
};

struct save_time_info
{
	int32_t vnum;
	int32_t timer;
};

struct save_info
{
	struct save_rent_info rent;
	std::vector<save_time_info> time;
};

// =======================================================================


// char-related structures ***********************************************


// memory structure for characters //
struct memory_rec_struct
{
	long
	id;
	long
	time;
	struct memory_rec_struct *next;
};

typedef struct memory_rec_struct
			memory_rec;


// This structure is purely intended to be an easy way to transfer //
// and return information about time (real or mudwise).            //
struct time_info_data
{
	int
	hours, day, month;
	sh_int year;
};

// shapirus
struct ignore_data
{
	long
	id;
	unsigned long
	mode;
	struct ignore_data *next;
};
// Alez
struct logon_data
{
	char * ip;
	long count;
	time_t lasttime; //by kilnik
	logon_data * next;
};

struct punish_data
{
	long duration;
	char * reason;
	int  level;
	long godid;
};

// An affect structure. //
class IAffectHandler;
struct affect_data
{
	affect_data() : type(0), duration(0), modifier(0), location(0),
		battleflag(0), bitvector(0), caster_id(0), must_handled(0),
		apply_time(0), next(0) {};

	sh_int type;		// The type of spell that caused this      //
	sh_int duration;	// For how long its effects will last      //
	int modifier;		// This is added to apropriate ability     //
	int location;		// Tells which ability to change(APPLY_XXX) //
	long
	battleflag;	   //*** SUCH AS HOLD,SIELENCE etc
	long
	bitvector;		// Tells which bits to set (AFF_XXX) //
	long
	caster_id; //Unique caster ID //
	bool
	must_handled; // Указывает муду что для аффекта должен быть вызван обработчик (пока только для комнат) //
	sh_int
	apply_time; // Указывает сколько аффект висит (пока используется только в комнатах) //
	AFFECT_DATA *next;
	boost::shared_ptr<IAffectHandler> handler; //обработчик аффектов
};

struct timed_type
{
	ubyte skill;		// Number of used skill/spell //
	ubyte time;		// Time for next using        //
	struct timed_type *next;
};


// Structure used for chars following other chars //
struct follow_type
{
	CHAR_DATA *follower;
	struct follow_type *next;
};

// Structure used for tracking a mob //
struct track_info
{
	int
	trk_info;
	int
	who;
	int
	dirs;
};

// Structure used for helpers //
struct helper_data_type
{
	int
	mob_vnum;
	struct helper_data_type *next_helper;
};

// Structure used for on_dead object loading //
struct load_data
{
	int
	obj_vnum;
	int
	load_prob;
	int
	load_type;
	int
	spec_param;
};

typedef
list < struct load_data *>load_list;

struct spell_mem_queue_item
{
	int
	spellnum;
	struct spell_mem_queue_item *link;
};

// descriptor-related structures ****************************************

struct txt_block
{
	char *text;
	int
	aliased;
	struct txt_block *next;
};


struct txt_q
{
	struct txt_block *head;
	struct txt_block *tail;
};

namespace Glory
{

class spend_glory;

}

namespace GloryConst
{

struct glory_olc;

}

namespace NamedStuff
{

struct stuff_node;

}

namespace scripting
{
	class Console;
}

namespace MapSystem
{
	struct Options;
}

namespace obj_sets_olc
{
	struct sedit;
}

class Board;

struct descriptor_data
{
	socket_t descriptor;	// file descriptor for socket    //
	char
	host[HOST_LENGTH + 1];	// hostname          //
	byte bad_pws;		// number of bad pw attemps this login //
	byte idle_tics;		// tics idle at password prompt     //
	int
	connected;		// mode of 'connectedness'    //
	int
	desc_num;		// unique num assigned to desc      //
	time_t input_time;
	time_t login_time;	// when the person connected     //
	char *showstr_head;	// for keeping track of an internal str   //
	char **showstr_vector;	// for paging through texts      //
	int
	showstr_count;		// number of pages to page through  //
	int
	showstr_page;		// which page are we currently showing?   //
	char **str;		// for the modify-str system     //
	size_t max_str;		//      -        //
	char *backstr;		// added for handling abort buffers //
	int mail_to;		// uid for mail system
	int
	has_prompt;		// is the user at a prompt?             //
	char
	inbuf[MAX_RAW_INPUT_LENGTH];	// buffer for raw input    //
	char
	last_input[MAX_INPUT_LENGTH];	// the last input       //
	char
	small_outbuf[SMALL_BUFSIZE];	// standard output buffer      //
	char *output;		// ptr to the current output buffer //
	char **history;		// History of commands, for ! mostly.  //
	int
	history_pos;		// Circular array position.      //
	int
	bufptr;		// ptr to end of current output     //
	int
	bufspace;		// space left in the output buffer  //
	struct txt_block *large_outbuf;	// ptr to large buffer, if we need it //
	struct txt_q
				input;			// q of unprocessed input     //
	CHAR_DATA *character;	// linked to char       //
	CHAR_DATA *original;	// original char if switched     //
	DESCRIPTOR_DATA *snooping;	// Who is this char snooping  //
	DESCRIPTOR_DATA *snoop_by;	// And who is snooping this char //
	DESCRIPTOR_DATA *next;	// link to next descriptor     //
	struct olc_data *olc;	//. OLC info - defined in olc.h   . //
	ubyte keytable;
	int
	options;		// descriptor flags       //
#if defined(HAVE_ZLIB)
	z_stream *deflate;	// compression engine        //
	int
	mccp_version;
#endif
	unsigned long ip; // ип адрес в виде числа для внутреннего пользования
	boost::weak_ptr<Board> board; // редактируемая доска
	boost::shared_ptr<struct Message> message; // редактируемое сообщение
	boost::shared_ptr<struct ClanOLC> clan_olc; // редактирование привилегий клана
	boost::shared_ptr<struct ClanInvite> clan_invite; // приглашение в дружину
	bool registered_email; // чтобы не шарить каждую секунду по списку мыл
	FILE *pers_log; // чтобы не открывать файл на каждую команду чара при персональном логе
	boost::shared_ptr<class Glory::spend_glory> glory; // вливание славы
	boost::shared_ptr<GloryConst::glory_olc> glory_const; // вливание славы2
	boost::shared_ptr<NamedStuff::stuff_node> named_obj;	// редактируемая именная шмотка
	boost::shared_ptr<scripting::Console> console;	// Скриптовая консоль
	unsigned long cur_vnum;					// текущий внум именной шмотки
	unsigned long old_vnum;					// старый внум именной шмотки
    boost::shared_ptr<MapSystem::Options> map_options; // редактирование опций режима карты
    bool snoop_with_map; // показывать снуперу карту цели с опциями самого снупера
    boost::array<int, ExtMoney::TOTAL_TYPES> ext_money; // обмен доп.денег
    boost::shared_ptr<obj_sets_olc::sedit> sedit; // редактирование сетов
};


// other miscellaneous structures **************************************


struct msg_type
{
	char *attacker_msg;	// message to attacker //
	char *victim_msg;	// message to victim   //
	char *room_msg;		// message to room     //
};


struct message_type
{
	struct msg_type
				die_msg;		// messages when death        //
	struct msg_type
				miss_msg;		// messages when miss         //
	struct msg_type
				hit_msg;		// messages when hit       //
	struct msg_type
				god_msg;		// messages when hit on god      //
	struct message_type *next;	// to next messages of this kind.   //
};


struct message_list
{
	int
	a_type;		// Attack type          //
	int
	number_of_attacks;	// How many attack messages to chose from. //
	struct message_type *msg;	// List of messages.       //
};

//MZ.load
struct zone_type
{
	char *name;			// type name //
	int ingr_qty;		// quantity of ingredient types //
	int *ingr_types;	// types of ingredients, which are loaded in zones of this type //
};
//-MZ.load


struct dex_skill_type
{
	int
	p_pocket;
	int
	p_locks;
	int
	traps;
	int
	sneak;
	int
	hide;
};


struct dex_app_type
{
	int
	reaction;
	int
	miss_att;
	int
	defensive;
};


struct str_app_type
{
	int
	tohit;			// To Hit (THAC0) Bonus/Penalty        //
	int
	todam;			// Damage Bonus/Penalty                //
	int
	carry_w;		// Maximum weight that can be carrried //
	int
	wield_w;		// Maximum weight that can be wielded  //
	int
	hold_w;		// MAXIMUM WEIGHT THAT CAN BE HELDED   //
};


struct wis_app_type
{
	int
	spell_additional;	// bitvector //
	int
	max_learn_l20;		// MAX SKILL on LEVEL 20        //
	int
	spell_success;		// spell using success          //
	int
	char_savings;		// saving spells (damage)       //
	int
	max_skills;
};


struct int_app_type
{
	int
	spell_aknowlege;	// chance to know spell               //
	int
	to_skilluse;		// ADD CHANSE FOR USING SKILL         //
	int
	mana_per_tic;
	int
	spell_success;		//  max count of spell on 1s level    //
	int
	improove;		// chance to improove skill           //
	int
	observation;		// chance to use SKILL_AWAKE/CRITICAL //
};

struct con_app_type
{
	int
	hitp;
	int
	ressurection;
	int
	affect_saving;		// saving spells (affects)  //
	int
	poison_saving;
	int
	critic_saving;
};

struct cha_app_type
{
	int
	leadership;
	int
	charms;
	int
	morale;
	int
	illusive;
	int
	dam_to_hit_rate;
};

struct size_app_type
{
	int
	ac;			// ADD VALUE FOR AC           //
	int
	interpolate;		// ADD VALUE FOR SOME SKILLS  //
	int
	initiative;
	int
	shocking;
};

struct weapon_app_type
{
	int
	shocking;
	int
	bashing;
	int
	parrying;
};

struct extra_affects_type
{
	int
	affect;
	int
	set_or_clear;
};

struct class_app_type
{
	int
	unknown_weapon_fault;
	int
	koef_con;
	int
	base_con;
	int
	min_con;
	int
	max_con;

	struct extra_affects_type *extra_affects;
//	struct obj_affected_type *extra_modifiers;
};

struct race_app_type
{
	struct extra_affects_type *extra_affects;
	struct obj_affected_type *extra_modifiers;
};

struct weather_data
{
	int
	hours_go;		// Time life from reboot //

	int
	pressure;		// How is the pressure ( Mb )            //
	int
	press_last_day;	// Average pressure last day             //
	int
	press_last_week;	// Average pressure last week            //

	int
	temperature;		// How is the temperature (C)            //
	int
	temp_last_day;		// Average temperature last day          //
	int
	temp_last_week;	// Average temperature last week         //

	int
	rainlevel;		// Level of water from rains             //
	int
	snowlevel;		// Level of snow                         //
	int
	icelevel;		// Level of ice                          //

	int
	weather_type;		// bitvector - some values for month     //

	int
	change;		// How fast and what way does it change. //
	int
	sky;			// How is the sky.   //
	int
	sunlight;		// And how much sun. //
	int
	moon_day;		// And how much moon //
	int
	season;
	int
	week_day_mono;
	int
	week_day_poly;
};

struct weapon_affect_types
{
	int
	aff_pos;
	int
	aff_bitvector;
	int
	aff_spell;
};

struct title_type
{
	char *title_m;
	char *title_f;
	int
	exp;
};


// element in monster and object index-tables   //
struct index_data
{
	int vnum;			// virtual number of this mob/obj       //
	int number;		// number of existing units of this mob/obj //
	int stored;		// number of things in rent file            //
	SPECIAL(*func);
	char *farg;		// string argument for special function     //
	struct trig_data *proto;	// for triggers... the trigger     //
	int zone;			// mob/obj zone rnum //
	size_t set_idx; // индекс сета в obj_sets::set_list, если != -1
};

// linked list for mob/object prototype trigger lists //
struct trig_proto_list
{
	int
	vnum;			// vnum of the trigger   //
	struct trig_proto_list *next;	// next trigger          //
};

struct social_messg  		// No argument was supplied //
{
	int
	ch_min_pos, ch_max_pos, vict_min_pos, vict_max_pos;
	char *char_no_arg;
	char *others_no_arg;

	// An argument was there, and a victim was found //
	char *char_found;	// if NULL, read no further, ignore args //
	char *others_found;
	char *vict_found;

	// An argument was there, but no victim was found //
	char *not_found;
};



struct social_keyword
{
	char *keyword;
	int
	social_message;
};

extern struct social_messg *soc_mess_list;
extern struct social_keyword *soc_keys_list;

struct pray_affect_type
{
	int
	metter;
	int
	location;
	int
	modifier;
	long
	bitvector;
	int
	battleflag;
};

#define  DAY_EASTER     -1

#define  GAPPLY_NONE                 0
#define  GAPPLY_SKILL_SUCCESS        1
#define  GAPPLY_SPELL_SUCCESS        2
#define  GAPPLY_SPELL_EFFECT         3
#define         GAPPLY_MODIFIER             4
#define         GAPPLY_AFFECT               5

/* pclean_criteria_data структура которая определяет через какой время
   неактивности будет удален чар
*/
struct pclean_criteria_data
{
	int
	level;			// max уровень для этого временного лимита //
	int
	days;			// временной лимит в днях        //
};

// Структрура для описания проталов для спела townportal //
struct portals_list_type
{
	char *wrd;		// кодовое слово //
	int
	vnum;			// vnum комнаты для портала (раньше был rnum, но зачем тут rnum?) //
	int
	level;			// минимальный уровень для запоминания портала //
	struct portals_list_type *next_portal;
};

struct char_portal_type
{
	int
	vnum;			// vnum комнаты для портала //
	struct char_portal_type *next;
};

// Структуры для act.wizard.cpp //

struct show_struct
{
	const char *cmd;
	const char
	level;
};

struct set_struct
{
	const char *cmd;
	const char
	level;
	const char
	pcnpc;
	const char
	type;
};

extern int grouping[NUM_CLASSES][MAX_REMORT+1];




//Polos.insert_wanted_gem

struct int3
{
	int type;
	int bit;
	int qty;
};


typedef map< string, int3 > alias_type;


class insert_wanted_gem
{
	map< int, alias_type > content;

public:
	void init();
	void show(CHAR_DATA *ch, int gem_vnum);
	int get_type(int gem_vnum, string str);
	int get_bit(int gem_vnum, string str);
	int get_qty(int gem_vnum, string str);
	int exist(int gem_vnum, string str);
};

//-Polos.insert_wanted_gem

//Polud
struct mob_guardian
{
	int max_wars_allow;
	bool agro_killers;
	bool agro_all_agressors;
	std::vector<zone_vnum> agro_argressors_in_zones;
};

typedef map <int, mob_guardian> guardian_type;

//-Polud

#endif // __STRUCTS_H__ //

