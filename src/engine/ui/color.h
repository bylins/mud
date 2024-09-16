/* ************************************************************************
*   File: screen.h                                      Part of Bylins    *
*  Usage: header file with ANSI color codes for online color              *
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

// комментарий на русском в надежде починить кодировки bitbucket

#ifndef COLOR_H_
#define COLOR_H_

#define KNRM  "\x1B[0;37m"
#define KRED  "\x1B[0;31m"
#define KGRN  "\x1B[0;32m"
#define KYEL  "\x1B[0;33m"
#define KBLU  "\x1B[0;34m"
#define KMAG  "\x1B[0;35m"
#define KCYN  "\x1B[0;36m"
#define KWHT  "\x1B[1;37m"
#define BWHT  "\x1B[0;37m"

#define KIDRK  "\x1B[1;30m"
#define KIRED  "\x1B[1;31m"
#define KIGRN  "\x1B[1;32m"
#define KIYEL  "\x1B[1;33m"
#define KIBLU  "\x1B[1;34m"
#define KIMAG  "\x1B[1;35m"
#define KICYN  "\x1B[1;36m"
#define KIWHT  "\x1B[1;37m"

int proc_color(char *inbuf);
const char *GetWarmValueColor(int current, int max);
const char *GetColdValueColor(int current, int max);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
