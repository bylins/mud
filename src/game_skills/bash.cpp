#include "bash.h"
#include "game_fight/pk.h"
#include "game_fight/common.h"
#include "game_fight/fight.h"
#include "protect.h"
#include "structs/global_objects.h"

// ************************* BASH PROCEDURES
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
	if (ch->IsHorsePrevents()) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}
	if (ch == vict) {
		SendMsgToChar("Ваш сокрушающий удар поверг вас наземь... Вы почувствовали себя глупо.\r\n", ch);
		return;
	}
	if (GET_POS(ch) < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	int lag;
	int damage = number(GET_SKILL(ch, ESkill::kBash) / 1.25, GET_SKILL(ch, ESkill::kBash) * 1.25);
	bool can_shield_bash = false;
	if (ch->GetSkill(ESkill::kShieldBash) && GET_EQ(ch, kShield) && (!PRF_FLAGGED(ch,kAwake))) {
		can_shield_bash = true;
		}
	bool shield_bash_success;

	if (can_shield_bash) {
		SkillRollResult result_shield_bash = MakeSkillTest(ch, ESkill::kShieldBash, vict);
		shield_bash_success = result_shield_bash.success;

		TrainSkill(ch, ESkill::kShieldBash, shield_bash_success, vict);
		if (shield_bash_success) {
			//Описание аффекта "ошарашен" для умения "удар щитом":
			Affect<EApply> af;
			af.type = ESpell::kConfuse;
			af.duration = 2;
			af.battleflag = kAfSameTime;
			af.bitvector = to_underlying(EAffect::kConfused);
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
			damage = number(ceil(((((GET_REAL_SIZE(ch) * ((GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kShield))) * 1.5)) / 5) + (GET_SKILL(ch,ESkill::kShieldBash) * 3)) + (GET_SKILL(ch, ESkill::kBash) * 2)) / 1.25),
						 ceil(((((GET_REAL_SIZE(ch) * ((GET_OBJ_WEIGHT(GET_EQ(ch, EEquipPos::kShield))) * 1.5)) / 5) + (GET_SKILL(ch,ESkill::kShieldBash) * 3)) + (GET_SKILL(ch, ESkill::kBash) * 2)) * 1.25)) *
				  GetRealLevel(ch) / 30;

			if (GetRealStr(ch) < 55) {
				damage /= 2;
			}
		}
	}

	SkillRollResult result = MakeSkillTest(ch, ESkill::kBash, vict);
	bool success = result.success;
	TrainSkill(ch, ESkill::kBash, success, vict);

	if (AFF_FLAGGED(vict, EAffect::kHold) || GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		success = true;
	}
	if (MOB_FLAGGED(vict, EMobFlag::kNoBash) || GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		success = false;
	}

	vict = TryToFindProtector(vict, ch);

// Если баш - фейл. Немного нахуевертил. Смысл в том чтобы оставаться на ногах даже при фейле баша и удара щитом, если удар щитом прокачан 200+.
	if (!success) {
		Damage dmg(SkillDmg(ESkill::kBash), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.Process(ch, vict);
		bool still_stands;
		if (number(1, 100) > (GET_SKILL(ch, ESkill::kShieldBash) / 2)) {
			still_stands = false;
		}
		if (!can_shield_bash || (!shield_bash_success && !still_stands)) {
			GET_POS(ch) = EPosition::kSit;
			SetWait(ch, 2, true);
			act("Попытавшись завалить $N3, Вы сами упали на землю! Поднимайтесь!",
				false, ch, nullptr,vict, kToChar);
			act("$N хотел$G повалить Вас на землю, но сам$G неуклюже распластал$U.",
				false,vict, nullptr, ch, kToChar);
			act("Попытавшись завалить $n3, $N0 сам$G упал$G на землю!",
				false,vict, nullptr, ch, kToNotVict | kToArenaListen);

		} else if ((can_shield_bash && shield_bash_success) || (can_shield_bash && !shield_bash_success && still_stands)) {
//Ввожу второй тип дамага - первый был для вывода правильного сообщения в игре о фейле баша. Второй - для нанесения нужного урона.
			Damage dmg2(SkillDmg(ESkill::kShieldBash), damage, fight::kPhysDmg, nullptr);
			dmg2.flags.set(fight::kIgnoreBlink);
			dmg2.Process(ch, vict);
			SetSkillCooldownInFight(ch, ESkill::kBash, 1);
		}
		return;
	} else {
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
		Damage dmg(SkillDmg(ESkill::kBash), damage, fight::kPhysDmg, nullptr);
		dmg.flags.set(fight::kNoFleeDmg);
		dmg.flags.set(fight::kIgnoreBlink);
		damage = dmg.Process(ch, vict);
		vict->DropFromHorse();
		// Сам баш:
		if (!IS_IMPL(vict)) {
			GET_POS(vict) = EPosition::kSit;
			SetWait(vict, 3, true);
		}
		lag = 1;

		// Если убил - то лага не будет:
		if (damage < 0) {
			lag = 0;
		}
		//Лаг на тот случай, если не используется/нет умения "удар щитом" или на жертве висит "Защита Богов":
		if (AFF_FLAGGED(vict, EAffect::kGodsShield)
		|| (!can_shield_bash)
		|| (!shield_bash_success)) {
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
	}

}





// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
