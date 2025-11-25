/**
\file do_reply.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 23.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/db/world_characters.h"
#include "gameplay/communication/talk.h"

void do_reply(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc())
		return;

	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar(SIELENCE, ch);
		return;
	}

	if (ch->IsFlagged(EPlrFlag::kDumbed)) {
		SendMsgToChar("Вам запрещено обращаться к другим игрокам!\r\n", ch);
		return;
	}

	skip_spaces(&argument);

	if (ch->get_answer_id() == kNobody)
		SendMsgToChar("Вам некому ответить!\r\n", ch);
	else if (!*argument)
		SendMsgToChar("Что вы собираетесь ответить?\r\n", ch);
	else {
		/*
		 * Make sure the person you're replying to is still playing by searching
		 * for them.  Note, now last tell is stored as player IDnum instead of
		 * a pointer, which is much better because it's safer, plus will still
		 * work if someone logs out and back in again.
		 */

		/*
		 * XXX: A descriptor list based search would be faster, although
		 *      we could not find link dead people.  Not that they can
		 *      hear tells anyway. :) -gg 2/24/98
		 */
		bool found = false;
		for (const auto &i : character_list) {
			if (!i->IsNpc()
				&& i->get_uid() == ch->get_answer_id()) {
				if (is_tell_ok(ch, i.get())) {
					perform_tell(ch, i.get(), argument);
				}

				found = true;

				break;
			}
		}

		if (!found) {
			SendMsgToChar("Этого игрока уже нет в игре.\r\n", ch);
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
