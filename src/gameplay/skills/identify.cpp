#include "identify.h"

#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "engine/db/global_objects.h"

void do_identify(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *cvict = nullptr, *caster = ch;
	ObjData *ovict = nullptr;
	struct TimedSkill timed;
	int k, level = 0;

	if (ch->IsNpc() || ch->GetSkill(ESkill::kIdentify) <= 0) {
		SendMsgToChar("Вам стоит сначала этому научиться.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (IsTimedBySkill(ch, ESkill::kIdentify)) {
		SendMsgToChar("Вы же недавно опознавали - подождите чуток.\r\n", ch);
		return;
	}

	k = generic_find(arg, EFind::kCharInRoom | EFind::kObjInventory | EFind::kObjRoom | EFind::kObjEquip, caster, &cvict, &ovict);
	if (!k) {
		SendMsgToChar("Похоже, здесь этого нет.\r\n", ch);
		return;
	}
	if (!IS_IMMORTAL(ch)) {
		timed.skill = ESkill::kIdentify;
		auto time = 12;
		time = std::max(1, time);
		timed.time = std::max(time - ((ch->GetSkill(ESkill::kIdentify) - 25) / 25), 1); //12..5 or 8..1
		ImposeTimedSkill(ch, &timed);
	}
	MANUAL_SPELL(SkillIdentify)
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
