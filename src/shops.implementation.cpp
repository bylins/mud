#include "shops.implementation.hpp"

#include "object.prototypes.hpp"
#include "logger.hpp"
#include "utils.h"
#include "liquid.hpp"
#include "char.hpp"
#include "glory.hpp"
#include "glory_const.hpp"
#include "ext_money.hpp"
#include "world.objects.hpp"
#include "handler.h"
#include "modify.h"
#include "named_stuff.hpp"
#include "pk.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <sstream>

extern int do_social(CHAR_DATA * ch, char *argument);	// implemented in the act.social.cpp
extern void do_echo(CHAR_DATA *ch, char *argument, int cmd, int subcmd);	// implemented in the act.wizard.cpp
extern char *diag_weapon_to_char(const CObjectPrototype* obj, int show_wear);	// implemented in the act.informative.cpp
extern char *diag_timer_to_char(const OBJ_DATA * obj);	// implemented in the act.informative.cpp
extern int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);	// implemented in class.cpp
extern int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj);	// implemented in class.cpp
extern int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);	// implemented in class.cpp
extern void mort_show_obj_values(const OBJ_DATA * obj, CHAR_DATA * ch, int fullness);	// implemented in spells.cpp
namespace ShopExt
{
	const int IDENTIFY_COST = 110;

	int spent_today = 0;

	bool check_money(CHAR_DATA *ch, long price, const std::string& currency)
	{
		if (currency == "слава")
		{
			const auto total_glory = GloryConst::get_glory(GET_UNIQUE(ch));
			return total_glory >= price;
		}

		if (currency == "куны")
		{
			return ch->get_gold() >= price;
		}

		if (currency == "лед")
		{
			return ch->get_ice_currency() >= price;
		}
		if (currency == "гривны")
		{
			return ch->get_hryvn() >= price;
		}

		return false;
	}

	void GoodsStorage::ObjectUIDChangeObserver::notify(OBJ_DATA& object, const int old_uid)
	{
		const auto i = m_parent.m_objects_by_uid.find(old_uid);
		if (i == m_parent.m_objects_by_uid.end())
		{
			log("LOGIC ERROR: Got notification about changing UID %d of the object that is not registered. "
				"Won't do anything.",
				old_uid);
			return;
		}

		if (i->second != &object)
		{
			log("LOGIC ERROR: Got notification about changing UID %d of the object at address %p. But object with such "
				"UID is registered at address %p. Won't do anything.",
				old_uid, &object, i->second);
			return;
		}

		m_parent.m_objects_by_uid.erase(i);
		m_parent.m_objects_by_uid.emplace(object.get_uid(), &object);
	}

	void GoodsStorage::add(OBJ_DATA* object)
	{
		const auto activity_i = m_activities.find(object);
		if (activity_i != m_activities.end())
		{
			log("LOGIC ERROR: Try to add object at ptr %p twice. Won't do anything. Object VNUM: %d",
				object, object->get_vnum());
			return;
		}

		const auto uid_i = m_objects_by_uid.find(object->get_uid());
		if (uid_i != m_objects_by_uid.end())
		{
			log("LOGIC ERROR: Try to add object at ptr %p with UID %d but such UID already presented for the object at address %p. "
				"Won't do anything. VNUM of the addee object: %d; VNUM of the presented object: %d.",
				object, object->get_uid(), uid_i->second, object->get_vnum(), uid_i->second->get_vnum());
			return;
		}

		m_activities.emplace(object, static_cast<int>(time(NULL)));
		m_objects_by_uid.emplace(object->get_uid(), object);
		object->subscribe_for_uid_change(m_object_uid_change_observer);
	}

	void GoodsStorage::remove(OBJ_DATA* object)
	{
		std::stringstream error;

		object->unsubscribe_from_uid_change(m_object_uid_change_observer);

		const auto uid = object->get_uid();
		const auto object_by_uid_i = m_objects_by_uid.find(uid);
		if (object_by_uid_i != m_objects_by_uid.end())
		{
			m_objects_by_uid.erase(object_by_uid_i);
		}
		else
		{
			error << "Try to remove object with UID " << uid << " but such UID is not registered.";
		}

		const auto activity_i = m_activities.find(object);
		if (activity_i != m_activities.end())
		{
			m_activities.erase(activity_i);
		}
		else
		{
			if (0 < error.tellp())
			{
				error << " ";
			}
			error << "Try to remove object at address " << object << " but object at this address is not registered.";
		}

		if (0 < error.tellp())
		{
			log("LOGIC ERROR: %s", error.str().c_str());
		}
	}

	OBJ_DATA* GoodsStorage::get_by_uid(const int uid) const
	{
		const auto i = m_objects_by_uid.find(uid);
		if (i != m_objects_by_uid.end())
		{
			return i->second;
		}

		return nullptr;
	}

	void GoodsStorage::clear()
	{
		m_activities.clear();
		for (const auto& uid_pair : m_objects_by_uid)
		{
			uid_pair.second->unsubscribe_from_uid_change(m_object_uid_change_observer);
		}
		m_objects_by_uid.clear();
	}

	const std::string& ItemNode::get_item_name(int keeper_vnum, int pad /*= 0*/) const
	{
		const auto desc_i = m_descs.find(keeper_vnum);
		if (desc_i != m_descs.end())
		{
			return desc_i->second.PNames[pad];
		}
		else
		{
			const auto rnum = obj_proto.rnum(m_vnum);
			const static std::string wrong_vnum = "<unknown VNUM>";
			if (-1 == rnum)
			{
				return wrong_vnum;
			}
			return GET_OBJ_PNAME(obj_proto[rnum], pad);
		}
	}

	void ItemNode::replace_descs(OBJ_DATA *obj, const int vnum) const
	{
		const auto desc_i = m_descs.find(vnum);
		if (!obj
			|| desc_i == m_descs.end())
		{
			return;
		}

		const auto& desc = desc_i->second;

		obj->set_description(desc.description.c_str());
		obj->set_aliases(desc.name.c_str());
		obj->set_short_description(desc.short_description.c_str());
		obj->set_PName(0, desc.PNames[0].c_str());
		obj->set_PName(1, desc.PNames[1].c_str());
		obj->set_PName(2, desc.PNames[2].c_str());
		obj->set_PName(3, desc.PNames[3].c_str());
		obj->set_PName(4, desc.PNames[4].c_str());
		obj->set_PName(5, desc.PNames[5].c_str());
		obj->set_sex(desc.sex);

		if (!desc.trigs.empty())
		{
			obj->attach_triggers(desc.trigs);
		}

		obj->set_ex_description(nullptr); //Пока в конфиге нельзя указать экстраописания - убираем нафиг

		if ((GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON)
			&& (GET_OBJ_VAL(obj, 1) > 0)) //Если работаем с непустой емкостью...
		{
			name_to_drinkcon(obj, GET_OBJ_VAL(obj, 2)); //...Следует указать содержимое емкости
		}
	}

	void ItemsList::add(const obj_vnum vnum, const long price, const ItemNode::uid_t uid)
	{
		const auto item = std::make_shared<ItemNode>(vnum, price, uid);
		m_items_list.push_back(std::move(item));
	}

	void ItemsList::add(const obj_vnum vnum, const long price)
	{
		const auto item = std::make_shared<ItemNode>(vnum, price);
		m_items_list.push_back(std::move(item));
	}

	const ItemsList::items_list_t::value_type& ItemsList::node(const size_t index) const
	{
		if (index < m_items_list.size())
		{
			return m_items_list[index];
		}

		// if we are here then this is an error
		log("LOGIC ERROR: Requested shop item with too high index %zd but shop has only %zd items", index, size());
		static decltype(m_items_list)::value_type null_ptr;
		return null_ptr;
	}

	void shop_node::process_buy(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument)
	{
		std::string buffer2(argument), buffer1;
		GetOneParam(buffer2, buffer1);
		boost::trim(buffer2);

		if (buffer1.empty())
		{
			tell_to_char(keeper, ch, "ЧТО ты хочешь купить?");
			return;
		}

		int item_num = 0, item_count = 1;

		if (buffer2.empty())
		{
			if (is_number(buffer1.c_str()))
			{
				// buy 5
				try
				{
					item_num = boost::lexical_cast<unsigned>(buffer1);
				}
				catch (const boost::bad_lexical_cast&)
				{
					item_num = 0;
				}
			}
			else
			{
				// buy sword
				item_num = get_item_num(buffer1, GET_MOB_VNUM(keeper));
			}
		}
		else if (is_number(buffer1.c_str()))
		{
			if (is_number(buffer2.c_str()))
			{
				// buy 5 10
				try
				{
					item_num = boost::lexical_cast<unsigned>(buffer2);
				}
				catch (const boost::bad_lexical_cast&)
				{
					item_num = 0;
				}
			}
			else
			{
				// buy 5 sword
				item_num = get_item_num(buffer2, GET_MOB_VNUM(keeper));
			}
			try
			{
				item_count = boost::lexical_cast<unsigned>(buffer1);
			}
			catch (const boost::bad_lexical_cast&)
			{
				item_count = 1000;
			}
		}
		else
		{
			tell_to_char(keeper, ch, "ЧТО ты хочешь купить?");
			return;
		}

		if (!item_count
			|| !item_num
			|| static_cast<size_t>(item_num) > m_items_list.size())
		{
			tell_to_char(keeper, ch, "Протри глаза, у меня нет такой вещи.");
			return;
		}

		if (item_count >= 1000)
		{
			tell_to_char(keeper, ch, "А морда не треснет?");
			return;
		}

		const auto item_index = item_num - 1;

		CObjectPrototype* tmp_obj = nullptr;
		bool obj_from_proto = true;
		const auto item = m_items_list.node(item_index);
		if (!item->empty())
		{
			tmp_obj = get_from_shelve(item_index);

			if (!tmp_obj)
			{
				log("SYSERROR : не удалось прочитать предмет (%s:%d)", __FILE__, __LINE__);
				send_to_char("Ошибочка вышла.\r\n", ch);
				return;
			}

			obj_from_proto = false;
		}

		auto proto = (tmp_obj ? tmp_obj : get_object_prototype(item->vnum()).get());
		if (!proto)
		{
			log("SYSERROR : не удалось прочитать прототип (%s:%d)", __FILE__, __LINE__);
			send_to_char("Ошибочка вышла.\r\n", ch);
			return;
		}

		const long price = item->get_price();
		if (!check_money(ch, price, currency))
		{
			snprintf(buf, MAX_STRING_LENGTH,
				"У вас нет столько %s!", ExtMoney::name_currency_plural(currency).c_str());
			tell_to_char(keeper, ch, buf);

			char local_buf[MAX_INPUT_LENGTH];
			switch (number(0, 3))
			{
			case 0:
				snprintf(local_buf, MAX_INPUT_LENGTH, "ругать %s!", GET_NAME(ch));
				do_social(keeper, local_buf);
				break;

			case 1:
				snprintf(local_buf, MAX_INPUT_LENGTH,
					"отхлебнул$g немелкий глоток %s",
					IS_MALE(keeper) ? "водки" : "медовухи");
				do_echo(keeper, local_buf, 0, SCMD_EMOTE);
				break;
			}
			return;
		}

		if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
			|| ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto)) > CAN_CARRY_W(ch)))
		{
			const auto& name = obj_from_proto
				? item->get_item_name(GET_MOB_VNUM(keeper), 3).c_str()
				: tmp_obj->get_short_description().c_str();
			snprintf(buf, MAX_STRING_LENGTH,
				"%s, я понимаю, своя ноша карман не тянет,\r\n"
				"но %s вам явно некуда положить.\r\n",
				GET_NAME(ch), name);
			send_to_char(buf, ch);
			return;
		}

		int bought = 0;
		int total_money = 0;
		int sell_count = can_sell_count(item_index);

		OBJ_DATA *obj = 0;
		while (bought < item_count
			&& check_money(ch, price, currency)
			&& IS_CARRYING_N(ch) < CAN_CARRY_N(ch)
			&& IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto) <= CAN_CARRY_W(ch)
			&& (bought < sell_count || sell_count == -1))
		{

			if (!item->empty())
			{
				obj = get_from_shelve(item_index);
				item->remove_uid(obj->get_uid());
				if (item->empty())
				{
					m_items_list.remove(item_index);
				}
				remove_from_storage(obj);
			}
			else
			{
				obj = world_objects.create_from_prototype_by_vnum(item->vnum()).get();
				item->replace_descs(obj, GET_MOB_VNUM(keeper));
			}

			if (obj)
			{
				if (GET_OBJ_ZONE(obj) == NOWHERE)
				{
					obj->set_zone(world[ch->in_room]->zone);
				}
				if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_UNIQUE_WHEN_PURCHASE))
				{
					obj->set_owner(GET_UNIQUE(ch));
				}
				obj_to_char(obj, ch);
				if (currency == "слава")
				{
					// книги за славу не фейлим
					if (OBJ_DATA::ITEM_BOOK == GET_OBJ_TYPE(obj))
					{
						obj->set_extra_flag(EExtraFlag::ITEM_NO_FAIL);
					}

					// снятие и логирование славы
					obj->set_owner(GET_UNIQUE(ch));
					GloryConst::add_total_spent(price);
					GloryConst::remove_glory(GET_UNIQUE(ch), price);
					GloryConst::transfer_log("%s bought %s for %ld const glory",
							GET_NAME(ch), GET_OBJ_PNAME(proto, 0).c_str(), price);
				}
				else if (currency == "лед")
				{
					// книги за лед, как и за славу, не фейлим
					if (OBJ_DATA::ITEM_BOOK == GET_OBJ_TYPE(obj))
					{
						obj->set_extra_flag(EExtraFlag::ITEM_NO_FAIL);
					}
					ch->sub_ice_currency(price);

				}
				else if (currency == "гривны")
				{
					// книги за гривны, как и за славу, не фейлим
					if (OBJ_DATA::ITEM_BOOK == GET_OBJ_TYPE(obj))
					{
						obj->set_extra_flag(EExtraFlag::ITEM_NO_FAIL);
					}
					ch->sub_hryvn(price);
					ch->spent_hryvn_sub(price);
					if (ch->get_spent_hryvn() > 1000)
					{
						send_to_char("Мессага о том, что гривны были сброшены.\r\n", ch);
						ch->reset_daily_quest();
					}
				}
				else
				{
					ch->remove_gold(price);
					spent_today += price;
				}
				++bought;

				total_money += price;
			}
			else
			{
				log("SYSERROR : не удалось загрузить предмет obj_vnum=%d (%s:%d)",
					GET_OBJ_VNUM(proto), __FILE__, __LINE__);
				send_to_char("Ошибочка вышла.\r\n", ch);
				return;
			}
		}

		if (bought < item_count)
		{
			if (!check_money(ch, price, currency))
			{
				snprintf(buf, MAX_STRING_LENGTH, "Мошенни%s, ты можешь оплатить только %d.",
					IS_MALE(ch) ? "к" : "ца", bought);
			}
			else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
			{
				snprintf(buf, MAX_STRING_LENGTH, "Ты сможешь унести только %d.", bought);
			}
			else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto) > CAN_CARRY_W(ch))
			{
				snprintf(buf, MAX_STRING_LENGTH, "Ты сможешь поднять только %d.", bought);
			}
			else if (bought > 0)
			{
				snprintf(buf, MAX_STRING_LENGTH, "Я продам тебе только %d.", bought);
			}
			else
			{
				snprintf(buf, MAX_STRING_LENGTH, "Я не продам тебе ничего.");
			}

			tell_to_char(keeper, ch, buf);
		}
		auto suffix = desc_count(total_money, WHAT_MONEYu);
		if (currency == "лед")
			suffix = desc_count(total_money, WHAT_ICEu);
		if (currency == "слава")
			suffix = desc_count(total_money, WHAT_GLORYu);
		if (currency == "гривны")
			suffix = desc_count(total_money, WHAT_TORCu);
		snprintf(buf, MAX_STRING_LENGTH, "Это будет стоить %d %s.", total_money, suffix);
		tell_to_char(keeper, ch, buf);

		if (obj)
		{
			if (GET_OBJ_COST(obj) > total_money)			{
				
				snprintf(buf, MAX_STRING_LENGTH, "Персонаж %s купил предмет %d за %d при его стоимости %d и прайсе %ld.", ch->get_name().c_str(), GET_OBJ_VNUM(obj), total_money, GET_OBJ_COST(obj), price);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			}
			send_to_char(ch, "Теперь вы стали %s %s.\r\n",
				IS_MALE(ch) ? "счастливым обладателем" : "счастливой обладательницей",
				obj->item_count_message(bought, 1).c_str());
		}
	}

	void shop_node::print_shop_list(CHAR_DATA *ch, const std::string& arg, int keeper_vnum) const
	{
		send_to_char(ch,
			" ##    Доступно   Предмет                                      Цена (%s)\r\n"
			"---------------------------------------------------------------------------\r\n",
			currency.c_str());
		int num = 1;
		std::stringstream out;
		std::string print_value;
		std::string name_value;

		for (size_t k = 0; k < m_items_list.size();)
		{
			int count = can_sell_count(num - 1);

			print_value.clear();
			name_value.clear();

			const auto& item = m_items_list.node(k);

			//Polud у проданных в магаз объектов отображаем в списке не значение из прототипа, а уже, возможно, измененное значение
			// чтобы не было в списках всяких "гриб @n1"
			if (item->empty())
			{
				print_value = item->get_item_name(keeper_vnum);
				const auto rnum = obj_proto.rnum(item->vnum());
				if (GET_OBJ_TYPE(obj_proto[rnum]) == OBJ_DATA::ITEM_DRINKCON)
				{
					print_value += " с " + std::string(drinknames[GET_OBJ_VAL(obj_proto[rnum], 2)]);
				}
			}
			else
			{
				const OBJ_DATA* tmp_obj = get_from_shelve(k);
				if (tmp_obj)
				{
					print_value = tmp_obj->get_short_description();
					name_value = tmp_obj->get_aliases();
					item->set_price(GET_OBJ_COST(tmp_obj));
				}
			}

			std::string numToShow = (count == -1 || count > 100 ? "Навалом" : boost::lexical_cast<std::string>(count));

			// имхо вполне логично раз уж мы получаем эту надпись в ней и искать
			if (arg.empty()
				|| isname(arg, print_value)
				|| (!name_value.empty()
					&& isname(arg, name_value)))
			{
				std::string format_str = "%4d)  %10s  %-"
					+ std::to_string(std::count(print_value.begin(), print_value.end(), '&') * 2 + 45) + "s %8d\r\n";
				out << boost::str(boost::format(format_str) % num++ % numToShow % print_value % item->get_price());
			}
			else
			{
				num++;
			}

			++k;
		}

		page_string(ch->desc, out.str());
	}

	bool init_type(const std::string& str, int& type)
	{
		if (is_abbrev(str, "свет")
			|| is_abbrev(str, "light"))
		{
			type = OBJ_DATA::ITEM_LIGHT;
		}
		else if (is_abbrev(str, "свиток")
			|| is_abbrev(str, "scroll"))
		{
			type = OBJ_DATA::ITEM_SCROLL;
		}
		else if (is_abbrev(str, "палочка")
			|| is_abbrev(str, "wand"))
		{
			type = OBJ_DATA::ITEM_WAND;
		}
		else if (is_abbrev(str, "посох")
			|| is_abbrev(str, "staff"))
		{
			type = OBJ_DATA::ITEM_STAFF;
		}
		else if (is_abbrev(str, "оружие")
			|| is_abbrev(str, "weapon"))
		{
			type = OBJ_DATA::ITEM_WEAPON;
		}
		else if (is_abbrev(str, "броня")
			|| is_abbrev(str, "armor"))
		{
			type = OBJ_DATA::ITEM_ARMOR;
		}
		else if (is_abbrev(str, "материал")
			|| is_abbrev(str, "material"))
		{
			type = OBJ_DATA::ITEM_MATERIAL;
		}
		else if (is_abbrev(str, "напиток")
			|| is_abbrev(str, "potion"))
		{
			type = OBJ_DATA::ITEM_POTION;
		}
		else if (is_abbrev(str, "прочее")
			|| is_abbrev(str, "другое")
			|| is_abbrev(str, "other"))
		{
			type = OBJ_DATA::ITEM_OTHER;
		}
		else if (is_abbrev(str, "контейнер")
			|| is_abbrev(str, "container"))
		{
			type = OBJ_DATA::ITEM_CONTAINER;
		}
		else if (is_abbrev(str, "емкость")
			|| is_abbrev(str, "tank"))
		{
			type = OBJ_DATA::ITEM_DRINKCON;
		}
		else if (is_abbrev(str, "книга")
			|| is_abbrev(str, "book"))
		{
			type = OBJ_DATA::ITEM_BOOK;
		}
		else if (is_abbrev(str, "руна")
			|| is_abbrev(str, "rune"))
		{
			type = OBJ_DATA::ITEM_INGREDIENT;
		}
		else if (is_abbrev(str, "ингредиент")
			|| is_abbrev(str, "ingradient"))
		{
			type = OBJ_DATA::ITEM_MING;
		}
		else if (is_abbrev(str, "легкие")
			|| is_abbrev(str, "легкая"))
		{
			type = OBJ_DATA::ITEM_ARMOR_LIGHT;
		}
		else if (is_abbrev(str, "средние")
			|| is_abbrev(str, "средняя"))
		{
			type = OBJ_DATA::ITEM_ARMOR_MEDIAN;
		}
		else if (is_abbrev(str, "тяжелые")
			|| is_abbrev(str, "тяжелая"))
		{
			type = OBJ_DATA::ITEM_ARMOR_HEAVY;
		}
		else if (is_abbrev(str, "колчан"))
		{
			type = OBJ_DATA::ITEM_MAGIC_CONTAINER;
		}
		else if (is_abbrev(str, "стрела"))
		{
			type = OBJ_DATA::ITEM_MAGIC_ARROW;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool init_wear(const std::string& str, EWearFlag& wear)
	{
		if (is_abbrev(str, "палец"))
		{
			wear = EWearFlag::ITEM_WEAR_FINGER;
		}
		else if (is_abbrev(str, "шея") || is_abbrev(str, "грудь"))
		{
			wear = EWearFlag::ITEM_WEAR_NECK;
		}
		else if (is_abbrev(str, "тело"))
		{
			wear = EWearFlag::ITEM_WEAR_BODY;
		}
		else if (is_abbrev(str, "голова"))
		{
			wear = EWearFlag::ITEM_WEAR_HEAD;
		}
		else if (is_abbrev(str, "ноги"))
		{
			wear = EWearFlag::ITEM_WEAR_LEGS;
		}
		else if (is_abbrev(str, "ступни"))
		{
			wear = EWearFlag::ITEM_WEAR_FEET;
		}
		else if (is_abbrev(str, "кисти"))
		{
			wear = EWearFlag::ITEM_WEAR_HANDS;
		}
		else if (is_abbrev(str, "руки"))
		{
			wear = EWearFlag::ITEM_WEAR_ARMS;
		}
		else if (is_abbrev(str, "щит"))
		{
			wear = EWearFlag::ITEM_WEAR_SHIELD;
		}
		else if (is_abbrev(str, "плечи"))
		{
			wear = EWearFlag::ITEM_WEAR_ABOUT;
		}
		else if (is_abbrev(str, "пояс"))
		{
			wear = EWearFlag::ITEM_WEAR_WAIST;
		}
		else if (is_abbrev(str, "колчан"))
		{
			wear = EWearFlag::ITEM_WEAR_QUIVER;
		}
		else if (is_abbrev(str, "запястья"))
		{
			wear = EWearFlag::ITEM_WEAR_WRIST;
		}
		else if (is_abbrev(str, "правая"))
		{
			wear = EWearFlag::ITEM_WEAR_WIELD;
		}
		else if (is_abbrev(str, "левая"))
		{
			wear = EWearFlag::ITEM_WEAR_HOLD;
		}
		else if (is_abbrev(str, "обе"))
		{
			wear = EWearFlag::ITEM_WEAR_BOTHS;
		}
		else
		{
			return false;
		}

		return true;
	}

	void shop_node::filter_shop_list(CHAR_DATA *ch, const std::string& arg, int keeper_vnum)
	{
		int num = 1;
		EWearFlag wear = EWearFlag::ITEM_WEAR_UNDEFINED;
		int type = -10;

		std::string print_value = "";
		std::string name_value = "";

		std::string filtr_value;
		const char *first_simvol = "";

		if (!arg.empty())
		{
			first_simvol = arg.c_str();
			filtr_value = arg.substr(1, arg.size() - 1);
		}

		switch (first_simvol[0])
		{
		case 'Т':
			if (!init_type(filtr_value, type))
			{
				send_to_char("Неверный тип предмета.\r\n", ch);
				return;
			}
			break;

		case 'О':
			if (!init_wear(filtr_value, wear))
			{
				send_to_char("Неверное место одевания предмета.\r\n", ch);
				return;
			}
			break;

		default:
			send_to_char("Неверный фильтр. \r\n", ch);
			return;;
			break;
		};

		send_to_char(ch,
			" ##    Доступно   Предмет(фильтр)                              Цена (%s)\r\n"
			"---------------------------------------------------------------------------\r\n",
			currency.c_str());

		std::stringstream out;
		for (auto k = 0u; k < m_items_list.size();)
		{
			int count = can_sell_count(num - 1);
			bool show_name = true;

			print_value = "";
			name_value = "";

			const auto& item = m_items_list.node(k);

			//Polud у проданных в магаз объектов отображаем в списке не значение из прототипа, а уже, возможно, измененное значение
			// чтобы не было в списках всяких "гриб @n1"
			if (item->empty())
			{
				print_value = item->get_item_name(keeper_vnum);
				const auto rnum = obj_proto.rnum(item->vnum());
				if (GET_OBJ_TYPE(obj_proto[rnum]) == OBJ_DATA::ITEM_DRINKCON)
				{
					print_value += " с " + std::string(drinknames[GET_OBJ_VAL(obj_proto[rnum], 2)]);
				}

				if (!((wear != EWearFlag::ITEM_WEAR_UNDEFINED && obj_proto[rnum]->has_wear_flag(wear))
					|| (type > 0 && type == GET_OBJ_TYPE(obj_proto[rnum]))))
				{
					show_name = false;
				}
			}
			else
			{
				OBJ_DATA* tmp_obj = get_from_shelve(k);
				if (tmp_obj)
				{
					if (!((wear != EWearFlag::ITEM_WEAR_UNDEFINED && CAN_WEAR(tmp_obj, wear))
						|| (type > 0 && type == GET_OBJ_TYPE(tmp_obj))))
					{
						show_name = false;
					}

					print_value = tmp_obj->get_short_description();
					name_value = tmp_obj->get_aliases();
					item->set_price(GET_OBJ_COST(tmp_obj));
				}
				else
				{
					m_items_list.remove(k);	// remove from shop object that we cannot instantiate

					continue;
				}
			}

			std::string numToShow = count == -1
				? "Навалом"
				: boost::lexical_cast<std::string>(count);

			if (show_name)
			{
				out << (boost::str(boost::format("%4d)  %10s  %-47s %8d\r\n") % num++ % numToShow % print_value % item->get_price()));
			}
			else
			{
				num++;
			}

			++k;
		}

		page_string(ch->desc, out.str());
	}

	void shop_node::process_cmd(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, const std::string& cmd)
	{
		std::string buffer(argument), buffer1;
		GetOneParam(buffer, buffer1);
		boost::trim(buffer);

		if (!can_buy
			&& (cmd == "Продать"
				|| cmd == "Оценить"))
		{
			tell_to_char(keeper, ch, "Извини, у меня свои поставщики...");

			return;
		}

		if (!*argument)
		{
			tell_to_char(keeper, ch, (cmd + " ЧТО?").c_str());
			return;
		}

		if (buffer1.empty())
		{
			return;
		}

		if (is_number(buffer1.c_str()))
		{
			int n = 0;
			try
			{
				n = std::stoi(buffer1, nullptr, 10);
			}
			catch (const std::invalid_argument &)
			{
			}

			OBJ_DATA* obj = get_obj_in_list_vis(ch, buffer, ch->carrying);

			if (!obj)
			{
				send_to_char("У вас нет " + buffer + "!\r\n", ch);
				return;
			}

			while (obj && n > 0)
			{
				const auto obj_next = get_obj_in_list_vis(ch, buffer, obj->get_next_content());
				do_shop_cmd(ch, keeper, obj, cmd);
				obj = obj_next;
				n--;
			}
		}
		else
		{
			skip_spaces(&argument);
			int i, dotmode = find_all_dots(argument);
			std::string buffer2(argument);
			switch (dotmode)
			{
			case FIND_INDIV:
				{
					const auto obj = get_obj_in_list_vis(ch, buffer2, ch->carrying);

					if (!obj)
					{
						if (cmd == "Чинить" && is_abbrev(argument, "экипировка"))
						{
							for (i = 0; i < NUM_WEARS; i++)
							{
								if (ch->equipment[i])
								{
									do_shop_cmd(ch, keeper, ch->equipment[i], cmd);
								}
							}
							return;
						}
						send_to_char("У вас нет " + buffer2 + "!\r\n", ch);
						return;
					}

					do_shop_cmd(ch, keeper, obj, cmd);
				}

				break;

			case FIND_ALL:
				{
					OBJ_DATA* obj_next = nullptr;
					for (auto obj = ch->carrying; obj; obj = obj_next)
					{
						obj_next = obj->get_next_content();
						do_shop_cmd(ch, keeper, obj, cmd);
					}
				}

				break;

			case FIND_ALLDOT:
				{
					auto obj = get_obj_in_list_vis(ch, buffer2, ch->carrying);
					if (!obj)
					{
						send_to_char("У вас нет " + buffer2 + "!\r\n", ch);
						return;
					}

					while (obj)
					{
						const auto obj_next = get_obj_in_list_vis(ch, buffer2, obj->get_next_content());
						do_shop_cmd(ch, keeper, obj, cmd);
						obj = obj_next;
					}
				}
				break;

			default:
				break;
			};
		}
	}

	void shop_node::process_ident(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, const std::string& cmd)
	{
		std::string buffer(argument);
		boost::trim(buffer);

		if (buffer.empty())
		{
			tell_to_char(keeper, ch, "Характеристики ЧЕГО ты хочешь узнать?");
			return;
		}

		unsigned item_num = 0;
		if (is_number(buffer.c_str()))
		{
			// характеристики 5
			try
			{
				item_num = boost::lexical_cast<unsigned>(buffer);
			}
			catch (const boost::bad_lexical_cast&)
			{
			}
		}
		else
		{
			// характеристики меч
			item_num = get_item_num(buffer, GET_MOB_VNUM(keeper));
		}

		if (!item_num
			|| item_num > items_list().size())
		{
			tell_to_char(keeper, ch, "Протри глаза, у меня нет такой вещи.");
			return;
		}

		const auto item_index = item_num - 1;

		const OBJ_DATA *ident_obj = nullptr;
		OBJ_DATA* tmp_obj = nullptr;
		const auto& item = m_items_list.node(item_index);
		if (item->empty())
		{
			const auto vnum = GET_MOB_VNUM(keeper);
			if (item->has_desc(vnum))
			{
				tmp_obj = world_objects.create_from_prototype_by_vnum(item->vnum()).get();
				item->replace_descs(tmp_obj, vnum);
				ident_obj = tmp_obj;
			}
			else
			{
				const auto rnum = obj_proto.rnum(item->vnum());
				const auto object = world_objects.create_raw_from_prototype_by_rnum(rnum);
				ident_obj = tmp_obj = object.get();
			}
		}
		else
		{
			ident_obj = get_from_shelve(item_index);
		}

		if (!ident_obj)
		{
			log("SYSERROR : не удалось получить объект (%s:%d)", __FILE__, __LINE__);
			send_to_char("Ошибочка вышла.\r\n", ch);
			return;
		}

		if (cmd == "Рассмотреть")
		{
			std::stringstream tell;
			tell << "Предмет " << ident_obj->get_short_description() << ": ";
			tell << item_types[GET_OBJ_TYPE(ident_obj)] << "\r\n";
			tell << diag_weapon_to_char(ident_obj, TRUE);
			tell << diag_timer_to_char(ident_obj);

			if (can_use_feat(ch, SKILLED_TRADER_FEAT)
				|| PRF_FLAGGED(ch, PRF_HOLYLIGHT))
			{
				sprintf(buf, "Материал : ");
				sprinttype(ident_obj->get_material(), material_name, buf + strlen(buf));
				sprintf(buf + strlen(buf), ".\r\n");
				tell << buf;
			}

			tell_to_char(keeper, ch, tell.str().c_str());
			if (invalid_anti_class(ch, ident_obj)
				|| invalid_unique(ch, ident_obj)
				|| NamedStuff::check_named(ch, ident_obj, 0))
			{
				tell.str("Но лучше бы тебе не заглядываться на эту вещь, не унесешь все равно.");
				tell_to_char(keeper, ch, tell.str().c_str());
			}
		}

		if (cmd == "Характеристики")
		{
			if (ch->get_gold() < IDENTIFY_COST)
			{
				tell_to_char(keeper, ch, "У вас нет столько денег!");
				char local_buf[MAX_INPUT_LENGTH];
				switch (number(0, 3))
				{
				case 0:
					snprintf(local_buf, MAX_INPUT_LENGTH, "ругать %s!", GET_NAME(ch));
					do_social(keeper, local_buf);
					break;

				case 1:
					snprintf(local_buf, MAX_INPUT_LENGTH,
						"отхлебнул$g немелкий глоток %s",
						IS_MALE(keeper) ? "водки" : "медовухи");
					do_echo(keeper, local_buf, 0, SCMD_EMOTE);
					break;
				}
			}
			else
			{
				snprintf(buf, MAX_STRING_LENGTH,
					"Эта услуга будет стоить %d %s.", IDENTIFY_COST,
					desc_count(IDENTIFY_COST, WHAT_MONEYu));
				tell_to_char(keeper, ch, buf);

				send_to_char(ch, "Характеристики предмета: %s\r\n", GET_OBJ_PNAME(ident_obj, 0).c_str());
				mort_show_obj_values(ident_obj, ch, 200);
				ch->remove_gold(IDENTIFY_COST);
			}
		}

		if (tmp_obj)
		{
			extract_obj(tmp_obj);
		}
	}

	void shop_node::clear_store()
	{
		using to_remove_list_t = std::list<OBJ_DATA*>;
		to_remove_list_t to_remove;
		for (const auto& stored_item : m_storage)
		{
			to_remove.push_back(stored_item.first);
		}

		m_storage.clear();

		for (const auto& item : to_remove)
		{
			extract_obj(item);
		}
	}

	void shop_node::remove_from_storage(OBJ_DATA *object)
	{
		m_storage.remove(object);
	}

	OBJ_DATA* shop_node::get_from_shelve(const size_t index) const
	{
		const auto node = m_items_list.node(index);
		const auto uid = node->uid();
		if (ItemNode::NO_UID == uid)
		{
			return nullptr;
		}

		return m_storage.get_by_uid(uid);
	}

	unsigned shop_node::get_item_num(std::string &item_name, int keeper_vnum) const
	{
		int num = 1;
		if (!item_name.empty() && a_isdigit(item_name[0]))
		{
			std::string::size_type dot_idx = item_name.find_first_of('.');
			if (dot_idx != std::string::npos)
			{
				std::string first_param = item_name.substr(0, dot_idx);
				item_name.erase(0, ++dot_idx);
				if (is_number(first_param.c_str()))
				{
					try
					{
						num = std::stol(first_param, nullptr, 10);
					}
					catch (const std::invalid_argument &)
					{
						return 0;
					}
				}
			}
		}

		int count = 0;
		std::string name_value = "";
		std::string print_value;
		for (unsigned i = 0; i < items_list().size(); ++i)
		{
			print_value = "";
			const auto& item = m_items_list.node(i);
			if (item->empty())
			{
				name_value = item->get_item_name(keeper_vnum);
				const auto rnum = obj_proto.rnum(item->vnum());
				if (GET_OBJ_TYPE(obj_proto[rnum]) == OBJ_DATA::ITEM_DRINKCON)
				{
					name_value += " " + std::string(drinknames[GET_OBJ_VAL(obj_proto[rnum], 2)]);
				}
			}
			else
			{
				OBJ_DATA * tmp_obj = get_from_shelve(i);
				if (!tmp_obj)
				{
					continue;
				}
				name_value = tmp_obj->get_aliases();
				print_value = tmp_obj->get_short_description();
			}

			if (isname(item_name, name_value) || isname(item_name, print_value))
			{
				++count;
				if (count == num)
				{
					return ++i;
				}
			}
		}

		return 0;
	}

	int shop_node::can_sell_count(const int item_index) const
	{
		const auto& item = m_items_list.node(item_index);
		if (!item->empty())
		{
			return static_cast<int>(item->size());
		}
		else
		{
			const auto rnum = obj_proto.rnum(item->vnum());
			int numToSell = obj_proto[rnum]->get_max_in_world();
			if (numToSell == 0)
			{
				return numToSell;
			}

			if (numToSell != -1)
			{
				numToSell -= MIN(numToSell, obj_proto.actual_count(rnum));	//считаем не только онлайн, но и то что в ренте
			}

			return numToSell;
		}
	}

	void shop_node::put_item_to_shop(OBJ_DATA* obj)
	{
		for (auto index = 0u; index < m_items_list.size(); ++index)
		{
			const auto& item = m_items_list.node(index);
			if (item->vnum() == obj->get_vnum())
			{
				if (item->empty())
				{
					extract_obj(obj);
					return;
				}
				else
				{
					OBJ_DATA* tmp_obj = get_from_shelve(index);
					if (!tmp_obj)
					{
						continue;
					}

					if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_MING //а у них всех один рнум
						|| obj->get_short_description() == tmp_obj->get_short_description())
					{
						item->add_uid(obj->get_uid());
						put_to_storage(obj);

						return;
					}
				}
			}
		}

		add_item(obj->get_vnum(), obj->get_cost(), obj->get_uid());

		put_to_storage(obj);
	}

	long get_sell_price(OBJ_DATA * obj)
	{
		long cost = GET_OBJ_COST(obj);
		long cost_obj = GET_OBJ_COST(obj);
		int timer = obj_proto[GET_OBJ_RNUM(obj)]->get_timer();
		if (timer < obj->get_timer())
		{
			obj->set_timer(timer);
		}
		cost = timer <= 0 ? 1 : (long)cost * ((float)obj->get_timer() / (float)timer); //учтем таймер

		// если цена продажи, выше, чем стоймость предмета
		if (cost > cost_obj)
		{
			cost = cost_obj;
		}

		return MMAX(1, cost);
	}

	void shop_node::do_shop_cmd(CHAR_DATA* ch, CHAR_DATA *keeper, OBJ_DATA* obj, std::string cmd)
	{
		if (!obj)
		{
			return;
		}

		int rnum = GET_OBJ_RNUM(obj);
		if (rnum < 0
			|| obj->get_extra_flag(EExtraFlag::ITEM_ARMORED)
			|| obj->get_extra_flag(EExtraFlag::ITEM_SHARPEN)
			|| obj->get_extra_flag(EExtraFlag::ITEM_NODROP))
		{
			tell_to_char(keeper, ch, std::string("Я не собираюсь иметь дела с этой вещью.").c_str());
			return;
		}

		if (GET_OBJ_VAL(obj, 2) == 0
			&& (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WAND
				|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_STAFF))
		{
			tell_to_char(keeper, ch, "Я не покупаю использованные вещи!");
			return;
		}

		if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_CONTAINER
			&& cmd != "Чинить")
		{
			if (obj->get_contains())
			{
				tell_to_char(keeper, ch, "Не надо предлагать мне кота в мешке.");
				return;
			}
		}

		long buy_price = GET_OBJ_COST(obj);
		long buy_price_old = get_sell_price(obj);

		int repair = GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj);
		int repair_price = MAX(1, GET_OBJ_COST(obj) * MAX(0, repair) / MAX(1, GET_OBJ_MAX(obj)));

		// если не купцы, то учитываем прибыль магазина, если купцы, то назначаем цену, при которой объект был куплен
		if (!can_use_feat(ch, SKILLED_TRADER_FEAT))
		{
			buy_price = MMAX(1, (buy_price * profit) / 100); //учтем прибыль магазина
		}
		else
		{
			buy_price = get_sell_price(obj);
		}

		// если цена покупки, выше, чем стоймость предмета
		if (buy_price > buy_price_old)
		{
			buy_price = buy_price_old;
		}

		std::string price_to_show = boost::lexical_cast<std::string>(buy_price) + " " + std::string(desc_count(buy_price, WHAT_MONEYu));

		if (cmd == "Оценить")
		{
			if (bloody::is_bloody(obj))
			{
				tell_to_char(keeper, ch, "Иди от крови отмой сначала!");
				return;
			}

			if (obj->get_extra_flag(EExtraFlag::ITEM_NOSELL)
				|| obj->get_extra_flag(EExtraFlag::ITEM_NAMED)
				|| obj->get_extra_flag(EExtraFlag::ITEM_REPOP_DECAY)
				|| obj->get_extra_flag(EExtraFlag::ITEM_ZONEDECAY))
			{
				tell_to_char(keeper, ch, "Такое я не покупаю.");
				return;
			}
			else
			{
				tell_to_char(keeper, ch, ("Я, пожалуй, куплю " + std::string(GET_OBJ_PNAME(obj, 3)) + " за " + price_to_show + ".").c_str());
			}
		}

		if (cmd == "Продать")
		{
			if (obj->get_extra_flag(EExtraFlag::ITEM_NOSELL)
				|| obj->get_extra_flag(EExtraFlag::ITEM_NAMED)
				|| obj->get_extra_flag(EExtraFlag::ITEM_REPOP_DECAY)
				|| (buy_price <= 1)
				|| obj->get_extra_flag(EExtraFlag::ITEM_ZONEDECAY)
				|| bloody::is_bloody(obj))
			{
				if (bloody::is_bloody(obj))
				{
					tell_to_char(keeper, ch, "Пшел вон убивец, и руки от крови отмой!");
				}
				else
				{
					tell_to_char(keeper, ch, "Такое я не покупаю.");
				}

				return;
			}
			else
			{
				obj_from_char(obj);
				tell_to_char(keeper, ch, ("Получи за " + std::string(GET_OBJ_PNAME(obj, 3)) + " " + price_to_show + ".").c_str());
				ch->add_gold(buy_price);
				put_item_to_shop(obj);				
			}
		}
		if (cmd == "Чинить")
		{
			if (bloody::is_bloody(obj))
			{
				tell_to_char(keeper, ch, "Я не буду чинить окровавленные вещи!");
				return;
			}

			if (repair <= 0)
			{
				tell_to_char(keeper, ch, (std::string(GET_OBJ_PNAME(obj, 3)) + " не нужно чинить.").c_str());
				return;
			}

			switch (obj->get_material())
			{
			case OBJ_DATA::MAT_BULAT:
			case OBJ_DATA::MAT_CRYSTALL:
			case OBJ_DATA::MAT_DIAMOND:
			case OBJ_DATA::MAT_SWORDSSTEEL:
				repair_price *= 2;
				break;

			case OBJ_DATA::MAT_SUPERWOOD:
			case OBJ_DATA::MAT_COLOR:
			case OBJ_DATA::MAT_GLASS:
			case OBJ_DATA::MAT_BRONZE:
			case OBJ_DATA::MAT_FARFOR:
			case OBJ_DATA::MAT_BONE:
			case OBJ_DATA::MAT_ORGANIC:
				repair_price += MAX(1, repair_price / 2);
				break;

			case OBJ_DATA::MAT_IRON:
			case OBJ_DATA::MAT_STEEL:
			case OBJ_DATA::MAT_SKIN:
			case OBJ_DATA::MAT_MATERIA:
				//repair_price = repair_price;
				break;

			default:
				//repair_price = repair_price;
				break;
			}

			if (repair_price <= 0
				|| obj->get_extra_flag(EExtraFlag::ITEM_DECAY)
				|| obj->get_extra_flag(EExtraFlag::ITEM_NOSELL)
				|| obj->get_extra_flag(EExtraFlag::ITEM_NODROP))
			{
				tell_to_char(keeper, ch, ("Я не буду тратить свое драгоценное время на " + GET_OBJ_PNAME(obj, 3) + ".").c_str());
				return;
			}

			tell_to_char(keeper, ch, ("Починка " + std::string(GET_OBJ_PNAME(obj, 1)) + " обойдется в "
				+ boost::lexical_cast<std::string>(repair_price) + " " + desc_count(repair_price, WHAT_MONEYu)).c_str());

			if (!IS_GOD(ch) && repair_price > ch->get_gold())
			{
				act("А вот их у тебя как-раз то и нет.", FALSE, ch, 0, 0, TO_CHAR);
				return;
			}

			if (!IS_GOD(ch))
			{
				ch->remove_gold(repair_price);
			}

			act("$n сноровисто починил$g $o3.", FALSE, keeper, obj, 0, TO_ROOM);

			obj->set_current_durability(GET_OBJ_MAX(obj));
		}
	}
}
