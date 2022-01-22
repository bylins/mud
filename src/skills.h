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

#include "structs/structs.h"
//#include "utils/pugixml.h"

#include <map>

extern const int kSkillCapOnZeroRemort;
extern const int kSkillCapBonusPerRemort;

class CharacterData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

const int SAVING_WILL = 0;
const int SAVING_CRITICAL = 1;
const int SAVING_STABILITY = 2;
const int SAVING_REFLEX = 3;
const int SAVING_COUNT = 4;
const int SAVING_NONE = 5; //Внимание! Элемента массива с этим номером НЕТ! Исп. в кач-ве заглушки для нефейлящихся спеллов.

enum EExtraAttack {
	EXTRA_ATTACK_UNUSED,
	EXTRA_ATTACK_THROW,
	EXTRA_ATTACK_BASH,
	EXTRA_ATTACK_KICK,
	EXTRA_ATTACK_CHOPOFF,
	EXTRA_ATTACK_DISARM,
	EXTRA_ATTACK_CUT_SHORTS,
	EXTRA_ATTACK_CUT_PICK
};

// PLAYER SKILLS - Numbered from 1 to MAX_SKILL_NUM //
enum ESkill : int {
	SKILL_UNDEF = -1,
	SKILL_INVALID = 0,
	SKILL_GLOBAL_COOLDOWN = 0,    // Internal - ID for global ability cooldown //
	SKILL_FIRST = 1,
	SKILL_PROTECT = SKILL_FIRST,    // *** Protect groupers    //
	SKILL_TOUCH = 2,    // *** Touch attacker       //
	SKILL_SHIT = 3,
	SKILL_MIGHTHIT = 4,
	SKILL_STUPOR = 5,
	SKILL_POISONED = 6,
	SKILL_SENSE = 7,
	SKILL_HORSE = 8,
	SKILL_HIDETRACK = 9,
	SKILL_RELIGION = 10,
	SKILL_MAKEFOOD = 11,
	SKILL_MULTYPARRY = 12,
	SKILL_TRANSFORMWEAPON = 13,
	SKILL_LEADERSHIP = 20,
	SKILL_PUNCTUAL = 21,
	SKILL_AWAKE = 22,
	SKILL_IDENTIFY = 23,
	SKILL_HEARING = 24,
	SKILL_CREATE_POTION = 25,
	SKILL_CREATE_SCROLL = 26,
	SKILL_CREATE_WAND = 27,
	SKILL_LOOK_HIDE = 28,
	SKILL_ARMORED = 29,
	SKILL_DRUNKOFF = 30,
	SKILL_AID = 31,
	SKILL_FIRE = 32,
	SKILL_CREATEBOW = 33,
	SKILL_THROW = 130,
	SKILL_BACKSTAB = 131,    // Reserved Skill[] DO NOT CHANGE //
	SKILL_BASH = 132,    // Reserved Skill[] DO NOT CHANGE //
	SKILL_HIDE = 133,    // Reserved Skill[] DO NOT CHANGE //
	SKILL_KICK = 134,    // Reserved Skill[] DO NOT CHANGE //
	SKILL_PICK_LOCK = 135,    // Reserved Skill[] DO NOT CHANGE //
	SKILL_PUNCH = 136,    // Reserved Skill[] DO NOT CHANGE //
	SKILL_RESCUE = 137,    // Reserved Skill[] DO NOT CHANGE //
	SKILL_SNEAK = 138,    // Reserved Skill[] DO NOT CHANGE //
	SKILL_STEAL = 139,    // Reserved Skill[] DO NOT CHANGE //
	SKILL_TRACK = 140,    // Reserved Skill[] DO NOT CHANGE //
	SKILL_CLUBS = 141,    // *** Weapon is club, etc    //
	SKILL_AXES = 142,    // *** Weapon is axe, etc     //
	SKILL_LONGS = 143,    // *** Weapon is long blades  //
	SKILL_SHORTS = 144,    // *** Weapon is short blades //
	SKILL_NONSTANDART = 145,    // *** Weapon is non-standart //
	SKILL_BOTHHANDS = 146,    // *** Weapon in both hands   //
	SKILL_PICK = 147,    // *** Weapon is pick         //
	SKILL_SPADES = 148,    // *** Weapon is spades       //
	SKILL_SATTACK = 149,
	SKILL_DISARM = 150,
	SKILL_PARRY = 151,
	SKILL_HEAL = 152,
	SKILL_MORPH = 153,
	SKILL_BOWS = 154,
	SKILL_ADDSHOT = 155,
	SKILL_CAMOUFLAGE = 156,
	SKILL_DEVIATE = 157,
	SKILL_BLOCK = 158,
	SKILL_LOOKING = 159,
	SKILL_CHOPOFF = 160,
	SKILL_REPAIR = 161,
	SKILL_UPGRADE = 164,
	SKILL_COURAGE = 165,
	SKILL_MANADRAIN = 166,
	SKILL_NOPARRYHIT = 167,
	SKILL_TOWNPORTAL = 168,
	SKILL_MAKE_STAFF = 169,  //смастерить предмет
	SKILL_MAKE_BOW = 170,
	SKILL_MAKE_WEAPON = 171,
	SKILL_MAKE_ARMOR = 172,
	SKILL_MAKE_JEWEL = 173,
	SKILL_MAKE_WEAR = 174,
	SKILL_MAKE_POTION = 175,
	SKILL_DIG = 176,
	SKILL_INSERTGEM = 177,
	SKILL_WARCRY = 178,
	SKILL_TURN_UNDEAD = 179,
	SKILL_IRON_WIND = 180,
	SKILL_STRANGLE = 181,
	SKILL_AIR_MAGIC = 182,
	SKILL_FIRE_MAGIC = 183,
	SKILL_WATER_MAGIC = 184,
	SKILL_EARTH_MAGIC = 185,
	SKILL_LIGHT_MAGIC = 186,
	SKILL_DARK_MAGIC = 187,
	SKILL_MIND_MAGIC = 188,
	SKILL_LIFE_MAGIC = 189,
	SKILL_STUN = 190,
	SKILL_MAKE_AMULET = 191,
	SKILL_INDEFINITE = 192,    // Reserved! Нужен, чтобы указывать "произвольный" скилл в некоторых случаях //

	// не забываем указывать максимальный номер скилла
	MAX_SKILL_NUM = SKILL_INDEFINITE
};

const int kKnowSkill = 1;

struct SkillRollResult {
	bool success = true;
	bool critical = false;
	short degree = 0;
};

template<>
ESkill ITEM_BY_NAME<ESkill>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<ESkill>(const ESkill item);

extern std::array<ESkill, MAX_SKILL_NUM - SKILL_PROTECT> AVAILABLE_SKILLS;

int SendSkillMessages(int dam, CharacterData *ch, CharacterData *vict, int attacktype, std::string add = "");

int CalcCurrentSkill(CharacterData *ch, ESkill skill, CharacterData *vict);
void ImproveSkill(CharacterData *ch, ESkill skill, int success, CharacterData *victim);
void TrainSkill(CharacterData *ch, ESkill skill, bool success, CharacterData *vict);

int min_skill_level(CharacterData *ch, int skill);
int min_skill_level_with_req(CharacterData *ch, int skill, int req_lvl);
bool IsAbleToGetSkill(CharacterData *ch, int skill);
bool can_get_skill_with_req(CharacterData *ch, int skill, int req_lvl);
int FindWeaponMasterBySkill(ESkill skill);
int CalcSkillRemortCap(const CharacterData *ch);
int CalcSkillSoftCap(const CharacterData *ch);
int CalcSkillHardCap(const CharacterData *ch, ESkill skill);
int CalcSkillMinCap(const CharacterData *ch, ESkill skill);
SkillRollResult MakeSkillTest(CharacterData *ch, ESkill skill_id, CharacterData *vict);
void SendSkillBalanceMsg(CharacterData *ch, const char *skill_name, int percent, int prob, bool success);
int CalculateSkillAwakeModifier(CharacterData *killer, CharacterData *victim);

#endif // SKILLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
