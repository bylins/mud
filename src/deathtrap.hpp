// deathtrap.hpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DEATHTRAP_HPP_INCLUDED
#define DEATHTRAP_HPP_INCLUDED

#include "structs.h"
#include "sysdep.h"
#include "conf.h"

struct ROOM_DATA;	// forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

/**
* Список слоу-дт (включая проваливание под лед), чтобы не гонять каждые 2 секунды по 64к комнатам.
* Добавлние/удаление при ребуте, правке в олц и проваливанию под лед
*/
namespace DeathTrap
{

// Инициализация списка при загрузке мада или редактирования комнат в олц
void load();
// Добавление новой комнаты с проверкой на присутствие
void add(ROOM_DATA* room);
// Удаление комнаты из списка слоу-дт
void remove(ROOM_DATA* room);
// Проверка активности дт, дергается каждые 2 секунды в хеарбите
void activity();
// Обработка обычных дт
int check_death_trap(CHAR_DATA * ch);
// Проверка комнаты на принадлежность к медленным дт
bool is_slow_dt(int rnum);
// \return true - чара может сразу убить при входе в ванрум
bool check_tunnel_death(CHAR_DATA *ch, int room_rnum);
// Дамаг чаров с бд в ван-румах, \return true - чара убили
bool tunnel_damage(CHAR_DATA *ch);

} // namespace DeathTrap



#endif // DEATHTRAP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
