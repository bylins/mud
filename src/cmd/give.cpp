#include "entities/char_data.h"
#include "entities/world_objects.h"
#include "game_fight/pk.h"
#include "house.h"
#include "utils/utils_char_obj.inl"

extern void get_check_money(CharData *ch, ObjData *obj, ObjData *cont);
extern void split_or_clan_tax(CharData *ch, long amount);

void perform_give(CharData *ch, CharData *vict, ObjData *obj) {
	if (!bloody::handle_transfer(ch, vict, obj))
		return;
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoItem) && !IS_GOD(ch)) {
		act("Неведомая сила помешала вам сделать это!",
			false, ch, nullptr, nullptr, kToChar);
		return;
	}
	if (NPC_FLAGGED(vict, ENpcFlag::kNoTakeItems)) {
		act("$N не нуждается в ваших подачках, своего барахла навалом.",
			false, ch, nullptr, vict, kToChar);
		return;
	}
	if (obj->has_flag(EObjFlag::kNodrop)) {
		act("Вы не можете передать $o3!", false, ch, obj, nullptr, kToChar);
		return;
	}
	if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
		act("У $N1 заняты руки.", false, ch, nullptr, vict, kToChar);
		return;
	}
	if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
		act("$E не может нести такой вес.", false, ch, nullptr, vict, kToChar);
		return;
	}
	if (!give_otrigger(obj, ch, vict)) {
		act("$E не хочет иметь дело с этой вещью.", false, ch, nullptr, vict, kToChar);
		return;
	}

	if (!receive_mtrigger(vict, ch, obj)) {
		act("$E не хочет иметь дело с этой вещью.", false, ch, nullptr, vict, kToChar);
		return;
	}

	act("Вы дали $o3 $N2.", false, ch, obj, vict, kToChar);
	act("$n дал$g вам $o3.", false, ch, obj, vict, kToVict);
	act("$n дал$g $o3 $N2.", true, ch, obj, vict, kToNotVict | kToArenaListen);

	if (!world_objects.get_by_raw_ptr(obj)) {
		return;    // object has been removed from world during script execution.
	}

	ExtractObjFromChar(obj);
	PlaceObjToInventory(obj, vict);

	// передача объектов-денег и кошельков
	get_check_money(vict, obj, nullptr);

	if (!ch->IsNpc() && !vict->IsNpc()) {
		ObjSaveSync::add(ch->get_uid(), vict->get_uid(), ObjSaveSync::CHAR_SAVE);
	}
}

// utility function for give
CharData *give_find_vict(CharData *ch, char *local_arg) {
	CharData *vict;

	if (!*local_arg) {
		SendMsgToChar("Кому?\r\n", ch);
		return (nullptr);
	} else if (!(vict = get_char_vis(ch, local_arg, EFind::kCharInRoom))) {
		SendMsgToChar(NOPERSON, ch);
		return (nullptr);
	} else if (vict == ch) {
		SendMsgToChar("Вы переложили ЭТО из одного кармана в другой.\r\n", ch);
		return (nullptr);
	} else
		return (vict);
}

void perform_give_gold(CharData *ch, CharData *vict, int amount) {
	if (amount <= 0) {
		SendMsgToChar("Ха-ха-ха (3 раза)...\r\n", ch);
		return;
	}
	if (ch->get_gold() < amount && (ch->IsNpc() || !IS_IMPL(ch))) {
		SendMsgToChar("И откуда вы их взять собираетесь?\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoItem) && !IS_GOD(ch)) {
		act("Неведомая сила помешала вам сделать это!",
			false, ch, nullptr, nullptr, kToChar);
		return;
	}
	SendMsgToChar(OK, ch);
	sprintf(buf, "$n дал$g вам %d %s.", amount, GetDeclensionInNumber(amount, EWhat::kMoneyU));
	act(buf, false, ch, nullptr, vict, kToVict);
	sprintf(buf, "$n дал$g %s $N2.", money_desc(amount, 3));
	act(buf, true, ch, nullptr, vict, kToNotVict | kToArenaListen);
	if (!(ch->IsNpc() || vict->IsNpc())) {
		sprintf(buf,
				"<%s> {%d} передал %d кун при личной встрече c %s.",
				ch->get_name().c_str(),
				GET_ROOM_VNUM(ch->in_room),
				amount,
				GET_PAD(vict, 4));
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
	}
	if (ch->IsNpc() || !IS_IMPL(ch)) {
		ch->remove_gold(amount);
	}
	// если денег дает моб - снимаем клан-налог
	if (ch->IsNpc() && !IS_CHARMICE(ch)) {
		vict->add_gold(amount);
		split_or_clan_tax(vict, amount);
	} else {
		vict->add_gold(amount);
	}
	bribe_mtrigger(vict, ch, amount);
}

void perform_give_nogat(CharData *ch, CharData *vict, int amount) {
	if (amount <= 0) {
		SendMsgToChar("Ха-ха-ха (3 раза)...\r\n", ch);
		return;
	}
	if (ch->get_nogata() < amount && (ch->IsNpc() || !IS_IMPL(ch))) {
		SendMsgToChar("И откуда ты их взять собирался?\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoItem) && !IS_GOD(ch)) {
		act("Неведомая сила помешала вам сделать это!",
			false, ch, nullptr, nullptr, kToChar);
		return;
	}
	SendMsgToChar(OK, ch);
	sprintf(buf, "$n дал$g вам %d %s.", amount, GetDeclensionInNumber(amount, EWhat::kNogataU));
	act(buf, false, ch, nullptr, vict, kToVict);
	if (amount > 4)
		sprintf(buf, "$n дал$g много %s $N2.", GetDeclensionInNumber(amount, EWhat::kNogataU));
	else
		sprintf(buf, "$n дал$g %s $N2.", GetDeclensionInNumber(amount, EWhat::kNogataU));
	act(buf, true, ch, nullptr, vict, kToNotVict | kToArenaListen);
	if (ch->IsNpc() || !IS_IMPL(ch)) {
		ch->sub_nogata(amount);
	}
	vict->add_nogata(amount);
}

void do_give(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	ObjData *obj, *next_obj;

	argument = one_argument(argument, arg);

	if (!*arg)
		SendMsgToChar("Дать что и кому?\r\n", ch);
	else if (is_number(arg)) {
		auto amount = std::stoi(arg);
		argument = one_argument(argument, arg);
		if (!strn_cmp("coin", arg, 4) || !strn_cmp("кун", arg, 3) || !str_cmp("денег", arg)) {
			one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg)) != nullptr)
				perform_give_gold(ch, vict, amount);
			return;
		}
		if (!strn_cmp("nogat", arg, 5) || !strn_cmp("ногат", arg, 5)) {
			one_argument(argument, arg);
			if ((vict = give_find_vict(ch, arg)) != nullptr)
				perform_give_nogat(ch, vict, amount);
			return;
		}
		if (!*arg) {
			sprintf(buf, "Чего %d вы хотите дать?\r\n", amount);
			SendMsgToChar(buf, ch);
		} else if (!(vict = give_find_vict(ch, argument))) {
			return;
		} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
			snprintf(buf, kMaxInputLength, "У вас нет '%s'.\r\n", arg);
			SendMsgToChar(buf, ch);
		} else {
			while (obj && amount--) {
				next_obj = get_obj_in_list_vis(ch, arg, obj->get_next_content());
				perform_give(ch, vict, obj);
				obj = next_obj;
			}
		}
	} else {
		one_argument(argument, buf1);
		if (!(vict = give_find_vict(ch, buf1)))
			return;
		auto dotmode = find_all_dots(arg);
		if (dotmode == kFindIndiv) {
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				snprintf(buf, kMaxInputLength, "У вас нет '%s'.\r\n", arg);
				SendMsgToChar(buf, ch);
			} else
				perform_give(ch, vict, obj);
		} else {
			if (dotmode == kFindAlldot && !*arg) {
				SendMsgToChar("Дать \"все\" какого типа предметов?\r\n", ch);
				return;
			}
			if (!ch->carrying)
				SendMsgToChar("У вас ведь ничего нет.\r\n", ch);
			else {
				bool has_items = false;
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->get_next_content();
					if (CAN_SEE_OBJ(ch, obj)
						&& (dotmode == kFindAll
							|| isname(arg, obj->get_aliases())
							|| CHECK_CUSTOM_LABEL(arg, obj, ch))) {
						perform_give(ch, vict, obj);
						has_items = true;
					}
				}
				if (!has_items) {
					SendMsgToChar(ch, "У вас нет '%s'.\r\n", arg);
				}
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
