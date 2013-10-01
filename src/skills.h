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

#include <map>
#include <boost/shared_ptr.hpp>
#include "pugixml.hpp"

// PLAYER SKILLS - Numbered from 1 to MAX_SKILL_NUM //
#define SKILL_THAC0                 0	// Internal //
#define SKILL_PROTECT               1 // *** Protect grouppers    //
#define SKILL_TOUCH                 2 // *** Touch attacker       //
#define SKILL_SHIT                  3
#define SKILL_MIGHTHIT              4
#define SKILL_STUPOR                5
#define SKILL_POISONED              6
#define SKILL_SENSE                 7
#define SKILL_HORSE                 8
#define SKILL_HIDETRACK             9
#define SKILL_RELIGION              10
#define SKILL_MAKEFOOD              11
#define SKILL_MULTYPARRY            12
#define SKILL_TRANSFORMWEAPON       13
//и щито?
#define SKILL_LEADERSHIP            20
#define SKILL_PUNCTUAL              21
#define SKILL_AWAKE                 22
#define SKILL_IDENTIFY              23
#define SKILL_HEARING               24
#define SKILL_CREATE_POTION         25
#define SKILL_CREATE_SCROLL         26
#define SKILL_CREATE_WAND           27
#define SKILL_LOOK_HIDE             28
#define SKILL_ARMORED               29
#define SKILL_DRUNKOFF              30
#define SKILL_AID                   31
#define SKILL_FIRE                  32
#define SKILL_CREATEBOW             33
// и че тут?
#define SKILL_THROW                 130
#define SKILL_BACKSTAB              131	// Reserved Skill[] DO NOT CHANGE //
#define SKILL_BASH                  132	// Reserved Skill[] DO NOT CHANGE //
#define SKILL_HIDE                  133	// Reserved Skill[] DO NOT CHANGE //
#define SKILL_KICK                  134	// Reserved Skill[] DO NOT CHANGE //
#define SKILL_PICK_LOCK             135	// Reserved Skill[] DO NOT CHANGE //
#define SKILL_PUNCH                 136	// Reserved Skill[] DO NOT CHANGE //
#define SKILL_RESCUE                137	// Reserved Skill[] DO NOT CHANGE //
#define SKILL_SNEAK                 138	// Reserved Skill[] DO NOT CHANGE //
#define SKILL_STEAL                 139	// Reserved Skill[] DO NOT CHANGE //
#define SKILL_TRACK                 140	// Reserved Skill[] DO NOT CHANGE //
#define SKILL_CLUBS                 141	// *** Weapon is club, etc    //
#define SKILL_AXES                  142	// *** Weapon is axe, etc     //
#define SKILL_LONGS                 143	// *** Weapon is long blades  //
#define SKILL_SHORTS                144	// *** Weapon is short blades //
#define SKILL_NONSTANDART           145	// *** Weapon is non-standart //
#define SKILL_BOTHHANDS             146	// *** Weapon in both hands   //
#define SKILL_PICK                  147	// *** Weapon is pick         //
#define SKILL_SPADES                148	// *** Weapon is spades       //
#define SKILL_SATTACK               149
#define SKILL_DISARM                150
#define SKILL_PARRY                 151
#define SKILL_HEAL                  152
#define SKILL_MORPH		            153
#define SKILL_BOWS                  154
#define SKILL_ADDSHOT               155
#define SKILL_CAMOUFLAGE            156
#define SKILL_DEVIATE               157
#define SKILL_BLOCK                 158
#define SKILL_LOOKING               159
#define SKILL_CHOPOFF               160
#define SKILL_REPAIR                161
#define SKILL_UPGRADE               164
#define SKILL_COURAGE               165
#define SKILL_MANADRAIN             166
#define SKILL_NOPARRYHIT            167
#define SKILL_TOWNPORTAL            168
// Crafting skills
#define SKILL_MAKE_STAFF            169
#define SKILL_MAKE_BOW              170
#define SKILL_MAKE_WEAPON           171
#define SKILL_MAKE_ARMOR            172
#define SKILL_MAKE_JEWEL            173
#define SKILL_MAKE_WEAR             174
#define SKILL_MAKE_POTION           175
#define SKILL_DIG                   176
#define SKILL_INSERTGEM             177
// Other skills
#define SKILL_WARCRY                178
#define SKILL_TURN_UNDEAD           179
#define SKILL_IRON_WIND             180
#define SKILL_STRANGLE              181
// не забываем указывать максимальный номер скилла
#define MAX_SKILL_NUM               181

int skill_message(int dam, CHAR_DATA * ch, CHAR_DATA * vict, int attacktype, std::string add = "");

int calculate_skill(CHAR_DATA * ch, int skill_no, int max_value, CHAR_DATA * vict);
void improove_skill(CHAR_DATA * ch, int skill_no, int success, CHAR_DATA * victim);

int train_skill(CHAR_DATA * ch, int skill_no, int max_value, CHAR_DATA * vict);
int min_skill_level(CHAR_DATA *ch, int skill);
bool can_get_skill(CHAR_DATA *ch, int skill);

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

typedef boost::shared_ptr<Skill> SkillPtr;
typedef std::map<std::string, SkillPtr> SkillListType;

class Skill
{
public:
    Skill();

    static int GetNumByID(std::string ID);   // Получение номера скилла по ИД
    static void Load(pugi::xml_node XMLSkillList);  // Парсинг конфига скиллов
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
