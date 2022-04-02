#ifndef BYLINS_CHOPOFF_H
#define BYLINS_CHOPOFF_H

#include "entities/char_data.h"

void go_chopoff(CharData *ch, CharData *vict);
void do_chopoff(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_CHOPOFF_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
