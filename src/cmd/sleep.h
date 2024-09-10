/**
\file sleep.h - a part of Bylins engine.
\authors Created by Sventovit.
\date 08.09.2024.
\brief 'Do sleep' command.
*/

#ifndef BYLINS_SRC_CMD_SLEEP_H_
#define BYLINS_SRC_CMD_SLEEP_H_

class CharData;

void do_sleep(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/);

#endif //BYLINS_SRC_CMD_SLEEP_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
