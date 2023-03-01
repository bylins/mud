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

#include "structs/structs.h"
#include "structs/descriptor_data.h"
#include <string>
#include <sstream>

#define NUM_RESERVED_DESCS    8

class CObjectPrototype;    // forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.
class CharData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

extern DescriptorData *descriptor_list;

// comm.cpp
void SendMsgToAll(const char *msg);
void SendMsgToChar(const char *msg, const CharData *ch);
void SendMsgToChar(const CharData *ch, const char *msg, ...) __attribute__((format(printf, 2, 3)));
void SendMsgToChar(const std::string &msg, const CharData *ch);
void send_stat_char(const CharData *ch);
void SendMsgToRoom(std::string_view msg, RoomRnum room, int to_awake);
void SendMsgToOutdoor(const char *msg, int control);
void SendMsgToGods(const char *msg);
void perform_to_all(const char *messg, CharData *ch);
#ifdef HAS_EPOLL
void close_socket(DescriptorData *d, int direct, int epoll, struct epoll_event *events, int n_ev);
#else
void close_socket(DescriptorData * d, int direct);
#endif

void perform_act(const char *orig,
				 CharData *ch,
				 const ObjData *obj,
				 const void *vict_obj,
				 CharData *to,
				 const int arena,
				 const std::string &kick_type);

inline void perform_act(const char *orig,
						CharData *ch,
						const ObjData *obj,
						const void *vict_obj,
						CharData *to,
						const std::string &kick_type) {
	perform_act(orig, ch, obj, vict_obj, to, 0, kick_type);
}
inline void perform_act(const char *orig,
						CharData *ch,
						const ObjData *obj,
						const void *vict_obj,
						CharData *to,
						const int arena) {
	perform_act(orig, ch, obj, vict_obj, to, arena, "");
}
inline void perform_act(const char *orig, CharData *ch, const ObjData *obj, const void *vict_obj, CharData *to) {
	perform_act(orig, ch, obj, vict_obj, to, 0, "");
}

void act(const char *str,
		 int hide_invisible,
		 CharData *ch,
		 const ObjData *obj,
		 const void *vict_obj,
		 int type,
		 const std::string &kick_type);

inline void act(const char *str,
				int hide_invisible,
				CharData *ch,
				const ObjData *obj,
				const void *vict_obj,
				int type) {
	act(str, hide_invisible, ch, obj, vict_obj, type, "");
}
inline void act(const std::stringstream &str,
				int hide_invisible,
				CharData *ch,
				const ObjData *obj,
				const void *vict_obj,
				int type) {
	act(str.str().c_str(), hide_invisible, ch, obj, vict_obj, type);
}

inline void act(const std::string &str,
				int hide_invisible,
				CharData *ch,
				const ObjData *obj,
				const void *vict_obj,
				int type) {
	act(str.c_str(), hide_invisible, ch, obj, vict_obj, type, "");
};

unsigned long get_ip(const char *addr);

#define SUN_CONTROL     (1 << 0)
#define WEATHER_CONTROL (1 << 1)

const int kToRoom = 1;
const int kToVict = 2;
const int kToNotVict = 3;
const int kToChar = 4;
const int kToRoomSensors = 5;		// В комнату, но только тем, кто чувствует жизнь
const int kToDeaf = 32;				// посылать только глухим
const int kToNotDeaf = 64;			// не посылать глухим
const int kToSleep = 128;			// to char, even if sleeping
const int kToArenaListen = 512;		// не отсылать сообщение с арены слушателям, чтоб не спамить передвижениями и тп
const int kToBriefShields = 1024;	// отсылать только тем, у кого включен режим EPrf::BRIEF_SHIELDS
const int kToNoBriefShields = 2048;	// отсылать только тем, у кого нет режима EPrf::BRIEF_SHIELDS

// I/O functions
int write_to_descriptor(socket_t desc, const char *txt, size_t total);
bool write_to_descriptor_with_options(DescriptorData *t, const char *buffer, size_t byffer_size, int &written);
void write_to_q(const char *txt, struct TextBlocksQueue *queue, int aliased);
void write_to_output(const char *txt, DescriptorData *d);
void string_add(DescriptorData *d, char *str);
void string_write(DescriptorData *d, const utils::AbstractStringWriter::shared_ptr &writer,
				  size_t len, int mailto, void *data);
int toggle_compression(DescriptorData *d);


typedef RETSIGTYPE sigfunc(int);

extern unsigned long cmd_cnt;

constexpr int DEFAULT_REBOOT_UPTIME  = 60*24*6;		//время до ближайшего ребута по умолчанию в минутах
const int UPTIME_THRESHOLD = 120;    				//минимальный аптайм для ребута в часах

void timediff(struct timeval *diff, struct timeval *a, struct timeval *b);

int main_function(int argc, char **argv);

void gifts();

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
