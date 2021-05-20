// Copyright (c) 2012 Krodo
// Part of Bylins http://www.mud.ru

#ifndef SETS_DROP_HPP_INCLUDED
#define SETS_DROP_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include <map>
namespace SetsDrop
{

// период сохранения списков мобов и таблицы дропа (минуты)
const int SAVE_PERIOD = 27;
// лоад списков при старте мада
void init();
// релоад списка сетов и перегенерация списка дропов
// без релоада статистики по убийствам мобов
void reload(int zone_vnum = 0);
// обновление таблицы дропа по таймеру
void reload_by_timer();
// проверка дропа сетины
int check_mob(int mob_rnum);
// тянем этот тупизм дальше
void renumber_obj_rnum(const int mob_rnum = -1);
// добавление инфы в систему справки
void init_xhelp();
void init_xhelp_full();
// печать таймера резета таблицы дропа перед страницей справки
void print_timer_str(CHAR_DATA *ch);
// сейв текущей таблицы дропа и шансов
void save_drop_table();
void create_clone_miniset(int vnum);
std::map<int, int> get_unique_mob();

} // namespace SetsDrop

#endif // SETS_DROP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
