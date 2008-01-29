/* ************************************************************************
*   File: fight.cpp                                     Part of Bylins    *
*  Usage: headers: Combat system                                          *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */
#ifndef _FIGHT_H_
#define _FIGHT_H_

#define RIGHT_WEAPON	1
#define LEFT_WEAPON	2

void die(CHAR_DATA * ch, CHAR_DATA * killer);
int thaco(int ch_class, int level);
void apply_weapon_bonus(int ch_class, int skill, int *damroll, int *hitroll);
#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_MAGIC))

#endif
