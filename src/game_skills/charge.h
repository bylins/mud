//
// Created by Svetodar on 29.08.2023.
//

#ifndef BYLINS_CHARGE_H
#define BYLINS_CHARGE_H

class CharData;

void go_charge(CharData *ch, int direction);
void do_charge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_CHARGE_H
