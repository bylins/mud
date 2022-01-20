#ifndef BYLINS_CREATE_H
#define BYLINS_CREATE_H

const short SCMD_RECIPE = 1;

class CharacterData;

void do_create(CharacterData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_CREATE_H
