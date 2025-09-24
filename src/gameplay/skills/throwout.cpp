//
// Created by Svetodar on 20.09.2025.
//

#include "throwout.h"
#include "act_movement.h"
#include "act_movement.cpp"
#include "engine/core/action_targeting.h"
#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/core/handler.h"
#include "engine/db/global_objects.h"

void do_throwout(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->GetSkill(ESkill::kThrowout) < 1) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	one_argument(argument, arg);
	CharData *vict = get_char_vis(ch, arg, EFind::kCharInRoom);
	if (!vict) {
		SendMsgToChar("Кого вы так сильно ненавидите, что хотите вышвырнуть отсюда?\r\n", ch);
		return;
	}
	if (!IsAffectedBySpell(ch, ESpell::kCourage)) {
		SendMsgToChar("Не получается! Вы ещё не достаточно разъярились!\r\n", ch);
		return;
	}
	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;
	if (ch->HasCooldown(ESkill::kThrowout)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}
	if (ch->IsOnHorse()) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
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
	go_throwout(ch, vict);
}

void go_throwout(CharData *ch, CharData *vict) {
//Поскольку это соло-умение, запрещаем использование если в клетке есть другие игроки.
	ActionTargeting::FoesRosterType roster{ch};
	for (const auto target: roster) {
		if (!target->IsNpc() && target != vict) {
			act("&rВы швырнули своего соперника куда подальше, но нечаянно попали им прямо в $N1!&n",
				false, ch, nullptr,target, kToChar);
			hit(vict, ch, ESkill::kUndefined, fight::kMainHand);
			return;
		}
	}

//Если у моба 30 или выше уровня больше 75% хп - вышвырнуть нельзя.
	int vict_hp_limit = 0.75 * vict->get_real_max_hit();
	if (GetRealLevel(vict) >= 30 && (vict->get_hit() > vict_hp_limit)) {
		act("Вы не можете вышвырнуть $N3 - $E ещё слишком цел$G и бодр$G!",
			false, ch, nullptr,vict, kToChar);
		return;
	}
	SkillRollResult result = MakeSkillTest(ch, ESkill::kThrowout, vict);
	bool success = result.success;

	TrainSkill(ch, ESkill::kThrowout, success, vict);
	if (!success) {
		act("&rВам не удалось вышвырнуть $N3!&n",
			false, ch, nullptr,vict, kToChar);
		act("$N попытал$U вышвырнуть Вас подальше отсюда, но это оказалось не так просто!",
			false,vict, nullptr, ch, kToChar);
		act("$N попытал$U вышвырнуть $n3 подальше отсюда, но только рассмешил$G всех в округе.",
			false,vict, nullptr, ch, kToNotVict | kToArenaListen);
		hit(vict, ch, ESkill::kUndefined, fight::kMainHand);
		SetSkillCooldown(ch, ESkill::kThrowout, 4);
	} else {
		int cooldown_if_success = std::max(10, 40 - (ch->GetSkill(ESkill::kThrowout) / 25));
		EDirection direction = SelectRndDirection(vict, 0);
	    if (IsCorrectDirection(vict, direction, true, false)) {
    		act("&YВы сгребли $N3 в охапку и вышвырнули отсюда прочь!&Y&n",
				false, ch, nullptr,vict, kToChar);
	        act("&r$N собрал$G вас в охапку и вышвырнул$G прочь!&n",
	            false, vict, nullptr, ch, kToChar);
	        act("$N собрал$G $n3 в охапку и вышвырнул$G $s прочь!",
	            false, vict, nullptr, ch, kToNotVict | kToArenaListen);
	    		if (vict->GetPosition() > EPosition::kSit) {
	    			vict->DropFromHorse();
	    			vict->SetPosition(EPosition::kSit);
	    			SetWait(vict, 3, false);
	    		}
			DoSimpleMove(vict, direction, false, nullptr, ThrowOut);
	    	SetSkillCooldown(ch, ESkill::kThrowout, cooldown_if_success);
	    }
	}
}