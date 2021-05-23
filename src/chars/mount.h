#ifndef BYLINS_MOUNT_H
#define BYLINS_MOUNT_H

class CHAR_DATA;

CHAR_DATA *get_horse(CHAR_DATA *ch);
void make_horse(CHAR_DATA *horse, CHAR_DATA *ch);

void do_horseon(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horseoff(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void do_horseget(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horseput(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horsetake(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_givehorse(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_stophorse(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_MOUNT_H
