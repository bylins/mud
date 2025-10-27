/**
\file do_switch.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_SWITCH_H_
#define BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_SWITCH_H_

class CharData;
void DoSwitch(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);
void DoReturn(CharData *ch, char *argument, int cmd, int subcmd);

#endif //BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_SWITCH_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
