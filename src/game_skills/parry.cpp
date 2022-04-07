#include "parry.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/common.h"

// **************** MULTYPARRY PROCEDURES
void go_multyparry(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kStopRight) || AFF_FLAGGED(ch, EAffect::kStopLeft) || IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	SET_AF_BATTLE(ch, kEafMultyparry);
	SendMsgToChar("Вы попробуете использовать веерную защиту.\r\n", ch);
}

void do_multyparry(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->is_npc() || !ch->get_skill(ESkill::kMultiparry)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kMultiparry)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (!ch->get_fighting()) {
		SendMsgToChar("Но вы ни с кем не сражаетесь?\r\n", ch);
		return;
	}

	ObjData *primary = GET_EQ(ch, EEquipPos::kWield), *offhand = GET_EQ(ch, EEquipPos::kHold);
	if (!(ch->is_npc()
		|| (primary
			&& GET_OBJ_TYPE(primary) == EObjType::kWeapon
			&& offhand
			&& GET_OBJ_TYPE(offhand) == EObjType::kWeapon)
		|| IS_IMMORTAL(ch)
		|| GET_GOD_FLAG(ch, EGf::kGodsLike))) {
		SendMsgToChar("Вы не можете отражать атаки безоружным.\r\n", ch);
		return;
	}
	if (GET_AF_BATTLE(ch, kEafOverwhelm)) {
		SendMsgToChar("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
		return;
	}
	go_multyparry(ch);
}

// **************** PARRY PROCEDURES
void go_parry(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kStopRight) || AFF_FLAGGED(ch, EAffect::kStopLeft) || IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	SET_AF_BATTLE(ch, kEafParry);
	SendMsgToChar("Вы попробуете отклонить следующую атаку.\r\n", ch);
}

void do_parry(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->is_npc() || !ch->get_skill(ESkill::kParry)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kParry)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!ch->get_fighting()) {
		SendMsgToChar("Но вы ни с кем не сражаетесь?\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch) && !GET_GOD_FLAG(ch, EGf::kGodsLike)) {
		if (GET_EQ(ch, EEquipPos::kBoths)) {
			SendMsgToChar("Вы не можете отклонить атаку двуручным оружием.\r\n", ch);
			return;
		}

		bool prim = 0, offh = 0;
		if (GET_EQ(ch, EEquipPos::kWield) && GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kWield)) == EObjType::kWeapon) {
			prim = 1;
		}
		if (GET_EQ(ch, EEquipPos::kHold) && GET_OBJ_TYPE(GET_EQ(ch, EEquipPos::kHold)) == EObjType::kWeapon) {
			offh = 1;
		}

		if (!prim && !offh) {
			SendMsgToChar("Вы не можете отклонить атаку безоружным.\r\n", ch);
			return;
		} else if (!prim || !offh) {
			SendMsgToChar("Вы можете отклонить атаку только с двумя оружиями в руках.\r\n", ch);
			return;
		}
	}

	if (GET_AF_BATTLE(ch, kEafOverwhelm)) {
		SendMsgToChar("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
		return;
	}
	go_parry(ch);
}

void parry_override(CharData *ch) {
	std::string message = "";
	if (GET_AF_BATTLE(ch, kEafBlock)) {
		message = "Вы прекратили прятаться за щит и бросились в бой.";
		CLR_AF_BATTLE(ch, kEafBlock);
	}
	if (GET_AF_BATTLE(ch, kEafParry)) {
		message = "Вы прекратили парировать атаки и бросились в бой.";
		CLR_AF_BATTLE(ch, kEafParry);
	}
	if (GET_AF_BATTLE(ch, kEafMultyparry)) {
		message = "Вы забыли о защите и бросились в бой.";
		CLR_AF_BATTLE(ch, kEafMultyparry);
	}
	act(message.c_str(), false, ch, 0, 0, kToChar);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :