// deathtrap.hpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DEATHTRAP_HPP_INCLUDED
#define DEATHTRAP_HPP_INCLUDED

#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/conf.h"

struct RoomData;    // forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

/**
* Список слоу-дт (включая проваливание под лед), чтобы не гонять каждые 2 секунды по 64к комнатам.
* Добавлние/удаление при ребуте, правке в олц и проваливанию под лед
*/
namespace deathtrap {

// Инициализация списка при загрузке мада или редактирования комнат в олц
void load();
// Добавление новой комнаты с проверкой на присутствие
void add(RoomData *room);
// Удаление комнаты из списка слоу-дт
void remove(RoomData *room);
// Проверка активности дт, дергается каждые 2 секунды в хеарбите
void activity();
// Обработка обычных дт
int check_death_trap(CharData *ch);
// Проверка комнаты на принадлежность к медленным дт
bool IsSlowDeathtrap(int rnum);
// \return true - чара может сразу убить при входе в ванрум
bool check_tunnel_death(CharData *ch, int room_rnum);
// Дамаг чаров с бд в ван-румах, \return true - чара убили
bool tunnel_damage(CharData *ch);

} // namespace DeathTrap



#endif // DEATHTRAP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
