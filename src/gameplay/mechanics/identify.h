/**
 \file identify.h - a part of the Bylins engine.
 \brief issue.spellhandlers: item/character identify-display mechanic (shared by the identify
        spells, the identify skill, and the trading systems: auction/exchange/shops/clan house).
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_IDENTIFY_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_IDENTIFY_H_

#include <string>

class CharData;
class ObjData;
class CObjectPrototype;

// Что записано в книге (заклинание, умение, рецепт, способность) одной строкой.
// Пусто, если предмет не книга или содержимое битое. Общая для опознания и осмотра (#3877).
// Если передан персонаж -- к строке добавляется "(вам недоступно)" для талантов, которых
// его класс не получает вовсе.
std::string GetBookContents(const CObjectPrototype *obj, CharData *ch = nullptr);

// Render an object's full stat block to `ch` (detail level scaled by `fullness`).
void MortShowObjValues(const ObjData *obj, CharData *ch, int fullness);
// Render a character's stat block to `ch` (detail level scaled by `fullness`).
void MortShowCharValues(CharData *victim, CharData *ch, int fullness);

#endif  // BYLINS_SRC_GAMEPLAY_MECHANICS_IDENTIFY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
