//
// Created by Svetodar on 29.08.2023.
//

#include "charge.h"

#include "act_movement.h"
#include "entities/char_data.h"
#include "game_fight/pk.h"
#include "game_fight/fight.h"
#include "protect.h"
#include "bash.h"

// ********************* CHARGE PROCEDURE

void go_charge(CharData *ch, int direction) {
	if (IsCorrectDirection(ch, direction, true, false)
		&& !ROOM_FLAGGED(EXIT(ch, direction)->to_room(), ERoomFlag::kDeathTrap)) {
		if (DoSimpleMove(ch, direction, true, nullptr, false)) {
			act("С криком ""За дружину!"" $n ринул$u в бой.",
				false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			SendMsgToChar("С криком ""За дружину!"" Вы ринулись в бой!\r\n", ch);
		}
	}
	int dam = 1;
	int victims_amount = 7;

	Damage dmg(SkillDmg(ESkill::kShieldBash), dam, fight::kPhysDmg, nullptr);

	for (auto vict : world[ch->in_room]->people) {
		if (MOB_FLAGGED(vict, EMobFlag::kProtect) || (!may_kill_here(ch, vict, arg)) || vict == ch) {
			--victims_amount;
			SendMsgToChar("Этого Бендер запретил бить!\r\n", ch);
		} else {
			TryToFindProtector(vict, ch);
			if (ch->GetSkill(ESkill::kShieldBash) && (ch->GetSkill(ESkill::kBash))) {
				SendMsgToChar("Щас будем бить этого!\r\n", ch);
				dmg.Process(ch, vict);
				--victims_amount;
				if (vict->purged()) {
					--victims_amount;
				}
			}
		}
		if (victims_amount <= 0) {
			SendMsgToChar("Конец списка!\r\n", ch);
			break;
		}
	}
}

const char *charge_dirs[] = {"север",
						   "восток",
						   "юг",
						   "запад",
						   "вверх",
						   "вниз",
						   "\n"};

void do_charge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int direction;

	if (AFF_FLAGGED(ch, EAffect::kHold) || ch->get_wait() > 0) {
		SendMsgToChar("Вы временно не в состоянии это сделать.\r\n", ch);
		return;
	}
	if (PRF_FLAGS(ch).get(EPrf::kIronWind)) {
		SendMsgToChar("Невидимые оковы мешают вам ринуться в бой!\r\n", ch);
		return;
	}
	if (GET_POS(ch) != EPosition::kStand) {
		SendMsgToChar("Вы не можете ринуться в бой из этого положения!\r\n", ch);
		return;
	}
	one_argument(argument, arg);
	if ((direction = search_block(arg, dirs, false)) >= 0 ||
		(direction = search_block(arg, charge_dirs, false)) >= 0) {
		go_charge(ch, direction);
		return;
	}
}