/* ************************************************************************
*   File: spells.cpp                                    Part of Bylins    *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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

#include "spells.h"
#include "engine/db/global_objects.h"
#include "engine/ui/cmd/do_follow.h"
#include "engine/ui/cmd/do_hire.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/ai/mobact.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/liquid.h"
#include "magic.h"
#include "magic_internal.h"
#include "gameplay/skills/animal_master.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/communication/parcel.h"
#include "administration/privilege.h"
#include "engine/ui/color.h"
#include "engine/ui/cmd/do_flee.h"
#include "engine/ui/cmd_god/do_stat.h"
#include "gameplay/mechanics/stuff.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/ai/mob_memory.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/minions.h"

#include <cmath>

extern char cast_argument[kMaxInputLength];
extern im_type *imtypes;
extern int top_imtypes;

void weight_change_object(ObjData *obj, int weight);
int CalcBaseAc(CharData *ch);
char *diag_weapon_to_char(const CObjectPrototype *obj, int show_wear);
void SetPrecipitations(int *wtype, int startvalue, int chance1, int chance2, int chance3);
int CalcAntiSavings(CharData *ch);
void do_tell(CharData *ch, char *argument, int cmd, int subcmd);
void RemoveEquipment(CharData *ch, int pos);
int pk_action_type_summon(CharData *agressor, CharData *victim);
int pk_increment_revenge(CharData *agressor, CharData *victim);

int what_sky = kSkyCloudless;
// * Special spells appear below.

ESkill GetMagicSkillId(ESpell spell_id) {
	switch (MUD::Spell(spell_id).GetElement()) {
		case EElement::kAir: return ESkill::kAirMagic;
			break;
		case EElement::kFire: return ESkill::kFireMagic;
			break;
		case EElement::kWater: return ESkill::kWaterMagic;
			break;
		case EElement::kEarth: return ESkill::kEarthMagic;
			break;
		case EElement::kLight: return ESkill::kLightMagic;
			break;
		case EElement::kDark: return ESkill::kDarkMagic;
			break;
		case EElement::kMind: return ESkill::kMindMagic;
			break;
		case EElement::kLife: return ESkill::kLifeMagic;
			break;
		case EElement::kUndefined: [[fallthrough]];
		default: return ESkill::kUndefined;
	}
}

//Определим мин уровень для изучения спелла из книги
//req_lvl - требуемый уровень из книги
int CalcMinSpellLvl(const CharData *ch, ESpell spell_id, int req_lvl) {
	int min_lvl = std::max(req_lvl, MUD::Class(ch->GetClass()).spells[spell_id].GetMinLevel())
		- (std::max(0, GetRealRemort(ch)/ MUD::Class(ch->GetClass()).GetSpellLvlDecrement()));

	return std::max(1, min_lvl);
}

int CalcMinSpellLvl(const CharData *ch, ESpell spell_id) {
	auto min_lvl = MUD::Class(ch->GetClass()).spells[spell_id].GetMinLevel()
		- GetRealRemort(ch)/ MUD::Class(ch->GetClass()).GetSpellLvlDecrement();

	return std::max(1, min_lvl);
}

int CalcMinRuneSpellLvl(const CharData *ch, ESpell spell_id) {
	int min_lvl;

	// Read from the new rune_spells registry.
	const auto &runes = MUD::RuneSpells();
	if (auto it = runes.find(spell_id); it != runes.end()) {
		min_lvl = it->second.min_caster_level - GetRealRemort(ch) / MUD::Class(ch->GetClass()).GetSpellLvlDecrement();
	} else {
		return 999;
	}
	return std::max(1, min_lvl);
}

bool CanGetSpell(const CharData *ch, ESpell spell_id, int req_lvl) {
	if (MUD::Class(ch->GetClass()).spells.IsUnavailable(spell_id) ||
		CalcMinSpellLvl(ch, spell_id, req_lvl) > GetRealLevel(ch) ||
		MUD::Class(ch->GetClass()).spells[spell_id].GetMinRemort() > GetRealRemort(ch)) {
		return false;
	}

	return true;
};

// Функция определяет возможность изучения спелла из книги или в гильдии
bool CanGetSpell(CharData *ch, ESpell spell_id) {
	if (MUD::Class(ch->GetClass()).spells.IsUnavailable(spell_id)) {
		return false;
	}

	if (CalcMinSpellLvl(ch, spell_id) > GetRealLevel(ch) ||
		MUD::Class(ch->GetClass()).spells[spell_id].GetMinRemort() > GetRealRemort(ch)) {
		return false;
	}

	return true;
};

typedef std::map<EIngredientFlag, std::string> EIngredientFlag_name_by_value_t;
typedef std::map<const std::string, EIngredientFlag> EIngredientFlag_value_by_name_t;
EIngredientFlag_name_by_value_t EIngredientFlag_name_by_value;
EIngredientFlag_value_by_name_t EIngredientFlag_value_by_name;

void init_EIngredientFlag_ITEM_NAMES() {
	EIngredientFlag_name_by_value.clear();
	EIngredientFlag_value_by_name.clear();

	EIngredientFlag_name_by_value[EIngredientFlag::kItemRunes] = "kItemRunes";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemCheckUses] = "kItemCheckUses";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemCheckLag] = "kItemCheckLag";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemCheckLevel] = "kItemCheckLevel";
	EIngredientFlag_name_by_value[EIngredientFlag::kItemDecayEmpty] = "kItemDecayEmpty";

	for (const auto &i : EIngredientFlag_name_by_value) {
		EIngredientFlag_value_by_name[i.second] = i.first;
	}
}

template<>
EIngredientFlag ITEM_BY_NAME(const std::string &name) {
	if (EIngredientFlag_name_by_value.empty()) {
		init_EIngredientFlag_ITEM_NAMES();
	}
	return EIngredientFlag_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM<EIngredientFlag>(const EIngredientFlag item) {
	if (EIngredientFlag_name_by_value.empty()) {
		init_EIngredientFlag_ITEM_NAMES();
	}
	return EIngredientFlag_name_by_value.at(item);
}


// Look up kSummonFail in `spell_id`'s sheaf (per-spell override on each summon-
// style spell, with kDefault random-variant fallback) and emit to the caster.
/*
#define MIN_NEWBIE_ZONE  20
#define MAX_NEWBIE_ZONE  79
#define MAX_SUMMON_TRIES 2000
*/

// Поиск комнаты для перемещающего заклинания
// ch - кого перемещают, rnum_start - первая комната диапазона, rnum_stop - последняя комната диапазона
int GetTeleportTargetRoom(CharData *ch, int rnum_start, int rnum_stop) {
	int *r_array;
	int n, i, j;
	int fnd_room = kNowhere;

	n = rnum_stop - rnum_start + 1;

	if (n <= 0) {
		return kNowhere;
	}

	r_array = (int *) malloc(n * sizeof(int));
	for (i = 0; i < n; ++i)
		r_array[i] = rnum_start + i;

	for (; n; --n) {
		j = number(0, n - 1);
		fnd_room = r_array[j];
		r_array[j] = r_array[n - 1];

		if (SECT(fnd_room) != ESector::kSecret &&
			!ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap) &&
			!ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel) &&
			!ROOM_FLAGGED(fnd_room, ERoomFlag::kNoTeleportIn) &&
			!ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap) &&
			!ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) &&
			(!ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom) || ch->IsImmortal()) &&
			Clan::MayEnter(ch, fnd_room, kHousePortal))
			break;
	}

	free(r_array);

	return n ? fnd_room : kNowhere;
}


// ПРЫЖОК в рамках зоны


void CheckAutoNosummon(CharData *ch) {
	if (ch->IsFlagged(EPrf::kAutonosummon) && ch->IsFlagged(EPrf::KSummonable)) {
		ch->UnsetFlag(EPrf::KSummonable);
		SendMsgToChar("Режим автопризыв: вы защищены от призыва.\r\n", ch);
	}
}


// pk_unique: when non-zero, marks this portal as a PK-revenge/fight pentagram and
// carries the imposing caster's uid. Replaces the old RoomData::pkPenterUnique
// (which was per-room, ambiguous when multiple pentas land in one room, and had
// to be cleared by hand). Read by show_room_affects (Pk variant selection) and
// do_enter (entry gate); cleared automatically when the affect expires.
void AddPortalTimer(CharData *ch, RoomData *from_room, RoomRnum to_room, int time,
					long pk_unique) {

	Affect<room_spells::ERoomApply> af;
	af.type = ESpell::kPortalTimer;
	af.affect_type = static_cast<room_spells::ERoomAffect>(0);
	af.duration = time; //раз в 2 секунды
	af.modifier = to_room;
	af.battleflag = 0;
	af.location = room_spells::ERoomApply::kNone;
	af.caster_id = ch? ch->get_uid() : 0;
	af.apply_time = 0;
	af.pk_unique = pk_unique;
	room_spells::affect_to_room(from_room, af);
	room_spells::AddRoomToAffected(from_room);
}

void RemovePortalGate(RoomRnum rnum) {
	auto aff = room_spells::FindAffect(world[rnum], ESpell::kPortalTimer);
	const RoomRnum to_room = (*aff)->modifier;
	// kPortal sheaf kCustomMsgThree holds "Пентаграмма была
	// разрушена." -- emitted to both char and room of each affected portal endpoint.
	const auto &broken_msg = MUD::SpellMessages().GetMessage(
			ESpell::kPortal, ESpellMsg::kCustomMsgThree);

	if (aff != world[rnum]->affected.end()) {
		room_spells::RoomRemoveAffect(world[rnum], aff);
		act(broken_msg.c_str(), false, world[rnum]->first_character(), 0, 0, kToRoom);
		act(broken_msg.c_str(), false, world[rnum]->first_character(), 0, 0, kToChar);
	}
	aff = room_spells::FindAffect(world[to_room], ESpell::kPortalTimer);
	if (aff != world[to_room]->affected.end()) {
		room_spells::RoomRemoveAffect(world[to_room], aff);
		act(broken_msg.c_str(), false, world[to_room]->first_character(), 0, 0, kToRoom);
		act(broken_msg.c_str(), false, world[to_room]->first_character(), 0, 0, kToChar);
	}
}


bool CatchBloodyCorpse(ObjData *l) {
	bool temp_bloody = false;
	ObjData *next_element;

	if (!l->get_contains()) {
		return false;
	}

	if (bloody::is_bloody(l->get_contains())) {
		return true;
	}

	if (!l->get_contains()->get_next_content()) {
		return false;
	}

	next_element = l->get_contains()->get_next_content();
	while (next_element) {
		if (next_element->get_contains()) {
			temp_bloody = CatchBloodyCorpse(next_element->get_contains());
			if (temp_bloody) {
				return true;
			}
		}

		if (bloody::is_bloody(next_element)) {
			return true;
		}

		next_element = next_element->get_contains();
	}

	return false;
}

int CheckCharmices(CharData *ch, CharData *victim, ESpell spell_id) {
	int cha_summ = 0, reformed_hp_summ = 0;
	bool undead_in_group = false, living_in_group = false;

	for (auto *k : ch->followers) {
		if (AFF_FLAGGED(k, EAffect::kCharmed)
			&& k->get_master() == ch) {
			cha_summ++;
			//hp_summ += GET_REAL_MAX_HIT(k->ch);
			reformed_hp_summ += GetReformedCharmiceHp(ch, k, spell_id);
// Проверка на тип последователей -- некрасиво, зато эффективно
			if (k->IsFlagged(EMobFlag::kCorpse)) {
				undead_in_group = true;
			} else {
				living_in_group = true;
			}
		}
	}

	if (undead_in_group && living_in_group) {
		mudlog("SYSERR: Undead and living in group simultaniously", NRM, kLvlGod, ERRLOG, true);
		return (false);
	}

	if (spell_id == ESpell::kCharm && undead_in_group) {
		SendMsgToChar("Ваша жертва боится ваших последователей.\r\n", ch);
		return (false);
	}

	if (spell_id != ESpell::kCharm && living_in_group) {
		SendMsgToChar("Ваш последователь мешает вам произнести это заклинание.\r\n", ch);
		return (false);
	}

	if (spell_id == ESpell::kClone && cha_summ >= std::max(1, (GetRealLevel(ch) + 4) / 5 - 2)) {
		SendMsgToChar("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (false);
	}

	if (spell_id != ESpell::kClone && cha_summ >= (GetRealLevel(ch) + 9) / 10) {
		SendMsgToChar("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (false);
	}

	if (spell_id != ESpell::kClone &&
		reformed_hp_summ + GetReformedCharmiceHp(ch, victim, spell_id) >= CalcCharmPoint(ch, spell_id)) {
		SendMsgToChar("Вам не под силу управлять такой боевой мощью.\r\n", ch);
		return (false);
	}
	return (true);
}


void ShowWeapon(CharData *ch, ObjData *obj) {
	if (obj->get_type() == EObjType::kWeapon) {
		*buf = '\0';
		if (CAN_WEAR(obj, EWearFlag::kWield)) {
			sprintf(buf, "Можно взять %s в правую руку.\r\n", OBJN(obj, ch, ECase::kAcc));
		}

		if (CAN_WEAR(obj, EWearFlag::kHold)) {
			sprintf(buf + strlen(buf), "Можно взять %s в левую руку.\r\n", OBJN(obj, ch, ECase::kAcc));
		}

		if (CAN_WEAR(obj, EWearFlag::kBoth)) {
			sprintf(buf + strlen(buf), "Можно взять %s в обе руки.\r\n", OBJN(obj, ch, ECase::kAcc));
		}

		if (*buf) {
			SendMsgToChar(buf, ch);
		}
	}
}

void PrintBookUpgradeSkill(CharData *ch, const ObjData *obj) {
	const auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(obj, 1));
	if (MUD::Skills().IsInvalid(skill_id)) {
		log("SYSERR: invalid skill_id: %d, ch_name=%s, ObjVnum=%d (%s %s %d)",
			GET_OBJ_VAL(obj, 1), ch->get_name().c_str(), GET_OBJ_VNUM(obj), __FILE__, __func__, __LINE__);
		return;
	}
	if (GET_OBJ_VAL(obj, 3) > 0) {
		SendMsgToChar(ch, "повышает умение \"%s\" (максимум %d)\r\n",
					  MUD::Skill(skill_id).GetName(), GET_OBJ_VAL(obj, 3));
	} else {
		SendMsgToChar(ch, "повышает умение \"%s\" (не больше максимума текущего перевоплощения)\r\n",
					  MUD::Skill(skill_id).GetName());
	}
}

// Per-type detail block of MortShowObjValues. Pulled out so the parent
// stays under the 200-line ceiling and the type-specific rendering can be
// read without scrolling past the shared header / footer code.
static void ShowObjTypeSpecificValues(const ObjData *obj, CharData *ch) {
	int i, j, drndice = 0, drsdice = 0;
	long int li;
	(void) i; (void) j; (void) li;  // some branches do not touch all of them
switch (obj->get_type()) {
	case EObjType::kScroll:
	case EObjType::kPotion: {
		std::ostringstream out;
		out << "Содержит заклинание:";
		for (auto val = 1; val < 4; ++val) {
			auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, val));
			if (MUD::Spell(spell_id).IsValid()) {
				out << " Ур. [" << GET_OBJ_VAL(obj, 0) << "] " << MUD::Spell(spell_id).GetName() << ",";
			}
		}
		if (out.str().back() == ',') {
			out.seekp(-1, out.end);
		}
		out << "\r\n";
		SendMsgToChar(out.str(), ch);
		break;
	}
	case EObjType::kWand:
	case EObjType::kStaff: sprintf(buf, "Вызывает заклинания: ");
		sprintf(buf + strlen(buf), " %s\r\n",
				MUD::Spell(static_cast<ESpell>(GET_OBJ_VAL(obj, 3))).GetCName());
		sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n",
				GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
		SendMsgToChar(buf, ch);
		break;

	case EObjType::kWeapon: drndice = GET_OBJ_VAL(obj, 1);
		drsdice = GET_OBJ_VAL(obj, 2);
		sprintf(buf, "Наносимые повреждения '%dD%d'", drndice, drsdice);
		sprintf(buf + strlen(buf), " среднее %.1f.\r\n", ((drsdice + 1) * drndice / 2.0));
		SendMsgToChar(buf, ch);
		break;

	case EObjType::kArmor:
	case EObjType::kLightArmor:
	case EObjType::kMediumArmor:
	case EObjType::kHeavyArmor: drndice = GET_OBJ_VAL(obj, 0);
		drsdice = GET_OBJ_VAL(obj, 1);
		sprintf(buf, "защита (AC) : %d\r\n", drndice);
		SendMsgToChar(buf, ch);
		sprintf(buf, "броня       : %d\r\n", drsdice);
		SendMsgToChar(buf, ch);
		break;

	case EObjType::kBook:
		switch (GET_OBJ_VAL(obj, 0)) {
			case EBook::kSpell: {
				auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, 1));
				if (spell_id >= ESpell::kFirst && spell_id <= ESpell::kLast) {
					drndice = GET_OBJ_VAL(obj, 1);
					if (MUD::Class(ch->GetClass()).spells.IsAvailable(spell_id)) {
						drsdice = CalcMinSpellLvl(ch, spell_id, GET_OBJ_VAL(obj, 2));
					} else {
						drsdice = kLvlImplementator;
					}
					sprintf(buf, "содержит заклинание        : \"%s\"\r\n", MUD::Spell(spell_id).GetCName());
					SendMsgToChar(buf, ch);
					sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
					SendMsgToChar(buf, ch);
				}
				break;
			}
			case EBook::kSkill: {
				auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(obj, 1));
				if (MUD::Skills().IsValid(skill_id)) {
					drndice = GET_OBJ_VAL(obj, 1);
					if (MUD::Class(ch->GetClass()).skills[skill_id].IsAvailable()) {
						drsdice = GetSkillMinLevel(ch, skill_id, GET_OBJ_VAL(obj, 2));
					} else {
						drsdice = kLvlImplementator;
					}
					sprintf(buf, "содержит секрет умения     : \"%s\"\r\n", MUD::Skill(skill_id).GetName());
					SendMsgToChar(buf, ch);
					sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
					SendMsgToChar(buf, ch);
				}
				break;
			}
			case EBook::kSkillUpgrade: PrintBookUpgradeSkill(ch, obj);
				break;

			case EBook::kReceipt: drndice = im_get_recipe(GET_OBJ_VAL(obj, 1));
				if (drndice >= 0) {
					drsdice = std::max(GET_OBJ_VAL(obj, 2), imrecipes[drndice].level);
					int count = imrecipes[drndice].remort;
					if (imrecipes[drndice].classknow[to_underlying(ch->GetClass())] != kKnownRecipe)
						drsdice = kLvlImplementator;
					sprintf(buf, "содержит рецепт отвара     : \"%s\"\r\n", imrecipes[drndice].name);
					SendMsgToChar(buf, ch);
					if (drsdice == -1 || count == -1) {
						SendMsgToChar(kColorBoldRed, ch);
						SendMsgToChar("Некорректная запись рецепта для вашего класса - сообщите Богам.\r\n", ch);
						SendMsgToChar(kColorNrm, ch);
					} else if (drsdice == kLvlImplementator) {
						sprintf(buf, "уровень изучения (количество ремортов) : %d (--)\r\n", drsdice);
						SendMsgToChar(buf, ch);
					} else {
						sprintf(buf, "уровень изучения (количество ремортов) : %d (%d)\r\n", drsdice, count);
						SendMsgToChar(buf, ch);
					}
				}
				break;

			case EBook::kFeat: {
				const auto feat_id = static_cast<EFeat>(GET_OBJ_VAL(obj, 1));
				if (MUD::Feat(feat_id).IsValid()) {
					if (CanGetFeat(ch, feat_id)) {
						drsdice = MUD::Class(ch->GetClass()).feats[feat_id].GetSlot();
					} else {
						drsdice = kLvlImplementator;
					}
					sprintf(buf, "содержит секрет способности : \"%s\"\r\n", MUD::Feat(feat_id).GetCName());
					SendMsgToChar(buf, ch);
					sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
					SendMsgToChar(buf, ch);
				}
			}
				break;

			default: SendMsgToChar(kColorBoldRed, ch);
				SendMsgToChar("НЕВЕРНО УКАЗАН ТИП КНИГИ - сообщите Богам\r\n", ch);
				SendMsgToChar(kColorNrm, ch);
				break;
		}
		break;

	case EObjType::kMagicIngredient: sprintbit(obj->get_spec_param(), ingradient_bits, buf2, sizeof(buf2));
		snprintf(buf, kMaxStringLength, "%s\r\n", buf2);
		SendMsgToChar(buf, ch);

		if (IS_SET(obj->get_spec_param(), kItemCheckUses)) {
			sprintf(buf, "можно применить %d раз\r\n", GET_OBJ_VAL(obj, 2));
			SendMsgToChar(buf, ch);
		}

		if (IS_SET(obj->get_spec_param(), kItemCheckLag)) {
			sprintf(buf, "можно применить 1 раз в %d сек", (i = GET_OBJ_VAL(obj, 0) & 0xFF));
			if (GET_OBJ_VAL(obj, 3) == 0 || GET_OBJ_VAL(obj, 3) + i < time(nullptr))
				strcat(buf, "(можно применять).\r\n");
			else {
				li = GET_OBJ_VAL(obj, 3) + i - time(nullptr);
				sprintf(buf + strlen(buf), "(осталось %ld сек).\r\n", li);
			}
			SendMsgToChar(buf, ch);
		}

		if (IS_SET(obj->get_spec_param(), kItemCheckLevel)) {
			sprintf(buf, "можно применить с %d уровня.\r\n", (GET_OBJ_VAL(obj, 0) >> 8) & 0x1F);
			SendMsgToChar(buf, ch);
		}

		if ((i = GetObjRnum(GET_OBJ_VAL(obj, 1))) >= 0) {
			sprintf(buf, "прототип %s%s%s.\r\n",
					kColorBoldCyn, obj_proto[i]->get_PName(ECase::kNom).c_str(), kColorNrm);
			SendMsgToChar(buf, ch);
		}
		break;

	case EObjType::kMagicComponent:
		for (j = 0; imtypes[j].id != GET_OBJ_VAL(obj, IM_TYPE_SLOT) && j <= top_imtypes;) {
			j++;
		}
		sprintf(buf, "Это ингредиент вида '%s%s%s'\r\n", kColorCyn, imtypes[j].name, kColorNrm);
		SendMsgToChar(buf, ch);
		i = GET_OBJ_VAL(obj, IM_POWER_SLOT);
		if (i > 45) { // тут явно опечатка была, кроме того у нас мобы и выше 40лвл
			SendMsgToChar("Вы не в состоянии определить качество этого ингредиента.\r\n", ch);
		} else {
			sprintf(buf, "Качество ингредиента ");
			if (i > 40)
				strcat(buf, "божественное.\r\n");
			else if (i > 35)
				strcat(buf, "идеальное.\r\n");
			else if (i > 30)
				strcat(buf, "наилучшее.\r\n");
			else if (i > 25)
				strcat(buf, "превосходное.\r\n");
			else if (i > 20)
				strcat(buf, "отличное.\r\n");
			else if (i > 15)
				strcat(buf, "очень хорошее.\r\n");
			else if (i > 10)
				strcat(buf, "выше среднего.\r\n");
			else if (i > 5)
				strcat(buf, "весьма посредственное.\r\n");
			else
				strcat(buf, "хуже не бывает.\r\n");
			SendMsgToChar(buf, ch);
		}
		break;

		//Информация о контейнерах (Купала)
	case EObjType::kContainer: sprintf(buf, "Максимально вместимый вес: %d.\r\n", GET_OBJ_VAL(obj, 0));
		SendMsgToChar(buf, ch);
		break;

		//Информация о емкостях (Купала)
	case EObjType::kLiquidContainer: drinkcon::identify(ch, obj);
		break;

	case EObjType::kMagicArrow:
	case EObjType::kMagicContaner: sprintf(buf, "Может вместить стрел: %d.\r\n", GET_OBJ_VAL(obj, 1));
		sprintf(buf, "Осталось стрел: %s%d&n.\r\n",
				GET_OBJ_VAL(obj, 2) > 3 ? "&G" : "&R", GET_OBJ_VAL(obj, 2));
		SendMsgToChar(buf, ch);
		break;

	default: break;
} // switch
}

void MortShowObjValues(const ObjData *obj, CharData *ch, int fullness) {
	int i;
	bool found;
	bool enhansed_scroll = false;
	
	if (fullness > 399) {
		enhansed_scroll = true;
	}
	SendMsgToChar("Вы узнали следующее:\r\n", ch);
	sprintf(buf, "Предмет \"%s\", тип : ", obj->get_short_description().c_str());
	sprinttype(obj->get_type(), item_types, buf2);
	strcat(buf, buf2);
	strcat(buf, "\r\n");
	SendMsgToChar(buf, ch);

	strcpy(buf, diag_weapon_to_char(obj, 2));
	if (*buf)
		SendMsgToChar(buf, ch);

	if (fullness < 20)
		return;

	//ShowWeapon(ch, obj);

	sprintf(buf, "Вес: %d, Цена: %d, Рента: %d(%d)\r\n",
			obj->get_weight(), obj->get_cost(), obj->get_rent_off(), obj->get_rent_on());
	SendMsgToChar(buf, ch);

	if (fullness < 30)
		return;
	sprinttype(obj->get_material(), material_name, buf2);
	snprintf(buf, kMaxStringLength, "Материал : %s, макс.прочность : %d, тек.прочность : %d\r\n", buf2,
			 obj->get_maximum_durability(), obj->get_current_durability());
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);

	if (fullness < 40)
		return;

	SendMsgToChar("Неудобен : ", ch);
	SendMsgToChar(kColorCyn, ch);
	obj->get_no_flags().sprintbits(no_bits, buf, sizeof(buf), ",", ch->IsImmortal() ? 4 : 0);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);

	if (fullness < 50)
		return;

	SendMsgToChar("Недоступен : ", ch);
	SendMsgToChar(kColorCyn, ch);
	obj->get_anti_flags().sprintbits(anti_bits, buf, sizeof(buf), ",", ch->IsImmortal() ? 4 : 0);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);

	if (obj->get_auto_mort_req() > 0) {
		SendMsgToChar(ch, "Требует перевоплощений : %s%d%s\r\n",
					  kColorCyn, obj->get_auto_mort_req(), kColorNrm);
	} else if (obj->get_auto_mort_req() < -1) {
		SendMsgToChar(ch, "Максимальное количество перевоплощение : %s%d%s\r\n",
					  kColorCyn, abs(obj->get_minimum_remorts()), kColorNrm);
	}

	if (fullness < 60)
		return;

	SendMsgToChar("Имеет экстрафлаги: ", ch);
	SendMsgToChar(kColorCyn, ch);
	obj->get_extra_flags().sprintbits(extra_bits, buf, sizeof(buf), ",", ch->IsImmortal() ? 4 : 0);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);
//enhansed_scroll = true; //для теста
	if (enhansed_scroll) {
		if (stable_objs::IsTimerUnlimited(obj))
			sprintf(buf2, "Таймер: %d/нерушимо.", obj_proto[obj->get_rnum()]->get_timer());
		else
			sprintf(buf2, "Таймер: %d/%d.", obj_proto[obj->get_rnum()]->get_timer(), obj->get_timer());
		char miw[128];
		if (GetObjMIW(obj->get_rnum()) < 0) {
			sprintf(miw, "%s", "бесконечно");
		} else {
			sprintf(miw, "%d", GetObjMIW(obj->get_rnum()));
		}
		snprintf(buf, kMaxStringLength, "&GСейчас в мире : %d. На постое : %d. Макс. в мире : %s. %s&n\r\n",
				 obj_proto.total_online(obj->get_rnum()), obj_proto.stored(obj->get_rnum()), miw, buf2);
		SendMsgToChar(buf, ch);
	}
	if (fullness < 75)
		return;

	ShowObjTypeSpecificValues(obj, ch);

	if (fullness < 90) {
		return;
	}

	SendMsgToChar("Накладывает на вас аффекты: ", ch);
	SendMsgToChar(kColorCyn, ch);
	obj->get_affect_flags().sprintbits(weapon_affects, buf, sizeof(buf), ",", ch->IsImmortal() ? 4 : 0);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);

	if (fullness < 100) {
		return;
	}

	found = false;
	for (i = 0; i < kMaxObjAffect; i++) {
		if (obj->get_affected(i).location != EApply::kNone
			&& obj->get_affected(i).modifier != 0) {
			if (!found) {
				SendMsgToChar("Дополнительные свойства :\r\n", ch);
				found = true;
			}
			print_obj_affects(ch, obj->get_affected(i));
		}
	}

	if (obj->get_type() == EObjType::kEnchant
		&& GET_OBJ_VAL(obj, 0) != 0) {
		if (!found) {
			SendMsgToChar("Дополнительные свойства :\r\n", ch);
			found = true;
		}
		SendMsgToChar(ch, "%s   %s вес предмета на %d%s\r\n", kColorCyn,
					  GET_OBJ_VAL(obj, 0) > 0 ? "увеличивает" : "уменьшает",
					  abs(GET_OBJ_VAL(obj, 0)), kColorNrm);
	}

	if (obj->has_skills()) {
		SendMsgToChar("Меняет умения :\r\n", ch);
		CObjectPrototype::skills_t skills;
		obj->get_skills(skills);
		int percent;
		for (const auto &it : skills) {
			auto skill_id = it.first;
			percent = it.second;

			if (percent == 0) // TODO: такого не должно быть?
				continue;

			sprintf(buf, "   %s%s%s%s%s%d%%%s\r\n",
					kColorCyn, MUD::Skill(skill_id).GetName(), kColorNrm,
					kColorCyn,
					percent < 0 ? " ухудшает на " : " улучшает на ", abs(percent), kColorNrm);
			SendMsgToChar(buf, ch);
		}
	}

	auto it = ObjData::set_table.begin();
	if (obj->has_flag(EObjFlag::kSetItem)) {
		for (; it != ObjData::set_table.end(); it++) {
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end()) {
				sprintf(buf,
						"Часть набора предметов: %s%s%s\r\n",
						kColorNrm,
						it->second.get_name().c_str(),
						kColorNrm);
				SendMsgToChar(buf, ch);
				for (auto & vnum : it->second) {
					const int r_num = GetObjRnum(vnum.first);
					if (r_num < 0) {
						SendMsgToChar("Неизвестный объект!!!\r\n", ch);
						continue;
					}
					sprintf(buf, "   %s\r\n", obj_proto[r_num]->get_short_description().c_str());
					SendMsgToChar(buf, ch);
				}
				break;
			}
		}
	}

	if (!obj->get_enchants().empty()) {
		obj->get_enchants().print(ch);
	}
	obj_sets::print_identify(ch, obj);
}

void MortShowCharValues(CharData *victim, CharData *ch, int fullness) {
	int val0, val1, val2;

	sprintf(buf, "Имя: %s\r\n", GET_NAME(victim));
	SendMsgToChar(buf, ch);
	if (!victim->IsNpc() && victim == ch) {
		sprintf(buf, "Написание : %s/%s/%s/%s/%s/%s\r\n",
				GET_PAD(victim, 0), GET_PAD(victim, 1), GET_PAD(victim, 2),
				GET_PAD(victim, 3), GET_PAD(victim, 4), GET_PAD(victim, 5));
		SendMsgToChar(buf, ch);
	}

	if (!victim->IsNpc() && victim == ch) {
		const auto &victimAge = CalcCharAge(victim);
		sprintf(buf, "Возраст %s  : %d лет, %d месяцев, %d дней и %d часов.\r\n",
				GET_PAD(victim, 1), victimAge->year, victimAge->month, victimAge->day, victimAge->hours);
		SendMsgToChar(buf, ch);
	}
	if (fullness < 20 && ch != victim)
		return;

	val0 = GET_HEIGHT(victim);
	val1 = GET_WEIGHT(victim);
	val2 = GET_SIZE(victim);
	sprintf(buf, "Вес %d, Размер %d\r\n", val1,
			val2);
	SendMsgToChar(buf, ch);
	if (fullness < 60 && ch != victim)
		return;

	val0 = GetRealLevel(victim);
	val1 = victim->get_hit();
	val2 = victim->get_real_max_hit();
	sprintf(buf, "Уровень : %d, может выдержать повреждений : %d(%d), ", val0, val1, val2);
	SendMsgToChar(buf, ch);
	SendMsgToChar(ch, "Перевоплощений : %d\r\n", GetRealRemort(victim));
	val0 = MIN(GET_AR(victim), 100);
	val1 = MIN(GET_MR(victim), 100);
	val2 = MIN(GET_PR(victim), 100);
	sprintf(buf,
			"Защита от чар : %d, Защита от магических повреждений : %d, Защита от физических повреждений : %d\r\n",
			val0,
			val1,
			val2);
	SendMsgToChar(buf, ch);
	if (fullness < 90 && ch != victim)
		return;

	SendMsgToChar(ch, "Атака : %d, Повреждения : %d\r\n",
				  GET_HR(victim), GET_DR(victim));
	SendMsgToChar(ch, "Защита : %d, Броня : %d, Поглощение : %d\r\n",
				  CalcBaseAc(victim), GET_ARMOUR(victim), GET_ABSORBE(victim));

	if (fullness < 100 || (ch != victim && !victim->IsNpc()))
		return;

	val0 = victim->get_str();
	val1 = victim->get_int();
	val2 = victim->get_wis();
	sprintf(buf, "Сила: %d, Ум: %d, Муд: %d, ", val0, val1, val2);
	val0 = victim->get_dex();
	val1 = victim->get_con();
	val2 = victim->get_cha();
	sprintf(buf + strlen(buf), "Ловк: %d, Тел: %d, Обаян: %d\r\n", val0, val1, val2);
	SendMsgToChar(buf, ch);

	if (fullness < 120 || (ch != victim && !victim->IsNpc()))
		return;

	int found = false;
	for (const auto &aff : victim->affected) {
		if (aff->location != EApply::kNone && aff->modifier != 0) {
			if (!found) {
				SendMsgToChar("Дополнительные свойства :\r\n", ch);
				found = true;
				SendMsgToChar(kColorBoldRed, ch);
			}
			sprinttype(aff->location, apply_types, buf2);
			snprintf(buf,
					 kMaxStringLength,
					 "   %s изменяет на %s%d\r\n",
					 buf2,
					 aff->modifier > 0 ? "+" : "",
					 aff->modifier);
			SendMsgToChar(buf, ch);
		}
	}
	SendMsgToChar(kColorNrm, ch);

	SendMsgToChar("Аффекты :\r\n", ch);
	SendMsgToChar(kColorBoldCyn, ch);
	victim->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, sizeof(buf2), "\r\n", ch->IsImmortal() ? 4 : 0);
	snprintf(buf, kMaxStringLength, "%s\r\n", buf2);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);
}

void SkillIdentify(int/* level*/, CharData *ch, CharData *victim, ObjData *obj) {
	if (obj) {
		MortShowObjValues(obj, ch, CalcCurrentSkill(ch, ESkill::kIdentify, nullptr));
		TrainSkill(ch, ESkill::kIdentify, true, nullptr);
	} else if (victim) {
		if (GetRealLevel(victim) < 3) {
			SendMsgToChar("Вы можете опознать только персонажа, достигнувшего третьего уровня.\r\n", ch);
			return;
		}
		MortShowCharValues(victim, ch, CalcCurrentSkill(ch, ESkill::kIdentify, victim));
		TrainSkill(ch, ESkill::kIdentify, true, victim);
	}
}


std::map<int /* vnum */, int /* count */> rune_list;

void AddRuneStats(CharData *ch, int vnum, int spelltype) {
	if (ch->IsNpc() || ESpellType::kRunes != spelltype) {
		return;
	}
	std::map<int, int>::iterator i = rune_list.find(vnum);
	if (rune_list.end() != i) {
		i->second += 1;
	} else {
		rune_list[vnum] = 1;
	}
}

void extract_item(CharData *ch, ObjData *obj, int spelltype) {
	int extract = false;
	if (!obj) {
		return;
	}

	obj->set_val(3, time(nullptr));

	if (IS_SET(obj->get_spec_param(), kItemCheckUses)) {
		obj->dec_val(2);
		if (GET_OBJ_VAL(obj, 2) <= 0
			&& IS_SET(obj->get_spec_param(), kItemDecayEmpty)) {
			extract = true;
		}
	} else if (spelltype != ESpellType::kRunes) {
		extract = true;
	}

	if (extract) {
		if (spelltype == ESpellType::kRunes) {
			snprintf(buf, kMaxStringLength, "$o%s рассыпал$U у вас в руках.",
					 char_get_custom_label(obj, ch).c_str());
			act(buf, false, ch, obj, nullptr, kToChar);
		}
		RemoveObjFromChar(obj);
		ExtractObjFromWorld(obj);
	}
}

int CheckRecipeValues(CharData *ch, ESpell spell_id, ESpellType spell_type, int showrecipe) {
	int item0 = -1, item1 = -1, item2 = -1, obj_num = -1;
	struct SpellCreateItem *items;

	if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
		return (false);
	}
	if (!spell_create.contains(spell_id)) {
		SendMsgToChar("Вам не доступно это заклинание.\r\n", ch);
		return (false);
	}
	if (spell_type == ESpellType::kItemCast) {
		items = &spell_create[spell_id].items;
	} else if (spell_type == ESpellType::kPotionCast) {
		items = &spell_create[spell_id].potion;
	} else if (spell_type == ESpellType::kWandCast) {
		items = &spell_create[spell_id].wand;
	} else if (spell_type == ESpellType::kScrollCast) {
		items = &spell_create[spell_id].scroll;
	} else if (spell_type == ESpellType::kRunes) {
		items = &spell_create[spell_id].runes;
	} else
		return (false);

	if (((obj_num = GetObjRnum(items->rnumber)) < 0 &&
		spell_type != ESpellType::kItemCast && spell_type != ESpellType::kRunes) ||
		((item0 = GetObjRnum(items->items[0])) +
			(item1 = GetObjRnum(items->items[1])) + (item2 = GetObjRnum(items->items[2])) < -2)) {
		if (showrecipe)
			SendMsgToChar("Боги хранят в секрете этот рецепт.\n\r", ch);
		return (false);
	}

	if (!showrecipe)
		return (true);
	else {
		strcpy(buf, "Вам потребуется :\r\n");
		if (item0 >= 0) {
			strcat(buf, kColorBoldRed);
			strcat(buf, obj_proto[item0]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}
		if (item1 >= 0) {
			strcat(buf, kColorBoldYel);
			strcat(buf, obj_proto[item1]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}
		if (item2 >= 0) {
			strcat(buf, kColorBoldGrn);
			strcat(buf, obj_proto[item2]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}
		if (obj_num >= 0 && (spell_type == ESpellType::kItemCast || spell_type == ESpellType::kRunes)) {
			strcat(buf, kColorBoldBlu);
			strcat(buf, obj_proto[obj_num]->get_PName(ECase::kNom).c_str());
			strcat(buf, "\r\n");
		}

		strcat(buf, kColorNrm);
		if (spell_type == ESpellType::kItemCast || spell_type == ESpellType::kRunes) {
			strcat(buf, "для создания магии '");
			strcat(buf, MUD::Spell(spell_id).GetCName());
			strcat(buf, "'.");
		} else {
			strcat(buf, "для создания ");
			strcat(buf, obj_proto[obj_num]->get_PName(ECase::kGen).c_str());
		}
		act(buf, false, ch, nullptr, nullptr, kToChar);
	}

	return (true);
}

/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
bool mag_item_ok(CharData *ch, ObjData *obj, int spelltype) {
	int num = 0;

	if (spelltype == ESpellType::kRunes
		&& obj->get_type() != EObjType::kMagicIngredient) {
		return false;
	}

	if (obj->get_type() == EObjType::kMagicIngredient) {
		if ((!IS_SET(obj->get_spec_param(), kItemRunes) && spelltype == ESpellType::kRunes)
			|| (IS_SET(obj->get_spec_param(), kItemRunes) && spelltype != ESpellType::kRunes)) {
			return false;
		}
	}

	if (IS_SET(obj->get_spec_param(), kItemCheckUses)
		&& GET_OBJ_VAL(obj, 2) <= 0) {
		return false;
	}

	if (IS_SET(obj->get_spec_param(), kItemCheckLag)) {
		num = 0;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag1S))
			num += 1;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag2S))
			num += 2;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag4S))
			num += 4;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag8S))
			num += 8;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag16S))
			num += 16;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag32S))
			num += 32;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag64S))
			num += 64;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLag128S))
			num += 128;
		if (GET_OBJ_VAL(obj, 3) + num - 5 * GetRealRemort(ch) >= time(nullptr))
			return false;
	}

	if (IS_SET(obj->get_spec_param(), kItemCheckLevel)) {
		num = 0;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel1))
			num += 1;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel2))
			num += 2;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel4))
			num += 4;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel8))
			num += 8;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel16))
			num += 16;
		if (IS_SET(GET_OBJ_VAL(obj, 0), kMiLevel32))
			num += 32;
		if (GetRealLevel(ch) + GetRealRemort(ch) < num)
			return false;
	}

	return true;
}

int CheckRecipeItems(CharData *ch, ESpell spell_id, ESpellType spell_type, int extract, CharData *tch) {
	ObjData *obj0 = nullptr, *obj1 = nullptr, *obj2 = nullptr, *obj3 = nullptr, *objo = nullptr;
	int item0 = -1, item1 = -1, item2 = -1, item3 = -1;
	int create = 0, obj_num = -1, percent = 0, num = 0;
	auto skill_id{ESkill::kUndefined};
	struct SpellCreateItem *items;

	if (spell_id <= ESpell::kUndefined) {
		return false;
	}
	if (!spell_create.contains(spell_id))
		return false;
	if (spell_type == ESpellType::kItemCast) {
		items = &spell_create[spell_id].items;
	} else if (spell_type == ESpellType::kPotionCast) {
		items = &spell_create[spell_id].potion;
		skill_id = ESkill::kCreatePotion;
		create = 1;
	} else if (spell_type == ESpellType::kWandCast) {
		items = &spell_create[spell_id].wand;
		skill_id = ESkill::kCreateWand;
		create = 1;
	} else if (spell_type == ESpellType::kScrollCast) {
		items = &spell_create[spell_id].scroll;
		skill_id = ESkill::kCreateScroll;
		create = 1;
	} else if (spell_type == ESpellType::kRunes) {
		items = &spell_create[spell_id].runes;
	} else {
		return (false);
	}
	item3 = items->rnumber;
	item0 = items->items[0];
	item1 = items->items[1];
	item2 = items->items[2];
	const int item0_rnum = item0 >= 0 ? GetObjRnum(item0) : -1;
	const int item1_rnum = item1 >= 0 ? GetObjRnum(item1) : -1;
	const int item2_rnum = item2 >= 0 ? GetObjRnum(item2) : -1;
	const int item3_rnum = item3 >= 0 ? GetObjRnum(item3) : -1;

	for (auto obj = ch->carrying; obj; obj = obj->get_next_content()) {
		if (item0 >= 0 && item0_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item0_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr)); //в вал3 сохраним время когда мы заюзали руну
			obj0 = obj;
			item0 = -2;
			objo = obj0;
			num++;
		} else if (item1 >= 0 && item1_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item1_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr));
			obj1 = obj;
			item1 = -2;
			objo = obj1;
			num++;
		} else if (item2 >= 0 && item2_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item2_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr));
			obj2 = obj;
			item2 = -2;
			objo = obj2;
			num++;
		} else if (item3 >= 0 && item3_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item3_rnum], 1)
			&& mag_item_ok(ch, obj, spell_type)) {
			obj->set_val(3, time(nullptr));
			obj3 = obj;
			item3 = -2;
			objo = obj3;
			num++;
		}
	}

	if (!objo ||
		(items->items[0] >= 0 && item0 >= 0) ||
		(items->items[1] >= 0 && item1 >= 0) ||
		(items->items[2] >= 0 && item2 >= 0) || (items->rnumber >= 0 && item3 >= 0)) {
		return (false);
	}

	if (extract) {
		if (spell_type == ESpellType::kRunes) {
			strcpy(buf, "Вы сложили ");
		} else {
			strcpy(buf, "Вы взяли ");
		}

		ObjData::shared_ptr obj;
		if (create) {
			obj = world_objects.create_from_prototype_by_vnum(obj_num);
			if (!obj) {
				return false;
			} else {
				percent = number(1, MUD::Skill(skill_id).difficulty);
				auto prob = CalcCurrentSkill(ch, skill_id, nullptr);

				if (MUD::Skills().IsValid(skill_id) && percent > prob) {
					percent = -1;
				}
			}
		}

		if (item0 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj0->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj0, 1), spell_type);
		}

		if (item1 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj1->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj1, 1), spell_type);
		}

		if (item2 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj2->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj2, 1), spell_type);
		}

		if (item3 == -2) {
			strcat(buf, kColorWht);
			strcat(buf, obj3->get_PName(ECase::kAcc).c_str());
			strcat(buf, ", ");
			AddRuneStats(ch, GET_OBJ_VAL(obj3, 1), spell_type);
		}

		strcat(buf, kColorNrm);

		if (create) {
			if (percent >= 0) {
				strcat(buf, " и создали $o3.");
				act(buf, false, ch, obj.get(), nullptr, kToChar);
				act("$n создал$g $o3.", false, ch, obj.get(), nullptr, kToRoom | kToArenaListen);
				PlaceObjToInventory(obj.get(), ch);
			} else {
				strcat(buf, " и попытались создать $o3.\r\n" "Ничего не вышло.");
				act(buf, false, ch, obj.get(), nullptr, kToChar);
				ExtractObjFromWorld(obj.get());
			}
		} else {
			if (spell_type == ESpellType::kItemCast) {
				strcat(buf, "и создали магическую смесь.\r\n");
				act(buf, false, ch, nullptr, nullptr, kToChar);
				act("$n смешал$g что-то в своей ноше.\r\n"
					"Вы почувствовали резкий запах.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			} else if (spell_type == ESpellType::kRunes) {
				sprintf(buf + strlen(buf),
						"котор%s вспыхнул%s ярким светом.%s",
						num > 1 ? "ые" : GET_OBJ_SUF_3(objo), num > 1 ? "и" : GET_OBJ_SUF_1(objo),
						ch->IsFlagged(EPrf::kCompact) ? "" : "\r\n");
				act(buf, false, ch, nullptr, nullptr, kToChar);
				act("$n сложил$g руны, которые вспыхнули ярким пламенем.",
					true, ch, nullptr, nullptr, kToRoom);
				sprintf(buf, "$n сложил$g руны в заклинание '%s'%s%s.",
						MUD::Spell(spell_id).GetCName(),
						(tch && tch != ch ? " на " : ""),
						(tch && tch != ch ? GET_PAD(tch, 1) : ""));
				act(buf, true, ch, nullptr, nullptr, kToArenaListen);
				auto magic_skill = GetMagicSkillId(spell_id);
				if (MUD::Skills().IsValid(magic_skill)) {
					TrainSkill(ch, magic_skill, true, tch);
				}
			}
		}
		extract_item(ch, obj0, spell_type);
		extract_item(ch, obj1, spell_type);
		extract_item(ch, obj2, spell_type);
		extract_item(ch, obj3, spell_type);
	}
	return (true);
}

void print_rune_stats(CharData *ch) {
	if (!ch->IsGrGod()) {
		SendMsgToChar(ch, "Только для иммов 33+.\r\n");
		return;
	}

	std::multimap<int, int> tmp_list;
	for (std::map<int, int>::const_iterator i = rune_list.begin(),
			 iend = rune_list.end(); i != iend; ++i) {
		tmp_list.insert(std::make_pair(i->second, i->first));
	}
	std::stringstream out;
	out << "Rune stats:\r\n" << "vnum -> count\r\n" << "--------------\r\n";

	for (std::multimap<int, int>::const_reverse_iterator i = tmp_list.rbegin(),
			 iend = tmp_list.rend(); i != iend; ++i) {
		
		out << i->second << " -> " << i->first << "\r\n";
	}
	SendMsgToChar(out.str(), ch);
}

void print_rune_log() {
	for (std::map<int, int>::const_iterator i = rune_list.begin(),
			 iend = rune_list.end(); i != iend; ++i) {
		log("RuneUsed: %d %d", i->first, i->second);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
