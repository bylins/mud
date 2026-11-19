/**
\file do_sneak.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 19.09.2024.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/affects/affect_data.h"
#include "gameplay/mechanics/mount.h"
#include "engine/db/global_objects.h"

void do_sneak(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int prob, percent;

	if (ch->IsNpc() || !GetSkill(ch, ESkill::kSneak)) {
		SendMsgToChar("Но вы не знаете как.\r\n", ch);
		return;
	}
	if (mount::IsOnHorse(ch)) {
		act("Вам стоит подумать о мягкой обуви для $N1", false, ch, nullptr, mount::GetHorse(ch), kToChar);
		return;
	}
	if (IsAffected(ch, EAffect::kGlitterDust)) {
		SendMsgToChar("Вы бесшумно крадетесь, отбрасывая тысячи солнечных зайчиков...\r\n", ch);
		return;
	}
	// Та же беда, что в Appear до правки: снимать весь кража-аффект заодно значило убивать узел
	// надетой вещи, и её волшебство не возвращалось до перенадевания. Вещевое не трогаем.
	RemoveAffectFromCharExceptEquipment(ch, EAffect::kSneak);
	SendMsgToChar("Хорошо, вы попытаетесь двигаться бесшумно.\r\n", ch);
	ch->Temporary.unset(ECharExtraFlag::kFailSneak);
	ch->Temporary.unset(ECharExtraFlag::kToldSneak);
	percent = number(1, MUD::Skill(ESkill::kSneak).difficulty);
	prob = CalcCurrentSkill(ch, ESkill::kSneak, nullptr);

	Affect<EApply> af;
	// Нижняя граница 1, иначе при навыке меньше делителя срок выходит нулевым и кража слетает на
	// первом же тике -- через пару секунд после "вы попытаетесь двигаться бесшумно". Старая
	// формула (до перехода на навык) считала в секундах и до нуля не доходила: на первом уровне
	// давала около семи секунд. Новая считает целыми игровыми часами, и 19/20 часа = 0.
	af.duration = CalcDuration(ch, ch, ESkill::kSneak, 0, 20, 1, 1);
	af.modifier = 0;
	af.location = EApply::kNone;
	af.battleflag.clear();
	if (percent > prob) {
		af.affect_type = EAffect::kSneak;
		af.battleflag = {kAfFailed};
	} else {
		af.affect_type = EAffect::kSneak;
	}
	affect_to_char(ch, af);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
