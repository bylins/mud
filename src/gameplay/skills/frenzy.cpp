//
// Created by Svetodar on 26.09.2025.
//

#include "engine/entities/char_data.h"
#include "gameplay/magic/magic.h"
#include "administration/privilege.h"
#include "gameplay/affects/affect_handler.h"
#include "skill_messages.h"
#include "engine/entities/obj_data.h"
#include "engine/db/global_objects.h"
#include "gameplay/fight/common.h"

#include <vector>

void do_frenzy(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (!GetSkill(ch, ESkill::kFrenzy)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kFrenzy, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}
	if (ch->GetPosition() != EPosition::kFight
		&& !privilege::IsImmortal(ch)
		&& (!IsAffected(ch, EAffect::kCourage)
			|| !IsAffected(ch, EAffect::kFrenzy)
			|| !IsAffectedOrAttempting(ch, EAffect::kBerserk))) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kFrenzy, ESkillMsg::kPeacefulRoom) + "\r\n", ch);
		return;
	}
	if (ch->Skills().HasActiveCooldown(ESkill::kFrenzy) && !privilege::IsImmortal(ch)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kFrenzy, ESkillMsg::kOnCooldown) + "\r\n", ch);
		return;
	}
	if (ch->get_move() < MUD::Spell(ESpell::kFrenzy).GetMaxMana()) {
		SendMsgToChar("У вас не хватает сил чтобы впасть в &Rисступление&n!\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kGroup)) {
		SendMsgToChar("Сперва уединитесь! В беспамятстве и ярости можно и добрый люд покалечить!\r\n", ch);
		return;
	}

	const int duration = CalcDuration(ch, ch, ESkill::kUndefined, 23, 0, 0, 0);;
	const int hp_regen = GetSkill(ch, ESkill::kFrenzy) / 12.5;
	const int dmg_multiplier = GetSkill(ch, ESkill::kFrenzy) / 12.5;

	Affect<EApply> af[3];
	af[0].duration = duration;
	af[0].modifier = hp_regen;
	af[0].location = EApply::kHpRegen;
	af[0].affect_type = EAffect::kFrenzy;
	af[0].battleflag = kAfPulsedec;
	af[1].duration = duration;
	af[1].modifier = dmg_multiplier;
	af[1].location = EApply::kPhysicDamagePercent;
	af[1].affect_type = EAffect::kFrenzy;
	af[1].battleflag = kAfPulsedec;
	// issue.affects-improve: frenzy prevents fleeing via EApply::kBind (was the kNoFlee affect).
	af[2].duration = duration;
	af[2].modifier = 1;
	af[2].location = EApply::kBind;
	af[2].affect_type = EAffect::kFrenzy;
	af[2].battleflag = kAfPulsedec;
	bool has_frenzy = false;
	bool can_be_angrier = false;
	// В цикле только СНИМАЕМ старые frenzy-аффекты и СОБИРАЕМ обновлённые копии.
	// affect_to_char нельзя звать внутри: он меняет ch->affected и зовёт
	// affect_total()/EmitAffectEvent, что инвалидирует итератор it (краш).
	std::vector<Affect<EApply>> readd;

	for (auto it = ch->affected.begin(); it != ch->affected.end();) {
		// issue.affects-improve: guard on the affect identity (EAffect::kFrenzy) -- the spell-typed
		// (*it)->type / ESpell::kFrenzy field is gone in the affect-migration model. The body keeps
		// master's iterator-safe rework (collect into `readd`, affect_to_char only AFTER the loop).
		if ((*it)->affect_type != EAffect::kFrenzy) {
			++it;
			continue;
		}
		auto a = *(*it);  // копия данных эффекта (НЕ ссылка!)
		has_frenzy = true;

		if (a.location == EApply::kHpRegen && a.modifier < af[0].modifier * 5) {
			a.modifier += af[0].modifier;
			can_be_angrier = true;
		}
		if (a.location == EApply::kPhysicDamagePercent && a.modifier < af[1].modifier * 5) {
			a.modifier += af[1].modifier;
			can_be_angrier = true;
		}
		a.duration = duration;
		readd.push_back(a);
		it = RemoveAffect(ch, it);  // только erase, возвращает валидный следующий
	}
	// Применяем собранные аффекты уже вне итерации по списку.
	for (const auto &a : readd) {
		affect_to_char(ch, a);
	}

	if (!has_frenzy) {
		for (auto & i : af) {
			ImposeAffect(ch, i, false, false, false, false);
		}
		// issue.affect-migration: imposition narration on the kFrenzy affect (self; no opponent).
		EmitAffectImpose(ch, nullptr, EAffect::kFrenzy, false);
	} else  {
		if (can_be_angrier) {
			SendMsgToChar("&RВы разъярились ещё сильнее!&n\r\n", ch);
		} else {
			SendMsgToChar("&RВы жаждете крови своих соперников!&n\r\n", ch);
		}
	}
	if (!privilege::IsImmortal(ch)) {
		constexpr int cooldown = 7;
		SetSkillCooldown(ch, ESkill::kFrenzy, cooldown);
		SetSkillCooldown(ch, ESkill::kGlobalCooldown, 1);
		ch->set_move(ch->get_move() - MUD::Spell(ESpell::kFrenzy).GetMaxMana());
	}
	TrainSkill(ch, ESkill::kFrenzy, true, nullptr);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp