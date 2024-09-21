#include "engine/db/obj_prototypes.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/fight/pk.h"
#include "engine/core/utils_char_obj.inl"

// чтоб не абузили длину. персональные пофиг, а клановые не надо.
const int kMaxLabelLength = 32;

void DoSign(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];

	char arg3[kMaxInputLength];
	char arg4[kMaxInputLength];

	ObjData *target = nullptr;
	int erase_only = 0; // 0 -- наносим новую метку, 1 -- удаляем старую
	int clan = 0; // клан режим, если единица. персональный, если не
	int no_target = 0; // красиво сделать не выйдет, будем через флаги

	char *objname = nullptr;
	char *labels = nullptr;

	if (ch->IsNpc())
		return;

	half_chop(argument, arg1, arg2);

	if (!strlen(arg1))
		no_target = 1;
	else {
		if (!strcmp(arg1, "клан")) { // если в arg1 "клан", то в arg2 ищем название объекта и метки
			clan = 1;
			if (strlen(arg2)) {
				half_chop(arg2, arg3, arg4); // в arg3 получаем название объекта
				objname = str_dup(arg3);
				if (strlen(arg4))
					labels = str_dup(arg4);
				else
					erase_only = 1;
			} else {
				no_target = 1;
			}
		} else { // слова "клан" не нашли, значит, ожидаем в arg1 сразу имя объекта и метки в arg2
			if (strlen(arg1)) {
				objname = str_dup(arg1);
				if (strlen(arg2))
					labels = str_dup(arg2);
				else
					erase_only = 1;
			} else {
				no_target = 1;
			}
		}
	}

	if (no_target) {
		SendMsgToChar("На чем царапаем?\r\n", ch);
	} else {
		if (!(target = get_obj_in_list_vis(ch, objname, ch->carrying))) {
			sprintf(buf, "У вас нет \'%s\'.\r\n", objname);
			SendMsgToChar(buf, ch);
		} else {
			if (erase_only) {
				target->remove_custom_label();
				act("Вы затерли надписи на $o5.", false, ch, target, nullptr, kToChar);
			} else if (labels) {
				if (strlen(labels) > kMaxLabelLength)
					labels[kMaxLabelLength] = '\0';

				// убираем тильды
				for (int i = 0; labels[i] != '\0'; i++)
					if (labels[i] == '~')
						labels[i] = '-';

				std::shared_ptr<custom_label> label(new custom_label());
				label->text_label = str_dup(labels);
				label->author = ch->get_uid();
				label->author_mail = str_dup(GET_EMAIL(ch));

				const char *msg = "Вы покрыли $o3 каракулями, которые никто кроме вас не разберет.";
				if (clan && ch->player_specials->clan) {
					label->clan_abbrev = str_dup(ch->player_specials->clan->GetAbbrev());
					msg = "Вы покрыли $o3 каракулями, понятными разве что вашим соратникам.";
				}
				target->set_custom_label(label);
				act(msg, false, ch, target, nullptr, kToChar);
			}
		}
	}

	if (objname) {
		free(objname);
	}

	if (labels) {
		free(labels);
	}
}

// для использования с чарами:
// возвращает метки предмета, если они есть и смотрящий является их автором или является членом соотв. клана
std::string char_get_custom_label(ObjData *obj, CharData *ch) {
	const char *delim_l = nullptr;
	const char *delim_r = nullptr;

	// разные скобки для клановых и личных
	if (obj->get_custom_label() && (ch->player_specials->clan && obj->get_custom_label()->clan_abbrev != nullptr &&
		is_alliance_by_abbr(ch, obj->get_custom_label()->clan_abbrev))) {
		delim_l = " *";
		delim_r = "*";
	} else {
		delim_l = " (";
		delim_r = ")";
	}

	std::stringstream buffer;
	if (AUTH_CUSTOM_LABEL(obj, ch)) {
		buffer << delim_l << obj->get_custom_label()->text_label << delim_r;
	}

	return buffer.str();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
