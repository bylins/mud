/**
\file enter.cpp - a part of Bylins engine.
\authors Created by Sventovit.
\date 08.09.2024.
\brief 'Do enter' command.
*/

#include "entities/char_data.h"
#include "color.h"
#include "game_fight/common.h"
#include "game_fight/pk.h"
#include "handler.h"
#include "house.h"
#include "game_mechanics/deathtrap.h"

void do_enter(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	RoomRnum door = kNowhere;
	RoomRnum from_room;
	int fnum;
	const char *p_str = "пентаграмма";
	struct FollowerType *k, *k_next;
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
				if (aff->type == ESpell::kPortalTimer && aff->bitvector != room_spells::ERoomAffect::kNoPortalExit && ++i == fnum) {
					door = aff->modifier;
				}
			}
			if (door == kNowhere) {
				SendMsgToChar("Вы не видите здесь пентаграмму.\r\n", ch);
			} else {
				from_room = ch->in_room;
				if (ch->IsOnHorse() && AFF_FLAGGED(ch->get_horse(), EAffect::kHold)) {
					act("$Z $N не в состоянии нести вас на себе.\r\n",
						false, ch, nullptr, ch->get_horse(), kToChar);
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
				if (ROOM_FLAGGED(door, ERoomFlag::kNohorse) && ch->IsOnHorse()) {
					act("$Z $N отказывается туда идти, и вам пришлось соскочить.",
						false, ch, nullptr, ch->get_horse(), kToChar);
					ch->dismount();
				}
				//проверка на ванрум и лошадь
				if (ROOM_FLAGGED(door, ERoomFlag::kTunnel) &&
					(num_pc_in_room(world[door]) > 0 || ch->IsOnHorse())) {
					if (num_pc_in_room(world[door]) > 0) {
						SendMsgToChar("Слишком мало места.\r\n", ch);
						return;
					} else {
						act("$Z $N заупрямил$U, и вам пришлось соскочить.",
							false, ch, nullptr, ch->get_horse(), kToChar);
						ch->dismount();
					}
				}
				// Обработка флагов NOTELEPORTIN и NOTELEPORTOUT здесь же
				if (!IS_IMMORTAL(ch)
					&& ((!ch->IsNpc() && !Clan::MayEnter(ch, door, kHousePortal))
						|| (ROOM_FLAGGED(from_room, ERoomFlag::kNoTeleportOut) || ROOM_FLAGGED(door, ERoomFlag::kNoTeleportIn))
						|| AFF_FLAGGED(ch, EAffect::kNoTeleport)
						|| (world[door]->pkPenterUnique
							&& (ROOM_FLAGGED(door, ERoomFlag::kArena) || ROOM_FLAGGED(door, ERoomFlag::kHouse))))) {
					sprintf(smallBuf, "%sПентаграмма ослепительно вспыхнула!%s\r\n",
							CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
					act(smallBuf, true, ch, nullptr, nullptr, kToChar);
					act(smallBuf, true, ch, nullptr, nullptr, kToRoom);

					SendMsgToChar("Мощным ударом вас отшвырнуло от пентаграммы.\r\n", ch);
					act("$n с визгом отлетел$g от пентаграммы.\r\n", true, ch,
						nullptr, nullptr, kToRoom | kToNotDeaf);
					act("$n отлетел$g от пентаграммы.\r\n", true, ch, nullptr, nullptr, kToRoom | kToDeaf);
					SetWaitState(ch, kBattleRound);
					return;
				}
				if (!enter_wtrigger(world[door], ch, -1))
					return;
				act("$n исчез$q в пентаграмме.", true, ch, nullptr, nullptr, kToRoom);
				if (world[from_room]->pkPenterUnique && world[from_room]->pkPenterUnique != GET_UNIQUE(ch)
					&& !IS_IMMORTAL(ch)) {
					SendMsgToChar(ch, "%sВаш поступок был расценен как потенциально агрессивный.%s\r\n",
								  CCIRED(ch, C_NRM), CCINRM(ch, C_NRM));
					pkPortal(ch);
				}
				RemoveCharFromRoom(ch);
				PlaceCharToRoom(ch, door);
				greet_mtrigger(ch, -1);
				greet_otrigger(ch, -1);
				SetWait(ch, 3, false);
				act("$n появил$u из пентаграммы.", true, ch, nullptr, nullptr, kToRoom);
				// ищем ангела и лошадь
				for (k = ch->followers; k; k = k_next) {
					k_next = k->next;
					if (IS_HORSE(k->follower) &&
						!k->follower->GetEnemy() &&
						!AFF_FLAGGED(k->follower, EAffect::kHold) &&
						k->follower->in_room == from_room && AWAKE(k->follower)) {
						if (!ROOM_FLAGGED(door, ERoomFlag::kNohorse)) {
							RemoveCharFromRoom(k->follower);
							PlaceCharToRoom(k->follower, door);
						}
					}
					if (AFF_FLAGGED(k->follower, EAffect::kHelper)
						&& !AFF_FLAGGED(k->follower, EAffect::kHold)
						&& (k->follower->IsFlagged(EMobFlag::kTutelar) || k->follower->IsFlagged(EMobFlag::kMentalShadow))
						&& !k->follower->GetEnemy()
						&& k->follower->in_room == from_room
						&& AWAKE(k->follower)) {
						act("$n исчез$q в пентаграмме.", true,
							k->follower, nullptr, nullptr, kToRoom);
						RemoveCharFromRoom(k->follower);
						PlaceCharToRoom(k->follower, door);
						SetWait(k->follower, 3, false);
						act("$n появил$u из пентаграммы.", true,
							k->follower, nullptr, nullptr, kToRoom);
					}
					if (IS_CHARMICE(k->follower) &&
						!AFF_FLAGGED(k->follower, EAffect::kHold) &&
						k->follower->GetPosition() == EPosition::kStand &&
						k->follower->in_room == from_room) {
						snprintf(buf2, kMaxStringLength, "войти пентаграмма");
						command_interpreter(k->follower, buf2);
					}
				}
				if (ch->desc != nullptr)
					look_at_room(ch, 0);
			}
		} else {    // an argument was supplied, search for door keyword
			for (door = 0; door < EDirection::kMaxDirNum; door++) {
				if (EXIT(ch, door)
					&& (isname(smallBuf, EXIT(ch, door)->keyword)
						|| isname(smallBuf, EXIT(ch, door)->vkeyword))) {
					perform_move(ch, door, 1, true, nullptr);
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
						perform_move(ch, door, 1, true, nullptr);
						return;
					}
		SendMsgToChar("Вы не можете найти вход.\r\n", ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
