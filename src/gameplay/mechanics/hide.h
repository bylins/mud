/**
\file hide.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 29.10.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_MECHANICS_HIDE_H_
#define BYLINS_SRC_GAMEPLAY_MECHANICS_HIDE_H_

class CharData;
int SkipHiding(CharData *ch, CharData *vict);
int SkipCamouflage(CharData *ch, CharData *vict);
int SkipSneaking(CharData *ch, CharData *vict);

#endif //BYLINS_SRC_GAMEPLAY_MECHANICS_HIDE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
