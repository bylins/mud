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

// Инициализация списка при загрузке мада
void load();
// Добавление новой комнаты с проверкой на присутствие
void add(ROOM_DATA* room);
// Удаление комнаты из списка слоу-дт
void remove(ROOM_DATA* room);
// Проверка активности дт, дергается каждые 2 секунды в хеарбите
void activity();

} // namespace DeathTrap

#endif // DEATHTRAP_HPP_INCLUDED
