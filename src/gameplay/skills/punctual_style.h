/**
\file punctual_style.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_PUNCTUAL_STYLE_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_PUNCTUAL_STYLE_H_

class CharData;
class ObjData;

struct PunctualStyleResult {
  int dmg;
  int dmg_critical;
};

[[nodiscard]] PunctualStyleResult ProcessPunctualHit(CharData *ch, CharData *victim, int dam, int dam_critic);
[[nodiscard]] int CalcPunctualCritDmg(CharData *ch, CharData * /*victim*/, ObjData *wielded);

#endif //BYLINS_SRC_GAMEPLAY_SKILLS_PUNCTUAL_STYLE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
