/**
\file do_page.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/network/descriptor_data.h"
#include "engine/core/handler.h"

void do_page(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d;
	CharData *vict;

	half_chop(argument, arg, buf2);

	if (ch->IsNpc())
		SendMsgToChar("Создания-не-персонажи этого не могут.. ступайте.\r\n", ch);
	else if (!*arg)
		SendMsgToChar("Whom do you wish to page?\r\n", ch);
	else {
		std::stringstream buffer;
		buffer << "\007\007*$n*" << buf2;
//		sprintf(buf, "\007\007*$n* %s", buf2);
		if (!str_cmp(arg, "all") || !str_cmp(arg, "все")) {
			if (IS_GRGOD(ch)) {
				for (d = descriptor_list; d; d = d->next) {
					if (STATE(d) == CON_PLAYING && d->character) {
						act(buf, false, ch, nullptr, d->character.get(), kToVict);
					}
				}
			} else {
				SendMsgToChar("Это доступно только БОГАМ!\r\n", ch);
			}
			return;
		}
		if ((vict = get_char_vis(ch, arg, EFind::kCharInWorld)) != nullptr) {
			act(buffer.str().c_str(), false, ch, nullptr, vict, kToVict);
			if (ch->IsFlagged(EPrf::kNoRepeat))
				SendMsgToChar(OK, ch);
			else
				act(buffer.str().c_str(), false, ch, nullptr, vict, kToChar);
		} else
			SendMsgToChar("Такой игрок отсутствует!\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
