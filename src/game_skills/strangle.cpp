#include "strangle.h"
#include "chopoff.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/common.h"
#include "handler.h"
#include "utils/random.h"
#include "protect.h"
#include "structs/global_objects.h"

void go_strangle(CharData *ch, CharData *vict) {
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || IsUnableToAct(ch)) {
		send_to_char("Сейчас у вас не получится выполнить этот прием.\r\n", ch);
		return;
	}

	if (ch->get_fighting()) {
		send_to_char("Вы не можете делать это в бою!\r\n", ch);
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	vict = TryToFindProtector(vict, ch);
	if (!pk_agro_action(ch, vict)) {
		return;
	}

	act("Вы попытались накинуть удавку на шею $N2.\r\n", false, ch, nullptr, vict, kToChar);

	int prob = CalcCurrentSkill(ch, ESkill::kStrangle, vict);
	int delay = 6 - MIN(4, (ch->get_skill(ESkill::kStrangle) + 30) / 50);
	int percent = number(1, MUD::Skills()[ESkill::kStrangle].difficulty);

	bool success = percent <= prob;
	TrainSkill(ch, ESkill::kStrangle, success, vict);
	SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kStrangle].name, percent, prob, success);
	if (!success) {
		Damage dmg(SkillDmg(ESkill::kStrangle), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.flags.set(fight::kIgnoreArmor);
		dmg.Process(ch, vict);
		SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 3);
	} else {
		Affect<EApplyLocation> af;
		af.type = kSpellStrangle;
		af.duration = IS_NPC(vict) ? 8 : 15;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.battleflag = kAfSameTime;
		af.bitvector = to_underlying(EAffectFlag::AFF_STRANGLED);
		affect_to_char(vict, af);

		int dam =
			(GET_MAX_HIT(vict) * GaussIntNumber((300 + 5 * ch->get_skill(ESkill::kStrangle)) / 70, 7.0, 1, 30)) / 100;
		dam = (IS_NPC(vict) ? MIN(dam, 6 * GET_MAX_HIT(ch)) : MIN(dam, 2 * GET_MAX_HIT(ch)));
		Damage dmg(SkillDmg(ESkill::kStrangle), dam, fight::kPhysDmg, nullptr);
		dmg.flags.set(fight::kIgnoreArmor);
		dmg.Process(ch, vict);
		if (GET_POS(vict) > EPosition::kDead) {
			SetWait(vict, 2, true);
			if (vict->ahorse()) {
				act("Рванув на себя, $N стащил$G Вас на землю.", false, vict, nullptr, ch, kToChar);
				act("Рванув на себя, Вы стащили $n3 на землю.", false, vict, nullptr, ch, kToVict);
				act("Рванув на себя, $N стащил$G $n3 на землю.",
					false,
					vict,
					nullptr,
					ch,
					kToNotVict | kToArenaListen);
				vict->drop_from_horse();
			}
			if (ch->get_skill(ESkill::kUndercut) && ch->isInSameRoom(vict)) {
				go_chopoff(ch, vict);
			}
			SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 2);
		}
	}

	TimedSkill timed;
	timed.skill = ESkill::kStrangle;
	timed.time = delay;
	timed_to_char(ch, &timed);
}

void do_strangle(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (!ch->get_skill(ESkill::kStrangle)) {
		send_to_char("Вы не умеете этого.\r\n", ch);
		return;
	}

	if (IsTimedBySkill(ch, ESkill::kStrangle) || ch->haveCooldown(ESkill::kStrangle)) {
		send_to_char("Так часто душить нельзя - человеки кончатся.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вы жаждете удавить?\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(vict, EAffectFlag::AFF_STRANGLED)) {
		send_to_char("Ваша жертва хватается руками за горло - не подобраться!\r\n", ch);
		return;
	}

	if (IS_UNDEAD(vict)
		|| GET_RACE(vict) == NPC_RACE_FISH
		|| GET_RACE(vict) == NPC_RACE_PLANT
		|| GET_RACE(vict) == NPC_RACE_THING) {
		send_to_char("Вы бы еще верстовой столб удавить попробовали...\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Воспользуйтесь услугами княжеского палача. Постоянным клиентам - скидки!\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument)) {
		return;
	}
	if (!check_pkill(ch, vict, arg)) {
		return;
	}
	go_strangle(ch, vict);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
