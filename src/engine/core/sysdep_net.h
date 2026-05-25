/* ************************************************************************
*   File: sysdep_net.h                                  Part of Bylins MUD *
*  Usage: socket-related system headers and prototypes                     *
*                                                                          *
*  Вынесено из sysdep.h (где было под #if defined(__COMM_C__)). Включается *
*  файлами, которым реально нужна сетевая часть (comm.cpp, утилиты).       *
*  Остальные TU не платят цену <sys/socket.h>, <netinet/in.h>, <signal.h>  *
*  и пр.                                                                   *
************************************************************************ */

#ifndef _SYSDEP_NET_H_
#define _SYSDEP_NET_H_

#include "sysdep.h"

// Header files only used in comm.cpp and some of the utils

#ifndef HAVE_STRUCT_IN_ADDR
struct in_addr
{
	unsigned long int s_addr;	// for inet_addr, etc.
};
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#elif defined(HAVE_SYS_FCNTL_H)
#include <sys/fcntl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_SIGNAL_H
# ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 2
#  include <signal.h>
#  undef _POSIX_C_SOURCE
# else
#  include <signal.h>		// GNU libc 6 already defines _POSIX_C_SOURCE.
# endif
#endif
#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif

// Function prototypes that are only used in comm.cpp and some of the utils

#ifndef NO_LIBRARY_PROTOTYPES

#ifdef NEED_ACCEPT_PROTO
int accept(socket_t s, struct sockaddr *addr, int *addrlen);
#endif

#ifdef NEED_BIND_PROTO
int bind(socket_t s, const struct sockaddr *name, int namelen);
#endif

#ifdef NEED_CHDIR_PROTO
int chdir(const char *path);
#endif

#ifdef NEED_CLOSE_PROTO
int close(int fildes);
#endif

#ifdef NEED_FCNTL_PROTO
int fcntl(int fildes, int cmd, /* arg */ ...);
#endif

#ifdef NEED_FPUTC_PROTO
int fputc(char c, FILE * stream);
#endif

#ifdef NEED_FPUTS_PROTO
int fputs(const char *s, FILE * stream);
#endif

#ifdef NEED_GETPEERNAME_PROTO
int getpeername(socket_t s, struct sockaddr *name, int *namelen);
#endif

#if defined(HAS_RLIMIT) && defined(NEED_GETRLIMIT_PROTO)
int getrlimit(int resource, struct rlimit *rlp);
#endif

#ifdef NEED_GETSOCKNAME_PROTO
int getsockname(socket_t s, struct sockaddr *name, int *namelen);
#endif

#ifdef NEED_HTONL_PROTO
ulong htonl(u_long hostlong);
#endif

#ifdef NEED_HTONS_PROTO
u_short htons(u_short hostshort);
#endif

#if defined(HAVE_INET_ADDR) && defined(NEED_INET_ADDR_PROTO)
unsigned long int inet_addr(const char *cp);
#endif

#if defined(HAVE_INET_ATON) && defined(NEED_INET_ATON_PROTO)
int inet_aton(const char *cp, struct in_addr *inp);
#endif

#ifdef NEED_INET_NTOA_PROTO
char *inet_ntoa(const struct in_addr in);
#endif

#ifdef NEED_LISTEN_PROTO
int listen(socket_t s, int backlog);
#endif

#ifdef NEED_NTOHL_PROTO
u_long ntohl(u_long netlong);
#endif

#ifdef NEED_PRINTF_PROTO
int printf(char *format, ...);
#endif

#ifdef NEED_READ_PROTO
ssize_t read(int fildes, void *buf, size_t nbyte);
#endif

#ifdef NEED_SELECT_PROTO
int select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval *timeout);
#endif

#ifdef NEED_SETITIMER_PROTO
int setitimer(int which, const struct itimerval *value, struct itimerval *ovalue);
#endif

#if defined(HAS_RLIMIT) && defined(NEED_SETRLIMIT_PROTO)
int setrlimit(int resource, const struct rlimit *rlp);
#endif

#ifdef NEED_SETSOCKOPT_PROTO
int setsockopt(socket_t s, int level, int optname, const char *optval, int optlen);
#endif

#ifdef NEED_SOCKET_PROTO
int socket(int domain, int type, int protocol);
#endif

#ifdef NEED_WRITE_PROTO
ssize_t write(int fildes, const void *buf, size_t nbyte);
#endif

#endif  // NO_LIBRARY_PROTOTYPES

#endif  // _SYSDEP_NET_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
