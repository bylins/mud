#include "gameplay/fight/pk.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/mount.h"
#include "skill_messages.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight.h"
#include "protect.h"
#include "engine/db/global_objects.h"
#include "dazzle.h"
#include "engine/core/target_resolver.h"
#include "engine/entities/char_data.h"

//
// Created by Svetodar on 04.01.2024.
//
void DoDazzle(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!privilege::IsImmortal(ch) && IsUnableToAct(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kDazzle, ESkillMsg::kCantFightNow) + "\r\n", ch);
		return;
	}
	if (!GetSkill(ch, ESkill::kDazzle)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kDazzle, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}
	if (!privilege::IsImmortal(ch) && ch->Skills().HasActiveCooldown(ESkill::kDazzle)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kDazzle, ESkillMsg::kOnCooldown) + "\r\n", ch);
		return;
	}
	if (mount::IsOnHorse(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kDazzle, ESkillMsg::kCantWhileMounted) + "\r\n", ch);
		return;
	}
	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kDazzle, ESkillMsg::kGetOnFeet) + "\r\n", ch);
		return;
	}
	if (!ch->GetEnemy()) {
		SendMsgToChar("Но с Вами никто не сражаеся! Что началось-то? Нормально же сидели...\r\n", ch);
		return;
	}
	CharData *vict = FindVictim(ch, argument);

	if (!vict) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kDazzle, ESkillMsg::kNoTarget) + "\r\n", ch);
		return;
	}
	if (vict == ch) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kDazzle, ESkillMsg::kCantTargetSelf) + "\r\n", ch);
		return;
	}
	if (!privilege::IsImmortal(ch) && IsAffectedWithCasterId(vict, ch, EAffect::kSuspiciousness)) {
		SendMsgToChar("Невозможно ослепить жертву повторно!\r\n", ch);
		return;
	}
	GoDazzle(ch, vict);
}


void GoDazzle(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kDazzle, ESkillMsg::kCantFightNow) + "\r\n", ch);
		return;
	}

	if (ch == vict) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kDazzle, ESkillMsg::kCantTargetSelf) + "\r\n", ch);
		return;
	}

	if (ch->GetPosition() < EPosition::kFight) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kDazzle, ESkillMsg::kGetOnFeet) + "\r\n", ch);
		return;
	}
	bool has_pepper = false;

	for (auto i = ch->carrying;i; i = i->get_next_content()) {
		if (i->get_type() == EObjType::kWand && i->GetSpellItemSpellNum(1) == 356
			&& i->GetPotionValueKey(ObjVal::EValueKey::kCurCharges) > 0) {
			int charges = i->GetPotionValueKey(ObjVal::EValueKey::kCurCharges);
			if (charges > 0) {
				i->set_val(2, charges - 1);
				has_pepper = true;
				break;
			}
		}
	}

	if (!privilege::IsImmortal(ch) && !has_pepper) {
		SendMsgToChar("&WЧем Вы собираетесь ослепить соперника?! У вас нет ни щепотки жгучей смеси!\r\n&n", ch);
		return;
	}

	int victims_amount = 3;
	if (GetSkill(ch, ESkill::kDazzle) > 200) {
		victims_amount = 4;
	};

	Affect<EApply> af;
	af.battleflag = kAfPulsedec;
	af.duration = 150 + (GetSkill(ch, ESkill::kDazzle) * 1.25);
	af.affect_type = EAffect::kBlind;

	Affect<EApply> af2;
	af2.duration = 5;
	af2.battleflag = kNone;
	af2.caster_id = ch->get_uid();
	af2.affect_type = EAffect::kSuspiciousness;

	target_resolver::FoesRosterType roster{ch};
	for (const auto target: roster) {
		if (!IsAffectedWithCasterId(ch, target, EAffect::kSuspiciousness)) {
			if (!privilege::IsImmortal(ch) && !target->IsNpc()) {
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
	if (!privilege::IsImmortal(ch)) {
		SetSkillCooldownInFight(ch, ESkill::kDazzle, 2);
	}
}
