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

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef GENCHAR_H_
#define GENCHAR_H_

#include "entities/entities_constants.h"

const int kBaseStatsSum = 95;
const int kGencharContinue = 1;
const int kGencharExit = 0;

const int G_STR = 0;
const int G_DEX = 1;
const int G_INT = 2;
const int G_WIS = 3;
const int G_CON = 4;
const int G_CHA = 5;

#define MIN_STR(ch) min_stats[(int) GET_CLASS(ch)][G_STR]
#define MIN_DEX(ch) min_stats[(int) GET_CLASS(ch)][G_DEX]
#define MIN_INT(ch) min_stats[(int) GET_CLASS(ch)][G_INT]
#define MIN_WIS(ch) min_stats[(int) GET_CLASS(ch)][G_WIS]
#define MIN_CON(ch) min_stats[(int) GET_CLASS(ch)][G_CON]
#define MIN_CHA(ch) min_stats[(int) GET_CLASS(ch)][G_CHA]

#define MAX_STR(ch) max_stats[(int) GET_CLASS(ch)][G_STR]
#define MAX_DEX(ch) max_stats[(int) GET_CLASS(ch)][G_DEX]
#define MAX_INT(ch) max_stats[(int) GET_CLASS(ch)][G_INT]
#define MAX_WIS(ch) max_stats[(int) GET_CLASS(ch)][G_WIS]
#define MAX_CON(ch) max_stats[(int) GET_CLASS(ch)][G_CON]
#define MAX_CHA(ch) max_stats[(int) GET_CLASS(ch)][G_CHA]

extern const char *genchar_help;

class CharData;

int genchar_parse(CharData *ch, char *arg);
void genchar_disp_menu(CharData *ch);
void roll_real_abils(CharData *ch);
void GetCase(const char *name, ESex sex, int caseNum, char *result);

extern int max_stats[][6];
extern int min_stats[][6];

#endif // GENCHAR_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
