/* ************************************************************************
*   File: spell_parser.cpp                              Part of Bylins    *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "spell_parser.hpp"

#include "skills/stun.h"

#include "object.prototypes.hpp"
#include "obj.hpp"
#include "spells.h"
#include "skills.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "dg_script/dg_scripts.h"
#include "fightsystem/pk.h"
#include "features.hpp"
#include "chars/char.hpp"
#include "name_list.hpp"
#include "depot.hpp"
#include "parcel.hpp"
#include "room.hpp"
#include "magic.h"
#include "fightsystem/fight.h"
#include "chars/world.characters.hpp"
#include "logger.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "skills.info.h"
#include "spells.info.h"

#include <vector>

extern struct spell_create_type spell_create[];
char cast_argument[MAX_STRING_LENGTH];

#define SpINFO spell_info[spellnum]
#define SkINFO skill_info[skillnum]
// Во сколько раз манакост манакастеров меньше манакоста НЕманакастеров
#define DRUID_MANA_COST_MODIFIER 0.5

extern int what_sky;

// local functions
void say_spell(CHAR_DATA * ch, int spellnum, CHAR_DATA * tch, OBJ_DATA * tobj);
int get_zone_rooms(int, int *, int *);

int spell_create_level(const CHAR_DATA* ch, int spellnum)
{
	int required_level = spell_create[spellnum].runes.min_caster_level;
	// дабы у простых игроков не отображлись иммовские заклы
	if (required_level >= LVL_GOD)
		return required_level;
	if (can_use_feat(ch, RUNE_ULTIMATE_FEAT))
		required_level -= 6;
	else if (can_use_feat(ch, RUNE_MASTER_FEAT))
		required_level -= 4;
	else if (can_use_feat(ch, RUNE_USER_FEAT))
		required_level -= 2;
	else if (can_use_feat(ch, RUNE_NEWBIE_FEAT))
		required_level -= 1;
	return required_level;
}

//	Коэффициент изменения мема относительно скилла магии.
int koef_skill_magic(int percent_skill)
{
//	Выделяем процент на который меняется мем
	return ((800 - percent_skill) / 8);

//	return 0;
}

int mag_manacost(const CHAR_DATA* ch, int spellnum)
{
	int mana_cost;

	if (IS_IMMORTAL(ch))
	{
		return 1;
	}

//	Мем рунных профессий(на сегодня только волхвы)
	if (IS_MANA_CASTER(ch) && GET_LEVEL(ch) >= spell_create_level(ch, spellnum))
	{
		const auto result = static_cast<int>(DRUID_MANA_COST_MODIFIER
			* (float) mana_gain_cs[VPOSI(55 - GET_REAL_INT(ch), 10, 50)]
			/ (float) int_app[VPOSI(55 - GET_REAL_INT(ch), 10, 50)].mana_per_tic
			* 60
			* MAX(SpINFO.mana_max
				- (SpINFO.mana_change
					* (GET_LEVEL(ch)
						- spell_create[spellnum].runes.min_caster_level)),
				SpINFO.mana_min));

		//Зависимости в таблице несколько корявые, поэтому изменение пустим в обратную сторону
		return result;
	};

//	Мем всех остальных
	if (!IS_MANA_CASTER(ch)
		&& GET_LEVEL(ch) >= MIN_CAST_LEV(SpINFO, ch)
		&& GET_REMORT(ch) >= MIN_CAST_REM(SpINFO, ch))
	{
		mana_cost = MAX(SpINFO.mana_max
			- (SpINFO.mana_change
				* (GET_LEVEL(ch) - MIN_CAST_LEV(SpINFO, ch))),
			SpINFO.mana_min);

		if (SpINFO.class_change[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] < 0)
		{
			mana_cost = mana_cost * (100 - MIN(99, abs(SpINFO.class_change[(int)GET_CLASS(ch)][(int)GET_KIN(ch)]))) / 100;
		}
		else
		{
			mana_cost = mana_cost * 100 / (100 - MIN(99, abs(SpINFO.class_change[(int)GET_CLASS(ch)][(int)GET_KIN(ch)])));
		}

//		Меняем мем на коэффициент скилла магии
		if ((GET_CLASS(ch) == CLASS_DRUID)
			|| (GET_CLASS(ch) == CLASS_PALADINE)
			|| (GET_CLASS(ch) == CLASS_MERCHANT))
		{
			return mana_cost;
		}
		return mana_cost * koef_skill_magic(ch->get_skill(get_magic_skill_number_by_spell(spellnum))) / 100; // при скилле 200 + 25%
	};

	return 99999;
}

// say_spell erodes buf, buf1, buf2
void say_spell(CHAR_DATA * ch, int spellnum, CHAR_DATA * tch, OBJ_DATA * tobj)
{
	char lbuf[256];
	const char *say_to_self, *say_to_other, *say_to_obj_vis, *say_to_something,
	*helpee_vict, *damagee_vict, *format;
	int  religion;


	*buf = '\0';
	strcpy(lbuf, SpINFO.syn);
	// Say phrase ?
	if (cast_phrase[spellnum][GET_RELIGION(ch)] == nullptr){
		sprintf(buf, "[ERROR]: say_spell: для спелла %d не объявлена cast_phrase", spellnum);
		mudlog(buf, CMP, LVL_GOD, SYSLOG, TRUE);
		return;
	}
	if (IS_NPC(ch)) {
		switch (GET_RACE(ch)) {
		case NPC_RACE_EVIL_SPIRIT:
		case NPC_RACE_GHOST:
		case NPC_RACE_HUMAN:
		case NPC_RACE_ZOMBIE:
		case NPC_RACE_SPIRIT:
			religion = number(RELIGION_POLY, RELIGION_MONO);
			if (*cast_phrase[spellnum][religion] != '\n')
				strcpy(buf, cast_phrase[spellnum][religion]);
			say_to_self = "$n пробормотал$g : '%s'.";
			say_to_other = "$n взглянул$g на $N3 и бросил$g : '%s'.";
			say_to_obj_vis = "$n глянул$g на $o3 и произнес$q : '%s'.";
			say_to_something = "$n произнес$q : '%s'.";
			damagee_vict = "$n зыркнул$g на вас и проревел$g : '%s'.";
			helpee_vict = "$n улыбнул$u вам и произнес$q : '%s'.";
			break;
		default:
			say_to_self = "$n издал$g непонятный звук.";
			say_to_other = "$n издал$g непонятный звук.";
			say_to_obj_vis = "$n издал$g непонятный звук.";
			say_to_something = "$n издал$g непонятный звук.";
			damagee_vict = "$n издал$g непонятный звук.";
			helpee_vict = "$n издал$g непонятный звук.";
		}
	}
	else {
		//если включен режим без повторов (подавление ехо) не показываем
		if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
			if (!ch->get_fighting()) //если персонаж не в бою, шлем строчку, если в бою ничего не шлем
				send_to_char(OK, ch);
		}
		else {
			if (IS_SET(SpINFO.routines, MAG_WARCRY))
				sprintf(buf, "Вы выкрикнули \"%s%s%s\".\r\n",
					SpINFO.violent ? CCIRED(ch, C_NRM) : CCIGRN(ch, C_NRM), SpINFO.name, CCNRM(ch, C_NRM));
			else
				sprintf(buf, "Вы произнесли заклинание \"%s%s%s\".\r\n",
					SpINFO.violent ? CCIRED(ch, C_NRM) : CCIGRN(ch, C_NRM), SpINFO.name, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		if (*cast_phrase[spellnum][GET_RELIGION(ch)] != '\n')
			strcpy(buf, cast_phrase[spellnum][GET_RELIGION(ch)]);
		say_to_self = "$n прикрыл$g глаза и прошептал$g : '%s'.";
		say_to_other = "$n взглянул$g на $N3 и произнес$q : '%s'.";
		say_to_obj_vis = "$n посмотрел$g на $o3 и произнес$q : '%s'.";
		say_to_something = "$n произнес$q : '%s'.";
		damagee_vict = "$n зыркнул$g на вас и произнес$q : '%s'.";
		helpee_vict = "$n подмигнул$g вам и произнес$q : '%s'.";
	}

	if (tch != NULL && IN_ROOM(tch) == ch->in_room)
	{
		if (tch == ch)
		{
			format = say_to_self;
		}
		else
		{
			format = say_to_other;
		}
	}
	else if (tobj != NULL && (tobj->get_in_room() == ch->in_room || tobj->get_carried_by() == ch))
	{
		format = say_to_obj_vis;
	}
	else
	{
		format = say_to_something;
	}

	sprintf(buf1, format, spell_name(spellnum));
	sprintf(buf2, format, buf);

	for (const auto i : world[ch->in_room]->people)
	{
		if (i == ch
			|| i == tch
			|| !i->desc
			|| !AWAKE(i)
			|| AFF_FLAGGED(i, EAffectFlag::AFF_DEAFNESS))
		{
			continue;
		}

		if (IS_SET(GET_SPELL_TYPE(i, spellnum), SPELL_KNOW | SPELL_TEMP))
		{
			perform_act(buf1, ch, tobj, tch, i);
		}
		else
		{
			perform_act(buf2, ch, tobj, tch, i);
		}
	}

	act(buf1, 1, ch, tobj, tch, TO_ARENA_LISTEN);

	if (tch != NULL
		&& tch != ch
		&& IN_ROOM(tch) == ch->in_room
		&& !AFF_FLAGGED(tch, EAffectFlag::AFF_DEAFNESS))
	{
		if (SpINFO.violent)
		{
			sprintf(buf1, damagee_vict,
				IS_SET(GET_SPELL_TYPE(tch, spellnum), SPELL_KNOW | SPELL_TEMP) ? spell_name(spellnum) : buf);
		}
		else
		{
			sprintf(buf1, helpee_vict,
				IS_SET(GET_SPELL_TYPE(tch, spellnum), SPELL_KNOW | SPELL_TEMP) ? spell_name(spellnum) : buf);
		}
		act(buf1, FALSE, ch, NULL, tch, TO_VICT);
	}
}

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE.
 */

const char *skill_name(int num)
{
	if (num > 0 && num <= MAX_SPELLS)
		return (skill_info[num].name);
	else if (num == -1)
		return "UNUSED";
	else
		return "UNDEFINED";
}

const char *spell_name(int num)
{
	if (num > 0 && num <= TOP_SPELL_DEFINE)
		return (spell_info[num].name);
	else if (num == -1)
		return "UNUSED";
	else
		return "UNDEFINED";
}

template <typename T>
void fix_name(T& name)
{
	size_t pos = 0;
	while ('\0' != name[pos] && pos < MAX_STRING_LENGTH)
	{
		if (('.' == name[pos]) || ('_' == name[pos]))
		{
			name[pos] = ' ';
		}
		++pos;
	}
}

void fix_name_feat(char *name)
{
	fix_name(name);
}

ESkill find_skill_num(const char *name)
{
	int ok;
	char const *temp, *temp2;
	char first[256], first2[256];

	for (const auto index : AVAILABLE_SKILLS)
	{
		if (is_abbrev(name, skill_info[index].name))
		{
			return (index);
		}

		ok = TRUE;
		// It won't be changed, but other uses of this function elsewhere may.
		temp = any_one_arg(skill_info[index].name, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok)
		{
			if (!is_abbrev(first2, first))
			{
				ok = FALSE;
			}
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}

		if (ok && !*first2)
		{
			return (index);
		}
	}

	return SKILL_INVALID;
}

int find_spell_num(const char *name)
{
	int index, ok;
	int use_syn = 0;
	char const *temp, *temp2, *realname;
	char first[256], first2[256];

	use_syn = (((ubyte) * name <= (ubyte) 'z')
			   && ((ubyte) * name >= (ubyte) 'a'))
			  || (((ubyte) * name <= (ubyte) 'Z') && ((ubyte) * name >= (ubyte) 'A'));

	for (index = 1; index <= TOP_SPELL_DEFINE; index++)
	{
		realname = (use_syn) ? spell_info[index].syn : spell_info[index].name;

		if (!realname || !*realname)
			continue;
		if (is_abbrev(name, realname))
			return (index);
		ok = TRUE;
		// It won't be changed, but other uses of this function elsewhere may.
		temp = any_one_arg(realname, first);
		temp2 = any_one_arg(name, first2);
		while (*first && *first2 && ok)
		{
			if (!is_abbrev(first2, first))
				ok = FALSE;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}
		if (ok && !*first2)
			return (index);

	}
	return (-1);
}

ESkill fix_name_and_find_skill_num(char *name)
{
	fix_name(name);
	return find_skill_num(name);
}

ESkill fix_name_and_find_skill_num(std::string& name)
{
	fix_name(name);
	return find_skill_num(name.c_str());
}

int fix_name_and_find_spell_num(char *name)
{
	fix_name(name);
	return find_spell_num(name);
}

int fix_name_and_find_spell_num(std::string& name)
{
	fix_name(name);
	return find_spell_num(name.c_str());
}

int may_cast_in_nomagic(CHAR_DATA * caster, CHAR_DATA* /*victim*/, int spellnum)
{
	// More than 33 level - may cast always
	if (IS_GRGOD(caster) || IS_SET(SpINFO.routines, MAG_WARCRY))
		return TRUE;
	if (IS_NPC(caster) && !(AFF_FLAGGED(caster, EAffectFlag::AFF_CHARM) || MOB_FLAGGED(caster, MOB_ANGEL)))
		return TRUE;

	return FALSE;
}


// может ли кастер зачитать заклинание?
int may_cast_here(CHAR_DATA * caster, CHAR_DATA * victim, int spellnum)
{
	int ignore;

	//  More than 33 level - may cast always
	if (IS_GRGOD(caster))
	{
		return TRUE;
	}

	if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOBATTLE) && SpINFO.violent)
	{
		return FALSE;
	}

	// не в мирке можно кастовать все что угодно
	if (!ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL))
	{
		return TRUE;
	}

	// Проверяю, что закл имеет одну из допустимых комбинаций параметров
	ignore = IS_SET(SpINFO.targets, TAR_IGNORE)
		|| IS_SET(SpINFO.routines, MAG_MASSES)
		|| IS_SET(SpINFO.routines, MAG_GROUPS);

	// цели нет
	if (!ignore && !victim)
	{
		return TRUE;
	}

	if (ignore && !IS_SET(SpINFO.routines, MAG_MASSES) && !IS_SET(SpINFO.routines, MAG_GROUPS))
	{
		if (SpINFO.violent)
		{
			return FALSE;	// нельзя злые кастовать
		}
		// если игнорируется цель, то должен быть GROUP или MASS
		// в противном случае на цель кастовать нельзя
		return victim == 0 ? TRUE : FALSE;
	}
	// остальные комбинации не проверяю

	// индивидуальная цель
	ignore = victim
		&& (IS_SET(SpINFO.targets, TAR_CHAR_ROOM)
			|| IS_SET(SpINFO.targets, TAR_CHAR_WORLD))
		&& !IS_SET(SpINFO.routines, MAG_AREAS);

	// начинаю проверять условия каста
	// Для добрых заклинаний - проверка на противника цели
	// Для злых заклинаний - проверка на цель
	for (const auto ch_vict : world[caster->in_room]->people)
	{
		if (IS_IMMORTAL(ch_vict))
			continue;	// имморталы на этот процесс не влияют
		if (!HERE(ch_vict))
			continue;	// лд не участвуют
		if (SpINFO.violent && same_group(caster, ch_vict))
			continue;	// на согрупника кастовться не будет
		if (ignore && ch_vict != victim)
			continue;	// только по 1 чару, и не текущему
		// ch_vict может стать потунциальной жертвой
		if (SpINFO.violent)
		{
			if (!may_kill_here(caster, ch_vict, NoArgument))
			{
				return 0;
			}
		}
		else
		{
			if (!may_kill_here(caster, ch_vict->get_fighting(), NoArgument))
			{
				return 0;
			}
		}
	}

	// check_pkill вызвать не могу, т.к. имя жертвы безвозвратно утеряно

	// Ура! Можно
	return (TRUE);
}

/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int call_magic(CHAR_DATA * caster, CHAR_DATA * cvict, OBJ_DATA * ovict, ROOM_DATA *rvict, int spellnum, int level) {

	if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
		return (0);

	if (caster && cvict) {
		cast_mtrigger(cvict, caster, spellnum);
	}

	if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC) && !may_cast_in_nomagic(caster, cvict, spellnum)) {
		send_to_char("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", caster);
		act("Магия $n1 потерпела неудачу и развеялась по воздуху.", FALSE, caster, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		return 0;
	}

	if (!may_cast_here(caster, cvict, spellnum)) {
		if (IS_SET(SpINFO.routines, MAG_WARCRY)) {
			send_to_char("Ваш громовой глас сотряс воздух, но ничего не произошло!\r\n", caster);
			act("Вы вздрогнули от неожиданного крика, но ничего не произошло.", FALSE, caster, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		} else {
			send_to_char("Ваша магия обратилась всего лишь в яркую вспышку!\r\n", caster);
			act("Яркая вспышка на миг осветила комнату, и тут же погасла.", FALSE, caster, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		}
		return 0;
	}

	if (SpellUsage::isActive)
		SpellUsage::AddSpellStat(GET_CLASS(caster), spellnum);

	if (IS_SET(SpINFO.routines, MAG_AREAS) || IS_SET(SpINFO.routines, MAG_MASSES))
		return callMagicToArea(caster, cvict, rvict, spellnum, level);

	if (IS_SET(SpINFO.routines, MAG_GROUPS))
		return callMagicToGroup(level, caster, spellnum);

	if (IS_SET(SpINFO.routines, MAG_ROOM))
		return RoomSpells::imposeSpellToRoom(level, caster, rvict, spellnum);

	return mag_single_target(level, caster, cvict, ovict, spellnum, SAVING_STABILITY);
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff  - [0] level   [1] max charges [2] num charges [3] spell num
 * wand   - [0] level   [1] max charges [2] num charges [3] spell num
 * scroll - [0] level   [1] spell num   [2] spell num   [3] spell num
 * potion - [0] level   [1] spell num   [2] spell num   [3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */

const char *what_sky_type[] = { "пасмурно",
								"cloudless",
								"облачно",
								"cloudy",
								"дождь",
								"raining",
								"ясно",
								"lightning",
								"\n"
							  };

const char *what_weapon[] = { "плеть",
							  "whip",
							  "дубина",
							  "club",
							  "\n"
							};

/**
* Поиск предмета для каста локейта (без учета видимости для чара и с поиском
* как в основном списке, так и в личных хранилищах с почтой).
*/
OBJ_DATA *find_obj_for_locate(CHAR_DATA *ch, const char *name)
{
//	OBJ_DATA *obj = ObjectAlias::locate_object(name);
	OBJ_DATA *obj = get_obj_vis_for_locate(ch, name);
	if (!obj)
	{
		obj = Depot::locate_object(name);
		if (!obj)
		{
			obj = Parcel::locate_object(name);
		}
	}
	return obj;
}

int find_cast_target(int spellnum, const char *t, CHAR_DATA * ch, CHAR_DATA ** tch, OBJ_DATA ** tobj, ROOM_DATA ** troom)
{
	*tch = NULL;
	*tobj = NULL;
	*troom = world[ch->in_room];
	if (spellnum == SPELL_CONTROL_WEATHER)
	{
		if ((what_sky = search_block(t, what_sky_type, FALSE)) < 0)
		{
			send_to_char("Не указан тип погоды.\r\n", ch);
			return FALSE;
		}
		else
			what_sky >>= 1;
	}

	if (spellnum == SPELL_CREATE_WEAPON)
	{
		if ((what_sky = search_block(t, what_weapon, FALSE)) < 0)
		{
			send_to_char("Не указан тип оружия.\r\n", ch);
			return FALSE;
		}
		else
			what_sky = 5 + (what_sky >> 1);
	}

	strcpy(cast_argument, t);

	if (IS_SET(SpINFO.targets, TAR_ROOM_THIS))
		return TRUE;
	if (IS_SET(SpINFO.targets, TAR_IGNORE))
		return TRUE;
	else if (*t)
	{
		if (IS_SET(SpINFO.targets, TAR_CHAR_ROOM))
		{
			if ((*tch = get_char_vis(ch, t, FIND_CHAR_ROOM)) != NULL)
			{
				if (SpINFO.violent && !check_pkill(ch, *tch, t))
					return FALSE;
				return TRUE;
			}
		}

		if (IS_SET(SpINFO.targets, TAR_CHAR_WORLD)) {
			if ((*tch = get_char_vis(ch, t, FIND_CHAR_WORLD)) != NULL) {
				// чтобы мобов не чекали
				if (IS_NPC(ch) || !IS_NPC(*tch)) {
					if (SpINFO.violent && !check_pkill(ch, *tch, t)) {
						return FALSE;
					}
					return TRUE;
				}
				if (!IS_NPC(ch)) {
					struct follow_type *k, *k_next;
					int tnum = 1, fnum = 0; // ищем одноимённые цели
					char tmpname[MAX_INPUT_LENGTH];
					char *tmp = tmpname;
					strcpy(tmp, t);
					tnum = get_number(&tmp); // возвращает 1, если первая цель
					for (k = ch->followers; k; k = k_next) {
						k_next = k->next;
						if (isname(tmp, k->follower->get_pc_name())) {
							if (++fnum == tnum) {// нашли!!
								*tch = k->follower;
								return TRUE;
							}
						}
					}
				}
			}
		}

		if (IS_SET(SpINFO.targets, TAR_OBJ_INV))
			if ((*tobj = get_obj_in_list_vis(ch, t, ch->carrying)) != NULL)
				return TRUE;

		if (IS_SET(SpINFO.targets, TAR_OBJ_EQUIP))
		{
			int tmp;
			if ((*tobj = get_object_in_equip_vis(ch, t, ch->equipment, &tmp)) != NULL)
				return TRUE;
		}

		if (IS_SET(SpINFO.targets, TAR_OBJ_ROOM))
			if ((*tobj = get_obj_in_list_vis(ch, t, world[ch->in_room]->contents)) != NULL)
				return TRUE;

		if (IS_SET(SpINFO.targets, TAR_OBJ_WORLD))
		{
//			if ((*tobj = get_obj_vis(ch, t)) != NULL)
//				return TRUE;
			if (spellnum == SPELL_LOCATE_OBJECT)
			{
				*tobj = find_obj_for_locate(ch, t);
			}
			else
			{
				*tobj = get_obj_vis(ch, t);
			}
			if (*tobj)
			{
				return true;
			}
		}
	}
	else
	{
		if (IS_SET(SpINFO.targets, TAR_FIGHT_SELF))
			if (ch->get_fighting() != NULL)
			{
				*tch = ch;
				return TRUE;
			}
		if (IS_SET(SpINFO.targets, TAR_FIGHT_VICT))
			if (ch->get_fighting() != NULL)
			{
				*tch = ch->get_fighting();
				return TRUE;
			}
		if (IS_SET(SpINFO.targets, TAR_CHAR_ROOM) && !SpINFO.violent)
		{
			*tch = ch;
			return TRUE;
		}
	}
	// TODO: добавить обработку TAR_ROOM_DIR и TAR_ROOM_WORLD
	if (IS_SET(SpINFO.routines, MAG_WARCRY))
		sprintf(buf, "И на %s же вы хотите так громко крикнуть?\r\n",
				IS_SET(SpINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)
				? "ЧТО" : "КОГО");
	else
		sprintf(buf, "На %s Вы хотите ЭТО колдовать?\r\n",
				IS_SET(SpINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP)
				? "ЧТО" : "КОГО");
	send_to_char(buf, ch);
	return FALSE;
}

void mag_objectmagic(CHAR_DATA * ch, OBJ_DATA * obj, const char *argument)
{
	int i, spellnum;
	int level;
	CHAR_DATA *tch = nullptr;
	OBJ_DATA *tobj = nullptr;
	ROOM_DATA *troom = nullptr;

	one_argument(argument, cast_argument);
	level = GET_OBJ_VAL(obj, 0);
	if (level == 0)
	{
		if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_STAFF)
		{
			level = DEFAULT_STAFF_LVL;
		}
		else if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WAND)
		{
			level = DEFAULT_WAND_LVL;
		}
	}

	if (obj->get_extra_flag(EExtraFlag::ITEM_TIMEDLVL))
	{
		int proto_timer = obj_proto[GET_OBJ_RNUM(obj)]->get_timer();
		if (proto_timer != 0)
		{
			level -= level * (proto_timer - obj->get_timer()) / proto_timer;
		}
	}

	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_STAFF:
		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), FALSE, ch, obj, 0, TO_CHAR);
			act(obj->get_action_description().c_str(), FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			act("Вы ударили $o4 о землю.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n ударил$g $o4 о землю.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		}

		if (GET_OBJ_VAL(obj, 2) <= 0)
		{
			send_to_char("Похоже, кончились заряды :)\r\n", ch);
			act("И ничего не случилось.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			obj->dec_val(2);
			WAIT_STATE(ch, PULSE_VIOLENCE);
			if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, 3), MAG_MASSES | MAG_AREAS))
			{
				call_magic(ch, nullptr, nullptr, world[ch->in_room], GET_OBJ_VAL(obj, 3), level);
			}
			else
			{
				const auto people_copy = world[ch->in_room]->people;
				for (const auto tch : people_copy)
				{
					if (ch != tch)
					{
						call_magic(ch, tch, nullptr, world[ch->in_room], GET_OBJ_VAL(obj, 3), level);
					}
				}
			}
		}

		break;

	case OBJ_DATA::ITEM_WAND:
		spellnum = GET_OBJ_VAL(obj, 3);

		if (GET_OBJ_VAL(obj, 2) <= 0) {
			send_to_char("Похоже, магия кончилась.\r\n", ch);
			return;
		}

		if (!*argument) {
			if (!IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, MAG_AREAS | MAG_MASSES)) {
				tch = ch;
			}
		} else {
			if (!find_cast_target(spellnum, argument, ch, &tch, &tobj, &troom)) {
				return;
			}
		}

		if (tch) {
			if (tch == ch) {
				if (!obj->get_action_description().empty()) {
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_CHAR);
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_ROOM | TO_ARENA_LISTEN);
				} else {
					act("Вы указали $o4 на себя.", FALSE, ch, obj, 0, TO_CHAR);
					act("$n указал$g $o4 на себя.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
				}
			} else {
				if (!obj->get_action_description().empty()) {
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_CHAR);
					act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_ROOM | TO_ARENA_LISTEN);
				} else {
					act("Вы ткнули $o4 в $N3.", FALSE, ch, obj, tch, TO_CHAR);
					act("$N указал$G $o4 на вас.", FALSE, tch, obj, ch, TO_CHAR);
					act("$n ткнул$g $o4 в $N3.", TRUE, ch, obj, tch, TO_NOTVICT | TO_ARENA_LISTEN);
				}
			}
		}
		else if (tobj) {
			if (!obj->get_action_description().empty()) {
				act(obj->get_action_description().c_str(), FALSE, ch, obj, tobj, TO_CHAR);
				act(obj->get_action_description().c_str(), FALSE, ch, obj, tobj, TO_ROOM | TO_ARENA_LISTEN);
			} else {
				act("Вы прикоснулись $o4 к $O2.", FALSE, ch, obj, tobj, TO_CHAR);
				act("$n прикоснул$u $o4 к $O2.", TRUE, ch, obj, tobj, TO_ROOM | TO_ARENA_LISTEN);
			}
		}
		else {
			if (!obj->get_action_description().empty()) {
				act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_CHAR);
				act(obj->get_action_description().c_str(), FALSE, ch, obj, tch, TO_ROOM | TO_ARENA_LISTEN);
			}
			else {
				act("Вы обвели $o4 вокруг комнаты.", FALSE, ch, obj, nullptr, TO_CHAR);
				act("$n обвел$g $o4 вокруг комнаты.", TRUE, ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
			}
		}

		obj->dec_val(2);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		call_magic(ch, tch, tobj, world[ch->in_room], GET_OBJ_VAL(obj, 3), level);
		break;

	case OBJ_DATA::ITEM_SCROLL:
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
		{
			send_to_char("Вы немы, как рыба.\r\n", ch);
			return;
		}
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
		{
			send_to_char("Вы ослеплены.\r\n", ch);
			return;
		}

		spellnum = GET_OBJ_VAL(obj, 1);
		if (!*argument)
			tch = ch;
		else if (!find_cast_target(spellnum, argument, ch, &tch, &tobj, &troom))
			return;

		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), FALSE, ch, obj, nullptr, TO_CHAR);
			act(obj->get_action_description().c_str(), FALSE, ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			act("Вы зачитали $o3, котор$W рассыпался в прах.", TRUE, ch, obj, 0, TO_CHAR);
			act("$n зачитал$g $o3.", FALSE, ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
		}

		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++)
		{
			if (call_magic(ch, tch, tobj, world[ch->in_room], GET_OBJ_VAL(obj, i), level) <= 0)
			{
				break;
			}
		}

		if (obj != nullptr)
		{
			extract_obj(obj);
		}
		break;

	case OBJ_DATA::ITEM_POTION:
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
		{
			send_to_char("Да вам сейчас и глоток воздуха не проглотить!\r\n", ch);
			return;
		}
		tch = ch;
		if (!obj->get_action_description().empty())
		{
			act(obj->get_action_description().c_str(), TRUE, ch, obj, nullptr, TO_CHAR);
			act(obj->get_action_description().c_str(), FALSE, ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
		}
		else
		{
			act("Вы осушили $o3.", FALSE, ch, obj, nullptr, TO_CHAR);
			act("$n осушил$g $o3.", TRUE, ch, obj, nullptr, TO_ROOM | TO_ARENA_LISTEN);
		}

		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++)
		{
			if (call_magic(ch, ch, nullptr, world[ch->in_room], GET_OBJ_VAL(obj, i), level) <= 0)
			{
				break;
			}
		}

		if (obj != nullptr)
		{
			extract_obj(obj);
		}
		break;

	default:
		log("SYSERR: Unknown object_type %d in mag_objectmagic.", GET_OBJ_TYPE(obj));
		break;
	}
}

/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */
int cast_spell(CHAR_DATA * ch, CHAR_DATA * tch, OBJ_DATA * tobj, ROOM_DATA * troom, int spellnum, int spell_subst)
{
	int ignore;
	ESkill skillnum = SKILL_INVALID;

	if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE){
		log("SYSERR: cast_spell trying to call spellnum %d/%d.\n", spellnum, TOP_SPELL_DEFINE);
		return (0);
	}
//проверка на алайнмент мобов

	if (tch && ch){
		if (IS_MOB(tch) && IS_MOB(ch) && !SAME_ALIGN(ch, tch) && !SpINFO.violent)
			return (0);
	}

	if (!troom)
		// Вызвали с пустой комнатой значит будем кастить тут
		troom = world[ch->in_room];

	if (GET_POS(ch) < SpINFO.min_position) {
		switch (GET_POS(ch))
		{
		case POS_SLEEPING:
			send_to_char("Вы спите и не могете думать больше ни о чем.\r\n", ch);
			break;
		case POS_RESTING:
			send_to_char("Вы расслаблены и отдыхаете. И далась вам эта магия?\r\n", ch);
			break;
		case POS_SITTING:
			send_to_char("Похоже, в этой позе Вы много не наколдуете.\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Невозможно! Вы сражаетесь! Это вам не шухры-мухры.\r\n", ch);
			break;
		default:
			send_to_char("Вам вряд ли это удастся.\r\n", ch);
			break;
		}
		return (0);
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && ch->get_master() == tch)	{
		send_to_char("Вы не посмеете поднять руку на вашего повелителя!\r\n", ch);
		return (0);
	}

	if (tch != ch && !IS_IMMORTAL(ch) && IS_SET(SpINFO.targets, TAR_SELF_ONLY)) {
		send_to_char("Вы можете колдовать это только на себя!\r\n", ch);
		return (0);
	}

	if (tch == ch && IS_SET(SpINFO.targets, TAR_NOT_SELF)) {
		send_to_char("Колдовать? ЭТО? На себя?! Да вы с ума сошли!\r\n", ch);
		return (0);
	}

	if ((!tch || IN_ROOM(tch) == NOWHERE) && !tobj && !troom &&
			IS_SET(SpINFO.targets,
				   TAR_CHAR_ROOM | TAR_CHAR_WORLD | TAR_FIGHT_SELF | TAR_FIGHT_VICT
				   | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_WORLD | TAR_OBJ_EQUIP | TAR_ROOM_THIS
				   | TAR_ROOM_DIR))
	{
		send_to_char("Цель заклинания не доступна.\r\n", ch);
		return (0);
	}

	// add by Pereplut: если цель уйдет из клетки в случае кастинга в бою (с лагом) - то
	// не будет кастоваться на соседние клетки, в случае если закл не глобальный
	if (tch && ch && IN_ROOM(tch) != ch->in_room) {
		if (!IS_SET(SpINFO.targets, TAR_CHAR_WORLD)) {
			send_to_char("Цель заклинания не доступна.\r\n", ch);
			return (0);
		}
	}

// Может-ли кастер зачитать заклинание если на нем эфект !смирение!?
// одиночная цель - запрет агро
//
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_PEACEFUL)){
		ignore = IS_SET(SpINFO.targets, TAR_IGNORE) ||
				 IS_SET(SpINFO.routines, MAG_MASSES) || IS_SET(SpINFO.routines, MAG_GROUPS);
		if (ignore)  { // индивидуальная цель
			if (SpINFO.violent) {
				send_to_char("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
				return FALSE;	// нельзя злые кастовать
			}
		}
		// начинаю проверять условия каста
		// Для добрых заклинаний - проверка на противника цели
		// Для злых заклинаний - проверка на цель
		for (const auto ch_vict : world[ch->in_room]->people) {
			if (SpINFO.violent) {
				if (ch_vict == tch) {
					send_to_char("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
					return FALSE;
				}
			}
			else {
				if (ch_vict == tch && !same_group(ch, ch_vict)) {
					send_to_char("Ваша душа полна смирения, и вы не желаете творить зло.\r\n", ch);
					return FALSE;
				}
			}
		}
	}

	//тренируем магические компетенции
	skillnum = get_magic_skill_number_by_spell(spellnum);
	if (skillnum != SKILL_INVALID && skillnum != SKILL_UNDEF) {
		train_skill(ch, skillnum, skill_info[skillnum].max_percent, tch);
	}
	// Комнату тут в say_spell не обрабатываем - будет сказал "что-то"
	say_spell(ch, spellnum, tch, tobj);
	// Уменьшаем кол-во заклов в меме
	if (GET_SPELL_MEM(ch, spell_subst) > 0)
		GET_SPELL_MEM(ch, spell_subst)--;
	else
		GET_SPELL_MEM(ch, spell_subst) = 0;
	// если включен автомем
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && PRF_FLAGGED(ch, PRF_AUTOMEM))
		MemQ_remember(ch, spell_subst);
	// если НПЦ - уменьшаем его макс.количество кастуемых спеллов
	if (IS_NPC(ch))
		GET_CASTER(ch) -= (IS_SET(spell_info[spellnum].routines, NPC_CALCULATE) ? 1 : 0);
	if (!IS_NPC(ch))
		affect_total(ch);

	return (call_magic(ch, tch, tobj, troom, spellnum, GET_LEVEL(ch)));
}

int spell_use_success(CHAR_DATA * ch, CHAR_DATA * victim, int casting_type, int spellnum)
{
	int prob = 100;
	int skill;
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE))
	{
		return TRUE;
	}

	switch (casting_type)
	{
	case SAVING_STABILITY:
	case SAVING_NONE:
		prob = wis_bonus(GET_REAL_WIS(ch), WIS_FAILS) + GET_CAST_SUCCESS(ch);

		if ((IS_MAGE(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_MAGE))
			|| (IS_CLERIC(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_CLERIC))
			|| (IS_PALADINE(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_PALADINE))
			|| (IS_MERCHANT(ch) && ch->in_room != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_MERCHANT)))
		{
			prob += 10;
		}

		if (IS_MAGE(ch) && equip_in_metall(ch))
		{
			prob -= 50;
		}
		break;
	}

	if (equip_in_metall(ch) && !can_use_feat(ch, COMBAT_CASTING_FEAT))
	{
		prob -= 50;
	}

	prob = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_SUCCESS, prob);
	if (GET_GOD_FLAG(ch, GF_GODSCURSE) ||
		(SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSLIKE)) ||
		(!SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSCURSE)))
	{
		prob -= 50;
	}

	if ((SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSCURSE)) ||
		(!SpINFO.violent && victim && GET_GOD_FLAG(victim, GF_GODSLIKE)))
	{
		prob += 50;
	}

	if (IS_NPC(ch) && (GET_LEVEL(ch) >= STRONG_MOB_LEVEL))
	{
		prob += GET_LEVEL(ch) - 20;
	}

	const ESkill skill_number = get_magic_skill_number_by_spell(spellnum);
	if (skill_number > 0)
	{
		skill = ch->get_skill(skill_number) / 20;
		prob += skill;
	}
	//prob*=ch->get_cond_penalty(P_CAST);
	// умения магии дают + к успеху колдовства
	return (prob > number(0, 100));
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
