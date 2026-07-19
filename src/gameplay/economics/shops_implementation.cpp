#include "shops_implementation.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/sight.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/identify.h"

#include "engine/db/obj_prototypes.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/glory_const.h"
#include "gameplay/economics/currencies.h"
#include "gameplay/quests/daily_quest.h"
#include "currencies.h"
#include "engine/db/global_objects.h"
#include "engine/db/world_objects.h"
#include "engine/core/obj_handler.h"
#include "engine/core/target_resolver.h"
#include "gameplay/mechanics/inventory.h"
#include "engine/ui/modify.h"
#include "gameplay/mechanics/named_stuff.h"
#include "gameplay/fight/pk.h"
#include "engine/entities/zone.h"
#include "engine/ui/objects_filter.h"
#include "gameplay/communication/talk.h"
#include "engine/ui/cmd_god/do_echo.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/ai/special_messages.h"

#include <fmt/format.h>
#include <sstream>

extern int invalid_anti_class(CharData *ch, const ObjData *obj);    // implemented in class.cpp
extern int invalid_unique(CharData *ch, const ObjData *obj);    // implemented in class.cpp
extern int invalid_no_class(CharData *ch, const ObjData *obj);    // implemented in class.cpp
extern int invalid_anti_class_proto(CharData *ch, const CObjectPrototype *obj);    // implemented in class.cpp
extern int invalid_no_class_proto(CharData *ch, const CObjectPrototype *obj);    // implemented in class.cpp
const char *sight::find_exdesc(const char *word, const std::vector<ExtraDescription> &list); // implemented in act.informative.cpp
namespace ShopExt {
const int IDENTIFY_COST = 110;

// issue.specials Phase 2: shop keeper messages live in shop_msg.xml. ShopMsg() returns the raw text;
// callers add the trailing "\r\n" for SendMsgToChar, fmt::format(fmt::runtime(..)) for named args.
using specials::ShopMsg;
using ESM = specials::EShopMsg;

// The keeper's "no money" flavour reaction (shared by process_buy and Характеристики): a random scold
// (kSwearing, an act line wrapped in the social colour &K..&n) or a drink emote (gender picks the
// beverage). Both are data-driven messages now.
static void KeeperNoMoneyReaction(CharData *keeper, CharData *ch) {
	switch (number(0, 3)) {
		case 0:
			act(("&K" + ShopMsg(ESM::kSwearing) + "&n").c_str(), false, keeper, nullptr, ch, kToRoom);
			break;
		case 1: {
			char local_buf[kMaxInputLength];
			snprintf(local_buf, kMaxInputLength, "%s",
					 ShopMsg(IsMale(keeper) ? ESM::kDrinkEmoteMale : ESM::kDrinkEmoteFemale).c_str());
			do_echo(keeper, local_buf, 0, kScmdEmote);
			break;
		}
	}
}

int spent_today = 0;

bool check_money(CharData *ch, long price, const std::string &currency) {
	return currencies::GetHand(*ch, currency) >= price;
}

void GoodsStorage::ObjectUIDChangeObserver::notify(ObjData &object, const int old_uid) {
	const auto i = m_parent.m_objects_by_uid.find(old_uid);
	if (i == m_parent.m_objects_by_uid.end()) {
		log("LOGIC ERROR: Got notification about changing UID %d of the object that is not registered. "
			"Won't do anything.",
			old_uid);
		return;
	}

	if (i->second != &object) {
		log("LOGIC ERROR: Got notification about changing UID %d of the object at address %p. But object with such "
			"UID is registered at address %p. Won't do anything.",
			old_uid, &object, i->second);
		return;
	}

	m_parent.m_objects_by_uid.erase(i);
	m_parent.m_objects_by_uid.emplace(object.get_unique_id(), &object);
}

void GoodsStorage::add(ObjData *object) {
	const auto activity_i = m_activities.find(object);
	if (activity_i != m_activities.end()) {
		log("LOGIC ERROR: Try to add object at ptr %p twice. Won't do anything. Object VNUM: %d",
			object, object->get_vnum());
		return;
	}

	const auto uid_i = m_objects_by_uid.find(object->get_unique_id());
	if (uid_i != m_objects_by_uid.end()) {
		log("LOGIC ERROR: Try to add object at ptr %p with UID %ld but such UID already presented for the object at address %p. "
			"Won't do anything. VNUM of the addee object: %d; VNUM of the presented object: %d.",
			object, object->get_unique_id(), uid_i->second, object->get_vnum(), uid_i->second->get_vnum());
		return;
	}

	m_activities.emplace(object, static_cast<int>(time(nullptr)));
	m_objects_by_uid.emplace(object->get_unique_id(), object);
	object->subscribe_for_unique_id_change(m_object_uid_change_observer);
}

void GoodsStorage::remove(ObjData *object) {
	std::stringstream error;

	object->unsubscribe_from_unique_id_change(m_object_uid_change_observer);

	const auto uid = object->get_unique_id();
	const auto object_by_uid_i = m_objects_by_uid.find(uid);
	if (object_by_uid_i != m_objects_by_uid.end()) {
		m_objects_by_uid.erase(object_by_uid_i);
	} else {
		error << "Try to remove object with UID " << uid << " but such UID is not registered.";
	}

	const auto activity_i = m_activities.find(object);
	if (activity_i != m_activities.end()) {
		m_activities.erase(activity_i);
	} else {
		if (0 < error.tellp()) {
			error << " ";
		}
		error << "Try to remove object at address " << object << " but object at this address is not registered.";
	}

	if (0 < error.tellp()) {
		log("LOGIC ERROR: %s", error.str().c_str());
	}
}

ObjData *GoodsStorage::get_by_uid(const int uid) const {
	const auto i = m_objects_by_uid.find(uid);
	if (i != m_objects_by_uid.end()) {
		return i->second;
	}

	return nullptr;
}

void GoodsStorage::clear() {
	m_activities.clear();
	for (const auto &uid_pair : m_objects_by_uid) {
		uid_pair.second->unsubscribe_from_unique_id_change(m_object_uid_change_observer);
	}
	m_objects_by_uid.clear();
}

const std::string &ItemNode::get_item_name(int keeper_vnum, grammar::ECase name_case /*= ECase::kNom*/) const {
	const auto desc_i = m_descs.find(keeper_vnum);
	if (desc_i != m_descs.end()) {
		return desc_i->second.PNames[name_case];
	} else {
		const auto rnum = obj_proto.get_rnum(m_vnum);
		const static std::string wrong_vnum = "<unknown VNUM>";
		if (-1 == rnum) {
			return wrong_vnum;
		}
		return obj_proto[rnum]->get_PName(name_case);
	}
}

void ItemNode::replace_descs(ObjData *obj, const int vnum) const {
	const auto desc_i = m_descs.find(vnum);
	if (!obj
		|| desc_i == m_descs.end()) {
		return;
	}

	const auto &desc = desc_i->second;

	obj->set_description(desc.description);
	obj->set_aliases(desc.name);
	obj->set_short_description(desc.short_description.c_str());
	obj->set_PName(grammar::ECase::kNom, desc.PNames[grammar::ECase::kNom].c_str());
	obj->set_PName(grammar::ECase::kGen, desc.PNames[grammar::ECase::kGen].c_str());
	obj->set_PName(grammar::ECase::kDat, desc.PNames[grammar::ECase::kDat].c_str());
	obj->set_PName(grammar::ECase::kAcc, desc.PNames[grammar::ECase::kAcc].c_str());
	obj->set_PName(grammar::ECase::kIns, desc.PNames[grammar::ECase::kIns].c_str());
	obj->set_PName(grammar::ECase::kPre, desc.PNames[grammar::ECase::kPre].c_str());
	obj->set_sex(desc.sex);

	if (!desc.trigs.empty()) {
		obj->attach_triggers(desc.trigs);
	}

	obj->ex_descriptions().clear(); //Пока в конфиге нельзя указать экстраописания - убираем нафиг

	if ((obj->get_type() == EObjType::kLiquidContainer)
		&& (GET_OBJ_VAL(obj, 1) > 0)) //Если работаем с непустой емкостью...
	{
		name_to_drinkcon(obj, GET_OBJ_VAL(obj, 2)); //...Следует указать содержимое емкости
	}
}

void ItemsList::add(const ObjVnum vnum, const long price, const ItemNode::uid_t uid) {
	const auto item = std::make_shared<ItemNode>(vnum, price, uid);
	m_items_list.push_back(std::move(item));
}

void ItemsList::add(const ObjVnum vnum, const long price) {
	const auto item = std::make_shared<ItemNode>(vnum, price);
	m_items_list.push_back(std::move(item));
}

const ItemsList::items_list_t::value_type &ItemsList::node(const size_t index) const {
	if (index < m_items_list.size()) {
		return m_items_list[index];
	}

	// if we are here then this is an error
	log("LOGIC ERROR: Requested shop item with too high index %zd but shop has only %zd items", index, size());
	static decltype(m_items_list)::value_type null_ptr;
	return null_ptr;
}

void shop_node::process_buy(CharData *ch, CharData *keeper, char *argument) {
	std::string buffer2(argument), buffer1;
	GetOneParam(buffer2, buffer1);
	utils::Trim(buffer2);

	if (buffer1.empty()) {
		tell_to_char(keeper, ch, ShopMsg(ESM::kBuyWhat).c_str());
		return;
	}

	int item_num = 0, item_count = 1;

	if (buffer2.empty()) {
		if (is_number(buffer1.c_str())) {
			// buy 5
			try {
				item_num = std::stoul(buffer1);
			}
			catch (const std::exception &) {
				item_num = 0;
			}
		} else {
			// buy sword
			item_num = get_item_num(buffer1, GET_MOB_VNUM(keeper));
		}
	} else if (is_number(buffer1.c_str())) {
		if (is_number(buffer2.c_str())) {
			// buy 5 10
			try {
				item_num = std::stoul(buffer2);
			}
			catch (std::exception &) {
				item_num = 0;
			}
		} else {
			// buy 5 sword
			item_num = get_item_num(buffer2, GET_MOB_VNUM(keeper));
		}
		try {
			item_count = std::stoul(buffer1);
		}
		catch (std::exception &) {
			item_count = 1000;
		}
	} else {
		tell_to_char(keeper, ch, ShopMsg(ESM::kBuyWhat).c_str());
		return;
	}

	if (!item_count
		|| !item_num
		|| static_cast<size_t>(item_num) > m_items_list.size()) {
		tell_to_char(keeper, ch, ShopMsg(ESM::kNoSuchItem).c_str());
		return;
	}

	if (item_count >= 1000) {
		tell_to_char(keeper, ch, ShopMsg(ESM::kTooGreedy).c_str());
		return;
	}

	const auto item_index = item_num - 1;

	CObjectPrototype *tmp_obj = nullptr;
	bool obj_from_proto = true;
	const auto item = m_items_list.node(item_index);
	if (!item->empty()) {
		tmp_obj = get_from_shelve(item_index);

		if (!tmp_obj) {
			log("SYSERROR : не удалось прочитать предмет (%s:%d)", __FILE__, __LINE__);
			SendMsgToChar(ShopMsg(ESM::kError) + "\r\n", ch);
			return;
		}

		obj_from_proto = false;
	}

	auto proto = (tmp_obj ? tmp_obj : GetObjectPrototype(item->vnum()).get());
	if (!proto) {
		log("SYSERROR : не удалось прочитать прототип (%s:%d)", __FILE__, __LINE__);
		SendMsgToChar(ShopMsg(ESM::kError) + "\r\n", ch);
		return;
	}

	// Блокируем продажу только для anti-класса ("Недоступен"): такую вещь нельзя
	// ни носить, ни взять (обжигает). Вещи с no-классом ("Неудобен") продаём:
	// носить их можно, к тому же в магазине покупают и чармисам (issue #3356).
	if (invalid_anti_class_proto(ch, proto)) {
		tell_to_char(keeper, ch, ShopMsg(ESM::kWrongClass).c_str());
		return;
	}

	const long price = item->get_price();
	if (!check_money(ch, price, currency)) {
		tell_to_char(keeper, ch, fmt::format(fmt::runtime(ShopMsg(ESM::kCantAfford)),
				fmt::arg("currency", MUD::Currencies().FindAvailableItem(currency).GetPluralName(grammar::ECase::kGen))).c_str());
		KeeperNoMoneyReaction(keeper, ch);
		return;
	}

	if ((ch->GetCarryingQuantity() + 1 > CAN_CARRY_N(ch))
		|| ((ch->GetCarryingWeight() + proto->get_weight()) > CAN_CARRY_W(ch))) {
		const auto &name = obj_from_proto
						   ? item->get_item_name(GET_MOB_VNUM(keeper), grammar::ECase::kAcc).c_str()
						   : tmp_obj->get_short_description().c_str();
		SendMsgToChar(fmt::format(fmt::runtime(ShopMsg(ESM::kHandsFull1)),
				fmt::arg("name", GET_NAME(ch))) + "\r\n", ch);
		SendMsgToChar(fmt::format(fmt::runtime(ShopMsg(ESM::kHandsFull2)),
				fmt::arg("item", name)) + "\r\n", ch);
		return;
	}

	int bought = 0;
	int total_money = 0;
	int sell_count = can_sell_count(item_index);

	ObjData *obj = nullptr;
	while (bought < item_count
		&& check_money(ch, price, currency)
		&& ch->GetCarryingQuantity() < CAN_CARRY_N(ch)
		&& ch->GetCarryingWeight() + proto->get_weight() <= CAN_CARRY_W(ch)
		&& (bought < sell_count || sell_count == -1)) {
		if (!item->empty()) {
			obj = get_from_shelve(item_index);
			if (obj != nullptr) {
				item->remove_uid(obj->get_unique_id());
				if (item->empty()) {
					m_items_list.remove(item_index);
				}
				remove_from_storage(obj);
			}
		} else {
			obj = world_objects.create_from_prototype_by_vnum(item->vnum()).get();
			item->replace_descs(obj, GET_MOB_VNUM(keeper));
		}

		if (obj) {
			load_otrigger(obj);
			obj->set_vnum_zone_from(GetZoneVnumByCharPlace(ch));
			if (obj->has_flag(EObjFlag::kBindOnPurchase)) {
				obj->set_owner(ch->get_uid());
			}
			PlaceObjToInventory(obj, ch);
			obj->set_where_obj(EWhereObj::kCharInventory);
			// книги, купленные не за основную валюту (золото), не ломаются
			if (currency != currencies::kGold && EObjType::kBook == obj->get_type()) {
				obj->set_extra_flag(EObjFlag::kNofail);
			}
			currencies::RemoveHand(*ch, currency, price);
			if (currency == currencies::kGlory) {
				// учёт постоянной славы во внешнем хранилище GloryConst
				GloryConst::add_total_spent(price);
				GloryConst::transfer_log("%s bought %s for %ld const glory",
										 GET_NAME(ch), proto->get_PName(grammar::ECase::kNom).c_str(), price);
			} else if (currency == currencies::kGold) {
				spent_today += price;
			}
			// трата валюты может затронуть систему дейликов
			if (DailyQuest::OnCurrencySpent(ch, currency, price)) {
				SendMsgToChar(ShopMsg(ESM::kHryvnReset) + "\r\n", ch);
			}
			++bought;

			total_money += price;
		} else {
			log("SYSERROR : не удалось загрузить предмет ObjVnum=%d (%s:%d)",
				GET_OBJ_VNUM(proto), __FILE__, __LINE__);
			SendMsgToChar(ShopMsg(ESM::kError) + "\r\n", ch);
			return;
		}
	}

	if (bought < item_count) {
		std::string reply;
		if (!check_money(ch, price, currency)) {
			reply = fmt::format(fmt::runtime(ShopMsg(IsMale(ch) ? ESM::kCheaterMale : ESM::kCheaterFemale)),
					fmt::arg("count", bought));
		} else if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)) {
			reply = fmt::format(fmt::runtime(ShopMsg(ESM::kCarryOnly)), fmt::arg("count", bought));
		} else if (ch->GetCarryingWeight() + proto->get_weight() > CAN_CARRY_W(ch)) {
			reply = fmt::format(fmt::runtime(ShopMsg(ESM::kLiftOnly)), fmt::arg("count", bought));
		} else if (bought > 0) {
			reply = fmt::format(fmt::runtime(ShopMsg(ESM::kSellOnly)), fmt::arg("count", bought));
		} else {
			reply = ShopMsg(ESM::kSellNothing);
		}

		tell_to_char(keeper, ch, reply.c_str());
	}
	const std::string suffix = MUD::Currencies().FindAvailableItem(currency).GetNameWithAmount(total_money, grammar::ECase::kAcc);

	tell_to_char(keeper, ch, fmt::format(fmt::runtime(ShopMsg(ESM::kPrice)),
			fmt::arg("amount", total_money), fmt::arg("currency", suffix)).c_str());

	if (obj) {
		if ((obj->get_cost() * bought) > total_money) {

			snprintf(buf,
					 kMaxStringLength,
					 "Персонаж %s купил предмет %d за %d при его стоимости %d и прайсе %ld.",
					 ch->get_name().c_str(),
					 GET_OBJ_VNUM(obj),
					 total_money,
					 obj->get_cost(),
					 price);
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		}
		SendMsgToChar(fmt::format(fmt::runtime(ShopMsg(IsMale(ch) ? ESM::kHappyOwnerMale : ESM::kHappyOwnerFemale)),
				fmt::arg("item", obj->item_count_message(bought, grammar::ECase::kGen))) + "\r\n", ch);
	}
}

void shop_node::print_shop_list(CharData *ch, const std::string &arg, int keeper_vnum) const {
	SendMsgToChar(fmt::format(fmt::runtime(ShopMsg(ESM::kListHeader)), fmt::arg("currency", MUD::Currencies().FindAvailableItem(currency).GetPluralName(grammar::ECase::kNom))) + "\r\n"
				  "---------------------------------------------------------------------------\r\n", ch);
	int num = 1;
	std::stringstream out;
	std::string print_value;
	std::string name_value;

	for (size_t k = 0; k < m_items_list.size();) {
		int count = can_sell_count(num - 1);

		print_value.clear();
		name_value.clear();

		const auto &item = m_items_list.node(k);

		//Polud у проданных в магаз объектов отображаем в списке не значение из прототипа, а уже, возможно, измененное значение
		// чтобы не было в списках всяких "гриб @n1"
		if (item->empty()) {
			print_value = item->get_item_name(keeper_vnum);
			const auto rnum = obj_proto.get_rnum(item->vnum());
			if (obj_proto[rnum]->get_type() == EObjType::kLiquidContainer) {
				print_value += " с " + std::string(drinknames[GET_OBJ_VAL(obj_proto[rnum], 2)]);
			}
		} else {
			const ObjData *tmp_obj = get_from_shelve(k);
			if (tmp_obj) {
				print_value = tmp_obj->get_short_description();
				name_value = tmp_obj->get_aliases();
				item->set_price(tmp_obj->get_cost());
			}
		}

		std::string numToShow = (count == -1 || count > 999 ? ShopMsg(ESM::kBulk) : fmt::format("{}", count));

		// имхо вполне логично раз уж мы получаем эту надпись в ней и искать
		if (arg.empty()
			|| isname(arg, print_value)
			|| (!name_value.empty()
				&& isname(arg, name_value))) {
			auto color_count = std::count(print_value.begin(), print_value.end(), '&')*2 + 45;
			auto format_str = fmt::format("{}{}{}",  "{:4})  {:10}  {:<", color_count,"} {:8}\r\n");
			out << fmt::format(fmt::runtime(format_str), num++, numToShow, print_value, item->get_price());
		} else {
			num++;
		}

		++k;
	}

	page_string(ch->desc, out.str());
}

void shop_node::filter_shop_list(CharData *ch, char *argument, int keeper_vnum) {
	int num = 1;
	std::string print_value;
	std::string name_value;

	one_argument(argument, arg);
	ParseFilter filter(ParseFilter::CLAN);

	if (!filter.parse_filter(ch, filter, argument)) {
			return;
	}

	SendMsgToChar(fmt::format(fmt::runtime(ShopMsg(ESM::kFilterSelection)),
			fmt::arg("filter", filter.print())) + "\r\n", ch);
	SendMsgToChar(fmt::format(fmt::runtime(ShopMsg(ESM::kFilterHeader)), fmt::arg("currency", MUD::Currencies().FindAvailableItem(currency).GetPluralName(grammar::ECase::kNom))) + "\r\n"
				  "---------------------------------------------------------------------------\r\n", ch);
	std::stringstream out;

	for (auto k = 0u; k < m_items_list.size(); k++) {
		int count = can_sell_count(num - 1);
		std::string numToShow = count == -1 ? ShopMsg(ESM::kBulk) : fmt::format("{}", count);

		print_value = "";
		name_value = "";

		const auto &item = m_items_list.node(k);

		//Polud у проданных в магаз объектов отображаем в списке не значение из прототипа, а уже, возможно, измененное значение
		// чтобы не было в списках всяких "гриб @n1"
		if (item->empty()) {
			print_value = item->get_item_name(keeper_vnum);
			const auto rnum = obj_proto.get_rnum(item->vnum());
			if (obj_proto[rnum]->get_type() == EObjType::kLiquidContainer) {
				print_value += " с " + std::string(drinknames[GET_OBJ_VAL(obj_proto[rnum], 2)]);
			}
			auto tmp_obj = world_objects.create_from_prototype_by_rnum(rnum);
			if (tmp_obj && filter.check(tmp_obj.get(), ch)) {
				out << fmt::format("{:>4})  {:>10}  {:<47} {:>8}\r\n",
						 num, numToShow, print_value, item->get_price());
			} 
			if (tmp_obj) {
				world_objects.remove(tmp_obj);
			}
		} else {
			ObjData *tmp_obj = get_from_shelve(k);
			if (tmp_obj) {
				if (filter.check(tmp_obj, ch)) {
					print_value = tmp_obj->get_short_description();
					name_value = tmp_obj->get_aliases();
					item->set_price(tmp_obj->get_cost());
					out << fmt::format("{:>4})  {:>10}  {:<47} {:>8}\r\n",
							   num, numToShow, print_value, item->get_price());
				}
			} else {
				m_items_list.remove(k);    // remove from shop object that we cannot instantiate
				continue;
			}
		}
		num++;
	}
	page_string(ch->desc, out.str());
}

void shop_node::process_cmd(CharData *ch, CharData *keeper, char *argument, const std::string &cmd) {
	std::string buffer(argument), buffer1;
	GetOneParam(buffer, buffer1);
	utils::Trim(buffer);

	if (!can_buy
		&& (cmd == "Продать"
			|| cmd == "Оценить")) {
		tell_to_char(keeper, ch, ShopMsg(ESM::kOwnSuppliers).c_str());

		return;
	}

	if (!*argument) {
		tell_to_char(keeper, ch, fmt::format(fmt::runtime(ShopMsg(ESM::kCmdWhat)),
				fmt::arg("cmd", cmd)).c_str());
		return;
	}

	if (buffer1.empty()) {
		return;
	}

	if (is_number(buffer1.c_str())) {
		int n = 0;
		try {
			n = std::stoi(buffer1, nullptr, 10);
		}
		catch (const std::invalid_argument &) {
		}

		ObjData *obj = get_obj_in_list_vis(ch, buffer, ch->carrying);

		if (!obj) {
			SendMsgToChar(fmt::format(fmt::runtime(ShopMsg(ESM::kNoItem)),
					fmt::arg("item", buffer.empty() ? buffer1 : buffer)) + "\r\n", ch);
			return;
		}

		while (obj && n > 0) {
			const auto obj_next = get_obj_in_list_vis(ch, buffer, obj->get_next_content());
			do_shop_cmd(ch, keeper, obj, cmd);
			obj = obj_next;
			n--;
		}
	} else {
		skip_spaces(&argument);
		int i, dotmode = find_all_dots(argument);
		std::string buffer2(argument);
		switch (dotmode) {
			case kFindIndiv: {
				const auto obj = get_obj_in_list_vis(ch, buffer2, ch->carrying);

				if (!obj) {
					if (cmd == "Чинить" && utils::IsAbbr(argument, "экипировка")) {
						for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
							if (ch->equipment[i]) {
								do_shop_cmd(ch, keeper, ch->equipment[i], cmd);
							}
						}
						return;
					}
					SendMsgToChar(fmt::format(fmt::runtime(ShopMsg(ESM::kNoItem)),
							fmt::arg("item", buffer2)) + "\r\n", ch);
					return;
				}

				do_shop_cmd(ch, keeper, obj, cmd);
			}

				break;

			case kFindAll: {
				ObjData *obj_next = nullptr;
				for (auto obj = ch->carrying; obj; obj = obj_next) {
					obj_next = obj->get_next_content();
					do_shop_cmd(ch, keeper, obj, cmd);
				}
			}

				break;

			case kFindAlldot: {
				auto obj = get_obj_in_list_vis(ch, buffer2, ch->carrying);
				if (!obj) {
					SendMsgToChar(fmt::format(fmt::runtime(ShopMsg(ESM::kNoItem)),
							fmt::arg("item", buffer2)) + "\r\n", ch);
					return;
				}

				while (obj) {
					const auto obj_next = get_obj_in_list_vis(ch, buffer2, obj->get_next_content());
					do_shop_cmd(ch, keeper, obj, cmd);
					obj = obj_next;
				}
			}
				break;

			default: break;
		};
	}
}

void shop_node::process_ident(CharData *ch, CharData *keeper, char *argument, const std::string &cmd) {
	std::string buffer(argument);
	utils::Trim(buffer);

	if (buffer.empty()) {
		tell_to_char(keeper, ch, ShopMsg(ESM::kIdentWhat).c_str());
		return;
	}

	unsigned item_num = 0;
	if (is_number(buffer.c_str())) {
		// характеристики 5
		try {
			item_num = std::stoul(buffer);
		}
		catch (std::exception &) {
		}
	} else {
		// характеристики меч
		item_num = get_item_num(buffer, GET_MOB_VNUM(keeper));
	}

	if (!item_num
		|| item_num > items_list().size()) {
		tell_to_char(keeper, ch, ShopMsg(ESM::kNoSuchItem).c_str());
		return;
	}

	const auto item_index = item_num - 1;

	const ObjData *ident_obj = nullptr;
	ObjData *tmp_obj = nullptr;
	const auto &item = m_items_list.node(item_index);
	if (item->empty()) {
		const auto vnum = GET_MOB_VNUM(keeper);
		if (item->has_desc(vnum)) {
			tmp_obj = world_objects.create_from_prototype_by_vnum(item->vnum()).get();
			item->replace_descs(tmp_obj, vnum);
			ident_obj = tmp_obj;
		} else {
			const auto rnum = obj_proto.get_rnum(item->vnum());
			const auto object = world_objects.create_raw_from_prototype_by_rnum(rnum);
			ident_obj = tmp_obj = object.get();
		}
	} else {
		ident_obj = get_from_shelve(item_index);
	}

	if (!ident_obj) {
		log("SYSERROR : не удалось получить объект (%s:%d)", __FILE__, __LINE__);
		SendMsgToChar(ShopMsg(ESM::kError) + "\r\n", ch);
		return;
	}

	if (cmd == "Рассмотреть") {
		std::stringstream tell;
		tell << fmt::format(fmt::runtime(ShopMsg(ESM::kInspectIntro)),
				fmt::arg("item", ident_obj->get_short_description()),
				fmt::arg("type", item_types[ident_obj->get_type()])) << "\r\n";
		tell << sight::diag_weapon_to_char(ident_obj, true);
		tell << sight::diag_timer_to_char(ident_obj);

		if (CanUseFeat(ch, EFeat::kSkilledTrader)) {
			char mat[kMaxInputLength];
			sprinttype(ident_obj->get_material(), material_name, mat);
			tell << fmt::format(fmt::runtime(ShopMsg(ESM::kMaterial)), fmt::arg("material", mat)) << "\r\n";
		}

		tell_to_char(keeper, ch, tell.str().c_str());

		// если есть расширенное описание - показываем отдельно
		// т.к. в магазине можно обратиться к объекту по номеру -> неизвестно под каким именени обратились к объекту
		// поэтому пытаемся найти расширенное описание по алиасам
		const char *desc = sight::find_exdesc(ident_obj->get_aliases().c_str(), ident_obj->get_ex_description());
		if (desc && strlen(desc)) {
			tell.str(std::string());
			tell << fmt::format(fmt::runtime(ShopMsg(ESM::kInspectExdesc)),
					fmt::arg("item", ident_obj->get_short_description())) << "\r\n";
			tell << desc;
			SendMsgToChar(tell.str().c_str(), ch);
		}

		if (invalid_anti_class(ch, ident_obj)
			|| invalid_unique(ch, ident_obj)
			|| NamedStuff::check_named(ch, ident_obj, 0)) {
			tell_to_char(keeper, ch, ShopMsg(ESM::kCantCarry).c_str());
		}
	}

	if (cmd == "Характеристики") {
		if (currencies::GetHand(*ch, currencies::kGold) < IDENTIFY_COST) {
			tell_to_char(keeper, ch, ShopMsg(ESM::kNoMoney).c_str());
			KeeperNoMoneyReaction(keeper, ch);
		} else {
			tell_to_char(keeper, ch, fmt::format(fmt::runtime(ShopMsg(ESM::kIdentCost)),
					fmt::arg("amount", IDENTIFY_COST),
					fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(IDENTIFY_COST, grammar::ECase::kNom).c_str())).c_str());

			SendMsgToChar(fmt::format(fmt::runtime(ShopMsg(ESM::kIdentResult)),
					fmt::arg("item", ident_obj->get_PName(grammar::ECase::kNom))) + "\r\n", ch);
			MortShowObjValues(ident_obj, ch, 200);
			currencies::RemoveHand(*ch, currencies::kGold, IDENTIFY_COST);
		}
	}

	if (tmp_obj) {
		ExtractObjFromWorld(tmp_obj);
	}
}

void shop_node::clear_store() {
	using to_remove_list_t = std::list<ObjData *>;
	to_remove_list_t to_remove;
	for (const auto &stored_item : m_storage) {
		to_remove.push_back(stored_item.first);
	}

	m_storage.clear();

	for (const auto &item : to_remove) {
		ExtractObjFromWorld(item);
	}
}

void shop_node::remove_from_storage(ObjData *object) {
	m_storage.remove(object);
}

ObjData *shop_node::GetObjFromShop(uid_t uid) const {
	return m_storage.get_by_uid(uid);
}

ObjData *shop_node::get_from_shelve(const size_t index) const {
	const auto node = m_items_list.node(index);
	const auto uid = node->uid();
	if (ItemNode::NO_UID == uid) {
		sprintf(buf, "ERROR: get_from_shelve: вернул NULL, index: %zu", index);
		mudlog(buf, LogMode::BRF, kLvlImplementator, SYSLOG, true);
		return nullptr;
	}

	return m_storage.get_by_uid(uid);
}

unsigned shop_node::get_item_num(std::string &item_name, int keeper_vnum) const {
	int num = 1;
	if (!item_name.empty() && a_isdigit(item_name[0])) {
		std::string::size_type dot_idx = item_name.find_first_of('.');
		if (dot_idx != std::string::npos) {
			std::string first_param = item_name.substr(0, dot_idx);
			item_name.erase(0, ++dot_idx);
			if (is_number(first_param.c_str())) {
				try {
					num = std::stol(first_param, nullptr, 10);
				}
				catch (const std::invalid_argument &) {
					return 0;
				}
			}
		}
	}

	int count = 0;
	std::string name_value = "";
	std::string print_value;
	for (unsigned i = 0; i < items_list().size(); ++i) {
		print_value = "";
		const auto &item = m_items_list.node(i);
		if (item->empty()) {
			name_value = utils::RemoveColors(item->get_item_name(keeper_vnum));
			const auto rnum = obj_proto.get_rnum(item->vnum());
			if (obj_proto[rnum]->get_type() == EObjType::kLiquidContainer) {
				name_value += " " + std::string(drinknames[GET_OBJ_VAL(obj_proto[rnum], 2)]);
			}
		} else {
			ObjData *tmp_obj = get_from_shelve(i);
			if (!tmp_obj) {
				continue;
			}
			name_value = tmp_obj->get_aliases();
			print_value = tmp_obj->get_short_description();
		}

		if (isname(item_name, name_value) || isname(item_name, print_value)) {
			++count;
			if (count == num) {
				return ++i;
			}
		}
	}

	return 0;
}

int shop_node::can_sell_count(const int item_index) const {
	const auto &item = m_items_list.node(item_index);
	if (!item->empty()) {
		return static_cast<int>(item->size());
	} else {
		const auto rnum = obj_proto.get_rnum(item->vnum());
		int numToSell = obj_proto[rnum]->get_max_in_world();
		if (numToSell == 0) {
			return numToSell;
		}

		if (numToSell != -1) {
			numToSell -=
				MIN(numToSell, obj_proto.actual_count(rnum));    //считаем не только онлайн, но и то что в ренте
		}

		return numToSell;
	}
}

void shop_node::put_item_to_shop(ObjData *obj) {
	for (auto index = 0u; index < m_items_list.size(); ++index) {
		const auto &item = m_items_list.node(index);
		if (item->vnum() == obj->get_vnum()) {
			if (item->empty()) {
				ExtractObjFromWorld(obj);
				return;
			} else {
				ObjData *tmp_obj = get_from_shelve(index);
				if (!tmp_obj) {
					continue;
				}

				if (obj->get_type() != EObjType::kMagicComponent //а у них всех один рнум
					|| obj->get_short_description() == tmp_obj->get_short_description()) {
					item->add_uid(obj->get_unique_id());
					put_to_storage(obj);
					return;
				}
			}
		}
	}
	add_item(obj->get_vnum(), obj->get_cost(), obj->get_unique_id());

	put_to_storage(obj);
}

long get_sell_price(ObjData *obj) {
	long cost = obj->get_cost();
	long cost_obj = obj->get_cost();
	int timer = obj_proto[obj->get_rnum()]->get_timer();
	if (timer < obj->get_timer()) {
		obj->set_timer(timer);
	}
	cost = timer <= 0 ? 1 : (long) cost * ((float) obj->get_timer() / (float) timer); //учтем таймер

	// если цена продажи, выше, чем стоймость предмета
	if (cost > cost_obj) {
		cost = cost_obj;
	}

	return std::max(1L, cost);
}

void shop_node::do_shop_cmd(CharData *ch, CharData *keeper, ObjData *obj, std::string cmd) {
	if (!obj) {
		return;
	}

	int rnum = obj->get_rnum();
	if (rnum < 0
		|| obj->has_flag(EObjFlag::kArmored)
		|| obj->has_flag(EObjFlag::kSharpen)
		|| obj->has_flag(EObjFlag::kNodrop)) {
		tell_to_char(keeper, ch, ShopMsg(ESM::kWontDeal).c_str());
		return;
	}

	if (obj->GetPotionValueKey(ObjVal::EValueKey::kCurCharges) <= 0
		&& (obj->get_type() == EObjType::kWand
			|| obj->get_type() == EObjType::kStaff)) {
		tell_to_char(keeper, ch, ShopMsg(ESM::kNoUsedItems).c_str());
		return;
	}

	if (obj->get_type() == EObjType::kContainer
		&& cmd != "Чинить") {
		if (obj->get_contains()) {
			tell_to_char(keeper, ch, ShopMsg(ESM::kNoPigInPoke).c_str());
			return;
		}
	}

	long buy_price = obj->get_cost();
	// валюта выплаты при скупке: валюта магазина, если её разрешено выдавать; иначе золото
	const std::string &payout_currency =
		(currency == currencies::kGlory || !MUD::Currencies().FindAvailableItem(currency).MerchantPayout())
		? currencies::kGold : currency;
	long buy_price_old = get_sell_price(obj);

	int repair = obj->get_maximum_durability() - obj->get_current_durability();
	int repair_price = std::max(1, obj->get_cost() * std::max(0, repair) / std::max(1, obj->get_maximum_durability()));

	// если не купцы, то учитываем прибыль магазина, если купцы, то назначаем цену, при которой объект был куплен
	if (!CanUseFeat(ch, EFeat::kSkilledTrader)) {
		buy_price = std::max(1L, (buy_price * profit) / 100); //учтем прибыль магазина
	} else {
		buy_price = get_sell_price(obj);
	}

	// если цена покупки, выше, чем стоймость предмета
	if (buy_price > buy_price_old) {
		buy_price = buy_price_old;
	}

	if (cmd == "Оценить") {
		if (bloody::is_bloody(obj)) {
			tell_to_char(keeper, ch, ShopMsg(ESM::kBloodyValue).c_str());
			return;
		}

		if (obj->has_flag(EObjFlag::kNosell)
			|| obj->has_flag(EObjFlag::kNamed)
			|| obj->has_flag(EObjFlag::kRepopDecay)
			|| obj->has_flag(EObjFlag::kZonedecay)) {
			tell_to_char(keeper, ch, ShopMsg(ESM::kWontBuy).c_str());
			return;
		} else {
			tell_to_char(keeper, ch, fmt::format(fmt::runtime(ShopMsg(ESM::kValueOffer)),
					fmt::arg("item", obj->get_PName(grammar::ECase::kAcc)), fmt::arg("amount", buy_price),
					fmt::arg("currency", MUD::Currencies().FindAvailableItem(payout_currency).GetNameWithAmount(buy_price, grammar::ECase::kNom).c_str())).c_str());
		}
	}

	if (cmd == "Продать") {
		if (obj->has_flag(EObjFlag::kNosell)
			|| obj->has_flag(EObjFlag::kNamed)
			|| obj->has_flag(EObjFlag::kRepopDecay)
			|| (buy_price <= 1)
			|| obj->has_flag(EObjFlag::kZonedecay)
			|| bloody::is_bloody(obj)) {
			if (bloody::is_bloody(obj)) {
				tell_to_char(keeper, ch, ShopMsg(ESM::kBloodySell).c_str());
			} else {
				tell_to_char(keeper, ch, ShopMsg(ESM::kWontBuy).c_str());
			}

			return;
		} else {
			RemoveObjFromChar(obj);
			tell_to_char(keeper, ch, fmt::format(fmt::runtime(ShopMsg(ESM::kSellPaid)),
					fmt::arg("item", obj->get_PName(grammar::ECase::kAcc)), fmt::arg("amount", buy_price),
					fmt::arg("currency", MUD::Currencies().FindAvailableItem(payout_currency).GetNameWithAmount(buy_price, grammar::ECase::kNom).c_str())).c_str());
			currencies::AddHand(*ch, payout_currency, buy_price);
			put_item_to_shop(obj);
			obj->set_where_obj(EWhereObj::kSeller);
		}
	}
	if (cmd == "Чинить") {
		if (bloody::is_bloody(obj)) {
			tell_to_char(keeper, ch, ShopMsg(ESM::kBloodyRepair).c_str());
			return;
		}

		if (repair <= 0) {
			tell_to_char(keeper, ch, fmt::format(fmt::runtime(ShopMsg(ESM::kNoRepairNeeded)),
					fmt::arg("item", obj->get_PName(grammar::ECase::kAcc))).c_str());
			return;
		}

		switch (obj->get_material()) {
			case EObjMaterial::kBulat:
			case EObjMaterial::kCrystal:
			case EObjMaterial::kDiamond:
			case EObjMaterial::kForgedSteel: repair_price *= 2;
				break;

			case EObjMaterial::kHardWood:
			case EObjMaterial::kPreciousMetel:
			case EObjMaterial::kGlass:
			case EObjMaterial::kBronze:
			case EObjMaterial::kCeramic:
			case EObjMaterial::kBone:
			case EObjMaterial::kOrganic: repair_price += MAX(1, repair_price / 2);
				break;

			case EObjMaterial::kIron:
			case EObjMaterial::kSteel:
			case EObjMaterial::kSkin:
			case EObjMaterial::kCloth:
				//repair_price = repair_price;
				break;

			default:
				//repair_price = repair_price;
				break;
		}

		if (repair_price <= 0
			|| obj->has_flag(EObjFlag::kDecay)
			|| obj->has_flag(EObjFlag::kNosell)
			|| obj->has_flag(EObjFlag::kNodrop)) {
			tell_to_char(keeper, ch, fmt::format(fmt::runtime(ShopMsg(ESM::kWontRepair)),
					fmt::arg("item", obj->get_PName(grammar::ECase::kAcc))).c_str());
			return;
		}
		std::string tell = fmt::format(fmt::runtime(ShopMsg(ESM::kRepairCost)),
				fmt::arg("item", obj->get_PName(grammar::ECase::kGen)), fmt::arg("amount", repair_price),
				fmt::arg("currency", MUD::Currency(currencies::kGoldVnum).GetNameWithAmount(repair_price, grammar::ECase::kNom).c_str()));
		tell_to_char(keeper, ch, tell.c_str());

		if (!privilege::IsGod(ch) && repair_price > currencies::GetHand(*ch, currencies::kGold)) {
			act(ShopMsg(ESM::kCantAffordRepair).c_str(), false, ch, 0, 0, kToChar);
			return;
		}

		if (!privilege::IsGod(ch)) {
			currencies::RemoveHand(*ch, currencies::kGold, repair_price);
		}

		act(ShopMsg(ESM::kRepaired).c_str(), false, keeper, obj, 0, kToRoom);

		obj->set_current_durability(obj->get_maximum_durability());
	}
}
}
