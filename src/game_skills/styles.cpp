//
// Created by ubuntu on 06/09/20.
//

#include "styles.h"
#include "color.h"
#include "handler.h"
#include "fightsystem/pk.h"
#include "fightsystem/common.h"
#include "game_skills/parry.h"

// ************* TOUCH PROCEDURES
void go_touch(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	act("Вы попытаетесь перехватить следующую атаку $N1.", false, ch, nullptr, vict, kToChar);
	SET_AF_BATTLE(ch, kEafTouch);
	ch->set_touching(vict);
}

void do_touch(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(ESkill::kIntercept)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kIntercept)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	ObjData *primary = GET_EQ(ch, WEAR_WIELD) ? GET_EQ(ch, WEAR_WIELD) : GET_EQ(ch,
																				WEAR_BOTHS);
	if (!(IS_IMMORTAL(ch) || IS_NPC(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE) || !primary)) {
		send_to_char("У вас заняты руки.\r\n", ch);
		return;
	}

	CharData *vict = nullptr;
	one_argument(argument, arg);
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		for (const auto i : world[ch->in_room]->people) {
			if (i->get_fighting() == ch) {
				vict = i;
				break;
			}
		}

		if (!vict) {
			if (!ch->get_fighting()) {
				send_to_char("Но вы ни с кем не сражаетесь.\r\n", ch);
				return;
			} else {
				vict = ch->get_fighting();
			}
		}
	}

	if (ch == vict) {
		send_to_char(GET_NAME(ch), ch);
		send_to_char(", вы похожи на котенка, ловящего собственный хвост.\r\n", ch);
		return;
	}
	if (vict->get_fighting() != ch && ch->get_fighting() != vict) {
		act("Но вы не сражаетесь с $N4.", false, ch, nullptr, vict, kToChar);
		return;
	}
	if (GET_AF_BATTLE(ch, kEafHammer)) {
		send_to_char("Невозможно. Вы приготовились к богатырскому удару.\r\n", ch);
		return;
	}

	if (!check_pkill(ch, vict, arg))
		return;

	parry_override(ch);

	go_touch(ch, vict);
}

// ************* DEVIATE PROCEDURES
void go_deviate(CharData *ch) {
	if (IsUnableToAct(ch)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (ch->isHorsePrevents()) {
		return;
	};
	SET_AF_BATTLE(ch, kEafDodge);
	send_to_char("Хорошо, вы попытаетесь уклониться от следующей атаки!\r\n", ch);
}

void do_deviate(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (IS_NPC(ch) || !ch->get_skill(ESkill::kDodge)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kDodge)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!(ch->get_fighting())) {
		send_to_char("Но вы ведь ни с кем не сражаетесь!\r\n", ch);
		return;
	}

	if (ch->isHorsePrevents()) {
		return;
	}

	if (GET_AF_BATTLE(ch, kEafDodge)) {
		send_to_char("Вы и так вертитесь, как волчок.\r\n", ch);
		return;
	};
	go_deviate(ch);
}

const char *cstyles[] = {"normal",
						 "обычный",
						 "punctual",
						 "точный",
						 "awake",
						 "осторожный",
						 "\n"
};

void do_style(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->haveCooldown(ESkill::kGlobalCooldown)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	int tp;
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Вы сражаетесь %s стилем.\r\n",
					 PRF_FLAGS(ch).get(PRF_PUNCTUAL) ? "точным" : PRF_FLAGS(ch).get(PRF_AWAKE) ? "осторожным"
																							   : "обычным");
		return;
	}
	if (TryFlipActivatedFeature(ch, argument)) {
		return;
	}
	if ((tp = search_block(arg, cstyles, false)) == -1) {
		send_to_char("Формат: стиль { название стиля }\r\n", ch);
		return;
	}
	tp >>= 1;
	if ((tp == 1 && !ch->get_skill(ESkill::kPunctual)) || (tp == 2 && !ch->get_skill(ESkill::kAwake))) {
		send_to_char("Вам неизвестен такой стиль боя.\r\n", ch);
		return;
	}

	switch (tp) {
		case 0:
		case 1:
		case 2:PRF_FLAGS(ch).unset(PRF_PUNCTUAL);
			PRF_FLAGS(ch).unset(PRF_AWAKE);

			if (tp == 1) {
				PRF_FLAGS(ch).set(PRF_PUNCTUAL);
			}
			if (tp == 2) {
				PRF_FLAGS(ch).set(PRF_AWAKE);
			}

			if (ch->get_fighting() && !(AFF_FLAGGED(ch, EAffectFlag::AFF_COURAGE) ||
				AFF_FLAGGED(ch, EAffectFlag::AFF_DRUNKED) || AFF_FLAGGED(ch, EAffectFlag::AFF_ABSTINENT))) {
				CLR_AF_BATTLE(ch, kEafPunctual);
				CLR_AF_BATTLE(ch, kEafAwake);
				if (tp == 1)
					SET_AF_BATTLE(ch, kEafPunctual);
				else if (tp == 2)
					SET_AF_BATTLE(ch, kEafAwake);
			}
			send_to_char(ch, "Вы выбрали %s%s%s стиль боя.\r\n",
						 CCRED(ch, C_SPR), tp == 0 ? "обычный" : tp == 1 ? "точный" : "осторожный", CCNRM(ch, C_OFF));
			break;
	}

	ch->setSkillCooldown(ESkill::kGlobalCooldown, 2);
}
