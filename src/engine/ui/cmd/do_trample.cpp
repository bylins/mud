#include "engine/entities/char_data.h"
#include "gameplay/fight/pk.h"
#include "gameplay/mechanics/groups.h"

void DoTrample(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *caster;
	int tp, lag = 0;
	const char *targets[] =
		{
			"костер",
			"пламя",
			"огонь",
			"fire",
			"метку",
			"надпись",
			"руны",
			"label",
			"\n"
		};

	if (ch->IsNpc()) {
		return;
	}

	one_argument(argument, arg);

	if ((!*arg) || ((tp = search_block(arg, targets, false)) == -1)) {
		SendMsgToChar("Что вы хотите затоптать?\r\n", ch);
		return;
	}
	tp >>= 2;

	switch (tp) {
		case 0: {
			if (world[ch->in_room]->fires) {
				if (world[ch->in_room]->fires < 5)
					--world[ch->in_room]->fires;
				else
					world[ch->in_room]->fires = 4;
				SendMsgToChar("Вы затоптали костер.\r\n", ch);
				act("$n затоптал$g костер.",
					false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
				if (world[ch->in_room]->fires == 0) {
					SendMsgToChar("Костер потух.\r\n", ch);
					act("Костер потух.",
						false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
				}
				lag = 1;
			} else {
				SendMsgToChar("А тут топтать и нечего :)\r\n", ch);
			}
			break;
		}
		case 1: {
			const auto &room = world[ch->in_room];
			auto aff_i = room->affected.end();
			auto aff_first = room->affected.end();

			//Find own rune label or first run label in room
			for (auto affect_it = room->affected.begin(); affect_it != room->affected.end(); ++affect_it) {
				if (affect_it->get()->type == ESpell::kRuneLabel) {
					if (affect_it->get()->caster_id == GET_UID(ch)) {
						aff_i = affect_it;
						break;
					}

					if (aff_first == room->affected.end()) {
						aff_first = affect_it;
					}
				}
			}

			if (aff_i == room->affected.end()) {
				//Own rune label not found. Use first in room
				aff_i = aff_first;
			}

			if (aff_i != room->affected.end()
				&& (AFF_FLAGGED(ch, EAffect::kDetectMagic)
					|| IS_IMMORTAL(ch)
					|| ch->IsFlagged(EPrf::kCoderinfo))) {
				SendMsgToChar("Шаркнув несколько раз по земле, вы стерли светящуюся надпись.\r\n", ch);
				act("$n шаркнул$g несколько раз по светящимся рунам, полностью их уничтожив.",
					false, ch, nullptr, nullptr, kToRoom | kToArenaListen);

				const auto &aff = *aff_i;
				if (GET_UID(ch) != aff->caster_id) {
					caster = find_char(aff->caster_id);
					if (caster && !same_group(ch, caster)) {
						pk_thiefs_action(ch, caster);
						sprintf(buf,
								"Послышался далекий звук лопнувшей струны, и перед вами промельнул призрачный облик %s.\r\n",
								GET_PAD(ch, 1));
						SendMsgToChar(buf, caster);
					}
				}
				room_spells::RoomRemoveAffect(world[ch->in_room], aff_i);
				lag = 3;
			} else {
				SendMsgToChar("А тут топтать и нечего :)\r\n", ch);
			}
			break;
		}
		default: break;
	}

	if (!IS_IMMORTAL(ch)) {
		SetWaitState(ch, lag * kBattleRound);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
