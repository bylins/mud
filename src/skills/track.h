#ifndef BYLINS_TRACK_H
#define BYLINS_TRACK_H

#include "chars/char.hpp"

#define CALC_TRACK(ch,vict) (calculate_skill(ch,SKILL_TRACK, 0))

int go_track(CHAR_DATA * ch, CHAR_DATA * victim, const ESkill skill_no);
void do_track(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_hidetrack(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_TRACK_H
