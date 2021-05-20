/**************************************************************************
*  File: trigedit.cpp                                      Part of Bylins   *
*  Usage: this header file is used in extending Oasis style OLC for       *
*  dg-scripts onto a CircleMUD that already has dg-scripts (as released   *
*  by Mark Heilpern on 1/1/98) implemented.                               *
*                                                                         *
* Parts of this file by Chris Jacobson of _Aliens vs Predator: The MUD_   *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef _DG_OLC_H_
#define _DG_OLC_H_

#include "dg_scripts.h"

// prototype exported functions from trigedit.cpp
//void script_copy(void *dst, void *src, int type);
void script_save_to_disk(FILE * fp, const void *item, int type);
//void dg_olc_script_free(DESCRIPTOR_DATA *d);
void dg_olc_script_copy(DESCRIPTOR_DATA * d);
void dg_script_menu(DESCRIPTOR_DATA * d);
int dg_script_edit_parse(DESCRIPTOR_DATA * d, char *arg);
void indent_trigger(std::string& cmd, int* level);

// define the largest set of commands for as trigger
#define MAX_CMD_LENGTH 32768	// 16k should be plenty and then some

#define NUM_TRIG_TYPE_FLAGS		26

// * Submodes of DG_OLC connectedness.
#define TRIGEDIT_MAIN_MENU              0
#define TRIGEDIT_TRIGTYPE               1
#define TRIGEDIT_CONFIRM_SAVESTRING	2
#define TRIGEDIT_NAME			3
#define TRIGEDIT_INTENDED		4
#define TRIGEDIT_TYPES			5
#define TRIGEDIT_COMMANDS		6
#define TRIGEDIT_NARG			7
#define TRIGEDIT_ARGUMENT		8

#define OLC_SCRIPT_EDIT		    82766
#define SCRIPT_MAIN_MENU		0
#define SCRIPT_NEW_TRIGGER		1
#define SCRIPT_DEL_TRIGGER		2

#define OLC_SCRIPT_EDIT_MODE(d)	((d)->olc->script_mode)	// parse input mode
#define OLC_SCRIPT(d)           ((d)->olc->script)	// script editing
#define OLC_ITEM_TYPE(d)	((d)->olc->item_type)	// mob/obj/room

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
