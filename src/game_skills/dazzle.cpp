#include "game_fight/pk.h"
#include "game_fight/common.h"
#include "game_fight/fight.h"
#include "protect.h"
#include "structs/global_objects.h"
#include "dazzle.h"
#include "action_targeting.h"
#include "handler.h"

//
// Created by Svetodar on 04.01.2024.
//
void DoDazzle(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (!ch->GetSkill(ESkill::kDazzle)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kDazzle)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	}
	if (ch->IsOnHorse()) {
		SendMsgToChar("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}
	if (GET_POS(ch) < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}
	if (!ch->GetEnemy()) {
		SendMsgToChar("Но с Вами никто не сражаеся! Что началось-то? Нормально же сидели...\r\n", ch);
		return;
	}
	CharData *vict = FindVictim(ch, argument);

	if (!vict) {
		SendMsgToChar("Кого же вы так сильно желаете лишить зрения?\r\n", ch);
		return;
	}
	if (vict == ch) {
		SendMsgToChar("Лучше уж выколите себе глаза, чтобы не мучаться!\r\n", ch);
		return;
	}
	if (IsAffectedBySpellWithCasterId(vict, ch, ESpell::kDazzle)) {
		SendMsgToChar("Невозможно ослепить жертву повторно!\r\n", ch);
		return;
	}
	GoDazzle(ch, vict);
}


void GoDazzle(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (ch == vict) {
		SendMsgToChar("Лучше уж выколите себе глаза, чтобы не мучаться!\r\n", ch);
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		SendMsgToChar("Вам стоит встать на ноги.\r\n", ch);
		return;
	}
	bool has_pepper = false;

	for (auto i = ch->carrying;i; i = i->get_next_content()) {
		if (GET_OBJ_TYPE(i) == EObjType::kWand && GET_OBJ_VAL(i,3) == 356 && GET_OBJ_VAL(i,2) > 0) {
			int charges = GET_OBJ_VAL(i,2);
			if (charges > 0) {
				i->set_val(2, charges - 1);
				has_pepper = true;
				break;
			}
		}
	}

	if (!has_pepper) {
		SendMsgToChar("&WЧем Вы собираетесь ослепить соперника?! У вас нет ни щепотки жгучей смеси!\r\n&n", ch);
		return;
	}

	int victims_amount = 3;
	if (ch->GetSkill(ESkill::kDazzle) > 200) {
		victims_amount = 4;
	};

	Affect<EApply> af;
	af.type = ESpell::kBlindness;
	af.battleflag = kAfPulsedec;
	af.duration = 150 + (GET_SKILL(ch, ESkill::kDazzle) * 1.25);
	af.bitvector = to_underlying(EAffect::kBlind);

	Affect<EApply> af2;
	af2.type = ESpell::kDazzle;
	af2.duration = 5;
	af2.battleflag = kNone;
	af2.caster_id = GET_ID(ch);

	ActionTargeting::FoesRosterType roster{ch};
	for (const auto target: roster) {
		if (!IsAffectedBySpellWithCasterId(ch, target, ESpell::kDazzle)) {
			if (!target->IsNpc()) {
				SendMsgToChar("Нельзя слепить человеков! Чтобы видали кому кланяться...\r\n", ch);
				continue;
			} else {
				if (target != ch && target->GetEnemy() == ch) {
					SkillRollResult result = MakeSkillTest(ch, ESkill::kDazzle,target);
					bool success = result.success;

					TrainSkill(ch, ESkill::kDazzle, success,target);
					TryToFindProtector(target, ch);
					if (success) {
						affect_to_char(target, af);
						affect_to_char(target, af2);
						act("&YЗачерпнув жмень едкой смеси, Вы метко швырнули ею в глаза $N1! Больше $E не желает Вас видеть!&n",
							false, ch, nullptr,target, kToChar);
						act("$N швырнул$G Вам в глаза какую-то ужасную пыль! Боже, как жжёт глаза!",
							false,target, nullptr, ch, kToChar);
						act("С криком \"А остренького не заказывали?!\", $N швырнул$G $n2 в глаза жмень какой-то смеси!",
							false,target, nullptr, ch, kToNotVict | kToArenaListen);
						if (ch->GetEnemy() != target) {
							stop_fighting(target, false);
						}
					} else {
						act("&WВам не удалось ослепить $N3. Теперь смотрите чтобы $E не выколол$G глаза Вам!&n",
							false, ch, nullptr,target, kToChar);
						act("$N попытал$U ослепить Вас каким-то порошком, но в итоге Вы просто пару раз чихнули. Даже приятно!",
							false,target, nullptr, ch, kToChar);
						act("$N попытал$U ослепить $n3 каким-то порошком, но в итоге только рассыпал$G его.",
							false,target, nullptr, ch, kToNotVict | kToArenaListen);
					}
				}
			}
		} else {
			act("&WНе получилось - $N уже давно понял$G, что от Вас можно ожидать всякого!&n",
				false, ch, nullptr, vict, kToChar);
		}
		--victims_amount;
		if (victims_amount <= 0) {
				break;
		}
	}
	if (!IS_IMMORTAL(ch)) {
		SetSkillCooldownInFight(ch, ESkill::kDazzle, 2);
	}
}
