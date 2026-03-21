/**
\file do_display.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 18.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"

const char *DISPLAY_HELP = "Формат: статус { { Ж | Э | З | В | Д | У | О | Б | П | К } | все | нет }\r\n";

void set_display_bits(CharData *ch, bool flag);

void do_display(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc()) {
		SendMsgToChar("И зачем это монстру? Не юродствуйте.\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	if (!*argument) {
		SendMsgToChar(DISPLAY_HELP, ch);
		return;
	}

	if (!str_cmp(argument, "on") || !str_cmp(argument, "all") ||
		!str_cmp(argument, "вкл") || !str_cmp(argument, "все")) {
		set_display_bits(ch, true);
	} else if (!str_cmp(argument, "off")
		|| !str_cmp(argument, "none")
		|| !str_cmp(argument, "выкл")
		|| !str_cmp(argument, "нет")) {
		set_display_bits(ch, false);
	} else {
		set_display_bits(ch, false);

		const size_t len = strlen(argument);
		for (size_t i = 0; i < len; i++) {
			switch (LOWER(argument[i])) {
				case 'h':
				case 'ж': ch->SetFlag(EPrf::kDispHp);
					break;
				case 'w':
				case 'з': ch->SetFlag(EPrf::kDispMana);
					break;
				case 'm':
				case 'э': ch->SetFlag(EPrf::kDispMove);
					break;
				case 'e':
				case 'в': ch->SetFlag(EPrf::kDispExits);
					break;
				case 'g':
				case 'д': ch->SetFlag(EPrf::kDispMoney);
					break;
				case 'l':
				case 'у': ch->SetFlag(EPrf::kDispLvl);
					break;
				case 'x':
				case 'о': ch->SetFlag(EPrf::kDispExp);
					break;
				case 'б':
				case 'f': ch->SetFlag(EPrf::kDispFight);
					break;
				case 'п':
				case 't': ch->SetFlag(EPrf::kDispTimed);
					break;
				case 'к':
				case 'c': ch->SetFlag(EPrf::kDispCooldowns);
					break;
				case ' ': break;
				default: SendMsgToChar(DISPLAY_HELP, ch);
					return;
			}
		}
	}

	SendMsgToChar(OK, ch);
}

void set_display_bits(CharData *ch, bool flag) {
	if (flag) {
		ch->SetFlag(EPrf::kDispHp);
		ch->SetFlag(EPrf::kDispMana);
		ch->SetFlag(EPrf::kDispMove);
		ch->SetFlag(EPrf::kDispExits);
		ch->SetFlag(EPrf::kDispMoney);
		ch->SetFlag(EPrf::kDispLvl);
		ch->SetFlag(EPrf::kDispExp);
		ch->SetFlag(EPrf::kDispFight);
		ch->SetFlag(EPrf::kDispCooldowns);
		if (!IS_IMMORTAL(ch)) {
			ch->SetFlag(EPrf::kDispTimed);
		}
	} else {
		ch->UnsetFlag(EPrf::kDispHp);
		ch->UnsetFlag(EPrf::kDispMana);
		ch->UnsetFlag(EPrf::kDispMove);
		ch->UnsetFlag(EPrf::kDispExits);
		ch->UnsetFlag(EPrf::kDispMoney);
		ch->UnsetFlag(EPrf::kDispLvl);
		ch->UnsetFlag(EPrf::kDispExp);
		ch->UnsetFlag(EPrf::kDispFight);
		ch->UnsetFlag(EPrf::kDispTimed);
		ch->UnsetFlag(EPrf::kDispCooldowns);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
