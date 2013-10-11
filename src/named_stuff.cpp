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
#include "utils.h"
#include "screen.h"
#include "char.hpp"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "house.h"
#include "dg_scripts.h"

extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern room_rnum r_unreg_start_room;

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
		stuf_node.append_attribute("vnum") = (int)i->first;
		stuf_node.append_attribute("uid") = i->second->uid;
		stuf_node.append_attribute("mail") = i->second->mail.c_str();
		if(i->second->can_clan)
			stuf_node.append_attribute("can_clan") = i->second->can_clan;
		if(i->second->can_alli)
			stuf_node.append_attribute("can_alli") = i->second->can_alli;
		if(!i->second->wear_msg_v.empty())
			stuf_node.append_attribute("wear_msg_v") = i->second->wear_msg_v.c_str();
		if(!i->second->wear_msg_a.empty())
			stuf_node.append_attribute("wear_msg_a") = i->second->wear_msg_a.c_str();
		if(!i->second->cant_msg_v.empty())
			stuf_node.append_attribute("cant_msg_v") = i->second->cant_msg_v.c_str();
		if(!i->second->cant_msg_a.empty())
			stuf_node.append_attribute("cant_msg_a") = i->second->cant_msg_a.c_str();
	}

	doc.save_file(LIB_PLRSTUFF"named_stuff_list.xml");
}

bool check_named(CHAR_DATA * ch, const OBJ_DATA * obj, const bool simple)
{
	if (!OBJ_FLAGGED(obj, ITEM_NAMED))
		return false; // если шмотка не именная - остальное и проверять не нужно
	StuffListType::iterator it = stuff_list.find(GET_OBJ_VNUM(obj));
	if (it != stuff_list.end())
	{
		if(!ch)// если нету персонажа то вещь недоступна, это чтобы чистились клан храны
			return true;
		if(IS_CHARMICE(ch)) // Чармисы тоже могут работать с именными вещами
		{
			CHAR_DATA *master = ch->master;
			if(WAITLESS(master)) // Чармис имма
				return false;
			if(it->second->uid==GET_UNIQUE(master)) // Чармис владельца предмета
				return false;
			else if(!strcmp(GET_EMAIL(master), it->second->mail.c_str()))  // Чармис владельца предмета судя по мылу
				return false;
			if(!simple && CLAN(master))
			{
				if((it->second->can_clan) && (CLAN(master)->is_clan_member(it->second->uid)))//Это чармис соклановца и предмет доступен соклановцам
					return false;
				if((it->second->can_alli) && (CLAN(master)->is_alli_member(it->second->uid)))//Предмет доступен альянсу и это чармис чара из альянса
					return false;
			}
		}
		if(IS_NPC(ch))
			return true;
		if(WAITLESS(ch)) // Имм
			return false;
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

bool wear_msg(CHAR_DATA * ch, OBJ_DATA * obj)
{
	StuffListType::iterator it = stuff_list.find(GET_OBJ_VNUM(obj));
	if (it != stuff_list.end()) {
		if (check_named(ch, obj, true))
		{
			if (!it->second->cant_msg_v.empty())
			{
				if (!it->second->cant_msg_a.empty())
					act(it->second->cant_msg_a.c_str(), FALSE, ch, obj, 0, TO_ROOM);
				act(it->second->cant_msg_v.c_str(), FALSE, ch, obj, 0, TO_CHAR);
				return true;
			}
			else
				return false;
		}
		else
		{
			if (!it->second->wear_msg_v.empty())
			{
				if (number(1, 100) <= 20)
				{
					if (!it->second->wear_msg_a.empty())
						act(it->second->wear_msg_a.c_str(), FALSE, ch, obj, 0, TO_ROOM);
					act(it->second->wear_msg_v.c_str(), FALSE, ch, obj, 0, TO_CHAR);
				}
				return true;
			}
			else
				return false;
		}
	}
	return false;
}

bool parse_nedit_menu(CHAR_DATA *ch, char *arg)
{
	int num;
	StuffNodePtr tmp_node(new stuff_node);
        char i[256];
	half_chop(arg, buf1, buf2);
	i[0]=0;
	if(!*buf1)
	{
		return false;
	}
	if ((*buf1<'1' || *buf1>'8') && (LOWER(*buf1)!='в' && LOWER(*buf1)!='х' && LOWER(*buf1)!='у'))
	{
		send_to_char(ch, "Неверный параметр %c!\r\n", *buf1);
		return false;
	}
	if(!*buf2 && LOWER(*buf1)!='в' && LOWER(*buf1)!='х' && LOWER(*buf1)!='у')
	{
		if (*buf1<'5' ||  *buf1>'8')
			send_to_char("Не указан второй параметр!\r\n", ch);
		else
		{
			switch (*buf1)
			{
				case '5':
					snprintf(i, 256, "&S%s&s\r\n", ch->desc->named_obj->wear_msg_v.c_str());
					break;
				case '6':
					snprintf(i, 256, "&S%s&s\r\n", ch->desc->named_obj->wear_msg_a.c_str());
					break;
				case '7':
					snprintf(i, 256, "&S%s&s\r\n", ch->desc->named_obj->cant_msg_v.c_str());
					break;
				case '8':
					snprintf(i, 256, "&S%s&s\r\n", ch->desc->named_obj->cant_msg_a.c_str());
					break;
				default:
					snprintf(i, 256, "&RОшибка.&n\r\n");
					break;
			}
			send_to_char(i, ch);
		}
		return false;
	}
	switch (LOWER(*buf1))
	{
		case '1':
			if(a_isdigit(*buf2) && sscanf(buf2, "%d", &num))
			{
				if(real_object(num) < 0)
				{
					send_to_char(ch, "Такого объекта не существует.\r\n");
					return false;
				}
				ch->desc->cur_vnum = num;
			}
			break;
		case '2':
			num = GetUniqueByName(buf2);
			if(num>0)
			{
				ch->desc->named_obj->uid = num;
				ch->desc->named_obj->mail = str_dup(player_table[get_ptable_by_unique(num)].mail);
			}
			else
			{
				send_to_char(ch, "Такого персонажа не существует.\r\n");
				return false;
			}
			break;
		case '3':
			if(*buf2 && a_isdigit(*buf2) && sscanf(buf2, "%d", &num))
				ch->desc->named_obj->can_clan = (int)(bool)num;
			break;
		case '4':
			if(*buf2 && a_isdigit(*buf2) && sscanf(buf2, "%d", &num))
				ch->desc->named_obj->can_alli = (int)(bool)num;
			break;
		case '5':
			if(*buf2)
			{
				ch->desc->named_obj->wear_msg_v = delete_doubledollar(buf2);
				if(!strcmp(ch->desc->named_obj->wear_msg_v.c_str(), "_"))
					ch->desc->named_obj->wear_msg_v == "";
			}
			break;
		case '6':
			if(*buf2)
			{
				ch->desc->named_obj->wear_msg_a = delete_doubledollar(buf2);
				if(!strcmp(ch->desc->named_obj->wear_msg_a.c_str(), "_"))
					ch->desc->named_obj->wear_msg_a == "";
			}
			break;
		case '7':
			if(*buf2)
			{
				ch->desc->named_obj->cant_msg_v = delete_doubledollar(buf2);
				if(!strcmp(ch->desc->named_obj->cant_msg_v.c_str(), "_"))
					ch->desc->named_obj->cant_msg_v == "";
			}
			break;
		case '8':
			if(*buf2)
			{
				ch->desc->named_obj->cant_msg_a = delete_doubledollar(buf2);
				if(!strcmp(ch->desc->named_obj->cant_msg_a.c_str(), "_"))
					ch->desc->named_obj->cant_msg_a == "";
			}
			break;
		case 'у':
			if(!ch->desc->old_vnum)
				return false;
			stuff_list.erase(ch->desc->old_vnum);
			STATE(ch->desc) = CON_PLAYING;
			send_to_char(OK, ch);
			save();
			return true;
			break;
		case 'в':
			tmp_node->uid = ch->desc->named_obj->uid;
			tmp_node->can_clan = ch->desc->named_obj->can_clan;
			tmp_node->can_alli = ch->desc->named_obj->can_alli;
			tmp_node->mail = ch->desc->named_obj->mail;
			tmp_node->wear_msg_v = ch->desc->named_obj->wear_msg_v;
			tmp_node->wear_msg_a = ch->desc->named_obj->wear_msg_a;
			tmp_node->cant_msg_v = ch->desc->named_obj->cant_msg_v;
			tmp_node->cant_msg_a = ch->desc->named_obj->cant_msg_a;
			if(ch->desc->old_vnum)
				stuff_list.erase(ch->desc->old_vnum);
			stuff_list[ch->desc->cur_vnum] = tmp_node;
			STATE(ch->desc) = CON_PLAYING;
			send_to_char(OK, ch);
			save();
			return true;
			break;
		case 'х':
			STATE(ch->desc) = CON_PLAYING;
			send_to_char(OK, ch);
			return true;
		default:
			break;
	}
	return false;
}

void nedit_menu(CHAR_DATA * ch)
{
	std::ostringstream out;

	out << CCIGRN(ch, C_SPR) << "1" << CCNRM(ch, C_SPR) << ") Vnum: " << ch->desc->cur_vnum << " Название: " << (real_object(ch->desc->cur_vnum)?obj_proto[real_object(ch->desc->cur_vnum)]->short_description:"&Rнеизвестно&n") << "\r\n";
	out << CCIGRN(ch, C_SPR) << "2" << CCNRM(ch, C_SPR) << ") Владелец: " << GetNameByUnique(ch->desc->named_obj->uid,0) << " e-mail: &S" << ch->desc->named_obj->mail << "&s\r\n";
	out << CCIGRN(ch, C_SPR) << "3" << CCNRM(ch, C_SPR) << ") Доступно клану: " << (int)(bool)ch->desc->named_obj->can_clan << "\r\n";
	out << CCIGRN(ch, C_SPR) << "4" << CCNRM(ch, C_SPR) << ") Доступно альянсу: " << (int)(bool)ch->desc->named_obj->can_alli << "\r\n";
	out << CCIGRN(ch, C_SPR) << "5" << CCNRM(ch, C_SPR) << ") Сообщение при одевании персу: " << ch->desc->named_obj->wear_msg_v << "\r\n";
	out << CCIGRN(ch, C_SPR) << "6" << CCNRM(ch, C_SPR) << ") Сообщение при одевании вокруг перса: " << ch->desc->named_obj->wear_msg_a << "\r\n";
	out << CCIGRN(ch, C_SPR) << "7" << CCNRM(ch, C_SPR) << ") Сообщение если вещь недоступна персу: " << ch->desc->named_obj->cant_msg_v << "\r\n";
	out << CCIGRN(ch, C_SPR) << "8" << CCNRM(ch, C_SPR) << ") Сообщение если вещь недоступна вокруг перса: " << ch->desc->named_obj->cant_msg_a << "\r\n";
	if(ch->desc->old_vnum)
		out << CCIGRN(ch, C_SPR) << "У" << CCNRM(ch, C_SPR) << ") Удалить\r\n";
	out << CCIGRN(ch, C_SPR) << "В" << CCNRM(ch, C_SPR) << ") Выйти и сохранить\r\n";
	out << CCIGRN(ch, C_SPR) << "Х" << CCNRM(ch, C_SPR) << ") Выйти без сохранения\r\n";
	send_to_char(out.str().c_str(), ch);
}

ACMD(do_named)
{
	mob_rnum r_num;
	std::string out;
	int first = 0, last = 0, found = 0, uid = -1;

	two_arguments(argument, buf, buf2);

	if (*buf)
	{
		if (is_number(buf))
		{
			first = atoi(buf);
			if (*buf2)
				last = atoi(buf2);
			else
				last = first;
			*buf = '\0';
		}
		else
		{
		 	last = 1;
		 	first = 0x7fffffff;
			uid = GetUniqueByName(buf);
			//*buf = '\0';
			if (uid > 0)
			{
				strncpy(buf, player_table[get_ptable_by_unique(uid)].mail, sizeof(buf));
			}
		}
	}

	switch (subcmd)
	{
		case SCMD_NAMED_LIST:
			sprintf(buf1, "Список именных предметов:\r\n");
			if(stuff_list.size() == 0)
			{
				out += buf1;
				out += " Пока что пусто.\r\n";
			}
			else
			{
				for (StuffListType::iterator it = stuff_list.begin(), iend = stuff_list.end(); it != iend; ++it)
				{
					if ((r_num = real_object(it->first)) < 0)
					{
						sprintf(buf2, "%6ld) Неизвестный объект\r\n",
							it->first);
						out += buf2;
					}
					else
					{
						if ((*buf && strstr(it->second->mail.c_str(), buf)) ||
						   (uid != -1 && uid == it->second->uid) ||
						   (uid == -1 && obj_index[r_num].vnum >= first && obj_index[r_num].vnum <= last))
						{
							sprintf(buf2, "%6d) %s",
									obj_index[r_num].vnum, colored_name(obj_proto[r_num]->short_description, 50));
							if (IS_GRGOD(ch) || PRF_FLAGGED(ch, PRF_CODERINFO))
								sprintf(buf2, "%s Игра:%d Пост:%d Владелец:%16s e-mail:&S%s&s\r\n", buf2,
									obj_index[r_num].number, obj_index[r_num].stored,
									GetNameByUnique(it->second->uid,false).c_str(), it->second->mail.c_str());
							else
								sprintf(buf2, "%s\r\n", buf2);
							if (found == 0)
								out += buf1;
							found++;
							out += buf2;
						}
					}
				}
			}
			if (!found)
			{
				sprintf(buf, "Нет таких именных вещей.\r\nСинтаксис %s [vnum [vnum] | имя | email]\r\n", CMD_NAME);
				out += buf;
			}
			send_to_char(out.c_str(), ch);
			break;
		case SCMD_NAMED_EDIT:
			int found = 0;
			if((first > 0 && first < 0x7fffffff) || uid != -1 || *buf)
			{
				if(first > 0 && first < 0x7fffffff && real_object(first) < 0)
				{
					send_to_char(ch, "Такого объекта не существует.\r\n");
					return;
				}

				StuffNodePtr tmp_node(new stuff_node);
				for (StuffListType::iterator it = stuff_list.begin(), iend = stuff_list.end(); it != iend; ++it)
				{
					if((uid == -1 && it->first == first) || it->second->uid == uid || !str_cmp(it->second->mail.c_str(), buf))
					{
						ch->desc->old_vnum = it->first;
						ch->desc->cur_vnum = it->first;
						tmp_node->uid = it->second->uid;
						tmp_node->can_clan = it->second->can_clan;
						tmp_node->can_alli = it->second->can_alli;
						tmp_node->mail = str_dup(it->second->mail.c_str());
						tmp_node->wear_msg_v = str_dup(it->second->wear_msg_v.c_str());
						tmp_node->wear_msg_a = str_dup(it->second->wear_msg_a.c_str());
						tmp_node->cant_msg_v = str_dup(it->second->cant_msg_v.c_str());
						tmp_node->cant_msg_a = str_dup(it->second->cant_msg_a.c_str());
						found++;
						break;
					}
				}
				if (!found && first > 0 && first < 0x7fffffff)
				{
					ch->desc->old_vnum = 0;
					ch->desc->cur_vnum = first;
					tmp_node->uid = 0;
					tmp_node->can_clan = 0;
					tmp_node->can_alli = 0;
					tmp_node->mail = str_dup("");
					tmp_node->wear_msg_v = str_dup("");
					tmp_node->wear_msg_a = str_dup("");
					tmp_node->cant_msg_v = str_dup("");
					tmp_node->cant_msg_a = str_dup("");
					found++;
				}
				if (found)
				{
					ch->desc->named_obj = tmp_node;
					STATE(ch->desc) = CON_NAMED_STUFF;
					nedit_menu(ch);
					return;
				}
				else
					tmp_node.reset();
			}
			send_to_char(ch, "Нет таких именных вещей.\r\nСинтаксис %s [vnum | имя | email]\r\n", CMD_NAME);
			//send_to_char("Укажите VNUM для редактирования.\r\n", ch);
			break;
	}
}

void receive_items(CHAR_DATA * ch, CHAR_DATA * mailman)
{
	if ((IN_ROOM(ch) == r_helled_start_room) ||
		(IN_ROOM(ch) == r_named_start_room) ||
		(IN_ROOM(ch) == r_unreg_start_room))
	{
		act("$n сказал$g вам : 'Вот выйдешь - тогда и получишь!'", FALSE, mailman, 0, ch, TO_VICT);
		return;
	}

	OBJ_DATA *obj;
	mob_rnum r_num;
	int found = 0;
	int in_world = 0;
	snprintf(buf1, MAX_STRING_LENGTH, "не найден именной предмет");
	for (StuffListType::const_iterator it = stuff_list.begin(), iend = stuff_list.end(); it != iend; ++it)
	{
		if((it->second->uid==GET_UNIQUE(ch)) || (!strcmp(GET_EMAIL(ch), it->second->mail.c_str())))
		{
			if ((r_num = real_object(it->first)) < 0)
			{
				send_to_char("Странно, но такого объекта не существует.\r\n", ch);
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

				act("$n дал$g вам $o3.", FALSE, mailman, obj, ch, TO_VICT);
				act("$N дал$G $n2 $o3.", FALSE, ch, obj, mailman, TO_ROOM);
			}
			else
			{
				snprintf(buf1, MAX_STRING_LENGTH,
					"не выдаем именной предмет %s Max:%d <= Current:%d",
					obj_proto[r_num]->short_description, GET_OBJ_MIW(obj_proto[r_num]), obj_index[r_num].stored + obj_index[r_num].number);
				in_world++;
			}
			snprintf(buf, MAX_STRING_LENGTH,
				"NamedStuff: %s vnum:%ld %s",
				GET_PAD(ch,0), it->first, buf1);
			mudlog(buf, LGH, LVL_IMMORT, SYSLOG, TRUE);
		}
	}
	if(!found) {
		if(!in_world)
			act("$n сказал$g вам : 'Кажется для тебя ничего нет'", FALSE, mailman, 0, ch, TO_VICT);
		else
			act("$n сказал$g вам : 'Забрал кто-то твои вещи'", FALSE, mailman, 0, ch, TO_VICT);
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
				mudlog(buf, NRM, LVL_BUILDER, SYSLOG, TRUE);
				continue;
			}

			if(real_object(vnum)<0) {
				snprintf(buf, MAX_STRING_LENGTH,
					"NamedStuff: предмет vnum=%ld не существует.", vnum);
				mudlog(buf, NRM, LVL_BUILDER, SYSLOG, TRUE);
			}
			if(node.attribute("uid")) {
				tmp_node->uid = boost::lexical_cast<long>(node.attribute("uid").value());
				name = GetNameByUnique(tmp_node->uid, false);// Ищем персонажа с указанным уид(богов игнорируем)
				if (name.empty())
				{
					snprintf(buf, MAX_STRING_LENGTH,
						"NamedStuff: Unique=%d - персонажа не существует(владелец предмета vnum=%ld).", tmp_node->uid, vnum);
					mudlog(buf, NRM, LVL_BUILDER, SYSLOG, TRUE);
				}
			}
			if(node.attribute("mail")) {
				tmp_node->mail = node.attribute("mail").value();
			}
			if(node.attribute("wear_msg")) {
				tmp_node->wear_msg_v = node.attribute("wear_msg").value();
			}
			if(node.attribute("wear_msg_v")) {
				tmp_node->wear_msg_v = node.attribute("wear_msg_v").value();
			}
			if(node.attribute("wear_msg_a")) {
				tmp_node->wear_msg_a = node.attribute("wear_msg_a").value();
			}
			if(node.attribute("cant_msg")) {
				tmp_node->cant_msg_v = node.attribute("cant_msg").value();
			}
			if(node.attribute("cant_msg_v")) {
				tmp_node->cant_msg_v = node.attribute("cant_msg_v").value();
			}
			if(node.attribute("cant_msg_a")) {
				tmp_node->cant_msg_a = node.attribute("cant_msg_a").value();
			}
			if (!valid_email(tmp_node->mail.c_str()))
			{
				std::string name = GetNameByUnique(tmp_node->uid, false);
				snprintf(buf, MAX_STRING_LENGTH,
					"NamedStuff: указан не корректный e-mail=&S%s&s для предмета vnum=%ld (владелец=%s).", tmp_node->mail.c_str(), vnum, (name.empty()?"неизвестен":name.c_str()));
				mudlog(buf, NRM, LVL_BUILDER, SYSLOG, TRUE);
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
		}
		catch (std::exception &e)
		{
			log("NamedStuff : exception %s (%s %s %d)", e.what(), __FILE__, __func__, __LINE__);
		}
	}
	snprintf(buf, MAX_STRING_LENGTH,
		"NamedStuff: список именных вещей загружен, всего объектов: %lu.",
		static_cast<unsigned long>(stuff_list.size()));
	mudlog(buf, CMP, LVL_BUILDER, SYSLOG, TRUE);
}

} // namespace NamedStuff
