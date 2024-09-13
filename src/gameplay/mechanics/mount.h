#ifndef BYLINS_MOUNT_H
#define BYLINS_MOUNT_H

class CharData;

CharData *get_horse(CharData *ch);
void make_horse(CharData *horse, CharData *ch);

void do_horseon(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horseoff(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void do_horseget(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horseput(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horsetake(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_givehorse(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_stophorse(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_MOUNT_H
