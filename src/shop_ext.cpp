// $RCSfile: shop_ext.cpp,v $     $Date: 2012/01/27 05:36:40 $     $Revision: 1.16 $
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#include <vector>
#include <string>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/array.hpp>
#include "pugixml.hpp"
#include "shop_ext.hpp"
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
#include "utils.h"
#include "parse.hpp"

/*
Пример конфига (plrstuff/shop/test.xml):
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

extern ACMD(do_echo);
extern int do_social(CHAR_DATA * ch, char *argument);
extern void mort_show_obj_values(const OBJ_DATA * obj, CHAR_DATA * ch, int fullness);
extern int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_align(CHAR_DATA * ch, const OBJ_DATA * obj);
extern char *diag_weapon_to_char(const OBJ_DATA * obj, int show_wear);
extern char *diag_timer_to_char(const OBJ_DATA * obj);


namespace ShopExt
{
long get_sell_price(OBJ_DATA * obj);

const int IDENTIFY_COST = 110;
int spent_today = 0;
const char *MSG_NO_STEAL_HERE = "$n, грязн$w воришка, чеши отседова!";

struct item_desc_node
{
		std::string name;
		std::string description;
		std::string short_description;
		boost::array<std::string, 6> PNames;
		int sex;
		std::list<unsigned> trigs;
};
typedef std::map<int/*vnum предмета*/, item_desc_node> ObjDescType;
std::map<std::string/*id шаблона*/, ObjDescType> item_descriptions;
typedef std::map<int/*vnum продавца*/, item_desc_node> ItemDescNodeList;

struct waste_node
{
	waste_node() : rnum(0), obj(NULL), last_activity(time(NULL)) {};
	int rnum;
	OBJ_DATA * obj;
	int last_activity;
};

struct item_node
{
	item_node() : rnum(0), vnum(0), price(0) {descs.clear(); temporary_ids.clear();};

	int rnum;
	int vnum;
	long price;
	std::vector<unsigned> temporary_ids;
	ItemDescNodeList descs;
};

typedef boost::shared_ptr<item_node> ItemNodePtr;
typedef std::vector<ItemNodePtr> ItemListType;


class shop_node : public DictionaryItem
{
public:
	shop_node() : waste_time_min(0), can_buy(true) {};

	std::list<int> mob_vnums;
	std::string currency;
	std::string id;
	ItemListType item_list;
	std::map<long, ItemListType> keeper_item_list;
	int profit;
	std::list<waste_node> waste;
	int waste_time_min;
	bool can_buy;
};


typedef boost::shared_ptr<shop_node> ShopNodePtr;
typedef std::vector<ShopNodePtr> ShopListType;

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


OBJ_DATA * get_obj_from_waste(ShopListType::const_iterator &shop, std::vector<unsigned> uids);

void log_shop_load()
{
	for (ShopListType::iterator i = shop_list.begin(); i != shop_list.end(); ++i)
	{
		for (std::list<int>::const_iterator it = (*i)->mob_vnums.begin(); it != (*i)->mob_vnums.end(); ++it)
			log("ShopExt: mob=%d",*it);
		log("ShopExt: currency=%s", (*i)->currency.c_str());
		for (ItemListType::iterator k = (*i)->item_list.begin(); k != (*i)->item_list.end(); ++k)
		{
			log("ItemList: vnum=%d, price=%ld", GET_OBJ_VNUM(obj_proto[(*k)->rnum]), (*k)->price);
		}
	}
}

void empty_waste(ShopListType::const_iterator &shop)
{
	std::list<waste_node>::const_iterator it;
	for (it = (*shop)->waste.begin(); it != (*shop)->waste.end(); ++it)
	{
		if (GET_OBJ_RNUM(it->obj) == it->rnum) extract_obj(it->obj);
	}
	(*shop)->waste.clear();
}

std::list<std::string> split(std::string s, char c)
{
	std::list<std::string> result;
	size_t idx = s.find_first_of(c);
	while (idx != std::string::npos)
	{
		int l = s.length();
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
			desc_node.sex = atoi(child.child_value());

			// парсим список триггеров
			pugi::xml_node trig_list = item.child("triggers");
			std::list<unsigned> trig_vnums;
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
		for (ShopListType::const_iterator i = shop_list.begin(); i != shop_list.end(); ++i)
		{
			empty_waste(i);
			for (std::list<int>::iterator k = (*i)->mob_vnums.begin();k!=(*i)->mob_vnums.end(); ++k)
			{
				int mob_rnum = real_mobile((*k));
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
		ShopNodePtr tmp_shop(new shop_node);
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
				mob_to_template[mob_vnum] = templateId;

			tmp_shop->mob_vnums.push_back(mob_vnum);
			// проверяем и сетим мобу спешиал
			// даже если дальше магаз не залоадится - моб будет выдавать ошибку на магазинные спешиалы
			int mob_rnum = real_mobile(mob_vnum);
			if (mob_rnum >= 0)
			{
				if (mob_index[mob_rnum].func && mob_index[mob_rnum].func != shop_ext)
				{
					snprintf(buf, MAX_STRING_LENGTH,
						"...shopkeeper already with special (mob_vnum=%d)", mob_vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				}else
					mob_index[mob_rnum].func = shop_ext;
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
			if (item_vnum < 0 || price < 0)
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
			ItemNodePtr tmp_item(new item_node);
			tmp_item->rnum = item_rnum;
			tmp_item->vnum = item_vnum;
			tmp_item->price = price == 0 ? GET_OBJ_COST(obj_proto[item_rnum]) : price; //если не указана цена - берем цену из прототипа
			tmp_shop->item_list.push_back(tmp_item);
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
						ItemNodePtr tmp_item(new item_node);
						tmp_item->rnum = item_rnum;
						tmp_item->vnum = (*it)->item_list[i].item_vnum;
						tmp_item->price = price == 0 ? GET_OBJ_COST(obj_proto[item_rnum]) : price;
						tmp_shop->item_list.push_back(tmp_item);
					}
				}
			}
		}
		if (tmp_shop->item_list.empty())
		{
			snprintf(buf, MAX_STRING_LENGTH, "...item list empty (shop_id=%s)", shop_id.c_str());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}

		//ищем замену описаний
		ItemListType::iterator it;

		std::map<std::string/*id шаблона*/, ObjDescType>::iterator id;

		for (it = tmp_shop->item_list.begin(); it != tmp_shop->item_list.end(); ++it)
		{
			for (id = item_descriptions.begin(); id != item_descriptions.end(); ++id)
			{
				if (id->second.find((*it)->vnum) != id->second.end())
				{
					item_desc_node tmp_desc(id->second[(*it)->vnum]);
					for (std::list<int>::iterator mob_vnum = tmp_shop->mob_vnums.begin(); mob_vnum != tmp_shop->mob_vnums.end(); ++mob_vnum)
					{
						if (mob_to_template.find((*mob_vnum)) != mob_to_template.end())
							if (mob_to_template[(*mob_vnum)] == id->first)
								(*it)->descs[(*mob_vnum)] = tmp_desc;
					}
				}
			}
		}
		shop_list.push_back(tmp_shop);
    }
    log_shop_load();
}

std::string get_item_name(ItemNodePtr item, int keeper_vnum, int pad = 0)
{
	std::string value;
	if (!item->descs.empty() && item->descs.find(keeper_vnum) != item->descs.end())
		value = item->descs[keeper_vnum].PNames[pad];
	else
		value = GET_OBJ_PNAME(obj_proto[item->rnum], pad);
	return value;
}

unsigned get_item_num(ShopListType::const_iterator &shop, std::string &item_name, int keeper_vnum)
{
	int num = 1;
	if (!item_name.empty() && isdigit(item_name[0]))
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
				} catch(boost::bad_lexical_cast const& e)
				{
					return 0;
				}
			}
		}
	}

	int count = 0;
	std::string name_value="";
	std::string print_value;
	for (unsigned i = 0; i < (*shop)->item_list.size(); ++i)
	{
		print_value="";
		if ((*shop)->item_list[i]->temporary_ids.empty())
		{
			name_value = get_item_name((*shop)->item_list[i], keeper_vnum);
			if (GET_OBJ_TYPE(obj_proto[(*shop)->item_list[i]->rnum]) == ITEM_DRINKCON)
				name_value += " " + std::string(drinknames[GET_OBJ_VAL(obj_proto[(*shop)->item_list[i]->rnum], 2)]);
		}
		else
		{
			OBJ_DATA * tmp_obj = get_obj_from_waste(shop, ((*shop)->item_list[i])->temporary_ids);
			if (!tmp_obj)
				continue;
			name_value = std::string(tmp_obj->aliases);
			print_value = std::string(tmp_obj->short_description);
		}

		if (isname(item_name, name_value.c_str()) || isname(item_name, print_value.c_str()))
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

std::string item_count_message(const OBJ_DATA *obj, int num, int pad)
{
	if (!obj || num <= 0 || pad < 0 || pad > 5 || !GET_OBJ_PNAME(obj, pad))
	{
		log("SYSERROR : obj=%s num=%d, pad=%d, pname=%s (%s:%d)",
			obj ? "true" : "false", num, pad, GET_OBJ_PNAME(obj, pad), __FILE__, __LINE__);
		return "<ERROR>";
	}

	char local_buf[MAX_INPUT_LENGTH];
	std::string out(GET_OBJ_PNAME(obj, pad));
	if (num > 1)
	{
		snprintf(local_buf, MAX_INPUT_LENGTH, " (x %d %s)", num, desc_count(num, WHAT_THINGa));
		out += local_buf;
	}
	return out;
}

bool check_money(CHAR_DATA *ch, long price, std::string currency)
{
	if (currency == "слава")
	{
		return Glory::get_glory(GET_UNIQUE(ch))
			+ GloryConst::get_glory(GET_UNIQUE(ch)) >= price;
	}
	if (currency == "куны")
		return ch->get_gold() >= price;
	return false;
}

int can_sell_count(ShopListType::const_iterator &shop, int item_num)
{
	if ((*shop)->item_list[item_num]->temporary_ids.size() != 0)
		return (*shop)->item_list[item_num]->temporary_ids.size();
	else
	{
		int numToSell = obj_proto[(*shop)->item_list[item_num]->rnum]->max_in_world;
		if (numToSell == 0)
			return numToSell;
		if (numToSell != -1)
			numToSell -= MIN(numToSell, obj_index[(*shop)->item_list[item_num]->rnum].number
					+ obj_index[(*shop)->item_list[item_num]->rnum].stored);//считаем не только онлайн, но и то что в ренте
		return numToSell;
	}
}

void remove_item_id(ShopListType::const_iterator &shop, unsigned uid)
{
	for (ItemListType::iterator k = (*shop)->item_list.begin();k!= (*shop)->item_list.end(); ++k)
	{
		for(std::vector<unsigned>::iterator it = (*k)->temporary_ids.begin(); it != (*k)->temporary_ids.end(); ++it)
		{
			if ((*it) == uid)
			{
				(*k)->temporary_ids.erase(it);
				if ((*k)->temporary_ids.empty())
					(*shop)->item_list.erase(k);
				return;
			}
		}
	}
}

void update_shop_timers(ShopListType::const_iterator &shop)
{
	std::list<waste_node>::iterator it;
	int cur_time = time(NULL);
	int waste_time = (*shop)->waste_time_min * 60;
	for (it = (*shop)->waste.begin(); it != (*shop)->waste.end();)
	{
		it->obj->dec_timer();
		if (it->obj->get_timer() <= 0 || ((waste_time > 0) && (cur_time - it->last_activity > waste_time)))
		{
			remove_item_id(shop, it->obj->uid);
			if (it->obj->item_number == it->rnum) extract_obj(it->obj);
			it = (*shop)->waste.erase(it);
		}
		else
			++it;
	}
}

void update_timers()
{
	for (ShopListType::const_iterator shop = shop_list.begin(); shop != shop_list.end(); ++shop)
	{
		update_shop_timers(shop);
	}
}

OBJ_DATA * get_obj_from_waste(ShopListType::const_iterator &shop, std::vector<unsigned> uids)
{
	std::list<waste_node>::iterator it;
	for (it = (*shop)->waste.begin(); it != (*shop)->waste.end();)
	{
		if (it->obj->item_number == it->rnum)
		{
			if (it->obj->uid == uids[0])
			{
				return (it->obj);
			}
			 ++it;
		}
		else
			it=(*shop)->waste.erase(it);

	}
	return 0;
}

void print_shop_list(CHAR_DATA *ch, ShopListType::const_iterator &shop, std::string arg, int keeper_vnum)
{
	send_to_char(ch,
		" ##    Доступно   Предмет                                      Цена (%s)\r\n"
		"---------------------------------------------------------------------------\r\n",
		(*shop)->currency.c_str());
	int num = 1;
	std::string out;
	std::string print_value="";
	std::string name_value="";

	for (ItemListType::iterator k = (*shop)->item_list.begin();
		k != (*shop)->item_list.end(); /* empty */)
	{
		int count = can_sell_count(shop, num - 1);

		print_value="";
		name_value="";

		//Polud у проданных в магаз объектов отображаем в списке не значение из прототипа, а уже, возможно, измененное значение
		// чтобы не было в списках всяких "гриб @n1"
		if ((*k)->temporary_ids.empty())
		{
			print_value = get_item_name((*k), keeper_vnum);
			if (GET_OBJ_TYPE(obj_proto[(*k)->rnum]) == ITEM_DRINKCON)
				print_value += " с " + std::string(drinknames[GET_OBJ_VAL(obj_proto[(*k)->rnum], 2)]);
		}
		else
		{
			OBJ_DATA * tmp_obj = get_obj_from_waste(shop, (*k)->temporary_ids);
			if (tmp_obj)
			{
				print_value = std::string(tmp_obj->short_description);
				name_value = std::string(tmp_obj->aliases);
				(*k)->price = get_sell_price(tmp_obj);
			}
			else
			{
				k = (*shop)->item_list.erase(k);
				continue;
			}
		}

		std::string numToShow = count == -1 ? "Навалом" : boost::lexical_cast<string>(count);

		// имхо вполне логично раз уж мы получаем эту надпись в ней и искать
		if (arg.empty()
			|| isname(arg.c_str(), print_value.c_str())
			|| (!name_value.empty() && isname(arg.c_str(), name_value.c_str())))
		{
			out += boost::str(boost::format("%3d)  %10s  %-47s %8d\r\n")
				% num++ % numToShow % print_value % (*k)->price);
		}
		else
		{
			num++;
		}

		++k;
	}
	page_string(ch->desc, out);
//	send_to_char("В корзине " + boost::lexical_cast<string>((*shop)->waste.size()) + " элементов\r\n", ch);
}

void remove_from_waste(ShopListType::const_iterator &shop, OBJ_DATA *obj)
{
	std::list<waste_node>::iterator it;
	for (it = (*shop)->waste.begin(); it != (*shop)->waste.end(); ++it)
	{
		if (it->obj == obj)
		{
			(*shop)->waste.erase(it);
			return;
		}
	}
}
void attach_triggers(OBJ_DATA *obj, std::list<unsigned> trigs)
{
	if (!obj->script)
		CREATE(obj->script, SCRIPT_DATA, 1);
	for (std::list<unsigned>::iterator it = trigs.begin(); it != trigs.end(); ++it)
	{
		int rnum = real_trigger(*it);
		if (rnum != -1)
			add_trigger(obj->script, read_trigger(rnum), -1);
	}
}

void replace_descs(OBJ_DATA *obj, ItemNodePtr item, int vnum)
{
	obj->description = strdup(item->descs[vnum].description.c_str());
	obj->aliases = strdup(item->descs[vnum].name.c_str());
	obj->short_description = strdup(item->descs[vnum].short_description.c_str());
	obj->PNames[0]= strdup(item->descs[vnum].PNames[0].c_str());
	obj->PNames[1]= strdup(item->descs[vnum].PNames[1].c_str());
	obj->PNames[2]= strdup(item->descs[vnum].PNames[2].c_str());
	obj->PNames[3]= strdup(item->descs[vnum].PNames[3].c_str());
	obj->PNames[4]= strdup(item->descs[vnum].PNames[4].c_str());
	obj->PNames[5]= strdup(item->descs[vnum].PNames[5].c_str());
	obj->obj_flags.Obj_sex = item->descs[vnum].sex;
	if (!item->descs[vnum].trigs.empty())
		attach_triggers(obj, item->descs[vnum].trigs);
	obj->ex_description = NULL; //Пока в конфиге нельзя указать экстраописания - убираем нафиг
	if ((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) && (GET_OBJ_VAL(obj, 1) > 0)) //Если работаем с непустой емкостью...
		name_to_drinkcon(obj, GET_OBJ_VAL(obj, 2)); //...Следует указать содержимое емкости
}

void process_buy(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, ShopListType::const_iterator &shop)
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

	if(buffer2.empty())
	{
		if (is_number(buffer1.c_str()))
		{
			// buy 5
			try
			{
				item_num = boost::lexical_cast<unsigned>(buffer1);
			} catch(boost::bad_lexical_cast const& e)
			{
				item_num = 0;
			}
		}
		else
		{
			// buy sword
			item_num = get_item_num(shop, buffer1, GET_MOB_VNUM(keeper));
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
			} catch(boost::bad_lexical_cast const& e)
			{
				item_num = 0;
			}
		}
		else
		{
			// buy 5 sword
			item_num = get_item_num(shop, buffer2, GET_MOB_VNUM(keeper));
		}
		try
		{
			item_count = boost::lexical_cast<unsigned>(buffer1);
		} catch(boost::bad_lexical_cast const& e)
		{
			item_count = 1000;
		}
	}
	else
	{
		tell_to_char(keeper, ch, "ЧТО ты хочешь купить?");
		return;
	}

	if (!item_count || !item_num || (unsigned)item_num > (*shop)->item_list.size())
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
	OBJ_DATA * tmp_obj = NULL;
	bool obj_from_proto = true;
	if (!(*shop)->item_list[item_num]->temporary_ids.empty())
	{
		tmp_obj = get_obj_from_waste(shop, ((*shop)->item_list[item_num])->temporary_ids);
		if (!tmp_obj)
		{
			log("SYSERROR : не удалось прочитать предмет (%s:%d)", __FILE__, __LINE__);
			send_to_char("Ошибочка вышла.\r\n", ch);
			return;
		}
		obj_from_proto = false;
	}
	const OBJ_DATA * const proto = ( tmp_obj ? tmp_obj : read_object_mirror((*shop)->item_list[item_num]->rnum, REAL));
	if (!proto)
	{
		log("SYSERROR : не удалось прочитать прототип (%s:%d)", __FILE__, __LINE__);
		send_to_char("Ошибочка вышла.\r\n", ch);
		return;
	}

	const long price = (*shop)->item_list[item_num]->price;

	if (!check_money(ch, price, (*shop)->currency))
	{
		snprintf(buf, MAX_STRING_LENGTH,
			"У вас нет столько %s!", (*shop)->currency == "куны" ? "денег" : "славы");
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
		snprintf(buf, MAX_STRING_LENGTH,
			"%s, я понимаю, своя ноша карман не тянет,\r\n"
			"но %s вам явно некуда положить.\r\n",
				GET_NAME(ch),
				obj_from_proto ? get_item_name((*shop)->item_list[item_num],
					GET_MOB_VNUM(keeper), 3).c_str() : tmp_obj->short_description);
		send_to_char(buf, ch);
		return;
	}

	int bought = 0;
	int total_money = 0;
	int sell_count = can_sell_count(shop, item_num);

	OBJ_DATA *obj = 0;
	while (bought < item_count
		&& check_money(ch, price, (*shop)->currency)
		&& IS_CARRYING_N(ch) < CAN_CARRY_N(ch)
		&& IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto) <= CAN_CARRY_W(ch)
		&& (bought < sell_count || sell_count == -1))
	{

		if (!(*shop)->item_list[item_num]->temporary_ids.empty())
		{
			obj = get_obj_from_waste(shop, (*shop)->item_list[item_num]->temporary_ids);
			(*shop)->item_list[item_num]->temporary_ids.erase((*shop)->item_list[item_num]->temporary_ids.begin());
			if ((*shop)->item_list[item_num]->temporary_ids.empty())
			{
				(*shop)->item_list.erase((*shop)->item_list.begin() + item_num);
			}
			remove_from_waste(shop, obj);
		}
		else
		{
			obj = read_object((*shop)->item_list[item_num]->rnum, REAL);
			if (obj && !(*shop)->item_list[item_num]->descs.empty() &&
				(*shop)->item_list[item_num]->descs.find(GET_MOB_VNUM(keeper)) != (*shop)->item_list[item_num]->descs.end())
				replace_descs(obj, (*shop)->item_list[item_num], GET_MOB_VNUM(keeper));
		}

		if (obj)
		{
			obj_to_char(obj, ch);
			if ((*shop)->currency == "слава")
			{
				// книги за славу не фейлим
				if (ITEM_BOOK == GET_OBJ_TYPE(obj))
				{
					SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NO_FAIL), ITEM_NO_FAIL);
				}
				// снятие и логирование славы
				GloryConst::add_total_spent(price);
				int removed = Glory::remove_glory(GET_UNIQUE(ch), price);
				if (removed > 0)
				{
					GloryConst::transfer_log("%s bought %s for %d temp glory",
						GET_NAME(ch), GET_OBJ_PNAME(proto, 0), removed);
				}
				if (removed != price)
				{
					GloryConst::remove_glory(GET_UNIQUE(ch), price - removed);
					GloryConst::transfer_log("%s bought %s for %d const glory",
						GET_NAME(ch), GET_OBJ_PNAME(proto, 0), price - removed);
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
		if (!check_money(ch, price, (*shop)->currency))
		{
			snprintf(buf, MAX_STRING_LENGTH,
				"Мошенни%s, ты можешь оплатить только %d.",
				IS_MALE(ch) ? "к" : "ца", bought);
		}
		else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		{
			snprintf(buf, MAX_STRING_LENGTH,
				"Ты сможешь унести только %d.", bought);
		}
		else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto) > CAN_CARRY_W(ch))
		{
			snprintf(buf, MAX_STRING_LENGTH,
				"Ты сможешь поднять только %d.", bought);
		}
		else
		{
			if (bought > 0)
				snprintf(buf, MAX_STRING_LENGTH,
					"Я продам тебе только %d.", bought);
			else
				snprintf(buf, MAX_STRING_LENGTH,
					"Я не продам тебе ничего.");
		}
		tell_to_char(keeper, ch, buf);
	}

	snprintf(buf, MAX_STRING_LENGTH,
		"Это будет стоить %d %s.", total_money,
		desc_count(total_money, (*shop)->currency == "куны"? WHAT_MONEYu : WHAT_GLORYu));
	tell_to_char(keeper, ch, buf);
	if (obj)
	{
		send_to_char(ch, "Теперь вы стали %s %s.\r\n",
			IS_MALE(ch) ? "счастливым обладателем" : "счастливой обладательницей",
			item_count_message(obj, bought, 1).c_str());
	}
}

long get_sell_price(OBJ_DATA * obj)
{
	long cost = GET_OBJ_COST(obj);
	cost = obj_proto[GET_OBJ_RNUM(obj)]->get_timer()<=0 ? 1 : cost * ((float)obj->get_timer() / (float)obj_proto[GET_OBJ_RNUM(obj)]->get_timer()); //учтем таймер

	return MMAX(1, cost);
}

void put_item_in_shop(ShopListType::const_iterator &shop, OBJ_DATA * obj)
{
	ItemListType::const_iterator it;
	for (it = (*shop)->item_list.begin(); it!=(*shop)->item_list.end(); ++it)
		if ((*it)->rnum == GET_OBJ_RNUM(obj))
		{
			if ((*it)->temporary_ids.empty())
			{
				extract_obj(obj);
				return;
			}
			else
			{
				OBJ_DATA * tmp_obj = get_obj_from_waste(shop, (*it)->temporary_ids);
				if (!tmp_obj) continue;
				if (GET_OBJ_TYPE(obj) != ITEM_MING || //а у них всех один рнум
					std::string(obj->short_description) == std::string(tmp_obj->short_description))
				{
					(*it)->temporary_ids.push_back(obj->uid);
					waste_node tmp_node;
					tmp_node.obj = obj;
					tmp_node.rnum = obj->item_number;
					(*shop)->waste.push_back(tmp_node);
					return;
				}
			}
		}

		ItemNodePtr tmp_item(new item_node);
		tmp_item->rnum = GET_OBJ_RNUM(obj);
		tmp_item->price = get_sell_price(obj);
		tmp_item->temporary_ids.push_back(obj->uid);
		(*shop)->item_list.push_back(tmp_item);
		waste_node tmp_node;
		tmp_node.rnum = obj->item_number;
		tmp_node.obj = obj;
		(*shop)->waste.push_back(tmp_node);
}


void do_shop_cmd(CHAR_DATA* ch, CHAR_DATA *keeper, OBJ_DATA* obj, ShopListType::const_iterator &shop, std::string cmd)
{
	if (!obj) return;
	int rnum = GET_OBJ_RNUM(obj);
	if (rnum < 0 || OBJ_FLAGGED(obj, ITEM_ARMORED) || OBJ_FLAGGED(obj, ITEM_SHARPEN) ||
		//OBJ_FLAGGED(obj, ITEM_NODONATE) || //Реально не пойму почему все торгаши так ненавидят стаф !пожертовать?!!!
		OBJ_FLAGGED(obj, ITEM_NODROP))
	{
		tell_to_char(keeper, ch, string("Я не собираюсь иметь дела с этой вещью.").c_str());
		return;
	}
	if ((GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF) && GET_OBJ_VAL(obj, 2) == 0)
	{
		tell_to_char(keeper, ch, "Я не покупаю использованные вещи!");
		return;
	}
	if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && cmd != "Чинить")
	{
		if (obj->contains)
		{
			tell_to_char(keeper, ch, string("Не надо предлагать мне кота в мешке.").c_str());
			return;
		}
	}
	long buy_price = GET_OBJ_COST(obj);

	buy_price = obj_proto[rnum]->get_timer()<=0 ? 1 : (long)buy_price*((float)obj->get_timer() / (float)obj_proto[rnum]->get_timer()); //учтем таймер

	buy_price = obj->obj_flags.Obj_max <=0 ? 1 : (long)buy_price*((float)obj->obj_flags.Obj_cur / (float)obj->obj_flags.Obj_max); //учтем повреждения

	int repair = GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj);
	int repair_price = MAX(1, GET_OBJ_COST(obj) * MAX(0, repair) / MAX(1, GET_OBJ_MAX(obj)));

	if (!can_use_feat(ch, SKILLED_TRADER_FEAT))
		 buy_price = MMAX(1, (buy_price * (*shop)->profit) / 100); //учтем прибыль магазина

	std::string price_to_show = boost::lexical_cast<string>(buy_price) + " " + string(desc_count(buy_price, WHAT_MONEYu));

	if (cmd == "Оценить")
	{
		if (OBJ_FLAGGED(obj, ITEM_NOSELL) || OBJ_FLAGGED(obj, ITEM_NAMED) || OBJ_FLAGGED(obj, ITEM_REPOP_DECAY))
		{
			tell_to_char(keeper, ch, string("Такое я не покупаю.").c_str());
			return;
		}else
			tell_to_char(keeper, ch, string("Я, пожалуй, куплю " + string(GET_OBJ_PNAME(obj, 3)) + " за " + price_to_show + ".").c_str());
	}

	if (cmd == "Продать")
	{
		if (OBJ_FLAGGED(obj, ITEM_NOSELL) || OBJ_FLAGGED(obj, ITEM_NAMED) || OBJ_FLAGGED(obj, ITEM_REPOP_DECAY))
		{
			tell_to_char(keeper, ch, string("Такое я не покупаю.").c_str());
			return;
		}else
		{
			obj_from_char(obj);
			tell_to_char(keeper, ch, string("Получи за " + string(GET_OBJ_PNAME(obj, 3)) + " " + price_to_show + ".").c_str());
			ch->add_gold(buy_price);
			put_item_in_shop(shop, obj);
		}
	}
	if (cmd == "Чинить")
	{
		if (repair <= 0)
		{
			tell_to_char(keeper, ch, string(string(GET_OBJ_PNAME(obj, 3))+" не нужно чинить.").c_str());
			return;
		}

		switch (obj->obj_flags.Obj_mater)
		{
		case MAT_BULAT:
		case MAT_CRYSTALL:
		case MAT_DIAMOND:
		case MAT_SWORDSSTEEL:
			repair_price *= 2;
			break;
		case MAT_SUPERWOOD:
		case MAT_COLOR:
		case MAT_GLASS:
		case MAT_BRONZE:
		case MAT_FARFOR:
		case MAT_BONE:
		case MAT_ORGANIC:
			repair_price += MAX(1, repair_price / 2);
			break;
		case MAT_IRON:
		case MAT_STEEL:
		case MAT_SKIN:
		case MAT_MATERIA:
			repair_price = repair_price;
			break;
		default:
			repair_price = repair_price;
		}

		if (repair_price <= 0 ||
				OBJ_FLAGGED(obj, ITEM_DECAY) ||
				OBJ_FLAGGED(obj, ITEM_NOSELL) || OBJ_FLAGGED(obj, ITEM_NODROP))
		{
			tell_to_char(keeper, ch, string("Я не буду тратить свое драгоценное время на " + string(GET_OBJ_PNAME(obj, 3))+".").c_str());
			return;
		}

		tell_to_char(keeper, ch, string("Починка " + string(GET_OBJ_PNAME(obj, 1)) + " обойдется в "
			+ boost::lexical_cast<string>(repair_price)+" " + desc_count(repair_price, WHAT_MONEYu)).c_str());

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

		GET_OBJ_CUR(obj) = GET_OBJ_MAX(obj);
	}
}

void process_cmd(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, ShopListType::const_iterator &shop, std::string cmd)
{
	OBJ_DATA *obj;
	OBJ_DATA * obj_next;
	std::string buffer(argument), buffer1;
	GetOneParam(buffer, buffer1);
	boost::trim(buffer);

	if ((cmd == "Продать" || cmd == "Оценить") && !(*shop)->can_buy)
	{
		tell_to_char(keeper, ch, "Извини, у меня свои поставщики...");
		return;
	}

	if (!*argument)
	{
		tell_to_char(keeper, ch, string(cmd + " ЧТО?").c_str());
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
		} catch(boost::bad_lexical_cast const& e)
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
				obj_next = get_obj_in_list_vis(ch, buffer, obj->next_content);
				do_shop_cmd(ch, keeper, obj, shop, cmd);
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
								for(i = 0; i < NUM_WEARS; i++)
									if (ch->equipment[i])
										do_shop_cmd(ch, keeper, ch->equipment[i], shop, cmd);
								return;
							}
							send_to_char("У вас нет " + buffer2 + "!\r\n", ch);
							return;
						}

						do_shop_cmd(ch, keeper, obj, shop, cmd);
						break;
				case FIND_ALL:
						for (obj = ch->carrying; obj; obj = obj_next)
						{
							obj_next = obj->next_content;
							do_shop_cmd(ch, keeper, obj, shop, cmd);
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
							obj_next = get_obj_in_list_vis(ch, buffer2, obj->next_content);
							do_shop_cmd(ch, keeper, obj, shop, cmd);
							obj = obj_next;
						}
						break;
				default:
					break;
			};
		}
	}
}

void process_ident(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, ShopListType::const_iterator &shop, std::string cmd)
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
		} catch(boost::bad_lexical_cast const& e)
		{
		}
	}
	else
	{
		// характеристики меч
		item_num = get_item_num(shop, buffer, GET_MOB_VNUM(keeper));
	}

	if (!item_num || item_num > (*shop)->item_list.size())
	{
		tell_to_char(keeper, ch, "Протри глаза, у меня нет такой вещи.");
		return;
	}

	--item_num;

	const OBJ_DATA *ident_obj = NULL;
	OBJ_DATA *tmp_obj = NULL;
	if ((*shop)->item_list[item_num]->temporary_ids.empty())
	{
		if (!(*shop)->item_list[item_num]->descs.empty() &&
			(*shop)->item_list[item_num]->descs.find(GET_MOB_VNUM(keeper)) != (*shop)->item_list[item_num]->descs.end())
		{
			tmp_obj = read_object((*shop)->item_list[item_num]->rnum, REAL);
			replace_descs(tmp_obj, (*shop)->item_list[item_num], GET_MOB_VNUM(keeper));
			ident_obj = tmp_obj;
		}
		else
			ident_obj = read_object_mirror((*shop)->item_list[item_num]->rnum, REAL);
	}
	else
		ident_obj = get_obj_from_waste(shop, (*shop)->item_list[item_num]->temporary_ids);

	if (!ident_obj)
	{
		log("SYSERROR : не удалось получить объект (%s:%d)", __FILE__, __LINE__);
		send_to_char("Ошибочка вышла.\r\n", ch);
		return;
	}
	if (cmd == "Рассмотреть")
	{
		std::string tell = "Предмет "+ std::string(ident_obj->short_description)+": ";
		tell += std::string(item_types[GET_OBJ_TYPE(ident_obj)])+"\r\n";
		tell += std::string(diag_weapon_to_char(ident_obj, TRUE));
		tell += std::string(diag_timer_to_char(ident_obj));
		if (can_use_feat(ch, SKILLED_TRADER_FEAT) || PRF_FLAGGED(ch, PRF_HOLYLIGHT))
		{
			sprintf(buf, "Материал : ");
			sprinttype(ident_obj->obj_flags.Obj_mater, material_name, buf+strlen(buf));
			sprintf(buf+strlen(buf), ".\r\n");
			tell += std::string(buf);
		}
		tell_to_char(keeper, ch, tell.c_str());
		if (invalid_anti_class(ch, ident_obj) || invalid_unique(ch, ident_obj) || NamedStuff::check_named(ch, ident_obj, 0))
		{
			tell = "Но лучше бы тебе не заглядываться на эту вещь, не унесешь все равно.";
			tell_to_char(keeper, ch, tell.c_str());
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

			send_to_char(ch, "Характеристики предмета: %s\r\n", GET_OBJ_PNAME(ident_obj, 0));
			mort_show_obj_values(ident_obj, ch, 200);
			ch->remove_gold(IDENTIFY_COST);
		}
	}
	if (tmp_obj)
		extract_obj(tmp_obj);
}

int get_spent_today()
{
	return spent_today;
}

void renumber_obj_rnum(int rnum)
{
	for (ShopListType::iterator i = shop_list.begin(); i != shop_list.end(); ++i)
	{
		for (ItemListType::iterator k = (*i)->item_list.begin(); k != (*i)->item_list.end(); ++k)
		{
			if ((*k)->rnum >= rnum)
			{
				(*k)->rnum += 1;
			}
		}
	}
}

} // namespace ShopExt

using namespace ShopExt;

SPECIAL(shop_ext)
{
	if (!ch->desc || IS_NPC(ch))
	{
		return 0;
	}
	if (!(CMD_IS("список") || CMD_IS("list")
		|| CMD_IS("купить") || CMD_IS("buy")
		|| CMD_IS("рассмотреть") || CMD_IS("examine")
		|| CMD_IS("характеристики") || CMD_IS("identify")
		|| CMD_IS("оценить") || CMD_IS("value")
		|| CMD_IS("продать") || CMD_IS("sell")
		|| CMD_IS("чинить") || CMD_IS("repair")
		|| CMD_IS("steal") || CMD_IS("украсть")))
	{
		return 0;
	}
	char argm[MAX_INPUT_LENGTH];
	CHAR_DATA * const keeper = reinterpret_cast<CHAR_DATA *>(me);
	ShopListType::const_iterator shop;
	for (shop = shop_list.begin(); shop != shop_list.end(); ++shop)
	{
		if (std::find((*shop)->mob_vnums.begin(), (*shop)->mob_vnums.end(), GET_MOB_VNUM(keeper)) != (*shop)->mob_vnums.end())
			break;
		if (shop_list.end() == shop)
		{
			log("SYSERROR : магазин не найден mob_vnum=%d (%s:%d)",
				GET_MOB_VNUM(keeper), __FILE__, __LINE__);
			send_to_char("Ошибочка вышла.\r\n", ch);
			return 1;
		}
	}

	if (CMD_IS("steal") || CMD_IS("украсть"))
	{
		sprintf(argm, "$N вскричал$g '%s'", MSG_NO_STEAL_HERE);
		sprintf(buf, "ругать %s", GET_NAME(ch));
		do_social(keeper, buf);
		act(argm, FALSE, ch, 0, keeper, TO_CHAR);
		return 1;
	}

	if (CMD_IS("список") || CMD_IS("list"))
	{
		std::string buffer = argument, buffer2;
		GetOneParam(buffer, buffer2);
		print_shop_list(ch, shop, buffer2, GET_MOB_VNUM(keeper));
		return 1;
	}
	if (CMD_IS("купить") || CMD_IS("buy"))
	{
		process_buy(ch, keeper, argument, shop);
		return 1;
	}
	if (CMD_IS("характеристики") || CMD_IS("identify"))
	{
		process_ident(ch, keeper, argument, shop, "Характеристики");
		return 1;
	}


	if (CMD_IS("value") || CMD_IS("оценить"))
	{
		process_cmd(ch, keeper, argument, shop, "Оценить");
		return 1;
	}
	if (CMD_IS("продать") || CMD_IS("sell"))
	{
		process_cmd(ch, keeper, argument, shop, "Продать");
		return 1;
	}
	if (CMD_IS("чинить") || CMD_IS("repair"))
	{
		process_cmd(ch, keeper, argument, shop, "Чинить");
		return 1;
	}
	if (CMD_IS("рассмотреть") || CMD_IS("examine"))
	{
		process_ident(ch, keeper, argument, shop, "Рассмотреть");
		return 1;
	}

	return 0;
}

// * Лоад странствующих продавцов в каждой ренте.
void town_shop_keepers()
{
	// список уже оработанных зон, чтобы не грузить двух и более торгашей в одну
	std::vector<int> zone_list;

	for (CHAR_DATA *ch = character_list; ch; ch = ch->next)
	{
		if (IS_RENTKEEPER(ch) && IN_ROOM(ch) > 0
			&& Clan::IsClanRoom(IN_ROOM(ch)) == Clan::ClanList.end()
			&& !ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF)
			&& GET_ROOM_VNUM(IN_ROOM(ch)) % 100 != 99
			&& std::find(zone_list.begin(), zone_list.end(), world[IN_ROOM(ch)]->zone) == zone_list.end())
		{
			int rnum_start, rnum_end;
			if (get_zone_rooms(world[IN_ROOM(ch)]->zone, &rnum_start, &rnum_end))
			{
				CHAR_DATA *mob = read_mobile(1901, VIRTUAL);
				if (mob)
				{
					char_to_room(mob, number(rnum_start, rnum_end));
				}
			}
			zone_list.push_back(world[IN_ROOM(ch)]->zone);
		}
	}
}

ACMD(do_shops_list)
{
	DictionaryPtr dic = DictionaryPtr(new Dictionary(SHOP));
	int n = dic->Size();
	std::string out = std::string();
	for (int i =0; i< n; i++)
		out = out + boost::lexical_cast<string>(i+1)+" " + dic->GetNameByNID(i) + " " + dic->GetTIDByNID(i) + "\r\n";
	send_to_char(out, ch);
}

void fill_shop_dictionary(DictionaryType &dic)
{
	ShopExt::ShopListType::const_iterator it = ShopExt::shop_list.begin();
	for (;it != ShopExt::shop_list.end();++it)
		dic.push_back((*it)->GetDictionaryItem());
};
