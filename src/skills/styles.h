#ifndef BYLINS_STYLES_H
#define BYLINS_STYLES_H

class CharacterData;

void go_touch(CharacterData *ch, CharacterData *vict);
void do_touch(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

void go_deviate(CharacterData *ch);
void do_deviate(CharacterData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

void do_style(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_STYLES_H
