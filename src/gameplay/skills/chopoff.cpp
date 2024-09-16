#include "chopoff.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/fight/fight_start.h"
#include "utils/random.h"
#include "engine/ui/color.h"
#include "engine/db/global_objects.h"

// ************************* CHOPOFF PROCEDURES
void go_chopoff(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (ch->IsFlagged(EPrf::kIronWind)) {
		SendMsgToChar("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	if (ch->IsHorsePrevents())
		return;

	if ((vict->GetPosition() < EPosition::kFight)) {
		if (number(1, 100) < ch->GetSkill(ESkill::kChopoff)) {
			SendMsgToChar("Вы приготовились провести подсечку, но вовремя остановились.\r\n", ch);
			ch->setSkillCooldown(ESkill::kChopoff, kBattleRound / 6);
			return;
		}
	}

	if (!pk_agro_action(ch, vict))
		return;

	int percent = number(1, MUD::Skill(ESkill::kChopoff).difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kChopoff, vict);

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
		vict->IsOnHorse() || vict->GetPosition() < EPosition::kFight || vict->IsFlagged(EMobFlag::kNoUndercut) || IS_IMMORTAL(
		vict))
		prob = 0;

	bool success = percent <= prob;
	TrainSkill(ch, ESkill::kChopoff, success, vict);
	SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kChopoff).name, percent, prob, success);
	if (!success) {
		sprintf(buf, "%sВы попытались подсечь $N3, но упали сами...%s", kColorWht, kColorNrm);
		act(buf, false, ch, nullptr, vict, kToChar);
		act("$n попытал$u подсечь вас, но упал$g сам$g.", false, ch, nullptr, vict, kToVict);
		act("$n попытал$u подсечь $N3, но упал$g сам$g.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		ch->SetPosition(EPosition::kSit);
		prob = 3;
		if (CanUseFeat(ch, EFeat::kEvasion)) {
			Affect<EApply> af;
			af.type = ESpell::kExpedient;
			af.location = EApply::kPhysicResist;
			af.modifier = 50;
			af.duration = CalcDuration(ch, 3, 0, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			ImposeAffect(ch, af, false, false, false, false);
			af.location = EApply::kAffectResist;
			ImposeAffect(ch, af, false, false, false, false);
			af.location = EApply::kMagicResist;
			ImposeAffect(ch, af, false, false, false, false);
			SendMsgToChar(ch, "%sВы покатились по земле, пытаясь избежать атак %s.%s\r\n", kColorBoldGrn, GET_PAD(vict, 1), kColorNrm);
			act("$n покатил$u по земле, пытаясь избежать ваших атак.", false, ch, nullptr, vict, kToVict);
			act("$n покатил$u по земле, пытаясь избежать атак $N1.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
		}
	} else {
		if (IS_HORSE(vict) && vict->get_master()->IsOnHorse()) {
			CharData *tch = vict->get_master();
			act("$n ловко подсек$q $N3, заставив $S споткнуться.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
			SendMsgToChar(ch, "%sВы провели подсечку, заставив %s споткнуться.%s\r\n",
						  kColorBoldBlu, GET_PAD(vict, 3), kColorNrm);
			percent = number(1, MUD::Skill(ESkill::kRiding).difficulty);
			prob = tch->GetSkill(ESkill::kRiding);
			if (percent < prob) {
				SendMsgToChar(tch, "Вы смогли удержаться на спине своего скакуна.\r\n");
			} else {
				vict->DropFromHorse();
				SetWait(vict, 1, false); //лошади тоже немнога лагу
			}
		} else {
			SendMsgToChar(ch, "%sВы провели подсечку, ловко усадив %s на землю.%s\r\n",
						  kColorBoldBlu, GET_PAD(vict, 3), kColorNrm);
			act("$n ловко подсек$q вас, усадив на попу.", false, ch, nullptr, vict, kToVict);
			act("$n ловко подсек$q $N3, уронив $S на землю.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
			SetWait(vict, 3, false);
			if (ch->isInSameRoom(vict)) {
				vict->SetPosition(EPosition::kSit);
			}
		}
		prob = 1;
	}
	appear(ch);
	if (!success) {
		SetWait(ch, prob, false);
		if (vict->IsNpc() && CAN_SEE(vict, ch) && vict->have_mind() && CLEAR_MIND(vict) && !vict->GetEnemy()) {
			hit(vict, ch, ESkill::kUndefined, AFF_FLAGGED(vict, EAffect::kStopRight) ? fight::kOffHand : fight::kMainHand);
		}
	} else {
		SetSkillCooldownInFight(ch, ESkill::kChopoff, prob);
		SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
	}
}

void do_chopoff(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->GetSkill(ESkill::kChopoff) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kChopoff)) {
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
		ch->SetExtraAttack(kExtraAttackChopoff, vict);
	}
}
