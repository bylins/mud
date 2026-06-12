/**
\file do_money.h - a part of the Bylins engine.
\authors Created by Claude.
\brief The "money"/"п╢п╣п╫я▄пЁп╦" command: lists every currency the character holds.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_DO_MONEY_H_
#define BYLINS_SRC_ENGINE_UI_CMD_DO_MONEY_H_

class CharData;

void do_money(CharData *ch, char *argument, int cmd, int subcmd);

#endif  // BYLINS_SRC_ENGINE_UI_CMD_DO_MONEY_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
