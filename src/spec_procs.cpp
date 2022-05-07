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

#include <boost/algorithm/string.hpp>

#include "act_movement.h"
#include "utils/utils_char_obj.inl"
#include "entities/char_player.h"
#include "game_mechanics/mount.h"
#include "entities/player_races.h"
#include "entities/world_characters.h"
#include "depot.h"
#include "game_fight/fight.h"
#include "game_fight/fight_hit.h"
#include "handler.h"
#include "house.h"
#include "game_magic/magic.h"
#include "color.h"
#include "game_magic/magic_utils.h"
#include "game_magic/magic_temp_spells.h"
#include "structs/global_objects.h"

extern int guild_info[][3];
extern CharData *get_player_of_name(const char *name);

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

int feat_slot_lvl(int remort, int slot_for_remort, int slot) {
	int result = 0;
	for (result = 1; result < kLvlImmortal; result++) {
		if (result * (5 + remort / slot_for_remort) / kLastFeatSlotLvl == slot) {
			break;
		}
	}
	/*
	  ВНИМАНИЕ: формула содрана с CalcFeatLvl (feats.h)!
	  (1+GetRealLevel(ch)*(5+GET_REAL_REMORT(ch)/MUD::Classes(ch->get_class()).GetRemortsNumForFeatSlot()/kLastFeatSlotLvl)
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
	int i = 0, j = 0, slot, max_slot = 0;
	char msg[kMaxStringLength];
	bool sfound;

	//Найдем максимальный слот, который вобще может потребоваться данному персонажу на текущем морте
	max_slot = CalcFeatSlotsAmount(ch);
	char **names = new char *[max_slot];

	for (int k = 0; k < max_slot; k++) {
		names[k] = new char[kMaxStringLength];
	}

	if (all_feats) {
		sprintf(names[0], "\r\nКруг 1  (1  уровень):\r\n");
	} else {
		*names[0] = '\0';
	}
	for (i = 1; i < max_slot; i++) {
		if (all_feats) {
			// на каком уровне будет слот i?
			j = feat_slot_lvl(GET_REMORT(ch), MUD::Classes(ch->GetClass()).GetRemortsNumForFeatSlot(), i);
			sprintf(names[i], "\r\nКруг %-2d (%-2d уровень):\r\n", i + 1, j);
		} else {
			*names[i] = '\0';
		}
	}

	sprintf(buf2, "\r\nВрожденные способности :\r\n");
	j = 0;
	if (all_feats) {
		if (clr(vict, C_NRM)) // реж цвет >= обычный
			SendMsgToChar(" Список способностей, доступных с текущим числом перевоплощений.\r\n"
						 " &gЗеленым цветом и пометкой [И] выделены уже изученные способности.\r\n&n"
						 " Пометкой [Д] выделены доступные для изучения способности.\r\n"
						 " &rКрасным цветом и пометкой [Н] выделены способности, недоступные вам в настоящий момент.&n\r\n\r\n",
						 vict);
		else
			SendMsgToChar(" Список способностей, доступных с текущим числом перевоплощений.\r\n"
						 " Пометкой [И] выделены уже изученные способности.\r\n"
						 " Пометкой [Д] выделены доступные для изучения способности.\r\n"
						 " Пометкой [Н] выделены способности, недоступные вам в настоящий момент.\r\n\r\n", vict);
		for (const auto &feat : MUD::Classes(ch->GetClass()).feats) {
			if (feat.IsUnavailable() &&
				!PlayerRace::FeatureCheck((int) GET_KIN(ch), (int) GET_RACE(ch), to_underlying(feat.GetId()))) {
				continue;
			}
			if (clr(vict, C_NRM)) {
				sprintf(buf, "        %s%s %-30s%s\r\n",
						ch->HaveFeat(feat.GetId()) ? KGRN :
						CanGetFeat(ch, feat.GetId()) ? KNRM : KRED,
						ch->HaveFeat(feat.GetId()) ? "[И]" :
						CanGetFeat(ch, feat.GetId()) ? "[Д]" : "[Н]",
						feat_info[feat.GetId()].name, KNRM);
			} else {
				sprintf(buf, "    %s %-30s\r\n",
						ch->HaveFeat(feat.GetId()) ? "[И]" :
						CanGetFeat(ch, feat.GetId()) ? "[Д]" : "[Н]",
						feat_info[feat.GetId()].name);
			}

			if (feat.IsInborn() ||
				PlayerRace::FeatureCheck((int) GET_KIN(ch), (int) GET_RACE(ch), to_underlying(feat.GetId()))) {
				strcat(buf2, buf);
				j++;
			} else if (feat.GetSlot() < max_slot) {
				strcat(names[feat.GetSlot()], buf);
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

		SendMsgToChar(buf1, vict);
//		page_string(ch->desc, buf, 1);
		if (j)
			SendMsgToChar(buf2, vict);

		for (int k = 0; k < max_slot; k++)
			delete[] names[k];

		delete[] names;

		return;
	}

// ======================================================

	sprintf(buf1, "Вы обладаете следующими способностями :\r\n");

	for (const auto &feat : MUD::Classes(ch->GetClass()).feats) {
		if (strlen(buf2) >= kMaxStringLength - 60) {
			strcat(buf2, "***ПЕРЕПОЛНЕНИЕ***\r\n");
			break;
		}
		if (ch->HaveFeat(feat.GetId())) {
			if (!feat_info[feat.GetId()].name || *feat_info[feat.GetId()].name == '!')
				continue;

			switch (feat.GetId()) {
				case EFeat::kBerserker:
				case EFeat::kLightWalk:
				case EFeat::kSpellCapabler:
				case EFeat::kRelocate:
				case EFeat::kShadowThrower:
					if (IsTimedByFeat(ch, feat.GetId())) {
						sprintf(buf, "[%3d] ", IsTimedByFeat(ch, feat.GetId()));
					} else {
						sprintf(buf, "[-!-] ");
					}
					break;
				case EFeat::kPowerAttack:
				case EFeat::kGreatPowerAttack:
				case EFeat::kAimingAttack:
				case EFeat::kGreatAimingAttack:
				case EFeat::kScirmisher:
				case EFeat::kDoubleThrower:
				case EFeat::kTripleThrower:
				case EFeat::kSerratedBlade:
					if (PRF_FLAGGED(ch, GetPrfWithFeatNumber(feat.GetId()))) {
						sprintf(buf, "[-%s*%s-] ", CCIGRN(vict, C_NRM), CCNRM(vict, C_NRM));
					} else {
						sprintf(buf, "[-:-] ");
					}
					break;
				default: sprintf(buf, "      ");
			}
			if (CanUseFeat(ch, feat.GetId())) {
				sprintf(buf + strlen(buf), "%s%s%s\r\n",
						CCIYEL(vict, C_NRM), feat_info[feat.GetId()].name, CCNRM(vict, C_NRM));
			} else if (clr(vict, C_NRM)) {
				sprintf(buf + strlen(buf), "%s\r\n", feat_info[feat.GetId()].name);
			} else {
				sprintf(buf, "[-Н-] %s\r\n", feat_info[feat.GetId()].name);
			}
			if (feat.IsInborn() ||
				PlayerRace::FeatureCheck((int) GET_KIN(ch), (int) GET_RACE(ch), to_underlying(feat.GetId()))) {
				sprintf(buf2 + strlen(buf2), "    ");
				strcat(buf2, buf);
				j++;
			} else {
				slot = feat.GetSlot();
				sfound = false;
				while (slot < max_slot) {
					if (*names[slot] == '\0') {
						sprintf(names[slot], " %s%-2d%s) ",
								CCGRN(vict, C_NRM), slot + 1, CCNRM(vict, C_NRM));
						strcat(names[slot], buf);
						sfound = true;
						break;
					} else {
						slot++;
					}
				}
				if (!sfound) {
					// Если способность не врожденная и под нее нет слота - удаляем нафик
					//	чтобы можно было менять слоты на лету и чтобы не читерили :)
					sprintf(msg, "WARNING: Unset out of slots feature '%s' for character '%s'!",
							feat_info[feat.GetId()].name, GET_NAME(ch));
					mudlog(msg, BRF, kLvlImplementator, SYSLOG, true);
					ch->UnsetFeat(feat.GetId());
				}
			}
		}
	}

	for (i = 0; i < max_slot; i++) {
		if (*names[i] == '\0')
			sprintf(names[i], " %s%-2d%s)       %s[пусто]%s\r\n",
					CCGRN(vict, C_NRM), i + 1, CCNRM(vict, C_NRM), CCIWHT(vict, C_NRM), CCNRM(vict, C_NRM));
		if (i >= CalcFeatLvl(ch))
			break;
		sprintf(buf1 + strlen(buf1), "%s", names[i]);
	}
	SendMsgToChar(buf1, vict);

	if (j)
		SendMsgToChar(buf2, vict);

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
		if (ch->GetSkill(skill.GetId())) {
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
							(kHoursPerDay - IsTimedBySkill(ch, skill_id)) / kHoursPerWarcry);
					break;
				case ESkill::kTurnUndead:
					if (CanUseFeat(ch, EFeat::kExorcist)) {
						sprintf(buf,
								"[-%d-] ",
								(kHoursPerDay - IsTimedBySkill(ch, skill_id)) / (kHoursPerTurnUndead - 2));
					} else {
						sprintf(buf, "[-%d-] ", (kHoursPerDay - IsTimedBySkill(ch, skill_id)) / kHoursPerTurnUndead);
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
					how_good(ch->GetSkill(skill_id), CalcSkillHardCap(ch, skill_id)),
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
	SendMsgToChar(buf2, vict);

}
const char *GetSpellColor(ESpell spell_id) {
	switch (spell_info[spell_id].spell_class) {
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
	using classes::CalcCircleSlotsAmount;

	char names[kMaxMemoryCircle][kMaxStringLength];
	std::string time_str;
	int slots[kMaxMemoryCircle], i, max_slot = 0, slot_num, gcount = 0, can_cast = 1;
	bool is_full = false;
	max_slot = 0;
	for (i = 0; i < kMaxMemoryCircle; i++) {
		*names[i] = '\0';
		slots[i] = 0;
	}
	for (const auto &spell : MUD::Classes(ch->GetClass()).spells) {
		auto spell_id = spell.GetId();
		if (!GET_SPELL_TYPE(ch, spell_id) && !all_spells)
			continue;
		if (!IS_MANA_CASTER(ch) && !IS_GOD(ch) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
			if (!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp) && !all_spells)
				continue;
		}
		if ((CalcMinSpellLvl(ch, spell_id) > GetRealLevel(ch) ||
			spell.GetMinRemort() > GET_REAL_REMORT(ch) ||
			CalcCircleSlotsAmount(ch, spell.GetCircle()) <= 0) &&
			all_spells && !GET_SPELL_TYPE(ch, spell_id)) {
			continue;
		}

		if (!spell_info[spell_id].name || *spell_info[spell_id].name == '!')
			continue;

		if ((GET_SPELL_TYPE(ch, spell_id) & 0xFF) == ESpellType::kRunes && !CheckRecipeItems(ch, spell_id, ESpellType::kRunes, false)) {
			if (all_spells) {
				can_cast = 0;
			} else {
				continue;
			}
		} else {
			can_cast = 1;
		}

		if (spell.GetMinRemort() > GET_REAL_REMORT(ch)) {
			slot_num = kMaxMemoryCircle - 1;
		} else {
			slot_num = spell.GetCircle() - 1;
		}
		max_slot = std::max(slot_num + 1, max_slot);
		if (IS_MANA_CASTER(ch)) {
			if (CalcSpellManacost(ch, spell_id) > GET_MAX_MANA(ch))
				continue;
			if (can_cast) {
				slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
										   "%s|<...%4d.> %s%-38s&n|",
										   slots[slot_num] % 114 <
											   10 ? "\r\n" : "  ",
										   CalcSpellManacost(ch, spell_id), GetSpellColor(spell_id), spell_info[spell_id].name);
			} else {
				slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
										   "%s|+--------+ %s%-38s&n|",
										   slots[slot_num] % 114 <
											   10 ? "\r\n" : "  ", GetSpellColor(spell_id), spell_info[spell_id].name);
			}
		} else {
			time_str.clear();
			if (IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp)) {
				time_str.append("[");
				time_str.append(std::to_string(MAX(1,
												   static_cast<int>(std::ceil(
													   static_cast<double>(Temporary_Spells::GetSpellLeftTime(ch,
																											  spell_id))
														   / kSecsPerMudHour)))));
				time_str.append("]");
			}
		if (CalcMinSpellLvl(ch, spell_id) > GetRealLevel(ch) && IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow)) {
			sprintf(buf1, "%d", CalcMinSpellLvl(ch, spell_id) - GetRealLevel(ch));
		}
		else {
			sprintf(buf1, "%s", "K");
		}
		slots[slot_num] += sprintf(names[slot_num] + slots[slot_num],
								   "%s|<%s%c%c%c%c%c%c%c>%s %-30s %-7s&n|",
				slots[slot_num] % 114 < 10 ? "\r\n" : "  ",
								   IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow) ? buf1 : ".",
								   IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kTemp) ? 'T' : '.',
								   IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kPotionCast) ? 'P' : '.',
								   IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kWandCast) ? 'W' : '.',
								   IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kScrollCast) ? 'S' : '.',
								   IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kItemCast) ? 'I' : '.',
								   IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kRunes) ? 'R' : '.',
								   '.',
								   GetSpellColor(spell_id),
								   spell_info[spell_id].name,
								   time_str.c_str());
		}
		is_full = true;
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
	SendMsgToChar(buf2, vict);
}

struct guild_learn_type {
	EFeat feat_no;
	ESkill skill_no;
	ESpell spell_no;
	int level;
};

struct guild_mono_type {
	guild_mono_type() : races(0), classes(0), religion(0), alignment(0), learn_info(0) {};
	Bitvector races;        // bitvector //
	Bitvector classes;        // bitvector //
	Bitvector religion;        // bitvector //
	Bitvector alignment;        // bitvector //
	struct guild_learn_type *learn_info;
};

struct guild_poly_type {
	Bitvector races;        // bitvector //
	Bitvector classes;        // bitvector //
	Bitvector religion;        // bitvector //
	Bitvector alignment;        // bitvector //
	EFeat feat_no;
	ESkill skill_no;
	ESpell spell_no;
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
	int i, num, type = 0, lines = 0, level, pgcount = 0, mgcount = 0;
	ESkill skill_id = ESkill::kUndefined;
	std::unique_ptr<struct guild_poly_type, decltype(free) *> poly_guild(nullptr, free);
	struct guild_mono_type mono_guild;
	std::unique_ptr<struct guild_learn_type, decltype(free) *> mono_guild_learn(nullptr, free);

	if (!(magic = fopen(LIB_MISC "guilds.lst", "r"))) {
		log("Cann't open guilds list file...");
		return;
	}
	auto spell_id{ESpell::kUndefined};
	auto feat_id{EFeat::kUndefined};
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
					(mono_guild.learn_info + mgcount)->skill_no = ESkill::kUndefined;
					(mono_guild.learn_info + mgcount)->feat_no = EFeat::kUndefined;
					(mono_guild.learn_info + mgcount)->spell_no = ESpell::kUndefined;
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
					(ptr + pgcount)->feat_no = EFeat::kUndefined;
					(ptr + pgcount)->skill_no = ESkill::kUndefined;
					(ptr + pgcount)->spell_no = ESpell::kUndefined;
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
			spell_id = static_cast<ESpell>(atoi(line));
			if (spell_id < ESpell::kFirst  || spell_id > ESpell::kLast) {
				spell_id = FixNameAndFindSpellId(line);
			}

			skill_id = static_cast<ESkill>(atoi(line1));
			if (MUD::Skills().IsInvalid(skill_id)) {
				skill_id = FixNameAndFindSkillId(line1);
			}

			feat_id = static_cast<EFeat>(atoi(line));
			if (feat_id < EFeat::kFirst || feat_id >= EFeat::kLast) {
				if ((pos = strchr(line1, '.')))
					*pos = ' ';
				feat_id = FindFeatId(line1);
			}

			if (MUD::Skills().IsInvalid(skill_id) && spell_id < ESpell::kFirst && feat_id < EFeat::kFirst) {
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
			ptr->spell_no = spell_id;
			ptr->skill_no = skill_id;
			ptr->feat_no = feat_id;
			ptr->level = level;
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

			spell_id = static_cast<ESpell>(atoi(line4));
			if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
				spell_id = FixNameAndFindSpellId(line4);
			}

			skill_id = static_cast<ESkill>(atoi(line5));
			if (MUD::Skills().IsInvalid(skill_id)) {
				skill_id = FixNameAndFindSkillId(line5);
			}

			feat_id = static_cast<EFeat>(atoi(line4));
			if (feat_id < EFeat::kFirst || feat_id > EFeat::kLast) {
				if ((pos = strchr(line5, '.')))
					*pos = ' ';

				feat_id = FindFeatId(line1);
				sprintf(buf, "feature number 2: %d", to_underlying(feat_id));
				feat_id = FindFeatId(line5);
			}
			if (MUD::Skills().IsInvalid(skill_id) && spell_id < ESpell::kFirst && feat_id < EFeat::kFirst) {
				log("Unknown skill, spell or feat for polyguild - \'%s\'", line5);
				graceful_exit(1);
			}
			if ((level = atoi(line6)) == 0 || level >= kLvlImmortal) {
				log("Use 1-%d level for guilds", kLvlImmortal);
				graceful_exit(1);
			}
			ptr->spell_no = spell_id;
			ptr->skill_no = skill_id;
			ptr->feat_no = feat_id;
			ptr->level = level;
			pgcount++;
		}
	}
	fclose(magic);
}

#define SCMD_LEARN 1

int guild_mono(CharData *ch, void *me, int cmd, char *argument) {
	int command = 0, gcount = 0, info_num = 0, found = false, sfound = false, i, bits;
	CharData *victim = (CharData *) me;

	if (ch->IsNpc()) {
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
	if (!IS_BITS(guild_mono_info[info_num].classes, to_underlying(ch->GetClass()))
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
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					const auto skill_no = (guild_mono_info[info_num].learn_info + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kUndefined != skill_no && (!ch->get_trained_skill(skill_no)
						|| IS_GRGOD(ch)) && CanGetSkill(ch, skill_no)) {
						gcount += sprintf(buf + gcount, "- умение %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), MUD::Skills(skill_no).GetName(), CCNRM(ch, C_NRM));
						found = true;
					}

					if (!(bits = -2 * bits) || bits == ESpellType::kTemp) {
						bits = ESpellType::kKnow;
					}

					const auto spell_no = (guild_mono_info[info_num].learn_info + i)->spell_no;
					if (spell_no > ESpell::kUndefined && (!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits) || IS_GRGOD(
						ch))) {
						gcount += sprintf(buf + gcount, "- магия %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						found = true;
					}

					const auto feat_no = (guild_mono_info[info_num].learn_info + i)->feat_no;
					if (feat_no > EFeat::kUndefined && !ch->HaveFeat(feat_no) && CanGetFeat(ch, feat_no)) {
						gcount += sprintf(buf + gcount, "- способность %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
						found = true;
					}
				}

				if (!found) {
					act("$N сказал$G : 'Похоже, твои знания полнее моих.'", false, ch, 0, victim, kToChar);
					return (1);
				} else {
					SendMsgToChar(buf, ch);
					return (1);
				}
			}

			if (!strn_cmp(argument, "все", strlen(argument)) || !strn_cmp(argument, "all", strlen(argument))) {
				for (i = 0, found = false, sfound = true;
					 (guild_mono_info[info_num].learn_info + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch))
						continue;

					const ESkill skill_no = (guild_mono_info[info_num].learn_info + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kUndefined != skill_no
						&& !ch->get_trained_skill(skill_no))    // sprintf(buf, "$N научил$G вас умению %s\"%s\"\%s",
					{
						//             CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
						// act(buf,false,ch,0,victim,TO_CHAR);
						// ch->get_skill(skill_no) = 10;
						sfound = true;
					}

					if (!(bits = -2 * bits) || bits == ESpellType::kTemp) {
						bits = ESpellType::kKnow;
					}

					const auto spell_no = (guild_mono_info[info_num].learn_info + i)->spell_no;
					if (spell_no > ESpell::kUndefined &&
						!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)) {
						gcount += sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						act(buf, false, ch, 0, victim, kToChar);
						if (IS_SET(bits, ESpellType::kKnow))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kKnow);
						if (IS_SET(bits, ESpellType::kItemCast))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kItemCast);
						if (IS_SET(bits, ESpellType::kRunes))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kRunes);
						if (IS_SET(bits, ESpellType::kPotionCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kPotionCast);
							ch->set_skill(ESkill::kCreatePotion, std::max(10, ch->get_trained_skill(ESkill::kCreatePotion)));
						}
						if (IS_SET(bits, ESpellType::kWandCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kWandCast);
							ch->set_skill(ESkill::kCreateWand, std::max(10, ch->get_trained_skill(ESkill::kCreateWand)));
						}
						if (IS_SET(bits, ESpellType::kScrollCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kScrollCast);
							ch->set_skill(ESkill::kCreateScroll, std::max(10, ch->get_trained_skill(ESkill::kCreateScroll)));
						}
						found = true;
					}

					const auto feat_no = (guild_mono_info[info_num].learn_info + i)->feat_no;
					if (feat_no >= EFeat::kFirst && feat_no <= EFeat::kLast) {
						if (!ch->HaveFeat(feat_no) && CanGetFeat(ch, feat_no)) {
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

			const auto feat_no = FindFeatId(argument);
			if (feat_no >= EFeat::kFirst && feat_no <= EFeat::kLast) {
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->feat_no >= EFeat::kFirst; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (feat_no == (guild_mono_info[info_num].learn_info + i)->feat_no) {
						if (ch->HaveFeat(feat_no)) {
							act("$N сказал$g вам : 'Ничем помочь не могу, ты уже владеешь этой способностью.'",
								false,
								ch,
								0,
								victim,
								kToChar);
						} else if (!CanGetFeat(ch, feat_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'", false, ch, 0, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас способности %s\"%s\"%s",
									CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
							act(buf, false, ch, 0, victim, kToChar);
							ch->SetFeat(feat_no);
						}
						found = true;
					}
				}

				if (!found) {
					act("$N сказал$G : 'Мне не ведомо такое мастерство.'", false, ch, 0, victim, kToChar);
				}

				return (1);
			}

			const ESkill skill_no = FixNameAndFindSkillId(argument);
			if (skill_no >= ESkill::kLast && skill_no <= ESkill::kLast) {
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->spell_no >= ESpell::kUndefined; i++) {
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
						} else if (!CanGetSkill(ch, skill_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'", false, ch, 0, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас умению %s\"%s\"%s",
									CCCYN(ch, C_NRM), MUD::Skills(skill_no).GetName(), CCNRM(ch, C_NRM));
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

			auto spell_no = FixNameAndFindSpellId(argument);
			if (spell_no > ESpell::kUndefined && spell_no <= ESpell::kLast) {
				for (i = 0, found = false; (guild_mono_info[info_num].learn_info + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_mono_info[info_num].learn_info + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (spell_no == (guild_mono_info[info_num].learn_info + i)->spell_no) {
						if (!(bits = -2 * to_underlying((guild_mono_info[info_num].learn_info + i)->skill_no))
							|| bits == ESpellType::kTemp) {
							bits = ESpellType::kKnow;
						}

						if ((GET_SPELL_TYPE(ch, spell_no) & bits) == bits) {
							if (IS_SET(bits, ESpellType::kKnow)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить это заклинание, тебе необходимо набрать\r\n"
											 "%s колд(овать) '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, ESpellType::kItemCast)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
											 "%s смешать '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, ESpellType::kRunes)) {
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
							if (IS_SET(bits, ESpellType::kKnow)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kKnow);
							}
							if (IS_SET(bits, ESpellType::kItemCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kItemCast);
							}
							if (IS_SET(bits, ESpellType::kRunes)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kRunes);
							}
							if (IS_SET(bits, ESpellType::kPotionCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kPotionCast);
								ch->set_skill(ESkill::kCreatePotion, std::max(10, ch->get_trained_skill(ESkill::kCreatePotion)));
							}
							if (IS_SET(bits, ESpellType::kWandCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kWandCast);
								ch->set_skill(ESkill::kCreateWand, std::max(10, ch->get_trained_skill(ESkill::kCreateWand)));
							}
							if (IS_SET(bits, ESpellType::kScrollCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kScrollCast);
								ch->set_skill(ESkill::kCreateScroll, std::max(10, ch->get_trained_skill(ESkill::kCreateScroll)));
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

	if (ch->IsNpc()) {
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
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}

					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, to_underlying(ch->GetClass()))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch)))
						continue;

					const ESkill skill_no = (guild_poly_info[info_num] + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kUndefined != skill_no &&
						(!ch->get_trained_skill(skill_no) || IS_GRGOD(ch)) && CanGetSkill(ch, skill_no)) {
						gcount += sprintf(buf + gcount, "- умение %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), MUD::Skills(skill_no).GetName(), CCNRM(ch, C_NRM));
						found = true;
					}

					if (!(bits = -2 * bits) || bits == ESpellType::kTemp) {
						bits = ESpellType::kKnow;
					}

					const auto spell_no = (guild_poly_info[info_num] + i)->spell_no;
					if (spell_no > ESpell::kUndefined &&
						(!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)
							|| IS_GRGOD(ch))) {
						gcount += sprintf(buf + gcount, "- магия %s\"%s\"%s\r\n",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						found = true;
					}

					const auto feat_no = (guild_poly_info[info_num] + i)->feat_no;
					if (feat_no >= EFeat::kFirst && feat_no <= EFeat::kLast) {
						if (!ch->HaveFeat(feat_no) && CanGetFeat(ch, feat_no)) {
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
					SendMsgToChar(buf, ch);
					return (1);
				}
			}

			if (!strn_cmp(argument, "все", strlen(argument))
				|| !strn_cmp(argument, "all", strlen(argument))) {
				for (i = 0, found = false, sfound = false; (guild_poly_info[info_num] + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, to_underlying(ch->GetClass()))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					const ESkill skill_no = (guild_poly_info[info_num] + i)->skill_no;
					bits = to_underlying(skill_no);
					if (ESkill::kUndefined != skill_no
						&& !ch->get_trained_skill(skill_no))    // sprintf(buf, "$N научил$G вас умению %s\"%s\"\%s",
					{
						//             CCCYN(ch, C_NRM), skill_name(skill_no), CCNRM(ch, C_NRM));
						// act(buf,false,ch,0,victim,TO_CHAR);
						// ch->get_skill(skill_no) = 10;
						sfound = true;
					}

					const auto feat_no = (guild_poly_info[info_num] + i)->feat_no;
					if (feat_no >= EFeat::kFirst && feat_no <= EFeat::kLast) {
						if (!ch->HaveFeat(feat_no) && CanGetFeat(ch, feat_no)) {
							sfound = true;
						}
					}

					if (!(bits = -2 * bits) || bits == ESpellType::kTemp) {
						bits = ESpellType::kKnow;
					}

					const auto spell_no = (guild_poly_info[info_num] + i)->spell_no;
					if (spell_no > ESpell::kUndefined &&
						!((GET_SPELL_TYPE(ch, spell_no) & bits) == bits)) {
						gcount += sprintf(buf, "$N научил$G вас магии %s\"%s\"%s",
										  CCCYN(ch, C_NRM), GetSpellName(spell_no), CCNRM(ch, C_NRM));
						act(buf, false, ch, 0, victim, kToChar);

						if (IS_SET(bits, ESpellType::kKnow))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kKnow);
						if (IS_SET(bits, ESpellType::kItemCast))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kItemCast);
						if (IS_SET(bits, ESpellType::kRunes))
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kRunes);
						if (IS_SET(bits, ESpellType::kPotionCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kPotionCast);
							ch->set_skill(ESkill::kCreatePotion, std::max(10, ch->get_trained_skill(ESkill::kCreatePotion)));
						}
						if (IS_SET(bits, ESpellType::kWandCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kWandCast);
							ch->set_skill(ESkill::kCreateWand, std::max(10, ch->get_trained_skill(ESkill::kCreateWand)));
						}
						if (IS_SET(bits, ESpellType::kScrollCast)) {
							SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kScrollCast);
							ch->set_skill(ESkill::kCreateScroll, std::max(10, ch->get_trained_skill(ESkill::kCreateScroll)));
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

			const auto skill_no = FixNameAndFindSkillId(argument);
			if (ESkill::kUndefined != skill_no && skill_no <= ESkill::kLast) {
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, to_underlying(ch->GetClass()))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					if (skill_no == (guild_poly_info[info_num] + i)->skill_no) {
						if (ch->get_trained_skill(skill_no)) {
							act("$N сказал$G вам : 'Ты уже владеешь этим умением.'",
								false, ch, 0, victim, kToChar);
						} else if (!CanGetSkill(ch, skill_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'",
								false, ch, 0, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас умению %s\"%s\"%s",
									CCCYN(ch, C_NRM), MUD::Skills(skill_no).GetName(), CCNRM(ch, C_NRM));
							act(buf, false, ch, 0, victim, kToChar);
							ch->set_skill(skill_no, 10);
						}
						found = true;
					}
				}

				if (found) {
					return 1;
				}
			}

			auto feat_no = FindFeatId(argument);
			if (feat_no == EFeat::kUndefined) {
				std::string str(argument);
				std::replace_if(str.begin(), str.end(), boost::is_any_of("_:"), ' ');
				feat_no = FindFeatId(str.c_str(), true);
			}

			if (feat_no != EFeat::kUndefined) {
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, to_underlying(ch->GetClass()))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					if (feat_no == (guild_poly_info[info_num] + i)->feat_no) {
						if (ch->HaveFeat(feat_no)) {
							act("$N сказал$G вам : 'Ничем помочь не могу, ты уже владеешь этой способностью.'",
								false, ch, nullptr, victim, kToChar);
						} else if (!CanGetFeat(ch, feat_no)) {
							act("$N сказал$G : 'Я не могу тебя этому научить.'", false, ch, 0, victim, kToChar);
						} else {
							sprintf(buf, "$N научил$G вас способности %s\"%s\"%s",
									CCCYN(ch, C_NRM), GetFeatName(feat_no), CCNRM(ch, C_NRM));
							act(buf, false, ch, nullptr, victim, kToChar);
							ch->SetFeat(feat_no);
						}
						found = true;
					}
				}

				if (found) {
					return 1;
				}
			}

			const auto spell_no = FixNameAndFindSpellId(argument);
			if (spell_no != ESpell::kUndefined) {
				for (i = 0, found = false; (guild_poly_info[info_num] + i)->spell_no >= ESpell::kUndefined; i++) {
					if ((guild_poly_info[info_num] + i)->level > GetRealLevel(ch)) {
						continue;
					}
					if (!(bits = -2 * to_underlying((guild_poly_info[info_num] + i)->skill_no))
						|| bits == ESpellType::kTemp) {
						bits = ESpellType::kKnow;
					}
					if (!IS_BITS((guild_poly_info[info_num] + i)->classes, to_underlying(ch->GetClass()))
						|| !IS_BITS((guild_poly_info[info_num] + i)->races, GET_RACE(ch))
						|| !IS_BITS((guild_poly_info[info_num] + i)->religion, GET_RELIGION(ch))) {
						continue;
					}

					if (spell_no == (guild_poly_info[info_num] + i)->spell_no) {
						if ((GET_SPELL_TYPE(ch, spell_no) & bits) == bits) {
							if (IS_SET(bits, ESpellType::kKnow)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить это заклинание, тебе необходимо набрать\r\n"
											 "%s колд(овать) '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, ESpellType::kItemCast)) {
								sprintf(buf, "$N сказал$G : \r\n"
											 "'$n, чтобы применить эту магию, тебе необходимо набрать\r\n"
											 "%s смешать '%s' <цель>%s и щелкнуть клавишей <ENTER>'.",
										CCCYN(ch, C_NRM), GetSpellName(spell_no),
										CCNRM(ch, C_NRM));
							} else if (IS_SET(bits, ESpellType::kRunes)) {
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
							if (IS_SET(bits, ESpellType::kKnow)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kKnow);
							}
							if (IS_SET(bits, ESpellType::kItemCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kItemCast);
							}
							if (IS_SET(bits, ESpellType::kRunes)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kRunes);
							}
							if (IS_SET(bits, ESpellType::kPotionCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kPotionCast);
								ch->set_skill(ESkill::kCreatePotion, std::max(10, ch->get_trained_skill(ESkill::kCreatePotion)));
							}
							if (IS_SET(bits, ESpellType::kWandCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kWandCast);
								ch->set_skill(ESkill::kCreateWand, std::max(10, ch->get_trained_skill(ESkill::kCreateWand)));
							}
							if (IS_SET(bits, ESpellType::kScrollCast)) {
								SET_BIT(GET_SPELL_TYPE(ch, spell_no), ESpellType::kScrollCast);
								ch->set_skill(ESkill::kCreateScroll, std::max(10, ch->get_trained_skill(ESkill::kCreateScroll)));
							}
						}
						found = true;
					}
				}

				if (found) {
					return 1;
				}
			}

			act("$N сказал$G вам : 'Я никогда и никого этому не учил$G.'", false, ch, 0, victim, kToChar);
			return (1);
	}

	return (0);
}

int horse_keeper(CharData *ch, void *me, int cmd, char *argument) {
	CharData *victim = (CharData *) me, *horse = nullptr;

	if (ch->IsNpc())
		return (0);

	if (!CMD_IS("лошадь") && !CMD_IS("horse"))
		return (0);

	if (ch->is_morphed()) {
		SendMsgToChar("Лошадка испугается вас в таком виде... \r\n", ch);
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
				kHorseCost, GetDeclensionInNumber(kHorseCost, EWhat::kMoneyA));
		act(buf, false, ch, 0, victim, kToChar);
		return (true);
	}

	if (!strn_cmp(argument, "купить", strlen(argument)) || !strn_cmp(argument, "buy", strlen(argument))) {
		if (ch->has_horse(false)) {
			act("$N засмеял$U : \"$n, ты шутишь, у тебя же есть скакун.\"", false, ch, 0, victim, kToChar);
			return (true);
		}
		if (ch->get_gold() < kHorseCost) {
			act("\"Ступай отсюда, злыдень, у тебя нет таких денег!\"-заорал$G $N",
				false, ch, 0, victim, kToChar);
			return (true);
		}
		if (!(horse = read_mobile(kHorseVnum, VIRTUAL))) {
			act("\"Извини, у меня нет для тебя скакуна.\"-смущенно произнес$Q $N",
				false, ch, 0, victim, kToChar);
			return (true);
		}
		make_horse(horse, ch);
		PlaceCharToRoom(horse, ch->in_room);
		sprintf(buf, "$N оседлал$G %s и отдал$G %s вам.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, false, ch, 0, victim, kToChar);
		sprintf(buf, "$N оседлал$G %s и отдал$G %s $n2.", GET_PAD(horse, 3), HSHR(horse));
		act(buf, false, ch, 0, victim, kToRoom);
		ch->remove_gold(kHorseCost);
		PLR_FLAGS(ch).set(EPlrFlag::kCrashSave);
		return (true);
	}

	if (!strn_cmp(argument, "продать", strlen(argument)) || !strn_cmp(argument, "sell", strlen(argument))) {
		if (!ch->has_horse(true)) {
			act("$N засмеял$U : \"$n, ты не влезешь в мое стойло.\"", false, ch, 0, victim, kToChar);
			return (true);
		}
		if (ch->IsOnHorse()) {
			act("\"Я не собираюсь платить еще и за всадника.\"-усмехнул$U $N",
				false, ch, 0, victim, kToChar);
			return (true);
		}

		if (!(horse = ch->get_horse()) || GET_MOB_VNUM(horse) != kHorseVnum) {
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
		ExtractCharFromWorld(horse, false);
		ch->add_gold((kHorseCost >> 1));
		PLR_FLAGS(ch).set(EPlrFlag::kCrashSave);
		return (true);
	}

	return (0);
}

bool item_nouse(ObjData *obj) {
	switch (GET_OBJ_TYPE(obj)) {
		case EObjType::kLightSource:
			if (GET_OBJ_VAL(obj, 2) == 0) {
				return true;
			}
			break;

		case EObjType::kScroll:
		case EObjType::kPotion:
			if (!GET_OBJ_VAL(obj, 1)
				&& !GET_OBJ_VAL(obj, 2)
				&& !GET_OBJ_VAL(obj, 3)) {
				return true;
			}
			break;

		case EObjType::kStaff:
		case EObjType::kWand:
			if (!GET_OBJ_VAL(obj, 2)) {
				return true;
			}
			break;

		case EObjType::kContainer:
			if (!system_obj::is_purse(obj)) {
				return true;
			}
			break;

		case EObjType::kOther:
		case EObjType::kTrash:
		case EObjType::kTrap:
		case EObjType::kNote:
		case EObjType::kLiquidContainer:
		case EObjType::kFood:
		case EObjType::kPen:
		case EObjType::kBoat:
		case EObjType::kFountain:
		case EObjType::kMagicIngredient: return true;

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
			ExtractObjFromChar(obj);
			PlaceObjToRoom(obj, ch->in_room);
		}
	}
}

int npc_scavenge(CharData *ch) {
	int max = 1;
	ObjData *obj, *best_obj, *cont, *best_cont, *cobj;

	if (!MOB_FLAGGED(ch, EMobFlag::kScavenger)) {
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
			if (GET_OBJ_TYPE(obj) == EObjType::kMagicIngredient
				|| Clan::is_clan_chest(obj)
				|| ClanSystem::is_ingr_chest(obj)) {
				continue;
			}

			if (GET_OBJ_TYPE(obj) == EObjType::kContainer
				&& !system_obj::is_purse(obj)) {
				if (IS_CORPSE(obj)) {
					continue;
				}

				// Заперто, открываем, если есть ключ
				if (OBJVAL_FLAGGED(obj, EContainerFlag::kLockedUp)
					&& HasKey(ch, GET_OBJ_VAL(obj, 2))) {
					do_doorcmd(ch, obj, 0, SCMD_UNLOCK);
				}

				// Заперто, взламываем, если умеем
				if (OBJVAL_FLAGGED(obj, EContainerFlag::kLockedUp)
					&& ch->GetSkill(ESkill::kPickLock)
					&& ok_pick(ch, 0, obj, 0, SCMD_PICK)) {
					do_doorcmd(ch, obj, 0, SCMD_PICK);
				}
				// Все равно заперто, ну тогда фиг с ним
				if (OBJVAL_FLAGGED(obj, EContainerFlag::kLockedUp)) {
					continue;
				}

				if (OBJVAL_FLAGGED(obj, EContainerFlag::kShutted)) {
					do_doorcmd(ch, obj, 0, SCMD_OPEN);
				}

				if (OBJVAL_FLAGGED(obj, EContainerFlag::kShutted)) {
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
				if (GET_OBJ_TYPE(best_obj) == EObjType::kMoney) {
					ch->add_gold(GET_OBJ_VAL(best_obj, 0));
					ExtractObjFromWorld(best_obj);
				} else {
					ExtractObjFromRoom(best_obj);
					PlaceObjToInventory(best_obj, ch);
				}
			} else {
				sprintf(buf, "$n достал$g $o3 из %s.", cont->get_PName(1).c_str());
				act(buf, false, ch, best_obj, 0, kToRoom);
				if (GET_OBJ_TYPE(best_obj) == EObjType::kMoney) {
					ch->add_gold(GET_OBJ_VAL(best_obj, 0));
					ExtractObjFromWorld(best_obj);
				} else {
					ExtractObjFromObj(best_obj);
					PlaceObjToInventory(best_obj, ch);
				}
			}
		}
	}
	return (max > 1);
}

int npc_loot(CharData *ch) {
	int max = false;
	ObjData *obj, *loot_obj, *next_loot, *cobj, *cnext_obj;

	if (!MOB_FLAGGED(ch, EMobFlag::kLooter))
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
					if ((GET_OBJ_TYPE(loot_obj) != EObjType::kContainer
						|| system_obj::is_purse(loot_obj))
						&& CAN_GET_OBJ(ch, loot_obj)
						&& !item_nouse(loot_obj)) {
						sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(1).c_str());
						act(buf, false, ch, loot_obj, 0, kToRoom);
						if (GET_OBJ_TYPE(loot_obj) == EObjType::kMoney) {
							ch->add_gold(GET_OBJ_VAL(loot_obj, 0));
							ExtractObjFromWorld(loot_obj);
						} else {
							ExtractObjFromObj(loot_obj);
							PlaceObjToInventory(loot_obj, ch);
							max++;
						}
					}
				}
				// Теперь не запертые контейнеры
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot) {
					next_loot = loot_obj->get_next_content();
					if (GET_OBJ_TYPE(loot_obj) == EObjType::kContainer) {
						if (IS_CORPSE(loot_obj)
							|| OBJVAL_FLAGGED(loot_obj, EContainerFlag::kLockedUp)
							|| system_obj::is_purse(loot_obj)) {
							continue;
						}
						auto value = loot_obj->get_val(1); //откроем контейнер
						REMOVE_BIT(value, EContainerFlag::kShutted);
						loot_obj->set_val(1, value);
						for (cobj = loot_obj->get_contains(); cobj; cobj = cnext_obj) {
							cnext_obj = cobj->get_next_content();
							if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj)) {
								sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(1).c_str());
								act(buf, false, ch, cobj, 0, kToRoom);
								if (GET_OBJ_TYPE(cobj) == EObjType::kMoney) {
									ch->add_gold(GET_OBJ_VAL(cobj, 0));
									ExtractObjFromWorld(cobj);
								} else {
									ExtractObjFromObj(cobj);
									PlaceObjToInventory(cobj, ch);
									max++;
								}
							}
						}
					}
				}
				// И наконец, лутим запертые контейнеры если есть ключ или можем взломать
				for (loot_obj = obj->get_contains(); loot_obj; loot_obj = next_loot) {
					next_loot = loot_obj->get_next_content();
					if (GET_OBJ_TYPE(loot_obj) == EObjType::kContainer) {
						if (IS_CORPSE(loot_obj)
							|| !OBJVAL_FLAGGED(loot_obj, EContainerFlag::kLockedUp)
							|| system_obj::is_purse(loot_obj)) {
							continue;
						}

						// Есть ключ?
						if (OBJVAL_FLAGGED(loot_obj, EContainerFlag::kLockedUp)
							&& HasKey(ch, GET_OBJ_VAL(loot_obj, 2))) {
							loot_obj->toggle_val_bit(1, EContainerFlag::kLockedUp);
						}

						// ...или взломаем?
						if (OBJVAL_FLAGGED(loot_obj, EContainerFlag::kLockedUp)
							&& ch->GetSkill(ESkill::kPickLock)
							&& ok_pick(ch, 0, loot_obj, 0, SCMD_PICK)) {
							loot_obj->toggle_val_bit(1, EContainerFlag::kLockedUp);
						}

						// Эх, не открыть. Ну ладно.
						if (OBJVAL_FLAGGED(loot_obj, EContainerFlag::kLockedUp)) {
							continue;
						}

						loot_obj->toggle_val_bit(1, EContainerFlag::kShutted);
						for (cobj = loot_obj->get_contains(); cobj; cobj = cnext_obj) {
							cnext_obj = cobj->get_next_content();
							if (CAN_GET_OBJ(ch, cobj) && !item_nouse(cobj)) {
								sprintf(buf, "$n вытащил$g $o3 из %s.", obj->get_PName(1).c_str());
								act(buf, false, ch, cobj, 0, kToRoom);
								if (GET_OBJ_TYPE(cobj) == EObjType::kMoney) {
									ch->add_gold(GET_OBJ_VAL(cobj, 0));
									ExtractObjFromWorld(cobj);
								} else {
									ExtractObjFromObj(cobj);
									PlaceObjToInventory(cobj, ch);
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
	int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth, EDirection::kEast, EDirection::kDown, EDirection::kUp};
	int retval = false;

	if (ch == nullptr || dir < 0 || dir >= EDirection::kMaxDirNum || ch->GetEnemy()) {
		return (false);
	} else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room() == kNowhere) {
		return (false);
	} else if (ch->has_master()
		&& ch->in_room == IN_ROOM(ch->get_master())) {
		return (false);
	} else if (EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kClosed)) {
		if (!EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kHasDoor)) {
			return (false);
		}

		const auto &rdata = EXIT(ch, dir);

		if (EXIT_FLAGGED(rdata, EExitFlag::kLocked)) {
			if (HasKey(ch, rdata->key)
				|| (!EXIT_FLAGGED(rdata, EExitFlag::kPickroof)
					&& !EXIT_FLAGGED(rdata, EExitFlag::kBrokenLock)
					&& CalcCurrentSkill(ch, ESkill::kPicks, 0) >= number(0, 100))) {
				do_doorcmd(ch, 0, dir, SCMD_UNLOCK);
				need_lock = true;
			} else {
				return (false);
			}
		}
		if (EXIT_FLAGGED(rdata, EExitFlag::kClosed)) {
			if (GET_REAL_INT(ch) >= 15
				|| GET_DEST(ch) != kNowhere
				|| MOB_FLAGGED(ch, EMobFlag::kOpensDoor)) {
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
			EXIT_FLAGGED(EXIT(ch, close_direction), EExitFlag::kHasDoor) &&
			EXIT(ch, close_direction)->to_room() != kNowhere) {
			do_doorcmd(ch, 0, close_direction, SCMD_CLOSE);
		}
	}

	if (need_lock) {
		const int lock_direction = retval ? rev_dir[dir] : dir;
		// запираем за собой только существующую дверь
		if (EXIT(ch, lock_direction) &&
			EXIT_FLAGGED(EXIT(ch, lock_direction), EExitFlag::kHasDoor) &&
			EXIT(ch, lock_direction)->to_room() != kNowhere) {
			do_doorcmd(ch, 0, lock_direction, SCMD_LOCK);
		}
	}

	return (retval);
}

int has_curse(ObjData *obj) {
	for (const auto &i : weapon_affect) {
		// Замена выражения на макрос
		if (i.aff_spell <= ESpell::kUndefined || !IS_OBJ_AFF(obj, i.aff_pos)) {
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
		|| GET_OBJ_TYPE(weapon) != EObjType::kWeapon) {
		return 0;
	}

	hits = CalcCurrentSkill(ch, static_cast<ESkill>(GET_OBJ_SKILL(weapon)), 0);
	damage = (GET_OBJ_VAL(weapon, 1) + 1) * (GET_OBJ_VAL(weapon, 2)) / 2;
	for (i = 0; i < kMaxObjAffect; i++) {
		auto &affected = weapon->get_affected(i);
		if (affected.location == EApply::kDamroll) {
			damage += affected.modifier;
		}

		if (affected.location == EApply::kHitroll) {
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

	if (!NPC_FLAGGED(ch, ENpcFlag::kWielding))
		return;

	if (ch->GetSkill(ESkill::kHammer) > 0
		&& ch->GetSkill(ESkill::kOverwhelm) < ch->GetSkill(ESkill::kHammer)) {
		return;
	}

	if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	if (GET_EQ(ch, EEquipPos::kHold)
		&& GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kHold)) == EObjType::kWeapon) {
		left = GET_EQ(ch, EEquipPos::kHold);
	}
	if (GET_EQ(ch, EEquipPos::kWield)
		&& GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kWield)) == EObjType::kWeapon) {
		right = GET_EQ(ch, EEquipPos::kWield);
	}
	if (GET_EQ(ch, EEquipPos::kBoths)
		&& GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kBoths)) == EObjType::kWeapon) {
		both = GET_EQ(ch, EEquipPos::kBoths);
	}

	if (GET_REAL_INT(ch) < 15 && ((left && right) || (both)))
		return;

	for (obj = ch->carrying; obj; obj = next) {
		next = obj->get_next_content();
		if (GET_OBJ_TYPE(obj) != EObjType::kWeapon
			|| GET_OBJ_UID(obj) != 0) {
			continue;
		}

		if (CAN_WEAR(obj, EWearFlag::kHold)
			&& OK_HELD(ch, obj)) {
			best_weapon(ch, obj, &left);
		} else if (CAN_WEAR(obj, EWearFlag::kWield)
			&& OK_WIELD(ch, obj)) {
			best_weapon(ch, obj, &right);
		} else if (CAN_WEAR(obj, EWearFlag::kBoth)
			&& OK_BOTH(ch, obj)) {
			best_weapon(ch, obj, &both);
		}
	}

	if (both
		&& calculate_weapon_class(ch, both) > calculate_weapon_class(ch, left) + calculate_weapon_class(ch, right)) {
		if (both == GET_EQ(ch, EEquipPos::kBoths)) {
			return;
		}
		if (GET_EQ(ch, EEquipPos::kBoths)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kBoths), 0, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kBoths, CharEquipFlag::show_msg), ch);
		}
		if (GET_EQ(ch, EEquipPos::kWield)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kWield), 0, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kWield, CharEquipFlag::show_msg), ch);
		}
		if (GET_EQ(ch, EEquipPos::kShield)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kShield), 0, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kShield, CharEquipFlag::show_msg), ch);
		}
		if (GET_EQ(ch, EEquipPos::kHold)) {
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kHold), 0, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kHold, CharEquipFlag::show_msg), ch);
		}
		//obj_from_char(both);
		EquipObj(ch, both, EEquipPos::kBoths, CharEquipFlag::show_msg);
	} else {
		if (left && GET_EQ(ch, EEquipPos::kHold) != left) {
			if (GET_EQ(ch, EEquipPos::kBoths)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kBoths), 0, kToRoom);
				PlaceObjToInventory(UnequipChar(ch, EEquipPos::kBoths, CharEquipFlag::show_msg), ch);
			}
			if (GET_EQ(ch, EEquipPos::kShield)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kShield), 0, kToRoom);
				PlaceObjToInventory(UnequipChar(ch, EEquipPos::kShield, CharEquipFlag::show_msg), ch);
			}
			if (GET_EQ(ch, EEquipPos::kHold)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kHold), 0, kToRoom);
				PlaceObjToInventory(UnequipChar(ch, EEquipPos::kHold, CharEquipFlag::show_msg), ch);
			}
			//obj_from_char(left);
			EquipObj(ch, left, EEquipPos::kHold, CharEquipFlag::show_msg);
		}
		if (right && GET_EQ(ch, EEquipPos::kWield) != right) {
			if (GET_EQ(ch, EEquipPos::kBoths)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kBoths), 0, kToRoom);
				PlaceObjToInventory(UnequipChar(ch, EEquipPos::kBoths, CharEquipFlag::show_msg), ch);
			}
			if (GET_EQ(ch, EEquipPos::kWield)) {
				act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, EEquipPos::kWield), 0, kToRoom);
				PlaceObjToInventory(UnequipChar(ch, EEquipPos::kWield, CharEquipFlag::show_msg), ch);
			}
			//obj_from_char(right);
			EquipObj(ch, right, EEquipPos::kWield, CharEquipFlag::show_msg);
		}
	}
}

void npc_armor(CharData *ch) {
	ObjData *obj, *next;
	int where = 0;

	if (!NPC_FLAGGED(ch, ENpcFlag::kArmoring))
		return;

	if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	for (obj = ch->carrying; obj; obj = next) {
		next = obj->get_next_content();

		if (!ObjSystem::is_armor_type(obj)
			|| !no_bad_affects(obj)) {
			continue;
		}

		if (CAN_WEAR(obj, EWearFlag::kFinger)) {
			where = EEquipPos::kFingerR;
		}

		if (CAN_WEAR(obj, EWearFlag::kNeck)) {
			where = EEquipPos::kNeck;
		}

		if (CAN_WEAR(obj, EWearFlag::kBody)) {
			where = EEquipPos::kBody;
		}

		if (CAN_WEAR(obj, EWearFlag::kHead)) {
			where = EEquipPos::kHead;
		}

		if (CAN_WEAR(obj, EWearFlag::kLegs)) {
			where = EEquipPos::kLegs;
		}

		if (CAN_WEAR(obj, EWearFlag::kFeet)) {
			where = EEquipPos::kFeet;
		}

		if (CAN_WEAR(obj, EWearFlag::kHands)) {
			where = EEquipPos::kHands;
		}

		if (CAN_WEAR(obj, EWearFlag::kArms)) {
			where = EEquipPos::kArms;
		}

		if (CAN_WEAR(obj, EWearFlag::kShield)) {
			where = EEquipPos::kShield;
		}

		if (CAN_WEAR(obj, EWearFlag::kShoulders)) {
			where = EEquipPos::kShoulders;
		}

		if (CAN_WEAR(obj, EWearFlag::kWaist)) {
			where = EEquipPos::kWaist;
		}

		if (CAN_WEAR(obj, EWearFlag::kQuiver)) {
			where = EEquipPos::kQuiver;
		}

		if (CAN_WEAR(obj, EWearFlag::kWrist)) {
			where = EEquipPos::kWristR;
		}

		if (!where) {
			continue;
		}

		if ((where == EEquipPos::kFingerR) || (where == EEquipPos::kNeck) || (where == EEquipPos::kWristR)) {
			if (GET_EQ(ch, where)) {
				where++;
			}
		}

		if (where == EEquipPos::kShield && (GET_EQ(ch, EEquipPos::kBoths) || GET_EQ(ch, EEquipPos::kHold))) {
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
			PlaceObjToInventory(UnequipChar(ch, where, CharEquipFlag::show_msg), ch);
		}
		//obj_from_char(obj);
		EquipObj(ch, obj, where, CharEquipFlag::show_msg);
		break;
	}
}

void npc_light(CharData *ch) {
	ObjData *obj, *next;

	if (GET_REAL_INT(ch) < 10 || IS_SHOPKEEPER(ch))
		return;

	if (AFF_FLAGGED(ch, EAffect::kInfravision))
		return;

	if ((obj = GET_EQ(ch, EEquipPos::kLight)) && (GET_OBJ_VAL(obj, 2) == 0 || !is_dark(ch->in_room))) {
		act("$n прекратил$g использовать $o3.", false, ch, obj, 0, kToRoom);
		PlaceObjToInventory(UnequipChar(ch, EEquipPos::kLight, CharEquipFlag::show_msg), ch);
	}

	if (!GET_EQ(ch, EEquipPos::kLight) && is_dark(ch->in_room)) {
		for (obj = ch->carrying; obj; obj = next) {
			next = obj->get_next_content();
			if (GET_OBJ_TYPE(obj) != EObjType::kLightSource) {
				continue;
			}
			if (GET_OBJ_VAL(obj, 2) == 0) {
				act("$n выбросил$g $o3.", false, ch, obj, 0, kToRoom);
				ExtractObjFromChar(obj);
				PlaceObjToRoom(obj, ch->in_room);
				continue;
			}
			//obj_from_char(obj);
			EquipObj(ch, obj, EEquipPos::kLight, CharEquipFlag::show_msg);
			return;
		}
	}
}

int npc_battle_scavenge(CharData *ch) {
	int max = false;
	ObjData *obj, *next_obj = nullptr;

	if (!MOB_FLAGGED(ch, EMobFlag::kScavenger))
		return (false);

	if (IS_SHOPKEEPER(ch))
		return (false);

	if (world[ch->in_room]->contents && number(0, GET_REAL_INT(ch)) > 10)
		for (obj = world[ch->in_room]->contents; obj; obj = next_obj) {
			next_obj = obj->get_next_content();
			if (CAN_GET_OBJ(ch, obj)
				&& !has_curse(obj)
				&& (ObjSystem::is_armor_type(obj)
					|| GET_OBJ_TYPE(obj) == EObjType::kWeapon)) {
				ExtractObjFromRoom(obj);
				PlaceObjToInventory(obj, ch);
				act("$n поднял$g $o3.", false, ch, obj, 0, kToRoom);
				max = true;
			}
		}
	return (max);
}

int npc_walk(CharData *ch) {
	int rnum, door = kBfsError;

	if (ch->in_room == kNowhere)
		return (kBfsError);

	if (GET_DEST(ch) == kNowhere || (rnum = real_room(GET_DEST(ch))) == kNowhere)
		return (kBfsError);

	if (ch->in_room == rnum) {
		if (ch->mob_specials.dest_count == 1)
			return (kBfsAlreadyThere);
		if (ch->mob_specials.dest_pos == ch->mob_specials.dest_count - 1 && ch->mob_specials.dest_dir >= 0)
			ch->mob_specials.dest_dir = -1;
		if (!ch->mob_specials.dest_pos && ch->mob_specials.dest_dir < 0)
			ch->mob_specials.dest_dir = 0;
		ch->mob_specials.dest_pos += ch->mob_specials.dest_dir >= 0 ? 1 : -1;
		if (((rnum = real_room(GET_DEST(ch))) == kNowhere)
			|| rnum == ch->in_room)
			return (kBfsError);
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

	if (!NPC_FLAGGED(ch, ENpcFlag::kStealing))
		return (false);

	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kPeaceful))
		return (false);

	if (victim->IsNpc() || IS_SHOPKEEPER(ch) || victim->GetEnemy())
		return (false);

	if (GetRealLevel(victim) >= kLvlImmortal)
		return (false);

	if (!CAN_SEE(ch, victim))
		return (false);

	if (AWAKE(victim) && (number(0, std::max(0, GetRealLevel(ch) - int_app[GET_REAL_INT(victim)].observation)) == 0)) {
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
				ExtractObjFromChar(best);
				PlaceObjToInventory(best, ch);
				max++;
			}
		}
	}
	return (max);
}

int npc_steal(CharData *ch) {
	if (!NPC_FLAGGED(ch, ENpcFlag::kStealing))
		return (false);

	if (GET_POS(ch) != EPosition::kStand || IS_SHOPKEEPER(ch) || ch->GetEnemy())
		return (false);

	for (const auto cons : world[ch->in_room]->people) {
		if (!cons->IsNpc()
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
	if (MOB_FLAGGED(ch, EMobFlag::kNoGroup)) {
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
		&& (AFF_FLAGGED(leader, EAffect::kCharmed)
			|| GET_POS(leader) < EPosition::kSleep)) {
		leader = nullptr;
	}

	// ноугруп моб не может быть лидером
	if (leader
		&& MOB_FLAGGED(leader, EMobFlag::kNoGroup)) {
		leader = nullptr;
	}

	// Find leader
	for (const auto vict : world[ch->in_room]->people) {
		if (!vict->IsNpc()
			|| GET_DEST(vict) != GET_DEST(ch)
			|| zone != ZONE(vict)
			|| group != GROUP(vict)
			|| MOB_FLAGGED(vict, EMobFlag::kNoGroup)
			|| AFF_FLAGGED(vict, EAffect::kCharmed)
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
			stop_follower(ch, kSfEmpty);
		}

		return;
	}

	if (leader->has_master()) {
		stop_follower(leader, kSfEmpty);
	}

	// Assign leader
	for (const auto vict : world[ch->in_room]->people) {
		if (!vict->IsNpc()
			|| GET_DEST(vict) != GET_DEST(ch)
			|| zone != ZONE(vict)
			|| group != GROUP(vict)
			|| AFF_FLAGGED(vict, EAffect::kCharmed)
			|| GET_POS(vict) < EPosition::kSleep) {
			continue;
		}

		if (vict == leader) {
			AFF_FLAGS(vict).set(EAffect::kGroup);
			continue;
		}

		if (!vict->has_master()) {
			leader->add_follower(vict);
		} else if (vict->get_master() != leader) {
			stop_follower(vict, kSfEmpty);
			leader->add_follower(vict);
		}
		AFF_FLAGS(vict).set(EAffect::kGroup);
	}
}

void npc_groupbattle(CharData *ch) {
	struct Follower *k;
	CharData *tch, *helper;

	if (!ch->IsNpc()
		|| !ch->GetEnemy()
		|| AFF_FLAGGED(ch, EAffect::kCharmed)
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
			&& !helper->GetEnemy()
			&& !helper->IsNpc()
			&& GET_POS(helper) > EPosition::kStun) {
			GET_POS(helper) = EPosition::kStand;
			SetFighting(helper, ch->GetEnemy());
			act("$n вступил$u за $N3.", false, helper, 0, ch, kToRoom);
		}
	}
}

int dump(CharData *ch, void * /*me*/, int cmd, char *argument) {
	ObjData *k;
	int value = 0;

	for (k = world[ch->in_room]->contents; k; k = world[ch->in_room]->contents) {
		act("$p рассыпал$U в прах!", false, 0, k, 0, kToRoom);
		ExtractObjFromWorld(k);
	}

	if (!CMD_IS("drop") || !CMD_IS("бросить"))
		return (0);

	do_drop(ch, argument, cmd, 0);

	for (k = world[ch->in_room]->contents; k; k = world[ch->in_room]->contents) {
		act("$p рассыпал$U в прах!", false, 0, k, 0, kToRoom);
		value += MAX(1, MIN(1, GET_OBJ_COST(k) / 10));
		ExtractObjFromWorld(k);
	}

	if (value) {
		SendMsgToChar("Боги оценили вашу жертву.\r\n", ch);
		act("$n оценен$y Богами.", true, ch, 0, 0, kToRoom);
		if (GetRealLevel(ch) < 3)
			EndowExpToChar(ch, value);
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
		if (!cons->IsNpc()->IsNpc()
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
		if (vict->GetEnemy() == ch
			&& !number(0, 4)) {
			target = vict;

			break;
		}
	}

	// if I didn't pick any of those, then just slam the guy I'm fighting //
	if (target == nullptr
		&& IN_ROOM(ch->GetEnemy()) == ch->in_room) {
		target = ch->GetEnemy();
	}

	// Hm...didn't pick anyone...I'll wait a round. //
	if (target == nullptr) {
		return (true);
	}

	if ((GetRealLevel(ch) > 13) && (number(0, 10) == 0)) {
		CastSpell(ch, target, nullptr, nullptr, ESpell::kSleep, ESpell::kSleep);
	}

	if ((GetRealLevel(ch) > 7) && (number(0, 8) == 0)) {
		CastSpell(ch, target, nullptr, nullptr, ESpell::kBlindness, ESpell::kBlindness);
	}

	if ((GetRealLevel(ch) > 12) && (number(0, 12) == 0)) {
		if (IS_EVIL(ch)) {
			CastSpell(ch, target, nullptr, nullptr, ESpell::kEnergyDrain, ESpell::kEnergyDrain);
		} else if (IS_GOOD(ch)) {
			CastSpell(ch, target, nullptr, nullptr, ESpell::kDispelEvil, ESpell::kDispelEvil);
		}
	}

	if (number(0, 4)) {
		return (true);
	}

	switch (GetRealLevel(ch)) {
		case 4:
		case 5: CastSpell(ch, target, nullptr, nullptr, ESpell::kMagicMissile, ESpell::kMagicMissile);
			break;
		case 6:
		case 7: CastSpell(ch, target, nullptr, nullptr, ESpell::kChillTouch, ESpell::kChillTouch);
			break;
		case 8:
		case 9: CastSpell(ch, target, nullptr, nullptr, ESpell::kBurningHands, ESpell::kBurningHands);
			break;
		case 10:
		case 11: CastSpell(ch, target, nullptr, nullptr, ESpell::kShockingGasp, ESpell::kShockingGasp);
			break;
		case 12:
		case 13: CastSpell(ch, target, nullptr, nullptr, ESpell::kLightingBolt, ESpell::kLightingBolt);
			break;
		case 14:
		case 15:
		case 16:
		case 17: CastSpell(ch, target, nullptr, nullptr, ESpell::kIceBolts, ESpell::kIceBolts);
			break;
		default: CastSpell(ch, target, nullptr, nullptr, ESpell::kFireball, ESpell::kFireball);
			break;
	}

	return (true);
}


// ********************************************************************
// *  Special procedures for mobiles                                  *
// ********************************************************************

/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.cpp if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
/*int guild_info[][3] =
	{
		// Midgaard
		{ECharClass::kConjurer, 3017, SCMD_SOUTH},
		{ECharClass::kSorcerer, 3004, SCMD_NORTH},
		{ECharClass::kThief, 3027, SCMD_EAST},
		{ECharClass::kWarrior, 3021, SCMD_EAST},

		// Brass Dragon
		{-999 *//* all *//* , 5065, SCMD_WEST},

		// this must go last -- add new guards above!
		{-1, -1, -1}
	};*/

/*int guild_guard(CharData *ch, void *me, int cmd, char * *//*argument*//*) {
	int i;
	CharData *guard = (CharData *) me;
	const char *buf = "Охранник остановил вас, преградив дорогу.\r\n";
	const char *buf2 = "Охранник остановил $n, преградив $m дорогу.";

	if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, EAffect::kBlind)
		|| AFF_FLAGGED(guard, EAffect::kHold))
		return (false);

	if (GetRealLevel(ch) >= kLvlImmortal)
		return (false);

	for (i = 0; guild_info[i][0] != -1; i++) {
		if ((ch->IsNpc() || ch->GetClass() != guild_info[i][0]) &&
			GET_ROOM_VNUM(ch->in_room) == guild_info[i][1] && cmd == guild_info[i][2]) {
			SendMsgToChar(buf, ch);
			act(buf2, false, ch, 0, 0, kToRoom);
			return (true);
		}
	}

	return (false);
}*/

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
				ExtractObjFromObj(temp);
				PlaceObjToRoom(temp, ch->in_room);
			}
			ExtractObjFromWorld(i);
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
		if (!CAN_WEAR(i, EWearFlag::kTake)) {
			continue;
		}

		if (GET_OBJ_TYPE(i) != EObjType::kLiquidContainer
			&& GET_OBJ_COST(i) >= 15) {
			continue;
		}

		act("$n picks up some trash.", false, ch, 0, 0, kToRoom);
		ExtractObjFromRoom(i);
		PlaceObjToInventory(i, ch);

		return true;
	}

	return false;
}

int cityguard(CharData *ch, void * /*me*/, int cmd, char * /*argument*/) {
	CharData *evil;
	int max_evil;

	if (cmd
		|| !AWAKE(ch)
		|| ch->GetEnemy()) {
		return (false);
	}

	max_evil = 1000;
	evil = 0;

	for (const auto tch : world[ch->in_room]->people) {
		if (!tch->IsNpc() && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, EPlrFlag::kKiller)) {
			act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", false, ch, 0, 0, kToRoom);
			hit(ch, tch, ESkill::kUndefined, fight::kMainHand);

			return (true);
		}
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (!tch->IsNpc() && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, EPlrFlag::kBurglar)) {
			act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", false, ch, 0, 0, kToRoom);
			hit(ch, tch, ESkill::kUndefined, fight::kMainHand);

			return (true);
		}
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (CAN_SEE(ch, tch) && tch->GetEnemy()) {
			if ((GET_ALIGNMENT(tch) < max_evil) && (tch->IsNpc() || tch->GetEnemy()->IsNpc())) {
				max_evil = GET_ALIGNMENT(tch);
				evil = tch;
			}
		}
	}

	if (evil
		&& (GET_ALIGNMENT(evil->GetEnemy()) >= 0)) {
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
		SendMsgToChar("Available pets are:\r\n", ch);
		for (const auto pet : world[pet_room]->people) {
			sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
			SendMsgToChar(buf, ch);
		}

		return (true);
	} else if (CMD_IS("buy")) {
		two_arguments(argument, buf, pet_name);

		if (!(pet = SearchCharInRoomByName(buf, pet_room))) {
			SendMsgToChar("There is no such pet!\r\n", ch);
			return (true);
		}
		if (ch->get_gold() < PET_PRICE(pet)) {
			SendMsgToChar("You don't have enough gold!\r\n", ch);
			return (true);
		}
		ch->remove_gold(PET_PRICE(pet));

		pet = read_mobile(GET_MOB_RNUM(pet), REAL);
		pet->set_exp(0);
		AFF_FLAGS(pet).set(EAffect::kCharmed);

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
		PlaceCharToRoom(pet, ch->in_room);
		ch->add_follower(pet);
		load_mtrigger(pet);

		// Be certain that pets can't get/carry/use/wield/wear items
		IS_CARRYING_W(pet) = 1000;
		IS_CARRYING_N(pet) = 100;

		SendMsgToChar("May you enjoy your pet.\r\n", ch);
		act("$n buys $N as a pet.", false, ch, 0, pet, kToRoom);

		return (1);
	}

	// All commands except list and buy
	return (0);
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
					ch->get_bank(), GetDeclensionInNumber(ch->get_bank(), EWhat::kMoneyA));
		else
			sprintf(buf, "У вас нет денег.\r\n");
		SendMsgToChar(buf, ch);
		return (1);
	} else if (CMD_IS("deposit") || CMD_IS("вложить") || CMD_IS("вклад")) {
		if ((amount = atoi(argument)) <= 0) {
			SendMsgToChar("Сколько вы хотите вложить?\r\n", ch);
			return (1);
		}
		if (ch->get_gold() < amount) {
			SendMsgToChar("О такой сумме вы можете только мечтать!\r\n", ch);
			return (1);
		}
		ch->remove_gold(amount, false);
		ch->add_bank(amount, false);
		sprintf(buf, "Вы вложили %d %s.\r\n", amount, GetDeclensionInNumber(amount, EWhat::kMoneyU));
		SendMsgToChar(buf, ch);
		act("$n произвел$g финансовую операцию.", true, ch, nullptr, nullptr, kToRoom);
		return (1);
	} else if (CMD_IS("withdraw") || CMD_IS("получить")) {
		if ((amount = atoi(argument)) <= 0) {
			SendMsgToChar("Уточните количество денег, которые вы хотите получить?\r\n", ch);
			return (1);
		}
		if (ch->get_bank() < amount) {
			SendMsgToChar("Да вы отродясь столько денег не видели!\r\n", ch);
			return (1);
		}
		ch->add_gold(amount, false);
		ch->remove_bank(amount, false);
		sprintf(buf, "Вы сняли %d %s.\r\n", amount, GetDeclensionInNumber(amount, EWhat::kMoneyU));
		SendMsgToChar(buf, ch);
		act("$n произвел$g финансовую операцию.", true, ch, nullptr, nullptr, kToRoom);
		return (1);
	} else if (CMD_IS("transfer") || CMD_IS("перевести")) {
		argument = one_argument(argument, arg);
		amount = atoi(argument);
		if (IS_GOD(ch) && !IS_IMPL(ch)) {
			SendMsgToChar("Почитить захотелось?\r\n", ch);
			return (1);

		}
		if (!*arg) {
			SendMsgToChar("Уточните кому вы хотите перевести?\r\n", ch);
			return (1);
		}
		if (amount <= 0) {
			SendMsgToChar("Уточните количество денег, которые вы хотите получить?\r\n", ch);
			return (1);
		}
		if (amount <= 100) {
			if (ch->get_bank() < (amount + 5)) {
				SendMsgToChar("У вас не хватит денег на налоги!\r\n", ch);
				return (1);
			}
		}

		if (ch->get_bank() < amount) {
			SendMsgToChar("Да вы отродясь столько денег не видели!\r\n", ch);
			return (1);
		}
		if (ch->get_bank() < amount + ((amount * 5) / 100)) {
			SendMsgToChar("У вас не хватит денег на налоги!\r\n", ch);
			return (1);
		}

		if ((vict = get_player_of_name(arg))) {
			ch->remove_bank(amount);
			if (amount <= 100) ch->remove_bank(5);
			else ch->remove_bank(((amount * 5) / 100));
			sprintf(buf, "%sВы перевели %d кун %s%s.\r\n", CCWHT(ch, C_NRM), amount,
					GET_PAD(vict, 2), CCNRM(ch, C_NRM));
			SendMsgToChar(buf, ch);
			vict->add_bank(amount);
			sprintf(buf, "%sВы получили %d кун банковским переводом от %s%s.\r\n", CCWHT(ch, C_NRM), amount,
					GET_PAD(ch, 1), CCNRM(ch, C_NRM));
			SendMsgToChar(buf, vict);
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
				SendMsgToChar("Такого персонажа не существует.\r\n", ch);
				delete vict;
				return (1);
			}

			ch->remove_bank(amount);
			if (amount <= 100) ch->remove_bank(5);
			else ch->remove_bank(((amount * 5) / 100));
			sprintf(buf, "%sВы перевели %d кун %s%s.\r\n", CCWHT(ch, C_NRM), amount,
					GET_PAD(vict, 2), CCNRM(ch, C_NRM));
			SendMsgToChar(buf, ch);
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
