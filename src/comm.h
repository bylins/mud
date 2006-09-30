// -*- coding: koi8-r -*-

//  File: comm.h                                        Part of Bylins
//  Usage: header file: prototypes of public communication functions
//
//  All rights reserved.  See license.doc for complete information.
//
//  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
//  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
//
//  $Author$
//  $Date$
//  $Revision$


#ifndef _COMM_H_
#define _COMM_H_

#include <string>

const int NUM_RESERVED_DESCS = 8;

// comm.cpp

void send_to_char(const char *messg, CHAR_DATA * ch);
void send_to_char(CHAR_DATA * ch, const char *messg, ...);
void send_to_char(const std::string & buffer, CHAR_DATA * ch);
void send_to_all(const char *messg);
void send_to_room(const char *messg, room_rnum room, int to_awake);

void perform_act(const char *orig, CHAR_DATA * ch, const OBJ_DATA * obj, const void *vict_obj, CHAR_DATA * to);
void send_to_outdoor(const char *messg, int control);

unsigned long get_ip(const char *addr);


const int SUN_CONTROL = (0 << 1);
const int WEATHER_CONTROL = (1 << 1);

enum ActType {
	TO_ROOM = 1,
	TO_VICT,
	TO_NOTVICT,
	TO_CHAR,
	TO_ROOM_HIDE, // В комнату, но только тем, кто чувствует жизнь
	CHECK_NODEAF = 32, // посылать только глухим
	CHECK_DEAF = 64, // не посылать глухим
	TO_SLEEP = 128 // to char, even if sleeping
};

void act(const char *str, int hide_invisible, CHAR_DATA * ch, const OBJ_DATA * obj, const void *vict_obj, int type);

// I/O functions
void write_to_output(const char *txt, DESCRIPTOR_DATA * d);
void write_to_q(const char *txt, struct txt_q *queue, int aliased);

int toggle_compression(DESCRIPTOR_DATA * d);

// файлы логов и их количество
const int NLOG = 3;

struct log_info_tag {
	FILE *logfile;
	char *filename;
	char *name;
};
typedef struct log_info_tag log_info;

enum LogType {
	SYSLOG = 0,
	ERRLOG,
	IMLOG
};

extern log_info logs[NLOG];

extern unsigned long cmd_cnt;

//время до ближайшего ребута по умолчанию в минутах
const int DEFAULT_REBOOT_UPTIME = 60*24*3;

//минимальный аптайм для ребута в часах
const int UPTIME_THRESHOLD = 72;

#endif
