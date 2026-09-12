/**
 \file identify.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: identify-display mechanic (extracted from spells.cpp).
*/

#include "gameplay/mechanics/identify.h"
#include "gameplay/affects/affect_messages.h"
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
#include "gameplay/mechanics/initiative.h"
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
		std::string line;
		if (CAN_WEAR(obj, EWearFlag::kWield)) {
			line = fmt::format("Можно взять {} в правую руку.\r\n", OBJN(obj, ch, grammar::ECase::kAcc));
		}

		if (CAN_WEAR(obj, EWearFlag::kHold)) {
			line += fmt::format("Можно взять {} в левую руку.\r\n", OBJN(obj, ch, grammar::ECase::kAcc));
		}

		if (CAN_WEAR(obj, EWearFlag::kBoth)) {
			line += fmt::format("Можно взять {} в обе руки.\r\n", OBJN(obj, ch, grammar::ECase::kAcc));
		}

		if (!line.empty()) {
			SendMsgToChar(line, ch);
		}
	}
}
*/
// "Недоступно" здесь -- это про класс, а не про уровень: класс либо получает талант когда-нибудь,
// либо не получает никогда. Нехватка уровня или ремортов временна, и молчать о ней честнее, чем
// пугать игрока словом "недоступно" (#3877).
static const char *UnavailableTail(bool unavailable) {
	return unavailable ? " (вам недоступно)" : "";
}

std::string GetBookContents(const CObjectPrototype *obj, CharData *ch) {
	if (!obj || obj->get_type() != EObjType::kBook) {
		return "";
	}
	switch (GET_OBJ_VAL(obj, 0)) {
		case EBook::kSpell: {
			const auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, 1));
			if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
				return "";
			}
			const bool unavailable = ch && MUD::Class(ch->GetClass()).spells.IsUnavailable(spell_id);
			return fmt::format("содержит заклинание        : \"{}\"{}",
							   MUD::Spell(spell_id).GetName(), UnavailableTail(unavailable));
		}
		case EBook::kSkill: {
			const auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(obj, 1));
			if (MUD::Skills().IsInvalid(skill_id)) {
				return "";
			}
			const bool unavailable = ch && MUD::Class(ch->GetClass()).skills.IsUnavailable(skill_id);
			return fmt::format("содержит секрет умения     : \"{}\"{}",
							   MUD::Skill(skill_id).GetName(), UnavailableTail(unavailable));
		}
		case EBook::kSkillUpgrade: {
			const auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(obj, 1));
			if (MUD::Skills().IsInvalid(skill_id)) {
				return "";
			}
			const bool unavailable = ch && MUD::Class(ch->GetClass()).skills.IsUnavailable(skill_id);
			if (GET_OBJ_VAL(obj, 3) > 0) {
				return fmt::format("повышает умение            : \"{}\" (максимум {}){}",
								   MUD::Skill(skill_id).GetName(), GET_OBJ_VAL(obj, 3),
								   UnavailableTail(unavailable));
			}
			return fmt::format("повышает умение            : \"{}\" (не больше максимума текущего перевоплощения){}",
							   MUD::Skill(skill_id).GetName(), UnavailableTail(unavailable));
		}
		case EBook::kReceipt: {
			const int recipe = im_get_recipe(GET_OBJ_VAL(obj, 1));
			if (recipe < 0) {
				return "";
			}
			const bool unavailable =
				ch && !MUD::Class(ch->GetClass()).FindIngredientRecipe(imrecipes[recipe].str_id);
			return fmt::format("содержит рецепт отвара     : \"{}\"{}",
							   imrecipes[recipe].name, UnavailableTail(unavailable));
		}
		case EBook::kFeat: {
			const auto feat_id = static_cast<EFeat>(GET_OBJ_VAL(obj, 1));
			if (!MUD::Feat(feat_id).IsValid()) {
				return "";
			}
			// У способностей доступность даёт не только класс, но и раса -- как в CanGetFeat.
			const bool unavailable = ch
				&& MUD::Class(ch->GetClass()).feats.IsUnavailable(feat_id)
				&& !MUD::PcRaces()[GET_RACE(ch)].HasFeature(feat_id);
			return fmt::format("содержит секрет способности: \"{}\"{}",
							   MUD::Feat(feat_id).GetName(), UnavailableTail(unavailable));
		}
		default: return "";
	}
}

// Порог изучения. Прочерк вместо числа -- когда класс этого таланта не получает вовсе:
// раньше сюда писали kLvlImplementator, и игрок читал "уровень изучения (для вас) : 34",
// где 34 означало не порог, а "никогда" (#3877).
std::string GetBookLearnLevel(const CObjectPrototype *obj, CharData *ch) {
	if (!obj || !ch || obj->get_type() != EObjType::kBook) {
		return "";
	}
	static const char *kNever = "--";
	switch (GET_OBJ_VAL(obj, 0)) {
		case EBook::kSpell: {
			const auto spell_id = static_cast<ESpell>(GET_OBJ_VAL(obj, 1));
			if (spell_id < ESpell::kFirst || spell_id > ESpell::kLast) {
				return "";
			}
			if (MUD::Class(ch->GetClass()).spells.IsUnavailable(spell_id)) {
				return fmt::format("уровень изучения (для вас) : {}", kNever);
			}
			return fmt::format("уровень изучения (для вас) : {}",
							   CalcMinSpellLvl(ch, spell_id, GET_OBJ_VAL(obj, 2)));
		}
		case EBook::kSkill:
		case EBook::kSkillUpgrade: {
			const auto skill_id = static_cast<ESkill>(GET_OBJ_VAL(obj, 1));
			if (MUD::Skills().IsInvalid(skill_id)) {
				return "";
			}
			if (MUD::Class(ch->GetClass()).skills.IsUnavailable(skill_id)) {
				return fmt::format("уровень изучения (для вас) : {}", kNever);
			}
			return fmt::format("уровень изучения (для вас) : {}",
							   GetSkillMinLevel(ch, skill_id, GET_OBJ_VAL(obj, 2)));
		}
		case EBook::kReceipt: {
			const int recipe = im_get_recipe(GET_OBJ_VAL(obj, 1));
			if (recipe < 0) {
				return "";
			}
			// issue.class-recipes: требования к рецепту берём у класса игрока.
			const auto *req = MUD::Class(ch->GetClass()).FindIngredientRecipe(imrecipes[recipe].str_id);
			if (!req) {
				return fmt::format("уровень изучения (количество ремортов) : {} (--)", kNever);
			}
			return fmt::format("уровень изучения (количество ремортов) : {} ({})",
							   std::max(GET_OBJ_VAL(obj, 2), req->level), req->remort);
		}
		case EBook::kFeat: {
			const auto feat_id = static_cast<EFeat>(GET_OBJ_VAL(obj, 1));
			if (!MUD::Feat(feat_id).IsValid()) {
				return "";
			}
			if (!CanGetFeat(ch, feat_id)) {
				return fmt::format("уровень изучения (для вас) : {}", kNever);
			}
			return fmt::format("уровень изучения (для вас) : {}",
							   MUD::Class(ch->GetClass()).feats[feat_id].GetSlot());
		}
		default: return "";
	}
}

static void ShowObjTypeSpecificValues(const ObjData *obj, CharData *ch) {
	std::string line;
	int i, j, drndice = 0, drsdice = 0;
	long int li;
	(void) i; (void) j; (void) li;  // some branches do not touch all of them
switch (obj->get_type()) {
	case EObjType::kScroll: {
		SendMsgToChar(utils::OutWordsList(SpellItemSpellsWithPotency(obj),
				ch->player_specials->saved.stringLength, ", ",
				std::string(kColorGrn) + "Содержит заклинание:" + kColorNrm + " ") + "\r\n", ch);
		break;
	}
		// issue.potion-hotfix: a potion reads its spells from the ObjVal keys and shows its maker-derived
	// POTENCY (Сила), never a per-spell level -- the drinker's own skill/stats are irrelevant.
	case EObjType::kPotion: {
		SendMsgToChar(utils::OutWordsList(SpellItemSpellsWithPotency(obj),
				ch->player_specials->saved.stringLength, ", ",
				std::string(kColorGrn) + "Содержит заклинание:" + kColorNrm + " ") + "\r\n", ch);
		break;
	}
	case EObjType::kWand:
	case EObjType::kStaff: line = fmt::format("Вызывает заклинания: ");
		line += fmt::format(" {}\r\n", MUD::Spell(static_cast<ESpell>(obj->GetPotionValueKey(ObjVal::EValueKey::kSpell1Num))).GetCName());
		line += fmt::format("Зарядов {} (осталось {}).\r\n", obj->GetPotionValueKey(ObjVal::EValueKey::kMaxCharges), obj->GetPotionValueKey(ObjVal::EValueKey::kCurCharges));
		SendMsgToChar(line, ch);
		break;

	case EObjType::kWeapon: drndice = GET_OBJ_VAL(obj, 1);
		drsdice = GET_OBJ_VAL(obj, 2);
		line = fmt::format("Наносимые повреждения '{}D{}'", drndice, drsdice);
		line += fmt::format(" среднее {:.1f}.\r\n", ((drsdice + 1) * drndice / 2.0));
		SendMsgToChar(line, ch);
		break;

	case EObjType::kArmor:
	case EObjType::kLightArmor:
	case EObjType::kMediumArmor:
	case EObjType::kHeavyArmor: drndice = GET_OBJ_VAL(obj, 0);
		drsdice = GET_OBJ_VAL(obj, 1);
		SendMsgToChar(fmt::format("защита (AC) : {}\r\n", drndice), ch);
		SendMsgToChar(fmt::format("броня       : {}\r\n", drsdice), ch);
		break;

	case EObjType::kBook: {
		// Тексты и пороги -- в GetBookContents/GetBookLearnLevel, общих с осмотром (#3877):
		// раньше те же шесть строк лежали здесь вторым экземпляром и успели разъехаться
		// (лишний пробел перед двоеточием у способностей, GetCName вместо GetName).
		const std::string contents = GetBookContents(obj, ch);
		if (contents.empty()) {
			// Пустая строка значит одно из двух: тип книги вне списка либо содержимое битое
			// (заклинание с номером 0, рецепт, которого нет в таблице). Раньше про первое
			// говорили игроку, а про второе молча писали в syslog -- и то, и другое нужно.
			const int book_type = GET_OBJ_VAL(obj, 0);
			const bool known_type = book_type >= EBook::kSpell && book_type <= EBook::kFeat;
			log("SYSERR: broken book #%d: type=%d val1=%d", GET_OBJ_VNUM(obj), book_type, GET_OBJ_VAL(obj, 1));
			SendMsgToChar(kColorBoldRed, ch);
			SendMsgToChar(known_type ? "СОДЕРЖИМОЕ КНИГИ НЕ ОПРЕДЕЛЕНО - сообщите Богам\r\n"
									 : "НЕВЕРНО УКАЗАН ТИП КНИГИ - сообщите Богам\r\n", ch);
			SendMsgToChar(kColorNrm, ch);
			break;
		}
		SendMsgToChar(contents + "\r\n", ch);
		const std::string learn_level = GetBookLearnLevel(obj, ch);
		if (!learn_level.empty()) {
			SendMsgToChar(learn_level + "\r\n", ch);
		}
		break;
	}

	case EObjType::kMagicIngredient:
		SendMsgToChar(sprintbit(obj->get_spec_param(), ingradient_bits) + "\r\n", ch);

		if (IS_SET(obj->get_spec_param(), kItemCheckUses)) {
			SendMsgToChar(fmt::format("можно применить {} раз\r\n", GET_OBJ_VAL(obj, 2)), ch);
		}

		if (IS_SET(obj->get_spec_param(), kItemCheckLag)) {
			line = fmt::format("можно применить 1 раз в {} сек", (i = GET_OBJ_VAL(obj, 0) & 0xFF));
			if (GET_OBJ_VAL(obj, 3) == 0 || GET_OBJ_VAL(obj, 3) + i < time(nullptr)) {
				line += "(можно применять).\r\n";
			} else {
				li = GET_OBJ_VAL(obj, 3) + i - time(nullptr);
				line += fmt::format("(осталось {} сек).\r\n", li);
			}
			SendMsgToChar(line, ch);
		}

		if (IS_SET(obj->get_spec_param(), kItemCheckLevel)) {
			SendMsgToChar(fmt::format("можно применить с {} уровня.\r\n", (GET_OBJ_VAL(obj, 0) >> 8) & 0x1F), ch);
		}

		if ((i = GetObjRnum(GET_OBJ_VAL(obj, 1))) >= 0) {
			SendMsgToChar(fmt::format("прототип {}{}{}.\r\n", kColorBoldCyn, obj_proto[i]->get_PName(grammar::ECase::kNom), kColorNrm), ch);
		}
		break;

	case EObjType::kMagicComponent:
		for (j = 0; imtypes[j].id != GET_OBJ_VAL(obj, IM_TYPE_SLOT) && j <= top_imtypes;) {
			j++;
		}
		SendMsgToChar(fmt::format("Это ингредиент вида '{}{}{}'\r\n", kColorCyn, imtypes[j].name, kColorNrm), ch);
		i = GET_OBJ_VAL(obj, IM_POWER_SLOT);
		if (i > 45) { // тут явно опечатка была, кроме того у нас мобы и выше 40лвл
			SendMsgToChar("Вы не в состоянии определить качество этого ингредиента.\r\n", ch);
		} else {
			// пороги и фразы -- одной таблицей вместо девяти веток со strcat
			static const std::pair<int, const char *> kQuality[] = {
				{40, "божественное.\r\n"},   {35, "идеальное.\r\n"},
				{30, "наилучшее.\r\n"},      {25, "превосходное.\r\n"},
				{20, "отличное.\r\n"},       {15, "очень хорошее.\r\n"},
				{10, "выше среднего.\r\n"},  {5, "весьма посредственное.\r\n"},
			};
			line = "Качество ингредиента ";
			const char *quality = "хуже не бывает.\r\n";
			for (const auto &[threshold, text] : kQuality) {
				if (i > threshold) {
					quality = text;
					break;
				}
			}
			line += quality;
			SendMsgToChar(line, ch);
		}
		break;

		//Информация о контейнерах (Купала)
	case EObjType::kContainer: line = fmt::format("Максимально вместимый вес: {}.\r\n", GET_OBJ_VAL(obj, 0));
		SendMsgToChar(line, ch);
		break;

		//Информация о емкостях (Купала)
	case EObjType::kLiquidContainer: drinkcon::identify(ch, obj);
		break;

	case EObjType::kMagicArrow:
	case EObjType::kMagicContaner: line = fmt::format("Может вместить стрел: {}.\r\n", GET_OBJ_VAL(obj, 1));
		SendMsgToChar(fmt::format("Осталось стрел: {}{}&n.\r\n", GET_OBJ_VAL(obj, 2) > 3 ? "&G" : "&R", GET_OBJ_VAL(obj, 2)), ch);
		break;

	default: break;
} // switch
}

void MortShowObjValues(const ObjData *obj, CharData *ch, int fullness) {
	std::string line;
	int i;
	bool found;
	bool enhansed_scroll = false;
	
	if (fullness > 399) {
		enhansed_scroll = true;
	}
	SendMsgToChar("Вы узнали следующее:\r\n", ch);
	line = fmt::format("Предмет \"{}\", тип : ", obj->get_short_description());
	line += GetTypeName(obj->get_type(), item_types);
	line += "\r\n";
	SendMsgToChar(line, ch);

	line = sight::diag_weapon_to_char(obj, 2);
	if (!line.empty())
		SendMsgToChar(line, ch);

	if (fullness < 20)
		return;

	//ShowWeapon(ch, obj);

	SendMsgToChar(fmt::format("Вес: {}, Цена: {}, Рента: {}({})\r\n", obj->get_weight(), obj->get_cost(), obj->get_rent_off(), obj->get_rent_on()), ch);

	if (fullness < 30)
		return;
	line = fmt::format("Материал : {}, макс.прочность : {}, тек.прочность : {}\r\n",
					   GetTypeName(obj->get_material(), material_name),
			 obj->get_maximum_durability(), obj->get_current_durability());
	SendMsgToChar(line, ch);
	SendMsgToChar(kColorNrm, ch);

	if (fullness < 40)
		return;

	SendMsgToChar("Неудобен : ", ch);
	SendMsgToChar(kColorCyn, ch);
	line = obj->get_no_flags().sprintbits(no_bits, ",", privilege::IsImmortal(ch) ? 4 : 0) + "\r\n";
	SendMsgToChar(line, ch);
	SendMsgToChar(kColorNrm, ch);

	if (fullness < 50)
		return;

	SendMsgToChar("Недоступен : ", ch);
	SendMsgToChar(kColorCyn, ch);
	line = obj->get_anti_flags().sprintbits(anti_bits, ",", privilege::IsImmortal(ch) ? 4 : 0) + "\r\n";
	SendMsgToChar(line, ch);
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
	line = obj->get_extra_flags().sprintbits(extra_bits, ",", privilege::IsImmortal(ch) ? 4 : 0) + "\r\n";
	SendMsgToChar(line, ch);
	SendMsgToChar(kColorNrm, ch);
//enhansed_scroll = true; //для теста
	if (enhansed_scroll) {
		std::string timer_line;
		if (stable_objs::IsTimerUnlimited(obj))
			timer_line = fmt::format("Таймер: {}/нерушимо.", obj_proto[obj->get_rnum()]->get_timer());
		else
			timer_line = fmt::format("Таймер: {}/{}.", obj_proto[obj->get_rnum()]->get_timer(), obj->get_timer());
		const std::string miw = GetObjMIW(obj->get_rnum()) < 0
			? std::string("бесконечно")
			: std::to_string(GetObjMIW(obj->get_rnum()));
		line = fmt::format("&GСейчас в мире : {}. На постое : {}. Макс. в мире : {}. {}&n\r\n",
						   obj_proto.total_online(obj->get_rnum()), obj_proto.stored(obj->get_rnum()),
						   miw, timer_line);
		SendMsgToChar(line, ch);
	}
	if (fullness < 75)
		return;

	ShowObjTypeSpecificValues(obj, ch);

	if (fullness < 90) {
		return;
	}

	SendMsgToChar("Накладывает на вас аффекты: ", ch);
	SendMsgToChar(kColorCyn, ch);
	line = obj->get_affect_flags().sprintbits(equipment_affects, ",", privilege::IsImmortal(ch) ? 4 : 0) + "\r\n";
	SendMsgToChar(line, ch);
	SendMsgToChar(kColorNrm, ch);
	if (obj->has_suppressed_affects()) {
		SendMsgToChar("Временно подавлены     :\r\n", ch);
		SendMsgToChar(kColorCyn, ch);
		std::string sup;   // one affect per indented line (unreadable comma-joined with 2+)
		for (const auto &pr : obj->suppressed_equip_affects()) {
			char hbuf[96];
			snprintf(hbuf, sizeof(hbuf), "    %s (%d %s)\r\n",
					affects::AffectMsg(pr.first, affects::EAffectMsgType::kShortDesc).c_str(), pr.second,
					grammar::GetDeclensionInNumber(pr.second, grammar::EWhat::kHour));
			sup += hbuf;
		}
		SendMsgToChar(sup.c_str(), ch);
		SendMsgToChar(kColorNrm, ch);
	}

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

			SendMsgToChar(fmt::format("   {}{}{}{}{}{}%{}\r\n", kColorCyn, MUD::Skill(skill_id).GetName(), kColorNrm, kColorCyn, percent < 0 ? " ухудшает на " : " улучшает на ", abs(percent), kColorNrm), ch);
		}
	}

	auto it = ObjData::set_table.begin();
	if (obj->has_flag(EObjFlag::kSetItem)) {
		for (; it != ObjData::set_table.end(); it++) {
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end()) {
				SendMsgToChar(fmt::format("Часть набора предметов: &n{}&n\r\n",
										  it->second.get_name()), ch);
				for (auto & vnum : it->second) {
					const int r_num = GetObjRnum(vnum.first);
					if (r_num < 0) {
						SendMsgToChar("Неизвестный объект!!!\r\n", ch);
						continue;
					}
					SendMsgToChar(fmt::format("   {}\r\n", obj_proto[r_num]->get_short_description()), ch);
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
	std::string line;
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
		// issue.affect-migration: affect short names come from affects::DescribeActive (the affected_bits[]
		// projection was removed). Join with "," then Split back into a list so master's OutWordsList wraps
		// to line width WITHOUT splitting multi-word names ("воздушный щит") -- the separator is a comma,
		// which never appears inside an affect name (the names contain spaces, not commas).
		std::vector<std::string> aff_list =
				utils::Split(affects::DescribeActive(victim->char_specials.saved.affected_by, ","), ',');
		ss << fmt::format("&G{}&n\r\n",
				utils::OutWordsList(aff_list, ch->player_specials->saved.stringLength, ", ", "Аффекты: "));
	}
	SendMsgToChar(ch, "%s", ss.str().c_str());
}

void MortShowCharValues(CharData *victim, CharData *ch, int fullness) {
	std::string line;
	int val0, val1, val2;

	if (victim->IsNpc()) {
		MobShowValues(ch, victim, fullness);
		return;
	}
	SendMsgToChar(fmt::format("Имя: {}\r\n", GET_NAME(victim)), ch);
	if (!victim->IsNpc() && victim == ch) {
		SendMsgToChar(fmt::format("Написание : {}/{}/{}/{}/{}/{}\r\n", GET_PAD(victim, 0), GET_PAD(victim, 1), GET_PAD(victim, 2), GET_PAD(victim, 3), GET_PAD(victim, 4), GET_PAD(victim, 5)), ch);
	}

	if (!victim->IsNpc() && victim == ch) {
		const auto &victimAge = CalcCharAge(victim);
		SendMsgToChar(fmt::format("Возраст {}  : {} лет, {} месяцев, {} дней и {} часов.\r\n", GET_PAD(victim, 1), victimAge->year, victimAge->month, victimAge->day, victimAge->hours), ch);
	}
	if (fullness < 20 && ch != victim)
		return;

	val0 = GET_HEIGHT(victim);
	val1 = GET_WEIGHT(victim);
	val2 = GET_SIZE(victim);
	SendMsgToChar(fmt::format("Вес {}, Размер {}\r\n", val1, val2), ch);
	if (fullness < 60 && ch != victim)
		return;

	val0 = GetRealLevel(victim);
	val1 = victim->get_hit();
	val2 = victim->get_real_max_hit();
	SendMsgToChar(fmt::format("Уровень : {}, может выдержать повреждений : {}({}), ", val0, val1, val2), ch);
	SendMsgToChar(ch, "Перевоплощений : %d\r\n", remort::GetRealRemort(victim));
	val0 = std::min(GET_AR(victim), 100);
	val1 = std::min(GET_MR(victim), 100);
	val2 = std::min(GET_PR(victim), 100);
	SendMsgToChar(fmt::format("Защита от чар : {}, Защита от магических повреждений : {}, "
							  "Защита от физических повреждений : {}\r\n", val0, val1, val2), ch);
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
	line = fmt::format("Сила: {}, Ум: {}, Муд: {}, ", val0, val1, val2);
	val0 = victim->get_dex();
	val1 = victim->get_con();
	val2 = victim->get_cha();
	line += fmt::format("Ловк: {}, Тел: {}, Обаян: {}\r\n", val0, val1, val2);
	SendMsgToChar(line, ch);

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

			SendMsgToChar(fmt::format("   {} изменяет на {}{}\r\n",
									  GetTypeName(aff->location, apply_types),
									  aff->modifier > 0 ? "+" : "", aff->modifier), ch);
		}
	}
	SendMsgToChar(kColorNrm, ch);

	SendMsgToChar("Аффекты :\r\n", ch);
	SendMsgToChar(kColorBoldCyn, ch);
	SendMsgToChar(affects::DescribeActive(victim->char_specials.saved.affected_by, "\r\n") + "\r\n", ch);
	SendMsgToChar(kColorNrm, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
