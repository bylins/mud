//
// Created by Svetodar on 21.07.2023.
//

#ifndef BYLINS_SLAY_H
#define BYLINS_SLAY_H

class CharData;

void do_slay(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void go_slay(CharData *ch, CharData *vict);

#endif //BYLINS_SLAY_H
