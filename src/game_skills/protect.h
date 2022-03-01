#ifndef BYLINS_PROTECT_H
#define BYLINS_PROTECT_H

class CharData;

void go_protect(CharData *ch, CharData *vict);
void do_protect(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
CharData *TryToFindProtector(CharData *victim, CharData *ch);

#endif //BYLINS_PROTECT_H
