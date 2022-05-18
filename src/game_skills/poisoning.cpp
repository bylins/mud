#include "entities/char_data.h"
#include "handler.h"
#include "liquid.h"
#include "game_mechanics/poison.h"
#include "utils/utils_char_obj.inl"

void DoPoisoning(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->GetSkill(ESkill::kPoisoning)) {
		SendMsgToChar("Вы не умеете этого.", ch);
		return;
	}

	argument = one_argument(argument, arg);
	skip_spaces(&argument);

	if (!*arg) {
		SendMsgToChar("Что вы хотите отравить?\r\n", ch);
		return;
	} else if (!*argument) {
		SendMsgToChar("Из чего вы собираете взять яд?\r\n", ch);
		return;
	}

	ObjData *weapon = nullptr;
	CharData *dummy = nullptr;
	int result = generic_find(arg, EFind::kObjInventory | EFind::kObjEquip, ch, &dummy, &weapon);

	if (!weapon || !result) {
		SendMsgToChar(ch, "У вас нет \'%s\'.\r\n", arg);
		return;
	} else if (GET_OBJ_TYPE(weapon) != EObjType::kWeapon) {
		SendMsgToChar("Вы можете нанести яд только на оружие.\r\n", ch);
		return;
	}

	ObjData *cont = get_obj_in_list_vis(ch, argument, ch->carrying);
	if (!cont) {
		SendMsgToChar(ch, "У вас нет \'%s\'.\r\n", argument);
		return;
	} else if (GET_OBJ_TYPE(cont) != EObjType::kLiquidContainer) {
		SendMsgToChar(ch, "%s не является емкостью.\r\n", cont->get_PName(0).c_str());
		return;
	} else if (GET_OBJ_VAL(cont, 1) <= 0) {
		SendMsgToChar(ch, "В %s нет никакой жидкости.\r\n", cont->get_PName(5).c_str());
		return;
	} else if (!poison_in_vessel(GET_OBJ_VAL(cont, 2))) {
		SendMsgToChar(ch, "В %s нет подходящего яда.\r\n", cont->get_PName(5).c_str());
		return;
	}

	auto cost = std::min(GET_OBJ_VAL(cont, 1), GetRealLevel(ch) <= 10 ? 1 : GetRealLevel(ch) <= 20 ? 2 : 3);
	cont->set_val(1, cont->get_val(1) - cost);
	weight_change_object(cont, -cost);
	if (!GET_OBJ_VAL(cont, 1)) {
		name_from_drinkcon(cont);
	}

	set_weap_poison(weapon, cont->get_val(2));

	snprintf(buf, sizeof(buf), "Вы осторожно нанесли немного %s на $o3.", drinks[cont->get_val(2)]);
	act(buf, false, ch, weapon, nullptr, kToChar);
	act("$n осторожно нанес$q яд на $o3.",
		false, ch, weapon, nullptr, kToRoom | kToArenaListen);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
