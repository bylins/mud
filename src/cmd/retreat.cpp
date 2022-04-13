#include "retreat.h"
#include "game_fight/fight.h"

// ***************** STOPFIGHT
void do_retreat(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (!ch->GetEnemy() || ch->IsNpc()) {
		SendMsgToChar("Но вы же ни с кем не сражаетесь.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		SendMsgToChar("Из этого положения отступать невозможно.\r\n", ch);
		return;
	}

	if (PRF_FLAGS(ch).get(EPrf::kIronWind) || AFF_FLAGGED(ch, EAffect::kLacky)) {
		SendMsgToChar("Вы не желаете отступать, не расправившись со всеми врагами!\r\n", ch);
		return;
	}

	CharData *tmp_ch = nullptr;
	for (const auto i : world[ch->in_room]->people) {
		if (i->GetEnemy() == ch) {
			tmp_ch = i;
			break;
		}
	}

	if (tmp_ch) {
		SendMsgToChar("Невозможно, вы сражаетесь за свою жизнь.\r\n", ch);
		return;
	} else {
		stop_fighting(ch, true);
		if (!(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike)))
			SetWaitState(ch, kPulseViolence);
		SendMsgToChar("Вы отступили из битвы.\r\n", ch);
		act("$n выбыл$g из битвы.", false, ch, 0, 0, kToRoom | kToArenaListen);
	}
}