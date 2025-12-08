/* ************************************************************************
*   File: skills.h                                     Part of Bylins    *
*  Usage: headers: Skills functions                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#ifndef SKILLS_H_
#define SKILLS_H_

#include <map>

#include "gameplay/magic/spells_constants.h"
#include "engine/structs/structs.h"

extern const int kZeroRemortSkillCap;
extern const int kSkillCapBonusPerRemort;
extern const int kMinTalentLevelDecrement;
extern const long kMinImprove;

class CharData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

enum EExtraAttack {
	kExtraAttackUnused = 0,
	kExtraAttackThrow,
	kExtraAttackBash,
	kExtraAttackKick,
	kExtraAttackChopoff,
	kExtraAttackDisarm,
	kExtraAttackCut,
	kExtraAttackSlay,
	kExtraAttackInjure,
};

/*
 * ID'ы скиллов. Если добавляем id - его обязательно нужно инициализировать в конфиге,
 * даже если сам скилл отключён, иначе в сислоге будет спам об ошибках (система пытается проверять
 * скилл с таким id и не находит его).
 */
enum class ESkill : int {
	kReligion = -3,			// Таймер молитвы тикает за счет TimedSkill. Нужно придумать, как от этого избавиться
	kAny = -2,    			// "Какой угодно" скилл. (Например, удар можно нанести любым видом оружия).  //
	kUndefined = -1,		// Неопределенный или некорректный скилл.
	kGlobalCooldown = 0,	// Internal - ID for global ability cooldown //
	kProtect = 1,
	kIntercept = 2,
	kLeftHit = 3,
	kHammer = 4,
	kOverwhelm = 5,
	kPoisoning = 6,
	kSense = 7,
	kRiding = 8,
	kHideTrack = 9,
	kSkinning = 11,
	kMultiparry = 12,
	kReforging = 13,
	kLeadership = 20,
	kPunctual = 21,
	kAwake = 22,
	kIdentify = 23,
	kHearing = 24,
	kCreatePotion = 25,
	kCreateScroll = 26,
	kCreateWand = 27,
	kPry = 28,
	kArmoring = 29,
	kHangovering = 30,
	kFirstAid = 31,
	kCampfire = 32,
	kCreateBow = 33,
	kSlay = 34,
	kFrenzy = 35,
	kShieldBash = 128,
	kCutting = 129,		// Скилл-заглушка для способности "порез", иначе некорректно выдаются сообщения
	kThrow = 130,
	kBackstab = 131,
	kBash = 132,
	kHide = 133,
	kKick = 134,
	kPickLock = 135,
	kPunch = 136,		// *** Weapon is bare hands //
	kRescue = 137,
	kSneak = 138,
	kSteal = 139,
	kTrack = 140,
	kClubs = 141,		// *** Weapon is club, etc    //
	kAxes = 142,		// *** Weapon is axe, etc     //
	kLongBlades = 143,	// *** Weapon is long blades  //
	kShortBlades = 144,	// *** Weapon is short blades //
	kNonstandart = 145,	// *** Weapon is non-standart //
	kTwohands = 146,	// *** Weapon in both hands   //
	kPicks = 147,		// *** Weapon is pick         //
	kSpades = 148,		// *** Weapon is spades       //
	kSideAttack = 149,
	kDisarm = 150,
	kParry = 151,
	kCharge = 152,
	kMorph = 153,
	kBows = 154,
	kAddshot = 155,
	kDisguise = 156,
	kDodge = 157,
	kShieldBlock = 158,
	kLooking = 159,
	kChopoff = 160,
	kRepair = 161,
	kDazzle = 162,
	kThrowout = 163,
	kSharpening = 164,
	kCourage = 165,
	kJinx = 166,
	kNoParryHit = 167,
	kTownportal = 168,
	kMakeStaff = 169,
	kMakeBow = 170,
	kMakeWeapon = 171,
	kMakeArmor = 172,
	kMakeJewel = 173,
	kMakeWear = 174,
	kMakePotion = 175,
	kDigging = 176,
	kJewelry = 177,
	kWarcry = 178,
	kTurnUndead = 179,
	kIronwind = 180,
	kStrangle = 181,
	kAirMagic = 182,
	kFireMagic = 183,
	kWaterMagic = 184,
	kEarthMagic = 185,
	kLightMagic = 186,
	kDarkMagic = 187,
	kMindMagic = 188,
	kLifeMagic = 189,
	kStun = 190,
	kMakeAmulet = 191,

	kFirst = kProtect,
	kLast = kMakeAmulet	// не забываем указывать максимальный номер скилла
};

const ESkill& operator++(ESkill &s);
std::ostream& operator<<(std::ostream & os, ESkill &s);

template<>
ESkill ITEM_BY_NAME<ESkill>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<ESkill>(ESkill item);

struct SkillRollResult {
	bool success{true};
	bool critical{false};
	bool CritLuck{false};
	int SkillRate{0};
	int degree{0};
};

struct TimedSkill {
	ESkill skill{ESkill::kUndefined};	// Used skill //
	ubyte time{0};						// Time for next using //
	TimedSkill *next{nullptr};
};

int SendSkillMessages(int dam, CharData *ch, CharData *vict, int attacktype, std::string add = "");
int SendSkillMessages(int dam, CharData *ch, CharData *vict, ESkill skill_id, std::string add = "");
int SendSkillMessages(int dam, CharData *ch, CharData *vict, ESpell spell_id, std::string add = "");

char *how_good(int skill_level, int skill_cap);
int CalcCurrentSkill(CharData *ch, ESkill skill_id, CharData *vict, bool need_log = true);
void RemoveAllSkills(CharData *ch);
void ImproveSkill(CharData *ch, ESkill skill, int success, CharData *victim);
void TrainSkill(CharData *ch, ESkill skill, bool success, CharData *vict);

int GetSkillMinLevel(CharData *ch, ESkill skill);
int GetSkillMinLevel(CharData *ch, ESkill skill, int req_lvl);
bool CanGetSkill(CharData *ch, ESkill skill);
bool CanGetSkill(CharData *ch, ESkill skill, int req_lvl);
int CalcSkillRemortCap(const CharData *ch);
int CalcSkillWisdomCap(const CharData *ch);
int CalcSkillHardCap(const CharData *ch, ESkill skill);
int CalcSkillMinCap(const CharData *ch, ESkill skill);
SkillRollResult MakeSkillTest(CharData *ch, ESkill skill_id, CharData *vict, bool need_log = true);
void SendSkillBalanceMsg(CharData *ch, const std::string &skill_name, int percent, int prob, bool success);
int CalculateSkillAwakeModifier(CharData *killer, CharData *victim);
bool CritLuckTest(CharData *ch, CharData *vict);
int CalculateSkillRate(CharData *ch, const ESkill skill_id, CharData *vict);
#endif // SKILLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
