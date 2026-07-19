/**************************************************************************
*   File: exchange.cpp                                 Part of Bylins     *
*  Usage: Exchange functions used by the MUD                              *
*                                                                         *
*                                                                         *
*  $Author$                                                         *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "exchange.h"
#include "administration/privilege.h"
#include "engine/db/global_objects.h"
#include "gameplay/economics/currencies.h"
#include "gameplay/mechanics/sight.h"
#include "utils/grammar/gender.h"
#include "utils/grammar/declensions.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/mechanics/identify.h"

#include <fmt/format.h>
#include <fmt/printf.h>

#include "gameplay/ai/special_messages.h"

#include <iterator>

#include "engine/db/obj_prototypes.h"
#include "engine/db/world_characters.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/entities_constants.h"
#include "engine/core/comm.h"
#include "engine/ui/interpreter.h"
#include "engine/core/target_resolver.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/inventory.h"
#include "engine/db/db.h"
#include "engine/ui/color.h"
#include "gameplay/crafting/im.h"
#include "gameplay/core/constants.h"
#include "engine/entities/char_data.h"
#include "engine/entities/char_player.h"
#include "gameplay/mechanics/named_stuff.h"
#include "engine/ui/modify.h"
#include "gameplay/communication/mail.h"
#include "engine/db/obj_save.h"
#include "gameplay/fight/pk.h"
#include "utils/logger.h"
#include "engine/ui/objects_filter.h"
#include "utils/utils.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/core/remort.h"

#include <stdexcept>
#include <sstream>
#include "engine/db/player_index.h"

//Используемые внешние ф-ии.
extern int get_buf_line(char **source, char *target);
extern int get_buf_lines(char **source, char *target);

extern int invalid_anti_class(CharData *ch, const ObjData *obj);
extern int invalid_unique(CharData *ch, const ObjData *obj);
extern int invalid_no_class(CharData *ch, const ObjData *obj);
extern int invalid_align(CharData *ch, const ObjData *obj);
extern void SetWait(CharData *ch, int waittime, int victim_in_room);
extern bool is_dig_stone(ObjData *obj);

//свои ф-ии
int exchange_exhibit(CharData *ch, char *arg);
int exchange_change_cost(CharData *ch, char *arg);
int exchange_withdraw(CharData *ch, char *arg);
int exchange_information(CharData *ch, char *arg);
int exchange_identify(CharData *ch, char *arg);
int exchange_purchase(CharData *ch, char *arg);
int exchange_offers(const CharData *ch, const char *arg);
bool exchange_setfilter(CharData *ch, char *argument);

int LoadExchange();
int exchange_database_reload(bool loadbackup);
int get_unique_lot();
void message_exchange(char *message, CharData *ch, ExchangeItem *j);
void show_lots(const char *filter, short int show_type, const CharData *ch);
int parse_exch_filter(ParseFilter &filter, char *buf, bool parse_affects);
void clear_exchange_lot(ExchangeItem *lot);
extern void sight::obj_info(CharData *ch, ObjData *obj, char buf[kMaxStringLength]);

void do_exchange(CharData *ch, char *argument, int cmd, int subcmd);

ExchangeItem *create_exchange_item();

ExchangeItem *exchange_item_list = nullptr;
std::vector<bool> lot_usage;

char arg1[kMaxInputLength], arg2[kMaxInputLength];

char info_message[] = ("базар выставить <предмет> <цена>        - выставить предмет на продажу\r\n"
					   "базар цена <#лот> <цена>                - изменить цену на свой лот\r\n"
					   "базар снять <#лот>                      - снять с продажи свой лот\r\n"
					   "базар информация <#лот>                 - информация о лоте\r\n"
					   "базар характеристики <#лот>             - характеристики лота (цена услуги 110 кун)\r\n"
					   "базар купить <#лот>                     - купить лот\r\n"
					   "базар предложения <временный фильр>     - все предложения согласно указанного фильтра плюс фильтрация\r\n"
					   "базар предложения мои                   - мои предложения\r\n"
					   "базар предложения все                   - все предложения\r\n"
					   "базар фильтрация <фильтр>               - фильтрация товара на базаре (запоминается)\r\n"
					   "базар фильтрация нет                    - стереть фильтр\r\n");

int exchange(CharData *ch, void * /*me*/, int cmd, char *argument) {
	if (CMD_IS("exchange") || CMD_IS("базар")) {
		if (ch->IsNpc())
			return 0;
		if (AFF_FLAGGED(ch, EAffect::kSilence)) {
			SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kSilenced) + "\r\n", ch);
			return 1;
		}
		if (ch->IsFlagged(EPlrFlag::kDumbed)) {
			SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kDumbed) + "\r\n", ch);
			return 1;
		}
		/*
				if (EPlrFlag::FLAGGED(ch, EPlrFlag::MUTE)) {
					SendMsgToChar("Вам не к лицу торговаться.\r\n", ch);
					return 1;
				}
		*/
		if (GetRealLevel(ch) < EXCHANGE_MIN_CHAR_LEV && !remort::GetRealRemort(ch)) {
			snprintf(buf1, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kLevelTooLow)), fmt::arg("level", EXCHANGE_MIN_CHAR_LEV)) + "\r\n").c_str());
			SendMsgToChar(buf1, ch);
			return 1;
		}
		if (NORENTABLE(ch)) {
			SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNoRentable) + "\r\n", ch);
			return 1;
		}

		argument = one_argument(argument, arg1);

		if (utils::IsAbbr(arg1, "выставить") || utils::IsAbbr(arg1, "exhibit"))
			exchange_exhibit(ch, argument);
		else if (utils::IsAbbr(arg1, "цена") || utils::IsAbbr(arg1, "cost"))
			exchange_change_cost(ch, argument);
		else if (utils::IsAbbr(arg1, "снять") || utils::IsAbbr(arg1, "withdraw"))
			exchange_withdraw(ch, argument);
		else if (utils::IsAbbr(arg1, "информация") || utils::IsAbbr(arg1, "information"))
			exchange_information(ch, argument);
		else if (utils::IsAbbr(arg1, "характеристики") || utils::IsAbbr(arg1, "identify"))
			exchange_identify(ch, argument);
		else if (utils::IsAbbr(arg1, "купить") || utils::IsAbbr(arg1, "purchase"))
			exchange_purchase(ch, argument);
		else if (utils::IsAbbr(arg1, "предложения") || utils::IsAbbr(arg1, "offers"))
			exchange_offers(ch, argument);
		else if (utils::IsAbbr(arg1, "фильтрация") || utils::IsAbbr(arg1, "filter"))
			exchange_setfilter(ch, argument);
		else if (utils::IsAbbr(arg1, "save") && (GetRealLevel(ch) >= kLvlImplementator))
			exchange_database_save();
		else if (utils::IsAbbr(arg1, "savebackup") && (GetRealLevel(ch) >= kLvlImplementator))
			exchange_database_save(true);
		else if (utils::IsAbbr(arg1, "reload") && (GetRealLevel(ch) >= kLvlImplementator))
			exchange_database_reload(false);
		else if (utils::IsAbbr(arg1, "reloadbackup") && (GetRealLevel(ch) >= kLvlImplementator))
			exchange_database_reload(true);
		else
			SendMsgToChar(info_message, ch);

		return 1;
	} else
		return 0;

}

int exchange_exhibit(CharData *ch, char *arg) {

	char obj_name[kMaxInputLength];
	int item_cost = 0, lot;
	ObjData *obj;
	char tmpbuf[kMaxInputLength];
	ExchangeItem *item = nullptr;
	ExchangeItem *j, *next_thing = nullptr;
	int counter;
	int counter_ming; //количиство ингридиентов
	int tax;    //налог

	if (!*arg) {
		SendMsgToChar(info_message, ch);
		return false;
	}
	if (GetRealLevel(ch) >= kLvlImmortal && GetRealLevel(ch) < kLvlImplementator) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kImmortalNoEconomy) + "\r\n", ch);
		return false;
	}

	arg = one_argument(arg, obj_name);
	arg = one_argument(arg, arg2);
	if (!*obj_name) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kFormat) + "\r\n", ch);
		return false;
	}
	if (!sscanf(arg2, "%d", &item_cost))
		item_cost = 0;
	if (!*obj_name) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNoItem) + "\r\n", ch);
		return false;
	}
	if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying))) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kDontHave) + "\r\n", ch);
		return false;
	}
	if (!bloody::handle_transfer(ch, nullptr, obj))
		return false;
	if (item_cost > 400000 && obj->has_flag(EObjFlag::kSetItem)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kPriceTooHigh) + "\r\n", ch);
		return false;
	}
	if (obj->get_type() != EObjType::kBook) {
		if (obj->has_flag(EObjFlag::kNorent)
			|| obj->has_flag(EObjFlag::kNosell)
			|| obj->has_flag(EObjFlag::kZonedecay)
			|| obj->has_flag(EObjFlag::kRepopDecay)
			|| obj->get_rnum() < 0) {
			SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNotForExchange) + "\r\n", ch);
			return false;
		}
	}
	if (obj->has_flag(EObjFlag::kDecay) || obj->has_flag(EObjFlag::kNodrop)
		|| obj->get_cost() <= 0
		|| obj->get_owner() > 0) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNotForExchange) + "\r\n", ch);
		return false;
	}
	if (obj->get_contains()) {
		snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kEmptyFirst)), fmt::arg("item", obj->get_PName(grammar::ECase::kAcc))) + "\r\n").c_str());
		SendMsgToChar(tmpbuf, ch);
		return false;
	} else if (SetSystem::is_big_set(obj, true)) {
		snprintf(buf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kBigSet)), fmt::arg("item", obj->get_PName(grammar::ECase::kNom))) + "\r\n").c_str());
		SendMsgToChar(utils::CAP(buf), ch);
		return false;
	}

	if (item_cost <= 0) {
		item_cost = std::max(1, obj->get_cost());
	}

	tax = (obj->get_type() != EObjType::kMagicComponent)
		  ? EXCHANGE_EXHIBIT_PAY + (int) (item_cost * EXCHANGE_EXHIBIT_PAY_COEFF)
		  : (int) (item_cost * EXCHANGE_EXHIBIT_PAY_COEFF / 2);
	if ((currencies::GetTotal(*ch, currencies::kGold) < tax)
		&& (GetRealLevel(ch) < kLvlImplementator)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNoTaxMoney) + "\r\n", ch);
		return false;
	}
	for (j = exchange_item_list, counter = 0, counter_ming = 0;
		 j && (counter + (counter_ming / 20) <= EXCHANGE_MAX_EXHIBIT_PER_CHAR + (remort::GetRealRemort(ch) * 2));
		 j = next_thing) {
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_SELLERID(j) == ch->get_uid()) {
			if (GET_EXCHANGE_ITEM(j)->get_type() != EObjType::kMagicComponent
				&& GET_EXCHANGE_ITEM(j)->get_type() != EObjType::kMagicIngredient) {
				counter++;
			} else {
				counter_ming++;
			}
		}
	}

	if (counter + (counter_ming / 20) >= EXCHANGE_MAX_EXHIBIT_PER_CHAR + (remort::GetRealRemort(ch) * 2)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kMaxItems) + "\r\n", ch);
		return false;
	}

	if ((lot = get_unique_lot()) <= 0) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kBazaarFull) + "\r\n", ch);
		return false;
	}
	item = create_exchange_item();
	GET_EXCHANGE_ITEM_LOT(item) = lot;
	GET_EXCHANGE_ITEM_SELLERID(item) = ch->get_uid();
	GET_EXCHANGE_ITEM_COST(item) = item_cost;
	item->time = time(0);
	skip_spaces(&arg);
	if (*arg)
		GET_EXCHANGE_ITEM_COMMENT(item) = str_dup(arg);
	else
		GET_EXCHANGE_ITEM_COMMENT(item) = nullptr;

	if (stable_objs::IsTimerUnlimited(obj)) // если нерушима таймер 1 неделя
		obj->set_timer(10080);
	GET_EXCHANGE_ITEM(item) = obj;
	RemoveObjFromChar(obj);

	snprintf(tmpbuf, kMaxInputLength, "%s", fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kExhibited)), fmt::arg("lot", GET_EXCHANGE_ITEM_LOT(item)), fmt::arg("amount", item_cost), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(item_cost, grammar::ECase::kNom).c_str())).c_str());
	act(tmpbuf, false, ch, 0, obj, kToChar);
	snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kBcastNew)), fmt::arg("lot", GET_EXCHANGE_ITEM_LOT(item)), fmt::arg("item", obj->get_PName(grammar::ECase::kNom)), fmt::arg("amount", item_cost), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(item_cost, grammar::ECase::kNom).c_str())) + "\r\n").c_str());
	message_exchange(tmpbuf, ch, item);

	currencies::RemoveTotal(*ch, currencies::kGold, tax);

	if (EXCHANGE_SAVEONEVERYOPERATION) {
		exchange_database_save();
		Crash_crashsave(ch);
		ch->save_char();
	}
	return (true);
}

int exchange_change_cost(CharData *ch, char *arg) {
	ExchangeItem *item = nullptr, *j, *next_thing = nullptr;
	int lot, newcost, pay;
	char tmpbuf[kMaxInputLength];

	if (!*arg) {
		SendMsgToChar(info_message, ch);
		return false;
	}
	if (GetRealLevel(ch) >= kLvlImmortal && GetRealLevel(ch) < kLvlImplementator) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kImmortalNoEconomy) + "\r\n", ch);
		return false;
	}
	if (sscanf(arg, "%d %d", &lot, &newcost) != 2) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kCostFormat) + "\r\n", ch);
		return false;
	}
	for (j = exchange_item_list; j && (!item); j = next_thing) {
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_LOT(j) == lot)
			item = j;
	}
	if ((lot < 0) || (!item)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kBadLotNum) + "\r\n", ch);
		return false;
	}
	if ((GET_EXCHANGE_ITEM_SELLERID(item) != ch->get_uid()) && (GetRealLevel(ch) < kLvlImplementator)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNotYourLot) + "\r\n", ch);
		return false;
	}
	if (newcost == GET_EXCHANGE_ITEM_COST(item)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kSameCost) + "\r\n", ch);
		return false;
	}
	if (newcost <= 0) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kBadCost) + "\r\n", ch);
		return false;
	}
	pay = newcost - GET_EXCHANGE_ITEM_COST(item);
	if (pay > 0)
		if ((currencies::GetTotal(*ch, currencies::kGold) < (pay * EXCHANGE_EXHIBIT_PAY_COEFF)) && (GetRealLevel(ch) < kLvlImplementator)) {
			SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNoTaxMoney) + "\r\n", ch);
			return false;
		}

	GET_EXCHANGE_ITEM_COST(item) = newcost;
	if (pay > 0) {
		currencies::RemoveTotal(*ch, currencies::kGold, static_cast<long>(pay * EXCHANGE_EXHIBIT_PAY_COEFF));
	}

	snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kCostSet)), fmt::arg("amount", newcost), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(newcost, grammar::ECase::kNom).c_str()), fmt::arg("item", GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kAcc)), fmt::arg("lot", GET_EXCHANGE_ITEM_LOT(item))) + "\r\n").c_str());
	SendMsgToChar(tmpbuf, ch);
	snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kBcastNewCost)), fmt::arg("lot", GET_EXCHANGE_ITEM_LOT(item)), fmt::arg("item", GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kNom)), fmt::arg("amount", newcost), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(newcost, grammar::ECase::kNom).c_str())) + "\r\n").c_str());
	message_exchange(tmpbuf, ch, item);
	SetWait(ch, 2, false);

	return true;
}

int exchange_withdraw(CharData *ch, char *arg) {
	ExchangeItem *item = nullptr, *j, *next_thing = nullptr;
	int lot;
	char tmpbuf[kMaxInputLength];

	if (!*arg) {
		SendMsgToChar(info_message, ch);
		return false;
	}
	if (GetRealLevel(ch) >= kLvlImmortal && GetRealLevel(ch) < kLvlImplementator) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kImmortalNoEconomy) + "\r\n", ch);
		return false;
	}

	if (!sscanf(arg, "%d", &lot)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNoLotNum) + "\r\n", ch);
		return false;
	}
	for (j = exchange_item_list; j && (!item); j = next_thing) {
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_LOT(j) == lot)
			item = j;
	}
	if ((lot < 0) || (!item)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kBadLotNum) + "\r\n", ch);
		return false;
	}
	if ((GET_EXCHANGE_ITEM_SELLERID(item) != ch->get_uid()) && (GetRealLevel(ch) < kLvlImplementator)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNotYourLot) + "\r\n", ch);
		return false;
	}
	act(specials::ExchMsg(specials::EExchMsg::kWithdrawn), false, ch, nullptr, GET_EXCHANGE_ITEM(item), kToChar);
	if (GET_EXCHANGE_ITEM_SELLERID(item) != ch->get_uid()) {
		snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kBcastWithdrawnGod)), fmt::arg("lot", lot), fmt::arg("item", GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kNom)), fmt::arg("suf", grammar::ObjSexEnding((GET_EXCHANGE_ITEM(item))->get_sex(), 6))) + "\r\n").c_str());
	} else {
		snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kBcastWithdrawnOwner)), fmt::arg("lot", lot), fmt::arg("item", GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kNom)), fmt::arg("suf", grammar::ObjSexEnding((GET_EXCHANGE_ITEM(item))->get_sex(), 6))) + "\r\n").c_str());
	}
	message_exchange(tmpbuf, ch, item);
	if (stable_objs::IsTimerUnlimited(GET_EXCHANGE_ITEM(item))) // если нерушима фрешим таймер из прототипа
		GET_EXCHANGE_ITEM(item)->set_timer(obj_proto.at(GET_EXCHANGE_ITEM(item)->get_rnum())->get_timer());
	PlaceObjToInventory(GET_EXCHANGE_ITEM(item), ch);
	clear_exchange_lot(item);

	if (EXCHANGE_SAVEONEVERYOPERATION) {
		exchange_database_save();
		Crash_crashsave(ch);
		ch->save_char();
	}

	return true;
}

int exchange_information(CharData *ch, char *arg) {
	ExchangeItem *item = nullptr, *j, *next_thing = nullptr;
	int lot;
	char buf2[kMaxInputLength];

	if (!*arg) {
		SendMsgToChar(info_message, ch);
		return false;
	}
	if (!sscanf(arg, "%d", &lot)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNoLotNum) + "\r\n", ch);
		return false;
	}
	for (j = exchange_item_list; j && (!item); j = next_thing) {
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_LOT(j) == lot)
			item = j;
	}
	if ((lot < 0) || (!item)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kBadLotNum) + "\r\n", ch);
		return false;
	}

	std::string out;
	out = fmt::sprintf("Лот %d. Цена %d\r\n", GET_EXCHANGE_ITEM_LOT(item), GET_EXCHANGE_ITEM_COST(item));
	out += fmt::sprintf("Предмет \"%s\", ", GET_EXCHANGE_ITEM(item)->get_short_description().c_str());
	if (GET_EXCHANGE_ITEM(item)->get_type() == EObjType::kWand
		|| GET_EXCHANGE_ITEM(item)->get_type() == EObjType::kStaff) {
		if (GET_EXCHANGE_ITEM(item)->GetPotionValueKey(ObjVal::EValueKey::kCurCharges)
			< GET_EXCHANGE_ITEM(item)->GetPotionValueKey(ObjVal::EValueKey::kMaxCharges)) {
			out += "(б/у), ";
		}
	}
	out += " тип ";
	sprinttype(GET_EXCHANGE_ITEM(item)->get_type(), item_types, buf2);
	if (*buf2) {
		out += fmt::sprintf("%s\n", buf2);
	}
	out += fmt::sprintf("%s%s\r\n", sight::diag_weapon_to_char(GET_EXCHANGE_ITEM(item), true), sight::diag_timer_to_char(GET_EXCHANGE_ITEM(item)));
	{
		char obj_buf[kMaxStringLength];
		snprintf(obj_buf, sizeof(obj_buf), "%s", out.c_str());
		sight::obj_info(ch, GET_EXCHANGE_ITEM(item), obj_buf);
		out = obj_buf;
	}
	out += "\n";
	if (invalid_anti_class(ch, GET_EXCHANGE_ITEM(item)) || invalid_unique(ch, GET_EXCHANGE_ITEM(item))
		|| NamedStuff::check_named(ch, GET_EXCHANGE_ITEM(item), 0)) {
		out += "Эта вещь вам недоступна!\n";
	}
	if (HaveIncompatibleAlign(ch, GET_EXCHANGE_ITEM(item)) || invalid_no_class(ch, GET_EXCHANGE_ITEM(item))) {
		out += "Вы не сможете пользоваться этой вещью.\n";
	}
	auto seller_name = GetNameById(GET_EXCHANGE_ITEM_SELLERID(item));
	snprintf(buf2, sizeof(buf2), "%s", seller_name.empty() ? "(сожран долгоносиком)" : seller_name.c_str());
	*buf2 = UPPER(*buf2);
	out += fmt::sprintf("Продавец %s\n", buf2);
	if (GET_EXCHANGE_ITEM_COMMENT(item)) {
		out += fmt::sprintf("Берестовая наклейка на лоте гласит: '%s'.\n", GET_EXCHANGE_ITEM_COMMENT(item));
	}
	SendMsgToChar(out, ch);
	return true;
}

int exchange_identify(CharData *ch, char *arg) {
	ExchangeItem *item = nullptr, *j, *next_thing = nullptr;
	int lot;
	if (!*arg) {
		SendMsgToChar(info_message, ch);
		return false;
	}
	if (!sscanf(arg, "%d", &lot)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNoLotNum) + "\r\n", ch);
		return false;
	}
	for (j = exchange_item_list; j && (!item); j = next_thing) {
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_LOT(j) == lot)
			item = j;
	}
	if ((lot < 0) || (!item)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kBadLotNum) + "\r\n", ch);
		return false;
	}

	if (GetRealLevel(ch) >= kLvlImmortal && GetRealLevel(ch) < kLvlImplementator) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kIdentImmortal) + "\r\n", ch);
		return false;
	}
	if ((currencies::GetTotal(*ch, currencies::kGold) < (EXCHANGE_IDENT_PAY)) && (GetRealLevel(ch) < kLvlImplementator)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kIdentNoMoney) + "\r\n", ch);
		return false;
	}
	MortShowObjValues(GET_EXCHANGE_ITEM(item), ch, 200);    //400 - полное опознание
	currencies::RemoveTotal(*ch, currencies::kGold, EXCHANGE_IDENT_PAY);
	SendMsgToChar("\r\n" + fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kIdentCharged)), fmt::arg("color", kColorBoldGrn), fmt::arg("amount", EXCHANGE_IDENT_PAY), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(EXCHANGE_IDENT_PAY, grammar::ECase::kNom).c_str()), fmt::arg("nocolor", kColorNrm)) + "\r\n", ch);

	return true;
}

CharData *get_char_by_id(int id) {
	for (const auto &i : character_list) {
		if (!i->IsNpc() && i->get_uid() == id) {
			return i.get();
		}
	}
	return 0;
}

int exchange_purchase(CharData *ch, char *arg) {
	ExchangeItem *item = nullptr, *j, *next_thing = nullptr;
	int lot;
	char tmpbuf[kMaxInputLength];
	CharData *seller;

	if (!*arg) {
		SendMsgToChar(info_message, ch);
		return false;
	}
	if (GetRealLevel(ch) >= kLvlImmortal && GetRealLevel(ch) < kLvlImplementator) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kImmortalNoEconomy) + "\r\n", ch);
		return false;
	}

	if (!sscanf(arg, "%d", &lot)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNoLotNum) + "\r\n", ch);
		return false;
	}
	for (j = exchange_item_list; j && (!item); j = next_thing) {
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_LOT(j) == lot)
			item = j;
	}
	if ((lot < 0) || (!item)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kBadLotNum) + "\r\n", ch);
		return false;
	}
	if (GET_EXCHANGE_ITEM_SELLERID(item) == ch->get_uid()) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kOwnLot) + "\r\n", ch);
		return false;
	}
	if ((currencies::GetTotal(*ch, currencies::kGold) < (GET_EXCHANGE_ITEM_COST(item))) && (GetRealLevel(ch) < kLvlImplementator)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kBankNoMoney) + "\r\n", ch);
		return false;
	}

	seller = get_char_by_id(GET_EXCHANGE_ITEM_SELLERID(item));

	if (seller == nullptr) {
		auto seller_name = GetNameById(GET_EXCHANGE_ITEM_SELLERID(item));

		auto seller_ptr = std::make_unique<Player>();
		seller = seller_ptr.get(); // TODO: переделать на стек
		if (seller_name.empty()
			|| LoadPlayerCharacter(seller_name.c_str(), seller, ELoadCharFlags::kFindId) < 0) {
			currencies::RemoveTotal(*ch, currencies::kGold, GET_EXCHANGE_ITEM_COST(item));

			act(specials::ExchMsg(specials::EExchMsg::kBought), false, ch, 0, GET_EXCHANGE_ITEM(item), kToChar);
			snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kBcastSold)), fmt::arg("lot", lot), fmt::arg("item", GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kNom)), fmt::arg("suf", grammar::ObjSexEnding((GET_EXCHANGE_ITEM(item))->get_sex(), 6)), fmt::arg("amount", GET_EXCHANGE_ITEM_COST(item)), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(GET_EXCHANGE_ITEM_COST(item), grammar::ECase::kNom).c_str())) + "\r\n").c_str());

			message_exchange(tmpbuf, ch, item);
			if (stable_objs::IsTimerUnlimited(GET_EXCHANGE_ITEM(item))) // если нерушима фрешим таймер из прототипа
				GET_EXCHANGE_ITEM(item)->set_timer(obj_proto.at(GET_EXCHANGE_ITEM(item)->get_rnum())->get_timer());
			PlaceObjToInventory(GET_EXCHANGE_ITEM(item), ch);
			clear_exchange_lot(item);
			if (EXCHANGE_SAVEONEVERYOPERATION) {
				exchange_database_save();
				Crash_crashsave(ch);
				ch->save_char();
			}

			return true;
		}

		currencies::AddBank(*seller, currencies::kGold, GET_EXCHANGE_ITEM_COST(item), false, true);
		currencies::RemoveTotal(*ch, currencies::kGold, GET_EXCHANGE_ITEM_COST(item));

		if ((seller)->player_specials->saved.ntfyExchangePrice && GET_EXCHANGE_ITEM_COST(item) >= (seller)->player_specials->saved.ntfyExchangePrice) {
			snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kSellerSold)), fmt::arg("lot", lot), fmt::arg("item", GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kNom)), fmt::arg("suf", grammar::ObjSexEnding((GET_EXCHANGE_ITEM(item))->get_sex(), 6)), fmt::arg("amount", GET_EXCHANGE_ITEM_COST(item)), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(GET_EXCHANGE_ITEM_COST(item), grammar::ECase::kNom).c_str())) + "\r\n").c_str());
			mail::add_by_id(GET_EXCHANGE_ITEM_SELLERID(item), -1, tmpbuf);
		}

		seller->save_char();

		act(specials::ExchMsg(specials::EExchMsg::kBought), false, ch, 0, GET_EXCHANGE_ITEM(item), kToChar);
		snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kBcastSold)), fmt::arg("lot", lot), fmt::arg("item", GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kNom)), fmt::arg("suf", grammar::ObjSexEnding((GET_EXCHANGE_ITEM(item))->get_sex(), 6)), fmt::arg("amount", GET_EXCHANGE_ITEM_COST(item)), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(GET_EXCHANGE_ITEM_COST(item), grammar::ECase::kNom).c_str())) + "\r\n").c_str());
		message_exchange(tmpbuf, ch, item);
		if (stable_objs::IsTimerUnlimited(GET_EXCHANGE_ITEM(item))) // если нерушима фрешим таймер из прототипа
			GET_EXCHANGE_ITEM(item)->set_timer(obj_proto.at(GET_EXCHANGE_ITEM(item)->get_rnum())->get_timer());
		PlaceObjToInventory(GET_EXCHANGE_ITEM(item), ch);
		clear_exchange_lot(item);
		if (EXCHANGE_SAVEONEVERYOPERATION) {
			exchange_database_save();
			Crash_crashsave(ch);
			ch->save_char();
		}

		return true;
	} else {
		currencies::AddBank(*seller, currencies::kGold, GET_EXCHANGE_ITEM_COST(item), false, true);
		currencies::RemoveTotal(*ch, currencies::kGold, GET_EXCHANGE_ITEM_COST(item));

		act(specials::ExchMsg(specials::EExchMsg::kBought), false, ch, 0, GET_EXCHANGE_ITEM(item), kToChar);
		snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kBcastSold)), fmt::arg("lot", lot), fmt::arg("item", GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kNom)), fmt::arg("suf", grammar::ObjSexEnding((GET_EXCHANGE_ITEM(item))->get_sex(), 6)), fmt::arg("amount", GET_EXCHANGE_ITEM_COST(item)), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(GET_EXCHANGE_ITEM_COST(item), grammar::ECase::kNom).c_str())) + "\r\n").c_str());
		message_exchange(tmpbuf, seller, item);
		snprintf(tmpbuf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kSellerSold)), fmt::arg("lot", lot), fmt::arg("item", GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kNom)), fmt::arg("suf", grammar::ObjSexEnding((GET_EXCHANGE_ITEM(item))->get_sex(), 6)), fmt::arg("amount", GET_EXCHANGE_ITEM_COST(item)), fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(GET_EXCHANGE_ITEM_COST(item), grammar::ECase::kNom).c_str())) + "\r\n").c_str());
		act(tmpbuf, false, seller, 0, nullptr, kToChar);

		PlaceObjToInventory(GET_EXCHANGE_ITEM(item), ch);
		clear_exchange_lot(item);
		if (EXCHANGE_SAVEONEVERYOPERATION) {
			exchange_database_save();
			Crash_crashsave(ch);
			ch->save_char();
			Crash_crashsave(seller);
			seller->save_char();
		}

		return true;
	}
}

/**
* Проверка длины фильтра.
* \return false - перебор
*         true - корректный
*/
bool correct_filter_length(const CharData *ch, const char *str) {
	if (strlen(str) >= FILTER_LENGTH) {
		SendMsgToChar(fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kFilterTooLong)), fmt::arg("amount", FILTER_LENGTH - 1)) + "\r\n", ch);
		return false;
	}
	return true;
}

int exchange_offers(const CharData *ch, const char *arg) {
//влом байты считать. Если кто хочет оптимизировать, посчитайте точно.
	std::string filter;
	filter.reserve(kMaxInputLength);
	short int show_type = 0;

	arg = one_argument(arg, arg1);
	if (utils::IsAbbr(arg1, "все") || utils::IsAbbr(arg1, "all")) {
		show_type = 2;
		filter = "М0+";
	} else if (utils::IsAbbr(arg1, "мои") || utils::IsAbbr(arg1, "mine")) {
		show_type = 1;
		filter = "В";
		filter += GET_NAME(ch);
	} else {
		while (*arg1) {
			arg1[0] = UPPER(arg1[0]);
			filter += arg1;
			filter += ' ';
			arg = one_argument(arg, arg1);
		}
	}
	if (show_type == 0 && EXCHANGE_FILTER(ch)) {
		filter = std::string(EXCHANGE_FILTER(ch)) + " " + filter;
	}
	if (!correct_filter_length(ch, filter.c_str())) {
		return 0;
	}

	show_lots(filter.c_str(), show_type, ch);
	return 1;
}

bool exchange_setfilter(CharData *ch, char *argument) {
	if (!*argument) {
		ParseFilter params(ParseFilter::EXCHANGE);
		if (!EXCHANGE_FILTER(ch)) {
			SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kFilterEmpty) + "\r\n", ch);
			params.parse_filter(ch, params, argument);
			return true;
		}
		SendMsgToChar(fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kFilterCurrent)), fmt::arg("filter", EXCHANGE_FILTER(ch))) + "\r\n", ch);
		return true;
	}
	char filter[kMaxInputLength];
	strcpy(filter, argument);
	one_argument(argument, arg);
	if (!correct_filter_length(ch, argument)) {
		return false;
	}
	if (!strncmp(argument, "нет", 3)) {
		if (EXCHANGE_FILTER(ch)) {
			free(EXCHANGE_FILTER(ch));
			EXCHANGE_FILTER(ch) = nullptr;
		}
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kFilterCleared) + "\r\n", ch);
		return true;
	}
	ParseFilter params(ParseFilter::EXCHANGE);
	if (!params.parse_filter(ch, params, argument)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kFilterBadFormat) + "\r\n", ch);
		snprintf(buf, kMaxInputLength, "%s", (fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kFilterCurrentShort)), fmt::arg("filter", params.print())) + "\r\n").c_str());
		char tmps[1] = ""; //обход варнинга
		params.parse_filter(ch, params, tmps);
		SendMsgToChar(buf, ch);
		free(EXCHANGE_FILTER(ch));
		EXCHANGE_FILTER(ch) = nullptr;
		return false;
	}
	SendMsgToChar(fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kFilterYours)), fmt::arg("filter", params.print())) + "\r\n", ch);
	if (EXCHANGE_FILTER(ch))
		free(EXCHANGE_FILTER(ch));
	EXCHANGE_FILTER(ch) = str_dup(filter);
	return true;
}

ExchangeItem *create_exchange_item(void) {
	ExchangeItem *item;
	CREATE(item, 1);
	GET_EXCHANGE_ITEM_LOT(item) = -1;
	GET_EXCHANGE_ITEM_SELLERID(item) = -1;
	GET_EXCHANGE_ITEM_COST(item) = 0;
	item->time = time(nullptr);
	GET_EXCHANGE_ITEM_COMMENT(item) = nullptr;
	GET_EXCHANGE_ITEM(item) = nullptr;
	item->next = exchange_item_list;
	exchange_item_list = item;
	return (item);
}

void extract_exchange_item(ExchangeItem *item) {
	REMOVE_FROM_LIST(item, exchange_item_list);
	if (GET_EXCHANGE_ITEM_LOT(item) > 0 && GET_EXCHANGE_ITEM_LOT(item) <= (int) lot_usage.size())
		lot_usage[GET_EXCHANGE_ITEM_LOT(item) - 1] = false;
	if (item->comment)
		free(item->comment);
	free(item);

}

void check_exchange(ObjData *obj) {
	if (!obj) {
		return;
	}
	ExchangeItem *j, *next_thing;

	for (j = exchange_item_list; j; j = next_thing) {
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM(j) == obj) {
			REMOVE_FROM_LIST(j, exchange_item_list);
			lot_usage[GET_EXCHANGE_ITEM_LOT(j) - 1] = false;
			if (j->comment) {
				free(j->comment);
			}
			free(j);
			break;
		}
	}
}

ExchangeItem *exchange_read_one_object_new(char **data, int *error) {
	char buffer[kMaxStringLength];
	ExchangeItem *item = nullptr;
	//char *d;

	*error = 1;
	// Станем на начало предмета
	for (; **data != EX_NEW_ITEM_CHAR; (*data)++)
		if (!**data || **data == EX_END_CHAR)
			return (item);

	*error = 2;

	// Пропустим #
	(*data)++;

	// Считаем lot предмета
	if (!get_buf_line(data, buffer))
		return (item);

	*error = 3;
	item = create_exchange_item();
	if ((GET_EXCHANGE_ITEM_LOT(item) = atoi(buffer)) <= 0)
		return (item);

	*error = 4;
	// Считаем seler_id предмета
	if (!get_buf_line(data, buffer))
		return (item);
	*error = 5;
	if (!(GET_EXCHANGE_ITEM_SELLERID(item) = atoi(buffer)))
		return (item);
	*error = 6;
	// Считаем цену предмета
	if (!get_buf_line(data, buffer))
		return (item);

	*error = 7;
	if (!(GET_EXCHANGE_ITEM_COST(item) = atoi(buffer)))
		return (item);

	if (!get_buf_line(data, buffer))
		return (item);
	*error = 8;
	if (!(item->time = atoi(buffer)))
		return (item);
	*error = 9;
	// Считаем comment предмета
	char *str_last_symb = strchr(*data, '\n');
	size_t copy_len = static_cast<size_t>(str_last_symb - *data);
	if (copy_len >= sizeof(buffer)) copy_len = sizeof(buffer) - 1;
	memcpy(buffer, *data, copy_len);
	buffer[copy_len] = '\0';
	*data = str_last_symb;

	if (strcmp(buffer, "EMPTY"))
		if (!(GET_EXCHANGE_ITEM_COMMENT(item) = str_dup(buffer)))
			return (item);

	*error = 0;

	GET_EXCHANGE_ITEM(item) = read_one_object_new(data, error).get();
	if (*error) {
		*error = 9;
	}

	return (item);
}

void exchange_write_one_object_new(std::stringstream &out, ExchangeItem *item) {
	out << "#" << GET_EXCHANGE_ITEM_LOT(item) << "\n"
		<< GET_EXCHANGE_ITEM_SELLERID(item) << "\n"
		<< GET_EXCHANGE_ITEM_COST(item) << "\n"
		<< item->time << "\n"
		<< (GET_EXCHANGE_ITEM_COMMENT(item) ? GET_EXCHANGE_ITEM_COMMENT(item) : "EMPTY\n")
		<< "\n";
	write_one_object(out, GET_EXCHANGE_ITEM(item), 0);
	out << "\n";
}

int LoadExchange() {
	FILE *fl;
	char *data, *readdata;
	ExchangeItem *item;
	int fsize, error, max_lot = 0;
	char buffer[kMaxStringLength];

	log("Exchange: loading database... (exchange.cpp)");

	if (!(fl = fopen(EXCHANGE_DATABASE_FILE, "r"))) {
		log("SYSERR: Error opening exchange database. (exchange.cpp)");
		return (0);
	}

	fseek(fl, 0L, SEEK_END);
	fsize = ftell(fl);

	CREATE(readdata, fsize + 1);
	fseek(fl, 0L, SEEK_SET);
	auto actual_size = fread(readdata, 1, fsize, fl);
	if (!actual_size || ferror(fl)) {
		fclose(fl);
		log("SYSERR: Memory error or cann't read exchange database file. (exchange.cpp)");
		free(readdata);
		return (0);
	};
	fclose(fl);

	data = readdata;
	*(data + actual_size) = '\0';

	// Новая база или старая?
	get_buf_line(&data, buffer);
	if (strstr(buffer, "!NEW!") == nullptr) {
		log("SYSERR: Old format exchange database file.");
		return 0;
	}

	for (fsize = 0; *data && *data != EX_END_CHAR; fsize++) {
		item = exchange_read_one_object_new(&data, &error);

		if (item == nullptr) {
			log("SYSERR: Error #%d reading exchange database file. (exchange.cpp)", error);
			return (0);
		}

		if (error) {
			log("SYSERR: Error #%d reading item from exchange database.", error);
			extract_exchange_item(item);
			continue;
		}

		// Предмет разваливается от старости
		if (GET_EXCHANGE_ITEM(item)->get_timer() <= 0) {
			std::string cap = GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kNom);
			cap[0] = UPPER(cap[0]);
			log("Exchange: - %s рассыпал%s от длительного использования.\r\n",
				cap.c_str(), grammar::ObjSexEnding((GET_EXCHANGE_ITEM(item))->get_sex(), 2));
			extract_exchange_item(item);
			continue;
		}
		max_lot = std::max(max_lot, GET_EXCHANGE_ITEM_LOT(item));
	}

	free(readdata);

	lot_usage.resize(max_lot, false);
	for (item = exchange_item_list; item; item = item->next)
		lot_usage[GET_EXCHANGE_ITEM_LOT(item) - 1] = true;

	log("Exchange: done loading database.");

	return (1);
}

int exchange_database_reload(bool loadbackup) {
	FILE *fl;
	char *data, *readdata;
	ExchangeItem *item, *j, *next_thing;
	int fsize, error, max_lot = 0;
	char buffer[kMaxStringLength];

	for (j = exchange_item_list; j; j = next_thing) {
		next_thing = j->next;
		extract_exchange_item(j);
	}
	if (loadbackup) {
		log("Exchange: reloading backup of database... (exchange.cpp)");
		if (!(fl = fopen(EXCHANGE_DATABASE_BACKUPFILE, "r"))) {
			log("SYSERR: Error opening exchange database backup. (exchange.cpp)");
			return (0);
		}
	} else {
		log("Exchange: reloading database... (exchange.cpp)");
		if (!(fl = fopen(EXCHANGE_DATABASE_FILE, "r"))) {
			log("SYSERR: Error opening exchange database. (exchange.cpp)");
			return (0);
		}
	}

	fseek(fl, 0L, SEEK_END);
	fsize = ftell(fl);

	CREATE(readdata, fsize + 1);
	fseek(fl, 0L, SEEK_SET);
	auto actual_size = fread(readdata, 1, fsize, fl);
	if (!actual_size || ferror(fl)) {
		fclose(fl);
		if (loadbackup)
			log("SYSERR: Memory error or cann't read exchange database backup file. (exchange.cpp)");
		else
			log("SYSERR: Memory error or cann't read exchange database file. (exchange.cpp)");
		free(readdata);
		return (0);
	};
	fclose(fl);

	data = readdata;
	*(data + actual_size) = '\0';

	// Новая база или старая?
	get_buf_line(&data, buffer);
	if (strstr(buffer, "!NEW!") == nullptr) {
		log("SYSERR: Old format exchange database file.");
		return 0;
	}

	for (fsize = 0; *data && *data != EX_END_CHAR; fsize++) {
		item = exchange_read_one_object_new(&data, &error);

		if (item == nullptr) {
			if (loadbackup)
				log("SYSERR: Error #%d reading exchange database backup file. (exchange.cpp)", error);
			else
				log("SYSERR: Error #%d reading exchange database file. (exchange.cpp)", error);
			return (0);
		}

		if (error) {
			if (loadbackup)
				log("SYSERR: Error #%d reading item from exchange database backup.", error);
			else
				log("SYSERR: Error #%d reading item from exchange database.", error);
			extract_exchange_item(item);
			continue;
		}

		// Предмет разваливается от старости
		if (GET_EXCHANGE_ITEM(item)->get_timer() <= 0) {
			std::string cap = GET_EXCHANGE_ITEM(item)->get_PName(grammar::ECase::kNom);
			cap[0] = UPPER(cap[0]);
			log("Exchange: - %s рассыпал%s от длительного использования.\r\n",
				cap.c_str(), grammar::ObjSexEnding((GET_EXCHANGE_ITEM(item))->get_sex(), 2));
			extract_exchange_item(item);
			continue;
		}
		max_lot = std::max(max_lot, GET_EXCHANGE_ITEM_LOT(item));
	}

	free(readdata);

	lot_usage.resize(max_lot, false);
	for (item = exchange_item_list; item; item = item->next)
		lot_usage[GET_EXCHANGE_ITEM_LOT(item) - 1] = true;

	if (loadbackup)
		log("Exchange: done reloading database backup.");
	else
		log("Exchange: done reloading database.");

	return (1);
}

/**
* \param backup - false (дефолт) обычное сохранение
*               - true сохранение в бэкап, вместо основного файла
*/
void exchange_database_save(bool backup) {
	log("Exchange: Saving exchange database...");

	std::stringstream out;
	out << "!NEW!\n";
	ExchangeItem *j, *next_thing;
	for (j = exchange_item_list; j; j = next_thing) {
		next_thing = j->next;
		exchange_write_one_object_new(out, j);
	}
	out << "$\n$\n";

	const char *filename = backup ? EXCHANGE_DATABASE_BACKUPFILE : EXCHANGE_DATABASE_FILE;
	std::ofstream file(filename);
	if (!file.is_open()) {
		sprintf(buf, "[SYSERROR] open exchange db ('%s')", filename);
		mudlog(buf, BRF, kLvlImmortal, SYSLOG, true);
		return;
	}
	file << out.rdbuf();
	file.close();

	log("Exchange: done saving database.");
	return;
}

int get_unique_lot(void) {
	int i;
	for (i = 0; i < (int) lot_usage.size(); i++)
		if (!lot_usage[i]) {
			lot_usage[i] = true;
			return i + 1;
		}

	if (lot_usage.size() < INT_MAX) {
		lot_usage.push_back(true);
		return i + 1;
	} else
		return -1;
}

void message_exchange(char *message, CharData *ch, ExchangeItem *j) {
	for (DescriptorData *i = descriptor_list; i; i = i->next) {
		if  (i->state == EConState::kPlaying
			&& (!ch || i != ch->desc)
			&& i->character
			&& !i->character->IsFlagged(EPrf::kNoExchange)
			&& !i->character->IsFlagged(EPlrFlag::kWriting)
			&& !ROOM_FLAGGED(i->character->in_room, ERoomFlag::kSoundproof)
			&& i->character->GetPosition() > EPosition::kSleep) {
			if (!i->character->IsFlagged(EPrf::kNoIngrMode)
				&& (GET_EXCHANGE_ITEM(j)->get_type() == EObjType::kMagicIngredient
					|| GET_EXCHANGE_ITEM(j)->get_type() == EObjType::kMagicComponent)) {
				continue;
			}
			ParseFilter params(ParseFilter::EXCHANGE);
			if (!EXCHANGE_FILTER(i->character) 
					|| (EXCHANGE_FILTER(i->character) && params.parse_filter(nullptr, params, EXCHANGE_FILTER(i->character))
					&& params.check(j))) {
				SendMsgToChar(i->character.get(), "&Y%s&n", message);
			}
		}
	}
}

void show_lots(const char *filter, short int show_type, const CharData *ch) {
	char tmpbuf[kMaxInputLength];
	bool any_item = 0;

	ParseFilter params(ParseFilter::EXCHANGE);
	if (!params.parse_filter(ch, params, filter)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kBadFilterString) + "\r\n", ch);
		log("Exchange: Player uses wrong filter '%s'", filter);
		return;
	}

	std::string buffer;
	SendMsgToChar(fmt::format(fmt::runtime(specials::ExchMsg(specials::EExchMsg::kFilterYours)), fmt::arg("filter", params.print())) + "\r\n", ch);
	if (privilege::IsGod(ch)) {
		buffer =
			"vnum    Лот      Предмет                                                     Цена  Состояние\r\n"
			"--------------------------------------------------------------------------------------------\r\n";
	} else {
		buffer =
			"Лот      Предмет                                                     Цена  Состояние\r\n"
			"--------------------------------------------------------------------------------------------\r\n";
	}
	// Буферы заводим один раз и переиспользуем (clear сохраняет capacity),
	// чтобы по совету Квирунда не реаллоцировать на каждой итерации, как
	// вёл бы себя char[kMaxInputLength].
	std::string item;
	item.reserve(kMaxInputLength);
	std::string aff;
	aff.reserve(kMaxInputLength);
	for (ExchangeItem *j = exchange_item_list; j; j = j->next) {
		if (show_type == 1 && !isname(GET_NAME(ch), GetNameById(GET_EXCHANGE_ITEM_SELLERID(j))))
			continue;
		item.clear();
		// ну идиотизм сидеть статить 5-10 страниц резных
		// Идиотизм - это тупым хардком по названию предмета вот так проверять.
		// Использовать флаг или добавить новый - лапки, да?
		if (utils::IsAbbr("резное запястье", GET_EXCHANGE_ITEM(j)->get_PName(grammar::ECase::kNom).c_str())
			|| utils::IsAbbr("широкое серебряное обручье", GET_EXCHANGE_ITEM(j)->get_PName(grammar::ECase::kNom).c_str())
			|| utils::IsAbbr("медное запястье", GET_EXCHANGE_ITEM(j)->get_PName(grammar::ECase::kNom).c_str())) {
			char affbuf[kMaxStringLength];
			GET_EXCHANGE_ITEM(j)->get_affect_flags().sprintbits(weapon_affects, affbuf, sizeof(affbuf), ",");
			// небольшое дублирование кода, чтобы зря не гонять по аффектам всех шмоток
			aff.assign(affbuf);
			if (aff == "ничего") {
				aff.clear();
				for (int n = 0; n < kMaxObjAffect; n++) {
					auto drndice = GET_EXCHANGE_ITEM(j)->get_affected(n).location;
					int drsdice = GET_EXCHANGE_ITEM(j)->get_affected(n).modifier;
					if ((drndice != EApply::kNone) && (drsdice != 0)) {
						bool negative = IsNegativeApply(drndice);
						if (drsdice < 0)
							negative = !negative;
						sprinttype(drndice, apply_types, buf2);
						fmt::format_to(std::back_inserter(aff), "{} {}{}", buf2, negative ? "-" : "+", abs(drsdice));
						break;
					}
				}
			}
			if (aff.empty()) {
				fmt::format_to(std::back_inserter(item), "[{:4}]   {}",
						GET_EXCHANGE_ITEM_LOT(j),
						GET_EXCHANGE_ITEM(j)->get_PName(grammar::ECase::kNom).c_str());
			} else {
				fmt::format_to(std::back_inserter(item), "[{:4}]   {} ({})",
						GET_EXCHANGE_ITEM_LOT(j),
						GET_EXCHANGE_ITEM(j)->get_PName(grammar::ECase::kNom).c_str(),
						aff);
			}
		} else if (is_dig_stone(GET_EXCHANGE_ITEM(j))
			&& GET_EXCHANGE_ITEM(j)->get_material() == EObjMaterial::kGlass) {
			fmt::format_to(std::back_inserter(item), "[{:4}]   {} (стекло)",
					GET_EXCHANGE_ITEM_LOT(j),
					GET_EXCHANGE_ITEM(j)->get_PName(grammar::ECase::kNom).c_str());
		} else {
			fmt::format_to(std::back_inserter(item), "[{:4}]   {}{}",
					GET_EXCHANGE_ITEM_LOT(j),
					GET_EXCHANGE_ITEM(j)->get_PName(grammar::ECase::kNom).c_str(),
					params.show_obj_aff(GET_EXCHANGE_ITEM(j)));
		}
		char *tmstr;
		tmstr = (char *) asctime(localtime(&(j->time)));
		if (privilege::IsGod(ch)) {//asctime добавляет перевод строки лишний
			snprintf(tmpbuf, sizeof(tmpbuf),
					"(%5d) %s %9d  %-s %s", GET_EXCHANGE_ITEM(j)->get_vnum(),
					colored_name(item.c_str(), 63, true),
					GET_EXCHANGE_ITEM_COST(j),
					sight::diag_obj_timer(GET_EXCHANGE_ITEM(j)),
					tmstr);
		} else {
			snprintf(tmpbuf, sizeof(tmpbuf),
					"%s %9d  %-s\r\n",
					colored_name(item.c_str(), 63, true),
					GET_EXCHANGE_ITEM_COST(j),
					sight::diag_obj_timer(GET_EXCHANGE_ITEM(j)));
			}
		// Такое вот кино, на выделения для каждой строчки тут уходило до 0.6 секунды при выводе всего базара. стринги рулят -- Krodo
		if (params.check(j)) {
			buffer += tmpbuf;
		}
		any_item = 1;
	}

	if (!any_item) {
		buffer = specials::ExchMsg(specials::EExchMsg::kBazaarEmpty) + "\r\n";
	}
	page_string(ch->desc, buffer);
}

void clear_exchange_lot(ExchangeItem *lot) {
	if (lot == nullptr) {
		return;
	}

	REMOVE_FROM_LIST(lot, exchange_item_list);
	lot_usage[GET_EXCHANGE_ITEM_LOT(lot) - 1] = false;
	if (lot->comment) {
		free(lot->comment);
	}
	free(lot);
}

void do_exchange(CharData *ch, char *argument, int cmd, int/* subcmd*/) {
	char *arg = str_dup(argument);
	argument = one_argument(argument, arg1);

	if (ch->IsNpc()) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNotHuman) + "\r\n", ch);
	} else if ((utils::IsAbbr(arg1, "выставить") || utils::IsAbbr(arg1, "exhibit")
		|| utils::IsAbbr(arg1, "цена") || utils::IsAbbr(arg1, "cost")
		|| utils::IsAbbr(arg1, "снять") || utils::IsAbbr(arg1, "withdraw")
		|| utils::IsAbbr(arg1, "купить") || utils::IsAbbr(arg1, "purchase")) && !privilege::IsImpl(ch)
		&& !specials::IsMobSpecialInRoom(ch->in_room, specials::ESpecial::kExchange)) {
		SendMsgToChar(specials::ExchMsg(specials::EExchMsg::kNeedTrader) + "\r\n", ch);
	} else
		exchange(ch, nullptr, cmd, arg);
	free(arg);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
