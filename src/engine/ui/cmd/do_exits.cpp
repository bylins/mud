/**
\file exits.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 11.09.2024.
\brief description.
*/

#include "engine/entities/char_data.h"
#include <fmt/format.h>

#include <string>
#include "administration/privilege.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/illumination.h"

void do_blind_exits(CharData *ch);

void DoExits(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	int door;

	if (ch->IsFlagged(EPrf::kBlindMode)) {
		do_blind_exits(ch);
		return;
	}
	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы слепы, как котенок!\r\n", ch);
		return;
	}

	std::string out;
	for (door = 0; door < EDirection::kMaxDirNum; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room() != kNowhere && !EXIT_FLAGGED(EXIT(ch, door), EExitFlag::kClosed)) {
			const RoomRnum rnum_exit_room = EXIT(ch, door)->to_room();
			// имя комнаты бывает нулевым (конструктор RoomData), а fmt на нуле бросает исключение
			const char *room_name = world[rnum_exit_room]->name ? world[rnum_exit_room]->name : "";
			std::string line;
			if (privilege::IsGod(ch)) {
				line = fmt::format("{:<6} - [{:5}] {}\r\n", dirs_rus[door],
								   GET_ROOM_VNUM(rnum_exit_room), room_name);
			} else {
				line = fmt::format("{:<6} - ", dirs_rus[door]);
				if (is_dark(rnum_exit_room) && !sight::CanSeeInDark(ch)) {
					line += "слишком темно\r\n";
				} else {
					if (ch->IsFlagged(EPrf::kMapper) && !ch->IsFlagged(EPlrFlag::kScriptWriter)
						&& !ROOM_FLAGGED(rnum_exit_room, ERoomFlag::kMoMapper)) {
						line += fmt::format("[{:7}] {}", GET_ROOM_VNUM(rnum_exit_room), room_name);
					} else {
						line += room_name;
					}
					line += "\r\n";
				}
			}
			// CAP(std::string) возвращает копию, а не правит на месте
			out += utils::CAP(line);
		}
	SendMsgToChar("Видимые выходы:\r\n", ch);
	if (!out.empty())
		SendMsgToChar(out, ch);
	else
		SendMsgToChar(" Замуровали, ДЕМОНЫ!\r\n", ch);
}

void do_blind_exits(CharData *ch) {
	int door;

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		SendMsgToChar("Вы слепы, как котенок!\r\n", ch);
		return;
	}

	std::string out;
	for (door = 0; door < EDirection::kMaxDirNum; door++)
		if (EXIT(ch, door) && EXIT(ch, door)->to_room() != kNowhere && !EXIT_FLAGGED(EXIT(ch, door), EExitFlag::kClosed)) {
			const RoomRnum rnum_exit_room = EXIT(ch, door)->to_room();
			const char *room_name = world[rnum_exit_room]->name ? world[rnum_exit_room]->name : "";
			std::string line;
			if (privilege::IsGod(ch)) {
				line = fmt::format("&W{} - [{}] {} ", dirs_rus[door],
								   GET_ROOM_VNUM(rnum_exit_room), room_name);
			} else {
				line = fmt::format("&W{} - ", dirs_rus[door]);
				if (is_dark(rnum_exit_room) && !sight::CanSeeInDark(ch)) {
					line += "слишком темно";
				} else {
					if (ch->IsFlagged(EPrf::kMapper) && !ch->IsFlagged(EPlrFlag::kScriptWriter)
						&& !ROOM_FLAGGED(rnum_exit_room, ERoomFlag::kMoMapper)) {
						line += fmt::format("[{}] {}", GET_ROOM_VNUM(rnum_exit_room), room_name);
					} else {
						line += room_name;
					}
				}
			}
			out += utils::CAP(line);
		}
	SendMsgToChar("Видимые выходы:\r\n", ch);
	if (!out.empty())
		SendMsgToChar(out + "&n\r\n", ch);
	else
		SendMsgToChar("&W Замуровали, ДЕМОНЫ!&n\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
