/**
\file exits.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include "gameplay/mechanics/illumination.h"

void do_blind_exits(CharData *ch);

void DoExits(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int door;

	*buf = '\0';
	*buf2 = '\0';

	if (ch->IsFlagged(EPrf::kBlindMode)) {
		do_blind_exits(ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы слепы, как котенок!\r\n", ch);
		return;
	}
	for (door = 0; door < EDirection::kMaxDirNum; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room() != kNowhere && !EXIT_FLAGGED(EXIT(ch, door), EExitFlag::kClosed)) {
			if (IS_GOD(ch))
				sprintf(buf2, "%-6s - [%5d] %s\r\n", dirs_rus[door],
						GET_ROOM_VNUM(EXIT(ch, door)->to_room()), world[EXIT(ch, door)->to_room()]->name);
			else {
				sprintf(buf2, "%-6s - ", dirs_rus[door]);
				if (is_dark(EXIT(ch, door)->to_room()) && !CAN_SEE_IN_DARK(ch))
					strcat(buf2, "слишком темно\r\n");
				else {
					const RoomRnum rnum_exit_room = EXIT(ch, door)->to_room();
					if (ch->IsFlagged(EPrf::kMapper) && !ch->IsFlagged(EPlrFlag::kScriptWriter)
						&& !ROOM_FLAGGED(rnum_exit_room, ERoomFlag::kMoMapper)) {
						sprintf(buf2 + strlen(buf2), "[%5d] %s", GET_ROOM_VNUM(rnum_exit_room), world[rnum_exit_room]->name);
					} else {
						strcat(buf2, world[rnum_exit_room]->name);
					}
					strcat(buf2, "\r\n");
				}
			}
			strcat(buf, CAP(buf2));
		}
	SendMsgToChar("Видимые выходы:\r\n", ch);
	if (*buf)
		SendMsgToChar(buf, ch);
	else
		SendMsgToChar(" Замуровали, ДЕМОНЫ!\r\n", ch);
}

void do_blind_exits(CharData *ch) {
	int door;

	*buf = '\0';
	*buf2 = '\0';

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы слепы, как котенок!\r\n", ch);
		return;
	}
	for (door = 0; door < EDirection::kMaxDirNum; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room() != kNowhere && !EXIT_FLAGGED(EXIT(ch, door), EExitFlag::kClosed)) {
			if (IS_GOD(ch))
				sprintf(buf2, "&W%s - [%d] %s ", dirs_rus[door],
						GET_ROOM_VNUM(EXIT(ch, door)->to_room()), world[EXIT(ch, door)->to_room()]->name);
			else {
				sprintf(buf2, "&W%s - ", dirs_rus[door]);
				if (is_dark(EXIT(ch, door)->to_room()) && !CAN_SEE_IN_DARK(ch))
					strcat(buf2, "слишком темно");
				else {
					const RoomRnum rnum_exit_room = EXIT(ch, door)->to_room();
					if (ch->IsFlagged(EPrf::kMapper) && !ch->IsFlagged(EPlrFlag::kScriptWriter)
						&& !ROOM_FLAGGED(rnum_exit_room, ERoomFlag::kMoMapper)) {
						sprintf(buf2 + strlen(buf2), "[%d] %s", GET_ROOM_VNUM(rnum_exit_room), world[rnum_exit_room]->name);
					} else {
						strcat(buf2, world[rnum_exit_room]->name);
					}
					strcat(buf2, "");
				}
			}
			strcat(buf, CAP(buf2));
		}
	SendMsgToChar("Видимые выходы:\r\n", ch);
	if (*buf)
		SendMsgToChar(ch, "%s&n\r\n", buf);
	else
		SendMsgToChar("&W Замуровали, ДЕМОНЫ!&n\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
