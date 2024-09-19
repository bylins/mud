/**
\file do_invisible.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/ui/cmd/do_visible.h"

void perform_immort_invis(CharData *ch, int level);

extern void appear(CharData *ch);

void do_invis(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int level;

	if (ch->IsNpc()) {
		SendMsgToChar("Вы не можете сделать этого.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg) {
		if (GET_INVIS_LEV(ch) > 0)
			perform_immort_vis(ch);
		else {
			if (GetRealLevel(ch) < kLvlImplementator)
				perform_immort_invis(ch, kLvlImmortal);
			else
				perform_immort_invis(ch, GetRealLevel(ch));
		}
	} else {
		level = MIN(atoi(arg), kLvlImplementator);
		if (level > GetRealLevel(ch) && !ch->IsFlagged(EPrf::kCoderinfo))
			SendMsgToChar("Вы не можете достичь невидимости выше вашего уровня.\r\n", ch);
		else if (GetRealLevel(ch) < kLvlImplementator && level > kLvlImmortal && !ch->IsFlagged(EPrf::kCoderinfo))
			perform_immort_invis(ch, kLvlImmortal);
		else if (level < 1)
			perform_immort_vis(ch);
		else
			perform_immort_invis(ch, level);
	}
}

void perform_immort_invis(CharData *ch, int level) {
	if (ch->IsNpc()) {
		return;
	}

	for (const auto tch : world[ch->in_room]->people) {
		if (tch == ch) {
			continue;
		}

		if (GetRealLevel(tch) >= GET_INVIS_LEV(ch) && GetRealLevel(tch) < level) {
			act("Вы вздрогнули, когда $n растворил$u на ваших глазах.",
				false, ch, nullptr, tch, kToVict);
		}

		if (GetRealLevel(tch) < GET_INVIS_LEV(ch) && GetRealLevel(tch) >= level) {
			act("$n медленно появил$u из пустоты.",
				false, ch, nullptr, tch, kToVict);
		}
	}

	SET_INVIS_LEV(ch, level);
	sprintf(buf, "Ваш уровень невидимости - %d.\r\n", level);
	SendMsgToChar(buf, ch);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
