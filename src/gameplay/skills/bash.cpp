#include "bash.h"
#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "protect.h"
#include "engine/db/global_objects.h"
#include "utils/backtrace.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/damage.h"
#include "gameplay/fight/fight.h"

#include <cmath>

// ************************* BASH PROCEDURES
void do_bash(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->GetSkill(ESkill::kBash)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого же вы так сильно желаете сбить?\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;
	
	do_bash(ch, vict);
}

void do_bash(CharData *ch, CharData *vict) {
	if (!ch->GetSkill(ESkill::kBash)) {
		log("ERROR: вызов баша для персонажа %s (%d) без проверки умения", ch->get_name().c_str(), GET_MOB_VNUM(ch));
		return;
	}

	if (ch->HasCooldown(ESkill::kBash)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}

	if (ch->HasCooldown(ESkill::kGlobalCooldown)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}

	if (ch->IsOnHorse()) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (vict == ch) {
		SendMsgToChar("Ваш сокрушающий удар поверг вас наземь... Вы почувствовали себя глупо.\r\n", ch);
		return;
	}

	if (IS_IMPL(ch) || !ch->GetEnemy()) {
		go_bash(ch, vict);
	} else if (IsHaveNoExtraAttack(ch)) {
		if (!ch->IsNpc())
			act("Хорошо. Вы попытаетесь сбить $N3.", false, ch, nullptr, vict, kToChar);
		ch->SetExtraAttack(kExtraAttackBash, vict);
	}
}


void go_bash(CharData *ch, CharData *vict) {
	if (vict->get_extracted_list()) { //уже раз убит и в списке на удаление
		return;
	}
	if (vict->IsFlagged(EMobFlag::kNoFight)) {
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		mudlog(fmt::format("ERROR: попытка сбашить моба {} #{} с флагом !сражается, сброшен коредамп", vict->get_name(), GET_MOB_VNUM(vict)));
		return;
	}
	if (IsUnableToAct(ch) || AFF_FLAGGED(ch, EAffect::kStopLeft)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kBash)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}

	if (ch->HasCooldown(ESkill::kGlobalCooldown)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}
	if (!(ch->IsNpc() || GET_EQ(ch, kShield) || IS_IMMORTAL(ch) || AFF_FLAGGED(vict, EAffect::kHold)
		|| GET_GOD_FLAG(vict, EGf::kGodscurse))) {
		SendMsgToChar("Вы не можете сделать этого без щита.\r\n", ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kIronWind)) {
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
	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	int lag;
	int damage = number(ch->GetSkill(ESkill::kBash) / 1.25, ch->GetSkill(ESkill::kBash) * 1.25);
	bool can_shield_bash = false;
	if (ch->GetSkill(ESkill::kShieldBash) && GET_EQ(ch, kShield) && !ch->IsFlagged(kAwake)) {
		can_shield_bash = true;
	}
	SkillRollResult result_shield_bash = MakeSkillTest(ch, ESkill::kShieldBash, vict);
	bool shield_bash_success = result_shield_bash.success;

	if (can_shield_bash) {
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
			const auto shield_bash = ch->GetSkill(ESkill::kShieldBash);
			const auto char_size = GET_REAL_SIZE(ch);
			const auto shield_weight = GET_EQ(ch, EEquipPos::kShield)->get_weight();
			const auto skill_base = (char_size * shield_weight * 1.5) / 5 + shield_bash * 3 + shield_bash * 2;
			damage = number(ceil(skill_base / 1.25), ceil(skill_base * 1.25)) * GetRealLevel(ch) / 30;
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
	if (vict->IsFlagged(EMobFlag::kNoBash) || GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		success = false;
	}

	vict = TryToFindProtector(vict, ch);

// Если баш - фейл. Немного нахуевертил. Смысл в том, чтобы оставаться на ногах даже при фейле баша и удара щитом, если удар щитом прокачан 200+.
	if (!success) {
		bool still_stands = true;
		if (number(1, 100) > (ch->GetSkill(ESkill::kShieldBash) / 2)) {
			still_stands = false;
		}
//Полный фейл, падаем на жопу:
		if (!can_shield_bash || (!shield_bash_success && !still_stands)) {
			SetFighting(ch, vict);
			SetFighting(vict, ch);
			ch->SetPosition(EPosition::kSit);
			SetWait(ch, 2, true);
			act("&WВы попытались сбить $N3, но упали сами. Учитесь.&n",
				false, ch, nullptr, vict, kToChar);
			act("&r$N хотел$G завалить вас, но, не рассчитав сил, упал$G сам$G.&n",
				false, vict, nullptr, ch, kToChar);
			act("$n избежал$G попытки $N1 сбить $s.",
				false, vict, nullptr, ch, kToNotVict | kToArenaListen);
//Фейл баша, но успех удара щитом, наносим урон (сообщения про "ошарашил" уже прописаны выше):
		} else if ((can_shield_bash && shield_bash_success)) {
			Damage dmg(SkillDmg(ESkill::kShieldBash), damage, fight::kPhysDmg, nullptr);
			dmg.flags.set(fight::kIgnoreBlink);
			dmg.Process(ch, vict);
			SetSkillCooldownInFight(ch, ESkill::kBash, 1);
//Фейл баша, фейл удара щитом, но удержались на ногах:
		} else if (can_shield_bash && !shield_bash_success && still_stands) {
			SetFighting(ch, vict);
			SetSkillCooldownInFight(ch, ESkill::kBash, 1);
			act("&WНеуклюже попытавшись ударить $N3 щитом, Вы сами еле удержались на ногах!&n",
				false, ch, nullptr, vict, kToChar);
			act("$N хотел$G сбить Вас, но в итоге сам$G еле удержал$U на ногах.",
				false, vict, nullptr, ch, kToChar);
			act("Неуклюже попытавшись сбить $n3, $N0 сам$G еле удержал$U на ногах.",
				false, vict, nullptr, ch, kToNotVict | kToArenaListen);
		}
		return;
	} else {
//делаем блокирование баша
		if ((vict->battle_affects.get(kEafBlock)
			|| (CanUseFeat(vict, EFeat::kDefender)
				&& GET_EQ(vict, kShield)
				&& vict->IsFlagged(EPrf::kAwake)
				&& vict->GetSkill(ESkill::kAwake)
				&& vict->GetSkill(ESkill::kShieldBlock)
				&& vict->GetPosition() > EPosition::kSit))
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
					DamageEquipment(vict, kShield, 30, 10);
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
		// Сам баш:
		if (!IS_IMPL(vict)) {
			if (vict->GetPosition() > EPosition::kSit) {
				vict->SetPosition(EPosition::kSit);
				vict->DropFromHorse();
			}
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
		case 0: SetWait(ch, 0, true);
			break;
		case 1: SetSkillCooldownInFight(ch, ESkill::kBash, 1);
			break;
		case 2: SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
			break;
	}

}





// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
