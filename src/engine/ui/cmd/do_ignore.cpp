/**
\file do_ignore.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 21.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "utils/utils_string.h"
#include "utils/utils.h"
#include "engine/core/comm.h"
#include "engine/ui/color.h"
#include "gameplay/communication/ignores.h"

void ignore_usage(CharData *ch);

void do_ignore(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];
	char arg3[kMaxInputLength];
	unsigned int mode = 0, list_empty = 1, all = 0, flag = 0;
	long vict_id;
	char buf[256], buf1[256], name[50];

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

// криво попросили -- выведем справку
	if (arg1[0] && (!arg2[0] || !arg3[0])) {
		ignore_usage(ch);
		return;
	}
// при вызове без параметров выведем весь список
	if (!arg1[0] && !arg2[0] && !arg3[0]) {
		sprintf(buf, "%sВы игнорируете следующих персонажей:%s\r\n", kColorWht, kColorNrm);
		SendMsgToChar(buf, ch);
		for (const auto &ignore : ch->get_ignores()) {
			if (!ignore->id)
				continue;
			if (ignore->id == -1) {
				strcpy(name, "Все");
			} else {
				strcpy(name, ign_find_name(ignore->id));
				name[0] = UPPER(name[0]);
			}
			sprintf(buf, "  %s: ", name);
			SendMsgToChar(buf, ch);
			mode = ignore->mode;
			SendMsgToChar(text_ignore_modes(mode, buf), ch);
			SendMsgToChar("\r\n", ch);
			list_empty = 0;
		}

		if (list_empty) {
			SendMsgToChar("  список пуст.\r\n", ch);
		}
		return;
	}

	if (utils::IsAbbr(arg2, "все"))
		all = 1;
	else if (utils::IsAbbr(arg2, "сказать"))
		flag = EIgnore::kTell;
	else if (utils::IsAbbr(arg2, "говорить"))
		flag = EIgnore::kSay;
	else if (utils::IsAbbr(arg2, "шептать"))
		flag = EIgnore::kWhisper;
	else if (utils::IsAbbr(arg2, "спросить"))
		flag = EIgnore::kAsk;
	else if (utils::IsAbbr(arg2, "эмоция"))
		flag = EIgnore::kEmote;
	else if (utils::IsAbbr(arg2, "кричать"))
		flag = EIgnore::kShout;
	else if (utils::IsAbbr(arg2, "болтать"))
		flag = EIgnore::kGossip;
	else if (utils::IsAbbr(arg2, "орать"))
		flag = EIgnore::kHoller;
	else if (utils::IsAbbr(arg2, "группа"))
		flag = EIgnore::kGroup;
	else if (utils::IsAbbr(arg2, "дружина"))
		flag = EIgnore::kClan;
	else if (utils::IsAbbr(arg2, "союзники"))
		flag = EIgnore::kAlliance;
	else if (utils::IsAbbr(arg2, "оффтоп"))
		flag = EIgnore::kOfftop;
	else {
		ignore_usage(ch);
		return;
	}

// имени "все" соответствует id -1
	if (utils::IsAbbr(arg1, "все")) {
		vict_id = -1;
	} else {
		// убедимся, что добавляемый чар на данный момент существует
		// и он не имм, а заодно получим его id
		switch (ign_find_id(arg1, &vict_id)) {
			case 0: SendMsgToChar("Нельзя игнорировать Богов, это плохо кончится.\r\n", ch);
				return;
			case -1: SendMsgToChar("Нет такого персонажа, уточните имя.\r\n", ch);
				return;
		}
	}

// ищем жертву в списке
	ignore_data::shared_ptr ignore = nullptr;
	for (const auto &ignore_i : ch->get_ignores()) {
		if (ignore_i->id == vict_id) {
			ignore = ignore_i;
			break;
		}
	}

	if (utils::IsAbbr(arg3, "добавить")) {
// создаем новый элемент списка в хвосте, если не нашли
		if (!ignore) {
			const auto cur = std::make_shared<ignore_data>();
			cur->id = vict_id;
			cur->mode = 0;
			ch->add_ignore(cur);
			ignore = cur;
		}
		mode = ignore->mode;
		if (all) {
			SET_BIT(mode, EIgnore::kTell);
			SET_BIT(mode, EIgnore::kSay);
			SET_BIT(mode, EIgnore::kWhisper);
			SET_BIT(mode, EIgnore::kAsk);
			SET_BIT(mode, EIgnore::kEmote);
			SET_BIT(mode, EIgnore::kShout);
			SET_BIT(mode, EIgnore::kGossip);
			SET_BIT(mode, EIgnore::kHoller);
			SET_BIT(mode, EIgnore::kGroup);
			SET_BIT(mode, EIgnore::kClan);
			SET_BIT(mode, EIgnore::kAlliance);
			SET_BIT(mode, EIgnore::kOfftop);
		} else {
			SET_BIT(mode, flag);
		}
		ignore->mode = mode;
	} else if (utils::IsAbbr(arg3, "убрать")) {
		if (!ignore || ignore->id != vict_id) {
			if (vict_id == -1) {
				SendMsgToChar("Вы и так не игнорируете всех сразу.\r\n", ch);
			} else {
				strcpy(name, ign_find_name(vict_id));
				name[0] = UPPER(name[0]);
				sprintf(buf,
						"Вы и так не игнорируете "
						"персонажа %s%s%s.\r\n", kColorWht, name, kColorNrm);
				SendMsgToChar(buf, ch);
			}
			return;
		}
		mode = ignore->mode;
		if (all) {
			mode = 0;
		} else {
			REMOVE_BIT(mode, flag);
		}
		ignore->mode = mode;
		if (!mode)
			ignore->id = 0;
	} else {
		ignore_usage(ch);
		return;
	}
	if (mode) {
		if (ignore->id == -1) {
			sprintf(buf, "Для всех сразу вы игнорируете:%s.\r\n", text_ignore_modes(ignore->mode, buf1));
			SendMsgToChar(buf, ch);
		} else {
			strcpy(name, ign_find_name(ignore->id));
			name[0] = UPPER(name[0]);
			sprintf(buf, "Для персонажа %s%s%s вы игнорируете:%s.\r\n",
					kColorWht, name, kColorNrm, text_ignore_modes(ignore->mode, buf1));
			SendMsgToChar(buf, ch);
		}
	} else {
		if (vict_id == -1) {
			SendMsgToChar("Вы больше не игнорируете всех сразу.\r\n", ch);
		} else {
			strcpy(name, ign_find_name(vict_id));
			name[0] = UPPER(name[0]);
			sprintf(buf, "Вы больше не игнорируете персонажа %s%s%s.\r\n",
					kColorWht, name, kColorNrm);
			SendMsgToChar(buf, ch);
		}
	}
}

void ignore_usage(CharData *ch) {
	SendMsgToChar("Формат команды: игнорировать <имя|все> <режим|все> <добавить|убрать>\r\n"
				  "Доступные режимы:\r\n"
				  "  сказать говорить шептать спросить эмоция кричать\r\n"
				  "  болтать орать группа дружина союзники\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
