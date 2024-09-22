/**
\file check_invoice.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "gameplay/communication/check_invoice.h"

#include "engine/entities/char_data.h"
#include "gameplay/clans/house.h"
#include "gameplay/communication/boards/boards.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/communication/parcel.h"
#include "gameplay/communication/mail.h"
#include "gameplay/mechanics/title.h"
#include "administration/names.h"

bool single_god_invoice(CharData *ch);

// * Вывод оповещений о новых сообщениях на досках, письмах, (неодобренных имен и титулов для иммов) при логине и релогине
bool login_change_invoice(CharData *ch) {
	bool hasMessages = false;

	hasMessages |= Boards::Static::LoginInfo(ch);

	if (IS_IMMORTAL(ch))
		hasMessages |= single_god_invoice(ch);

	if (mail::has_mail(ch->get_uid())) {
		hasMessages = true;
		SendMsgToChar("&RВас ожидает письмо. ЗАЙДИТЕ НА ПОЧТУ!&n\r\n", ch);
	}
	if (Parcel::has_parcel(ch)) {
		hasMessages = true;
		SendMsgToChar("&RВас ожидает посылка. ЗАЙДИТЕ НА ПОЧТУ!&n\r\n", ch);
	}
	hasMessages |= Depot::show_purged_message(ch);
	if (CLAN(ch)) {
		hasMessages |= CLAN(ch)->print_mod(ch);
	}

	return hasMessages;
}

// * Генерация списка неодобренных титулов и имен и вывод их имму
bool single_god_invoice(CharData *ch) {
	bool hasMessages = false;
	hasMessages |= TitleSystem::show_title_list(ch);
	hasMessages |= NewNames::show(ch);
	return hasMessages;
}

// * Поиск незанятых иммов онлайн для вывода им неодобренных титулов и имен раз в 5 минут
void god_work_invoice() {
	for (DescriptorData *d = descriptor_list; d; d = d->next) {
		if (d->character && STATE(d) == CON_PLAYING) {
			if (IS_IMMORTAL(d->character)
				|| GET_GOD_FLAG(d->character, EGf::kDemigod)) {
				single_god_invoice(d->character.get());
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
