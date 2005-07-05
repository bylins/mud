/* ************************************************************************
*   File: genchar.h                                     Part of Bylins    *
*  Usage: header file for character generation                            *
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

#define SUM_ALL_STATS 93
#define SUM_ALL_STATS_NOGEN 95
#define SUM_STATS(ch) (GET_STR(ch) + GET_DEX(ch) + GET_INT(ch) +  GET_WIS(ch) + GET_CON(ch) +  GET_CHA(ch))

#define GENCHAR_CONTINUE 1
#define GENCHAR_EXIT 0

extern char *genchar_help;

void genchar_disp_menu(CHAR_DATA * ch);
int genchar_parse(CHAR_DATA * ch, char *arg);
void roll_real_abils(CHAR_DATA * ch, bool hand = 0);
