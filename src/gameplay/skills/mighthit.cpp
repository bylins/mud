#include "mighthit.h"

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
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
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
		ch->battle_affects.set(kEafHammer);
	}
}

void DoMighthit(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->GetSkill(ESkill::kHammer) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого вы хотите СИЛЬНО ударить?\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	DoMighthit(ch, vict);
}

void DoMighthit(CharData *ch, CharData *victim) {
	if (ch->GetSkill(ESkill::kHammer) < 1) {
		log("ERROR: вызов молота для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}

	if (ch->HasCooldown(ESkill::kHammer)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (victim == ch) {
		SendMsgToChar("Вы СИЛЬНО ударили себя. Но вы и не спали.\r\n", ch);
		return;
	}

	if (ch->battle_affects.get(kEafTouch)) {
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

	parry_override(ch);
	GoMighthit(ch, victim);
}

void ProcessMighthit(CharData *ch, CharData *victim, HitData &hit_data) {
	ch->battle_affects.unset(kEafHammer);
	hit_data.SetFlag(fight::kIgnoreBlink);
	if (IsArmedWithMighthitWeapon(ch) && !ch->battle_affects.get(kEafTouch)) {
		PerformMighthit(ch, victim, hit_data);
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

	if (IS_IMMORTAL(victim)) {
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
			sprintf(buf, "&b&qВаш богатырский удар задел %s.&Q&n\r\n", PERS(victim, ch, 3));
			SendMsgToChar(buf, ch);
			lag = 1;
			SetWaitState(victim, kBattleRound);
			Affect<EApply> af;
			af.type = ESpell::kBattle;
			af.bitvector = to_underlying(EAffect::kStopFight);
			af.location = EApply::kNone;
			af.modifier = 0;
			af.duration = CalcDuration(victim, 1, 0, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			ImposeAffect(victim, af, true, false, true, false);
			sprintf(buf, "&R&qВаше сознание затуманилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			SendMsgToChar(buf, victim);
			act("$N содрогнул$U от богатырского удара $n1.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
			if (!number(0, 2)) {
				ProcessMighthitBash(ch, victim);
			}
		} else if (might < 800) {
			sprintf(buf, "&g&qВаш богатырский удар пошатнул %s.&Q&n\r\n", PERS(victim, ch, 3));
			SendMsgToChar(buf, ch);
			lag = 2;
			hit_data.dam += (hit_data.dam / 1);
			SetWaitState(victim, 2 * kBattleRound);
			Affect<EApply> af;
			af.type = ESpell::kBattle;
			af.bitvector = to_underlying(EAffect::kStopFight);
			af.location = EApply::kNone;
			af.modifier = 0;
			af.duration = CalcDuration(victim, 2, 0, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			ImposeAffect(victim, af, true, false, true, false);
			sprintf(buf, "&R&qВаше сознание помутилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			SendMsgToChar(buf, victim);
			act("$N пошатнул$U от богатырского удара $n1.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
			if (!number(0, 1)) {
				ProcessMighthitBash(ch, victim);
			}
		} else {
			sprintf(buf, "&G&qВаш богатырский удар сотряс %s.&Q&n\r\n", PERS(victim, ch, 3));
			SendMsgToChar(buf, ch);
			lag = 2;
			hit_data.dam *= 4;
			SetWaitState(victim, 3 * kBattleRound);
			Affect<EApply> af;
			af.type = ESpell::kBattle;
			af.bitvector = to_underlying(EAffect::kStopFight);
			af.location = EApply::kNone;
			af.modifier = 0;
			af.duration = CalcDuration(victim, 3, 0, 0, 0, 0);
			af.battleflag = kAfBattledec | kAfPulsedec;
			ImposeAffect(victim, af, true, false, true, false);
			sprintf(buf, "&R&qВаше сознание померкло после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
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
	SetWaitState(victim, 3 * kBattleRound);

	if (victim->GetPosition() > EPosition::kSit) {
		victim->SetPosition(EPosition::kSit);
		victim->DropFromHorse();
		SendMsgToChar(victim, "&R&qБогатырский удар %s сбил вас с ног.&Q&n\r\n", PERS(ch, victim, 1));
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
