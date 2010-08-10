// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 WorM
// Part of Bylins http://www.mud.ru

#include <list>
#include <map>
#include <string>
#include <iomanip>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/array.hpp>
#include "pugixml.hpp"
#include <boost/algorithm/string.hpp>

#include "named_stuff.hpp"
#include "structs.h"
#include "screen.h"
#include "char.hpp"
#include "comm.h"
#include "db.h"
#include "genchar.h"
#include "handler.h"
#include "house.h"
#include "char_player.hpp"
#include "dg_scripts.h"

namespace NamedStuff
{

StuffListType stuff_list;

/*void save()
{
	pugi::xml_document doc;
	doc.append_child().set_name("named_stuff_list");
	pugi::xml_node obj_list = doc.child("named_stuff_list");

	for (StuffListType::const_iterator i = stuff_list.begin(), iend = stuff_list.end(); i != iend; ++i)
	{
		pugi::xml_node stuf_node = obj_list.append_child();
		stuf_node.set_name("obj");
		stuf_node.append_attribute("vnum") = i->first;
		stuf_node.append_attribute("uid") = i->second->uid;
		stuf_node.append_attribute("mail") = i->second->mail.c_str();
		if(i->second->can_clan)
			stuf_node.append_attribute("can_clan") = i->second->can_clan;
		if(i->second->can_alli)
			stuf_node.append_attribute("can_alli") = i->second->can_alli;
	}

	doc.save_file(LIB_PLRSTUFF"named_stuff_list.xml");
}*/

bool check_named(CHAR_DATA * ch, OBJ_DATA * obj, const bool simple)
{
	StuffListType::iterator it = stuff_list.find(GET_OBJ_VNUM(obj));
	if (it != stuff_list.end())
	{
		if(IS_NPC(ch))
			return true;
		if(it->second->uid==GET_UNIQUE(ch))//Это владелец предмета
			return false;
		else if(!strcmp(GET_EMAIL(ch), it->second->mail.c_str()))//Это владелец предмета судя по мылу
			return false;
		if(!simple && CLAN(ch))//Предмет доступен сокланам иди альянсу
		{
			if((it->second->can_clan) && (CLAN(ch)->is_clan_member(it->second->uid)))//Это соклановец и предмет доступен соклановцам
				return false;
			if((it->second->can_alli) && (CLAN(ch)->is_alli_member(it->second->uid)))//Предмет доступен альянсу и это чар из альянса
				return false;
		}
		return true;
	}
	else
		return false;
}

void receive_items(CHAR_DATA * ch, CHAR_DATA * mailman)
{
	OBJ_DATA *obj;
	mob_rnum r_num;
	int found = 0;
	for (StuffListType::const_iterator it = stuff_list.begin(), iend = stuff_list.end(); it != iend; ++it)
	{
		if((it->second->uid==GET_UNIQUE(ch)) || (!strcmp(GET_EMAIL(ch), it->second->mail.c_str())))
		{
			if ((r_num = real_object(it->first)) < 0)
			{
				send_to_char("Странно но такого объекта не существует.\r\n", ch);
				continue;
			}
			obj = read_object(r_num, REAL);
			if((GET_OBJ_MIW(obj) >= obj_index[GET_OBJ_RNUM(obj)].stored + obj_index[GET_OBJ_RNUM(obj)].number) ||//Проверка на макс в мире
			  (GET_OBJ_MIW(obj) >= 1))//Пока что если в мире нету то тоже загрузить
			{
				found++;
				SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NAMED), ITEM_NAMED);
				obj_to_char(obj, ch);
				free_script(SCRIPT(obj));//детачим все триги чтоб не обламывать соклановцев и т.п.
				SCRIPT(obj) = NULL;
                load_otrigger(obj);
				obj_decay(obj);
				snprintf(buf, MAX_STRING_LENGTH,
					"NamedStuff: %s recieves named obj %s",
					GET_NAME(ch), obj->name);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);

				act("$n дал$g Вам $o3.", FALSE, mailman, obj, ch, TO_VICT);
				act("$N дал$G $n2 $o3.", FALSE, ch, obj, mailman, TO_ROOM);
			}
			else
			{
				extract_obj(obj);
			}
		}
	}
	if(!found) {
		act("$n сказал$g Вам : 'Кажется для тебя ничего нет'", FALSE, mailman, 0, ch, TO_VICT);
	}
}

void load()
{
	stuff_list.clear();

	pugi::xml_document doc;
	doc.load_file(LIB_PLRSTUFF"named_stuff_list.xml");

	pugi::xml_node obj_list = doc.child("named_stuff_list");
	for (pugi::xml_node node = obj_list.child("obj"); node; node = node.next_sibling("obj"))
	{
		StuffNodePtr tmp_node(new stuff_node);
		try
		{
			long vnum = boost::lexical_cast<long>(node.attribute("vnum").value());
			if (stuff_list.find(vnum) != stuff_list.end())
			{
				snprintf(buf, MAX_STRING_LENGTH,
					"NamedStuff: дубликат записи vnum=%ld (%s %s %d) пропущен",
					vnum, __FILE__, __func__, __LINE__);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			if(real_object(vnum)<0) {
				snprintf(buf, MAX_STRING_LENGTH,
					"NamedStuff: предмет vnum %ld не существует.", vnum);
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			}
			if(node.attribute("uid")) {
				tmp_node->uid = boost::lexical_cast<long>(node.attribute("uid").value());
				std::string name = GetNameByUnique(tmp_node->uid, false);// Ищем персонажа с указанным уид(богов игнорируем)
				if (name.empty())
				{
					snprintf(buf, MAX_STRING_LENGTH,
						"NamedStuff: UID %d - персонажа не существует(владелец предмета vnum %ld).", tmp_node->uid, vnum);
					mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
				}
			}
			if(node.attribute("mail")) {
				tmp_node->mail = node.attribute("mail").value();
			}
			if (tmp_node->mail.empty())
			{
				std::string name = GetNameByUnique(tmp_node->uid, false);
				snprintf(buf, MAX_STRING_LENGTH,
					"NamedStuff: указан пустой e-mail для предмета vnum:%ld (владелец предмета %s).", vnum, (name.empty()?"неизвестен":name.c_str()));
				mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			}
			if(node.attribute("can_clan"))
				tmp_node->can_clan = boost::lexical_cast<int>(node.attribute("can_clan").value());
			else
				tmp_node->can_clan = 0;
			if(node.attribute("can_alli"))
				tmp_node->can_alli = (bool)node.attribute("can_alli").value();
			else
				tmp_node->can_alli = 0;
			stuff_list[vnum] = tmp_node;
		}
		catch (std::exception &e)
		{
			log("NamedStuff : exception %s (%s %s %d)", e.what(), __FILE__, __func__, __LINE__);
		}
	}
	snprintf(buf, MAX_STRING_LENGTH,
		"NamedStuff: список именных вещей загружен всего объектов: %d.", stuff_list.size());
	mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
	/*#ifdef TEST_BUILD
    	save();
	#endif*/
}

} // namespace NamedStuff
