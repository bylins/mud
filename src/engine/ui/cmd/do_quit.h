/**
\file do_quit.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_DO_QUIT_H_
#define BYLINS_SRC_ENGINE_UI_CMD_DO_QUIT_H_

const int kScmdQuit{1};

class CharData;
void do_quit(CharData *ch, char *argument, int/* cmd*/, int subcmd);

#endif //BYLINS_SRC_ENGINE_UI_CMD_DO_QUIT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
