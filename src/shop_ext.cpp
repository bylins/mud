// $RCSfile: shop_ext.cpp,v $     $Date: 2012/01/27 05:36:40 $     $Revision: 1.16 $
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#include <vector>
#include <string>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "pugixml.hpp"

#include "shop_ext.hpp"
#include "char.hpp"
#include "db.h"
#include "comm.h"
#include "shop.h"
#include "handler.h"
#include "constants.h"
#include "room.hpp"
#include "glory.hpp"
#include "glory_const.hpp"
#include "screen.h"
#include "house.h"

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

namespace ShopExt
{

const int IDENTIFY_COST = 110;
int spent_today = 0;

struct item_node
{
	item_node() : rnum(0), price(0), timer(-1), temporary_id(0) {};

	int rnum;
	int price;
	int timer;
	unsigned temporary_id;
};

typedef boost::shared_ptr<item_node> ItemNodePtr;
typedef std::vector<ItemNodePtr> ItemListType;

struct shop_node
{
	shop_node() : waste_time_min(0) {};

	std::list<int> mob_vnums;
	std::string currency;
	std::string id;
	ItemListType item_list;
	int profit;
	std::list<OBJ_DATA *> waste;
	int waste_time_min;
};

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

typedef boost::shared_ptr<shop_node> ShopNodePtr;
typedef std::vector<ShopNodePtr> ShopListType;
typedef boost::shared_ptr<item_set> ItemSetPtr;
typedef std::vector<ItemSetPtr> ItemSetListType;
ShopListType shop_list;

void log_shop_load()
{
	for (ShopListType::iterator i = shop_list.begin(); i != shop_list.end(); ++i)
	{
		for (std::list<int>::const_iterator it = (*i)->mob_vnums.begin(); it != (*i)->mob_vnums.end(); ++it)
			log("ShopExt: mob=%d",*it);
		log("ShopExt: currency=%s", (*i)->currency.c_str());
		for (ItemListType::iterator k = (*i)->item_list.begin(); k != (*i)->item_list.end(); ++k)
		{
			log("ItemList: vnum=%d, price=%d", GET_OBJ_VNUM(obj_proto[(*k)->rnum]), (*k)->price);
		}
	}
}

void load()
{
	shop_list.clear();

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
			int item_vnum = xmlparse_int(item, "vnum");
			int price = xmlparse_int(item, "price");
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
		// иним сам магазин
		ShopNodePtr tmp_shop(new shop_node);
		tmp_shop->id = shop_id;
		tmp_shop->currency = currency;
		tmp_shop->profit = profit;

		for (pugi::xml_node mob = node.child("mob"); mob; mob=mob.next_sibling("mob"))
		{
			int mob_vnum = xmlparse_int(mob, "mob_vnum");
			if (mob_vnum < 0)
			{
				snprintf(buf, MAX_STRING_LENGTH,
					"...bad shop attributes (mob_vnum=%d shop id=%s)", mob_vnum, shop_id.c_str());
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}
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
					return;
				}
				mob_index[mob_rnum].func = shop_ext;
			}
			else
			{
				snprintf(buf, MAX_STRING_LENGTH, "...incorrect mob_vnum=%d", mob_vnum);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				return;
			}
		}
		// и список его продукции
		for (pugi::xml_node item = node.child("item"); item; item = item.next_sibling("item"))
		{
			int item_vnum = xmlparse_int(item, "vnum");
			int price = xmlparse_int(item, "price");
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
		shop_list.push_back(tmp_shop);
    }
    log_shop_load();
}

unsigned get_item_num(ShopListType::const_iterator &shop, std::string &item_name)
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
				num = boost::lexical_cast<int>(first_param);
			}
		}
	}

	int count = 0;
	for (unsigned i = 0; i < (*shop)->item_list.size(); ++i)
	{
		if (isname(item_name, GET_OBJ_PNAME(obj_proto[(*shop)->item_list[i]->rnum], 0)))
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

bool check_money(CHAR_DATA *ch, int price, std::string currency)
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
	if ((*shop)->item_list[item_num]->temporary_id != 0)
		return 1;
	else
	{
		int numToSell = obj_proto[(*shop)->item_list[item_num]->rnum]->max_in_world;
		if (numToSell != -1) 
			numToSell -= obj_index[(*shop)->item_list[item_num]->rnum].number;
		return numToSell;
	}
}

OBJ_DATA * get_obj_from_waste(ShopListType::const_iterator &shop, unsigned uid)
{
	std::list<OBJ_DATA *>::const_iterator it;
	for (it = (*shop)->waste.begin(); it != (*shop)->waste.end(); ++it)
	{
		if ((*it)->uid == uid)
			return (*it);
	}
	return 0;
}

void print_shop_list(CHAR_DATA *ch, ShopListType::const_iterator &shop, std::string arg)
{
	send_to_char(ch,
		" ##    Доступно   Предмет                                      Цена (%s)\r\n"
		"---------------------------------------------------------------------------\r\n",
		(*shop)->currency.c_str());
	int num = 1;
	std::string out;
	for (ItemListType::const_iterator k = (*shop)->item_list.begin(),
		kend = (*shop)->item_list.end(); k != kend; ++k)
	{
		int count = can_sell_count(shop, num - 1);
		
		std::string print_value="";

//Polud у проданных в магаз объектов отображаем в списке не значение из прототипа, а уже, возможно, измененное значение
// чтобы не было в списках всяких "гриб @n1"
		if ((*k)->temporary_id == 0)
			print_value = GET_OBJ_PNAME(obj_proto[(*k)->rnum], 0);
		else
		{
			OBJ_DATA * tmp_obj = get_obj_from_waste(shop, (*k)->temporary_id);
			if (tmp_obj)
				print_value = std::string(tmp_obj->short_description);
		}

		std::string numToShow = count == -1 ? "Навалом" : boost::lexical_cast<string>(count);
		
		if (arg.empty() || isname(arg.c_str(), GET_OBJ_PNAME(obj_proto[(*k)->rnum], 0)))
				out += boost::str(boost::format("%3d)  %10s  %-47s %8d\r\n")
					% num++ % numToShow % print_value % (*k)->price);
			else
				num++;
	}
	send_to_char(out, ch);
//	send_to_char("В корзине " + boost::lexical_cast<string>((*shop)->waste.size()) + " элементов\r\n", ch);
}

/**
 * Симуляция телла продавца.
 */
void tell_to_char(CHAR_DATA *keeper, CHAR_DATA *ch, const char *arg)
{
	char local_buf[MAX_INPUT_LENGTH];
	snprintf(local_buf, MAX_INPUT_LENGTH,
		"%s сказал%s Вам : '%s'", GET_NAME(keeper), GET_CH_SUF_1(keeper), arg);
	send_to_char(ch, "%s%s%s\r\n",
		CCICYN(ch, C_NRM), CAP(local_buf), CCNRM(ch, C_NRM));
}

void remove_from_waste(ShopListType::const_iterator &shop, OBJ_DATA *obj)
{
	std::list<OBJ_DATA *>::iterator it;
	for (it = (*shop)->waste.begin(); it != (*shop)->waste.end(); ++it)
	{
		if ((*it) == obj)
		{
			(*shop)->waste.erase(it);
			return;
		}
	}
}

void empty_waste(ShopListType::const_iterator &shop)
{
	std::list<OBJ_DATA *>::const_iterator it;
	for (it = (*shop)->waste.begin(); it != (*shop)->waste.end(); ++it)
	{
		extract_obj((*it));
	}
	(*shop)->waste.clear();
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
			item_num = boost::lexical_cast<unsigned>(buffer1);
		}
		else
		{
			// buy sword
			item_num = get_item_num(shop, buffer1);
		}
	}
	else if (is_number(buffer1.c_str()))
	{
		if (is_number(buffer2.c_str()))
		{
			// buy 5 10
			item_num = boost::lexical_cast<unsigned>(buffer2);
		}
		else
		{
			// buy 5 sword
			item_num = get_item_num(shop, buffer2);
		}
		item_count = boost::lexical_cast<unsigned>(buffer1);
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

	--item_num;
	const OBJ_DATA * proto = read_object_mirror((*shop)->item_list[item_num]->rnum, REAL);
	if (!proto)
	{
		log("SYSERROR : не удалось прочитать прототип (%s:%d)", __FILE__, __LINE__);
		send_to_char("Ошибочка вышла.\r\n", ch);
		return;
	}

	const int price = (*shop)->item_list[item_num]->price;

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
	if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)))
	{
		snprintf(buf, MAX_STRING_LENGTH,
			"%s, я понимаю, своя ноша карман не тянет,\r\n"
			"но %s Вам явно некуда положить.\r\n", GET_NAME(ch), OBJN(proto, ch, 3));
		send_to_char(buf, ch);
		return;
	}
	if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto)) > CAN_CARRY_W(ch))
	{
		snprintf(buf, MAX_STRING_LENGTH,
			"%s, я понимаю, своя ноша карман не тянет,\r\n"
			"но %s Вам явно не поднять.\r\n", GET_NAME(ch), OBJN(proto, ch, 3));
		send_to_char(buf, ch);
		return;
	}
	
	int bought = 0;
	int total_money = 0;


	OBJ_DATA *obj = NULL;
	while (bought < item_count
		&& check_money(ch, price, (*shop)->currency)
		&& IS_CARRYING_N(ch) < CAN_CARRY_N(ch)
		&& IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto) <= CAN_CARRY_W(ch)
		&& (bought < can_sell_count(shop, item_num) || can_sell_count(shop, item_num) == -1))
	{

		if ((*shop)->item_list[item_num]->temporary_id != 0)
		{
			obj = get_obj_from_waste(shop, (*shop)->item_list[item_num]->temporary_id);
			(*shop)->item_list.erase((*shop)->item_list.begin() + item_num);
			remove_from_waste(shop, obj);
		}
		else
			obj = read_object((*shop)->item_list[item_num]->rnum, REAL);
		
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
			snprintf(buf, MAX_STRING_LENGTH,
				"Я продам тебе только %d.", bought);
		}
		tell_to_char(keeper, ch, buf);
	}

	snprintf(buf, MAX_STRING_LENGTH,
		"Это будет стоить %d %s.", total_money,
		desc_count(total_money, (*shop)->currency == "куны"? WHAT_MONEYu : WHAT_GLORYu));
	tell_to_char(keeper, ch, buf);
	send_to_char(ch, "Теперь Вы стали %s %s.\r\n",
		IS_MALE(ch) ? "счастливым обладателем" : "счастливой обладательницей",
		item_count_message(obj, bought, 1).c_str());
}

void do_shop_cmd(CHAR_DATA* ch, CHAR_DATA *keeper, OBJ_DATA* obj, ShopListType::const_iterator &shop, std::string cmd)
{
	if (!obj) return;
	int rnum = GET_OBJ_RNUM(obj);
	if (rnum < 0) return;
	if (OBJ_FLAGGED(obj, ITEM_ARMORED) || OBJ_FLAGGED(obj, ITEM_SHARPEN) ||
		OBJ_FLAGGED(obj, ITEM_NODONATE) || OBJ_FLAGGED(obj, ITEM_NODROP) ||
		OBJ_FLAGGED(obj, ITEM_NOSELL))
	{
		tell_to_char(keeper, ch, string("Я не собираюсь иметь дела с этой вещью").c_str());
		return;
	}
	
	int buy_price = GET_OBJ_COST(obj);
	
	buy_price = (buy_price * obj->get_timer()) / obj_proto[rnum]->get_timer(); //учтем таймер
	
	int new_sell_price = MMAX(1, buy_price);

	buy_price = (buy_price * obj->obj_flags.Obj_cur) / obj->obj_flags.Obj_max; //учтем повреждения

	int repair = GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj);
	int repair_price = MAX(1, GET_OBJ_COST(obj) * MAX(0, repair) / MAX(1, GET_OBJ_MAX(obj)));

	if (ch->get_class()	!= CLASS_MERCHANT) 
		 buy_price = MMAX(1, (buy_price * (*shop)->profit) / 100); //учтем прибыль магазина
	
	std::string price_to_show = boost::lexical_cast<string>(buy_price) + " " + string(desc_count(buy_price, WHAT_MONEYu));

	if (cmd == "Оценить")
			tell_to_char(keeper, ch, string("Я, пожалуй, куплю " + string(GET_OBJ_PNAME(obj, 3)) + " за " + price_to_show + ".").c_str());

	if (cmd == "Продать")
	{
		obj_from_char(obj);

		ItemNodePtr tmp_item(new item_node);
		tmp_item->rnum = rnum;
		tmp_item->price = new_sell_price;
		tmp_item->timer = obj->get_timer();
		tmp_item->temporary_id = obj->uid;
		(*shop)->item_list.push_back(tmp_item);
		tell_to_char(keeper, ch, string("Получи за " + string(GET_OBJ_PNAME(obj, 3)) + " " + price_to_show + ".").c_str());
		ch->add_gold(buy_price);

		(*shop)->waste.push_back(obj);
	}
	if (cmd == "Чинить")
	{
		if (repair <= 0)
		{
			tell_to_char(keeper, ch, string(string(GET_OBJ_PNAME(obj, 2))+" не нужно чинить.").c_str());
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
			tell_to_char(keeper, ch, string("Я не буду тратить свое драгоценное время на " + string(GET_OBJ_PNAME(obj, 2))+".").c_str());
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

	if (!*argument)
	{
		tell_to_char(keeper, ch, string(cmd + " ЧТО?").c_str());
		return;
	}

	skip_spaces(&argument);
	int i, dotmode = find_all_dots(argument);
	std::string buffer(argument);
	OBJ_DATA * obj_next;
	switch (dotmode)
	{
		case FIND_INDIV:

				obj = get_obj_in_list_vis(ch, buffer, ch->carrying);

				if (!obj)
				{
					if (cmd == "Чинить" && is_abbrev(argument, "экипировка"))
					{
						for(i = 0; i < NUM_WEARS; i++)
							if (ch->equipment[i]) 
								do_shop_cmd(ch, keeper, ch->equipment[i], shop, cmd);
						return;
					}
					send_to_char("У Вас нет " + buffer + "!\r\n", ch);
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
				obj = ch->carrying;
				while (obj)
				{
					obj_next = obj->next_content;
					obj = get_obj_in_list_vis(ch, buffer, obj);
					do_shop_cmd(ch, keeper, obj, shop, cmd);
					obj = obj_next;
				}
				break;
		default:
			break;
	};

}

void process_ident(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, ShopListType::const_iterator &shop)
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
		item_num = boost::lexical_cast<unsigned>(buffer);
	}
	else
	{
		// характеристики меч
		item_num = get_item_num(shop, buffer);
	}

	if (!item_num || item_num > (*shop)->item_list.size())
	{
		tell_to_char(keeper, ch, "Протри глаза, у меня нет такой вещи.");
		return;
	}

	--item_num;

	const OBJ_DATA * const proto = read_object_mirror((*shop)->item_list[item_num]->rnum, REAL);
	if (!proto)
	{
		log("SYSERROR : не удалось прочитать прототип (%s:%d)", __FILE__, __LINE__);
		send_to_char("Ошибочка вышла.\r\n", ch);
		return;
	}

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
		return;
	}

	snprintf(buf, MAX_STRING_LENGTH,
		"Эта услуга будет стоить %d %s.", IDENTIFY_COST,
		desc_count(IDENTIFY_COST, WHAT_MONEYu));
	tell_to_char(keeper, ch, buf);

	send_to_char(ch, "Характеристики предмета: %s\r\n", GET_OBJ_PNAME(proto, 0));
	mort_show_obj_values(proto, ch, 200);
	ch->remove_gold(IDENTIFY_COST);
}

SPECIAL(shop_ext)
{
	if (!ch->desc || IS_NPC(ch))
	{
		return 0;
	}
	if (!(CMD_IS("список") || CMD_IS("list")
		|| CMD_IS("купить") || CMD_IS("buy")
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
		print_shop_list(ch, shop, buffer2);
		return 1;
	}
	if (CMD_IS("купить") || CMD_IS("buy"))
	{
		process_buy(ch, keeper, argument, shop);
		return 1;
	}
	if (CMD_IS("характеристики") || CMD_IS("identify"))
	{
		process_ident(ch, keeper, argument, shop);
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

	return 0;
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

/**
 * Лоад странствующих продавцов в каждой ренте.
 */
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
