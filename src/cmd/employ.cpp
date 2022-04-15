#include "employ.h"

#include "entities/char_data.h"
#include "handler.h"
#include "game_magic/magic_items.h"

void apply_enchant(CharData *ch, ObjData *obj, std::string text);

void do_employ(CharData *ch, char *argument, int cmd, int subcmd) {
	ObjData *mag_item;
	int do_hold = 0;
	two_arguments(argument, arg, buf);
	char *buf_temp = str_dup(buf);
	if (!*arg) {
		sprintf(buf2, "Что вы хотите %s?\r\n", CMD_NAME);
		SendMsgToChar(buf2, ch);
		return;
	}

	if (PRF_FLAGS(ch).get(EPrf::kIronWind)) {
		SendMsgToChar("Вы в бою и вам сейчас не до магических выкрутасов!\r\n", ch);
		return;
	}

	mag_item = GET_EQ(ch, kHold);
	if (!mag_item
		|| !isname(arg, mag_item->get_aliases())) {
		switch (subcmd) {
			case SCMD_RECITE:
			case SCMD_QUAFF:
				if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying))) {
					snprintf(buf2, kMaxStringLength, "Окститесь, нет у вас %s.\r\n", arg);
					SendMsgToChar(buf2, ch);
					return;
				}
				break;
			case SCMD_USE: mag_item = get_obj_in_list_vis(ch, arg, ch->carrying);
				if (!mag_item
					|| GET_OBJ_TYPE(mag_item) != EObjType::kEnchant) {
					snprintf(buf2, kMaxStringLength, "Возьмите в руку '%s' перед применением!\r\n", arg);
					SendMsgToChar(buf2, ch);
					return;
				}
				break;
			default: log("SYSERR: Unknown subcmd %d passed to do_employ.", subcmd);
				return;
		}
	}
	switch (subcmd) {
		case SCMD_QUAFF:
			if (PRF_FLAGS(ch).get(EPrf::kIronWind)) {
				SendMsgToChar("Не стоит отвлекаться в бою!\r\n", ch);
				return;
			}
			if (GET_OBJ_TYPE(mag_item) != EObjType::kPotion) {
				SendMsgToChar("Осушить вы можете только напиток (ну, Богам еще пЫво по вкусу ;)\r\n", ch);
				return;
			}
			do_hold = 1;
			break;
		case SCMD_RECITE:
			if (GET_OBJ_TYPE(mag_item) != EObjType::kScroll) {
				SendMsgToChar("Пригодны для зачитывания только свитки.\r\n", ch);
				return;
			}
			do_hold = 1;
			break;
		case SCMD_USE:
			if (GET_OBJ_TYPE(mag_item) == EObjType::kEnchant) {
				apply_enchant(ch, mag_item, buf);
				return;
			}
			if (GET_OBJ_TYPE(mag_item) != EObjType::kWand
				&& GET_OBJ_TYPE(mag_item) != EObjType::kStaff) {
				SendMsgToChar("Применять можно только магические предметы!\r\n", ch);
				return;
			}
			// палочки с чармами/оживлялками юзают только кастеры и дружи до 25 левева
			if (GET_OBJ_VAL(mag_item, 3) == kSpellCharm
				|| GET_OBJ_VAL(mag_item, 3) == kSpellAnimateDead
				|| GET_OBJ_VAL(mag_item, 3) == kSpellResurrection) {
				if (!IsAbleToUseFeat(ch, EFeat::kMagicUser)) {
					SendMsgToChar("Да, штука явно магическая! Но совершенно непонятно как ей пользоваться. :(\r\n", ch);
					return;
				}
			}
			break;
	}
	if (do_hold && GET_EQ(ch, EEquipPos::kHold) != mag_item) {
		if (GET_EQ(ch, EEquipPos::kBoths))
			do_hold = EEquipPos::kBoths;
		else if (GET_EQ(ch, EEquipPos::kShield))
			do_hold = EEquipPos::kShield;
		else
			do_hold = EEquipPos::kHold;

		if (GET_EQ(ch, do_hold)) {
			act("Вы прекратили использовать $o3.", false, ch, GET_EQ(ch, do_hold), 0, kToChar);
			act("$n прекратил$g использовать $o3.", false, ch, GET_EQ(ch, do_hold), 0, kToRoom | kToArenaListen);
			PlaceObjToInventory(UnequipChar(ch, do_hold, CharEquipFlags()), ch);
		}
		if (GET_EQ(ch, EEquipPos::kHold))
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kHold, CharEquipFlags()), ch);
		//obj_from_char(mag_item);
		EquipObj(ch, mag_item, EEquipPos::kHold, CharEquipFlags());
	}
	if ((do_hold && GET_EQ(ch, EEquipPos::kHold) == mag_item) || (!do_hold))
		EmployMagicItem(ch, mag_item, buf_temp);
	free(buf_temp);

}

void apply_enchant(CharData *ch, ObjData *obj, std::string text) {
	std::string tmp_buf;
	GetOneParam(text, tmp_buf);
	if (tmp_buf.empty()) {
		SendMsgToChar("Укажите цель применения.\r\n", ch);
		return;
	}

	ObjData *target = get_obj_in_list_vis(ch, tmp_buf, ch->carrying);
	if (!target) {
		SendMsgToChar(ch, "Окститесь, у вас нет такого предмета для зачаровывания.\r\n");
		return;
	}

	if (target->has_flag(EObjFlag::KSetItem)) {
		SendMsgToChar(ch, "Сетовый предмет не может быть зачарован.\r\n");
		return;
	}
	if (GET_OBJ_TYPE(target) == EObjType::kEnchant) {
		SendMsgToChar(ch, "Этот предмет уже магический и не может быть зачарован.\r\n");
		return;
	}

	if (target->get_enchants().check(ObjectEnchant::ENCHANT_FROM_OBJ)) {
		SendMsgToChar(ch, "На %s уже наложено зачарование.\r\n",
					  target->get_PName(3).c_str());
		return;
	}

	auto check_slots = GET_OBJ_WEAR(obj) & GET_OBJ_WEAR(target);
	if (check_slots > 0
		&& check_slots != to_underlying(EWearFlag::kTake)) {
		SendMsgToChar(ch, "Вы успешно зачаровали %s.\r\n", GET_OBJ_PNAME(target, 0).c_str());
		ObjectEnchant::enchant ench(obj);
		ench.apply_to_obj(target);
		ExtractObjFromWorld(obj);
	} else {
		int slots = obj->get_wear_flags();
		REMOVE_BIT(slots, EWearFlag::kTake);
		if (sprintbit(slots, wear_bits, buf2)) {
			SendMsgToChar(ch, "Это зачарование применяется к предметам со слотами надевания: %s\r\n", buf2);
		} else {
			SendMsgToChar(ch, "Некорретное зачарование, не проставлены слоты надевания.\r\n");
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
