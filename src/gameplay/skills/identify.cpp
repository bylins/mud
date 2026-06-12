#include "identify.h"
#include "skill_messages.h"

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/identify.h"
#include "gameplay/skills/skills.h"

static void SkillIdentify(int/* level*/, CharData *ch, CharData *victim, ObjData *obj) {
	if (obj) {
		MortShowObjValues(obj, ch, CalcCurrentSkill(ch, ESkill::kIdentify, nullptr));
		TrainSkill(ch, ESkill::kIdentify, true, nullptr);
	} else if (victim) {
		if (GetRealLevel(victim) < 3) {
			SendMsgToChar("Вы можете опознать только персонажа, достигнувшего третьего уровня.\r\n", ch);
			return;
		}
		MortShowCharValues(victim, ch, CalcCurrentSkill(ch, ESkill::kIdentify, victim));
		TrainSkill(ch, ESkill::kIdentify, true, victim);
	}
}

void do_identify(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *cvict = nullptr, *caster = ch;
	ObjData *ovict = nullptr;
	struct TimedSkill timed;
	int k, level = 0;

	if (ch->IsNpc() || GetSkill(ch, ESkill::kIdentify) <= 0) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kIdentify, ESkillMsg::kDontKnowSkill) + "\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (IsTimedBySkill(ch, ESkill::kIdentify)) {
		SendMsgToChar(MUD::SkillMessages().GetMessage(ESkill::kIdentify, ESkillMsg::kOnCooldown) + "\r\n", ch);
		return;
	}

	k = generic_find(arg, EFind::kCharInRoom | EFind::kObjInventory | EFind::kObjRoom | EFind::kObjEquip, caster, &cvict, &ovict);
	if (!k) {
		SendMsgToChar("Похоже, здесь этого нет.\r\n", ch);
		return;
	}
	if (!ch->IsImmortal()) {
		timed.skill = ESkill::kIdentify;
		auto time = 12;
		time = std::max(1, time);
		timed.time = std::max(time - ((GetSkill(ch, ESkill::kIdentify) - 25) / 25), 1); //12..5 or 8..1
		ImposeTimedSkill(ch, &timed);
	}
	SkillIdentify(level, caster, cvict, ovict);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
