#ifndef BYLINS_STAT_H
#define BYLINS_STAT_H

class CharacterData;
class ObjectData;

extern const char *pc_class_types[];

void do_stat(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_stat_character(CharacterData *ch, CharacterData *k, int virt);
void do_stat_object(CharacterData *ch, ObjectData *j, int virt);
void do_stat_room(CharacterData *ch, int rnum);

#endif //BYLINS_STAT_H
