//
// Created by Svetodar on 29.08.2023.
//

#include "charge.h"

#include "engine/core/char_movement.h"
#include "engine/entities/char_data.h"
#include "gameplay/fight/pk.h"
#include "protect.h"
#include "bash.h"
#include "engine/core/action_targeting.h"
#include "gameplay/fight/common.h"
#include "gameplay/ai/mobact.h"
#include "gameplay/mechanics/damage.h"

void DoCharge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int direction;

	if (!ch->GetSkill(ESkill::kCharge)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kHold) || ch->get_wait() > 0) {
		SendMsgToChar("Вы временно не в состоянии это сделать.\r\n", ch);
		return;
	}
	if (ch->IsOnHorse()) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}
	if (ch->IsFlagged(EPrf::kIronWind)) {
		SendMsgToChar("Невидимые оковы мешают вам ринуться в бой!\r\n", ch);
		return;
	}
	if (ch->GetPosition() != EPosition::kStand) {
		SendMsgToChar("Вы не можете ринуться в бой из этого положения!\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	if ((direction = search_block(arg, dirs, false)) >= 0 ||
		(direction = search_block(arg, dirs_rus, false)) >= 0) {
		GoCharge(ch, direction);
	} else {
		SendMsgToChar("В каком направлении Вы желаете ринуться в бой?\r\n", ch);
	}
}

void GoCharge(CharData *ch, int direction) {
	if (IsCorrectDirection(ch, direction, true, false)) {
		act("$n истошно завопил$g и ринул$u в бой, лихо размахивая своим оружием.",
			false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		if (PerformSimpleMove(ch, direction, true, nullptr, EMoveType::kDefault)) {
			SendMsgToChar("С криком \"За дружину!\" Вы ринулись в бой!\r\n", ch);
			act("Разъярённ$w $n прибежал$g сюда, явно с намерениями кого-нибудь поколотить!",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
	} else {
		SendMsgToChar("Вы желаете убиться об стенку?!\r\n", ch);
		return;
	}

	bool is_awake = false;
	if (ch->IsFlagged(EPrf::kAwake)) {
		is_awake = true;
	}
	auto skill_charge = ch->GetSkill(ESkill::kCharge);
	int dam = (number(ceil(skill_charge * 3 / 1.25), ceil(skill_charge * 3 * 1.25)) * GetRealLevel(ch)) / 30;

	if (is_awake) {
		dam /= 2;
	}

	int victims_amount = 2;
	if (is_awake) {
		victims_amount = 2 + ch->GetSkill(ESkill::kCharge) / 40;
	}

	Affect<EApply> af;
	af.type = ESpell::kNoCharge;
	af.duration = 4;
	af.battleflag = kNone;
	af.bitvector = to_underlying(EAffect::kNoCharge);
	af.caster_id = ch->get_uid();

	Affect<EApply> af2;
	af2.type = ESpell::kUndefined;
	af2.duration = 3;
	af2.battleflag = kAfSameTime;
	af2.bitvector = to_underlying(EAffect::kConfused);
	SetWait(ch, 1, false);

	Damage dmg(SkillDmg(ESkill::kCharge), dam, fight::kPhysDmg, nullptr);

	ActionTargeting::FoesRosterType roster{ch};
	for (const auto target: roster) {
		if (target->purged() || target->in_room == kNowhere)
			continue;
		if (target->IsFlagged(EMobFlag::kProtect) || !may_kill_here(ch,target, arg) ||target == ch || !CAN_SEE(ch,target)) {
			--victims_amount;
		} else {
			if (IsAffectedBySpellWithCasterId(ch, target, ESpell::kNoCharge)) {
				act("$N0 уже на страже - вы не сможете повторно испугать $S своим натиском!",
					false, ch, nullptr,target, kToChar);
				dmg.dam = 0;
				dmg.Process(ch, target);
			} else {
				if (!target->IsNpc()) {
					af.duration *= 30;
				}
				affect_to_char(target, af);

				SkillRollResult result = MakeSkillTest(ch, ESkill::kCharge,target);
				bool success = result.success;

				TrainSkill(ch, ESkill::kCharge, success,target);
				TryToFindProtector(target, ch);
				if (!success) {
					act("Вам не удалось привести $N3 в замешательство своим натиском!",
						false, ch, nullptr,target, kToChar);
					act("$N попытал$U неожиданно напасть на Вас, но Вы вовремя спохватились и приняли бой!",
						false,target, nullptr, ch, kToChar);
					act("$N попытал$U обескуражить $n3 яростным натиском, но только рассмешил$G всех в округе.",
						false,target, nullptr, ch, kToNotVict | kToArenaListen);
					dmg.dam = 0;
					dmg.Process(target, ch);
				} else {
					if (ch->GetSkill(ESkill::kShieldBash) && ch->GetSkill(ESkill::kBash) && (GET_EQ(ch, kShield)) && !is_awake) {
						go_bash(ch,target);
					} else {
						act("Хорошенько разогнавшись Вы прописали $N2 роскошного тумака!",
							false, ch, nullptr,target, kToChar);
						act("А вот вам и гостинец - $N одарил$G Вас смачной оплеухой!",
							false,target, nullptr, ch, kToChar);
						act("$N разогнал$U и отвесил$G $n2 внушительного тумака!",
							false,target, nullptr, ch, kToNotVict | kToArenaListen);
						dmg.flags.set(fight::kIgnoreBlink);
						dmg.Process(ch,target);
					}
				}
			}
			--victims_amount;
		}
		if (victims_amount <= 0) {
			break;
		}
	}
	mob_ai::do_aggressive_room(ch,1);
}
