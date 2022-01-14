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

//#include "boards/boards_types.h"
#include "sysdep.h"

#include <vector>
#include <list>
#include <bitset>
#include <string>
#include <fstream>
#include <iterator>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include <memory>

typedef int8_t sbyte;
typedef uint8_t ubyte;
typedef int16_t sh_int;
typedef uint16_t ush_int;

typedef struct index_data INDEX_DATA;
typedef struct time_info_data TIME_INFO_DATA;

// This structure describe new bitvector structure                  //
typedef uint32_t bitvector_t;

#if !defined(__cplusplus)    // Anyone know a portable method?
typedef char bool;
#endif

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)    // Hm, sysdep.h?
typedef char byte;
#endif

typedef int room_vnum;    // A room's vnum type //
typedef int obj_vnum;    // An object's vnum type //
typedef int mob_vnum;    // A mob's vnum type //
typedef int zone_vnum;    // A virtual zone number.  //
typedef int trg_vnum;    // A virtual trigger number.  //

using rnum_t = int;
typedef rnum_t room_rnum;    // A room's real (internal) number type //
typedef rnum_t obj_rnum;    // An object's real (internal) num type //
typedef rnum_t mob_rnum;    // A mobile's real (internal) num type //
typedef rnum_t zone_rnum;    // A zone's real (array index) number. //
typedef rnum_t trg_rnum;    // A trigger's real (array index) number. //

const int kMaxRemort = 75;

namespace ExtMoney {
const unsigned kTorcGold = 0;        // золотые гривны
const unsigned kTorcSilver = 1;        // серебряные гривны
const unsigned kTorcBronze = 2;        // бронзовые гривны
const unsigned kTotalTypes = 3;        // терминатор всегда в конце
} // namespace ExtMoney

constexpr bitvector_t kIntZero = 0u << 30;
constexpr bitvector_t kIntOne = 1u << 30;
constexpr bitvector_t kIntTwo = 2u << 30;
constexpr bitvector_t kIntThree = 3u << 30;

namespace currency {
enum { GOLD, GLORY, TORC, ICE, NOGATA };
}

const int kMaxAliasLehgt = 100;
const std::nullptr_t NoArgument = nullptr;

extern const char *nothing_string;


/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD 3.0 to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ 1

const __uint8_t kMaxDest = 50;

class CHAR_DATA;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.
class OBJ_DATA;    // forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.
class TRIG_DATA;

// preamble ************************************************************

const __int8_t kNoHouse = -1;        // nil reference for non house
const __int8_t kNowhere = 0;        // nil reference for room-database
const __int8_t kNothing = -1;        // nil reference for objects
const __int8_t kNobody = -1;        // nil reference for mobiles

// misc editor defines *************************************************

// format modes for format_text
constexpr int kFormatIndent = 1 << 0;

const __uint8_t kCodePageAlt = 1;
const __uint8_t kCodePageWin = 2;
const __uint8_t kCodePageWinz = 3;
const __uint8_t kCodePageWinzZ = 4;
const __uint8_t kCodePageUTF8 = 5;
const __uint8_t kCodePageWinzOld = 6;
const __uint8_t kCodePageLast = 7;

const int kKtSelectmenu = 255;

// room-related defines ************************************************

const int kHolesTime = 1;

constexpr bitvector_t AF_BATTLEDEC = 1 << 0;
constexpr bitvector_t AF_DEADKEEP = 1 << 1;
constexpr bitvector_t AF_PULSEDEC = 1 << 2;
constexpr bitvector_t AF_SAME_TIME = 1 << 3; // тикает раз в две секунды или во время раунда в бою (чтобы не между раундами)

constexpr bitvector_t WEATHER_QUICKCOOL = 1 << 0;
constexpr bitvector_t WEATHER_QUICKHOT = 1 << 1;
constexpr bitvector_t WEATHER_LIGHTRAIN = 1 << 2;
constexpr bitvector_t WEATHER_MEDIUMRAIN = 1 << 3;
constexpr bitvector_t WEATHER_BIGRAIN = 1 << 4;
constexpr bitvector_t WEATHER_GRAD = 1 << 5;
constexpr bitvector_t WEATHER_LIGHTSNOW = 1 << 6;
constexpr bitvector_t WEATHER_MEDIUMSNOW = 1 << 7;
constexpr bitvector_t WEATHER_BIGSNOW = 1 << 8;
constexpr bitvector_t WEATHER_LIGHTWIND = 1 << 9;
constexpr bitvector_t WEATHER_MEDIUMWIND = 1 << 10;
constexpr bitvector_t WEATHER_BIGWIND = 1 << 11;

template<typename T>
struct Unimplemented {};

template<typename E>
const std::string &NAME_BY_ITEM(const E item) {
	return Unimplemented<E>::FAIL;
}

template<typename E>
E ITEM_BY_NAME(const std::string &name) {
	return Unimplemented<E>::FAIL;
}

template<typename E>
inline E ITEM_BY_NAME(const char *name) { return ITEM_BY_NAME<E>(std::string(name)); }

// PC Kin - выпилить к чертям
const __uint8_t kNumKins = 3;

// Descriptor flags //
constexpr bitvector_t DESC_CANZLIB = 1 << 0;    // Client says compression capable.   //

// Affect bits: used in char_data.char_specials.saved.affected_by //
// WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") //
enum class EAffectFlag : bitvector_t {
	AFF_BLIND = 1u << 0,                    ///< (R) Char is blind
	AFF_INVISIBLE = 1u << 1,                ///< Char is invisible
	AFF_DETECT_ALIGN = 1u << 2,                ///< Char is sensitive to align
	AFF_DETECT_INVIS = 1u << 3,                ///< Char can see invis entities
	AFF_DETECT_MAGIC = 1u << 4,                ///< Char is sensitive to magic
	AFF_SENSE_LIFE = 1u << 5,                ///< Char can sense hidden life
	AFF_WATERWALK = 1u << 6,                ///< Char can walk on water
	AFF_SANCTUARY = 1u << 7,                ///< Char protected by sanct.
	AFF_GROUP = 1u << 8,                    ///< (R) Char is grouped
	AFF_CURSE = 1u << 9,                    ///< Char is cursed
	AFF_INFRAVISION = 1u << 10,                ///< Char can see in dark
	AFF_POISON = 1u << 11,                    ///< (R) Char is poisoned
	AFF_PROTECT_EVIL = 1u << 12,            ///< Char protected from evil
	AFF_PROTECT_GOOD = 1u << 13,            ///< Char protected from good
	AFF_SLEEP = 1u << 14,                    ///< (R) Char magically asleep
	AFF_NOTRACK = 1u << 15,                    ///< Char can't be tracked
	AFF_TETHERED = 1u << 16,                ///< Room for future expansion
	AFF_BLESS = 1u << 17,                    ///< Room for future expansion
	AFF_SNEAK = 1u << 18,                    ///< Char can move quietly
	AFF_HIDE = 1u << 19,                    ///< Char is hidden
	AFF_COURAGE = 1u << 20,                    ///< Room for future expansion
	AFF_CHARM = 1u << 21,                    ///< Char is charmed
	AFF_HOLD = 1u << 22,
	AFF_FLY = 1u << 23,
	AFF_SILENCE = 1u << 24,
	AFF_AWARNESS = 1u << 25,
	AFF_BLINK = 1u << 26,
	AFF_HORSE = 1u << 27,                    ///< NPC - is horse, PC - is horsed
	AFF_NOFLEE = 1u << 28,
	AFF_SINGLELIGHT = 1u << 29,
	AFF_HOLYLIGHT = kIntOne | (1u << 0),
	AFF_HOLYDARK = kIntOne | (1u << 1),
	AFF_DETECT_POISON = kIntOne | (1u << 2),
	AFF_DRUNKED = kIntOne | (1u << 3),
	AFF_ABSTINENT = kIntOne | (1u << 4),
	AFF_STOPRIGHT = kIntOne | (1u << 5),
	AFF_STOPLEFT = kIntOne | (1u << 6),
	AFF_STOPFIGHT = kIntOne | (1u << 7),
	AFF_HAEMORRAGIA = kIntOne | (1u << 8),
	AFF_CAMOUFLAGE = kIntOne | (1u << 9),
	AFF_WATERBREATH = kIntOne | (1u << 10),
	AFF_SLOW = kIntOne | (1u << 11),
	AFF_HASTE = kIntOne | (1u << 12),
	AFF_SHIELD = kIntOne | (1u << 13),
	AFF_AIRSHIELD = kIntOne | (1u << 14),
	AFF_FIRESHIELD = kIntOne | (1u << 15),
	AFF_ICESHIELD = kIntOne | (1u << 16),
	AFF_MAGICGLASS = kIntOne | (1u << 17),
	AFF_STAIRS = kIntOne | (1u << 18),
	AFF_STONEHAND = kIntOne | (1u << 19),
	AFF_PRISMATICAURA = kIntOne | (1u << 20),
	AFF_HELPER = kIntOne | (1u << 21),
	AFF_EVILESS = kIntOne | (1u << 22),
	AFF_AIRAURA = kIntOne | (1u << 23),
	AFF_FIREAURA = kIntOne | (1u << 24),
	AFF_ICEAURA = kIntOne | (1u << 25),
	AFF_DEAFNESS = kIntOne | (1u << 26),
	AFF_CRYING = kIntOne | (1u << 27),
	AFF_PEACEFUL = kIntOne | (1u << 28),
	AFF_MAGICSTOPFIGHT = kIntOne | (1u << 29),
	AFF_BERSERK = kIntTwo | (1u << 0),
	AFF_LIGHT_WALK = kIntTwo | (1u << 1),
	AFF_BROKEN_CHAINS = kIntTwo | (1u << 2),
	AFF_CLOUD_OF_ARROWS = kIntTwo | (1u << 3),
	AFF_SHADOW_CLOAK = kIntTwo | (1u << 4),
	AFF_GLITTERDUST = kIntTwo | (1u << 5),
	AFF_AFFRIGHT = kIntTwo | (1u << 6),
	AFF_SCOPOLIA_POISON = kIntTwo | (1u << 7),
	AFF_DATURA_POISON = kIntTwo | (1u << 8),
	AFF_SKILLS_REDUCE = kIntTwo | (1u << 9),
	AFF_NOT_SWITCH = kIntTwo | (1u << 10),
	AFF_BELENA_POISON = kIntTwo | (1u << 11),
	AFF_NOTELEPORT = kIntTwo | (1u << 12),
	AFF_LACKY = kIntTwo | (1u << 13),
	AFF_BANDAGE = kIntTwo | (1u << 14),
	AFF_NO_BANDAGE = kIntTwo | (1u << 15),
	AFF_MORPH = kIntTwo | (1u << 16),
	AFF_STRANGLED = kIntTwo | (1u << 17),
	AFF_RECALL_SPELLS = kIntTwo | (1u << 18),
	AFF_NOOB_REGEN = kIntTwo | (1u << 19),
	AFF_VAMPIRE = kIntTwo | (1u << 20),
	AFF_EXPEDIENT = kIntTwo | (1u << 21),
	AFF_COMMANDER = kIntTwo | (1u << 22),
	AFF_EARTHAURA = kIntTwo | (1u << 23),
	AFF_DOMINATION = kIntTwo | (1u << 24)
};

template<>
const std::string &NAME_BY_ITEM<EAffectFlag>(EAffectFlag item);
template<>
EAffectFlag ITEM_BY_NAME<EAffectFlag>(const std::string &name);

typedef std::list<EAffectFlag> affects_list_t;

// Modes of connectedness: used by descriptor_data.state //
//ОБЕЗАТЕЛЬНО ДОБАВИТЬ В connected_types[]!!!!//
const __uint8_t CON_PLAYING = 0;        // Playing - Nominal state //
const __uint8_t CON_CLOSE = 1;            // Disconnecting     //
const __uint8_t CON_GET_NAME = 2;        // By what name ..?     //
const __uint8_t CON_NAME_CNFRM = 3;    // Did I get that right, x?   //
const __uint8_t CON_PASSWORD = 4;        // Password:         //
const __uint8_t CON_NEWPASSWD = 5;        // Give me a password for x   //
const __uint8_t CON_CNFPASSWD = 6;        // Please retype password: //
const __uint8_t CON_QSEX = 7;            // Sex?           //
const __uint8_t CON_QCLASS = 8;        // Class?         //
const __uint8_t CON_RMOTD = 9;            // PRESS RETURN after MOTD //
const __uint8_t CON_MENU = 10;            // Your choice: (main menu)   //
const __uint8_t CON_EXDESC = 11;        // Enter a new description:   //
const __uint8_t CON_CHPWD_GETOLD = 12; // Changing passwd: get old   //
const __uint8_t CON_CHPWD_GETNEW = 13; // Changing passwd: get new   //
const __uint8_t CON_CHPWD_VRFY = 14;    // Verify new password     //
const __uint8_t CON_DELCNF1 = 15;        // Delete confirmation 1   //
const __uint8_t CON_DELCNF2 = 16;        // Delete confirmation 2   //
const __uint8_t CON_DISCONNECT = 17;    // In-game disconnection   //
const __uint8_t CON_OEDIT = 18;        //. OLC mode - object edit     . //
const __uint8_t CON_REDIT = 19;        //. OLC mode - room edit       . //
const __uint8_t CON_ZEDIT = 20;        //. OLC mode - zone info edit  . //
const __uint8_t CON_MEDIT = 21;        //. OLC mode - mobile edit     . //
const __uint8_t CON_TRIGEDIT = 22;        //. OLC mode - trigger edit    . //
const __uint8_t CON_NAME2 = 23;
const __uint8_t CON_NAME3 = 24;
const __uint8_t CON_NAME4 = 25;
const __uint8_t CON_NAME5 = 26;
const __uint8_t CON_NAME6 = 27;
const __uint8_t CON_RELIGION = 28;
const __uint8_t CON_RACE = 29;
const __uint8_t CON_LOWS = 30;
const __uint8_t CON_GET_KEYTABLE = 31;
const __uint8_t CON_GET_EMAIL = 32;
const __uint8_t CON_ROLL_STATS = 33;
const __uint8_t CON_MREDIT = 34;        // OLC mode - make recept edit //
const __uint8_t CON_QKIN = 35;
const __uint8_t CON_QCLASSV = 36;
const __uint8_t CON_QCLASSS = 37;
const __uint8_t CON_MAP_MENU = 38;
const __uint8_t CON_COLOR = 39;
const __uint8_t CON_WRITEBOARD = 40;        // написание на доску
const __uint8_t CON_CLANEDIT = 41;            // команда house
const __uint8_t CON_NEW_CHAR = 42;
const __uint8_t CON_SPEND_GLORY = 43;        // вливание славы через команду у чара
const __uint8_t CON_RESET_STATS = 44;        // реролл статов при входе в игру
const __uint8_t CON_BIRTHPLACE = 45;        // выбираем где начать игру
const __uint8_t CON_WRITE_MOD = 46;        // пишет клановое сообщение дня
const __uint8_t CON_GLORY_CONST = 47;        // вливает славу2
const __uint8_t CON_NAMED_STUFF = 48;        // редактирует именной стаф
const __uint8_t
	CON_RESET_KIN = 49;        // выбор расы после смены/удаления оной (или иного способа испоганивания значения)
const __uint8_t CON_RESET_RACE = 50;        // выбор РОДА посла смены/сброса оного
const __uint8_t CON_CONSOLE = 51;            // Интерактивная скриптовая консоль
const __uint8_t CON_TORC_EXCH = 52;        // обмен гривен
const __uint8_t CON_MENU_STATS = 53;        // оплата сброса стартовых статов из главного меню
const __uint8_t CON_SEDIT = 54;            // sedit - редактирование сетов
const __uint8_t CON_RESET_RELIGION = 55;    // сброс религии из меню сброса статов
const __uint8_t CON_RANDOM_NUMBER = 56;    // Verification code entry: where player enter in the game from new location
const __uint8_t CON_INIT = 57;                // just connected
// не забываем отражать новые состояния в connected_types -- Krodo

// object-related defines ******************************************* //

template<typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
	return static_cast<typename std::underlying_type<E>::type>(e);
}

// Modifier constants used with obj affects ('A' fields) //
enum EApplyLocation {
	APPLY_NONE = 0,    // No effect         //
	APPLY_STR = 1,    // Apply to strength    //
	APPLY_DEX = 2,    // Apply to dexterity      //
	APPLY_INT = 3,    // Apply to constitution   //
	APPLY_WIS = 4,    // Apply to wisdom      //
	APPLY_CON = 5,    // Apply to constitution   //
	APPLY_CHA = 6,    // Apply to charisma    //
	APPLY_CLASS = 7,    // Reserved       //
	APPLY_LEVEL = 8,    // Reserved       //
	APPLY_AGE = 9,    // Apply to age         //
	APPLY_CHAR_WEIGHT = 10,    // Apply to weight      //
	APPLY_CHAR_HEIGHT = 11,    // Apply to height      //
	APPLY_MANAREG = 12,    // Apply to max mana    //
	APPLY_HIT = 13,    // Apply to max hit points //
	APPLY_MOVE = 14,    // Apply to max move points   //
	APPLY_GOLD = 15,    // Reserved       //
	APPLY_EXP = 16,    // Reserved       //
	APPLY_AC = 17,    // Apply to Armor Class    //
	APPLY_HITROLL = 18,    // Apply to hitroll     //
	APPLY_DAMROLL = 19,    // Apply to damage roll    //
	APPLY_SAVING_WILL = 20,    // Apply to save throw: paralz   //
	APPLY_RESIST_FIRE = 21,    // Apply to RESIST throw: fire  //
	APPLY_RESIST_AIR = 22,    // Apply to RESIST throw: air   //
	APPLY_SAVING_CRITICAL = 23,    // Apply to save throw: breath   //
	APPLY_SAVING_STABILITY = 24,    // Apply to save throw: spells   //
	APPLY_HITREG = 25,
	APPLY_MOVEREG = 26,
	APPLY_C1 = 27,
	APPLY_C2 = 28,
	APPLY_C3 = 29,
	APPLY_C4 = 30,
	APPLY_C5 = 31,
	APPLY_C6 = 32,
	APPLY_C7 = 33,
	APPLY_C8 = 34,
	APPLY_C9 = 35,
	APPLY_SIZE = 36,
	APPLY_ARMOUR = 37,
	APPLY_POISON = 38,
	APPLY_SAVING_REFLEX = 39,
	APPLY_CAST_SUCCESS = 40,
	APPLY_MORALE = 41,
	APPLY_INITIATIVE = 42,
	APPLY_RELIGION = 43,
	APPLY_ABSORBE = 44,
	APPLY_LIKES = 45,
	APPLY_RESIST_WATER = 46,    // Apply to RESIST throw: water  //
	APPLY_RESIST_EARTH = 47,    // Apply to RESIST throw: earth  //
	APPLY_RESIST_VITALITY = 48,    // Apply to RESIST throw: light, dark, critical damage  //
	APPLY_RESIST_MIND = 49,    // Apply to RESIST throw: mind magic  //
	APPLY_RESIST_IMMUNITY = 50,    // Apply to RESIST throw: poison, disease etc.  //
	APPLY_AR = 51,    // Apply to Magic affect resist //
	APPLY_MR = 52,    // Apply to Magic damage resist //
	APPLY_ACONITUM_POISON = 53,
	APPLY_SCOPOLIA_POISON = 54,
	APPLY_BELENA_POISON = 55,
	APPLY_DATURA_POISON = 56,
	APPLY_FREE_FOR_USE = 57, // занимайте
	APPLY_BONUS_EXP = 58,
	APPLY_BONUS_SKILLS = 59,
	APPLY_PLAQUE = 60,
	APPLY_MADNESS = 61,
	APPLY_PR = 62,
	APPLY_RESIST_DARK = 63,
	APPLY_VIEW_DT = 64,
	APPLY_PERCENT_EXP = 65, //бонус +экспа
	APPLY_PERCENT_DAM = 66, // бонус +повреждение
	APPLY_SPELL_BLINK = 67, // мигание заклом
	NUM_APPLIES
};

template<>
const std::string &NAME_BY_ITEM<EApplyLocation>(EApplyLocation item);
template<>
EApplyLocation ITEM_BY_NAME<EApplyLocation>(const std::string &name);

// APPLY - эффекты для комнат //
enum ERoomApplyLocation {
	APPLY_ROOM_NONE = 0,
	APPLY_ROOM_POISON = 1,    // Изменяет в комнате уровень ядности //
	APPLY_ROOM_FLAME = 2,    // Изменяет в комнате уровень огня (для потомков) //
	NUM_ROOM_APPLIES = 3
};

struct obj_affected_type {
	EApplyLocation location;    // Which ability to change (APPLY_XXX) //
	int modifier;                // How much it changes by              //

	obj_affected_type() : location(APPLY_NONE), modifier(0) {}

	obj_affected_type(EApplyLocation __location, int __modifier)
		: location(__location), modifier(__modifier) {}

	// для сравнения в sedit
	bool operator!=(const obj_affected_type &r) const {
		return (location != r.location || modifier != r.modifier);
	}
	bool operator==(const obj_affected_type &r) const {
		return !(*this != r);
	}
};

constexpr bitvector_t TRACK_NPC = 1 << 0;
constexpr bitvector_t TRACK_HIDE = 1 << 1;

// other miscellaneous defines ****************************************** //

enum { DRUNK, FULL, THIRST };
// pernalty types
enum { P_DAMROLL, P_HITROLL, P_CAST, P_MEM_GAIN, P_MOVE_GAIN, P_HIT_GAIN, P_AC };

// Sun state for weather_data //
const __uint8_t SUN_DARK = 0;
const __uint8_t SUN_RISE = 1;
const __uint8_t SUN_LIGHT = 2;
const __uint8_t SUN_SET = 3;

// Moon change type //
const __uint8_t MOON_INCREASE = 0;
const __uint8_t MOON_DECREASE = 1;

// Sky conditions for weather_data //
const __uint8_t SKY_CLOUDLESS = 0;
const __uint8_t SKY_CLOUDY = 1;
const __uint8_t SKY_RAINING = 2;
const __uint8_t SKY_LIGHTNING = 3;

constexpr bitvector_t EXTRA_FAILHIDE = 1 << 0;
constexpr bitvector_t EXTRA_FAILSNEAK = 1 << 1;
constexpr bitvector_t EXTRA_FAILCAMOUFLAGE = 1 << 2;
constexpr bitvector_t EXTRA_GRP_KILL_COUNT = 1 << 3; // для избежания повторных записей моба в списки SetsDrop

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
const __uint8_t LVL_FREEZE = LVL_GRGOD;

const __uint8_t NUM_OF_DIRS = 6;        // number of directions in a room (nsewud) //
const __uint8_t MAGIC_NUMBER = 0x06;    // Arbitrary number that won't be in a string //

constexpr long long OPT_USEC = 40000;    // 25 passes per second //
constexpr long long PASSES_PER_SEC = 1000000 / OPT_USEC;

#define RL_SEC    * PASSES_PER_SEC

#define PULSE_ZONE      (1 RL_SEC)
#define PULSE_MOBILE    (10 RL_SEC)
#define PULSE_VIOLENCE  (2 RL_SEC)
#define ZONES_RESET    1    // number of zones to reset at one time //
#define PULSE_LOGROTATE (10 RL_SEC)

// Variables for the output buffering system //
constexpr __uint16_t kMaxSockBuf = 48 * 1024;        // Size of kernel's sock buf   //
const __uint16_t kMaxPromptLength = 256;        // Max length of prompt        //
const __uint8_t kGarbageSpace = 32;                // Space for **OVERFLOW** etc  //
const __uint16_t kSmallBufsize = 1024;            // Static output buffer size   //
// Max amount of output that can be buffered //
constexpr __uint16_t kLargeBufSize = kMaxSockBuf - kGarbageSpace - kMaxPromptLength;

// Keep last 5 commands
const int kHistorySize = 5;
const int kMaxStringLength = 32768;
const int kMaxExtendLength = 0xFFFF;
const int kMaxTrglineLength = 1024;
const int kMaxInputLength = 1024;   // Max length per *line* of input //
const int kMaxRawInputLength = 1024;   // Max size of *raw* input //
const int kMaxMessages = 600;
const int kMaxNameLength = 20;
const int kMinNameLength = 5;
const int kHostLength = 30;
const int kExdscrLength = 512;
const int kMaxAffect = 128;
const int kMaxObjAffect = 8;
const int kMaxTimedSkills = 16;
const int kMaxFeats = 256;
const int kMaxTimedFeats = 16;
const int kMaxHits = 32000; // Максимальное количество хитов и дамага //
const long kMaxMoneyKept = 1000000000; // планка на кол-во денег у чара на руках и в банке (раздельно) //

const short INT_STUPID_MOD = 10;
const short INT_MIDDLE_AI = 30;
const short INT_HIGH_AI = 40;
const short CHARACTER_HP_FOR_MOB_PRIORITY_ATTACK = 100;
const short STRONG_MOB_LEVEL = 30;
const short MAX_MOB_LEVEL = 100;
const short MAX_SAVE = 400; //максимальное значение воля, здоровье, стойкость, реакция

bool sprintbitwd(bitvector_t bitvector, const char *names[], char *result, const char *div, const int print_flag = 0);

inline bool sprintbit(bitvector_t bitvector, const char *names[], char *result, const int print_flag = 0) {
	return sprintbitwd(bitvector, names, result, ",", print_flag);
}

// Extra description: used in objects, mobiles, and rooms //
struct EXTRA_DESCR_DATA {
	using shared_ptr = std::shared_ptr<EXTRA_DESCR_DATA>;
	EXTRA_DESCR_DATA() : keyword(nullptr), description(nullptr), next(nullptr) {}
	~EXTRA_DESCR_DATA();
	void set_keyword(std::string const &keyword);
	void set_description(std::string const &description);

	char *keyword;        // Keyword in look/examine          //
	char *description;    // What to see                      //
	shared_ptr next;    // Next in list                     //
};

// header block for rent files.  BEWARE: Changing it will ruin rent files  //
struct save_rent_info {
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

struct save_time_info {
	int32_t vnum;
	int32_t timer;
};

struct save_info {
	struct save_rent_info rent;
	std::vector<save_time_info> time;
};

// =======================================================================


// char-related structures ***********************************************


// memory structure for characters //
struct memory_rec_struct {
	long id;
	long time;
	struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;

// This structure is purely intended to be an easy way to transfer //
// and return information about time (real or mudwise).            //
struct time_info_data {
	int
		hours, day, month;
	sh_int year;
};

// Alez
struct logon_data {
	char *ip;
	long count;
	time_t lasttime; //by kilnik
	bool is_first;
};

class punish_data {
 public:
	punish_data();

	long duration;
	char *reason;
	int level;
	long godid;
};

struct timed_type {
	ubyte skill;        // Number of used skill/spell //
	ubyte time;        // Time for next using        //
	struct timed_type *next;
};

// Structure used for entities following other entities //
struct follow_type {
	CHAR_DATA *follower;
	struct follow_type *next;
};

// Structure used for tracking a mob //
struct track_info {
	int trk_info;
	int who;
	int dirs;
};

// Structure used for helpers //
struct helper_data_type {
	int
		mob_vnum;
	struct helper_data_type *next_helper;
};

// Structure used for on_dead object loading //
struct load_data {
	int
		obj_vnum;
	int
		load_prob;
	int
		load_type;
	int
		spec_param;
};

typedef std::list<struct load_data *> load_list;

struct spell_mem_queue_item {
	int
		spellnum;
	struct spell_mem_queue_item *link;
};

// descriptor-related structures ****************************************

struct txt_block {
	char *text;
	int
		aliased;
	struct txt_block *next;
};

struct txt_q {
	struct txt_block *head;
	struct txt_block *tail;
	txt_q() :
		head(nullptr), tail(nullptr) {}
};

namespace Glory {

class spend_glory;

}

namespace GloryConst {

struct glory_olc;

}

namespace NamedStuff {

struct stuff_node;

}

#if defined WITH_SCRIPTING
namespace scripting
{
	class Console;
}
#endif

namespace MapSystem {
struct Options;
}

namespace obj_sets_olc {
class sedit;
}

#ifndef HAVE_ZLIB
struct z_stream;
#endif

class AbstractStringWriter {
 public:
	using shared_ptr = std::shared_ptr<AbstractStringWriter>;

	virtual ~AbstractStringWriter() {}
	virtual const char *get_string() const = 0;
	virtual void set_string(const char *data) = 0;
	virtual void append_string(const char *data) = 0;
	virtual size_t length() const = 0;
	virtual void clear() = 0;
};

class DelegatedStringWriter : public AbstractStringWriter {
 public:
	DelegatedStringWriter(char *&managed) : m_delegated_string(managed) {}
	virtual const char *get_string() const override { return m_delegated_string; }
	virtual void set_string(const char *string) override;
	virtual void append_string(const char *string) override;
	virtual size_t length() const override { return m_delegated_string ? strlen(m_delegated_string) : 0; }
	virtual void clear() override;

 private:
	char *&m_delegated_string;
};

class AbstractStdStringWriter : public AbstractStringWriter {
 public:
	virtual const char *get_string() const override { return string().c_str(); }
	virtual void set_string(const char *string) override { this->string() = string; }
	virtual void append_string(const char *string) override { this->string() += string; }
	virtual size_t length() const override { return string().length(); }
	virtual void clear() override { string().clear(); }

 private:
	virtual std::string &string() = 0;
	virtual const std::string &string() const = 0;
};

class StdStringWriter : public AbstractStdStringWriter {
 private:
	virtual std::string &string() override { return m_string; }
	virtual const std::string &string() const override { return m_string; }

	std::string m_string;
};

class DelegatedStdStringWriter : public AbstractStringWriter {
 public:
	DelegatedStdStringWriter(std::string &string) : m_string(string) {}
	virtual const char *get_string() const override { return m_string.c_str(); }
	virtual void set_string(const char *string) override { m_string = string; }
	virtual void append_string(const char *string) override { m_string += string; }
	virtual size_t length() const override { return m_string.length(); }
	virtual void clear() override { m_string.clear(); }

 private:
	std::string &m_string;
};

// other miscellaneous structures **************************************


struct msg_type {
	char *attacker_msg;    // message to attacker //
	char *victim_msg;    // message to victim   //
	char *room_msg;        // message to room     //
};

struct message_type {
	struct msg_type die_msg;        // messages when death        //
	struct msg_type miss_msg;        // messages when miss         //
	struct msg_type hit_msg;        // messages when hit       //
	struct msg_type god_msg;        // messages when hit on god      //
	struct message_type *next;    // to next messages of this kind.   //
};

struct message_list {
	int a_type;        // Attack type          //
	int number_of_attacks;    // How many attack messages to chose from. //
	struct message_type *msg;    // List of messages.       //
};

struct zone_type {
	char *name;            // type name //
	int ingr_qty;        // quantity of ingredient types //
	int *ingr_types;    // types of ingredients, which are loaded in zones of this type //
};

struct int_app_type {
	int spell_aknowlege;    // chance to know spell               //
	int to_skilluse;        // ADD CHANSE FOR USING SKILL         //
	int mana_per_tic;
	int spell_success;        //  max count of spell on 1s level    //
	int improve;        // chance to improve skill           //
	int observation;        // chance to use SKILL_AWAKE/CRITICAL //
};

struct cha_app_type {
	int leadership;
	int charms;
	int morale;
	int illusive;
	int dam_to_hit_rate;
};

struct size_app_type {
	int ac;            // ADD VALUE FOR AC           //
	int interpolate;        // ADD VALUE FOR SOME SKILLS  //
	int initiative;
	int shocking;
};

struct weapon_app_type {
	int shocking;
	int bashing;
	int parrying;
};

struct extra_affects_type {
	EAffectFlag affect;
	bool set_or_clear;
};

struct class_app_type {
	using extra_affects_list_t = std::vector<extra_affects_type>;

	int unknown_weapon_fault;
	int koef_con;
	int base_con;
	int min_con;
	int max_con;

	const extra_affects_list_t *extra_affects;
};

struct race_app_type {
	struct extra_affects_type *extra_affects;
	struct obj_affected_type *extra_modifiers;
};

struct weather_data {
	int hours_go;        // Time life from reboot //

	int pressure;        // How is the pressure ( Mb )            //
	int press_last_day;    // Average pressure last day             //
	int press_last_week;    // Average pressure last week            //

	int temperature;        // How is the temperature (C)            //
	int temp_last_day;        // Average temperature last day          //
	int temp_last_week;    // Average temperature last week         //

	int rainlevel;        // Level of water from rains             //
	int snowlevel;        // Level of snow                         //
	int icelevel;        // Level of ice                          //

	int weather_type;        // bitvector - some values for month     //

	int change;        // How fast and what way does it change. //
	int sky;            // How is the sky.   //
	int sunlight;        // And how much sun. //
	int moon_day;        // And how much moon //
	int season;
	int week_day_mono;
	int week_day_poly;
};

enum class EWeaponAffectFlag : bitvector_t {
	WAFF_BLINDNESS = (1 << 0),
	WAFF_INVISIBLE = (1 << 1),
	WAFF_DETECT_ALIGN = (1 << 2),
	WAFF_DETECT_INVISIBLE = (1 << 3),
	WAFF_DETECT_MAGIC = (1 << 4),
	WAFF_SENSE_LIFE = (1 << 5),
	WAFF_WATER_WALK = (1 << 6),
	WAFF_SANCTUARY = (1 << 7),
	WAFF_CURSE = (1 << 8),
	WAFF_INFRAVISION = (1 << 9),
	WAFF_POISON = (1 << 10),
	WAFF_PROTECT_EVIL = (1 << 11),
	WAFF_PROTECT_GOOD = (1 << 12),
	WAFF_SLEEP = (1 << 13),
	WAFF_NOTRACK = (1 << 14),
	WAFF_BLESS = (1 << 15),
	WAFF_SNEAK = (1 << 16),
	WAFF_HIDE = (1 << 17),
	WAFF_HOLD = (1 << 18),
	WAFF_FLY = (1 << 19),
	WAFF_SILENCE = (1 << 20),
	WAFF_AWARENESS = (1 << 21),
	WAFF_BLINK = (1 << 22),
	WAFF_NOFLEE = (1 << 23),
	WAFF_SINGLE_LIGHT = (1 << 24),
	WAFF_HOLY_LIGHT = (1 << 25),
	WAFF_HOLY_DARK = (1 << 26),
	WAFF_DETECT_POISON = (1 << 27),
	WAFF_SLOW = (1 << 28),
	WAFF_HASTE = (1 << 29),
	WAFF_WATER_BREATH = kIntOne | (1 << 0),
	WAFF_HAEMORRAGIA = kIntOne | (1 << 1),
	WAFF_CAMOUFLAGE = kIntOne | (1 << 2),
	WAFF_SHIELD = kIntOne | (1 << 3),
	WAFF_AIR_SHIELD = kIntOne | (1 << 4),
	WAFF_FIRE_SHIELD = kIntOne | (1 << 5),
	WAFF_ICE_SHIELD = kIntOne | (1 << 6),
	WAFF_MAGIC_GLASS = kIntOne | (1 << 7),
	WAFF_STONE_HAND = kIntOne | (1 << 8),
	WAFF_PRISMATIC_AURA = kIntOne | (1 << 9),
	WAFF_AIR_AURA = kIntOne | (1 << 10),
	WAFF_FIRE_AURA = kIntOne | (1 << 11),
	WAFF_ICE_AURA = kIntOne | (1 << 12),
	WAFF_DEAFNESS = kIntOne | (1 << 13),
	WAFF_COMMANDER = kIntOne | (1 << 14),
	WAFF_EARTHAURA = kIntOne | (1 << 15),
	WAFF_DOMINATION = kIntOne | (1 << 16),
// не забудьте поправить WAFF_COUNT
};

constexpr size_t WAFF_COUNT = 47;

template<>
EWeaponAffectFlag ITEM_BY_NAME<EWeaponAffectFlag>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM(const EWeaponAffectFlag item);

struct weapon_affect_types {
	EWeaponAffectFlag aff_pos;
	uint32_t aff_bitvector;
	int aff_spell;
};

struct title_type {
	char *title_m;
	char *title_f;
	int exp;
};

struct index_data {
	index_data() : vnum(0), number(0), stored(0), func(nullptr), farg(nullptr), proto(nullptr), zone(0), set_idx(-1) {}
	index_data(int _vnum)
		: vnum(_vnum), number(0), stored(0), func(nullptr), farg(nullptr), proto(nullptr), zone(0), set_idx(-1) {}

	int vnum;            // virtual number of this mob/obj       //
	int number;        // number of existing units of this mob/obj //
	int stored;        // number of things in rent file            //
	int (*func)(CHAR_DATA *, void *, int, char *);
	char *farg;        // string argument for special function     //
	TRIG_DATA *proto;    // for triggers... the trigger     //
	int zone;            // mob/obj zone rnum //
	size_t set_idx; // индекс сета в obj_sets::set_list, если != -1
};

struct social_messg        // No argument was supplied //
{
	int ch_min_pos;
	int ch_max_pos;
	int vict_min_pos;
	int vict_max_pos;
	char *char_no_arg;
	char *others_no_arg;

	// An argument was there, and a victim was found //
	char *char_found;    // if NULL, read no further, ignore args //
	char *others_found;
	char *vict_found;

	// An argument was there, but no victim was found //
	char *not_found;
};

struct social_keyword {
	char *keyword;
	int social_message;
};

extern struct social_messg *soc_mess_list;
extern struct social_keyword *soc_keys_list;

struct pray_affect_type {
	int metter;
	EApplyLocation location;
	int modifier;
	uint32_t bitvector;
	int battleflag;
};

const __uint8_t GAPPLY_NONE = 0;
const __uint8_t GAPPLY_SKILL_SUCCESS = 1;
const __uint8_t GAPPLY_SPELL_SUCCESS = 2;
const __uint8_t GAPPLY_SPELL_EFFECT = 3;
const __uint8_t GAPPLY_MODIFIER = 4;
const __uint8_t GAPPLY_AFFECT = 5;

/* pclean_criteria_data структура которая определяет через какой время
   неактивности будет удален чар
*/
struct pclean_criteria_data {
	int level;            // max уровень для этого временного лимита //
	int days;            // временной лимит в днях        //
};

// Структрура для описания проталов для спела townportal //
struct portals_list_type {
	char *wrd;        // кодовое слово //
	int vnum;            // vnum комнаты для портала (раньше был rnum, но зачем тут rnum?) //
	int level;            // минимальный уровень для запоминания портала //
	struct portals_list_type *next_portal;
};

struct char_portal_type {
	int vnum;            // vnum комнаты для портала //
	struct char_portal_type *next;
};

// Структуры для act.wizard.cpp //

struct show_struct {
	const char *cmd;
	const char level;
};

struct set_struct {
	const char *cmd;
	const char level;
	const char pcnpc;
	const char type;
};

//Polos.insert_wanted_gem
struct int3 {
	int type;
	int bit;
	int qty;
};

typedef std::unordered_map<std::string, int3> alias_type;

class insert_wanted_gem {
	std::unordered_map<int, alias_type> content;

 public:
	void init();
	void show(CHAR_DATA *ch, int gem_vnum);
	int get_type(int gem_vnum, const std::string &str);
	int get_bit(int gem_vnum, const std::string &str);
	int get_qty(int gem_vnum, const std::string &str);
	int exist(const int gem_vnum, const std::string &str) const;
	bool is_gem(int get_vnum);
	std::string get_random_str_for(const int gem_vnum);
};

//-Polos.insert_wanted_gem

struct mob_guardian {
	int max_wars_allow;
	bool agro_killers;
	bool agro_all_agressors;
	std::vector<zone_vnum> agro_argressors_in_zones;
};

typedef std::unordered_map<int, mob_guardian> guardian_type;

#endif // __STRUCTS_H__ //

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
