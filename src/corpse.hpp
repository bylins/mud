// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#ifndef CORPSE_HPP_INCLUDED
#define CORPSE_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

void make_arena_corpse(CHAR_DATA * ch, CHAR_DATA * killer);
OBJ_DATA *make_corpse(CHAR_DATA * ch, CHAR_DATA * killer = NULL);

namespace GlobalDrop
{

void init();
void save();
void renumber_obj_rnum(int rnum);
bool check_mob(OBJ_DATA *corpse, CHAR_DATA *ch);

// период сохранения временного файла с мобами (в минутах)
const int SAVE_PERIOD = 10;
// для генерации книги улучшения умения
const int BOOK_UPRGD_VNUM = 1920;

} // namespace GlobalDrop

#endif // CORPSE_HPP_INCLUDED
