// Copyright (c) 2016 bodrich
// Part of Bylins http://www.bylins.su
#pragma once

#include "bonus_types.h"

#include <string>

class CHAR_DATA;    // to avoid inclusion of "char.hpp"

namespace Bonus {
// используется для ограниченя получения любого (не только от общеегрового бонуса) бонусного опыта указанным чаром
// для НПС - результат всегда false
bool can_get_bonus_exp(CHAR_DATA *ch);

void setup_bonus(int duration_in_seconds, int multilpier, EBonusType type);

void do_bonus_by_character(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_bonus_info(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

// активен ли указанный тип бонуса
bool is_bonus_active(EBonusType type);

// активен ли любой из бонусов
bool is_bonus_active();

void timer_bonus();

// строковое представление времени до конца бонуса
std::string time_to_bonus_end_as_string();

// строковой представление активного бонуса. если бонуса нет - пустая строка
std::string active_bonus_as_string();

int get_mult_bonus();
void bonus_log_load();
void show_log(CHAR_DATA *ch);
void dg_do_bonus(char *cmd);
}
