#ifndef BYLINS_GROUP_H
#define BYLINS_GROUP_H

#include "chars/char.hpp"

void change_leader(CHAR_DATA *ch, CHAR_DATA *vict);
void do_group(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_ungroup(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_report(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_GROUP_H
