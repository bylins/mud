#include "entities/char_data.h"
#include "handler.h"
#include "utils/utils_char_obj.inl"

void do_mark(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char arg1[kMaxInputLength];
	char arg2[kMaxInputLength];

	int cont_dotmode, found = 0;
	ObjData *cont;
	CharData *tmp_char;

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		SendMsgToChar("Что вы хотите маркировать?\r\n", ch);
	} else if (!*arg2 || !is_number(arg2)) {
		SendMsgToChar("Не указан или неверный маркер.\r\n", ch);
	} else {
		cont_dotmode = find_all_dots(arg1);
		if (cont_dotmode == kFindIndiv) {
			generic_find(arg1, EFind::kObjInventory | EFind::kObjRoom | EFind::kObjEquip, ch, &tmp_char, &cont);
			if (!cont) {
				sprintf(buf, "У вас нет '%s'.\r\n", arg1);
				SendMsgToChar(buf, ch);
				return;
			}
			cont->set_owner(atoi(arg2));
			act("Вы пометили $o3.", false, ch, cont, nullptr, kToChar);
		} else {
			if (cont_dotmode == kFindAlldot && !*arg1) {
				SendMsgToChar("Пометить что \"все\"?\r\n", ch);
				return;
			}
			for (cont = ch->carrying; cont; cont = cont->get_next_content()) {
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == kFindAll
						|| isname(arg1, cont->get_aliases()))) {
					cont->set_owner(atoi(arg2));
					act("Вы пометили $o3.", false, ch, cont, nullptr, kToChar);
					found = true;
				}
			}
			for (cont = world[ch->in_room]->contents; cont; cont = cont->get_next_content()) {
				if (CAN_SEE_OBJ(ch, cont)
					&& (cont_dotmode == kFindAll
						|| isname(arg2, cont->get_aliases()))) {
					cont->set_owner(atoi(arg2));
					act("Вы пометили $o3.", false, ch, cont, nullptr, kToChar);
					found = true;
				}
			}
			if (!found) {
				if (cont_dotmode == kFindAll) {
					SendMsgToChar("Вы не смогли найти ничего для маркировки.\r\n", ch);
				} else {
					sprintf(buf, "Вы что-то не видите здесь '%s'.\r\n", arg1);
					SendMsgToChar(buf, ch);
				}
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
