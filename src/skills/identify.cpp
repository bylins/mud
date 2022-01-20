#include "identify.h"

#include "entities/char.h"

#include "handler.h"

void do_identify(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharacterData *cvict = nullptr, *caster = ch;
	ObjectData *ovict = nullptr;
	struct Timed timed;
	int k, level = 0;

	if (IS_NPC(ch) || ch->get_skill(SKILL_IDENTIFY) <= 0) {
		send_to_char("Вам стоит сначала этому научиться.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (timed_by_skill(ch, SKILL_IDENTIFY)) {
		send_to_char("Вы же недавно опознавали - подождите чуток.\r\n", ch);
		return;
	}

	k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, caster, &cvict, &ovict);
	if (!k) {
		send_to_char("Похоже, здесь этого нет.\r\n", ch);
		return;
	}
	if (!IS_IMMORTAL(ch)) {
		timed.skill = SKILL_IDENTIFY;
		timed.time = MAX((can_use_feat(ch, CONNOISEUR_FEAT) ? getModifier(CONNOISEUR_FEAT, FEAT_TIMER) : 12)
							 - ((GET_SKILL(ch, SKILL_IDENTIFY) - 25) / 25), 1); //12..5 or 8..1
		timed_to_char(ch, &timed);
	}
	MANUAL_SPELL(skill_identify)
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
