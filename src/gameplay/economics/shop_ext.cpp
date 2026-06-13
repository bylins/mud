// $RCSfile: shop_ext.cpp,v $     $Date: 2012/01/27 05:36:40 $     $Revision: 1.16 $
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#include "shop_ext.h"
#include "engine/olc/vedun/enum_registry.h"   // vedun::RegisterEditorEnums (refresh ShopItemSetId)
#include <cstdlib>
#include "gameplay/economics/currencies.h"
#include "utils/grammar/declensions.h"
#include "gameplay/ai/subcmd_resolver.h"
#include "gameplay/ai/special_messages.h"
#include "gameplay/mechanics/identify.h"

#include "third_party_libs/pugixml/pugixml.h"

#include "engine/db/global_objects.h"
#include "engine/db/obj_prototypes.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "shops_implementation.h"
#include "gameplay/ai/spec_procs.h"

extern int do_social(CharData *ch, char *argument);    // implemented in the act.social.cpp

// здесь хранятся все предметы из магазинов вида внум_предмета, цена
//std::map<int, int> items_list_for_checks;
namespace ShopExt {
const char *MSG_NO_STEAL_HERE = "$n, грязн$w воришка, чеши отседова!";

typedef std::map<int/*vnum предмета*/, item_desc_node> ObjDescType;
std::map<std::string/*id шаблона*/, ObjDescType> item_descriptions;

namespace {
ItemSetListType g_item_sets;   // catalog from cfg/economics/shop_item_sets.xml

// A valid shop / item-set id: CamelCase -- starts with a letter, letters and digits only.
bool IsValidShopId(const std::string &id) {
	auto is_alpha = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); };
	if (id.empty() || !is_alpha(id[0])) {
		return false;
	}
	for (const char c : id) {
		if (!is_alpha(c) && !(c >= '0' && c <= '9')) {
			return false;
		}
	}
	return true;
}
}

const ItemSetListType &item_sets() {
	return g_item_sets;
}

void ShopItemSetsLoader::Load(parser_wrapper::DataNode data) {
	g_item_sets.clear();
	for (auto set_node : data.Children()) {
		if (std::string(set_node.GetName()) != "shop_item_set") {
			continue;
		}
		auto set = std::make_shared<item_set>();
		set->_id = set_node.GetValue("id");
		set->comment = set_node.GetValue("comment");
		for (auto item : set_node.Children()) {
			if (std::string(item.GetName()) != "item") {
				continue;
			}
			item_set_node node;
			node.item_vnum = parse::ReadAsInt(item.GetValue("vnum"));
			node.item_price = parse::ReadAsInt(item.GetValue("price"));
			set->item_list.push_back(node);
		}
		g_item_sets.push_back(set);
	}
}

void ShopItemSetsLoader::Reload(parser_wrapper::DataNode data) {
	Load(data);
	vedun::RegisterEditorEnums();   // refresh ShopItemSetId so new/removed sets show in the editor
	load(true);   // rebuild shops so catalog edits take effect immediately
}

std::string ShopItemSetsLoader::EditableWhat() const {
	return "shopitemset";   // no underscores -- matches currencyname/shopmsg/mobrace convention
}

std::vector<cfg_manager::EditableElement> ShopItemSetsLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &set : g_item_sets) {
		std::string label = set->_id;
		if (!set->comment.empty()) {
			label += "  -- " + set->comment;
		}
		out.push_back({set->_id, label});
	}
	return out;
}

cfg_manager::ValidationResult ShopItemSetsLoader::Validate(parser_wrapper::DataNode &doc) const {
	for (auto set_node : doc.Children()) {
		if (std::string(set_node.GetName()) != "shop_item_set") {
			continue;
		}
		const std::string id = set_node.GetValue("id");
		if (id.empty()) {
			return {false, "a <shop_item_set> has an empty id"};
		}
		for (auto item : set_node.Children()) {
			if (std::string(item.GetName()) != "item") {
				continue;
			}
			if (parse::ReadAsInt(item.GetValue("vnum")) < 0 || parse::ReadAsInt(item.GetValue("price")) < 0) {
				return {false, "shop_item_set '" + id + "': negative item vnum/price"};
			}
		}
	}
	return {true, ""};
}

std::string ShopItemSetsLoader::CanonicalElementId(const std::string &id) const {
	return IsValidShopId(id) ? id : "";   // CamelCase: letters/digits, starts with a letter
}

parser_wrapper::DataNode ShopItemSetsLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	auto node = root.AddChild("shop_item_set");
	node.SetValue("id", id);
	node.SetValue("comment", "");
	return node;   // items are added afterwards in the editor
}

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
			desc_node.PNames[grammar::ECase::kNom] = child.child_value();
			child = item.child("PNames1");
			desc_node.PNames[grammar::ECase::kGen] = child.child_value();
			child = item.child("PNames2");
			desc_node.PNames[grammar::ECase::kDat] = child.child_value();
			child = item.child("PNames3");
			desc_node.PNames[grammar::ECase::kAcc] = child.child_value();
			child = item.child("PNames4");
			desc_node.PNames[grammar::ECase::kIns] = child.child_value();
			child = item.child("PNames5");
			desc_node.PNames[grammar::ECase::kPre] = child.child_value();
			child = item.child("sex");
			desc_node.sex = static_cast<EGender>(atoi(child.child_value()));

			// парсим список триггеров
			pugi::xml_node script_trig_list = item.child("triggers");
			CObjectPrototype::triggers_list_t trig_vnums;
			for (pugi::xml_node trig = script_trig_list.child("trig"); trig; trig = trig.next_sibling("trig")) {
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

// Build (or rebuild) the shop list from cfg/economics/shops.xml. The item-set catalog
// (ShopItemSetsLoader) must already be loaded.
void ShopsLoader::Load(parser_wrapper::DataNode data) {
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
	load_item_desc();

	for (auto node : data.Children()) {
		if (std::string(node.GetName()) != "shop") {
			continue;
		}
		const std::string shop_id = node.GetValue("id");
		const auto tmp_shop = std::make_shared<shop_node>();
		tmp_shop->id = shop_id;
		tmp_shop->currency = node.GetValue("currency");
		tmp_shop->profit = std::atoi(node.GetValue("profit"));
		tmp_shop->can_buy = std::string(node.GetValue("can_buy")) != "false";
		const char *wt = node.GetValue("waste_time_min");
		tmp_shop->waste_time_min = (wt && *wt) ? std::atoi(wt) : 180;
		tmp_shop->SetDictionaryName(shop_id);
		tmp_shop->SetDictionaryTID(shop_id);

		std::map<int, std::string> mob_to_template;
		for (auto child : node.Children()) {
			const std::string cname = child.GetName();
			if (cname == "mob") {
				const int mob_vnum = std::atoi(child.GetValue("mob_vnum"));
				const std::string templateId = child.GetValue("template");
				if (mob_vnum < 0) {
					snprintf(buf, kMaxStringLength, "...bad shop attributes (mob_vnum=%d shop id=%s)", mob_vnum, shop_id.c_str());
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					continue;
				}
				if (!templateId.empty()) {
					mob_to_template[mob_vnum] = templateId;
				}
				tmp_shop->add_mob_vnum(mob_vnum);
			} else if (cname == "item") {
				const int item_vnum = std::atoi(child.GetValue("vnum"));
				const int price = std::atoi(child.GetValue("price"));
				if (item_vnum < 0 || price < 0) {
					snprintf(buf, kMaxStringLength, "...bad shop attributes (item_vnum=%d, price=%d)", item_vnum, price);
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					continue;
				}
				const int item_rnum = GetObjRnum(item_vnum);
				if (item_rnum < 0) {
					snprintf(buf, kMaxStringLength, "...incorrect item_vnum=%d", item_vnum);
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					continue;
				}
				const auto item_price = price == 0 ? obj_proto[item_rnum]->get_cost() : price;
				tmp_shop->add_item(item_vnum, item_price);
			} else if (cname == "item_set") {
				const std::string itemSetId = child.GetValue("id");
				for (const auto &set : item_sets()) {
					if (set->_id != itemSetId) {
						continue;
					}
					for (const auto &entry : set->item_list) {
						const int item_rnum = GetObjRnum(entry.item_vnum);
						if (item_rnum < 0) {
							snprintf(buf, kMaxStringLength, "...incorrect item_vnum=%d in item_set=%s", (int) entry.item_vnum, set->_id.c_str());
							mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
							continue;
						}
						const auto item_price = entry.item_price == 0 ? obj_proto[item_rnum]->get_cost() : entry.item_price;
						tmp_shop->add_item(entry.item_vnum, item_price);
					}
				}
			}
		}

		if (tmp_shop->empty()) {
			snprintf(buf, kMaxStringLength, "...item list empty (shop_id=%s)", shop_id.c_str());
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			continue;
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

		// Assign the shop special only to keepers of a shop that actually loaded.
		for (const auto &mob_vnum : tmp_shop->mob_vnums()) {
			const auto keeper_rnum = GetMobRnum(mob_vnum);
			if (keeper_rnum < 0) {
				snprintf(buf, kMaxStringLength, "...incorrect mob_vnum=%d", mob_vnum);
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				continue;
			}
			if (mob_index[keeper_rnum].func && mob_index[keeper_rnum].func != shop_ext) {
				snprintf(buf, kMaxStringLength, "...shopkeeper already with special (mob_vnum=%d)", mob_vnum);
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				continue;
			}
			specials::RegisterMob(mob_vnum, specials::ESpecial::kShop);
		}

		shop_list.push_back(tmp_shop);
	}
	log_shop_load();
}

void ShopsLoader::Reload(parser_wrapper::DataNode data) {
	Load(data);
}

std::string ShopsLoader::EditableWhat() const {
	return "shop";
}

std::vector<cfg_manager::EditableElement> ShopsLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &shop : shop_list) {
		out.push_back({shop->id, shop->id});
	}
	return out;
}

cfg_manager::ValidationResult ShopsLoader::Validate(parser_wrapper::DataNode &doc) const {
	for (auto node : doc.Children()) {
		if (std::string(node.GetName()) != "shop") {
			continue;
		}
		if (std::string(node.GetValue("id")).empty()) {
			return {false, "a <shop> has an empty id"};
		}
	}
	return {true, ""};
}

std::string ShopsLoader::CanonicalElementId(const std::string &id) const {
	return IsValidShopId(id) ? id : "";   // CamelCase: letters/digits, starts with a letter
}

parser_wrapper::DataNode ShopsLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	auto node = root.AddChild("shop");
	node.SetValue("id", id);
	node.SetValue("currency", "kKuna");
	node.SetValue("profit", "0");
	node.SetValue("waste_time_min", "180");
	node.SetValue("can_buy", "false");
	return node;   // keeper mobs and stock are added afterwards in the editor
}

// Thin entry points: route through cfg_manager so boot, `reload shop` and the item-set
// editor share one path (cfg_manager supplies the DataNode).
void load(bool reload) {
	if (reload) {
		MUD::CfgManager().ReloadCfg("shops");
	} else {
		MUD::CfgManager().LoadCfg("shops");
	}
}

int get_spent_today() {
	return spent_today;
}

} // namespace ShopExt

using namespace ShopExt;

namespace {
// issue.specials: shop subcommands. One function per action; the resolver maps the synonym words to
// these handlers and auto-generates the "Чего желаете" tooltip from the same list. The legacy steal
// override is gone (shops are entity-verb "магазин <действие>" now).
enum class EShopCmd { kList, kBuy, kSell, kValue, kRepair, kFilter, kExamine, kIdentify };

shop_node::shared_ptr ShopOf(CharData *ch, CharData *keeper) {
	for (const auto &s : shop_list) {
		if (std::find(s->mob_vnums().begin(), s->mob_vnums().end(), GET_MOB_VNUM(keeper)) != s->mob_vnums().end()) {
			return s;
		}
	}
	log("SYSERROR : магазин не найден mob_vnum=%d (%s:%d)", GET_MOB_VNUM(keeper), __FILE__, __LINE__);
	SendMsgToChar(specials::ShopMsg(specials::EShopMsg::kError) + "\r\n", ch);
	return nullptr;
}

int ShopList(CharData *ch, void *me, char *rest) {
	auto *const keeper = reinterpret_cast<CharData *>(me);
	const auto shop = ShopOf(ch, keeper);
	if (!shop) {
		return 1;
	}
	std::string buffer = rest, buffer2;
	GetOneParam(buffer, buffer2);
	shop->print_shop_list(ch, buffer2, GET_MOB_VNUM(keeper));
	return 1;
}

int ShopFilter(CharData *ch, void *me, char *rest) {
	auto *const keeper = reinterpret_cast<CharData *>(me);
	const auto shop = ShopOf(ch, keeper);
	if (!shop) {
		return 1;
	}
	shop->filter_shop_list(ch, rest, GET_MOB_VNUM(keeper));
	return 1;
}

int ShopBuy(CharData *ch, void *me, char *rest) {
	auto *const keeper = reinterpret_cast<CharData *>(me);
	const auto shop = ShopOf(ch, keeper);
	if (!shop) {
		return 1;
	}
	shop->process_buy(ch, keeper, rest);
	return 1;
}

int ShopSell(CharData *ch, void *me, char *rest) {
	auto *const keeper = reinterpret_cast<CharData *>(me);
	const auto shop = ShopOf(ch, keeper);
	if (!shop) {
		return 1;
	}
	shop->process_cmd(ch, keeper, rest, "Продать");
	return 1;
}

int ShopValue(CharData *ch, void *me, char *rest) {
	auto *const keeper = reinterpret_cast<CharData *>(me);
	const auto shop = ShopOf(ch, keeper);
	if (!shop) {
		return 1;
	}
	shop->process_cmd(ch, keeper, rest, "Оценить");
	return 1;
}

int ShopRepair(CharData *ch, void *me, char *rest) {
	auto *const keeper = reinterpret_cast<CharData *>(me);
	const auto shop = ShopOf(ch, keeper);
	if (!shop) {
		return 1;
	}
	shop->process_cmd(ch, keeper, rest, "Чинить");
	return 1;
}

int ShopExamine(CharData *ch, void *me, char *rest) {
	auto *const keeper = reinterpret_cast<CharData *>(me);
	const auto shop = ShopOf(ch, keeper);
	if (!shop) {
		return 1;
	}
	shop->process_ident(ch, keeper, rest, "Рассмотреть");
	return 1;
}

int ShopIdentify(CharData *ch, void *me, char *rest) {
	auto *const keeper = reinterpret_cast<CharData *>(me);
	const auto shop = ShopOf(ch, keeper);
	if (!shop) {
		return 1;
	}
	shop->process_ident(ch, keeper, rest, "Характеристики");
	return 1;
}

const SubCmdResolver kShopCmds([] { return specials::ShopMsg(specials::EShopMsg::kGreeting); }, {
	{{"список", "list"}, static_cast<int>(EShopCmd::kList), ShopList},
	{{"купить", "buy"}, static_cast<int>(EShopCmd::kBuy), ShopBuy},
	{{"продать", "sell"}, static_cast<int>(EShopCmd::kSell), ShopSell},
	{{"оценить", "value"}, static_cast<int>(EShopCmd::kValue), ShopValue},
	{{"чинить", "repair"}, static_cast<int>(EShopCmd::kRepair), ShopRepair},
	{{"фильтровать", "filter"}, static_cast<int>(EShopCmd::kFilter), ShopFilter},
	{{"рассмотреть", "examine"}, static_cast<int>(EShopCmd::kExamine), ShopExamine},
	{{"характеристики", "identify"}, static_cast<int>(EShopCmd::kIdentify), ShopIdentify},
});
} // namespace

int shop_ext(CharData *ch, void *me, int /*cmd*/, char *argument) {
	if (!ch->desc
		|| ch->IsNpc()) {
		return 0;
	}
	return kShopCmds.Dispatch(ch, me, argument);
}

// * Лоад странствующих продавцов в каждой ренте.
void town_shop_keepers() {
	// список уже оработанных зон, чтобы не грузить двух и более торгашей в одну
	std::set<int> zone_list;

	for (const auto &ch : character_list) {
		if (specials::IsRentkeeper(ch.get())
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
		if ((currencies::GetBank(*ch, currencies::kGold) < kChestIdentPay) && (GetRealLevel(ch) < kLvlImplementator)) {
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
				if (isname(stufina, obj->get_PName(grammar::ECase::kNom))) {
					SendMsgToChar(ch, "Характеристики предмета: %s\r\n", stufina);
					MortShowObjValues(obj, ch, 200);
					currencies::RemoveBank(*ch, currencies::kGold, kChestIdentPay);
					SendMsgToChar(ch,
								  "&GЗа информацию о предмете с вашего банковского счета сняли %d %s&n\r\n",
								  kChestIdentPay,
								  MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(kChestIdentPay, grammar::ECase::kNom).c_str());
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
