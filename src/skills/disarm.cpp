#include "disarm.h"

#include "fightsystem/pk.h"
#include "fightsystem/common.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/fight_start.h"
#include "handler.h"
#include "utils/random.h"
#include "color.h"
#include "structs/global_objects.h"

// ************* DISARM PROCEDURES
void go_disarm(CharData *ch, CharData *vict) {
	ObjData *wielded = GET_EQ(vict, WEAR_WIELD) ? GET_EQ(vict, WEAR_WIELD) :
					   GET_EQ(vict, WEAR_BOTHS), *helded = GET_EQ(vict, WEAR_HOLD);

	if (IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (!((wielded && GET_OBJ_TYPE(wielded) != ObjData::ITEM_LIGHT)
		|| (helded && GET_OBJ_TYPE(helded) != ObjData::ITEM_LIGHT))) {
		return;
	}
	int pos = 0;
	if (number(1, 100) > 30) {
		pos = wielded ? (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD) : WEAR_HOLD;
	} else {
		pos = helded ? WEAR_HOLD : (GET_EQ(vict, WEAR_BOTHS) ? WEAR_BOTHS : WEAR_WIELD);
	}

	if (!pos || !GET_EQ(vict, pos))
		return;
	if (!pk_agro_action(ch, vict))
		return;
	int percent = number(1, MUD::Skills()[ESkill::kDisarm].difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kDisarm, vict);
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(vict, GF_GODSCURSE) || GET_GOD_FLAG(ch, GF_GODSLIKE))
		prob = percent;
	if (IS_IMMORTAL(vict) || GET_GOD_FLAG(ch, GF_GODSCURSE) || GET_GOD_FLAG(vict, GF_GODSLIKE)
		|| can_use_feat(vict, STRONGCLUTCH_FEAT))
		prob = 0;

	bool success = percent <= prob;
	TrainSkill(ch, ESkill::kDisarm, success, vict);
	SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kDisarm].name, percent, prob, success);
	if (!success || GET_EQ(vict, pos)->get_extra_flag(EExtraFlag::ITEM_NODISARM)) {
		send_to_char(ch,
					 "%sВы не сумели обезоружить %s...%s\r\n",
					 CCWHT(ch, C_NRM),
					 GET_PAD(vict, 3),
					 CCNRM(ch, C_NRM));
		prob = 3;
	} else {
		wielded = GET_EQ(vict, pos);
		send_to_char(ch, "%sВы ловко выбили %s из рук %s!%s\r\n",
					 CCIBLU(ch, C_NRM), wielded->get_PName(3).c_str(), GET_PAD(vict, 1), CCNRM(ch, C_NRM));
		send_to_char(vict, "Ловкий удар %s выбил %s%s из ваших рук.\r\n",
					 GET_PAD(ch, 1), wielded->get_PName(3).c_str(), char_get_custom_label(wielded, vict).c_str());
		act("$n ловко выбил$g $o3 из рук $N1.", true, ch, wielded, vict, kToNotVict | kToArenaListen);
		unequip_char(vict, pos, CharEquipFlags());
		SetSkillCooldown(ch, ESkill::kGlobalCooldown, IS_NPC(vict) ? 1 : 2);
		prob = 2;

		if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA) || (!IS_MOB(vict)) || vict->has_master()) {
			obj_to_char(wielded, vict);
		} else {
			obj_to_room(wielded, IN_ROOM(vict));
			obj_decay(wielded);
		};
	}

	appear(ch);
	if (IS_NPC(vict) && CAN_SEE(vict, ch) && vict->have_mind() && GET_WAIT(ch) <= 0) {
		set_hit(vict, ch);
	}
	SetSkillCooldown(ch, ESkill::kDisarm, prob);
	SetSkillCooldown(ch, ESkill::kGlobalCooldown, 1);
}

void do_disarm(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(ESkill::kDisarm)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kDisarm)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого обезоруживаем?\r\n", ch);
		return;
	}

	if (ch == vict) {
		send_to_char("Попробуйте набрать \"снять <название.оружия>\".\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (!((GET_EQ(vict, WEAR_WIELD)
		&& GET_OBJ_TYPE(GET_EQ(vict, WEAR_WIELD)) != ObjData::ITEM_LIGHT)
		|| (GET_EQ(vict, WEAR_HOLD)
			&& GET_OBJ_TYPE(GET_EQ(vict, WEAR_HOLD)) != ObjData::ITEM_LIGHT)
		|| (GET_EQ(vict, WEAR_BOTHS)
			&& GET_OBJ_TYPE(GET_EQ(vict, WEAR_BOTHS)) != ObjData::ITEM_LIGHT))) {
		send_to_char("Вы не можете обезоружить безоружное создание.\r\n", ch);
		return;
	}

	if (IS_IMPL(ch) || !ch->get_fighting()) {
		go_disarm(ch, vict);
	} else if (IsHaveNoExtraAttack(ch)) {
		act("Хорошо. Вы попытаетесь разоружить $N3.", false, ch, nullptr, vict, kToChar);
		ch->set_extra_attack(kExtraAttackDisarm, vict);
	}
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
