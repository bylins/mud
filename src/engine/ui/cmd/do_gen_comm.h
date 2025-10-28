/**
\file do_gen_comm.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_DO_GEN_COMM_H_
#define BYLINS_SRC_ENGINE_UI_CMD_DO_GEN_COMM_H_

inline constexpr int kScmdHoller{0};
inline constexpr int kScmdShout{1};
inline constexpr int kScmdGossip{2};
inline constexpr int kScmdAuction{3};

class CharData;
void do_gen_comm(CharData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_SRC_ENGINE_UI_CMD_DO_GEN_COMM_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
