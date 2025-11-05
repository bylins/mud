/**
\file punctual_style.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_PUNCTUAL_STYLE_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_PUNCTUAL_STYLE_H_

#include "gameplay/fight/fight_hit.h"

class CharData;
class ObjData;
void ProcessPunctualStyle(CharData *ch, CharData *victim, HitData &hit_data);
[[nodiscard]] int CalcPunctualCritDmg(CharData *ch, CharData * /*victim*/, ObjData *wielded);

#endif //BYLINS_SRC_GAMEPLAY_SKILLS_PUNCTUAL_STYLE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
