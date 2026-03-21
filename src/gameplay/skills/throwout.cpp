//
// Created by Svetodar on 20.09.2025.
//

#include "throwout.h"

#include "gameplay/fight/fight.h"
#include "engine/core/char_movement.h"
#include "engine/core/action_targeting.h"
#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight_hit.h"

void DoThrowout(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->GetSkill(ESkill::kThrowout) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kThrowout)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}
	one_argument(argument, arg);
	CharData *vict = FindVictim(ch, argument);

	if (!vict) {
		SendMsgToChar("Кого вы так сильно ненавидите, что хотите вышвырнуть отсюда?\r\n", ch);
		return;
	}
	if (!IsAffectedBySpell(ch, ESpell::kFrenzy) && !IS_IMMORTAL(ch)) {
		SendMsgToChar("Не получается! Вы ещё не достаточно разъярились!\r\n", ch);
		return;
	}
	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;
	if (NPC_FLAGGED(vict, ENpcFlag::kBlockDown)
		||NPC_FLAGGED(vict, ENpcFlag::kBlockUp)
		||NPC_FLAGGED(vict, ENpcFlag::kBlockNorth)
		||NPC_FLAGGED(vict, ENpcFlag::kBlockSouth)
		||NPC_FLAGGED(vict, ENpcFlag::kBlockEast)
		||NPC_FLAGGED(vict, ENpcFlag::kBlockWest)) {
			act("$N3 вышвырнуть невозможно!",
			false, ch, nullptr,vict, kToChar);
		return;
	}
	if (!IS_IMMORTAL(ch) && ch->HasCooldown(ESkill::kThrowout)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}
	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}
	if (vict == ch) {
		SendMsgToChar("Вышвырнуть самого себя?!\r\n", ch);
		return;
	}
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	GoThrowout(ch, vict);
}

void ProcessThrowoutFail(CharData *ch, CharData *vict) {
	act("&rВам не удалось вышвырнуть $N3!&n",
	false, ch, nullptr,vict, kToChar);
	act("$N попытал$U вышвырнуть Вас отсюда, но это оказалось не так просто!",
		false,vict, nullptr, ch, kToChar);
	act("$N попытал$U вышвырнуть $n3 отсюда, но только рассмешил$G всех в округе.",
		false,vict, nullptr, ch, kToNotVict | kToArenaListen);
	hit(vict, ch, ESkill::kUndefined, fight::kMainHand);
	SetSkillCooldown(ch, ESkill::kThrowout, 2);
}

void GoThrowout(CharData *ch, CharData *vict) {
//Если у моба 30 или выше уровня больше 75% хп - вышвырнуть нельзя.
	int vict_hp_limit = 0.75 * vict->get_real_max_hit();
	if (!IS_IMMORTAL(ch) && GetRealLevel(vict) >= 30 && (vict->get_hit() > vict_hp_limit)) {
		act("Вы не можете вышвырнуть $N3 - $E ещё слишком цел$G и бодр$G!",
			false, ch, nullptr,vict, kToChar);
		return;
	}
	SkillRollResult result = MakeSkillTest(ch, ESkill::kThrowout, vict);
	bool success = result.success;

	if (!success) {
		ProcessThrowoutFail(ch, vict);
	} else {
		//Поскольку это соло-умение, запрещаем использование если в клетке есть другие игроки.
		ActionTargeting::FoesRosterType roster{ch};
		for (const auto target: roster) {
			if (!target->IsNpc() && target != vict) {
				act("Вы швырнули своего соперника куда подальше, но нечаянно попали им прямо в $N3!",
					false, ch, nullptr,target, kToChar);
				act("$N попытал$U вышвырнуть отсюда $n3, но только рассмешил$G всех в округе.",
						false,vict, nullptr, ch, kToNotVict | kToArenaListen);
				hit(vict, ch, ESkill::kUndefined, fight::kMainHand);
				return;
			}
		}
		int cooldown_if_success = std::max(10, 20 - (ch->GetSkill(ESkill::kThrowout) / 25));
		EDirection direction = SelectRndDirection(vict, 0);
	    if (IsCorrectDirection(vict, direction, false, false)) {
    		act("&YВы сгребли $N3 в охапку и вышвырнули отсюда прочь!&Y&n",
				false, ch, nullptr,vict, kToChar);
	        act("&r$N сгреб$Q вас в охапку и вышвырнул$G прочь!&n",
	            false, vict, nullptr, ch, kToChar);
	        act("$N сгреб$Q $n3 в охапку и вышвырнул$G $s прочь!",
	            false, vict, nullptr, ch, kToNotVict | kToArenaListen);
	    		if (vict->GetPosition() > EPosition::kSit) {
	    			vict->DropFromHorse();
	    			vict->SetPosition(EPosition::kSit);
	    			SetWait(vict, 2, false);
	    			SetSkillCooldown(ch, ESkill::kGlobalCooldown, 1);
	    		}
	    	stop_fighting(vict, true);
			PerformSimpleMove(vict, direction, false, nullptr, EMoveType::kThrowOut);
	    	if (!IS_IMMORTAL(ch)) {
	    		SetSkillCooldown(ch, ESkill::kThrowout, cooldown_if_success);
	    	}
	    } else if (!IsCorrectDirection(vict, direction, false, false)) {
			ProcessThrowoutFail(ch, vict);
	    }
	}
	TrainSkill(ch, ESkill::kThrowout, success, vict);
}