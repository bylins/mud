/**
\file intercept.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_INTERCEPT_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_INTERCEPT_H_

class CharData;
void DoIntercept(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void ProcessIntercept(CharData *ch, CharData *vict, int *dam);

#endif //BYLINS_SRC_GAMEPLAY_SKILLS_INTERCEPT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
