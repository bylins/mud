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
#include "char.hpp"
#include "room.hpp"

#include <vector>

// Externals
void do_say(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sense(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
extern const char *dirs[];
extern const char *DirsTo[];
extern int track_through_doors;
extern CHAR_DATA *mob_proto;
int attack_best(CHAR_DATA * ch, CHAR_DATA * vict);

// local functions
void bfs_enqueue(room_rnum room, int dir);
void bfs_dequeue(void);
void bfs_clear_queue(void);
int find_first_step(room_rnum src, room_rnum target, CHAR_DATA * ch);
void do_track(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

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
		if (world[src]->zone != world[target]->zone)
			return (BFS_ERROR);

		get_zone_rooms(world[src]->zone, &rnum_start, &rnum_stop);
		// Запрещаем мобам искать через двери ...
		through_doors = FALSE;
		edge = EDGE_ZONE;
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

// * Functions and Commands which use the above functions. *
int go_track(CHAR_DATA * ch, CHAR_DATA * victim, const ESkill skill_no)
{
	int percent, dir;
	int if_sense;

	if (AFF_FLAGGED(victim, EAffectFlag::AFF_NOTRACK) && (skill_no != SKILL_SENSE))
	{
		return BFS_ERROR;
	}
	// 101 is a complete failure, no matter what the proficiency.
	//Временная затычка. Перевести на резисты
	//Изменил макс скилл со 100 до 200, чтобы не ломать алгоритм, в данном значении вернем старое значение.
	if_sense = (skill_no == SKILL_SENSE) ? 100 : 0;
	percent = number(0, skill_info[skill_no].max_percent - if_sense);
	//current_skillpercent = GET_SKILL(ch, SKILL_SENSE);
	if ((!IS_NPC(victim)) && (!IS_GOD(ch)) && (!IS_NPC(ch))) //Если цель чар и ищет не бог
	{ 
		percent = MIN(99, number(0, GET_REMORT(victim)) + percent);
	}
	if (percent > calculate_skill(ch, skill_no, victim))
	{
		int tries = 10;
		// Find a random direction. :)
		do
		{
			dir = number(0, NUM_OF_DIRS - 1);
		}
		while (!CAN_GO(ch, dir) && --tries);
		return dir;
	}

	// They passed the skill check.
	return find_first_step(ch->in_room, victim->in_room, ch);
}

int go_sense(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int percent, dir, skill = calculate_skill(ch, SKILL_SENSE, victim);
	
	skill = skill - MAX(1, (GET_REMORT(victim) - GET_REMORT(ch)) * 5); // разница в ремортах *5 вычитается из текущего умения
	skill = skill - MAX(1, (GET_LEVEL(victim) - GET_LEVEL(ch)) * 5);
	skill = MAX(0, skill);
	percent = number(0, skill_info[SKILL_SENSE].max_percent);
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
    if (PRF_FLAGGED(ch, PRF_TESTER))
            send_to_char(ch, "НЮХ: skill == %d percent ==%d реморт цеди %d\r\n", skill, percent, GET_REMORT(victim));
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
		improove_skill(ch, SKILL_SENSE, TRUE, vict);
		sprintf(buf, "Чувство подсказало вам : \"Ступай %s.\"\r\n", DirsTo[dir]);
		break;
	}
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
}

const char *track_when[] = { "совсем свежие",
							 "свежие",
							 "менее полудневной давности",
							 "примерно полудневной давности",
							 "почти дневной давности",
							 "примерно дневной давности",
							 "совсем старые"
						   };

#define CALC_TRACK(ch,vict) (calculate_skill(ch,SKILL_TRACK, 0))

int age_track(CHAR_DATA* /*ch*/, int time, int calc_track)
{
	int when = 0;

	if (calc_track >= number(1, 50))
	{
		if (time & 0x03)	// 2 
			when = 0;
		else if (time & 0x1F)	// 5 
			when = 1;
		else if (time & 0x3FF)	// 10 
			when = 2;
		else if (time & 0x7FFF)	// 15 
			when = 3;
		else if (time & 0xFFFFF)	// 20 
			when = 4;
		else if (time & 0x3FFFFFF)	// 26 
			when = 5;
		else
			when = 6;
	}
	else
		when = number(0, 6);
	return (when);
}

void do_track(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	CHAR_DATA *vict = NULL;
	struct track_data *track;
	int found = FALSE, calc_track = 0, track_t, i;
	char name[MAX_INPUT_LENGTH];

	// The character must have the track skill. 
	if (IS_NPC(ch) || !ch->get_skill(SKILL_TRACK))
	{
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("Вы слепы как крот.\r\n", ch);
		return;
	}

	if (!check_moves(ch, can_use_feat(ch, TRACKER_FEAT) ? TRACK_MOVES / 2 : TRACK_MOVES))
		return;

	calc_track = CALC_TRACK(ch, NULL);
	act("Похоже, $n кого-то выслеживает.", FALSE, ch, 0, 0, TO_ROOM);
	one_argument(argument, arg);

	// No argument - show all 
	if (!*arg)
	{
		for (track = world[ch->in_room]->track; track; track = track->next)
		{
			*name = '\0';
			if (IS_SET(track->track_info, TRACK_NPC))
			{
				strcpy(name, GET_NAME(mob_proto + track->who));
			}
			else
			{
				for (std::size_t c = 0; c < player_table.size(); c++)
				{
					if (player_table[c].id() == track->who)
					{
						strcpy(name, player_table[c].name());
						break;
					}
				}
			}

			if (*name && calc_track > number(1, 40))
			{
				CAP(name);
				for (track_t = i = 0; i < NUM_OF_DIRS; i++)
				{
					track_t |= track->time_outgone[i];
					track_t |= track->time_income[i];
				}
				sprintf(buf, "%s : следы %s.\r\n", name,
						track_when[age_track(ch, track_t, calc_track)]);
				send_to_char(buf, ch);
				found = TRUE;
			}
		}
		if (!found)
			send_to_char("Вы не видите ничьих следов.\r\n", ch);
		return;
	}

	if ((vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
	{
		act("Вы же в одной комнате с $N4!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	// found victim 
	for (track = world[ch->in_room]->track; track; track = track->next)
	{
		*name = '\0';
		if (IS_SET(track->track_info, TRACK_NPC))
		{
			strcpy(name, GET_NAME(mob_proto + track->who));
		}
		else
		{
			for (std::size_t c = 0; c < player_table.size(); c++)
			{
				if (player_table[c].id() == track->who)
				{
					strcpy(name, player_table[c].name());
					break;
				}
			}
		}

		if (*name && isname(arg, name))
			break;
		else
			*name = '\0';
	}

	if (calc_track < number(1, 40) || !*name || ROOM_FLAGGED(ch->in_room, ROOM_NOTRACK))
	{
		send_to_char("Вы не видите похожих следов.\r\n", ch);
		return;
	}

	ch->track_dirs = 0;
	CAP(name);
	sprintf(buf, "%s:\r\n", name);

	for (int c = 0; c < NUM_OF_DIRS; c++)
	{
		if ((track && track->time_income[c]
				&& calc_track >= number(0, skill_info[SKILL_TRACK].max_percent))
				|| (!track && calc_track < number(0, skill_info[SKILL_TRACK].max_percent)))
		{
			found = TRUE;
			sprintf(buf + strlen(buf), "- %s следы ведут %s\r\n",
					track_when[age_track
							   (ch,
								track ? track->
								time_income[c] : (1 << number(0, 25)), calc_track)], DirsFrom[Reverse[c]]);
		}
		if ((track && track->time_outgone[c]
				&& calc_track >= number(0, skill_info[SKILL_TRACK].max_percent))
				|| (!track && calc_track < number(0, skill_info[SKILL_TRACK].max_percent)))
		{
			found = TRUE;
			SET_BIT(ch->track_dirs, 1 << c);
			sprintf(buf + strlen(buf), "- %s следы ведут %s\r\n",
					track_when[age_track
							   (ch,
								track ? track->
								time_outgone[c] : (1 << number(0, 25)), calc_track)], DirsTo[c]);
		}
	}

	if (!found)
	{
		sprintf(buf, "След неожиданно оборвался.\r\n");
	}
	send_to_char(buf, ch);
	return;
}

void do_hidetrack(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	struct track_data *track[NUM_OF_DIRS + 1], *temp;
	int percent, prob, i, croom, found = FALSE, dir, rdir;

	if (IS_NPC(ch) || !ch->get_skill(SKILL_HIDETRACK))
	{
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	croom = ch->in_room;

	for (dir = 0; dir < NUM_OF_DIRS; dir++)
	{
		track[dir] = NULL;
		rdir = Reverse[dir];
		if (EXITDATA(croom, dir) &&
				EXITDATA(EXITDATA(croom, dir)->to_room(), rdir) &&
				EXITDATA(EXITDATA(croom, dir)->to_room(), rdir)->to_room() == croom)
		{
			for (temp = world[EXITDATA(croom, dir)->to_room()]->track; temp; temp = temp->next)
				if (!IS_SET(temp->track_info, TRACK_NPC)
						&& GET_IDNUM(ch) == temp->who && !IS_SET(temp->track_info, TRACK_HIDE)
						&& IS_SET(temp->time_outgone[rdir], 3))
				{
					found = TRUE;
					track[dir] = temp;
					break;
				}
		}
	}

	track[NUM_OF_DIRS] = NULL;
	for (temp = world[ch->in_room]->track; temp; temp = temp->next)
		if (!IS_SET(temp->track_info, TRACK_NPC) &&
				GET_IDNUM(ch) == temp->who && !IS_SET(temp->track_info, TRACK_HIDE))
		{
			found = TRUE;
			track[NUM_OF_DIRS] = temp;
			break;
		}

	if (!found)
	{
		send_to_char("Вы не видите своих следов.\r\n", ch);
		return;
	}
	if (!check_moves(ch, can_use_feat(ch, STEALTHY_FEAT) ? HIDETRACK_MOVES / 2 : HIDETRACK_MOVES))
		return;
	percent = number(1, skill_info[SKILL_HIDETRACK].max_percent);
	prob = calculate_skill(ch, SKILL_HIDETRACK, 0);
	if (percent > prob)
	{
		send_to_char("Вы безуспешно попытались замести свои следы.\r\n", ch);
		if (!number(0, 25 - timed_by_skill(ch, SKILL_HIDETRACK) ? 0 : 15))
			improove_skill(ch, SKILL_HIDETRACK, FALSE, 0);
	}
	else
	{
		send_to_char("Вы успешно замели свои следы.\r\n", ch);
		if (!number(0, 25 - timed_by_skill(ch, SKILL_HIDETRACK) ? 0 : 15))
			improove_skill(ch, SKILL_HIDETRACK, TRUE, 0);
		prob -= percent;
		for (i = 0; i <= NUM_OF_DIRS; i++)
			if (track[i])
			{
				if (i < NUM_OF_DIRS)
					track[i]->time_outgone[Reverse[i]] <<= MIN(31, prob);
				else
					for (rdir = 0; rdir < NUM_OF_DIRS; rdir++)
					{
						track[i]->time_income[rdir] <<= MIN(31, prob);
						track[i]->time_outgone[rdir] <<= MIN(31, prob);
					}
				//sprintf(buf,"Заметены следы %d\r\n",i);
				//send_to_char(buf,ch);
			}
	}

	for (i = 0; i <= NUM_OF_DIRS; i++)
		if (track[i])
			SET_BIT(track[i]->track_info, TRACK_HIDE);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
