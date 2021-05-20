// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 WorM
// Part of Bylins http://www.mud.ru

#ifndef NAMEDSTUFF_HPP_INCLUDED
#define NAMEDSTUFF_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "interpreter.h"

#include <unordered_map>

#define SCMD_NAMED_LIST 	1
#define SCMD_NAMED_EDIT 	2

namespace NamedStuff
{

void do_named(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void nedit_menu(CHAR_DATA *ch);
bool parse_nedit_menu(CHAR_DATA *ch, char *arg);

struct stuff_node
{
	int uid;		// uid персонажа
	std::string mail;	// e-mail персонажей которым можно подбирать
	int can_clan;		// может ли клан подбирать
	int can_alli;		// может ли альянс подбирать
	std::string wear_msg_v;	// сообщение при одевании предмета персу
	std::string wear_msg_a;	// --//-- вокруг перса
	std::string cant_msg_v;	// сообщение если нельзя взять/одеть предмет персу
	std::string cant_msg_a;	// --//-- вокруг перса
};

// общий список именного стафа
typedef std::shared_ptr<stuff_node> StuffNodePtr;
typedef std::unordered_map<long /* vnum предмета */, StuffNodePtr> StuffListType;

extern StuffListType stuff_list;

void save();
void load();
//Проверка доступен ли именной предмет чару, simple без проверки клана и союзов
//Возвращаемое значение по аналогии с check_anti_classes false-доступен true-недоступен
bool check_named(CHAR_DATA * ch, const OBJ_DATA * obj, const bool simple=false);
bool wear_msg(CHAR_DATA * ch, OBJ_DATA * obj);
//Процедура получения именного стафа у ямщика
void receive_items(CHAR_DATA * ch, CHAR_DATA * mailman);

} // namespace NamedStuff

#endif // NAMEDSTUFF_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
