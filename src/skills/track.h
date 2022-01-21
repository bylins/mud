#ifndef BYLINS_TRACK_H
#define BYLINS_TRACK_H

#include "skills.h"

class CharacterData;

int go_track(CharacterData *ch, CharacterData *victim, const ESkill skill_no);
void do_track(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void do_hidetrack(CharacterData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_TRACK_H
