#include "manadrain.h"
#include "utils/logger.h"
#include "administration/privilege.h"
#include "skill_messages.h"

#include "engine/entities/char_data.h"
#include "gameplay/abilities/timed_abilities.h"
#include "gameplay/fight/pk.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/damage.h"

extern bool CritLuckTest(CharData *ch, CharData *vict);

void do_manadrain(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {

	struct TimedSkill timed;
	int drained_mana, prob, percent, skill;

	one_argument(argument, arg);

	if (ch->IsNpc() || !GetSkill(ch, ESkill::kJinx)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kJinx, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kJinx) || ch->Skills().HasActiveCooldown(ESkill::kJinx)) {
		SendMsgToChar("Так часто не получится.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kJinx, ESkillMsg::kNoTarget) + "\r\n", ch);
		return;
	}
//	if (vict->purged()) {
//		return;
//	}

	if (ch == vict) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kJinx, ESkillMsg::kCantTargetSelf) + "\r\n", ch);
		return;
	}

	if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kPeaceful) || ROOM_FLAGGED(ch->in_room, ERoomFlag::kNoBattle)) {
		SendMsgToChar("Поищите другое место для выражения своих кровожадных наклонностей.\r\n", ch);
		return;
	}

	if (!vict->IsNpc()) {
		SendMsgToChar("На живом человеке? Креста не вас нет!\r\n", ch);
		return;
	}
	if (IsAffected(vict, EAffect::kGodsShield) || vict->IsFlagged(EMobFlag::kProtect)) {
		SendMsgToChar("Боги хранят вашу жертву.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument)) {
		return;
	}

	skill = GetSkill(ch, ESkill::kJinx);

	percent = number(1, MUD::Skill(ESkill::kJinx).difficulty);
	prob = std::max(20, 90 - 5 * std::max(0, GetRealLevel(vict) - GetRealLevel(ch) - std::max(0, (skill - 80) / 6)));
	auto tmp1 = std::max(0, (skill - 80) / 6);
	auto tmp2 = 5 * std::max(0, GetRealLevel(vict) - GetRealLevel(ch) - std::max(0, (skill - 80) / 6));

	SendToTC(ch, true, true, true, "&gСГЛАЗ: percent %d prob %d skillbonus %d difflevel %d&n\r\n", percent, prob, tmp1, tmp2);
	TrainSkill(ch, ESkill::kJinx, percent > prob, vict);
	Damage manadrainDamage(SkillDmg(ESkill::kJinx), fight::kZeroDmg, fight::kMagicDmg, nullptr);
	manadrainDamage.element = EElement::kDark;
	bool success = percent <= prob;
	if (success) {
		skill = std::max(10, skill - 10 * std::max(0, GetRealLevel(ch) - GetRealLevel(vict)));
		drained_mana = (Mana(GetRealWis(ch)) - ch->mem_queue.stored) * skill / 100;
		ch->mem_queue.stored = std::min(Mana(GetRealWis(ch)), ch->mem_queue.stored + drained_mana);
		manadrainDamage.dam = 10;
	}
	manadrainDamage.flags.set(fight::kIgnoreBlink);
	manadrainDamage.Process(ch, vict);

	if (!privilege::IsImmortal(ch)) {
		timed.skill = ESkill::kJinx;
		if (CritLuckTest(ch, vict) || !success)
			timed.time = 1;
		else
			timed.time = 6 - std::min(4, (GetSkill(ch, ESkill::kJinx) + 30) / 50);
		ImposeTimedSkill(ch, &timed);
	}
}
