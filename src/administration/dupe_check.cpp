/**
 \file dupe_check.cpp - a part of the Bylins engine.
 \brief issue.interpreter-cleaning (Bucket 2): same-IP / same-email duplicate-login detection,
        extracted from interpreter.cpp.
*/
#include "administration/dupe_check.h"

#include "administration/proxy.h"
#include "administration/privilege.h"
#include "engine/network/descriptor_data.h"
#include "engine/entities/char_data.h"
#include "engine/core/comm.h"
#include "engine/db/db.h"
#include "utils/utils.h"
#include "utils/utils_string.h"

// Проверяет допустимость подключения с данного IP.
// Возвращает 1 если подключение разрешено, 0 если нет.
// Порядок проверок:
//   1. Бессмертные - всегда OK
//   2. Нет дубликатов IP - OK
//   3. Есть proxy запись - проверяем лимит (даже для зарегистрированных)
//   4. Нет proxy записи - проверяем регистрацию персонажа/email
//   5. Не зарегистрирован - в комнату незарегов
int check_dupes_host(DescriptorData *d, bool autocheck) {
	if (!d->character || privilege::IsImmortal(d->character.get()) || d->character->desc->original) {
		return 1;
	}

	if (CountPlayersFromIp(d) == 0) {
		return 1;
	}

	auto proxy_result = CheckProxy(d);

	switch (proxy_result) {
		case EProxyCheck::kLimitReached:
			if (autocheck) {
				return 1;
			}
			SendMsgToChar("&RС вашего IP адреса находится максимально допустимое количество игроков.\r\n"
						  "Обратитесь к Богам для увеличения лимита игроков с вашего адреса.&n",
						  d->character.get());
			return 0;

		case EProxyCheck::kAllowed:
			return 1;

		case EProxyCheck::kNotInList:
			break;
	}

	if (!autocheck) {
		if (RegisterSystem::IsRegistered(d->character.get())) {
			return 1;
		}
		if (RegisterSystem::IsRegisteredEmail(GET_EMAIL(d->character))) {
			d->registered_email = true;
			return 1;
		}
	}

	if (d->character->in_room == r_unreg_start_room
		|| d->character->get_was_in_room() == r_unreg_start_room) {
		return 0;
	}

	for (auto *i = descriptor_list; i; i = i->next) {
		if (i != d && i->ip == d->ip && i->character && !privilege::IsImmortal(i->character.get())
			&& (i->state == EConState::kPlaying || i->state == EConState::kMenu)) {
			SendMsgToChar(d->character.get(),
						  "&RВы вошли с игроком %s с одного IP(%s)!\r\n"
						  "Вам необходимо обратиться к Богам для регистрации.\r\n"
						  "Пока вы будете помещены в комнату для незарегистрированных игроков.&n\r\n",
						  GET_PAD(i->character, 4), i->host);
			sprintf(buf,
					"! ВХОД С ОДНОГО IP ! незарегистрированного игрока.\r\n"
					"Вошел - %s, в игре - %s, IP - %s.\r\n"
					"Игрок помещен в комнату незарегистрированных игроков.",
					GET_NAME(d->character), GET_NAME(i->character), d->host);
			mudlog(buf, NRM, MAX(kLvlImmortal, GET_INVIS_LEV(d->character)), SYSLOG, true);
			break;
		}
	}
	return 0;
}

int check_dupes_email(DescriptorData *d) {
	if (!d->character
		|| privilege::IsImmortal(d->character.get())) {
		return (1);
	}

	for (const auto &ch : character_list) {
		if (ch == d->character
			|| ch->IsNpc()) {
			continue;
		}

		if (!privilege::IsImmortal(ch.get())
			&& (!str_cmp(GET_EMAIL(ch), GET_EMAIL(d->character)))) {
			sprintf(buf, "Персонаж с таким email уже находится в игре, вы не можете войти одновременно с ним!");
			SendMsgToChar(buf, d->character.get());
			return (0);
		}
	}

	return 1;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
