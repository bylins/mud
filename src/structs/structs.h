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

#ifndef STRUCTS_STRUCTS_H_
#define STRUCTS_STRUCTS_H_

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

//#include "utils/wrapper.h"

using sbyte = int8_t;
using ubyte = uint8_t;
using sh_int = int16_t ;
using ush_int = uint16_t;

// This structure describe new bitvector structure
using Bitvector = uint32_t;
constexpr Bitvector kIntOne = 1u << 30;
constexpr Bitvector kIntTwo = 2u << 30;
constexpr Bitvector kIntThree = 3u << 30;

using Vnum = int;
using RoomVnum = Vnum;	// A room's vnum type //
using ObjVnum = Vnum;	// An object's vnum type //
using MobVnum = Vnum;	// A mob's vnum type //
using ZoneVnum = Vnum;	// A virtual zone number.  //
using TrgVnum = Vnum;	// A virtual trigger number.  //

using Rnum = int;
using RoomRnum = Rnum;	// A room's real (internal) number type //
using ObjRnum = Rnum;	// An object's real (internal) num type //
using MobRnum = Rnum;	// A mobile's real (internal) num type //
using ZoneRnum = Rnum;	// A zone's real (array index) number. //
using TrgRnum = Rnum;	// A trigger's real (array index) number. //

#if !defined(__cplusplus)    // Anyone know a portable method?
typedef char bool;
#endif

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)    // Hm, sysdep.h?
typedef char byte;
#endif

const int kMaxRemort = 75;
const int kMaxPlayerLevel = 30;

// Структуры валют надо вынести в отдельный модуль с механикой валют ***************************************

namespace ExtMoney {
const unsigned kTorcGold = 0;        // золотые гривны
const unsigned kTorcSilver = 1;        // серебряные гривны
const unsigned kTorcBronze = 2;        // бронзовые гривны
const unsigned kTotalTypes = 3;        // терминатор всегда в конце
} // namespace ExtMoney


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

class CharData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.
class ObjData;    // forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.
class Trigger;

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
const int kNumKins = 3;

// Descriptor flags //
//constexpr bitvector_t DESC_CANZLIB = 1 << 0;    // Client says compression capable.   //

// object-related defines ******************************************* //

template<typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
	return static_cast<typename std::underlying_type<E>::type>(e);
}

constexpr Bitvector TRACK_NPC = 1 << 0;
constexpr Bitvector TRACK_HIDE = 1 << 1;

// other miscellaneous defines ****************************************** //

enum { DRUNK, FULL, THIRST };
// pernalty types
enum { P_DAMROLL, P_HITROLL, P_CAST, P_MEM_GAIN, P_MOVE_GAIN, P_HIT_GAIN, P_AC };

constexpr Bitvector EXTRA_FAILHIDE = 1 << 0;
constexpr Bitvector EXTRA_FAILSNEAK = 1 << 1;
constexpr Bitvector EXTRA_FAILCAMOUFLAGE = 1 << 2;
constexpr Bitvector EXTRA_GRP_KILL_COUNT = 1 << 3; // для избежания повторных записей моба в списки SetsDrop

// other #defined constants ********************************************* //

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 *
 * kLevelImplementator should always be the HIGHEST possible immortal level, and
 * kLevelImmortal should always be the LOWEST immortal level.  The number of
 * mortal levels will always be kLevelImmortal - 1.
 */
const int kLvlImplementator = 34;
const int kLvlGreatGod = 33;
const int kLvlBuilder = 33;
const int kLvlGod = 32;
const int kLvlImmortal = 31;
const int kLvlFreeze = kLvlGreatGod; // Level of the 'freeze' command //

const __uint8_t kMagicNumber = 0x06;    // Arbitrary number that won't be in a string //

constexpr long long kOptUsec = 40000;    // 25 passes per second //
constexpr long long kPassesPerSec = 1000000 / kOptUsec;
constexpr long long kRealSec = kPassesPerSec;
constexpr long long kPulseZone = (1*kRealSec);
constexpr long long kPulseMobile = (10*kRealSec);
constexpr long long kPulseViolence = (2*kRealSec);

const int kZonesReset = 1;    // number of zones to reset at one time //
//#define PULSE_LOGROTATE (10 kRealSec)

// Variables for the output buffering system //
constexpr __uint16_t kMaxSockBuf = 48 * 1024;	// Size of kernel's sock buf   //
const __uint16_t kMaxPromptLength = 256;		// Max length of prompt        //
const __uint8_t kGarbageSpace = 32;				// Space for **OVERFLOW** etc  //
const __uint16_t kSmallBufsize = 1024;			// Static output buffer size   //
// Max amount of output that can be buffered //
constexpr __uint16_t kLargeBufSize = kMaxSockBuf - kGarbageSpace - kMaxPromptLength;

// Keep last 5 commands
const int kHistorySize = 5;
const int kMaxStringLength = 32768;
const int kMaxExtendLength = 0xFFFF;
const int kMaxTrglineLength = 1024;
const int kMaxInputLength = 1024;   // Max length per *line* of input //
const int kMaxRawInputLength = 1024;   // Max size of *raw* input //
const int kMaxNameLength = 20;
const int kMinNameLength = 5;
const int kHostLength = 30;
const int kExdscrLength = 512;
const int kMaxAffect = 128;
const int kMaxObjAffect = 8;
const int kMaxHits = 32000; // Максимальное количество хитов и дамага //
const long kMaxMoneyKept = 1000000000; // планка на кол-во денег у чара на руках и в банке (раздельно) //

const int kMinCharLevel = 0;
const int kMaxMobLevel = 100;
const int kMaxSaving = 400; //максимальное значение воля, здоровье, стойкость, реакция
constexpr int kMinSaving = -kMaxSaving;
const int kMaxResistance = 100;
constexpr int kMinResistance = -kMaxResistance;
const int kStrongMobLevel = 30;

bool sprintbitwd(Bitvector bitvector, const char *names[], char *result, const char *div, const int print_flag = 0);

inline bool sprintbit(Bitvector bitvector, const char *names[], char *result, const int print_flag = 0) {
	return sprintbitwd(bitvector, names, result, ",", print_flag);
}

// =======================================================================

// char-related structures ***********************************************
// memory structure for characters //
struct MemoryRecord {
	long id = 0;
	long time = 0;
	struct MemoryRecord *next = nullptr;
};

// This structure is purely intended to be an easy way to transfer //
// and return information about time (real or mudwise).            //
struct TimeInfoData {
	int hours = 0;
	int day = 0;
	int month = 0;
	sh_int year = 0;
};

struct Logon {
	char *ip;
	long count;
	time_t lasttime;
	bool is_first;
};

struct Punish {
	long duration = 0;
	char *reason = nullptr;
	int level = 0;
	long godid = 0;
};

// Structure used for entities following other entities //
struct Follower {
	CharData *ch = nullptr;
	struct Follower *next = nullptr;
};

// Structure used for tracking a mob //
/*struct track_info {
	int trk_info = 0;
	int who = 0;
	int dirs = 0;
};*/

// Structure used for helpers //
// Это следует перенести в spec_proc после ее распиливания (сейчас вряд ли есть смысл огромный хедер везде тащить)
struct Helper {
	MobVnum mob_vnum = 0;
	struct Helper *next = nullptr;
};


// ===============================================================
// Structure used for on_dead object loading //
// Эту механику следует вырезать.
struct LoadingItem {
	ObjVnum obj_vnum = 0;
	int load_prob = 0;
	int load_type = 0;
	int spec_param = 0;
};

using OnDeadLoadList = std::list<struct LoadingItem *>;
// ===============================================================

// Перенести в работу с очередью мема
struct SpellMemQueueItem {
	int spellnum = 0;
	struct SpellMemQueueItem *link = nullptr;
};

// descriptor-related structures ****************************************

struct TextBlock {
	char *text = nullptr;
	int aliased = 0;
	struct TextBlock *next = nullptr;
};

struct TextBlocksQueue {
	struct TextBlock *head = nullptr;
	struct TextBlock *tail = nullptr;
};
// ===============================================================

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
// ===============================================================

// ===============================================================
// other miscellaneous structures **************************************

struct IndexData {
	IndexData() : vnum(0), total_online(0), stored(0), func(nullptr), farg(nullptr), proto(nullptr), zone(0), set_idx(-1) {}
	IndexData(int _vnum)
		: vnum(_vnum), total_online(0), stored(0), func(nullptr), farg(nullptr), proto(nullptr), zone(0), set_idx(-1) {}

	Vnum vnum;            // virtual number of this mob/obj       //
	int total_online;        // number of existing units of this mob/obj //
	int stored;        // number of things in rent file            //
	int (*func)(CharData *, void *, int, char *);
	char *farg;        // string argument for special function     //
	Trigger *proto;    // for triggers... the trigger     //
	int zone;            // mob/obj zone rnum //
	size_t set_idx; // индекс сета в obj_sets::set_list, если != -1
};
// ===============================================================

const __uint8_t GAPPLY_NONE = 0;
const __uint8_t GAPPLY_SKILL_SUCCESS = 1;
const __uint8_t GAPPLY_SPELL_SUCCESS = 2;
const __uint8_t GAPPLY_SPELL_EFFECT = 3;
const __uint8_t GAPPLY_MODIFIER = 4;
const __uint8_t GAPPLY_AFFECT = 5;

// ===============================================================
// Структрура для описания проталов для спела townportal //
// Механику порталов надо обобщить и вынести в отдельный файл в game_mechanics
// После чего использовать ее для врат, !перехода! и триггерной постановки портала.
struct Portal {
	char *wrd = nullptr;			// кодовое слово
	RoomVnum vnum = 0;				// vnum комнаты для портала
	int level = 0;					// минимальный уровень для запоминания
	struct Portal *next = nullptr;
};

struct CharacterPortal {
	int vnum = 0;            // vnum комнаты для портала //
	struct CharacterPortal *next = nullptr;
};
// ===============================================================
// Структуры для act.wizard.cpp //
// После распиливания акт.визард надо вынести в соответтующие файлы
struct show_struct {
	const char *cmd = nullptr;
	const char level = 0;
};

struct set_struct {
	const char *cmd = nullptr;
	const char level = 0;
	const char pcnpc = 0;
	const char type = 0;
};
// ===============================================================

namespace parser_wrapper {
// forward declaration
class DataNode;
}

enum ECase {
	kNom = 0,
	kGen,
	kDat,
	kAcc,
	kIns,
	kPre,
	kNumGrammaticalCases
};

namespace base_structs {

class ItemName {
 public:
	ItemName();

	using Ptr = std::unique_ptr<ItemName>;
	using NameCases = std::array<std::string, ECase::kNumGrammaticalCases>;
	ItemName(ItemName &&i) noexcept;
	ItemName &operator=(ItemName &&i) noexcept;

	[[nodiscard]] const std::string &GetSingular(ECase name_case = ECase::kNom) const;
	[[nodiscard]] const std::string &GetPlural(ECase name_case = ECase::kNom) const;
	static Ptr Build(parser_wrapper::DataNode &node);

 private:
	NameCases singular_names_;
	NameCases plural_names_;
};

}

#endif // STRUCTS_STRUCTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
