/**************************************************************************
*  File: diskio.h    		              	           Part of Bylins *
*  Usage: Fast file buffering  				                  *
*	                                                                  *
*  (C) Copyright 1998 by Brian Boyle                                      *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#ifndef _DISKIO_H_
#define _DISKIO_H_

#define FB_READ        (1 << 0)    // read from disk       //
#define FB_WRITE    (1 << 1)    // write to disk        //
#define FB_APPEND    (1 << 2)    // write with append    //

#define FB_STARTSIZE    4192    // 4k starting buffer for writes //

/*#ifndef IS_SET
#define IS_SET(flag, bits)	((flag) & (bits))
#endif
*/

#include <stdio.h>
#include <string>
#include "engine/core/sysdep.h"

namespace DiskIo {

bool read_line(FILE *fl, std::string &line, bool cut_cr_lf);

}
typedef struct {
	char *buf;        // start of buffer                      //
	char *ptr;        // current location pointer             //
	int size;        // size in bytes of buffer              //
	int flags;        // read/write/append, future expansion  //
	char *name;        // filename (for delayed writing)       //
} FBFILE;

void ExtractTagFromArgument(char *argument, char *tag);
int fbgetline(FBFILE *fbfl, char *line);
FBFILE *fbopen(char *fname, int mode);
size_t fbclose(FBFILE *fbfl);
int fbprintf(FBFILE *fbfl, const char *format, ...) __attribute__((format(gnu_printf, 2, 3)));
void fbrewind(FBFILE *fbfl);
int fbcat(char *fromfilename, FBFILE *tofile);
char *fbgetstring(FBFILE *fl);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
