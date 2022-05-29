#include "bash.h"
#include "game_fight/pk.h"
#include "game_fight/common.h"
#include "game_fight/fight.h"
#include "game_fight/fight_hit.h"
#include "protect.h"
#include "structs/global_objects.h"

// ************************* BASH PROCEDURES
void go_bash(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch) || AFF_FLAGGED(ch, EAffect::kStopLeft)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (!(ch->IsNpc() || GET_EQ(ch, kShield) || IS_IMMORTAL(ch) || AFF_FLAGGED(vict, EAffect::kHold)
		|| GET_GOD_FLAG(vict, EGf::kGodscurse))) {
		SendMsgToChar("Вы не можете сделать этого без щита.\r\n", ch);
		return;
	};

	if (PRF_FLAGS(ch).get(EPrf::kIronWind)) {
		SendMsgToChar("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	if (ch->IsHorsePrevents())
		return;

	if (ch == vict) {
		SendMsgToChar("Ваш сокрушающий удар поверг вас наземь... Вы почувствовали себя глупо.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	vict = TryToFindProtector(vict, ch);

	int percent = number(1, MUD::Skill(ESkill::kBash).difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kBash, vict);

	if (AFF_FLAGGED(vict, EAffect::kHold) || GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		prob = percent;
	}
	if (MOB_FLAGGED(vict, EMobFlag::kNoBash) || GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		prob = 0;
	}
	bool success = percent <= prob;
	TrainSkill(ch, ESkill::kBash, success, vict);

	SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kBash).name, percent, prob, success);
	if (!success) {
		Damage dmg(SkillDmg(ESkill::kBash), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.Process(ch, vict);
		GET_POS(ch) = EPosition::kSit;
		prob = 3;
	} else {
		//не дадим башить мобов в лаге которые спят, оглушены и прочее
		if (GET_POS(vict) <= EPosition::kStun && vict->get_wait() > 0) {
			SendMsgToChar("Ваша жертва и так слишком слаба, надо быть милосерднее.\r\n", ch);
			ch->setSkillCooldown(ESkill::kGlobalCooldown, kPulseViolence);
			return;
		}

		int dam = str_bonus(GET_REAL_STR(ch), STR_TO_DAM) + GetRealDamroll(ch) +
			std::max(0, ch->GetSkill(ESkill::kBash) / 10 - 5) + GetRealLevel(ch) / 5;

//делаем блокирование баша
		if ((GET_AF_BATTLE(vict, kEafBlock)
			|| (CanUseFeat(vict, EFeat::kDefender)
				&& GET_EQ(vict, kShield)
				&& PRF_FLAGGED(vict, EPrf::kAwake)
				&& vict->GetSkill(ESkill::kAwake)
				&& vict->GetSkill(ESkill::kShieldBlock)
				&& GET_POS(vict) > EPosition::kSit))
			&& !AFF_FLAGGED(vict, EAffect::kStopFight)
			&& !AFF_FLAGGED(vict, EAffect::kMagicStopFight)
			&& !AFF_FLAGGED(vict, EAffect::kStopLeft)
			&& vict->get_wait() <= 0
			&& AFF_FLAGGED(vict, EAffect::kHold) == 0) {
			if (!(GET_EQ(vict, kShield) || vict->IsNpc() || IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, EGf::kGodsLike)))
				SendMsgToChar("У вас нечем отразить атаку противника.\r\n", vict);
			else {
				int range, prob2;
				range = number(1, MUD::Skill(ESkill::kShieldBlock).difficulty);
				prob2 = CalcCurrentSkill(vict, ESkill::kShieldBlock, ch);
				bool success2 = prob2 >= range;
				TrainSkill(vict, ESkill::kShieldBlock, success2, ch);
				if (!success2) {
					act("Вы не смогли блокировать попытку $N1 сбить вас.",
						false, vict, nullptr, ch, kToChar);
					act("$N не смог$Q блокировать вашу попытку сбить $S.",
						false, ch, nullptr, vict, kToChar);
					act("$n не смог$q блокировать попытку $N1 сбить $s.",
						true, vict, nullptr, ch, kToNotVict | kToArenaListen);
				} else {
					act("Вы блокировали попытку $N1 сбить вас с ног.",
						false, vict, nullptr, ch, kToChar);
					act("Вы хотели сбить $N1, но он$G блокировал$G Вашу попытку.",
						false, ch, nullptr, vict, kToChar);
					act("$n блокировал$g попытку $N1 сбить $s.",
						true, vict, nullptr, ch, kToNotVict | kToArenaListen);
					alt_equip(vict, kShield, 30, 10);
					if (!ch->GetEnemy()) {
						SetFighting(ch, vict);
						SetWait(ch, 1, true);
						//setSkillCooldownInFight(ch, ESkill::kBash, 1);
					}
					return;
				}
			}
		}

		prob = 0; // если башем убил - лага не будет
		Damage dmg(SkillDmg(ESkill::kBash), dam, fight::kPhysDmg, nullptr);
		dmg.flags.set(fight::kNoFleeDmg);
		dam = dmg.Process(ch, vict);

		if (dam > 0 || (dam == 0 && AFF_FLAGGED(vict, EAffect::kShield))) {
			prob = 2;
			if (!vict->drop_from_horse()) {
				GET_POS(vict) = EPosition::kSit;
				SetWait(vict, 3, false);
			}
		}
	}
	SetWait(ch, prob, true);
}

void do_bash(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if ((ch->IsNpc() && (!AFF_FLAGGED(ch, EAffect::kHelper))) || !ch->GetSkill(ESkill::kBash)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kBash)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (ch->IsOnHorse()) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого же вы так сильно желаете сбить?\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar("Ваш сокрушающий удар поверг вас наземь... Вы почувствовали себя глупо.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !ch->GetEnemy()) {
		go_bash(ch, vict);
	} else if (IsHaveNoExtraAttack(ch)) {
		if (!ch->IsNpc())
			act("Хорошо. Вы попытаетесь сбить $N3.", false, ch, nullptr, vict, kToChar);
		ch->SetExtraAttack(kExtraAttackBash, vict);
	}
}



// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
