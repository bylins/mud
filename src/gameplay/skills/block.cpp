#include "block.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight_hit.h"

// ******************* BLOCK PROCEDURES
void go_block(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kStopLeft)) {
		SendMsgToChar("Ваша рука парализована.\r\n", ch);
		return;
	}
	SET_AF_BATTLE(ch, kEafBlock);
	SendMsgToChar("Хорошо, вы попробуете отразить щитом следующую атаку.\r\n", ch);
}

void do_block(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kShieldBlock)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kShieldBlock)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (!ch->GetEnemy()) {
		SendMsgToChar("Но вы ни с кем не сражаетесь!\r\n", ch);
		return;
	};
	if (!(ch->IsNpc()
		|| GET_EQ(ch, kShield)
		|| IS_IMMORTAL(ch)
		|| GET_GOD_FLAG(ch, EGf::kGodsLike))) {
		SendMsgToChar("Вы не можете сделать это без щита.\r\n", ch);
		return;
	}
	if (GET_AF_BATTLE(ch, kEafBlock)) {
		SendMsgToChar("Вы уже прикрываетесь щитом!\r\n", ch);
		return;
	}
	go_block(ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
