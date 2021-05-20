// description.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "description.h"

#include "logger.hpp"
#include "structs.h"
#include "conf.h"
#include "sysdep.h"

#include <stdexcept>

std::vector<std::string> RoomDescription::_desc_list;
RoomDescription::reboot_map_t RoomDescription::_reboot_map;

/**
* добавление описания в массив с проверкой на уникальность
* \param text - описание комнаты
* \return номер описания в глобальном массиве
*/
size_t RoomDescription::add_desc(const std::string &text)
{
	reboot_map_t::const_iterator it = _reboot_map.find(text);
	if (it != _reboot_map.end())
	{
		return it->second;
	}
	else
	{
		_desc_list.push_back(text);
		_reboot_map[text] = _desc_list.size();
		return _desc_list.size();
	}
}

const static std::string empty_string = "";

/**
* поиск описания по его порядковому номеру в массиве
* \param desc_num - порядковый номер описания (descripton_num в room_data)
* \return строка описания или пустая строка в случае невалидного номера
*/
const std::string& RoomDescription::show_desc(size_t desc_num)
{
	try
	{
		return _desc_list.at(--desc_num);
	}
	catch (const std::out_of_range&)
	{
		log("SYSERROR : bad room description num '%zd' (%s %s %d)", desc_num, __FILE__, __func__, __LINE__);
		return empty_string;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
