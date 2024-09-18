/**
\file wiznet.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 15.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/network/descriptor_data.h"
#include "engine/ui/color.h"
#include "gameplay/communication/remember.h"

void do_wiznet(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	DescriptorData *d;
	char emote = false;
	char bookmark1 = false;
	char bookmark2 = false;
	int level = kLvlGod;

	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument) {
		SendMsgToChar
			("Формат: wiznet <text> | #<level> <text> | *<emotetext> |\r\n "
			 "        wiznet @<level> *<emotetext> | wiz @\r\n", ch);
		return;
	}

	//	if (EPrf::FLAGGED(ch, EPrf::CODERINFO)) return;

	// Опускаем level для gf_demigod
	if (GET_GOD_FLAG(ch, EGf::kDemigod))
		level = kLvlImmortal;

	// использование доп. аргументов
	switch (*argument) {
		case '*': emote = true;
			break;
		case '#':
			// Установить уровень имм канала
			one_argument(argument + 1, buf1);
			if (is_number(buf1)) {
				half_chop(argument + 1, buf1, argument);
				level = std::max(atoi(buf1), kLvlImmortal);
				if (level > GetRealLevel(ch)) {
					SendMsgToChar("Вы не можете изрекать выше вашего уровня.\r\n", ch);
					return;
				}
			} else if (emote)
				argument++;
			break;
		case '@':
			// Обнаруживаем всех кто может (теоретически) нас услышать
			for (d = descriptor_list; d; d = d->next) {
				if (STATE(d) == CON_PLAYING &&
					(IS_IMMORTAL(d->character) || GET_GOD_FLAG(d->character, EGf::kDemigod)) &&
					!d->character->IsFlagged(EPrf::kNoWiz) && (CAN_SEE(ch, d->character) || IS_IMPL(ch))) {
					if (!bookmark1) {
						strcpy(buf1,
							   "Боги/привилегированные которые смогут (наверное) вас услышать:\r\n");
						bookmark1 = true;
					}
					sprintf(buf1 + strlen(buf1), "  %s", GET_NAME(d->character));
					if (d->character->IsFlagged(EPlrFlag::kWriting))
						strcat(buf1, " (пишет)\r\n");
					else if (d->character->IsFlagged(EPlrFlag::kMailing))
						strcat(buf1, " (пишет письмо)\r\n");
					else
						strcat(buf1, "\r\n");
				}
			}
			for (d = descriptor_list; d; d = d->next) {
				if (STATE(d) == CON_PLAYING &&
					(IS_IMMORTAL(d->character) || GET_GOD_FLAG(d->character, EGf::kDemigod)) &&
					d->character->IsFlagged(EPrf::kNoWiz) && CAN_SEE(ch, d->character)) {
					if (!bookmark2) {
						if (!bookmark1)
							strcpy(buf1,
								   "Боги/привилегированные которые не смогут вас услышать:\r\n");
						else
							strcat(buf1,
								   "Боги/привилегированные которые не смогут вас услышать:\r\n");

						bookmark2 = true;
					}
					sprintf(buf1 + strlen(buf1), "  %s\r\n", GET_NAME(d->character));
				}
			}
			SendMsgToChar(buf1, ch);

			return;
		default: break;
	}
	if (ch->IsFlagged(EPrf::kNoWiz)) {
		SendMsgToChar("Вы вне игры!\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	if (!*argument) {
		SendMsgToChar("Не думаю, что Боги одобрят это.\r\n", ch);
		return;
	}
	if (level != kLvlGod) {
		sprintf(buf1, "%s%s: <%d> %s%s\r\n", GET_NAME(ch),
				emote ? "" : " богам", level, emote ? "<--- " : "", argument);
	} else {
		sprintf(buf1, "%s%s: %s%s\r\n", GET_NAME(ch), emote ? "" : " богам", emote ? "<--- " : "", argument);
	}
	snprintf(buf2, kMaxStringLength, "&c%s&n", buf1);
	Remember::add_to_flaged_cont(Remember::wiznet_, buf2, level);

	// пробегаемся по списку дескрипторов чаров и кто должен - тот услышит богов
	for (d = descriptor_list; d; d = d->next) {
		if ((STATE(d) == CON_PLAYING) &&    // персонаж должен быть в игре
			((GetRealLevel(d->character) >= level) ||    // уровень равным или выше level
				(GET_GOD_FLAG(d->character, EGf::kDemigod) && level == 31)) &&    // демигоды видят 31 канал
			(!d->character->IsFlagged(EPrf::kNoWiz)) &&    // игрок с режимом NOWIZ не видит имм канала
			(!d->character->IsFlagged(EPlrFlag::kWriting)) &&    // пишущий не видит имм канала
			(!d->character->IsFlagged(EPlrFlag::kMailing)))    // отправляющий письмо не видит имм канала
		{
			// отправляем сообщение чару
			snprintf(buf2, kMaxStringLength, "%s%s%s",
					 kColorCyn, buf1, kColorNrm);
			d->character->remember_add(buf2, Remember::ALL);
			// не видино своих мессаг если 'режим repeat'
			if (d != ch->desc
				|| !(d->character->IsFlagged(EPrf::kNoRepeat))) {
				SendMsgToChar(buf2, d->character.get());
			}
		}
	}
	if (ch->IsFlagged(EPrf::kNoRepeat)) {
		SendMsgToChar(OK, ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
