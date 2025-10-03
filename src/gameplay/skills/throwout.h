//
// Created by Svetodar on 20.09.2025.
//

#ifndef BYLINS_THROWOUT_H
#define BYLINS_THROWOUT_H
#include "engine/entities/char_data.h"

class CharData;

void go_throwout(CharData *ch, CharData *vict);
void do_throwout(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);


#endif //BYLINS_THROWOUT_H