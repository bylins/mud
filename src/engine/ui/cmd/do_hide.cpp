/**
\file do_hide.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 17.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/skills/skills.h"
#include "engine/db/global_objects.h"


void do_hide(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int prob, percent;

	if (ch->IsNpc() || !GetSkill(ch, ESkill::kHide)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (mount::IsOnHorse(ch)) {
		act("А куда вы хотите спрятать $N3?", false, ch, nullptr, mount::GetHorse(ch), kToChar);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы слепы и не видите куда прятаться.\r\n", ch);
		return;
	}

	if (IsAffected(ch, EAffect::kGlitterDust)) {
		SendMsgToChar("Спрятаться?! Да вы сверкаете как корчма во время гулянки!.\r\n", ch);
		return;
	}

	// Повторная попытка спрятаться разрешена, и так было всегда: прежний узел прятки снимается, и
	// бросок идёт заново -- как это делает "подкрасться". Запрет повтора, стоявший здесь с
	// issue.affect-migration, держала метка неудачной попытки: она живёт столько же, сколько
	// успех (до ~5 минут на навыке 200), флага не поднимает, и выходило "вы уже пытаетесь
	// спрятаться" у персонажа, который в "кто" ничего не прячет и спрятаться не может. Если
	// антиспам на прятку когда-нибудь понадобится, его место -- таймер умения, как IsTimedBySkill
	// у маскировки, а не метка неудачи.
	// Волшебство надетой вещи (kAfFromEquipment) не трогаем: его подавляет только бой.
	RemoveAffectFromCharExceptEquipment(ch, EAffect::kHide);

	SendMsgToChar("Хорошо, вы попытаетесь спрятаться.\r\n", ch);
	ch->Temporary.unset(ECharExtraFlag::kFailHide);
	ch->Temporary.unset(ECharExtraFlag::kToldHide);
	percent = number(1, MUD::Skill(ESkill::kHide).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kHide, nullptr);

	Affect<EApply> af;
	// issue #3664: скилл-скейл прятки. base=1 (пол ~60с), бонус = skill/50, кап total=5.
	// ~60с на низком скилле -> ~5 мин на 200 (шаг 60с; секундная точность тут не нужна).
	af.duration = CalcDuration(ch, ch, ESkill::kHide, 1, 50, 0, 5);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag.clear();

	if (percent > prob) {
		af.affect_type = EAffect::kHide;
		af.battleflag = {kAfFailed};
	} else {
		af.affect_type = EAffect::kHide;
	}

	affect_to_char(ch, af);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
