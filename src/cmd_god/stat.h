#ifndef BYLINS_STAT_H
#define BYLINS_STAT_H
#include <string>

class CharData;
class ObjData;

std::string print_special(CharData *mob);
void do_stat(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_stat_character(CharData *ch, CharData *k, int virt);
void do_stat_object(CharData *ch, ObjData *j, int virt);
void do_stat_room(CharData *ch, int rnum);

#endif //BYLINS_STAT_H
