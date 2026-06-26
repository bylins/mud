#include "mighthit.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/mount.h"
#include "skill_messages.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/fight/common.h"
#include "parry.h"
#include "protect.h"
#include "engine/db/global_objects.h"

void ProcessMighthitBash(CharData *ch, CharData *victim);
void PerformMighthit(CharData *ch, CharData *victim, HitData &hit_data);

void GoMighthit(CharData *ch, CharData *victim) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kHammer, ESkillMsg::kCantFightNow) + "\r\n", ch);
		return;
	}

	if (ch->IsFlagged(EPrf::kIronWind)) {
		SendMsgToChar("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	victim = TryToFindProtector(victim, ch);

	if (!ch->GetEnemy()) {
		ch->battle_affects.set(kEafHammer);
		hit(ch, victim, ESkill::kHammer, fight::kMainHand);
		if (ch->Skills().GetCooldown(ESkill::kHammer) > 0) {
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
		ch->battle_affects.set(kEafHammer);
	}
}

void DoMighthit(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (GetSkill(ch, ESkill::kHammer) < 1) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kHammer, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kHammer, ESkillMsg::kNoTarget) + "\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	DoMighthit(ch, vict);
}

void DoMighthit(CharData *ch, CharData *victim) {
	if (GetSkill(ch, ESkill::kHammer) < 1) {
		log("ERROR: вызов молота для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}

	if (ch->Skills().HasActiveCooldown(ESkill::kHammer)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kHammer, ESkillMsg::kOnCooldown) + "\r\n", ch);
		return;
	};

	if (victim == ch) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kHammer, ESkillMsg::kCantTargetSelf) + "\r\n", ch);
		return;
	}

	if (ch->battle_affects.get(kEafTouch)) {
		if (!ch->IsNpc())
			SendMsgToChar("Невозможно. Вы сосредоточены на захвате противника.\r\n", ch);
		return;
	}
	if (!ch->IsNpc() && !privilege::IsImmortal(ch)
		&& (GET_EQ(ch, EEquipPos::kBoths)
			|| GET_EQ(ch, EEquipPos::kWield)
			|| GET_EQ(ch, EEquipPos::kHold)
			|| GET_EQ(ch, EEquipPos::kShield)
			|| GET_EQ(ch, EEquipPos::kLight))) {
		SendMsgToChar("Ваша экипировка мешает вам нанести удар.\r\n", ch);
		return;
	}

	CheckParryOverride(ch);
	GoMighthit(ch, victim);
}

void ProcessMighthit(CharData *ch, CharData *victim, HitData &hit_data) {
	// в эти условия ничего добавлять не надо, иначе kEafHammer не снимется
	// с моба по ходу боя, если он не может по каким-то причинам смолотить
	if (ch->battle_affects.get(kEafHammer) && ch->get_wait() <= 0) {
		ch->battle_affects.unset(kEafHammer);
		hit_data.SetFlag(fight::kIgnoreBlink);
		if (IsArmedWithMighthitWeapon(ch) && !ch->battle_affects.get(kEafTouch)) {
			PerformMighthit(ch, victim, hit_data);
		}
	}
}

void PerformMighthit(CharData *ch, CharData *victim, HitData &hit_data) {
	int percent = number(1, MUD::Skill(ESkill::kHammer).difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kHammer, victim);
	TrainSkill(ch, ESkill::kHammer, percent <= prob, victim);
	int lag = 0, might = 0;

	if (AFF_FLAGGED(victim, EAffect::kHold)) {
		percent = number(1, 25);
	}

	if (privilege::IsImmortal(victim)) {
		prob = 0;
	}

	SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kHammer).name, percent, prob, percent <= prob);
	if (percent > prob || hit_data.dam == 0) {
		sprintf(buf, "&c&qВаш богатырский удар пропал впустую.&Q&n\r\n");
		SendMsgToChar(buf, ch);
		lag = 3;
		hit_data.dam = 0;
	} else if (victim->IsFlagged(EMobFlag::kNoHammer)) {
		sprintf(buf, "&c&qНа других надо силу проверять!&Q&n\r\n");
		SendMsgToChar(buf, ch);
		lag = 1;
		hit_data.dam = 0;
	} else {
		might = prob * 100 / percent;
		if (might < 180) {
			sprintf(buf, "&b&qВаш богатырский удар задел %s.&Q&n\r\n", sight::PersonName(victim, ch, 3));
			SendMsgToChar(buf, ch);
			lag = 1;
			SetBattleLag(victim, 1);
			Affect<EApply> af;
			af.affect_type = EAffect::kStopFight;
			af.location = EApply::kNone;
			af.modifier = 0;
			af.duration = CalcDuration(victim, victim, ESkill::kUndefined, 1, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			ImposeAffect(victim, af, true, false, true, false);
			sprintf(buf, "&R&qВаше сознание затуманилось после удара %s.&Q&n\r\n", sight::PersonName(ch, victim, 1));
			SendMsgToChar(buf, victim);
			act("$N содрогнул$U от богатырского удара $n1.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
			if (!number(0, 2)) {
				ProcessMighthitBash(ch, victim);
			}
		} else if (might < 800) {
			sprintf(buf, "&g&qВаш богатырский удар пошатнул %s.&Q&n\r\n", sight::PersonName(victim, ch, 3));
			SendMsgToChar(buf, ch);
			lag = 2;
			hit_data.dam += (hit_data.dam / 1);
			SetBattleLag(victim, 2);
			Affect<EApply> af;
			af.affect_type = EAffect::kStopFight;
			af.location = EApply::kNone;
			af.modifier = 0;
			af.duration = CalcDuration(victim, victim, ESkill::kUndefined, 2, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			ImposeAffect(victim, af, true, false, true, false);
			sprintf(buf, "&R&qВаше сознание помутилось после удара %s.&Q&n\r\n", sight::PersonName(ch, victim, 1));
			SendMsgToChar(buf, victim);
			act("$N пошатнул$U от богатырского удара $n1.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
			if (!number(0, 1)) {
				ProcessMighthitBash(ch, victim);
			}
		} else {
			sprintf(buf, "&G&qВаш богатырский удар сотряс %s.&Q&n\r\n", sight::PersonName(victim, ch, 3));
			SendMsgToChar(buf, ch);
			lag = 2;
			hit_data.dam *= 4;
			SetBattleLag(victim, 3);
			Affect<EApply> af;
			af.affect_type = EAffect::kStopFight;
			af.location = EApply::kNone;
			af.modifier = 0;
			af.duration = CalcDuration(victim, victim, ESkill::kUndefined, 3, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			ImposeAffect(victim, af, true, false, true, false);
			sprintf(buf, "&R&qВаше сознание померкло после удара %s.&Q&n\r\n", sight::PersonName(ch, victim, 1));
			SendMsgToChar(buf, victim);
			act("$N зашатал$U от богатырского удара $n1.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
			ProcessMighthitBash(ch, victim);
		}
	}
	//set_wait(ch, lag, true);
	// Временный костыль, чтоб пофиксить лищний раунд КД
	lag = std::max(1, lag - 1);
	SetSkillCooldown(ch, ESkill::kHammer, lag);
}

void ProcessMighthitBash(CharData *ch, CharData *victim) {
	if (victim->IsFlagged(EMobFlag::kNoBash) || !AFF_FLAGGED(victim, EAffect::kHold)) {
		return;
	}

	act("$n обреченно повалил$u на землю.", true, victim, nullptr, nullptr, kToRoom | kToArenaListen);
	SetBattleLag(victim, 3);

	if (victim->GetPosition() > EPosition::kSit) {
		victim->SetPosition(EPosition::kSit);
		mount::DropFromHorse(victim);
		SendMsgToChar(victim, "&R&qБогатырский удар %s сбил вас с ног.&Q&n\r\n", sight::PersonName(ch, victim, 1));
	}
}

bool IsArmedWithMighthitWeapon(CharData *ch) {
	if (!GET_EQ(ch, EEquipPos::kBoths)
		&& !GET_EQ(ch, EEquipPos::kWield)
		&& !GET_EQ(ch, EEquipPos::kHold)
		&& !GET_EQ(ch, EEquipPos::kLight)
		&& !GET_EQ(ch, EEquipPos::kShield)) {
		return true;
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
