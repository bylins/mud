/* ************************************************************************
*  File: dg_event.cpp                                      Part of Bylins *
*                                                                         *
*  Usage: This file contains a simplified event system to allow wait      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                         *
*  $Date$                                           *
*  $Revision$                                                   *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "logger.hpp"
#include "dg_event.h"
#include "db.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"

// * define statics
static struct event_info *event_list = NULL;


// * Add an event to the current list
struct event_info *add_event(int time, EVENT(*func), void *info)
{
	struct event_info *this_data, *prev, *curr;

	CREATE(this_data, 1);
	this_data->time_remaining = time;
	this_data->func = func;
	this_data->info = info;

	// sort the event into the list in next-to-fire order
	if (event_list == NULL)
		event_list = this_data;
	else if (this_data->time_remaining <= event_list->time_remaining)
	{
		this_data->next = event_list;
		event_list = this_data;
	}
	else
	{
		prev = event_list;
		curr = prev->next;

		while (curr && (curr->time_remaining > this_data->time_remaining))
		{
			prev = curr;
			curr = curr->next;
		}

		this_data->next = curr;
		prev->next = this_data;
	}

	return this_data;
}

void remove_event(struct event_info *event)
{
	struct event_info *curr;

	if (event_list == event)
	{
		event_list = event->next;
	}
	else
	{
		curr = event_list;
		while (curr && (curr->next != event))
			curr = curr->next;
		if (!curr)
			return;	// failed to find it
		curr->next = curr->next->next;
	}
	free(event);
}

void process_events(void)
{
	struct event_info *e = event_list;
	struct event_info *del;
	struct timeval start, stop, result;
	int trig_vnum;

	gettimeofday(&start, NULL);

	while (e)
	{
		if (--(e->time_remaining) == 0)
		{
			trig_vnum = GET_TRIG_VNUM(((struct wait_event_data *)(e->info))->trigger);
			e->func(e->info);

			del = e;
			e = e->next;

			remove_event(del);
			// На отработку отложенных тригов выделяем всего 50 мсекунд
			// По исчерпанию лимита откладываем отработку на следующий тик.
			// Делаем для более равномерного распределения времени процессора.
			gettimeofday(&stop, NULL);
			timediff(&result, &stop, &start);

			if (result.tv_sec > 0 || result.tv_usec >= MAX_TRIG_USEC)
			{
				// Выводим номер триггера который переполнил время работы.
				sprintf(buf,
						"[TrigVNum: %d]: process_events overflow %ld sec. %ld us.",
						trig_vnum, result.tv_sec, result.tv_usec);
				mudlog(buf, BRF, -1, ERRLOG, TRUE);

				break;
			}
		}
		else
			e = e->next;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
