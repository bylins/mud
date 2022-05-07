#include "mixture.h"

#include "game_magic/spells.h"
#include "game_magic/magic_utils.h"
#include "handler.h"
#include "administration/privilege.h"
#include "game_magic/spells_info.h"
#include "game_mechanics/mem_queue.h"

void do_mixture(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	if (ch->IsNpc())
		return;
	if (IS_IMMORTAL(ch) && !privilege::CheckFlag(ch, privilege::kUseSkills)) {
		SendMsgToChar("Не положено...\r\n", ch);
		return;
	}

	CharData *tch;
	ObjData *tobj;
	RoomData *troom;
	char *s, *t;
	int target = 0;

	// get: blank, spell name, target name
	s = strtok(argument, "'*!");
	if (!s) {
		if (subcmd == SCMD_RUNES)
			SendMsgToChar("Что вы хотите сложить?\r\n", ch);
		else if (subcmd == SCMD_ITEMS)
			SendMsgToChar("Что вы хотите смешать?\r\n", ch);
		return;
	}
	s = strtok(nullptr, "'*!");
	if (!s) {
		SendMsgToChar("Название вызываемой магии смеси должно быть заключено в символы : ' или * или !\r\n", ch);
		return;
	}
	t = strtok(nullptr, "\0");

	auto spell_id = FixNameAndFindSpellId(s);
	if (spell_id == ESpell::kUndefined) {
		SendMsgToChar("И откуда вы набрались рецептов?\r\n", ch);
		return;
	}

	if (((!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kItemCast)
		&& subcmd == SCMD_ITEMS)
		|| (!IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kRunes)
			&& subcmd == SCMD_RUNES)) && !IS_GOD(ch)) {
		SendMsgToChar("Это блюдо вам явно не понравится.\r\n" "Научитесь его правильно готовить.\r\n", ch);
		return;
	}

	if (!CheckRecipeValues(ch, spell_id, subcmd == SCMD_ITEMS ? ESpellType::kItemCast : ESpellType::kRunes, false))
		return;

	if (!CheckRecipeItems(ch, spell_id, subcmd == SCMD_ITEMS ? ESpellType::kItemCast : ESpellType::kRunes, false)) {
		if (subcmd == SCMD_ITEMS)
			SendMsgToChar("У вас нет нужных ингредиентов!\r\n", ch);
		else if (subcmd == SCMD_RUNES)
			SendMsgToChar("У вас нет нужных рун!\r\n", ch);
		return;
	}

	// Find the target
	if (t != nullptr)
		one_argument(t, arg);
	else
		*arg = '\0';

	target = FindCastTarget(spell_id, arg, ch, &tch, &tobj, &troom);

	if (target && (tch == ch) && spell_info[spell_id].violent) {
		SendMsgToChar("Лекари не рекомендуют использовать ЭТО на себя!\r\n", ch);
		return;
	}

	if (!target) {
		SendMsgToChar("Тяжеловато найти цель для вашей магии!\r\n", ch);
		return;
	}

	if (tch != ch && !IS_IMMORTAL(ch) && IS_SET(spell_info[spell_id].targets, kTarSelfOnly)) {
		SendMsgToChar("Вы можете колдовать это только на себя!\r\n", ch);
		return;
	}

	if (IS_MANA_CASTER(ch)) {
		if (GetRealLevel(ch) < CalcRequiredLevel(ch, spell_id)) {
			SendMsgToChar("Вы еще слишком малы, чтобы колдовать такое.\r\n", ch);
			return;
		}

		if (ch->mem_queue.stored < CalcSpellManacost(ch, spell_id)) {
			SendMsgToChar("У вас маловато магической энергии!\r\n", ch);
			return;
		} else {
			ch->mem_queue.stored = ch->mem_queue.stored - CalcSpellManacost(ch, spell_id);
		}
	}

	if (CheckRecipeItems(ch, spell_id, subcmd == SCMD_ITEMS ? ESpellType::kItemCast : ESpellType::kRunes, true, tch)) {
		if (!CalcCastSuccess(ch, tch, ESaving::kStability, spell_id)) {
			SetWaitState(ch, kPulseViolence);
			if (!tch || !SendSkillMessages(0, ch, tch, spell_id)) {
				if (subcmd == SCMD_ITEMS)
					SendMsgToChar("Вы неправильно смешали ингредиенты!\r\n", ch);
				else if (subcmd == SCMD_RUNES)
					SendMsgToChar("Вы не смогли правильно истолковать значение рун!\r\n", ch);
			}
		} else {
			if (CallMagic(ch, tch, tobj, world[ch->in_room], spell_id, GetRealLevel(ch)) >= 0) {
				if (!(IS_IMMORTAL(ch) || ch->get_wait() > 0 ))
					SetWaitState(ch, kPulseViolence);
			}
		}
	}
}
