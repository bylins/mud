// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef OBJSAVE_HPP_INCLUDED
#define OBJSAVE_HPP_INCLUDED

#include "objsave.h"
#include "obj.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

// these factors should be unique integers
#define RENT_FACTOR 	1
#define CRYO_FACTOR 	4

OBJ_DATA::shared_ptr read_one_object_new(char **data, int *error);
void write_one_object(std::stringstream &out, OBJ_DATA * object, int location);
int Crash_offer_rent(CHAR_DATA * ch, CHAR_DATA * receptionist, int display, int factor, int *totalcost);
void Crash_rentsave(CHAR_DATA * ch, int cost);
void Crash_crashsave(CHAR_DATA * ch);
int Crash_write_timer(const std::size_t index);
void Crash_rent_time(int dectime);
void Crash_save_all(void);
void Crash_frac_save_all(int frac_part);
void Crash_frac_rent_time(int frac_part);

namespace ObjSaveSync
{

enum { CHAR_SAVE, CLAN_SAVE, PERS_CHEST_SAVE, PARCEL_SAVE };

void add(int init_uid, int targ_uid, int targ_type);
void check(int uid, int type);

} // namespace ObjSaveSync

#endif // OBJSAVE_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
