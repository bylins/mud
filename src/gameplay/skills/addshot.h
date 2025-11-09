/**
\file addshot.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 06.11.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_ADDSHOT_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_ADDSHOT_H_

#include "gameplay/skills/skills.h"
#include "gameplay/fight/fight_constants.h"

class CharData;
void ProcessMultyShotHits(CharData *ch, CharData *victim, ESkill type, fight::AttackType weapon);

#endif //BYLINS_SRC_GAMEPLAY_SKILLS_ADDSHOT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
