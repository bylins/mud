// deathtrap.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "conf.h"
#include <list>
#include <algorithm>

#include "deathtrap.hpp"
#include "constants.h"
#include "db.h"
#include "spells.h"
#include "utils.h"
#include "handler.h"

namespace DeathTrap {

// список текущих слоу-дт в маде
std::list<ROOM_DATA*> room_list;

} // namespace DeathTrap

/**
* Инициализация списка при загрузке мада или редактирования комнат в олц
*/
void DeathTrap::load()
{
	// на случай релоада, свапать смысла нету
	room_list.clear();

	for (int i = FIRST_ROOM; i <= top_of_world; ++i)
		if (ROOM_FLAGGED(i, ROOM_SLOWDEATH) || ROOM_FLAGGED(i, ROOM_ICEDEATH))
			room_list.push_back(world[i]);
}

/**
* Добавление новой комнаты с проверкой на присутствие
* \param room - комната, кот. добавляем
*/
void DeathTrap::add(ROOM_DATA* room)
{
	std::list<ROOM_DATA*>::const_iterator it = std::find(room_list.begin(), room_list.end(), room);
	if (it == room_list.end())
		room_list.push_back(room);
}

/**
* Удаление комнаты из списка слоу-дт
* \param room - комната, кот. удаляем
*/
void DeathTrap::remove(ROOM_DATA* room)
{
	room_list.remove(room);
}

/**
* Проверка активности дт, дергается каждые 2 секунды в хеарбите.
*/
void DeathTrap::activity()
{
	CHAR_DATA *ch, *next;

	for(std::list<ROOM_DATA*>::const_iterator it = room_list.begin(); it != room_list.end(); ++it)
		for (ch = (*it)->people; ch; ch = next) {
			next = ch->next_in_room;
			if (!IS_NPC(ch) && (damage(ch, ch, MAX(1, GET_REAL_MAX_HIT(ch) >> 2), TYPE_ROOMDEATH, FALSE) < 0))
				log("Player %s died in slow DT (room %d)", GET_NAME(ch), (*it)->number);
		}
}

namespace OneWayPortal {

// список односторонних порталов <куда указывает, откуда поставлен>
std::map<ROOM_DATA*, ROOM_DATA*> portal_list;

} // namespace OneWayPortal

/**
* Добавление портала в список
* \param to_room - куда ставится пента
* \param from_room - откуда ставится
*/
void OneWayPortal::add(ROOM_DATA* to_room, ROOM_DATA* from_room)
{
	portal_list[to_room] = from_room;
}

/**
* Удаление портала из списка
* \param to_room - куда указывает пента
*/
void OneWayPortal::remove(ROOM_DATA* to_room)
{
	std::map<ROOM_DATA*, ROOM_DATA*>::iterator it = portal_list.find(to_room);
	if (it != portal_list.end())
		portal_list.erase(it);
}

/**
* Проверка на наличие комнаты в списке
* \param to_room - куда указывает пента
* \return указатель на источник пенты
*/
ROOM_DATA* OneWayPortal::get_from_room(ROOM_DATA* to_room)
{
	std::map<ROOM_DATA*, ROOM_DATA*>::const_iterator it = portal_list.find(to_room);
	if (it != portal_list.end())
		return it->second;
	return 0;
}
