#include "flee.h"

#include "act_movement.h"
#include "features.h"
#include "utils/random.h"
#include "entities/char_data.h"
#include "entities/entities_constants.h"
#include <cmath>

void reduce_exp_after_flee(CharData *ch, CharData *victim, RoomRnum room) {
	if (IsAbleToUseFeat(ch, EFeat::kRetreat) || ROOM_FLAGGED(room, ROOM_ARENA))
		return;

	const auto loss = MAX(1, GET_REAL_MAX_HIT(victim) - GET_HIT(victim)) * GetRealLevel(victim);
	gain_exp(ch, -loss);
}

// ********************* FLEE PROCEDURE
void go_flee(CharData *ch) {
	if (AFF_FLAGGED(ch, EAffect::kHold) || GET_WAIT(ch) > 0) {
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kNoFlee) || AFF_FLAGGED(ch, EAffect::kLacky)
		|| PRF_FLAGS(ch).get(EPrf::kIronWind)) {
		send_to_char("Невидимые оковы мешают вам сбежать.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		send_to_char("Вы не можете сбежать из этого положения.\r\n", ch);
		return;
	}

	if (!WAITLESS(ch))
		WAIT_STATE(ch, kPulseViolence);

	if (ch->ahorse() && (GET_POS(ch->get_horse()) < EPosition::kFight || GET_MOB_HOLD(ch->get_horse()))) {
		send_to_char("Ваш скакун не в состоянии вынести вас из боя!\r\n", ch);
		return;
	}

	int dirs[EDirection::kMaxDirNum];
	int correct_dirs = 0;

	for (auto i = 0; i < EDirection::kMaxDirNum; ++i) {
		if (legal_dir(ch, i, true, false) && !ROOM_FLAGGED(EXIT(ch, i)->to_room(), ROOM_DEATH)) {
			dirs[correct_dirs] = i;
			++correct_dirs;
		}
	}

	if (correct_dirs > 0
		&& !bernoulli_trial(std::pow((1.0 - static_cast<double>(correct_dirs) / EDirection::kMaxDirNum), EDirection::kMaxDirNum))) {
		const auto direction = dirs[number(0, correct_dirs - 1)];
		const auto was_fighting = ch->get_fighting();
		const auto was_in = ch->in_room;

		if (do_simple_move(ch, direction, true, 0, true)) {
			act("$n запаниковал$g и пытал$u сбежать!", true, ch, 0, 0, kToRoom | kToArenaListen);
			send_to_char("Вы быстро убежали с поля битвы.\r\n", ch);
			if (ch->ahorse()) {
				act("Верн$W $N вынес$Q вас из боя.", false, ch, 0, ch->get_horse(), kToChar);
			}

			if (was_fighting && !ch->is_npc()) {
				reduce_exp_after_flee(ch, was_fighting, was_in);
			}
		} else {
			act("$n запаниковал$g и попытал$u убежать, но не смог$q!", false, ch, 0, 0, kToRoom | kToArenaListen);
			send_to_char("ПАНИКА ОВЛАДЕЛА ВАМИ. Вы не смогли сбежать!\r\n", ch);
		}
	} else {
		act("$n запаниковал$g и попытал$u убежать, но не смог$q!", false, ch, 0, 0, kToRoom | kToArenaListen);
		send_to_char("ПАНИКА ОВЛАДЕЛА ВАМИ. Вы не смогли сбежать!\r\n", ch);
	}
}

void go_dir_flee(CharData *ch, int direction) {
	if (GET_MOB_HOLD(ch) || GET_WAIT(ch) > 0) {
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kNoFlee) || AFF_FLAGGED(ch, EAffect::kLacky)
		|| PRF_FLAGS(ch).get(EPrf::kIronWind)) {
		send_to_char("Невидимые оковы мешают вам сбежать.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		send_to_char("Вы не сможете сбежать из этого положения.\r\n", ch);
		return;
	}

	if (legal_dir(ch, direction, true, false)
		&& !ROOM_FLAGGED(EXIT(ch, direction)->to_room(), ROOM_DEATH)) {
		if (do_simple_move(ch, direction, true, 0, true)) {
			const auto was_in = ch->in_room;
			const auto was_fighting = ch->get_fighting();

			act("$n запаниковал$g и попытал$u убежать.", false, ch, 0, 0, kToRoom | kToArenaListen);
			send_to_char("Вы быстро убежали с поля битвы.\r\n", ch);
			if (was_fighting && !ch->is_npc()) {
				reduce_exp_after_flee(ch, was_fighting, was_in);
			}

			if (!WAITLESS(ch)) {
				WAIT_STATE(ch, 1 * kPulseViolence);
			}
			return;
		}
	}
	go_flee(ch);
}

const char *FleeDirs[] = {"север",
						  "восток",
						  "юг",
						  "запад",
						  "вверх",
						  "вниз",
						  "\n"};

void do_flee(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int direction = -1;
	if (!ch->get_fighting()) {
		send_to_char("Но вы ведь ни с кем не сражаетесь!\r\n", ch);
		return;
	}
	if (IsAbleToUseFeat(ch, EFeat::kCalmness) || GET_GOD_FLAG(ch, EGf::kGodsLike)) {
		one_argument(argument, arg);
		if ((direction = search_block(arg, dirs, false)) >= 0 ||
			(direction = search_block(arg, FleeDirs, false)) >= 0) {
			go_dir_flee(ch, direction);
			return;
		}
	}
	go_flee(ch);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
