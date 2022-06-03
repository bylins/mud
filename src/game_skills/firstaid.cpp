#include "entities/char_data.h"
#include "handler.h"
#include "structs/global_objects.h"

void DoFirstaid(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	struct TimedSkill timed;

	if (!ch->GetSkill(ESkill::kFirstAid)) {
		SendMsgToChar("Вам следует этому научиться.\r\n", ch);
		return;
	}
	if (!IS_GOD(ch) && IsTimedBySkill(ch, ESkill::kFirstAid)) {
		SendMsgToChar("Так много лечить нельзя - больных не останется.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	CharData *vict;
	if (!*arg) {
		vict = ch;
	} else {
		vict = get_char_vis(ch, arg, EFind::kCharInRoom);
		if (!vict) {
			SendMsgToChar("Кого вы хотите подлечить?\r\n", ch);
			return;
		}
	}

	if (vict->GetEnemy()) {
		act("$N сражается, $M не до ваших телячьих нежностей.", false, ch, nullptr, vict, kToChar);
		return;
	}
	if (vict->IsNpc() && !IS_CHARMICE(vict)) {
		SendMsgToChar("Вы не красный крест - лечить всех подряд.\r\n", ch);
		return;
	}
	int percent = number(1, MUD::Skills()[ESkill::kFirstAid].difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kFirstAid, vict);

	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, EGf::kGodsLike) || GET_GOD_FLAG(vict, EGf::kGodsLike)) {
		percent = prob;
	}
	if (GET_GOD_FLAG(ch, EGf::kGodscurse) || GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		prob = 0;
	}

	auto success = (prob >= percent);
	auto need{false};
	if ((GET_REAL_MAX_HIT(vict) > 0 && (GET_HIT(vict) * 100 / GET_REAL_MAX_HIT(vict)) < 31) ||
		(GET_REAL_MAX_HIT(vict) <= 0 && GET_HIT(vict) < GET_REAL_MAX_HIT(vict)) ||
		(GET_HIT(vict) < GET_REAL_MAX_HIT(vict) && CanUseFeat(ch, EFeat::kHealer))) {
		need = true;
		if (success) {
			if (!PRF_FLAGGED(ch, EPrf::kTester)) {
				int dif = GET_REAL_MAX_HIT(vict) - GET_HIT(vict);
				int add = std::min(dif, (dif * (prob - percent) / 100) + 1);
				GET_HIT(vict) += add;
			} else {
				percent = CalcCurrentSkill(ch, ESkill::kFirstAid, vict);
				prob = static_cast<int>(percent*GetRealLevel(ch)/2);
				SendMsgToChar(ch, "&RУровень цели %d. Восстановлено %dhp , умение %d\r\n",
							  GetRealLevel(vict), prob, percent);
				GET_HIT(vict) += prob;
				GET_HIT(vict) = std::min(GET_HIT(vict), GET_REAL_MAX_HIT(vict));
				update_pos(vict);
			}
		}
	}

	int count = 0;
	auto spell_id{ESpell::kUndefined};
	if (PRF_FLAGGED(ch, EPrf::kTester)) {
		count = (GET_SKILL(ch, ESkill::kFirstAid) - 20) / 30;
		SendMsgToChar(ch, "Снимаю %d аффектов\r\n", count);

		const auto remove_count = vict->remove_random_affects(count);
		SendMsgToChar(ch, "Снято %ld аффектов\r\n", remove_count);

		//
		need = true;
		prob = true;
	} else {
		count = std::min(MAX_FIRSTAID_REMOVE, MAX_FIRSTAID_REMOVE * prob / 100);

		for (percent = 0, prob = need; !need && percent < MAX_FIRSTAID_REMOVE &&
			GetRemovableSpellId(percent) != ESpell::kUndefined; percent++) {
			if (IsAffectedBySpell(vict, GetRemovableSpellId(percent))) {
				need = true;
				if (percent < count) {
					spell_id = GetRemovableSpellId(percent);
					prob = true;
				}
			}
		}
	}

	if (!need) {
		act("$N в лечении не нуждается.", false, ch, nullptr, vict, kToChar);
	} else if (!prob) {
		act("У вас не хватит умения вылечить $N3.", false, ch, nullptr, vict, kToChar);
	} else {
		timed.skill = ESkill::kFirstAid;
		int time = IS_IMMORTAL(ch) ? 1 : IS_PALADINE(ch) ? 4 : IS_SORCERER(ch) ? 2 : 6;
		if (CanUseFeat(ch, EFeat::kPhysicians))
			time /=2;
		timed.time = time;
		ImposeTimedSkill(ch, &timed);
		if (vict != ch) {
			ImproveSkill(ch, ESkill::kFirstAid, success, nullptr);
			if (success) {
				act("Вы оказали первую помощь $N2.", false, ch, nullptr, vict, kToChar);
				act("$N оказал$G вам первую помощь.", false, vict, nullptr, ch, kToChar);
				act("$n оказал$g первую помощь $N2.",
					true, ch, nullptr, vict, kToNotVict | kToArenaListen);
				if (spell_id != ESpell::kUndefined) {
					RemoveAffectFromChar(vict, spell_id);
				}
			} else {
				act("Вы безрезультатно попытались оказать первую помощь $N2.",
					false, ch, nullptr, vict, kToChar);
				act("$N безрезультатно попытал$U оказать вам первую помощь.",
					false, vict, nullptr, ch, kToChar);
				act("$n безрезультатно попытал$u оказать первую помощь $N2.",
					true, ch, nullptr, vict, kToNotVict | kToArenaListen);
			}
		} else {
			if (success) {
				act("Вы оказали себе первую помощь.",
					false, ch, nullptr, nullptr, kToChar);
				act("$n оказал$g себе первую помощь.",
					false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
				if (spell_id != ESpell::kUndefined) {
					RemoveAffectFromChar(vict, spell_id);
				}
			} else {
				act("Вы безрезультатно попытались оказать себе первую помощь.",
					false, ch, nullptr, vict, kToChar);
				act("$n безрезультатно попытал$u оказать себе первую помощь.",
					false, ch, nullptr, vict, kToRoom | kToArenaListen);
			}
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
