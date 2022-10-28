#include "chopoff.h"

#include "game_fight/pk.h"
#include "game_fight/common.h"
#include "game_fight/fight_hit.h"
#include "game_fight/fight_start.h"
#include "utils/random.h"
#include "color.h"
#include "structs/global_objects.h"

// ************************* CHOPOFF PROCEDURES
void go_chopoff(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (PRF_FLAGS(ch).get(EPrf::kIronWind)) {
		SendMsgToChar("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	if (ch->IsHorsePrevents())
		return;

	if ((GET_POS(vict) < EPosition::kFight)) {
		if (number(1, 100) < ch->GetSkill(ESkill::kUndercut)) {
			SendMsgToChar("Вы приготовились провести подсечку, но вовремя остановились.\r\n", ch);
			ch->setSkillCooldown(ESkill::kUndercut, kBattleRound / 6);
			return;
		}
	}

	if (!pk_agro_action(ch, vict))
		return;

	int percent = number(1, MUD::Skill(ESkill::kUndercut).difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kUndercut, vict);

	if (IsAffectedBySpell(ch, ESpell::kWeb)) {
		prob /= 3;
	}
	if (GET_GOD_FLAG(ch, EGf::kGodsLike)
		|| AFF_FLAGGED(vict, EAffect::kHold)
		|| GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		prob = percent;
	}

	if (GET_GOD_FLAG(ch, EGf::kGodscurse) ||
		GET_GOD_FLAG(vict, EGf::kGodsLike) ||
		vict->IsOnHorse() || GET_POS(vict) < EPosition::kFight || MOB_FLAGGED(vict, EMobFlag::kNoUndercut) || IS_IMMORTAL(
		vict))
		prob = 0;

	bool success = percent <= prob;
	TrainSkill(ch, ESkill::kUndercut, success, vict);
	SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kUndercut).name, percent, prob, success);
	if (!success) {
		sprintf(buf, "%sВы попытались подсечь $N3, но упали сами...%s", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
		act(buf, false, ch, nullptr, vict, kToChar);
		act("$n попытал$u подсечь вас, но упал$g сам$g.", false, ch, nullptr, vict, kToVict);
		act("$n попытал$u подсечь $N3, но упал$g сам$g.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		GET_POS(ch) = EPosition::kSit;
		prob = 3;
		if (CanUseFeat(ch, EFeat::kEvasion)) {
			Affect<EApply> af;
			af.type = ESpell::kExpedient;
			af.location = EApply::kPhysicResist;
			af.modifier = 50;
			af.duration = CalcDuration(ch, 3, 0, 0, 0, 0);
			af.battleflag = kAfBattledec;
			ImposeAffect(ch, af, false, false, false, false);
			af.location = EApply::kAffectResist;
			ImposeAffect(ch, af, false, false, false, false);
			af.location = EApply::kMagicResist;
			ImposeAffect(ch, af, false, false, false, false);
			SendMsgToChar(ch,
						  "%sВы покатились по земле, пытаясь избежать атак %s.%s\r\n",
						  CCIGRN(ch, C_NRM),
						  GET_PAD(vict, 1),
						  CCNRM(ch, C_NRM));
			act("$n покатил$u по земле, пытаясь избежать ваших атак.", false, ch, nullptr, vict, kToVict);
			act("$n покатил$u по земле, пытаясь избежать атак $N1.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		}
	} else {
		SendMsgToChar(ch,
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

		if (IS_HORSE(vict) && vict->get_master()->IsOnHorse()) {
			vict->drop_from_horse();
		}
		prob = 1;
	}

	appear(ch);
	if (vict->IsNpc() && CAN_SEE(vict, ch) && vict->have_mind() && vict->get_wait() <= 0) {
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
	if (ch->GetSkill(ESkill::kUndercut) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kUndercut)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (ch->IsOnHorse()) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого вы собираетесь подсечь?\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar("Вы можете воспользоваться командой <сесть>.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !ch->GetEnemy())
		go_chopoff(ch, vict);
	else if (IsHaveNoExtraAttack(ch)) {
		if (!ch->IsNpc())
			act("Хорошо. Вы попытаетесь подсечь $N3.", false, ch, nullptr, vict, kToChar);
		ch->SetExtraAttack(kExtraAttackUndercut, vict);
	}
}
