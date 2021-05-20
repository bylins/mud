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

#ifndef _SCREEN_H_
#define _SCREEN_H_

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


#define KNUL  ""

// conditional color.  pass it a pointer to a char_data and a color level.
#define C_OFF	0
#define C_SPR	1
#define C_NRM	2
#define C_CMP	3
#define _clrlevel(ch) (!IS_NPC(ch) ? (PRF_FLAGGED((ch), PRF_COLOR_1) ? 1 : 0) + \
       		                         (PRF_FLAGGED((ch), PRF_COLOR_2) ? 2 : 0) : 0)
#define clr(ch,lvl) (_clrlevel(ch) >= (lvl))
#define CCNRM(ch,lvl)  (clr((ch),(lvl))?KNRM:KNUL)
#define CCRED(ch,lvl)  (clr((ch),(lvl))?KRED:KNUL)
#define CCGRN(ch,lvl)  (clr((ch),(lvl))?KGRN:KNUL)
#define CCYEL(ch,lvl)  (clr((ch),(lvl))?KYEL:KNUL)
#define CCBLU(ch,lvl)  (clr((ch),(lvl))?KBLU:KNUL)
#define CCMAG(ch,lvl)  (clr((ch),(lvl))?KMAG:KNUL)
#define CCCYN(ch,lvl)  (clr((ch),(lvl))?KCYN:KNUL)
#define CCWHT(ch,lvl)  (clr((ch),(lvl))?KWHT:KNUL)
#define CCINRM(ch,lvl)  (clr((ch),(lvl))?KIDRK:KNUL)
#define CCIRED(ch,lvl)  (clr((ch),(lvl))?KIRED:KNUL)
#define CCIGRN(ch,lvl)  (clr((ch),(lvl))?KIGRN:KNUL)
#define CCIYEL(ch,lvl)  (clr((ch),(lvl))?KIYEL:KNUL)
#define CCIBLU(ch,lvl)  (clr((ch),(lvl))?KIBLU:KNUL)
#define CCIMAG(ch,lvl)  (clr((ch),(lvl))?KIMAG:KNUL)
#define CCICYN(ch,lvl)  (clr((ch),(lvl))?KICYN:KNUL)
#define CCIWHT(ch,lvl)  (clr((ch),(lvl))?KIDRK:KNUL)

#define COLOR_LEV(ch) (_clrlevel(ch))

#define QNRM CCNRM(ch,C_SPR)
#define QRED CCRED(ch,C_SPR)
#define QGRN CCGRN(ch,C_SPR)
#define QYEL CCYEL(ch,C_SPR)
#define QBLU CCBLU(ch,C_SPR)
#define QMAG CCMAG(ch,C_SPR)
#define QCYN CCCYN(ch,C_SPR)
#define QWHT CCWHT(ch,C_SPR)

#define CCMANA(ch,C_SPR,perc) (perc >=90 ? CCCYN(ch,C_SPR)  :\
			       perc >= 5 ? CCICYN(ch,C_SPR) :\
					   CCIBLU(ch,C_SPR))

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
