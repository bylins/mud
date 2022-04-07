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

#include "dg_event.h"
#include "db.h"
#include "dg_scripts.h"
#include "utils/utils.h"
#include "entities/char_data.h"
#include "comm.h"

// * define statics
static struct TriggerEvent *event_list = nullptr;

// * Add an event to the current list
struct TriggerEvent *add_event(int time, EVENT(*func), void *info) {
	struct TriggerEvent *this_data, *prev, *curr;

	CREATE(this_data, 1);
	this_data->time_remaining = time;
	this_data->func = func;
	this_data->info = info;

	// sort the event into the list in next-to-fire order
	if (event_list == nullptr)
		event_list = this_data;
	else if (this_data->time_remaining <= event_list->time_remaining) {
		this_data->next = event_list;
		event_list = this_data;
	} else {
		prev = event_list;
		curr = prev->next;

		while (curr && (curr->time_remaining > this_data->time_remaining)) {
			prev = curr;
			curr = curr->next;
		}

		this_data->next = curr;
		prev->next = this_data;
	}

	return this_data;
}

void remove_event(struct TriggerEvent *event) {
	struct TriggerEvent *curr;

	if (event_list == event) {
		event_list = event->next;
	} else {
		curr = event_list;
		while (curr && (curr->next != event))
			curr = curr->next;
		if (!curr)
			return;    // failed to find it
		curr->next = curr->next->next;
	}
	free(event);
}

void process_events(void) {
	struct TriggerEvent *e = event_list;
	struct TriggerEvent *del;
	int trig_vnum;
	int timewarning = 50;

	auto now = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	auto start = now_ms.time_since_epoch();

	while (e) {
		if (--(e->time_remaining) == 0) {
			trig_vnum = GET_TRIG_VNUM(((struct wait_event_data *) (e->info))->trigger);
			e->func(e->info);

			del = e;
			e = e->next;

			remove_event(del);
			// На отработку отложенных тригов выделяем всего 50 мсекунд
			// По исчерпанию лимита откладываем отработку на следующий тик.
			// Делаем для более равномерного распределения времени процессора.
			now = std::chrono::system_clock::now();
			now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
			auto end = now_ms.time_since_epoch();
			long timediff = end.count() - start.count();

			if (timediff > timewarning) {
				// Выводим номер триггера который переполнил время работы.
				sprintf(buf, "[TrigVNum: %d]: process_events overflow %ld ms.  warning  > %d ms",
						trig_vnum, timediff, timewarning);
				mudlog(buf, BRF, -1, ERRLOG, true);
				break;
			}
		} else
			e = e->next;
	}
}

void print_event_list(CharData *ch)
{
	sprintf(buf, "В данный момент выполняются следующие триггеры:\r\n");
	SendMsgToChar(buf, ch);

	short trig_counter = 1;
	TriggerEvent *e = event_list;
	while (e) {
		const wait_event_data *wed = static_cast<wait_event_data*>(e->info);
		if (!wed || !wed->trigger) {
			e = e->next;
			continue;
		}
		sprintf(buf, "[%-3d] Trigger: %s, VNum: [%5d]\r\n", trig_counter, GET_TRIG_NAME(wed->trigger), GET_TRIG_VNUM(wed->trigger));
		if (GET_TRIG_WAIT(wed->trigger) && wed->trigger->curr_state != nullptr) {
			sprintf(buf+strlen(buf), "    Wait: %d, Current line: %s\r\n", GET_TRIG_WAIT(wed->trigger)->time_remaining, wed->trigger->curr_state->cmd.c_str());
		}
		SendMsgToChar(buf, ch);

		++trig_counter;
		e = e->next;
	}

	sprintf(buf, "Итого триггеров %d.\r\n", trig_counter - 1);
	SendMsgToChar(buf, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
