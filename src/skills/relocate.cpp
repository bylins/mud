#include "relocate.h"

#include "entities/char_data.h"
#include "house.h"
#include "color.h"
#include "handler.h"
#include "fightsystem/pk.h"

extern void CheckAutoNosummon(CharData *ch);

void do_relocate(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	struct TimedFeat timed;

	if (!can_use_feat(ch, RELOCATE_FEAT)) {
		send_to_char("Вам это недоступно.\r\n", ch);
		return;
	}

	if (IsTimed(ch, RELOCATE_FEAT)
#ifdef TEST_BUILD
		&& !IS_IMMORTAL(ch)
#endif
		) {
		send_to_char("Невозможно использовать это так часто.\r\n", ch);
		return;
	}

	RoomRnum to_room, fnd_room;
	one_argument(argument, arg);
	if (!*arg) {
		send_to_char("Переместиться на кого?", ch);
		return;
	}

	CharData *victim = get_player_vis(ch, arg, FIND_CHAR_WORLD);

	if (!victim) {
		send_to_char(NOPERSON, ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	if (GetRealLevel(victim) > GetRealLevel(ch) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) && !same_group(ch, victim)) {
		send_to_char("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	if (!IS_GOD(ch)) {
		if (ROOM_FLAGGED(ch->in_room, ROOM_NOTELEPORTOUT)) {
			send_to_char("Попытка перемещения не удалась.\r\n", ch);
			return;
		}
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_NOTELEPORT)) {
			send_to_char("Попытка перемещения не удалась.\r\n", ch);
			return;
		}
	}

	to_room = IN_ROOM(victim);

	if (to_room == kNowhere) {
		send_to_char("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	if (!Clan::MayEnter(ch, to_room, HCE_PORTAL))
		fnd_room = Clan::CloseRent(to_room);
	else
		fnd_room = to_room;

	if (fnd_room != to_room && !IS_GOD(ch)) {
		send_to_char("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	if (!IS_GOD(ch) &&
		(SECT(fnd_room) == kSectSecret ||
			ROOM_FLAGGED(fnd_room, ROOM_DEATH) ||
			ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH) ||
			ROOM_FLAGGED(fnd_room, ROOM_TUNNEL) ||
			ROOM_FLAGGED(fnd_room, ROOM_NORELOCATEIN) ||
			ROOM_FLAGGED(fnd_room, ROOM_ICEDEATH) || (ROOM_FLAGGED(fnd_room, ROOM_GODROOM) && !IS_IMMORTAL(ch)))) {
		send_to_char("Попытка перемещения не удалась.\r\n", ch);
		return;
	}
	timed.feat = RELOCATE_FEAT;
	if (!enter_wtrigger(world[fnd_room], ch, -1))
			return;
	act("$n медленно исчез$q из виду.", true, ch, nullptr, nullptr, kToRoom);
	send_to_char("Лазурные сполохи пронеслись перед вашими глазами.\r\n", ch);
	char_from_room(ch);
	char_to_room(ch, fnd_room);
	ch->dismount();
	act("$n медленно появил$u откуда-то.", true, ch, nullptr, nullptr, kToRoom);
	if (!(PRF_FLAGGED(victim, PRF_SUMMONABLE) || same_group(ch, victim) || IS_IMMORTAL(ch)
		|| ROOM_FLAGGED(fnd_room, ROOM_ARENA))) {
		send_to_char(ch, "%sВаш поступок был расценен как потенциально агрессивный.%s\r\n",
					 CCIRED(ch, C_NRM), CCINRM(ch, C_NRM));
		pkPortal(ch);
		timed.time = 18 - MIN(GET_REAL_REMORT(ch), 15);
		WAIT_STATE(ch, 3 * kPulseViolence);
		Affect<EApplyLocation> af;
		af.duration = CalcDuration(ch, 3, 0, 0, 0, 0);
		af.bitvector = to_underlying(EAffectFlag::AFF_NOTELEPORT);
		af.battleflag = kAfPulsedec;
		affect_to_char(ch, af);
	} else {
		timed.time = 2;
		WAIT_STATE(ch, kPulseViolence);
	}
	ImposeTimedFeat(ch, &timed);
	look_at_room(ch, 0);
	CheckAutoNosummon(victim);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
