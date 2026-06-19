#include "relocate.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/mount.h"

#include "engine/entities/char_data.h"
#include "gameplay/clans/house.h"
#include "engine/ui/color.h"
#include "engine/core/handler.h"
#include "engine/core/target_resolver.h"
#include "gameplay/fight/pk.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/magic/magic_utils.h"          // IsRoomBlocked (issue.no-teleport-out)
#include "engine/db/global_objects.h"            // MUD::Spell
#include "gameplay/core/remort.h"

extern void CheckAutoNosummon(CharData *ch);

void do_relocate(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	struct TimedFeat timed;

	if (!CanUseFeat(ch, EFeat::kRelocate)) {
		SendMsgToChar("Вам это недоступно.\r\n", ch);
		return;
	}

	if (IsTimedByFeat(ch, EFeat::kRelocate)
#ifdef TEST_BUILD
		&& !privilege::IsImmortal(ch)
#endif
		) {
		SendMsgToChar("Невозможно использовать это так часто.\r\n", ch);
		return;
	}

	RoomRnum to_room, fnd_room;
	one_argument(argument, arg);
	if (!*arg) {
		SendMsgToChar("Переместиться на кого?", ch);
		return;
	}

	CharData *victim = target_resolver::FindPlayerVis(ch, arg);

	if (!victim) {
		SendMsgToChar(CommonMsg(ECommonMsg::kNoPerson) + "\r\n", ch);
		return;
	}

	if (victim->IsNpc()) {
		SendMsgToChar("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	if (GetRealLevel(victim) > GetRealLevel(ch) && !group::same_group(ch, victim)) {
		SendMsgToChar("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	// Room-flag gating reuses kRelocate spell's <blocking><room_flags> (single source of
	// truth: edit spells.xml to retune both the cast and this feat). issue.no-teleport-out
	// dropped the inline ROOM_FLAGGED(kNoTeleportOut) check; the same XML config now also
	// extends kNoMagic blocking to the feat (previously code didn't gate on kNoMagic).
	if (!privilege::IsGod(ch)
		&& IsRoomBlocked(world[ch->in_room],
						 MUD::Spell(ESpell::kRelocate).actions.GetBlocking())) {
		SendMsgToChar("Попытка перемещения не удалась.\r\n", ch);
		return;
	}
	if (!privilege::IsGod(ch) && AFF_FLAGGED(ch, EAffect::kNoTeleport)) {
		SendMsgToChar("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	to_room = victim->in_room;

	if (to_room == kNowhere) {
		SendMsgToChar("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	if (!Clan::MayEnter(ch, to_room, kHousePortal))
		fnd_room = Clan::CloseRent(to_room);
	else
		fnd_room = to_room;

	if (fnd_room != to_room && !privilege::IsGod(ch)) {
		SendMsgToChar("Попытка перемещения не удалась.\r\n", ch);
		return;
	}

	if (!privilege::IsGod(ch) &&
		(SECT(fnd_room) == ESector::kSecret ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kSlowDeathTrap) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kTunnel) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kNoRelocateIn) ||
			ROOM_FLAGGED(fnd_room, ERoomFlag::kIceTrap) || (ROOM_FLAGGED(fnd_room, ERoomFlag::kGodsRoom) && !privilege::IsImmortal(ch)))) {
		SendMsgToChar("Попытка перемещения не удалась.\r\n", ch);
		return;
	}
	timed.feat = EFeat::kRelocate;
	if (!enter_wtrigger(world[fnd_room], ch, -1))
			return;
	act("$n медленно исчез$q из виду.", true, ch, nullptr, nullptr, kToRoom);
	SendMsgToChar("Лазурные сполохи пронеслись перед вашими глазами.\r\n", ch);
	RemoveCharFromRoom(ch);
	PlaceCharToRoom(ch, fnd_room);
	mount::Dismount(ch);
	act("$n медленно появил$u откуда-то.", true, ch, nullptr, nullptr, kToRoom);
	if (!(victim->IsFlagged(EPrf::KSummonable) || group::same_group(ch, victim) || privilege::IsImmortal(ch)
		|| ROOM_FLAGGED(fnd_room, ERoomFlag::kArena))) {
		SendMsgToChar(ch, "%sВаш поступок был расценен как потенциально агрессивный.%s\r\n",
					  kColorBoldRed, kColorBoldBlk);
		pkPortal(ch);
		timed.time = 18 - MIN(remort::GetRealRemort(ch), 15);
		SetBattleLag(ch, 3);
		Affect<EApply> af;
		af.duration = CalcDuration(ch, ch, ESkill::kUndefined, 3, 0, 0, 0);
		af.affect_type = EAffect::kNoTeleport;
		af.battleflag = kAfPulsedec;
		affect_to_char(ch, af);
	} else {
		timed.time = 2;
		SetBattleLag(ch, 1);
	}
	ImposeTimedFeat(ch, &timed);
	sight::look_at_room(ch, 0);
	CheckAutoNosummon(victim);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
