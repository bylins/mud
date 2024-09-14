#include "mighthit.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/fight/common.h"
#include "parry.h"
#include "protect.h"

// ************************* MIGHTHIT PROCEDURES
void go_mighthit(CharData *ch, CharData *victim) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (ch->IsFlagged(EPrf::kIronWind)) {
		SendMsgToChar("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	victim = TryToFindProtector(victim, ch);

	if (!ch->GetEnemy()) {
		SET_AF_BATTLE(ch, kEafHammer);
		hit(ch, victim, ESkill::kHammer, fight::kMainHand);
		if (ch->getSkillCooldown(ESkill::kHammer) > 0) {
			SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
		}
		//set_wait(ch, 2, true);
		return;
	}

	if ((victim->GetEnemy() != ch) && (ch->GetEnemy() != victim)) {
		act("$N не сражается с вами, не трогайте $S.", false, ch, nullptr, victim, kToChar);
	} else {
		act("Вы попытаетесь нанести богатырский удар по $N2.", false, ch, nullptr, victim, kToChar);
		if (ch->GetEnemy() != victim) {
			stop_fighting(ch, 2);
			SetFighting(ch, victim);
			SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
			//set_wait(ch, 2, true);
		}
		SET_AF_BATTLE(ch, kEafHammer);
	}
}

void do_mighthit(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->GetSkill(ESkill::kHammer) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kHammer)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого вы хотите СИЛЬНО ударить?\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar("Вы СИЛЬНО ударили себя. Но вы и не спали.\r\n", ch);
		return;
	}

	if (GET_AF_BATTLE(ch, kEafTouch)) {
		if (!ch->IsNpc())
			SendMsgToChar("Невозможно. Вы сосредоточены на захвате противника.\r\n", ch);
		return;
	}
	if (!ch->IsNpc() && !IS_IMMORTAL(ch)
		&& (GET_EQ(ch, EEquipPos::kBoths)
			|| GET_EQ(ch, EEquipPos::kWield)
			|| GET_EQ(ch, EEquipPos::kHold)
			|| GET_EQ(ch, EEquipPos::kShield)
			|| GET_EQ(ch, EEquipPos::kLight))) {
		SendMsgToChar("Ваша экипировка мешает вам нанести удар.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	parry_override(ch);

	go_mighthit(ch, vict);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
