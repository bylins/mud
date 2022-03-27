#include "ironwind.h"

#include "fightsystem/pk.h"
#include "fightsystem/common.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "game_skills/parry.h"

void go_iron_wind(CharData *ch, CharData *victim) {
	if (IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (GET_POS(ch) < EPosition::kFight) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}
	if (PRF_FLAGS(ch).get(PRF_IRON_WIND)) {
		send_to_char("Вы уже впали в неистовство.\r\n", ch);
		return;
	}
	if (ch->get_fighting() && (ch->get_fighting() != victim)) {
		act("$N не сражается с вами, не трогайте $S.", false, ch, nullptr, victim, kToChar);
		return;
	}

	parry_override(ch);

	act("Вас обуяло безумие боя, и вы бросились на $N3!\r\n", false, ch, nullptr, victim, kToChar);
	ObjData *weapon;
	if ((weapon = GET_EQ(ch, EEquipPos::kWield)) || (weapon = GET_EQ(ch, EEquipPos::kBoths))) {
		strcpy(buf, "$n взревел$g и ринул$u на $N3, бешено размахивая $o4!");
		strcpy(buf2, "$N взревел$G и ринул$U на вас, бешено размахивая $o4!");
	} else {
		strcpy(buf, "$n бешено взревел$g и ринул$u на $N3!");
		strcpy(buf2, "$N бешено взревел$G и ринул$U на вас!");
	};
	act(buf, false, ch, weapon, victim, kToNotVict | kToArenaListen);
	act(buf2, false, victim, weapon, ch, kToChar);

	if (!ch->get_fighting()) {
		PRF_FLAGS(ch).set(PRF_IRON_WIND);
		SET_AF_BATTLE(ch, kEafIronWind);
		hit(ch, victim, ESkill::kUndefined, fight::kMainHand);
		SetWait(ch, 2, true);
		//ch->setSkillCooldown(ESkill::kGlobalCooldown, 2);
		//ch->setSkillCooldown(ESkill::kIronwind, 2);
	} else {
		PRF_FLAGS(ch).set(PRF_IRON_WIND);
		SET_AF_BATTLE(ch, kEafIronWind);
	}
}

void do_iron_wind(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(ESkill::kIronwind)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	};
	if (ch->haveCooldown(ESkill::kIronwind)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (GET_AF_BATTLE(ch, kEafOverwhelm) || GET_AF_BATTLE(ch, kEafHammer)) {
		send_to_char("Невозможно! Вы слишкм заняты боем!\r\n", ch);
		return;
	};
	int moves = GET_MAX_MOVE(ch) / (2 + MAX(15, ch->get_skill(ESkill::kIronwind)) / 15);
	if (GET_MAX_MOVE(ch) < moves * 2) {
		send_to_char("Вы слишком устали...\r\n", ch);
		return;
	}
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_COURAGE) && !IS_IMMORTAL(ch) && !GET_GOD_FLAG(ch, GF_GODSLIKE)) {
		send_to_char("Вы слишком здравомыслящи для этого...\r\n", ch);
		return;
	};
	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого вам угодно изрубить в капусту?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы с чувством собственного достоинства мощно пустили ветры... Железные.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument)) {
		return;
	}
	if (!check_pkill(ch, vict, arg)) {
		return;
	}

	go_iron_wind(ch, vict);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp
