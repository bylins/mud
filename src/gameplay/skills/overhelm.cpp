#include "overhelm.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_hit.h"
#include "parry.h"
#include "protect.h"
#include "engine/db/global_objects.h"

void PerformOverhelm(CharData *ch, CharData *victim, HitData &hit_data);
[[nodiscard]] int CalcOverhelmDmg(CharData *ch, CharData *victim, int dmg);

void GoOverhelm(CharData *ch, CharData *victim) {
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
		ch->battle_affects.set(kEafOverwhelm);
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
		ch->battle_affects.set(kEafOverwhelm);
	}
}

void DoOverhelm(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
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

	DoOverhelm(ch, vict);
}

void DoOverhelm(CharData *ch, CharData *victim) {
	if (ch->GetSkill(ESkill::kOverwhelm) < 1) {
		log("ERROR: вызов глуша для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}

	if (ch->HasCooldown(ESkill::kOverwhelm)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (victim == ch) {
		SendMsgToChar("Вы громко заорали, заглушая свой собственный голос.\r\n", ch);
		return;
	}

	CheckParryOverride(ch);
	GoOverhelm(ch, victim);
}

void ProcessOverhelm(CharData *ch, CharData *victim, HitData &hit_data) {
	// в эти условия ничего добавлять не надо, иначе kEafOverwhelm не снимется
	// с моба по ходу боя, если он не может по каким-то причинам смолотить
	if (ch->battle_affects.get(kEafOverwhelm) && ch->get_wait() <= 0) {
		PerformOverhelm(ch, victim, hit_data);
	}
}

void PerformOverhelm(CharData *ch, CharData *victim, HitData &hit_data) {
	ch->battle_affects.unset(kEafOverwhelm);
	hit_data.SetFlag(fight::kIgnoreBlink);
	const int minimum_weapon_weigth = 19;
	if (IS_IMMORTAL(ch)) {
		hit_data.dam = CalcOverhelmDmg(ch, victim, hit_data.dam);
	} else if (ch->IsNpc()) {
		const bool wielded_with_bow = hit_data.wielded &&
			(static_cast<ESkill>(hit_data.wielded->get_spec_param()) == ESkill::kBows);
		if (AFF_FLAGGED(ch, EAffect::kCharmed) || AFF_FLAGGED(ch, EAffect::kHelper)) {
			// проверка оружия для глуша чармисов
			const bool wielded_for_stupor = GET_EQ(ch, EEquipPos::kWield) || GET_EQ(ch, EEquipPos::kBoths);
			const bool weapon_weigth_ok = hit_data.wielded && (hit_data.wielded->get_weight() >= minimum_weapon_weigth);
			if (wielded_for_stupor && !wielded_with_bow && weapon_weigth_ok) {
				hit_data.dam = CalcOverhelmDmg(ch, victim, hit_data.dam);
			}
		} else {
			if (!wielded_with_bow) {
				hit_data.dam = CalcOverhelmDmg(ch, victim, hit_data.dam);
			}
		}
	} else if (hit_data.wielded) {
		if (static_cast<ESkill>(hit_data.wielded->get_spec_param()) == ESkill::kBows) {
			SendMsgToChar("Луком оглушить нельзя.\r\n", ch);
		} else if (!ch->battle_affects.get(kEafParry) && !ch->battle_affects.get(kEafMultyparry)) {
			if (hit_data.wielded->get_weight() >= minimum_weapon_weigth) {
				hit_data.dam = CalcOverhelmDmg(ch, victim, hit_data.dam);
			} else {
				SendMsgToChar("&WВаше оружие слишком легкое, чтобы им можно было оглушить!&Q&n\r\n", ch);
			}
		}
	}
	else {
		sprintf(buf, "&c&qВы оказались без оружия, а пальцем оглушить нельзя.&Q&n\r\n");
		SendMsgToChar(buf, ch);
		sprintf(buf, "&c&q%s оказался без оружия и не смог вас оглушить.&Q&n\r\n", GET_NAME(ch));
		SendMsgToChar(buf, victim);
	}
}

int CalcOverhelmDmg(CharData *ch, CharData *victim, int dmg) {
	int percent = number(1, MUD::Skill(ESkill::kOverwhelm).difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kOverwhelm, victim);
	TrainSkill(ch, ESkill::kOverwhelm, prob >= percent, victim);
	SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kOverwhelm).name, percent, prob, prob >= percent);
	int lag = 0;

	if (AFF_FLAGGED(victim, EAffect::kHold)) {
		prob = std::max(prob, percent * 150 / 100 + 1);
	}

	if (IS_IMMORTAL(victim)) {
		prob = 0;
	}

	if (prob < percent || dmg == 0 || victim->IsFlagged(EMobFlag::kNoOverwhelm)) {
		sprintf(buf, "&c&qВы попытались оглушить %s, но не смогли.&Q&n\r\n", PERS(victim, ch, 3));
		SendMsgToChar(buf, ch);
		lag = 3;
		dmg = 0;
	} else if (prob * 100 / percent < 300) {
		sprintf(buf, "&g&qВаша мощная атака оглушила %s.&Q&n\r\n", PERS(victim, ch, 3));
		SendMsgToChar(buf, ch);
		lag = 2;
		int k = ch->GetSkill(ESkill::kOverwhelm) / 30;
		if (!victim->IsNpc()) {
			k = std::min(2, k);
		}
		dmg *= std::max(2, number(1, k));
		SetWaitState(victim, 3 * kBattleRound);
		sprintf(buf, "&R&qВаше сознание слегка помутилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
		SendMsgToChar(buf, victim);
		act("$n оглушил$a $N3.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
	} else {
		if (victim->IsFlagged(EMobFlag::kNoBash)) {
			sprintf(buf, "&G&qВаш мощнейший удар оглушил %s.&Q&n\r\n", PERS(victim, ch, 3));
		} else {
			sprintf(buf, "&G&qВаш мощнейший удар сбил %s с ног.&Q&n\r\n", PERS(victim, ch, 3));
		}
		SendMsgToChar(buf, ch);
		if (victim->IsFlagged(EMobFlag::kNoBash)) {
			act("$n мощным ударом оглушил$a $N3.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
		} else {
			act("$n своим оглушающим ударом сбил$a $N3 с ног.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
		}
		lag = 2;
		int k = ch->GetSkill(ESkill::kOverwhelm) / 20;
		if (!victim->IsNpc()) {
			k = std::min(4, k);
		}
		dmg *= std::max(3, number(1, k));
		SetWaitState(victim, 3 * kBattleRound);
		if (victim->GetPosition() > EPosition::kSit && !victim->IsFlagged(EMobFlag::kNoBash)) {
			victim->SetPosition(EPosition::kSit);
			victim->DropFromHorse();
			sprintf(buf, "&R&qОглушающий удар %s сбил вас с ног.&Q&n\r\n", PERS(ch, victim, 1));
			SendMsgToChar(buf, victim);
		} else {
			sprintf(buf, "&R&qВаше сознание слегка помутилось после удара %s.&Q&n\r\n", PERS(ch, victim, 1));
			SendMsgToChar(buf, victim);
		}
	}
	//set_wait(ch, lag, true);
	// Временный костыль, чтоб пофиксить лищний раунд КД
	lag = std::max(2, lag - 1);
	SetSkillCooldown(ch, ESkill::kOverwhelm, lag);
	return dmg;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
