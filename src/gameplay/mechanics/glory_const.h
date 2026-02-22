// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2010 Krodo
// Part of Bylins http://www.mud.ru

#ifndef GLORY_CONST_HPP_INCLUDED
#define GLORY_CONST_HPP_INCLUDED

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/ui/interpreter.h"

namespace GloryConst {
struct glory_olc;
const int cur_ver = 1;//текущая версия нужна чтоб определить надо обновлять xml файл/пересчитывать статы или нет

// процент комиссии на вложенную славу при вынимании
const int STAT_RETURN_FEE = 0; // комис за перевод славы в статах персонажа (проценты)
const int TRANSFER_FEE = 0; // минимальный комис за перевод славы другому игроку (в процентах)
//    const int MIN_TRANSFER_AMOUNT = 1; 
const int MIN_TRANSFER_TAX = 20; // //минимальное кол-во славы для перевода

//Кол-во единиц жизни, добавляемое за раз
const int HP_FACTOR = 50;
const int SUCCESS_FACTOR = 7;
const int SAVE_FACTOR = 15;
const int RESIST_FACTOR = 7;
const int MANAREG_FACTOR = 50;
const int BONUSPSYS_FACTOR = 3;
const int BONUSMAG_FACTOR = 3;

int get_glory(long uid);
void add_glory(long uid, int amount);
int remove_glory(long uid, int amount);

void do_glory(CharData *ch, char *argument, int cmd, int subcmd);
void do_spend_glory(CharData *ch, char *argument, int cmd, int subcmd);
void spend_glory_menu(CharData *ch);
bool parse_spend_glory_menu(CharData *ch, char *arg);
void glory_hide(CharData *ch, bool mode);

void save();
void load();

void set_stats(CharData *ch);
int main_stats_count(CharData *ch);

void show_stats(CharData *ch);

void transfer_log(const char *format, ...) __attribute__((format(gnu_printf, 1, 2)));
void add_total_spent(int amount);
void apply_modifiers(CharData *ch);
void PrintGloryChart(CharData *ch);

} // namespace GloryConst

#endif // GLORY_CONST_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
