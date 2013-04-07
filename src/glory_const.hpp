// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#ifndef GLORY_CONST_HPP_INCLUDED
#define GLORY_CONST_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "interpreter.h"

namespace GloryConst
{

const int cur_ver=1;//текущая версия нужна чтоб определить надо обновлять xml файл/пересчитывать статы или нет

// процент комиссии на вложенную славу при вынимании
const int STAT_RETURN_FEE = 10;
// комис за перевод славы (проценты)
const int TRANSFER_FEE = 5;
// минимальный комис за перевод славы (не проценты)
const int MIN_TRANSFER_TAX = 50;
// минимальное кол-во славы для перевода (>= MIN_TRANSFER_TAX)
const int MIN_TRANSFER_AMOUNT = 100;

//Кол-во единиц жизни, добавляемое за раз
const int HP_FACTOR=50; //перенесено в glory_const.hpp

//каст
const int SUCCESS_FACTOR=7;

//Сависы
const int SAVE_FACTOR = 15;

//Резисты
const int RESIST_FACTOR = 7;

int get_glory(long uid);
void add_glory(long uid, int amount);
int remove_glory(long uid, int amount);

ACMD(do_glory);
ACMD(do_spend_glory);
void spend_glory_menu(CHAR_DATA *ch);
bool parse_spend_glory_menu(CHAR_DATA *ch, char *arg);

void save();
void load();

void set_stats(CHAR_DATA *ch);
int main_stats_count(CHAR_DATA *ch);

void show_stats(CHAR_DATA *ch);

void transfer_log(const char *format, ...);
void add_total_spent(int amount);
void apply_modifiers(CHAR_DATA *ch);

} // namespace GloryConst

#endif // GLORY_CONST_HPP_INCLUDED
