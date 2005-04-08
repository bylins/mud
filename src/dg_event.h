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
/*
** how often will heartbeat() call our event function?
*/
#define PULSE_DG_EVENT 1


/*
** macro used to prototype the callback function for an event
*/
#define EVENT(function) void (function)(void *info)


/*
** define event related structures
*/
struct event_info {
	int time_remaining;
	 EVENT(*func);
	void *info;
	struct event_info *next;
};


/*
** prototype event functions
*/
struct event_info *add_event(int time, EVENT(*func), void *info);
void remove_event(struct event_info *event);
void process_events(void);
