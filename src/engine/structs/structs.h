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

#include "engine/core/sysdep.h"

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

using sbyte = int8_t;
using ubyte = uint8_t;
using sh_int = int16_t ;
using ush_int = uint16_t;

using Bitvector = uint32_t;
constexpr Bitvector kIntOne = 1u << 30;
constexpr Bitvector kIntTwo = 2u << 30;
constexpr Bitvector kIntThree = 3u << 30;

using Vnum = int;
using RoomVnum = Vnum;
using ObjVnum = Vnum;
using MobVnum = Vnum;
using ZoneVnum = Vnum;
using TrgVnum = Vnum;

using Rnum = int;
using RoomRnum = Rnum;
using ObjRnum = Rnum;
using MobRnum = Rnum;
using ZoneRnum = Rnum;
using TrgRnum = Rnum;

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)    // Hm, sysdep.h?
typedef char byte;
#endif

const int kMinRemort = 0;
const int kMaxRemort = 75;
const int kMaxPlayerLevel = 30;
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

const __int8_t kNowhere = 0;        // nil reference for room-database
const __int8_t kNothing = -1;        // nil reference for objects
const __int8_t kNobody = -1;        // nil reference for mobiles

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

// object-related defines ******************************************* //

template<typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
	return static_cast<typename std::underlying_type<E>::type>(e);
}

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
constexpr long long kBattleRound = (2*kRealSec);
const int kZonesReset = 1;    // number of zones to reset at one time //

// Variables for the output buffering system //
constexpr __uint16_t kMaxSockBuf = 48 * 1024;	// Size of kernel's sock buf   //
const __uint16_t kMaxPromptLength = 256;		// Max length of prompt        //
const __uint8_t kGarbageSpace = 32;				// Space for **OVERFLOW** etc  //
const __uint16_t kSmallBufsize = 1024;			// Static output buffer size   //
// Max amount of output that can be buffered //
constexpr __uint16_t kLargeBufSize = kMaxSockBuf - kGarbageSpace - kMaxPromptLength;

const int kHistorySize = 5; // Keep last commands
const int kMaxStringLength = 32768;
const int kMaxExtendLength = 0xFFFF;
const std::size_t kMaxInputLength = 2048;   // Max length per *line* of input //
const std::size_t kMaxTrglineLength = kMaxInputLength;
const int kMaxRawInputLength = kMaxInputLength;   // Max size of *raw* input //
const int kMaxNameLength = 20;
const int kMinNameLength = 5;
const int kHostLength = 30;
const int kExdscrLength = 512;
const int kMaxObjAffect = 8;
const int kMaxHits = 32000;
const long kMaxMoneyKept = 1000000000L; // планка на кол-во денег у чара на руках и в банке (раздельно) //
const int_least32_t MAX_TIME = 0x7fffffff;
const int kMinCharLevel = 0;
const int kMaxMobLevel = 100;
const int kMaxSaving = 400;
constexpr int kMinSaving = -kMaxSaving;
const int kMaxNpcResist = 100;
constexpr int kMinResistance = -kMaxNpcResist;
const int kStrongMobLevel = 30;

struct TimeInfoData {
	int hours = 0;
	int day = 0;
	int month = 0;
	sh_int year = 0;
};

struct Punish {
	long duration = 0;
	char *reason = nullptr;
	int level = 0;
	long godid = 0;
};

struct FollowerType {
	CharData *follower = nullptr;
	struct FollowerType *next = nullptr;
};

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

#ifndef HAVE_ZLIB
struct z_stream;
#endif

struct IndexData {
	IndexData() : vnum(0), total_online(0), stored(0), func(nullptr), farg(nullptr), proto(nullptr), zone(0), set_idx(-1) {}
	explicit IndexData(int _vnum)
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

enum ECase {
	kNom = 0,
	kGen,
	kDat,
	kAcc,
	kIns,
	kPre,
	kFirstCase = kNom,
	kLastCase = kPre,
};

namespace parser_wrapper {
// forward declaration
class DataNode;
}

namespace base_structs {

class ItemName {
 public:
	ItemName();

	using Ptr = std::unique_ptr<ItemName>;
	using NameCases = std::array<std::string, ECase::kLastCase + 1>;
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
//спецпроцедуры
int exchange(CharData *ch, void *me, int cmd, char *argument);
int horse_keeper(CharData *ch, void *me, int cmd, char *argument);
int torc(CharData *ch, void *me, int cmd, char *argument);
int mercenary(CharData *ch, void * /*me*/, int cmd, char *argument);
int shop_ext(CharData *ch, void *me, int cmd, char *argument);

#endif // STRUCTS_STRUCTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :