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

#include "boards/boards_types.h"
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
//-Polos.insert_wanted_gem

/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD 3.0 to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ 1        // TRUE/FALSE aren't defined yet.

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

// Флаги комнатных аффектов НЕ сохраняются в файлах и возникают только от заклов //
constexpr bitvector_t AFF_ROOM_LIGHT = 1 << 0;
constexpr bitvector_t AFF_ROOM_FOG = 1 << 1;
constexpr bitvector_t AFF_ROOM_RUNE_LABEL = 1 << 2;                // Комната помечена SPELL_MAGIC_LABEL //
constexpr bitvector_t AFF_ROOM_FORBIDDEN = 1 << 3;                // Комната помечена SPELL_FORBIDDEN //
constexpr bitvector_t AFF_ROOM_HYPNOTIC_PATTERN = 1 << 4;        // Комната под SPELL_HYPNOTIC_PATTERN //
constexpr bitvector_t AFF_ROOM_EVARDS_BLACK_TENTACLES = 1 << 5; // Комната под SPELL_EVARDS_BLACK_TENTACLES //
constexpr bitvector_t AFF_ROOM_METEORSTORM = 1 << 6;            // Комната под SPELL_METEORSTORM //
constexpr bitvector_t AFF_ROOM_THUNDERSTORM = 1 << 7;             // SPELL_THUNDERSTORM

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

enum class ESex : byte {
	kNeutral = 0,
	kMale = 1,
	kFemale = 2,
	kPoly = 3,
	kLast
};

constexpr ESex kDefaultSex = ESex::kMale;

template<>
ESex ITEM_BY_NAME<ESex>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM(ESex item);

// PC religions //
const __uint8_t kReligionPoly = 0;
const __uint8_t kReligionMono = 1;

typedef std::array<const char *, static_cast<std::size_t>(ESex::kLast)> religion_genders_t;
typedef std::array<religion_genders_t, 3> religion_names_t;
extern const religion_names_t religion_name;

// PC races //
// * Все расы персонажей-игроков теперь описываются в playerraces.xml

// PC Kin
const __uint8_t kNumKins = 3;

// NPC races
const __uint8_t NPC_RACE_BASIC = 100;
const __uint8_t NPC_RACE_HUMAN = 101;
const __uint8_t NPC_RACE_HUMAN_ANIMAL = 102;
const __uint8_t NPC_RACE_BIRD = 103;
const __uint8_t NPC_RACE_ANIMAL = 104;
const __uint8_t NPC_RACE_REPTILE = 105;
const __uint8_t NPC_RACE_FISH = 106;
const __uint8_t NPC_RACE_INSECT = 107;
const __uint8_t NPC_RACE_PLANT = 108;
const __uint8_t NPC_RACE_THING = 109;
const __uint8_t NPC_RACE_ZOMBIE = 110;
const __uint8_t NPC_RACE_GHOST = 111;
const __uint8_t NPC_RACE_EVIL_SPIRIT = 112;
const __uint8_t NPC_RACE_SPIRIT = 113;
const __uint8_t NPC_RACE_MAGIC_CREATURE = 114;
const __uint8_t NPC_RACE_NEXT = 115;

// Virtual NPC races
const __uint8_t NPC_BOSS = 200;
const __uint8_t NPC_UNIQUE = 201;

// GODs FLAGS
constexpr bitvector_t GF_GODSLIKE = 1 << 0;
constexpr bitvector_t GF_GODSCURSE = 1 << 1;
constexpr bitvector_t GF_HIGHGOD = 1 << 2;
constexpr bitvector_t GF_REMORT = 1 << 3;
constexpr bitvector_t GF_DEMIGOD = 1 << 4;    // Морталы с привилегиями богов //
constexpr bitvector_t GF_PERSLOG = 1 << 5;
constexpr bitvector_t GF_TESTER = 1 << 6;

// Positions
const __uint8_t POS_DEAD = 0;
const __uint8_t POS_MORTALLYW = 1;    // mortally wounded  //
const __uint8_t POS_INCAP = 2;
const __uint8_t POS_STUNNED = 3;
const __uint8_t POS_SLEEPING = 4;
const __uint8_t POS_RESTING = 5;
const __uint8_t POS_SITTING = 6;
const __uint8_t POS_FIGHTING = 7;
const __uint8_t POS_STANDING = 8;

// Player flags: used by char_data.char_specials.act
constexpr bitvector_t PLR_KILLER = 1 << 0;            // Player is a player-killer     //
constexpr bitvector_t PLR_THIEF = 1 << 1;            // Player is a player-thief      //
constexpr bitvector_t PLR_FROZEN = 1 << 2;            // Player is frozen        //
constexpr bitvector_t PLR_DONTSET = 1 << 3;            // Don't EVER set (ISNPC bit)  //
constexpr bitvector_t PLR_WRITING = 1 << 4;            // Player writing (board/mail/olc)  //
constexpr bitvector_t PLR_MAILING = 1 << 5;            // Player is writing mail     //
constexpr bitvector_t PLR_CRASH = 1 << 6;            // Player needs to be crash-saved   //
constexpr bitvector_t PLR_SITEOK = 1 << 7;            // Player has been site-cleared  //
constexpr bitvector_t PLR_MUTE = 1 << 8;            // Player not allowed to shout/goss/auct  //
constexpr bitvector_t PLR_NOTITLE = 1 << 9;            // Player not allowed to set title  //
constexpr bitvector_t PLR_DELETED = 1 << 10;        // Player deleted - space reusable  //
constexpr bitvector_t PLR_LOADROOM = 1 << 11;        // Player uses nonstandard loadroom  (не используется) //
constexpr bitvector_t PLR_AUTOBOT = 1 << 12;        // Player автоматический игрок //
constexpr bitvector_t PLR_NODELETE = 1 << 13;        // Player shouldn't be deleted //
constexpr bitvector_t PLR_INVSTART = 1 << 14;        // Player should enter game wizinvis //
constexpr bitvector_t PLR_CRYO = 1 << 15;            // Player is cryo-saved (purge prog)   //
constexpr bitvector_t PLR_HELLED = 1 << 16;            // Player is in Hell //
constexpr bitvector_t PLR_NAMED = 1 << 17;            // Player is in Names Room //
constexpr bitvector_t PLR_REGISTERED = 1 << 18;
constexpr bitvector_t PLR_DUMB = 1 << 19;            // Player is not allowed to tell/emote/social //
constexpr bitvector_t PLR_SCRIPTWRITER = 1 << 20;   // скриптер
constexpr bitvector_t PLR_SPAMMER = 1 << 21;        // спаммер
// свободно
constexpr bitvector_t PLR_DELETE = 1 << 28;            // RESERVED - ONLY INTERNALLY (MOB_DELETE) //
constexpr bitvector_t PLR_FREE = 1 << 29;            // RESERVED - ONLY INTERBALLY (MOB_FREE)//


// Mobile flags: used by char_data.char_specials.act
constexpr bitvector_t MOB_SPEC = 1 << 0;            // Mob has a callable spec-proc  //
constexpr bitvector_t MOB_SENTINEL = 1 << 1;        // Mob should not move     //
constexpr bitvector_t MOB_SCAVENGER = 1 << 2;    // Mob picks up stuff on the ground //
constexpr bitvector_t MOB_ISNPC = 1 << 3;            // (R) Automatically set on all Mobs   //
constexpr bitvector_t MOB_AWARE = 1 << 4;            // Mob can't be backstabbed      //
constexpr bitvector_t MOB_AGGRESSIVE = 1 << 5;    // Mob hits players in the room  //
constexpr bitvector_t MOB_STAY_ZONE = 1 << 6;    // Mob shouldn't wander out of zone //
constexpr bitvector_t MOB_WIMPY = 1 << 7;            // Mob flees if severely injured //
constexpr bitvector_t MOB_AGGR_DAY = 1 << 8;        // //
constexpr bitvector_t MOB_AGGR_NIGHT = 1 << 9;    // //
constexpr bitvector_t MOB_AGGR_FULLMOON = 1 << 10;  // //
constexpr bitvector_t MOB_MEMORY = 1 << 11;            // remember attackers if attacked   //
constexpr bitvector_t MOB_HELPER = 1 << 12;            // attack PCs fighting other NPCs   //
constexpr bitvector_t MOB_NOCHARM = 1 << 13;        // Mob can't be charmed    //
constexpr bitvector_t MOB_NOSUMMON = 1 << 14;        // Mob can't be summoned      //
constexpr bitvector_t MOB_NOSLEEP = 1 << 15;        // Mob can't be slept      //
constexpr bitvector_t MOB_NOBASH = 1 << 16;            // Mob can't be bashed (e.g. trees) //
constexpr bitvector_t MOB_NOBLIND = 1 << 17;        // Mob can't be blinded    //
constexpr bitvector_t MOB_MOUNTING = 1 << 18;
constexpr bitvector_t MOB_NOHOLD = 1 << 19;
constexpr bitvector_t MOB_NOSIELENCE = 1 << 20;
constexpr bitvector_t MOB_AGGRMONO = 1 << 21;
constexpr bitvector_t MOB_AGGRPOLY = 1 << 22;
constexpr bitvector_t MOB_NOFEAR = 1 << 23;
constexpr bitvector_t MOB_NOGROUP = 1 << 24;
constexpr bitvector_t MOB_CORPSE = 1 << 25;
constexpr bitvector_t MOB_LOOTER = 1 << 26;
constexpr bitvector_t MOB_PROTECT = 1 << 27;
constexpr bitvector_t MOB_DELETE = 1 << 28;            // RESERVED - ONLY INTERNALLY //
constexpr bitvector_t MOB_FREE = 1 << 29;            // RESERVED - ONLY INTERBALLY //

constexpr bitvector_t MOB_SWIMMING = kIntOne | (1 << 0);
constexpr bitvector_t MOB_FLYING = kIntOne | (1 << 1);
constexpr bitvector_t MOB_ONLYSWIMMING = kIntOne | (1 << 2);
constexpr bitvector_t MOB_AGGR_WINTER = kIntOne | (1 << 3);
constexpr bitvector_t MOB_AGGR_SPRING = kIntOne | (1 << 4);
constexpr bitvector_t MOB_AGGR_SUMMER = kIntOne | (1 << 5);
constexpr bitvector_t MOB_AGGR_AUTUMN = kIntOne | (1 << 6);
constexpr bitvector_t MOB_LIKE_DAY = kIntOne | (1 << 7);
constexpr bitvector_t MOB_LIKE_NIGHT = kIntOne | (1 << 8);
constexpr bitvector_t MOB_LIKE_FULLMOON = kIntOne | (1 << 9);
constexpr bitvector_t MOB_LIKE_WINTER = kIntOne | (1 << 10);
constexpr bitvector_t MOB_LIKE_SPRING = kIntOne | (1 << 11);
constexpr bitvector_t MOB_LIKE_SUMMER = kIntOne | (1 << 12);
constexpr bitvector_t MOB_LIKE_AUTUMN = kIntOne | (1 << 13);
constexpr bitvector_t MOB_NOFIGHT = kIntOne | (1 << 14);
constexpr bitvector_t MOB_EADECREASE = kIntOne | (1 << 15); // понижает количество своих атак по мере убывания тек.хп
constexpr bitvector_t MOB_HORDE = kIntOne | (1 << 16);
constexpr bitvector_t MOB_CLONE = kIntOne | (1 << 17);
constexpr bitvector_t MOB_NOTKILLPUNCTUAL = kIntOne | (1 << 18);
constexpr bitvector_t MOB_NOTRIP = kIntOne | (1 << 19);
constexpr bitvector_t MOB_ANGEL = kIntOne | (1 << 20);
constexpr bitvector_t
	MOB_GUARDIAN = kIntOne | (1 << 21); //Polud моб-стражник, ставится программно, берется из файла guards.xml
constexpr bitvector_t MOB_IGNORE_FORBIDDEN = kIntOne | (1 << 22); // игнорирует печать
constexpr bitvector_t MOB_NO_BATTLE_EXP = kIntOne | (1 << 23); // не дает экспу за удары
constexpr bitvector_t MOB_NOMIGHTHIT = kIntOne | (1 << 24); // нельзя оглушить богатырским молотом
constexpr bitvector_t MOB_GHOST = kIntOne | (1 << 25); // Используется для ментальной тени
constexpr bitvector_t
	MOB_PLAYER_SUMMON = kIntOne | (1 << 26); // Моб является суммоном игрока (ангел, тень, храны, трупы, умки)

constexpr bitvector_t MOB_FIREBREATH = kIntTwo | (1 << 0);
constexpr bitvector_t MOB_GASBREATH = kIntTwo | (1 << 1);
constexpr bitvector_t MOB_FROSTBREATH = kIntTwo | (1 << 2);
constexpr bitvector_t MOB_ACIDBREATH = kIntTwo | (1 << 3);
constexpr bitvector_t MOB_LIGHTBREATH = kIntTwo | (1 << 4);
constexpr bitvector_t MOB_NOTRAIN = kIntTwo | (1 << 5);
constexpr bitvector_t MOB_NOREST = kIntTwo | (1 << 6);
constexpr bitvector_t MOB_AREA_ATTACK = kIntTwo | (1 << 7);
constexpr bitvector_t MOB_NOSTUPOR = kIntTwo | (1 << 8);
constexpr bitvector_t MOB_NOHELPS = kIntTwo | (1 << 9);
constexpr bitvector_t MOB_OPENDOOR = kIntTwo | (1 << 10);
constexpr bitvector_t MOB_IGNORNOMOB = kIntTwo | (1 << 11);
constexpr bitvector_t MOB_IGNORPEACE = kIntTwo | (1 << 12);
constexpr bitvector_t MOB_RESURRECTED =
	kIntTwo | (1 << 13);    // поднят через !поднять труп! или !оживить труп! Ставится только програмно//
constexpr bitvector_t MOB_RUSICH = kIntTwo | (1 << 14);
constexpr bitvector_t MOB_VIKING = kIntTwo | (1 << 15);
constexpr bitvector_t MOB_STEPNYAK = kIntTwo | (1 << 16);
constexpr bitvector_t MOB_AGGR_RUSICHI = kIntTwo | (1 << 17);
constexpr bitvector_t MOB_AGGR_VIKINGI = kIntTwo | (1 << 18);
constexpr bitvector_t MOB_AGGR_STEPNYAKI = kIntTwo | (1 << 19);
constexpr bitvector_t MOB_NORESURRECTION = kIntTwo | (1 << 20);
constexpr bitvector_t MOB_AWAKE = kIntTwo | (1 << 21);
constexpr bitvector_t MOB_IGNORE_FORMATION = kIntTwo | (1 << 22);

constexpr bitvector_t NPC_NORTH = 1 << 0;
constexpr bitvector_t NPC_EAST = 1 << 1;
constexpr bitvector_t NPC_SOUTH = 1 << 2;
constexpr bitvector_t NPC_WEST = 1 << 3;
constexpr bitvector_t NPC_UP = 1 << 4;
constexpr bitvector_t NPC_DOWN = 1 << 5;
constexpr bitvector_t NPC_POISON = 1 << 6;
constexpr bitvector_t NPC_INVIS = 1 << 7;
constexpr bitvector_t NPC_SNEAK = 1 << 8;
constexpr bitvector_t NPC_CAMOUFLAGE = 1 << 9;
constexpr bitvector_t NPC_MOVEFLY = 1 << 11;
constexpr bitvector_t NPC_MOVECREEP = 1 << 12;
constexpr bitvector_t NPC_MOVEJUMP = 1 << 13;
constexpr bitvector_t NPC_MOVESWIM = 1 << 14;
constexpr bitvector_t NPC_MOVERUN = 1 << 15;
constexpr bitvector_t NPC_AIRCREATURE = 1 << 20;
constexpr bitvector_t NPC_WATERCREATURE = 1 << 21;
constexpr bitvector_t NPC_EARTHCREATURE = 1 << 22;
constexpr bitvector_t NPC_FIRECREATURE = 1 << 23;
constexpr bitvector_t NPC_HELPED = 1 << 24;
constexpr bitvector_t NPC_FREEDROP = 1 << 25;
constexpr bitvector_t NPC_NOINGRDROP = 1 << 26;

constexpr bitvector_t NPC_STEALING = kIntOne | (1 << 0);
constexpr bitvector_t NPC_WIELDING = kIntOne | (1 << 1);
constexpr bitvector_t NPC_ARMORING = kIntOne | (1 << 2);
constexpr bitvector_t NPC_USELIGHT = kIntOne | (1 << 3);
constexpr bitvector_t NPC_NOTAKEITEMS = kIntOne | (1 << 4);

// Descriptor flags //
constexpr bitvector_t DESC_CANZLIB = 1 << 0;    // Client says compression capable.   //

// Preference flags: used by char_data.player_specials.pref //
constexpr bitvector_t PRF_BRIEF = 1 << 0;        // Room descs won't normally be shown //
constexpr bitvector_t PRF_COMPACT = 1 << 1;    // No extra CRLF pair before prompts  //
constexpr bitvector_t PRF_NOHOLLER = 1 << 2;    // Не слышит команду "орать"   //
constexpr bitvector_t PRF_NOTELL = 1 << 3;        // Не слышит команду "сказать" //
constexpr bitvector_t PRF_DISPHP = 1 << 4;        // Display hit points in prompt   //
constexpr bitvector_t PRF_DISPMANA = 1 << 5;    // Display mana points in prompt   //
constexpr bitvector_t PRF_DISPMOVE = 1 << 6;    // Display move points in prompt   //
constexpr bitvector_t PRF_AUTOEXIT = 1 << 7;    // Display exits in a room      //
constexpr bitvector_t PRF_NOHASSLE = 1 << 8;    // Aggr mobs won't attack    //
constexpr bitvector_t PRF_SUMMONABLE = 1 << 9;  // Can be summoned         //
constexpr bitvector_t PRF_QUEST = 1 << 10;        // On quest                       //
constexpr bitvector_t PRF_NOREPEAT = 1 << 11;   // No repetition of comm commands  //
constexpr bitvector_t PRF_HOLYLIGHT = 1 << 12;  // Can see in dark        //
constexpr bitvector_t PRF_COLOR_1 = 1 << 13;    // Color (low bit)       //
constexpr bitvector_t PRF_COLOR_2 = 1 << 14;    // Color (high bit)         //
constexpr bitvector_t PRF_NOWIZ = 1 << 15;        // Can't hear wizline       //
constexpr bitvector_t PRF_LOG1 = 1 << 16;        // On-line System Log (low bit)   //
constexpr bitvector_t PRF_LOG2 = 1 << 17;        // On-line System Log (high bit)  //
constexpr bitvector_t PRF_NOAUCT = 1 << 18;        // Can't hear auction channel     //
constexpr bitvector_t PRF_NOGOSS = 1 << 19;        // Не слышит команду "болтать" //
constexpr bitvector_t PRF_DISPFIGHT = 1 << 20;  // Видит свое состояние в бою      //
constexpr bitvector_t PRF_ROOMFLAGS = 1 << 21;  // Can see room flags (ROOM_x)  //
constexpr bitvector_t PRF_DISPEXP = 1 << 22;
constexpr bitvector_t PRF_DISPEXITS = 1 << 23;
constexpr bitvector_t PRF_DISPLEVEL = 1 << 24;
constexpr bitvector_t PRF_DISPGOLD = 1 << 25;
constexpr bitvector_t PRF_DISPTICK = 1 << 26;
constexpr bitvector_t PRF_PUNCTUAL = 1 << 27;
constexpr bitvector_t PRF_AWAKE = 1 << 28;
constexpr bitvector_t PRF_CODERINFO = 1 << 29;

constexpr bitvector_t PRF_AUTOMEM = kIntOne | 1 << 0;
constexpr bitvector_t PRF_NOSHOUT = kIntOne | 1 << 1;                // Не слышит команду "кричать"  //
constexpr bitvector_t PRF_GOAHEAD = kIntOne | 1 << 2;                // Добавление IAC GA после промпта //
constexpr bitvector_t PRF_SHOWGROUP = kIntOne | 1 << 3;            // Показ полного состава группы //
constexpr bitvector_t PRF_AUTOASSIST = kIntOne | 1 << 4;            // Автоматическое вступление в бой //
constexpr bitvector_t PRF_AUTOLOOT = kIntOne | 1 << 5;                // Autoloot //
constexpr bitvector_t PRF_AUTOSPLIT = kIntOne | 1 << 6;            // Autosplit //
constexpr bitvector_t PRF_AUTOMONEY = kIntOne | 1 << 7;            // Automoney //
constexpr bitvector_t PRF_NOARENA = kIntOne | 1 << 8;                // Не слышит арену //
constexpr bitvector_t PRF_NOEXCHANGE = kIntOne | 1 << 9;            // Не слышит базар //
constexpr bitvector_t PRF_NOCLONES = kIntOne | 1 << 10;            // Не видит в группе чужих клонов //
constexpr bitvector_t PRF_NOINVISTELL = kIntOne | 1 << 11;            // Не хочет, чтобы телял "кто-то" //
constexpr bitvector_t PRF_POWERATTACK = kIntOne | 1 << 12;            // мощная атака //
constexpr bitvector_t PRF_GREATPOWERATTACK = kIntOne | 1 << 13;    // улучшеная мощная атака //
constexpr bitvector_t PRF_AIMINGATTACK = kIntOne | 1 << 14;        // прицельная атака //
constexpr bitvector_t PRF_GREATAIMINGATTACK = kIntOne | 1 << 15;    // улучшеная прицельная атака //
constexpr bitvector_t PRF_NEWS_MODE = kIntOne | 1 << 16;            // вариант чтения новостей мада и дружины
constexpr bitvector_t PRF_BOARD_MODE = kIntOne | 1 << 17;            // уведомления о новых мессагах на досках
constexpr bitvector_t PRF_DECAY_MODE = kIntOne | 1 << 18;            // канал хранилища, рассыпание шмота
constexpr bitvector_t PRF_TAKE_MODE = kIntOne | 1 << 19;            // канал хранилища, положили/взяли
constexpr bitvector_t PRF_PKL_MODE = kIntOne | 1 << 20;            // уведомления о добавлении/убирании в пкл
constexpr bitvector_t PRF_POLIT_MODE = kIntOne | 1 << 21;            // уведомления об изменении политики, своей и чужой
constexpr bitvector_t PRF_IRON_WIND = kIntOne | 1 << 22;            // включен скилл "железный ветер"
constexpr bitvector_t PRF_PKFORMAT_MODE = kIntOne | 1 << 23;        // формат пкл/дрл
constexpr bitvector_t PRF_WORKMATE_MODE = kIntOne | 1 << 24;        // показ входов/выходов соклановцев
constexpr bitvector_t PRF_OFFTOP_MODE = kIntOne | 1 << 25;        // вкл/выкл канала оффтопа
constexpr bitvector_t PRF_ANTIDC_MODE = kIntOne | 1 << 26;        // режим защиты от дисконекта в бою
constexpr bitvector_t
	PRF_NOINGR_MODE = kIntOne | 1 << 27;        // не показывать продажу/покупку ингров в канале базара
constexpr bitvector_t PRF_NOINGR_LOOT = kIntOne | 1 << 28;        // не лутить ингры в режиме автограбежа
constexpr bitvector_t
	PRF_DISP_TIMED = kIntOne | 1 << 29;            // показ задержек для характерных профам умений и способностей

constexpr bitvector_t PRF_IGVA_PRONA = kIntTwo | 1 << 0;            // для стоп-списка оффтоп
constexpr bitvector_t PRF_EXECUTOR = kIntTwo | 1 << 1;            // палач
constexpr bitvector_t PRF_DRAW_MAP = kIntTwo | 1 << 2;            // отрисовка карты при осмотре клетки
constexpr bitvector_t PRF_CAN_REMORT = kIntTwo | 1 << 3;            // разрешение на реморт через жертвование гривн
constexpr bitvector_t PRF_ENTER_ZONE = kIntTwo | 1 << 4;            // вывод названия/среднего уровня при входе в зону
constexpr bitvector_t
	PRF_MISPRINT = kIntTwo | 1 << 5;            // показ непрочитанных сообщений на доске опечаток при входе
constexpr bitvector_t PRF_BRIEF_SHIELDS = kIntTwo | 1 << 6;        // краткий режим сообщений при срабатывании маг.щитов
constexpr bitvector_t PRF_AUTO_NOSUMMON = kIntTwo
	| 1 << 7;        // автоматическое включение режима защиты от призыва ('реж призыв') после удачного суммона/пенты
constexpr bitvector_t PRF_SDEMIGOD = kIntTwo | 1 << 8;            // Для канала демигодов
constexpr bitvector_t PRF_BLIND = kIntTwo | 1 << 9;                // примочки для слепых
constexpr bitvector_t PRF_MAPPER = kIntTwo | 1 << 10;                // Показывает хеши рядом с названием комнаты
constexpr bitvector_t PRF_TESTER = kIntTwo | 1 << 11;                // отображать допинфу при годсфлаге тестер
constexpr bitvector_t
	PRF_IPCONTROL = kIntTwo | 1 << 12;            // отправлять код на мыло при заходе из новой подсети
constexpr bitvector_t PRF_SKIRMISHER = kIntTwo | 1 << 13;            // персонаж "в строю" в группе
constexpr bitvector_t PRF_DOUBLE_THROW = kIntTwo | 1 << 14;        // готов использовать двойной бросок
constexpr bitvector_t PRF_TRIPLE_THROW = kIntTwo | 1 << 15;        // готов использовать тройной бросок
constexpr bitvector_t PRF_SHADOW_THROW = kIntTwo | 1 << 16;        // применяет "теневой бросок"
constexpr bitvector_t PRF_DISP_COOLDOWNS = kIntTwo | 1 << 17;        // Показывать кулдауны скиллов в промпте
constexpr bitvector_t PRF_TELEGRAM = kIntTwo | 1 << 18;            // Активирует телеграм-канал у персонажа

// при добавлении не забываем про preference_bits[]

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

// modes of ignoring
constexpr bitvector_t IGNORE_TELL = 1 << 0;
constexpr bitvector_t IGNORE_SAY = 1 << 1;
constexpr bitvector_t IGNORE_CLAN = 1 << 2;
constexpr bitvector_t IGNORE_ALLIANCE = 1 << 3;
constexpr bitvector_t IGNORE_GOSSIP = 1 << 4;
constexpr bitvector_t IGNORE_SHOUT = 1 << 5;
constexpr bitvector_t IGNORE_HOLLER = 1 << 6;
constexpr bitvector_t IGNORE_GROUP = 1 << 7;
constexpr bitvector_t IGNORE_WHISPER = 1 << 8;
constexpr bitvector_t IGNORE_ASK = 1 << 9;
constexpr bitvector_t IGNORE_EMOTE = 1 << 10;
constexpr bitvector_t IGNORE_OFFTOP = 1 << 11;

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

// Character equipment positions: used as index for char_data.equipment[] //
// NOTE: Don't confuse these constants with the ITEM_ bitvectors
//       which control the valid places you can wear a piece of equipment
const __uint8_t WEAR_LIGHT = 0;
const __uint8_t WEAR_FINGER_R = 1;
const __uint8_t WEAR_FINGER_L = 2;
const __uint8_t WEAR_NECK_1 = 3;
const __uint8_t WEAR_NECK_2 = 4;
const __uint8_t WEAR_BODY = 5;
const __uint8_t WEAR_HEAD = 6;
const __uint8_t WEAR_LEGS = 7;
const __uint8_t WEAR_FEET = 8;
const __uint8_t WEAR_HANDS = 9;
const __uint8_t WEAR_ARMS = 10;
const __uint8_t WEAR_SHIELD = 11;
const __uint8_t WEAR_ABOUT = 12;
const __uint8_t WEAR_WAIST = 13;
const __uint8_t WEAR_WRIST_R = 14;
const __uint8_t WEAR_WRIST_L = 15;
const __uint8_t WEAR_WIELD = 16;      // правая рука
const __uint8_t WEAR_HOLD = 17;      // левая рука
const __uint8_t WEAR_BOTHS = 18;      // обе руки
const __uint8_t WEAR_QUIVER = 19;      // под лук (колчан)
const __uint8_t NUM_WEARS = 20;    // This must be the # of eq positions!! //


// object-related defines ******************************************* //

// Типы магических книг //
const __uint8_t BOOK_SPELL = 0;    // Книга заклинания //
const __uint8_t BOOK_SKILL = 1;    // Книга умения //
const __uint8_t BOOK_UPGRD = 2;    // Увеличение умения //
const __uint8_t BOOK_RECPT = 3;    // Книга рецепта //
const __uint8_t BOOK_FEAT = 4;        // Книга способности (feats) //

template<typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
	return static_cast<typename std::underlying_type<E>::type>(e);
}

// Take/Wear flags: used by obj_data.obj_flags.wear_flags //
enum class EWearFlag : bitvector_t {
	ITEM_WEAR_UNDEFINED = 0,    // Special value
	ITEM_WEAR_TAKE = 1 << 0,    // Item can be takes      //
	ITEM_WEAR_FINGER = 1 << 1,    // Can be worn on finger  //
	ITEM_WEAR_NECK = 1 << 2,    // Can be worn around neck   //
	ITEM_WEAR_BODY = 1 << 3,    // Can be worn on body    //
	ITEM_WEAR_HEAD = 1 << 4,    // Can be worn on head    //
	ITEM_WEAR_LEGS = 1 << 5,    // Can be worn on legs //
	ITEM_WEAR_FEET = 1 << 6,    // Can be worn on feet //
	ITEM_WEAR_HANDS = 1 << 7,    // Can be worn on hands   //
	ITEM_WEAR_ARMS = 1 << 8,    // Can be worn on arms //
	ITEM_WEAR_SHIELD = 1 << 9,    // Can be used as a shield   //
	ITEM_WEAR_ABOUT = 1 << 10,    // Can be worn about body    //
	ITEM_WEAR_WAIST = 1 << 11,    // Can be worn around waist  //
	ITEM_WEAR_WRIST = 1 << 12,    // Can be worn on wrist   //
	ITEM_WEAR_WIELD = 1 << 13,    // Can be wielded      //
	ITEM_WEAR_HOLD = 1 << 14,    // Can be held      //
	ITEM_WEAR_BOTHS = 1 << 15,
	ITEM_WEAR_QUIVER = 1 << 16      // колчан
};

template<>
const std::string &NAME_BY_ITEM<EWearFlag>(EWearFlag item);
template<>
EWearFlag ITEM_BY_NAME<EWearFlag>(const std::string &name);

// Extra object flags: used by obj_data.obj_flags.extra_flags //
enum class EExtraFlag : bitvector_t {
	ITEM_GLOW = 1 << 0,                        ///< Item is glowing
	ITEM_HUM = 1 << 1,                        ///< Item is humming
	ITEM_NORENT = 1 << 2,                    ///< Item cannot be rented
	ITEM_NODONATE = 1 << 3,                    ///< Item cannot be donated
	ITEM_NOINVIS = 1 << 4,                    ///< Item cannot be made invis
	ITEM_INVISIBLE = 1 << 5,                ///< Item is invisible
	ITEM_MAGIC = 1 << 6,                    ///< Item is magical
	ITEM_NODROP = 1 << 7,                    ///< Item is cursed: can't drop
	ITEM_BLESS = 1 << 8,                    ///< Item is blessed
	ITEM_NOSELL = 1 << 9,                    ///< Not usable by good people
	ITEM_DECAY = 1 << 10,                    ///< Not usable by evil people
	ITEM_ZONEDECAY = 1 << 11,                ///< Not usable by neutral people
	ITEM_NODISARM = 1 << 12,                ///< Not usable by mages
	ITEM_NODECAY = 1 << 13,
	ITEM_POISONED = 1 << 14,
	ITEM_SHARPEN = 1 << 15,
	ITEM_ARMORED = 1 << 16,
	ITEM_DAY = 1 << 17,
	ITEM_NIGHT = 1 << 18,
	ITEM_FULLMOON = 1 << 19,
	ITEM_WINTER = 1 << 20,
	ITEM_SPRING = 1 << 21,
	ITEM_SUMMER = 1 << 22,
	ITEM_AUTUMN = 1 << 23,
	ITEM_SWIMMING = 1 << 24,
	ITEM_FLYING = 1 << 25,
	ITEM_THROWING = 1 << 26,
	ITEM_TICKTIMER = 1 << 27,
	ITEM_FIRE = 1 << 28,                    ///< ...горит
	ITEM_REPOP_DECAY = 1 << 29,                ///< рассыпется при репопе зоны
	ITEM_NOLOCATE = kIntOne | (1 << 0),        ///< нельзя отлокейтить
	ITEM_TIMEDLVL = kIntOne | (1 << 1),        ///< для маг.предметов уровень уменьшается со временем
	ITEM_NOALTER = kIntOne | (1 << 2),        ///< свойства предмета не могут быть изменены магией
	ITEM_WITH1SLOT = kIntOne | (1 << 3),    ///< в предмет можно вплавить 1 камень
	ITEM_WITH2SLOTS = kIntOne | (1 << 4),    ///< в предмет можно вплавить 2 камня
	ITEM_WITH3SLOTS = kIntOne | (1 << 5),    ///< в предмет можно вплавить 3 камня (овер)
	ITEM_SETSTUFF = kIntOne | (1 << 6),        ///< Item is set object
	ITEM_NO_FAIL = kIntOne | (1 << 7),        ///< не фейлится при изучении (в случае книги)
	ITEM_NAMED = kIntOne | (1 << 8),        ///< именной предмет
	ITEM_BLOODY = kIntOne | (1 << 9),        ///< окровавленная вещь (снятая с трупа)
	ITEM_1INLAID = kIntOne | (1 << 10),        ///< TODO: не используется, см convert_obj_values()
	ITEM_2INLAID = kIntOne | (1 << 11),
	ITEM_3INLAID = kIntOne | (1 << 12),
	ITEM_NOPOUR = kIntOne | (1 << 13),        ///< нельзя перелить
	ITEM_UNIQUE = kIntOne | (1
		<< 14),        // объект уникальный, т.е. если у чара есть несколько шмоток с одним внумом, которые одеваются
	// на разные слоты, то чар может одеть на себя только одну шмотку
	ITEM_TRANSFORMED = kIntOne | (1 << 15),        // Наложено заклинание заколдовать оружие
	ITEM_FREE_FOR_USE = kIntOne | (1 << 16),    // пока свободно, можно использовать
	ITEM_NOT_UNLIMIT_TIMER = kIntOne | (1 << 17), // Не может быть нерушимой
	ITEM_UNIQUE_WHEN_PURCHASE = kIntOne | (1 << 18), // станет именной при покупке в магазе
	ITEM_NOT_ONE_CLANCHEST = kIntOne | (1 << 19) //1 штука из набора не лезет в хран


};

template<>
const std::string &NAME_BY_ITEM<EExtraFlag>(EExtraFlag item);
template<>
EExtraFlag ITEM_BY_NAME<EExtraFlag>(const std::string &name);

enum class ENoFlag : bitvector_t {
	ITEM_NO_MONO = 1 << 0,
	ITEM_NO_POLY = 1 << 1,
	ITEM_NO_NEUTRAL = 1 << 2,
	ITEM_NO_MAGIC_USER = 1 << 3,
	ITEM_NO_CLERIC = 1 << 4,
	ITEM_NO_THIEF = 1 << 5,
	ITEM_NO_WARRIOR = 1 << 6,
	ITEM_NO_ASSASINE = 1 << 7,
	ITEM_NO_GUARD = 1 << 8,
	ITEM_NO_PALADINE = 1 << 9,
	ITEM_NO_RANGER = 1 << 10,
	ITEM_NO_SMITH = 1 << 11,
	ITEM_NO_MERCHANT = 1 << 12,
	ITEM_NO_DRUID = 1 << 13,
	ITEM_NO_BATTLEMAGE = 1 << 14,
	ITEM_NO_CHARMMAGE = 1 << 15,
	ITEM_NO_DEFENDERMAGE = 1 << 16,
	ITEM_NO_NECROMANCER = 1 << 17,
	ITEM_NO_FIGHTER_USER = 1 << 18,
	ITEM_NO_KILLER = kIntOne | 1 << 0,
	ITEM_NO_COLORED = kIntOne | 1 << 1,    // нельзя цветным //
	ITEM_NO_BD = kIntOne | 1 << 2,
	ITEM_NO_MALE = kIntTwo | 1 << 6,
	ITEM_NO_FEMALE = kIntTwo | 1 << 7,
	ITEM_NO_CHARMICE = kIntTwo | 1 << 8,
	ITEM_NO_POLOVCI = kIntTwo | 1 << 9,
	ITEM_NO_PECHENEGI = kIntTwo | 1 << 10,
	ITEM_NO_MONGOLI = kIntTwo | 1 << 11,
	ITEM_NO_YIGURI = kIntTwo | 1 << 12,
	ITEM_NO_KANGARI = kIntTwo | 1 << 13,
	ITEM_NO_XAZARI = kIntTwo | 1 << 14,
	ITEM_NO_SVEI = kIntTwo | 1 << 15,
	ITEM_NO_DATCHANE = kIntTwo | 1 << 16,
	ITEM_NO_GETTI = kIntTwo | 1 << 17,
	ITEM_NO_UTTI = kIntTwo | 1 << 18,
	ITEM_NO_XALEIGI = kIntTwo | 1 << 19,
	ITEM_NO_NORVEZCI = kIntTwo | 1 << 20,
	ITEM_NO_RUSICHI = kIntThree | 1 << 0,
	ITEM_NO_STEPNYAKI = kIntThree | 1 << 1,
	ITEM_NO_VIKINGI = kIntThree | 1 << 2,
	ITEM_NOT_FOR_NOPK = kIntThree | (1 << 3)      // не может быть взята !пк кланом
};

template<>
const std::string &NAME_BY_ITEM<ENoFlag>(ENoFlag item);
template<>
ENoFlag ITEM_BY_NAME<ENoFlag>(const std::string &name);

enum class EAntiFlag : bitvector_t {
	ITEM_AN_MONO = 1 << 0,
	ITEM_AN_POLY = 1 << 1,
	ITEM_AN_NEUTRAL = 1 << 2,
	ITEM_AN_MAGIC_USER = 1 << 3,
	ITEM_AN_CLERIC = 1 << 4,
	ITEM_AN_THIEF = 1 << 5,
	ITEM_AN_WARRIOR = 1 << 6,
	ITEM_AN_ASSASINE = 1 << 7,
	ITEM_AN_GUARD = 1 << 8,
	ITEM_AN_PALADINE = 1 << 9,
	ITEM_AN_RANGER = 1 << 10,
	ITEM_AN_SMITH = 1 << 11,
	ITEM_AN_MERCHANT = 1 << 12,
	ITEM_AN_DRUID = 1 << 13,
	ITEM_AN_BATTLEMAGE = 1 << 14,
	ITEM_AN_CHARMMAGE = 1 << 15,
	ITEM_AN_DEFENDERMAGE = 1 << 16,
	ITEM_AN_NECROMANCER = 1 << 17,
	ITEM_AN_FIGHTER_USER = 1 << 18,
	ITEM_AN_KILLER = kIntOne | (1 << 0),
	ITEM_AN_COLORED = kIntOne | (1 << 1),    // нельзя цветным //
	ITEM_AN_BD = kIntOne | (1 << 2),
	ITEM_AN_SEVERANE = kIntTwo | 1 << 0,  // недоступность по родам
	ITEM_AN_POLANE = kIntTwo | 1 << 1,
	ITEM_AN_KRIVICHI = kIntTwo | 1 << 2,
	ITEM_AN_VATICHI = kIntTwo | 1 << 3,
	ITEM_AN_VELANE = kIntTwo | 1 << 4,
	ITEM_AN_DREVLANE = kIntTwo | 1 << 5,
	ITEM_AN_MALE = kIntTwo | 1 << 6,
	ITEM_AN_FEMALE = kIntTwo | 1 << 7,
	ITEM_AN_CHARMICE = kIntTwo | 1 << 8,
	ITEM_AN_POLOVCI = kIntTwo | 1 << 9,
	ITEM_AN_PECHENEGI = kIntTwo | 1 << 10,
	ITEM_AN_MONGOLI = kIntTwo | 1 << 11,
	ITEM_AN_YIGURI = kIntTwo | 1 << 12,
	ITEM_AN_KANGARI = kIntTwo | 1 << 13,
	ITEM_AN_XAZARI = kIntTwo | 1 << 14,
	ITEM_AN_SVEI = kIntTwo | 1 << 15,
	ITEM_AN_DATCHANE = kIntTwo | 1 << 16,
	ITEM_AN_GETTI = kIntTwo | 1 << 17,
	ITEM_AN_UTTI = kIntTwo | 1 << 18,
	ITEM_AN_XALEIGI = kIntTwo | 1 << 19,
	ITEM_AN_NORVEZCI = kIntTwo | 1 << 20,
	ITEM_AN_RUSICHI = kIntThree | 1 << 0,
	ITEM_AN_STEPNYAKI = kIntThree | 1 << 1,
	ITEM_AN_VIKINGI = kIntThree | 1 << 2,
	ITEM_NOT_FOR_NOPK = kIntThree | (1 << 3)      // не может быть взята !пк кланом
};

template<>
const std::string &NAME_BY_ITEM<EAntiFlag>(EAntiFlag item);
template<>
EAntiFlag ITEM_BY_NAME<EAntiFlag>(const std::string &name);

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

// Container flags - value[1] //
constexpr bitvector_t CONT_CLOSEABLE = 1 << 0;    // Container can be closed //
constexpr bitvector_t CONT_PICKPROOF = 1 << 1;    // Container is pickproof  //
constexpr bitvector_t CONT_CLOSED = 1 << 2;        // Container is closed     //
constexpr bitvector_t CONT_LOCKED = 1 << 3;        // Container is locked     //
constexpr bitvector_t CONT_BROKEN = 1 << 4;        // Container is locked     //

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
	int
		trk_info;
	int
		who;
	int
		dirs;
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
