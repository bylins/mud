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

#ifndef _SKILLS_H_
#define _SKILLS_H_

#include "structs.h"
#include "pugixml.hpp"

#include <map>

class CHAR_DATA;	// forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

enum ExtraAttackEnumType
{
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
enum ESkill: int
{
	SKILL_INVALID = 0,
	SKILL_THAC0 = 0,	// Internal //
	SKILL_FIRST = 1,
	SKILL_PROTECT = SKILL_FIRST,	// *** Protect groupers    //
	SKILL_TOUCH = 2,	// *** Touch attacker       //
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
	SKILL_BACKSTAB = 131,	// Reserved Skill[] DO NOT CHANGE //
	SKILL_BASH = 132,	// Reserved Skill[] DO NOT CHANGE //
	SKILL_HIDE = 133,	// Reserved Skill[] DO NOT CHANGE //
	SKILL_KICK = 134,	// Reserved Skill[] DO NOT CHANGE //
	SKILL_PICK_LOCK = 135,	// Reserved Skill[] DO NOT CHANGE //
	SKILL_PUNCH = 136,	// Reserved Skill[] DO NOT CHANGE //
	SKILL_RESCUE = 137,	// Reserved Skill[] DO NOT CHANGE //
	SKILL_SNEAK = 138,	// Reserved Skill[] DO NOT CHANGE //
	SKILL_STEAL = 139,	// Reserved Skill[] DO NOT CHANGE //
	SKILL_TRACK = 140,	// Reserved Skill[] DO NOT CHANGE //
	SKILL_CLUBS = 141,	// *** Weapon is club, etc    //
	SKILL_AXES = 142,	// *** Weapon is axe, etc     //
	SKILL_LONGS = 143,	// *** Weapon is long blades  //
	SKILL_SHORTS = 144,	// *** Weapon is short blades //
	SKILL_NONSTANDART = 145,	// *** Weapon is non-standart //
	SKILL_BOTHHANDS = 146,	// *** Weapon in both hands   //
	SKILL_PICK = 147,	// *** Weapon is pick         //
	SKILL_SPADES = 148,	// *** Weapon is spades       //
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

	// не забываем указывать максимальный номер скилла
	MAX_SKILL_NUM = SKILL_MAKE_AMULET
};
inline bool is_magic_skill(int skill) 
{
	if (skill >= SKILL_AIR_MAGIC && skill <= SKILL_LIFE_MAGIC)
		return true;
	else
		return false;
}
template <> ESkill ITEM_BY_NAME<ESkill>(const std::string& name);
template <> const std::string& NAME_BY_ITEM<ESkill>(const ESkill item);

extern std::array<ESkill, MAX_SKILL_NUM - SKILL_PROTECT> AVAILABLE_SKILLS;

int skill_message(int dam, CHAR_DATA * ch, CHAR_DATA * vict, int attacktype, std::string add = "");

int calculate_skill(CHAR_DATA * ch, const ESkill skill_no, CHAR_DATA * vict);
void improove_skill(CHAR_DATA * ch, const ESkill skill_no, int success, CHAR_DATA * victim);

int train_skill(CHAR_DATA * ch, const ESkill skill_no, int max_value, CHAR_DATA * vict);
int min_skill_level(CHAR_DATA *ch, int skill);
int min_skill_level_with_req(CHAR_DATA *ch, int skill, int req_lvl);
bool can_get_skill(CHAR_DATA *ch, int skill);
bool can_get_skill_with_req(CHAR_DATA *ch, int skill, int req_lvl);
int find_weapon_focus_by_skill(ESkill skill);
int find_weapon_master_by_skill(ESkill skill);

// ГОРНОЕ ДЕЛО

#define DIG_DFLT_HOLE_MAX_DEEP		10
#define DIG_DFLT_INSTR_CRASH_CHANCE	2
#define DIG_DFLT_TREASURE_CHANCE	30000
#define DIG_DFLT_PANDORA_CHANCE		80000
#define DIG_DFLT_MOB_CHANCE		300
#define DIG_DFLT_TRASH_CHANCE		100
#define DIG_DFLT_LAG			4
#define DIG_DFLT_PROB_DIVIDE		3
#define DIG_DFLT_GLASS_CHANCE		3
#define DIG_DFLT_NEED_MOVES		15

#define DIG_DFLT_STONE1_SKILL		15
#define DIG_DFLT_STONE2_SKILL		25
#define	DIG_DFLT_STONE3_SKILL		35
#define DIG_DFLT_STONE4_SKILL		50
#define DIG_DFLT_STONE5_SKILL		70
#define	DIG_DFLT_STONE6_SKILL		80
#define	DIG_DFLT_STONE7_SKILL		90
#define	DIG_DFLT_STONE8_SKILL		95
#define	DIG_DFLT_STONE9_SKILL		99

#define DIG_DFLT_STONE1_VNUM		900
#define DIG_DFLT_TRASH_VNUM_START	920
#define DIG_DFLT_TRASH_VNUM_END		922
#define DIG_DFLT_MOB_VNUM_START		100
#define DIG_DFLT_MOB_VNUM_END		103
#define DIG_DFLT_PANDORA_VNUM		919
// предмет с названием 'стекло' для продажи в магазине
const int DIG_GLASS_VNUM = 1919;

struct skillvariables_dig
{
	int hole_max_deep;
	int instr_crash_chance;
	int treasure_chance;
	int pandora_chance;
	int mob_chance;
	int trash_chance;
	int lag;
	int prob_divide;
	int glass_chance;
	int need_moves;

	int stone1_skill;
	int stone2_skill;
	int stone3_skill;
	int stone4_skill;
	int stone5_skill;
	int stone6_skill;
	int stone7_skill;
	int stone8_skill;
	int stone9_skill;

	int stone1_vnum;
	int trash_vnum_start;
	int trash_vnum_end;
	int mob_vnum_start;
	int mob_vnum_end;
	int pandora_vnum;
};

// ЮВЕЛИР

#define INSGEM_DFLT_LAG			4
#define INSGEM_DFLT_MINUS_FOR_AFFECT	15
#define INSGEM_DFLT_PROB_DIVIDE		1
#define INSGEM_DFLT_DIKEY_PERCENT	10
#define INSGEM_DFLT_TIMER_PLUS_PERCENT	10
#define INSGEM_DFLT_TIMER_MINUS_PERCENT	10

struct skillvariables_insgem
{
	int lag;
	int minus_for_affect;
	int prob_divide;
	int dikey_percent;
	int timer_plus_percent;
	int timer_minus_percent;
};

int calculate_awake_mod(CHAR_DATA *killer, CHAR_DATA *victim);

/*
    В перспективе описанный далее класс должен будет содержать
    всю информацию по скиллам и использоваться вместо скилл_инфо
    и прочего.
    Пока что тут только распарс файла и перевод идентификатора
    в номер скилла.
    Это все нужно для совместимости со старой системой.
*/

#define SKILL_UNDEFINED -1
#define SKILL_NAME_UNDEFINED "undefined"
#define SKILLS_FILE "skills.xml"
#define SKILLS_MAIN_TAG "skills"
#define SKILLS_ERROR_STR "...skills.xml reading fail"

class Skill;

typedef std::shared_ptr<Skill> SkillPtr;
typedef std::map<std::string, SkillPtr> SkillListType;

class Skill
{
public:
    Skill();

    static int GetNumByID(const std::string& ID);   // Получение номера скилла по ИД
    static void Load(const pugi::xml_node& XMLSkillList);  // Парсинг конфига скиллов
    static SkillListType SkillList;                 // Глобальный скилллист

    // Доступ к полям
    std::string Name() {return this->_Name;}
    int Number() {return this->_Number;}
    int MaxPercent() {return this->_MaxPercent;}

private:
    std::string _Name;  // Имя скилла на русском
    int _Number;        // Номер скилла
    int _MaxPercent;    // Максимальная процент

    static void ParseSkill(pugi::xml_node SkillNode);   // Парсинг описания одного скилла
};

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
