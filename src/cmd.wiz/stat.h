#ifndef BYLINS_STAT_H
#define BYLINS_STAT_H

#include "chars/character.h"

extern const char *pc_class_types[];

void do_stat(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_stat_character(CHAR_DATA * ch, CHAR_DATA * k, int virt);
void do_stat_object(CHAR_DATA * ch, OBJ_DATA * j, int virt);
void do_stat_room(CHAR_DATA * ch, int rnum);

#endif //BYLINS_STAT_H
