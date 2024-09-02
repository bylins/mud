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
std::list<TriggerEvent> event_list;

// * Add an event to the current list
TriggerEvent add_event(int time, EVENT(*func), void *info) {
	TriggerEvent this_data;

	this_data.time_remaining = time;
	this_data.func = func;
	this_data.info = info;
	this_data.deleted = false;
	auto it = std::find_if(event_list.begin(), event_list.end(), [time](auto i) {return time >= i.time_remaining;});

	if (it != event_list.end()) {
		event_list.insert(it, this_data);
	} else {
		event_list.push_back(this_data);
	}
	return this_data;
}

void remove_event(TriggerEvent event) {
	std::erase_if(event_list, [event](auto it) {return it.info == event.info;});
}

void process_events(void) {
	int trig_vnum;
	int timewarning = 50;

	auto now = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	auto start = now_ms.time_since_epoch();

	for (auto &e : event_list) {
		const auto time_remaining = e.time_remaining;
		if (time_remaining == 0) {
			trig_vnum = GET_TRIG_VNUM(((struct wait_event_data *) (e.info))->trigger);
				sprintf(buf, "[TrigVNum: %d]: process_events удаляю", trig_vnum);
				mudlog(buf, BRF, -1, ERRLOG, true);
			e.func(e.info);
//			e.time_remaining = 0;
			e.deleted = true;;
			// На отработку отложенных тригов выделяем всего 50 мсекунд
			// По исчерпанию лимита откладываем отработку на следующий тик.
			// Делаем для более равномерного распределения времени процессора.
			now = std::chrono::system_clock::now();
			now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
			auto end = now_ms.time_since_epoch();
			long timediff = end.count() - start.count();
			if (timediff > timewarning) {
				// Выводим номер триггера который переполнил время работы.
				sprintf(buf, "[TrigVNum: %d]: process_events overflow %ld ms.  warning  > %d ms", trig_vnum, timediff, timewarning);
				mudlog(buf, BRF, -1, ERRLOG, true);
				break;
			}
		} else {
			e.time_remaining--;
		}
	}
	std::erase_if(event_list, [](auto flag) {return flag.deleted;});
}

void print_event_list(CharData *ch)
{
	sprintf(buf, "В данный момент выполняются следующие триггеры:\r\n");
	SendMsgToChar(buf, ch);

	short trig_counter = 1;
	for (auto &e : event_list) {
		const wait_event_data *wed = static_cast<wait_event_data*>(e.info);
		if (!wed || !wed->trigger) {
			continue;
		}
		sprintf(buf, "[%-3d] Trigger: %s, VNum: [%5d]\r\n", trig_counter, GET_TRIG_NAME(wed->trigger), GET_TRIG_VNUM(wed->trigger));
//		if (wed->trigger->wait_line != nullptr) {
		if (wed->trigger->wait_event.time_remaining > 0 && wed->trigger->wait_line != nullptr) {
			sprintf(buf+strlen(buf), "    Wait: %d, Current line: %s (num line: %d)\r\n", GET_TRIG_WAIT(wed->trigger).time_remaining, 
					wed->trigger->wait_line->cmd.c_str(), wed->trigger->wait_line->line_num);
		}
		SendMsgToChar(buf, ch);
		++trig_counter;
	}
	sprintf(buf, "Итого триггеров %d.\r\n", trig_counter - 1);
	SendMsgToChar(buf, ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
