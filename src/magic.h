/* ************************************************************************
*   File: magic.h                                     Part of Bylins      *
*  Usage: header: low-level functions for magic; spell template code      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */
#ifndef _MAGIC_H_
#define _MAGIC_H_

/* These mobiles do not exist. */
#define MOB_SKELETON      100
#define MOB_ZOMBIE        101
#define MOB_BONEDOG       102
#define MOB_BONEDRAGON    103
#define LAST_NECR_MOB	  103
#define MOB_KEEPER        104
#define MOB_FIREKEEPER    105

#define MAX_SPELL_AFFECTS 5	/* change if more needed */

#define SpINFO spell_info[spellnum]

#endif
