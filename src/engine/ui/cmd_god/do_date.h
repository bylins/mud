/**
\file do_date.h - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#ifndef BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_DATE_H_
#define BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_DATE_H_

#include <sstream>

class CharData;
void PrintUptime(std::ostringstream &out);
void DoPageDateTime(CharData *ch, char * /*argument*/, int/* cmd*/, int subcmd);

#endif //BYLINS_SRC_ENGINE_UI_CMD_GOD_DO_DATE_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
