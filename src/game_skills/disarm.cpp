#include "disarm.h"

#include "game_fight/pk.h"
#include "game_fight/common.h"
#include "game_fight/fight_hit.h"
#include "game_fight/fight_start.h"
#include "handler.h"
#include "utils/random.h"
#include "color.h"
#include "structs/global_objects.h"

// ************* DISARM PROCEDURES
void go_disarm(CharData *ch, CharData *vict) {
	ObjData *wielded = GET_EQ(vict, EEquipPos::kWield) ? GET_EQ(vict, EEquipPos::kWield) :
					   GET_EQ(vict, EEquipPos::kBoths), *helded = GET_EQ(vict, EEquipPos::kHold);

	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (!((wielded && GET_OBJ_TYPE(wielded) != EObjType::kLightSource)
		|| (helded && GET_OBJ_TYPE(helded) != EObjType::kLightSource))) {
		return;
	}
	int pos = 0;
	if (number(1, 100) > 30) {
		pos = wielded ? (GET_EQ(vict, EEquipPos::kBoths) ? EEquipPos::kBoths : EEquipPos::kWield) : EEquipPos::kHold;
	} else {
		pos = helded ? EEquipPos::kHold : (GET_EQ(vict, EEquipPos::kBoths) ? EEquipPos::kBoths : EEquipPos::kWield);
	}

	if (!pos || !GET_EQ(vict, pos))
		return;
	if (!pk_agro_action(ch, vict))
		return;
	int percent = number(1, MUD::Skills()[ESkill::kDisarm].difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kDisarm, vict);
	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(vict, EGf::kGodscurse) || GET_GOD_FLAG(ch, EGf::kGodsLike))
		prob = percent;
	if (IS_IMMORTAL(vict) || GET_GOD_FLAG(ch, EGf::kGodscurse) || GET_GOD_FLAG(vict, EGf::kGodsLike)
		|| IsAbleToUseFeat(vict, EFeat::kStrongClutch))
		prob = 0;

	bool success = percent <= prob;
	TrainSkill(ch, ESkill::kDisarm, success, vict);
	SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kDisarm].name, percent, prob, success);
	if (!success || GET_EQ(vict, pos)->has_flag(EObjFlag::kNodisarm)) {
		SendMsgToChar(ch,
					  "%sВы не сумели обезоружить %s...%s\r\n",
					  CCWHT(ch, C_NRM),
					  GET_PAD(vict, 3),
					  CCNRM(ch, C_NRM));
		prob = 3;
	} else {
		wielded = GET_EQ(vict, pos);
		SendMsgToChar(ch, "%sВы ловко выбили %s из рук %s!%s\r\n",
					  CCIBLU(ch, C_NRM), wielded->get_PName(3).c_str(), GET_PAD(vict, 1), CCNRM(ch, C_NRM));
		SendMsgToChar(vict, "Ловкий удар %s выбил %s%s из ваших рук.\r\n",
					  GET_PAD(ch, 1), wielded->get_PName(3).c_str(), char_get_custom_label(wielded, vict).c_str());
		act("$n ловко выбил$g $o3 из рук $N1.", true, ch, wielded, vict, kToNotVict | kToArenaListen);
		UnequipChar(vict, pos, CharEquipFlags());
		SetSkillCooldown(ch, ESkill::kGlobalCooldown, vict->IsNpc() ? 1 : 2);
		prob = 2;

		if (ROOM_FLAGGED(IN_ROOM(vict), ERoomFlag::kArena) || (!IS_MOB(vict)) || vict->has_master()) {
			PlaceObjToInventory(wielded, vict);
		} else {
			PlaceObjToRoom(wielded, IN_ROOM(vict));
			CheckObjDecay(wielded);
		};
	}

	appear(ch);
	if (vict->IsNpc() && CAN_SEE(vict, ch) && vict->have_mind() && ch->get_wait() <= 0) {
		set_hit(vict, ch);
	}
	SetSkillCooldown(ch, ESkill::kDisarm, prob);
	SetSkillCooldown(ch, ESkill::kGlobalCooldown, 1);
}

void do_disarm(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->get_skill(ESkill::kDisarm)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kDisarm)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		SendMsgToChar("Кого обезоруживаем?\r\n", ch);
		return;
	}

	if (ch == vict) {
		SendMsgToChar("Попробуйте набрать \"снять <название.оружия>\".\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (!((GET_EQ(vict, EEquipPos::kWield)
		&& GET_OBJ_TYPE(GET_EQ(vict, EEquipPos::kWield)) != EObjType::kLightSource)
		|| (GET_EQ(vict, EEquipPos::kHold)
			&& GET_OBJ_TYPE(GET_EQ(vict, EEquipPos::kHold)) != EObjType::kLightSource)
		|| (GET_EQ(vict, EEquipPos::kBoths)
			&& GET_OBJ_TYPE(GET_EQ(vict, EEquipPos::kBoths)) != EObjType::kLightSource))) {
		SendMsgToChar("Вы не можете обезоружить безоружное создание.\r\n", ch);
		return;
	}

	if (IS_IMPL(ch) || !ch->GetEnemy()) {
		go_disarm(ch, vict);
	} else if (IsHaveNoExtraAttack(ch)) {
		act("Хорошо. Вы попытаетесь разоружить $N3.", false, ch, nullptr, vict, kToChar);
		ch->SetExtraAttack(kExtraAttackDisarm, vict);
	}
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
