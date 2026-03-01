/* ************************************************************************
*   File: conf.h                                        Part of Bylins    *
*  Usage: header file: Generated automatically by configure               *
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

#ifndef _CONF_H_
#define _CONF_H_

#ifdef _MSC_VER
#pragma comment(lib, "wsock32.lib")

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif // _CRT_SECURE_NO_DEPRECATE

#define _SCL_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)
#pragma warning(disable:4244)	// Possible loss of data.
#endif // _MSC_VER

#ifdef _WIN32
#define CIRCLE_WINDOWS
#define NOCRYPT 1
#define TEST_BUILD 1
#define LOG_AUTOFLUSH 1

#ifdef __MINGW32__
#include <sys/time.h>
#else
#define __func__ ""
#endif

#ifdef __BORLANDC__
#include <dir.h>
#else				// MSVC
#include <direct.h>
#endif

#define mkdir(str, int) _mkdir(str)
#undef max
#define max max
#undef min
#define min min

#ifndef __MINGW32__
#define ssize_t int
#endif
#define socklen_t int

#if (_MSC_VER < 1900)
#define snprintf _snprintf
#endif
#if _MSC_VER
void gettimeofday(struct timeval *t, void *dummy);
#endif
#else // _WIN32
// Define if you have <sys/wait.h> that is POSIX.1 compatible.
#define HAVE_SYS_WAIT_H 1
// Define if you can safely include both <sys/time.h> and <time.h>.
#define TIME_WITH_SYS_TIME 1
// Define if we're compiling CircleMUD under any type of UNIX system.
#define CIRCLE_UNIX 1
// Define if you have the <arpa/inet.h> header file.
#define HAVE_ARPA_INET_H 1
// Define if you have the <arpa/telnet.h> header file.
#define HAVE_ARPA_TELNET_H 1
// Define if you have the <memory.h> header file.
#define HAVE_MEMORY_H 1
// Define if you have the <netdb.h> header file.
#define HAVE_NETDB_H 1
// Define if you have the <netinet/in.h> header file.
#define HAVE_NETINET_IN_H 1
// Define if you have the <signal.h> header file.
#define HAVE_SIGNAL_H 1
// Define if you have the <strings.h> header file.
#define HAVE_STRINGS_H 1
// Define if you have the <sys/fcntl.h> header file.
#define HAVE_SYS_FCNTL_H 1
// Define if you have the <sys/resource.h> header file.
#define HAVE_SYS_RESOURCE_H 1
// Define if you have the <sys/socket.h> header file.
#define HAVE_SYS_SOCKET_H 1
// Define if you have the <sys/stat.h> header file.
#define HAVE_SYS_STAT_H 1
// Define if you have the <sys/time.h> header file.
#define HAVE_SYS_TIME_H 1
// Define if you have the <sys/types.h> header file.
#define HAVE_SYS_TYPES_H 1
// Define if you have the <sys/uio.h> header file.
#define HAVE_SYS_UIO_H 1
// Define if you have the <unistd.h> header file.
#define HAVE_UNISTD_H 1
#endif // _WIN32

// Define as the return type of signal handlers (int or void).
#define RETSIGTYPE void
// Define if you have the ANSI C header files.
#define STDC_HEADERS 1
// Define is the system has struct in_addr.
#define HAVE_STRUCT_IN_ADDR 1
// Define if you have the inet_addr function.
#define HAVE_INET_ADDR 1
// Define if you have the inet_aton function.
//#define HAVE_INET_ATON 1
// Define if you have the <assert.h> header file.
#define HAVE_ASSERT_H 1
// Define if you have the <crypt.h> header file.
//#define HAVE_CRYPT_H 1
// Define if you have the <errno.h> header file.
#define HAVE_ERRNO_H 1
// Define if you have the <fcntl.h> header file.
#define HAVE_FCNTL_H 1
// Define if you have the <limits.h> header file.
#define HAVE_LIMITS_H 1
// Define if you have the <net/errno.h> header file.
// #undef HAVE_NET_ERRNO_H
// Define if you have the <string.h> header file.
#define HAVE_STRING_H 1
// Define if you have the <sys/select.h> header file.
// #undef HAVE_SYS_SELECT_H
// Define if you have the malloc library (-lmalloc).
// #undef HAVE_LIBMALLOC
// Define to `unsigned' if <sys/types.h> doesn't define.
// #undef size_t
// Define to `int' if <sys/types.h> doesn't define.
// #undef ssize_t
#define BOOST_NO_MT
#define BOOST_DISABLE_THREADS

#ifndef HAVE_ZLIB
struct z_stream;
#endif

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
