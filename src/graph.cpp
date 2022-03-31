/* ************************************************************************
*   File: graph.cpp                                     Part of Bylins    *
*  Usage: various graph algorithms                                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "handler.h"
#include "utils/random.h"
#include "structs/global_objects.h"

// Externals
void do_say(CharData *ch, char *argument, int cmd, int subcmd);
void do_sense(CharData *ch, char *argument, int cmd, int subcmd);
extern const char *dirs[];
extern const char *DirsTo[];
extern int track_through_doors;
extern CharData *mob_proto;

// local functions
void bfs_enqueue(RoomRnum room, int dir);
void bfs_dequeue(void);
void bfs_clear_queue(void);
int find_first_step(RoomRnum src, RoomRnum target, CharData *ch);

struct bfs_queue_struct {
	RoomRnum room;
	char dir;
};

#define EDGE_ZONE   1
#define EDGE_WORLD  2

// Utility macros
#define MARK(room)    (world[room]->set_flag(ROOM_BFS_MARK))
#define UNMARK(room)    (world[room]->unset_flag(ROOM_BFS_MARK))
#define IS_MARKED(room)    (ROOM_FLAGGED(room, ROOM_BFS_MARK))
#define TOROOM(x, y)    (world[(x)]->dir_option[(y)]->to_room())
#define IS_CLOSED(x, y)    (EXIT_FLAGGED(world[(x)]->dir_option[(y)], EX_CLOSED))
#define IS_LOCKED(x, y)    (EXIT_FLAGGED(world[(x)]->dir_option[(y)], EX_LOCKED))

int VALID_EDGE(RoomRnum x, int y, int edge_range, bool through_locked_doors, bool through_closed_doors, bool through_notrack) {
	if (world[x]->dir_option[y] == nullptr || TOROOM(x, y) == kNowhere)
		return 0;

	// Попытка уползти в другую зону
	if (edge_range == EDGE_ZONE && (world[x]->zone_rn != world[TOROOM(x, y)]->zone_rn))
		return 0;

	if (!through_closed_doors && IS_CLOSED(x, y))
		return 0;

	if (!through_locked_doors && IS_LOCKED(x, y))
		return 0;

	const bool respect_notrack = through_notrack ? false : ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK);
	if (respect_notrack || IS_MARKED(TOROOM(x, y)))
		return 0;

	return 1;
}

/*
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */
int find_first_step(RoomRnum src, RoomRnum target, CharData *ch) {
	int curr_dir, edge;
	bool through_locked_doors = false;
	bool through_closed_doors = false;
	bool through_notrack = false;
	RoomRnum curr_room, rnum_start = FIRST_ROOM, rnum_stop = top_of_world;

	if (src < FIRST_ROOM || src > top_of_world || target < FIRST_ROOM || target > top_of_world) {
		log("SYSERR: Illegal value %d or %d passed to find_first_step. (%s)", src, target, __FILE__);
		return (kBfsError);
	}

	if (src == target)
		return (kBfsAlreadyThere);

	// clear marks first, some OLC systems will save the mark.
	if (ch->is_npc()) {
		// Запрещаем искать мобам  в другой зоне ...
//		if (world[src]->zone != world[target]->zone)
//			return (BFS_ERROR);

		// Запрещаем мобам искать через запертые двери
		through_locked_doors = false;
		// если моб умеет открыть двери - ищем через закрытые двери
		through_closed_doors = MOB_FLAGGED(ch, EMobFlag::kOpensDoor);
		// notrack мобам не помеха
		through_notrack = true;
		if (MOB_FLAGGED(ch, EMobFlag::kStayZone)) {
			get_zone_rooms(world[src]->zone_rn, &rnum_start, &rnum_stop);
			edge = EDGE_ZONE;
		} else {
			edge = EDGE_WORLD;
		}
	} else {
		// Игроки полноценно ищут в мире.
		through_locked_doors = true;
		through_closed_doors = true;
		// но не могут искать через notrack
		through_notrack = false;
		edge = EDGE_WORLD;
	}

	for (curr_room = rnum_start; curr_room <= rnum_stop; curr_room++)
		UNMARK(curr_room);

	MARK(src);

	// переписано на вектор без реального очищения, чтобы не заниматься сотнями аллокаций памяти в секунду зря -- Krodo
	static std::vector<bfs_queue_struct> bfs_queue;
	static struct bfs_queue_struct temp_queue;

	// first, enqueue the first steps, saving which direction we're going.
	for (curr_dir = 0; curr_dir < EDirection::kMaxDirNum; curr_dir++) {
		if (VALID_EDGE(src, curr_dir, edge, through_locked_doors, through_closed_doors, through_notrack)) {
			MARK(TOROOM(src, curr_dir));
			temp_queue.room = TOROOM(src, curr_dir);
			temp_queue.dir = curr_dir;
			bfs_queue.push_back(temp_queue);
		}
	}

	// now, do the classic BFS.
	for (unsigned int i = 0; i < bfs_queue.size(); ++i) {
		if (bfs_queue[i].room == target) {
			curr_dir = bfs_queue[i].dir;
			bfs_queue.clear();
			return curr_dir;
		} else {
			for (curr_dir = 0; curr_dir < EDirection::kMaxDirNum; curr_dir++) {
				if (VALID_EDGE(bfs_queue[i].room, curr_dir, edge, through_locked_doors, through_closed_doors, through_notrack)) {
					MARK(TOROOM(bfs_queue[i].room, curr_dir));
					temp_queue.room = TOROOM(bfs_queue[i].room, curr_dir);
					temp_queue.dir = bfs_queue[i].dir;
					bfs_queue.push_back(temp_queue);
				}
			}
		}
	}
	bfs_queue.clear();
	sprintf(buf, "Mob (mob: %s vnum: %d) can't find path.", GET_NAME(ch), GET_MOB_VNUM(ch));
	mudlog(buf, NRM, -1, ERRLOG, true);
	return (kBfsNoPath);
}

int go_sense(CharData *ch, CharData *victim) {
	int percent, dir, skill = CalcCurrentSkill(ch, ESkill::kSense, victim);

	skill = skill
		- MAX(1, (GET_REAL_REMORT(victim) - GET_REAL_REMORT(ch)) * 5); // разница в ремортах *5 вычитается из текущего умения
	skill = skill - MAX(1, (GetRealLevel(victim) - GetRealLevel(ch)) * 5);
	skill = MAX(0, skill);
	percent = number(0, MUD::Skills()[ESkill::kSense].difficulty);
	if (percent > skill) {
		int tries = 10;
		do {
			dir = number(0, EDirection::kMaxDirNum - 1);
		} while (!CAN_GO(ch, dir) && --tries);
		return dir;
	}
	ch->send_to_TC(false, true, false,
				   "НЮХ: skill == %d percent ==%d реморт цели %d\r\n", skill, percent, GET_REAL_REMORT(victim));
	return find_first_step(ch->in_room, victim->in_room, ch);
}

void do_sense(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *vict;
	int dir;

	// The character must have the track skill.
	if (ch->is_npc() || !ch->get_skill(ESkill::kSense)) {
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffect::kBlind)) {
		send_to_char("Вы слепы как крот.\r\n", ch);
		return;
	}

	if (!check_moves(ch, IsAbleToUseFeat(ch, EFeat::kTracker) ? SENSE_MOVES / 2 : SENSE_MOVES))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Кого вы хотите найти?\r\n", ch);
		return;
	}
	// The person can't see the victim.
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD))) {
		send_to_char("Ваши чувства молчат.\r\n", ch);
		return;
	}

	// We can't track the victim.
	//Старый комментарий. Раньше было много !трека, теперь его мало
	if (AFF_FLAGGED(vict, EAffect::kNoTrack)) {
		send_to_char("Ваши чувства молчат.\r\n", ch);
		return;
	}
	act("Похоже, $n кого-то ищет.", false, ch, 0, 0, kToRoom);

	dir = go_sense(ch, vict);

	switch (dir) {
		case kBfsError: strcpy(buf, "Хммм... Ваше чувство подвело вас.");
			break;
		case kBfsAlreadyThere: strcpy(buf, "Вы же в одной комнате с $N4!");
			break;
		case kBfsNoPath: strcpy(buf, "Ваши чувства молчат.");
			break;
		default:        // Success!
			ImproveSkill(ch, ESkill::kSense, true, vict);
			sprintf(buf, "Чувство подсказало вам : \"Ступай %s.\"\r\n", DirsTo[dir]);
			break;
	}
	act(buf, false, ch, 0, vict, kToChar);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
