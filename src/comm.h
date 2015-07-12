/* ************************************************************************
*   File: comm.h                                        Part of Bylins    *
*  Usage: header file: prototypes of public communication functions       *
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

#ifndef _COMM_H_
#define _COMM_H_

#include <string>

#define NUM_RESERVED_DESCS	8

// comm.cpp
void send_to_all(const char *messg);
void send_to_char(const char *messg, CHAR_DATA * ch);
void send_to_char(CHAR_DATA * ch, const char *messg, ...);
void send_to_char(const std::string & buffer, CHAR_DATA * ch);
void send_stat_char(CHAR_DATA * ch);
void send_to_room(const char *messg, room_rnum room, int to_awake);
void send_to_outdoor(const char *messg, int control);
void send_to_gods(const char *messg);
void perform_to_all(const char *messg, CHAR_DATA * ch);
#ifdef HAS_EPOLL
void close_socket(DESCRIPTOR_DATA * d, int direct, int epoll, struct epoll_event *events, int n_ev);
#else
void close_socket(DESCRIPTOR_DATA * d, int direct);
#endif
void perform_act(const char *orig, CHAR_DATA * ch, const OBJ_DATA * obj, const void *vict_obj, CHAR_DATA * to, const int arena = 0);
void act(const char *str, int hide_invisible, CHAR_DATA * ch, const OBJ_DATA * obj, const void *vict_obj, int type);
unsigned long get_ip(const char *addr);

#define SUN_CONTROL     (1 << 0)
#define WEATHER_CONTROL (1 << 1)

#define TO_ROOM		1
#define TO_VICT		2
#define TO_NOTVICT	3
#define TO_CHAR		4
#define TO_ROOM_HIDE    5	// В комнату, но только тем, кто чувствует жизнь
#define CHECK_NODEAF        32   // посылать только глухим
#define CHECK_DEAF          64   // не посылать глухим
#define TO_SLEEP            128  // to char, even if sleeping
#define TO_ARENA_LISTEN     512  // не отсылать сообщение с арены слушателям, чтоб не спамить передвижениями и тп
#define TO_BRIEF_SHIELDS    1024 // отсылать только тем, у кого включен режим PRF_BRIEF_SHIELDS
#define TO_NO_BRIEF_SHIELDS 2048 // отсылать только тем, у кого нет режима PRF_BRIEF_SHIELDS

// I/O functions
int write_to_descriptor(socket_t desc, const char *txt, size_t total);
void write_to_q(const char *txt, struct txt_q *queue, int aliased);
void write_to_output(const char *txt, DESCRIPTOR_DATA * d);
void string_add(DESCRIPTOR_DATA * d, char *str);
void string_write(DESCRIPTOR_DATA * d, char **txt, size_t len, int mailto, void *data);

int toggle_compression(DESCRIPTOR_DATA * d);

#define SEND_TO_Q(messg, desc)		write_to_output((messg), desc)
#define SEND_TO_SOCKET(messg, desc)	write_to_descriptor((desc), (messg), strlen(messg))

#define USING_SMALL(d)	((d)->output == (d)->small_outbuf)
#define USING_LARGE(d)  ((d)->output == (d)->large_outbuf)

typedef RETSIGTYPE sigfunc(int);

// файлы логов и их количество
#define		NLOG	3

struct log_info_tag
{
	FILE *logfile;
	const char *filename;
	const char *name;
};
typedef struct log_info_tag log_info;

extern const int SYSLOG;
extern const int ERRLOG;
extern const int IMLOG;

extern log_info logs[NLOG];

extern unsigned long cmd_cnt;

#define DEFAULT_REBOOT_UPTIME 60*24*6	//время до ближайшего ребута по умолчанию в минутах
#define UPTIME_THRESHOLD      120	//минимальный аптайм для ребута в часах

void timediff(struct timeval *diff, struct timeval *a, struct timeval *b);

#endif
