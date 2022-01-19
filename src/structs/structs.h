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

using sbyte = int8_t;
using ubyte = uint8_t;
using sh_int = int16_t ;
using ush_int = uint16_t;

typedef struct index_data INDEX_DATA;
typedef struct TimeInfoData TIME_INFO_DATA;

// This structure describe new bitvector structure
using bitvector_t = uint32_t;
constexpr bitvector_t kIntOne = 1u << 30;
constexpr bitvector_t kIntTwo = 2u << 30;
constexpr bitvector_t kIntThree = 3u << 30;

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

// Перенести в аффекты
constexpr bitvector_t AF_BATTLEDEC = 1 << 0;
constexpr bitvector_t AF_DEADKEEP = 1 << 1;
constexpr bitvector_t AF_PULSEDEC = 1 << 2;
constexpr bitvector_t AF_SAME_TIME = 1 << 3; // тикает раз в две секунды или во время раунда в бою (чтобы не между раундами)

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

// object-related defines ******************************************* //

template<typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
	return static_cast<typename std::underlying_type<E>::type>(e);
}

constexpr bitvector_t TRACK_NPC = 1 << 0;
constexpr bitvector_t TRACK_HIDE = 1 << 1;

// other miscellaneous defines ****************************************** //

enum { DRUNK, FULL, THIRST };
// pernalty types
enum { P_DAMROLL, P_HITROLL, P_CAST, P_MEM_GAIN, P_MOVE_GAIN, P_HIT_GAIN, P_AC };

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
 * kLevelImplementator should always be the HIGHEST possible immortal level, and
 * kLevelImmortal should always be the LOWEST immortal level.  The number of
 * mortal levels will always be kLevelImmortal - 1.
 */
const short kLevelImplementator = 34;
const short kLevelGreatGod = 33;
const short kLevelBuilder = 33;
const short kLevelGod = 32;
const short kLevelImmortal = 31;
const short kLevelFreeze = kLevelGreatGod; // Level of the 'freeze' command //

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

// Перенести в мобакт
const short kStupidMod = 10;
const short kMiddleAI = 30;
const short kHighAI = 40;
const short kCharacterHPForMobPriorityAttack = 100;
const short kStrongMobLevel = 30;
const short kMaxMobLevel = 100;
const short kMaxSaving = 400; //максимальное значение воля, здоровье, стойкость, реакция

bool sprintbitwd(bitvector_t bitvector, const char *names[], char *result, const char *div, const int print_flag = 0);

inline bool sprintbit(bitvector_t bitvector, const char *names[], char *result, const int print_flag = 0) {
	return sprintbitwd(bitvector, names, result, ",", print_flag);
}

// header block for rent files.  BEWARE: Changing it will ruin rent files  //
struct SaveRentInfo {
	SaveRentInfo() : time(0), rentcode(0), net_cost_per_diem(0), gold(0),
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

struct SaveTimeInfo {
	int32_t vnum;
	int32_t timer;
};

struct SaveInfo {
	struct SaveRentInfo rent;
	std::vector<SaveTimeInfo> time;
};

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

struct Timed {
	ubyte skill = 0;					// Number of used skill/spell //
	ubyte time = 0;						// Time for next using        //
	struct Timed *next = nullptr;
};

// Structure used for entities following other entities //
struct Follower {
	CHAR_DATA *ch = nullptr;
	struct Follower *next = nullptr;
};

// Structure used for tracking a mob //
struct track_info {
	int trk_info = 0;
	int who = 0;
	int dirs = 0;
};

// Structure used for helpers //
struct Helper {
	MobVnum mob_vnum = 0;
	struct Helper *next = nullptr;
};

// Перенести в загрузку предметов
// Structure used for on_dead object loading //
struct LoadingItem {
	ObjVnum obj_vnum = 0;
	int load_prob = 0;
	int load_type = 0;
	int spec_param = 0;
};

using OnDeadLoadList = std::list<struct LoadingItem *>;

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

	virtual ~AbstractStringWriter() = default;
	[[nodiscard]] virtual const char *get_string() const = 0;
	virtual void set_string(const char *data) = 0;
	virtual void append_string(const char *data) = 0;
	[[nodiscard]] virtual size_t length() const = 0;
	virtual void clear() = 0;
};

class DelegatedStringWriter : public AbstractStringWriter {
 public:
	DelegatedStringWriter(char *&managed) : m_delegated_string_(managed) {}
	virtual const char *get_string() const override { return m_delegated_string_; }
	virtual void set_string(const char *string) override;
	virtual void append_string(const char *string) override;
	virtual size_t length() const override { return m_delegated_string_ ? strlen(m_delegated_string_) : 0; }
	virtual void clear() override;

 private:
	char *&m_delegated_string_;
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
	virtual std::string &string() override { return m_string_; }
	virtual const std::string &string() const override { return m_string_; }

	std::string m_string_;
};

class DelegatedStdStringWriter : public AbstractStringWriter {
 public:
	DelegatedStdStringWriter(std::string &string) : m_string_(string) {}
	virtual const char *get_string() const override { return m_string_.c_str(); }
	virtual void set_string(const char *string) override { m_string_ = string; }
	virtual void append_string(const char *string) override { m_string_ += string; }
	virtual size_t length() const override { return m_string_.length(); }
	virtual void clear() override { m_string_.clear(); }

 private:
	std::string &m_string_;
};

// other miscellaneous structures **************************************

struct AttackMsg {
	char *attacker_msg = nullptr;	// message to attacker //
	char *victim_msg = nullptr;		// message to victim   //
	char *room_msg = nullptr;		// message to room     //
};

struct AttackMsgSet {
	struct AttackMsg die_msg;        // messages when death        //
	struct AttackMsg miss_msg;        // messages when miss         //
	struct AttackMsg hit_msg;        // messages when hit       //
	struct AttackMsg god_msg;        // messages when hit on god      //
	struct AttackMsgSet *next = nullptr;    // to next messages of this kind.   //
};

struct AttackMessages {
	int attack_type = 0;				// Attack type          //
	int number_of_attacks = 0;			// How many attack messages to chose from. //
	struct AttackMsgSet *msg = nullptr;	// List of messages.       //
};

struct ZoneCategory {
	char *name = nullptr;            // type name //
	int ingr_qty = 0;        // quantity of ingredient types //
	int *ingr_types = nullptr;    // types of ingredients, which are loaded in zones of this type //
};

struct IntApplies {
	int spell_aknowlege;    // drop_chance to know spell               //
	int to_skilluse;        // ADD CHANSE FOR USING SKILL         //
	int mana_per_tic;
	int spell_success;        //  max count of spell on 1s level    //
	int improve;        // drop_chance to improve skill           //
	int observation;        // drop_chance to use SKILL_AWAKE/CRITICAL //
};

struct ChaApplies {
	int leadership;
	int charms;
	int morale;
	int illusive;
	int dam_to_hit_rate;
};

struct SizeApplies {
	int ac;            // ADD VALUE FOR AC           //
	int interpolate;        // ADD VALUE FOR SOME SKILLS  //
	int initiative;
	int shocking;
};

struct WeaponApplies {
	int shocking;
	int bashing;
	int parrying;
};

/*struct race_app_type {
	struct extra_affects_type *extra_affects;
	struct obj_affected_type *extra_modifiers;
};*/

struct title_type {
	char *title_m = nullptr;
	char *title_f = nullptr;
	int exp = 0;
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

struct SocialMessages {
	int ch_min_pos = 0;
	int ch_max_pos = 0;
	int vict_min_pos = 0;
	int vict_max_pos = 0;
	char *char_no_arg = nullptr;
	char *others_no_arg = nullptr;

	// An argument was there, and a victim was found //
	char *char_found = nullptr;    // if NULL, read no further, ignore args //
	char *others_found = nullptr;
	char *vict_found = nullptr;

	// An argument was there, but no victim was found //
	char *not_found = nullptr;
};

struct SocialKeyword {
	char *keyword = nullptr;
	int social_message = 0;
};

extern struct SocialMessages *soc_mess_list;
extern struct SocialKeyword *soc_keys_list;

const __uint8_t GAPPLY_NONE = 0;
const __uint8_t GAPPLY_SKILL_SUCCESS = 1;
const __uint8_t GAPPLY_SPELL_SUCCESS = 2;
const __uint8_t GAPPLY_SPELL_EFFECT = 3;
const __uint8_t GAPPLY_MODIFIER = 4;
const __uint8_t GAPPLY_AFFECT = 5;

/* PCCleanCriteria структура которая определяет через какой время
   неактивности будет удален чар
*/
struct PCCleanCriteria {
	int level = 0;	// max уровень для этого временного лимита //
	int days = 0;	// временной лимит в днях        //
};

// Структрура для описания проталов для спела townportal //
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

// Структуры для act.wizard.cpp //

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

#endif // STRUCTS_STRUCTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
