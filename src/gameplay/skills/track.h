#ifndef BYLINS_TRACK_H
#define BYLINS_TRACK_H

#include "skills.h"

class CharData;

constexpr Bitvector TRACK_NPC = 1 << 0;
constexpr Bitvector TRACK_HIDE = 1 << 1;

int go_track(CharData *ch, CharData *victim, ESkill skill_no);
void do_track(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_hidetrack(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_TRACK_H
