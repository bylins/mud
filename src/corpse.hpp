// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#ifndef CORPSE_HPP_INCLUDED
#define CORPSE_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

void make_arena_corpse(CHAR_DATA * ch, CHAR_DATA * killer);
OBJ_DATA *make_corpse(CHAR_DATA * ch);

namespace GlobalDrop
{

void init();
void save();
// период сохранения временного файла с мобами (в минутах)
const int SAVE_PERIOD = 10;

} // namespace GlobalDrop

#endif // CORPSE_HPP_INCLUDED
