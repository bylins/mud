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
	}

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

	int lag;
	int dam;
	SkillRollResult result = MakeSkillTest(ch, ESkill::kBash, vict);
	bool success = result.success;

	//Описание аффекта "ошарашен" для умения "удар щитом":
	Affect<EApply> af;
	af.duration = 2;
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag = kAfSameTime;
	af.bitvector = to_underlying(EAffect::kConfused);

	if (AFF_FLAGGED(vict, EAffect::kHold) || GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		success = true;
	}
	if (MOB_FLAGGED(vict, EMobFlag::kNoBash) || GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		success = false;
	}

	TrainSkill(ch, ESkill::kBash, success, vict);

	// Если баш - фейл, смотрим сработает ли "удар щитом", если он вообще есть:
	if (!success) {
		if (ch->GetSkill(ESkill::kShieldBash)) {
			SkillRollResult result_ShB = MakeSkillTest(ch, ESkill::kShieldBash, vict);
			bool successShB = result_ShB.success;

			//Успех "удара щитом":
			if (successShB && GET_EQ(ch, kShield)) {
				if (!vict->IsNpc()) {
					af.duration *= 30;
				}
				affect_to_char(vict, af);
				act("Вы ошарашили $N3 ударом щита!",
					false, ch, nullptr, vict, kToChar);
				act("$N0 ошарашил$G Вас ударом щита!",
					false, vict, nullptr, ch, kToChar);
				act("$N0 ошарашил$G $n3 ударом щита!",
					false, vict, nullptr, ch, kToNotVict | kToArenaListen);

				dam = number(ceil((((GET_REAL_SIZE(ch) * ((GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kShield))) * 1.5)) / 6) + (GET_SKILL(ch,ESkill::kShieldBash) * 5)) / 1.25),
							 ceil((((GET_REAL_SIZE(ch) * ((GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kShield))) * 1.5)) / 6) + (GET_SKILL(ch,ESkill::kShieldBash) * 5)) * 1.25));

				Damage dmg(SkillDmg(ESkill::kBash), dam, fight::kPhysDmg, nullptr);
				dmg.flags.set(fight::kIgnoreBlink);
				dmg.Process(ch, vict);
				vict->DropFromHorse();
				lag = 1;

				// Фейл и баша и "удара щитом":
			} else {
				Damage dmg(SkillDmg(ESkill::kBash), fight::kZeroDmg, fight::kPhysDmg, nullptr);
				dmg.flags.set(fight::kIgnoreBlink);
				dmg.Process(ch, vict);
				GET_POS(ch) = EPosition::kSit;
				lag = 3;
			}

			// Фейл баша при отсутствии "удара щитом":
		} else {
			Damage dmg(SkillDmg(ESkill::kBash), fight::kZeroDmg, fight::kPhysDmg, nullptr);
			dmg.flags.set(fight::kIgnoreBlink);
			dmg.Process(ch, vict);
			GET_POS(ch) = EPosition::kSit;
			lag = 3;
		}

	} else {
		// Удар щитом не будет работать, если персонаж в осторожке, если нет щита, или если нет умения:
		if ((!ch->GetSkill(ESkill::kShieldBash))
		|| (!GET_EQ(ch, kShield))
		|| (PRF_FLAGGED(ch,kAwake))) {
			dam = number(ceil(ch->GetSkill(ESkill::kBash) * 1.25), ceil(ch->GetSkill(ESkill::kBash) / 1.25));

		// Успех баша и, соответственно "удара щитом":
		} else {
			MakeSkillTest(ch, ESkill::kShieldBash, vict);
			TrainSkill(ch, ESkill::kShieldBash, success, vict);
			if (!vict->IsNpc()) {
				af.duration *= 30;
			}
			affect_to_char(vict, af);
			act("Вы ошарашили $N3 ударом щита!",
				false, ch, nullptr, vict, kToChar);
			act("$N0 ошарашил$G Вас ударом щита!",
				false, vict, nullptr, ch, kToChar);
			act("$N0 ошарашил$G $n3 ударом щита!",
				false, vict, nullptr, ch, kToNotVict | kToArenaListen);
			dam = number(ceil((((GET_REAL_SIZE(ch) * ((GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kShield))) * 1.5)) / 6) + (GET_SKILL(ch,ESkill::kShieldBash) * 5)) / 1.25),
						 ceil((((GET_REAL_SIZE(ch) * ((GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kShield))) * 1.5)) / 6) + (GET_SKILL(ch,ESkill::kShieldBash) * 5)) * 1.25));
		}

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
		Damage dmg(SkillDmg(ESkill::kBash), dam, fight::kPhysDmg, nullptr);
		dmg.flags.set(fight::kIgnoreBlink);
		dam = dmg.Process(ch, vict);
		vict->DropFromHorse();
		// Сам баш:
		GET_POS(vict) = EPosition::kSit;
		SetWait(vict, 3, false);
		lag = 1;

		// Если убил - то лага не будет:
		if (dam < 0) {
			lag = 0;
		}

		//Лаг на тот случай, если нет умения "удар щитом" или на жертве висит "Защита Богов":
		if (AFF_FLAGGED(vict, EAffect::kGodsShield)
		|| (!GET_SKILL(ch, ESkill::kShieldBash))) {
			lag = 2;
		}
	}

	//разные типы лагов в зависимости от того, есть ли "удар щитом", а так же при фейле/успехе и т.д.
	switch (lag) {
		case 0:
			SetWait(ch, 0, true);
			break;
		case 1:
			SetSkillCooldownInFight(ch, ESkill::kBash, 1);
			break;
		case 2:
			SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
			break;
		case 3:
			SetWait(ch, 2, true);
			break;
	}

}

void do_bash(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->GetSkill(ESkill::kBash)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kBash)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}

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
