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
		percent = 0;
	}
	if (GET_GOD_FLAG(ch, EGf::kGodscurse) || GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		prob = 0;
	}
	auto success = (prob >= percent);
	bool need = false;
	bool enough_skill = false;
	if ((GET_REAL_MAX_HIT(vict) > 0 && (GET_HIT(vict) * 100 / GET_REAL_MAX_HIT(vict)) < 31) ||
		(GET_REAL_MAX_HIT(vict) <= 0 && GET_HIT(vict) < GET_REAL_MAX_HIT(vict)) ||
		(GET_HIT(vict) < GET_REAL_MAX_HIT(vict) && CanUseFeat(ch, EFeat::kHealer))) {
		need = true;
		enough_skill = true;
		if (success) {
			int dif = std::min(GET_REAL_MAX_HIT(vict), GET_REAL_MAX_HIT(vict) - GET_HIT(vict));
			int add = std::min(dif, (dif * (prob - percent) / 100) + 1);
			GET_HIT(vict) += add;
		}
	}
	auto spell_id{ESpell::kUndefined};
	for (int count = MAX_FIRSTAID_REMOVE - 1; count >= 0; count--) {
		spell_id = GetRemovableSpellId(count);
		if (IsAffectedBySpell(vict, spell_id)) {
			need = true;
			if (prob / 10  > count) {
				enough_skill = true;
				break;
			}
		}
	}
	if (!need) {
		act("$N в лечении не нуждается.", false, ch, nullptr, vict, kToChar);
	} else if (!enough_skill) {
		act("У вас не хватило умения вылечить $N3.", false, ch, nullptr, vict, kToChar);
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
				act("$n оказал$g первую помощь $N2.",
					true, ch, nullptr, vict, kToNotVict | kToArenaListen);
				if (spell_id != ESpell::kUndefined) {
					RemoveAffectFromChar(vict, spell_id);
				}
				if (GET_SEX(ch) == EGender::kMale)
					sprintf(buf, "%s оказал вам первую помощь.\r\n", ch->get_name().c_str());
				else 
					sprintf(buf, "%s оказала вам первую помощь.\r\n", ch->get_name().c_str());
				SendMsgToChar(buf, vict);
				if (vict->get_wait() > 0)
					vict->set_wait(0);
				update_pos(vict);
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
