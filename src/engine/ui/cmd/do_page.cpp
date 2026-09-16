/**
\file do_page.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include <fmt/format.h>

#include "engine/entities/char_data.h"
#include "administration/privilege.h"
#include "engine/network/descriptor_data.h"
#include "engine/core/target_resolver.h"

void do_page(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d;
	CharData *vict;

	char name[kMaxInputLength];
	char message[kMaxStringLength];
	half_chop(argument, name, message);

	if (ch->IsNpc())
		SendMsgToChar("Создания-не-персонажи этого не могут.. ступайте.\r\n", ch);
	else if (!*name)
		SendMsgToChar("Whom do you wish to page?\r\n", ch);
	else {
		// В ветке "page all" в эфир уходил глобальный buf, который в этой функции никто
		// не заполнял: всем игрокам рассылалось то, что осталось в буфере от чужого кода.
		// Пробел после *$n* был в исходном sprintf и потерялся при переводе на поток.
		const std::string page_text = fmt::format("\007\007*$n* {}", message);
		if (!str_cmp(name, "all") || !str_cmp(name, "все")) {
			if (privilege::IsGrGod(ch)) {
				for (d = descriptor_list; d; d = d->next) {
					if (d->state == EConState::kPlaying && d->character) {
						act(page_text, false, ch, nullptr, d->character.get(), kToVict);
					}
				}
			} else {
				SendMsgToChar("Это доступно только БОГАМ!\r\n", ch);
			}
			return;
		}
		vict = target_resolver::FindCharInWorld(ch, name);
		if ((vict != nullptr)) {
			act(page_text, false, ch, nullptr, vict, kToVict);
			if (ch->IsFlagged(EPrf::kNoRepeat))
				SendMsgToChar(CommonMsg(ECommonMsg::kOk) + "\r\n", ch);
			else
				act(page_text, false, ch, nullptr, vict, kToChar);
		} else
			SendMsgToChar("Такой игрок отсутствует!\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
