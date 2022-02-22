#include "chopoff.h"

#include "fightsystem/pk.h"
#include "fightsystem/common.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/fight_start.h"
#include "utils/random.h"
#include "color.h"
#include "structs/global_objects.h"

// ************************* CHOPOFF PROCEDURES
void go_chopoff(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		send_to_char("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	if (ch->isHorsePrevents())
		return;

	if ((GET_POS(vict) < EPosition::kFight)) {
		if (number(1, 100) < ch->get_skill(ESkill::kUndercut)) {
			send_to_char("Вы приготовились провести подсечку, но вовремя остановились.\r\n", ch);
			ch->setSkillCooldown(ESkill::kUndercut, kPulseViolence / 6);
			return;
		}
	}

	if (!pk_agro_action(ch, vict))
		return;

	int percent = number(1, MUD::Skills()[ESkill::kUndercut].difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kUndercut, vict);

	if (check_spell_on_player(ch, kSpellWeb)) {
		prob /= 3;
	}
	if (GET_GOD_FLAG(ch, GF_GODSLIKE)
		|| GET_MOB_HOLD(vict)
		|| GET_GOD_FLAG(vict, GF_GODSCURSE)) {
		prob = percent;
	}

	if (GET_GOD_FLAG(ch, GF_GODSCURSE) ||
		GET_GOD_FLAG(vict, GF_GODSLIKE) ||
		vict->ahorse() || GET_POS(vict) < EPosition::kFight || MOB_FLAGGED(vict, MOB_NOTRIP) || IS_IMMORTAL(vict))
		prob = 0;

	bool success = percent <= prob;
	TrainSkill(ch, ESkill::kUndercut, success, vict);
	SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kUndercut].name, percent, prob, success);
	if (!success) {
		sprintf(buf, "%sВы попытались подсечь $N3, но упали сами...%s", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		act(buf, false, ch, nullptr, vict, kToChar);
		act("$n попытал$u подсечь вас, но упал$g сам$g.", false, ch, nullptr, vict, kToVict);
		act("$n попытал$u подсечь $N3, но упал$g сам$g.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		GET_POS(ch) = EPosition::kSit;
		prob = 3;
		if (can_use_feat(ch, EVASION_FEAT)) {
			Affect<EApplyLocation> af;
			af.type = kSpellExpedient;
			af.location = EApplyLocation::APPLY_PR;
			af.modifier = 50;
			af.duration = CalcDuration(ch, 3, 0, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			affect_join(ch, af, false, false, false, false);
			af.location = EApplyLocation::APPLY_AR;
			affect_join(ch, af, false, false, false, false);
			af.location = EApplyLocation::APPLY_MR;
			affect_join(ch, af, false, false, false, false);
			send_to_char(ch,
						 "%sВы покатились по земле, пытаясь избежать атак %s.%s\r\n",
						 CCIGRN(ch, C_NRM),
						 GET_PAD(vict, 1),
						 CCNRM(ch, C_NRM));
			act("$n покатил$u по земле, пытаясь избежать ваших атак.", false, ch, nullptr, vict, kToVict);
			act("$n покатил$u по земле, пытаясь избежать атак $N1.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		}
	} else {
		send_to_char(ch,
					 "%sВы провели подсечку, ловко усадив %s на землю.%s\r\n",
					 CCIBLU(ch, C_NRM),
					 GET_PAD(vict, 3),
					 CCNRM(ch, C_NRM));
		act("$n ловко подсек$q вас, усадив на попу.", false, ch, nullptr, vict, kToVict);
		act("$n ловко подсек$q $N3, уронив $S на землю.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		SetWait(vict, 3, false);

		if (ch->isInSameRoom(vict)) {
			GET_POS(vict) = EPosition::kSit;
		}

		if (IS_HORSE(vict) && vict->get_master()->ahorse()) {
			vict->drop_from_horse();
		}
		prob = 1;
	}

	appear(ch);
	if (IS_NPC(vict) && CAN_SEE(vict, ch) && vict->have_mind() && vict->get_wait() <= 0) {
		set_hit(vict, ch);
	}

	if (!success) {
		SetWait(ch, prob, false);
	} else {
		SetSkillCooldownInFight(ch, ESkill::kUndercut, prob);
		SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
	}
}

void do_chopoff(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_skill(ESkill::kUndercut) < 1) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kUndercut)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (ch->ahorse()) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы собираетесь подсечь?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы можете воспользоваться командой <сесть>.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !ch->get_fighting())
		go_chopoff(ch, vict);
	else if (IsHaveNoExtraAttack(ch)) {
		if (!IS_NPC(ch))
			act("Хорошо. Вы попытаетесь подсечь $N3.", false, ch, nullptr, vict, kToChar);
		ch->set_extra_attack(kExtraAttackUndercut, vict);
	}
}
