// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#ifndef CORPSE_HPP_INCLUDED
#define CORPSE_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

void make_arena_corpse(CHAR_DATA * ch, CHAR_DATA * killer);
OBJ_DATA *make_corpse(CHAR_DATA * ch, CHAR_DATA * killer = NULL);

namespace FullSetDrop
{

// период сохранения списков мобов (минуты)
const int SAVE_PERIOD = 27;
// для сохранения списков по отдельности
const bool SOLO_TYPE = false;
const bool GROUP_TYPE = true;

// лоад списков при старте мада
void init();
// добавление моба в списки в зависимости от кол-ва убивших игроков
void add_kill(CHAR_DATA *mob, int members);
// сохранение списков
void save_list(bool list_type);
// вывод размера список по show stats
void show_stats(CHAR_DATA *ch);
// релоад списка сетов и перегенерация списка дропов
// без релоада списков соло и груп-киллов
void reload();
// проверка дропа сетины
int check_mob(int mob_rnum);
// тянем этот тупизм дальше
void renumber_obj_rnum(int rnum);
// добавление инфы в систему справки
void init_xhelp();
void init_xhelp_full();
// тестовый сбор статы по убийствам мобов
void add_mob_stat(CHAR_DATA *mob, int members);
// инит списка тестовой статы мобов
void init_mob_stat();
// сейв тестовой статы мобов
void save_mob_stat();

} // namespace FullSetDrop

namespace GlobalDrop
{

void init();
void save();
void renumber_obj_rnum(int rnum);
bool check_mob(OBJ_DATA *corpse, CHAR_DATA *ch);

// период сохранения временного файла с мобами (в минутах)
const int SAVE_PERIOD = 10;
// для генерации книги улучшения умения
const int BOOK_UPGRD_VNUM = 1920;
// для генерации энчантов
const int WARR1_ENCHANT_VNUM = 1921;
const int WARR2_ENCHANT_VNUM = 1922;
const int WARR3_ENCHANT_VNUM = 1923;

} // namespace GlobalDrop

#endif // CORPSE_HPP_INCLUDED
