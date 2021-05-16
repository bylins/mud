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

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "logger.hpp"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "constants.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "skills.h"
#include "features.hpp"
#include "random.hpp"
#include "skills_info.h"

#include <vector>

// Externals
void do_say(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sense(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
extern const char *dirs[];
extern const char *DirsTo[];
extern int track_through_doors;
extern CHAR_DATA *mob_proto;

// local functions
void bfs_enqueue(room_rnum room, int dir);
void bfs_dequeue(void);
void bfs_clear_queue(void);
int find_first_step(room_rnum src, room_rnum target, CHAR_DATA * ch);

struct bfs_queue_struct
{
	room_rnum room;
	char dir;
};

#define EDGE_ZONE   1
#define EDGE_WORLD  2

// Utility macros
#define MARK(room)	(GET_ROOM(room)->set_flag(ROOM_BFS_MARK))
#define UNMARK(room)	(GET_ROOM(room)->unset_flag(ROOM_BFS_MARK))
#define IS_MARKED(room)	(ROOM_FLAGGED(room, ROOM_BFS_MARK))
#define TOROOM(x, y)	(world[(x)]->dir_option[(y)]->to_room())
#define IS_CLOSED(x, y)	(EXIT_FLAGGED(world[(x)]->dir_option[(y)], EX_CLOSED))

int VALID_EDGE(room_rnum x, int y, int edge_range, int through_doors)
{
	if (world[x]->dir_option[y] == NULL || TOROOM(x, y) == NOWHERE)
		return 0;

	// Попытка уползти в другую зону
	if (edge_range == EDGE_ZONE && (world[x]->zone != world[TOROOM(x, y)]->zone))
		return 0;

	if (through_doors == FALSE && IS_CLOSED(x, y))
		return 0;

	if (ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK) || IS_MARKED(TOROOM(x, y)))
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
int find_first_step(room_rnum src, room_rnum target, CHAR_DATA * ch)
{
	int curr_dir, edge, through_doors;
	room_rnum curr_room, rnum_start = FIRST_ROOM, rnum_stop = top_of_world;

	if (src < FIRST_ROOM || src > top_of_world || target < FIRST_ROOM || target > top_of_world)
	{
		log("SYSERR: Illegal value %d or %d passed to find_first_step. (%s)", src, target, __FILE__);
		return (BFS_ERROR);
	}

	if (src == target)
		return (BFS_ALREADY_THERE);

	// clear marks first, some OLC systems will save the mark.
	if (IS_NPC(ch))
	{
		// Запрещаем искать мобам  в другой зоне ...
//		if (world[src]->zone != world[target]->zone)
//			return (BFS_ERROR);

		get_zone_rooms(world[src]->zone, &rnum_start, &rnum_stop);
		// Запрещаем мобам искать через двери ...
		through_doors = FALSE;
//		edge = EDGE_ZONE;
	}
	else
	{
		// Игроки полноценно ищут в мире.
		through_doors = TRUE;
		edge = EDGE_WORLD;
	}

	for (curr_room = rnum_start; curr_room <= rnum_stop; curr_room++)
		UNMARK(curr_room);

	MARK(src);

	// переписано на вектор без реального очищения, чтобы не заниматься сотнями аллокаций памяти в секунду зря -- Krodo
	static std::vector<bfs_queue_struct> bfs_queue;
	static struct bfs_queue_struct temp_queue;

	// first, enqueue the first steps, saving which direction we're going.
	for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
	{
		if (VALID_EDGE(src, curr_dir, edge, through_doors))
		{
			MARK(TOROOM(src, curr_dir));
			temp_queue.room = TOROOM(src, curr_dir);
			temp_queue.dir = curr_dir;
			bfs_queue.push_back(temp_queue);
		}
	}

	// now, do the classic BFS.
	for (unsigned int i = 0; i < bfs_queue.size(); ++i)
	{
		if (bfs_queue[i].room == target)
		{
			curr_dir = bfs_queue[i].dir;
			bfs_queue.clear();
			return curr_dir;
		}
		else
		{
			for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
			{
				if (VALID_EDGE(bfs_queue[i].room, curr_dir, edge, through_doors))
				{
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
	mudlog(buf, NRM, -1, ERRLOG, TRUE);
	return (BFS_NO_PATH);
}


int go_sense(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int percent, dir, skill = CalcCurrentSkill(ch, SKILL_SENSE, victim);

	skill = skill - MAX(1, (GET_REMORT(victim) - GET_REMORT(ch)) * 5); // разница в ремортах *5 вычитается из текущего умения
	skill = skill - MAX(1, (GET_LEVEL(victim) - GET_LEVEL(ch)) * 5);
	skill = MAX(0, skill);
	percent = number(0, skill_info[SKILL_SENSE].difficulty);
	if (percent > skill)
	{
		int tries = 10;
		do
		{
			dir = number(0, NUM_OF_DIRS - 1);
		}
		while (!CAN_GO(ch, dir) && --tries);
		return dir;
	}
    ch->send_to_TC(false, true, false,
		"НЮХ: skill == %d percent ==%d реморт цели %d\r\n", skill, percent, GET_REMORT(victim));
    return find_first_step(ch->in_room, victim->in_room, ch);
}

void do_sense(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict;
	int dir;

	// The character must have the track skill.
	if (IS_NPC(ch) || !ch->get_skill(SKILL_SENSE))
	{
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("Вы слепы как крот.\r\n", ch);
		return;
	}

	if (!check_moves(ch, can_use_feat(ch, TRACKER_FEAT) ? SENSE_MOVES / 2 : SENSE_MOVES))
		return;

	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("Кого вы хотите найти?\r\n", ch);
		return;
	}
	// The person can't see the victim.
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
	{
		send_to_char("Ваши чувства молчат.\r\n", ch);
		return;
	}

	// We can't track the victim.
	//Старый комментарий. Раньше было много !трека, теперь его мало
	if (AFF_FLAGGED(vict, EAffectFlag::AFF_NOTRACK))
	{
		send_to_char("Ваши чувства молчат.\r\n", ch);
		return;
	}
	act("Похоже, $n кого-то ищет.", FALSE, ch, 0, 0, TO_ROOM);

	dir = go_sense(ch, vict);

	switch (dir)
	{
	case BFS_ERROR:
		strcpy(buf, "Хммм... Ваше чувство подвело вас.");
		break;
	case BFS_ALREADY_THERE:
		strcpy(buf, "Вы же в одной комнате с $N4!");
		break;
	case BFS_NO_PATH:
		strcpy(buf, "Ваши чувства молчат.");
		break;
	default:		// Success!
      ImproveSkill(ch, SKILL_SENSE, TRUE, vict);
		sprintf(buf, "Чувство подсказало вам : \"Ступай %s.\"\r\n", DirsTo[dir]);
		break;
	}
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
