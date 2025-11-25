/**
\file DoGlory.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 26.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "utils/utils_string.h"
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/glory_misc.h"
#include "engine/entities/char_player.h"
#include "engine/core/handler.h"
#include "administration/karma.h"

enum DoGloryMode {
	kShowGlory = 0,
	kAddGlory,
	kSubGlory,
	kSubStats,
	kSubTrans,
	kSubHide
};

void DoGlory(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	// Команда простановки славы (оффлайн/онлайн)
	// Без параметров выводит славу у игрока
	// + cлава прибавляет славу
	// - cлава убавляет славу
	char num[kMaxInputLength];
	char arg1[kMaxInputLength];
	int mode = 0;
	char *reason;

	if (!*argument) {
		SendMsgToChar("Формат команды : \r\n"
					  "   glory <имя> +|-<кол-во славы> причина\r\n"
					  "   glory <имя> remove <кол-во статов> причина (снимание уже вложенной чаром славы)\r\n"
					  "   glory <имя> transfer <имя принимаюего славу> причина (переливание славы на другого чара)\r\n"
					  "   glory <имя> hide on|off причина (показывать или нет чара в топе славы)\r\n", ch);
		return;
	}
	reason = two_arguments(argument, arg, num);
	skip_spaces(&reason);

	if (!*num)
		mode = kShowGlory;
	else if (*num == '+')
		mode = kAddGlory;
	else if (*num == '-')
		mode = kSubGlory;
	else if (utils::IsAbbr(num, "remove")) {
		// тут у нас в num получается remove, в arg1 кол-во и в reason причина
		reason = one_argument(reason, arg1);
		skip_spaces(&reason);
		mode = kSubStats;
	} else if (utils::IsAbbr(num, "transfer")) {
		// а тут в num transfer, в arg1 имя принимающего славу и в reason причина
		reason = one_argument(reason, arg1);
		skip_spaces(&reason);
		mode = kSubTrans;
	} else if (utils::IsAbbr(num, "hide")) {
		// а тут в num hide, в arg1 on|off и в reason причина
		reason = any_one_arg(reason, arg1);
		skip_spaces(&reason);
		mode = kSubHide;
	}

	// точки убираем, чтобы карма всегда писалась
	skip_dots(&reason);

	if (mode != kShowGlory) {
		if ((reason == nullptr) || (*reason == 0)) {
			SendMsgToChar("Укажите причину изменения славы?\r\n", ch);
			return;
		}
	}

	CharData *vict = get_player_vis(ch, arg, EFind::kCharInWorld);
	Player t_vict; // TODO: надо выносить во вторую функцию, чтобы зря не создавать
	if (!vict) {
		if (LoadPlayerCharacter(arg, &t_vict, ELoadCharFlags::kFindId) < 0) {
			SendMsgToChar("Такого персонажа не существует.\r\n", ch);
			return;
		}
		vict = &t_vict;
	}

	switch (mode) {
		case kAddGlory: {
			int amount = atoi((num + 1));
			Glory::add_glory(vict->get_uid(), amount);
			SendMsgToChar(ch, "%s добавлено %d у.е. славы (Всего: %d у.е.).\r\n",
						  GET_PAD(vict, 2), amount, Glory::get_glory(vict->get_uid()));
			imm_log("(GC) %s sets +%d glory to %s.", GET_NAME(ch), amount, GET_NAME(vict));
			// запись в карму
			sprintf(buf, "Change glory +%d by %s", amount, GET_NAME(ch));
			AddKarma(vict, buf, reason);
			GloryMisc::add_log(mode, amount, std::string(buf), std::string(reason), vict);
			break;
		}
		case kSubGlory: {
			int amount = Glory::remove_glory(vict->get_uid(), atoi((num + 1)));
			if (amount <= 0) {
				SendMsgToChar(ch, "У %s нет свободной славы.", GET_PAD(vict, 1));
				break;
			}
			SendMsgToChar(ch, "У %s вычтено %d у.е. славы (Всего: %d у.е.).\r\n",
						  GET_PAD(vict, 1), amount, Glory::get_glory(vict->get_uid()));
			imm_log("(GC) %s sets -%d glory to %s.", GET_NAME(ch), amount, GET_NAME(vict));
			// запись в карму
			sprintf(buf, "Change glory -%d by %s", amount, GET_NAME(ch));
			AddKarma(vict, buf, reason);
			GloryMisc::add_log(mode, amount, std::string(buf), std::string(reason), vict);
			break;
		}
		case kSubStats: {
			if (Glory::remove_stats(vict, ch, atoi(arg1))) {
				sprintf(buf, "Remove stats %s by %s", arg1, GET_NAME(ch));
				AddKarma(vict, buf, reason);
				GloryMisc::add_log(mode, 0, std::string(buf), std::string(reason), vict);
			}
			break;
		}
		case kSubTrans: {
			Glory::transfer_stats(vict, ch, arg1, reason);
			break;
		}
		case kSubHide: {
			Glory::hide_char(vict, ch, arg1);
			sprintf(buf, "Hide %s by %s", arg1, GET_NAME(ch));
			AddKarma(vict, buf, reason);
			GloryMisc::add_log(mode, 0, std::string(buf), std::string(reason), vict);
			break;
		}
		default: Glory::show_glory(vict, ch);
	}

	vict->save_char();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
