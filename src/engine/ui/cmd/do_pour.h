/**
\file do_pour.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 28.10.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_DO_POUR_H_
#define BYLINS_SRC_ENGINE_UI_CMD_DO_POUR_H_

inline constexpr int kScmdPour{0};
inline constexpr int kScmdFill{1};

class CharData;
void do_pour(CharData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_SRC_ENGINE_UI_CMD_DO_POUR_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
