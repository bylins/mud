#ifndef BYLINS_EMPLOY_H
#define BYLINS_EMPLOY_H

const short SCMD_USE = 0;
const short SCMD_QUAFF = 1;
const short SCMD_RECITE = 2;

class CharacterData;

void do_employ(CharacterData *ch, char *argument, int cmd, int subcmd);

#endif //BYLINS_EMPLOY_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
