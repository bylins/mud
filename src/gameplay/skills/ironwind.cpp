#include "ironwind.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight_hit.h"
#include "parry.h"
#include "engine/core/handler.h"

void go_iron_wind(CharData *ch, CharData *victim) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kIronWind)) {
		SendMsgToChar("Вы уже впали в неистовство.\r\n", ch);
		return;
	}
	if (ch->GetEnemy() && (ch->GetEnemy() != victim)) {
		act("$N не сражается с вами, не трогайте $S.", false, ch, nullptr, victim, kToChar);
		return;
	}

	CheckParryOverride(ch);

	act("Вас обуяло безумие боя, и вы бросились на $N3!\r\n", false, ch, nullptr, victim, kToChar);
	ObjData *weapon;
	if ((weapon = GET_EQ(ch, EEquipPos::kWield)) || (weapon = GET_EQ(ch, EEquipPos::kBoths))) {
		strcpy(buf, "$n взревел$g и ринул$u на $N3, бешено размахивая $o4!");
		strcpy(buf2, "$N взревел$G и ринул$U на вас, бешено размахивая $o4!");
	} else {
		strcpy(buf, "$n бешено взревел$g и ринул$u на $N3!");
		strcpy(buf2, "$N бешено взревел$G и ринул$U на вас!");
	};
	act(buf, false, ch, weapon, victim, kToNotVict | kToArenaListen);
	act(buf2, false, victim, weapon, ch, kToChar);

	if (!ch->GetEnemy()) {
		ch->SetFlag(EPrf::kIronWind);
		ch->battle_affects.set(kEafIronWind);
		hit(ch, victim, ESkill::kUndefined, fight::kMainHand);
		SetWait(ch, 2, true);
		//ch->setSkillCooldown(ESkill::kGlobalCooldown, 2);
		//ch->setSkillCooldown(ESkill::kIronwind, 2);
	} else {
		ch->SetFlag(EPrf::kIronWind);
		ch->battle_affects.set(kEafIronWind);
	}
}

void do_iron_wind(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kIronwind)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	};
	if (ch->HasCooldown(ESkill::kIronwind)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (ch->battle_affects.get(kEafOverwhelm) || ch->battle_affects.get(kEafHammer)) {
		SendMsgToChar("Невозможно! Вы слишкм заняты боем!\r\n", ch);
		return;
	};
	int moves = ch->get_max_move() / (2 + std::max(15, ch->GetSkill(ESkill::kIronwind)) / 15);
	if (ch->get_max_move() < moves * 2) {
		SendMsgToChar("Вы слишком устали...\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого вам угодно изрубить в капусту?\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar("Вы с чувством собственного достоинства мощно пустили ветры... Железные.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument)) {
		return;
	}
	if (!check_pkill(ch, vict, arg)) {
		return;
	}

	go_iron_wind(ch, vict);
}

void ProcessIronWindHits(CharData *ch, fight::AttackType weapon) {
	int percent = 0, prob = 0, div = 0, moves = 0;
	/*
	первая дополнительная атака правой наносится 100%
	вторая дополнительная атака правой начинает наноситься с 80%+ скилла, но не более чем с 80% вероятностью
	первая дополнительная атака левой начинает наноситься сразу, но не более чем с 80% вероятностью
	вторая дополнительная атака левей начинает наноситься с 170%+ скилла, но не более чем с 30% вероятности
	*/
	if (ch->IsFlagged(EPrf::kIronWind)) {
		percent = ch->GetSkill(ESkill::kIronwind);
		moves = ch->get_max_move() / (6 + std::max(10, percent) / 10);
		prob = ch->battle_affects.get(kEafIronWind);
		if (prob && !check_moves(ch, moves)) {
			ch->battle_affects.unset(kEafIronWind);
		} else if (!prob && (ch->get_move() > moves)) {
			ch->battle_affects.set(kEafIronWind);
		};
	};
	if (ch->battle_affects.get(kEafIronWind)) {
		TrainSkill(ch, ESkill::kIronwind, true, ch->GetEnemy());
		if (weapon == fight::kMainHand) {
			div = 100 + std::min(80, std::max(1, percent - 80));
			prob = 100;
		} else {
			div = std::min(80, percent + 10);
			prob = 80 - std::min(30, std::max(0, percent - 170));
		};
		while (div > 0) {
			if (number(1, 100) < div)
				hit(ch, ch->GetEnemy(), ESkill::kUndefined, weapon);
			div -= prob;
		};
	};
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp
