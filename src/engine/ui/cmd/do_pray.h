/**
\file do_pray.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_DO_PRAY_H_
#define BYLINS_SRC_ENGINE_UI_CMD_DO_PRAY_H_

#define SCMD_PRAY   0
#define SCMD_DONATE 1

class CharData;
void do_pray(CharData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_SRC_ENGINE_UI_CMD_DO_PRAY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
