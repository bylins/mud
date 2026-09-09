#include "engine/entities/char_data.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/sight.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/minions.h"
#include "engine/db/world_objects.h"
#include "gameplay/economics/currencies.h"
#include "gameplay/fight/pk.h"
#include "gameplay/clans/house.h"
#include "engine/core/utils_char_obj.inl"
#include "engine/core/target_resolver.h"
#include "engine/db/global_objects.h"
#include "utils/utils_string.h"

#include <fmt/format.h>

extern void get_check_money(CharData *ch, ObjData *obj, ObjData *cont);
extern void split_or_clan_tax(CharData *ch, long amount);

void perform_give(CharData *ch, CharData *vict, ObjData *obj) {
	if (!bloody::handle_transfer(ch, vict, obj))
		return;
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoItem) && !privilege::IsGod(ch)) {
		act("Неведомая сила помешала вам сделать это!",
			false, ch, nullptr, nullptr, kToChar);
		return;
	}
	if (vict->IsNpc() && NPC_FLAGGED(vict, ENpcFlag::kNoTakeItems)) {
		act("$N не нуждается в ваших подачках, своего барахла навалом.",
			false, ch, nullptr, vict, kToChar);
		return;
	}
	if (vict->IsNpc() && mob_index[vict->get_rnum()].func && mob_index[vict->get_rnum()].func != guilds::GuildInfo::DoGuildLearn) {
		act("$N не нуждается в ваших подачках, своего барахла навалом.",
			false, ch, nullptr, vict, kToChar);
		mudlog(fmt::format("Попытка мобу с спецпроцедурой дать предмет: Моб {} #{} в комнате #{}, игрок: {}",
						   GET_NAME(vict), GET_MOB_VNUM(vict), GET_ROOM_VNUM(vict->in_room), GET_NAME(ch)),
			   CMP, kLvlGod, SYSLOG, true);
		return;
	}
	if (obj->has_flag(EObjFlag::kNodrop)) {
		act("Вы не можете передать $o3!", false, ch, obj, nullptr, kToChar);
		return;
	}
	if (vict->GetCarryingQuantity() >= CAN_CARRY_N(vict)) {
		act("У $N1 заняты руки.", false, ch, nullptr, vict, kToChar);
		return;
	}
	if (obj->get_weight() + vict->GetCarryingWeight() > CAN_CARRY_W(vict)) {
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

	RemoveObjFromChar(obj);
	PlaceObjToInventory(obj, vict);

	// передача объектов-денег и кошельков
	get_check_money(vict, obj, nullptr);

	if (!ch->IsNpc() && !vict->IsNpc()) {
		ObjSaveSync::add(ch->get_uid(), vict->get_uid(), ObjSaveSync::CHAR_SAVE);
	}
}

// utility function for give
CharData *give_find_vict(CharData *ch, const std::string &local_arg) {
	CharData *vict;

	if (local_arg.empty()) {
		SendMsgToChar("Кому?\r\n", ch);
		return (nullptr);
	} else if (!(vict = target_resolver::FindCharInRoom(ch, local_arg))) {
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
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
	if (currencies::GetHand(*ch, currencies::kGold) < amount && (ch->IsNpc() || !privilege::IsImpl(ch))) {
		SendMsgToChar("И откуда вы их взять собираетесь?\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoItem) && !privilege::IsGod(ch)) {
		act("Неведомая сила помешала вам сделать это!",
			false, ch, nullptr, nullptr, kToChar);
		return;
	}
	const std::string gold = MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(amount, grammar::ECase::kAcc);
	act(fmt::format("Вы дали {} {} $N2.", amount, gold), false, ch, nullptr, vict, kToChar);
	act(fmt::format("$n дал$g вам {} {}.", amount, gold), false, ch, nullptr, vict, kToVict);
	act(fmt::format("$n дал$g {} $N2.",
					MUD::Currency(currencies::kGoldVnum).GetObjCName(amount, grammar::ECase::kAcc)),
		true, ch, nullptr, vict, kToNotVict | kToArenaListen);
	if (!(ch->IsNpc() || vict->IsNpc())) {
		mudlog(fmt::format("<{}> {{{}}} передал {} кун при личной встрече c {}.",
						   ch->get_name(), GET_ROOM_VNUM(ch->in_room), amount, GET_PAD(vict, 4)),
			   NRM, kLvlGreatGod, MONEY_LOG, true);
	}
	if (ch->IsNpc() || !privilege::IsImpl(ch)) {
		currencies::RemoveHand(*ch, currencies::kGold, amount);
	}
	// если денег дает моб - снимаем клан-налог
	if (ch->IsNpc() && !IsCharmice(ch)) {
		currencies::AddHand(*vict, currencies::kGold, amount);
		split_or_clan_tax(vict, amount);
	} else {
		currencies::AddHand(*vict, currencies::kGold, amount);
	}
	bribe_mtrigger(vict, ch, amount);
}

void perform_give_currency(CharData *ch, CharData *vict, const currencies::CurrencyInfo &cur, int amount) {
	if (amount <= 0) {
		SendMsgToChar("Ха-ха-ха (3 раза)...\r\n", ch);
		return;
	}
	if (currencies::GetHand(*ch, cur.GetTextId()) < amount && (ch->IsNpc() || !privilege::IsImpl(ch))) {
		SendMsgToChar("И откуда ты их взять собирался?\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoItem) && !privilege::IsGod(ch)) {
		act("Неведомая сила помешала вам сделать это!",
			false, ch, nullptr, nullptr, kToChar);
		return;
	}
	const std::string money = cur.GetNameWithAmount(amount, grammar::ECase::kAcc);
	act(fmt::format("Вы дали {} {} $N2.", amount, money), false, ch, nullptr, vict, kToChar);
	act(fmt::format("$n дал$g вам {} {}.", amount, money), false, ch, nullptr, vict, kToVict);
	act(fmt::format("$n дал$g {} {} $N2.", amount, money), true, ch, nullptr, vict, kToNotVict | kToArenaListen);
	if (ch->IsNpc() || !privilege::IsImpl(ch)) {
		currencies::RemoveHand(*ch, cur.GetTextId(), amount);
	}
	currencies::AddHand(*vict, cur.GetTextId(), amount, true);
}

void do_give(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	ObjData *obj, *next_obj;

	std::string remains;
	std::string what = utils::ExtractFirstArgumentLower(argument, remains);

	if (what.empty()) {
		SendMsgToChar("Дать что и кому?\r\n", ch);
		return;
	}

	if (is_number(what.c_str())) {
		auto amount = std::stoi(what);
		what = utils::ExtractFirstArgumentLower(remains, remains);
		if (utils::IsAbbr("coin", what.c_str()) || utils::IsAbbr("кун", what.c_str()) || !str_cmp("денег", what)) {
			if ((vict = give_find_vict(ch, utils::ExtractFirstArgumentLower(remains))) != nullptr) {
				perform_give_gold(ch, vict, amount);
			}
			return;
		}
		if (const auto *cur = currencies::FindBySearch(what); cur && cur->IsGiveable()) {
			if ((vict = give_find_vict(ch, utils::ExtractFirstArgumentLower(remains))) != nullptr) {
				perform_give_currency(ch, vict, *cur, amount);
			}
			return;
		}
		if (what.empty()) {
			SendMsgToChar(fmt::format("Чего {} вы хотите дать?\r\n", amount), ch);
		} else if (!(vict = give_find_vict(ch, remains))) {
			return;
		} else if (!(obj = get_obj_in_list_vis(ch, what, ch->carrying))) {
			if (const auto *cur = currencies::FindBySearch(what); cur && !cur->IsGiveable()) {
				SendMsgToChar("Эту валюту нельзя передать другому.\r\n", ch);
			} else {
				SendMsgToChar(fmt::format("У вас нет '{}'.\r\n", what), ch);
			}
		} else {
			while (obj && amount--) {
				next_obj = get_obj_in_list_vis(ch, what, obj->get_next_content());
				perform_give(ch, vict, obj);
				obj = next_obj;
			}
		}
		return;
	}

	if (!(vict = give_find_vict(ch, utils::ExtractFirstArgumentLower(remains)))) {
		return;
	}
	auto dotmode = find_all_dots(what);
	if (dotmode == kFindIndiv) {
		if (!(obj = get_obj_in_list_vis(ch, what, ch->carrying))) {
			SendMsgToChar(fmt::format("У вас нет '{}'.\r\n", what), ch);
		} else {
			perform_give(ch, vict, obj);
		}
		return;
	}

	if (dotmode == kFindAlldot && what.empty()) {
		SendMsgToChar("Дать \"все\" какого типа предметов?\r\n", ch);
		return;
	}
	if (!ch->carrying) {
		SendMsgToChar("У вас ведь ничего нет.\r\n", ch);
		return;
	}

	bool has_items = false;
	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->get_next_content();
		if (obj->get_extracted_list()) {
			continue;
		}
		if (sight::CanSeeObj(ch, obj)
			&& (dotmode == kFindAll
				|| isname(what, obj->get_aliases())
				|| CHECK_CUSTOM_LABEL(what.c_str(), obj, ch))) {
			perform_give(ch, vict, obj);
			has_items = true;
		}
	}
	if (!has_items) {
		SendMsgToChar(fmt::format("У вас нет '{}'.\r\n", what), ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
