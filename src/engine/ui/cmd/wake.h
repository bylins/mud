//
// Created by Sventovit on 08.09.2024.
//

#ifndef BYLINS_SRC_CMD_WAKE_H_
#define BYLINS_SRC_CMD_WAKE_H_

const int kScmdWake{0};
const int kScmdWakeUp{1};

class CharData;
void do_wake(CharData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_SRC_CMD_WAKE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
