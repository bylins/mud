/**
\file intercept.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 04.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/db/global_objects.h"
#include "administration/privilege.h"

#include "skill_messages.h"
#include "engine/entities/char_data.h"
#include "gameplay/fight/common.h"
#include "parry.h"
#include "engine/core/handler.h"
#include "engine/core/target_resolver.h"

void GoIntercept(CharData *ch, CharData *vict);
void PerformIntercept(CharData *ch, CharData *vict, HitData &hit_data);

void DoIntercept(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kIntercept)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kIntercept, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kIntercept)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kIntercept, ESkillMsg::kOnCooldown) + "\r\n", ch);
		return;
	};

	ObjData *primary = GET_EQ(ch, EEquipPos::kWield) ? GET_EQ(ch, EEquipPos::kWield) : GET_EQ(ch, EEquipPos::kBoths);
	if (!(privilege::IsImmortal(ch) || ch->IsNpc() || GET_GOD_FLAG(ch, EGf::kGodsLike) || !primary)) {
		SendMsgToChar("У вас заняты руки.\r\n", ch);
		return;
	}

	CharData *vict{nullptr};
	one_argument(argument, arg);
	vict = target_resolver::FindCharInRoom(ch, arg);
	if (!vict) {
		for (const auto i : world[ch->in_room]->people) {
			if (i->GetEnemy() == ch) {
				vict = i;
				break;
			}
		}

		if (!vict) {
			if (!ch->GetEnemy()) {
				SendMsgToChar("Но вы ни с кем не сражаетесь.\r\n", ch);
				return;
			} else {
				vict = ch->GetEnemy();
			}
		}
	}

	if (ch == vict) {
		SendMsgToChar(GET_NAME(ch), ch);
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kIntercept, ESkillMsg::kCantTargetSelf) + "\r\n", ch);
		return;
	}
	if (vict->GetEnemy() != ch && ch->GetEnemy() != vict) {
		act("Но вы не сражаетесь с $N4.", false, ch, nullptr, vict, kToChar);
		return;
	}
	if (ch->battle_affects.get(kEafHammer)) {
		SendMsgToChar("Невозможно. Вы приготовились к богатырскому удару.\r\n", ch);
		return;
	}

	if (!check_pkill(ch, vict, arg))
		return;

	CheckParryOverride(ch);
	GoIntercept(ch, vict);
}

void GoIntercept(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kIntercept, ESkillMsg::kCantFightNow) + "\r\n", ch);
		return;
	}
	act("Вы попытаетесь перехватить следующую атаку $N1.", false, ch, nullptr, vict, kToChar);
	ch->battle_affects.set(kEafTouch);
	ch->set_touching(vict);
}

void ProcessIntercept(CharData *ch, HitData &hit_data) {
	if (!hit_data.hit_no_parry) {
		const auto &people = world[ch->in_room]->people;
		for (const auto vict : people) {
			if (hit_data.dam < 0) {
				break;
			}
			PerformIntercept(ch, vict, hit_data);
		}
	}
}

void PerformIntercept(CharData *ch, CharData *vict, HitData &hit_data) {
	if (vict->get_touching() == ch
		&& !AFF_FLAGGED(vict, EAffect::kStopFight)
		&& !AFF_FLAGGED(vict, EAffect::kMagicStopFight)
		&& !AFF_FLAGGED(vict, EAffect::kStopRight)
		&& vict->get_wait() <= 0
		&& !AFF_FLAGGED(vict, EAffect::kHold)
		&& (privilege::IsImmortal(vict) || vict->IsNpc()
			|| !(GET_EQ(vict, EEquipPos::kWield) || GET_EQ(vict, EEquipPos::kBoths)))
		&& vict->GetPosition() > EPosition::kSleep) {
		int percent = number(1, MUD::Skill(ESkill::kIntercept).difficulty);
		int prob = CalcCurrentSkill(vict, ESkill::kIntercept, ch);
		TrainSkill(vict, ESkill::kIntercept, prob >= percent, ch);
		SendSkillBalanceMsg(ch, MUD::Skill(ESkill::kIntercept).name, percent, prob, prob >= 70);
		if (privilege::IsImmortal(vict) || GET_GOD_FLAG(vict, EGf::kGodsLike)) {
			percent = prob;
		}
		if (GET_GOD_FLAG(vict, EGf::kGodscurse)) {
			percent = 0;
		}
		vict->battle_affects.unset(kEafTouch);
		vict->battle_affects.set(kEafUsedright);
		vict->set_touching(nullptr);
		if (prob < percent) {
			act("Вы не смогли перехватить атаку $N1.", false, vict, nullptr, ch, kToChar);
			act("$N не смог$Q перехватить вашу атаку.", false, ch, nullptr, vict, kToChar);
			act("$n не смог$q перехватить атаку $N1.", true, vict, nullptr, ch, kToNotVict | kToArenaListen);
			prob = 2;
		} else {
			act("Вы перехватили атаку $N1.", false, vict, nullptr, ch, kToChar);
			act("$N перехватил$G вашу атаку.", false, ch, nullptr, vict, kToChar);
			act("$n перехватил$g атаку $N1.", true, vict, nullptr, ch, kToNotVict | kToArenaListen);
			hit_data.dam = -1;
			prob = 1;
		}
		SetSkillCooldownInFight(vict, ESkill::kGlobalCooldown, 1);
		SetSkillCooldownInFight(vict, ESkill::kIntercept, prob);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
