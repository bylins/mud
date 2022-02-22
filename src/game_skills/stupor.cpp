#include "stupor.h"

#include "fightsystem/pk.h"
#include "fightsystem/common.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "game_skills/parry.h"
#include "protect.h"

// ************************* STUPOR PROCEDURES
void go_stupor(CharData *ch, CharData *victim) {
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
		SET_AF_BATTLE(ch, kEafOverwhelm);
		hit(ch, victim, ESkill::kOverwhelm, fight::kMainHand);
		//set_wait(ch, 2, true);
		if (ch->getSkillCooldown(ESkill::kOverwhelm) > 0) {
			SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
		}
	} else {
		act("Вы попытаетесь оглушить $N3.", false, ch, nullptr, victim, kToChar);
		if (ch->get_fighting() != victim) {
			stop_fighting(ch, false);
			set_fighting(ch, victim);
			//set_wait(ch, 2, true);
			SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
		}
		SET_AF_BATTLE(ch, kEafOverwhelm);
	}
}

void do_stupor(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_skill(ESkill::kOverwhelm) < 1) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kOverwhelm)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы хотите оглушить?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы громко заорали, заглушая свой собственный голос.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	parry_override(ch);

	go_stupor(ch, vict);
}
