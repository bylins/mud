// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef MAP_HPP_INCLUDED
#define MAP_HPP_INCLUDED

#include <string>
#include <string_view>
#include "engine/ui/map_options.h"

class CharData;

void do_map(CharData *ch, char *argument, int cmd, int subcmd);

namespace MapSystem {

void print_map(CharData *ch, CharData *imm = 0);
void do_command(CharData *ch, const std::string &arg);

// Клетка карты хранится строкой вида "&K - &n": цвет, видимая часть, сброс. Сброс после
// каждой клетки стоит дорого -- на проводе это ещё семь байт, а клеток в карте сотни
// (у имма в городе выходило под шесть килобайт на шаг). Эти две функции разбирают клетку
// на части, чтобы печатать цвет только при смене, а сбрасывать раз в конце строки.

// Цветовой ключ клетки ("&K") или пусто, если клетка без цвета.
std::string_view SignColor(std::string_view sign);

// Видимая часть клетки (" - ") -- без ведущего цвета и завершающего "&n".
std::string_view SignGlyph(std::string_view sign);

} // namespace MapSystem

#endif // MAP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
