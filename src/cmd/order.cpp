#include "order.h"

#include "handler.h"

// ****************** CHARM ORDERS PROCEDURES
void do_order(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	char name[kMaxInputLength], message[kMaxInputLength];
	bool found = false;
	RoomRnum org_room;
	CharData *vict;

	if (!ch)
		return;

	half_chop(argument, name, message);
	if (GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		SendMsgToChar("Вы прокляты Богами и никто не слушается вас!\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kSilence)) {
		SendMsgToChar("Вы не в состоянии приказывать сейчас.\r\n", ch);
		return;
	}
	if (!*name || !*message)
		SendMsgToChar("Приказать что и кому?\r\n", ch);
	else if (!(vict = get_char_vis(ch, name, EFind::kCharInRoom)) &&
		!utils::IsAbbr(name, "followers") && !utils::IsAbbr(name, "все") && !utils::IsAbbr(name, "всем"))
		SendMsgToChar("Вы не видите такого персонажа.\r\n", ch);
	else if (ch == vict && !utils::IsAbbr(name, "все") && !utils::IsAbbr(name, "всем"))
		SendMsgToChar("Вы начали слышать императивные голоса - срочно к психиатру!\r\n", ch);
	else {
		if (vict && !vict->IsNpc() && !IS_GOD(ch)) {
			SendMsgToChar(ch, "Игрокам приказывать могут только Боги!\r\n");
			return;
		}

		if (AFF_FLAGGED(ch, EAffect::kCharmed)) {
			SendMsgToChar("В таком состоянии вы не можете сами отдавать приказы.\r\n", ch);
			return;
		}

		if (vict
			&& !utils::IsAbbr(name, "все")
			&& !utils::IsAbbr(name, "всем")
			&& !utils::IsAbbr(name, "followers")) {
			sprintf(buf, "$N приказал$g вам '%s'", message);
			act(buf, false, vict, 0, ch, kToChar | kToNotDeaf);
			act("$n отдал$g приказ $N2.", false, ch, 0, vict, kToRoom | kToNotDeaf);

			if (vict->get_master() != ch
				|| !(AFF_FLAGGED(vict, EAffect::kCharmed) || AFF_FLAGGED(vict, EAffect::kHelper))
				|| AFF_FLAGGED(vict, EAffect::kDeafness)) {
				if (!IS_POLY(vict)) {
					act("$n безразлично смотрит по сторонам.", false, vict, 0, 0, kToRoom);
				} else {
					act("$n безразлично смотрят по сторонам.", false, vict, 0, 0, kToRoom);
				}
			} else {
				SendMsgToChar(OK, ch);
				if (vict->get_wait() <= 0) {
					command_interpreter(vict, message);
				} else if (vict->GetEnemy()) {
					vict->last_comm = message;
				}
			}
		} else {
			org_room = ch->in_room;
			act("$n отдал$g приказ.", false, ch, 0, 0, kToRoom | kToNotDeaf);

			CharData::followers_list_t followers = ch->get_followers_list();

			for (const auto follower : followers) {
				if (org_room != follower->in_room) {
					continue;
				}

				if ((AFF_FLAGGED(follower, EAffect::kCharmed) || AFF_FLAGGED(follower, EAffect::kHelper))
					&& !AFF_FLAGGED(follower, EAffect::kDeafness)) {
					found = true;
					if (follower->get_wait() <= 0) {
						command_interpreter(follower, message);
					} else if (follower->GetEnemy()) {
						follower->last_comm = message;
					}
				}
			}

			if (found) {
				SendMsgToChar(OK, ch);
			} else {
				SendMsgToChar("Вы страдаете манией величия!\r\n", ch);
			}
		}
	}
}
