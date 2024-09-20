/**
\file do_visible.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/ui/cmd/do_visible.h"

#include "engine/entities/char_data.h"

extern void appear(CharData *ch);

void do_visible(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_IMMORTAL(ch)) {
		perform_immort_vis(ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kInvisible)
		|| AFF_FLAGGED(ch, EAffect::kDisguise)
		|| AFF_FLAGGED(ch, EAffect::kHide)
		|| AFF_FLAGGED(ch, EAffect::kSneak)) {
		appear(ch);
		SendMsgToChar("Вы перестали быть невидимым.\r\n", ch);
	} else
		SendMsgToChar("Вы и так видимы.\r\n", ch);
}

void perform_immort_vis(CharData *ch) {
	if (GET_INVIS_LEV(ch) == 0 &&
		!AFF_FLAGGED(ch, EAffect::kHide) && !AFF_FLAGGED(ch, EAffect::kInvisible)
		&& !AFF_FLAGGED(ch, EAffect::kDisguise)) {
		SendMsgToChar("Ну вот вас и заметили. Стало ли вам легче от этого?\r\n", ch);
		return;
	}

	SET_INVIS_LEV(ch, 0);
	appear(ch);
	SendMsgToChar("Вы теперь полностью видны.\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
