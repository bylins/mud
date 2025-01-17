#include "stupor.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_hit.h"
#include "parry.h"
#include "protect.h"

// ************************* STUPOR PROCEDURES
void go_stupor(CharData *ch, CharData *victim) {
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
		SET_AF_BATTLE(ch, kEafOverwhelm);
		hit(ch, victim, ESkill::kOverwhelm, fight::kMainHand);
		//set_wait(ch, 2, true);
		if (ch->getSkillCooldown(ESkill::kOverwhelm) > 0) {
			SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
		}
	} else {
		act("Вы попытаетесь оглушить $N3.", false, ch, nullptr, victim, kToChar);
		if (ch->GetEnemy() != victim) {
			stop_fighting(ch, false);
			SetFighting(ch, victim);
			//set_wait(ch, 2, true);
			SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
		}
		SET_AF_BATTLE(ch, kEafOverwhelm);
	}
}

void do_stupor(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->GetSkill(ESkill::kOverwhelm) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого вы хотите оглушить?\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

    do_stupor(ch, vict);
}

void do_stupor(CharData *ch, CharData *vict) {
	if (ch->GetSkill(ESkill::kOverwhelm) < 1) {
		log("ERROR: вызов глуша для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}

	if (ch->HasCooldown(ESkill::kOverwhelm)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (vict == ch) {
		SendMsgToChar("Вы громко заорали, заглушая свой собственный голос.\r\n", ch);
		return;
	}

	parry_override(ch);

	go_stupor(ch, vict);
}
