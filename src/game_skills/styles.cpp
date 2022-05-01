//
// Created by ubuntu on 06/09/20.
//

#include "styles.h"
#include "color.h"
#include "handler.h"
#include "game_fight/pk.h"
#include "game_fight/common.h"
#include "game_skills/parry.h"

// ************* TOUCH PROCEDURES
void go_touch(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch)) {
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	act("Вы попытаетесь перехватить следующую атаку $N1.", false, ch, nullptr, vict, kToChar);
	SET_AF_BATTLE(ch, kEafTouch);
	ch->set_touching(vict);
}

void do_touch(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kIntercept)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kIntercept)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	ObjData *primary = GET_EQ(ch, EEquipPos::kWield) ? GET_EQ(ch, EEquipPos::kWield) : GET_EQ(ch, EEquipPos::kBoths);
	if (!(IS_IMMORTAL(ch) || ch->IsNpc() || GET_GOD_FLAG(ch, EGf::kGodsLike) || !primary)) {
		SendMsgToChar("У вас заняты руки.\r\n", ch);
		return;
	}

	CharData *vict = nullptr;
	one_argument(argument, arg);
	if (!(vict = get_char_vis(ch, arg, EFind::kCharInRoom))) {
		for (const auto i : world[ch->in_room]->people) {
			if (i->GetEnemy() == ch) {
				vict = i;
				break;
			}
		}

		if (!vict) {
			if (!ch->GetEnemy()) {
				SendMsgToChar("Но вы ни с кем не сражаетесь.\r\n", ch);
				return;
			} else {
				vict = ch->GetEnemy();
			}
		}
	}

	if (ch == vict) {
		SendMsgToChar(GET_NAME(ch), ch);
		SendMsgToChar(", вы похожи на котенка, ловящего собственный хвост.\r\n", ch);
		return;
	}
	if (vict->GetEnemy() != ch && ch->GetEnemy() != vict) {
		act("Но вы не сражаетесь с $N4.", false, ch, nullptr, vict, kToChar);
		return;
	}
	if (GET_AF_BATTLE(ch, kEafHammer)) {
		SendMsgToChar("Невозможно. Вы приготовились к богатырскому удару.\r\n", ch);
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
		SendMsgToChar("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}
	if (ch->IsHorsePrevents()) {
		return;
	};
	SET_AF_BATTLE(ch, kEafDodge);
	SendMsgToChar("Хорошо, вы попытаетесь уклониться от следующей атаки!\r\n", ch);
}

void do_deviate(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->IsNpc() || !ch->GetSkill(ESkill::kDodge)) {
		SendMsgToChar("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->HasCooldown(ESkill::kDodge)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (!(ch->GetEnemy())) {
		SendMsgToChar("Но вы ведь ни с кем не сражаетесь!\r\n", ch);
		return;
	}

	if (ch->IsHorsePrevents()) {
		return;
	}

	if (GET_AF_BATTLE(ch, kEafDodge)) {
		SendMsgToChar("Вы и так вертитесь, как волчок.\r\n", ch);
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
	if (ch->HasCooldown(ESkill::kGlobalCooldown)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	int tp;
	one_argument(argument, arg);

	if (!*arg) {
		SendMsgToChar(ch, "Вы сражаетесь %s стилем.\r\n",
					  PRF_FLAGS(ch).get(EPrf::kPunctual) ? "точным" : PRF_FLAGS(ch).get(EPrf::kAwake) ? "осторожным"
																									  : "обычным");
		return;
	}
	if (TryFlipActivatedFeature(ch, argument)) {
		return;
	}
	if ((tp = search_block(arg, cstyles, false)) == -1) {
		SendMsgToChar("Формат: стиль { название стиля }\r\n", ch);
		return;
	}
	tp >>= 1;
	if ((tp == 1 && !ch->GetSkill(ESkill::kPunctual)) || (tp == 2 && !ch->GetSkill(ESkill::kAwake))) {
		SendMsgToChar("Вам неизвестен такой стиль боя.\r\n", ch);
		return;
	}

	switch (tp) {
		case 0:
		case 1:
		case 2:PRF_FLAGS(ch).unset(EPrf::kPunctual);
			PRF_FLAGS(ch).unset(EPrf::kAwake);

			if (tp == 1) {
				PRF_FLAGS(ch).set(EPrf::kPunctual);
			}
			if (tp == 2) {
				PRF_FLAGS(ch).set(EPrf::kAwake);
			}

			if (ch->GetEnemy() && !(AFF_FLAGGED(ch, EAffect::kCourage) ||
				AFF_FLAGGED(ch, EAffect::kDrunked) || AFF_FLAGGED(ch, EAffect::kAbstinent))) {
				CLR_AF_BATTLE(ch, kEafPunctual);
				CLR_AF_BATTLE(ch, kEafAwake);
				if (tp == 1)
					SET_AF_BATTLE(ch, kEafPunctual);
				else if (tp == 2)
					SET_AF_BATTLE(ch, kEafAwake);
			}
			SendMsgToChar(ch, "Вы выбрали %s%s%s стиль боя.\r\n",
						  CCRED(ch, C_SPR), tp == 0 ? "обычный" : tp == 1 ? "точный" : "осторожный", CCNRM(ch, C_OFF));
			break;
	}

	ch->setSkillCooldown(ESkill::kGlobalCooldown, 2);
}
