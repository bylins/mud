/**
\file do_spellstat.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 27.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/statistics/spell_usage.h"
#include "engine/ui/modify.h"

void DoPageSpellStat(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	skip_spaces(&argument);

	if (!*argument) {
		SendMsgToChar("заклстат [стоп|старт|очистить|показать|сохранить]\r\n", ch);
		return;
	}

	if (!str_cmp(argument, "старт")) {
		SpellUsage::is_active = true;
		SpellUsage::start = time(nullptr);
		SendMsgToChar("Сбор включен.\r\n", ch);
		return;
	}

	if (!SpellUsage::is_active) {
		SendMsgToChar("Сбор выключен. Включите сбор 'заклстат старт'.\r\n", ch);
		return;
	}

	if (!str_cmp(argument, "стоп")) {
		SpellUsage::Clear();
		SpellUsage::is_active = false;
		SendMsgToChar("Сбор выключен.\r\n", ch);
		return;
	}

	if (!str_cmp(argument, "показать")) {
		page_string(ch->desc, SpellUsage::StatToPrint());
		return;
	}

	if (!str_cmp(argument, "очистить")) {
		SpellUsage::Clear();
		return;
	}

	if (!str_cmp(argument, "сохранить")) {
		SpellUsage::Save();
		return;
	}

	SendMsgToChar("заклстат: неизвестный аргумент\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
