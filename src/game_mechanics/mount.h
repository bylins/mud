#ifndef BYLINS_MOUNT_H
#define BYLINS_MOUNT_H

class CharacterData;

CharacterData *get_horse(CharacterData *ch);
void make_horse(CharacterData *horse, CharacterData *ch);

void do_horseon(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horseoff(CharacterData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void do_horseget(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horseput(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_horsetake(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_givehorse(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_stophorse(CharacterData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_MOUNT_H
