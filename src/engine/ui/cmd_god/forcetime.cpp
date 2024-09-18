/**
\file forcetime.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 14.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"

void DoForcetime(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int m, t = 0;
	char *ca;

	// Parse command line
	for (ca = strtok(argument, " "); ca; ca = strtok(nullptr, " ")) {
		m = LOWER(ca[strlen(ca) - 1]);
		if (m == 'h')    // hours
			m = 60 * 60;
		else if (m == 'm')    // minutes
			m = 60;
		else if (m == 's')    // seconds
			m = 1;
		else
			m = 0;

		if ((m *= atoi(ca)) > 0)
			t += m;
		else {
			// no time shift with undefined arguments
			t = 0;
			break;
		}
	}

	if (t <= 0) {
		SendMsgToChar("Сдвиг игрового времени (h - часы, m - минуты, s - секунды).\r\n", ch);
		return;
	}

	for (m = 0; m < t * kPassesPerSec; m++) {
		GlobalObjects::heartbeat()(t * kPassesPerSec - m);
	}

	SendMsgToChar(ch, "Вы перевели игровое время на %d сек вперед.\r\n", t);
	sprintf(buf, "(GC) %s перевел игровое время на %d сек вперед.", GET_NAME(ch), t);
	mudlog(buf, NRM, kLvlImmortal, IMLOG, false);
	SendMsgToChar(OK, ch);

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
