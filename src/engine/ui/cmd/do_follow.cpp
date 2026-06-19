#include "do_follow.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/mechanics/follow.h"

#include "gameplay/fight/fight.h"
#include "engine/core/handler.h"
#include "engine/core/target_resolver.h"
#include "engine/db/global_objects.h"
#include "utils/backtrace.h"

void PerformDropGold(CharData *ch, int amount);




void do_follow(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *leader;
	one_argument(argument, smallBuf);

	if (ch->IsNpc() && AFF_FLAGGED(ch, EAffect::kCharmed) && ch->GetEnemy())
		return;
	if (*smallBuf) {
		if (!str_cmp(smallBuf, "я") || !str_cmp(smallBuf, "self") || !str_cmp(smallBuf, "me")) {
			if (!ch->has_master()) {
				SendMsgToChar("Но вы ведь ни за кем не следуете...\r\n", ch);
			} else {
				follow::StopFollower(ch, follow::kSfEmpty);
			}
			return;
		}
		leader = target_resolver::FindCharInRoom(ch, smallBuf);
		if (!leader) {
			SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
			return;
		}
	} else {
		SendMsgToChar("За кем вы хотите следовать?\r\n", ch);
		return;
	}

	if (ch->get_master() == leader) {
		act("Вы уже следуете за $N4.", false, ch, 0, leader, kToChar);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kCharmed)
		&& ch->has_master()) {
		act("Но вы можете следовать только за $N4!", false, ch, 0, ch->get_master(), kToChar);
	} else        // Not Charmed follow person
	{
		if (leader == ch) {
			if (!ch->has_master()) {
				SendMsgToChar("Вы уже следуете за собой.\r\n", ch);
				return;
			}
			follow::StopFollower(ch, follow::kSfEmpty);
		} else {
			if (follow::CircleFollow(ch, leader)) {
				SendMsgToChar("Так у вас целый хоровод получится.\r\n", ch);
				return;
			}

			if (ch->has_master()) {
				follow::StopFollower(ch, follow::kSfEmpty);
			}
			//AFF_FLAGS(ch).unset(EAffectFlag::AFF_GROUP);
			ch->removeGroupFlags();
			for (auto *f : ch->followers) {
				//AFF_FLAGS(f->ch).unset(EAffectFlag::AFF_GROUP);
				f->removeGroupFlags();
			}

			follow::AddFollower(leader, ch);
		}
	}
}
