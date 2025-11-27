/**
\file hide.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 29.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "awake.h"

void MakeVisible(CharData *ch, EAffect affect);

// \TODO Слвбо понятно, зачем тут проверять скиллы. По идее, скилл должен наложить на чара аффект.
// Дальше мы просто проверяем силу этого аффекта. Потрейнить скилл можно и отдельно.

int SkipHiding(CharData *ch, CharData *vict) {
	int percent, prob;

	if (MAY_SEE(ch, vict, ch) && IsAffectedBySpell(ch, ESpell::kHide)) {
		if (awake_hide(ch)) {
			SendMsgToChar("Вы попытались спрятаться, но ваша экипировка выдала вас.\r\n", ch);
			RemoveAffectFromChar(ch, ESpell::kHide);
			MakeVisible(ch, EAffect::kHide);
			EXTRA_FLAGS(ch).set(EXTRA_FAILHIDE);
		} else if (IsAffectedBySpell(ch, ESpell::kHide)) {
			if (AFF_FLAGGED(vict, EAffect::kDetectLife)) {
				act("$N почувствовал$G ваше присутствие.", false, ch, nullptr, vict, kToChar);
				return false;
			}
			percent = number(1, 82 + GetRealInt(vict));
			prob = CalcCurrentSkill(ch, ESkill::kHide, vict);
			if (percent > prob) {
				RemoveAffectFromChar(ch, ESpell::kHide);
				AFF_FLAGS(ch).unset(EAffect::kHide);
				act("Вы не сумели остаться незаметным.", false, ch, nullptr, vict, kToChar);
			} else {
				ImproveSkill(ch, ESkill::kHide, true, vict);
				act("Вам удалось остаться незаметным.", false, ch, nullptr, vict, kToChar);
				return true;
			}
		}
	}
	return false;
}

int SkipCamouflage(CharData *ch, CharData *vict) {
	int percent, prob;

	if (MAY_SEE(ch, vict, ch) && IsAffectedBySpell(ch, ESpell::kCamouflage)) {
		if (awake_camouflage(ch)) {
			SendMsgToChar("Вы попытались замаскироваться, но ваша экипировка выдала вас.\r\n", ch);
			RemoveAffectFromChar(ch, ESpell::kCamouflage);
			MakeVisible(ch, EAffect::kDisguise);
			EXTRA_FLAGS(ch).set(EXTRA_FAILCAMOUFLAGE);
		} else if (IsAffectedBySpell(ch, ESpell::kCamouflage)) {
			if (AFF_FLAGGED(vict, EAffect::kDetectLife)) {
				act("$N почувствовал$G ваше присутствие.", false, ch, nullptr, vict, kToChar);
				return false;
			}
			percent = number(1, 82 + GetRealInt(vict));
			prob = CalcCurrentSkill(ch, ESkill::kDisguise, vict);
			if (percent > prob) {
				RemoveAffectFromChar(ch, ESpell::kCamouflage);
				AFF_FLAGS(ch).unset(EAffect::kDisguise);
				ImproveSkill(ch, ESkill::kDisguise, false, vict);
				act("Вы не сумели правильно замаскироваться.", false, ch, nullptr, vict, kToChar);
			} else {
				ImproveSkill(ch, ESkill::kDisguise, true, vict);
				act("Ваша маскировка оказалась на высоте.", false, ch, nullptr, vict, kToChar);
				return true;
			}
		}
	}
	return false;
}

int SkipSneaking(CharData *ch, CharData *vict) {
	int percent, prob, absolute_fail;
	bool try_fail;

	if (MAY_SEE(ch, vict, ch) && IsAffectedBySpell(ch, ESpell::kSneak)) {
		if (awake_sneak(ch))    //if (affected_by_spell(ch,SPELL_SNEAK))
		{
			SendMsgToChar("Вы попытались подкрасться, но ваша экипировка выдала вас.\r\n", ch);
			RemoveAffectFromChar(ch, ESpell::kSneak);
			MakeVisible(ch, EAffect::kSneak);
			RemoveAffectFromChar(ch, ESpell::kHide);
			AFF_FLAGS(ch).unset(EAffect::kHide);
			AFF_FLAGS(ch).unset(EAffect::kSneak);
		} else if (IsAffectedBySpell(ch, ESpell::kSneak)) {
			percent = number(1, 112 + (GetRealInt(vict) * (vict->get_role(static_cast<unsigned>(EMobClass::kBoss)) ? 3 : 1)) +
				(GetRealLevel(vict) > 30 ? GetRealLevel(vict) : 0));
			prob = CalcCurrentSkill(ch, ESkill::kSneak, vict);

			int catch_level = (GetRealLevel(vict) - GetRealLevel(ch));
			if (catch_level > 5) {
				//5% шанс фэйла при prob==200 всегда, при prob = 100 - 10%, если босс, шанс множим на 5
				absolute_fail = ((200 - prob) / 20 + 5) * (vict->get_role(static_cast<unsigned>(EMobClass::kBoss)) ? 5 : 1);
				try_fail = number(1, 100) < absolute_fail;
			} else
				try_fail = false;

			if ((percent > prob) || try_fail) {
				RemoveAffectFromChar(ch, ESpell::kSneak);
				RemoveAffectFromChar(ch, ESpell::kHide);
				AFF_FLAGS(ch).unset(EAffect::kHide);
				AFF_FLAGS(ch).unset(EAffect::kSneak);
				ImproveSkill(ch, ESkill::kSneak, false, vict);
				act("Вы не сумели пробраться незаметно.", false, ch, nullptr, vict, kToChar);
			} else {
				ImproveSkill(ch, ESkill::kSneak, true, vict);
				act("Вам удалось прокрасться незаметно.", false, ch, nullptr, vict, kToChar);
				return true;
			}
		}
	}
	return false;
}

void MakeVisible(CharData *ch, const EAffect affect) {
	char to_room[kMaxStringLength], to_char[kMaxStringLength];

	*to_room = *to_char = 0;

	switch (affect) {
		case EAffect::kHide: strcpy(to_char, "Вы прекратили прятаться.\r\n");
			strcpy(to_room, "$n прекратил$g прятаться.");
			break;

		case EAffect::kDisguise: strcpy(to_char, "Вы прекратили маскироваться.\r\n");
			strcpy(to_room, "$n прекратил$g маскироваться.");
			break;

		default: break;
	}
	AFF_FLAGS(ch).unset(affect);
	ch->check_aggressive = true;
	if (*to_char)
		SendMsgToChar(to_char, ch);
	if (*to_room)
		act(to_room, false, ch, nullptr, nullptr, kToRoom);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
