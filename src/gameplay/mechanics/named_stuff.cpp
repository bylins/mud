// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 WorM
// Part of Bylins http://www.mud.ru

#include <sstream>
#include "named_stuff.h"
#include "utils/russian_keys.h"
#include "utils/native_text.h"
#include <fmt/format.h>
#include "administration/privilege.h"
#include "gameplay/mechanics/minions.h"

#include "engine/db/world_objects.h"
#include "engine/db/obj_prototypes.h"
#include "engine/entities/obj_data.h"
#include "engine/ui/color.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/db.h"
#include "engine/core/obj_handler.h"
#include "gameplay/mechanics/inventory.h"
#include "gameplay/clans/house.h"
#include "engine/scripting/dg_scripts.h"
#include "third_party_libs/pugixml/pugixml.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/structs/structs.h"

#include <list>
#include <map>
#include <string>
#include <iomanip>
#include <vector>
#include "engine/db/player_index.h"

extern RoomRnum r_helled_start_room;
extern RoomRnum r_named_start_room;
extern RoomRnum r_unreg_start_room;

extern void SetWait(CharData *ch, int waittime, int victim_in_room);

namespace NamedStuff {

StuffListType stuff_list;

void save() {
	pugi::xml_document doc;
	doc.append_child().set_name("named_stuff_list");
	pugi::xml_node obj_list = doc.child("named_stuff_list");

	for (StuffListType::const_iterator i = stuff_list.begin(), iend = stuff_list.end(); i != iend; ++i) {
		pugi::xml_node stuf_node = obj_list.append_child();
		stuf_node.set_name("obj");
		stuf_node.append_attribute("vnum") = (int) i->first;
		stuf_node.append_attribute("uid") = i->second->uid;
		stuf_node.append_attribute("mail") = i->second->mail.c_str();
		if (i->second->can_clan)
			stuf_node.append_attribute("can_clan") = i->second->can_clan;
		if (i->second->can_alli)
			stuf_node.append_attribute("can_alli") = i->second->can_alli;
		if (!i->second->wear_msg_v.empty())
			stuf_node.append_attribute("wear_msg_v") = i->second->wear_msg_v.c_str();
		if (!i->second->wear_msg_a.empty())
			stuf_node.append_attribute("wear_msg_a") = i->second->wear_msg_a.c_str();
		if (!i->second->cant_msg_v.empty())
			stuf_node.append_attribute("cant_msg_v") = i->second->cant_msg_v.c_str();
		if (!i->second->cant_msg_a.empty())
			stuf_node.append_attribute("cant_msg_a") = i->second->cant_msg_a.c_str();
	}

	// Граница записи: XML лежит в нативной кодировке, пишем как есть. Чтение
	// (read_data_file) принимает и старый KOI8-R (issue #3787).
	std::ostringstream xml;
	doc.save(xml, "\t", pugi::format_default, pugi::encoding_utf8);
	native_text::write_file_native(LIB_USERDATA"named_items.xml", xml.str());
}

bool check_named(CharData *ch, const ObjData *obj, const bool simple) {
	if (!obj->has_flag(EObjFlag::kNamed)) {
		return false; // если шмотка не именная - остальное и проверять не нужно
	}
	StuffListType::iterator it = stuff_list.find(GET_OBJ_VNUM(obj));
	if (it != stuff_list.end()) {
		if (!ch)// если нету персонажа то вещь недоступна, это чтобы чистились клан храны
		{
			return true;
		}

		if (IsCharmice(ch)) // Чармисы тоже могут работать с именными вещами
		{
			CharData *master = ch->get_master();
			if (privilege::IsImmortal(master)) // Чармис имма
			{
				return false;
			}

			if (it->second->uid == master->get_uid()) // Чармис владельца предмета
			{
				return false;
			} else if (!strcmp(GET_EMAIL(master), it->second->mail.c_str()))  // Чармис владельца предмета судя по мылу
			{
				return false;
			}

			if (!simple && CLAN(master)) {
				if ((it->second->can_clan)
					&& (CLAN(master)->is_clan_member(it->second->uid)))//Это чармис соклановца и предмет доступен соклановцам
				{
					return false;
				}

				if ((it->second->can_alli)
					&& (CLAN(master)->is_alli_member(it->second->uid)))//Предмет доступен альянсу и это чармис чара из альянса
				{
					return false;
				}
			}
		}
		if (ch->IsNpc())
			return true;
		if (privilege::IsImmortal(ch)) // Имм
			return false;
		if (it->second->uid == ch->get_uid())//Это владелец предмета
			return false;
		else if (!strcmp(GET_EMAIL(ch), it->second->mail.c_str()))//Это владелец предмета судя по мылу
			return false;
		if (!simple && CLAN(ch))//Предмет доступен сокланам или альянсу
		{
			if ((it->second->can_clan)
				&& (CLAN(ch)->is_clan_member(it->second->uid)))//Это соклановец и предмет доступен соклановцам
				return false;
			if ((it->second->can_alli)
				&& (CLAN(ch)->is_alli_member(it->second->uid)))//Предмет доступен альянсу и это чар из альянса
				return false;
		}
		return true;
	} else
		return false;
}

bool wear_msg(CharData *ch, ObjData *obj) {
	StuffListType::iterator it = stuff_list.find(GET_OBJ_VNUM(obj));
	if (it != stuff_list.end()) {
		if (check_named(ch, obj, true)) {
			if (!it->second->cant_msg_v.empty()) {
				if (!it->second->cant_msg_a.empty())
					act(it->second->cant_msg_a.c_str(), false, ch, obj, 0, kToRoom);
				act(it->second->cant_msg_v.c_str(), false, ch, obj, 0, kToChar);
				return true;
			} else
				return false;
		} else {
			if (!it->second->wear_msg_v.empty()) {
				if (number(1, 100) <= 20) {
					if (!it->second->wear_msg_a.empty())
						act(it->second->wear_msg_a.c_str(), false, ch, obj, 0, kToRoom);
					act(it->second->wear_msg_v.c_str(), false, ch, obj, 0, kToChar);
				}
				return true;
			} else
				return false;
		}
	}
	return false;
}

bool parse_nedit_menu(CharData *ch, char *arg) {
	int num;
	StuffNodePtr tmp_node(new stuff_node);
	char param[kMaxInputLength], value[kMaxInputLength];
	half_chop(arg, param, value);
	if (!*param) {
		return false;
	}
	if ((*param < '1' || *param > '8') && (native_text::first_char_code_lower(param) != rus::kVe
			&& native_text::first_char_code_lower(param) != rus::kHa
			&& native_text::first_char_code_lower(param) != rus::kU)) {
		// Печатаем символ целиком, а не первый байт: под UTF-8 русская буква в char не влезает,
		// и игрок получал в ответ обломок вместо своей буквы (issue #3797).
		SendMsgToChar(fmt::format("Неверный параметр {}!\r\n",
								  std::string_view(param, native_text::char_bytes(param))), ch);
		return false;
	}
	if (!*value && native_text::first_char_code_lower(param) != rus::kVe
		&& native_text::first_char_code_lower(param) != rus::kHa
		&& native_text::first_char_code_lower(param) != rus::kU) {
		if (*param < '5' || *param > '8') {
			SendMsgToChar("Не указан второй параметр!\r\n", ch);
		} else {
			std::string msg;
			switch (*param) {
				case '5': msg = fmt::format("&S{}&s\r\n", ch->desc->named_obj->wear_msg_v);
					break;
				case '6': msg = fmt::format("&S{}&s\r\n", ch->desc->named_obj->wear_msg_a);
					break;
				case '7': msg = fmt::format("&S{}&s\r\n", ch->desc->named_obj->cant_msg_v);
					break;
				case '8': msg = fmt::format("&S{}&s\r\n", ch->desc->named_obj->cant_msg_a);
					break;
				default: msg = "&RОшибка.&n\r\n";
					break;
			}
			SendMsgToChar(msg, ch);
		}
		return false;
	}

	switch (native_text::first_char_code_lower(param)) {
		case '1':
			if (a_isdigit(*value) && sscanf(value, "%d", &num)) {
				if (GetObjRnum(num) < 0) {
					SendMsgToChar(ch, "Такого объекта не существует.\r\n");
					return false;
				}
				ch->desc->cur_vnum = num;
			}
			break;

		case '2': num = GetUniqueByName(value);
			if (0 >= num) {
				SendMsgToChar(ch, "Такого персонажа не существует.\r\n");
				return false;
			}
			ch->desc->named_obj->uid = num;
			ch->desc->named_obj->mail = player_table[GetPtableByUnique(num)].mail;
			break;

		case '3':
			if (*value && a_isdigit(*value) && sscanf(value, "%d", &num)) {
				ch->desc->named_obj->can_clan = 0 == num ? 0 : 1;
			}
			break;

		case '4':
			if (*value && a_isdigit(*value) && sscanf(value, "%d", &num)) {
				ch->desc->named_obj->can_alli = 0 == num ? 0 : 1;
			}
			break;

		case '5':
			if (*value) {
				ch->desc->named_obj->wear_msg_v = delete_doubledollar(value);
				/* TODO: review me
				if(!strcmp(ch->desc->named_obj->wear_msg_v.c_str(), "_"))
					ch->desc->named_obj->wear_msg_v == "";
					*/
			}
			break;

		case '6':
			if (*value) {
				ch->desc->named_obj->wear_msg_a = delete_doubledollar(value);
				/* TODO: review me
				if(!strcmp(ch->desc->named_obj->wear_msg_a.c_str(), "_"))
					ch->desc->named_obj->wear_msg_a == "";
					*/
			}
			break;

		case '7':
			if (*value) {
				ch->desc->named_obj->cant_msg_v = delete_doubledollar(value);
				/* TODO: review me
				if(!strcmp(ch->desc->named_obj->cant_msg_v.c_str(), "_"))
					ch->desc->named_obj->cant_msg_v == "";
					*/
			}
			break;

		case '8':
			if (*value) {
				ch->desc->named_obj->cant_msg_a = delete_doubledollar(value);
				/* TODO: review me
				if(!strcmp(ch->desc->named_obj->cant_msg_a.c_str(), "_"))
					ch->desc->named_obj->cant_msg_a == "";
					*/
			}
			break;

		case rus::kU:
			if (!ch->desc->old_vnum)
				return false;
			stuff_list.erase(ch->desc->old_vnum);
			ch->desc->state = EConState::kPlaying;
			SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
			save();
			return true;

		case rus::kVe: tmp_node->uid = ch->desc->named_obj->uid;
			tmp_node->can_clan = ch->desc->named_obj->can_clan;
			tmp_node->can_alli = ch->desc->named_obj->can_alli;
			tmp_node->mail = ch->desc->named_obj->mail;
			tmp_node->wear_msg_v = ch->desc->named_obj->wear_msg_v;
			tmp_node->wear_msg_a = ch->desc->named_obj->wear_msg_a;
			tmp_node->cant_msg_v = ch->desc->named_obj->cant_msg_v;
			tmp_node->cant_msg_a = ch->desc->named_obj->cant_msg_a;
			if (ch->desc->old_vnum)
				stuff_list.erase(ch->desc->old_vnum);
			stuff_list[ch->desc->cur_vnum] = tmp_node;
			ch->desc->state = EConState::kPlaying;
			SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
			save();
			return true;

		case rus::kHa: ch->desc->state = EConState::kPlaying;
			SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
			return true;

		default: break;
	}
	return false;
}

void nedit_menu(CharData *ch) {
	std::ostringstream out;

	out << kColorBoldGrn << "1" << kColorNrm << ") Vnum: " << ch->desc->cur_vnum << " Название: "
		<< (GetObjRnum(ch->desc->cur_vnum)
			? obj_proto[GetObjRnum(ch->desc->cur_vnum)]->get_short_description().c_str() : "&Rнеизвестно&n") << "\r\n";
	out << kColorBoldGrn << "2" << kColorNrm << ") Владелец: "
		<< GetNameByUnique(ch->desc->named_obj->uid, 0) << " e-mail: &S" << ch->desc->named_obj->mail << "&s\r\n";
	out << kColorBoldGrn << "3" << kColorNrm << ") Доступно клану: "
		<< (0 == ch->desc->named_obj->can_clan ? 0 : 1) << "\r\n";
	out << kColorBoldGrn << "4" << kColorNrm << ") Доступно альянсу: "
		<< (0 == ch->desc->named_obj->can_alli ? 0 : 1) << "\r\n";
	out << kColorBoldGrn << "5" << kColorNrm << ") Сообщение при одевании персу: "
		<< ch->desc->named_obj->wear_msg_v << "\r\n";
	out << kColorBoldGrn << "6" << kColorNrm << ") Сообщение при одевании вокруг перса: "
		<< ch->desc->named_obj->wear_msg_a << "\r\n";
	out << kColorBoldGrn << "7" << kColorNrm << ") Сообщение если вещь недоступна персу: "
		<< ch->desc->named_obj->cant_msg_v << "\r\n";
	out << kColorBoldGrn << "8" << kColorNrm << ") Сообщение если вещь недоступна вокруг перса: "
		<< ch->desc->named_obj->cant_msg_a << "\r\n";
	if (ch->desc->old_vnum) {
		out << kColorBoldGrn << "У" << kColorNrm << ") Удалить\r\n";
	}
	out << kColorBoldGrn << "В" << kColorNrm << ") Выйти и сохранить\r\n";
	out << kColorBoldGrn << "Х" << kColorNrm << ") Выйти без сохранения\r\n";
	SendMsgToChar(out.str().c_str(), ch);
}

void do_named(CharData *ch, char *argument, int cmd, int subcmd) {
	MobRnum r_num;
	std::string out;
	bool have_missed_items = false;
	int first = 0, last = 0, found = 0, uid = -1;

	char arg_first[kMaxInputLength], arg_second[kMaxInputLength];
	two_arguments(argument, arg_first, arg_second);
	// Фильтр по владельцу: либо почта найденного персонажа, либо то, что ввели,
	// -- тогда ищем подстроку в почте. При поиске по номерам фильтр пустой.
	std::string mail_filter;

	if (*arg_first) {
		if (is_number(arg_first)) {
			first = atoi(arg_first);
			last = *arg_second ? atoi(arg_second) : first;
		} else {
			last = 1;
			first = 0x7fffffff;
			uid = GetUniqueByName(arg_first);
			mail_filter = arg_first;
			if (uid > 0) {
				mail_filter = player_table[GetPtableByUnique(uid)].mail;
			}
		}
	}

	switch (subcmd) {
		case SCMD_NAMED_LIST: {
			const std::string header = "Список именных предметов:\r\n";
			if (stuff_list.empty()) {
				out += header;
				out += " Пока что пусто.\r\n";
			} else {
				for (StuffListType::iterator it = stuff_list.begin(), iend = stuff_list.end(); it != iend; ++it) {
					const bool by_mail = !mail_filter.empty()
						&& it->second->mail.find(mail_filter) != std::string::npos;
					if ((r_num = GetObjRnum(it->first)) < 0) {
						if (by_mail
							|| (uid != -1
								&& uid == it->second->uid)
							|| (uid == -1
								&& it->first >= first
								&& it->first <= last)) {
							if (found == 0) {
								out += header;
							}
							found++;
							out += fmt::format("{:6}) &R*&n{:<31} Владелец:{:<16} e-mail:&S{}&s\r\n",
											   it->first + 1,
											   "Несуществующий предмет",
											   GetNameByUnique(it->second->uid, false),
											   it->second->mail);
						}
					} else {
						if (by_mail
							|| (uid != -1
								&& uid == it->second->uid)
							|| (uid == -1
								&& obj_proto[r_num]->get_vnum() >= first
								&& obj_proto[r_num]->get_vnum() <= last)) {
							// Колонка с названием -- 32 символа влево, как и в ветке выше:
							// раньше сюда передавали -32, и ширина превращалась в 255 пробелов.
							const std::string line =
								fmt::format("{:6}) {}",
											obj_proto[r_num]->get_vnum(),
											colored_name(obj_proto[r_num]->get_short_description().c_str(), 32, true));
							if (found == 0) {
								out += header;
							}
							found++;
							if (privilege::IsGrGod(ch) || ch->IsFlagged(EPrf::kCoderinfo)) {
								out += fmt::format("{} Игра:{} Пост:{} Владелец:{:<16} e-mail:&S{}&s\r\n",
												   line,
												   obj_proto.total_online(r_num), obj_proto.stored(r_num),
												   GetNameByUnique(it->second->uid, false), it->second->mail);
							} else {
								out += line + "\r\n";
							}
						}
					}
				}
			}
			if (!found) {
				out += fmt::format("Нет таких именных вещей.\r\nСинтаксис {} [vnum [vnum] | имя | email]\r\n",
								   cmd_info[cmd].command);
			}
			SendMsgToChar(out.c_str(), ch);
			break;
		}
		case SCMD_NAMED_EDIT: int found = 0;
			if ((first > 0 && first < 0x7fffffff) || uid != -1 || !mail_filter.empty()) {
				if (first > 0 && first < 0x7fffffff && GetObjRnum(first) < 0) {
					SendMsgToChar(ch, "Такого объекта не существует.\r\n");
					return;
				}

				StuffNodePtr tmp_node(new stuff_node);
				for (
					StuffListType::iterator it = stuff_list.begin(), iend = stuff_list.end();
					it != iend;
					++it) {
					if ((uid == -1 && it->first == first) || it->second->uid == uid
						|| !str_cmp(it->second->mail.c_str(), mail_filter.c_str())) {
						if (GetObjRnum(it->first) < 0) {
							if (!have_missed_items) {
								out += "&RВнимание!!!&n\r\nНесуществующие объекты в списке именых вещей:\r\n";
								have_missed_items = true;
							}
							out += fmt::format("vnum:{:9} uid:{:9} mail:{}\r\n",
											   it->first,
											   it->second->uid,
											   it->second->mail);
							continue;
						}
						ch->desc->old_vnum = it->first;
						ch->desc->cur_vnum = it->first;
						tmp_node->uid = it->second->uid;
						tmp_node->can_clan = it->second->can_clan;
						tmp_node->can_alli = it->second->can_alli;
						tmp_node->mail = it->second->mail;
						tmp_node->wear_msg_v = it->second->wear_msg_v;
						tmp_node->wear_msg_a = it->second->wear_msg_a;
						tmp_node->cant_msg_v = it->second->cant_msg_v;
						tmp_node->cant_msg_a = it->second->cant_msg_a;
						found++;
						break;
					}
				}
				if (!found && first > 0 && first < 0x7fffffff) {
					ch->desc->old_vnum = 0;
					ch->desc->cur_vnum = first;
					tmp_node->uid = 0;
					tmp_node->can_clan = 0;
					tmp_node->can_alli = 0;
					tmp_node->mail.clear();
					tmp_node->wear_msg_v.clear();
					tmp_node->wear_msg_a.clear();
					tmp_node->cant_msg_v.clear();
					tmp_node->cant_msg_a.clear();
					found++;
				}
				if (have_missed_items) {
					out += "\r\n\r\n";
					SendMsgToChar(out.c_str(), ch);
				}
				if (found) {
					ch->desc->named_obj = tmp_node;
					ch->desc->state = EConState::kNamedStuff;

					nedit_menu(ch);
					return;
				} else
					tmp_node.reset();
			}
			SendMsgToChar(ch, "Нет таких именных вещей.\r\nСинтаксис %s [vnum | имя | email]\r\n",
						  cmd_info[cmd].command);
			//SendMsgToChar("Укажите VNUM для редактирования.\r\n", ch);
			break;
	}
}

void receive_items(CharData *ch, CharData *mailman) {
	if ((ch->in_room == r_helled_start_room) ||
		(ch->in_room == r_named_start_room) ||
		(ch->in_room == r_unreg_start_room)) {
		act("$n сказал$g вам : 'Вот выйдешь - тогда и получишь!'", false, mailman, 0, ch, kToVict);
		return;
	}

	MobRnum r_num;
	int found = 0;
	int in_world = 0;
	std::string reason = "не найден именной предмет";
	for (StuffListType::const_iterator it = stuff_list.begin(), iend = stuff_list.end(); it != iend; ++it) {
		if ((it->second->uid == ch->get_uid()) || (!strcmp(GET_EMAIL(ch), it->second->mail.c_str()))) {
			if ((r_num = GetObjRnum(it->first)) < 0) {
				SendMsgToChar("Странно, но такого объекта не существует.\r\n", ch);
				reason = "объект не существует!!!";
				continue;
			}
			if ((GetObjMIW(r_num) > obj_proto.actual_count(r_num))    //Проверка на макс в мире
				|| (obj_proto.actual_count(r_num) < 1))//Пока что если в мире нету то тоже загрузить
			{
				found++;
				reason = fmt::format("выдаем именной предмет {} Max:{} > Current:{}",
									 obj_proto[r_num]->get_short_description(),
									 GetObjMIW(r_num),
									 obj_proto.actual_count(r_num));
				const auto obj = world_objects.create_from_prototype_by_rnum(r_num);
				obj->set_extra_flag(EObjFlag::kNamed);
				PlaceObjToInventory(obj.get(), ch);
				obj->cleanup_script();
				CheckObjDecay(obj.get());

				act("$n дал$g вам $o3.", false, mailman, obj.get(), ch, kToVict);
				act("$N дал$G $n2 $o3.", false, ch, obj.get(), mailman, kToRoom);
			} else {
				reason = fmt::format("не выдаем именной предмет {} Max:{} <= Current:{}",
									 obj_proto[r_num]->get_short_description(),
									 GetObjMIW(r_num),
									 obj_proto.actual_count(r_num));
				in_world++;
			}
			mudlog(fmt::format("NamedStuff: {} vnum:{} {}", GET_PAD(ch, 0), it->first, reason),
				   LGH, kLvlImmortal, SYSLOG, true);
		}
	}

	if (!found) {
		if (!in_world) {
			act("$n сказал$g вам : 'Кажется для тебя ничего нет'", false, mailman, 0, ch, kToVict);
		} else {
			act("$n сказал$g вам : 'Забрал кто-то твои вещи'", false, mailman, 0, ch, kToVict);
		}
	}

	SetWait(ch, 3, false);
}

void load() {
	stuff_list.clear();

	pugi::xml_document doc;
	// Файл лежит на диске в KOI8-R; читаем через границу кодировки, а разбираем уже
	// буфер в нативной кодировке движка (issue #3681). Под KOI8-R это тождество.
	const std::string xml_named_stuff = native_text::read_data_file(LIB_USERDATA"named_items.xml");
	doc.load_buffer(xml_named_stuff.data(), xml_named_stuff.size());

	pugi::xml_node obj_list = doc.child("named_stuff_list");
	for (pugi::xml_node node = obj_list.child("obj"); node; node = node.next_sibling("obj")) {
		StuffNodePtr tmp_node(new stuff_node);
		try {
			long vnum = std::stol(node.attribute("vnum").value(), nullptr, 10);
			std::string name;
			if (stuff_list.find(vnum) != stuff_list.end()) {
				mudlog(fmt::format("NamedStuff: дубликат записи vnum={} пропущен", vnum),
					   NRM, kLvlBuilder, SYSLOG, true);
				continue;
			}

			if (GetObjRnum(vnum) < 0) {
				mudlog(fmt::format("NamedStuff: предмет vnum={} не существует.", vnum),
					   NRM, kLvlBuilder, SYSLOG, true);
			}
			if (node.attribute("uid")) {
				tmp_node->uid = std::stol(node.attribute("uid").value(), nullptr, 10);
				name = GetNameByUnique(tmp_node->uid, false);// Ищем персонажа с указанным уид(богов игнорируем)
				if (name.empty()) {
					mudlog(fmt::format("NamedStuff: Unique={} - персонажа не существует(владелец предмета vnum={}).",
									   tmp_node->uid, vnum),
						   NRM, kLvlBuilder, SYSLOG, true);
				}
			}
			if (node.attribute("mail")) {
				tmp_node->mail = node.attribute("mail").value();
			}
			if (node.attribute("wear_msg")) {
				tmp_node->wear_msg_v = node.attribute("wear_msg").value();
			}
			if (node.attribute("wear_msg_v")) {
				tmp_node->wear_msg_v = node.attribute("wear_msg_v").value();
			}
			if (node.attribute("wear_msg_a")) {
				tmp_node->wear_msg_a = node.attribute("wear_msg_a").value();
			}
			if (node.attribute("cant_msg")) {
				tmp_node->cant_msg_v = node.attribute("cant_msg").value();
			}
			if (node.attribute("cant_msg_v")) {
				tmp_node->cant_msg_v = node.attribute("cant_msg_v").value();
			}
			if (node.attribute("cant_msg_a")) {
				tmp_node->cant_msg_a = node.attribute("cant_msg_a").value();
			}
			if (!IsValidEmail(tmp_node->mail.c_str())) {
				std::string name = GetNameByUnique(tmp_node->uid, false);
				mudlog(fmt::format("NamedStuff: указан не корректный e-mail=&S{}&s для предмета vnum={} (владелец={}).",
								   tmp_node->mail,
								   vnum,
								   name.empty() ? std::string("неизвестен") : name),
					   NRM, kLvlBuilder, SYSLOG, true);
			}
			if (node.attribute("can_clan"))
				tmp_node->can_clan = std::stoi(node.attribute("can_clan").value(), nullptr, 10);
			else
				tmp_node->can_clan = 0;
			if (node.attribute("can_alli"))
				tmp_node->can_alli = std::stoi(node.attribute("can_alli").value(), nullptr, 10);
			else
				tmp_node->can_alli = 0;
			stuff_list[vnum] = tmp_node;
		}
		catch (std::exception &e) {
			log("NamedStuff : exception %s (%s %s %d)", e.what(), __FILE__, __func__, __LINE__);
		}
	}
	mudlog(fmt::format("NamedStuff: список именных вещей загружен, всего объектов: {}.", stuff_list.size()),
		   CMP, kLvlBuilder, SYSLOG, true);
}

} // namespace NamedStuff

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
