#ifndef BYLINS_TRACK_H
#define BYLINS_TRACK_H

#include "game_skills/skills.h"

class CharData;

int go_track(CharData *ch, CharData *victim, const ESkill skill_no);
void do_track(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_hidetrack(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_TRACK_H
