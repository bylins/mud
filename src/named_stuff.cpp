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
#include "olc.h"

extern void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);

namespace NamedStuff
{

StuffListType stuff_list;

void save()
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
}

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
		if(!simple && CLAN(ch))//Предмет доступен сокланам или альянсу
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

bool parse_nedit_menu(CHAR_DATA *ch, char *arg)
{
	int num;
	StuffNodePtr tmp_node(new stuff_node);
	two_arguments(arg, buf1, buf2);
	if(!*buf1)
	{
		return false;
	}
	if(!*buf2 && *buf1!='в' && *buf1!='В' && *buf1!='х' && *buf1!='Х')
	{
		send_to_char("Не указан второй параметр!\r\n", ch);
		return false;
	}
	switch (*buf1)
	{
		case '1':
			if(a_isdigit(*buf2) && sscanf(buf2, "%d", &num))
			{
				if(real_object(num) < 0)
				{
					send_to_char(ch, "Такого объекта не существует.\r\n");
					return false;
				}
				OLC_NUM(ch->desc) = num;
				//send_to_char(ch, "Ок.\r\n");
			}
			break;
		case '2':
			num = GetUniqueByName(buf2);
			if(num>0)
			{
				OLC_MODE(ch->desc) = num;
				if(OLC_STORAGE(ch->desc))
					free(OLC_STORAGE(ch->desc));
				OLC_STORAGE(ch->desc) = str_dup(player_table[get_ptable_by_unique(num)].mail);
			}
			else
			{
				send_to_char(ch, "Такого персонажа не существует.\r\n");
				return false;
			}
			break;
		case '3':
			if(*buf2 && a_isdigit(*buf2) && sscanf(buf2, "%d", &num))
				OLC_VAL(ch->desc) = (int)(bool)num;
			break;
		case '4':
			if(*buf2 && a_isdigit(*buf2) && sscanf(buf2, "%d", &num))
				OLC_ZNUM(ch->desc) = (int)(bool)num;
			break;
		case 'У':
		case 'у':
			if(ch->desc->olc->item_type)
				stuff_list.erase(ch->desc->olc->item_type);
			cleanup_olc(ch->desc, CLEANUP_ALL);
			STATE(ch->desc) = CON_PLAYING;
			send_to_char(OK, ch);
			save();
			return 1;
			break;
		case 'В':
		case 'в':
			tmp_node->uid = OLC_MODE(ch->desc);
			tmp_node->can_clan = OLC_VAL(ch->desc);
			tmp_node->can_alli = OLC_ZNUM(ch->desc);
			tmp_node->mail = str_dup(OLC_STORAGE(ch->desc));
			if(ch->desc->olc->item_type)
				stuff_list.erase(ch->desc->olc->item_type);
			stuff_list[OLC_NUM(ch->desc)] = tmp_node;
			cleanup_olc(ch->desc, CLEANUP_ALL);
			STATE(ch->desc) = CON_PLAYING;
			send_to_char(OK, ch);
			save();
			return 1;
			break;
		case 'Х':
		case 'х':
			cleanup_olc(ch->desc, CLEANUP_ALL);
			STATE(ch->desc) = CON_PLAYING;
			send_to_char(OK, ch);
			return 1;
		default:
			break;
	}
	return false;
}

void nedit_menu(CHAR_DATA * ch)
{
	std::ostringstream out;

	out << CCIGRN(ch, C_SPR) << "1" << CCNRM(ch, C_SPR) << ") Vnum: " << OLC_NUM(ch->desc) << "\r\n";
	out << CCIGRN(ch, C_SPR) << "2" << CCNRM(ch, C_SPR) << ") Владелец: " << GetNameByUnique(OLC_MODE(ch->desc),0) << " e-mail: " << OLC_STORAGE(ch->desc) << "\r\n";
	out << CCIGRN(ch, C_SPR) << "3" << CCNRM(ch, C_SPR) << ") Доступно клану: " << (int)(bool)OLC_VAL(ch->desc) << "\r\n";
	out << CCIGRN(ch, C_SPR) << "4" << CCNRM(ch, C_SPR) << ") Доступно альянсу: " << (int)(bool)OLC_ZNUM(ch->desc) << "\r\n";
	if(ch->desc->olc->item_type)
		out << CCIGRN(ch, C_SPR) << "У" << CCNRM(ch, C_SPR) << ") Удалить\r\n";
	out << CCIGRN(ch, C_SPR) << "В" << CCNRM(ch, C_SPR) << ") Выйти и сохранить\r\n";
	out << CCIGRN(ch, C_SPR) << "Х" << CCNRM(ch, C_SPR) << ") Выйти без сохранения\r\n";
	send_to_char(out.str().c_str(), ch);
}

ACMD(do_named)
{
	mob_rnum r_num;
	std::string out;

	switch (subcmd)
	{
		case SCMD_NAMED_LIST:
			out += "Список именных предметов:\r\n";
			if(stuff_list.size() == 0)
			{
				out += " Пока что пусто.\r\n";
			}
			else
			{
				for (StuffListType::iterator it = stuff_list.begin(), iend = stuff_list.end(); it != iend; ++it)
				{
					if ((r_num = real_object(it->first)) < 0)
					{
						sprintf(buf2, "%6ld) Неизвестный объект",
							it->first);
					}
					else
					{
						sprintf(buf2, "%6d) %45s",
								obj_index[r_num].vnum, obj_proto[r_num]->short_description);
						if (GET_LEVEL(ch) >= LVL_GRGOD || PRF_FLAGGED(ch, PRF_CODERINFO))
							sprintf(buf2, "%s Игра:%d Пост:%d\r\n", buf2,
									obj_index[r_num].number, obj_index[r_num].stored);
						else
							sprintf(buf2, "%s\r\n", buf2);
					}
					out += buf2;
				}
			}
			send_to_char(out.c_str(), ch);
			break;
		case SCMD_NAMED_EDIT:
			int vnum;
			StuffListType::iterator it;
			one_argument(argument, buf1);
			if(*buf1 && a_isdigit(*buf1) && sscanf(buf1, "%d", &vnum))
			{
				if(real_object(vnum) < 0)
				{
					send_to_char(ch, "Такого объекта не существует.\r\n");
					return;
				}
				ch->desc->olc = new olc_data;
				if((it = stuff_list.find(vnum)) != stuff_list.end())
				{
					ch->desc->olc->item_type = vnum;
					OLC_MODE(ch->desc) = it->second->uid;
					OLC_NUM(ch->desc) = vnum;
					OLC_VAL(ch->desc) = it->second->can_clan;
					OLC_ZNUM(ch->desc) = it->second->can_alli;
					OLC_STORAGE(ch->desc) = str_dup(it->second->mail.c_str());
				}
				else
				{
					OLC_NUM(ch->desc) = vnum;
					OLC_VAL(ch->desc) = 0;
					OLC_ZNUM(ch->desc) = 0;
					CREATE(OLC_STORAGE(ch->desc), char, 256);
					OLC_STORAGE(ch->desc) = str_dup("");
				}
				STATE(ch->desc) = CON_NAMED_STUFF;
				nedit_menu(ch);
				return;
			}
			sprintf(buf, "Укажите VNUM для редактирования.\r\n");
			send_to_char(buf, ch);
			break;
	}
}

void receive_items(CHAR_DATA * ch, CHAR_DATA * mailman)
{
	OBJ_DATA *obj;
	mob_rnum r_num;
	int found = 0;
	snprintf(buf1, MAX_STRING_LENGTH, "не найден именной предмет");
	for (StuffListType::const_iterator it = stuff_list.begin(), iend = stuff_list.end(); it != iend; ++it)
	{
		if((it->second->uid==GET_UNIQUE(ch)) || (!strcmp(GET_EMAIL(ch), it->second->mail.c_str())))
		{
			if ((r_num = real_object(it->first)) < 0)
			{
				send_to_char("Странно но такого объекта не существует.\r\n", ch);
				snprintf(buf1, MAX_STRING_LENGTH, "объект не существует!!!");
				continue;
			}
			if((GET_OBJ_MIW(obj_proto[r_num]) > obj_index[r_num].stored + obj_index[r_num].number) ||//Проверка на макс в мире
			  (obj_index[r_num].stored + obj_index[r_num].number < 1))//Пока что если в мире нету то тоже загрузить
			{
                found++;
				snprintf(buf1, MAX_STRING_LENGTH,
					"выдаем именной предмет %s Max:%d > Current:%d",
					obj_proto[r_num]->short_description, GET_OBJ_MIW(obj_proto[r_num]), obj_index[r_num].stored + obj_index[r_num].number);
				obj = read_object(r_num, REAL);
				SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NAMED), ITEM_NAMED);
				obj_to_char(obj, ch);
				free_script(SCRIPT(obj));//детачим все триги чтоб не обламывать соклановцев и т.п.
				SCRIPT(obj) = NULL;
				obj_decay(obj);

				act("$n дал$g Вам $o3.", FALSE, mailman, obj, ch, TO_VICT);
				act("$N дал$G $n2 $o3.", FALSE, ch, obj, mailman, TO_ROOM);
			}
			else
			{
				snprintf(buf1, MAX_STRING_LENGTH,
					"не выдаем именной предмет %s Max:%d <= Current:%d",
					obj_proto[r_num]->short_description, GET_OBJ_MIW(obj_proto[r_num]), obj_index[r_num].stored + obj_index[r_num].number);
			}
			snprintf(buf, MAX_STRING_LENGTH,
				"NamedStuff: %s vnum:%ld %s",
				GET_PAD(ch,0), it->first, buf1);
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		}
	}
	if(!found) {
		act("$n сказал$g Вам : 'Кажется для тебя ничего нет'", FALSE, mailman, 0, ch, TO_VICT);
	}
	set_wait(ch, 3, FALSE);
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
			std::string name;
			if (stuff_list.find(vnum) != stuff_list.end())
			{
				snprintf(buf, MAX_STRING_LENGTH,
					"NamedStuff: дубликат записи vnum=%ld пропущен",
					vnum);
				mudlog(buf, CMP, LVL_BUILDER, SYSLOG, TRUE);
				continue;
			}

			if(real_object(vnum)<0) {
				snprintf(buf, MAX_STRING_LENGTH,
					"NamedStuff: предмет vnum=%ld не существует.", vnum);
				mudlog(buf, CMP, LVL_BUILDER, SYSLOG, TRUE);
			}
			if(node.attribute("uid")) {
				tmp_node->uid = boost::lexical_cast<long>(node.attribute("uid").value());
				name = GetNameByUnique(tmp_node->uid, false);// Ищем персонажа с указанным уид(богов игнорируем)
				if (name.empty())
				{
					snprintf(buf, MAX_STRING_LENGTH,
						"NamedStuff: Unique=%d - персонажа не существует(владелец предмета vnum=%ld).", tmp_node->uid, vnum);
					mudlog(buf, CMP, LVL_BUILDER, SYSLOG, TRUE);
				}
			}
			if(node.attribute("mail")) {
				tmp_node->mail = node.attribute("mail").value();
			}
			if (!valid_email(tmp_node->mail.c_str()))
			{
				std::string name = GetNameByUnique(tmp_node->uid, false);
				snprintf(buf, MAX_STRING_LENGTH,
					"NamedStuff: указан не корректный e-mail=%s для предмета vnum=%ld (владелец=%s).", tmp_node->mail.c_str(), vnum, (name.empty()?"неизвестен":name.c_str()));
				mudlog(buf, CMP, LVL_BUILDER, SYSLOG, TRUE);
			}
			if(node.attribute("can_clan"))
				tmp_node->can_clan = boost::lexical_cast<int>(node.attribute("can_clan").value());
			else
				tmp_node->can_clan = 0;
			if(node.attribute("can_alli"))
				tmp_node->can_alli = boost::lexical_cast<int>(node.attribute("can_alli").value());
			else
				tmp_node->can_alli = 0;
			stuff_list[vnum] = tmp_node;
			//snprintf(buf, MAX_STRING_LENGTH,
			//	"NamedStuff: прочитан предмет vnum=%ld владелец=%s Unique=%d доступен клану=%d альянсу=%d",
			//	vnum, name.c_str(), tmp_node->uid, tmp_node->can_clan, tmp_node->can_alli);
			//mudlog(buf, CMP, LVL_BUILDER, SYSLOG, TRUE);
		}
		catch (std::exception &e)
		{
			log("NamedStuff : exception %s (%s %s %d)", e.what(), __FILE__, __func__, __LINE__);
		}
	}
	snprintf(buf, MAX_STRING_LENGTH,
		"NamedStuff: список именных вещей загружен, всего объектов: %d.", stuff_list.size());
	mudlog(buf, CMP, LVL_BUILDER, SYSLOG, TRUE);
}

} // namespace NamedStuff
