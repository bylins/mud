//
// Created by Svetodar on 29.08.2023.
//

#include "charge.h"

#include "act_movement.h"
#include "entities/char_data.h"
#include "game_fight/pk.h"
#include "game_fight/fight.h"
#include "game_fight/fight_hit.h"
#include "protect.h"
#include "bash.h"
#include "action_targeting.h"
#include "game_fight/common.h"

// ********************* CHARGE PROCEDURE

void do_charge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int direction;

	if (AFF_FLAGGED(ch, EAffect::kHold) || ch->get_wait() > 0) {
		SendMsgToChar("Вы временно не в состоянии это сделать.\r\n", ch);
		return;
	}
	if (PRF_FLAGS(ch).get(EPrf::kIronWind)) {
		SendMsgToChar("Невидимые оковы мешают вам ринуться в бой!\r\n", ch);
		return;
	}
	if (GET_POS(ch) != EPosition::kStand) {
		SendMsgToChar("Вы не можете ринуться в бой из этого положения!\r\n", ch);
		return;
	}
	one_argument(argument, arg);
	if ((direction = search_block(arg, dirs, false)) >= 0 ||
		(direction = search_block(arg, dirs_rus, false)) >= 0) {
		go_charge(ch, direction);
		return;
	}
}

void go_charge(CharData *ch, int direction) {
	int dam;
	int victims_amount;

	if (IsCorrectDirection(ch, direction, true, false)
		&& !ROOM_FLAGGED(EXIT(ch, direction)->to_room(), ERoomFlag::kDeathTrap)) {
		act("$n истошно завопил$g и ринул$u в бой, лихо размахивая своим оружием.",
			false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		if (DoSimpleMove(ch, direction, true, nullptr, false)) {
			SendMsgToChar("С криком \"За дружину!\" Вы ринулись в бой!\r\n", ch);
			act("Разъярённ$w $n прибежал$g сюда, явно с намерениями кого-нибудь поколотить!",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
	} else {
		SendMsgToChar("Вы желаете убиться об стенку?!\r\n", ch);
		return;
	}
	dam = (number(ceil((ceil GET_SKILL(ch,ESkill::kCharge) * 3) / 1.25),
					  ceil(((ceil GET_SKILL(ch,ESkill::kCharge) * 3) * 1.25))) *
			   GetRealLevel(ch)) / 30;

	if (PRF_FLAGGED(ch, EPrf::kAwake)) {
		dam /= 2;
	}
	Affect<EApply> af;
	af.type = ESpell::kNoCharge;
	af.duration = -1;
	af.location = EApply::kNone;
	af.battleflag = kAfSameTime;
	af.bitvector = to_underlying(EAffect::kNoCharge);

	Affect<EApply> af2;
	af2.type = ESpell::kUndefined;
	af2.duration = 3;
	af2.modifier = 0;
	af2.location = EApply::kNone;
	af2.battleflag = kAfSameTime;
	af2.bitvector = to_underlying(EAffect::kConfused);

	Damage dmg(SkillDmg(ESkill::kCharge), dam, fight::kPhysDmg, nullptr);
//Ищем цели, потом проверяем можем ли теоретически атаковать, потом проверяем на успех умения, потом на !чардж, потом на возможность удара щитом
	victims_amount = 2 + ((ch->GetSkill(ESkill::kCharge) / 40));
	ActionTargeting::FoesRosterType roster0{ch};

	for (const auto target: roster0) {
		if (MOB_FLAGGED(target, EMobFlag::kProtect) || (!may_kill_here(ch, target, arg)) || target == ch || !CAN_SEE(ch, target)) {
			--victims_amount;
		} else {
			if (!AFF_FLAGGED(target, EAffect::kNoCharge)) {
				affect_to_char(target, af);
				target->set_charge_apply_time();
			}
			--victims_amount;
		}
		if (victims_amount <= 0) {
			break;
		}
	}

	ActionTargeting::FoesRosterType roster{ch};
	for (const auto target: roster) {
		if (MOB_FLAGGED(target, EMobFlag::kProtect) || (!may_kill_here(ch, target, arg)) || target == ch || !CAN_SEE(ch, target)) {
			--victims_amount;
		} else {
			SkillRollResult result = MakeSkillTest(ch, ESkill::kCharge, target);
			bool success = result.success;

			TrainSkill(ch, ESkill::kCharge, success, target);
			TryToFindProtector(target, ch);
			if (!success) {
				act("Вам не удалось обескуражить $N3 своим бешенным натиском!",
					false, ch, nullptr, target, kToChar);
				act("$N попытал$u неожиданно напасть на Вас, но Вы вовремя спохватились и приняли бой!",
					false, target, nullptr, ch, kToChar);
				act("$n попытал$u обескуражить $N3 яростным натиском, но только рассмешил$g всех в округе.",
					false, target, nullptr, ch, kToNotVict | kToArenaListen);
				hit(target, ch, ESkill::kUndefined, fight::kMainHand);
			} else {
//Если есть !чардж, но еще не прошло 3 секунды со времени его применения
				if (AFF_FLAGGED(target, EAffect::kNoCharge) && (time(0) <= target->ChargeApplyTime + 6)) {
//Сможем ли использовать "удар щитом" или просто ударим?
					if (ch->GetSkill(ESkill::kShieldBash) && (ch->GetSkill(ESkill::kBash)) && (GET_EQ(ch, kShield)) && (!PRF_FLAGGED(ch, EPrf::kAwake))) {
						go_bash(ch, target);
					} else {
						act("Хорошенько разогнавшись Вы прописали $N2 роскошного тумака!",
							false, ch, nullptr, target, kToChar);
						act("А вот вам и гостинец - $N одарил$G Вас смачной оплеухой!",
							false, target, nullptr, ch, kToChar);
						act("$n разогнал$u и отвесил$g $N2 внушительного тумака!",
							false, target, nullptr, ch, kToNotVict | kToArenaListen);
						dmg.Process(ch, target);
//При успехе вешаем и "ошарашен"
						affect_to_char(target, af2);
					}
				} else {
					act("Вам не удалось привести $N3 в замешательство своим натиском!",
						false, ch, nullptr, target, kToChar);
					hit(target, ch, ESkill::kUndefined, fight::kMainHand);
				}
			}
				--victims_amount;
		}
			if (victims_amount <= 0) {
				SetWait(ch, 3, true);
				break;
			}
	}
}