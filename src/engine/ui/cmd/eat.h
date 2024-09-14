#ifndef BYLINS_SRC_CMD_EAT_H_
#define BYLINS_SRC_CMD_EAT_H_

const int kScmdEat = 0;
const int kScmdTaste = 1;
const int kScmdDevour = 4;

class CharData;

void do_eat(CharData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_SRC_CMD_EAT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
