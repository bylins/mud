#include "parry.h"

#include "gameplay/fight/pk.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/fight/common.h"

// **************** MULTYPARRY PROCEDURES
void go_multyparry(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kStopRight) || AFF_FLAGGED(ch, EAffect::kStopLeft) || IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	ch->battle_affects.set(kEafMultyparry);
	SendMsgToChar("Вы попробуете использовать веерную защиту.\r\n", ch);
}

void do_multyparry(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kMultiparry)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kMultiparry)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (!ch->GetEnemy()) {
		SendMsgToChar("Но вы ни с кем не сражаетесь?\r\n", ch);
		return;
	}

	ObjData *primary = GET_EQ(ch, EEquipPos::kWield), *offhand = GET_EQ(ch, EEquipPos::kHold);
	if (!(ch->IsNpc()
		|| (primary
			&& primary->get_type() == EObjType::kWeapon
			&& offhand
			&& offhand->get_type() == EObjType::kWeapon)
		|| IS_IMMORTAL(ch)
		|| GET_GOD_FLAG(ch, EGf::kGodsLike))) {
		SendMsgToChar("Вы не можете отражать атаки безоружным.\r\n", ch);
		return;
	}
	if (ch->battle_affects.get(kEafOverwhelm)) {
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

	ch->battle_affects.set(kEafParry);
	SendMsgToChar("Вы попробуете отклонить следующую атаку.\r\n", ch);
}

void do_parry(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kParry)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kParry)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!ch->GetEnemy()) {
		SendMsgToChar("Но вы ни с кем не сражаетесь?\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch) && !GET_GOD_FLAG(ch, EGf::kGodsLike)) {
		if (GET_EQ(ch, EEquipPos::kBoths)) {
			SendMsgToChar("Вы не можете отклонить атаку двуручным оружием.\r\n", ch);
			return;
		}

		bool prim = 0, offh = 0;
		if (GET_EQ(ch, EEquipPos::kWield) && GET_EQ(ch, EEquipPos::kWield)->get_type() == EObjType::kWeapon) {
			prim = 1;
		}
		if (GET_EQ(ch, EEquipPos::kHold) && GET_EQ(ch, EEquipPos::kHold)->get_type() == EObjType::kWeapon) {
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

	if (ch->battle_affects.get(kEafOverwhelm)) {
		SendMsgToChar("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
		return;
	}
	go_parry(ch);
}

void parry_override(CharData *ch) {
	std::string message = "";
	if (ch->battle_affects.get(kEafBlock)) {
		message = "Вы прекратили прятаться за щит и бросились в бой.";
		ch->battle_affects.unset(kEafBlock);
	}
	if (ch->battle_affects.get(kEafParry)) {
		message = "Вы прекратили парировать атаки и бросились в бой.";
		ch->battle_affects.unset(kEafParry);
	}
	if (ch->battle_affects.get(kEafMultyparry)) {
		message = "Вы забыли о защите и бросились в бой.";
		ch->battle_affects.unset(kEafMultyparry);
	}
	act(message.c_str(), false, ch, 0, 0, kToChar);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :