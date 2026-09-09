#include "engine/entities/char_data.h"
#include "skill_messages.h"
#include "engine/db/global_objects.h"
#include "engine/core/target_resolver.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/mechanics/poison.h"
#include "engine/core/utils_char_obj.inl"
#include "utils/utils_string.h"

#include <fmt/format.h>

void DoPoisoning(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!GetSkill(ch, ESkill::kPoisoning)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kPoisoning, ESkillMsg::kDontKnowSkill), ch);
		return;
	}

	std::string remains;
	const std::string weapon_name = utils::ExtractFirstArgumentLower(argument, remains);

	if (weapon_name.empty()) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kPoisoning, ESkillMsg::kNoTarget) + "\r\n", ch);
		return;
	} else if (remains.empty()) {
		SendMsgToChar("Из чего вы собираете взять яд?\r\n", ch);
		return;
	}

	ObjData *weapon = nullptr;
	CharData *dummy = nullptr;
	int result = generic_find(weapon_name, EFind::kObjInventory | EFind::kObjEquip, ch, &dummy, &weapon);

	if (!weapon || !result) {
		SendMsgToChar(fmt::format("У вас нет '{}'.\r\n", weapon_name), ch);
		return;
	} else if (weapon->get_type() != EObjType::kWeapon) {
		SendMsgToChar("Вы можете нанести яд только на оружие.\r\n", ch);
		return;
	}

	ObjData *cont = get_obj_in_list_vis(ch, remains, ch->carrying);
	if (!cont) {
		SendMsgToChar(fmt::format("У вас нет '{}'.\r\n", remains), ch);
		return;
	} else if (cont->get_type() != EObjType::kLiquidContainer) {
		SendMsgToChar(ch, "%s не является емкостью.\r\n", cont->get_PName(grammar::ECase::kNom).c_str());
		return;
	} else if (GET_OBJ_VAL(cont, 1) <= 0) {
		SendMsgToChar(ch, "В %s нет никакой жидкости.\r\n", cont->get_PName(grammar::ECase::kPre).c_str());
		return;
	} else if (!poison_in_vessel(GET_OBJ_VAL(cont, 2))) {
		SendMsgToChar(ch, "В %s нет подходящего яда.\r\n", cont->get_PName(grammar::ECase::kPre).c_str());
		return;
	}

	auto cost = std::min(GET_OBJ_VAL(cont, 1), GetRealLevel(ch) <= 10 ? 1 : GetRealLevel(ch) <= 20 ? 2 : 3);
	cont->set_val(1, cont->get_val(1) - cost);
	weight_change_object(cont, -cost);
	if (!GET_OBJ_VAL(cont, 1)) {
		name_from_drinkcon(cont);
	}

	set_weap_poison(weapon, cont->get_val(2));

	act(fmt::format("Вы осторожно нанесли немного {} на $o3.", drinks[cont->get_val(2)]),
		false, ch, weapon, nullptr, kToChar);
	act("$n осторожно нанес$q яд на $o3.",
		false, ch, weapon, nullptr, kToRoom | kToArenaListen);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
