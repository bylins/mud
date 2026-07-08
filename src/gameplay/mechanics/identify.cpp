/**
 \file identify.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: identify-display mechanic (extracted from spells.cpp).
*/

#include "gameplay/mechanics/identify.h"
#include "administration/privilege.h"

#include "gameplay/mechanics/magic_item.h"
#include "gameplay/magic/spells.h"
#include "gameplay/magic/magic_utils.h"
#include "engine/db/global_objects.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/db/obj_prototypes.h"
#include "engine/ui/color.h"
#include "engine/ui/cmd_god/do_stat.h"
#include "gameplay/mechanics/stuff.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/dungeons.h"
#include "engine/entities/obj_data.h"
#include "gameplay/core/remort.h"
#include "gameplay/fight/fight_hit.h"
#include "utils/grammar/declensions.h"

#include <cmath>

// Defined in sight.cpp / spells.cpp (no header); forward-declared as the trading code does.
int CalcBaseAc(CharData *ch);
int CalcHitroll(CharData *ch);

/* It's not used yet, so I commented it out.
static void ShowWeapon(CharData *ch, ObjData *obj) {
	if (obj->get_type() == EObjType::kWeapon) {
		*buf = '\0';
		if (CAN_WEAR(obj, EWearFlag::kWield)) {
			sprintf(buf, "Можно взять %s в правую руку.\r\n", OBJN(obj, ch, grammar::ECase::kAcc));
		}

		if (CAN_WEAR(obj, EWearFlag::kHold)) {
			sprintf(buf + strlen(buf), "Можно взять %s в левую руку.\r\n", OBJN(obj, ch, grammar::ECase::kAcc));
		}

		if (CAN_WEAR(obj, EWearFlag::kBoth)) {
			sprintf(buf + strlen(buf), "Можно взять %s в обе руки.\r\n", OBJN(obj, ch, grammar::ECase::kAcc));
		}

		if (*buf) {
			SendMsgToChar(buf, ch);
		}
	}
}
*/
static void PrintBookUpgradeSkill(CharData *ch, const ObjData *obj) {
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

static void ShowObjTypeSpecificValues(const ObjData *obj, CharData *ch) {
	int i, j, drndice = 0, drsdice = 0;
	long int li;
	(void) i; (void) j; (void) li;  // some branches do not touch all of them
switch (obj->get_type()) {
	case EObjType::kScroll: {
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
	// issue.potion-hotfix: a potion reads its spells from the ObjVal keys and shows its maker-derived
	// POTENCY (Сила), never a per-spell level -- the drinker's own skill/stats are irrelevant.
	case EObjType::kPotion: {
		std::ostringstream out;
		out << "Содержит заклинание:";
		const ObjVal::EValueKey spell_keys[3] = {
			ObjVal::EValueKey::kPotionSpell1Num,
			ObjVal::EValueKey::kPotionSpell2Num,
			ObjVal::EValueKey::kPotionSpell3Num};
		for (const auto key : spell_keys) {
			const auto spell_id = static_cast<ESpell>(obj->GetPotionValueKey(key));
			if (MUD::Spell(spell_id).IsValid()) {
				const int potency = static_cast<int>(PotionPotency(obj, spell_id) + 0.5f);
				out << " Сила [" << potency << "] " << MUD::Spell(spell_id).GetName() << ",";
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
					// issue.class-recipes: требования к рецепту берём у класса игрока.
					{
						const auto *req = MUD::Class(ch->GetClass()).FindIngredientRecipe(imrecipes[drndice].str_id);
						sprintf(buf, "содержит рецепт отвара     : \"%s\"\r\n", imrecipes[drndice].name);
						SendMsgToChar(buf, ch);
						if (!req) {
							sprintf(buf, "уровень изучения (количество ремортов) : %d (--)\r\n", kLvlImplementator);
							SendMsgToChar(buf, ch);
						} else {
							drsdice = std::max(GET_OBJ_VAL(obj, 2), req->level);
							sprintf(buf, "уровень изучения (количество ремортов) : %d (%d)\r\n", drsdice, req->remort);
							SendMsgToChar(buf, ch);
						}
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
					kColorBoldCyn, obj_proto[i]->get_PName(grammar::ECase::kNom).c_str(), kColorNrm);
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

	strcpy(buf, sight::diag_weapon_to_char(obj, 2));
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
	obj->get_no_flags().sprintbits(no_bits, buf, sizeof(buf), ",", privilege::IsImmortal(ch) ? 4 : 0);
	strncat(buf, "\r\n", sizeof(buf) - strlen(buf) - 1);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);

	if (fullness < 50)
		return;

	SendMsgToChar("Недоступен : ", ch);
	SendMsgToChar(kColorCyn, ch);
	obj->get_anti_flags().sprintbits(anti_bits, buf, sizeof(buf), ",", privilege::IsImmortal(ch) ? 4 : 0);
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
	obj->get_extra_flags().sprintbits(extra_bits, buf, sizeof(buf), ",", privilege::IsImmortal(ch) ? 4 : 0);
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
	obj->get_affect_flags().sprintbits(weapon_affects, buf, sizeof(buf), ",", privilege::IsImmortal(ch) ? 4 : 0);
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

void MobShowValues(CharData *ch, CharData *victim, int skill) {
	std::stringstream ss;

	if (skill > 1) {
		ss << fmt::format("Имя: {}\r\n", utils::CAP(victim->get_name()));
	}
	if (skill > 29) {
		ss << fmt::format("Размер: {}({}), вес {}({})\r\n", GET_SIZE(victim), GET_REAL_SIZE(victim), GET_WEIGHT(victim), GET_REAL_WEIGHT(victim));
	}
	if (skill > 49) {
		ss << fmt::format("Уровень {}, перевоплощений {}, может выдержать {}({}) {} повреждений\r\n", GetRealLevel(victim), remort::GetRealRemort(victim), victim->get_hit(), victim->get_real_max_hit(),
			grammar::GetDeclensionInNumber(victim->get_hit(), grammar::EWhat::kOneU));
	}
	if (skill > 59) {
		int ac = CalcBaseAc(victim) / 10;
		if (ac < 5) {
			const int mod = (1 - condition::GetCondPenalty(victim, condition::kAc)) * 40;
			ac = ac + mod > 5 ? 5 : ac + mod;
		}
		ss << fmt::format("Броня: {}, защита: {}, поглощение {}\r\n", GET_ARMOUR(victim), ac, GET_ABSORBE(victim));
	}
	if (skill > 79) {
		HitData hit_params;
		hit_params.weapon = fight::kMainHand;
		hit_params.skill_num = ESkill::kAny; 
		hit_params.Init(victim, victim);
		ss << fmt::format("Попадание: {}, повреждение: {}, успех колдовства: {}, удача: {}, инициатива: {}\r\n",
				GET_REAL_HR(victim) + str_bonus(GetRealStr(victim), STR_TO_HIT), hit_params.CalcDmg(victim, false), CalcAntiSavings(victim), victim->calc_morale(), calc_initiative(victim, false));
	}
	if (skill > 99) {
	ss << fmt::format("Cила: {}({}), ловкость: {}({}), телосложение: {}({}), ум: {}({}), мудрость: {}({}), обаяние: {}({})\r\n",
			victim->get_str(), GetRealStr(victim), victim->get_dex(), GetRealDex(victim), victim->get_con(), GetRealCon(victim),  victim->get_int(), GetRealInt(victim),
			victim->get_wis(), GetRealWis(victim), victim->get_cha(), GetRealCha(victim));
	}
	if (skill > 119) {
		ss << fmt::format("Бонусы: маг.урон: {}, физ. урон: {}\r\n",
				victim->add_abils.percent_spellpower_add, victim->add_abils.percent_physdam_add);
	}
	if (skill > 149) {
		ss << fmt::format("Защита от чар : {}, Защита от магических повреждений : {}, Защита от физических повреждений : {}\r\n",
				std::min(GET_AR(victim), 100), std::min(GET_MR(victim), 100), std::min(GET_PR(victim), 100));
	}
	if (skill > 179) {
	ss << fmt::format("Сопротивление: огню: {}, воздуху: {}, воде: {}, земле: {}, тьме: {}, тяжелым ранам: {}, разум: {}, ядам и болезням: {}\r\n",
			std::min(GET_RESIST(victim, EResist::kFire), 75), std::min(GET_RESIST(victim, EResist::kAir), 75),
			std::min(GET_RESIST(victim, EResist::kWater), 75), std::min(GET_RESIST(victim, EResist::kEarth), 75),
			std::min(GET_RESIST(victim, EResist::kDark), 75), std::min(GET_RESIST(victim, EResist::kVitality), 75),
			std::min(GET_RESIST(victim, EResist::kMind), 75), std::min(GET_RESIST(victim, EResist::kImmunity), 75));
	}
	if (skill > 199) {
		ss << fmt::format("Спас броски: воля: {}, здоровье: {}, стойкость: {}, реакция: {}\r\n",
				-CalcSaving(victim, victim, ESaving::kWill, false),
				-CalcSaving(victim, victim, ESaving::kCritical, false),
				-CalcSaving(victim, victim, ESaving::kStability, false),
				-CalcSaving(victim, victim, ESaving::kReflex, false));
	}
	if (skill > 249) {
		// Разделитель ", " (не " "!): имена аффектов сами содержат пробелы
		// ("воздушный щит"), и при " " их рвал бы OutWordsList по пробелам.
		// Split по запятой собирает их обратно целиком (имена с пробелами сохраняются).
		victim->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, sizeof(buf2), ", ");
		std::vector<std::string> aff_list = utils::Split(buf2, ',');
		// "Аффекты: " префиксом: учитывается в ширине строки, но без лишней запятой после метки.
		ss << fmt::format("&G{}&n\r\n",
				utils::OutWordsList(aff_list, ch->player_specials->saved.stringLength, ", ", "Аффекты: "));
	}
	SendMsgToChar(ch, "%s", ss.str().c_str());
}

void MortShowCharValues(CharData *victim, CharData *ch, int fullness) {
	int val0, val1, val2;

	if (victim->IsNpc()) {
		MobShowValues(ch, victim, fullness);
		return;
	}
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
	SendMsgToChar(ch, "Перевоплощений : %d\r\n", remort::GetRealRemort(victim));
	val0 = std::min(GET_AR(victim), 100);
	val1 = std::min(GET_MR(victim), 100);
	val2 = std::min(GET_PR(victim), 100);
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
	victim->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, sizeof(buf2), "\r\n", privilege::IsImmortal(ch) ? 4 : 0);
	snprintf(buf, kMaxStringLength, "%s\r\n", buf2);
	SendMsgToChar(buf, ch);
	SendMsgToChar(kColorNrm, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
