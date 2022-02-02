#include "parry.h"

#include "fightsystem/pk.h"
#include "fightsystem/fight_hit.h"
#include "fightsystem/common.h"

using namespace FightSystem;

// **************** MULTYPARRY PROCEDURES
void go_multyparry(CharacterData *ch) {
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT) || dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	SET_AF_BATTLE(ch, kEafMultyparry);
	send_to_char("Вы попробуете использовать веерную защиту.\r\n", ch);
}

void do_multyparry(CharacterData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(ESkill::kMultiparry)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kMultiparry)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	if (!ch->get_fighting()) {
		send_to_char("Но вы ни с кем не сражаетесь?\r\n", ch);
		return;
	}

	ObjectData *primary = GET_EQ(ch, WEAR_WIELD), *offhand = GET_EQ(ch, WEAR_HOLD);
	if (!(IS_NPC(ch)
		|| (primary
			&& GET_OBJ_TYPE(primary) == ObjectData::ITEM_WEAPON
			&& offhand
			&& GET_OBJ_TYPE(offhand) == ObjectData::ITEM_WEAPON)
		|| IS_IMMORTAL(ch)
		|| GET_GOD_FLAG(ch, GF_GODSLIKE))) {
		send_to_char("Вы не можете отражать атаки безоружным.\r\n", ch);
		return;
	}
	if (GET_AF_BATTLE(ch, kEafOverwhelm)) {
		send_to_char("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
		return;
	}
	go_multyparry(ch);
}

// **************** PARRY PROCEDURES
void go_parry(CharacterData *ch) {
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STOPRIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_STOPLEFT) || dontCanAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	SET_AF_BATTLE(ch, kEafParry);
	send_to_char("Вы попробуете отклонить следующую атаку.\r\n", ch);
}

void do_parry(CharacterData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(ESkill::kParry)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kParry)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!ch->get_fighting()) {
		send_to_char("Но вы ни с кем не сражаетесь?\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch) && !GET_GOD_FLAG(ch, GF_GODSLIKE)) {
		if (GET_EQ(ch, WEAR_BOTHS)) {
			send_to_char("Вы не можете отклонить атаку двуручным оружием.\r\n", ch);
			return;
		}

		bool prim = 0, offh = 0;
		if (GET_EQ(ch, WEAR_WIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ObjectData::ITEM_WEAPON) {
			prim = 1;
		}
		if (GET_EQ(ch, WEAR_HOLD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ObjectData::ITEM_WEAPON) {
			offh = 1;
		}

		if (!prim && !offh) {
			send_to_char("Вы не можете отклонить атаку безоружным.\r\n", ch);
			return;
		} else if (!prim || !offh) {
			send_to_char("Вы можете отклонить атаку только с двумя оружиями в руках.\r\n", ch);
			return;
		}
	}

	if (GET_AF_BATTLE(ch, kEafOverwhelm)) {
		send_to_char("Невозможно! Вы стараетесь оглушить противника.\r\n", ch);
		return;
	}
	go_parry(ch);
}

void parry_override(CharacterData *ch) {
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
	act(message.c_str(), false, ch, 0, 0, TO_CHAR);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :