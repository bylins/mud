// $RCSfile: shop_ext.cpp,v $     $Date: 2012/01/27 05:36:40 $     $Revision: 1.16 $
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#include "shop_ext.h"

#include "engine/db/global_objects.h"
#include "engine/db/obj_prototypes.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "shops_implementation.h"
#include "gameplay/ai/spec_procs.h"

extern int do_social(CharData *ch, char *argument);    // implemented in the act.social.cpp
extern void mort_show_obj_values(const ObjData *obj, CharData *ch, int fullness, bool enhansed_scroll);

// здесь хранятся все предметы из магазинов вида внум_предмета, цена
//std::map<int, int> items_list_for_checks;
namespace ShopExt {
const char *MSG_NO_STEAL_HERE = "$n, грязн$w воришка, чеши отседова!";

typedef std::map<int/*vnum предмета*/, item_desc_node> ObjDescType;
std::map<std::string/*id шаблона*/, ObjDescType> item_descriptions;

struct item_set_node {
	long item_vnum;
	int item_price;
};

struct item_set {
	item_set() = default;

	std::string _id;
	std::vector<item_set_node> item_list;
};

typedef std::shared_ptr<item_set> ItemSetPtr;
typedef std::vector<ItemSetPtr> ItemSetListType;

ShopListType &shop_list = GlobalObjects::Shops();

void log_shop_load() {
	for (const auto &shop : shop_list) {
		for (const auto &mob_vnum : shop->mob_vnums()) {
			log("ShopExt: mob=%d", mob_vnum);
		}

		log("ShopExt: currency=%s", shop->currency.c_str());

		const auto &items = shop->items_list();
		for (auto index = 0u; index < items.size(); ++index) {
			const auto &item = items.node(index);
			log("ItemList: vnum=%d, price=%ld", item->vnum(), item->get_price());
		}
	}
}

void load_item_desc() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_PLRSTUFF"/shop/item_desc.xml");
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}

	pugi::xml_node node_list = doc.child("templates");
	if (!node_list) {
		snprintf(buf, kMaxStringLength, "...templates list read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	item_descriptions.clear();
	pugi::xml_node child;
	for (pugi::xml_node item_template = node_list.child("template"); item_template;
		 item_template = item_template.next_sibling("template")) {
		std::string templateId = item_template.attribute("id").value();
		for (pugi::xml_node item = item_template.child("item"); item; item = item.next_sibling("item")) {

			int item_vnum = parse::ReadAttrAsInt(item, "vnum");
			if (item_vnum <= 0) {
				snprintf(buf, kMaxStringLength,
						 "...bad item description attributes (item_vnum=%d)", item_vnum);
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				return;
			}
			item_desc_node desc_node;
			child = item.child("name");
			desc_node.name = child.child_value();
			child = item.child("description");
			desc_node.description = child.child_value();
			child = item.child("short_description");
			desc_node.short_description = child.child_value();
			child = item.child("PNames0");
			desc_node.PNames[0] = child.child_value();
			child = item.child("PNames1");
			desc_node.PNames[1] = child.child_value();
			child = item.child("PNames2");
			desc_node.PNames[2] = child.child_value();
			child = item.child("PNames3");
			desc_node.PNames[3] = child.child_value();
			child = item.child("PNames4");
			desc_node.PNames[4] = child.child_value();
			child = item.child("PNames5");
			desc_node.PNames[5] = child.child_value();
			child = item.child("sex");
			desc_node.sex = static_cast<EGender>(atoi(child.child_value()));

			// парсим список триггеров
			pugi::xml_node trig_list = item.child("triggers");
			CObjectPrototype::triggers_list_t trig_vnums;
			for (pugi::xml_node trig = trig_list.child("trig"); trig; trig = trig.next_sibling("trig")) {
				int trig_vnum;
				std::string tmp_value = trig.child_value();
				utils::Trim(tmp_value);
				try {
					trig_vnum = std::stoi(tmp_value, nullptr, 10);
				}
				catch (const std::invalid_argument &) {
					snprintf(buf,
							 kMaxStringLength,
							 "...error while casting to num (item_vnum=%d, casting value=%s)",
							 item_vnum,
							 tmp_value.c_str());
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					continue;
				}

				if (trig_vnum <= 0) {
					snprintf(buf,
							 kMaxStringLength,
							 "...error while parsing triggers (item_vnum=%d, parsed value=%s)",
							 item_vnum,
							 tmp_value.c_str());
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					return;
				}
				trig_vnums.push_back(trig_vnum);
			}
			desc_node.trigs = trig_vnums;
			item_descriptions[templateId][item_vnum] = desc_node;
		}
	}
}

void load(bool reload) {
	if (reload) {
		for (const auto &shop : shop_list) {
			shop->clear_store();

			for (const auto &mob_vnum : shop->mob_vnums()) {
				int mob_rnum = GetMobRnum(mob_vnum);
				if (mob_rnum >= 0) {
					mob_index[mob_rnum].func = nullptr;
				}
			}
		}

		shop_list.clear();
	}
	load_item_desc();

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_PLRSTUFF"/shop/shops.xml");
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	pugi::xml_node node_list = doc.child("shop_list");
	if (!node_list) {
		snprintf(buf, kMaxStringLength, "...shop_list read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	//наборы предметов - "заготовки" для реальных предметов в магазинах. живут только на время лоада
	ItemSetListType itemSetList;
	pugi::xml_node itemSets = doc.child("shop_item_sets");
	for (pugi::xml_node itemSet = itemSets.child("shop_item_set"); itemSet;
		 itemSet = itemSet.next_sibling("shop_item_set")) {
		std::string itemSetId = itemSet.attribute("id").value();
		ItemSetPtr tmp_set(new item_set);
		tmp_set->_id = itemSetId;

		for (pugi::xml_node item = itemSet.child("item"); item; item = item.next_sibling("item")) {
			int item_vnum = parse::ReadAttrAsInt(item, "vnum");
			int price = parse::ReadAttrAsInt(item, "price");
			if (item_vnum < 0 || price < 0) {
				snprintf(buf,
						 kMaxStringLength,
						 "...bad shop item set attributes (item_set=%s, item_vnum=%d, price=%d)",
						 itemSetId.c_str(),
						 item_vnum,
						 price);
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				return;
			}
			struct item_set_node tmp_node;
			tmp_node.item_price = price;
			tmp_node.item_vnum = item_vnum;
			tmp_set->item_list.push_back(tmp_node);
		}
		itemSetList.push_back(tmp_set);
	}

	for (pugi::xml_node node = node_list.child("shop"); node; node = node.next_sibling("shop")) {
		std::string shop_id = node.attribute("id").value();
		std::string currency = node.attribute("currency").value();
		int profit = node.attribute("profit").as_int();
		std::string can_buy_value = node.attribute("can_buy").value();
		bool shop_can_buy = can_buy_value != "false";
		int store_time_min =
			(node.attribute("waste_time_min").value() ? node.attribute("waste_time_min").as_int() : 180);

		// иним сам магазин
		const auto tmp_shop = std::make_shared<shop_node>();
		tmp_shop->id = shop_id;
		tmp_shop->currency = currency;
		tmp_shop->profit = profit;
		tmp_shop->can_buy = shop_can_buy;
		tmp_shop->waste_time_min = store_time_min;
		//словарные данные
		tmp_shop->SetDictionaryName(shop_id);//а нету у магазинов названия
		tmp_shop->SetDictionaryTID(shop_id);

		std::map<int, std::string> mob_to_template;

		for (pugi::xml_node mob = node.child("mob"); mob; mob = mob.next_sibling("mob")) {
			int mob_vnum = parse::ReadAttrAsInt(mob, "mob_vnum");
			std::string templateId = mob.attribute("template").value();
			if (mob_vnum < 0) {
				snprintf(buf, kMaxStringLength,
						 "...bad shop attributes (mob_vnum=%d shop id=%s)", mob_vnum, shop_id.c_str());
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				return;
			}

			if (!templateId.empty()) {
				mob_to_template[mob_vnum] = templateId;
			}

			tmp_shop->add_mob_vnum(mob_vnum);
			// проверяем и сетим мобу спешиал
			// даже если дальше магаз не залоадится - моб будет выдавать ошибку на магазинные спешиалы
			auto mob_rnum = GetMobRnum(mob_vnum);
			if (mob_rnum >= 0) {
				if (mob_index[mob_rnum].func
					&& mob_index[mob_rnum].func != shop_ext) {
					snprintf(buf, kMaxStringLength,
							 "...shopkeeper already with special (mob_vnum=%d)", mob_vnum);
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				} else {
					mob_index[mob_rnum].func = shop_ext;
				}
			} else {
				snprintf(buf, kMaxStringLength, "...incorrect mob_vnum=%d", mob_vnum);
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			}
		}

		// и список его продукции
		for (pugi::xml_node item = node.child("item"); item; item = item.next_sibling("item")) {
			int item_vnum = parse::ReadAttrAsInt(item, "vnum");
			int price = parse::ReadAttrAsInt(item, "price");
			if (item_vnum < 0
				|| price < 0) {
				snprintf(buf, kMaxStringLength,
						 "...bad shop attributes (item_vnum=%d, price=%d)", item_vnum, price);
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				return;
			}

			// проверяем шмотку
			int item_rnum = GetObjRnum(item_vnum);
			if (item_rnum < 0) {
				snprintf(buf, kMaxStringLength, "...incorrect item_vnum=%d", item_vnum);
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				return;
			}

			// иним ее в магазе
			const auto item_price = price == 0 ? GET_OBJ_COST(obj_proto[item_rnum])
											   : price; //если не указана цена - берем цену из прототипа
			tmp_shop->add_item(item_vnum, item_price);
		}

		//и еще добавим наборы
		for (pugi::xml_node itemSet = node.child("item_set"); itemSet; itemSet = itemSet.next_sibling("item_set")) {
			std::string itemSetId = itemSet.child_value();
			for (ItemSetListType::const_iterator it = itemSetList.begin(); it != itemSetList.end(); ++it) {
				if ((*it)->_id == itemSetId) {
					for (unsigned i = 0; i < (*it)->item_list.size(); i++) {
						// проверяем шмотку
						int item_rnum = GetObjRnum((*it)->item_list[i].item_vnum);
						if (item_rnum < 0) {
							snprintf(buf,
									 kMaxStringLength,
									 "...incorrect item_vnum=%d in item_set=%s",
									 (int) (*it)->item_list[i].item_vnum,
									 (*it)->_id.c_str());
							mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
							return;
						}
						// иним ее в магазе
						const auto item_vnum = (*it)->item_list[i].item_vnum;
						const int price = (*it)->item_list[i].item_price;
						const auto item_price = price == 0 ? GET_OBJ_COST(obj_proto[item_rnum]) : price;
						tmp_shop->add_item(item_vnum, item_price);
						/*
						Список инится но нигде не используется. к удалению
						if (items_list_for_checks.count(item_vnum) != 1)
						{
							items_list_for_checks.insert(std::pair<int, int>(item_vnum, item_price));
						}
						else
						{
							if (items_list_for_checks[item_vnum] > item_price)
								items_list_for_checks[item_vnum] = item_price;
						}
						*/
					}
				}
			}
		}

		if (tmp_shop->empty()) {
			snprintf(buf, kMaxStringLength, "...item list empty (shop_id=%s)", shop_id.c_str());
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return;
		}

		const auto &items = tmp_shop->items_list();
		for (auto index = 0u; index < items.size(); ++index) {
			const auto &item = items.node(index);
			for (auto id = item_descriptions.begin(); id != item_descriptions.end(); ++id) {
				if (id->second.find(item->vnum()) != id->second.end()) {
					item_desc_node tmp_desc(id->second[item->vnum()]);
					for (const auto &mob_vnum : tmp_shop->mob_vnums()) {
						if (mob_to_template.find(mob_vnum) != mob_to_template.end()
							&& mob_to_template[mob_vnum] == id->first) {
							item->add_desc(mob_vnum, tmp_desc);
						}
					}
				}
			}
		}

		shop_list.push_back(tmp_shop);
	}

	log_shop_load();
}

int get_spent_today() {
	return spent_today;
}

} // namespace ShopExt

using namespace ShopExt;

int shop_ext(CharData *ch, void *me, int cmd, char *argument) {
	if (!ch->desc
		|| ch->IsNpc()) {
		return 0;
	}

	if (!(CMD_IS("список")
		|| CMD_IS("list")
		|| CMD_IS("купить")
		|| CMD_IS("buy")
		|| CMD_IS("продать")
		|| CMD_IS("sell")
		|| CMD_IS("оценить")
		|| CMD_IS("value")
		|| CMD_IS("чинить")
		|| CMD_IS("repair")
		|| CMD_IS("украсть")
		|| CMD_IS("steal")
		|| CMD_IS("фильтровать")
		|| CMD_IS("filter")
		|| CMD_IS("рассмотреть")
		|| CMD_IS("examine")
		|| CMD_IS("характеристики")
		|| CMD_IS("identify"))) {
		return 0;
	}

	char argm[kMaxInputLength];
	CharData *const keeper = reinterpret_cast<CharData *>(me);
	shop_node::shared_ptr shop;
	for (const auto &s : shop_list) {
		const auto found =
			std::find(s->mob_vnums().begin(), s->mob_vnums().end(), GET_MOB_VNUM(keeper)) != s->mob_vnums().end();
		if (found) {
			shop = s;
			break;
		}
	}

	if (!shop) {
		log("SYSERROR : магазин не найден mob_vnum=%d (%s:%d)", GET_MOB_VNUM(keeper), __FILE__, __LINE__);
		SendMsgToChar("Ошибочка вышла.\r\n", ch);

		return 1;
	}

	if (CMD_IS("steal")
		|| CMD_IS("украсть")) {
		sprintf(argm, "$N вскричал$G '%s'", MSG_NO_STEAL_HERE);
		sprintf(buf, "ругать %s", GET_NAME(ch));
		do_social(keeper, buf);
		act(argm, false, ch, 0, keeper, kToChar);
		return 1;
	}

	if (CMD_IS("список")
		|| CMD_IS("list")) {
		std::string buffer = argument, buffer2;
		GetOneParam(buffer, buffer2);
		shop->print_shop_list(ch, buffer2, GET_MOB_VNUM(keeper));
		return 1;
	}

	if (CMD_IS("фильтровать")
		|| CMD_IS("filter")) {
		shop->filter_shop_list(ch, argument, GET_MOB_VNUM(keeper));
		return 1;
	}

	if (CMD_IS("купить")
		|| CMD_IS("buy")) {
		shop->process_buy(ch, keeper, argument);
		return 1;
	}
	if (CMD_IS("характеристики") || CMD_IS("identify")) {
		shop->process_ident(ch, keeper, argument, "Характеристики");
		return 1;
	}

	if (CMD_IS("value") || CMD_IS("оценить")) {
		shop->process_cmd(ch, keeper, argument, "Оценить");
		return 1;
	}
	if (CMD_IS("продать") || CMD_IS("sell")) {
		shop->process_cmd(ch, keeper, argument, "Продать");
		return 1;
	}
	if (CMD_IS("чинить") || CMD_IS("repair")) {
		shop->process_cmd(ch, keeper, argument, "Чинить");
		return 1;
	}
	if (CMD_IS("рассмотреть") || CMD_IS("examine")) {
		shop->process_ident(ch, keeper, argument, "Рассмотреть");
		return 1;
	}

	return 0;
}

// * Лоад странствующих продавцов в каждой ренте.
void town_shop_keepers() {
	// список уже оработанных зон, чтобы не грузить двух и более торгашей в одну
	std::set<int> zone_list;

	for (const auto &ch : character_list) {
		if (IS_RENTKEEPER(ch)
			&& ch->in_room > 0
			&& !Clan::GetClanByRoom(ch->in_room)
			&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena)
			&& GetZoneVnumByCharPlace(ch.get()) > 39 // в служебках нехрен делать
			&& GET_ROOM_VNUM(ch->in_room) % 100 != 99
			&& zone_list.find(world[ch->in_room]->zone_rn) == zone_list.end()) {
			int rnum_start, rnum_end;
			if (GetZoneRooms(world[ch->in_room]->zone_rn, &rnum_start, &rnum_end)) {
				CharData *mob = ReadMobile(1901, kVirtual);
				if (mob) {
					PlaceCharToRoom(mob, number(rnum_start, rnum_end));
				}
			}
			zone_list.insert(world[ch->in_room]->zone_rn);
		}
	}
}

// оставил как пример поиска obj в магазинах, может пригодится
void DoStoreShop(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		SendMsgToChar("Чаво?\r\n", ch);
		return;
	}
	if (!*argument) {
		SendMsgToChar("Чего хотим? Могу пока только характерстики.\r\n", ch);
		return;
	}
	char *stufina = one_argument(argument, arg);

	if (utils::IsAbbr(arg, "характеристики") || utils::IsAbbr(arg, "identify") || utils::IsAbbr(arg, "опознать")) {
		if ((ch->get_bank() < kChestIdentPay) && (GetRealLevel(ch) < kLvlImplementator)) {
			SendMsgToChar("У вас недостаточно денег в банке для такого исследования.\r\n", ch);
			return;
		}
		SendMsgToChar(ch, "Лорим предмет %s\r\n", stufina);
		for (const auto &shop : GlobalObjects::Shops()) {
			const auto &item_list = shop->items_list();
			for (size_t i = 0; i < item_list.size(); i++) {
				if (item_list.node(i)->uid() == ShopExt::ItemNode::NO_UID) {
					continue;
				}
				const auto obj = shop->GetObjFromShop(item_list.node(i)->uid());
				if (isname(stufina, GET_OBJ_PNAME(obj, 0))) {
					SendMsgToChar(ch, "Характеристики предмета: %s\r\n", stufina);
					bool full = false;
					mort_show_obj_values(obj, ch, 200, full);
					ch->remove_bank(kChestIdentPay);
					SendMsgToChar(ch,
								  "&GЗа информацию о предмете с вашего банковского счета сняли %d %s&n\r\n",
								  kChestIdentPay,
								  GetDeclensionInNumber(kChestIdentPay, EWhat::kMoneyU));
					return;
				}
			}
		}
		SendMsgToChar("Ничего не найдено.\r\n", ch);
	} else
		SendMsgToChar("Чего хотим? Могу пока только характерстики.\r\n", ch);
}

void do_shops_list(CharData *ch) {
/*	DictionaryPtr dic = DictionaryPtr(new Dictionary(SHOP));
	size_t n = dic->Size();
	std::ostringstream out;
	for (size_t i = 0; i < n; i++) {
		out << std::to_string(i + 1) << " " << dic->GetNameByNID(i) << " " << dic->GetTIDByNID(i) + "\r\n";
	}
*/
	std::ostringstream out;

	for (const auto &shop : shop_list) {
		out << shop->GetDictionaryName() << "\r\nvnum : ";
		for (const auto &mob_vnum : shop->mob_vnums()) {
			out << mob_vnum << " ";
		}
		out << "\r\n";
	}
	SendMsgToChar(out.str().c_str(), ch);
}

void fill_shop_dictionary(DictionaryType &dic) {
	ShopExt::ShopListType::const_iterator it = ShopExt::shop_list.begin();
	for (; it != ShopExt::shop_list.end(); ++it)
		dic.push_back((*it)->GetDictionaryItem());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
