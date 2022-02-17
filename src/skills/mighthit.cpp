#include "mighthit.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/common.h"
#include "skills/parry.h"
#include "protect.h"

using namespace FightSystem;

// ************************* MIGHTHIT PROCEDURES
void go_mighthit(CharData *ch, CharData *victim) {
	if (IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		send_to_char("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	victim = TryToFindProtector(victim, ch);

	if (!ch->get_fighting()) {
		SET_AF_BATTLE(ch, kEafHammer);
		hit(ch, victim, ESkill::kHammer, FightSystem::kMainHand);
		if (ch->getSkillCooldown(ESkill::kHammer) > 0) {
			SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
		}
		//set_wait(ch, 2, true);
		return;
	}

	if ((victim->get_fighting() != ch) && (ch->get_fighting() != victim)) {
		act("$N не сражается с вами, не трогайте $S.", false, ch, 0, victim, kToChar);
	} else {
		act("Вы попытаетесь нанести богатырский удар по $N2.", false, ch, 0, victim, kToChar);
		if (ch->get_fighting() != victim) {
			stop_fighting(ch, 2);
			set_fighting(ch, victim);
			SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
			//set_wait(ch, 2, true);
		}
		SET_AF_BATTLE(ch, kEafHammer);
	}
}

void do_mighthit(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_skill(ESkill::kHammer) < 1) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kHammer)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы хотите СИЛЬНО ударить?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы СИЛЬНО ударили себя. Но вы и не спали.\r\n", ch);
		return;
	}

	if (GET_AF_BATTLE(ch, kEafTouch)) {
		if (!IS_NPC(ch))
			send_to_char("Невозможно. Вы сосредоточены на захвате противника.\r\n", ch);
		return;
	}
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
		&& (GET_EQ(ch, WEAR_BOTHS)
			|| GET_EQ(ch, WEAR_WIELD)
			|| GET_EQ(ch, WEAR_HOLD)
			|| GET_EQ(ch, WEAR_SHIELD)
			|| GET_EQ(ch, WEAR_LIGHT))) {
		send_to_char("Ваша экипировка мешает вам нанести удар.\r\n", ch);
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
