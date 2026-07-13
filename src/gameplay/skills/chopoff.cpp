#include "chopoff.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/mount.h"

#include "skill_messages.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/ui/cmd/do_kill.h"
#include "utils/random.h"
#include "engine/ui/color.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/sight.h"

// issue.chardata-cleaning: was the global CLEAR_MIND (only used here).
static bool ClearMind(const CharData *ch) {
	return !ch->battle_affects.get(kEafOverwhelm) && !ch->battle_affects.get(kEafHammer);
}

// issue.chardata-cleaning: was CharData::have_mind (only used here): an autonomous NPC,
// not a charmed pet or a ridden horse, retaliates against a missed chopoff.
static bool IsAutonomous(const CharData *ch) {
	return !AFF_FLAGGED(ch, EAffect::kCharmed) && !mount::IsHorse(ch);
}


// ************************* CHOPOFF PROCEDURES
void go_chopoff(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kChopoff, ESkillMsg::kCantFightNow) + "\r\n", ch);
		return;
	}

	if (ch->IsFlagged(EPrf::kIronWind)) {
		SendMsgToChar("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	if (mount::IsBlockedByHorse(ch))
		return;

	if ((vict->GetPosition() < EPosition::kFight)) {
		if (number(1, 100) < GetSkill(ch, ESkill::kChopoff)) {
			SendMsgToChar("Вы приготовились провести подсечку, но вовремя остановились.\r\n", ch);
			ch->setSkillCooldown(ESkill::kChopoff, kBattleRound / 6);
			return;
		}
	}

	if (!pk_agro_action(ch, vict))
		return;

	int percent = number(1, MUD::Skill(ESkill::kChopoff).difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kChopoff, vict);

	if (IsAffectedWithFlag(ch, kAfEntanglement)) {
		prob /= 3;
	}
	if (GET_GOD_FLAG(ch, EGf::kGodsLike)
		|| AFF_FLAGGED(vict, EAffect::kHold)
		|| GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		prob = percent;
	}

	if (GET_GOD_FLAG(ch, EGf::kGodscurse) ||
		GET_GOD_FLAG(vict, EGf::kGodsLike) ||
		mount::IsOnHorse(vict) || vict->GetPosition() < EPosition::kFight || vict->IsFlagged(EMobFlag::kNoUndercut) || privilege::IsImmortal(vict))
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
			af.affect_type = EAffect::kEvade;
			af.location = EApply::kPhysicResist;
			af.modifier = 50;
			af.duration = CalcDuration(ch, ch, ESkill::kUndefined, 3, 0, 0, 0);
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
		if (mount::IsHorse(vict) && mount::IsOnHorse(vict->get_master())) {
			CharData *tch = vict->get_master();
			act("$n ловко подсек$q $N3, заставив $S споткнуться.", true, ch, nullptr, vict, kToNotVict | kToArenaListen);
			SendMsgToChar(ch, "%sВы провели подсечку, заставив %s споткнуться.%s\r\n",
						  kColorBoldBlu, GET_PAD(vict, 3), kColorNrm);
			percent = number(1, MUD::Skill(ESkill::kRiding).difficulty);
			prob = GetSkill(tch, ESkill::kRiding);
			if (percent < prob) {
				SendMsgToChar(tch, "Вы смогли удержаться на спине своего скакуна.\r\n");
			} else {
				mount::DropFromHorse(vict);
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
				mount::DropFromHorse(vict);
			}
		}
		prob = 1;
	}
	sight::Appear(ch);
	if (!success) {
		SetWait(ch, prob, false);
		if (vict->IsNpc() && sight::CanSee(vict, ch) && IsAutonomous(vict) && ClearMind(vict) && !vict->GetEnemy()) {
			hit(vict, ch, ESkill::kUndefined, AFF_FLAGGED(vict, EAffect::kStopRight) ? fight::kOffHand : fight::kMainHand);
		}
	} else {
		SetSkillCooldownInFight(ch, ESkill::kChopoff, prob);
		SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
	}
}

void do_chopoff(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (GetSkill(ch, ESkill::kChopoff) < 1) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kChopoff, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kChopoff, ESkillMsg::kNoTarget) + "\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	do_chopoff(ch, vict);
}

void do_chopoff(CharData *ch, CharData *vict) {
	if (GetSkill(ch, ESkill::kChopoff) < 1) {
		log("ERROR: вызов подножки для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}

	if (ch->Skills().HasActiveCooldown(ESkill::kChopoff)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kChopoff, ESkillMsg::kOnCooldown) + "\r\n", ch);
		return;
	};

	if (mount::IsOnHorse(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kChopoff, ESkillMsg::kCantWhileMounted) + "\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kChopoff, ESkillMsg::kCantTargetSelf) + "\r\n", ch);
		return;
	}

	if (privilege::IsImpl(ch) || !ch->GetEnemy())
		go_chopoff(ch, vict);
	else if (IsHaveNoExtraAttack(ch)) {
		if (!ch->IsNpc())
			act("Хорошо. Вы попытаетесь подсечь $N3.", false, ch, nullptr, vict, kToChar);
		ch->SetExtraAttack(kExtraAttackChopoff, vict);
	}
}
