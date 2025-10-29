//
// Created by Sventovit on 08.09.2024.
//

#ifndef BYLINS_SRC_CMD_WHERE_H_
#define BYLINS_SRC_CMD_WHERE_H_

class CharData;

void DoWhere(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void DoFindObjByRnum(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_CMD_WHERE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
