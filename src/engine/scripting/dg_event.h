/* ************************************************************************
*  File: dg_event.h                                        Part of Bylins *
*                                                                         *
*  Usage: header: a simplified event system                               *
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

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef _DG_EVENT_H_
#define _DG_EVENT_H_

// * how often will heartbeat() call our event function?
#define PULSE_DG_EVENT 1

class CharData;

// * macro used to prototype the callback function for an event
#define EVENT(function) void (function)(void *info)

// * define event related structures
struct TriggerEvent {
	int time_remaining;
	EVENT(*func);
	void *info;
	bool deleted;
TriggerEvent () {
	time_remaining = 0;
	deleted = false;
}
};

// * prototype event functions
TriggerEvent add_event(int time, EVENT(*func), void *info);
void remove_event(TriggerEvent event);
void process_events(void);

// список всех ивентов в очереди
void print_event_list(CharData *ch);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
