#include "manadrain.h"

#include "handler.h"
#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/common.h"
#include "fightsystem/fight_hit.h"
#include "structs/global_objects.h"

void do_manadrain(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {

	struct TimedSkill timed;
	int drained_mana, prob, percent, skill;

	one_argument(argument, arg);

	if (ch->is_npc() || !ch->get_skill(ESkill::kJinx)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kJinx) || ch->haveCooldown(ESkill::kJinx)) {
		send_to_char("Так часто не получится.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы столь сильно ненавидите?\r\n", ch);
		return;
	}

	if (ch == vict) {
		send_to_char("Вы укусили себя за левое ухо.\r\n", ch);
		return;
	}

	if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) || ROOM_FLAGGED(ch->in_room, ROOM_NOBATTLE)) {
		send_to_char("Поищите другое место для выражения своих кровожадных наклонностей.\r\n", ch);
		return;
	}

	if (!vict->is_npc()) {
		send_to_char("На живом человеке? Креста не вас нет!\r\n", ch);
		return;
	}

	if (affected_by_spell(vict, kSpellGodsShield) || MOB_FLAGGED(vict, MOB_PROTECT)) {
		send_to_char("Боги хранят вашу жертву.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument)) {
		return;
	}

	skill = ch->get_skill(ESkill::kJinx);

	percent = number(1, MUD::Skills()[ESkill::kJinx].difficulty);
	prob = MAX(20, 90 - 5 * MAX(0, GetRealLevel(vict) - GetRealLevel(ch)));
	ImproveSkill(ch, ESkill::kJinx, percent > prob, vict);

	Damage manadrainDamage(SkillDmg(ESkill::kJinx), fight::kZeroDmg, fight::kMagicDmg, nullptr);
	manadrainDamage.magic_type = kTypeDark;
	if (percent <= prob) {
		skill = MAX(10, skill - 10 * MAX(0, GetRealLevel(ch) - GetRealLevel(vict)));
		drained_mana = (GET_MAX_MANA(ch) - GET_MANA_STORED(ch)) * skill / 100;
		GET_MANA_STORED(ch) = MIN(GET_MAX_MANA(ch), GET_MANA_STORED(ch) + drained_mana);
		manadrainDamage.dam = 10;
	}
	manadrainDamage.Process(ch, vict);

	if (!IS_IMMORTAL(ch)) {
		timed.skill = ESkill::kJinx;
		timed.time = 6 - MIN(4, (ch->get_skill(ESkill::kJinx) + 30) / 50);
		timed_to_char(ch, &timed);
	}

}
