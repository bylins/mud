#include "identify.h"

#include "entities/char_data.h"

#include "handler.h"

void do_identify(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *cvict = nullptr, *caster = ch;
	ObjData *ovict = nullptr;
	struct TimedSkill timed;
	int k, level = 0;

	if (ch->is_npc() || ch->get_skill(ESkill::kIdentify) <= 0) {
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
		timed.time = MAX((IsAbleToUseFeat(ch, EFeat::kConnoiseur) ? GetModifier(EFeat::kConnoiseur, kFeatTimer) : 12)
							 - ((GET_SKILL(ch, ESkill::kIdentify) - 25) / 25), 1); //12..5 or 8..1
		ImposeTimedSkill(ch, &timed);
	}
	MANUAL_SPELL(SkillIdentify)
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
