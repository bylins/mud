/**
\file do_quest.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/ui/cmd/do_quest.h"
#include "engine/core/comm.h"

void do_quest(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	SendMsgToChar("У Вас нет никаких ежедневных поручений.\r\nЧтобы взять новые, наберите &Wпоручения получить&n.\r\n",
				  ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
