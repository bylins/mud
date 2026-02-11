// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef OBJSAVE_HPP_INCLUDED
#define OBJSAVE_HPP_INCLUDED

#include "obj_save.h"
#include "engine/entities/obj_data.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/conf.h"

// these factors should be unique integers
const int RENT_FACTOR = 1;
const int CRYO_FACTOR = 4;

struct SaveRentInfo {
	SaveRentInfo() : time(0), rentcode(0), net_cost_per_diem(0), gold(0),
					 account(0), nitems(0), oitems(0), spare1(0), spare2(0), spare3(0),
					 spare4(0), spare5(0), spare6(0), spare7(0) {};

	int32_t time;
	int32_t rentcode;
	int32_t net_cost_per_diem;
	int32_t gold;
	int32_t account;
	int32_t nitems;
	int32_t oitems;
	int32_t spare1;
	int32_t spare2;
	int32_t spare3;
	int32_t spare4;
	int32_t spare5;
	int32_t spare6;
	int32_t spare7;
};

struct SaveTimeInfo {
	int32_t vnum;
	int32_t timer;
};

struct SaveInfo {
	struct SaveRentInfo rent;
	std::vector<SaveTimeInfo> time;
};

ObjData::shared_ptr read_one_object_new(char **data, int *error);
void write_one_object(std::stringstream &out, ObjData *object, int location);
int Crash_offer_rent(CharData *ch, CharData *receptionist, int display, int factor, int *totalcost);
void Crash_rentsave(CharData *ch, int cost);
void Crash_crashsave(CharData *ch);
int Crash_write_timer(std::size_t index);
void Crash_rent_time(int dectime);
void Crash_save_all();
void Crash_frac_save_all(int frac_part);
void Crash_frac_rent_time(int frac_part);
void ClearCrashSavedObjects(std::size_t index);

namespace ObjSaveSync {

enum { CHAR_SAVE, CLAN_SAVE, PERS_CHEST_SAVE, PARCEL_SAVE };

void add(long init_uid, long targ_uid, int targ_type);
void check(long uid, int type);

} // namespace ObjSaveSync

#endif // OBJSAVE_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
