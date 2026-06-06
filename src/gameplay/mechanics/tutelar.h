/**
\file tutelar.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 05.11.2025.
\brief Призыв ангела-хранителя и его функции спасения и лечения.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_TUTELAR_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_TUTELAR_H_

class CharData;
void SummonTutelar(CharData *ch);
void CheckTutelarSelfSacrfice(CharData *ch, CharData *victim);
void TryToRescueWithTutelar(CharData *ch);

#endif //BYLINS_SRC_GAMEPLAY_SKILLS_TUTELAR_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
