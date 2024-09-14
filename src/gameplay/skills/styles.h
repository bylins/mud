#ifndef BYLINS_STYLES_H
#define BYLINS_STYLES_H

class CharData;

void go_touch(CharData *ch, CharData *vict);
void do_touch(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

void go_deviate(CharData *ch);
void do_deviate(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

void do_style(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_STYLES_H
