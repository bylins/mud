//
// Created by ubuntu on 06/09/20.
//

#include "styles.h"
#include "engine/ui/color.h"
#include "gameplay/fight/pk.h"

const char *cstyles[] = {"normal",
						 "обычный",
						 "punctual",
						 "точный",
						 "awake",
						 "осторожный",
						 "\n"
};

void DoStyle(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if (ch->HasCooldown(ESkill::kGlobalCooldown)) {
		SendMsgToChar("Вам нужно набраться сил.\r\n", ch);
		return;
	};
	int tp;
	one_argument(argument, arg);

	if (!*arg) {
		SendMsgToChar(ch, "Вы сражаетесь %s стилем.\r\n",
					  ch->IsFlagged(EPrf::kPunctual) ? "точным" : ch->IsFlagged(EPrf::kAwake) ? "осторожным"
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
		case 2:ch->UnsetFlag(EPrf::kPunctual);
			ch->UnsetFlag(EPrf::kAwake);

			if (tp == 1) {
				ch->SetFlag(EPrf::kPunctual);
			}
			if (tp == 2) {
				ch->SetFlag(EPrf::kAwake);
			}

			if (ch->GetEnemy() && !(AFF_FLAGGED(ch, EAffect::kCourage) ||
				AFF_FLAGGED(ch, EAffect::kDrunked) || AFF_FLAGGED(ch, EAffect::kAbstinent))) {
				ch->battle_affects.unset(kEafPunctual);
				ch->battle_affects.unset(kEafAwake);
				if (tp == 1)
					ch->battle_affects.set(kEafPunctual);
				else if (tp == 2)
					ch->battle_affects.set(kEafAwake);
			}
			SendMsgToChar(ch, "Вы выбрали %s%s%s стиль боя.\r\n",
						  kColorBoldRed, tp == 0 ? "обычный" : tp == 1 ? "точный" : "осторожный", kColorNrm);
			break;
	}

	ch->setSkillCooldown(ESkill::kGlobalCooldown, 2);
}
