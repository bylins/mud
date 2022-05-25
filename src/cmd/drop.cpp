//#include "drop.h"

#include "entities/char_data.h"
#include "game_economics/currencies.h"
#include "handler.h"
#include "game_fight/pk.h"

void PerformDropGold(CharData *ch, int amount) {
	if (amount <= 0) {
		SendMsgToChar("Да, похоже вы слишком переиграли сегодня.\r\n", ch);
	} else if (ch->get_gold() < amount) {
		SendMsgToChar("У вас нет такой суммы!\r\n", ch);
	} else {
		SetWaitState(ch, kPulseViolence);    // to prevent coin-bombing
		if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoItem)) {
			act("Неведомая сила помешала вам сделать это!",
				false, ch, nullptr, nullptr, kToChar);
			return;
		}
		//Находим сначала кучку в комнате
		int additional_amount = 0;
		ObjData *next_obj;
		for (ObjData *existing_obj = world[ch->in_room]->contents; existing_obj; existing_obj = next_obj) {
			next_obj = existing_obj->get_next_content();
			if (GET_OBJ_TYPE(existing_obj) == EObjType::kMoney && GET_OBJ_VAL(existing_obj, 1) == currency::GOLD) {
				//Запоминаем стоимость существующей кучки и удаляем ее
				additional_amount = GET_OBJ_VAL(existing_obj, 0);
				ExtractObjFromRoom(existing_obj);
				ExtractObjFromWorld(existing_obj);
			}
		}

		const auto obj = currencies::CreateCurrencyObj(amount + additional_amount);
		int result = drop_wtrigger(obj.get(), ch);

		if (!result) {
			ExtractObjFromWorld(obj.get());
			return;
		}

		// Если этот моб трупа не оставит, то не выводить сообщение иначе ужасно коряво смотрится в бою и в тригах
		if (!ch->IsNpc() || !MOB_FLAGGED(ch, EMobFlag::kCorpse)) {
			SendMsgToChar(ch, "Вы бросили %d %s на землю.\r\n",
						  amount, GetDeclensionInNumber(amount, EWhat::kMoneyU));
			sprintf(buf,
					"<%s> {%d} выбросил %d %s на землю.",
					ch->get_name().c_str(),
					GET_ROOM_VNUM(ch->in_room),
					amount,
					GetDeclensionInNumber(amount, EWhat::kMoneyU));
			mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
			sprintf(buf, "$n бросил$g %s на землю.",
					currencies::GetCurrencyObjDescription(currencies::KunaVnum, amount, ECase::kAcc));
			act(buf, true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		PlaceObjToRoom(obj.get(), ch->in_room);

		ch->remove_gold(amount);
	}
}

const char *drop_op[3] =
	{
		"бросить", "бросили", "бросил"
	};

void PerformDrop(CharData *ch, ObjData *obj) {
	if (!drop_otrigger(obj, ch))
		return;
	if (!bloody::handle_transfer(ch, nullptr, obj))
		return;
	if (!drop_wtrigger(obj, ch))
		return;

	if (obj->has_flag(EObjFlag::kNodrop)) {
		sprintf(buf, "Вы не можете %s $o3!", drop_op[0]);
		act(buf, false, ch, obj, nullptr, kToChar);
		return;
	}
	sprintf(buf, "Вы %s $o3.", drop_op[1]);
	act(buf, false, ch, obj, nullptr, kToChar);
	sprintf(buf, "$n %s$g $o3.", drop_op[2]);
	act(buf, true, ch, obj, nullptr, kToRoom | kToArenaListen);
	ExtractObjFromChar(obj);

	PlaceObjToRoom(obj, ch->in_room);
	CheckObjDecay(obj);
}

void DoDrop(CharData *ch, char *argument, int/* cmd*/, int /*subcmd*/) {
	ObjData *obj, *next_obj;

	argument = one_argument(argument, arg);

	if (!*arg) {
		sprintf(buf, "Что вы хотите %s?\r\n", drop_op[0]);
		SendMsgToChar(buf, ch);
		return;
	} else if (is_number(arg)) {
		auto multi = std::stoi(arg);
		one_argument(argument, arg);
		if (!str_cmp("coins", arg) || !str_cmp("coin", arg) || !str_cmp("кун", arg) || !str_cmp("денег", arg))
			PerformDropGold(ch, multi);
		else if (multi <= 0)
			SendMsgToChar("Не имеет смысла.\r\n", ch);
		else if (!*arg) {
			sprintf(buf, "%s %d чего?\r\n", drop_op[0], multi);
			SendMsgToChar(buf, ch);
		} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
			snprintf(buf, kMaxInputLength, "У вас нет ничего похожего на %s.\r\n", arg);
			SendMsgToChar(buf, ch);
		} else {
			do {
				next_obj = get_obj_in_list_vis(ch, arg, obj->get_next_content());
				PerformDrop(ch, obj);
				obj = next_obj;
			} while (obj && --multi);
		}
	} else {
		const auto dotmode = find_all_dots(arg);
		// Can't junk or donate all
		if (dotmode == kFindAll) {
			if (!ch->carrying)
				SendMsgToChar("А у вас ничего и нет.\r\n", ch);
			else
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->get_next_content();
					PerformDrop(ch, obj);
				}
		} else if (dotmode == kFindAlldot) {
			if (!*arg) {
				sprintf(buf, "%s \"все\" какого типа предметов?\r\n", drop_op[0]);
				SendMsgToChar(buf, ch);
				return;
			}
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				snprintf(buf, kMaxInputLength, "У вас нет ничего похожего на '%s'.\r\n", arg);
				SendMsgToChar(buf, ch);
			}
			while (obj) {
				next_obj = get_obj_in_list_vis(ch, arg, obj->get_next_content());
				PerformDrop(ch, obj);
				obj = next_obj;
			}
		} else {
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				snprintf(buf, kMaxInputLength, "У вас нет '%s'.\r\n", arg);
				SendMsgToChar(buf, ch);
			} else
				PerformDrop(ch, obj);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
