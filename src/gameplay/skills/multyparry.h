/**
\file multyparry.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 05.11.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_MULTYPARRY_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_MULTYPARRY_H_

#include "gameplay/fight/fight_hit.h"

class CharData;
void do_multyparry(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void ProcessMultyparry(CharData *ch, CharData *victim, HitData &hit_data);

#endif //BYLINS_SRC_GAMEPLAY_SKILLS_MULTYPARRY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
