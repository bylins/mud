//
// Created by Svetodar on 29.08.2023.
//

#ifndef BYLINS_CHARGE_H
#define BYLINS_CHARGE_H

class CharData;

void do_charge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void go_charge(CharData *ch, int direction);


#endif //BYLINS_CHARGE_H
