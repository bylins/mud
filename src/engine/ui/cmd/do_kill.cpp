#include "do_kill.h"

#include "gameplay/fight/fight.h"
#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/skills/protect.h"
#include "gameplay/fight/common.h"

void DoHit(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Вы не видите цели.\r\n", ch);
		return;
	}
	if (vict == ch) {
		SendMsgToChar("Вы ударили себя... Ведь больно же!\r\n", ch);
		act("$n ударил$g себя, и громко завопил$g 'Мамочка, больно ведь...'",
			false,
			ch,
			0,
			vict,
			kToRoom | kToNotDeaf | kToArenaListen);
		act("$n ударил$g себя", false, ch, 0, vict, kToRoom | kToDeaf | kToArenaListen);
		return;
	}
	if (!may_kill_here(ch, vict, argument)) {
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kCharmed) && (ch->get_master() == vict)) {
		act("$N слишком дорог для вас, чтобы бить $S.", false, ch, 0, vict, kToChar);
		return;
	}

	if (subcmd != kScmdMurder && !check_pkill(ch, vict, arg)) {
		return;
	}

	if (ch->GetEnemy()) {
		if (vict == ch->GetEnemy()) {
			act("Вы уже сражаетесь с $N4.", false, ch, 0, vict, kToChar);
			return;
		}
		if (ch != vict->GetEnemy()) {
			act("$N не сражается с вами, не трогайте $S.", false, ch, 0, vict, kToChar);
			return;
		}
		vict = TryToFindProtector(vict, ch);
		stop_fighting(ch, 2);
		SetFighting(ch, vict);
		SetWait(ch, 2, true);
		//ch->setSkillCooldown(kGlobalCooldown, 2);
		return;
	}
	if ((ch->GetPosition() == EPosition::kStand) && (vict != ch->GetEnemy())) {
		set_hit(ch, vict);
	} else {
		SendMsgToChar("Вам явно не до боя!\r\n", ch);
	}
}

void DoKill(CharData *ch, char *argument, int cmd, int subcmd) {
	if (!IS_GRGOD(ch)) {
		DoHit(ch, argument, cmd, subcmd);
		return;
	};
	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого вы жизни лишить хотите-то?\r\n", ch);
		return;
	}
	if (ch == vict) {
		SendMsgToChar("Вы мазохист... :(\r\n", ch);
		return;
	};
	if (IS_IMPL(vict) || vict->IsFlagged(EPrf::kCoderinfo)) {
		SendMsgToChar("А если он вас чайником долбанет? Думай, Господи, думай!\r\n", ch);
	} else {
		act("Вы обратили $N3 в прах! Взглядом! Одним!", false, ch, 0, vict, kToChar);
		act("$N обратил$g вас в прах своим ненавидящим взором!", false, vict, 0, ch, kToChar);
		act("$n просто испепелил$g взглядом $N3!", false, ch, 0, vict, kToNotVict | kToArenaListen);
		raw_kill(vict, ch);
	};
};
