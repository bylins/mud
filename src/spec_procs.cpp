/* ************************************************************************
*   File: spec_procs.cpp                                Part of Bylins    *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
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

#include "obj.hpp"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spell_parser.hpp"
#include "spells.h"
#include "skills.h"
#include "screen.h"
#include "dg_scripts.h"
#include "constants.h"
#include "features.hpp"
#include "house.h"
#include "char.hpp"
#include "char_player.hpp"
#include "room.hpp"
#include "depot.hpp"
#include "player_races.hpp"
#include "magic.h"
#include "fight.h"
#include "fight_hit.hpp"
#include "char_obj_utils.inl"
#include "world.characters.hpp"
#include "logger.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "temp_spells.hpp"
#include <cmath>
#include <boost/algorithm/string.hpp>

#include <string>

//   external vars
extern DESCRIPTOR_DATA *descriptor_list;
extern INDEX_DATA *mob_index;
extern TIME_INFO_DATA time_info;
extern struct spell_create_type spell_create[];
extern int guild_info[][3];

typedef int special_f(CHAR_DATA*, void*, int, char*);

// extern functions
void do_drop(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_gen_door(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_say(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
int go_track(CHAR_DATA * ch, CHAR_DATA * victim, const ESkill skill_no);
int has_key(CHAR_DATA * ch, obj_vnum key);
int find_first_step(room_rnum src, room_rnum target, CHAR_DATA * ch);
void do_doorcmd(CHAR_DATA * ch, OBJ_DATA * obj, int door, int scmd);
void ASSIGNMASTER(mob_vnum mob, special_f, int learn_info);
int has_key(CHAR_DATA * ch, obj_vnum key);
int ok_pick(CHAR_DATA * ch, obj_vnum keynum, OBJ_DATA* obj, int door, int scmd);

// local functions
char *how_good(CHAR_DATA * ch, int percent);
int feat_slot_lvl(int remort, int slot_for_remort, int slot);
void list_feats(CHAR_DATA * ch, CHAR_DATA * vict, bool all_feats);
void list_skills(CHAR_DATA * ch, CHAR_DATA * vict, const char* filter = NULL);
void list_spells(CHAR_DATA * ch, CHAR_DATA * vict, int all_spells);
int slot_for_char(CHAR_DATA * ch, int i);
int guild_mono(CHAR_DATA *ch, void *me, int cmd, char* argument);
int guild_poly(CHAR_DATA *ch, void *me, int cmd, char* argument);
int guild(CHAR_DATA *ch, void *me, int cmd, char* argument);
int dump(CHAR_DATA *ch, void *me, int cmd, char* argument);
int mayor(CHAR_DATA *ch, void *me, int cmd, char* argument);
int snake(CHAR_DATA *ch, void *me, int cmd, char* argument);
int thief(CHAR_DATA *ch, void *me, int cmd, char* argument);
int magic_user(CHAR_DATA *ch, void *me, int cmd, char* argument);
int guild_guard(CHAR_DATA *ch, void *me, int cmd, char* argument);
int fido(CHAR_DATA *ch, void *me, int cmd, char* argument);
int janitor(CHAR_DATA *ch, void *me, int cmd, char* argument);
int cityguard(CHAR_DATA *ch, void *me, int cmd, char* argument);
int pet_shops(CHAR_DATA *ch, void *me, int cmd, char* argument);
int bank(CHAR_DATA *ch, void *me, int cmd, char* argument);


// ********************************************************************
// *  Special procedures for mobiles                                  *
// ********************************************************************

char *how_good(CHAR_DATA * ch, int percent)
{
	static char out_str[128];

	if (percent < 0)
		strcpy(out_str, " !Ошибка! ");
	else if (percent == 0)
		sprintf(out_str, " %s(не изучено)%s", CCINRM(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 10)
		sprintf(out_str, " %s(ужасно)%s", CCINRM(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 20)
		sprintf(out_str, " %s(очень плохо)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 30)
		sprintf(out_str, " %s(плохо)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 40)
		sprintf(out_str, " %s(слабо)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 50)
		sprintf(out_str, " %s(ниже среднего)%s", CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 60)
		sprintf(out_str, " %s(средне)%s", CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 70)
		sprintf(out_str, " %s(выше среднего)%s", CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 80)
		sprintf(out_str, " %s(хорошо)%s", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 90)
		sprintf(out_str, " %s(очень хорошо)%s", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 100)
		sprintf(out_str, " %s(отлично)%s", CCIYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 110)
		sprintf(out_str, " %s(превосходно)%s", CCIYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 120)
		sprintf(out_str, " %s(великолепно)%s", CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 130)
		sprintf(out_str, " %s(мастерски)%s", CCIBLU(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 140)
		sprintf(out_str, " %s(идеально)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 150)
		sprintf(out_str, " %s(совершенно)%s", CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 160)
		sprintf(out_str, " %s(бесподобно)%s", CCMAG(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 170)
		sprintf(out_str, " %s(возвышенно)%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 180)
		sprintf(out_str, " %s(заоблачно)%s", CCICYN(ch, C_NRM), CCNRM(ch, C_NRM));
	else if (percent <= 190)
		sprintf(out_str, " %s(божественно)%s", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
	else
		sprintf(out_str, " %s(недостижимо)%s", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
	sprintf(out_str + strlen(out_str), " %d%%", percent);
	return out_str;
}

const char *prac_types[] = { "spell",
							 "skill"
						   };

#define LEARNED_LEVEL	0	// % known which is considered "learned" //
#define MAX_PER_PRAC	1	// max percent gain in skill per practice //
#define MIN_PER_PRAC	2	// min percent gain in skill per practice //
#define PRAC_TYPE	3	// should it say 'spell' or 'skill'?     //

// actual prac_params are in class.cpp //
extern int prac_params[4][NUM_PLAYER_CLASSES];

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

int feat_slot_lvl(int remort, int slot_for_remort, int slot)
{
	int result = 0;
	for (result = 1; result < LVL_IMMORT; result++)
	{
		if (result*(5 + remort / slot_for_remort) / 28 == slot)
		{
			break;
		}
	}
	/*
	ВНИМАНИЕ: формула содрана с NUM_LEV_FEAT (utils.h)!
	((int) 1+GET_LEVEL(ch)*(5+GET_REMORT(ch)/feat_slot_for_remort[(int) GET_CLASS(ch)])/28)
	сделано это потому, что "обратная" формула, использованная ранее в list_feats,
	выдавала неверные результаты ввиду нюансов округления
	*/
	return result;
}

/*
1. Определяем есть ли способность, если нет - новый цикл
2. Если есть
2.2 Врожденная : таймер(отступ)+имя -> буфер врожденных
2.3 Неврожденная: таймер(отступ)+имя -> 3
3. Если слот <= макс слота и свободен -> номер слота+таймер+имя в массив имен под номер слота
3.2. Если слот занят, увеличиваеим номер слота на 1 и переход на 3 пункт
3.3 Иначе удаляем способность.
Примечание: удаление реализовано с целью сделать возможнным изменение слота в процессе игры.
Лишние способности у персонажей удалятся автоматически при использовании команды "способности".
*/
void list_feats(CHAR_DATA * ch, CHAR_DATA * vict, bool all_feats)
{
	int i = 0, j = 0, sortpos, slot, max_slot=0;
	char msg[MAX_STRING_LENGTH];
	bool sfound;

	//Найдем максимальный слот, который вобще может потребоваться данному персонажу на текущем морте
	max_slot =  MAX_ACC_FEAT(ch);
	char  **names = new char*[max_slot];
	for (int k = 0; k< max_slot; k++)
		names[k]=new char[MAX_STRING_LENGTH];

	if (all_feats)
	{
		sprintf(names[0], "\r\nКруг 1  (1  уровень):\r\n");
	}
	else
		*names[0] = '\0';
	for (i = 1; i < max_slot; i++)
		if (all_feats)
		{
			//j = i*28/(5+GET_REMORT(ch)/feat_slot_for_remort[(int) GET_CLASS(ch)]); // старая формула, работавшая криво!
			j = feat_slot_lvl(GET_REMORT(ch), feat_slot_for_remort[(int) GET_CLASS(ch)], i); // на каком уровне будет слот i?
			sprintf(names[i], "\r\nКруг %-2d (%-2d уровень):\r\n", i + 1, j);
		}
		else
			*names[i] = '\0';

	sprintf(buf2, "\r\nВрожденные способности :\r\n");
	j = 0;
	if (all_feats)
	{
		if (clr(vict, C_NRM)) // реж цвет >= обычный
			send_to_char(" Список способностей, доступных с текущим числом перевоплощений.\r\n"
						" &gЗеленым цветом и пометкой [И] выделены уже изученные способности.\r\n&n"
						" Пометкой [Д] выделены доступные для изучения способности.\r\n"
						" &rКрасным цветом и пометкой [Н] выделены способности, недоступные вам в настоящий момент.&n\r\n\r\n", vict);
		else
			send_to_char(" Список способностей, доступных с текущим числом перевоплощений.\r\n"
						" Пометкой [И] выделены уже изученные способности.\r\n"
						" Пометкой [Д] выделены доступные для изучения способности.\r\n"
						" Пометкой [Н] выделены способности, недоступные вам в настоящий момент.\r\n\r\n", vict);
		for (sortpos = 1; sortpos < MAX_FEATS; sortpos++)
		{
			if (!feat_info[sortpos].classknow[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] && !PlayerRace::FeatureCheck((int)GET_KIN(ch),(int)GET_RACE(ch),sortpos))
				continue;
			if (clr(vict, C_NRM))
				sprintf(buf, "        %s%s %-30s%s\r\n",
					HAVE_FEAT(ch, sortpos) ? KGRN :
					can_get_feat(ch, sortpos) ? KNRM : KRED,
					HAVE_FEAT(ch, sortpos) ? "[И]" :
					can_get_feat(ch, sortpos) ? "[Д]" : "[Н]",
					feat_info[sortpos].name, KNRM);
			else
				sprintf(buf, "    %s %-30s\r\n",
					HAVE_FEAT(ch, sortpos) ? "[И]" :
					can_get_feat(ch, sortpos) ? "[Д]" : "[Н]",
					feat_info[sortpos].name);

			if (feat_info[sortpos].natural_classfeat[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] || PlayerRace::FeatureCheck((int)GET_KIN(ch),(int)GET_RACE(ch),sortpos))
			{
				strcat(buf2, buf);
				j++;
			}
			else if (FEAT_SLOT(ch, sortpos) < max_slot)
			{
				const auto slot = FEAT_SLOT(ch, sortpos);
				strcat(names[slot], buf);
			}
		}
		sprintf(buf1, "--------------------------------------");
		for (i = 0; i < max_slot; i++)
		{
			if (strlen(buf1) >= MAX_STRING_LENGTH - 60)
			{
				strcat(buf1, "***ПЕРЕПОЛНЕНИЕ***\r\n");
				break;
			}
			sprintf(buf1 + strlen(buf1), "%s", names[i]);
		}

		send_to_char(buf1, vict);
//		page_string(ch->desc, buf, 1);
		if (j)
			send_to_char(buf2, vict);

		for (int k = 0; k < max_slot; k++)
			delete[] names[k];

		delete[] names;

		return;
	}

// ======================================================

	sprintf(buf1, "Вы обладаете следующими способностями :\r\n");

	for (sortpos = 1; sortpos < MAX_FEATS; sortpos++)
	{
		if (strlen(buf2) >= MAX_STRING_LENGTH - 60)
		{
			strcat(buf2, "***ПЕРЕПОЛНЕНИЕ***\r\n");
			break;
		}
		if (HAVE_FEAT(ch, sortpos))
		{
			if (!feat_info[sortpos].name || *feat_info[sortpos].name == '!')
				continue;

			switch (sortpos)
			{
			case BERSERK_FEAT:
			case LIGHT_WALK_FEAT:
			case SPELL_CAPABLE_FEAT:
			case RELOCATE_FEAT:
				if (timed_by_feat(ch, sortpos))
					sprintf(buf, "[%3d] ", timed_by_feat(ch, sortpos));
				else
					sprintf(buf, "[-!-] ");
				break;
			case POWER_ATTACK_FEAT:
				if (PRF_FLAGGED(ch, PRF_POWERATTACK))
					sprintf(buf, "[-%s*%s-] ", CCIGRN(vict, C_NRM), CCNRM(vict, C_NRM));
				else
					sprintf(buf, "[-!-] ");
				break;
			case GREAT_POWER_ATTACK_FEAT:
				if (PRF_FLAGGED(ch, PRF_GREATPOWERATTACK))
					sprintf(buf, "[-%s*%s-] ", CCIGRN(vict, C_NRM), CCNRM(vict, C_NRM));
				else
					sprintf(buf, "[-!-] ");
				break;
			case AIMING_ATTACK_FEAT:
				if (PRF_FLAGGED(ch, PRF_AIMINGATTACK))
					sprintf(buf, "[-%s*%s-] ", CCIGRN(vict, C_NRM), CCNRM(vict, C_NRM));
				else
					sprintf(buf, "[-!-] ");
				break;
			case GREAT_AIMING_ATTACK_FEAT:
				if (PRF_FLAGGED(ch, PRF_GREATAIMINGATTACK))
					sprintf(buf, "[-%s*%s-] ", CCIGRN(vict, C_NRM), CCNRM(vict, C_NRM));
				else
					sprintf(buf, "[-!-] ");
				break;
			default:
				sprintf(buf, "      ");
			}
			if (can_use_feat(ch, sortpos))
				sprintf(buf + strlen(buf), "%s%s%s\r\n",
					CCIYEL(vict, C_NRM), feat_info[sortpos].name, CCNRM(vict, C_NRM));
			else if (clr(vict, C_NRM))
				sprintf(buf + strlen(buf), "%s\r\n", feat_info[sortpos].name);
			else
				sprintf(buf, "[-Н-] %s\r\n", feat_info[sortpos].name);
			if (feat_info[sortpos].natural_classfeat[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] || PlayerRace::FeatureCheck((int)GET_KIN(ch),(int)GET_RACE(ch),sortpos))
			{
				sprintf(buf2 + strlen(buf2), "    ");
				strcat(buf2, buf);
				j++;
			}
			else
			{
				slot = FEAT_SLOT(ch, sortpos);
				sfound = FALSE;
				while (slot < max_slot)
				{
					if (*names[slot] == '\0')
					{
						sprintf(names[slot], " %s%-2d%s) ",
								CCGRN(vict, C_NRM), slot + 1, CCNRM(vict, C_NRM));
						strcat(names[slot],  buf);
						sfound = TRUE;
						break;
					}
					else
						slot++;
				}
				if (!sfound)
				{
					// Если способность не врожденная и под нее нет слота - удаляем нафик
					//	чтобы можно было менять слоты на лету и чтобы не читерили :)
					sprintf(msg, "WARNING: Unset out of slots feature '%s' for character '%s'!",
							feat_info[sortpos].name, GET_NAME(ch));
					mudlog(msg, BRF, LVL_IMPL, SYSLOG, TRUE);
					UNSET_FEAT(ch, sortpos);
				}
			}
		}
	}

	for (i = 0; i < max_slot; i++)
	{
		if (*names[i] == '\0')
			sprintf(names[i], " %s%-2d%s)       %s[пусто]%s\r\n",
					CCGRN(vict, C_NRM), i + 1, CCNRM(vict, C_NRM), CCIWHT(vict, C_NRM), CCNRM(vict, C_NRM));
		if (i >= NUM_LEV_FEAT(ch))
			break;
		sprintf(buf1 + strlen(buf1), "%s", names[i]);
	}
	send_to_char(buf1, vict);

	if (j)
		send_to_char(buf2, vict);

	for (int k = 0; k < max_slot; k++)
		delete[] names[k];
	
	delete[] names;
}

void list_skills(CHAR_DATA * ch, CHAR_DATA * vict, const char* filter/* = NULL*/)
{
	int i = 0, bonus = 0;

	sprintf(buf, "Вы владеете следующими умениями :\r\n");

	strcpy(buf2, buf);
	if (!IS_NPC(ch)
		&& !ch->affected.empty())
	{
		for (const auto& aff : ch->affected)
		{
			if (aff->location == APPLY_BONUS_SKILLS) // скушал свиток с скилл бонусом
			{
				bonus = aff->modifier; // сколько крут стал 
			}
		}
	}

	typedef std::list<std::string> skills_t;
	skills_t skills;

	for (const auto sortpos : AVAILABLE_SKILLS)
	{
		if (ch->get_skill(sortpos))
		{
			// filter out skills without name or name beginning with '!' character
			if (!skill_info[sortpos].name
				|| *skill_info[sortpos].name == '!')
			{
				continue;
			}

			// filter out skill that does not correspond to filter condition
			if (filter
				&& NULL == strstr(skill_info[sortpos].name, filter))
			{
				continue;
			}

			switch (sortpos)
			{
			case SKILL_WARCRY:
				sprintf(buf, "[-%d-] ", (HOURS_PER_DAY - timed_by_skill(ch, sortpos)) / HOURS_PER_WARCRY);
				break;
			case SKILL_AID:
			case SKILL_DRUNKOFF:
			case SKILL_IDENTIFY:
			case SKILL_CAMOUFLAGE:
			case SKILL_COURAGE:
			case SKILL_MANADRAIN:
			case SKILL_TOWNPORTAL:
			case SKILL_TURN_UNDEAD:
			case SKILL_STRANGLE:
			case SKILL_STUN:
				if (timed_by_skill(ch, sortpos))
					sprintf(buf, "[%3d] ", timed_by_skill(ch, sortpos));
				else
					sprintf(buf, "[-!-] ");
				break;
			default:
				sprintf(buf, "      ");
			}
			
			sprintf(buf + strlen(buf), "%-23s %s\r\n",
				skill_info[sortpos].name,
				how_good(ch, ch->get_skill(sortpos) + bonus));

			skills.push_back(buf);

			i++;
		}
	}

	if (!i)
	{
		if (NULL == filter)
		{
			sprintf(buf2 + strlen(buf2), "Нет умений.\r\n");
		}
		else
		{
			sprintf(buf2 + strlen(buf2), "Нет умений, удовлетворяющих фильтру.\r\n");
		}
	}
	else
	{
		// output set of skills
		size_t buf2_length = strlen(buf2);
		for (skills_t::const_iterator i = skills.begin(); i != skills.end(); ++i)
		{
			if (buf2_length + i->length() >= MAX_STRING_LENGTH - 60)		// why 60?
			{
				strcat(buf2, "***ПЕРЕПОЛНЕНИЕ***\r\n");
				break;
			}

			strncat(buf2 + buf2_length, i->c_str(), i->length());
			buf2_length += i->length();
		}
	}

	send_to_char(buf2, vict);
}
const char *spells_color(int spellnum )
{
		switch (spell_info[spellnum].spell_class)
		{
		case STYPE_AIR:
			return "&W";
			break;

		case STYPE_FIRE:
			return "&R";
			break;

	        case STYPE_WATER:
			return "&C";
			break;

	        case STYPE_EARTH:
			return "&y";
			break;

	        case STYPE_LIGHT:
			return "&Y";
			break;

	        case STYPE_DARK:
			return "&K";
			break;

	        case STYPE_MIND:
			return "&M";
			break;

	        case STYPE_LIFE:
			return "&G";
			break;

	        case STYPE_NEUTRAL:
	        	return "&n";
			break;
	        default:
	        	return "&n";
			break;
	        }	
}


/* Параметр all_spells введен для того чтобы предметные кастеры
   смогли посмотреть заклинания которые они могут колдовать
   на своем уровне, но на которые у них нет необходимых предметов
   при параметре TRUE */

void list_spells(CHAR_DATA * ch, CHAR_DATA * vict, int all_spells)
{
	char names[MAX_SLOT][MAX_STRING_LENGTH];
	std::string time_str;
	int slots[MAX_SLOT], i, max_slot = 0, slot_num, is_full, gcount = 0, can_cast = 1;

	is_full = 0;
	max_slot = 0;
	for (i = 0; i < MAX_SLOT; i++)
	{
		*names[i] = '\0';
		slots[i] = 0;
	}
	for (i = 1; i <= MAX_SPELLS; i++)
	{
		if (!GET_SPELL_TYPE(ch, i) && !all_spells)
			continue;

		if ((MIN_CAST_LEV(spell_info[i], ch) > GET_LEVEL(ch)
				|| MIN_CAST_REM(spell_info[i], ch) > GET_REMORT(ch)
				|| slot_for_char(ch, spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) <= 0)
				&& all_spells && !GET_SPELL_TYPE(ch, i))
			continue;

		if (!spell_info[i].name || *spell_info[i].name == '!')
			continue;

		if ((GET_SPELL_TYPE(ch, i) & 0xFF) == SPELL_RUNES && !check_recipe_items(ch, i, SPELL_RUNES, FALSE) )
		{
			if (all_spells)
			{
				can_cast = 0;
			}
			else
			{
				continue;
			}
		}
		else
		{
			can_cast = 1;
		}

		if (MIN_CAST_REM(spell_info[i], ch) > GET_REMORT(ch))
			slot_num = MAX_SLOT - 1;
		else
			slot_num = spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
		max_slot = MAX(slot_num + 1, max_slot);
		if (IS_MANA_CASTER(ch))
		{
			if (GET_MANA_COST(ch, i) > GET_MAX_MANA(ch))
				continue;
			if (can_cast)
			{
				slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
										   "%s|<...%4d.> %s%-38s&n|",
										   slots[slot_num] % 114 <
										   10 ? "\r\n" : "  ",
										   GET_MANA_COST(ch, i), spells_color(i), spell_info[i].name);
			}
			else
			{
				slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
										   "%s|+--------+ %s%-38s&n|",
										   slots[slot_num] % 114 <
										   10 ? "\r\n" : "  ", spells_color(i), spell_info[i].name);
			}
		}
		else
		{
			time_str.clear();
			if (IS_SET(GET_SPELL_TYPE(ch, i), SPELL_TEMP))
			{
				time_str.append("[");
				time_str.append(std::to_string(MAX(1, static_cast<int>(std::ceil(static_cast<double>(Temporary_Spells::spell_left_time(ch, i)) / SECS_PER_MUD_HOUR)))));
				time_str.append("]");
			}


			slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
				"%s|<%c%c%c%c%c%c%c%c>%s %-30s %-7s&n|", 

				slots[slot_num] % 114 < 10 ? "\r\n" : "  ",
				IS_SET(GET_SPELL_TYPE(ch, i),
					SPELL_KNOW) ? ((MIN_CAST_LEV(spell_info[i], ch) > GET_LEVEL(ch)) ? 'N' : 'K') : '.',
				IS_SET(GET_SPELL_TYPE(ch, i),
					SPELL_TEMP) ? 'T' : '.',
				IS_SET(GET_SPELL_TYPE(ch, i),
					SPELL_POTION) ? 'P' : '.',
				IS_SET(GET_SPELL_TYPE(ch, i),
					SPELL_WAND) ? 'W' : '.',
				IS_SET(GET_SPELL_TYPE(ch, i),
					SPELL_SCROLL) ? 'S' : '.',
				IS_SET(GET_SPELL_TYPE(ch, i),
					SPELL_ITEMS) ? 'I' : '.',
				IS_SET(GET_SPELL_TYPE(ch, i),
					SPELL_RUNES) ? 'R' : '.',
				'.',
				spells_color(i),
				spell_info[i].name,
				time_str.c_str());
		}
		is_full++;
	};
	gcount = sprintf(buf2 + gcount, "  %sВам доступна следующая магия :%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	if (is_full)
	{
		for (i = 0; i < max_slot; i++)
		{
			if (slots[i] != 0)
			{
				if (!IS_MANA_CASTER(ch))
					gcount += sprintf(buf2 + gcount, "\r\nКруг %d", i + 1);
			}
			if (slots[i])
				gcount += sprintf(buf2 + gcount, "%s", names[i]);
			//else
			//gcount += sprintf(buf2+gcount,"\n\rПусто.");
		}
	}
	else
		gcount += sprintf(buf2 + gcount, "\r\nВ настоящее время магия вам недоступна!");
	gcount += sprintf(buf2 + gcount, "\r\n");
	//page_string(ch->desc, buf2, 1);
	send_to_char(buf2, vict);
}

struct guild_learn_type
{
	int feat_no;
	ESkill skill_no;
	int spell_no;
	int level;
};

struct guild_mono_type
{
	guild_mono_type() : races(0), classes(0), religion(0), alignment(0), learn_info(0) {};
	int races;		// bitvector //
	int classes;		// bitvector //
	int religion;		// bitvector //
	int alignment;		// bitvector //
	struct guild_learn_type *learn_info;
};

struct guild_poly_type
{
	int races;		// bitvector //
	int classes;		// bitvector //
	int religion;		// bitvector //
	int alignment;		// bitvector //
	int feat_no;
	ESkill skill_no;
	int spell_no;
	int level;
};

int GUILDS_MONO_USED = 0;
int GUILDS_POLY_USED = 0;

struct guild_mono_type *guild_mono_info = nullptr;
struct guild_poly_type **guild_poly_info = nullptr;

void init_guilds(void)
{
	FILE *magic;
	char name[MAX_INPUT_LENGTH],
	line[256], line1[256], line2[256], line3[256], line4[256], line5[256], line6[256], *pos;
	int i, spellnum, skillnum, featnum, num, type = 0, lines = 0, level, pgcount = 0, mgcount = 0;
	std::unique_ptr<struct guild_poly_type, decltype(free) *> poly_guild(nullptr, free);
	struct guild_mono_type mono_guild;
	std::unique_ptr<struct guild_learn_type, decltype(free) *> mono_guild_learn(nullptr, free);

	if (!(magic = fopen(LIB_MISC "guilds.lst", "r")))
	{
		log("Cann't open guilds list file...");
		return;
	}
	while (get_line(magic, name))
	{
		if (!name[0] || name[0] == ';')
			continue;
		log("<%s>", name);
		if ((lines = sscanf(name, "%s %s %s %s %s %s %s", line, line1, line2, line3, line4, line5, line6)) == 0)
			continue;
		// log("%d",lines);

		if (!strn_cmp(line, "monoguild", strlen(line))
				|| !strn_cmp(line, "одноклассовая", strlen(line)))
		{
			type = 1;
			if (lines < 5)
			{
				log("Bad format for monoguild header, #s #s #s #s #s need...");
				graceful_exit(1);
			}
			mono_guild_learn.reset();
			mono_guild.learn_info = nullptr;
			mono_guild.races = 0;
			mono_guild.classes = 0;
			mono_guild.religion = 0;
			mono_guild.alignment = 0;
			mgcount = 0;
			for (i = 0; *(line1 + i); i++)
				if (strchr("!1xX", *(line1 + i)))
					SET_BIT(mono_guild.races, (1 << i));
			for (i = 0; *(line2 + i); i++)
				if (strchr("!1xX", *(line2 + i)))
					SET_BIT(mono_guild.classes, (1 << i));
			for (i = 0; *(line3 + i); i++)
				if (strchr("!1xX", *(line3 + i)))
					SET_BIT(mono_guild.religion, (1 << i));
			for (i = 0; *(line4 + i); i++)
				if (strchr("!1xX", *(line4 + i)))
					SET_BIT(mono_guild.alignment, (1 << i));
		}
		else if (!strn_cmp(line, "polyguild", strlen(line))
				 || !strn_cmp(line, "многоклассовая", strlen(line)))
		{
			type = 2;
			poly_guild.reset();
			pgcount = 0;
		}
		else if (!strn_cmp(line, "master", strlen(line))
				 || !strn_cmp(line, "учитель", strlen(line)))
		{
			if ((num = atoi(line1)) == 0 || real_mobile(num) < 0)
			{
				log("WARNING: Can't assign master %s in guilds.lst. Skipped.", line1);
				continue;
			}

			if (!((type == 1 && mono_guild_learn) || type == 11) &&
					!((type == 2 && poly_guild) || type == 12))
			{
				log("WARNING: Can't define guild info for master %s. Skipped.", line1);
				continue;
			}
			if (type == 1 || type == 11)
			{
				if (type == 1)
				{
					if (!guild_mono_info)
					{
						CREATE(guild_mono_info, GUILDS_MONO_USED + 1);
					}
					else
					{
						RECREATE(guild_mono_info, GUILDS_MONO_USED + 1);
					}
					log("Create mono guild %d", GUILDS_MONO_USED + 1);
					mono_guild.learn_info = mono_guild_learn.release();
					RECREATE(mono_guild.learn_info, mgcount + 1);
					(mono_guild.learn_info + mgcount)->skill_no = SKILL_INVALID;
					(mono_guild.learn_info + mgcount)->feat_no = -1;
					(mono_guild.learn_info + mgcount)->spell_no = -1;
					(mono_guild.learn_info + mgcount)->level = -1;
					guild_mono_info[GUILDS_MONO_USED] = mono_guild;
					GUILDS_MONO_USED++;
					type = 11;
				}
				log("Assign mono guild %d to mobile %s", GUILDS_MONO_USED, line1);
				ASSIGNMASTER(num, guild_mono, GUILDS_MONO_USED);
			}
			if (type == 2 || type == 12)
			{
				if (type == 2)
				{
					if (!guild_poly_info)
					{
						CREATE(guild_poly_info, GUILDS_POLY_USED + 1);
					}
					else
					{
						RECREATE(guild_poly_info, GUILDS_POLY_USED + 1);
					}
					log("Create poly guild %d", GUILDS_POLY_USED + 1);
					auto ptr = poly_guild.release();
					RECREATE(ptr, pgcount + 1);
					(ptr + pgcount)->feat_no = -1;
					(ptr + pgcount)->skill_no = SKILL_INVALID;
					(ptr + pgcount)->spell_no = -1;
					(ptr + pgcount)->level = -1;
					guild_poly_info[GUILDS_POLY_USED] = ptr;
					GUILDS_POLY_USED++;
					type = 12;
				}
				log("Assign poly guild %d to mobile %s", GUILDS_POLY_USED, line1);
				ASSIGNMASTER(num, guild_poly, GUILDS_POLY_USED);
			}
		}
		else if (type == 1)
		{
			if (lines < 3)
			{
				log("You need use 3 arguments for monoguild");
				graceful_exit(1);
			}
			if ((spellnum = atoi(line)) == 0 || spellnum > MAX_SPELLS)
			{
				spellnum = fix_name_and_find_spell_num(line);
			}
			if ((skillnum = atoi(line1)) == 0 || skillnum > MAX_SKILL_NUM)
			{
				skillnum = fix_name_and_find_skill_num(line1);
			}

			if ((featnum = atoi(line1)) == 0 || featnum >= MAX_FEATS)
			{
				if ((pos = strchr(line1, '.')))
					* pos = ' ';
				featnum = find_feat_num(line1);
			}

			if (skillnum <= 0 && spellnum <= 0 && featnum <= 0)
			{
				log("Unknown skill, spell or feat for monoguild");
				graceful_exit(1);
			}
			if ((level = atoi(line2)) == 0 || level >= LVL_IMMORT)
			{
				log("Use 1-%d level for guilds", LVL_IMMORT);
				graceful_exit(1);
			}

			auto ptr = mono_guild_learn.release();
			if (!ptr)
			{
				CREATE(ptr, mgcount + 1);
			}
			else
			{
				RECREATE(ptr, mgcount + 1);
			}
			mono_guild_learn.reset(ptr);

			ptr += mgcount;
			ptr->spell_no = MAX(0, spellnum);
			ptr->skill_no = static_cast<ESkill>(MAX(0, skillnum));
			ptr->feat_no = MAX(0, featnum);
			ptr->level = level;
			// log("->%d %d %d<-",spellnum,skillnum,level);
			mgcount++;
		}
		else if (type == 2)
		{
			if (lines < 7)
			{
				log("You need use 7 arguments for polyguild");
				graceful_exit(1);
			}
			auto ptr = poly_guild.release();
			if (!ptr)
			{
				CREATE(ptr, pgcount + 1);
			}
			else
			{
				RECREATE(ptr, pgcount + 1);
			}
			poly_guild.reset(ptr);

			ptr += pgcount;
			ptr->races = 0;
			ptr->classes = 0;
			ptr->religion = 0;
			ptr->alignment = 0;
			for (i = 0; *(line + i); i++)
				if (strchr("!1xX", *(line + i)))
					SET_BIT(ptr->races, (1 << i));
			for (i = 0; *(line1 + i); i++)
				if (strchr("!1xX", *(line1 + i)))
					SET_BIT(ptr->classes, (1 << i));
			for (i = 0; *(line2 + i); i++)
				if (strchr("!1xX", *(line2 + i)))
					SET_BIT(ptr->religion, (1 << i));
			for (i = 0; *(line3 + i); i++)
				if (strchr("!1xX", *(line3 + i)))
					SET_BIT(ptr->alignment, (1 << i));
			if ((spellnum = atoi(line4)) == 0 || spellnum > MAX_SPELLS)
			{
				spellnum = fix_name_and_find_spell_num(line4);
			}
			if ((skillnum = atoi(line5)) == 0 || skillnum > MAX_SKILL_NUM)
			{
				skillnum = fix_name_and_find_skill_num(line5);
			}

			if ((featnum = atoi(line5)) == 0 || featnum >= MAX_FEATS)
			{
				if ((pos = strchr(line5, '.')))
					* pos = ' ';

				featnum = find_feat_num(line1);
				sprintf(buf, "feature number 2: %d", featnum);
				featnum = find_feat_num(line5);
			}
			if (skillnum <= 0 && spellnum <= 0 && featnum <= 0)
			{
				log("Unknown skill, spell or feat for polyguild");
				graceful_exit(1);
			}
			if ((level = atoi(line6)) == 0 || level >= LVL_IMMORT)
			{
				log("Use 1-%d level for guilds", LVL_IMMORT);
				graceful_exit(1);
			}
			ptr->spell_no = MAX(0, spellnum);
			ptr->skill_no = static_cast<ESkill>(MAX(0, skillnum));
			ptr->feat_no = MAX(0, featnum);
			ptr->level = level;
			// log("->%d %d %d<-",spellnum,skillnum,level);
			pgcount++;
		}
	}
	fclose(magic);
	return;
}

#define SCMD_LEARN 1

int guild_mono(CHAR_DATA *ch, void *me, int cmd, char* argument)
{
	int command = 0, gcount = 0, info_num = 0, found = FALSE, sfound = FALSE, i, bits;
	CHAR_DATA *victim = (CHAR_DATA *) me;

	if (IS_NPC(ch))
	{
		return 0;
	}

	if (CMD_IS("учить")
		|| CMD_IS("practice"))
	{
		command = SCMD_LEARN;
	}
	else
	{
		return 0;
	}

	info_num = mob_index[victim->get_rnum()].stored;
	if (info_num <= 0
		|| info_num > GUILDS_MONO_USED)
	{
		act("$N сказал$G : 'Извини, $n, я уже в отставке.'", FALSE, ch, 0, victim, TO_CHAR);
		return (1);
	}

	info_num--;
	if (!IS_BITS(guild_mono_info[info_num].classes, GET_CLASS(ch))
		|| !IS_BITS(guild_mono_info[info_num].races, GET_RACE(ch))
		|| !IS_BITS(guild_mono_info[info_num].religion, GET_RELIGION(ch)))
	{
		act("$N сказал$g : '$n, я не учу таких, как ты.'", FALSE, ch, 0, victim, TO_CHAR);
		return 1;
	}

	skip_spaces(&argument);

	switch (command)
	{
	case SCMD_LEARN:
		if (!*argument)
		{
			gcount += sprintf(buf, "Я могу научить тебя следующему:\r\n");
			for (i = 0, found = FALSE; (guild_mono_info[info_num].learn_info + i)->spell_no >= 0; i++)
			{
				if ((guild_mono_info[info_num].learn_info + i)->level > GET_LEVEL(ch))
				{
					continue;
				}

				const auto skill_no = (guild_mono_info[info_num].learn_info + i)->skill_no;
				bits = skill_no;
				if (SKILL_INVALID != skill_no
					&& (!ch->get_skill(skill_no)
						|| IS_GRGOD(ch))
					&& can_get_skill(ch, skill_no))
				{
					gcount += sprintf(buf + gcount, "- умение %s\"%s\"%s\r\n",
						CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
					found = TRUE;
				}

				if (!(bits = -2 * bits)
					|| bits == SPELL_TEMP)
				{
					bits = SPELL_KNOW;
				}

				const auto spell_no = (guild_mono_info[info_num].learn_info + i)->spell_no;
				if (spell_no
					&& (!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)
						|| IS_GRGOD(ch)))
				{
					gcount += sprintf(buf + gcount, "- магия %s\"%s\"%s\r\n",
						CCCYN(ch, C_NRM), spell_name(spell_no), CCNRM(ch, C_NRM));
					found = TRUE;
				}

				const auto feat_no = (guild_mono_info[info_num].learn_info + i)->feat_no;
				if (feat_no > 0
					&& !HAVE_FEAT(ch, feat_no)
					&& can_get_feat(ch, feat_no))
				{
					gcount += sprintf(buf + gcount, "- способность %s\"%s\"%s\r\n",
						CCCYN(ch, C_NRM), feat_name(feat_no), CCNRM(ch, C_NRM));
					found = TRUE;
				}
			}

			if (!found)
			{
				act("$N сказал$G : 'Похоже, твои знания полнее моих.'", FALSE, ch, 0, victim, TO_CHAR);
				return (1);
			}
			else
			{
				send_to_char(buf, ch);
				return (1);
			}
		}

		if (!strn_cmp(argument, "все", strlen(argument)) || !strn_cmp(argument, "all", strlen(argument)))
		{
			for (i = 0, found = FALSE, sfound = TRUE;
			(guild_mono_info[info_num].learn_info + i)->spell_no >= 0; i++)
			{
				if ((guild_mono_info[info_num].learn_info + i)->level > GET_LEVEL(ch))
					continue;

				const ESkill skill_no = (guild_mono_info[info_num].learn_info + i)->skill_no;
				bits = skill_no;
				if (SKILL_INVALID != skill_no
					&& !ch->get_skill(skill_no))  	// sprintf(buf, "$N научил$G вас умению %s\"%s\"\%s",
				{
					//             CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
					// act(buf,FALSE,ch,0,victim,TO_CHAR);
					// ch->get_skill(skill_no) = 10;
					sfound = TRUE;
				}

				if (!(bits = -2 * bits) || bits == SPELL_TEMP)
				{
					bits = SPELL_KNOW;
				}

				const int spell_no = (guild_mono_info[info_num].learn_info + i)->spell_no;
				if (spell_no
					&& !((GET_SPELL_TYPE(ch, spell_no) & bits) == bits))
				{
					gcount += sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
						CCCYN(ch, C_NRM), spell_name(spell_no), CCNRM(ch, C_NRM));
					act(buf, FALSE, ch, 0, victim, TO_CHAR);
					if (IS_SET(bits, SPELL_KNOW))
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_KNOW);
					if (IS_SET(bits, SPELL_ITEMS))
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_ITEMS);
					if (IS_SET(bits, SPELL_RUNES))
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_RUNES);
					if (IS_SET(bits, SPELL_POTION))
					{
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_POTION);
						ch->set_skill(SKILL_CREATE_POTION, MAX(10, ch->get_skill(SKILL_CREATE_POTION)));
					}
					if (IS_SET(bits, SPELL_WAND))
					{
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_WAND);
						ch->set_skill(SKILL_CREATE_WAND, MAX(10, ch->get_skill(SKILL_CREATE_WAND)));
					}
					if (IS_SET(bits, SPELL_SCROLL))
					{
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_SCROLL);
						ch->set_skill(SKILL_CREATE_SCROLL, MAX(10, ch->get_skill(SKILL_CREATE_SCROLL)));
					}
					found = TRUE;
				}

				const int feat_no = (guild_mono_info[info_num].learn_info + i)->feat_no;
				if (feat_no > 0
					&& feat_no < MAX_FEATS)
				{
					if (!HAVE_FEAT(ch, feat_no) && can_get_feat(ch, feat_no))
					{
						sfound = TRUE;
					}
				}
			}

			if (sfound)
			{
				act("$N сказал$G : \r\n"
					"'$n, к сожалению, это может сильно затруднить твое постижение умений.'\r\n"
					"'Выбери лишь самые необходимые тебе умения.'", FALSE, ch, 0, victim, TO_CHAR);
			}

			if (!found)
			{
				act("$N ничему новому вас не научил$G.", FALSE, ch, 0, victim, TO_CHAR);
			}

			return (1);
		}

		const int feat_no = find_feat_num(argument);
		if ((feat_no > 0 && feat_no < MAX_FEATS))
		{
			for (i = 0, found = FALSE; (guild_mono_info[info_num].learn_info + i)->feat_no >= 0; i++)
			{
				if ((guild_mono_info[info_num].learn_info + i)->level > GET_LEVEL(ch))
				{
					continue;
				}

				if (feat_no == (guild_mono_info[info_num].learn_info + i)->feat_no)
				{
					if (HAVE_FEAT(ch, feat_no))
					{
						act("$N сказал$g вам : 'Ничем помочь не могу, ты уже владеешь этой способностью.'", FALSE, ch, 0, victim, TO_CHAR);
					}
					else if (!can_get_feat(ch, feat_no))
					{
						act("$N сказал$G : 'Я не могу тебя этому научить.'", FALSE, ch, 0, victim, TO_CHAR);
					}
					else
					{
						sprintf(buf, "$N научил$G вас способности %s\"%s\"%s",
							CCCYN(ch, C_NRM), feat_name(feat_no), CCNRM(ch, C_NRM));
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
						SET_FEAT(ch, feat_no);
					}
					found = TRUE;
				}
			}

			if (!found)
			{
				act("$N сказал$G : 'Мне не ведомо такое мастерство.'", FALSE, ch, 0, victim, TO_CHAR);
			}

			return (1);
		}

		const ESkill skill_no = fix_name_and_find_skill_num(argument);
		if ((SKILL_INVALID != skill_no
			&& skill_no <= MAX_SKILL_NUM))
		{
			for (i = 0, found = FALSE; (guild_mono_info[info_num].learn_info + i)->spell_no >= 0; i++)
			{
				if ((guild_mono_info[info_num].learn_info + i)->level > GET_LEVEL(ch))
				{
					continue;
				}

				if (skill_no == (guild_mono_info[info_num].learn_info + i)->skill_no)
				{
					if (ch->get_skill(skill_no))
					{
						act("$N сказал$g вам : 'Ничем помочь не могу, ты уже владеешь этим умением.'", FALSE, ch, 0, victim, TO_CHAR);
					}
					else if (!can_get_skill(ch, skill_no))
					{
						act("$N сказал$G : 'Я не могу тебя этому научить.'", FALSE, ch, 0, victim, TO_CHAR);
					}
					else
					{
						sprintf(buf, "$N научил$G вас умению %s\"%s\"%s",
							CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
						ch->set_skill(skill_no, 10);
					}
					found = TRUE;
				}
			}

			if (!found)
			{
				act("$N сказал$G : 'Мне не ведомо такое умение.'", FALSE, ch, 0, victim, TO_CHAR);
			}

			return (1);
		}

		const int spell_no = fix_name_and_find_spell_num(argument);
		if (spell_no > 0
			&& spell_no <= MAX_SPELLS)
		{
			for (i = 0, found = FALSE; (guild_mono_info[info_num].learn_info + i)->spell_no >= 0; i++)
			{
				if ((guild_mono_info[info_num].learn_info + i)->level > GET_LEVEL(ch))
				{
					continue;
				}

				if (spell_no == (guild_mono_info[info_num].learn_info + i)->spell_no)
				{
					if (!(bits = -2 * (guild_mono_info[info_num].learn_info + i)->skill_no)
						|| bits == SPELL_TEMP)
					{
						bits = SPELL_KNOW;
					}

					if ((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)
					{
						if (IS_SET(bits, SPELL_KNOW))
						{
							sprintf(buf, "$N сказал$G : \r\n"
								"'$n, чтобы применить это заклинание, тебе необходимо набрать\r\n"
								"%s колд(овать) '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
								CCCYN(ch, C_NRM), spell_name(spell_no),
								CCNRM(ch, C_NRM));
						}
						else if (IS_SET(bits, SPELL_ITEMS))
						{
							sprintf(buf, "$N сказал$G : \r\n"
								"'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
								"%s смешать '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
								CCCYN(ch, C_NRM), spell_name(spell_no),
								CCNRM(ch, C_NRM));
						}
						else if (IS_SET(bits, SPELL_RUNES))
						{
							sprintf(buf, "$N сказал$G : \r\n"
								"'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
								"%s руны '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
								CCCYN(ch, C_NRM), spell_name(spell_no),
								CCNRM(ch, C_NRM));
						}
						else
						{
							strcpy(buf, "Вы уже умеете это.");
						}
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
					}
					else
					{
						sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
							CCCYN(ch, C_NRM), spell_name(spell_no), CCNRM(ch, C_NRM));
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
						if (IS_SET(bits, SPELL_KNOW))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_KNOW);
						}
						if (IS_SET(bits, SPELL_ITEMS))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_ITEMS);
						}
						if (IS_SET(bits, SPELL_RUNES))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_RUNES);
						}
						if (IS_SET(bits, SPELL_POTION))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_POTION);
							ch->set_skill(SKILL_CREATE_POTION, MAX(10, ch->get_skill(SKILL_CREATE_POTION)));
						}
						if (IS_SET(bits, SPELL_WAND))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_WAND);
							ch->set_skill(SKILL_CREATE_WAND, MAX(10, ch->get_skill(SKILL_CREATE_WAND)));
						}
						if (IS_SET(bits, SPELL_SCROLL))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_SCROLL);
							ch->set_skill(SKILL_CREATE_SCROLL, MAX(10, ch->get_skill(SKILL_CREATE_SCROLL)));
						}
					}
					found = TRUE;
				}
			}

			if (!found)
			{
				act("$N сказал$G : 'Я и сам$G не знаю такой магии.'", FALSE, ch, 0, victim, TO_CHAR);
			}

			return (1);
		}

		act("$N сказал$G вам : 'Я никогда и никого этому не учил$G.'", FALSE, ch, 0, victim, TO_CHAR);
		return (1);
	}

	return (0);
}

int guild_poly(CHAR_DATA *ch, void *me, int cmd, char* argument)
{
	int command = 0, gcount = 0, info_num = 0, found = FALSE, sfound = FALSE, i, bits;
	CHAR_DATA *victim = (CHAR_DATA *) me;

	if (IS_NPC(ch))
	{
		return 0;
	}

	if (CMD_IS("учить") || CMD_IS("practice"))
	{
		command = SCMD_LEARN;
	}
	else
	{
		return 0;
	}

	if ((info_num = mob_index[victim->get_rnum()].stored) <= 0 || info_num > GUILDS_POLY_USED)
	{
		act("$N сказал$G : 'Извини, $n, я уже в отставке.'", FALSE, ch, 0, victim, TO_CHAR);
		return 1;
	}

	info_num--;

	skip_spaces(&argument);

	switch (command)
	{
	case SCMD_LEARN:
		if (!*argument)
		{
			gcount += sprintf(buf, "Я могу научить тебя следующему:\r\n");
			for (i = 0, found = FALSE; (guild_poly_info[info_num] + i)->spell_no >= 0; i++)
			{
				if ((guild_poly_info[info_num] + i)->level > GET_LEVEL(ch))
				{
					continue;
				}

				if (!IS_BITS((guild_poly_info[info_num] + i)->classes, GET_CLASS(ch))
					|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
					|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch)))
					continue;

				const ESkill skill_no = (guild_poly_info[info_num] + i)->skill_no;
				bits = skill_no;
				if (SKILL_INVALID != skill_no
					&& (!ch->get_skill(skill_no)
						|| IS_GRGOD(ch))
					&& can_get_skill(ch, skill_no))
				{
					gcount += sprintf(buf + gcount, "- умение %s\"%s\"%s\r\n",
						CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
					found = TRUE;
				}

				if (!(bits = -2 * bits)
					|| bits == SPELL_TEMP)
				{
					bits = SPELL_KNOW;
				}

				const int spell_no = (guild_poly_info[info_num] + i)->spell_no;
				if (spell_no
					&& (!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)
						|| IS_GRGOD(ch)))
				{
					gcount += sprintf(buf + gcount, "- магия %s\"%s\"%s\r\n",
						CCCYN(ch, C_NRM), spell_name(spell_no), CCNRM(ch, C_NRM));
					found = TRUE;
				}

				const int feat_no = (guild_poly_info[info_num] + i)->feat_no;
				if (feat_no > 0
					&& feat_no < MAX_FEATS)
				{
					if (!HAVE_FEAT(ch, feat_no) && can_get_feat(ch, feat_no))
					{
						gcount += sprintf(buf + gcount, "- способность %s\"%s\"%s\r\n",
							CCCYN(ch, C_NRM), feat_name(feat_no), CCNRM(ch, C_NRM));
						found = TRUE;
					}
				}
			}

			if (!found)
			{
				act("$N сказал$G : 'Похоже, я не смогу тебе помочь.'", FALSE, ch, 0, victim, TO_CHAR);
				return (1);
			}
			else
			{
				send_to_char(buf, ch);
				return (1);
			}
		}

		if (!strn_cmp(argument, "все", strlen(argument))
			|| !strn_cmp(argument, "all", strlen(argument)))
		{
			for (i = 0, found = FALSE, sfound = FALSE; (guild_poly_info[info_num] + i)->spell_no >= 0; i++)
			{
				if ((guild_poly_info[info_num] + i)->level > GET_LEVEL(ch))
				{
					continue;
				}
				if (!IS_BITS((guild_poly_info[info_num] + i)->classes, GET_CLASS(ch))
					|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
					|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch)))
				{
					continue;
				}

				const ESkill skill_no = (guild_poly_info[info_num] + i)->skill_no;
				bits = skill_no;
				if (SKILL_INVALID != skill_no
					&& !ch->get_skill(skill_no))  	// sprintf(buf, "$N научил$G вас умению %s\"%s\"\%s",
				{
					//             CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
					// act(buf,FALSE,ch,0,victim,TO_CHAR);
					// ch->get_skill(skill_no) = 10;
					sfound = TRUE;
				}

				const int feat_no = (guild_poly_info[info_num] + i)->feat_no;
				if (feat_no > 0
					&& feat_no < MAX_FEATS)
				{
					if (!HAVE_FEAT(ch, feat_no)
						&& can_get_feat(ch, feat_no))
					{
						sfound = TRUE;
					}
				}

				if (!(bits = -2 * bits) || bits == SPELL_TEMP)
				{
					bits = SPELL_KNOW;
				}

				const int spell_no = (guild_poly_info[info_num] + i)->spell_no;
				if (spell_no
					&& !((GET_SPELL_TYPE(ch, spell_no) & bits) == bits))
				{
					gcount += sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
						CCCYN(ch, C_NRM), spell_name(spell_no), CCNRM(ch, C_NRM));
					act(buf, FALSE, ch, 0, victim, TO_CHAR);

					if (IS_SET(bits, SPELL_KNOW))
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_KNOW);
					if (IS_SET(bits, SPELL_ITEMS))
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_ITEMS);
					if (IS_SET(bits, SPELL_RUNES))
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_RUNES);
					if (IS_SET(bits, SPELL_POTION))
					{
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_POTION);
						ch->set_skill(SKILL_CREATE_POTION, MAX(10, ch->get_skill(SKILL_CREATE_POTION)));
					}
					if (IS_SET(bits, SPELL_WAND))
					{
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_WAND);
						ch->set_skill(SKILL_CREATE_WAND, MAX(10, ch->get_skill(SKILL_CREATE_WAND)));
					}
					if (IS_SET(bits, SPELL_SCROLL))
					{
						SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_SCROLL);
						ch->set_skill(SKILL_CREATE_SCROLL, MAX(10, ch->get_skill(SKILL_CREATE_SCROLL)));
					}
					found = TRUE;
				}
			}

			if (sfound)
			{
				act("$N сказал$G : \r\n"
					"'$n, к сожалению, это может сильно затруднить твое постижение умений.'\r\n"
					"'Выбери лишь самые необходимые тебе умения.'", FALSE, ch, 0, victim, TO_CHAR);
			}

			if (!found)
			{
				act("$N ничему новому вас не научил$G.", FALSE, ch, 0, victim, TO_CHAR);
			}

			return (1);
		}

		const ESkill skill_no = fix_name_and_find_skill_num(argument);
		if (SKILL_INVALID != skill_no
			&& skill_no <= MAX_SKILL_NUM)
		{
			for (i = 0, found = FALSE; (guild_poly_info[info_num] + i)->spell_no >= 0; i++)
			{
				if ((guild_poly_info[info_num] + i)->level > GET_LEVEL(ch))
				{
					continue;
				}
				if (!IS_BITS((guild_poly_info[info_num] + i)->classes, GET_CLASS(ch))
					|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
					|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch)))
				{
					continue;
				}

				if (skill_no == (guild_poly_info[info_num] + i)->skill_no)
				{
					if (ch->get_skill(skill_no))
					{
						act("$N сказал$G вам : 'Ты уже владеешь этим умением.'", FALSE, ch, 0, victim, TO_CHAR);
					}
					else if (!can_get_skill(ch, skill_no))
					{
						act("$N сказал$G : 'Я не могу тебя этому научить.'", FALSE, ch, 0, victim, TO_CHAR);
					}
					else
					{
						sprintf(buf, "$N научил$G вас умению %s\"%s\"%s",
							CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
						ch->set_skill(skill_no, 10);
					}
					found = TRUE;
				}
			}

			if (!found)
			{
				//				found_skill = FALSE;
				//				act("$N сказал$G : 'Мне не ведомо такое умение.'", FALSE, ch, 0, victim, TO_CHAR);
			}
			else
			{
				return (1);
			}
		}

		int feat_no = find_feat_num(argument);
		if (feat_no < 0 || feat_no >= MAX_FEATS)
		{
			std::string str(argument);
			std::replace_if(str.begin(), str.end(), boost::is_any_of("_:"), ' ');
			feat_no = find_feat_num(str.c_str(), true);
		}

		if (feat_no > 0 && feat_no < MAX_FEATS)
		{
			for (i = 0, found = FALSE; (guild_poly_info[info_num] + i)->feat_no >= 0; i++)
			{
				if ((guild_poly_info[info_num] + i)->level > GET_LEVEL(ch))
				{
					continue;
				}
				if (!IS_BITS((guild_poly_info[info_num] + i)->classes, GET_CLASS(ch))
					|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
					|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch)))
				{
					continue;
				}
				if (feat_no == (guild_poly_info[info_num] + i)->feat_no)
				{
					if (HAVE_FEAT(ch, feat_no))
					{
						act("$N сказал$G вам : 'Ничем помочь не могу, ты уже владеешь этой способностью.'", FALSE, ch, 0, victim, TO_CHAR);
					}
					else if (!can_get_feat(ch, feat_no))
					{
						act("$N сказал$G : 'Я не могу тебя этому научить.'", FALSE, ch, 0, victim, TO_CHAR);
					}
					else
					{
						sprintf(buf, "$N научил$G вас способности %s\"%s\"%s",
							CCCYN(ch, C_NRM), feat_name(feat_no), CCNRM(ch, C_NRM));
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
						SET_FEAT(ch, feat_no);
					}
					found = TRUE;
				}
			}

			if (!found)
			{
				//				found_feat = FALSE;
				//				act("$N сказал$G : 'Мне не ведомо такое умение.'", FALSE, ch, 0, victim, TO_CHAR);
			}
			else
			{
				return (1);
			}
		}

		const int spell_no = fix_name_and_find_spell_num(argument);
		if (spell_no > 0 && spell_no <= MAX_SPELLS)
		{
			for (i = 0, found = FALSE; (guild_poly_info[info_num] + i)->spell_no >= 0; i++)
			{
				if ((guild_poly_info[info_num] + i)->level > GET_LEVEL(ch))
				{
					continue;
				}
				if (!(bits = -2 * (guild_poly_info[info_num] + i)->skill_no)
					|| bits == SPELL_TEMP)
				{
					bits = SPELL_KNOW;
				}
				if (!IS_BITS((guild_poly_info[info_num] + i)->classes, GET_CLASS(ch))
					|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
					|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch)))
				{
					continue;
				}

				if (spell_no == (guild_poly_info[info_num] + i)->spell_no)
				{
					if ((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)
					{
						if (IS_SET(bits, SPELL_KNOW))
						{
							sprintf(buf, "$N сказал$G : \r\n"
								"'$n, чтобы применить это заклинание, тебе необходимо набрать\r\n"
								"%s колд(овать) '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
								CCCYN(ch, C_NRM), spell_name(skill_no),
								CCNRM(ch, C_NRM));
						}
						else if (IS_SET(bits, SPELL_ITEMS))
						{
							sprintf(buf, "$N сказал$G : \r\n"
								"'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
								"%s смешать '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
								CCCYN(ch, C_NRM), spell_name(skill_no),
								CCNRM(ch, C_NRM));
						}
						else if (IS_SET(bits, SPELL_RUNES))
						{
							sprintf(buf, "$N сказал$G : \r\n"
								"'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
								"%s руны '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
								CCCYN(ch, C_NRM), spell_name(skill_no),
								CCNRM(ch, C_NRM));
						}
						else
						{
							strcpy(buf, "Вы уже умеете это.");
						}
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
					}
					else
					{
						sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
							CCCYN(ch, C_NRM), spell_name(spell_no), CCNRM(ch, C_NRM));
						act(buf, FALSE, ch, 0, victim, TO_CHAR);
						if (IS_SET(bits, SPELL_KNOW))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_KNOW);
						}
						if (IS_SET(bits, SPELL_ITEMS))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_ITEMS);
						}
						if (IS_SET(bits, SPELL_RUNES))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_RUNES);
						}
						if (IS_SET(bits, SPELL_POTION))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_POTION);
							ch->set_skill(SKILL_CREATE_POTION, MAX(10, ch->get_skill(SKILL_CREATE_POTION)));
						}
						if (IS_SET(bits, SPELL_WAND))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_WAND);
							ch->set_skill(SKILL_CREATE_WAND, MAX(10, ch->get_skill(SKILL_CREATE_WAND)));
						}
						if (IS_SET(bits, SPELL_SCROLL))
						{
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), SPELL_SCROLL);
							ch->set_skill(SKILL_CREATE_SCROLL, MAX(10, ch->get_skill(SKILL_CREATE_SCROLL)));
						}
					}
					found = TRUE;
				}
			}

			if (!found)
			{
				//				found_spell = FALSE;
				//				act("$N сказал$G : 'Я и сам$G не знаю такого заклинания.'", FALSE, ch, 0, victim, TO_CHAR);
			}
			else
			{
				return (1);
			}
		}

		act("$N сказал$G вам : 'Я никогда и никого этому не учил$G.'", FALSE, ch, 0, victim, TO_CHAR);
		return (1);
	}

	return (0);
}

int horse_keeper(CHAR_DATA *ch, void *me, int cmd, char* argument)
{
	CHAR_DATA *victim = (CHAR_DATA *) me, *horse = NULL;

	if (IS_NPC(ch))
		return (0);

	if (!CMD_IS("лошадь") && !CMD_IS("horse"))
		return (0);

	if (ch->is_morphed())
	{
		send_to_char("Лошадка испугается вас в таком виде... \r\n", ch);
		return (TRUE);
	}

	skip_spaces(&argument);

	if (!*argument)
	{
		if (has_horse(ch, FALSE))
		{
			act("$N поинтересовал$U : \"$n, зачем тебе второй скакун? У тебя ведь одно седалище.\"",
				FALSE, ch, 0, victim, TO_CHAR);
			return (TRUE);
		}
		sprintf(buf, "$N сказал$G : \"Я продам тебе скакуна за %d %s.\"",
				HORSE_COST, desc_count(HORSE_COST, WHAT_MONEYa));
		act(buf, FALSE, ch, 0, victim, TO_CHAR);
		return (TRUE);
	}

	if (!strn_cmp(argument, "купить", strlen(argument)) || !strn_cmp(argument, "buy", strlen(argument)))
	{
		if (has_horse(ch, FALSE))
		{
			act("$N засмеял$U : \"$n, ты шутишь, у тебя же есть скакун.\"", FALSE, ch, 0, victim, TO_CHAR);
			return (TRUE);
		}
		if (ch->get_gold() < HORSE_COST)
		{
			act("\"Ступай отсюда, злыдень, у тебя нет таких денег!\"-заорал$G $N",
				FALSE, ch, 0, victim, TO_CHAR);
			return (TRUE);
		}
		if (!(horse = read_mobile(HORSE_VNUM, VIRTUAL)))
		{
			act("\"Извини, у меня нет для тебя скакуна.\"-смущенно произнес$Q $N",
				FALSE, ch, 0, victim, TO_CHAR);
			return (TRUE);
		}
		make_horse(horse, ch);
		char_to_room(horse, ch->in_room);
		sprintf(buf, "$N оседлал$G %s и отдал$G %s вам.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, FALSE, ch, 0, victim, TO_CHAR);
		sprintf(buf, "$N оседлал$G %s и отдал$G %s $n2.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, FALSE, ch, 0, victim, TO_ROOM);
		ch->remove_gold(HORSE_COST);
		PLR_FLAGS(ch).set(PLR_CRASH);
		return (TRUE);
	}


	if (!strn_cmp(argument, "продать", strlen(argument)) || !strn_cmp(argument, "sell", strlen(argument)))
	{
		if (!has_horse(ch, TRUE))
		{
			act("$N засмеял$U : \"$n, ты не влезешь в мое стойло.\"", FALSE, ch, 0, victim, TO_CHAR);
			return (TRUE);
		}
		if (on_horse(ch))
		{
			act("\"Я не собираюсь платить еще и за всадника.\"-усмехнул$U $N",
				FALSE, ch, 0, victim, TO_CHAR);
			return (TRUE);
		}

		if (!(horse = get_horse(ch)) || GET_MOB_VNUM(horse) != HORSE_VNUM)
		{
			act("\"Извини, твой скакун мне не подходит.\"- заявил$G $N", FALSE, ch, 0, victim, TO_CHAR);
			return (TRUE);
		}

		if (IN_ROOM(horse) != IN_ROOM(victim))
		{
			act("\"Извини, твой скакун где-то бродит.\"- заявил$G $N", FALSE, ch, 0, victim, TO_CHAR);
			return (TRUE);
		}

		sprintf(buf, "$N расседлал$G %s и отвел$G %s в стойло.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, FALSE, ch, 0, victim, TO_CHAR);
		sprintf(buf, "$N расседлал$G %s и отвел$G %s в стойло.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, FALSE, ch, 0, victim, TO_ROOM);
		extract_char(horse, FALSE);
		ch->add_gold((HORSE_COST >> 1));
		PLR_FLAGS(ch).set(PLR_CRASH);
		return (TRUE);
	}

	return (0);
}

int check_room_tracks(const room_rnum room, const long victim_id)
{
	for (auto track = world[room]->track; track; track = track->next)
	{
		if (track->who == victim_id)
		{
			for (int i = 0; i < NUM_OF_DIRS; i++)
			{
				if (IS_SET(track->time_outgone[i], 7))
				{
					return i;
				}
			}
		}
	}

	return BFS_ERROR;
}

int find_door(CHAR_DATA* ch, const bool track_method)
{
	bool msg = false;

	for (const auto& vict : character_list)
	{
		if (CAN_SEE(ch, vict) && IN_ROOM(vict) != NOWHERE)
		{
			for (auto names = MEMORY(ch); names; names = names->next)
			{
				if (GET_IDNUM(vict) == names->id
					&& (!MOB_FLAGGED(ch, MOB_STAY_ZONE)
						|| world[ch->in_room]->zone == world[IN_ROOM(vict)]->zone))
				{
					if (!msg)
					{
						msg = true;
						act("$n начал$g внимательно искать чьи-то следы.", FALSE, ch, 0, 0, TO_ROOM);
					}

					const auto door = track_method
						? check_room_tracks(ch->in_room, GET_IDNUM(vict))
						: go_track(ch, vict.get(), SKILL_TRACK);

					if (BFS_ERROR != door)
					{
						return door;
					}
				}
			}
		}
	}

	return BFS_ERROR;
}

int npc_track(CHAR_DATA * ch)
{
	const auto result = find_door(ch, GET_REAL_INT(ch) < number(15, 20));

	return result;
}

bool item_nouse(OBJ_DATA * obj)
{
	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_LIGHT:
		if (GET_OBJ_VAL(obj, 2) == 0)
		{
			return true;
		}
		break;

	case OBJ_DATA::ITEM_SCROLL:
	case OBJ_DATA::ITEM_POTION:
		if (!GET_OBJ_VAL(obj, 1)
			&& !GET_OBJ_VAL(obj, 2)
			&& !GET_OBJ_VAL(obj, 3))
		{
			return true;
		}
		break;

	case OBJ_DATA::ITEM_STAFF:
	case OBJ_DATA::ITEM_WAND:
		if (!GET_OBJ_VAL(obj, 2))
		{
			return true;
		}
		break;

	case OBJ_DATA::ITEM_CONTAINER:
		if (!system_obj::is_purse(obj))
		{
			return true;
		}
		break;

	case OBJ_DATA::ITEM_OTHER:
	case OBJ_DATA::ITEM_TRASH:
	case OBJ_DATA::ITEM_TRAP:
	case OBJ_DATA::ITEM_NOTE:
	case OBJ_DATA::ITEM_DRINKCON:
	case OBJ_DATA::ITEM_FOOD:
	case OBJ_DATA::ITEM_PEN:
	case OBJ_DATA::ITEM_BOAT:
	case OBJ_DATA::ITEM_FOUNTAIN:
	case OBJ_DATA::ITEM_MING:
		return true;

	default:
		break;
	}

	return false;
}

void npc_dropunuse(CHAR_DATA * ch)
{
	OBJ_DATA *obj, *nobj;
	for (obj = ch->carrying; obj; obj = nobj)
	{
		nobj = obj->get_next_content();
		if (item_nouse(obj))
		{
			act("$n выбросил$g $o3.", FALSE, ch, obj, 0, TO_ROOM);
			obj_from_char(obj);
			obj_to_room(obj, ch->in_room);
		}
	}
}

int npc_scavenge(CHAR_DATA * ch)
{
	int max = 1;
	OBJ_DATA *obj, *best_obj, *cont, *best_cont, *cobj;

	if (!MOB_FLAGGED(ch, MOB_SCAVENGER))
	{
		return (FALSE);
	}

	if (IS_SHOPKEEPER(ch))
	{
		return (FALSE);
	}

	npc_dropunuse(ch);
	if (world[ch->in_room]->contents && number(0, 25) <= GET_REAL_INT(ch))
	{
		max = 1;
		best_obj = NULL;
		cont = NULL;
		best_cont = NULL;
		for (obj = world[ch->in_room]->contents; obj; obj = obj->get_next_content())
		{
			if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_MING
				|| Clan::is_clan_chest(obj)
				|| ClanSystem::is_ingr_chest(obj))
			{
				continue;
			}

			if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_CONTAINER
				&& !system_obj::is_purse(obj))
			{
				if (IS_CORPSE(obj))
				{
					continue;
				}

				// Заперто, открываем, если есть ключ
				if (OBJVAL_FLAGGED(obj, CONT_LOCKED)
					&& has_key(ch, GET_OBJ_VAL(obj, 2)))
				{
					do_doorcmd(ch, obj, 0, SCMD_UNLOCK);
				}

				// Заперто, взламываем, если умеем
				if (OBJVAL_FLAGGED(obj, CONT_LOCKED)
					&& ch->get_skill(SKILL_PICK_LOCK)
					&& ok_pick(ch, 0, obj, 0, SCMD_PICK))
				{
					do_doorcmd(ch, obj, 0, SCMD_PICK);
				}
				// Все равно заперто, ну тогда фиг с ним
				if (OBJVAL_FLAGGED(obj, CONT_LOCKED))
				{
					continue;
				}

				if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
				{
					do_doorcmd(ch, obj, 0, SCMD_OPEN);
				}

				if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
				{
					continue;
				}

				for (cobj = obj->get_contains(); cobj; cobj = cobj->get_next_content())
				{
					if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj) && GET_OBJ_COST(cobj) > max)
					{
						cont = obj;
						best_cont = best_obj = cobj;
						max = GET_OBJ_COST(cobj);
					}
				}
			}
			else if (!IS_CORPSE(obj) &&
				CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max && !item_nouse(obj))
			{
				best_obj = obj;
				max = GET_OBJ_COST(obj);
			}
		}
		if (best_obj != NULL)
		{
			if (best_obj != best_cont)
			{
				act("$n поднял$g $o3.", FALSE, ch, best_obj, 0, TO_ROOM);
				if (GET_OBJ_TYPE(best_obj) == OBJ_DATA::ITEM_MONEY)
				{
					ch->add_gold(GET_OBJ_VAL(best_obj, 0));
					extract_obj(best_obj);
				}
				else
				{
					obj_from_room(best_obj);
					obj_to_char(best_obj, ch);
				}
			}
			else
			{
				sprintf(buf, "$n достал$g $o3 из %s.", cont->get_PName(1).c_str());
				act(buf, FALSE, ch, best_obj, 0, TO_ROOM);
				if (GET_OBJ_TYPE(best_obj) == OBJ_DATA::ITEM_MONEY)
				{
					ch->add_gold(GET_OBJ_VAL(best_obj, 0));
					extract_obj(best_obj);
				}
				else
				{
					obj_from_obj(best_obj);
					obj_to_char(best_obj, ch);
				}
			}
		}
	}
	return (max > 1);
}

int npc_loot(CHAR_DATA * ch)
{
	int max = FALSE;
	OBJ_DATA *obj, *loot_obj, *next_loot, *cobj, *cnext_obj;

	if (!MOB_FLAGGED(ch, MOB_LOOTER))
		return (FALSE);
	if (IS_SHOPKEEPER(ch))
		return (FALSE);
	npc_dropunuse(ch);
	if (world[ch->in_room]->contents && number(0, GET_REAL_INT(ch)) > 10)
	{
		for (obj = world[ch->in_room]->contents; obj; obj = obj->get_next_content())
		{
			if (CAN_SEE_OBJ(ch, obj) && IS_CORPSE(obj))
			{
				// Сначала лутим то, что не в контейнерах
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot)
				{
					next_loot = loot_obj->get_next_content();
					if ((GET_OBJ_TYPE(loot_obj) != OBJ_DATA::ITEM_CONTAINER
							|| system_obj::is_purse(loot_obj))
						&& CAN_GET_OBJ(ch, loot_obj)
						&& !item_nouse(loot_obj))
					{
						sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(1).c_str());
						act(buf, FALSE, ch, loot_obj, 0, TO_ROOM);
						if (GET_OBJ_TYPE(loot_obj) == OBJ_DATA::ITEM_MONEY)
						{
							ch->add_gold(GET_OBJ_VAL(loot_obj, 0));
							extract_obj(loot_obj);
						}
						else
						{
							obj_from_obj(loot_obj);
							obj_to_char(loot_obj, ch);
							max++;
						}
					}
				}
				// Теперь не запертые контейнеры
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot)
				{
					next_loot = loot_obj->get_next_content();
					if (GET_OBJ_TYPE(loot_obj) == OBJ_DATA::ITEM_CONTAINER)
					{
						if (IS_CORPSE(loot_obj)
							|| OBJVAL_FLAGGED(loot_obj, CONT_LOCKED)
							|| system_obj::is_purse(loot_obj))
						{
							continue;
						}
						auto value = loot_obj->get_val(1); //откроем контейнер
						REMOVE_BIT(value, CONT_CLOSED);
						loot_obj->set_val(1, value);
						for (cobj = loot_obj->get_contains(); cobj; cobj = cnext_obj)
						{
							cnext_obj = cobj->get_next_content();
							if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj))
							{
								sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(1).c_str());
								act(buf, FALSE, ch, cobj, 0, TO_ROOM);
								if (GET_OBJ_TYPE(cobj) == OBJ_DATA::ITEM_MONEY)
								{
									ch->add_gold(GET_OBJ_VAL(cobj, 0));
									extract_obj(cobj);
								}
								else
								{
									obj_from_obj(cobj);
									obj_to_char(cobj, ch);
									max++;
								}
							}
						}
					}
				}
				// И наконец, лутим запертые контейнеры если есть ключ или можем взломать
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot)
				{
					next_loot = loot_obj->get_next_content();
					if (GET_OBJ_TYPE(loot_obj) == OBJ_DATA::ITEM_CONTAINER)
					{
						if (IS_CORPSE(loot_obj)
							|| !OBJVAL_FLAGGED(loot_obj, CONT_LOCKED)
							|| system_obj::is_purse(loot_obj))
						{
							continue;
						}

						// Есть ключ?
						if (OBJVAL_FLAGGED(loot_obj, CONT_LOCKED)
							&& has_key(ch, GET_OBJ_VAL(loot_obj, 2)))
						{
							loot_obj->toggle_val_bit(1, CONT_LOCKED);
						}

						// ...или взломаем?
						if (OBJVAL_FLAGGED(loot_obj, CONT_LOCKED)
							&& ch->get_skill(SKILL_PICK_LOCK)
							&& ok_pick(ch, 0, loot_obj, 0, SCMD_PICK))
						{
							loot_obj->toggle_val_bit(1, CONT_LOCKED);
						}

						// Эх, не открыть. Ну ладно.
						if (OBJVAL_FLAGGED(loot_obj, CONT_LOCKED))
						{
							continue;
						}

						loot_obj->toggle_val_bit(1, CONT_CLOSED);
						for (cobj = loot_obj->get_contains(); cobj; cobj = cnext_obj)
						{
							cnext_obj = cobj->get_next_content();
							if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj))
							{
								sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(1).c_str());
								act(buf, FALSE, ch, cobj, 0, TO_ROOM);
								if (GET_OBJ_TYPE(cobj) == OBJ_DATA::ITEM_MONEY)
								{
									ch->add_gold(GET_OBJ_VAL(cobj, 0));
									extract_obj(cobj);
								}
								else
								{
									obj_from_obj(cobj);
									obj_to_char(cobj, ch);
									max++;
								}
							}
						}
					}
				}
			}
		}
	}
	return (max);
}

int npc_move(CHAR_DATA * ch, int dir, int/* need_specials_check*/)
{
	int need_close = FALSE, need_lock = FALSE;
	int rev_dir[] = { SOUTH, WEST, NORTH, EAST, DOWN, UP };
	int retval = FALSE;

	if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || ch->get_fighting())
	{
		return (FALSE);
	}
	else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room() == NOWHERE)
	{
		return (FALSE);
	}
	else if (ch->has_master()
		&& ch->in_room == IN_ROOM(ch->get_master()))
	{
		return (FALSE);
	}
	else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))
	{
		if (!EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR))
		{
			return (FALSE);
		}

		const auto& rdata = EXIT(ch, dir);

		if (EXIT_FLAGGED(rdata, EX_LOCKED))
		{
			if (has_key(ch, rdata->key)
				|| (!EXIT_FLAGGED(rdata, EX_PICKPROOF)
					&& !EXIT_FLAGGED(rdata, EX_BROKEN)
					&& calculate_skill(ch, SKILL_PICK, 0) >= number(0, 100)))
			{
				do_doorcmd(ch, 0, dir, SCMD_UNLOCK);
				need_lock = TRUE;
			}
			else
			{
				return (FALSE);
			}
		}
		if (EXIT_FLAGGED(rdata, EX_CLOSED))
		{
			if (GET_REAL_INT(ch) >= 15
				|| GET_DEST(ch) != NOWHERE
				|| MOB_FLAGGED(ch, MOB_OPENDOOR))
			{
				do_doorcmd(ch, 0, dir, SCMD_OPEN);
				need_close = TRUE;
			}
		}
	}

	retval = perform_move(ch, dir, 1, FALSE, 0);

	if (need_close)
	{
		if (retval)
			do_doorcmd(ch, 0, rev_dir[dir], SCMD_CLOSE);
		else
			do_doorcmd(ch, 0, dir, SCMD_CLOSE);
	}

	if (need_lock)
	{
		if (retval)
			do_doorcmd(ch, 0, rev_dir[dir], SCMD_LOCK);
		else
			do_doorcmd(ch, 0, dir, SCMD_LOCK);
	}

	return (retval);
}

int has_curse(OBJ_DATA * obj)
{
	for (const auto& i : weapon_affect)
	{
		// Замена выражения на макрос
		if (i.aff_spell <= 0 || !IS_OBJ_AFF(obj, i.aff_pos))
		{
			continue;
		}
		if (IS_SET(spell_info[i.aff_spell].routines, NPC_AFFECT_PC | NPC_DAMAGE_PC))
		{
			return TRUE;
		}
	}
	return FALSE;
}

int calculate_weapon_class(CHAR_DATA * ch, OBJ_DATA * weapon)
{
	int damage = 0, hits = 0, i;

	if (!weapon
		|| GET_OBJ_TYPE(weapon) != OBJ_DATA::ITEM_WEAPON)
	{
		return 0;
	}

	hits = calculate_skill(ch, static_cast<ESkill>(GET_OBJ_SKILL(weapon)), 0);
	damage = (GET_OBJ_VAL(weapon, 1) + 1) * (GET_OBJ_VAL(weapon, 2)) / 2;
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		auto& affected = weapon->get_affected(i);
		if (affected.location == APPLY_DAMROLL)
		{
			damage += affected.modifier;
		}

		if (affected.location == APPLY_HITROLL)
		{
			hits += affected.modifier * 10;
		}
	}

	if (has_curse(weapon))
		return (0);

	return (damage + (hits > 200 ? 10 : hits / 20));
}

void best_weapon(CHAR_DATA * ch, OBJ_DATA * sweapon, OBJ_DATA ** dweapon)
{
	if (*dweapon == NULL)
	{
		if (calculate_weapon_class(ch, sweapon) > 0)
		{
			*dweapon = sweapon;
		}
	}
	else if (calculate_weapon_class(ch, sweapon) > calculate_weapon_class(ch, *dweapon))
	{
		*dweapon = sweapon;
	}
}

void npc_wield(CHAR_DATA * ch)
{
	OBJ_DATA *obj, *next, *right = NULL, *left = NULL, *both = NULL;

	if (!NPC_FLAGGED(ch, NPC_WIELDING))
		return;

	if (ch->get_skill(SKILL_MIGHTHIT) > 0
		&& ch->get_skill(SKILL_STUPOR) < ch->get_skill(SKILL_MIGHTHIT))
	{
		return;
	}

	if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	if (GET_EQ(ch, WEAR_HOLD)
		&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == OBJ_DATA::ITEM_WEAPON)
	{
		left = GET_EQ(ch, WEAR_HOLD);
	}
	if (GET_EQ(ch, WEAR_WIELD)
		&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == OBJ_DATA::ITEM_WEAPON)
	{
		right = GET_EQ(ch, WEAR_WIELD);
	}
	if (GET_EQ(ch, WEAR_BOTHS)
		&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS)) == OBJ_DATA::ITEM_WEAPON)
	{
		both = GET_EQ(ch, WEAR_BOTHS);
	}

	if (GET_REAL_INT(ch) < 15 && ((left && right) || (both)))
		return;

	for (obj = ch->carrying; obj; obj = next)
	{
		next = obj->get_next_content();
		if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_WEAPON
			|| GET_OBJ_UID(obj) != 0)
		{
			continue;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD)
			&& OK_HELD(ch, obj))
		{
			best_weapon(ch, obj, &left);
		}
		else if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD)
			&& OK_WIELD(ch, obj))
		{
			best_weapon(ch, obj, &right);
		}
		else if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS)
			&& OK_BOTH(ch, obj))
		{
			best_weapon(ch, obj, &both);
		}
	}

	if (both
		&& calculate_weapon_class(ch, both) > calculate_weapon_class(ch, left) + calculate_weapon_class(ch, right))
	{
		if (both == GET_EQ(ch, WEAR_BOTHS))
		{
			return;
		}
		if (GET_EQ(ch, WEAR_BOTHS))
		{
			act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, WEAR_BOTHS), 0, TO_ROOM);
			obj_to_char(unequip_char(ch, WEAR_BOTHS | 0x40), ch);
		}
		if (GET_EQ(ch, WEAR_WIELD))
		{
			act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, WEAR_WIELD), 0, TO_ROOM);
			obj_to_char(unequip_char(ch, WEAR_WIELD | 0x40), ch);
		}
		if (GET_EQ(ch, WEAR_SHIELD))
		{
			act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, WEAR_SHIELD), 0, TO_ROOM);
			obj_to_char(unequip_char(ch, WEAR_SHIELD | 0x40), ch);
		}
		if (GET_EQ(ch, WEAR_HOLD))
		{
			act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, WEAR_HOLD), 0, TO_ROOM);
			obj_to_char(unequip_char(ch, WEAR_HOLD | 0x40), ch);
		}
		//obj_from_char(both);
		equip_char(ch, both, WEAR_BOTHS | 0x100);
	}
	else
	{
		if (left && GET_EQ(ch, WEAR_HOLD) != left)
		{
			if (GET_EQ(ch, WEAR_BOTHS))
			{
				act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, WEAR_BOTHS), 0, TO_ROOM);
				obj_to_char(unequip_char(ch, WEAR_BOTHS | 0x40), ch);
			}
			if (GET_EQ(ch, WEAR_SHIELD))
			{
				act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, WEAR_SHIELD), 0, TO_ROOM);
				obj_to_char(unequip_char(ch, WEAR_SHIELD | 0x40), ch);
			}
			if (GET_EQ(ch, WEAR_HOLD))
			{
				act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, WEAR_HOLD), 0, TO_ROOM);
				obj_to_char(unequip_char(ch, WEAR_HOLD | 0x40), ch);
			}
			//obj_from_char(left);
			equip_char(ch, left, WEAR_HOLD | 0x100);
		}
		if (right && GET_EQ(ch, WEAR_WIELD) != right)
		{
			if (GET_EQ(ch, WEAR_BOTHS))
			{
				act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, WEAR_BOTHS), 0, TO_ROOM);
				obj_to_char(unequip_char(ch, WEAR_BOTHS | 0x40), ch);
			}
			if (GET_EQ(ch, WEAR_WIELD))
			{
				act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, WEAR_WIELD), 0, TO_ROOM);
				obj_to_char(unequip_char(ch, WEAR_WIELD | 0x40), ch);
			}
			//obj_from_char(right);
			equip_char(ch, right, WEAR_WIELD | 0x100);
		}
	}
}

void npc_armor(CHAR_DATA * ch)
{
	OBJ_DATA *obj, *next;
	int where = 0;

	if (!NPC_FLAGGED(ch, NPC_ARMORING))
		return;

	if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	for (obj = ch->carrying; obj; obj = next)
	{
		next = obj->get_next_content();

		if (!ObjSystem::is_armor_type(obj)
			|| !no_bad_affects(obj))
		{
			continue;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FINGER))
		{
			where = WEAR_FINGER_R;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_NECK))
		{
			where = WEAR_NECK_1;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BODY))
		{
			where = WEAR_BODY;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HEAD))
		{
			where = WEAR_HEAD;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_LEGS))
		{
			where = WEAR_LEGS;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FEET))
		{
			where = WEAR_FEET;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HANDS))
		{
			where = WEAR_HANDS;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ARMS))
		{
			where = WEAR_ARMS;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_SHIELD))
		{
			where = WEAR_SHIELD;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ABOUT))
		{
			where = WEAR_ABOUT;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WAIST))
		{
			where = WEAR_WAIST;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_QUIVER))
		{
			where = WEAR_QUIVER;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WRIST))
		{
			where = WEAR_WRIST_R;
		}

		if (!where)
		{
			continue;
		}

		if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) || (where == WEAR_WRIST_R))
		{
			if (GET_EQ(ch, where))
			{
				where++;
			}
		}

		if (where == WEAR_SHIELD && (GET_EQ(ch, WEAR_BOTHS) || GET_EQ(ch, WEAR_HOLD)))
		{
			continue;
		}

		if (GET_EQ(ch, where))
		{
			if (GET_REAL_INT(ch) < 15)
			{
				continue;
			}

			if (GET_OBJ_VAL(obj, 0) + GET_OBJ_VAL(obj, 1) * 3 <= GET_OBJ_VAL(GET_EQ(ch, where), 0) + GET_OBJ_VAL(GET_EQ(ch, where), 1) * 3
				|| has_curse(obj))
			{
				continue;
			}
			act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, where), 0, TO_ROOM);
			obj_to_char(unequip_char(ch, where | 0x40), ch);
		}
		//obj_from_char(obj);
		equip_char(ch, obj, where | 0x100);
		break;
	}
}

void npc_light(CHAR_DATA * ch)
{
	OBJ_DATA *obj, *next;

	if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_INFRAVISION))
		return;

	if ((obj = GET_EQ(ch, WEAR_LIGHT)) && (GET_OBJ_VAL(obj, 2) == 0 || !IS_DARK(ch->in_room)))
	{
		act("$n прекратил$g использовать $o3.", FALSE, ch, obj, 0, TO_ROOM);
		obj_to_char(unequip_char(ch, WEAR_LIGHT | 0x40), ch);
	}

	if (!GET_EQ(ch, WEAR_LIGHT) && IS_DARK(ch->in_room))
	{
		for (obj = ch->carrying; obj; obj = next)
		{
			next = obj->get_next_content();
			if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_LIGHT)
			{
				continue;
			}
			if (GET_OBJ_VAL(obj, 2) == 0)
			{
				act("$n выбросил$g $o3.", FALSE, ch, obj, 0, TO_ROOM);
				obj_from_char(obj);
				obj_to_room(obj, ch->in_room);
				continue;
			}
			//obj_from_char(obj);
			equip_char(ch, obj, WEAR_LIGHT | 0x100);
			return;
		}
	}
}


int npc_battle_scavenge(CHAR_DATA * ch)
{
	int max = FALSE;
	OBJ_DATA *obj, *next_obj = NULL;

	if (!MOB_FLAGGED(ch, MOB_SCAVENGER))
		return (FALSE);

	if (IS_SHOPKEEPER(ch))
		return (FALSE);

	if (world[ch->in_room]->contents && number(0, GET_REAL_INT(ch)) > 10)
		for (obj = world[ch->in_room]->contents; obj; obj = next_obj)
		{
			next_obj = obj->get_next_content();
			if (CAN_GET_OBJ(ch, obj)
				&& !has_curse(obj)
				&& (ObjSystem::is_armor_type(obj)
					|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON))
			{
				obj_from_room(obj);
				obj_to_char(obj, ch);
				act("$n поднял$g $o3.", FALSE, ch, obj, 0, TO_ROOM);
				max = TRUE;
			}
		}
	return (max);
}


int npc_walk(CHAR_DATA * ch)
{
	int rnum, door = BFS_ERROR;

	if (ch->in_room == NOWHERE)
		return (BFS_ERROR);

	if (GET_DEST(ch) == NOWHERE || (rnum = real_room(GET_DEST(ch))) == NOWHERE)
		return (BFS_ERROR);

	// Не разрешаем ходы моба если он ушел в другую зону от маршрута.
	if (world[ch->in_room]->zone != world[rnum]->zone)
		return (BFS_NO_PATH);

	if (ch->in_room == rnum)
	{
		if (ch->mob_specials.dest_count == 1)
			return (BFS_ALREADY_THERE);
		if (ch->mob_specials.dest_pos == ch->mob_specials.dest_count - 1 && ch->mob_specials.dest_dir >= 0)
			ch->mob_specials.dest_dir = -1;
		if (!ch->mob_specials.dest_pos && ch->mob_specials.dest_dir < 0)
			ch->mob_specials.dest_dir = 0;
		ch->mob_specials.dest_pos += ch->mob_specials.dest_dir >= 0 ? 1 : -1;
		if (((rnum = real_room(GET_DEST(ch))) == NOWHERE)
				|| rnum == ch->in_room)
			return (BFS_ERROR);
		else
			return (npc_walk(ch));
	}

	door = find_first_step(ch->in_room, real_room(GET_DEST(ch)), ch);

	return (door);
}

int do_npc_steal(CHAR_DATA * ch, CHAR_DATA * victim)
{
	OBJ_DATA *obj, *best = NULL;
	int gold;
	int max = 0;

	if (!NPC_FLAGGED(ch, NPC_STEALING))
		return (FALSE);

	if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
		return (FALSE);

	if (IS_NPC(victim) || IS_SHOPKEEPER(ch) || victim->get_fighting())
		return (FALSE);

	if (GET_LEVEL(victim) >= LVL_IMMORT)
		return (FALSE);

	if (!CAN_SEE(ch, victim))
		return (FALSE);

	if (AWAKE(victim) && (number(0, MAX(0, GET_LEVEL(ch) - int_app[GET_REAL_INT(victim)].observation)) == 0))
	{
		act("Вы обнаружили руку $n1 в своем кармане.", FALSE, ch, 0, victim, TO_VICT);
		act("$n пытал$u обокрасть $N3.", TRUE, ch, 0, victim, TO_NOTVICT);
	}
	else  		// Steal some gold coins
	{
		gold = (int)((victim->get_gold() * number(1, 10)) / 100);
		if (gold > 0)
		{
			ch->add_gold(gold);
			victim->remove_gold(gold);
		}
		// Steal something from equipment
		if (IS_CARRYING_N(ch) < CAN_CARRY_N(ch) && calculate_skill(ch, SKILL_STEAL, victim)
			>= number(1, 100) - (AWAKE(victim) ? 100 : 0))
		{
			for (obj = victim->carrying; obj; obj = obj->get_next_content())
				if (CAN_SEE_OBJ(ch, obj) && IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)
					<= CAN_CARRY_W(ch) && (!best || GET_OBJ_COST(obj) > GET_OBJ_COST(best)))
					best = obj;
			if (best)
			{
				obj_from_char(best);
				obj_to_char(best, ch);
				max++;
			}
		}
	}
	return (max);
}

int npc_steal(CHAR_DATA * ch)
{
	if (!NPC_FLAGGED(ch, NPC_STEALING))
		return (FALSE);

	if (GET_POS(ch) != POS_STANDING || IS_SHOPKEEPER(ch) || ch->get_fighting())
		return (FALSE);

	for (const auto cons : world[ch->in_room]->people)
	{
		if (!IS_NPC(cons)
			&& !IS_IMMORTAL(cons)
			&& (number(0, GET_REAL_INT(ch)) > 10))
		{
			return (do_npc_steal(ch, cons));
		}
	}

	return FALSE;
}

#define ZONE(ch)  (GET_MOB_VNUM(ch) / 100)
#define GROUP(ch) ((GET_MOB_VNUM(ch) % 100) / 10)

void npc_group(CHAR_DATA * ch)
{
	CHAR_DATA *leader = NULL;
	int zone = ZONE(ch), group = GROUP(ch), members = 0;

	if (GET_DEST(ch) == NOWHERE || ch->in_room == NOWHERE)
		return;

	if (ch->has_master()
		&& ch->in_room == IN_ROOM(ch->get_master()))
	{
		leader = ch->get_master();
	}

	if (!ch->has_master())
	{
		leader = ch;
	}

	if (leader
		&& (AFF_FLAGGED(leader, EAffectFlag::AFF_CHARM)
			|| GET_POS(leader) < POS_SLEEPING))
	{
		leader = NULL;
	}

	// Find leader
	for (const auto vict : world[ch->in_room]->people)
	{
		if (!IS_NPC(vict)
			|| GET_DEST(vict) != GET_DEST(ch)
			|| zone != ZONE(vict)
			|| group != GROUP(vict)
			|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
			|| GET_POS(vict) < POS_SLEEPING)
		{
			continue;
		}

		members++;

		if (!leader
			|| GET_REAL_INT(vict) > GET_REAL_INT(leader))
		{
			leader = vict;
		}
	}

	if (members <= 1)
	{
		if (ch->has_master())
		{
			stop_follower(ch, SF_EMPTY);
		}

		return;
	}

	if (leader->has_master())
	{
		stop_follower(leader, SF_EMPTY);
	}

	// Assign leader
	for (const auto vict : world[ch->in_room]->people)
	{
		if (!IS_NPC(vict)
			|| GET_DEST(vict) != GET_DEST(ch)
			|| zone != ZONE(vict)
			|| group != GROUP(vict)
			|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
			|| GET_POS(vict) < POS_SLEEPING)
		{
			continue;
		}

		if (vict == leader)
		{
			AFF_FLAGS(vict).set(EAffectFlag::AFF_GROUP);
			continue;
		}

		if (!vict->has_master())
		{
			leader->add_follower(vict);
		}
		else if (vict->get_master() != leader)
		{
			stop_follower(vict, SF_EMPTY);
			leader->add_follower(vict);
		}
		AFF_FLAGS(vict).set(EAffectFlag::AFF_GROUP);
	}
}

void npc_groupbattle(CHAR_DATA * ch)
{
	struct follow_type *k;
	CHAR_DATA *tch, *helper;

	if (!IS_NPC(ch)
		|| !ch->get_fighting()
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		|| !ch->has_master()
		|| ch->in_room == NOWHERE
		|| !ch->followers)
	{
		return;
	}

	k = ch->has_master() ? ch->get_master()->followers : ch->followers;
	tch = ch->has_master() ? ch->get_master() : ch;
	for (; k; (k = tch ? k : k->next), tch = NULL)
	{
		helper = tch ? tch : k->follower;
		if (ch->in_room == IN_ROOM(helper)
			&& !helper->get_fighting()
			&& !IS_NPC(helper)
			&& GET_POS(helper) > POS_STUNNED)
		{
			GET_POS(helper) = POS_STANDING;
			set_fighting(helper, ch->get_fighting());
			act("$n вступил$u за $N3.", FALSE, helper, 0, ch, TO_ROOM);
		}
	}
}

int dump(CHAR_DATA *ch, void* /*me*/, int cmd, char* argument)
{
	OBJ_DATA *k;
	int value = 0;

	for (k = world[ch->in_room]->contents; k; k = world[ch->in_room]->contents)
	{
		act("$p рассыпал$U в прах!", FALSE, 0, k, 0, TO_ROOM);
		extract_obj(k);
	}

	if (!CMD_IS("drop") || !CMD_IS("бросить"))
		return (0);

	do_drop(ch, argument, cmd, 0);

	for (k = world[ch->in_room]->contents; k; k = world[ch->in_room]->contents)
	{
		act("$p рассыпал$U в прах!", FALSE, 0, k, 0, TO_ROOM);
		value += MAX(1, MIN(1, GET_OBJ_COST(k) / 10));
		extract_obj(k);
	}

	if (value)
	{
		send_to_char("Боги оценили вашу жертву.\r\n", ch);
		act("$n оценен$y Богами.", TRUE, ch, 0, 0, TO_ROOM);
		if (GET_LEVEL(ch) < 3)
			gain_exp(ch, value);
		else
			ch->add_gold(value);
	}
	return (1);
}

#if 0
void mayor(CHAR_DATA *ch, void *me, int cmd, char* argument)
{
	const char open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
	const char close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

	static const char *path = NULL;
	static int index;
	static bool move = FALSE;

	if (!move)
	{
		if (time_info.hours == 6)
		{
			move = TRUE;
			path = open_path;
			index = 0;
		}
		else if (time_info.hours == 20)
		{
			move = TRUE;
			path = close_path;
			index = 0;
		}
	}
	if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) || (GET_POS(ch) == POS_FIGHTING))
		return (FALSE);

	switch (path[index])
	{
	case '0':
	case '1':
	case '2':
	case '3':
		perform_move(ch, path[index] - '0', 1, FALSE);
		break;

	case 'W':
		GET_POS(ch) = POS_STANDING;
		act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'S':
		GET_POS(ch) = POS_SLEEPING;
		act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'a':
		act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
		act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'b':
		act("$n says 'What a view!  I must get something done about that dump!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'c':
		act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'd':
		act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'e':
		act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'E':
		act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
		break;

	case 'O':
		do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
		do_gen_door(ch, "gate", 0, SCMD_OPEN);
		break;

	case 'C':
		do_gen_door(ch, "gate", 0, SCMD_CLOSE);
		do_gen_door(ch, "gate", 0, SCMD_LOCK);
		break;

	case '.':
		move = FALSE;
		break;

	}

	index++;
	return (FALSE);
}
#endif

// ********************************************************************
// *  General special procedures for mobiles                          *
// ********************************************************************

int snake(CHAR_DATA *ch, void* /*me*/, int cmd, char* /*argument*/)
{
	if (cmd)
		return (FALSE);

	if (GET_POS(ch) != POS_FIGHTING)
		return (FALSE);

	if (ch->get_fighting() && (ch->get_fighting()->in_room == ch->in_room) && (number(0, 42 - GET_LEVEL(ch)) == 0))
	{
		act("$n bites $N!", 1, ch, 0, ch->get_fighting(), TO_NOTVICT);
		act("$n bites you!", 1, ch, 0, ch->get_fighting(), TO_VICT);
		call_magic(ch, ch->get_fighting(), NULL, world[ch->in_room], SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
		return (TRUE);
	}
	return (FALSE);
}

int thief(CHAR_DATA *ch, void* /*me*/, int cmd, char* /*argument*/)
{
	if (cmd)
		return (FALSE);

	if (GET_POS(ch) != POS_STANDING)
		return (FALSE);

	for (const auto cons : world[ch->in_room]->people)
	{
		if (!IS_NPC(cons)
			&& GET_LEVEL(cons) < LVL_IMMORT
			&& !number(0, 4))
		{
			do_npc_steal(ch, cons);

			return TRUE;
		}
	}

	return FALSE;
}

int magic_user(CHAR_DATA *ch, void* /*me*/, int cmd, char* /*argument*/)
{
	if (cmd || GET_POS(ch) != POS_FIGHTING)
	{
		return (FALSE);
	}

	CHAR_DATA *target = nullptr;
	// pseudo-randomly choose someone in the room who is fighting me //
	for (const auto vict : world[ch->in_room]->people)
	{
		if (vict->get_fighting() == ch
			&& !number(0, 4))
		{
			target = vict;

			break;
		}
	}

	// if I didn't pick any of those, then just slam the guy I'm fighting //
	if (target == nullptr
		&& IN_ROOM(ch->get_fighting()) == ch->in_room)
	{
		target = ch->get_fighting();
	}

	// Hm...didn't pick anyone...I'll wait a round. //
	if (target == nullptr)
	{
		return (TRUE);
	}

	if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
	{
		cast_spell(ch, target, NULL, NULL, SPELL_SLEEP, SPELL_SLEEP);
	}

	if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
	{
		cast_spell(ch, target, NULL, NULL, SPELL_BLINDNESS, SPELL_BLINDNESS);
	}

	if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0))
	{
		if (IS_EVIL(ch))
		{
			cast_spell(ch, target, NULL, NULL, SPELL_ENERGY_DRAIN, SPELL_ENERGY_DRAIN);
		}
		else if (IS_GOOD(ch))
		{
			cast_spell(ch, target, NULL, NULL, SPELL_DISPEL_EVIL, SPELL_DISPEL_EVIL);
		}
	}

	if (number(0, 4))
	{
		return (TRUE);
	}

	switch (GET_LEVEL(ch))
	{
	case 4:
	case 5:
		cast_spell(ch, target, NULL, NULL, SPELL_MAGIC_MISSILE, SPELL_MAGIC_MISSILE);
		break;
	case 6:
	case 7:
		cast_spell(ch, target, NULL, NULL, SPELL_CHILL_TOUCH, SPELL_CHILL_TOUCH);
		break;
	case 8:
	case 9:
		cast_spell(ch, target, NULL, NULL, SPELL_BURNING_HANDS, SPELL_BURNING_HANDS);
		break;
	case 10:
	case 11:
		cast_spell(ch, target, NULL, NULL, SPELL_SHOCKING_GRASP, SPELL_SHOCKING_GRASP);
		break;
	case 12:
	case 13:
		cast_spell(ch, target, NULL, NULL, SPELL_LIGHTNING_BOLT, SPELL_LIGHTNING_BOLT);
		break;
	case 14:
	case 15:
	case 16:
	case 17:
		cast_spell(ch, target, NULL, NULL, SPELL_COLOR_SPRAY, SPELL_COLOR_SPRAY);
		break;
	default:
		cast_spell(ch, target, NULL, NULL, SPELL_FIREBALL, SPELL_FIREBALL);
		break;
	}

	return (TRUE);
}


// ********************************************************************
// *  Special procedures for mobiles                                  *
// ********************************************************************

int guild_guard(CHAR_DATA *ch, void *me, int cmd, char* /*argument*/)
{
	int i;
	CHAR_DATA *guard = (CHAR_DATA *) me;
	const char *buf = "Охранник остановил вас, преградив дорогу.\r\n";
	const char *buf2 = "Охранник остановил $n, преградив $m дорогу.";

	if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, EAffectFlag::AFF_BLIND)
			|| AFF_FLAGGED(guard, EAffectFlag::AFF_HOLD))
		return (FALSE);

	if (GET_LEVEL(ch) >= LVL_IMMORT)
		return (FALSE);

	for (i = 0; guild_info[i][0] != -1; i++)
	{
		if ((IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) &&
				GET_ROOM_VNUM(ch->in_room) == guild_info[i][1] && cmd == guild_info[i][2])
		{
			send_to_char(buf, ch);
			act(buf2, FALSE, ch, 0, 0, TO_ROOM);
			return (TRUE);
		}
	}

	return (FALSE);
}

// TODO: повырезать все это
int puff(CHAR_DATA* /*ch*/, void* /*me*/, int/* cmd*/, char* /*argument*/)
{
	return 0;
}

int fido(CHAR_DATA *ch, void* /*me*/, int cmd, char* /*argument*/)
{
	OBJ_DATA *i, *temp, *next_obj;

	if (cmd || !AWAKE(ch))
		return (FALSE);

	for (i = world[ch->in_room]->contents; i; i = i->get_next_content())
	{
		if (IS_CORPSE(i))
		{
			act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
			for (temp = i->get_contains(); temp; temp = next_obj)
			{
				next_obj = temp->get_next_content();
				obj_from_obj(temp);
				obj_to_room(temp, ch->in_room);
			}
			extract_obj(i);
			return (TRUE);
		}
	}
	return (FALSE);
}

int janitor(CHAR_DATA *ch, void* /*me*/, int cmd, char* /*argument*/)
{
	OBJ_DATA *i;

	if (cmd || !AWAKE(ch))
		return (FALSE);

	for (i = world[ch->in_room]->contents; i; i = i->get_next_content())
	{
		if (!CAN_WEAR(i, EWearFlag::ITEM_WEAR_TAKE))
		{
			continue;
		}

		if (GET_OBJ_TYPE(i) != OBJ_DATA::ITEM_DRINKCON
			&& GET_OBJ_COST(i) >= 15)
		{
			continue;
		}

		act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
		obj_from_room(i);
		obj_to_char(i, ch);

		return TRUE;
	}

	return FALSE;
}

int cityguard(CHAR_DATA *ch, void* /*me*/, int cmd, char* /*argument*/)
{
	CHAR_DATA *evil;
	int max_evil;

	if (cmd
		|| !AWAKE(ch)
		|| ch->get_fighting())
	{
		return (FALSE);
	}

	max_evil = 1000;
	evil = 0;

	for (const auto tch : world[ch->in_room]->people)
	{
		if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_KILLER))
		{
			act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
			hit(ch, tch, TYPE_UNDEFINED, 1);

			return (TRUE);
		}
	}

	for (const auto tch : world[ch->in_room]->people)
	{
		if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_THIEF))
		{
			act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
			hit(ch, tch, TYPE_UNDEFINED, 1);

			return (TRUE);
		}
	}

	for (const auto tch : world[ch->in_room]->people)
	{
		if (CAN_SEE(ch, tch) && tch->get_fighting())
		{
			if ((GET_ALIGNMENT(tch) < max_evil) && (IS_NPC(tch) || IS_NPC(tch->get_fighting())))
			{
				max_evil = GET_ALIGNMENT(tch);
				evil = tch;
			}
		}
	}

	if (evil
		&& (GET_ALIGNMENT(evil->get_fighting()) >= 0))
	{
		act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
		hit(ch, evil, TYPE_UNDEFINED, 1);

		return (TRUE);
	}

	return (FALSE);
}


#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

int pet_shops(CHAR_DATA *ch, void* /*me*/, int cmd, char* argument)
{
	char buf[MAX_STRING_LENGTH], pet_name[256];
	room_rnum pet_room;
	CHAR_DATA *pet;

	pet_room = ch->in_room + 1;

	if (CMD_IS("list"))
	{
		send_to_char("Available pets are:\r\n", ch);
		for (const auto pet : world[pet_room]->people)
		{
			sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
			send_to_char(buf, ch);
		}

		return (TRUE);
	}
	else if (CMD_IS("buy"))
	{
		two_arguments(argument, buf, pet_name);

		if (!(pet = get_char_room(buf, pet_room)))
		{
			send_to_char("There is no such pet!\r\n", ch);
			return (TRUE);
		}
		if (ch->get_gold() < PET_PRICE(pet))
		{
			send_to_char("You don't have enough gold!\r\n", ch);
			return (TRUE);
		}
		ch->remove_gold(PET_PRICE(pet));

		pet = read_mobile(GET_MOB_RNUM(pet), REAL);
		pet->set_exp(0);
		AFF_FLAGS(pet).set(EAffectFlag::AFF_CHARM);

		if (*pet_name)
		{
			sprintf(buf, "%s %s", pet->get_pc_name().c_str(), pet_name);
			// free(pet->get_pc_name()); don't free the prototype!
			pet->set_pc_name(buf);

			sprintf(buf,
					"%sA small sign on a chain around the neck says 'My name is %s'\r\n",
					pet->player_data.description.c_str(), pet_name);
			// free(pet->player_data.description); don't free the prototype!
			pet->player_data.description = str_dup(buf);
		}
		char_to_room(pet, ch->in_room);
		ch->add_follower(pet);
		load_mtrigger(pet);

		// Be certain that pets can't get/carry/use/wield/wear items
		IS_CARRYING_W(pet) = 1000;
		IS_CARRYING_N(pet) = 100;

		send_to_char("May you enjoy your pet.\r\n", ch);
		act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

		return (1);
	}

	// All commands except list and buy
	return (0);
}


CHAR_DATA *get_player_of_name(const char *name)
{
	for (const auto& i : character_list)
	{
		if (IS_NPC(i))
		{
			continue;
		}

		if (!isname(name, i->get_pc_name()))
		{
			continue;
		}

		return i.get();
	}

	return nullptr;
}

// ********************************************************************
// *  Special procedures for objects                                  *
// ********************************************************************

int bank(CHAR_DATA *ch, void* /*me*/, int cmd, char* argument)
{
	int amount;
	CHAR_DATA *vict;

	if (CMD_IS("balance") || CMD_IS("баланс") || CMD_IS("сальдо"))
	{
		if (ch->get_bank() > 0)
			sprintf(buf, "У вас на счету %ld %s.\r\n",
					ch->get_bank(), desc_count(ch->get_bank(), WHAT_MONEYa));
		else
			sprintf(buf, "У вас нет денег.\r\n");
		send_to_char(buf, ch);
		return (1);
	}
	else if (CMD_IS("deposit") || CMD_IS("вложить") || CMD_IS("вклад"))
	{
		if ((amount = atoi(argument)) <= 0)
		{
			send_to_char("Сколько вы хотите вложить?\r\n", ch);
			return (1);
		}
		if (ch->get_gold() < amount)
		{
			send_to_char("О такой сумме вы можете только мечтать!\r\n", ch);
			return (1);
		}
		ch->remove_gold(amount, false);
		ch->add_bank(amount, false);
		sprintf(buf, "Вы вложили %d %s.\r\n", amount, desc_count(amount, WHAT_MONEYu));
		send_to_char(buf, ch);
		act("$n произвел$g финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
		return (1);
	}
	else if (CMD_IS("withdraw") || CMD_IS("получить"))
	{
		if ((amount = atoi(argument)) <= 0)
		{
			send_to_char("Уточните количество денег, которые вы хотите получить?\r\n", ch);
			return (1);
		}
		if (ch->get_bank() < amount)
		{
			send_to_char("Да вы отродясь столько денег не видели!\r\n", ch);
			return (1);
		}
		ch->add_gold(amount, false);
		ch->remove_bank(amount, false);
		sprintf(buf, "Вы сняли %d %s.\r\n", amount, desc_count(amount, WHAT_MONEYu));
		send_to_char(buf, ch);
		act("$n произвел$g финансовую операцию.", TRUE, ch, 0, FALSE, TO_ROOM);
		return (1);
	}
	else if (CMD_IS("transfer") || CMD_IS("перевести"))
	{
		argument = one_argument(argument, arg);
		amount = atoi(argument);
		if (IS_GOD(ch) && !IS_IMPL(ch))
		{
			send_to_char("Почитить захотелось?\r\n", ch);
			return (1);

		}
		if (!*arg)
		{
			send_to_char("Уточните кому вы хотите перевести?\r\n", ch);
			return (1);
		}
		if (amount <= 0)
		{
			send_to_char("Уточните количество денег, которые вы хотите получить?\r\n", ch);
			return (1);
		}
		if (amount <= 100)
		{
                    if (ch->get_bank() < (amount + 5))
                    {
                        send_to_char("У вас не хватит денег на налоги!\r\n", ch);
			return (1);
                    }
		}

		if (ch->get_bank() < amount)
		{
			send_to_char("Да вы отродясь столько денег не видели!\r\n", ch);
			return (1);
		}
		if (ch->get_bank() < amount + ((amount * 5) / 100))
		{
			send_to_char("У вас не хватит денег на налоги!\r\n", ch);
			return (1);
		}

		if ((vict = get_player_of_name(arg)))
		{
			ch->remove_bank(amount);
                        if (amount <= 100) ch->remove_bank(5);
                        else ch->remove_bank(((amount * 5) / 100));
			sprintf(buf, "%sВы перевели %d кун %s%s.\r\n", CCWHT(ch, C_NRM), amount,
					GET_PAD(vict, 2), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
			vict->add_bank(amount);
			sprintf(buf, "%sВы получили %d кун банковским переводом от %s%s.\r\n", CCWHT(ch, C_NRM), amount,
					GET_PAD(ch, 1), CCNRM(ch, C_NRM));
			send_to_char(buf, vict);
			sprintf(buf, "<%s> {%d} перевел %d кун банковским переводом %s.", ch->get_name().c_str(), GET_ROOM_VNUM(ch->in_room), amount, GET_PAD(vict, 2));
			mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
			return (1);

		}
		else
		{
			vict = new Player; // TODO: переделать на стек
			if (load_char(arg, vict) < 0)
			{
				send_to_char("Такого персонажа не существует.\r\n", ch);
				delete vict;
				return (1);
			}

			ch->remove_bank(amount);
                        if (amount <= 100) ch->remove_bank(5);
                        else ch->remove_bank(((amount * 5) / 100));
			sprintf(buf, "%sВы перевели %d кун %s%s.\r\n", CCWHT(ch, C_NRM), amount,
					GET_PAD(vict, 2), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
			sprintf(buf, "<%s> {%d} перевел %d кун банковским переводом %s.", ch->get_name().c_str(), GET_ROOM_VNUM(ch->in_room), amount, GET_PAD(vict, 2));
			mudlog(buf, NRM, LVL_GRGOD, MONEY_LOG, TRUE);
			vict->add_bank(amount);
			Depot::add_offline_money(GET_UNIQUE(vict), amount);
			vict->save_char();

			delete vict;
			return (1);
		}
	}
	else if (CMD_IS("казна"))
		return (Clan::BankManage(ch, argument));
	return 0;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
