#ifndef BYLINS_TRACK_H
#define BYLINS_TRACK_H

#include "skills.h"

class CHAR_DATA;

int go_track(CHAR_DATA *ch, CHAR_DATA *victim, const ESkill skill_no);
void do_track(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_hidetrack(CHAR_DATA *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_TRACK_H
