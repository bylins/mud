/**
\file leadership.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_LEADERSHIP_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_LEADERSHIP_H_

class CharData;
int CalcLeadership(CharData *ch);
int CalcLeadershipGroupExpKoeff(CharData *leader, int inroom_members, int koeff);
int CalcLeadershipGroupSizeBonus(CharData *leader);
void UpdateLeadership(CharData *ch, CharData *killer);

#endif //BYLINS_SRC_GAMEPLAY_SKILLS_LEADERSHIP_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
