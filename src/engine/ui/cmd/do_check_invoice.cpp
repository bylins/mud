/**
\file do_check_invoice.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/core/comm.h"
#include "gameplay/communication/check_invoice.h"

void DoCheckInvoice(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (!login_change_invoice(ch)) {
		SendMsgToChar("Проверка показала: новых сообщений нет.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
