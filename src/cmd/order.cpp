#include "order.h"

#include "handler.h"

// ****************** CHARM ORDERS PROCEDURES
void do_order(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char name[kMaxInputLength], message[kMaxInputLength];
	bool found = false;
	RoomRnum org_room;
	CharacterData *vict;

	if (!ch)
		return;

	half_chop(argument, name, message);
	if (GET_GOD_FLAG(ch, GF_GODSCURSE)) {
		send_to_char("Вы прокляты Богами и никто не слушается вас!\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED)) {
		send_to_char("Вы не в состоянии приказывать сейчас.\r\n", ch);
		return;
	}
	if (!*name || !*message)
		send_to_char("Приказать что и кому?\r\n", ch);
	else if (!(vict = get_char_vis(ch, name, FIND_CHAR_ROOM)) &&
		!utils::IsAbbrev(name, "followers") && !utils::IsAbbrev(name, "все") && !utils::IsAbbrev(name, "всем"))
		send_to_char("Вы не видите такого персонажа.\r\n", ch);
	else if (ch == vict && !utils::IsAbbrev(name, "все") && !utils::IsAbbrev(name, "всем"))
		send_to_char("Вы начали слышать императивные голоса - срочно к психиатру!\r\n", ch);
	else {
		if (vict && !IS_NPC(vict) && !IS_GOD(ch)) {
			send_to_char(ch, "Игрокам приказывать могут только Боги!\r\n");
			return;
		}

		if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)) {
			send_to_char("В таком состоянии вы не можете сами отдавать приказы.\r\n", ch);
			return;
		}

		if (vict
			&& !utils::IsAbbrev(name, "все")
			&& !utils::IsAbbrev(name, "всем")
			&& !utils::IsAbbrev(name, "followers")) {
			sprintf(buf, "$N приказал$g вам '%s'", message);
			act(buf, false, vict, 0, ch, TO_CHAR | CHECK_DEAF);
			act("$n отдал$g приказ $N2.", false, ch, 0, vict, TO_ROOM | CHECK_DEAF);

			if (vict->get_master() != ch
				|| !(AFF_FLAGGED(vict, EAffectFlag::AFF_CHARM) || AFF_FLAGGED(vict, EAffectFlag::AFF_HELPER))
				|| AFF_FLAGGED(vict, EAffectFlag::AFF_DEAFNESS)) {
				if (!IS_POLY(vict)) {
					act("$n безразлично смотрит по сторонам.", false, vict, 0, 0, TO_ROOM);
				} else {
					act("$n безразлично смотрят по сторонам.", false, vict, 0, 0, TO_ROOM);
				}
			} else {
				send_to_char(OK, ch);
				if (vict->get_wait() <= 0) {
					command_interpreter(vict, message);
				} else if (vict->get_fighting()) {
					if (vict->last_comm != nullptr) {
						free(vict->last_comm);
					}
					vict->last_comm = str_dup(message);
				}
			}
		} else {
			org_room = ch->in_room;
			act("$n отдал$g приказ.", false, ch, 0, 0, TO_ROOM | CHECK_DEAF);

			CharacterData::followers_list_t followers = ch->get_followers_list();

			for (const auto follower : followers) {
				if (org_room != follower->in_room) {
					continue;
				}

				if ((AFF_FLAGGED(follower, EAffectFlag::AFF_CHARM) || AFF_FLAGGED(follower, EAffectFlag::AFF_HELPER))
					&& !AFF_FLAGGED(follower, EAffectFlag::AFF_DEAFNESS)) {
					found = true;
					if (follower->get_wait() <= 0) {
						command_interpreter(follower, message);
					} else if (follower->get_fighting()) {
						if (follower->last_comm != nullptr) {
							free(follower->last_comm);
						}
						follower->last_comm = str_dup(message);
					}
				}
			}

			if (found) {
				send_to_char(OK, ch);
			} else {
				send_to_char("Вы страдаете манией величия!\r\n", ch);
			}
		}
	}
}
