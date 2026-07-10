//#include "drop.h"
#include "gameplay/magic/magic.h"   // issue.obj-affects: RunObjAffectTrigger

#include "engine/entities/char_data.h"
#include "utils/grammar/declensions.h"
#include "gameplay/economics/currencies.h"
#include "engine/core/obj_handler.h"
#include "engine/core/target_resolver.h"
#include "gameplay/mechanics/inventory.h"
#include "gameplay/fight/pk.h"
#include "engine/db/global_objects.h"

extern ObjData::shared_ptr CreateCurrencyObj(long quantity);
extern ObjData::shared_ptr CreateCurrencyObj(long quantity, int currency_vnum);

void PerformDropGold(CharData *ch, int amount) {
	if (amount <= 0) {
		SendMsgToChar("Да, похоже вы слишком переиграли сегодня.\r\n", ch);
	} else if (currencies::GetHand(*ch, currencies::kGold) < amount) {
		SendMsgToChar("У вас нет такой суммы!\r\n", ch);
	} else {
		SetBattleLag(ch, 1);    // to prevent coin-bombing
		if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoItem)) {
			act("Неведомая сила помешала вам сделать это!",
				false, ch, nullptr, nullptr, kToChar);
			return;
		}
		//Находим сначала кучку в комнате
		int additional_amount = 0;
		for (auto it = world[ch->in_room]->contents.begin(); it != world[ch->in_room]->contents.end(); ) {
			auto existing_obj = *it;
			++it;
			if (existing_obj->get_type() == EObjType::kMoney && GET_OBJ_VAL(existing_obj, 1) == currencies::kGoldVnum) {
				//Запоминаем стоимость существующей кучки и удаляем ее
				additional_amount = GET_OBJ_VAL(existing_obj, 0);
				RemoveObjFromRoom(existing_obj);
				ExtractObjFromWorld(existing_obj);
			}
		}

		const auto obj = CreateCurrencyObj(amount + additional_amount);
		int result = drop_wtrigger(obj.get(), ch);

		if (!result) {
			ExtractObjFromWorld(obj.get());
			return;
		}

		// Если этот моб трупа не оставит, то не выводить сообщение иначе ужасно коряво смотрится в бою и в тригах
		if (!ch->IsNpc() || !ch->IsFlagged(EMobFlag::kCorpse)) {
			SendMsgToChar(ch, "Вы бросили %d %s на землю.\r\n",
						  amount, MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(amount, grammar::ECase::kNom).c_str());
			sprintf(buf,
					"<%s> {%d} выбросил %d %s на землю.",
					ch->get_name().c_str(),
					GET_ROOM_VNUM(ch->in_room),
					amount,
					MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(amount, grammar::ECase::kNom).c_str());
			mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
			sprintf(buf, "$n бросил$g %s на землю.",
					MUD::Currency(currencies::kGoldVnum).GetObjCName(amount, grammar::ECase::kAcc));
			act(buf, true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		PlaceObjToRoom(obj.get(), ch->in_room);

		currencies::RemoveHand(*ch, currencies::kGold, amount);
	}
}

// issue.currency-storage: drop any objectable currency as a money pile (mirrors PerformDropGold,
// parametrised by the currency). do_get reads the object's val[1] and credits the right currency.
void PerformDropCurrency(CharData *ch, const currencies::CurrencyInfo &cur, int amount) {
	const int currency_vnum = cur.GetId();
	if (amount <= 0) {
		SendMsgToChar("Да, похоже вы слишком переиграли сегодня.\r\n", ch);
		return;
	}
	if (currencies::GetHand(*ch, currency_vnum) < amount) {
		SendMsgToChar("У вас нет такой суммы!\r\n", ch);
		return;
	}
	SetBattleLag(ch, 1);
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoItem)) {
		act("Неведомая сила помешала вам сделать это!", false, ch, nullptr, nullptr, kToChar);
		return;
	}
	// Merge with an existing pile of the same currency already lying in the room.
	int additional_amount = 0;
	for (auto it = world[ch->in_room]->contents.begin(); it != world[ch->in_room]->contents.end(); ) {
		auto existing_obj = *it;
		++it;
		if (existing_obj->get_type() == EObjType::kMoney && GET_OBJ_VAL(existing_obj, 1) == currency_vnum) {
			additional_amount = GET_OBJ_VAL(existing_obj, 0);
			RemoveObjFromRoom(existing_obj);
			ExtractObjFromWorld(existing_obj);
		}
	}
	const auto obj = CreateCurrencyObj(amount + additional_amount, currency_vnum);
	if (!drop_wtrigger(obj.get(), ch)) {
		ExtractObjFromWorld(obj.get());
		return;
	}
	if (!ch->IsNpc() || !ch->IsFlagged(EMobFlag::kCorpse)) {
		SendMsgToChar(ch, "Вы бросили %d %s на землю.\r\n",
					  amount, cur.GetNameWithAmount(amount, grammar::ECase::kNom).c_str());
		sprintf(buf, "<%s> {%d} выбросил %d %s на землю.",
				ch->get_name().c_str(), GET_ROOM_VNUM(ch->in_room), amount,
				cur.GetNameWithAmount(amount, grammar::ECase::kNom).c_str());
		mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
		sprintf(buf, "$n бросил$g %s на землю.", cur.GetObjCName(amount, grammar::ECase::kAcc));
		act(buf, true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
	}
	PlaceObjToRoom(obj.get(), ch->in_room);
	currencies::RemoveHand(*ch, currency_vnum, amount);
}

const char *drop_op[3] =
	{
		"бросить", "бросили", "бросил"
	};

void PerformDrop(CharData *ch, ObjData *obj) {
	if (!drop_otrigger(obj, ch, kOtrigDropInroom))
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
	RemoveObjFromChar(obj);
	PlaceObjToRoom(obj, ch->in_room);
	RunObjAffectTrigger(obj, ch, talents_actions::EActionTrigger::kDrop);   // issue.obj-affects
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
		else if (const auto *cur = currencies::FindBySearch(arg); cur && cur->IsObjectable())
			PerformDropCurrency(ch, *cur, multi);
		else if (multi <= 0)
			SendMsgToChar("Не имеет смысла.\r\n", ch);
		else if (!*arg) {
			sprintf(buf, "%s %d чего?\r\n", drop_op[0], multi);
			SendMsgToChar(buf, ch);
		} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
			if (const auto *cur = currencies::FindBySearch(arg); cur && !cur->IsObjectable()) {
				SendMsgToChar("Эту валюту нельзя бросить на землю.\r\n", ch);
			} else {
				snprintf(buf, kMaxInputLength, "У вас нет ничего похожего на %s.\r\n", arg);
				SendMsgToChar(buf, ch);
			}
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
					if (obj->get_extracted_list())
						continue;
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
