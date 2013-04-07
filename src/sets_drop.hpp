// Part of Bylins http://www.mud.ru

#ifndef SETS_DROP_HPP_INCLUDED
#define SETS_DROP_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace SetsDrop
{

// период сохранения списков мобов (минуты)
const int SAVE_PERIOD = 27;
// лоад списков при старте мада
void init();
// вывод размера список по show stats
void show_stats(CHAR_DATA *ch);
// релоад списка сетов и перегенерация списка дропов
// без релоада статистики по убийствам мобов
void reload(int zone_vnum = 0);
// обновление таблицы дропа по таймеру
void reload_by_timer();
// проверка дропа сетины
int check_mob(int mob_rnum);
// тянем этот тупизм дальше
void renumber_obj_rnum(const int rnum, const int mob_rnum = -1);
// добавление инфы в систему справки
void init_xhelp();
void init_xhelp_full();
// сбор статы по убийствам мобов
void add_mob_stat(CHAR_DATA *mob, int members);
// сейв статы мобов
void save_mob_stat();
// печать статистики имму по конкретной зоне
void show_zone_stat(CHAR_DATA *ch, int zone_vnum);
// печать таймера резета таблицы дропа перед страницей справки
void print_timer_str(CHAR_DATA *ch);

} // namespace SetsDrop

#endif // SETS_DROP_HPP_INCLUDED
