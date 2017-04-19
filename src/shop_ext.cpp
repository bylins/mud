// $RCSfile: shop_ext.cpp,v $     $Date: 2012/01/27 05:36:40 $     $Revision: 1.16 $
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#include "shop_ext.hpp"

#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "obj.hpp"
#include "char.hpp"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "constants.h"
#include "dg_scripts.h"
#include "room.hpp"
#include "glory.hpp"
#include "glory_const.hpp"
#include "named_stuff.hpp"
#include "screen.h"
#include "house.h"
#include "modify.h"
#include "liquid.hpp"
#include "logger.hpp"
#include "utils.h"
#include "parse.hpp"
#include "pugixml.hpp"
#include "pk.h"
#include "ext_money.hpp"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/array.hpp>

#include <vector>
#include <string>

/*
Пример конфига (sample configuration) (plrstuff/shop/test.xml):
<?xml version="1.0"?>
<shop_item_sets>
	<shop_item_set id="GEMS">
	        <item vnum="909" price="100" />
        	<item vnum="910" price="100" />
	        <item vnum="911" price="100" />
        	<item vnum="912" price="100" />
	        <item vnum="913" price="100" />
        	<item vnum="914" price="100" />
	        <item vnum="915" price="100" />
	        <item vnum="916" price="100" />
        	<item vnum="917" price="100" />
	</shop_item_set>
</shop_item_sets>

<shop_list>
    <shop currency="куны" id="GEMS_SHOP" profit="40" waste_time_min="0">
	<mob mob_vnum="4010" />
	<mob mob_vnum="4015" />
	<item_set>GEMS</item_set>
        <item vnum="4001" price="500" />
    </shop>
    <shop currency="куны" id="BANDAGE_SHOP" profit="60" waste_time_min="15">
	<mob mob_vnum="4018"/>
	<mob mob_vnum="4019"/>
        <item vnum="1913" price="500" />
       	<item vnum="1914" price="1000" />
        <item vnum="1915" price="2000" />
       	<item vnum="1916" price="4000" />
        <item vnum="1917" price="8000" />
       	<item vnum="1918" price="16000" />
        <item vnum="1919" price="25" />
    </shop>
</shop_list>
*/

extern void do_echo(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
extern int do_social(CHAR_DATA * ch, char *argument);
extern void mort_show_obj_values(const OBJ_DATA * obj, CHAR_DATA * ch, int fullness);
extern int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_align(CHAR_DATA * ch, const OBJ_DATA * obj);
extern char *diag_weapon_to_char(const CObjectPrototype* obj, int show_wear);
extern char *diag_timer_to_char(const OBJ_DATA * obj);

namespace ShopExt
{
long get_sell_price(OBJ_DATA * obj);

const int IDENTIFY_COST = 110;
int spent_today = 0;
const char *MSG_NO_STEAL_HERE = "$n, грязн$w воришка, чеши отседова!";
EWearFlag wear = EWearFlag::ITEM_WEAR_UNDEFINED;
int type = -10;

struct item_desc_node
{
	std::string name;
	std::string description;
	std::string short_description;
	boost::array<std::string, 6> PNames;
	ESex sex;
	CObjectPrototype::triggers_list_t trigs;
};

typedef std::map<int/*vnum предмета*/, item_desc_node> ObjDescType;
std::map<std::string/*id шаблона*/, ObjDescType> item_descriptions;

struct waste_node
{
	waste_node() : rnum(0), obj(NULL), last_activity(time(NULL)) {}

	int rnum;
	OBJ_DATA* obj;
	int last_activity;
};

struct item_node
{
	using shared_ptr = std::shared_ptr<item_node>;
	using temporary_ids_t = std::vector<unsigned>;
	using item_descriptions_t = std::map<int/*vnum продавца*/, item_desc_node>;

	item_node();

	const std::string& get_item_name(int keeper_vnum, int pad = 0) const;
	void replace_descs(OBJ_DATA *obj, int vnum);

	int rnum;
	int vnum;
	long price;
	temporary_ids_t temporary_ids;
	item_descriptions_t descs;
};

item_node::item_node() : rnum(0), vnum(0), price(0)
{
	descs.clear();
	temporary_ids.clear();
}

const std::string& item_node::get_item_name(int keeper_vnum, int pad /*= 0*/) const
{
	const auto desc_i = descs.find(keeper_vnum);
	if (desc_i != descs.end())
	{
		return desc_i->second.PNames[pad];
	}
	else
	{
		return GET_OBJ_PNAME(obj_proto[rnum], pad);
	}
}

void item_node::replace_descs(OBJ_DATA *obj, int vnum)
{
	obj->set_description(descs[vnum].description.c_str());
	obj->set_aliases(descs[vnum].name.c_str());
	obj->set_short_description(descs[vnum].short_description.c_str());
	obj->set_PName(0, descs[vnum].PNames[0].c_str());
	obj->set_PName(1, descs[vnum].PNames[1].c_str());
	obj->set_PName(2, descs[vnum].PNames[2].c_str());
	obj->set_PName(3, descs[vnum].PNames[3].c_str());
	obj->set_PName(4, descs[vnum].PNames[4].c_str());
	obj->set_PName(5, descs[vnum].PNames[5].c_str());
	obj->set_sex(descs[vnum].sex);

	if (!descs[vnum].trigs.empty())
	{
		obj->attach_triggers(descs[vnum].trigs);
	}

	obj->set_ex_description(nullptr); //Пока в конфиге нельзя указать экстраописания - убираем нафиг

	if ((GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON)
		&& (GET_OBJ_VAL(obj, 1) > 0)) //Если работаем с непустой емкостью...
	{
		name_to_drinkcon(obj, GET_OBJ_VAL(obj, 2)); //...Следует указать содержимое емкости
	}
}

bool check_money(CHAR_DATA *ch, long price, const std::string& currency)
{
	if (currency == "слава")
	{
		const auto total_glory = Glory::get_glory(GET_UNIQUE(ch)) + GloryConst::get_glory(GET_UNIQUE(ch));
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

	return false;
}

bool init_type(const char *str)
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
	else
	{
		return false;
	}

	return true;
}

bool init_wear(const char *str)
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

class shop_node : public DictionaryItem
{
public:
	using shared_ptr = std::shared_ptr<shop_node>;

	using items_list_t = std::vector<item_node::shared_ptr>;
	using mob_vnums_t = std::list<mob_vnum>;
	using waste_list_t = std::list<waste_node>;

	shop_node(): waste_time_min(0), can_buy(true) {};

	std::string currency;
	std::string id;
	int profit;
	int waste_time_min;
	bool can_buy;

	void add_item(const item_node::shared_ptr& node) { m_items_list.push_back(node); }

	const auto& items_list() const { return m_items_list; }
	auto& items_list() { return m_items_list; }	// get rid of me

	void add_mob_vnum(const mob_vnum vnum) { m_mob_vnums.push_back(vnum); }

	const auto& mob_vnums() const { return m_mob_vnums; }

	void add_waste(const waste_node& node) { m_waste.push_back(node); }

	const auto& waste() const { return m_waste; }
	auto& waste() { return m_waste; }
	void clear_waste() { m_waste.clear(); }

	void update_shop_timers();
	void process_buy(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument);
	void print_shop_list(CHAR_DATA *ch, std::string arg, int keeper_vnum);	// it should be const
	void filter_shop_list(CHAR_DATA *ch, std::string arg, int keeper_vnum);
	void process_cmd(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, const std::string& cmd);
	void process_ident(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, const std::string& cmd);	// it should be const

private:
	void remove_item_id(unsigned uid);
	OBJ_DATA* get_obj_from_waste(const item_node::temporary_ids_t& uids);
	void remove_from_waste(OBJ_DATA *obj);
	unsigned get_item_num(std::string &item_name, int keeper_vnum);	// it should be const
	int can_sell_count(int item_num) const;
	void put_item_in_shop(OBJ_DATA* obj);
	void do_shop_cmd(CHAR_DATA* ch, CHAR_DATA *keeper, OBJ_DATA* obj, std::string cmd);

	items_list_t m_items_list;
	mob_vnums_t m_mob_vnums;
	waste_list_t m_waste;
};

void shop_node::update_shop_timers()
{
	int cur_time = time(NULL);
	int waste_time = waste_time_min * 60;
	for (auto it = waste().begin(); it != waste().end();)
	{
		if (GET_OBJ_TYPE(it->obj) == CObjectPrototype::ITEM_BOOK)
		{
			it->obj->dec_timer();
		}

		if (it->obj->get_timer() <= 0
			|| ((waste_time > 0)
				&& (cur_time - it->last_activity > waste_time)))
		{
			remove_item_id(it->obj->get_uid());

			if (it->obj->get_rnum() == it->rnum)
			{
				extract_obj(it->obj);
			}

			it = waste().erase(it);
		}
		else
		{
			++it;
		}
	}
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

	if (!item_count || !item_num || (unsigned)item_num > items_list().size())
	{
		tell_to_char(keeper, ch, "Протри глаза, у меня нет такой вещи.");
		return;
	}

	if (item_count >= 1000)
	{
		tell_to_char(keeper, ch, "А морда не треснет?");
		return;
	}

	--item_num;
	CObjectPrototype* tmp_obj = nullptr;
	bool obj_from_proto = true;
	if (!items_list()[item_num]->temporary_ids.empty())
	{
		tmp_obj = get_obj_from_waste(items_list()[item_num]->temporary_ids);

		if (!tmp_obj)
		{
			log("SYSERROR : не удалось прочитать предмет (%s:%d)", __FILE__, __LINE__);
			send_to_char("Ошибочка вышла.\r\n", ch);
			return;
		}

		obj_from_proto = false;
	}

	auto proto = (tmp_obj ? tmp_obj : get_object_prototype(items_list()[item_num]->rnum, REAL).get());
	if (!proto)
	{
		log("SYSERROR : не удалось прочитать прототип (%s:%d)", __FILE__, __LINE__);
		send_to_char("Ошибочка вышла.\r\n", ch);
		return;
	}

	const long price = items_list()[item_num]->price;

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
			? items_list()[item_num]->get_item_name(GET_MOB_VNUM(keeper), 3).c_str()
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
	int sell_count = can_sell_count(item_num);

	OBJ_DATA *obj = 0;
	while (bought < item_count
		&& check_money(ch, price, currency)
		&& IS_CARRYING_N(ch) < CAN_CARRY_N(ch)
		&& IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto) <= CAN_CARRY_W(ch)
		&& (bought < sell_count || sell_count == -1))
	{

		if (!items_list()[item_num]->temporary_ids.empty())
		{
			obj = get_obj_from_waste(items_list()[item_num]->temporary_ids);
			items_list()[item_num]->temporary_ids.erase(items_list()[item_num]->temporary_ids.begin());
			if (items_list()[item_num]->temporary_ids.empty())
			{
				items_list().erase(items_list().begin() + item_num);
			}
			remove_from_waste(obj);
		}
		else
		{
			obj = world_objects.create_from_prototype_by_rnum(items_list()[item_num]->rnum).get();
			if (obj
				&& !items_list()[item_num]->descs.empty()
				&& items_list()[item_num]->descs.find(GET_MOB_VNUM(keeper)) != items_list()[item_num]->descs.end())
			{
				items_list()[item_num]->replace_descs(obj, GET_MOB_VNUM(keeper));
			}
		}

		if (obj)
		{
			if (GET_OBJ_ZONE(obj) == NOWHERE)
			{
				obj->set_zone(world[ch->in_room]->zone);
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
				GloryConst::add_total_spent(price);

				int removed = Glory::remove_glory(GET_UNIQUE(ch), price);
				if (removed > 0)
				{
					GloryConst::transfer_log("%s bought %s for %d temp glory",
						GET_NAME(ch), GET_OBJ_PNAME(proto, 0).c_str(), removed);
				}

				if (removed != price)
				{
					GloryConst::remove_glory(GET_UNIQUE(ch), price - removed);
					GloryConst::transfer_log("%s bought %s for %d const glory",
						GET_NAME(ch), GET_OBJ_PNAME(proto, 0).c_str(), price - removed);
				}
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

	const auto suffix = desc_count(total_money, currency == "куны" ? WHAT_MONEYu : (currency == "лед" ? WHAT_ICEu : WHAT_GLORYu));
	snprintf(buf, MAX_STRING_LENGTH, "Это будет стоить %d %s.", total_money, suffix);
	tell_to_char(keeper, ch, buf);

	if (obj)
	{
		send_to_char(ch, "Теперь вы стали %s %s.\r\n",
			IS_MALE(ch) ? "счастливым обладателем" : "счастливой обладательницей",
			obj->item_count_message(bought, 1).c_str());
	}
}

void shop_node::print_shop_list(CHAR_DATA *ch, std::string arg, int keeper_vnum)
{
	send_to_char(ch,
		" ##    Доступно   Предмет                                      Цена (%s)\r\n"
		"---------------------------------------------------------------------------\r\n",
		currency.c_str());
	int num = 1;
	std::stringstream out;
	std::string print_value = "";
	std::string name_value = "";

	for (auto k = items_list().begin(); k != items_list().end(); /* empty */)
	{
		int count = can_sell_count(num - 1);

		print_value = "";
		name_value = "";

		//Polud у проданных в магаз объектов отображаем в списке не значение из прототипа, а уже, возможно, измененное значение
		// чтобы не было в списках всяких "гриб @n1"
		if ((*k)->temporary_ids.empty())
		{
			print_value = (*k)->get_item_name(keeper_vnum);
			if (GET_OBJ_TYPE(obj_proto[(*k)->rnum]) == OBJ_DATA::ITEM_DRINKCON)
			{
				print_value += " с " + std::string(drinknames[GET_OBJ_VAL(obj_proto[(*k)->rnum], 2)]);
			}
		}
		else
		{
			OBJ_DATA* tmp_obj = get_obj_from_waste((*k)->temporary_ids);
			if (tmp_obj)
			{
				print_value = tmp_obj->get_short_description();
				name_value = tmp_obj->get_aliases();
				(*k)->price = GET_OBJ_COST(tmp_obj);
			}
			else
			{
				k = items_list().erase(k);
				continue;
			}
		}

		std::string numToShow = (count == -1 || count > 100 ? "Навалом" : boost::lexical_cast<std::string>(count));

		// имхо вполне логично раз уж мы получаем эту надпись в ней и искать
		if (arg.empty()
			|| isname(arg, print_value)
			|| (!name_value.empty()
				&& isname(arg, name_value)))
		{
			std::string format_str = "%4d)  %10s  %-" + std::to_string(std::count(print_value.begin(), print_value.end(), '&') * 2 + 45) + "s %8d\r\n";
			out << boost::str(boost::format(format_str)
				% num++ % numToShow % print_value % (*k)->price);
		}
		else
		{
			num++;
		}

		++k;
	}

	page_string(ch->desc, out.str());
}

void shop_node::filter_shop_list(CHAR_DATA *ch, std::string arg, int keeper_vnum)
{
	int num = 1;
	wear = EWearFlag::ITEM_WEAR_UNDEFINED;
	type = -10;

	std::string print_value = "";
	std::string name_value = "";

	const char *filtr_value = "";
	const char *first_simvol = "";

	if (!arg.empty())
	{
		first_simvol = arg.c_str();
		filtr_value = arg.substr(1, arg.size() - 1).c_str();
	}

	switch (first_simvol[0])
	{
	case 'Т':
		if (!init_type(filtr_value))
		{
			send_to_char("Неверный тип предмета.\r\n", ch);
			return;
		}
		break;

	case 'О':
		if (!init_wear(filtr_value))
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
	for (auto k = items_list().begin(); k != items_list().end(); /* empty */)
	{
		int count = can_sell_count(num - 1);
		bool show_name = true;

		print_value = "";
		name_value = "";

		//Polud у проданных в магаз объектов отображаем в списке не значение из прототипа, а уже, возможно, измененное значение
		// чтобы не было в списках всяких "гриб @n1"
		if ((*k)->temporary_ids.empty())
		{
			print_value = (*k)->get_item_name(keeper_vnum);
			if (GET_OBJ_TYPE(obj_proto[(*k)->rnum]) == OBJ_DATA::ITEM_DRINKCON)
			{
				print_value += " с " + std::string(drinknames[GET_OBJ_VAL(obj_proto[(*k)->rnum], 2)]);
			}

			if (!((wear != EWearFlag::ITEM_WEAR_UNDEFINED
				&& obj_proto[(*k)->rnum]->has_wear_flag(wear))
				|| (type > 0
					&& type == GET_OBJ_TYPE(obj_proto[(*k)->rnum]))))
			{
				show_name = false;
			}
		}
		else
		{
			OBJ_DATA* tmp_obj = get_obj_from_waste((*k)->temporary_ids);
			if (tmp_obj)
			{
				if (!((wear != EWearFlag::ITEM_WEAR_UNDEFINED && CAN_WEAR(tmp_obj, wear))
					|| (type > 0 && type == GET_OBJ_TYPE(tmp_obj))))
				{
					show_name = false;
				}

				print_value = tmp_obj->get_short_description();
				name_value = tmp_obj->get_aliases();
				(*k)->price = GET_OBJ_COST(tmp_obj);
			}
			else
			{
				k = items_list().erase(k);
				continue;
			}
		}

		std::string numToShow = count == -1
			? "Навалом"
			: boost::lexical_cast<std::string>(count);

		if (show_name)
		{
			out << (boost::str(boost::format("%4d)  %10s  %-47s %8d\r\n") % num++ % numToShow % print_value % (*k)->price));
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
	OBJ_DATA *obj;
	OBJ_DATA * obj_next;
	std::string buffer(argument), buffer1;
	GetOneParam(buffer, buffer1);
	boost::trim(buffer);

	if ((cmd == "Продать" || cmd == "Оценить") && !can_buy)
	{
		tell_to_char(keeper, ch, "Извини, у меня свои поставщики...");
		return;
	}

	if (!*argument)
	{
		tell_to_char(keeper, ch, (cmd + " ЧТО?").c_str());
		return;
	}

	if (!buffer1.empty())
	{
		if (is_number(buffer1.c_str()))
		{
			int n = 0;
			try
			{
				n = boost::lexical_cast<int>(buffer1);
			}
			catch (const boost::bad_lexical_cast&)
			{
			}

			obj = get_obj_in_list_vis(ch, buffer, ch->carrying);

			if (!obj)
			{
				send_to_char("У вас нет " + buffer + "!\r\n", ch);
				return;
			}

			while (obj && n > 0)
			{
				obj_next = get_obj_in_list_vis(ch, buffer, obj->get_next_content());
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
				obj = get_obj_in_list_vis(ch, buffer2, ch->carrying);

				if (!obj)
				{
					if (cmd == "Чинить" && is_abbrev(argument, "экипировка"))
					{
						for (i = 0; i < NUM_WEARS; i++)
							if (ch->equipment[i])
								do_shop_cmd(ch, keeper, ch->equipment[i], cmd);
						return;
					}
					send_to_char("У вас нет " + buffer2 + "!\r\n", ch);
					return;
				}

				do_shop_cmd(ch, keeper, obj, cmd);
				break;

			case FIND_ALL:
				for (obj = ch->carrying; obj; obj = obj_next)
				{
					obj_next = obj->get_next_content();
					do_shop_cmd(ch, keeper, obj, cmd);
				}
				break;

			case FIND_ALLDOT:
				obj = get_obj_in_list_vis(ch, buffer2, ch->carrying);
				if (!obj)
				{
					send_to_char("У вас нет " + buffer2 + "!\r\n", ch);
					return;
				}

				while (obj)
				{
					obj_next = get_obj_in_list_vis(ch, buffer2, obj->get_next_content());
					do_shop_cmd(ch, keeper, obj, cmd);
					obj = obj_next;
				}
				break;

			default:
				break;
			};
		}
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

	if (!item_num || item_num > items_list().size())
	{
		tell_to_char(keeper, ch, "Протри глаза, у меня нет такой вещи.");
		return;
	}

	--item_num;

	const OBJ_DATA *ident_obj = nullptr;
	OBJ_DATA* tmp_obj = nullptr;
	if (items_list()[item_num]->temporary_ids.empty())
	{
		if (!items_list()[item_num]->descs.empty() &&
			items_list()[item_num]->descs.find(GET_MOB_VNUM(keeper)) != items_list()[item_num]->descs.end())
		{
			tmp_obj = world_objects.create_from_prototype_by_rnum(items_list()[item_num]->rnum).get();
			items_list()[item_num]->replace_descs(tmp_obj, GET_MOB_VNUM(keeper));
			ident_obj = tmp_obj;
		}
		else
		{
			const auto object = world_objects.create_raw_from_prototype_by_rnum(items_list()[item_num]->rnum);
			ident_obj = tmp_obj = object.get();
		}
	}
	else
	{
		ident_obj = get_obj_from_waste(items_list()[item_num]->temporary_ids);
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

void shop_node::remove_item_id(unsigned uid)
{
	for (auto k = items_list().begin(); k != items_list().end(); ++k)
	{
		for (auto it = (*k)->temporary_ids.begin(); it != (*k)->temporary_ids.end(); ++it)
		{
			if ((*it) == uid)
			{
				(*k)->temporary_ids.erase(it);

				if ((*k)->temporary_ids.empty())
				{
					items_list().erase(k);
				}

				return;
			}
		}
	}
}

OBJ_DATA* shop_node::get_obj_from_waste(const item_node::temporary_ids_t& uids)
{
	for (auto it = waste().begin(); it != waste().end();)
	{
		if (it->obj->get_rnum() == it->rnum)
		{
			if (0 != uids.size()
				&& it->obj->get_uid() == uids[0])
			{
				return it->obj;
			}

			++it;
		}
		else
		{
			it = waste().erase(it);
		}
	}

	return nullptr;
}

void shop_node::remove_from_waste(OBJ_DATA *obj)
{
	for (auto it = waste().begin(); it != waste().end(); ++it)
	{
		if (it->obj == obj)
		{
			waste().erase(it);

			return;
		}
	}
}

unsigned shop_node::get_item_num(std::string &item_name, int keeper_vnum)
{
	int num = 1;
	if (!item_name.empty() && a_isdigit(item_name[0]))
	{
		std::string::size_type dot_idx = item_name.find_first_of(".");
		if (dot_idx != std::string::npos)
		{
			std::string first_param = item_name.substr(0, dot_idx);
			item_name.erase(0, ++dot_idx);
			if (is_number(first_param.c_str()))
			{
				try
				{
					num = boost::lexical_cast<int>(first_param);
				}
				catch (const boost::bad_lexical_cast&)
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
		if (items_list()[i]->temporary_ids.empty())
		{
			name_value = items_list()[i]->get_item_name(keeper_vnum);
			if (GET_OBJ_TYPE(obj_proto[items_list()[i]->rnum]) == OBJ_DATA::ITEM_DRINKCON)
			{
				name_value += " " + std::string(drinknames[GET_OBJ_VAL(obj_proto[items_list()[i]->rnum], 2)]);
			}
		}
		else
		{
			OBJ_DATA * tmp_obj = get_obj_from_waste((items_list()[i])->temporary_ids);
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

int shop_node::can_sell_count(int item_num) const
{
	if (items_list()[item_num]->temporary_ids.size() != 0)
	{
		return static_cast<int>(items_list()[item_num]->temporary_ids.size());
	}
	else
	{
		int numToSell = obj_proto[items_list()[item_num]->rnum]->get_max_in_world();
		if (numToSell == 0)
		{
			return numToSell;
		}

		if (numToSell != -1)
		{
			numToSell -= MIN(numToSell, obj_proto.actual_count(items_list()[item_num]->rnum));	//считаем не только онлайн, но и то что в ренте
		}

		return numToSell;
	}
}

void shop_node::put_item_in_shop(OBJ_DATA* obj)
{
	for (const auto& it : items_list())
	{
		if (it->rnum == GET_OBJ_RNUM(obj))
		{
			if (it->temporary_ids.empty())
			{
				extract_obj(obj);
				return;
			}
			else
			{
				OBJ_DATA* tmp_obj = get_obj_from_waste(it->temporary_ids);
				if (!tmp_obj)
				{
					continue;
				}

				if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_MING //а у них всех один рнум
					|| obj->get_short_description() == tmp_obj->get_short_description())
				{
					it->temporary_ids.push_back(obj->get_uid());
					waste_node tmp_node;
					tmp_node.obj = obj;
					tmp_node.rnum = obj->get_rnum();
					add_waste(tmp_node);

					return;
				}
			}
		}
	}

	const auto tmp_item = std::make_shared<item_node>();
	tmp_item->rnum = GET_OBJ_RNUM(obj);
	tmp_item->price = GET_OBJ_COST(obj);
	tmp_item->temporary_ids.push_back(obj->get_uid());
	add_item(tmp_item);

	waste_node tmp_node;
	tmp_node.rnum = obj->get_rnum();
	tmp_node.obj = obj;
	add_waste(tmp_node);
}

void shop_node::do_shop_cmd(CHAR_DATA* ch, CHAR_DATA *keeper, OBJ_DATA* obj, std::string cmd)
{
	if (!obj) return;
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
			put_item_in_shop(obj);
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

using ShopListType = std::vector<shop_node::shared_ptr>;

struct item_set_node
{
	long item_vnum;
	int item_price;
};

struct item_set
{
	item_set() {};

	std::string _id;
	std::vector<item_set_node> item_list;
};

typedef boost::shared_ptr<item_set> ItemSetPtr;
typedef std::vector<ItemSetPtr> ItemSetListType;
ShopListType shop_list;

void log_shop_load()
{
	for (const auto& shop : shop_list)
	{
		for (const auto& mob_vnum : shop->mob_vnums())
		{
			log("ShopExt: mob=%d", mob_vnum);
		}

		log("ShopExt: currency=%s", shop->currency.c_str());

		for (const auto& item : shop->items_list())
		{
			log("ItemList: vnum=%d, price=%ld", obj_proto[item->rnum]->get_vnum(), item->price);
		}
	}
}

void empty_waste(const shop_node::shared_ptr &shop)
{
	for (const auto& waste_item : shop->waste())
	{
		if (GET_OBJ_RNUM(waste_item.obj) == waste_item.rnum)
		{
			extract_obj(waste_item.obj);
		}
	}

	shop->clear_waste();
}

std::list<std::string> split(std::string s, char c)
{
	std::list<std::string> result;
	size_t idx = s.find_first_of(c);
	while (idx != std::string::npos)
	{
		size_t l = s.length();
		result.push_back(s.substr(0, idx));
		s = s.substr(idx+1, l-idx);
		idx = s.find_first_of(c);
	}
	if (!s.empty())
		result.push_back(s);
	return result;
}

void load_item_desc()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_PLRSTUFF"/shop/item_desc.xml");
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	pugi::xml_node node_list = doc.child("templates");
    if (!node_list)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...templates list read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }
	item_descriptions.clear();
	pugi::xml_node child;
	for (pugi::xml_node item_template = node_list.child("template"); item_template; item_template = item_template.next_sibling("template"))
	{
		std::string templateId = item_template.attribute("id").value();
		for (pugi::xml_node item = item_template.child("item"); item; item = item.next_sibling("item"))
		{

			int item_vnum = Parse::attr_int(item, "vnum");
			if (item_vnum <= 0)
			{
					snprintf(buf, MAX_STRING_LENGTH,
						"...bad item description attributes (item_vnum=%d)", item_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
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
			desc_node.sex = static_cast<ESex>(atoi(child.child_value()));

			// парсим список триггеров
			pugi::xml_node trig_list = item.child("triggers");
			CObjectPrototype::triggers_list_t trig_vnums;
			for (pugi::xml_node trig = trig_list.child("trig"); trig; trig = trig.next_sibling("trig"))
			{
				int trig_vnum;
				std::string tmp_value = trig.child_value();
				boost::trim(tmp_value);
				try
				{
					trig_vnum = boost::lexical_cast<unsigned>(tmp_value);
				}
				catch (boost::bad_lexical_cast &)
				{
					snprintf(buf, MAX_STRING_LENGTH, "...error while casting to num (item_vnum=%d, casting value=%s)", item_vnum, tmp_value.c_str());
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					continue;
				}

				if (trig_vnum <= 0)
				{
					snprintf(buf, MAX_STRING_LENGTH, "...error while parsing triggers (item_vnum=%d, parsed value=%s)", item_vnum, tmp_value.c_str());
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
					return;
				}
				trig_vnums.push_back(trig_vnum);
			}
			desc_node.trigs = trig_vnums;
			item_descriptions[templateId][item_vnum] = desc_node;
		}
	}
}

void load(bool reload)
{
	if (reload)
	{
		for (const auto& shop : shop_list)
		{
			empty_waste(shop);

			for (const auto& mob_vnum : shop->mob_vnums())
			{
				int mob_rnum = real_mobile(mob_vnum);
				if (mob_rnum >= 0)
				{
					mob_index[mob_rnum].func = NULL;
				}
			}
		}

		shop_list.clear();
	}
	load_item_desc();

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_PLRSTUFF"/shop/shops.xml");
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
    pugi::xml_node node_list = doc.child("shop_list");
    if (!node_list)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...shop_list read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }
	//наборы предметов - "заготовки" для реальных предметов в магазинах. живут только на время лоада
	ItemSetListType itemSetList;
	pugi::xml_node itemSets = doc.child("shop_item_sets");
	for (pugi::xml_node itemSet = itemSets.child("shop_item_set"); itemSet; itemSet = itemSet.next_sibling("shop_item_set"))
	{
		std::string itemSetId = itemSet.attribute("id").value();
		ItemSetPtr tmp_set(new item_set);
		tmp_set->_id = itemSetId;

		for (pugi::xml_node item = itemSet.child("item"); item; item = item.next_sibling("item"))
		{
			int item_vnum = Parse::attr_int(item, "vnum");
			int price = Parse::attr_int(item, "price");
			if (item_vnum < 0 || price < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH,
					"...bad shop item set attributes (item_set=%s, item_vnum=%d, price=%d)", itemSetId.c_str(), item_vnum, price);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}
			struct item_set_node tmp_node;
			tmp_node.item_price = price;
			tmp_node.item_vnum = item_vnum;
			tmp_set->item_list.push_back(tmp_node);
		}
		itemSetList.push_back(tmp_set);
	}

	for (pugi::xml_node node = node_list.child("shop"); node; node = node.next_sibling("shop"))
	{
		std::string shop_id = node.attribute("id").value();
		std::string currency = node.attribute("currency").value();
		int profit = node.attribute("profit").as_int();
		std::string can_buy_value = node.attribute("can_buy").value();
		bool shop_can_buy = can_buy_value != "false";
		int waste_time_min = (node.attribute("waste_time_min").value() ? node.attribute("waste_time_min").as_int() : 180);

		// иним сам магазин
		const auto tmp_shop = std::make_shared<shop_node>();
		tmp_shop->id = shop_id;
		tmp_shop->currency = currency;
		tmp_shop->profit = profit;
		tmp_shop->can_buy = shop_can_buy;
		tmp_shop->waste_time_min = waste_time_min;
		//словарные данные
		tmp_shop->SetDictionaryName(shop_id);//а нету у магазинов названия
		tmp_shop->SetDictionaryTID(shop_id);

		std::map<int, std::string> mob_to_template;

		for (pugi::xml_node mob = node.child("mob"); mob; mob=mob.next_sibling("mob"))
		{
			int mob_vnum = Parse::attr_int(mob, "mob_vnum");
			std::string templateId = mob.attribute("template").value();
			if (mob_vnum < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH,
					"...bad shop attributes (mob_vnum=%d shop id=%s)", mob_vnum, shop_id.c_str());
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}

			if (!templateId.empty())
			{
				mob_to_template[mob_vnum] = templateId;
			}

			tmp_shop->add_mob_vnum(mob_vnum);
			// проверяем и сетим мобу спешиал
			// даже если дальше магаз не залоадится - моб будет выдавать ошибку на магазинные спешиалы
			int mob_rnum = real_mobile(mob_vnum);
			if (mob_rnum >= 0)
			{
				if (mob_index[mob_rnum].func
					&& mob_index[mob_rnum].func != shop_ext)
				{
					snprintf(buf, MAX_STRING_LENGTH,
						"...shopkeeper already with special (mob_vnum=%d)", mob_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				}
				else
				{
					mob_index[mob_rnum].func = shop_ext;
				}
			}
			else
			{
				snprintf(buf, MAX_STRING_LENGTH, "...incorrect mob_vnum=%d", mob_vnum);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			}
		}

		// и список его продукции
		for (pugi::xml_node item = node.child("item"); item; item = item.next_sibling("item"))
		{
			int item_vnum = Parse::attr_int(item, "vnum");
			int price = Parse::attr_int(item, "price");
			std::string items_template = item.attribute("template").value();
			if (item_vnum < 0
				|| price < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH,
					"...bad shop attributes (item_vnum=%d, price=%d)", item_vnum, price);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}

			// проверяем шмотку
			int item_rnum = real_object(item_vnum);
			if (item_rnum < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH, "...incorrect item_vnum=%d", item_vnum);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}

			// иним ее в магазе
			const auto tmp_item = std::make_shared<item_node>();
			tmp_item->rnum = item_rnum;
			tmp_item->vnum = item_vnum;
			tmp_item->price = price == 0 ? GET_OBJ_COST(obj_proto[item_rnum]) : price; //если не указана цена - берем цену из прототипа

			tmp_shop->add_item(tmp_item);
		}

		//и еще добавим наборы
		for (pugi::xml_node itemSet = node.child("item_set"); itemSet; itemSet = itemSet.next_sibling("item_set"))
		{
			std::string itemSetId = itemSet.child_value();
			for (ItemSetListType::const_iterator it = itemSetList.begin(); it != itemSetList.end();++it)
			{
				if ((*it)->_id == itemSetId)
				{
					for (unsigned i = 0; i < (*it)->item_list.size(); i++)
					{
						// проверяем шмотку
						int item_rnum = real_object((*it)->item_list[i].item_vnum);
						if (item_rnum < 0)
						{
							snprintf(buf, MAX_STRING_LENGTH, "...incorrect item_vnum=%d in item_set=%s", (int)(*it)->item_list[i].item_vnum, (*it)->_id.c_str());
							mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
							return;
						}
						int price = (*it)->item_list[i].item_price;
						// иним ее в магазе
						const auto tmp_item = std::make_shared<item_node>();
						tmp_item->rnum = item_rnum;
						tmp_item->vnum = (*it)->item_list[i].item_vnum;
						tmp_item->price = price == 0 ? GET_OBJ_COST(obj_proto[item_rnum]) : price;
						tmp_shop->add_item(tmp_item);
					}
				}
			}
		}

		if (tmp_shop->items_list().empty())
		{
			snprintf(buf, MAX_STRING_LENGTH, "...item list empty (shop_id=%s)", shop_id.c_str());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}

		for (auto& it : tmp_shop->items_list())
		{
			for (auto id = item_descriptions.begin(); id != item_descriptions.end(); ++id)
			{
				if (id->second.find(it->vnum) != id->second.end())
				{
					item_desc_node tmp_desc(id->second[it->vnum]);
					for (const auto& mob_vnum : tmp_shop->mob_vnums())
					{
						if (mob_to_template.find(mob_vnum) != mob_to_template.end()
							&& mob_to_template[mob_vnum] == id->first)
						{
							it->descs[mob_vnum] = tmp_desc;
						}
					}
				}
			}
		}

		shop_list.push_back(tmp_shop);
    }

    log_shop_load();
}

void update_timers()
{
	for (auto& shop : shop_list)
	{
		shop->update_shop_timers();
	}
}

long get_sell_price(OBJ_DATA * obj)
{
	long cost = GET_OBJ_COST(obj);
	long cost_obj = GET_OBJ_COST(obj);
	int timer = obj_proto[GET_OBJ_RNUM(obj)]->get_timer();
	if (timer < obj->get_timer())
	    obj->set_timer(timer);
	cost = timer <= 0 ? 1 : (long)cost * ((float)obj->get_timer() / (float)timer); //учтем таймер

	// если цена продажи, выше, чем стоймость предмета
	if (cost > cost_obj)
	{
		cost = cost_obj;
	}
	return MMAX(1, cost);
}

int get_spent_today()
{
	return spent_today;
}

} // namespace ShopExt

using namespace ShopExt;

int shop_ext(CHAR_DATA *ch, void *me, int cmd, char* argument)
{
	if (!ch->desc
		|| IS_NPC(ch))
	{
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
		|| CMD_IS("identify")))
	{
		return 0;
	}

	char argm[MAX_INPUT_LENGTH];
	CHAR_DATA * const keeper = reinterpret_cast<CHAR_DATA *>(me);
	shop_node::shared_ptr shop;
	for (const auto& s : shop_list)
	{
		const auto found = std::find(s->mob_vnums().begin(), s->mob_vnums().end(), GET_MOB_VNUM(keeper)) != s->mob_vnums().end();
		if (found)
		{
			shop = s;
			break;
		}
	}

	if (!shop)
	{
		log("SYSERROR : магазин не найден mob_vnum=%d (%s:%d)", GET_MOB_VNUM(keeper), __FILE__, __LINE__);
		send_to_char("Ошибочка вышла.\r\n", ch);

		return 1;
	}


	if (CMD_IS("steal")
		|| CMD_IS("украсть"))
	{
		sprintf(argm, "$N вскричал$g '%s'", MSG_NO_STEAL_HERE);
		sprintf(buf, "ругать %s", GET_NAME(ch));
		do_social(keeper, buf);
		act(argm, FALSE, ch, 0, keeper, TO_CHAR);
		return 1;
	}

	if (CMD_IS("список")
		|| CMD_IS("list"))
	{
		std::string buffer = argument, buffer2;
		GetOneParam(buffer, buffer2);
		shop->print_shop_list(ch, buffer2, GET_MOB_VNUM(keeper));
		return 1;
	}

	if (CMD_IS("фильтровать")
		|| CMD_IS("filter"))
	{
		std::string buffer = argument, buffer2;
		GetOneParam(buffer, buffer2);
		shop->filter_shop_list(ch, buffer2, GET_MOB_VNUM(keeper));
		return 1;
	}

	if (CMD_IS("купить")
		|| CMD_IS("buy"))
	{
		shop->process_buy(ch, keeper, argument);
		return 1;
	}
	if (CMD_IS("характеристики") || CMD_IS("identify"))
	{
		shop->process_ident(ch, keeper, argument, "Характеристики");
		return 1;
	}


	if (CMD_IS("value") || CMD_IS("оценить"))
	{
		shop->process_cmd(ch, keeper, argument, "Оценить");
		return 1;
	}
	if (CMD_IS("продать") || CMD_IS("sell"))
	{
		shop->process_cmd(ch, keeper, argument, "Продать");
		return 1;
	}
	if (CMD_IS("чинить") || CMD_IS("repair"))
	{
		shop->process_cmd(ch, keeper, argument, "Чинить");
		return 1;
	}
	if (CMD_IS("рассмотреть") || CMD_IS("examine"))
	{
		shop->process_ident(ch, keeper, argument, "Рассмотреть");
		return 1;
	}

	return 0;
}

// * Лоад странствующих продавцов в каждой ренте.
void town_shop_keepers()
{
	// список уже оработанных зон, чтобы не грузить двух и более торгашей в одну
	std::set<int> zone_list;

	for (CHAR_DATA *ch = character_list; ch; ch = ch->get_next())
	{
		if (IS_RENTKEEPER(ch)
			&& ch->in_room > 0
			&& !Clan::GetClanByRoom(ch->in_room)
			&& !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)
			&& GET_ROOM_VNUM(ch->in_room) % 100 != 99
			&& zone_list.find(world[ch->in_room]->zone) == zone_list.end())
		{
			int rnum_start, rnum_end;
			if (get_zone_rooms(world[ch->in_room]->zone, &rnum_start, &rnum_end))
			{
				CHAR_DATA *mob = read_mobile(1901, VIRTUAL);
				if (mob)
				{
					char_to_room(mob, number(rnum_start, rnum_end));
				}
			}
			zone_list.insert(world[ch->in_room]->zone);
		}
	}
}

void do_shops_list(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	DictionaryPtr dic = DictionaryPtr(new Dictionary(SHOP));
	size_t n = dic->Size();
	std::string out;
	for (size_t i = 0; i < n; i++)
	{
		out = out + boost::lexical_cast<std::string>(i + 1) + " " + dic->GetNameByNID(i) + " " + dic->GetTIDByNID(i) + "\r\n";
	}
	send_to_char(out, ch);
}

void fill_shop_dictionary(DictionaryType &dic)
{
	ShopExt::ShopListType::const_iterator it = ShopExt::shop_list.begin();
	for (;it != ShopExt::shop_list.end();++it)
		dic.push_back((*it)->GetDictionaryItem());
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
