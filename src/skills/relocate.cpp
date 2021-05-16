#include "relocate.h"

#include "../features.hpp"
#include "../chars/character.h"
#include "../house.h"
#include "../comm.h"
#include "../screen.h"
#include "../structs.h"
#include "../handler.h"
#include "../fightsystem/pk.h"
#include "../dg_script/dg_scripts.h"

void do_relocate(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	struct timed_type timed;

	if (!can_use_feat(ch, RELOCATE_FEAT))
	{
		send_to_char("Вам это недоступно.\r\n", ch);
		return;
	}

	if (timed_by_feat(ch, RELOCATE_FEAT)
#ifdef TEST_BUILD
		&& !IS_IMMORTAL(ch)
#endif
	  )
	{
		send_to_char("Невозможно использовать это так часто.\r\n", ch);
		return;
	}

	room_rnum to_room, fnd_room;
	one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("Переместиться на кого?", ch);
		return;
	}

	CHAR_DATA* victim = get_player_vis(ch, arg, FIND_CHAR_WORLD);
	if (!victim)
	{
		send_to_char(NOPERSON, ch);
		return;
	}

	if (IS_NPC(victim)
		|| (GET_LEVEL(victim) > GET_LEVEL(ch) && !same_group(ch, victim))
		|| IS_IMMORTAL(victim))
	{
		send_to_char("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	if (!IS_GOD(ch))
	{
		if (ROOM_FLAGGED(ch->in_room, ROOM_NOTELEPORTOUT))
		{
			send_to_char("Попытка перемещения не удалась.\r\n", ch);
			return;
		}
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_NOTELEPORT))
		{
			send_to_char("Попытка перемещения не удалась.\r\n", ch);
			return;
		}
	}

	to_room = IN_ROOM(victim);

	if (to_room == NOWHERE)
	{
		send_to_char("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	if (!Clan::MayEnter(ch, to_room, HCE_PORTAL))
		fnd_room = Clan::CloseRent(to_room);
	else
		fnd_room = to_room;

	if (fnd_room != to_room && !IS_GOD(ch))
	{
		send_to_char("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	if (!IS_GOD(ch) &&
			(SECT(fnd_room) == SECT_SECRET ||
			 ROOM_FLAGGED(fnd_room, ROOM_DEATH) ||
			 ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH) ||
			 ROOM_FLAGGED(fnd_room, ROOM_TUNNEL) ||
			 ROOM_FLAGGED(fnd_room, ROOM_NORELOCATEIN) ||
			 ROOM_FLAGGED(fnd_room, ROOM_ICEDEATH) || (ROOM_FLAGGED(fnd_room, ROOM_GODROOM) && !IS_IMMORTAL(ch))))
	{
		send_to_char("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	timed.skill = RELOCATE_FEAT;
	act("$n медленно исчез$q из виду.", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("Лазурные сполохи пронеслись перед вашими глазами.\r\n", ch);
	char_from_room(ch);
	char_to_room(ch, fnd_room);
    ch->dismount();
	act("$n медленно появил$u откуда-то.", TRUE, ch, 0, 0, TO_ROOM);
	if (!(PRF_FLAGGED(victim, PRF_SUMMONABLE) || same_group(ch, victim) || IS_IMMORTAL(ch) || ROOM_FLAGGED(fnd_room, ROOM_ARENA)))
	{
		send_to_char(ch, "%sВаш поступок был расценен как потенциально агрессивный.%s\r\n",
			CCIRED(ch, C_NRM), CCINRM(ch, C_NRM));
		pkPortal(ch);
		timed.time = 18 - MIN(GET_REMORT(ch),15);
		WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
		AFFECT_DATA<EApplyLocation> af;
		af.duration = pc_duration(ch, 3, 0, 0, 0, 0);
		af.bitvector = to_underlying(EAffectFlag::AFF_NOTELEPORT);
		af.battleflag = AF_PULSEDEC;
		affect_to_char(ch, af);
	}
	else
	{
		timed.time = 2;
		WAIT_STATE(ch, PULSE_VIOLENCE);
	}
	timed_feat_to_char(ch, &timed);
	look_at_room(ch, 0);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
