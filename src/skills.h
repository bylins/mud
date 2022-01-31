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

#include "structs/structs.h"

extern const int kSkillCapOnZeroRemort;
extern const int kSkillCapBonusPerRemort;
extern const long kMinImprove;

class CharacterData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

enum EExtraAttack {
	kExtraAttackUnused = 0,
	kExtraAttackThrow,
	kExtraAttackBash,
	kExtraAttackKick,
	kExtraAttackUndercut,
	kExtraAttackDisarm,
	kExtraAttackCutShorts,
	kExtraAttackPick
};

/*
 * ID'ы скиллов. Если добавляем id - его обязательно нужно инициализировать в SkillsInfo,
 * даже если сам скилл отключён, иначе в сислоге будет спам об ошибках (система пытается проверять
 * скилл с таким id и не находит его).
 */
enum class ESkill : int {
	kReligion = -4,			// Таймер молитвы тикает за счет TimedSkill. Нужно придумать, как от этого избавиться
	kAny = -3,    			// "Какой угодно" скилл. (Например, удар можно нанести любым видом оружия).  //
	kUndefined = -2,		// Неопределенный скилл.
	kIncorrect = -1,		// Неизвестный, но некорректный скилл.
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
						/* Просвет в 1 номер */
	kMultiparry = 12,
	kReforging = 13,
						/* Просвет почему-то */
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
						/* Снова просвет */
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
					/* Очередной просвет */
	kMorph = 153,
	kBows = 154,
	kAddshot = 155,
	kDisguise = 156,
	kDodge = 157,
	kShieldBlock = 158,
	kLooking = 159,
	kUndercut = 160,
	kRepair = 161,
					/* Снова просвет */
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

template<>
ESkill ITEM_BY_NAME<ESkill>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<ESkill>(ESkill item);

struct SkillRollResult {
	bool success{true};
	bool critical{false};
	int degree{0};
};

struct TimedSkill {
	ESkill skill{ESkill::kIncorrect};	// Used skill //
	ubyte time{0};						// Time for next using //
	TimedSkill *next{nullptr};
};

extern std::array<ESkill, to_underlying(ESkill::kLast) + 1> AVAILABLE_SKILLS;

int SendSkillMessages(int dam, CharacterData *ch, CharacterData *vict, int attacktype, std::string add = "");

int CalcCurrentSkill(CharacterData *ch, ESkill skill_id, CharacterData *vict);
void ImproveSkill(CharacterData *ch, ESkill skill, int success, CharacterData *victim);
void TrainSkill(CharacterData *ch, ESkill skill, bool success, CharacterData *vict);

int GetSkillMinLevel(CharacterData *ch, ESkill skill);
int GetSkillMinLevel(CharacterData *ch, ESkill skill, int req_lvl);
bool IsAbleToGetSkill(CharacterData *ch, ESkill skill);
bool IsAbleToGetSkill(CharacterData *ch, ESkill skill, int req_lvl);
int FindWeaponMasterFeat(ESkill skill);
int CalcSkillRemortCap(const CharacterData *ch);
int CalcSkillWisdomCap(const CharacterData *ch);
int CalcSkillHardCap(const CharacterData *ch, ESkill skill);
int CalcSkillMinCap(const CharacterData *ch, ESkill skill);
SkillRollResult MakeSkillTest(CharacterData *ch, ESkill skill_id, CharacterData *vict);
void SendSkillBalanceMsg(CharacterData *ch, const std::string &skill_name, int percent, int prob, bool success);
int CalculateSkillAwakeModifier(CharacterData *killer, CharacterData *victim);

#endif // SKILLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
