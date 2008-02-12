// deathtrap.hpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DEATHTRAP_HPP_INCLUDED
#define DEATHTRAP_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

/**
* Список слоу-дт (включая проваливание под лед), чтобы не гонять каждые 2 секунды по 64к комнатам.
* Добавлние/удаление при ребуте, правке в олц и проваливанию под лед
*/
namespace DeathTrap {

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
// Убирание номагик флагов в комнатах слоу-дт
void check_nomagic_slowdeath(int rnum);
// Проверка комнаты на принадлежность к медленным дт
bool DeathTrap::is_slow_dt(int rnum);
} // namespace DeathTrap

/**
* Список односторонних порталов (по втригеру и вратам), единственная цель - не перебирать все
* комнаты при резете зоны для удаления всех пент на ее комнаты, ибо занимает много ресурсов.
*/
namespace OneWayPortal {

void add(ROOM_DATA* to_room, ROOM_DATA* from_room);
void remove(ROOM_DATA* to_room);
ROOM_DATA* get_from_room(ROOM_DATA* to_room);

} // namespace OneWayPortal

#endif // DEATHTRAP_HPP_INCLUDED
