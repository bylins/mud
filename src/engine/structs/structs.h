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
using ullong = unsigned long long;

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
using byte = char;
#endif

const int kMinRemort = 0;
const int kMaxRemort = 99;
const int kMaxPlayerLevel = 30;
const int kMaxAliasLehgt = 100;
const std::nullptr_t NoArgument = nullptr;

const uint8_t kMaxDest = 50;

class CharData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.
class ObjData;    // forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.
class Trigger;

const int8_t kNowhere = 0;        // nil reference for room-database
const ZoneRnum kNoZone = -1;      // nil reference for zone-database
const int8_t kNothing = -1;        // nil reference for objects
const int8_t kNobody = -1;        // nil reference for mobiles

constexpr int kFormatIndent = 1 << 0;

const uint8_t kCodePageAlt = 1;
const uint8_t kCodePageWin = 2;
const uint8_t kCodePageWinz = 3;
const uint8_t kCodePageWinzZ = 4;
const uint8_t kCodePageUTF8 = 5;
const uint8_t kCodePageWinzOld = 6;
const uint8_t kCodePageLast = 7;

const int kKtSelectmenu = 255;

const int kLvlImplementator = 34;
const int kLvlGreatGod = 33;
const int kLvlBuilder = 33;
const int kLvlGod = 32;
const int kLvlImmortal = 31;
const int kLvlFreeze = kLvlGreatGod; // Level of the 'freeze' command //
const uint8_t kMagicNumber = 0x06;    // Arbitrary number that won't be in a string //

constexpr long long kOptUsec = 40000;    // 25 passes per second //
constexpr long long kPassesPerSec = 1000000 / kOptUsec;
constexpr long long kRealSec = kPassesPerSec;
constexpr long long kPulseZone = (1*kRealSec);
constexpr long long kPulseMobile = (10*kRealSec);
constexpr long long kBattleRound = (2*kRealSec);
const int kZonesReset = 1;    // number of zones to reset at one time //

// Variables for the output buffering system //
constexpr uint16_t kMaxSockBuf = 48 * 1024;	// Size of kernel's sock buf   //
const uint16_t kMaxPromptLength = 256;		// Max length of prompt        //
const uint8_t kGarbageSpace = 32;				// Space for **OVERFLOW** etc  //
const uint16_t kSmallBufsize = 1024;			// Static output buffer size   //
// Max amount of output that can be buffered //
constexpr uint16_t kLargeBufSize = kMaxSockBuf - kGarbageSpace - kMaxPromptLength;

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

struct FollowerType {
	CharData *follower = nullptr;
	struct FollowerType *next = nullptr;
};

#endif // STRUCTS_STRUCTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
