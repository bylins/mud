//
// Created by Svetodar on 26.09.2025.
//

#include "engine/entities/char_data.h"
#include "engine/entities/obj_data.h"
#include "engine/core/handler.h"
#include "engine/db/global_objects.h"
#include "gameplay/fight/common.h"

void do_frenzy(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (!ch->GetSkill(ESkill::kFrenzy)) {
		SendMsgToChar("Вам это не по силам.\r\n", ch);
		return;
	}
	if (ch->GetPosition() != EPosition::kFight
		&& !IS_IMMORTAL(ch)
		&& (!IsAffectedBySpell(ch, ESpell::kCourage)
			|| !IsAffectedBySpell(ch, ESpell::kFrenzy)
			|| !IsAffectedBySpell(ch, ESpell::kBerserk))) {
		SendMsgToChar("Вы не можете впасть в &Rисступление&n находясь в тишине и покое!\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kFrenzy) && !IS_IMMORTAL(ch)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
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

	const int duration = CalcDuration(ch, 23, 0, 0, 0, 0);;
	const int hp_regen = ch->GetSkill(ESkill::kFrenzy) / 12.5;
	const int dmg_multiplier = ch->GetSkill(ESkill::kFrenzy) / 12.5;

	Affect<EApply> af[2];
	af[0].type = ESpell::kFrenzy;
	af[0].duration = duration;
	af[0].modifier = hp_regen;
	af[0].location = EApply::kHpRegen;
	af[0].bitvector = to_underlying(EAffect::kFrenzy);
	af[0].battleflag = kAfPulsedec;
	af[1].type = ESpell::kFrenzy;
	af[1].duration = duration;
	af[1].modifier = dmg_multiplier;
	af[1].location = EApply::kPhysicDamagePercent;
	af[1].bitvector = to_underlying(EAffect::kNoFlee);
	af[1].battleflag = kAfPulsedec;
	bool has_frenzy = false;
	bool can_be_angrier = false;

	for (auto it = ch->affected.begin(); it != ch->affected.end();) {
		auto a = *(*it);  // копия данных эффекта (НЕ ссылка!)

		if (a.type == ESpell::kFrenzy) {
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
			it = ch->AffectRemove(it);
			affect_to_char(ch, a);
			// continue не обязателен: it уже установлен на следующий
		} else {
			++it;
		}
	}

	if (!has_frenzy) {
		SendMsgToChar("&RЖажда крови затмила ваш разум и Вы пришли в исступление!&n\r\n", ch);
		act("$N выпучил$G глаза и издал$G бешеный вопль! Похоже разум окончательно покинул $S...",
			false,nullptr, nullptr, ch, kToNotVict | kToArenaListen);
		for (auto & i : af) {
			ImposeAffect(ch, i, false, false, false, false);
		}
	} else  {
		if (can_be_angrier) {
			SendMsgToChar("&RВы разъярились ещё сильнее!&n\r\n", ch);
		} else {
			SendMsgToChar("&RВы жаждете крови своих соперников!&n\r\n", ch);
		}
	}
	if (!IS_IMMORTAL(ch)) {
		constexpr int cooldown = 7;
		SetSkillCooldown(ch, ESkill::kFrenzy, cooldown);
		SetSkillCooldown(ch, ESkill::kGlobalCooldown, 1);
		ch->set_move(ch->get_move() - MUD::Spell(ESpell::kFrenzy).GetMaxMana());
	}
	TrainSkill(ch, ESkill::kFrenzy, true, nullptr);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp