#include "backstab.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight.h"
#include "fightsystem/common.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/fight_start.h"
#include "handler.h"
#include "protect.h"
#include "structs/global_objects.h"

using namespace FightSystem;

// делегат обработки команды заколоть
void do_backstab(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->get_skill(ESkill::kBackstab) < 1) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kBackstab)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (ch->ahorse()) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	one_argument(argument, arg);
	CharData *vict = get_char_vis(ch, arg, FIND_CHAR_ROOM);
	if (!vict) {
		send_to_char("Кого вы так сильно ненавидите, что хотите заколоть?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Вы, определенно, садомазохист!\r\n", ch);
		return;
	}

	if (!GET_EQ(ch, WEAR_WIELD) && (!IS_NPC(ch) || IS_CHARMICE(ch))) {
		send_to_char("Требуется держать оружие в правой руке.\r\n", ch);
		return;
	}

	if ((!IS_NPC(ch) || IS_CHARMICE(ch)) && GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != type_pierce) {
		send_to_char("ЗаКОЛоть можно только КОЛющим оружием!\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (vict->get_fighting() && !can_use_feat(ch, THIEVES_STRIKE_FEAT)) {
		send_to_char("Ваша цель слишком быстро движется - вы можете пораниться!\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;
	go_backstab(ch, vict);
}

// *********************** BACKSTAB VICTIM
// Проверка на стаб в бою происходит до вызова этой функции
void go_backstab(CharData *ch, CharData *vict) {

	if (ch->isHorsePrevents())
		return;

	vict = TryToFindProtector(vict, ch);

	if (!pk_agro_action(ch, vict))
		return;

	if ((MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) && !IS_GOD(ch)) {
		act("Вы заметили, что $N попытал$u вас заколоть!", false, vict, nullptr, ch, kToChar);
		act("$n заметил$g вашу попытку заколоть $s!", false, vict, nullptr, ch, kToVict);
		act("$n заметил$g попытку $N1 заколоть $s!", false, vict, nullptr, ch, kToNotVict | kToArenaListen);
		set_hit(vict, ch);
		return;
	}

	bool success = false;
	if (PRF_FLAGGED(ch, PRF_TESTER)) {
		SkillRollResult result = MakeSkillTest(ch, ESkill::kBackstab, vict);
		success = result.success;
	} else {
		int percent = number(1, MUD::Skills()[ESkill::kBackstab].difficulty);
		int prob = CalcCurrentSkill(ch, ESkill::kBackstab, vict);

		if (can_use_feat(ch, SHADOW_STRIKE_FEAT)) {
			prob = prob + prob * 20 / 100;
		};

		if (vict->get_fighting()) {
			prob = prob * (GET_REAL_DEX(ch) + 50) / 100;
		}

		if (AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE)) {
			prob += 5;
		}
		if (GET_MOB_HOLD(vict)) {
			prob = prob * 5 / 4;
		}
		if (GET_GOD_FLAG(vict, GF_GODSCURSE)) {
			prob = percent;
		}
		if (GET_GOD_FLAG(vict, GF_GODSLIKE) || GET_GOD_FLAG(ch, GF_GODSCURSE)) {
			prob = 0;
		}
		success = percent <= prob;
		SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kBackstab].name, percent, prob, success);
	}

	TrainSkill(ch, ESkill::kBackstab, success, vict);
	if (!success) {
		Damage dmg(SkillDmg(ESkill::kBackstab), kZeroDmg, kPhysDmg, GET_EQ(ch, WEAR_WIELD));
		dmg.process(ch, vict);
	} else {
		hit(ch, vict, ESkill::kBackstab, FightSystem::kMainHand);
	}
	SetWait(ch, 1, true);
	SetSkillCooldownInFight(ch, ESkill::kGlobalCooldown, 1);
	SetSkillCooldownInFight(ch, ESkill::kBackstab, 2);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
