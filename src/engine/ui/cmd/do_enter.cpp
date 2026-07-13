/**
\file enter.cpp - a part of Bylins engine.
\authors Created by Sventovit.
\date 08.09.2024.
\brief 'Do enter' command.
*/

#include "engine/entities/char_data.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/mount.h"
#include "engine/core/char_movement.h"
#include "engine/ui/color.h"
#include "gameplay/fight/common.h"
#include "gameplay/fight/pk.h"
#include "engine/core/char_handler.h"
#include "utils/utils_parse.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/deathtrap.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/magic/magic_rooms.h"   // FindRoomPkPortalUid (issue.affect-flags)

void DoEnter(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	RoomRnum door = kNowhere;
	RoomRnum from_room;
	int fnum;
	const char *p_str = "пентаграмма";
	char *pnumber;

	one_argument(argument, smallBuf);
	pnumber = smallBuf;
	if (!(fnum = get_number(&pnumber))) {
		SendMsgToChar("Здесь такой нет!\r\n", ch);
		return;
	}

	if (*smallBuf) {
		if (isname(smallBuf, p_str)) {

			int i = 0;
			for (const auto &aff : world[ch->in_room]->affected) {
				if (aff->affect_type == room_spells::ERoomAffect::kPortalTimer && ++i == fnum) {
					door = aff->modifier;
				}
			}
			if (door == kNowhere) {
				SendMsgToChar("Вы не видите здесь пентаграмму.\r\n", ch);
			} else {
				from_room = ch->in_room;
				if (mount::IsOnHorse(ch) && AFF_FLAGGED(mount::GetHorse(ch), EAffect::kHold)) {
					act("$Z $N не в состоянии нести вас на себе.\r\n",
						false, ch, nullptr, mount::GetHorse(ch), kToChar);
					return;
				}
				// не пускать в ванрумы после пк, если его там прибьет сразу
				if (deathtrap::check_tunnel_death(ch, door)) {
					SendMsgToChar("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
					return;
				}
				// Если чар под местью, и портал односторонний, то не пускать
				if (NORENTABLE(ch) && !ch->IsNpc()) {
					SendMsgToChar("Грехи мешают вам воспользоваться вратами.\r\n", ch);
					return;
				}
				//проверка на флаг нельзя_верхом
				if (ROOM_FLAGGED(door, ERoomFlag::kNohorse) && mount::IsOnHorse(ch)) {
					act("$Z $N отказывается туда идти, и вам пришлось соскочить.",
						false, ch, nullptr, mount::GetHorse(ch), kToChar);
					mount::Dismount(ch);
				}
				//проверка на ванрум и лошадь
				if (ROOM_FLAGGED(door, ERoomFlag::kTunnel) &&
					(num_pc_in_room(world[door]) > 0 || mount::IsOnHorse(ch))) {
					if (num_pc_in_room(world[door]) > 0) {
						SendMsgToChar("Слишком мало места.\r\n", ch);
						return;
					} else {
						act("$Z $N заупрямил$U, и вам пришлось соскочить.",
							false, ch, nullptr, mount::GetHorse(ch), kToChar);
						mount::Dismount(ch);
					}
				}
				// Обработка флагов NOTELEPORTIN и NOTELEPORTOUT здесь же. PK-пента
				// в комнате-приёмнике теперь хранится на самом аффекте (pk_unique),
				// а не на флаге комнаты (issue.affect-flags).
				if (!privilege::IsImmortal(ch)
					&& ((!ch->IsNpc() && !Clan::MayEnter(ch, door, kHousePortal))
						|| (ROOM_FLAGGED(from_room, ERoomFlag::kNoTeleportOut) || ROOM_FLAGGED(door, ERoomFlag::kNoTeleportIn))
						|| AFF_FLAGGED(ch, EAffect::kNoTeleport)
						|| (room_spells::FindRoomPkPortalUid(world[door]) != 0
							&& (ROOM_FLAGGED(door, ERoomFlag::kArena) || ROOM_FLAGGED(door, ERoomFlag::kHouse))))) {
					sprintf(smallBuf, "%sПентаграмма ослепительно вспыхнула!%s\r\n",
							kColorWht, kColorNrm);
					act(smallBuf, true, ch, nullptr, nullptr, kToChar);
					act(smallBuf, true, ch, nullptr, nullptr, kToRoom);

					SendMsgToChar("Мощным ударом вас отшвырнуло от пентаграммы.\r\n", ch);
					act("$n с визгом отлетел$g от пентаграммы.\r\n", true, ch,
						nullptr, nullptr, kToRoom | kToNotDeaf);
					act("$n отлетел$g от пентаграммы.\r\n", true, ch, nullptr, nullptr, kToRoom | kToDeaf);
					SetBattleLag(ch, 1);
					return;
				}
				if (!enter_wtrigger(world[door], ch, -1))
					return;
				act("$n исчез$q в пентаграмме.", true, ch, nullptr, nullptr, kToRoom);
				// Чужая PK-пента в from_room => агрессивный поступок (issue.affect-flags:
				// поле pkPenterUnique уехало на аффект; helper отдаёт чужой pk_unique,
				// исключая ch's собственный, чтобы вход в свою же пенту не штрафовался).
				if (!privilege::IsImmortal(ch)
					&& room_spells::FindRoomPkPortalUid(world[from_room], ch->get_uid()) != 0) {
					SendMsgToChar(ch, "%sВаш поступок был расценен как потенциально агрессивный.%s\r\n",
								  kColorBoldRed, kColorBoldBlk);
					pkPortal(ch);
				}
				RemoveCharFromRoom(ch);
				PlaceCharToRoom(ch, door);
				greet_mtrigger(ch, -1);
				greet_otrigger(ch, -1);
				SetWait(ch, 3, false);
				act("$n появил$u из пентаграммы.", true, ch, nullptr, nullptr, kToRoom);
				// ищем ангела и лошадь
				for (auto *k : ch->followers) {
					if (mount::IsHorse(k) &&
						!k->GetEnemy() &&
						!AFF_FLAGGED(k, EAffect::kHold) &&
						k->in_room == from_room && AWAKE(k)) {
						if (!ROOM_FLAGGED(door, ERoomFlag::kNohorse)) {
							RemoveCharFromRoom(k);
							PlaceCharToRoom(k, door);
						}
					}
					if (AFF_FLAGGED(k, EAffect::kHelper)
						&& !AFF_FLAGGED(k, EAffect::kHold)
						&& (k->IsFlagged(EMobFlag::kTutelar) || k->IsFlagged(EMobFlag::kMentalShadow))
						&& !k->GetEnemy()
						&& k->in_room == from_room
						&& AWAKE(k)) {
						act("$n исчез$q в пентаграмме.", true,
							k, nullptr, nullptr, kToRoom);
						RemoveCharFromRoom(k);
						PlaceCharToRoom(k, door);
						SetWait(k, 3, false);
						act("$n появил$u из пентаграммы.", true,
							k, nullptr, nullptr, kToRoom);
					}
					if (IsCharmice(k) &&
						!AFF_FLAGGED(k, EAffect::kHold) &&
						k->GetPosition() == EPosition::kStand &&
						k->in_room == from_room) {
						if (fnum > 1) {
							snprintf(buf2, kMaxStringLength, "enter %d.%s", fnum, smallBuf);
						} else {
							snprintf(buf2, kMaxStringLength, "enter %s", smallBuf);
						}
						command_interpreter(k, buf2);
					}
				}
				if (ch->desc != nullptr)
					sight::look_at_room(ch, 0);
			}
		} else {    // an argument was supplied, search for door keyword
			for (door = 0; door < EDirection::kMaxDirNum; door++) {
				if (EXIT(ch, door)
					&& (isname(smallBuf, EXIT(ch, door)->keyword)
						|| isname(smallBuf, EXIT(ch, door)->vkeyword))) {
					PerformMove(ch, door, 1, true, nullptr);
					return;
				}
			}
			sprintf(buf2, "Вы не нашли здесь '%s'.\r\n", smallBuf);
			SendMsgToChar(buf2, ch);
		}
	} else if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kIndoors))
		SendMsgToChar("Вы уже внутри.\r\n", ch);
	else            // try to locate an entrance
	{
		for (door = 0; door < EDirection::kMaxDirNum; door++)
			if (EXIT(ch, door))
				if (EXIT(ch, door)->to_room() != kNowhere)
					if (!EXIT_FLAGGED(EXIT(ch, door), EExitFlag::kClosed) &&
						ROOM_FLAGGED(EXIT(ch, door)->to_room(), ERoomFlag::kIndoors)) {
						PerformMove(ch, door, 1, true, nullptr);
						return;
					}
		SendMsgToChar("Вы не можете найти вход.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
