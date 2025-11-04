/**
\file deviate.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_DEVIATE_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_DEVIATE_H_

#include "gameplay/fight/fight_hit.h"

class CharData;
void GoDeviate(CharData *ch);
void DoDeviate(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);
void ProcessDeviate(CharData *ch, CharData *victim, HitData &hit_data);

#endif //BYLINS_SRC_GAMEPLAY_SKILLS_DEVIATE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
