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

#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <list>
#include <new>
#include <utility>
#include <vector>
#include <sstream>
#include <algorithm>

#include "engine/core/conf.h"
#include "engine/core/config.h"
#include "utils/id_converter.h"
#include "utils_string.h"
#include "logger.h"
#include "utils/backtrace.h"

#include <third_party_libs/fmt/include/fmt/format.h>

struct RoomData;    // forward declaration to avoid inclusion of room.hpp and any dependencies of that header.
class CharData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.
struct DescriptorData;

// external declarations and prototypes *********************************

// -------------------------------DO_NOT_REMOVE--------------------------------
// Full ASCII table for fast replace
//const char ascii_table_full[] = {
//	'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e', '\x0f',	//16
//	'\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f',	//32
//	'\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f',	//48
//	'\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f',	//64
//	'\x40', '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47', '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f',	//80
//	'\x50', '\x51', '\x52', '\x53', '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f',	//96
//	'\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f',	//112
//	'\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77', '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d', '\x7e', '\x7f',	//128
//	'\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',	//144
//	'\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f',	//160
//	'\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7', '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf',	//176
//	'\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5', '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf',	//192
//	'\xc0', '\xc1', '\xc2', '\xc3', '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf',	//208
//	'\xd0', '\xd1', '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7', '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',	//224
//	'\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed', '\xee', '\xef',	//240
//	'\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7', '\xf8', '\xf9', '\xfa', '\xfb', '\xfc', '\xfd', '\xfe', '\xff'	//256
//};

// Full ASCII table for fast erase entities
//const bool a_isdigit_table[] = {
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
//};

#define not_null(ptr, str) ((ptr) && *(ptr)) ? (ptr) : (str) ? (str) : "undefined"

inline const char *not_empty(const std::string &s) {
	return s.empty() ? "undefined" : s.c_str();
}

inline const char *not_empty(const std::string &s, const char *subst) {
	return s.empty() ? (subst ? subst : "undefined") : s.c_str();
}

//extern struct Weather weather_info;
extern char AltToKoi[];
extern char KoiToAlt[];
extern char WinToKoi[];
extern char KoiToWin[];
extern char KoiToWin2[];
extern char AltToLat[];

// public functions in utils.cpp
char *rustime(const struct tm *timeptr);
char *str_dup(const char *source);
char *str_add(char *dst, const char *src);
int str_cmp(const char *arg1, const char *arg2);
int str_cmp(const std::string &arg1, const char *arg2);
int str_cmp(const char *arg1, const std::string &arg2);
int str_cmp(const std::string &arg1, const std::string &arg2);
int strn_cmp(const char *arg1, const char *arg2, size_t n);
int strn_cmp(const std::string &arg1, const char *arg2, size_t n);
int strn_cmp(const char *arg1, const std::string &arg2, size_t n);
int strn_cmp(const std::string &arg1, const std::string &arg2, size_t n);
int touch(const char *path);
// \todo убрать и оставить только в random.h
int number(int from, int to);
int RollDices(int number, int size);

void sprinttype(int type, const char *names[], char *result);
int get_line(FILE *fl, char *buf);
int replace_str(const utils::AbstractStringWriter::shared_ptr &writer, const char *pattern, const char *replacement, int rep_all, int max_size);
void format_text(const utils::AbstractStringWriter::shared_ptr &writer, int mode, DescriptorData *d, size_t maxlen);
void koi_to_alt(char *str, int len);
std::string koi_to_alt(const std::string &input);
void koi_to_win(char *str, int len);
void koi_to_utf8(char *str_i, char *str_o);
void utf8_to_koi(char *str_i, char *str_o);
// \todo Заменить на фунции из STD
int roundup(float fl);
bool IsValidEmail(const char *address);
void skip_dots(char **string);
char *str_str(const char *cs, const char *ct);
void kill_ems(char *str);
void cut_one_word(std::string &str, std::string &word);
size_t strl_cpy(char *dst, const char *src, size_t siz);
bool is_head(std::string name);

template<typename T> inline std::string to_string(const T &t) {
	std::stringstream ss;
	ss << t;
	return ss.str();
};

extern const char *ACTNULL;

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == nullptr) i = ACTNULL; else i = (expression);

#define MIN_TITLE_LEV   25

// undefine MAX and MIN so that our functions are used instead
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

constexpr int kSfEmpty = 1 << 0;
constexpr int kSfFollowerdie = 1 << 1;
constexpr int kSfMasterdie = 1 << 2;
constexpr int kSfCharmlost = 1 << 3;
constexpr int kSfSilence = 1 << 4;

int MAX(int a, int b);
int MIN(int a, int b);

#define KtoW(c) ((ubyte)(c) < 128 ? (c) : KoiToWin[(ubyte)(c)-128])
#define KtoW2(c) ((ubyte)(c) < 128 ? (c) : KoiToWin2[(ubyte)(c)-128])
#define KtoA(c) ((ubyte)(c) < 128 ? (c) : KoiToAlt[(ubyte)(c)-128])
#define WtoK(c) ((ubyte)(c) < 128 ? (c) : WinToKoi[(ubyte)(c)-128])
#define AtoK(c) ((ubyte)(c) < 128 ? (c) : AltToKoi[(ubyte)(c)-128])
#define AtoL(c) ((ubyte)(c) < 128 ? (c) : AltToLat[(ubyte)(c)-128])

// various constants ****************************************************
// get_filename() //
const int kAliasFile = 1;
const int kScriptVarsFile = 2;
const int kPlayersFile = 3;
const int kTextCrashFile = 4;
const int kTimeCrashFile = 5;
const int kPersDepotFile = 6;
const int kShareDepotFile = 7;
const int kPurgeDepotFile = 8;

// breadth-first searching //
const int kBfsError = -1;
const int kBfsAlreadyThere = -2;
const int kBfsNoPath = -3;

// real-life time (remember Real Life?)
const int kSecsPerRealMin = 60;
constexpr int kSecsPerRealHour = 60*kSecsPerRealMin;
constexpr int kSecsPerRealDay = 24*kSecsPerRealHour;

#define IS_IMMORTAL(ch)     (!(ch)->IsNpc() && (ch)->GetLevel() >= kLvlImmortal)
#define IS_GOD(ch)          (!(ch)->IsNpc() && (ch)->GetLevel() >= kLvlGod)
#define IS_GRGOD(ch)        (!(ch)->IsNpc() && (ch)->GetLevel() >= kLvlGreatGod)
#define IS_IMPL(ch)         (!(ch)->IsNpc() && (ch)->GetLevel() >= kLvlImplementator)

#define IS_SHOPKEEPER(ch) (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == shop_ext)
#define IS_RENTKEEPER(ch) (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == receptionist)
#define IS_POSTKEEPER(ch) (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == postmaster)
#define IS_BANKKEEPER(ch) (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == bank)

// string utils *********************************************************

#define LOWER(c)   (a_lcc(c))
#define UPPER(c)   (a_ucc(c))
#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')

// memory utils *********************************************************

template<typename T>
inline void CREATE(T *&result, const size_t number) {
	result = static_cast<T *>(calloc(number, sizeof(T)));
	if (!result) {
		perror("SYSERR: calloc failure");
		abort();
	}
}

/*template<>
inline void CREATE(ExtraDescription *&, const size_t) {
	throw std::runtime_error("for ExtraDescription you have to use operator new");
}*/

template<typename T>
inline void RECREATE(T *&result, const size_t number) {
	result = static_cast<T *>(realloc(result, number * sizeof(T)));
	if (!result) {
		perror("SYSERR: realloc failure");
		abort();
	}
}

template<typename T>
inline T *NEWCREATE() {
	T *result = new(std::nothrow) T;
	if (!result) {
		perror("SYSERR: new failure");
		abort();
	}
	return result;
}

template<typename T>
inline void NEWCREATE(T *&result) {
	result = NEWCREATE<T>();
}

template<typename T, typename I>
inline void NEWCREATE(T *&result, const I &init_value) {
	result = new(std::nothrow) T(init_value);
	if (!result) {
		perror("SYSERR: new failure");
		abort();
	}
}

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
template<typename ListType, typename GetNextFunc>
inline void REMOVE_FROM_LIST(ListType *item, ListType *&head, GetNextFunc next) {
	if ((item) == (head)) {
		head = next(item);
	} else {
		auto temp = head;
		while (temp && (next(temp) != (item))) {
			temp = next(temp);
		}
		if (temp) {
			next(temp) = next(item);
		}
	}
}

template<typename ListType>
inline void REMOVE_FROM_LIST(ListType *item, ListType *&head) {
	REMOVE_FROM_LIST(item, head, [](ListType *list) -> ListType *& { return list->next; });
}

template<typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
	return static_cast<typename std::underlying_type<E>::type>(e);
};

// basic bitvector utils ************************************************

template<typename T>
struct UNIMPLEMENTED {};

template<typename T>
inline bool IS_SET(const T flag, const Bitvector bit) {
	return 0 != (flag & 0x3FFFFFFF & bit);
}

template<typename T, typename EnumType>
inline void SET_BIT(T &var, const EnumType bit) {
	var |= (to_underlying(bit) & 0x3FFFFFFF);
}

template<typename T>
inline void SET_BIT(T &var, const Bitvector bit) {
	var |= (bit & 0x3FFFFFFF);
}

template<typename T>
inline void SET_BIT(T &var, const int bit) {
	var |= (bit & 0x3FFFFFFF);
}

template<typename T, typename EnumType>
inline void REMOVE_BIT(T &var, const EnumType bit) {
	var &= ~(to_underlying(bit) & 0x3FFFFFFF);
}

template<typename T>
inline void REMOVE_BIT(T &var, const Bitvector bit) {
	var &= ~(bit & 0x3FFFFFFF);
}

template<typename T>
inline void REMOVE_BIT(T &var, const int bit) {
	var &= ~(bit & 0x3FFFFFFF);
}

template<typename T>
inline void TOGGLE_BIT(T &var, const Bitvector bit) {
	var = var ^ (bit & 0x3FFFFFFF);
}


#define NPC_FLAGS(ch)  ((ch)->mob_specials.npc_flags)
#define EXTRA_FLAGS(ch) ((ch)->Temporary)

#define IS_MOB(ch)          ((ch)->IsNpc() && (ch)->get_rnum() >= 0)

#define NPC_FLAGGED(ch, flag)   (NPC_FLAGS(ch).get(flag))
#define EXTRA_FLAGGED(ch, flag) (EXTRA_FLAGS(ch).get(flag))
#define ROOM_FLAGGED(loc, flag) (world[(loc)]->get_flag(flag))
#define EXIT_FLAGGED(exit, flag)     (IS_SET((exit)->exit_info, (flag)))
#define OBJVAL_FLAGGED(obj, flag)    (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define OBJWEAR_FLAGGED(obj, mask)   ((obj)->get_wear_mask(mask))

// room utils ***********************************************************
#define SECT(room)   (world[(room)]->sector_type)
#define GET_ROOM_SKY(room) (world[room]->weather.duration > 0 ? world[room]->weather.sky : weather_info.sky)

#define VALID_RNUM(rnum)   ((rnum) > 0 && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) ((RoomVnum)(VALID_RNUM(rnum) ? world[(rnum)]->vnum : kNowhere))
#define GET_ROOM_SPEC(room) (VALID_RNUM(room) ? world[(room)]->func : nullptr)

// char utils ***********************************************************
#define IS_MANA_CASTER(ch) ((ch)->GetClass() == ECharClass::kMagus)
#define GET_REAL_AGE(ch) (CalcCharAge((ch))->year + GET_AGE_ADD(ch))
#define GET_PC_NAME(ch) ((ch)->GetCharAliases().c_str())
#define GET_NAME(ch)    ((ch)->get_name().c_str())
#define GET_MAX_MANA(ch)      (mana[MIN(50, GetRealWis(ch))])
#define GET_MEM_CURRENT(ch)   ((ch)->mem_queue.Empty() ? 0 : CalcSpellManacost(ch, (ch)->mem_queue.queue->spell_id))

#define GET_EMAIL(ch)          ((ch)->player_specials->saved.EMail)
#define GET_LASTIP(ch)         ((ch)->player_specials->saved.LastIP)
#define GET_GOD_FLAG(ch, flag)  (IS_SET((ch)->player_specials->saved.GodsLike, flag))
#define SET_GOD_FLAG(ch, flag)  (SET_BIT((ch)->player_specials->saved.GodsLike, flag))
#define CLR_GOD_FLAG(ch, flag)  (REMOVE_BIT((ch)->player_specials->saved.GodsLike, flag))
#define LAST_LOGON(ch)         ((ch)->get_last_logon())
#define LAST_EXCHANGE(ch)         ((ch)->get_last_exchange())
#define NAME_GOD(ch)  ((ch)->player_specials->saved.NameGod)
#define NAME_ID_GOD(ch)  ((ch)->player_specials->saved.NameIDGod)

#define STRING_LENGTH(ch)  ((ch)->player_specials->saved.stringLength)
#define STRING_WIDTH(ch)  ((ch)->player_specials->saved.stringWidth)
#define NOTIFY_EXCH_PRICE(ch)  ((ch)->player_specials->saved.ntfyExchangePrice)

#define POSI(val)      (((val) < 50) ? (((val) > 0) ? (val) : 1) : 50)

template<typename T>
inline T VPOSI(const T val, const T min, const T max) {
	return ((val < max) ? ((val > min) ? val : min) : max);
}

#define GET_KIN(ch)     ((ch)->player_data.Kin)
#define GET_HEIGHT(ch)  ((ch)->player_data.height)
#define GET_HEIGHT_ADD(ch) ((ch)->add_abils.height_add)
#define GET_REAL_HEIGHT(ch) (GET_HEIGHT(ch) + GET_HEIGHT_ADD(ch))
#define GET_WEIGHT(ch)  ((ch)->player_data.weight)
#define GET_WEIGHT_ADD(ch) ((ch)->add_abils.weight_add)
#define GET_REAL_WEIGHT(ch) (GET_WEIGHT(ch) + GET_WEIGHT_ADD(ch))

#define GET_RELIGION(ch) ((ch)->player_data.Religion)
#define GET_RACE(ch) ((ch)->player_data.Race)
#define GET_PAD(ch, i)    ((ch)->player_data.PNames[i].c_str())
#define GET_DRUNK_STATE(ch) ((ch)->player_specials->saved.DrunkState)

#define GET_STR_ADD(ch) ((ch)->get_str_add())
#define GET_CON_ADD(ch) ((ch)->get_con_add())
#define GET_WIS_ADD(ch) ((ch)->get_wis_add())
#define GET_INT_ADD(ch) ((ch)->get_int_add())
#define GET_CHA_ADD(ch) ((ch)->get_cha_add())
#define GET_SIZE(ch)    ((ch)->real_abils.size)
#define GET_SIZE_ADD(ch)  ((ch)->add_abils.size_add)
#define GET_REAL_SIZE(ch) (VPOSI(GET_SIZE(ch) + GET_SIZE_ADD(ch), 1, 100))
#define GET_POS_SIZE(ch)  (POSI(GET_REAL_SIZE(ch) >> 1))
#define GET_HR(ch)         ((ch)->real_abils.hitroll)
#define GET_HR_ADD(ch)    ((ch)->add_abils.hr_add)
#define GET_REAL_HR(ch)   (VPOSI(GET_HR(ch)+GET_HR_ADD(ch), -50, (IS_MORTIFIER(ch) ? 100 : 50)))
#define GET_DR(ch)         ((ch)->real_abils.damroll)
#define GET_DR_ADD(ch)    ((ch)->add_abils.dr_add)
#define GET_AC(ch)         ((ch)->real_abils.armor)
#define GET_AC_ADD(ch)    ((ch)->add_abils.ac_add)
#define GET_MORALE(ch)       ((ch)->add_abils.morale)
#define GET_INITIATIVE(ch)   ((ch)->add_abils.initiative_add)
#define GET_POISON(ch)      ((ch)->add_abils.poison_add)
#define GET_SKILL_REDUCE(ch)      ((ch)->add_abils.skill_reduce_add)
#define GET_CAST_SUCCESS(ch) ((ch)->add_abils.cast_success)
#define GET_PRAY(ch)         ((ch)->add_abils.pray_add)

#define GET_MANAREG(ch)   ((ch)->add_abils.manareg)
#define GET_ARMOUR(ch)    ((ch)->add_abils.armour)
#define GET_ABSORBE(ch)   ((ch)->add_abils.absorb)
#define GET_AGE_ADD(ch)   ((ch)->add_abils.age_add)
#define GET_RESIST(ch, i)  ((ch)->add_abils.apply_resistance[i])
#define GET_AR(ch)        ((ch)->add_abils.aresist)
#define GET_MR(ch)        ((ch)->add_abils.mresist)
#define GET_PR(ch)        ((ch)->add_abils.presist)
#define GET_LIKES(ch)     ((ch)->mob_specials.like_work)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)

// Получение кубиков урона - работает только для мобов!
#define GET_NDD(ch) ((ch)->mob_specials.damnodice)
#define GET_SDD(ch) ((ch)->mob_specials.damsizedice)

const int kAligEvilLess = -300;
const int kAligGoodMore = 300;

#define GET_ALIGNMENT(ch)     ((ch)->char_specials.saved.alignment)

const int kNameLevel = 5;
#define NAME_FINE(ch)          (NAME_GOD(ch)>1000)
#define NAME_BAD(ch)           (NAME_GOD(ch)<1000 && NAME_GOD(ch))

#define GET_COND(ch, i)        ((ch)->player_specials->saved.conditions[(i)])
#define GET_LOADROOM(ch)    ((ch)->player_specials->saved.load_room)
#define GET_WIMP_LEV(ch)    ((ch)->player_specials->saved.wimp_level)
#define GET_BAD_PWS(ch)        ((ch)->player_specials->saved.bad_pws)
#define POOFIN(ch)            ((ch)->player_specials->poofin)
#define POOFOUT(ch)            ((ch)->player_specials->poofout)
#define NORENTABLE(ch)        ((ch)->IsNpc() ? 0 : (ch)->player_specials->may_rent)
#define AGRESSOR(ch)        ((ch)->player_specials->agressor)
#define AGRO(ch)            ((ch)->player_specials->agro_time)

#define EXCHANGE_FILTER(ch)    ((ch)->player_specials->Exchange_filter)

#define GET_ALIASES(ch)        ((ch)->player_specials->aliases)
#define GET_RSKILL(ch)        ((ch)->player_specials->rskill)
#define GET_LOGS(ch)        ((ch)->player_specials->logs)

// Punishments structs
#define MUTE_REASON(ch)        ((ch)->player_specials->pmute.reason)
#define DUMB_REASON(ch)        ((ch)->player_specials->pdumb.reason)
#define HELL_REASON(ch)        ((ch)->player_specials->phell.reason)
#define FREEZE_REASON(ch)    ((ch)->player_specials->pfreeze.reason)
#define GCURSE_REASON(ch)    ((ch)->player_specials->pgcurse.reason)
#define NAME_REASON(ch)        ((ch)->player_specials->pname.reason)
#define UNREG_REASON(ch)    ((ch)->player_specials->punreg.reason)

#define GET_MUTE_LEV(ch)    ((ch)->player_specials->pmute.level)
#define GET_DUMB_LEV(ch)    ((ch)->player_specials->pdumb.level)
#define GET_HELL_LEV(ch)    ((ch)->player_specials->phell.level)
#define GET_FREEZE_LEV(ch)    ((ch)->player_specials->pfreeze.level)
#define GET_GCURSE_LEV(ch)    ((ch)->player_specials->pgcurse.level)
#define GET_NAME_LEV(ch)    ((ch)->player_specials->pname.level)
#define GET_UNREG_LEV(ch)    ((ch)->player_specials->punreg.level)

#define MUTE_GODID(ch)        ((ch)->player_specials->pmute.godid)
#define DUMB_GODID(ch)        ((ch)->player_specials->pdumb.godid)
#define HELL_GODID(ch)        ((ch)->player_specials->phell.godid)
#define FREEZE_GODID(ch)    ((ch)->player_specials->pfreeze.godid)
#define GCURSE_GODID(ch)    ((ch)->player_specials->pgcurse.godid)
#define NAME_GODID(ch)        ((ch)->player_specials->pname.godid)
#define UNREG_GODID(ch)        ((ch)->player_specials->punreg.godid)

#define GCURSE_DURATION(ch)    ((ch)->player_specials->pgcurse.duration)
#define MUTE_DURATION(ch)    ((ch)->player_specials->pmute.duration)
#define DUMB_DURATION(ch)    ((ch)->player_specials->pdumb.duration)
#define FREEZE_DURATION(ch)    ((ch)->player_specials->pfreeze.duration)
#define HELL_DURATION(ch)    ((ch)->player_specials->phell.duration)
#define NAME_DURATION(ch)    ((ch)->player_specials->pname.duration)
#define UNREG_DURATION(ch)    ((ch)->player_specials->punreg.duration)

#define KARMA(ch)            ((ch)->player_specials->Karma)
#define LOGON_LIST(ch)        ((ch)->player_specials->logons)

#define CLAN(ch)            ((ch)->player_specials->clan)
#define CLAN_MEMBER(ch)        ((ch)->player_specials->clan_member)
#define GET_CLAN_STATUS(ch)    ((ch)->player_specials->clanStatus)

#define IS_SPELL_SET(ch, i, pct) (GET_SPELL_TYPE((ch), (i)) & (pct))
#define GET_SPELL_TYPE(ch, i) ((ch)->real_abils.SplKnw[to_underlying(i)])
#define UNSET_SPELL_TYPE(ch, i, pct) (GET_SPELL_TYPE((ch), (i)) &= ~(pct))
#define GET_SPELL_MEM(ch, i)  ((ch)->real_abils.SplMem[to_underlying(i)])
#define SET_SPELL_MEM(ch, i, pct) ((ch)->real_abils.SplMem[to_underlying(i)] = (pct))

#define GET_EQ(ch, i)      ((ch)->equipment[i])

#define GET_MOB_SPEC(ch)   (IS_MOB(ch) ? mob_index[(ch)->get_rnum()].func : nullptr)
#define GET_MOB_RNUM(mob)  (mob)->get_rnum()
#define GET_MOB_VNUM(mob)  (IS_MOB(mob) ? mob_index[(mob)->get_rnum()].vnum : -1)

#define GET_DEFAULT_POS(ch)   ((ch)->mob_specials.default_pos)
#define MEMORY(ch)          ((ch)->mob_specials.memory)
#define GET_DEST(ch)        (((ch)->mob_specials.dest_count ? \
                              (ch)->mob_specials.dest[(ch)->mob_specials.dest_pos] : \
                              kNowhere))
#define GET_ACTIVITY(ch)    ((ch)->mob_specials.activity)
#define GET_GOLD_NoDs(ch)   ((ch)->mob_specials.GoldNoDs)
#define GET_GOLD_SiDs(ch)   ((ch)->mob_specials.GoldSiDs)
#define GET_HORSESTATE(ch)  ((ch)->mob_specials.HorseState)
#define GET_LASTROOM(ch)    ((ch)->mob_specials.LastRoom)

#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, EAffect::kInfravision) || (!(ch)->IsNpc() && (ch)->IsFlagged(EPrf::kHolylight)))

#define IS_GOOD(ch)          (GET_ALIGNMENT(ch) >= kAligGoodMore)
#define IS_EVIL(ch)          (GET_ALIGNMENT(ch) <= kAligEvilLess)
#define ALIGN_DELTA  10
#define SAME_ALIGN(ch, vict)  (GET_ALIGNMENT(ch)>GET_ALIGNMENT(vict)?\
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
#define GET_CH_SUF_7(ch) (IS_NOSEXY(ch) ? "ым" :\
                          IS_MALE(ch) ? "ым"  :\
                          IS_FEMALE(ch) ? "ой" : "ыми")
#define GET_CH_SUF_8(ch) (IS_NOSEXY(ch) ? "ое" :\
                          IS_MALE(ch) ? "ой"  :\
                          IS_FEMALE(ch) ? "ая" : "ие")

#define GET_CH_VIS_SUF_1(ch, och) (!CAN_SEE(och,ch) ? "" :\
                          IS_NOSEXY(ch) ? "о" :\
                          IS_MALE(ch) ? ""  :\
                          IS_FEMALE(ch) ? "а" : "и")
#define GET_CH_VIS_SUF_2(ch, och) (!CAN_SEE(och,ch) ? "ся" :\
                          IS_NOSEXY(ch) ? "ось" :\
                          IS_MALE(ch) ? "ся"  :\
                          IS_FEMALE(ch) ? "ась" : "ись")
#define GET_CH_VIS_SUF_3(ch, och) (!CAN_SEE(och,ch) ? "ый" :\
                          IS_NOSEXY(ch) ? "ое" :\
                          IS_MALE(ch) ? "ый"  :\
                          IS_FEMALE(ch) ? "ая" : "ые")
#define GET_CH_VIS_SUF_4(ch, och) (!CAN_SEE(och,ch) ? "" :\
                          IS_NOSEXY(ch) ? "ло" :\
                          IS_MALE(ch) ? ""  :\
                          IS_FEMALE(ch) ? "ла" : "ли")
#define GET_CH_VIS_SUF_5(ch, och) (!CAN_SEE(och,ch) ? "ел" :\
                          IS_NOSEXY(ch) ? "ло" :\
                          IS_MALE(ch) ? "ел"  :\
                          IS_FEMALE(ch) ? "ла" : "ли")
#define GET_CH_VIS_SUF_6(ch, och) (!CAN_SEE(och,ch) ? "" :\
                          IS_NOSEXY(ch) ? "о" :\
                          IS_MALE(ch) ? ""  :\
                          IS_FEMALE(ch) ? "а" : "ы")
#define GET_CH_VIS_SUF_7(ch, och) (!CAN_SEE(och,ch) ? "ым" :\
                          IS_NOSEXY(ch) ? "ым" :\
                          IS_MALE(ch) ? "ой"  :\
                          IS_FEMALE(ch) ? "ым" : "ыми")
#define GET_CH_VIS_SUF_8(ch, och) (!CAN_SEE(och,ch) ? "ой" :\
                          IS_NOSEXY(ch) ? "ое" :\
                          IS_MALE(ch) ? "ой"  :\
                          IS_FEMALE(ch) ? "ая" : "ие")

#define GET_OBJ_SEX(obj) ((obj)->get_sex())
#define IS_OBJ_NOSEXY(obj)    (GET_OBJ_SEX(obj) == EGender::kNeutral)
#define IS_OBJ_MALE(obj)   (GET_OBJ_SEX(obj) == EGender::kMale)
#define IS_OBJ_FEMALE(obj)    (GET_OBJ_SEX(obj) == EGender::kFemale)
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
#define GET_OBJ_SUF_8(ch) (IS_OBJ_NOSEXY(obj) ? "ое" :\
                          IS_OBJ_MALE(obj) ? "ой"  :\
                          IS_OBJ_FEMALE(obj) ? "ая" : "ие")

#define GET_OBJ_VIS_SUF_1(obj, ch) (!CAN_SEE_OBJ(ch,obj) ? "о" :\
                            IS_OBJ_NOSEXY(obj) ? "о" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "а" : "и")
#define GET_OBJ_VIS_SUF_2(obj, ch) (!CAN_SEE_OBJ(ch,obj) ? "ось" :\
                            IS_OBJ_NOSEXY(obj) ? "ось" :\
                            IS_OBJ_MALE(obj) ? "ся"  :\
                            IS_OBJ_FEMALE(obj) ? "ась" : "ись")
#define GET_OBJ_VIS_SUF_3(obj, ch) (!CAN_SEE_OBJ(ch,obj) ? "ый" :\
                            IS_OBJ_NOSEXY(obj) ? "ое" :\
                            IS_OBJ_MALE(obj) ? "ый"  :\
                            IS_OBJ_FEMALE(obj) ? "ая" : "ые")
#define GET_OBJ_VIS_SUF_4(obj, ch) (!CAN_SEE_OBJ(ch,obj) ? "ло" :\
                            IS_OBJ_NOSEXY(obj) ? "ло" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "ла" : "ли")
#define GET_OBJ_VIS_SUF_5(obj, ch) (!CAN_SEE_OBJ(ch,obj) ? "ло" :\
                            IS_OBJ_NOSEXY(obj) ? "ло" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "ла" : "ли")
#define GET_OBJ_VIS_SUF_6(obj, ch) (!CAN_SEE_OBJ(ch,obj) ? "о" :\
                            IS_OBJ_NOSEXY(obj) ? "о" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "а" : "ы")
#define GET_OBJ_VIS_SUF_7(obj, ch) (!CAN_SEE_OBJ(ch,obj) ? "е" :\
                            IS_OBJ_NOSEXY(obj) ? "е" :\
                            IS_OBJ_MALE(obj) ? ""  :\
                            IS_OBJ_FEMALE(obj) ? "а" : "и")
#define GET_OBJ_VIS_SUF_8(obj, ch) (!CAN_SEE_OBJ(ch,obj) ? "ой" :\
                          IS_OBJ_NOSEXY(obj) ? "ое" :\
                          IS_OBJ_MALE(obj) ? "ой"  :\
                          IS_OBJ_FEMALE(obj) ? "ая" : "ие")

#define GET_CH_EXSUF_1(ch) (IS_NOSEXY(ch) ? "им" :\
                            IS_MALE(ch) ? "им"  :\
                            IS_FEMALE(ch) ? "ей" : "ими")
#define GET_CH_POLY_1(ch) (IS_POLY(ch) ? "те" : "")

#define GET_OBJ_POLY_1(ch, obj) ((GET_OBJ_SEX(obj) == EGender::kPoly) ? "ят" : "ит")

#define PUNCTUAL_WAIT_STATE(ch, cycle) do { (ch)->punctual_wait = (cycle); } while(0)

// compound utilities and other macros *********************************

#define HSHR(ch) (EGender::kNeutral != ((ch)->get_sex()) ? (IS_MALE(ch) ? "его": (IS_FEMALE(ch) ? "ее" : "их")) :"его")
#define HSSH(ch) (EGender::kNeutral != ((ch)->get_sex()) ? (IS_MALE(ch) ? "он": (IS_FEMALE(ch) ? "она" : "они")) :"оно")
#define HMHR(ch) (EGender::kNeutral != ((ch)->get_sex()) ? (IS_MALE(ch) ? "ему": (IS_FEMALE(ch) ? "ей" : "им")) :"ему")
#define HYOU(ch) (EGender::kNeutral != ((ch)->get_sex()) ? (IS_MALE(ch) ? "ваш": (IS_FEMALE(ch) ? "ваша" : (IS_NOSEXY(ch) ? "ваше": "ваши"))) :"ваш")

#define OSHR(ch) (EGender::kNeutral != GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch) == EGender::kMale ? "его": (GET_OBJ_SEX(ch) == EGender::kFemale ? "ее" : "их")) :"его")
#define OSSH(ch) (EGender::kNeutral != GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch) == EGender::kMale ? "он": (GET_OBJ_SEX(ch) == EGender::kFemale ? "она" : "они")) :"оно")
#define OMHR(ch) (EGender::kNeutral != GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch) == EGender::kMale ? "ему": (GET_OBJ_SEX(ch) == EGender::kFemale ? "ей" : "им")) :"ему")
#define OYOU(ch) (EGender::kNeutral != GET_OBJ_SEX(ch) ? (GET_OBJ_SEX(ch) == EGender::kMale ? "ваш": (GET_OBJ_SEX(ch) == EGender::kFemale ? "ваша" : "ваши")) :"ваше")

#define HERE(ch)  (((ch)->IsNpc() || (ch)->desc || NORENTABLE(ch)))

// Can subject see character "obj" without light
#define MORT_CAN_SEE_CHAR(sub, obj) (HERE(obj) && INVIS_OK(sub,obj))

#define IMM_CAN_SEE_CHAR(sub, obj) \
        (MORT_CAN_SEE_CHAR(sub, obj) || (!(sub)->IsNpc() && (sub)->IsFlagged(EPrf::kHolylight)))

#define CAN_SEE_CHAR(sub, obj) (SELF(sub, obj) || \
        ((GetRealLevel(sub) >= ((obj)->IsNpc() ? 0 : GET_INVIS_LEV(obj))) && \
         IMM_CAN_SEE_CHAR(sub, obj)))
// End of CAN_SEE

#define GET_PAD_PERS(pad) ((pad) == 5 ? "ком-то" :\
                           (pad) == 4 ? "кем-то" :\
                           (pad) == 3 ? "кого-то" :\
                           (pad) == 2 ? "кому-то" :\
                           (pad) == 1 ? "кого-то" : "кто-то")

#define PERS(ch, vict, pad) (CAN_SEE(vict, ch) ? GET_PAD(ch,pad) : GET_PAD_PERS(pad))
//для арены
#define APERS(ch, vict, pad, arena) ((arena) || CAN_SEE(vict, ch) ? GET_PAD(ch,pad) : GET_PAD_PERS(pad))

//для арены
#define AOBJS(obj, vict, arena) ((arena) || CAN_SEE_OBJ((vict), (obj)) ? \
                      (obj)->get_short_description().c_str() : "что-то")

#define GET_PAD_OBJ(pad)  ((pad) == 5 ? "чем-то" :\
                           (pad) == 4 ? "чем-то" :\
                           (pad) == 3 ? "что-то" :\
                           (pad) == 2 ? "чему-то" :\
                           (pad) == 1 ? "чего-то" : "что-то")

//для арены
#define AOBJN(obj, vict, pad, arena) ((arena) || CAN_SEE_OBJ((vict), (obj)) ? \
                           (!(obj)->get_PName(pad).empty()) ? (obj)->get_PName(pad).c_str() : (obj)->get_short_description().c_str() \
                           : GET_PAD_OBJ(pad))

#define EXITDATA(room, door) (((room) >= 0 && (room) <= top_of_world) ? \
                             world[(room)]->dir_option[(door)] : nullptr)

#define EXIT(ch, door)  (world[(ch)->in_room]->dir_option[door])

#define CAN_GO(ch, door) ((ch)?((EXIT(ch,door) && \
          (EXIT(ch,door)->to_room() != kNowhere) && \
          !IS_SET(EXIT(ch, door)->exit_info, EExitFlag::kClosed))):0)

#define IS_SORCERER(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kSorcerer))
#define IS_THIEF(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kThief))
#define IS_ASSASINE(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kAssasine))
#define IS_WARRIOR(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kWarrior))
#define IS_PALADINE(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kPaladine))
#define IS_RANGER(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kRanger))
#define IS_GUARD(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kGuard))
#define IS_VIGILANT(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kVigilant))
#define IS_MERCHANT(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kMerchant))
#define IS_MAGUS(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kMagus))
#define IS_CONJURER(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kConjurer))
#define IS_CHARMER(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kCharmer))
#define IS_WIZARD(ch)		(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kWizard))
#define IS_NECROMANCER(ch)	(!(ch)->IsNpc() && ((ch)->GetClass() == ECharClass::kNecromancer))

// \todo Ввести для комнат флаг а-ля "место отдыха", а это убрать.
#define LIKE_ROOM(ch) ((IS_SORCERER(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForSorcerers)) || \
                       (IsMage(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForMages)) || \
                       (IS_WARRIOR(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForWarriors)) || \
                       (IS_THIEF(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForThieves)) || \
                       (IS_ASSASINE(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForAssasines)) || \
                       (IS_GUARD(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForGuards)) || \
                       (IS_PALADINE(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForPaladines)) || \
                       (IS_RANGER(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForRangers)) || \
                       (IS_VIGILANT(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForge)) || \
                       (IS_MERCHANT(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForMerchants)) || \
                       (IS_MAGUS(ch) && ROOM_FLAGGED((ch)->in_room, ERoomFlag::kForMaguses)))

#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ERoomFlag::kIndoors))


// PADS for something ***************************************************
enum class EWhat : int  {
	kDay,
	kHour,
	kYear,
	kPoint,
	kMinA,
	kMinU,
	kMoneyA,
	kMoneyU,
	kThingA,
	kThingU,
	kLvl,
	kMoveA,
	kMoveU,
	kOneA,
	kOneU,
	kSec,
	kDegree,
	kRow,
	kObject,
	kObjU,
	kRemort,
	kWeek,
	kMonth,
	kWeekU,
	kGlory,
	kGloryU,
	kPeople,
	kStr,
	kGulp,
	kTorc,
	kGoldTorc,
	kSilverTorc,
	kBronzeTorc,
	kTorcU,
	kGoldTorcU,
	kSilverTorcU,
	kBronzeTorcU,
	kIceU,
	kNogataU
};

const char *GetDeclensionInNumber(long amount, EWhat of_what);
std::string FormatTimeToStr(long in_timer, bool flag = false);

// defines for fseek
#ifndef SEEK_SET
#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2
#endif

#define SENDOK(ch)   (((ch)->desc || CheckScript((ch), MTRIG_ACT)) && \
               (to_sleeping || AWAKE(ch)) && \
                     !(ch)->IsFlagged(EPlrFlag::kWriting))

extern const bool a_isspace_table[];
inline bool a_isspace(const unsigned char c) {
	return a_isspace_table[c];
}

// Далеко не все из следующих функций используются в коде, но пусть будут (переписано с асма AL'ом)
inline bool a_isascii(const unsigned char c) {
	return c >= 32;
}

inline bool a_isprint(const unsigned char c) {
	return c >= 32;
}

extern const bool a_islower_table[];
inline bool a_islower(const unsigned char c) {
	return a_islower_table[c];
}

extern const bool a_isupper_table[];
inline bool a_isupper(const unsigned char c) {
	return a_isupper_table[c];
}

extern const bool a_isdigit_table[];
inline bool a_isdigit(const unsigned char c) {
	return a_isdigit_table[c];
}

extern const bool a_isalpha_table[];
inline bool a_isalpha(const unsigned char c) {
	return a_isalpha_table[c];
}

extern const bool a_isalnum_table[];
inline bool a_isalnum(const unsigned char c) {
	return a_isalnum_table[c];
}

extern const bool a_isxdigit_table[];
inline bool a_isxdigit(const unsigned char c) {
	return a_isxdigit_table[c];
}

extern const char a_ucc_table[];
inline char a_ucc(const unsigned char c) {
	return a_ucc_table[c];
}

extern const char a_lcc_table[];
inline char a_lcc(const unsigned char c) {
	return a_lcc_table[c];
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

class pred_separator {
	bool (*pred)(unsigned char);
	bool l_not;
 public:
	explicit
	pred_separator(separator_mode _mode, bool _l_not = false) : l_not(_l_not) {
		switch (_mode) {
			case A_ISSPACE: pred = a_isspace;
				break;
			case A_ISASCII: pred = a_isascii;
				break;
			case A_ISPRINT: pred = a_isprint;
				break;
			case A_ISLOWER: pred = a_islower;
				break;
			case A_ISUPPER: pred = a_isupper;
				break;
			case A_ISDIGIT: pred = a_isdigit;
				break;
			case A_ISALPHA: pred = a_isalpha;
				break;
			case A_ISALNUM: pred = a_isalnum;
				break;
			case A_ISXDIGIT: pred = a_isxdigit;
				break;
		}
	}

	explicit
	pred_separator() : pred(a_isspace), l_not(false) {}

	void reset() {}

	bool operator()(std::string::const_iterator &next, std::string::const_iterator end, std::string &tok) {
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

// ВЕЯРМН ЯЙНОХОЮЯРЕМН
template<class T>
void StrTrim(T str) {
	int start = 0; // number of leading spaces
	char *buffer = str;
	while (*str && *str++ == ' ') ++start;
	while (*str++); // move to end of string
	int end = str - buffer - 1;
	while (end > 0 && buffer[end - 1] == ' ') --end; // backup over trailing spaces
	buffer[end] = 0; // remove trailing spaces
	if (end <= start || start == 0) return; // exit if no leading spaces or string is now empty
	str = buffer + start;
	while ((*buffer++ = *str++));  // remove leading spaces: K&R
}

template<class T>
void skip_spaces(T string) {
	for (; **string && a_isspace(**string); (*string)++);
}


std::string thousands_sep(long long n);

#define IS_CORPSE(obj)     ((obj)->get_type() == EObjType::kContainer && \
               GET_OBJ_VAL((obj), 3) == ObjData::CORPSE_INDICATOR)
#define IS_MOB_CORPSE(obj) (IS_CORPSE(obj) &&  GET_OBJ_VAL((obj), 2) != -1)

/// аналог sprintbitwd и производных
/// \param bits - bitset|boost::dynamic_bitset
/// \param names - vector|array<string|const char*> список названий битов
/// div - разделитель между битами при распечатке
/// str - строка, куда печаются имена битов (добавлением в конец)
/// print_num - печать номер бита рядом с его именем
/// 	(для олц, счет битов начинается с 1), по дефолту = false
template<class T, class N>
void print_bitset(const N &bits, const T &names,
				  const char *div, std::string &str, bool print_num = false) {
	static char tmp_buf[10];
	bool first = true;

	for (unsigned i = 0; i < bits.size(); ++i) {
		if (bits.test(i) == true) {
			if (!first) {
				str += div;
			} else {
				first = false;
			}

			if (print_num) {
				snprintf(tmp_buf, sizeof(tmp_buf), "%d:", i + 1);
				str += tmp_buf;
			}

			if (i < names.size()) {
				str += names[i];
			} else {
				str += "UNDEF";
			}
		}
	}
}

#ifdef WIN32
class CCheckTable
{
public:
	typedef int(*original_t)(int);
	typedef bool(*table_t) (unsigned char);
	CCheckTable(original_t original, table_t table) : m_original(original), m_table(table) {}
	bool test_values() const;
	double test_time() const;
	void check() const;

private:
	original_t m_original;
	table_t m_table;
};
#endif

// global buffering system
#ifdef DB_CPP_
char buf[kMaxStringLength];
char buf1[kMaxStringLength];
char buf2[kMaxStringLength];
char arg[kMaxInputLength];
char smallBuf[kMaxRawInputLength];
#else
extern char buf[kMaxStringLength];
extern char buf1[kMaxStringLength];
extern char buf2[kMaxStringLength];
extern char arg[kMaxInputLength];
extern char smallBuf[kMaxRawInputLength];
#endif

#define plant_magic(x)    do { (x)[sizeof(x) - 1] = kMagicNumber; } while (0)
#define test_magic(x)    ((x)[sizeof(x) - 1])

/*
* This function is called every 30 seconds from heartbeat().  It checks
* the four global buffers in CircleMUD to ensure that no one has written
* past their bounds.  If our check digit is not there (and the position
* doesn't have a NUL which may result from snprintf) then we gripe that
* someone has overwritten our buffer.  This could cause a false positive
* if someone uses the buffer as a non-terminated character array but that
* is not likely. -gg
*/
void sanity_check();

inline void graceful_exit(int retcode) {
	log("Exit with code %d (%s %s %d)", retcode, __FILE__, __func__, __LINE__);
	_exit(retcode);
}

bool isname(const char *str, const char *namelist);
inline bool isname(const std::string &str, const char *namelist) { return isname(str.c_str(), namelist); }
inline bool isname(const char *str, const std::string &namelist) { return isname(str, namelist.c_str()); }
inline bool isname(const std::string &str, const std::string &namelist) {
	return isname(str.c_str(),
				  namelist.c_str());
}

const char *one_word(const char *argument, char *first_arg);

void ReadEndString(std::ifstream &file);
// замена символа (в данном случае конца строки) на свою строку, для остального функций хватает
void StringReplace(std::string &buffer, char s, const std::string &d);
std::string &format_news_message(std::string &text);

template<typename T>
class JoinRange {
 public:
	explicit JoinRange(const T &container, std::string delimiter = ", ") :
		m_begin(container.begin()),
		m_end(container.end()),
		m_delimiter(std::move(delimiter)) {
	}

	JoinRange(const typename T::const_iterator &begin,
			  const typename T::const_iterator &end,
			  std::string delimiter = ", ") :
		m_begin(begin),
		m_end(end),
		m_delimiter(std::move(delimiter)) {
	}

	std::ostream &output(std::ostream &os) const {
		bool first = true;
		for (auto i = m_begin; i != m_end; ++i) {
			os << (first ? "" : m_delimiter) << *i;
			first = false;
		}

		return os;
	}

	[[nodiscard]] std::string as_string() const;

 private:
	typename T::const_iterator m_begin;
	typename T::const_iterator m_end;
	std::string m_delimiter;
};

template<typename T>
std::string JoinRange<T>::as_string() const {
	std::stringstream ss;
	output(ss);
	return ss.str();
}

template<typename T>
std::ostream &operator<<(std::ostream &os, const JoinRange<T> &range_printer) { return range_printer.output(os); }

template<typename T>
void joinRange(const typename T::const_iterator &begin,
			   const typename T::const_iterator &end,
			   std::string &result,
			   const std::string &delimiter = ", ") {
	std::stringstream ss;
	ss << JoinRange<T>(begin, end, delimiter);
	result = ss.str();
}

template<typename T>
void joinList(const T &list, std::string &result, const std::string &delimiter = ", ") {
	joinRange<T>(list.begin(), list.end(), result, delimiter);
}

template<typename... T>
constexpr auto make_array(T &&... values) ->
std::array<
	typename std::decay<
		typename std::common_type<T...>::type>::type,
	sizeof...(T)> {
	return std::array<
		typename std::decay<
			typename std::common_type<T...>::type>::type,
		sizeof...(T)>{std::forward<T>(values)...};
}

inline int posi_value(int current, int max) {
	if (current < 0) {
		return (-1);
	} else if (current >= max) {
		return (10);
	}

	return (current * 10 / std::max(max, 1));
}

class StreamFlagsHolder {
 public:
	explicit StreamFlagsHolder(std::ostream &os) : m_stream(os), m_flags(os.flags()) {}
	~StreamFlagsHolder() { m_stream.flags(m_flags); }

 private:
	std::ostream &m_stream;
	std::ios::fmtflags m_flags;
};
template <typename T>
struct reversion_wrapper { T& iterable; };

template <typename T>
auto begin (reversion_wrapper<T> w) { return std::rbegin(w.iterable); }

template <typename T>
auto end (reversion_wrapper<T> w) { return std::rend(w.iterable); }

template <typename T>
reversion_wrapper<T> reverse (T&& iterable) { return { iterable }; }

/**
 *  Напечатать число в виде строки с разделителем разрядов.
 *  @param num  - обрабатываемоле число.
 *  @param separator - разделитель разрядов.
 */
std::string PrintNumberByDigits(long long num, char separator = ' ');

void PruneCrlf(char *txt);

bool sprintbitwd(Bitvector bitvector, const char *names[], char *result, const char *div, int print_flag = 0);

inline bool sprintbit(Bitvector bitvector, const char *names[], char *result, const int print_flag = 0) {
	return sprintbitwd(bitvector, names, result, ",", print_flag);
}

#endif // UTILS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
