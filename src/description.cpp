// description.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include <stdexcept>

#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "description.h"

std::vector<std::string> RoomDescription::_desc_list;
std::map<std::string, int> RoomDescription::_reboot_map;

/**
* добавление описания в массив с проверкой на уникальность
* \param text - описание комнаты
* \return номер описания в глобальном массиве
*/
int RoomDescription::add_desc(const std::string &text)
{
	std::map<std::string, int>::const_iterator it = _reboot_map.find(text);
	if (it != _reboot_map.end())
		return it->second;
	else
	{
		_desc_list.push_back(text);
		_reboot_map[text] = _desc_list.size();
		return _desc_list.size();
	}
}

/**
* поиск описания по его порядковому номеру в массиве
* \param desc_num - порядковый номер описания (descripton_num в room_data)
* \return строка описания или пустая строка в случае невалидного номера
*/
const std::string RoomDescription::show_desc(int desc_num)
{
	try
	{
		return _desc_list.at(--desc_num);
	}
	catch (std::out_of_range)
	{
		log("SYSERROR : bad room description num '%d' (%s %s %d)", desc_num, __FILE__, __func__, __LINE__);
		return "";
	}
}
