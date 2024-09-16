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

#include "engine/entities/entities_constants.h"

const int kBaseStatsSum = 95;
const int kGencharContinue = 1;
const int kGencharExit = 0;

const int G_STR = 0;
const int G_DEX = 1;
const int G_INT = 2;
const int G_WIS = 3;
const int G_CON = 4;
const int G_CHA = 5;

extern const char *genchar_help;

class CharData;

int genchar_parse(CharData *ch, char *arg);
void genchar_disp_menu(CharData *ch);
void SetStartAbils(CharData *ch);
void GetCase(std::string name, EGender sex, int caseNum, char *data);

extern int max_stats[][6];
extern int min_stats[][6];

#endif // GENCHAR_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
