/**
\file hide.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 29.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "awake.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/hide.h"
#include "gameplay/affects/affect_messages.h"


// \TODO Слвбо понятно, зачем тут проверять скиллы. По идее, скилл должен наложить на чара аффект.
// Дальше мы просто проверяем силу этого аффекта. Потрейнить скилл можно и отдельно.

int SkipHiding(CharData *ch, CharData *vict) {
	int percent, prob;

	if (sight::MaySee(ch, vict, ch) && IsAffectedFlagOnly(ch, EAffect::kHide)) {
		if (awake_hide(ch)) {
			SendMsgToChar("Вы попытались спрятаться, но ваша экипировка выдала вас.\r\n", ch);
			RemoveAffectFromChar(ch, EAffect::kHide);
			MakeVisible(ch, EAffect::kHide);
			ch->Temporary.set(ECharExtraFlag::kFailHide);
		} else if (IsAffectedFlagOnly(ch, EAffect::kHide)) {
			if (AFF_FLAGGED(vict, EAffect::kDetectLife)) {
				act("$N почувствовал$G ваше присутствие.", false, ch, nullptr, vict, kToChar);
				return false;
			}
			percent = number(1, 82 + GetRealInt(vict));
			prob = CalcCurrentSkill(ch, ESkill::kHide, vict);
			if (percent > prob) {
				RemoveAffectFromChar(ch, EAffect::kHide);
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

	if (sight::MaySee(ch, vict, ch) && IsAffectedFlagOnly(ch, EAffect::kDisguise)) {
		if (awake_camouflage(ch)) {
			SendMsgToChar("Вы попытались замаскироваться, но ваша экипировка выдала вас.\r\n", ch);
			RemoveAffectFromChar(ch, EAffect::kDisguise);
			MakeVisible(ch, EAffect::kDisguise);
			ch->Temporary.set(ECharExtraFlag::kFailCamouflage);
		} else if (IsAffectedFlagOnly(ch, EAffect::kDisguise)) {
			if (AFF_FLAGGED(vict, EAffect::kDetectLife)) {
				act("$N почувствовал$G ваше присутствие.", false, ch, nullptr, vict, kToChar);
				return false;
			}
			percent = number(1, 82 + GetRealInt(vict));
			prob = CalcCurrentSkill(ch, ESkill::kDisguise, vict);
			if (percent > prob) {
				RemoveAffectFromChar(ch, EAffect::kDisguise);
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

	if (sight::MaySee(ch, vict, ch) && IsAffectedFlagOnly(ch, EAffect::kSneak)) {
		if (awake_sneak(ch))    //if (affected_by_spell(ch,SPELL_SNEAK))
		{
			SendMsgToChar("Вы попытались подкрасться, но ваша экипировка выдала вас.\r\n", ch);
			RemoveAffectFromChar(ch, EAffect::kSneak);
			MakeVisible(ch, EAffect::kSneak);
			RemoveAffectFromChar(ch, EAffect::kHide);
			AFF_FLAGS(ch).unset(EAffect::kHide);
			AFF_FLAGS(ch).unset(EAffect::kSneak);
		} else if (IsAffectedFlagOnly(ch, EAffect::kSneak)) {
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
				RemoveAffectFromChar(ch, EAffect::kSneak);
				RemoveAffectFromChar(ch, EAffect::kHide);
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
	const std::string &to_char = affects::AffectMsgRaw(affect, affects::EAffectMsgType::kAffDispelledToChar);
	const std::string &to_room = affects::AffectMsgRaw(affect, affects::EAffectMsgType::kAffDispelledToRoom);
	AFF_FLAGS(ch).unset(affect);
	ch->check_aggressive = true;
	if (!to_char.empty()) {
		SendMsgToChar(to_char.c_str(), ch);
	}
	if (!to_room.empty()) {
		act(to_room.c_str(), false, ch, nullptr, nullptr, kToRoom);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
