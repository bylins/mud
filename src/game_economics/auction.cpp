/**************************************************************************
*   File: Auction.cpp                                  Part of Bylins     *
*  Usage: Auction functions used by the MUD                               *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "auction.h"

#include "entities/obj_data.h"
#include "color.h"
#include "handler.h"
#include "game_mechanics/named_stuff.h"
#include "fightsystem/pk.h"

const int kMaxAuctionLot = 3;
const int kMaxAuctionTactBuy = 5;
const int kAuctionPulses = 30;
constexpr int kMaxAuctionTact = kMaxAuctionTactBuy + 3;

// external functions
extern int invalid_anti_class(CharData *ch, const ObjData *obj);
extern int invalid_unique(CharData *ch, const ObjData *obj);
extern int invalid_no_class(CharData *ch, const ObjData *obj);
extern int invalid_align(CharData *ch, ObjData *obj);
extern char *diag_weapon_to_char(const CObjectPrototype *obj, int show_wear);
extern char *diag_timer_to_char(const ObjData *obj);
extern void SetWait(CharData *ch, int waittime, int victim_in_room);
extern void obj_info(CharData *ch, ObjData *obj, char buf[kMaxStringLength]);
extern void mort_show_obj_values(const ObjData *obj, CharData *ch, int fullness, bool enhansed_scroll);

AuctionItem auction_lots[kMaxAuctionLot] = {{-1, nullptr, -1, nullptr, -1, nullptr, -1, nullptr, 0, 0},
											{-1, nullptr, -1, nullptr, -1, nullptr, -1, nullptr, 0, 0},
											{-1, nullptr, -1, nullptr, -1, nullptr, -1, nullptr, 0, 0}    /*,
	{-1, nullptr, -1, nullptr, -1, nullptr, -1, nullptr, 0, 0},
	{-1, nullptr, -1, nullptr, -1, nullptr, -1, nullptr, 0, 0}  */
};

const char *tact_message[] = {"РАЗ!",
							  "ДВА!!",
							  "Т-Р-Р-Р-И!!!",
							  "ЧЕТЫРЕ!!!!",
							  "ПЯТЬ!!!!!",
							  "\n"
};

const char *auction_cmd[] = {"поставить", "set",
							 "снять", "close",
							 "ставка", "value",
							 "продать", "sell",
							 "транспорт", "transport",
							 "рассмотреть", "examine",
							 "характеристики", "identify",
							 "\n"
};

void showlots(CharData *ch) {
	char tmpbuf[kMaxInputLength];

	CharData *sch;
	//CharacterData *bch;
	ObjData *obj;

	for (int i = 0; i < kMaxAuctionLot; i++) {
		sch = GET_LOT(i)->seller;
		//bch = GET_LOT(i)->buyer;
		obj = GET_LOT(i)->item;

		if (!sch || !obj) {
			send_to_char(ch, "Аукцион : лот %2d - свободен.\r\n", i);
			continue;
		}
		if (GET_LOT(i)->prefect && GET_LOT(i)->prefect != ch) {
			sprintf(tmpbuf, "Аукцион : лот %2d - %s%s%s (частный заказ).\r\n",
					i, CCIYEL(ch, C_NRM), obj->get_PName(0).c_str(), CCNRM(ch, C_NRM));
			send_to_char(tmpbuf, ch);
			continue;
		}

		sprintf(tmpbuf, "Аукцион : лот %2d - %s%s%s - ставка %d %s, попытка %d, владелец %s.\r\n",
				i, CCIYEL(ch, C_NRM), obj->get_PName(0).c_str(), CCNRM(ch, C_NRM),
				GET_LOT(i)->cost, GetDeclensionInNumber(GET_LOT(i)->cost, EWhat::kMoneyA),
				GET_LOT(i)->tact < 0 ? 1 : GET_LOT(i)->tact + 1, GET_NAME(sch));

		if (GET_LOT(i)->prefect && GET_LOT(i)->prefect_unique == GET_UNIQUE(ch)) {
			strcat(tmpbuf, "(Специально для вас).\r\n");
		}
		send_to_char(tmpbuf, ch);
	}
}

bool auction_drive(CharData *ch, char *argument) {
	int mode = -1, value = -1, lot = -1;
	CharData *tch = nullptr;
	AuctionItem *lotis;
	ObjData *obj;
	char operation[kMaxInputLength], whom[kMaxInputLength];
	char tmpbuf[kMaxInputLength];

	if (!*argument) {
		showlots(ch);
		return false;
	}
	argument = one_argument(argument, operation);
	if ((mode = search_block(operation, auction_cmd, false)) < 0) {
		send_to_char("Команды аукциона : поставить, снять, ставка, продать, транспорт, характеристики, рассмотреть.\r\n",
					 ch);
		return false;
	}
	mode >>= 1;
	switch (mode) {
		case 0:        // Set lot
			if (!(lotis = free_auction(&lot))) {
				send_to_char("Нет свободных брокеров.\r\n", ch);
				return false;
			}
			*operation = '\0';
			*whom = '\0';
			if (!sscanf(argument, "%s %d %s", operation, &value, whom)) {
				send_to_char("Формат: аукцион поставить вещь [нач. ставка] [для кого]\r\n", ch);
				return false;
			}
			if (!*operation) {
				send_to_char("Не указан предмет.\r\n", ch);
				return false;
			}
			if (!(obj = get_obj_in_list_vis(ch, operation, ch->carrying))) {
				send_to_char("У вас этого нет.\r\n", ch);
				return false;
			}
			if (GET_OBJ_TYPE(obj) != EObjType::ITEM_BOOK) {
				if (obj->has_flag(EObjFlag::kNorent)
					|| obj->has_flag(EObjFlag::kNosell)) {
					send_to_char("Этот предмет не предназначен для аукциона.\r\n", ch);
					return false;
				}
			}
			if (obj->has_flag(EObjFlag::kDecay)
				|| obj->has_flag(EObjFlag::kNodrop)
				|| GET_OBJ_COST(obj) <= 0
				|| obj->get_owner() > 0) {
				send_to_char("Этот предмет не предназначен для аукциона.\r\n", ch);
				return false;
			}
			if (obj_on_auction(obj)) {
				send_to_char("Вы уже поставили на аукцион этот предмет.\r\n", ch);
				return false;
			}
			if (obj->get_contains()) {
				sprintf(tmpbuf, "Опустошите %s перед продажей.\r\n", obj->get_PName(3).c_str());
				send_to_char(tmpbuf, ch);
				return false;
			}
/*			if (IS_GOD(ch)) {
				sprintf(tmpbuf, "&CДелай что-нибудь полезное для мада, фридроп или мобу подложи эту штуку!\n\r\n");
				send_to_char(tmpbuf, ch);
				return false;
			}
*/
			if (value <= 0) {
				value = MAX(1, GET_OBJ_COST(obj));
			};
			if (*whom) {
				if (!(tch = get_player_vis(ch, whom, FIND_CHAR_WORLD))) {
					send_to_char("Вы не видите этого игрока.\r\n", ch);
					return false;
				}
				/*	  if (IS_NPC (tch))
						{
						  send_to_char ("О этом персонаже позаботятся Боги.\r\n", ch);
						  return false;
						}*/
				if (ch == tch) {
					send_to_char("Но это же вы!\r\n", ch);
					return false;
				}
			};
			lotis->item_id = obj->get_id();
			lotis->item = obj;
			lotis->cost = value;
			lotis->tact = -1;
			lotis->seller_unique = GET_UNIQUE(ch);
			lotis->seller = ch;
			lotis->buyer_unique = lotis->prefect_unique = -1;
			lotis->buyer = lotis->prefect = nullptr;
			if (tch) {
				lotis->prefect_unique = GET_UNIQUE(tch);
				lotis->prefect = tch;
			}

			if (tch) {
				sprintf(tmpbuf, "Вы выставили на аукцион $O3 за %d %s (для %s)",
						value, GetDeclensionInNumber(value, EWhat::kMoneyU), GET_PAD(tch, 1));
			} else {
				sprintf(tmpbuf, "Вы выставили на аукцион $O3 за %d %s", value,
						GetDeclensionInNumber(value, EWhat::kMoneyU));
			}
			act(tmpbuf, false, ch, 0, obj, kToChar);
			sprintf(tmpbuf,
					"Аукцион : новый лот %d - %s - начальная ставка %d %s. \r\n",
					lot, obj->get_PName(0).c_str(), value, GetDeclensionInNumber(value, EWhat::kMoneyA));
			message_auction(tmpbuf, nullptr);
			SetWait(ch, 1, false);
			return true;
			break;
		case 1:        // Close
			if (!sscanf(argument, "%d", &lot)) {
				send_to_char("Не указан номер лота.\r\n", ch);
				return false;
			}
			if (lot < 0 || lot >= kMaxAuctionLot) {
				send_to_char("Неверный номер лота.\r\n", ch);
				return false;
			}
			if (GET_LOT(lot)->seller != ch || GET_LOT(lot)->seller_unique != GET_UNIQUE(ch)) {
				send_to_char("Это не ваш лот.\r\n", ch);
				return false;
			}
			act("Вы сняли $O3 с аукциона.\r\n", false, ch, 0, GET_LOT(lot)->item, kToChar);
			sprintf(tmpbuf, "Аукцион : лот %d(%s) снят%s с аукциона владельцем.\r\n", lot,
					GET_LOT(lot)->item->get_PName(0).c_str(), GET_OBJ_SUF_6(GET_LOT(lot)->item));
			clear_auction(lot);
			message_auction(tmpbuf, nullptr);
			SetWait(ch, 1, false);
			return true;
			break;
		case 2:        // Set
			if (sscanf(argument, "%d %d", &lot, &value) != 2) {
				send_to_char("Формат: аукцион ставка лот новая.цена\r\n", ch);
				return false;
			}
			if (lot < 0 || lot >= kMaxAuctionLot) {
				send_to_char("Неверный номер лота.\r\n", ch);
				return false;
			}
			if (!GET_LOT(lot)->item || GET_LOT(lot)->item_id <= 0 ||
				!GET_LOT(lot)->seller || GET_LOT(lot)->seller_unique <= 0) {
				send_to_char("Лот пуст.\r\n", ch);
				return false;
			}
			if (GET_LOT(lot)->seller == ch || GET_LOT(lot)->seller_unique == GET_UNIQUE(ch)) {
				send_to_char("Но это же ваш лот!\r\n", ch);
				return false;
			}
			if (GET_LOT(lot)->prefect && GET_LOT(lot)->prefect_unique > 0 &&
				(GET_LOT(lot)->prefect != ch || GET_LOT(lot)->prefect_unique != GET_UNIQUE(ch))) {
				send_to_char("Этот лот имеет другого покупателя.\r\n", ch);
				return false;
			}
			if (GET_LOT(lot)->item->get_carried_by() != GET_LOT(lot)->seller) {
				send_to_char("Вещь утеряна владельцем.\r\n", ch);
				sprintf(tmpbuf, "Аукцион : лот %d (%s) снят, ввиду смены владельца.", lot,
						GET_LOT(lot)->item->get_PName(0).c_str());
				clear_auction(lot);
				message_auction(tmpbuf, nullptr);
				return true;
			}
			if (value < GET_LOT(lot)->cost) {
				send_to_char("Ваша ставка ниже текущей.\r\n", ch);
				return false;
			}
			if (GET_LOT(lot)->buyer && value < GET_LOT(lot)->cost + MAX(1, GET_LOT(lot)->cost / 20)) {
				send_to_char("Повышайте ставку не ниже 5% текущей.\r\n", ch);
				return false;
			}
			if (value > ch->get_gold() + ch->get_bank()) {
				send_to_char("У вас нет такой суммы.\r\n", ch);
				return false;
			}
			GET_LOT(lot)->cost = value;
			GET_LOT(lot)->tact = -1;
			GET_LOT(lot)->buyer = ch;
			GET_LOT(lot)->buyer_unique = GET_UNIQUE(ch);
			sprintf(tmpbuf, "Хорошо, вы согласны заплатить %d %s за %s (лот %d).\r\n",
					value, GetDeclensionInNumber(value, EWhat::kMoneyU), GET_LOT(lot)->item->get_PName(3).c_str(), lot);
			send_to_char(tmpbuf, ch);
			sprintf(tmpbuf,
					"Принята ставка %s на лот %d(%s) %d %s.\r\n",
					GET_PAD(ch, 1),
					lot,
					GET_LOT(lot)->item->get_PName(0).c_str(),
					value,
					GetDeclensionInNumber(value, EWhat::kMoneyA));
			send_to_char(tmpbuf, GET_LOT(lot)->seller);
			sprintf(tmpbuf, "Аукцион : лот %d(%s) - новая ставка %d %s.", lot,
					GET_LOT(lot)->item->get_PName(0).c_str(), value, GetDeclensionInNumber(value, EWhat::kMoneyA));
			message_auction(tmpbuf, nullptr);
			SetWait(ch, 1, false);
			return true;
			break;

		case 3:        // Sell
			if (!sscanf(argument, "%d", &lot)) {
				send_to_char("Не указан номер лота.\r\n", ch);
				return false;
			}
			if (lot < 0 || lot >= kMaxAuctionLot) {
				send_to_char("Неверный номер лота.\r\n", ch);
				return false;
			}
			if (GET_LOT(lot)->seller != ch || GET_LOT(lot)->seller_unique != GET_UNIQUE(ch)) {
				send_to_char("Это не ваш лот.\r\n", ch);
				return false;
			}
			if (!GET_LOT(lot)->buyer) {
				send_to_char("Покупателя на ваш товар пока нет.\r\n", ch);
				return false;
			}

			GET_LOT(lot)->prefect = GET_LOT(lot)->buyer;
			GET_LOT(lot)->prefect_unique = GET_LOT(lot)->buyer_unique;
			if (GET_LOT(lot)->tact < kMaxAuctionTactBuy) {
				sprintf(whom, "Аукцион : лот %d(%s) продан с аукциона за %d %s.",
						lot, GET_LOT(lot)->item->get_PName(0).c_str(), GET_LOT(lot)->cost,
						GetDeclensionInNumber(GET_LOT(lot)->cost, EWhat::kMoneyU));
				GET_LOT(lot)->tact = kMaxAuctionTactBuy;
			} else
				*whom = '\0';
			sell_auction(lot);
			if (*whom) {
				strcpy(tmpbuf, whom);
				message_auction(tmpbuf, nullptr);
				return true;
			}
			SetWait(ch, 1, false);
			return false;
			break;
		case 4:        // Transport //
			if (!sscanf(argument, "%d", &lot)) {
				send_to_char("Не указан номер лота для передачи.\r\n", ch);
				return false;
			}
			if (lot < 0 || lot >= kMaxAuctionLot) {
				send_to_char("Неверный номер лота.\r\n", ch);
				return false;
			}
			if (!GET_LOT(lot)->item ||
				GET_LOT(lot)->item_id <= 0 || !GET_LOT(lot)->seller || GET_LOT(lot)->seller_unique <= 0) {
				send_to_char("Лот пуст.\r\n", ch);
				return false;
			}

			if (GET_LOT(lot)->seller == ch && GET_LOT(lot)->seller_unique == GET_UNIQUE(ch)) {
				send_to_char("Это может сделать только покупатель.\r\n", ch);
				return false;
			}

			if (GET_LOT(lot)->prefect != ch || GET_LOT(lot)->prefect_unique != GET_UNIQUE(ch)) {
				send_to_char("Вы еще не купили этот предмет.\r\n", ch);
				return false;
			}

			if IS_IMMORTAL(ch) {
				send_to_char("Господи, ну зачем тебе это?.\r\n", ch);
				return false;
			}
			trans_auction(lot);
			return true;
			break;
		case 5:        //Info
			ObjData *obj;
			if (!sscanf(argument, "%d", &lot)) {
				send_to_char("Не указан номер лота для изучения.\r\n", ch);
				return false;
			}

			if (lot < 0 || lot >= kMaxAuctionLot) {
				send_to_char("Неверный номер лота.\r\n", ch);
				return false;
			}

			if (!GET_LOT(lot)->item ||
				GET_LOT(lot)->item_id <= 0 || !GET_LOT(lot)->seller || GET_LOT(lot)->seller_unique <= 0) {
				send_to_char("Лот пуст.\r\n", ch);
				return false;
			}
			if (GetRealLevel(GET_LOT(lot)->seller) >= kLvlImmortal) {
				send_to_char("Неисповедимы пути божественные.\r\n", ch);
				return false;
			}
			obj = GET_LOT(lot)->item;
			sprintf(buf, "Предмет \"%s\", ", obj->get_short_description().c_str());
			if ((GET_OBJ_TYPE(obj) == EObjType::ITEM_WAND)
				|| (GET_OBJ_TYPE(obj) == EObjType::ITEM_STAFF)) {
				if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1)) {
					strcat(buf, "(б/у), ");
				}
			}
			strcat(buf, " тип ");
			sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
			if (*buf2) {
				strcat(buf, buf2);
				strcat(buf, "\n");
			};
			strcat(buf, diag_weapon_to_char(obj, true));
			strcat(buf, diag_timer_to_char(obj));
			strcat(buf, "\r\n");
			obj_info(ch, obj, buf);
			strcat(buf, "\n");
			if (invalid_anti_class(ch, obj) || invalid_unique(ch, obj) || NamedStuff::check_named(ch, obj, 0)) {
				sprintf(buf2, "Эта вещь вам недоступна!");
				strcat(buf, buf2);
				strcat(buf, "\n");
			}
			if ((!ch->is_npc() && invalid_align(ch, obj))
				|| invalid_no_class(ch, obj)) {
				sprintf(buf2, "Вы не сможете пользоваться этой вещью.");
				strcat(buf, buf2);
				strcat(buf, "\n");
			}
			send_to_char(buf, ch);
			return true;
			break;
		case 6:        //Identify
			ObjData *iobj;
			if (!sscanf(argument, "%d", &lot)) {
				send_to_char("Не указан номер лота для изучения.\r\n", ch);
				return false;
			}

			if (lot < 0 || lot >= kMaxAuctionLot) {
				send_to_char("Неверный номер лота.\r\n", ch);
				return false;
			}

			if (!GET_LOT(lot)->item || GET_LOT(lot)->item_id <= 0 ||
				!GET_LOT(lot)->seller || GET_LOT(lot)->seller_unique <= 0) {
				send_to_char("Лот пуст.\r\n", ch);
				return false;
			}

			if (GET_LOT(lot)->seller == ch || GET_LOT(lot)->seller_unique == GET_UNIQUE(ch)) {
				send_to_char("Но это же ваш лот!\r\n", ch);
				return false;
			}

			if (GetRealLevel(GET_LOT(lot)->seller) >= kLvlImmortal) {
				send_to_char("Неисповедимы пути божественные.\r\n", ch);
				return false;
			}

			iobj = GET_LOT(lot)->item;

			if (GetRealLevel(ch) >= kLvlImmortal && GetRealLevel(ch) < kLvlImplementator) {
				send_to_char("Господи, а ведь смертные за это деньги платят.\r\n", ch);
				return false;
			}
			if ((ch->get_total_gold() < AUCTION_IDENT_PAY) && (GetRealLevel(ch) < kLvlImplementator)) {
				send_to_char("У вас не хватит на это денег!\r\n", ch);
				return false;
			}
			bool full = false;
			mort_show_obj_values(iobj, ch, 200, full);    //200 - весь текст
			ch->remove_both_gold(AUCTION_IDENT_PAY);
			send_to_char(ch,
						 "\r\n%sЗа информацию о предмете с вашего счета сняли %d %s%s\r\n",
						 CCIGRN(ch, C_NRM),
						 AUCTION_IDENT_PAY,
						 GetDeclensionInNumber(AUCTION_IDENT_PAY, EWhat::kMoneyU),
						 CCNRM(ch, C_NRM));

			return true;
			break;
	}
	return false;
}

void message_auction(char *message, CharData *ch) {
	DescriptorData *i;

	// now send all the strings out
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING &&
			(!ch || i != ch->desc) &&
			i->character &&
			!PRF_FLAGGED(i->character, EPrf::kNoAuction) &&
			!PLR_FLAGGED(i->character, EPlrFlag::kWriting) &&
			!ROOM_FLAGGED(IN_ROOM(i->character), ERoomFlag::kSoundproof) && GET_POS(i->character) > EPosition::kSleep) {
			if (COLOR_LEV(i->character) >= C_NRM) {
				send_to_char("&Y&q", i->character.get());
			}

			act(message, false, i->character.get(), 0, 0, kToChar | kToSleep);

			if (COLOR_LEV(i->character) >= C_NRM) {
				send_to_char("&Q&n", i->character.get());
			}
		}
	}
}

void clear_auction(int lot) {
	if (lot < 0 || lot >= kMaxAuctionLot)
		return;
	GET_LOT(lot)->seller = GET_LOT(lot)->buyer = GET_LOT(lot)->prefect = nullptr;
	GET_LOT(lot)->seller_unique = GET_LOT(lot)->buyer_unique = GET_LOT(lot)->prefect_unique = -1;
	GET_LOT(lot)->item = nullptr;
	GET_LOT(lot)->item_id = -1;
}

int check_sell(int lot) {
	CharData *ch, *tch;
	ObjData *obj;
	char tmpbuf[kMaxInputLength];

	if (lot < 0 || lot >= kMaxAuctionLot || !(ch = GET_LOT(lot)->seller)
		|| GET_UNIQUE(ch) != GET_LOT(lot)->seller_unique || !(tch = GET_LOT(lot)->buyer)
		|| GET_UNIQUE(tch) != GET_LOT(lot)->buyer_unique || !(obj = GET_LOT(lot)->item)
		|| obj->get_id() != GET_LOT(lot)->item_id)
		return (false);

	if (obj->get_carried_by() != ch) {
		sprintf(tmpbuf, "Аукцион : лот %d(%s) снят, ввиду смены владельца", lot, obj->get_PName(0).c_str());
		message_auction(tmpbuf, nullptr);
		clear_auction(lot);
		return (false);
	}

	if (obj->get_contains()) {
		sprintf(tmpbuf, "Опустошите %s перед продажей.\r\n", obj->get_PName(3).c_str());
		send_to_char(tmpbuf, ch);
		if (GET_LOT(lot)->tact >= kMaxAuctionTact) {
			sprintf(tmpbuf,
					"Аукцион : лот %d(%s) снят с аукциона распорядителем торгов.",
					lot,
					obj->get_PName(0).c_str());
			message_auction(tmpbuf, nullptr);
			clear_auction(lot);
			return (false);
		}
	}

	if (tch->get_total_gold() < GET_LOT(lot)->cost) {
		sprintf(tmpbuf, "У вас не хватает денег на покупку %s.\r\n", obj->get_PName(1).c_str());
		send_to_char(tmpbuf, tch);
		sprintf(tmpbuf, "У покупателя %s не хватает денег.\r\n", obj->get_PName(1).c_str());
		send_to_char(tmpbuf, ch);
		sprintf(tmpbuf, "Аукцион : лот %d(%s) снят с аукциона распорядителем торгов.", lot, obj->get_PName(0).c_str());
		message_auction(tmpbuf, nullptr);
		clear_auction(lot);
		return (false);
	}
	// Продать можно.
	return (true);
}

void trans_auction(int lot) {
	CharData *ch, *tch;
	ObjData *obj;
	std::string tmpstr;
	char tmpbuff[kMaxInputLength];

	ch = GET_LOT(lot)->seller;
	tch = GET_LOT(lot)->prefect;
	obj = GET_LOT(lot)->item;

	if (!check_sell(lot))
		return;

	// Вещь стоит более 1000 кун.
	if (GET_LOT(lot)->cost < 1000) {
		send_to_char("Стоимость предмета слишком мала, чтобы его передать.\r\n", tch);
		return;
	}
	// У покупателя есть 10% суммы на счету.
	if (tch->get_total_gold() < (GET_LOT(lot)->cost + GET_LOT(lot)->cost / 10)) {
		send_to_char("У вас не хватает денег на передачу предмета.", tch);
		return;
	}

	if (ch->in_room == IN_ROOM(tch)) {
		// Проверка на нахождение в одной комнате.
		tmpstr = "$n стоит рядом с вами.";
		act(tmpstr.c_str(), false, ch, 0, tch, kToVict | kToSleep);
		return;
	};
	// Проверяем условия передачи предмета.
	// Оба чара в мирке
	// Оба чара без БД
	if (NORENTABLE(ch)) {
		tmpstr = "Завершите боевые действия для передачи " + obj->get_PName(1) + " $N2.\r\n";

		act(tmpstr.c_str(), false, ch, 0, tch, kToChar | kToSleep);

		tmpstr = "$n2 необходимо завершить боевые действия для передачи " + obj->get_PName(1) + " вам.\r\n";

		act(tmpstr.c_str(), false, ch, 0, tch, kToVict | kToSleep);
		return;
	}

	if (NORENTABLE(tch)) {
		tmpstr = "Завершите боевые действия для получения денег от $n1.\r\n";

		act(tmpstr.c_str(), false, ch, 0, tch, kToVict | kToSleep);

		tmpstr = "$N2 необходимо завершить боевые действия для получения денег от вас.";
		act(tmpstr.c_str(), false, ch, 0, tch, kToChar | kToSleep);
		return;
	}

	if (!bloody::handle_transfer(tch, ch, obj)) {
		act("$N2 стоит сначала кровь смыть с товара.", false, ch, 0, tch, kToChar | kToSleep);
		return;
	}

	if (!is_post(ch->in_room)) {
		// Проверка на то что продавец на ренте.
		tmpstr = "Вам необходимо прибыть к ближайшей яме для передачи " + obj->get_PName(1) + " $N2.\r\n";

		act(tmpstr.c_str(), false, ch, 0, tch, kToChar | kToSleep);

		tmpstr = "$N2 необходимо прибыть к ближайшей яме для передачи " + obj->get_PName(1) + " вам.\r\n";

		act(tmpstr.c_str(), false, tch, 0, ch, kToChar | kToSleep);
		return;
	}

	if (!is_post(IN_ROOM(tch))) {
		// Проверка на то что продавец на ренте.
		tmpstr = "Вам необходимо прибыть к ближайшей яме для передачи денег $N2.\r\n";
		act(tmpstr.c_str(), false, tch, 0, ch, kToChar | kToSleep);

		tmpstr = "$N2 необходимо прибыть к ближайшей яме для передачи денег вам.\r\n";
		act(tmpstr.c_str(), false, ch, 0, tch, kToChar | kToSleep);
		return;
	}

	if (obj->get_contains()) {
		sprintf(tmpbuff, "Продажа %s не возможна.\r\n", obj->get_PName(3).c_str());
		send_to_char(tmpbuff, ch);
		sprintf(tmpbuff, "Транспортировка %s в данный момент не возможна.\r\n", obj->get_PName(1).c_str());
		send_to_char(tmpbuff, tch);
		return;
	}

// - Забираем предмет у продавца
	tmpstr = "Перед вами появился слегка трезвый Иван Царевич на Сивке-бурке.";

	act(tmpstr.c_str(), false, ch, 0, tch, kToChar);
	act(tmpstr.c_str(), false, tch, 0, tch, kToChar);

	tmpstr = "Перед $n4 появился Иван-Царевич на Сивке-бурке.";

	act(tmpstr.c_str(), false, ch, 0, ch, kToRoom);
	act(tmpstr.c_str(), false, tch, 0, tch, kToRoom);

	act("Иван-Царевич дал вам кучку кун.", false, ch, 0, ch, kToChar);
	act("Иван-Царевич дал гору кун $n2", false, ch, 0, ch, kToRoom);

	tmpstr = "Вы отдали " + obj->get_PName(3) + " Ивану-Царевичу.";
	act(tmpstr.c_str(), false, ch, 0, ch, kToChar);

	tmpstr = "$n отдал$g " + obj->get_PName(3) + " Ивану-Царевичу.";
	act(tmpstr.c_str(), false, ch, 0, ch, kToRoom);

	act("Вы дали кучку кун Ивану-Царевичу.", false, tch, 0, tch, kToChar);
	act("$n дал$g гору кун Ивану-Царевичу.", false, tch, 0, tch, kToRoom);

	tmpstr = "Иван-Царевич отдал " + obj->get_PName(3) + " вам.";
	act(tmpstr.c_str(), false, tch, 0, tch, kToChar);

	tmpstr = "Иван-Царевич отдал " + obj->get_PName(3) + " $n2.";
	act(tmpstr.c_str(), false, tch, 0, tch, kToRoom);

	tmpstr = "Иван-Царевич исчез в клубах пыли. На его суме вы заметили надпись:\r\n";
	tmpstr += "'Иван да Сивка - бЕрежно дОпрем дОбро'.";

	act(tmpstr.c_str(), false, ch, 0, ch, kToChar);
	act(tmpstr.c_str(), false, ch, 0, ch, kToRoom);

	act(tmpstr.c_str(), false, tch, 0, tch, kToChar);
	act(tmpstr.c_str(), false, tch, 0, tch, kToRoom);

	// Фонить закончили осуществляем обмен.

	tmpstr = "Вы продали " + obj->get_PName(3) + " с аукциона.\r\n";
	send_to_char(tmpstr.c_str(), ch);
	tmpstr = "Вы купили " + obj->get_PName(3) + " на аукционе.\r\n";
	send_to_char(tmpstr.c_str(), tch);

	obj_from_char(obj);
	obj_to_char(obj, tch);

	ch->add_bank(GET_LOT(lot)->cost);
	tch->remove_both_gold(GET_LOT(lot)->cost + (GET_LOT(lot)->cost / 10));

	clear_auction(lot);
	return;
}

void sell_auction(int lot) {
	CharData *ch, *tch;
	ObjData *obj;
	std::string tmpstr;
	char tmpbuff[kMaxInputLength];

	ch = GET_LOT(lot)->seller;
	tch = GET_LOT(lot)->buyer;
	obj = GET_LOT(lot)->item;

	if (!check_sell(lot))
		return;

	if (ch->in_room != IN_ROOM(tch)
		|| !ROOM_FLAGGED(ch->in_room, ERoomFlag::kPeaceful)) {
		if (GET_LOT(lot)->tact >= kMaxAuctionTact) {
			sprintf(tmpbuff,
					"Аукцион : лот %d(%s) снят с аукциона распорядителем торгов.",
					lot,
					obj->get_PName(0).c_str());

			message_auction(tmpbuff, nullptr);
			clear_auction(lot);
			return;
		}
		tmpstr = "Вам необходимо прибыть в комнату аукциона к $n2 для получения " +
			obj->get_PName(1) + "\r\nили воспользоваться услугами ямщика.";

		act(tmpstr.c_str(), false, ch, 0, tch, kToVict | kToSleep);

		tmpstr = "Вам необходимо прибыть в комнату аукциона к $N2 для получения денег за " + obj->get_PName(3) + ".";

		act(tmpstr.c_str(), false, ch, 0, tch, kToChar | kToSleep);
		GET_LOT(lot)->tact = MAX(GET_LOT(lot)->tact, kMaxAuctionTactBuy);
		return;
	}

	if (obj->get_contains()) {
		sprintf(tmpbuff, "Продажа %s не возможна.\r\n", obj->get_PName(3).c_str());
		send_to_char(tmpbuff, ch);
		return;
	}

	tmpstr = "Вы продали " + obj->get_PName(3) + " с аукциона.\r\n";
	send_to_char(tmpstr.c_str(), ch);

	tmpstr = "Вы купили " + obj->get_PName(3) + " на аукционе.\r\n";
	send_to_char(tmpstr.c_str(), tch);

	obj_from_char(obj);
	obj_to_char(obj, tch);

	ch->add_bank(GET_LOT(lot)->cost);
	tch->remove_both_gold(GET_LOT(lot)->cost);

	clear_auction(lot);
	return;
}

void check_auction(CharData *ch, ObjData *obj) {
	int i;
	char tmpbuf[kMaxInputLength];
	if (ch) {
		for (i = 0; i < kMaxAuctionLot; i++) {
			if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
				continue;
			if (GET_LOT(i)->seller == ch || GET_LOT(i)->seller_unique == GET_UNIQUE(ch)
				|| GET_LOT(i)->buyer == ch || GET_LOT(i)->buyer_unique == GET_UNIQUE(ch)
				|| GET_LOT(i)->prefect == ch || GET_LOT(i)->prefect_unique == GET_UNIQUE(ch)) {
				sprintf(tmpbuf, "Аукцион : лот %d(%s) снят с аукциона распорядителем.",
						i, GET_LOT(i)->item->get_PName(0).c_str());
				message_auction(tmpbuf, ch);
				clear_auction(i);
			}
		}
	} else if (obj) {
		for (i = 0; i < kMaxAuctionLot; i++) {
			if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
				continue;
			if (GET_LOT(i)->item == obj || GET_LOT(i)->item_id == obj->get_id()) {
				sprintf(tmpbuf, "Аукцион : лот %d(%s) снят с аукциона распорядителем.",
						i, GET_LOT(i)->item->get_PName(0).c_str());
				message_auction(tmpbuf, obj->get_carried_by());
				clear_auction(i);
			}
		}
	} else {
		for (i = 0; i < kMaxAuctionLot; i++) {
			if (!GET_LOT(i)->seller
				|| !GET_LOT(i)->item) {
				continue;
			}
			if (GET_LOT(i)->item->get_carried_by() != GET_LOT(i)->seller
				|| (GET_LOT(i)->buyer
					&& (GET_LOT(i)->buyer->get_total_gold() < GET_LOT(i)->cost))) {
				sprintf(tmpbuf, "Аукцион : лот %d(%s) снят с аукциона распорядителем.",
						i, GET_LOT(i)->item->get_PName(0).c_str());
				message_auction(tmpbuf, nullptr);
				clear_auction(i);
			}
		}
	}
}

void tact_auction(void) {
	int i;
	char tmpbuf[kMaxInputLength];

	check_auction(nullptr, nullptr);

	for (i = 0; i < kMaxAuctionLot; i++) {
		if (!GET_LOT(i)->seller || !GET_LOT(i)->item)
			continue;
		if (++GET_LOT(i)->tact < kMaxAuctionTactBuy) {
			sprintf(tmpbuf, "Аукцион : лот %d(%s), %d %s, %s", i,
					GET_LOT(i)->item->get_PName(0).c_str(), GET_LOT(i)->cost,
					GetDeclensionInNumber(GET_LOT(i)->cost, EWhat::kMoneyA), tact_message[GET_LOT(i)->tact]);
			message_auction(tmpbuf, nullptr);
			continue;
		} else if (GET_LOT(i)->tact < kMaxAuctionTact) {
			if (!GET_LOT(i)->buyer) {
				sprintf(tmpbuf, "Аукцион : лот %d(%s) снят распорядителем ввиду отсутствия спроса.",
						i, GET_LOT(i)->item->get_PName(0).c_str());
				message_auction(tmpbuf, nullptr);
				clear_auction(i);
				continue;
			}
			if (!GET_LOT(i)->prefect) {
				sprintf(tmpbuf, "Аукцион : лот %d(%s), %d %s - ПРОДАНО.",
						i, GET_LOT(i)->item->get_PName(0).c_str(), GET_LOT(i)->cost,
						GetDeclensionInNumber(GET_LOT(i)->cost, EWhat::kMoneyA));
				message_auction(tmpbuf, nullptr);
				GET_LOT(i)->prefect = GET_LOT(i)->buyer;
				GET_LOT(i)->prefect_unique = GET_LOT(i)->buyer_unique;
			}
			sell_auction(i);
		} else
			sell_auction(i);
	}
}

AuctionItem *free_auction(int *lotnum) {
	int i;
	for (i = 0; i < kMaxAuctionLot; i++) {
		if (!GET_LOT(i)->seller && !GET_LOT(i)->item) {
			*lotnum = i;
			return (GET_LOT(i));
		}
	}

	return (nullptr);
}

int obj_on_auction(ObjData *obj) {
	int i;
	for (i = 0; i < kMaxAuctionLot; i++) {
		if (GET_LOT(i)->item == obj && GET_LOT(i)->item_id == obj->get_id())
			return (true);
	}

	return (false);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
