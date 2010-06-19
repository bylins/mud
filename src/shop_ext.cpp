// $RCSfile$     $Date$     $Revision$
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

/*
Пример конфига (plrstuff/shop/test.xml):
<?xml version="1.0"?>
<shop_list>
	<shop mob_vnum="4015" type="1">
		<item vnum="4000" price="1" />
		<item vnum="4001" price="2" />
		<item vnum="4002" price="1000" />
		<item vnum="4003" price="10000" />
		<item vnum="4004" price="100000" />
		<item vnum="4005" price="99999" />
		<item vnum="4006" price="9999" />
		<item vnum="4007" price="999" />
	</shop>
</shop_list>
*/

extern ACMD(do_echo);
extern int do_social(CHAR_DATA * ch, char *argument);
extern void mort_show_obj_values(const OBJ_DATA * obj, CHAR_DATA * ch, int fullness);

namespace ShopExt
{

enum { SIMPLE_SHOP = 0, GLORY_SHOP, TOTAL_SHOP_TYPES };

const int IDENTIFY_COST = 110;
int spent_today = 0;

struct item_node
{
	item_node() : rnum(0), price(0) {};

	int rnum;
	int price;
};

typedef boost::shared_ptr<item_node> ItemNodePtr;
typedef std::vector<ItemNodePtr> ItemListType;

struct shop_node
{
	shop_node() : mob_vnum(0), type(0) {};

	int mob_vnum;
	int type;
	ItemListType item_list;
};

typedef boost::shared_ptr<shop_node> ShopNodePtr;
typedef std::vector<ShopNodePtr> ShopListType;
ShopListType shop_list;

int parse_shop_attribute(pugi::xml_node &node, const char *text)
{
	int result = -1;

	pugi::xml_attribute attr = node.attribute(text);
	if (!attr)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s read fail", text);
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
	}
	else
	{
		try
		{
			result = boost::lexical_cast<int>(attr.value());
		}
		catch(...)
		{
			snprintf(buf, MAX_STRING_LENGTH, "...%s lexical_cast fail", text);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		}
	}
	return result;
}

void load()
{
	shop_list.clear();

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(LIB_PLRSTUFF"/shop/test.xml");
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
	for (pugi::xml_node node = node_list.child("shop"); node; node = node.next_sibling("shop"))
	{
		int mob_vnum = parse_shop_attribute(node, "mob_vnum");
		int shop_type = parse_shop_attribute(node, "type");
		if (mob_vnum < 0 || shop_type < 0 || shop_type >= TOTAL_SHOP_TYPES)
		{
			snprintf(buf, MAX_STRING_LENGTH,
				"...bad shop attributes (mob_vnum=%d, type=%d)", mob_vnum, shop_type);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}
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
		// иним сам магазин
		ShopNodePtr tmp_shop(new shop_node);
		tmp_shop->mob_vnum = mob_vnum;
		tmp_shop->type = shop_type;
		// и список его продукции
		for (pugi::xml_node item = node.child("item"); item; item = item.next_sibling("item"))
		{
			int item_vnum = parse_shop_attribute(item, "vnum");
			int price = parse_shop_attribute(item, "price");
			if (item_vnum < 0 || price <= 0)
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
			tmp_item->price = price;
			tmp_shop->item_list.push_back(tmp_item);
		}
		if (tmp_shop->item_list.empty())
		{
			snprintf(buf, MAX_STRING_LENGTH, "...item list empty (mob_vnum=%d)", mob_vnum);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}
		shop_list.push_back(tmp_shop);
    }
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

bool check_money(CHAR_DATA *ch, int price, int type)
{
	if (GLORY_SHOP == type)
	{
		return Glory::get_glory(GET_UNIQUE(ch))
			+ GloryConst::get_glory(GET_UNIQUE(ch)) >= price;
	}

	return ch->get_gold() >= price;
}

void print_shop_list(CHAR_DATA *ch, ShopListType::const_iterator &shop)
{
	send_to_char(ch,
		" ##    Доступно   Предмет                                      Цена (%s)\r\n"
		"---------------------------------------------------------------------------\r\n",
		GLORY_SHOP == (*shop)->type ? "слава" : "куны");
	int num = 1;
	std::string out;
	for (ItemListType::const_iterator k = (*shop)->item_list.begin(),
		kend = (*shop)->item_list.end(); k != kend; ++k)
	{
		out += boost::str(boost::format("%3d)   Навалом    %-47s %8d\r\n")
			% num++ % GET_OBJ_PNAME(obj_proto[(*k)->rnum], 0) % (*k)->price);
	}
	send_to_char(out, ch);
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

	unsigned item_num = 0, item_count = 1;

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

	if (!item_count || !item_num || item_num > (*shop)->item_list.size())
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

	const int price = (*shop)->item_list[item_num]->price;

	if (!check_money(ch, price, (*shop)->type))
	{
		snprintf(buf, MAX_STRING_LENGTH,
			"У вас нет столько %s!", SIMPLE_SHOP == (*shop)->type ? "денег" : "славы");
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

	unsigned bought = 0;
	int total_money = 0;
	while (bought < item_count
		&& check_money(ch, price, (*shop)->type)
		&& IS_CARRYING_N(ch) < CAN_CARRY_N(ch)
		&& IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(proto) <= CAN_CARRY_W(ch))
	{
		OBJ_DATA *obj = read_object((*shop)->item_list[item_num]->rnum, REAL);
		if (obj)
		{
			obj_to_char(obj, ch);
			if (GLORY_SHOP == (*shop)->type)
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
		if (!check_money(ch, price, (*shop)->type))
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
		desc_count(total_money, SIMPLE_SHOP == (*shop)->type ? WHAT_MONEYu : WHAT_GLORYu));
	tell_to_char(keeper, ch, buf);
	send_to_char(ch, "Теперь Вы стали %s %s.\r\n",
		IS_MALE(ch) ? "счастливым обладателем" : "счастливой обладательницей",
		item_count_message(proto, bought, 1).c_str());
}

void process_ident(CHAR_DATA *ch, CHAR_DATA *keeper, char *argument, ShopListType::const_iterator &shop)
{
	std::string buffer(argument);
	boost::trim(buffer);

	if (buffer.empty())
	{
		tell_to_char(keeper, ch, "ЧТО ты хочешь опознать?");
		return;
	}

	unsigned item_num = 0;
	if (is_number(buffer.c_str()))
	{
		// опознать 5
		item_num = boost::lexical_cast<unsigned>(buffer);
	}
	else
	{
		// опознать меч
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
		|| CMD_IS("опознать") || CMD_IS("identify")))
	{
		return 0;
	}

	CHAR_DATA * const keeper = reinterpret_cast<CHAR_DATA *>(me);
	ShopListType::const_iterator shop = std::find_if(shop_list.begin(), shop_list.end(),
		boost::bind(std::equal_to<int>(),
		boost::bind(&shop_node::mob_vnum, _1), GET_MOB_VNUM(keeper)));

	if (shop_list.end() == shop)
	{
		log("SYSERROR : магазин не найден mob_vnum=%d (%s:%d)",
			GET_MOB_VNUM(keeper), __FILE__, __LINE__);
		send_to_char("Ошибочка вышла.\r\n", ch);
		return 1;
	}

	if (CMD_IS("список") || CMD_IS("list"))
	{
		print_shop_list(ch, shop);
		return 1;
	}
	if (CMD_IS("купить") || CMD_IS("buy"))
	{
		process_buy(ch, keeper, argument, shop);
		return 1;
	}
	if (CMD_IS("опознать") || CMD_IS("identify"))
	{
		process_ident(ch, keeper, argument, shop);
		return 1;
	}

	return 0;
}

int get_spent_today()
{
	return spent_today;
}

} // namespace ShopExt
