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

#include "act_movement.h"
#include "utils/utils_char_obj.inl"
#include "entities/char_player.h"
#include "game_mechanics/mount.h"
#include "entities/player_races.h"
#include "entities/world_characters.h"
#include "cmd/follow.h"
#include "depot.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "house.h"
//#include "logger.h"
#include "game_magic/magic.h"
#include "color.h"
#include "game_magic/magic_utils.h"
#include "game_magic/magic_temp_spells.h"
#include "structs/global_objects.h"

//   external vars
/*extern DescriptorData *descriptor_list;
extern IndexData *mob_index;
extern TimeInfoData time_info;
extern struct spell_create_type spell_create[];*/
extern int guild_info[][3];

typedef int special_f(CharData *, void *, int, char *);

// extern functions
void do_drop(CharData *ch, char *argument, int cmd, int subcmd);
void do_say(CharData *ch, char *argument, int cmd, int subcmd);
int find_first_step(RoomRnum src, RoomRnum target, CharData *ch);
void ASSIGNMASTER(MobVnum mob, special_f, int learn_info);

// local functions
char *how_good(int skill_level, int skill_cap);
int feat_slot_lvl(int remort, int slot_for_remort, int slot);
void list_feats(CharData *ch, CharData *vict, bool all_feats);
void list_skills(CharData *ch, CharData *vict, const char *filter = nullptr);
void list_spells(CharData *ch, CharData *vict, int all_spells);
int guild_mono(CharData *ch, void *me, int cmd, char *argument);
int guild_poly(CharData *ch, void *me, int cmd, char *argument);
int guild(CharData *ch, void *me, int cmd, char *argument);
int dump(CharData *ch, void *me, int cmd, char *argument);
int mayor(CharData *ch, void *me, int cmd, char *argument);
//int thief(CharacterData *ch, void *me, int cmd, char* argument);
int magic_user(CharData *ch, void *me, int cmd, char *argument);
int guild_guard(CharData *ch, void *me, int cmd, char *argument);
int fido(CharData *ch, void *me, int cmd, char *argument);
int janitor(CharData *ch, void *me, int cmd, char *argument);
int cityguard(CharData *ch, void *me, int cmd, char *argument);
int pet_shops(CharData *ch, void *me, int cmd, char *argument);
int bank(CharData *ch, void *me, int cmd, char *argument);

// ********************************************************************
// *  Special procedures for mobiles                                  *
// ********************************************************************

char *how_good(int skill_level, int skill_cap) {
	static char out_str[128];
	int skill_percent = skill_level * 100 / skill_cap;

	if (skill_level < 0)
		strcpy(out_str, " !Ошибка! ");
	else if (skill_level == 0)
		sprintf(out_str, " %s(не изучено)", KIDRK);
	else if (skill_percent <= 10)
		sprintf(out_str, " %s(ужасно)", KIDRK);
	else if (skill_percent <= 20)
		sprintf(out_str, " %s(очень плохо)", KRED);
	else if (skill_percent <= 30)
		sprintf(out_str, " %s(плохо)", KRED);
	else if (skill_percent <= 40)
		sprintf(out_str, " %s(слабо)", KIRED);
	else if (skill_percent <= 50)
		sprintf(out_str, " %s(ниже среднего)", KIRED);
	else if (skill_percent <= 55)
		sprintf(out_str, " %s(средне)", KYEL);
	else if (skill_percent <= 60)
		sprintf(out_str, " %s(выше среднего)", KYEL);
	else if (skill_percent <= 70)
		sprintf(out_str, " %s(хорошо)", KYEL);
	else if (skill_percent <= 75)
		sprintf(out_str, " %s(очень хорошо)", KIYEL);
	else if (skill_percent <= 80)
		sprintf(out_str, " %s(отлично)", KIYEL);
	else if (skill_percent <= 90)
		sprintf(out_str, " %s(превосходно)", KGRN);
	else if (skill_percent <= 95)
		sprintf(out_str, " %s(великолепно)", KGRN);
	else if (skill_percent <= 100)
		sprintf(out_str, " %s(мастерски)", KIGRN);
	else if (skill_percent <= 110)
		sprintf(out_str, " %s(идеально)", KIGRN);
	else if (skill_percent <= 120)
		sprintf(out_str, " %s(совершенно)", KMAG);
	else if (skill_percent <= 130)
		sprintf(out_str, " %s(бесподобно)", KMAG);
	else if (skill_percent <= 140)
		sprintf(out_str, " %s(возвышенно)", KCYN);
	else if (skill_percent <= 150)
		sprintf(out_str, " %s(заоблачно)", KICYN);
	else if (skill_percent <= 160)
		sprintf(out_str, " %s(божественно)", KWHT);
	else
		sprintf(out_str, " %s(недостижимо)", KWHT);
	sprintf(out_str + strlen(out_str), " %d", skill_level);
	return out_str;
}

const char *prac_types[] = {"spell",
							"skill"
};

#define LEARNED_LEVEL    0    // % known which is considered "learned" //
#define MAX_PER_PRAC    1    // max percent gain in skill per practice //
#define MIN_PER_PRAC    2    // min percent gain in skill per practice //
#define PRAC_TYPE    3    // should it say 'spell' or 'skill'?     //

// actual prac_params are in class.cpp //
extern int prac_params[4][kNumPlayerClasses];

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

int feat_slot_lvl(int remort, int slot_for_remort, int slot) {
	int result = 0;
	for (result = 1; result < kLvlImmortal; result++) {
		if (result * (5 + remort / slot_for_remort) / 28 == slot) {
			break;
		}
	}
	/*
	  ВНИМАНИЕ: формула содрана с NUM_LEV_FEAT (utils.h)!
	  ((int) 1+GetRealLevel(ch)*(5+GET_REAL_REMORT(ch)/feat_slot_for_remort[(int) GET_CLASS(ch)])/28)
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
void list_feats(CharData *ch, CharData *vict, bool all_feats) {
	int i = 0, j = 0, sortpos, slot, max_slot = 0;
	char msg[kMaxStringLength];
	bool sfound;

	//Найдем максимальный слот, который вобще может потребоваться данному персонажу на текущем морте
	max_slot = MAX_ACC_FEAT(ch);
	char **names = new char *[max_slot];
	for (int k = 0; k < max_slot; k++)
		names[k] = new char[kMaxStringLength];

	if (all_feats) {
		sprintf(names[0], "\r\nКруг 1  (1  уровень):\r\n");
	} else
		*names[0] = '\0';
	for (i = 1; i < max_slot; i++)
		if (all_feats) {
			j = feat_slot_lvl(GET_REMORT(ch),
							  feat_slot_for_remort[(int) GET_CLASS(ch)],
							  i); // на каком уровне будет слот i?
			sprintf(names[i], "\r\nКруг %-2d (%-2d уровень):\r\n", i + 1, j);
		} else
			*names[i] = '\0';

	sprintf(buf2, "\r\nВрожденные способности :\r\n");
	j = 0;
	if (all_feats) {
		if (clr(vict, C_NRM)) // реж цвет >= обычный
			send_to_char(" Список способностей, доступных с текущим числом перевоплощений.\r\n"
						 " &gЗеленым цветом и пометкой [И] выделены уже изученные способности.\r\n&n"
						 " Пометкой [Д] выделены доступные для изучения способности.\r\n"
						 " &rКрасным цветом и пометкой [Н] выделены способности, недоступные вам в настоящий момент.&n\r\n\r\n",
						 vict);
		else
			send_to_char(" Список способностей, доступных с текущим числом перевоплощений.\r\n"
						 " Пометкой [И] выделены уже изученные способности.\r\n"
						 " Пометкой [Д] выделены доступные для изучения способности.\r\n"
						 " Пометкой [Н] выделены способности, недоступные вам в настоящий момент.\r\n\r\n", vict);
		for (sortpos = 1; sortpos < kMaxFeats; sortpos++) {
			if (!feat_info[sortpos].is_known[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
				&& !PlayerRace::FeatureCheck((int) GET_KIN(ch), (int) GET_RACE(ch), sortpos))
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

			if (feat_info[sortpos].is_inborn[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
				|| PlayerRace::FeatureCheck((int) GET_KIN(ch), (int) GET_RACE(ch), sortpos)) {
				strcat(buf2, buf);
				j++;
			} else if (FEAT_SLOT(ch, sortpos) < max_slot) {
				const auto slot = FEAT_SLOT(ch, sortpos);
				strcat(names[slot], buf);
			}
		}
		sprintf(buf1, "--------------------------------------");
		for (i = 0; i < max_slot; i++) {
			if (strlen(buf1) >= kMaxStringLength - 60) {
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

	for (sortpos = 1; sortpos < kMaxFeats; sortpos++) {
		if (strlen(buf2) >= kMaxStringLength - 60) {
			strcat(buf2, "***ПЕРЕПОЛНЕНИЕ***\r\n");
			break;
		}
		if (HAVE_FEAT(ch, sortpos)) {
			if (!feat_info[sortpos].name || *feat_info[sortpos].name == '!')
				continue;

			switch (sortpos) {
				case BERSERK_FEAT:
				case LIGHT_WALK_FEAT:
				case SPELL_CAPABLE_FEAT:
				case RELOCATE_FEAT:
				case SHADOW_THROW_FEAT:
					if (IsTimed(ch, sortpos))
						sprintf(buf, "[%3d] ", IsTimed(ch, sortpos));
					else
						sprintf(buf, "[-!-] ");
					break;
				case POWER_ATTACK_FEAT:
				case GREAT_POWER_ATTACK_FEAT:
				case AIMING_ATTACK_FEAT:
				case GREAT_AIMING_ATTACK_FEAT:
				case SKIRMISHER_FEAT:
				case DOUBLE_THROW_FEAT:
				case TRIPLE_THROW_FEAT:
					if (PRF_FLAGGED(ch, GetPrfWithFeatNumber(sortpos)))
						sprintf(buf, "[-%s*%s-] ", CCIGRN(vict, C_NRM), CCNRM(vict, C_NRM));
					else
						sprintf(buf, "[-:-] ");
					break;
				default: sprintf(buf, "      ");
			}
			if (can_use_feat(ch, sortpos))
				sprintf(buf + strlen(buf), "%s%s%s\r\n",
						CCIYEL(vict, C_NRM), feat_info[sortpos].name, CCNRM(vict, C_NRM));
			else if (clr(vict, C_NRM))
				sprintf(buf + strlen(buf), "%s\r\n", feat_info[sortpos].name);
			else
				sprintf(buf, "[-Н-] %s\r\n", feat_info[sortpos].name);
			if (feat_info[sortpos].is_inborn[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]
				|| PlayerRace::FeatureCheck((int) GET_KIN(ch), (int) GET_RACE(ch), sortpos)) {
				sprintf(buf2 + strlen(buf2), "    ");
				strcat(buf2, buf);
				j++;
			} else {
				slot = FEAT_SLOT(ch, sortpos);
				sfound = false;
				while (slot < max_slot) {
					if (*names[slot] == '\0') {
						sprintf(names[slot], " %s%-2d%s) ",
								CCGRN(vict, C_NRM), slot + 1, CCNRM(vict, C_NRM));
						strcat(names[slot], buf);
						sfound = true;
						break;
					} else
						slot++;
				}
				if (!sfound) {
					// Если способность не врожденная и под нее нет слота - удаляем нафик
					//	чтобы можно было менять слоты на лету и чтобы не читерили :)
					sprintf(msg, "WARNING: Unset out of slots feature '%s' for character '%s'!",
							feat_info[sortpos].name, GET_NAME(ch));
					mudlog(msg, BRF, kLvlImplementator, SYSLOG, true);
					UNSET_FEAT(ch, sortpos);
				}
			}
		}
	}

	for (i = 0; i < max_slot; i++) {
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

void list_skills(CharData *ch, CharData *vict, const char *filter/* = nullptr*/) {
	int i = 0;

	sprintf(buf, "Вы владеете следующими умениями:\r\n");
	strcpy(buf2, buf);
	typedef std::list<std::string> skills_t;
	skills_t skills;

	for (const auto &skill : MUD::Skills()) {
		if (ch->get_skill(skill.GetId())) {
			// filter out skills without name or name beginning with '!' character
			if (skill.IsInvalid()) {
				continue;
			}
			// filter out skill that does not correspond to filter condition
			if (filter && nullptr == strstr(skill.GetName(), filter)) {
				continue;
			}
			auto skill_id = skill.GetId();
			switch (skill_id) {
				case ESkill::kWarcry:
					sprintf(buf,
							"[-%d-] ",
							(HOURS_PER_DAY - IsTimedBySkill(ch, skill_id)) / kHoursPerWarcry);
					break;
				case ESkill::kTurnUndead:
					if (can_use_feat(ch, EXORCIST_FEAT)) {
						sprintf(buf,
								"[-%d-] ",
								(HOURS_PER_DAY - IsTimedBySkill(ch, skill_id)) / (kHoursPerTurnUndead - 2));
					} else {
						sprintf(buf, "[-%d-] ", (HOURS_PER_DAY - IsTimedBySkill(ch, skill_id)) / kHoursPerTurnUndead);
					}
					break;
				case ESkill::kFirstAid:
				case ESkill::kHangovering:
				case ESkill::kIdentify:
				case ESkill::kDisguise:
				case ESkill::kCourage:
				case ESkill::kJinx:
				case ESkill::kTownportal:
				case ESkill::kStrangle:
				case ESkill::kStun:
				case ESkill::kRepair:
					if (IsTimedBySkill(ch, skill_id))
						sprintf(buf, "[%3d] ", IsTimedBySkill(ch, skill_id));
					else
						sprintf(buf, "[-!-] ");
					break;
				default: sprintf(buf, "      ");
			}

			sprintf(buf + strlen(buf), "%-23s %s (%d)%s \r\n",
					skill.GetName(),
					how_good(ch->get_skill(skill_id), CalcSkillHardCap(ch, skill_id)),
					CalcSkillMinCap(ch, skill_id),
					CCNRM(ch, C_NRM));

			skills.push_back(buf);

			i++;
		}
	}

	if (!i) {
		if (nullptr == filter) {
			sprintf(buf2 + strlen(buf2), "Нет умений.\r\n");
		} else {
			sprintf(buf2 + strlen(buf2), "Нет умений, удовлетворяющих фильтру.\r\n");
		}
	} else {
		// output set of skills
		size_t buf2_length = strlen(buf2);
		for (skills_t::const_iterator i = skills.begin(); i != skills.end(); ++i) {
			// why 60?
			if (buf2_length + i->length() >= kMaxStringLength - 60) {
				strcat(buf2, "***ПЕРЕПОЛНЕНИЕ***\r\n");
				break;
			}

			strncat(buf2 + buf2_length, i->c_str(), i->length());
			buf2_length += i->length();
		}
	}
	send_to_char(buf2, vict);

}
const char *spells_color(int spellnum) {
	switch (spell_info[spellnum].spell_class) {
		case kTypeAir: return "&W";
			break;

		case kTypeFire: return "&R";
			break;

		case kTypeWater: return "&C";
			break;

		case kTypeEarth: return "&y";
			break;

		case kTypeLight: return "&Y";
			break;

		case kTypeDark: return "&K";
			break;

		case kTypeMind: return "&M";
			break;

		case kTypeLife: return "&G";
			break;

		case kTypeNeutral: return "&n";
			break;
		default: return "&n";
			break;
	}
}


/* Параметр all_spells введен для того чтобы предметные кастеры
   смогли посмотреть заклинания которые они могут колдовать
   на своем уровне, но на которые у них нет необходимых предметов
   при параметре true */
#include "game_classes/classes_spell_slots.h"
void list_spells(CharData *ch, CharData *vict, int all_spells) {
	using PlayerClass::slot_for_char;

	char names[kMaxSlot][kMaxStringLength];
	std::string time_str;
	int slots[kMaxSlot], i, max_slot = 0, slot_num, is_full, gcount = 0, can_cast = 1;

	is_full = 0;
	max_slot = 0;
	for (i = 0; i < kMaxSlot; i++) {
		*names[i] = '\0';
		slots[i] = 0;
	}
	for (i = 1; i <= kSpellCount; i++) {
		if (!GET_SPELL_TYPE(ch, i) && !all_spells)
			continue;
		if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA_DOMINATION)) {
			if (!IS_SET(GET_SPELL_TYPE(ch, i), kSpellTemp) && !all_spells)
				continue;
		}
		if ((MIN_CAST_LEV(spell_info[i], ch) > GetRealLevel(ch)
			|| MIN_CAST_REM(spell_info[i], ch) > GET_REAL_REMORT(ch)
			|| slot_for_char(ch, spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) <= 0)
			&& all_spells && !GET_SPELL_TYPE(ch, i))
			continue;

		if (!spell_info[i].name || *spell_info[i].name == '!')
			continue;

		if ((GET_SPELL_TYPE(ch, i) & 0xFF) == kSpellRunes && !CheckRecipeItems(ch, i, kSpellRunes, false)) {
			if (all_spells) {
				can_cast = 0;
			} else {
				continue;
			}
		} else {
			can_cast = 1;
		}

		if (MIN_CAST_REM(spell_info[i], ch) > GET_REAL_REMORT(ch))
			slot_num = kMaxSlot - 1;
		else
			slot_num = spell_info[i].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] - 1;
		max_slot = MAX(slot_num + 1, max_slot);
		if (IS_MANA_CASTER(ch)) {
			if (GET_MANA_COST(ch, i) > GET_MAX_MANA(ch))
				continue;
			if (can_cast) {
				slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
										   "%s|<...%4d.> %s%-38s&n|",
										   slots[slot_num] % 114 <
											   10 ? "\r\n" : "  ",
										   GET_MANA_COST(ch, i), spells_color(i), spell_info[i].name);
			} else {
				slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
										   "%s|+--------+ %s%-38s&n|",
										   slots[slot_num] % 114 <
											   10 ? "\r\n" : "  ", spells_color(i), spell_info[i].name);
			}
		} else {
			time_str.clear();
			if (IS_SET(GET_SPELL_TYPE(ch, i), kSpellTemp)) {
				time_str.append("[");
				time_str.append(std::to_string(MAX(1,
												   static_cast<int>(std::ceil(
													   static_cast<double>(Temporary_Spells::spell_left_time(ch, i))
														   / SECS_PER_MUD_HOUR)))));
				time_str.append("]");
			}
		if (MIN_CAST_LEV(spell_info[i], ch) > GetRealLevel(ch) && IS_SET(GET_SPELL_TYPE(ch, i), kSpellKnow)) {
			sprintf(buf1, "%d", MIN_CAST_LEV(spell_info[i], ch) - GetRealLevel(ch));
		}
		else {
			sprintf(buf1, "%s", "K");
		}
		slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
				"%s|<%s%c%c%c%c%c%c%c>%s %-30s %-7s&n|",
				slots[slot_num] % 114 < 10 ? "\r\n" : "  ",
								   IS_SET(GET_SPELL_TYPE(ch, i), kSpellKnow) ? buf1 : ".",
								   IS_SET(GET_SPELL_TYPE(ch, i), kSpellTemp) ? 'T' : '.',
								   IS_SET(GET_SPELL_TYPE(ch, i), kSpellPotion) ? 'P' : '.',
								   IS_SET(GET_SPELL_TYPE(ch, i), kSpellWand) ? 'W' : '.',
								   IS_SET(GET_SPELL_TYPE(ch, i), kSpellScroll) ? 'S' : '.',
								   IS_SET(GET_SPELL_TYPE(ch, i), kSpellItems) ? 'I' : '.',
								   IS_SET(GET_SPELL_TYPE(ch, i), kSpellRunes) ? 'R' : '.',
				'.',
				spells_color(i),
				spell_info[i].name,
				time_str.c_str());
		}
		is_full++;
	};
	gcount = sprintf(buf2 + gcount, "  %sВам доступна следующая магия :%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	if (is_full) {
		for (i = 0; i < max_slot; i++) {
			if (slots[i] != 0) {
				if (!IS_MANA_CASTER(ch))
					gcount += sprintf(buf2 + gcount, "\r\nКруг %d", i + 1);
			}
			if (slots[i])
				gcount += sprintf(buf2 + gcount, "%s", names[i]);
			//else
			//gcount += sprintf(buf2+gcount,"\n\rПусто.");
		}
	} else
		gcount += sprintf(buf2 + gcount, "\r\nВ настоящее время магия вам недоступна!");
	gcount += sprintf(buf2 + gcount, "\r\n");
	//page_string(ch->desc, buf2, 1);
	send_to_char(buf2, vict);
}

struct guild_learn_type {
	int feat_no;
	ESkill skill_no;
	int spell_no;
	int level;
};

struct guild_mono_type {
	guild_mono_type() : races(0), classes(0), religion(0), alignment(0), learn_info(0) {};
	int races;        // bitvector //
	int classes;        // bitvector //
	int religion;        // bitvector //
	int alignment;        // bitvector //
	struct guild_learn_type *learn_info;
};

struct guild_poly_type {
	int races;        // bitvector //
	int classes;        // bitvector //
	int religion;        // bitvector //
	int alignment;        // bitvector //
	int feat_no;
	ESkill skill_no;
	int spell_no;
	int level;
};

int GUILDS_MONO_USED = 0;
int GUILDS_POLY_USED = 0;

struct guild_mono_type *guild_mono_info = nullptr;
struct guild_poly_type **guild_poly_info = nullptr;

void init_guilds() {
	FILE *magic;
	char name[kMaxInputLength],
		line[256], line1[256], line2[256], line3[256], line4[256], line5[256], line6[256], *pos;
	int i, spellnum, featnum, num, type = 0, lines = 0, level, pgcount = 0, mgcount = 0;
	ESkill skill_id = ESkill::kIncorrect;
	std::unique_ptr<struct guild_poly_type, decltype(free) *> poly_guild(nullptr, free);
	struct guild_mono_type mono_guild;
	std::unique_ptr<struct guild_learn_type, decltype(free) *> mono_guild_learn(nullptr, free);

	if (!(magic = fopen(LIB_MISC "guilds.lst", "r"))) {
		log("Cann't open guilds list file...");
		return;
	}
	while (get_line(magic, name)) {
		if (!name[0] || name[0] == ';')
			continue;
		log("<%s>", name);
		if ((lines = sscanf(name, "%s %s %s %s %s %s %s", line, line1, line2, line3, line4, line5, line6)) == 0)
			continue;
		// log("%d",lines);

		if (!strn_cmp(line, "monoguild", strlen(line))
			|| !strn_cmp(line, "одноклассовая", strlen(line))) {
			type = 1;
			if (lines < 5) {
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
		} else if (!strn_cmp(line, "polyguild", strlen(line))
			|| !strn_cmp(line, "многоклассовая", strlen(line))) {
			type = 2;
			poly_guild.reset();
			pgcount = 0;
		} else if (!strn_cmp(line, "master", strlen(line))
			|| !strn_cmp(line, "учитель", strlen(line))) {
			if ((num = atoi(line1)) == 0 || real_mobile(num) < 0) {
				log("WARNING: Can't assign master %s in guilds.lst. Skipped.", line1);
				continue;
			}

			if (!((type == 1 && mono_guild_learn) || type == 11) &&
				!((type == 2 && poly_guild) || type == 12)) {
				log("WARNING: Can't define guild info for master %s. Skipped.", line1);
				continue;
			}
			if (type == 1 || type == 11) {
				if (type == 1) {
					if (!guild_mono_info) {
						CREATE(guild_mono_info, GUILDS_MONO_USED + 1);
					} else {
						RECREATE(guild_mono_info, GUILDS_MONO_USED + 1);
					}
					log("Create mono guild %d", GUILDS_MONO_USED + 1);
					mono_guild.learn_info = mono_guild_learn.release();
					RECREATE(mono_guild.learn_info, mgcount + 1);
					(mono_guild.learn_info + mgcount)->skill_no = ESkill::kIncorrect;
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
			if (type == 2 || type == 12) {
				if (type == 2) {
					if (!guild_poly_info) {
						CREATE(guild_poly_info, GUILDS_POLY_USED + 1);
					} else {
						RECREATE(guild_poly_info, GUILDS_POLY_USED + 1);
					}
					log("Create poly guild %d", GUILDS_POLY_USED + 1);
					auto ptr = poly_guild.release();
					RECREATE(ptr, pgcount + 1);
					(ptr + pgcount)->feat_no = -1;
					(ptr + pgcount)->skill_no = ESkill::kIncorrect;
					(ptr + pgcount)->spell_no = -1;
					(ptr + pgcount)->level = -1;
					guild_poly_info[GUILDS_POLY_USED] = ptr;
					GUILDS_POLY_USED++;
					type = 12;
				}
				//log("Assign poly guild %d to mobile %s", GUILDS_POLY_USED, line1);
				ASSIGNMASTER(num, guild_poly, GUILDS_POLY_USED);
			}
		} else if (type == 1) {
			if (lines < 3) {
				log("You need use 3 arguments for monoguild");
				graceful_exit(1);
			}
			if ((spellnum = atoi(line)) == 0 || spellnum > kSpellCount) {
				spellnum = FixNameAndFindSpellNum(line);
			}

			skill_id = static_cast<ESkill>(atoi(line1));
			if (MUD::Skills().IsInvalid(skill_id)) {
				skill_id = FixNameAndFindSkillNum(line1);
			}

			if ((featnum = atoi(line1)) == 0 || featnum >= kMaxFeats) {
				if ((pos = strchr(line1, '.')))
					*pos = ' ';
				featnum = FindFeatNum(line1);
			}

			if (MUD::Skills().IsInvalid(skill_id) && spellnum <= 0 && featnum <= 0) {
				log("Unknown skill, spell or feat for monoguild");
				graceful_exit(1);
			}

			if ((level = atoi(line2)) == 0 || level >= kLvlImmortal) {
				log("Use 1-%d level for guilds", kLvlImmortal);
				graceful_exit(1);
			}

			auto ptr = mono_guild_learn.release();
			if (!ptr) {
				CREATE(ptr, mgcount + 1);
			} else {
				RECREATE(ptr, mgcount + 1);
			}
			mono_guild_learn.reset(ptr);

			ptr += mgcount;
			ptr->spell_no = MAX(0, spellnum);
			ptr->skill_no = skill_id;
			ptr->feat_no = MAX(0, featnum);
			ptr->level = level;
			// log("->%d %d %d<-",spellnum,skill_id,level);
			mgcount++;
		} else if (type == 2) {
			if (lines < 7) {
				log("You need use 7 arguments for polyguild");
				graceful_exit(1);
			}
			auto ptr = poly_guild.release();
			if (!ptr) {
				CREATE(ptr, pgcount + 1);
			} else {
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

			if ((spellnum = atoi(line4)) == 0 || spellnum > kSpellCount) {
				spellnum = FixNameAndFindSpellNum(line4);
			}

			skill_id = static_cast<ESkill>(atoi(line5));
			if (MUD::Skills().IsInvalid(skill_id)) {
				skill_id = FixNameAndFindSkillNum(line5);
			}

			if ((featnum = atoi(line5)) == 0 || featnum >= kMaxFeats) {
				if ((pos = strchr(line5, '.')))
					*pos = ' ';

				featnum = FindFeatNum(line1);
				sprintf(buf, "feature number 2: %d", featnum);
				featnum = FindFeatNum(line5);
			}
			if (MUD::Skills().IsInvalid(skill_id) && spellnum <= 0 && featnum <= 0) {
				log("Unknown skill, spell or feat for polyguild - \'%s\'", line5);
				graceful_exit(1);
			}
			if ((level = atoi(line6)) == 0 || level >= kLvlImmortal) {
				log("Use 1-%d level for guilds", kLvlImmortal);
				graceful_exit(1);
			}
			ptr->spell_no = std::max(0, spellnum);
			ptr->skill_no = skill_id;
			ptr->feat_no = std::max(0, featnum);
			ptr->level = level;
			// log("->%d %d %d<-",spellnum,skill_id,level);
			pgcount++;
		}
	}
	fclose(magic);
}

#define SCMD_LEARN 1

int guild_mono(CharData *ch, void *me, int cmd, char *argument) {
	int command = 0, gcount = 0, info_num = 0, found = false, sfound = false, i, bits;
	CharData *victim = (CharData *) me;

	if (IS_NPC(ch)) {
		return 0;
	}

	if (CMD_IS("учить") || CMD_IS("practice")) {
		command = SCMD_LEARN;
	} else {
		return 0;
	}

	info_num = mob_index[victim->get_rnum()].stored;
	if (info_num <= 0 || info_num > GUILDS_MONO_USED) {
		act("$N сказал$G : 'Извини, $n, я уже в отставке.'", false, ch, nullptr, victim, kToChar);
		return (1);
	}

	info_num--;
	if (!IS_BITS(guild_mono_info[info_num].classes, GET_CLASS(ch))
		|| !IS_BITS(guild_mono_info[info_num].races, GET_RACE(ch))
		|| !IS_BITS(guild_mono_info[info_num].religion, GET_RELIGION(ch))) {
		act("$N сказал$g : '$n, я не учу таких, как ты.'", false, ch, nullptr, victim, kToChar);
		return 1;
	}

	skip_spaces(&argument);

	switch (command) {
		case SCMD_LEARN:
			if (!*argument) {
				gcount += sprintf(buf, "Я могу научить тебя следующему:\r\n");
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->spell_no >= 0; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					const auto skill_no = (guild_mono_info[info_num].learn_info + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kIncorrect != skill_no && (!ch->get_trained_skill(skill_no)
						|| IS_GRGOD(ch)) && IsAbleToGetSkill(ch, skill_no)) {
						gcount += sprintf(buf + gcount, "- умение %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), MUD::Skills()[skill_no].GetName(), CCNRM(ch, C_NRM));
						found = true;
					}

					if (!(bits = -2 * bits) || bits == kSpellTemp) {
						bits = kSpellKnow;
					}

					const auto spell_no = (guild_mono_info[info_num].learn_info + i)->spell_no;
					if (spell_no && (!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits) || IS_GRGOD(ch))) {
						gcount += sprintf(buf + gcount, "- магия %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						found = true;
					}

					const auto feat_no = (guild_mono_info[info_num].learn_info + i)->feat_no;
					if (feat_no > 0 && !HAVE_FEAT(ch, feat_no) && can_get_feat(ch, feat_no)) {
						gcount += sprintf(buf + gcount, "- способность %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
						found = true;
					}
				}

				if (!found) {
					act("$N сказал$G : 'Похоже, твои знания полнее моих.'", false, ch, 0, victim, kToChar);
					return (1);
				} else {
					send_to_char(buf, ch);
					return (1);
				}
			}

			if (!strn_cmp(argument, "все", strlen(argument)) || !strn_cmp(argument, "all", strlen(argument))) {
				for (i = 0, found = false, sfound = true;
					 (guild_mono_info[info_num].learn_info + i)->spell_no >= 0; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch))
						continue;

					const ESkill skill_no = (guild_mono_info[info_num].learn_info + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kIncorrect != skill_no
						&& !ch->get_trained_skill(skill_no))    // sprintf(buf, "$N научил$G вас умению %s\"%s\"\%s",
					{
						//             CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
						// act(buf,false,ch,0,victim,TO_CHAR);
						// ch->get_skill(skill_no) = 10;
						sfound = true;
					}

					if (!(bits = -2 * bits) || bits == kSpellTemp) {
						bits = kSpellKnow;
					}

					const int spell_no = (guild_mono_info[info_num].learn_info + i)->spell_no;
					if (spell_no
						&& !((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)) {
						gcount += sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						act(buf, false, ch, 0, victim, kToChar);
						if (IS_SET(bits, kSpellKnow))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellKnow);
						if (IS_SET(bits, kSpellItems))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellItems);
						if (IS_SET(bits, kSpellRunes))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellRunes);
						if (IS_SET(bits, kSpellPotion)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellPotion);
							ch->set_skill(ESkill::kCreatePotion, MAX(10, ch->get_trained_skill(ESkill::kCreatePotion)));
						}
						if (IS_SET(bits, kSpellWand)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellWand);
							ch->set_skill(ESkill::kCreateWand, MAX(10, ch->get_trained_skill(ESkill::kCreateWand)));
						}
						if (IS_SET(bits, kSpellScroll)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellScroll);
							ch->set_skill(ESkill::kCreateScroll, MAX(10, ch->get_trained_skill(ESkill::kCreateScroll)));
						}
						found = true;
					}

					const int feat_no = (guild_mono_info[info_num].learn_info + i)->feat_no;
					if (feat_no > 0
						&& feat_no < kMaxFeats) {
						if (!HAVE_FEAT(ch, feat_no) && can_get_feat(ch, feat_no)) {
							sfound = true;
						}
					}
				}

				if (sfound) {
					act("$N сказал$G : \r\n"
						"'$n, к сожалению, это может сильно затруднить твое постижение умений.'\r\n"
						"'Выбери лишь самые необходимые тебе умения.'", false, ch, 0, victim, kToChar);
				}

				if (!found) {
					act("$N ничему новому вас не научил$G.", false, ch, 0, victim, kToChar);
				}

				return (1);
			}

			const int feat_no = FindFeatNum(argument);
			if ((feat_no > 0 && feat_no < kMaxFeats)) {
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->feat_no >= 0; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (feat_no == (guild_mono_info[info_num].learn_info + i)->feat_no) {
						if (HAVE_FEAT(ch, feat_no)) {
							act("$N сказал$g вам : 'Ничем помочь не могу, ты уже владеешь этой способностью.'",
								false,
								ch,
								0,
								victim,
								kToChar);
						} else if (!can_get_feat(ch, feat_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'", false, ch, 0, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас способности %s\"%s\"%s",
									CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
							act(buf, false, ch, 0, victim, kToChar);
							SET_FEAT(ch, feat_no);
						}
						found = true;
					}
				}

				if (!found) {
					act("$N сказал$G : 'Мне не ведомо такое мастерство.'", false, ch, 0, victim, kToChar);
				}

				return (1);
			}

			const ESkill skill_no = FixNameAndFindSkillNum(argument);
			if (skill_no >= ESkill::kLast && skill_no <= ESkill::kLast) {
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->spell_no >= 0; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (skill_no == (guild_mono_info[info_num].learn_info + i)->skill_no) {
						if (ch->get_trained_skill(skill_no)) {
							act("$N сказал$g вам : 'Ничем помочь не могу, ты уже владеешь этим умением.'",
								false,
								ch,
								0,
								victim,
								kToChar);
						} else if (!IsAbleToGetSkill(ch, skill_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'", false, ch, 0, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас умению %s\"%s\"%s",
									CCCYN(ch, C_NRM), MUD::Skills()[skill_no].GetName(), CCNRM(ch, C_NRM));
							act(buf, false, ch, nullptr, victim, kToChar);
							ch->set_skill(skill_no, 10);
						}
						found = true;
					}
				}

				if (!found) {
					act("$N сказал$G : 'Мне не ведомо такое умение.'", false, ch, 0, victim, kToChar);
				}

				return (1);
			}

			const int spell_no = FixNameAndFindSpellNum(argument);
			if (spell_no > 0
				&& spell_no <= kSpellCount) {
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->spell_no >= 0; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (spell_no == (guild_mono_info[info_num].learn_info + i)->spell_no) {
						if (!(bits = -2 * to_underlying((guild_mono_info[info_num].learn_info + i)->skill_no))
							|| bits == kSpellTemp) {
							bits = kSpellKnow;
						}

						if ((GET_SPELL_TYPE(ch, spell_no) & bits) == bits) {
							if (IS_SET(bits, kSpellKnow)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить это заклинание, тебе необходимо набрать\r\n"
											 "%s колд(овать) '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, kSpellItems)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
											 "%s смешать '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, kSpellRunes)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
											 "%s руны '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else {
								strcpy(buf, "Вы уже умеете это.");
							}
							act(buf, false, ch, 0, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
									CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
							act(buf, false, ch, 0, victim, kToChar);
							if (IS_SET(bits, kSpellKnow)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellKnow);
							}
							if (IS_SET(bits, kSpellItems)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellItems);
							}
							if (IS_SET(bits, kSpellRunes)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellRunes);
							}
							if (IS_SET(bits, kSpellPotion)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellPotion);
								ch->set_skill(ESkill::kCreatePotion, MAX(10, ch->get_trained_skill(ESkill::kCreatePotion)));
							}
							if (IS_SET(bits, kSpellWand)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellWand);
								ch->set_skill(ESkill::kCreateWand, MAX(10, ch->get_trained_skill(ESkill::kCreateWand)));
							}
							if (IS_SET(bits, kSpellScroll)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellScroll);
								ch->set_skill(ESkill::kCreateScroll, MAX(10, ch->get_trained_skill(ESkill::kCreateScroll)));
							}
						}
						found = true;
					}
				}

				if (!found) {
					act("$N сказал$G : 'Я и сам$G не знаю такой магии.'", false, ch, 0, victim, kToChar);
				}

				return (1);
			}

			act("$N сказал$G вам : 'Я никогда и никого этому не учил$G.'", false, ch, 0, victim, kToChar);
			return (1);
	}

	return (0);
}

int guild_poly(CharData *ch, void *me, int cmd, char *argument) {
	int command = 0, gcount = 0, info_num = 0, found = false, sfound = false, i, bits;
	CharData *victim = (CharData *) me;

	if (IS_NPC(ch)) {
		return 0;
	}

	if (CMD_IS("учить") || CMD_IS("practice")) {
		command = SCMD_LEARN;
	} else {
		return 0;
	}

	if ((info_num = mob_index[victim->get_rnum()].stored) <= 0 || info_num > GUILDS_POLY_USED) {
		act("$N сказал$G : 'Извини, $n, я уже в отставке.'", false, ch, 0, victim, kToChar);
		return 1;
	}

	info_num--;

	skip_spaces(&argument);

	switch (command) {
		case SCMD_LEARN:
			if (!*argument) {
				gcount += sprintf(buf, "Я могу научить тебя следующему:\r\n");
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= 0; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, GET_CLASS(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch)))
						continue;

					const ESkill skill_no = (guild_poly_info[info_num] + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kIncorrect != skill_no
						&& (!ch->get_trained_skill(skill_no)
							|| IS_GRGOD(ch))
						&& IsAbleToGetSkill(ch, skill_no)) {
						gcount += sprintf(buf + gcount, "- умение %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), MUD::Skills()[skill_no].GetName(), CCNRM(ch, C_NRM));
						found = true;
					}

					if (!(bits = -2 * bits)
						|| bits == kSpellTemp) {
						bits = kSpellKnow;
					}

					const int spell_no = (guild_poly_info[info_num] + i)->spell_no;
					if (spell_no
						&& (!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)
							|| IS_GRGOD(ch))) {
						gcount += sprintf(buf + gcount, "- магия %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						found = true;
					}

					const int feat_no = (guild_poly_info[info_num] + i)->feat_no;
					if (feat_no > 0
						&& feat_no < kMaxFeats) {
						if (!HAVE_FEAT(ch, feat_no) && can_get_feat(ch, feat_no)) {
							gcount += sprintf(buf + gcount, "- способность %s\"%s\"%s\r\n",
											  CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
							found = true;
						}
					}
				}

				if (!found) {
					act("$N сказал$G : 'Похоже, я не смогу тебе помочь.'", false, ch, 0, victim, kToChar);
					return (1);
				} else {
					send_to_char(buf, ch);
					return (1);
				}
			}

			if (!strn_cmp(argument, "все", strlen(argument))
				|| !strn_cmp(argument, "all", strlen(argument))) {
				for (i = 0, found = false, sfound = false; (guild_poly_info[info_num] + i)->spell_no >= 0; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, GET_CLASS(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					const ESkill skill_no = (guild_poly_info[info_num] + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kIncorrect != skill_no
						&& !ch->get_trained_skill(skill_no))    // sprintf(buf, "$N научил$G вас умению %s\"%s\"\%s",
					{
						//             CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
						// act(buf,false,ch,0,victim,TO_CHAR);
						// ch->get_skill(skill_no) = 10;
						sfound = true;
					}

					const int feat_no = (guild_poly_info[info_num] + i)->feat_no;
					if (feat_no > 0
						&& feat_no < kMaxFeats) {
						if (!HAVE_FEAT(ch, feat_no)
							&& can_get_feat(ch, feat_no)) {
							sfound = true;
						}
					}

					if (!(bits = -2 * bits) || bits == kSpellTemp) {
						bits = kSpellKnow;
					}

					const int spell_no = (guild_poly_info[info_num] + i)->spell_no;
					if (spell_no
						&& !((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)) {
						gcount += sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						act(buf, false, ch, 0, victim, kToChar);

						if (IS_SET(bits, kSpellKnow))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellKnow);
						if (IS_SET(bits, kSpellItems))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellItems);
						if (IS_SET(bits, kSpellRunes))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellRunes);
						if (IS_SET(bits, kSpellPotion)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellPotion);
							ch->set_skill(ESkill::kCreatePotion, MAX(10, ch->get_trained_skill(ESkill::kCreatePotion)));
						}
						if (IS_SET(bits, kSpellWand)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellWand);
							ch->set_skill(ESkill::kCreateWand, MAX(10, ch->get_trained_skill(ESkill::kCreateWand)));
						}
						if (IS_SET(bits, kSpellScroll)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellScroll);
							ch->set_skill(ESkill::kCreateScroll, MAX(10, ch->get_trained_skill(ESkill::kCreateScroll)));
						}
						found = true;
					}
				}

				if (sfound) {
					act("$N сказал$G : \r\n"
						"'$n, к сожалению, это может сильно затруднить твое постижение умений.'\r\n"
						"'Выбери лишь самые необходимые тебе умения.'", false, ch, 0, victim, kToChar);
				}

				if (!found) {
					act("$N ничему новому вас не научил$G.", false, ch, 0, victim, kToChar);
				}

				return (1);
			}

			const ESkill skill_no = FixNameAndFindSkillNum(argument);
			if (ESkill::kIncorrect != skill_no && skill_no <= ESkill::kLast) {
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= 0; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, GET_CLASS(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					if (skill_no == (guild_poly_info[info_num] + i)->skill_no) {
						if (ch->get_trained_skill(skill_no)) {
							act("$N сказал$G вам : 'Ты уже владеешь этим умением.'", false, ch, 0, victim, kToChar);
						} else if (!IsAbleToGetSkill(ch, skill_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'", false, ch, 0, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас умению %s\"%s\"%s",
									CCCYN(ch, C_NRM), MUD::Skills()[skill_no].GetName(), CCNRM(ch, C_NRM));
							act(buf, false, ch, 0, victim, kToChar);
							ch->set_skill(skill_no, 10);
						}
						found = true;
					}
				}

				if (!found) {
					//				found_skill = false;
					//				act("$N сказал$G : 'Мне не ведомо такое умение.'", false, ch, 0, victim, TO_CHAR);
				} else {
					return (1);
				}
			}

			int feat_no = FindFeatNum(argument);
			if (feat_no < 0 || feat_no >= kMaxFeats) {
				std::string str(argument);
				std::replace_if(str.begin(), str.end(), boost::is_any_of("_:"), ' ');
				feat_no = FindFeatNum(str.c_str(), true);
			}

			if (feat_no > 0 && feat_no < kMaxFeats) {
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->feat_no >= 0; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, GET_CLASS(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}
					if (feat_no == (guild_poly_info[info_num] + i)->feat_no) {
						if (HAVE_FEAT(ch, feat_no)) {
							act("$N сказал$G вам : 'Ничем помочь не могу, ты уже владеешь этой способностью.'",
								false,
								ch,
								0,
								victim,
								kToChar);
						} else if (!can_get_feat(ch, feat_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'", false, ch, 0, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас способности %s\"%s\"%s",
									CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
							act(buf, false, ch, 0, victim, kToChar);
							SET_FEAT(ch, feat_no);
						}
						found = true;
					}
				}

				if (!found) {
					//				found_feat = false;
					//				act("$N сказал$G : 'Мне не ведомо такое умение.'", false, ch, 0, victim, TO_CHAR);
				} else {
					return (1);
				}
			}

			const int spell_no = FixNameAndFindSpellNum(argument);
			if (spell_no > 0 && spell_no <= kSpellCount) {
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= 0; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!(bits = -2 * to_underlying((guild_poly_info[info_num] + i)->skill_no))
						|| bits == kSpellTemp) {
						bits = kSpellKnow;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, GET_CLASS(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					if (spell_no == (guild_poly_info[info_num] + i)->spell_no) {
						if ((GET_SPELL_TYPE(ch, spell_no) & bits) == bits) {
							if (IS_SET(bits, kSpellKnow)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить это заклинание, тебе необходимо набрать\r\n"
											 "%s колд(овать) '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, kSpellItems)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
											 "%s смешать '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, kSpellRunes)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
											 "%s руны '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else {
								strcpy(buf, "Вы уже умеете это.");
							}
							act(buf, false, ch, 0, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
									CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
							act(buf, false, ch, 0, victim, kToChar);
							if (IS_SET(bits, kSpellKnow)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellKnow);
							}
							if (IS_SET(bits, kSpellItems)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellItems);
							}
							if (IS_SET(bits, kSpellRunes)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellRunes);
							}
							if (IS_SET(bits, kSpellPotion)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellPotion);
								ch->set_skill(ESkill::kCreatePotion, MAX(10, ch->get_trained_skill(ESkill::kCreatePotion)));
							}
							if (IS_SET(bits, kSpellWand)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellWand);
								ch->set_skill(ESkill::kCreateWand, MAX(10, ch->get_trained_skill(ESkill::kCreateWand)));
							}
							if (IS_SET(bits, kSpellScroll)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), kSpellScroll);
								ch->set_skill(ESkill::kCreateScroll, MAX(10, ch->get_trained_skill(ESkill::kCreateScroll)));
							}
						}
						found = true;
					}
				}

				if (!found) {
					//				found_spell = false;
					//				act("$N сказал$G : 'Я и сам$G не знаю такого заклинания.'", false, ch, 0, victim, TO_CHAR);
				} else {
					return (1);
				}
			}

			act("$N сказал$G вам : 'Я никогда и никого этому не учил$G.'", false, ch, 0, victim, kToChar);
			return (1);
	}

	return (0);
}

int horse_keeper(CharData *ch, void *me, int cmd, char *argument) {
	CharData *victim = (CharData *) me, *horse = nullptr;

	if (IS_NPC(ch))
		return (0);

	if (!CMD_IS("лошадь") && !CMD_IS("horse"))
		return (0);

	if (ch->is_morphed()) {
		send_to_char("Лошадка испугается вас в таком виде... \r\n", ch);
		return (true);
	}

	skip_spaces(&argument);

	if (!*argument) {
		if (ch->has_horse(false)) {
			act("$N поинтересовал$U : \"$n, зачем тебе второй скакун? У тебя ведь одно седалище.\"",
				false, ch, 0, victim, kToChar);
			return (true);
		}
		sprintf(buf, "$N сказал$G : \"Я продам тебе скакуна за %d %s.\"",
				HORSE_COST, desc_count(HORSE_COST, WHAT_MONEYa));
		act(buf, false, ch, 0, victim, kToChar);
		return (true);
	}

	if (!strn_cmp(argument, "купить", strlen(argument)) || !strn_cmp(argument, "buy", strlen(argument))) {
		if (ch->has_horse(false)) {
			act("$N засмеял$U : \"$n, ты шутишь, у тебя же есть скакун.\"", false, ch, 0, victim, kToChar);
			return (true);
		}
		if (ch->get_gold() < HORSE_COST) {
			act("\"Ступай отсюда, злыдень, у тебя нет таких денег!\"-заорал$G $N",
				false, ch, 0, victim, kToChar);
			return (true);
		}
		if (!(horse = read_mobile(HORSE_VNUM, VIRTUAL))) {
			act("\"Извини, у меня нет для тебя скакуна.\"-смущенно произнес$Q $N",
				false, ch, 0, victim, kToChar);
			return (true);
		}
		make_horse(horse, ch);
		char_to_room(horse, ch->in_room);
		sprintf(buf, "$N оседлал$G %s и отдал$G %s вам.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, false, ch, 0, victim, kToChar);
		sprintf(buf, "$N оседлал$G %s и отдал$G %s $n2.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, false, ch, 0, victim, kToRoom);
		ch->remove_gold(HORSE_COST);
		PLR_FLAGS(ch).set(PLR_CRASH);
		return (true);
	}

	if (!strn_cmp(argument, "продать", strlen(argument)) || !strn_cmp(argument, "sell", strlen(argument))) {
		if (!ch->has_horse(true)) {
			act("$N засмеял$U : \"$n, ты не влезешь в мое стойло.\"", false, ch, 0, victim, kToChar);
			return (true);
		}
		if (ch->ahorse()) {
			act("\"Я не собираюсь платить еще и за всадника.\"-усмехнул$U $N",
				false, ch, 0, victim, kToChar);
			return (true);
		}

		if (!(horse = ch->get_horse()) || GET_MOB_VNUM(horse) != HORSE_VNUM) {
			act("\"Извини, твой скакун мне не подходит.\"- заявил$G $N", false, ch, 0, victim, kToChar);
			return (true);
		}

		if (IN_ROOM(horse) != IN_ROOM(victim)) {
			act("\"Извини, твой скакун где-то бродит.\"- заявил$G $N", false, ch, 0, victim, kToChar);
			return (true);
		}

		sprintf(buf, "$N расседлал$G %s и отвел$G %s в стойло.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, false, ch, 0, victim, kToChar);
		sprintf(buf, "$N расседлал$G %s и отвел$G %s в стойло.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, false, ch, 0, victim, kToRoom);
		extract_char(horse, false);
		ch->add_gold((HORSE_COST >> 1));
		PLR_FLAGS(ch).set(PLR_CRASH);
		return (true);
	}

	return (0);
}

bool item_nouse(ObjData *obj) {
	switch (GET_OBJ_TYPE(obj)) {
		case ObjData::ITEM_LIGHT:
			if (GET_OBJ_VAL(obj, 2) == 0) {
				return true;
			}
			break;

		case ObjData::ITEM_SCROLL:
		case ObjData::ITEM_POTION:
			if (!GET_OBJ_VAL(obj, 1)
				&& !GET_OBJ_VAL(obj, 2)
				&& !GET_OBJ_VAL(obj, 3)) {
				return true;
			}
			break;

		case ObjData::ITEM_STAFF:
		case ObjData::ITEM_WAND:
			if (!GET_OBJ_VAL(obj, 2)) {
				return true;
			}
			break;

		case ObjData::ITEM_CONTAINER:
			if (!system_obj::is_purse(obj)) {
				return true;
			}
			break;

		case ObjData::ITEM_OTHER:
		case ObjData::ITEM_TRASH:
		case ObjData::ITEM_TRAP:
		case ObjData::ITEM_NOTE:
		case ObjData::ITEM_DRINKCON:
		case ObjData::ITEM_FOOD:
		case ObjData::ITEM_PEN:
		case ObjData::ITEM_BOAT:
		case ObjData::ITEM_FOUNTAIN:
		case ObjData::ITEM_MING: return true;

		default: break;
	}

	return false;
}

void npc_dropunuse(CharData *ch) {
	ObjData *obj, *nobj;
	for (obj = ch->carrying; obj; obj = nobj) {
		nobj = obj->get_next_content();
		if (item_nouse(obj)) {
			act("$n выбросил$g $o3.", false, ch, obj, 0, kToRoom);
			obj_from_char(obj);
			obj_to_room(obj, ch->in_room);
		}
	}
}

int npc_scavenge(CharData *ch) {
	int max = 1;
	ObjData *obj, *best_obj, *cont, *best_cont, *cobj;

	if (!MOB_FLAGGED(ch, MOB_SCAVENGER)) {
		return (false);
	}

	if (IS_SHOPKEEPER(ch)) {
		return (false);
	}

	npc_dropunuse(ch);
	if (world[ch->in_room]->contents && number(0, 25) <= GET_REAL_INT(ch)) {
		max = 1;
		best_obj = nullptr;
		cont = nullptr;
		best_cont = nullptr;
		for (obj = world[ch->in_room]->contents; obj; obj = obj->get_next_content()) {
			if (GET_OBJ_TYPE(obj) == ObjData::ITEM_MING
				|| Clan::is_clan_chest(obj)
				|| ClanSystem::is_ingr_chest(obj)) {
				continue;
			}

			if (GET_OBJ_TYPE(obj) == ObjData::ITEM_CONTAINER
				&& !system_obj::is_purse(obj)) {
				if (IS_CORPSE(obj)) {
					continue;
				}

				// Заперто, открываем, если есть ключ
				if (OBJVAL_FLAGGED(obj, CONT_LOCKED)
					&& has_key(ch, GET_OBJ_VAL(obj, 2))) {
					do_doorcmd(ch, obj, 0, SCMD_UNLOCK);
				}

				// Заперто, взламываем, если умеем
				if (OBJVAL_FLAGGED(obj, CONT_LOCKED)
					&& ch->get_skill(ESkill::kPickLock)
					&& ok_pick(ch, 0, obj, 0, SCMD_PICK)) {
					do_doorcmd(ch, obj, 0, SCMD_PICK);
				}
				// Все равно заперто, ну тогда фиг с ним
				if (OBJVAL_FLAGGED(obj, CONT_LOCKED)) {
					continue;
				}

				if (OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
					do_doorcmd(ch, obj, 0, SCMD_OPEN);
				}

				if (OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
					continue;
				}

				for (cobj = obj->get_contains(); cobj; cobj = cobj->get_next_content()) {
					if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj) && GET_OBJ_COST(cobj) > max) {
						cont = obj;
						best_cont = best_obj = cobj;
						max = GET_OBJ_COST(cobj);
					}
				}
			} else if (!IS_CORPSE(obj) &&
				CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max && !item_nouse(obj)) {
				best_obj = obj;
				max = GET_OBJ_COST(obj);
			}
		}
		if (best_obj != nullptr) {
			if (best_obj != best_cont) {
				act("$n поднял$g $o3.", false, ch, best_obj, 0, kToRoom);
				if (GET_OBJ_TYPE(best_obj) == ObjData::ITEM_MONEY) {
					ch->add_gold(GET_OBJ_VAL(best_obj, 0));
					extract_obj(best_obj);
				} else {
					obj_from_room(best_obj);
					obj_to_char(best_obj, ch);
				}
			} else {
				sprintf(buf, "$n достал$g $o3 из %s.", cont->get_PName(1).c_str());
				act(buf, false, ch, best_obj, 0, kToRoom);
				if (GET_OBJ_TYPE(best_obj) == ObjData::ITEM_MONEY) {
					ch->add_gold(GET_OBJ_VAL(best_obj, 0));
					extract_obj(best_obj);
				} else {
					obj_from_obj(best_obj);
					obj_to_char(best_obj, ch);
				}
			}
		}
	}
	return (max > 1);
}

int npc_loot(CharData *ch) {
	int max = false;
	ObjData *obj, *loot_obj, *next_loot, *cobj, *cnext_obj;

	if (!MOB_FLAGGED(ch, MOB_LOOTER))
		return (false);
	if (IS_SHOPKEEPER(ch))
		return (false);
	npc_dropunuse(ch);
	if (world[ch->in_room]->contents && number(0, GET_REAL_INT(ch)) > 10) {
		for (obj = world[ch->in_room]->contents; obj; obj = obj->get_next_content()) {
			if (CAN_SEE_OBJ(ch, obj) && IS_CORPSE(obj)) {
				// Сначала лутим то, что не в контейнерах
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot) {
					next_loot = loot_obj->get_next_content();
					if ((GET_OBJ_TYPE(loot_obj) != ObjData::ITEM_CONTAINER
						|| system_obj::is_purse(loot_obj))
						&& CAN_GET_OBJ(ch, loot_obj)
						&& !item_nouse(loot_obj)) {
						sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(1).c_str());
						act(buf, false, ch, loot_obj, 0, kToRoom);
						if (GET_OBJ_TYPE(loot_obj) == ObjData::ITEM_MONEY) {
							ch->add_gold(GET_OBJ_VAL(loot_obj, 0));
							extract_obj(loot_obj);
						} else {
							obj_from_obj(loot_obj);
							obj_to_char(loot_obj, ch);
							max++;
						}
					}
				}
				// Теперь не запертые контейнеры
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot) {
					next_loot = loot_obj->get_next_content();
					if (GET_OBJ_TYPE(loot_obj) == ObjData::ITEM_CONTAINER) {
						if (IS_CORPSE(loot_obj)
							|| OBJVAL_FLAGGED(loot_obj, CONT_LOCKED)
							|| system_obj::is_purse(loot_obj)) {
							continue;
						}
						auto value = loot_obj->get_val(1); //откроем контейнер
						REMOVE_BIT(value, CONT_CLOSED);
						loot_obj->set_val(1, value);
						for (cobj = loot_obj->get_contains(); cobj; cobj = cnext_obj) {
							cnext_obj = cobj->get_next_content();
							if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj)) {
								sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(1).c_str());
								act(buf, false, ch, cobj, 0, kToRoom);
								if (GET_OBJ_TYPE(cobj) == ObjData::ITEM_MONEY) {
									ch->add_gold(GET_OBJ_VAL(cobj, 0));
									extract_obj(cobj);
								} else {
									obj_from_obj(cobj);
									obj_to_char(cobj, ch);
									max++;
								}
							}
						}
					}
				}
				// И наконец, лутим запертые контейнеры если есть ключ или можем взломать
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot) {
					next_loot = loot_obj->get_next_content();
					if (GET_OBJ_TYPE(loot_obj) == ObjData::ITEM_CONTAINER) {
						if (IS_CORPSE(loot_obj)
							|| !OBJVAL_FLAGGED(loot_obj, CONT_LOCKED)
							|| system_obj::is_purse(loot_obj)) {
							continue;
						}

						// Есть ключ?
						if (OBJVAL_FLAGGED(loot_obj, CONT_LOCKED)
							&& has_key(ch, GET_OBJ_VAL(loot_obj, 2))) {
							loot_obj->toggle_val_bit(1, CONT_LOCKED);
						}

						// ...или взломаем?
						if (OBJVAL_FLAGGED(loot_obj, CONT_LOCKED)
							&& ch->get_skill(ESkill::kPickLock)
							&& ok_pick(ch, 0, loot_obj, 0, SCMD_PICK)) {
							loot_obj->toggle_val_bit(1, CONT_LOCKED);
						}

						// Эх, не открыть. Ну ладно.
						if (OBJVAL_FLAGGED(loot_obj, CONT_LOCKED)) {
							continue;
						}

						loot_obj->toggle_val_bit(1, CONT_CLOSED);
						for (cobj = loot_obj->get_contains(); cobj; cobj = cnext_obj) {
							cnext_obj = cobj->get_next_content();
							if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj)) {
								sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(1).c_str());
								act(buf, false, ch, cobj, 0, kToRoom);
								if (GET_OBJ_TYPE(cobj) == ObjData::ITEM_MONEY) {
									ch->add_gold(GET_OBJ_VAL(cobj, 0));
									extract_obj(cobj);
								} else {
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

int npc_move(CharData *ch, int dir, int/* need_specials_check*/) {
	int need_close = false, need_lock = false;
	int rev_dir[] = {kDirSouth, kDirWest, kDirNorth, kDirEast, kDirDown, kDirUp};
	int retval = false;

	if (ch == nullptr || dir < 0 || dir >= kDirMaxNumber || ch->get_fighting()) {
		return (false);
	} else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room() == kNowhere) {
		return (false);
	} else if (ch->has_master()
		&& ch->in_room == IN_ROOM(ch->get_master())) {
		return (false);
	} else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) {
		if (!EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR)) {
			return (false);
		}

		const auto &rdata = EXIT(ch, dir);

		if (EXIT_FLAGGED(rdata, EX_LOCKED)) {
			if (has_key(ch, rdata->key)
				|| (!EXIT_FLAGGED(rdata, EX_PICKPROOF)
					&& !EXIT_FLAGGED(rdata, EX_BROKEN)
					&& CalcCurrentSkill(ch, ESkill::kPicks, 0) >= number(0, 100))) {
				do_doorcmd(ch, 0, dir, SCMD_UNLOCK);
				need_lock = true;
			} else {
				return (false);
			}
		}
		if (EXIT_FLAGGED(rdata, EX_CLOSED)) {
			if (GET_REAL_INT(ch) >= 15
				|| GET_DEST(ch) != kNowhere
				|| MOB_FLAGGED(ch, MOB_OPENDOOR)) {
				do_doorcmd(ch, 0, dir, SCMD_OPEN);
				need_close = true;
			}
		}
	}

	retval = perform_move(ch, dir, 1, false, 0);

	if (need_close) {
		const int close_direction = retval ? rev_dir[dir] : dir;
		// закрываем за собой только существующую дверь
		if (EXIT(ch, close_direction) &&
			EXIT_FLAGGED(EXIT(ch, close_direction), EX_ISDOOR) &&
			EXIT(ch, close_direction)->to_room() != kNowhere) {
			do_doorcmd(ch, 0, close_direction, SCMD_CLOSE);
		}
	}

	if (need_lock) {
		const int lock_direction = retval ? rev_dir[dir] : dir;
		// запираем за собой только существующую дверь
		if (EXIT(ch, lock_direction) &&
			EXIT_FLAGGED(EXIT(ch, lock_direction), EX_ISDOOR) &&
			EXIT(ch, lock_direction)->to_room() != kNowhere) {
			do_doorcmd(ch, 0, lock_direction, SCMD_LOCK);
		}
	}

	return (retval);
}

int has_curse(ObjData *obj) {
	for (const auto &i : weapon_affect) {
		// Замена выражения на макрос
		if (i.aff_spell <= 0 || !IS_OBJ_AFF(obj, i.aff_pos)) {
			continue;
		}
		if (IS_SET(spell_info[i.aff_spell].routines, kNpcAffectPc | kNpcDamagePc)) {
			return true;
		}
	}
	return false;
}

int calculate_weapon_class(CharData *ch, ObjData *weapon) {
	int damage = 0, hits = 0, i;

	if (!weapon
		|| GET_OBJ_TYPE(weapon) != ObjData::ITEM_WEAPON) {
		return 0;
	}

	hits = CalcCurrentSkill(ch, static_cast<ESkill>(GET_OBJ_SKILL(weapon)), 0);
	damage = (GET_OBJ_VAL(weapon, 1) + 1) * (GET_OBJ_VAL(weapon, 2)) / 2;
	for (i = 0; i < kMaxObjAffect; i++) {
		auto &affected = weapon->get_affected(i);
		if (affected.location == APPLY_DAMROLL) {
			damage += affected.modifier;
		}

		if (affected.location == APPLY_HITROLL) {
			hits += affected.modifier * 10;
		}
	}

	if (has_curse(weapon))
		return (0);

	return (damage + (hits > 200 ? 10 : hits / 20));
}

void best_weapon(CharData *ch, ObjData *sweapon, ObjData **dweapon) {
	if (*dweapon == nullptr) {
		if (calculate_weapon_class(ch, sweapon) > 0) {
			*dweapon = sweapon;
		}
	} else if (calculate_weapon_class(ch, sweapon) > calculate_weapon_class(ch, *dweapon)) {
		*dweapon = sweapon;
	}
}

void npc_wield(CharData *ch) {
	ObjData *obj, *next, *right = nullptr, *left = nullptr, *both = nullptr;

	if (!NPC_FLAGGED(ch, NPC_WIELDING))
		return;

	if (ch->get_skill(ESkill::kHammer) > 0
		&& ch->get_skill(ESkill::kOverwhelm) < ch->get_skill(ESkill::kHammer)) {
		return;
	}

	if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	if (GET_EQ(ch, WEAR_HOLD)
		&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ObjData::ITEM_WEAPON) {
		left = GET_EQ(ch, WEAR_HOLD);
	}
	if (GET_EQ(ch, WEAR_WIELD)
		&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ObjData::ITEM_WEAPON) {
		right = GET_EQ(ch, WEAR_WIELD);
	}
	if (GET_EQ(ch, WEAR_BOTHS)
		&& GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS)) == ObjData::ITEM_WEAPON) {
		both = GET_EQ(ch, WEAR_BOTHS);
	}

	if (GET_REAL_INT(ch) < 15 && ((left && right) || (both)))
		return;

	for (obj = ch->carrying; obj; obj = next) {
		next = obj->get_next_content();
		if (GET_OBJ_TYPE(obj) != ObjData::ITEM_WEAPON
			|| GET_OBJ_UID(obj) != 0) {
			continue;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD)
			&& OK_HELD(ch, obj)) {
			best_weapon(ch, obj, &left);
		} else if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD)
			&& OK_WIELD(ch, obj)) {
			best_weapon(ch, obj, &right);
		} else if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS)
			&& OK_BOTH(ch, obj)) {
			best_weapon(ch, obj, &both);
		}
	}

	if (both
		&& calculate_weapon_class(ch, both) > calculate_weapon_class(ch, left) + calculate_weapon_class(ch, right)) {
		if (both == GET_EQ(ch, WEAR_BOTHS)) {
			return;
		}
		if (GET_EQ(ch, WEAR_BOTHS)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, WEAR_BOTHS), 0, kToRoom);
			obj_to_char(unequip_char(ch, WEAR_BOTHS, CharEquipFlag::show_msg), ch);
		}
		if (GET_EQ(ch, WEAR_WIELD)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, WEAR_WIELD), 0, kToRoom);
			obj_to_char(unequip_char(ch, WEAR_WIELD, CharEquipFlag::show_msg), ch);
		}
		if (GET_EQ(ch, WEAR_SHIELD)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, WEAR_SHIELD), 0, kToRoom);
			obj_to_char(unequip_char(ch, WEAR_SHIELD, CharEquipFlag::show_msg), ch);
		}
		if (GET_EQ(ch, WEAR_HOLD)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, WEAR_HOLD), 0, kToRoom);
			obj_to_char(unequip_char(ch, WEAR_HOLD, CharEquipFlag::show_msg), ch);
		}
		//obj_from_char(both);
		equip_char(ch, both, WEAR_BOTHS, CharEquipFlag::show_msg);
	} else {
		if (left && GET_EQ(ch, WEAR_HOLD) != left) {
			if (GET_EQ(ch, WEAR_BOTHS)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, WEAR_BOTHS), 0, kToRoom);
				obj_to_char(unequip_char(ch, WEAR_BOTHS, CharEquipFlag::show_msg), ch);
			}
			if (GET_EQ(ch, WEAR_SHIELD)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, WEAR_SHIELD), 0, kToRoom);
				obj_to_char(unequip_char(ch, WEAR_SHIELD, CharEquipFlag::show_msg), ch);
			}
			if (GET_EQ(ch, WEAR_HOLD)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, WEAR_HOLD), 0, kToRoom);
				obj_to_char(unequip_char(ch, WEAR_HOLD, CharEquipFlag::show_msg), ch);
			}
			//obj_from_char(left);
			equip_char(ch, left, WEAR_HOLD, CharEquipFlag::show_msg);
		}
		if (right && GET_EQ(ch, WEAR_WIELD) != right) {
			if (GET_EQ(ch, WEAR_BOTHS)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, WEAR_BOTHS), 0, kToRoom);
				obj_to_char(unequip_char(ch, WEAR_BOTHS, CharEquipFlag::show_msg), ch);
			}
			if (GET_EQ(ch, WEAR_WIELD)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, WEAR_WIELD), 0, kToRoom);
				obj_to_char(unequip_char(ch, WEAR_WIELD, CharEquipFlag::show_msg), ch);
			}
			//obj_from_char(right);
			equip_char(ch, right, WEAR_WIELD, CharEquipFlag::show_msg);
		}
	}
}

void npc_armor(CharData *ch) {
	ObjData *obj, *next;
	int where = 0;

	if (!NPC_FLAGGED(ch, NPC_ARMORING))
		return;

	if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	for (obj = ch->carrying; obj; obj = next) {
		next = obj->get_next_content();

		if (!ObjSystem::is_armor_type(obj)
			|| !no_bad_affects(obj)) {
			continue;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FINGER)) {
			where = WEAR_FINGER_R;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_NECK)) {
			where = WEAR_NECK_1;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BODY)) {
			where = WEAR_BODY;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HEAD)) {
			where = WEAR_HEAD;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_LEGS)) {
			where = WEAR_LEGS;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_FEET)) {
			where = WEAR_FEET;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HANDS)) {
			where = WEAR_HANDS;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ARMS)) {
			where = WEAR_ARMS;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_SHIELD)) {
			where = WEAR_SHIELD;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_ABOUT)) {
			where = WEAR_ABOUT;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WAIST)) {
			where = WEAR_WAIST;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_QUIVER)) {
			where = WEAR_QUIVER;
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WRIST)) {
			where = WEAR_WRIST_R;
		}

		if (!where) {
			continue;
		}

		if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) || (where == WEAR_WRIST_R)) {
			if (GET_EQ(ch, where)) {
				where++;
			}
		}

		if (where == WEAR_SHIELD && (GET_EQ(ch, WEAR_BOTHS) || GET_EQ(ch, WEAR_HOLD))) {
			continue;
		}

		if (GET_EQ(ch, where)) {
			if (GET_REAL_INT(ch) < 15) {
				continue;
			}

			if (GET_OBJ_VAL(obj, 0) + GET_OBJ_VAL(obj, 1) * 3
				<= GET_OBJ_VAL(GET_EQ(ch, where), 0) + GET_OBJ_VAL(GET_EQ(ch, where), 1) * 3
				|| has_curse(obj)) {
				continue;
			}
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, where), 0, kToRoom);
			obj_to_char(unequip_char(ch, where, CharEquipFlag::show_msg), ch);
		}
		//obj_from_char(obj);
		equip_char(ch, obj, where, CharEquipFlag::show_msg);
		break;
	}
}

void npc_light(CharData *ch) {
	ObjData *obj, *next;

	if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_INFRAVISION))
		return;

	if ((obj = GET_EQ(ch, WEAR_LIGHT)) && (GET_OBJ_VAL(obj, 2) == 0 || !IS_DARK(ch->in_room))) {
		act("$n прекратил$g использовать $o3.", false, ch, obj, 0, kToRoom);
		obj_to_char(unequip_char(ch, WEAR_LIGHT, CharEquipFlag::show_msg), ch);
	}

	if (!GET_EQ(ch, WEAR_LIGHT) && IS_DARK(ch->in_room)) {
		for (obj = ch->carrying; obj; obj = next) {
			next = obj->get_next_content();
			if (GET_OBJ_TYPE(obj) != ObjData::ITEM_LIGHT) {
				continue;
			}
			if (GET_OBJ_VAL(obj, 2) == 0) {
				act("$n выбросил$g $o3.", false, ch, obj, 0, kToRoom);
				obj_from_char(obj);
				obj_to_room(obj, ch->in_room);
				continue;
			}
			//obj_from_char(obj);
			equip_char(ch, obj, WEAR_LIGHT, CharEquipFlag::show_msg);
			return;
		}
	}
}

int npc_battle_scavenge(CharData *ch) {
	int max = false;
	ObjData *obj, *next_obj = nullptr;

	if (!MOB_FLAGGED(ch, MOB_SCAVENGER))
		return (false);

	if (IS_SHOPKEEPER(ch))
		return (false);

	if (world[ch->in_room]->contents && number(0, GET_REAL_INT(ch)) > 10)
		for (obj = world[ch->in_room]->contents; obj; obj = next_obj) {
			next_obj = obj->get_next_content();
			if (CAN_GET_OBJ(ch, obj)
				&& !has_curse(obj)
				&& (ObjSystem::is_armor_type(obj)
					|| GET_OBJ_TYPE(obj) == ObjData::ITEM_WEAPON)) {
				obj_from_room(obj);
				obj_to_char(obj, ch);
				act("$n поднял$g $o3.", false, ch, obj, 0, kToRoom);
				max = true;
			}
		}
	return (max);
}

int npc_walk(CharData *ch) {
	int rnum, door = BFS_ERROR;

	if (ch->in_room == kNowhere)
		return (BFS_ERROR);

	if (GET_DEST(ch) == kNowhere || (rnum = real_room(GET_DEST(ch))) == kNowhere)
		return (BFS_ERROR);

	if (ch->in_room == rnum) {
		if (ch->mob_specials.dest_count == 1)
			return (BFS_ALREADY_THERE);
		if (ch->mob_specials.dest_pos == ch->mob_specials.dest_count - 1 && ch->mob_specials.dest_dir >= 0)
			ch->mob_specials.dest_dir = -1;
		if (!ch->mob_specials.dest_pos && ch->mob_specials.dest_dir < 0)
			ch->mob_specials.dest_dir = 0;
		ch->mob_specials.dest_pos += ch->mob_specials.dest_dir >= 0 ? 1 : -1;
		if (((rnum = real_room(GET_DEST(ch))) == kNowhere)
			|| rnum == ch->in_room)
			return (BFS_ERROR);
		else
			return (npc_walk(ch));
	}

	door = find_first_step(ch->in_room, real_room(GET_DEST(ch)), ch);

	return (door);
}

int do_npc_steal(CharData *ch, CharData *victim) {
	ObjData *obj, *best = nullptr;
	int gold;
	int max = 0;

	if (!NPC_FLAGGED(ch, NPC_STEALING))
		return (false);

	if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
		return (false);

	if (IS_NPC(victim) || IS_SHOPKEEPER(ch) || victim->get_fighting())
		return (false);

	if (GetRealLevel(victim) >= kLvlImmortal)
		return (false);

	if (!CAN_SEE(ch, victim))
		return (false);

	if (AWAKE(victim) && (number(0, MAX(0, GetRealLevel(ch) - int_app[GET_REAL_INT(victim)].observation)) == 0)) {
		act("Вы обнаружили руку $n1 в своем кармане.", false, ch, 0, victim, kToVict);
		act("$n пытал$u обокрасть $N3.", true, ch, 0, victim, kToNotVict);
	} else        // Steal some gold coins
	{
		gold = (int) ((victim->get_gold() * number(1, 10)) / 100);
		if (gold > 0) {
			ch->add_gold(gold);
			victim->remove_gold(gold);
		}
		// Steal something from equipment
		if (IS_CARRYING_N(ch) < CAN_CARRY_N(ch) && CalcCurrentSkill(ch, ESkill::kSteal, victim)
			>= number(1, 100) - (AWAKE(victim) ? 100 : 0)) {
			for (obj = victim->carrying; obj; obj = obj->get_next_content())
				if (CAN_SEE_OBJ(ch, obj) && IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)
					<= CAN_CARRY_W(ch) && (!best || GET_OBJ_COST(obj) > GET_OBJ_COST(best)))
					best = obj;
			if (best) {
				obj_from_char(best);
				obj_to_char(best, ch);
				max++;
			}
		}
	}
	return (max);
}

int npc_steal(CharData *ch) {
	if (!NPC_FLAGGED(ch, NPC_STEALING))
		return (false);

	if (GET_POS(ch) != EPosition::kStand || IS_SHOPKEEPER(ch) || ch->get_fighting())
		return (false);

	for (const auto cons : world[ch->in_room]->people) {
		if (!IS_NPC(cons)
			&& !IS_IMMORTAL(cons)
			&& (number(0, GET_REAL_INT(ch)) > 10)) {
			return (do_npc_steal(ch, cons));
		}
	}

	return false;
}

#define ZONE(ch)  (GET_MOB_VNUM(ch) / 100)
#define GROUP(ch) ((GET_MOB_VNUM(ch) % 100) / 10)

void npc_group(CharData *ch) {
	CharData *leader = nullptr;
	int zone = ZONE(ch), group = GROUP(ch), members = 0;

	if (GET_DEST(ch) == kNowhere || ch->in_room == kNowhere)
		return;

	// ноугруп мобы не вступают в группу
	if (MOB_FLAGGED(ch, MOB_NOGROUP)) {
		return;
	}

	if (ch->has_master()
		&& ch->in_room == IN_ROOM(ch->get_master())) {
		leader = ch->get_master();
	}

	if (!ch->has_master()) {
		leader = ch;
	}

	if (leader
		&& (AFF_FLAGGED(leader, EAffectFlag::AFF_CHARM)
			|| GET_POS(leader) < EPosition::kSleep)) {
		leader = nullptr;
	}

	// ноугруп моб не может быть лидером
	if (leader
		&& MOB_FLAGGED(leader, MOB_NOGROUP)) {
		leader = nullptr;
	}

	// Find leader
	for (const auto vict : world[ch->in_room]->people) {
		if (!IS_NPC(vict)
			|| GET_DEST(vict) != GET_DEST(ch)
			|| zone != ZONE(vict)
			|| group != GROUP(vict)
			|| MOB_FLAGGED(vict, MOB_NOGROUP)
			|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
			|| GET_POS(vict) < EPosition::kSleep) {
			continue;
		}

		members++;

		if (!leader
			|| GET_REAL_INT(vict) > GET_REAL_INT(leader)) {
			leader = vict;
		}
	}

	if (members <= 1) {
		if (ch->has_master()) {
			stop_follower(ch, SF_EMPTY);
		}

		return;
	}

	if (leader->has_master()) {
		stop_follower(leader, SF_EMPTY);
	}

	// Assign leader
	for (const auto vict : world[ch->in_room]->people) {
		if (!IS_NPC(vict)
			|| GET_DEST(vict) != GET_DEST(ch)
			|| zone != ZONE(vict)
			|| group != GROUP(vict)
			|| AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM)
			|| GET_POS(vict) < EPosition::kSleep) {
			continue;
		}

		if (vict == leader) {
			AFF_FLAGS(vict).set(EAffectFlag::AFF_GROUP);
			continue;
		}

		if (!vict->has_master()) {
			leader->add_follower(vict);
		} else if (vict->get_master() != leader) {
			stop_follower(vict, SF_EMPTY);
			leader->add_follower(vict);
		}
		AFF_FLAGS(vict).set(EAffectFlag::AFF_GROUP);
	}
}

void npc_groupbattle(CharData *ch) {
	struct Follower *k;
	CharData *tch, *helper;

	if (!IS_NPC(ch)
		|| !ch->get_fighting()
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		|| !ch->has_master()
		|| ch->in_room == kNowhere
		|| !ch->followers) {
		return;
	}

	k = ch->has_master() ? ch->get_master()->followers : ch->followers;
	tch = ch->has_master() ? ch->get_master() : ch;
	for (; k; (k = tch ? k : k->next), tch = nullptr) {
		helper = tch ? tch : k->ch;
		if (ch->in_room == IN_ROOM(helper)
			&& !helper->get_fighting()
			&& !IS_NPC(helper)
			&& GET_POS(helper) > EPosition::kStun) {
			GET_POS(helper) = EPosition::kStand;
			set_fighting(helper, ch->get_fighting());
			act("$n вступил$u за $N3.", false, helper, 0, ch, kToRoom);
		}
	}
}

int dump(CharData *ch, void * /*me*/, int cmd, char *argument) {
	ObjData *k;
	int value = 0;

	for (k = world[ch->in_room]->contents; k; k = world[ch->in_room]->contents) {
		act("$p рассыпал$U в прах!", false, 0, k, 0, kToRoom);
		extract_obj(k);
	}

	if (!CMD_IS("drop") || !CMD_IS("бросить"))
		return (0);

	do_drop(ch, argument, cmd, 0);

	for (k = world[ch->in_room]->contents; k; k = world[ch->in_room]->contents) {
		act("$p рассыпал$U в прах!", false, 0, k, 0, kToRoom);
		value += MAX(1, MIN(1, GET_OBJ_COST(k) / 10));
		extract_obj(k);
	}

	if (value) {
		send_to_char("Боги оценили вашу жертву.\r\n", ch);
		act("$n оценен$y Богами.", true, ch, 0, 0, kToRoom);
		if (GetRealLevel(ch) < 3)
			gain_exp(ch, value);
		else
			ch->add_gold(value);
	}
	return (1);
}

#if 0
void mayor(CharacterData *ch, void *me, int cmd, char* argument)
{
const char open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
const char close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

static const char *path = nullptr;
static int index;
static bool move = false;

if (!move)
{
if (time_info.hours == 6)
{
move = true;
path = open_path;
index = 0;
}
else if (time_info.hours == 20)
{
move = true;
path = close_path;
index = 0;
}
}
if (cmd || !move || (GET_POS(ch) < EPosition::kSleep) || (GET_POS(ch) == EPosition::kFight))
return (false);

switch (path[index])
{
case '0':
case '1':
case '2':
case '3':
perform_move(ch, path[index] - '0', 1, false);
break;

case 'W':
GET_POS(ch) = EPosition::kStand;
act("$n awakens and groans loudly.", false, ch, 0, 0, TO_ROOM);
break;

case 'S':
GET_POS(ch) = EPosition::kSleep;
act("$n lies down and instantly falls asleep.", false, ch, 0, 0, TO_ROOM);
break;

case 'a':
act("$n says 'Hello Honey!'", false, ch, 0, 0, TO_ROOM);
act("$n smirks.", false, ch, 0, 0, TO_ROOM);
break;

case 'b':
act("$n says 'What a view!  I must get something done about that dump!'", false, ch, 0, 0, TO_ROOM);
break;

case 'c':
act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'", false, ch, 0, 0, TO_ROOM);
break;

case 'd':
act("$n says 'Good day, citizens!'", false, ch, 0, 0, TO_ROOM);
break;

case 'e':
act("$n says 'I hereby declare the bazaar open!'", false, ch, 0, 0, TO_ROOM);
break;

case 'E':
act("$n says 'I hereby declare Midgaard closed!'", false, ch, 0, 0, TO_ROOM);
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
move = false;
break;

}

index++;
return (false);
}
#endif

// ********************************************************************
// *  General special procedures for mobiles                          *
// ********************************************************************

//int thief(CharacterData *ch, void* /*me*/, int cmd, char* /*argument*/)
/*
{
	if (cmd)
		return (false);

	if (GET_POS(ch) != EPosition::kStand)
		return (false);

	for (const auto cons : world[ch->in_room]->people)
	{
		if (!IS_NPC(cons)
			&& GetRealLevel(cons) < kLevelImmortal
			&& !number(0, 4))
		{
			do_npc_steal(ch, cons);

			return true;
		}
	}

	return false;
}
*/
int magic_user(CharData *ch, void * /*me*/, int cmd, char * /*argument*/) {
	if (cmd || GET_POS(ch) != EPosition::kFight) {
		return (false);
	}

	CharData *target = nullptr;
	// pseudo-randomly choose someone in the room who is fighting me //
	for (const auto vict : world[ch->in_room]->people) {
		if (vict->get_fighting() == ch
			&& !number(0, 4)) {
			target = vict;

			break;
		}
	}

	// if I didn't pick any of those, then just slam the guy I'm fighting //
	if (target == nullptr
		&& IN_ROOM(ch->get_fighting()) == ch->in_room) {
		target = ch->get_fighting();
	}

	// Hm...didn't pick anyone...I'll wait a round. //
	if (target == nullptr) {
		return (true);
	}

	if ((GetRealLevel(ch) > 13) && (number(0, 10) == 0)) {
		CastSpell(ch, target, nullptr, nullptr, kSpellSleep, kSpellSleep);
	}

	if ((GetRealLevel(ch) > 7) && (number(0, 8) == 0)) {
		CastSpell(ch, target, nullptr, nullptr, kSpellBlindness, kSpellBlindness);
	}

	if ((GetRealLevel(ch) > 12) && (number(0, 12) == 0)) {
		if (IS_EVIL(ch)) {
			CastSpell(ch, target, nullptr, nullptr, kSpellEnergyDrain, kSpellEnergyDrain);
		} else if (IS_GOOD(ch)) {
			CastSpell(ch, target, nullptr, nullptr, kSpellDispelEvil, kSpellDispelEvil);
		}
	}

	if (number(0, 4)) {
		return (true);
	}

	switch (GetRealLevel(ch)) {
		case 4:
		case 5: CastSpell(ch, target, nullptr, nullptr, kSpellMagicMissile, kSpellMagicMissile);
			break;
		case 6:
		case 7: CastSpell(ch, target, nullptr, nullptr, kSpellChillTouch, kSpellChillTouch);
			break;
		case 8:
		case 9: CastSpell(ch, target, nullptr, nullptr, kSpellBurningHands, kSpellBurningHands);
			break;
		case 10:
		case 11: CastSpell(ch, target, nullptr, nullptr, kSpellShockingGasp, kSpellShockingGasp);
			break;
		case 12:
		case 13: CastSpell(ch, target, nullptr, nullptr, kSpellLightingBolt, kSpellLightingBolt);
			break;
		case 14:
		case 15:
		case 16:
		case 17: CastSpell(ch, target, nullptr, nullptr, kSpellIceBolts, kSpellIceBolts);
			break;
		default: CastSpell(ch, target, nullptr, nullptr, kSpellFireball, kSpellFireball);
			break;
	}

	return (true);
}


// ********************************************************************
// *  Special procedures for mobiles                                  *
// ********************************************************************

int guild_guard(CharData *ch, void *me, int cmd, char * /*argument*/) {
	int i;
	CharData *guard = (CharData *) me;
	const char *buf = "Охранник остановил вас, преградив дорогу.\r\n";
	const char *buf2 = "Охранник остановил $n, преградив $m дорогу.";

	if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, EAffectFlag::AFF_BLIND)
		|| AFF_FLAGGED(guard, EAffectFlag::AFF_HOLD))
		return (false);

	if (GetRealLevel(ch) >= kLvlImmortal)
		return (false);

	for (i = 0; guild_info[i][0] != -1; i++) {
		if ((IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) &&
			GET_ROOM_VNUM(ch->in_room) == guild_info[i][1] && cmd == guild_info[i][2]) {
			send_to_char(buf, ch);
			act(buf2, false, ch, 0, 0, kToRoom);
			return (true);
		}
	}

	return (false);
}

// TODO: повырезать все это
int puff(CharData * /*ch*/, void * /*me*/, int/* cmd*/, char * /*argument*/) {
	return 0;
}

int fido(CharData *ch, void * /*me*/, int cmd, char * /*argument*/) {
	ObjData *i, *temp, *next_obj;

	if (cmd || !AWAKE(ch))
		return (false);

	for (i = world[ch->in_room]->contents; i; i = i->get_next_content()) {
		if (IS_CORPSE(i)) {
			act("$n savagely devours a corpse.", false, ch, 0, 0, kToRoom);
			for (temp = i->get_contains(); temp; temp = next_obj) {
				next_obj = temp->get_next_content();
				obj_from_obj(temp);
				obj_to_room(temp, ch->in_room);
			}
			extract_obj(i);
			return (true);
		}
	}
	return (false);
}

int janitor(CharData *ch, void * /*me*/, int cmd, char * /*argument*/) {
	ObjData *i;

	if (cmd || !AWAKE(ch))
		return (false);

	for (i = world[ch->in_room]->contents; i; i = i->get_next_content()) {
		if (!CAN_WEAR(i, EWearFlag::ITEM_WEAR_TAKE)) {
			continue;
		}

		if (GET_OBJ_TYPE(i) != ObjData::ITEM_DRINKCON
			&& GET_OBJ_COST(i) >= 15) {
			continue;
		}

		act("$n picks up some trash.", false, ch, 0, 0, kToRoom);
		obj_from_room(i);
		obj_to_char(i, ch);

		return true;
	}

	return false;
}

int cityguard(CharData *ch, void * /*me*/, int cmd, char * /*argument*/) {
	CharData *evil;
	int max_evil;

	if (cmd
		|| !AWAKE(ch)
		|| ch->get_fighting()) {
		return (false);
	}

	max_evil = 1000;
	evil = 0;

	for (const auto tch : world[ch->in_room]->people) {
		if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
			act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", false, ch, 0, 0, kToRoom);
			hit(ch, tch, ESkill::kUndefined, fight::kMainHand);

			return (true);
		}
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_THIEF)) {
			act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", false, ch, 0, 0, kToRoom);
			hit(ch, tch, ESkill::kUndefined, fight::kMainHand);

			return (true);
		}
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (CAN_SEE(ch, tch) && tch->get_fighting()) {
			if ((GET_ALIGNMENT(tch) < max_evil) && (IS_NPC(tch) || IS_NPC(tch->get_fighting()))) {
				max_evil = GET_ALIGNMENT(tch);
				evil = tch;
			}
		}
	}

	if (evil
		&& (GET_ALIGNMENT(evil->get_fighting()) >= 0)) {
		act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", false, ch, 0, 0, kToRoom);
		hit(ch, evil, ESkill::kUndefined, fight::kMainHand);

		return (true);
	}

	return (false);
}

#define PET_PRICE(pet) (GetRealLevel(pet) * 300)

int pet_shops(CharData *ch, void * /*me*/, int cmd, char *argument) {
	char buf[kMaxStringLength], pet_name[256];
	RoomRnum pet_room;
	CharData *pet;

	pet_room = ch->in_room + 1;

	if (CMD_IS("list")) {
		send_to_char("Available pets are:\r\n", ch);
		for (const auto pet : world[pet_room]->people) {
			sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
			send_to_char(buf, ch);
		}

		return (true);
	} else if (CMD_IS("buy")) {
		two_arguments(argument, buf, pet_name);

		if (!(pet = get_char_room(buf, pet_room))) {
			send_to_char("There is no such pet!\r\n", ch);
			return (true);
		}
		if (ch->get_gold() < PET_PRICE(pet)) {
			send_to_char("You don't have enough gold!\r\n", ch);
			return (true);
		}
		ch->remove_gold(PET_PRICE(pet));

		pet = read_mobile(GET_MOB_RNUM(pet), REAL);
		pet->set_exp(0);
		AFF_FLAGS(pet).set(EAffectFlag::AFF_CHARM);

		if (*pet_name) {
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
		act("$n buys $N as a pet.", false, ch, 0, pet, kToRoom);

		return (1);
	}

	// All commands except list and buy
	return (0);
}

CharData *get_player_of_name(const char *name) {
	for (const auto &i : character_list) {
		if (IS_NPC(i)) {
			continue;
		}

		if (!isname(name, i->get_pc_name())) {
			continue;
		}

		return i.get();
	}

	return nullptr;
}

// ********************************************************************
// *  Special procedures for objects                                  *
// ********************************************************************
int bank(CharData *ch, void * /*me*/, int cmd, char *argument) {
	int amount;
	CharData *vict;

	if (CMD_IS("balance") || CMD_IS("баланс") || CMD_IS("сальдо")) {
		if (ch->get_bank() > 0)
			sprintf(buf, "У вас на счету %ld %s.\r\n",
					ch->get_bank(), desc_count(ch->get_bank(), WHAT_MONEYa));
		else
			sprintf(buf, "У вас нет денег.\r\n");
		send_to_char(buf, ch);
		return (1);
	} else if (CMD_IS("deposit") || CMD_IS("вложить") || CMD_IS("вклад")) {
		if ((amount = atoi(argument)) <= 0) {
			send_to_char("Сколько вы хотите вложить?\r\n", ch);
			return (1);
		}
		if (ch->get_gold() < amount) {
			send_to_char("О такой сумме вы можете только мечтать!\r\n", ch);
			return (1);
		}
		ch->remove_gold(amount, false);
		ch->add_bank(amount, false);
		sprintf(buf, "Вы вложили %d %s.\r\n", amount, desc_count(amount, WHAT_MONEYu));
		send_to_char(buf, ch);
		act("$n произвел$g финансовую операцию.", true, ch, nullptr, nullptr, kToRoom);
		return (1);
	} else if (CMD_IS("withdraw") || CMD_IS("получить")) {
		if ((amount = atoi(argument)) <= 0) {
			send_to_char("Уточните количество денег, которые вы хотите получить?\r\n", ch);
			return (1);
		}
		if (ch->get_bank() < amount) {
			send_to_char("Да вы отродясь столько денег не видели!\r\n", ch);
			return (1);
		}
		ch->add_gold(amount, false);
		ch->remove_bank(amount, false);
		sprintf(buf, "Вы сняли %d %s.\r\n", amount, desc_count(amount, WHAT_MONEYu));
		send_to_char(buf, ch);
		act("$n произвел$g финансовую операцию.", true, ch, nullptr, nullptr, kToRoom);
		return (1);
	} else if (CMD_IS("transfer") || CMD_IS("перевести")) {
		argument = one_argument(argument, arg);
		amount = atoi(argument);
		if (IS_GOD(ch) && !IS_IMPL(ch)) {
			send_to_char("Почитить захотелось?\r\n", ch);
			return (1);

		}
		if (!*arg) {
			send_to_char("Уточните кому вы хотите перевести?\r\n", ch);
			return (1);
		}
		if (amount <= 0) {
			send_to_char("Уточните количество денег, которые вы хотите получить?\r\n", ch);
			return (1);
		}
		if (amount <= 100) {
			if (ch->get_bank() < (amount + 5)) {
				send_to_char("У вас не хватит денег на налоги!\r\n", ch);
				return (1);
			}
		}

		if (ch->get_bank() < amount) {
			send_to_char("Да вы отродясь столько денег не видели!\r\n", ch);
			return (1);
		}
		if (ch->get_bank() < amount + ((amount * 5) / 100)) {
			send_to_char("У вас не хватит денег на налоги!\r\n", ch);
			return (1);
		}

		if ((vict = get_player_of_name(arg))) {
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
			sprintf(buf,
					"<%s> {%d} перевел %d кун банковским переводом %s.",
					ch->get_name().c_str(),
					GET_ROOM_VNUM(ch->in_room),
					amount,
					GET_PAD(vict, 2));
			mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
			return (1);

		} else {
			vict = new Player; // TODO: переделать на стек
			if (load_char(arg, vict) < 0) {
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
			sprintf(buf,
					"<%s> {%d} перевел %d кун банковским переводом %s.",
					ch->get_name().c_str(),
					GET_ROOM_VNUM(ch->in_room),
					amount,
					GET_PAD(vict, 2));
			mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
			vict->add_bank(amount);
			Depot::add_offline_money(GET_UNIQUE(vict), amount);
			vict->save_char();

			delete vict;
			return (1);
		}
	} else if (CMD_IS("казна"))
		return (Clan::BankManage(ch, argument));
	return 0;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
