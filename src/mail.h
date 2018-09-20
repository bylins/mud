/* ************************************************************************
*   File: mail.h                                        Part of Bylins    *
*  Usage: header file for mail system                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
************************************************************************ */

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef _MAIL_H_
#define _MAIL_H_

#include <cstring>

class CHAR_DATA;	// forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

namespace mail
{

bool has_mail(int uid);
void add(int to_uid, int from_uid, const char* message);
void add_by_id(int to_uid, int from_uid, char* message);
void receive(CHAR_DATA* ch, CHAR_DATA* mailman);
void save();
void load();
size_t get_msg_count();
void add_notice(int uid);

} // namespace mail

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
