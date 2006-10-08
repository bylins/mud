/* ************************************************************************
*   File: comm.cpp                                      Part of Bylins    *
*  Usage: Communication, socket handling, main(), central game loop       *
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

/*
 * Compression support.  Currently could be used with:
 *
 *   MUD Client for Linux, by Erwin S. Andreasen
 *     http://www.andreasen.org/mcl/
 *
 *   mcclient, by Oliver 'Nemon@AR' Jowett
 *     http://homepages.ihug.co.nz/~icecube/compress/
 *
 * Contact them for help with the clients. Contact greerga@circlemud.org
 * for problems with the server end of the connection.  If you think you
 * have found a bug, please test another MUD for the same problem to see
 * if it is a client or server problem.
 */

#define __COMM_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

#ifdef CIRCLE_MACINTOSH		/* Includes for the Macintosh */
# define SIGPIPE 13
# define SIGALRM 14
  /* GUSI headers */
# include <sys/ioctl.h>
  /* Codewarrior dependant */
# include <SIOUX.h>
# include <console.h>
#endif

#ifdef CIRCLE_WINDOWS		/* Includes for Win32 */
# ifdef __BORLANDC__
#  include <dir.h>
# else				/* MSVC */
#  include <direct.h>
# endif
# include <mmsystem.h>
#endif				/* CIRCLE_WINDOWS */

#ifdef CIRCLE_AMIGA		/* Includes for the Amiga */
# include <sys/ioctl.h>
# include <clib/socket_protos.h>
#endif				/* CIRCLE_AMIGA */

#ifdef CIRCLE_ACORN		/* Includes for the Acorn (RiscOS) */
# include <socklib.h>
# include <inetlib.h>
# include <sys/ioctl.h>
#endif

/*
 * Note, most includes for all platforms are in sysdep.h.  The list of
 * files that is included is controlled by conf.h for that platform.
 */

#include "sys/types.h"
#include "sys/socket.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "house.h"
#include "olc.h"
#include "dg_scripts.h"
#include "screen.h"
#include "ban.hpp"
#include "auction.h"
#include "exchange.h"
#include "deathtrap.hpp"
#include "title.hpp"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/* externs */
extern int num_invalid;
extern char *GREETINGS;
extern const char *circlemud_version;
extern int circle_restrict;
extern int mini_mud;
extern FILE *player_fl;
extern ush_int DFLT_PORT;
extern const char *DFLT_DIR;
extern const char *DFLT_IP;
extern const char *LOGNAME;
extern int max_playing;
extern int nameserver_is_slow;	/* see config.cpp */
extern int auto_save;		/* see config.cpp */
extern int autosave_time;	/* see config.cpp */
extern int mana[];

extern char *help;
extern zone_rnum top_of_zone_table;
extern struct zone_data *zone_table;
extern const char *save_info_msg[];	/* In olc.cpp */
extern CHAR_DATA *character_list;
extern CHAR_DATA *combat_list;
extern void proc_color(char *inbuf, int color);
extern void tact_auction(void);
extern time_t boot_time;

/* external global objects and containers */
extern BanList *ban;

/* local globals */
DESCRIPTOR_DATA *descriptor_list = NULL;	/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
/*
circle_shutdown = 0 - do not reboot
circle_shutdown = 1 - shutdown/reboot normally with RENT_CRASH
circle_shutdown = 2 - reboot with normal rent
*/
int circle_reboot = 0;		/* reboot the game after a shutdown */
int shutdown_time = 0;		/* reboot at this time */
int no_specials = 0;		/* Suppress ass. of special routines */
int max_players = 0;		/* max descriptors available */
int tics = 0;			/* for extern checkpointing */
int scheck = 0;			/* for syntax checking mode */
long last_rent_check = 0;	/* at what time checked rented time */
struct timeval null_time;	/* zero-valued time structure */
int dg_act_check;		/* toggle for act_trigger */
unsigned long dg_global_pulse = 0;	/* number of pulses since game start */
unsigned long cmd_cnt = 0;
unsigned long int number_of_bytes_read = 0;
unsigned long int number_of_bytes_written = 0;

int reboot_uptime = DEFAULT_REBOOT_UPTIME;	// uptime until reboot in minutes

log_info logs[NLOG] = {
	{NULL, "syslog", "СИСТЕМНЫЙ"},
	{NULL, "log/errlog.txt", "ОШИБКИ МИРА"},
	{NULL, "log/imlog.txt", "ИНГРЕДИЕНТНАЯ МАГИЯ"}
};

char src_path[4096];

/* functions in this file */
RETSIGTYPE unrestrict_game(int sig);
RETSIGTYPE reap(int sig);
RETSIGTYPE checkpointing(int sig);
RETSIGTYPE hupsig(int sig);
ssize_t perform_socket_read(socket_t desc, char *read_point, size_t space_left);
ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length);
void echo_off(DESCRIPTOR_DATA * d);
void echo_on(DESCRIPTOR_DATA * d);
void sanity_check(void);
void circle_sleep(struct timeval *timeout);
int get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(ush_int port);
void signal_setup(void);
void game_loop(socket_t mother_desc);
socket_t init_socket(ush_int port);
int new_descriptor(socket_t s);

int get_max_players(void);
int process_output(DESCRIPTOR_DATA * t);
int process_input(DESCRIPTOR_DATA * t);
void timediff(struct timeval *diff, struct timeval *a, struct timeval *b);
void timeadd(struct timeval *sum, struct timeval *a, struct timeval *b);
void flush_queues(DESCRIPTOR_DATA * d);
void nonblock(socket_t s);
int perform_subst(DESCRIPTOR_DATA * t, char *orig, char *subst);
int perform_alias(DESCRIPTOR_DATA * d, char *orig);
void record_usage(void);
char *make_prompt(DESCRIPTOR_DATA * point);
void check_idle_passwords(void);
void heartbeat();
struct in_addr *get_bind_addr(void);
int parse_ip(const char *addr, struct in_addr *inaddr);
int set_sendbuf(socket_t s);
void setup_logs(void);
int open_logfile(log_info * li, FILE * stderr_fp);
void make_who2html();

#if defined(POSIX)
sigfunc *my_signal(int signo, sigfunc * func);
#endif
#if defined(HAVE_ZLIB)
void *zlib_alloc(void *opaque, unsigned int items, unsigned int size);
void zlib_free(void *opaque, void *address);
#endif


/* extern fcnts */
void SaveGlobalUID(void);
void boot_world(void);
void player_affect_update(void);	/* In spells.cpp */
void room_affect_update(void);		/* In spells.cpp */
void mobile_affect_update(void);
void mobile_activity(int activity_level, int missed_pulses);
void perform_violence(void);
void process_events(void);
void show_string(DESCRIPTOR_DATA * d, char *input);
void weather_and_time(int mode);
void redit_save_to_disk(int zone_num);
void oedit_save_to_disk(int zone_num);
void medit_save_to_disk(int zone_num);
void sedit_save_to_disk(int zone_num);
void zedit_save_to_disk(int zone_num);
void hour_update();
int real_zone(int number);
void koi_to_alt(char *str, int len);
void koi_to_win(char *str, int len);
void koi_to_winz(char *str, int len);
void Crash_rent_time(int dectime);
void Crash_ldsave(CHAR_DATA * ch);
void Crash_save_all_rent();
int level_exp(CHAR_DATA * ch, int level);
void flush_player_index(void);
void dupe_player_index(void);
void Crash_frac_save_all(int frac_part);
void Crash_frac_rent_time(int frac_part);
unsigned long TxtToIp(const char * text);
void underwater_check(void);
//F@N|
int exchange_database_save(void);
int exchange_database_savebackup(void);

#ifdef __CXREF__
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#undef FD_CLR
#define FD_ZERO(x)
#define FD_SET(x, y) 0
#define FD_ISSET(x, y) 0
#define FD_CLR(x, y)
#endif

#if defined(HAVE_ZLIB)
/*
 * MUD Client for Linux and mcclient compression support.
 * "The COMPRESS option (unofficial and completely arbitary) is
 * option 85." -- mcclient documentation as of Dec '98.
 *
 * [ Compression protocol documentation below, from Compress.cpp ]
 *
 * Server sends  IAC WILL COMPRESS
 * We reply with IAC DO COMPRESS
 *
 * Later the server sends IAC SB COMPRESS WILL SE, and immediately following
 * that, begins compressing
 *
 * Compression ends on a Z_STREAM_END, no other marker is used
 */

int mccp_start(DESCRIPTOR_DATA * t, int ver);
int mccp_end(DESCRIPTOR_DATA * t, int ver);

#define TELOPT_COMPRESS        85
#define TELOPT_COMPRESS2       86
const char compress_will[] = { IAC, WILL, TELOPT_COMPRESS2,
	IAC, WILL, TELOPT_COMPRESS, '\0'
};
const char compress_start_v1[] = { IAC, SB, TELOPT_COMPRESS, WILL, SE, '\0' };
const char compress_start_v2[] = { IAC, SB, TELOPT_COMPRESS2, IAC, SE, '\0' };

#endif

const char str_goahead[] = { IAC, GA, 0 };


/***********************************************************************
*  main game loop and related stuff                                    *
***********************************************************************/

#if defined(CIRCLE_WINDOWS) || defined(CIRCLE_MACINTOSH)

/*
 * Windows doesn't have gettimeofday, so we'll simulate it.
 * The Mac doesn't have gettimeofday either.
 * Borland C++ warns: "Undefined structure 'timezone'"
 */
void gettimeofday(struct timeval *t, struct timezone *dummy)
{
#if defined(CIRCLE_WINDOWS)
	DWORD millisec = GetTickCount();
#elif defined(CIRCLE_MACINTOSH)
	unsigned long int millisec;
	millisec = (int) ((float) TickCount() * 1000.0 / 60.0);
#endif

	t->tv_sec = (int) (millisec / 1000);
	t->tv_usec = (millisec % 1000) * 1000;
}

#endif				/* CIRCLE_WINDOWS || CIRCLE_MACINTOSH */


#define plant_magic(x)	do { (x)[sizeof(x) - 1] = MAGIC_NUMBER; } while (0)
#define test_magic(x)	((x)[sizeof(x) - 1])

int main(int argc, char **argv)
{
#ifdef OS_UNIX
	extern char *malloc_options;
	malloc_options = "A";
#endif

	ush_int port;
	int pos = 1;
	const char *dir;

	/* Initialize these to check for overruns later. */
	plant_magic(buf);
	plant_magic(buf1);
	plant_magic(buf2);
	plant_magic(arg);

#ifdef CIRCLE_MACINTOSH
	/*
	 * ccommand() calls the command line/io redirection dialog box from
	 * Codewarriors's SIOUX library
	 */
	argc = ccommand(&argv);
	/* Initialize the GUSI library calls.  */
	GUSIDefaultSetup();
#endif

	port = DFLT_PORT;
	dir = DFLT_DIR;

	while ((pos < argc) && (*(argv[pos]) == '-')) {
		switch (*(argv[pos] + 1)) {
		case 'o':
			if (*(argv[pos] + 2))
				LOGNAME = argv[pos] + 2;
			else if (++pos < argc)
				LOGNAME = argv[pos];
			else {
				puts("SYSERR: File name to log to expected after option -o.");
				exit(1);
			}
			break;
		case 'd':
			if (*(argv[pos] + 2))
				dir = argv[pos] + 2;
			else if (++pos < argc)
				dir = argv[pos];
			else {
				puts("SYSERR: Directory arg expected after option -d.");
				exit(1);
			}
			break;
		case 'm':
			mini_mud = 1;
			puts("Running in minimized mode & with no rent check.");
			break;
		case 'c':
			scheck = 1;
			puts("Syntax check mode enabled.");
			break;
		case 'r':
			circle_restrict = 1;
			puts("Restricting game -- no new players allowed.");
			break;
		case 's':
			no_specials = 1;
			puts("Suppressing assignment of special routines.");
			break;
		case 'h':
			/* From: Anil Mahajan <amahajan@proxicom.com> */
			printf
			    ("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n"
			     "  -c             Enable syntax check mode.\n"
			     "  -d <directory> Specify library directory (defaults to 'lib').\n"
			     "  -h             Print this command line argument help.\n"
			     "  -m             Start in mini-MUD mode.\n"
			     "  -o <file>      Write log to <file> instead of stderr.\n"
			     "  -r             Restrict MUD -- no new players allowed.\n"
			     "  -s             Suppress special procedure assignments.\n", argv[0]);
			exit(0);
		default:
			printf("SYSERR: Unknown option -%c in argument string.\n", *(argv[pos] + 1));
			break;
		}
		pos++;
	}

	if (pos < argc) {
		if (!isdigit(*argv[pos])) {
			printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
			exit(1);
		} else if ((port = atoi(argv[pos])) <= 1024) {
			printf("SYSERR: Illegal port number %d.\n", port);
			exit(1);
		}
	}

	/* All arguments have been parsed, try to open log file. */
	setup_logs();

	/*
	 * Moved here to distinguish command line options and to show up
	 * in the log if stderr is redirected to a file.
	 */
	log(circlemud_version);
	log(DG_SCRIPT_VERSION);
	if (chdir(dir) < 0) {
		perror("SYSERR: Fatal error changing to data directory");
		exit(1);
	}
	log("Using %s as data directory.", dir);

	if (scheck) {
		boot_world();
		log("Done.");
	} else {
		log("Running game on port %d.", port);

		// стль и буст юзаются уже немало где, а про их экспешены никто не думает
		// пока хотя бы стльные ловить и просто логировать факт того, что мы вышли
		// по эксепшену для удобства отладки и штатного сброса сислога в файл, т.к. в коре будет фиг
		try {
			init_game(port);
		}
		catch (std::exception &e) {
			log("STD exception: %s", e.what());
		}
		catch (...) {
			log("Unknown exception :(");
		}
	}

	return (0);
}



/* Init sockets, run game, and cleanup sockets */
void init_game(ush_int port)
{
	socket_t mother_desc;

	/* We don't want to restart if we crash before we get up. */
	touch(KILLSCRIPT_FILE);
	touch("../.crash");
	circle_srandom(time(0));

	log("Finding player limit.");
	max_players = get_max_players();

	log("Opening mother connection.");
	mother_desc = init_socket(port);

	boot_db();

#if defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)
	log("Signal trapping.");
	signal_setup();
#endif

	/* If we made it this far, we will be able to restart without problem. */
	remove(KILLSCRIPT_FILE);
	log("Entering game loop.");

	game_loop(mother_desc);

	if (circle_shutdown == 2) {
		log("Entering Crash_save_all_rent");
		Crash_save_all_rent();	//save all
	}
	//Crash_save_all();

	SaveGlobalUID();
	exchange_database_save();	//exchange database save
	Clan::ChestUpdate();
	Clan::ChestSave();
	Clan::ClanSave();
	TitleSystem::save_title_list();

	log("Closing all sockets.");
	while (descriptor_list)
		close_socket(descriptor_list, TRUE);

	CLOSE_SOCKET(mother_desc);
	if (circle_reboot != 2 && olc_save_list) {	/* Don't save zones. */
		struct olc_save_info *entry, *next_entry;
		int rznum;

		for (entry = olc_save_list; entry; entry = next_entry) {
			next_entry = entry->next;
			if (entry->type < 0 || entry->type > 4) {
				sprintf(buf, "OLC: Illegal save type %d!", entry->type);
				log(buf);
			} else if ((rznum = real_zone(entry->zone * 100)) == -1) {
				sprintf(buf, "OLC: Illegal save zone %d!", entry->zone);
				log(buf);
			} else if (rznum < 0 || rznum > top_of_zone_table) {
				sprintf(buf, "OLC: Invalid real zone number %d!", rznum);
				log(buf);
			} else {
				sprintf(buf, "OLC: Reboot saving %s for zone %d.",
					save_info_msg[(int) entry->type], zone_table[rznum].number);
				log(buf);
				switch (entry->type) {
				case OLC_SAVE_ROOM:
					redit_save_to_disk(rznum);
					break;
				case OLC_SAVE_OBJ:
					oedit_save_to_disk(rznum);
					break;
				case OLC_SAVE_MOB:
					medit_save_to_disk(rznum);
					break;
				case OLC_SAVE_ZONE:
					zedit_save_to_disk(rznum);
					break;
				case OLC_SAVE_SHOP:
					sedit_save_to_disk(rznum);
					break;
				default:
					log("Unexpected olc_save_list->type");
					break;
				}
			}
		}
	}
	free_db();
	if (circle_reboot) {
		log("Rebooting.");
		exit(52);	/* what's so great about HHGTTG, anyhow? */
	}
	log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
socket_t init_socket(ush_int port)
{
	socket_t s;
	int opt;
	struct sockaddr_in sa;

#ifdef CIRCLE_WINDOWS
	{
		WORD wVersionRequested;
		WSADATA wsaData;

		wVersionRequested = MAKEWORD(1, 1);

		if (WSAStartup(wVersionRequested, &wsaData) != 0) {
			log("SYSERR: WinSock not available!");
			exit(1);
		}
		if ((wsaData.iMaxSockets - 4) < max_players) {
			max_players = wsaData.iMaxSockets - 4;
		}
		log("Max players set to %d", max_players);

		if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
			log("SYSERR: Error opening network connection: Winsock error #%d", WSAGetLastError());
			exit(1);
		}
	}
#else
	/*
	 * Should the first argument to socket() be AF_INET or PF_INET?  I don't
	 * know, take your pick.  PF_INET seems to be more widely adopted, and
	 * Comer (_Internetworking with TCP/IP_) even makes a point to say that
	 * people erroneously use AF_INET with socket() when they should be using
	 * PF_INET.  However, the man pages of some systems indicate that AF_INET
	 * is correct; some such as ConvexOS even say that you can use either one.
	 * All implementations I've seen define AF_INET and PF_INET to be the same
	 * number anyway, so the point is (hopefully) moot.
	 */

	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("SYSERR: Error creating socket");
		exit(1);
	}
#endif				/* CIRCLE_WINDOWS */

#if defined(SO_REUSEADDR) && !defined(CIRCLE_MACINTOSH)
	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
		perror("SYSERR: setsockopt REUSEADDR");
		exit(1);
	}
#endif

	set_sendbuf(s);

/*
 * The GUSI sockets library is derived from BSD, so it defines
 * SO_LINGER, even though setsockopt() is unimplimented.
 *	(from Dean Takemori <dean@UHHEPH.PHYS.HAWAII.EDU>)
 */
#if defined(SO_LINGER) && !defined(CIRCLE_MACINTOSH)
	{
		struct linger ld;

		ld.l_onoff = 0;
		ld.l_linger = 0;
		if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0)
			perror("SYSERR: setsockopt SO_LINGER");	/* Not fatal I suppose. */
	}
#endif

	/* Clear the structure */
	memset((char *) &sa, 0, sizeof(sa));

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr = *(get_bind_addr());

	if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("SYSERR: bind");
		CLOSE_SOCKET(s);
		exit(1);
	}
	nonblock(s);
	listen(s, 5);
	return (s);
}


int get_max_players(void)
{
#ifndef CIRCLE_UNIX
	return (max_playing);
#else

	int max_descs = 0;
	const char *method;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.  HAS_RLIMIT is defined in sysdep.h.
 */
#ifdef HAS_RLIMIT
	{
		struct rlimit limit;

		/* find the limit of file descs */
		method = "rlimit";
		if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
			perror("SYSERR: calling getrlimit");
			exit(1);
		}

		/* set the current to the maximum */
		limit.rlim_cur = limit.rlim_max;
		if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
			perror("SYSERR: calling setrlimit");
			exit(1);
		}
#ifdef RLIM_INFINITY
		if (limit.rlim_max == RLIM_INFINITY)
			max_descs = max_playing + NUM_RESERVED_DESCS;
		else
			max_descs = MIN(max_playing + NUM_RESERVED_DESCS, limit.rlim_max);
#else
		max_descs = MIN(max_playing + NUM_RESERVED_DESCS, limit.rlim_max);
#endif
	}

#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
	method = "OPEN_MAX";
	max_descs = OPEN_MAX;	/* Uh oh.. rlimit didn't work, but we have
				 * OPEN_MAX */
#elif defined (_SC_OPEN_MAX)
	/*
	 * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
	 * try the POSIX sysconf() function.  (See Stevens' _Advanced Programming
	 * in the UNIX Environment_).
	 */
	method = "POSIX sysconf";
	errno = 0;
	if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
		if (errno == 0)
			max_descs = max_playing + NUM_RESERVED_DESCS;
		else {
			perror("SYSERR: Error calling sysconf");
			exit(1);
		}
	}
#else
	/* if everything has failed, we'll just take a guess */
	method = "random guess";
	max_descs = max_playing + NUM_RESERVED_DESCS;
#endif

	/* now calculate max _players_ based on max descs */
	max_descs = MIN(max_playing, max_descs - NUM_RESERVED_DESCS);

	if (max_descs <= 0) {
		log("SYSERR: Non-positive max player limit!  (Set at %d using %s).", max_descs, method);
		exit(1);
	}
	log("   Setting player limit to %d using %s.", max_descs, method);
	return (max_descs);
#endif				/* CIRCLE_UNIX */
}

int shutting_down(void)
{
	static int lastmessage = 0;
	int wait;

	if (!circle_shutdown)
		return (FALSE);
	if (!shutdown_time || time(NULL) >= shutdown_time)
		return (TRUE);
	if (lastmessage == shutdown_time || lastmessage == time(NULL))
		return (FALSE);
	wait = shutdown_time - time(NULL);

	if (wait == 10 || wait == 30 || wait == 60 || wait == 120 || wait % 300 == 0) {
		if (circle_reboot) {
			remove("../.crash");
			sprintf(buf, "ПЕРЕЗАГРУЗКА через ");
		} else {
			remove("../.crash");
			sprintf(buf, "ОСТАНОВКА через ");
		}
		if (wait < 60)
			sprintf(buf + strlen(buf), "%d %s.\r\n", wait, desc_count(wait, WHAT_SEC));
		else
			sprintf(buf + strlen(buf), "%d %s.\r\n", wait / 60, desc_count(wait / 60, WHAT_MINu));
		send_to_all(buf);
		lastmessage = time(NULL);
	}
	return (FALSE);
}

// log rotation
inline void rotate(log_info * li, long pos, struct tm *time)
{
	char newpath[256];
	char cwd[4096];
	int rc;

	if (pos != ftell(li->logfile)) {
		log("[ROTATE] Rotating %s", li->filename);
	} else {
		log("[ROTATE] No need to rotate %s, it's unchanged", li->filename);
		return;
	}

// o.k., let's rotate
	fclose(li->logfile);

	snprintf(newpath, 256, "%s.%d-%.2d-%.2d_%.2d-%.2d",
		 li->filename, time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min);

	getcwd(cwd, 4096);
	chdir(src_path);
	rc = rename(li->filename, newpath);

	if (rc != 0) {
		fprintf(stderr, "SYSERR: Log rotation has caused an error: '%s', exiting.\n", strerror(errno));
		exit(1);
	}

	if (!open_logfile(li, NULL)) {
		fprintf(stderr, "SYSERR: Couldn't open %s, giving up.\n", li->filename);
		exit(1);
	}
	chdir(cwd);
}

inline void process_io(fd_set input_set, fd_set output_set, fd_set exc_set, fd_set null_set,
			socket_t mother_desc, int maxdesc)
{
	DESCRIPTOR_DATA *d, *next_d;
	char comm[MAX_INPUT_LENGTH];
	int aliased;

	/* Poll (without blocking) for new input, output, and exceptions */
	if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time)
	    < 0) {
		perror("SYSERR: Select poll");
		return;
	}
	/* If there are new connections waiting, accept them. */
	if (FD_ISSET(mother_desc, &input_set))
		new_descriptor(mother_desc);

	/* Kick out the freaky folks in the exception set and marked for close */
	for (d = descriptor_list; d; d = next_d) {
		next_d = d->next;
		if (FD_ISSET(d->descriptor, &exc_set)) {
			FD_CLR(d->descriptor, &input_set);
			FD_CLR(d->descriptor, &output_set);
			close_socket(d, TRUE);
		}
	}

	/* Process descriptors with input pending */
	for (d = descriptor_list; d; d = next_d) {
		next_d = d->next;
		if (FD_ISSET(d->descriptor, &input_set))
			if (process_input(d) < 0)
				close_socket(d, FALSE);
	}

	/* Process commands we just read from process_input */
	for (d = descriptor_list; d; d = next_d) {
		next_d = d->next;

		/*
		 * Not combined to retain --(d->wait) behavior. -gg 2/20/98
		 * If no wait state, no subtraction.  If there is a wait
		 * state then 1 is subtracted. Therefore we don't go less
		 * than 0 ever and don't require an 'if' bracket. -gg 2/27/99
		 */
		if (d->character) {
			GET_WAIT_STATE(d->character) -= (GET_WAIT_STATE(d->character) > 0 ? 1 : 0);
			GET_PUNCTUAL_WAIT_STATE(d->character) -=
			    (GET_PUNCTUAL_WAIT_STATE(d->character) > 0 ? 1 : 0);
			SET_COMMSTATE(d->character, 0);
			if (WAITLESS(d->character)
			    || GET_WAIT_STATE(d->character) < 0)
				GET_WAIT_STATE(d->character) = 0;
			if (WAITLESS(d->character)
			    || GET_PUNCTUAL_WAIT_STATE(d->character) < 0)
				GET_PUNCTUAL_WAIT_STATE(d->character) = 0;
			if (GET_WAIT_STATE(d->character))
				continue;
		}
		// Шоб в меню долго не сидели !
		if (!get_from_q(&d->input, comm, &aliased)) {
			if (STATE(d) != CON_PLAYING &&
			    STATE(d) != CON_DISCONNECT &&
			    time(NULL) - d->input_time > 300 && d->character && !IS_GOD(d->character))
				close_socket(d, TRUE);
			continue;
		}

		d->input_time = time(NULL);
		if (d->character) {	/* Reset the idle timer & pull char back from void if necessary */
			d->character->char_specials.timer = 0;
			if (STATE(d) == CON_PLAYING && GET_WAS_IN(d->character) != NOWHERE) {
				if (d->character->in_room != NOWHERE)
					char_from_room(d->character);
				char_to_room(d->character, GET_WAS_IN(d->character));
				GET_WAS_IN(d->character) = NOWHERE;
				act("$n вернул$u.", TRUE, d->character, 0, 0, TO_ROOM);
				GET_WAIT_STATE(d->character) = 1;
			}
		}
		d->has_prompt = 0;
		if (d->showstr_count && STATE(d) != CON_DISCONNECT && STATE(d) != CON_CLOSE)	/* Reading something w/ pager */
			show_string(d, comm);
		else if (d->str && STATE(d) != CON_DISCONNECT && STATE(d) != CON_CLOSE)
			string_add(d, comm);
		else if (STATE(d) != CON_PLAYING)	/* In menus, etc. */
			nanny(d, comm);
		else {	/* else: we're playing normally. */
			if (aliased)	/* To prevent recursive aliases. */
				d->has_prompt = 1;	/* To get newline before next cmd output. */
			else if (perform_alias(d, comm))	/* Run it through aliasing system */
				get_from_q(&d->input, comm, &aliased);
			command_interpreter(d->character, comm);	/* Send it to interpreter */
			cmd_cnt++;
		}
	}

	for (d = descriptor_list; d; d = next_d) {
		next_d = d->next;
		if ((!d->has_prompt || *(d->output)) && FD_ISSET(d->descriptor, &output_set)) {
			if (process_output(d) < 0)
				close_socket(d, FALSE);	// закрыл соединение
			else
				d->has_prompt = 1;	// признак того, что промпт уже выводил
			// следующий после команды или очередной
			// порции вывода
		}
	}

#if 0
	/* Send queued output out to the operating system (ultimately to user). */
	for (d = descriptor_list; d; d = next_d) {
		next_d = d->next;
		if (*(d->output) && FD_ISSET(d->descriptor, &output_set)) {	/* Output for this player is ready */
			if (process_output(d) < 0)
				close_socket(d, FALSE);
			else
				d->has_prompt = 1;
		}
	}

	/* Print prompts for other descriptors who had no other output */
	for (d = descriptor_list; d; d = d->next) {
		if (!d->has_prompt) {	//sprintf(buf,"\r\n%s",make_prompt(d));
		//from_koi(buf, d->keytable);
			SEND_TO_Q(make_prompt(d), d);
			SEND_TO_Q(end_line, d);
			d->has_prompt = 2;
		}
	}
#endif

	/* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
	for (d = descriptor_list; d; d = next_d) {
		next_d = d->next;
		if (STATE(d) == CON_CLOSE || STATE(d) == CON_DISCONNECT)
			close_socket(d, FALSE);
	}

}

/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(socket_t mother_desc)
{
	fd_set input_set, output_set, exc_set, null_set;
	struct timeval last_time, opt_time, process_time, temp_time;
	struct timeval before_sleep, now, timeout;
	DESCRIPTOR_DATA *d;
	int missed_pulses = 0, maxdesc;

	/* initialize various time values */
	null_time.tv_sec = 0;
	null_time.tv_usec = 0;
	opt_time.tv_usec = OPT_USEC;
	opt_time.tv_sec = 0;
	FD_ZERO(&null_set);
	last_rent_check = time(NULL);

	gettimeofday(&last_time, (struct timezone *) 0);

	/* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
	while (!shutting_down()) {	/* Sleep if we don't have any connections */
		if (descriptor_list == NULL) {
			log("No connections.  Going to sleep.");
			make_who2html();
			FD_ZERO(&input_set);
			FD_SET(mother_desc, &input_set);
			if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
				if (errno == EINTR)
					log("Waking up to process signal.");
				else
					perror("SYSERR: Select coma");
			} else
				log("New connection.  Waking up.");
			gettimeofday(&last_time, (struct timezone *) 0);
		}
		/* Set up the input, output, and exception sets for select(). */
		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		FD_ZERO(&exc_set);
		FD_SET(mother_desc, &input_set);

		maxdesc = mother_desc;
		for (d = descriptor_list; d; d = d->next) {
#ifndef CIRCLE_WINDOWS
			if (d->descriptor > maxdesc)
				maxdesc = d->descriptor;
#endif
			FD_SET(d->descriptor, &input_set);
			FD_SET(d->descriptor, &output_set);
			FD_SET(d->descriptor, &exc_set);
		}

		/*
		 * At this point, we have completed all input, output and heartbeat
		 * activity from the previous iteration, so we have to put ourselves
		 * to sleep until the next 0.1 second tick.  The first step is to
		 * calculate how long we took processing the previous iteration.
		 */

		gettimeofday(&before_sleep, (struct timezone *) 0);	/* current time */
		timediff(&process_time, &before_sleep, &last_time);

		/*
		 * If we were asleep for more than one pass, count missed pulses and sleep
		 * until we're resynchronized with the next upcoming pulse.
		 */
		if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) {
			missed_pulses = 0;
		} else {
			missed_pulses = process_time.tv_sec * PASSES_PER_SEC;
			missed_pulses += process_time.tv_usec / OPT_USEC;
			process_time.tv_sec = 0;
			process_time.tv_usec = process_time.tv_usec % OPT_USEC;
		}

		/* Calculate the time we should wake up */
		timediff(&temp_time, &opt_time, &process_time);
		timeadd(&last_time, &before_sleep, &temp_time);

		/* Now keep sleeping until that time has come */
		gettimeofday(&now, (struct timezone *) 0);
		timediff(&timeout, &last_time, &now);

		/* Go to sleep */
		do {
			circle_sleep(&timeout);
			gettimeofday(&now, (struct timezone *) 0);
			timediff(&timeout, &last_time, &now);
		}
		while (timeout.tv_usec || timeout.tv_sec);

		/*
		 * Now, we execute as many pulses as necessary--just one if we haven't
		 * missed any pulses, or make up for lost time if we missed a few
		 * pulses by sleeping for too long.
		 */
		missed_pulses++;

		if (missed_pulses <= 0) {
			log("SYSERR: **BAD** MISSED_PULSES NONPOSITIVE (%d), TIME GOING BACKWARDS!!", missed_pulses);
			missed_pulses = 1;
		}

		/* If we missed more than 30 seconds worth of pulses, just do 30 secs */
		/* изменили на 4 сек */
		/* изменили на 1 сек -- слишком уж опасно лагает :) */
		if (missed_pulses > (1 * PASSES_PER_SEC)) {
			log("SYSERR: Missed %d seconds worth of pulses.", missed_pulses / PASSES_PER_SEC);
			missed_pulses = 1 * PASSES_PER_SEC;
		}

		/* Now execute the heartbeat functions */
		while (missed_pulses--) {
			process_io(input_set, output_set, exc_set, null_set, mother_desc, maxdesc);
			heartbeat();
		}

		// dupe_player_index();
		// _exit(0);

#ifdef CIRCLE_UNIX
		/* Update tics for deadlock protection (UNIX only) */
		tics++;
#endif
	}
}

void beat_points_update(int pulse);
#define FRAC_SAVE TRUE

inline void heartbeat()
{
//	CHAR_DATA *ch = NULL, *k, *next;
	static int mins_since_crashsave = 0, pulse = 0, lr_firstrun = 1;
	int uptime_minutes = 0;
	long check_at = 0, save_start = 0;
	static struct tm syslog_o;
	struct tm *syslog_n;
	static long syslog_pos = 0;

	pulse++;
	/* Roll pulse over after 10 hours */
	if (pulse >= (600 * 60 * PASSES_PER_SEC))
		pulse = 0;

	dg_global_pulse++;

	if (!(pulse % PASSES_PER_SEC))
		uptime_minutes = ((time(NULL) - boot_time) / 60);

	//log("---------- Start heartbeat ----------");
	//log("Process events...");
	process_events();
	//log("Stop it...");

	if (!((pulse + 1) % PULSE_DG_SCRIPT)) {	//log("Triggers check...");
		script_trigger_check();
		//log("Stop it...");
	}

	if (!((pulse + 2) % (30 * PASSES_PER_SEC))) {	//log("Sanity check...");
		sanity_check();
		//log("Stop it...");
	}

  /*** Remove after hour update
  if (!(pulse % PULSE_ZONE))
     {//log("Zone update...");
      zone_update();
     }
   ****************************/

	if (!(pulse % (40 * PASSES_PER_SEC))) {	/* 40 seconds *///log("Check idle password...");
		check_idle_passwords();
		//log("Stop it...");
	}
	/* Old proc
	   if (!(pulse % PULSE_MOBILE))
	   {// log("Mobile activity...");
		mobile_activity();
	   }
	 */
	//log("Mobile activity...");
//  mobile_activity(pulse % PULSE_MOBILE);

// экономим процессор. mobile_activity() дергать каждый пульс совсем не обязательно.
// второй аргумент -- период вызова в пульсах
// желательно, чтобы этот период был таким, чтобы различные задержки,
// выраженные в количестве пульсов, были ему кратны.
	if (!(pulse % 10))
	{
		mobile_activity(pulse, 10);
	}
	//log("Stop it...");

	if (!(pulse % (2 * PASSES_PER_SEC))) {
		DeathTrap::activity();
		underwater_check();
	}

	if (!((pulse + 3) % PULSE_VIOLENCE)) {	//log("Perform violence...");
		perform_violence();
		//log("Stop it...");
	}

	if (!(pulse % (30 * PASSES_PER_SEC))) {
		make_who2html();
		if (uptime_minutes >= (reboot_uptime - 30) && shutdown_time == 0) {
			//reboot after 30 minutes minimum. Auto reboot cannot run earlier.
			send_to_all("АВТОМАТИЧЕСКАЯ ПЕРЕЗАГРУЗКА ЧЕРЕЗ 30 МИНУТ.\r\n");
			shutdown_time = time(NULL) + 1800;
			circle_shutdown = 2;
			circle_reboot = 1;
		}
	}

	if (!(pulse % (AUCTION_PULSES * PASSES_PER_SEC))) {	//log("Auction update...");
		tact_auction();
		//log("Stop it...");
	}

	if (!(pulse % (SECS_PER_ROOM_AFFECT * PASSES_PER_SEC))) {	//log ("Player affect update...");
		room_affect_update();
		//log("Stop it...");
	}

	if (!(pulse % (SECS_PER_PLAYER_AFFECT * PASSES_PER_SEC))) {	//log ("Player affect update...");
		player_affect_update();
		//log("Stop it...");
	}



	if (!(pulse % (TIME_KOEFF * SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {	//log("Hour msg update...");
		hour_update();
		//log("Stop it...");
		//log("Weather and time...");
		weather_and_time(1);
		//log("Stop it...");
		//log("Paste mobiles...");
		paste_mobiles(-1);
		//log("Stop it...");
	}

	if (!((pulse + 5) % PULSE_ZONE)) {	//log("Zone update...");
		zone_update();
		//log("Stop it...");
	}


	if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {	//log("Affect update...");
		mobile_affect_update();
		//log("Stop it...");
		//log("Point update...");
		point_update();
		//log("Stop it...");
		//log("Save player index...");
		flush_player_index();
		//log("Stop it...");
	}

	if (pulse == 720) {	//log("Dupe player index...");
		dupe_player_index();
		//log("Stop it...");
	}
	//log("Beat points update...");
	if (!(pulse % PASSES_PER_SEC))
		beat_points_update(pulse / PASSES_PER_SEC);
	//  log("Stop it...");

	if (FRAC_SAVE && auto_save && !((pulse + 7) % PASSES_PER_SEC)) {	// 1 game secunde
		//log("Fractional Crash save all...");
		Crash_frac_save_all((pulse / PASSES_PER_SEC) % PLAYER_SAVE_ACTIVITY);
		//log("Stop it...");
		//log("Fractional Rent timer save all...");
		Crash_frac_rent_time((pulse / PASSES_PER_SEC) % OBJECT_SAVE_ACTIVITY);
		//log("Stop it...");
	}
//F@N++
	if (EXCHANGE_AUTOSAVETIME && auto_save && !((pulse + 9) % (EXCHANGE_AUTOSAVETIME * PASSES_PER_SEC))) {
		exchange_database_save();
	}

	if (EXCHANGE_AUTOSAVEBACKUPTIME && !((pulse + 9) % (EXCHANGE_AUTOSAVEBACKUPTIME * PASSES_PER_SEC))) {
		exchange_database_savebackup();
	}
//F@N--

	if (auto_save && !((pulse + 9) % (60 * PASSES_PER_SEC))) {
		SaveGlobalUID();
	}

	if (!FRAC_SAVE && auto_save && !((pulse + 11) % (60 * PASSES_PER_SEC))) {	// 1 minute
		if (++mins_since_crashsave >= autosave_time) {
			mins_since_crashsave = 0;
			//log("Crash save all...");
			Crash_save_all();
			//log("Stop it...");
			check_at = time(NULL);
			if (last_rent_check > check_at)
				last_rent_check = check_at;
			if (((check_at - last_rent_check) / 60)) {	//log("Crash rent time...");
				save_start = time(NULL);
				Crash_rent_time((check_at - last_rent_check) / 60);
				//log("Saving rent timer time = %ld(s)",time(NULL) - save_start);
				last_rent_check = time(NULL) - (check_at - last_rent_check) % 60;
				//log("Stop it...");
			}
		}
	}

	// снятие денег за шмот в клановых сундуках. сейв кланов, сундуков
	if (!((pulse + 14) % (60 * CHEST_UPDATE_PERIOD * PASSES_PER_SEC))) {
		Clan::ChestUpdate();
		Clan::ChestSave();
		Clan::ClanSave();
	}
	// оповещение о скорой кончине денег в дружине
	if (!((pulse + 15) % (60 * CHEST_INVOICE_PERIOD * PASSES_PER_SEC)))
		Clan::ChestInvoice();
	// обновление статов экспы в топе кланов для тех, кто вырубил показ на лету
	if (!((pulse + 16) % (60 * CLAN_TOP_REFRESH_PERIOD * PASSES_PER_SEC)))
		Clan::SyncTopExp();

	if (!((pulse + 17) % (5 * 60 * PASSES_PER_SEC))) {	/* 5 minutes *///log("Record usage...");
		record_usage();
		ban->reload_proxy_ban(ban->RELOAD_MODE_TMPFILE);
	}

	// раз в 5 минут вывод + сохранение списков неодобренных
	// TODO: добить тут с именами
	if (!((pulse + 18) % (5 * 60 * PASSES_PER_SEC))) {
		god_work_invoice();
		TitleSystem::save_title_list();
	}

// shapirus: ротация логов. сислог каждые 2 часа, остальные раз в сутки.
	if (!((pulse + 19) % PULSE_LOGROTATE)) {
		time_t r_now = time(NULL);
		syslog_n = localtime(&r_now);

// инициализируем
		if (lr_firstrun) {
			memcpy(&syslog_o, syslog_n, sizeof(struct tm));
			lr_firstrun = 0;
		}
// проверка необходимости ротации и ротация
		if ((syslog_n->tm_year != syslog_o.tm_year ||
		     syslog_n->tm_mon != syslog_o.tm_mon ||
		     syslog_n->tm_mday != syslog_o.tm_mday ||
		     syslog_n->tm_hour != syslog_o.tm_hour) && syslog_n->tm_hour % 5 == 0) {
			rotate(&logs[0], syslog_pos, syslog_n);
			syslog_pos = ftell(logs[0].logfile);
			memcpy(&syslog_o, syslog_n, sizeof(struct tm));
		}
	}


	//log("---------- Stop heartbeat ----------");
}


/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
void timediff(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
	if (a->tv_sec < b->tv_sec)
		*rslt = null_time;
	else if (a->tv_sec == b->tv_sec) {
		if (a->tv_usec < b->tv_usec)
			*rslt = null_time;
		else {
			rslt->tv_sec = 0;
			rslt->tv_usec = a->tv_usec - b->tv_usec;
		}
	} else {		/* a->tv_sec > b->tv_sec */
		rslt->tv_sec = a->tv_sec - b->tv_sec;
		if (a->tv_usec < b->tv_usec) {
			rslt->tv_usec = a->tv_usec + 1000000 - b->tv_usec;
			rslt->tv_sec--;
		} else
			rslt->tv_usec = a->tv_usec - b->tv_usec;
	}
}

/*
 * Add 2 time values.
 *
 * Patch sent by "d. hall" <dhall@OOI.NET> to fix 'static' usage.
 */
void timeadd(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
	rslt->tv_sec = a->tv_sec + b->tv_sec;
	rslt->tv_usec = a->tv_usec + b->tv_usec;

	while (rslt->tv_usec >= 1000000) {
		rslt->tv_usec -= 1000000;
		rslt->tv_sec++;
	}
}


void record_usage(void)
{
	int sockets_connected = 0, sockets_playing = 0;
	DESCRIPTOR_DATA *d;

	for (d = descriptor_list; d; d = d->next) {
		sockets_connected++;
		if (STATE(d) == CON_PLAYING)
			sockets_playing++;
	}

	log("nusage: %-3d sockets connected, %-3d sockets playing", sockets_connected, sockets_playing);

#ifdef RUSAGE			/* Not RUSAGE_SELF because it doesn't guarantee prototype. */
	{
		struct rusage ru;

		getrusage(RUSAGE_SELF, &ru);
		log("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
		    ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
	}
#endif

}



/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(DESCRIPTOR_DATA * d)
{
	char off_string[] = {
		(char) IAC,
		(char) WILL,
		(char) TELOPT_ECHO,
		(char) 0
	};

	SEND_TO_Q(off_string, d);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(DESCRIPTOR_DATA * d)
{
	char on_string[] = {
		(char) IAC,
		(char) WONT,
		(char) TELOPT_ECHO,
		(char) IAC,
		(char) WONT,
		(char) TELOPT_NAOFFD,
		(char) IAC,
		(char) WONT,
		(char) TELOPT_NAOCRD,
		(char) 0,
	};

	SEND_TO_Q(on_string, d);
}

int posi_value(int real, int max)
{
	if (real < 0)
		return (-1);
	else if (real >= max)
		return (10);

	return (real * 10 / MAX(max, 1));
}

char *color_value(CHAR_DATA * ch, int real, int max)
{
	static char color[256];
	switch (posi_value(real, max)) {
	case -1:
	case 0:
	case 1:
		sprintf(color, "%s", CCRED(ch, C_NRM));
		break;
	case 2:
	case 3:
		sprintf(color, "%s", CCIRED(ch, C_NRM));
		break;
	case 4:
	case 5:
		sprintf(color, "%s", CCIYEL(ch, C_NRM));
		break;
	case 6:
	case 7:
	case 8:
		sprintf(color, "%s", CCIGRN(ch, C_NRM));
		break;
	default:
		sprintf(color, "%s", CCGRN(ch, C_NRM));
		break;
	}
	return (color);
}

/*
char *show_state(CHAR_DATA *ch, CHAR_DATA *victim)
{ int ch_hp = 11;
  static char *WORD_STATE[12] =
              {"Умирает",
               "Ужасное",
               "О.Плохое",
               "Плохое",
               "Плохое",
               "Среднее",
               "Среднее",
               "Хорошее",
               "Хорошее",
               "Хорошее",
               "О.хорошее",
               "Велиполепное"};

  ch_hp = posi_value(GET_HIT(victim),GET_REAL_MAX_HIT(victim)) + 1;
  sprintf(buf, "%s[%s:%s]%s ",
          color_value(ch, GET_HIT(victim), GET_REAL_MAX_HIT(victim)),
          PERS(victim,ch,0),
          WORD_STATE[ch_hp],
          CCNRM(ch, C_NRM));
  return buf;
}
*/

char *show_state(CHAR_DATA * ch, CHAR_DATA * victim)
{
	int ch_hp = 11;
	static char *WORD_STATE[12] = { "Смертельно ранен",
		"О.тяжело ранен",
		"О.тяжело ранен",
		"Тяжело ранен",
		"Тяжело ранен",
		"Ранен",
		"Ранен",
		"Ранен",
		"Легко ранен",
		"Легко ранен",
		"Слегка ранен",
		"Невредим"
	};

	ch_hp = posi_value(GET_HIT(victim), GET_REAL_MAX_HIT(victim)) + 1;
	sprintf(buf, "%s[%s:%s%s]%s ",
		color_value(ch, GET_HIT(victim), GET_REAL_MAX_HIT(victim)),
		PERS(victim, ch, 0), WORD_STATE[ch_hp], GET_CH_SUF_6(victim), CCNRM(ch, C_NRM));
	return buf;
}





char *make_prompt(DESCRIPTOR_DATA * d)
{
	static char prompt[MAX_PROMPT_LENGTH + 1];
	static char *dirs[] = { "С", "В", "Ю", "З", "^", "v" };

	int ch_hp, sec_hp;
	int door;
	int perc;

	/* Note, prompt is truncated at MAX_PROMPT_LENGTH chars (structs.h ) */
	if (d->showstr_count)
		sprintf(prompt, "\rЛистать : <RETURN>, Q<К>онец, R<П>овтор, B<Н>азад, или номер страницы (%d/%d).", d->showstr_page, d->showstr_count);
	else if (d->str)
		strcpy(prompt, "] ");
	else if (STATE(d) == CON_PLAYING && !IS_NPC(d->character)) {
		int count = 0;
		*prompt = '\0';

		// Invisibitity
		if (GET_INVIS_LEV(d->character))
			count += sprintf(prompt + count, "i%d ", GET_INVIS_LEV(d->character));

		// Hits state
		if (PRF_FLAGGED(d->character, PRF_DISPHP)) {
			count +=
			    sprintf(prompt + count, "%s",
				    color_value(d->character, GET_HIT(d->character), GET_REAL_MAX_HIT(d->character)));
			count += sprintf(prompt + count, "%dH%s ", GET_HIT(d->character), CCNRM(d->character, C_NRM));
		}
		// Moves state
		if (PRF_FLAGGED(d->character, PRF_DISPMOVE)) {
			count +=
			    sprintf(prompt + count, "%s",
				    color_value(d->character, GET_MOVE(d->character), GET_REAL_MAX_MOVE(d->character)));
			count += sprintf(prompt + count, "%dM%s ", GET_MOVE(d->character), CCNRM(d->character, C_NRM));
		}
		// Mana state
		if (PRF_FLAGGED(d->character, PRF_DISPMANA)
		    && IS_MANA_CASTER(d->character)) {
			perc = (100 * GET_MANA_STORED(d->character)) / GET_MAX_MANA(d->character);
			count +=
			    sprintf(prompt + count, "%s%dз%s ",
				    CCMANA(d->character, C_NRM, perc),
				    GET_MANA_STORED(d->character), CCNRM(d->character, C_NRM));
		}
		// Expirience
		// if (PRF_FLAGGED(d->character, PRF_DISPEXP))
		//    count += sprintf(prompt + count, "%ldx ", GET_EXP(d->character));
		if (PRF_FLAGGED(d->character, PRF_DISPEXP)) {
			if (IS_IMMORTAL(d->character))
				count += sprintf(prompt + count, "??? ");
			else
				count += sprintf(prompt + count, "%ldо ",
						 level_exp(d->character,
							   GET_LEVEL(d->character) + 1) - GET_EXP(d->character));
		}
		// Mem Info
		if (PRF_FLAGGED(d->character, PRF_DISPMANA)
		    && !IS_MANA_CASTER(d->character)) {
			if (!MEMQUEUE_EMPTY(d->character)) {
				door = mana_gain(d->character);
				if (door) {
					sec_hp =
					    MAX(0, 1 + GET_MEM_TOTAL(d->character) - GET_MEM_COMPLETED(d->character));
					sec_hp = sec_hp * 60 / door;
					ch_hp = sec_hp / 60;
					sec_hp %= 60;
					count += sprintf(prompt + count, "Зауч:%d:%02d ", ch_hp, sec_hp);
				} else
					count += sprintf(prompt + count, "Зауч:- ");
			} else
				count += sprintf(prompt + count, "Зауч:0 ");
		}


		if (!FIGHTING(d->character)
		    || IN_ROOM(d->character) != IN_ROOM(FIGHTING(d->character))) {	/* SHOW NON COMBAT INFO */

			if (PRF_FLAGGED(d->character, PRF_DISPLEVEL))
				count += sprintf(prompt + count, "%dL ", GET_LEVEL(d->character));

			if (PRF_FLAGGED(d->character, PRF_DISPGOLD))
				count += sprintf(prompt + count, "%dG ", GET_GOLD(d->character));

			if (PRF_FLAGGED(d->character, PRF_DISPEXITS)) {
				count += sprintf(prompt + count, "Вых:");
				if (!AFF_FLAGGED(d->character, AFF_BLIND))
					for (door = 0; door < NUM_OF_DIRS; door++) {
						if (EXIT(d->character, door) &&
						    EXIT(d->character, door)->to_room != NOWHERE &&
						    !EXIT_FLAGGED(EXIT(d->character, door), EX_HIDDEN))
							count +=
							    EXIT_FLAGGED(EXIT(d->character, door),
									 EX_CLOSED) ? sprintf(prompt + count,
											      "(%s)",
											      dirs[door]) :
							    sprintf(prompt + count, "%s", dirs[door]);
					}
			}
		} else {
			if (PRF_FLAGGED(d->character, PRF_DISPFIGHT))
				count += sprintf(prompt + count, "%s", show_state(d->character, d->character));
			if (FIGHTING(FIGHTING(d->character))
			    && FIGHTING(FIGHTING(d->character)) != d->character)
				count +=
				    sprintf(prompt + count, "%s",
					    show_state(d->character, FIGHTING(FIGHTING(d->character))));
			count += sprintf(prompt + count, "%s", show_state(d->character, FIGHTING(d->character)));
		};
		strcat(prompt, "> ");
	} else if (STATE(d) == CON_PLAYING && IS_NPC(d->character))
		sprintf(prompt, "{%s}-> ", GET_NAME(d->character));
	else
		*prompt = '\0';

	return (prompt);
}


void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{
	struct txt_block *newt;

	CREATE(newt, struct txt_block, 1);
	newt->text = str_dup(txt);
	newt->aliased = aliased;

	/* queue empty? */
	if (!queue->head) {
		newt->next = NULL;
		queue->head = queue->tail = newt;
	} else {
		queue->tail->next = newt;
		queue->tail = newt;
		newt->next = NULL;
	}
}



int get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
	struct txt_block *tmp;

	/* queue empty? */
	if (!queue->head)
		return (0);

	tmp = queue->head;
	strcpy(dest, queue->head->text);
	*aliased = queue->head->aliased;
	queue->head = queue->head->next;

	free(tmp->text);
	free(tmp);

	return (1);
}



/* Empty the queues before closing connection */
void flush_queues(DESCRIPTOR_DATA * d)
{
	int dummy;

	if (d->large_outbuf) {
		d->large_outbuf->next = bufpool;
		bufpool = d->large_outbuf;
	}
	while (get_from_q(&d->input, buf2, &dummy));
}

/* Add a new string to a player's output queue */
void write_to_output(const char *txt, DESCRIPTOR_DATA * t)
{
	int size;

	/* if we're in the overflow state already, ignore this new output */
	if (t->bufptr < 0)
		return;

	if ((ubyte) * txt == 255) {
		return;
	}

	size = strlen(txt);

	/* if we have enough space, just write to buffer and that's it! */
	if (t->bufspace >= size) {
		strcpy(t->output + t->bufptr, txt);
		t->bufspace -= size;
		t->bufptr += size;
		return;
	}
	/*
	 * If the text is too big to fit into even a large buffer, chuck the
	 * new text and switch to the overflow state.
	 */
	if (size + t->bufptr > LARGE_BUFSIZE - 1) {
		t->bufptr = -1;
		buf_overflows++;
		return;
	}
	buf_switches++;

	/* if the pool has a buffer in it, grab it */
	if (bufpool != NULL) {
		t->large_outbuf = bufpool;
		bufpool = bufpool->next;
	} else {		/* else create a new one */
		CREATE(t->large_outbuf, struct txt_block, 1);
		CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
		buf_largecount++;
	}

	strcpy(t->large_outbuf->text, t->output);	/* copy to big buffer */
	t->output = t->large_outbuf->text;	/* make big buffer primary */
	strcat(t->output, txt);	/* now add new text */

	/* set the pointer for the next write */
	t->bufptr = strlen(t->output);
	/* calculate how much space is left in the buffer */
	t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;
}



/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


/*
 * get_bind_addr: Return a struct in_addr that should be used in our
 * call to bind().  If the user has specified a desired binding
 * address, we try to bind to it; otherwise, we bind to INADDR_ANY.
 * Note that inet_aton() is preferred over inet_addr() so we use it if
 * we can.  If neither is available, we always bind to INADDR_ANY.
 */

struct in_addr *get_bind_addr()
{
	static struct in_addr bind_addr;

	/* Clear the structure */
	memset((char *) &bind_addr, 0, sizeof(bind_addr));

	/* If DLFT_IP is unspecified, use INADDR_ANY */
	if (DFLT_IP == NULL) {
		bind_addr.s_addr = htonl(INADDR_ANY);
	} else {
		/* If the parsing fails, use INADDR_ANY */
		if (!parse_ip(DFLT_IP, &bind_addr)) {
			log("SYSERR: DFLT_IP of %s appears to be an invalid IP address", DFLT_IP);
			bind_addr.s_addr = htonl(INADDR_ANY);
		}
	}

	/* Put the address that we've finally decided on into the logs */
	if (bind_addr.s_addr == htonl(INADDR_ANY))
		log("Binding to all IP interfaces on this host.");
	else
		log("Binding only to IP address %s", inet_ntoa(bind_addr));

	return (&bind_addr);
}

#ifdef HAVE_INET_ATON

/*
 * inet_aton's interface is the same as parse_ip's: 0 on failure, non-0 if
 * successful
 */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
	return (inet_aton(addr, inaddr));
}

#elif HAVE_INET_ADDR

/* inet_addr has a different interface, so we emulate inet_aton's */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
	long ip;

	if ((ip = inet_addr(addr)) == -1) {
		return (0);
	} else {
		inaddr->s_addr = (unsigned long) ip;
		return (1);
	}
}

#else

/* If you have neither function - sorry, you can't do specific binding. */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
	log("SYSERR: warning: you're trying to set DFLT_IP but your system has no\n"
	    "functions to parse IP addresses (how bizarre!)");
	return (0);
}

#endif				/* INET_ATON and INET_ADDR */

unsigned long get_ip(const char *addr)
{
	static struct in_addr ip;
	parse_ip(addr, &ip);
	return (ip.s_addr);
}


/* Sets the kernel's send buffer size for the descriptor */
int set_sendbuf(socket_t s)
{
#if defined(SO_SNDBUF) && !defined(CIRCLE_MACINTOSH)
	int opt = MAX_SOCK_BUF;

	if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
		perror("SYSERR: setsockopt SNDBUF");
		return (-1);
	}
#if 0
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *) &opt, sizeof(opt)) < 0) {
		perror("SYSERR: setsockopt RCVBUF");
		return (-1);
	}
#endif

#endif

	return (0);
}

int new_descriptor(int s)
{
	socket_t desc;
	int sockets_connected = 0;
	socklen_t i;
	static int last_desc = 0;	/* last descriptor number */
	DESCRIPTOR_DATA *newd;
	struct sockaddr_in peer;
	struct hostent *from;

	/* accept the new connection */
	i = sizeof(peer);
	if ((desc = accept(s, (struct sockaddr *) &peer, &i)) == INVALID_SOCKET) {
		perror("SYSERR: accept");
		return (-1);
	}
	/* keep it from blocking */
	nonblock(desc);

	/* set the send buffer size */
	if (set_sendbuf(desc) < 0) {
		CLOSE_SOCKET(desc);
		return (0);
	}

	/* make sure we have room for it */
	for (newd = descriptor_list; newd; newd = newd->next)
		sockets_connected++;

	if (sockets_connected >= max_players) {
		SEND_TO_SOCKET("Sorry, RUS MUD is full right now... please try again later!\r\n", desc);
		CLOSE_SOCKET(desc);
		return (0);
	}
	/* create a new descriptor */
	CREATE(newd, DESCRIPTOR_DATA, 1);
	memset((char *) newd, 0, sizeof(DESCRIPTOR_DATA));

	/* find the sitename */
	if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr, sizeof(peer.sin_addr), AF_INET))) {	/* resolution failed */
		if (!nameserver_is_slow)
			perror("SYSERR: gethostbyaddr");

		/* find the numeric site address */
		strncpy(newd->host, (char *) inet_ntoa(peer.sin_addr), HOST_LENGTH);
		*(newd->host + HOST_LENGTH) = '\0';
	} else {
		strncpy(newd->host, from->h_name, HOST_LENGTH);
		*(newd->host + HOST_LENGTH) = '\0';
	}

	// ип в виде числа
	newd->ip = TxtToIp(newd->host);

	/* determine if the site is banned */
#if 0
	/*
	 * Log new connections - probably unnecessary, but you may want it.
	 * Note that your immortals may wonder if they see a connection from
	 * your site, but you are wizinvis upon login.
	 */
	sprintf(buf2, "New connection from [%s]", newd->host);
	mudlog(buf2, CMP, LVL_GOD, SYSLOG, FALSE);
#endif
	if (ban->is_banned(newd->host) == BanList::BAN_ALL) {
		time_t bantime = ban->getBanDate(newd->host);
		sprintf(buf, "Sorry, your IP is banned till %s",
			bantime == -1 ? "Infinite duration\r\n" : asctime(localtime(&bantime)));
		write_to_descriptor(desc, buf, strlen(buf));
		CLOSE_SOCKET(desc);
		// sprintf(buf2, "Connection attempt denied from [%s]", newd->host);
		// mudlog(buf2, CMP, LVL_GOD, SYSLOG, TRUE);
		free(newd);
		return (0);
	}

	/* initialize descriptor data */
	newd->descriptor = desc;
	newd->idle_tics = 0;
	newd->output = newd->small_outbuf;
	newd->bufspace = SMALL_BUFSIZE - 1;
	newd->input_time = time(0);
	newd->login_time = time(0);
	*newd->output = '\0';
	newd->bufptr = 0;
	newd->has_prompt = 1;	/* prompt is part of greetings */
	newd->keytable = KT_SELECTMENU;
	STATE(newd) = CON_GET_KEYTABLE;
	/*
	 * This isn't exactly optimal but allows us to make a design choice.
	 * Do we embed the history in descriptor_data or keep it dynamically
	 * allocated and allow a user defined history size?
	 */
	CREATE(newd->history, char *, HISTORY_SIZE);

	if (++last_desc == 1000)
		last_desc = 1;
	newd->desc_num = last_desc;

	/* prepend to list */
	newd->next = descriptor_list;
	descriptor_list = newd;

	SEND_TO_Q("Using keytable\r\n"
		  "  0) Koi-8\r\n"
		  "  1) Alt\r\n"
		  "  2) Windows(JMC,MMC)\r\n"
		  "  3) Windows(zMUD)\r\n" "  4) Windows(zMUD ver. 6+)\r\n" "Select one : ", newd);

#if defined(HAVE_ZLIB)
//  write_to_descriptor(newd->descriptor, will_sig, strlen(will_sig));
	write_to_descriptor(newd->descriptor, compress_will, strlen(compress_will));
#endif

	return (0);
}


/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 */
int process_output(DESCRIPTOR_DATA * t)
{
	char i[MAX_SOCK_BUF * 2], o[MAX_SOCK_BUF * 2], *pi, *po;
	int written = 0, offset, result, c;


	// Отправляю данные снуперам
	/* handle snooping: prepend "% " and send to snooper */
	if (t->output && t->snoop_by) {
		SEND_TO_Q("% ", t->snoop_by);
		SEND_TO_Q(t->output, t->snoop_by);
		SEND_TO_Q("%%", t->snoop_by);
	}

	pi = i;
	po = o;
	/* we may need this \r\n for later -- see below */
	strcpy(i, "\r\n");

	/* now, append the 'real' output */
	strcpy(i + 2, t->output);

	/* if we're in the overflow state, notify the user */
	if (t->bufptr < 0)
		strcat(i, "**OVERFLOW**\r\n");

	/* add the extra CRLF if the person isn't in compact mode */
	if (STATE(t) == CON_PLAYING && t->character && !IS_NPC(t->character) && !PRF_FLAGGED(t->character, PRF_COMPACT))
		strcat(i, "\r\n");

	/* add a prompt */
	strncat(i, make_prompt(t), MAX_PROMPT_LENGTH);

	/* easy color */
	if (t->character)
		proc_color(i, (clr(t->character, C_NRM)));

	/*
	 * now, send the output.  If this is an 'interruption', use the prepended
	 * CRLF, otherwise send the straight output sans CRLF.
	 */

	for (c = 0; *(pi + c); c++)
		*(pi + c) = (*(pi + c) == '_') ? ' ' : *(pi + c);

	switch (t->keytable) {
	case KT_ALT:
		for (; *pi; *po = KtoA(*pi), pi++, po++);
		break;
	case KT_WIN:
		for (; *pi; *po = KtoW(*pi), pi++, po++)
			if (*pi == 'я')
				*(po++) = 255;
		break;
	case KT_WINZ:
		for (; *pi; *po = KtoW2(*pi), pi++, po++);
		break;
	case KT_WINZ6:
		for (; *pi; *po = KtoW2(*pi), pi++, po++);
		break;
	default:
		for (; *pi; *po = *pi, pi++, po++);
		break;
	}
	*po = '\0';
	for (c = 0; o[c]; c++) {
		i[c] = o[c];
	}
	i[c] = 0;

	if (t->has_prompt)	/* && !t->connected) */
		offset = 0;
	else
		offset = 2;

	if (t->character && PRF_FLAGGED(t->character, PRF_GOAHEAD))
		strncat(i, str_goahead, MAX_PROMPT_LENGTH);

	/*
	 * This huge #ifdef could be a function of its own, if desired. -gg 2/27/99
	 */
#if defined(HAVE_ZLIB)
	if (t->deflate) {	/* Complex case, compression, write it out. */
		/* Keep compiler happy, and MUD, just in case we don't write anything. */
		result = 1;

		/* First we set up our input data. */
		t->deflate->avail_in = strlen(i + offset);
		t->deflate->next_in = (Bytef *) (i + offset);

		do {
			int df, prevsize = SMALL_BUFSIZE - t->deflate->avail_out;

			/* If there is input or the output has reset from being previously full, run compression again. */
			if (t->deflate->avail_in || t->deflate->avail_out == SMALL_BUFSIZE)
				if ((df = deflate(t->deflate, Z_SYNC_FLUSH)) != 0)
					log("SYSERR: process_output: deflate() returned %d.", df);

			/* There should always be something new to write out. */
			result =
			    write_to_descriptor(t->descriptor, t->small_outbuf + prevsize,
						SMALL_BUFSIZE - t->deflate->avail_out - prevsize);

			/* Wrap the buffer when we've run out of buffer space for the output. */
			if (t->deflate->avail_out == 0) {
				t->deflate->avail_out = SMALL_BUFSIZE;
				t->deflate->next_out = (Bytef *) t->small_outbuf;
			}

			/* Oops. This shouldn't happen, I hope. -gg 2/19/99 */
			if (result <= 0)
				return -1;

			/* Need to loop while we still have input or when the output buffer was previously full. */
		}
		while (t->deflate->avail_out == SMALL_BUFSIZE || t->deflate->avail_in);
	} else
#endif
		result = write_to_descriptor(t->descriptor, i + offset, strlen(i + offset));

	written = result >= 0 ? result : -result;

	/*
	 * if we were using a large buffer, put the large buffer on the buffer pool
	 * and switch back to the small one
	 */
	if (t->large_outbuf) {
		t->large_outbuf->next = bufpool;
		bufpool = t->large_outbuf;
		t->large_outbuf = NULL;
		t->output = t->small_outbuf;
	}
	/* reset total bufspace back to that of a small buffer */
	t->bufspace = SMALL_BUFSIZE - 1;
	t->bufptr = 0;
	*(t->output) = '\0';

	/* Error, cut off. */
	if (result == 0)
		return (-1);

	/* Normal case, wrote ok. */
	if (result > 0)
		return (1);

	/*
	 * We blocked, restore the unwritten output. Known
	 * bug in that the snooping immortal will see it twice
	 * but details...
	 */
	write_to_output(i + written + offset, t);
	return (0);
}


/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

#if defined(CIRCLE_WINDOWS)

ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
	ssize_t result;

	result = send(desc, txt, length, 0);

	if (result > 0) {
		/* Write was sucessful */
		number_of_bytes_written += result;
		return (result);
	}

	if (result == 0) {
		/* This should never happen! */
		log("SYSERR: Huh??  write() returned 0???  Please report this!");
		return (-1);
	}

	/* result < 0: An error was encountered. */

	/* Transient error? */
	if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
		return (0);

	/* Must be a fatal error. */
	return (-1);
}

#else

#if defined(CIRCLE_ACORN)
#define write	socketwrite
#endif

/* perform_socket_write for all Non-Windows platforms */
ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
	ssize_t result;

	result = write(desc, txt, length);

	if (result > 0) {
		/* Write was successful. */
		number_of_bytes_written += result;
		return (result);
	}

	if (result == 0) {
		/* This should never happen! */
		log("SYSERR: Huh??  write() returned 0???  Please report this!");
		return (-1);
	}

	/*
	 * result < 0, so an error was encountered - is it transient?
	 * Unfortunately, different systems use different constants to
	 * indicate this.
	 */

#ifdef EAGAIN			/* POSIX */
	if (errno == EAGAIN)
		return (0);
#endif

#ifdef EWOULDBLOCK		/* BSD */
	if (errno == EWOULDBLOCK)
		return (0);
#endif

#ifdef EDEADLK			/* Macintosh */
	if (errno == EDEADLK)
		return (0);
#endif

	/* Looks like the error was fatal.  Too bad. */
	return (-1);
}

#endif				/* CIRCLE_WINDOWS */


/*
 * write_to_descriptor takes a descriptor, and text to write to the
 * descriptor.  It keeps calling the system-level write() until all
 * the text has been delivered to the OS, or until an error is
 * encountered. 'written' is updated to add how many bytes were sent
 * over the socket successfully prior to the return. It is not zero'd.
 *
 * Returns:
 *  +  All is well and good.
 *  0  A fatal or unexpected error was encountered.
 *  -  The socket write would block.
 */
int write_to_descriptor(socket_t desc, const char *txt, size_t total)
{
	ssize_t bytes_written, total_written = 0;

	if (total == 0) {
		log("write_to_descriptor: write nothing?!");
		return 0;
	}

	while (total > 0) {
		bytes_written = perform_socket_write(desc, txt, total);

		if (bytes_written < 0) {
			/* Fatal error.  Disconnect the player. */
			perror("SYSERR: write_to_descriptor");
			return (0);
		} else if (bytes_written == 0) {
			/*
			 * Temporary failure -- socket buffer full.  For now we'll just
			 * cut off the player, but eventually we'll stuff the unsent
			 * text into a buffer and retry the write later.  JE 30 June 98.
			 * Implemented the anti-cut-off code he wanted. GG 13 Jan 99.
			 */
			log("WARNING: write_to_descriptor: socket write would block.");
			return (-total_written);
		} else {
			txt += bytes_written;
			total -= bytes_written;
			total_written += bytes_written;
		}
	}

	return (total_written);
}


/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
ssize_t perform_socket_read(socket_t desc, char *read_point, size_t space_left)
{
	ssize_t ret;

#if defined(CIRCLE_ACORN)
	ret = recv(desc, read_point, space_left, MSG_DONTWAIT);
#elif defined(CIRCLE_WINDOWS)
	ret = recv(desc, read_point, space_left, 0);
#else
	ret = read(desc, read_point, space_left);
#endif

	/* Read was successful. */
	if (ret > 0) {
		number_of_bytes_read += ret;
		return (ret);
	}

	/* read() returned 0, meaning we got an EOF. */
	if (ret == 0) {
		log("WARNING: EOF on socket read (connection broken by peer)");
		return (-1);
	}

	/*
	 * read returned a value < 0: there was an error
	 */

#if defined(CIRCLE_WINDOWS)	/* Windows */
	if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINTR)
		return (0);
#else

#ifdef EINTR			/* Interrupted system call - various platforms */
	if (errno == EINTR)
		return (0);
#endif

#ifdef EAGAIN			/* POSIX */
	if (errno == EAGAIN)
		return (0);
#endif

#ifdef EWOULDBLOCK		/* BSD */
	if (errno == EWOULDBLOCK)
		return (0);
#endif				/* EWOULDBLOCK */

#ifdef EDEADLK			/* Macintosh */
	if (errno == EDEADLK)
		return (0);
#endif

#endif				/* CIRCLE_WINDOWS */

	/*
	 * We don't know what happened, cut them off. This qualifies for
	 * a SYSERR because we have no idea what happened at this point.
	 */
	perror("SYSERR: perform_socket_read: about to lose connection");
	return (-1);
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(DESCRIPTOR_DATA * t)
{
	int buf_length, failed_subst;
	ssize_t bytes_read;
	size_t space_left;
	char *ptr, *read_point, *write_point, *nl_pos;
	char tmp[MAX_INPUT_LENGTH];

	/* first, find the point where we left off reading data */
	buf_length = strlen(t->inbuf);
	read_point = t->inbuf + buf_length;
	space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

	do {
		if (space_left <= 0) {
			log("WARNING: process_input: about to close connection: input overflow");
			return (-1);
		}

		bytes_read = perform_socket_read(t->descriptor, read_point, space_left);

		if (bytes_read < 0) {	/* Error, disconnect them. */
			return (-1);
		} else if (bytes_read == 0)	/* Just blocking, no problems. */
			return (0);

		/* at this point, we know we got some data from the read */

		read_point[bytes_read] = '\0';	/* terminate the string */

#if defined(HAVE_ZLIB)
		/* Search for an "Interpret As Command" marker. */
		for (ptr = read_point; *ptr; ptr++) {
			if (ptr[0] != (char) IAC)
				continue;
			if (ptr[1] == (char) IAC) {
				// последовательность IAC IAC
				// следует заменить просто на один IAC, но
				// для раскладок KT_WIN/KT_WINZ6 это произойдет ниже.
				// Почему так сделано - не знаю, но заменять не буду.
				++ptr;
			} else if (ptr[1] == (char) DO) {
				if (ptr[2] == (char) TELOPT_COMPRESS)
					mccp_start(t, 1);
				else if (ptr[2] == (char) TELOPT_COMPRESS2)
					mccp_start(t, 2);
				else
					continue;
				memmove(ptr, ptr + 3, bytes_read - (ptr - read_point) - 3 + 1);
				bytes_read -= 3;
				--ptr;
			} else if (ptr[1] == (char) DONT) {
				if (ptr[2] == (char) TELOPT_COMPRESS)
					mccp_end(t, 1);
				else if (ptr[2] == (char) TELOPT_COMPRESS2)
					mccp_end(t, 2);
				else
					continue;
				memmove(ptr, ptr + 3, bytes_read - (ptr - read_point) - 3 + 1);
				bytes_read -= 3;
				--ptr;
			}
		}
#endif

		/* search for a newline in the data we just read */
		for (ptr = read_point, nl_pos = NULL; *ptr && !nl_pos;) {
			if (ISNEWL(*ptr))
				nl_pos = ptr;
			ptr++;
		}

		read_point += bytes_read;
		space_left -= bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
#if !defined(POSIX_NONBLOCK_BROKEN)
	}
	while (nl_pos == NULL);
#else
	}
	while (0);

	if (nl_pos == NULL)
		return (0);
#endif				/* POSIX_NONBLOCK_BROKEN */

	/*
	 * okay, at this point we have at least one newline in the string; now we
	 * can copy the formatted data to a new array for further processing.
	 */

	read_point = t->inbuf;

	while (nl_pos != NULL) {
		write_point = tmp;
		space_left = MAX_INPUT_LENGTH - 1;

		for (ptr = read_point; (space_left > 1) && (ptr < nl_pos); ptr++) {
			/* Нафиг точку с запятой - задрали уроды с тригерами (Кард) */
			if (*ptr == ';' && (STATE(t) == CON_PLAYING || (STATE(t) == CON_EXDESC))) {
				/* Иммам или морталам с GF_DEMIGOD разрешено использовать ";". */
				if (GET_LEVEL(t->character) < LVL_IMMORT && !GET_GOD_FLAG(t->character, GF_DEMIGOD))
					*ptr = ',';
			}
			if (*ptr == '&' && (STATE(t) == CON_PLAYING || (STATE(t) == CON_EXDESC))) {
				if (GET_LEVEL(t->character) < LVL_IMPL)
					*ptr = '8';
			}
			if (*ptr == '\\' && (STATE(t) == CON_PLAYING || (STATE(t) == CON_EXDESC))) {
				if (GET_LEVEL(t->character) < LVL_GRGOD)
					*ptr = '/';
			}
	     /*****/
			if (*ptr == '\b' || *ptr == 127) {	/* handle backspacing or delete key */
				if (write_point > tmp) {
					if (*(--write_point) == '$') {
						write_point--;
						space_left += 2;
					} else
						space_left++;
				}
			} else if (isascii(*ptr) && isprint(*ptr)) {
				if ((*(write_point++) = *ptr) == '$') {	/* copy one character */
					*(write_point++) = '$';	/* if it's a $, double it */
					space_left -= 2;
				} else
					space_left--;
			} else if ((ubyte) * ptr > 127) {
				switch (t->keytable) {
				default:
					t->keytable = 0;
				case 0:
					*(write_point++) = *ptr;
					break;
				case KT_ALT:
					*(write_point++) = AtoK(*ptr);
					break;
				case KT_WIN:
				case KT_WINZ6:
					*(write_point++) = WtoK(*ptr);
					if (*ptr == (char) 255 && *(ptr + 1) == (char) 255 && ptr + 1 < nl_pos)
						ptr++;
					break;
				case KT_WINZ:
					*(write_point++) = WtoK(*ptr);
					break;
				}
				space_left--;
			}

			/* Для того чтобы работали все триги в змаде - заменяем все вводимые 'z'
			   на 'я' */
			if (STATE(t) == CON_PLAYING || (STATE(t) == CON_EXDESC)) {
				if (t->keytable == KT_WINZ6 || t->keytable == KT_WINZ) {
					if (*(write_point - 1) == 'z') {
						*(write_point - 1) = 'я';
					}
				}
			}

		}

		*write_point = '\0';

		if ((space_left <= 0) && (ptr < nl_pos)) {
			char buffer[MAX_INPUT_LENGTH + 64];

			sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
			SEND_TO_Q(buffer, t);
		}
		if (t->snoop_by) {
			SEND_TO_Q("% ", t->snoop_by);
			SEND_TO_Q(tmp, t->snoop_by);
			SEND_TO_Q("\r\n", t->snoop_by);
		}
		failed_subst = 0;

		if ((tmp[0] == '~') && (tmp[1] == 0)) {
			// очистка входной очереди
			int dummy;
			while (get_from_q(&t->input, buf2, &dummy));
			// SEND_TO_Q("Входной буфер очищен.\r\n", t);
			tmp[0] = 0;
		} else if (*tmp == '!' && !(*(tmp + 1)))
			/* Redo last command. */
			strcpy(tmp, t->last_input);
		else if (*tmp == '!' && *(tmp + 1)) {
			char *commandln = (tmp + 1);
			int starting_pos = t->history_pos,
			    cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

			skip_spaces(&commandln);
			for (; cnt != starting_pos; cnt--) {
				if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
					strcpy(tmp, t->history[cnt]);
					strcpy(t->last_input, tmp);
					SEND_TO_Q(tmp, t);
					SEND_TO_Q("\r\n", t);
					break;
				}
				if (cnt == 0)	/* At top, loop to bottom. */
					cnt = HISTORY_SIZE;
			}
		} else if (*tmp == '^') {
			if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
				strcpy(t->last_input, tmp);
		} else {
			strcpy(t->last_input, tmp);
			if (t->history[t->history_pos])
				free(t->history[t->history_pos]);	/* Clear the old line. */
			t->history[t->history_pos] = str_dup(tmp);	/* Save the new. */
			if (++t->history_pos >= HISTORY_SIZE)	/* Wrap to top. */
				t->history_pos = 0;
		}

		if (!failed_subst)
			write_to_q(tmp, &t->input, 0);

		/* find the end of this line */
		while (ISNEWL(*nl_pos))
			nl_pos++;

		/* see if there's another newline in the input buffer */
		read_point = ptr = nl_pos;
		for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
			if (ISNEWL(*ptr))
				nl_pos = ptr;
	}

	/* now move the rest of the buffer up to the beginning for the next pass */
	write_point = t->inbuf;
	while (*read_point)
		*(write_point++) = *(read_point++);
	*write_point = '\0';

	return (1);
}



/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
int perform_subst(DESCRIPTOR_DATA * t, char *orig, char *subst)
{
	char newsub[MAX_INPUT_LENGTH + 5];

	char *first, *second, *strpos;

	/*
	 * first is the position of the beginning of the first string (the one
	 * to be replaced
	 */
	first = subst + 1;

	/* now find the second '^' */
	if (!(second = strchr(first, '^'))) {
		SEND_TO_Q("Invalid substitution.\r\n", t);
		return (1);
	}
	/* terminate "first" at the position of the '^' and make 'second' point
	 * to the beginning of the second string */
	*(second++) = '\0';

	/* now, see if the contents of the first string appear in the original */
	if (!(strpos = strstr(orig, first))) {
		SEND_TO_Q("Invalid substitution.\r\n", t);
		return (1);
	}
	/* now, we construct the new string for output. */

	/* first, everything in the original, up to the string to be replaced */
	strncpy(newsub, orig, (strpos - orig));
	newsub[(strpos - orig)] = '\0';

	/* now, the replacement string */
	strncat(newsub, second, (MAX_INPUT_LENGTH - strlen(newsub) - 1));

	/* now, if there's anything left in the original after the string to
	 * replaced, copy that too. */
	if (((strpos - orig) + strlen(first)) < strlen(orig))
		strncat(newsub, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(newsub) - 1));

	/* terminate the string in case of an overflow from strncat */
	newsub[MAX_INPUT_LENGTH - 1] = '\0';
	strcpy(subst, newsub);

	return (0);
}



void close_socket(DESCRIPTOR_DATA * d, int direct)
{
	DESCRIPTOR_DATA *temp;

/*  if (!direct && d->character && RENTABLE(d->character))
     return; */
	// Нельзя делать лд при wait_state
	if (d->character && !direct) {
		if (CHECK_WAIT(d->character))
			return;
	}

	REMOVE_FROM_LIST(d, descriptor_list, next);
	CLOSE_SOCKET(d->descriptor);
	flush_queues(d);

	/* Forget snooping */
	if (d->snooping)
		d->snooping->snoop_by = NULL;

	if (d->snoop_by) {
		SEND_TO_Q("Ваш подопечный выключил компьютер.\r\n", d->snoop_by);
		d->snoop_by->snooping = NULL;
	}
	/*. Kill any OLC stuff . */
	switch (d->connected) {
	case CON_OEDIT:
	case CON_REDIT:
	case CON_ZEDIT:
	case CON_MEDIT:
	case CON_SEDIT:
	case CON_TRIGEDIT:
		cleanup_olc(d, CLEANUP_ALL);
	default:
		break;
	}

	if (d->character) {	/*
				 * Plug memory leak, from Eric Green.
				 */
		if (!IS_NPC(d->character) && (PLR_FLAGGED(d->character, PLR_MAILING) || STATE(d) == CON_WRITEBOARD) && d->str) {
			if (*(d->str))
				free(*(d->str));
			if (d->str != NULL)
				free(d->str);
		}

		if (STATE(d) == CON_WRITEBOARD || STATE(d) == CON_CLANEDIT)
			STATE(d) = CON_PLAYING;

		if (STATE(d) == CON_PLAYING || STATE(d) == CON_DISCONNECT) {
			act("$n потерял$g связь.", TRUE, d->character, 0, 0, TO_ROOM);
			if (!IS_NPC(d->character)) {
				save_char(d->character, NOWHERE);
				check_light(d->character, LIGHT_NO, LIGHT_NO, LIGHT_NO, LIGHT_NO, -1);
				Crash_ldsave(d->character);
				sprintf(buf, "Closing link to: %s.", GET_NAME(d->character));
				mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
			}
			d->character->desc = NULL;
		} else {
			sprintf(buf, "Losing player: %s.", GET_NAME(d->character) ? GET_NAME(d->character) : "<null>");
			mudlog(buf, LGH, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
			free_char(d->character);
		}
	} else
		mudlog("Losing descriptor without char.", LGH, LVL_IMMORT, SYSLOG, TRUE);

	/* JE 2/22/95 -- part of my unending quest to make switch stable */
	if (d->original && d->original->desc)
		d->original->desc = NULL;

	/* Clear the command history. */
	if (d->history) {
		int cnt;
		for (cnt = 0; cnt < HISTORY_SIZE; cnt++)
			if (d->history[cnt])
				free(d->history[cnt]);
		free(d->history);
	}

	if (d->showstr_head)
		free(d->showstr_head);
	if (d->showstr_count)
		free(d->showstr_vector);
#if defined(HAVE_ZLIB)
	if (d->deflate) {
		deflateEnd(d->deflate);
		free(d->deflate);
	}
#endif

	// TODO: деструктур не вызывается, пока у нас дескриптор не стал классом
	d->board.reset();
	d->message.reset();
	d->clan_olc.reset();
	d->clan_invite.reset();

	free(d);
}


void check_idle_passwords(void)
{
	DESCRIPTOR_DATA *d, *next_d;

	for (d = descriptor_list; d; d = next_d) {
		next_d = d->next;
		if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_NAME && STATE(d) != CON_GET_KEYTABLE)
			continue;
		if (!d->idle_tics) {
			d->idle_tics++;
			continue;
		} else {
			echo_on(d);
			SEND_TO_Q("\r\nTimed out... goodbye.\r\n", d);
			STATE(d) = CON_CLOSE;
		}
	}
}



/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

#if defined(CIRCLE_WINDOWS)

void nonblock(socket_t s)
{
	unsigned long val = 1;
	ioctlsocket(s, FIONBIO, &val);
}

#elif defined(CIRCLE_AMIGA)

void nonblock(socket_t s)
{
	long val = 1;
	IoctlSocket(s, FIONBIO, &val);
}

#elif defined(CIRCLE_ACORN)

void nonblock(socket_t s)
{
	int val = 1;
	socket_ioctl(s, FIONBIO, &val);
}

#elif defined(CIRCLE_VMS)

void nonblock(socket_t s)
{
	int val = 1;

	if (ioctl(s, FIONBIO, &val) < 0) {
		perror("SYSERR: Fatal error executing nonblock (comm.c)");
		exit(1);
	}
}

#elif defined(CIRCLE_UNIX) || defined(CIRCLE_OS2) || defined(CIRCLE_MACINTOSH)

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s)
{
	int flags;

	flags = fcntl(s, F_GETFL, 0);
	flags |= O_NONBLOCK;
	if (fcntl(s, F_SETFL, flags) < 0) {
		perror("SYSERR: Fatal error executing nonblock (comm.c)");
		exit(1);
	}
}

#endif				/* CIRCLE_UNIX || CIRCLE_OS2 || CIRCLE_MACINTOSH */


/* ******************************************************************
*  signal-handling functions (formerly signals.c).  UNIX only.      *
****************************************************************** */

#if defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)

RETSIGTYPE unrestrict_game(int sig)
{
	mudlog("Received SIGUSR2 - completely unrestricting game (emergent)", BRF, LVL_IMMORT, SYSLOG, TRUE);
	ban->clear_all();
	circle_restrict = 0;
	num_invalid = 0;
}

#ifdef CIRCLE_UNIX

/* clean up our zombie kids to avoid defunct processes */
RETSIGTYPE reap(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);

	my_signal(SIGCHLD, reap);
}

RETSIGTYPE crash_handle(int sig)
{
	log("Crash detected !");
	// Сливаем файловые буферы.
	fflush(stdout);
	fflush(stderr);

	for (int i = 0; i < NLOG; ++i)
		fflush(logs[i].logfile);

	my_signal(SIGSEGV, SIG_DFL);
	my_signal(SIGBUS, SIG_DFL);
}


RETSIGTYPE checkpointing(int sig)
{
	if (!tics) {
		log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
		abort();
	} else
		tics = 0;
}

RETSIGTYPE hupsig(int sig)
{
	log("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
	exit(1);		/* perhaps something more elegant should
				 * substituted */
}

#endif				/* CIRCLE_UNIX */

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

#ifndef POSIX
#define my_signal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func)
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
#ifdef SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

	if (sigaction(signo, &act, &oact) < 0)
		return (SIG_ERR);

	return (oact.sa_handler);
}
#endif				/* POSIX */


void signal_setup(void)
{
#ifndef CIRCLE_MACINTOSH
	struct itimerval itime;
	struct timeval interval;

	/*
	 * user signal 2: unrestrict game.  Used for emergencies if you lock
	 * yourself out of the MUD somehow.  (Duh...)
	 */
	my_signal(SIGUSR2, unrestrict_game);

	/*
	 * set up the deadlock-protection so that the MUD aborts itself if it gets
	 * caught in an infinite loop for more than 3 minutes.
	 */
	interval.tv_sec = 180;
	interval.tv_usec = 0;
	itime.it_interval = interval;
	itime.it_value = interval;
	setitimer(ITIMER_VIRTUAL, &itime, NULL);
	my_signal(SIGVTALRM, checkpointing);

	/* just to be on the safe side: */
	my_signal(SIGHUP, hupsig);
	my_signal(SIGCHLD, reap);
#endif				/* CIRCLE_MACINTOSH */
	my_signal(SIGINT, hupsig);
	my_signal(SIGTERM, hupsig);
	my_signal(SIGPIPE, SIG_IGN);
	my_signal(SIGALRM, SIG_IGN);
	my_signal(SIGSEGV, crash_handle);
	my_signal(SIGBUS, crash_handle);

}

#endif				/* CIRCLE_UNIX || CIRCLE_MACINTOSH */

/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */
void send_stat_char(CHAR_DATA * ch)
{
	char fline[256];
	sprintf(fline, "%d[%d]HP %d[%d]Mv %dG %dL ",
		GET_HIT(ch), GET_REAL_MAX_HIT(ch), GET_MOVE(ch), GET_REAL_MAX_MOVE(ch), GET_GOLD(ch), GET_LEVEL(ch));
	SEND_TO_Q(fline, ch->desc);
}

void send_to_char(const char *messg, CHAR_DATA * ch)
{
	if (ch->desc && messg)
		SEND_TO_Q(messg, ch->desc);
}

// New edition :)
void send_to_char(CHAR_DATA * ch, const char *messg, ...)
{
	va_list args;
	char tmpbuf[MAX_STRING_LENGTH];

	va_start(args, messg);
	vsprintf(tmpbuf, messg, args);
	va_end(args);

	if (ch->desc && messg)
		SEND_TO_Q(tmpbuf, ch->desc);
}

// ═ ╒╝Б Б╔ ╔И╔ ╝╓╜═ ╔╓╗Х╜ ├)
void send_to_char(const std::string & buffer, CHAR_DATA * ch)
{
	if (ch->desc && !buffer.empty())
		SEND_TO_Q(buffer.c_str(), ch->desc);
}


void send_to_all(const char *messg)
{
	DESCRIPTOR_DATA *i;

	if (messg == NULL)
		return;

	for (i = descriptor_list; i; i = i->next)
		if (STATE(i) == CON_PLAYING)
			SEND_TO_Q(messg, i);
}


void send_to_outdoor(const char *messg, int control)
{
	int room;
	DESCRIPTOR_DATA *i;

	if (!messg || !*messg)
		return;

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING || i->character == NULL)
			continue;
		if (!AWAKE(i->character) || !OUTSIDE(i->character))
			continue;
		room = IN_ROOM(i->character);
		if (!control ||
		    (IS_SET(control, SUN_CONTROL) &&
		     room != NOWHERE &&
		     SECT(room) != SECT_UNDERWATER &&
		     !AFF_FLAGGED(i->character, AFF_BLIND)) ||
		    (IS_SET(control, WEATHER_CONTROL) &&
		     room != NOWHERE &&
		     SECT(room) != SECT_UNDERWATER &&
		     !ROOM_FLAGGED(room, ROOM_NOWEATHER) && world[IN_ROOM(i->character)]->weather.duration <= 0))
			SEND_TO_Q(messg, i);
	}
}


void send_to_gods(const char *messg)
{
	DESCRIPTOR_DATA *i;

	if (!messg || !*messg)
		return;

	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING || i->character == NULL)
			continue;
		if (!IS_GOD(i->character))
			continue;
		SEND_TO_Q(messg, i);
	}
}


void send_to_room(const char *messg, room_rnum room, int to_awake)
{
	CHAR_DATA *i;

	if (messg == NULL)
		return;

	for (i = world[room]->people; i; i = i->next_in_room)
		if (i->desc && !IS_NPC(i) && (!to_awake || AWAKE(i)))
			SEND_TO_Q(messg, i->desc);
}


/* higher-level communication: the act() function */
void perform_act(const char *orig, CHAR_DATA * ch, const OBJ_DATA * obj, const void *vict_obj, CHAR_DATA * to)
{
	const char *i = NULL;
	char lbuf[MAX_STRING_LENGTH], *buf;
	ubyte padis;
	int stopbyte;
	CHAR_DATA *dg_victim = NULL;
	OBJ_DATA *dg_target = NULL;
	char *dg_arg = NULL;

	buf = lbuf;

	for (stopbyte = 0; stopbyte < MAX_STRING_LENGTH; stopbyte++) {
		if (*orig == '$') {
			switch (*(++orig)) {
			case 'n':
				if (*(orig + 1) < '0' || *(orig + 1) > '5')
					i = IS_IMMORTAL(ch) ? GET_NAME(ch) : PERS(ch, to, 0);
				else {
					padis = *(++orig) - '0';
					i = IS_IMMORTAL(ch) ? GET_PAD(ch, padis) : PERS(ch, to, padis);
				}
				break;
			case 'N':
				if (*(orig + 1) < '0' || *(orig + 1) > '5') {
					CHECK_NULL(vict_obj, PERS((const CHAR_DATA *) vict_obj, to, 0));
				} else {
					padis = *(++orig) - '0';
					CHECK_NULL(vict_obj, PERS((const CHAR_DATA *) vict_obj, to, padis));
				}
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'm':
				i = HMHR(ch);
				break;
			case 'M':
				if (vict_obj)
					i = HMHR((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, OMHR(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 's':
				i = HSHR(ch);
				break;
			case 'S':
				if (vict_obj)
					i = HSHR((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, OSHR(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'e':
				i = HSSH(ch);
				break;
			case 'E':
				if (vict_obj)
					i = HSSH((const CHAR_DATA *) vict_obj);
				else
					CHECK_NULL(obj, OSSH(obj));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'o':
				if (*(orig + 1) < '0' || *(orig + 1) > '5') {
					CHECK_NULL(obj, OBJN(obj, to, 0));
				} else {
					padis = *(++orig) - '0';
					CHECK_NULL(obj, OBJN(obj, to, padis > 5 ? 0 : padis));
				}
				break;
			case 'O':
				if (*(orig + 1) < '0' || *(orig + 1) > '5') {
					CHECK_NULL(vict_obj, OBJN((const OBJ_DATA *) vict_obj, to, 0));
				} else {
					padis = *(++orig) - '0';
					CHECK_NULL(vict_obj,
						   OBJN((const OBJ_DATA *) vict_obj, to, padis > 5 ? 0 : padis));
				}
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'p':
				CHECK_NULL(obj, OBJS(obj, to));
				break;
			case 'P':
				CHECK_NULL(vict_obj, OBJS((const OBJ_DATA *) vict_obj, to));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 't':
				CHECK_NULL(obj, (const char *) obj);
				break;

			case 'T':
				CHECK_NULL(vict_obj, (const char *) vict_obj);
				break;

			case 'F':
				CHECK_NULL(vict_obj, (const char *) vict_obj);
				break;

			case '$':
				i = "$";
				break;

			case 'a':
				i = IS_IMMORTAL(ch) ? GET_CH_SUF_6(ch) : GET_CH_VIS_SUF_6(ch, to);
				break;
			case 'A':
				if (vict_obj)
					i = GET_CH_VIS_SUF_6((const CHAR_DATA *) vict_obj, to);
				else
					CHECK_NULL(obj, GET_OBJ_VIS_SUF_6(obj, to));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'g':
				i = IS_IMMORTAL(ch) ? GET_CH_SUF_1(ch) : GET_CH_VIS_SUF_1(ch, to);
				break;
			case 'G':
				if (vict_obj)
					i = GET_CH_VIS_SUF_1((const CHAR_DATA *) vict_obj, to);
				else
					CHECK_NULL(obj, GET_OBJ_VIS_SUF_1(obj, to));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'y':
				i = IS_IMMORTAL(ch) ? GET_CH_SUF_5(ch) : GET_CH_VIS_SUF_5(ch, to);
				break;
			case 'Y':
				if (vict_obj)
					i = GET_CH_VIS_SUF_5((const CHAR_DATA *) vict_obj, to);
				else
					CHECK_NULL(obj, GET_OBJ_VIS_SUF_5(obj, to));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'u':
				i = IS_IMMORTAL(ch) ? GET_CH_SUF_2(ch) : GET_CH_VIS_SUF_2(ch, to);
				break;
			case 'U':
				if (vict_obj)
					i = GET_CH_VIS_SUF_2((const CHAR_DATA *) vict_obj, to);
				else
					CHECK_NULL(obj, GET_OBJ_VIS_SUF_2(obj, to));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'w':
				i = IS_IMMORTAL(ch) ? GET_CH_SUF_3(ch) : GET_CH_VIS_SUF_3(ch, to);
				break;
			case 'W':
				if (vict_obj)
					i = GET_CH_VIS_SUF_3((const CHAR_DATA *) vict_obj, to);
				else
					CHECK_NULL(obj, GET_OBJ_VIS_SUF_3(obj, to));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			case 'q':
				i = IS_IMMORTAL(ch) ? GET_CH_SUF_4(ch) : GET_CH_VIS_SUF_4(ch, to);
				break;
			case 'Q':
				if (vict_obj)
					i = GET_CH_VIS_SUF_4((const CHAR_DATA *) vict_obj, to);
				else
					CHECK_NULL(obj, GET_OBJ_VIS_SUF_4(obj, to));
				dg_victim = (CHAR_DATA *) vict_obj;
				break;

			default:
				log("SYSERR: Illegal $-code to act(): %c", *orig);
				log("SYSERR: %s", orig);
				i = "";
				break;
			}
			while ((*buf = *(i++)))
				buf++;
			orig++;
		} else if (*orig == '\\') {
			if (*(orig + 1) == 'r') {
				*(buf++) = '\r';
				orig += 2;
			} else if (*(orig + 1) == 'n') {
				*(buf++) = '\n';
				orig += 2;
			} else
				*(buf++) = *(orig++);
		} else if (!(*(buf++) = *(orig++)))
			break;
	}

	*(--buf) = '\r';
	*(++buf) = '\n';
	*(++buf) = '\0';

	if (to->desc) {
		/* Делаем первый символ большим, учитывая &X */
		if (lbuf[0] == '&')
			CAP(lbuf + 2);
		SEND_TO_Q(CAP(lbuf), to->desc);
	}

	if ((IS_NPC(to) && dg_act_check) && (to != ch))
		act_mtrigger(to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);
}

/* moved this to utils.h --- mah */
#ifndef SENDOK
#define SENDOK(ch)	((ch)->desc && (to_sleeping || AWAKE(ch)) && \
			(IS_NPC(ch) || !PLR_FLAGGED((ch), PLR_WRITING)))
#endif

void act(const char *str, int hide_invisible, CHAR_DATA * ch, const OBJ_DATA * obj, const void *vict_obj, int type)
{
	CHAR_DATA *to;
	int to_sleeping, check_deaf, check_nodeaf, stopcount;

	if (!str || !*str)
		return;

	if (!(dg_act_check = !(type & DG_NO_TRIG)))
		type &= ~DG_NO_TRIG;

	/*
	 * Warning: the following TO_SLEEP code is a hack.
	 *
	 * I wanted to be able to tell act to deliver a message regardless of sleep
	 * without adding an additional argument.  TO_SLEEP is 128 (a single bit
	 * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
	 * command.  It's not legal to combine TO_x's with each other otherwise.
	 * TO_SLEEP only works because its value "happens to be" a single bit;
	 * do not change it to something else.  In short, it is a hack.
	 */

	/* check if TO_SLEEP is there, and remove it if it is. */
	if ((to_sleeping = (type & TO_SLEEP)))
		type &= ~TO_SLEEP;
	if ((check_deaf = (type & CHECK_DEAF)))
		type &= ~CHECK_DEAF;
	if ((check_nodeaf = (type & CHECK_NODEAF)))
		type &= ~CHECK_NODEAF;

	if (type == TO_CHAR) {
		if (ch &&
		    SENDOK(ch) &&
		    IN_ROOM(ch) != NOWHERE &&
		    (!check_deaf || !AFF_FLAGGED(ch, AFF_DEAFNESS)) && (!check_nodeaf || AFF_FLAGGED(ch, AFF_DEAFNESS)))
			perform_act(str, ch, obj, vict_obj, ch);
		return;
	}

	if (type == TO_VICT) {
		if ((to = (CHAR_DATA *) vict_obj) != NULL &&
		    SENDOK(to) &&
		    IN_ROOM(to) != NOWHERE &&
		    (!check_deaf || !AFF_FLAGGED(to, AFF_DEAFNESS)) && (!check_nodeaf || AFF_FLAGGED(to, AFF_DEAFNESS)))
			perform_act(str, ch, obj, vict_obj, to);
		return;
	}
	/* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */
	/* or TO_ROOM_HIDE */

	if (ch && ch->in_room != NOWHERE)
		to = world[ch->in_room]->people;
	else if (obj && obj->in_room != NOWHERE)
		to = world[obj->in_room]->people;
	else {
		log("SYSERR: no valid target to act('%s')!", str);
		return;
	}

	for (stopcount = 0; to && stopcount < 1000; to = to->next_in_room, stopcount++) {
		if (!SENDOK(to) || (to == ch))
			continue;
		if (hide_invisible && ch && !CAN_SEE(to, ch))
			continue;
		if ((type != TO_ROOM && type != TO_ROOM_HIDE) && to == vict_obj)
			continue;
//надо отдельно PRF_DEAF
/*       if (!IS_NPC(to) && check_deaf && PRF_FLAGGED(to, PRF_NOTELL))
          continue; */
		if (check_deaf && AFF_FLAGGED(to, AFF_DEAFNESS))
			continue;
		if (check_nodeaf && !AFF_FLAGGED(to, AFF_DEAFNESS))
			continue;
		if (type == TO_ROOM_HIDE && !AFF_FLAGGED(to, AFF_SENSE_LIFE))
			continue;
		perform_act(str, ch, obj, vict_obj, to);
	}
}

/*
 * This function is called every 30 seconds from heartbeat().  It checks
 * the four global buffers in CircleMUD to ensure that no one has written
 * past their bounds.  If our check digit is not there (and the position
 * doesn't have a NUL which may result from snprintf) then we gripe that
 * someone has overwritten our buffer.  This could cause a false positive
 * if someone uses the buffer as a non-terminated character array but that
 * is not likely. -gg
 */
void sanity_check(void)
{
	int ok = TRUE;

	/*
	 * If any line is false, 'ok' will become false also.
	 */
	ok &= (test_magic(buf) == MAGIC_NUMBER || test_magic(buf) == '\0');
	ok &= (test_magic(buf1) == MAGIC_NUMBER || test_magic(buf1) == '\0');
	ok &= (test_magic(buf2) == MAGIC_NUMBER || test_magic(buf2) == '\0');
	ok &= (test_magic(arg) == MAGIC_NUMBER || test_magic(arg) == '\0');

	/*
	 * This isn't exactly the safest thing to do (referencing known bad memory)
	 * but we're doomed to crash eventually, might as well try to get something
	 * useful before we go down. -gg
	 * However, lets fix the problem so we don't spam the logs. -gg 11/24/98
	 */
	if (!ok) {
		log("SYSERR: *** Buffer overflow! ***\n" "buf: %s\nbuf1: %s\nbuf2: %s\narg: %s", buf, buf1, buf2, arg);

		plant_magic(buf);
		plant_magic(buf1);
		plant_magic(buf2);
		plant_magic(arg);
	}
#if 0
	log("Statistics: buf=%d buf1=%d buf2=%d arg=%d", strlen(buf), strlen(buf1), strlen(buf2), strlen(arg));
#endif
}

extern FILE *logfile;
/* Prefer the file over the descriptor. */
void setup_logs(void)
{
	FILE *s_fp;
	int i;

	for (i = 0; i < NLOG; ++i) {

#if defined(__MWERKS__) || defined(__GNUC__)
		s_fp = stderr;
#else
		if ((s_fp = fdopen(STDERR_FILENO, "w")) == NULL) {
			puts("SYSERR: Error opening stderr, trying stdout.");

			if ((s_fp = fdopen(STDOUT_FILENO, "w")) == NULL) {
				puts("SYSERR: Error opening stdout, trying a file.");
				if (logs[i].filename == NULL || *logs[i].filename == '\0') {
					puts("SYSERR: No filename specified.");
					exit(1);
				}
			}
		}
#endif

		getcwd(src_path, 4096);

		if (!logs[i].filename || *logs[i].filename == '\0') {
			logs[i].logfile = s_fp;
			puts("Using file descriptor for logging.");
			continue;
		}
		if (!open_logfile(logs + i, NULL))	//s_fp
		{
			puts("SYSERR: Couldn't open anything to log to, giving up.");
			exit(1);
		}
	}
	logfile = logs[SYSLOG].logfile;
}

int open_logfile(log_info * li, FILE * stderr_fp)
{
	if (stderr_fp)		/* freopen() the descriptor. */
		li->logfile = freopen(li->filename, "w", stderr_fp);
	else
		li->logfile = fopen(li->filename, "w");

	if (li->logfile) {
		printf("Using log file '%s'%s.\n", li->filename, stderr_fp ? " with redirection" : "");
		return (TRUE);
	}

	printf("SYSERR: Error opening file '%s': %s\n", li->filename, strerror(errno));
	return (FALSE);
}

/*
 * This may not be pretty but it keeps game_loop() neater than if it was inline.
 */
#if defined(CIRCLE_WINDOWS)

inline void circle_sleep(struct timeval *timeout)
{
	Sleep(timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
}

#else

inline void circle_sleep(struct timeval *timeout)
{
	if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, timeout) < 0) {
		if (errno != EINTR) {
			perror("SYSERR: Select sleep");
			exit(1);
		}
	}
}

#endif				/* CIRCLE_WINDOWS */

#if defined(HAVE_ZLIB)

/* Compression stuff. */

void *zlib_alloc(void *opaque, unsigned int items, unsigned int size)
{
	return calloc(items, size);
}

void zlib_free(void *opaque, void *address)
{
	free(address);
}

#endif


#if defined(HAVE_ZLIB)

int mccp_start(DESCRIPTOR_DATA * t, int ver)
{
	int derr;

	if (t->deflate)
		return 1;	// компрессия уже включена

	/* Set up zlib structures. */
	CREATE(t->deflate, z_stream, 1);
	t->deflate->zalloc = zlib_alloc;
	t->deflate->zfree = zlib_free;
	t->deflate->opaque = NULL;
	t->deflate->next_in = (Bytef *) t->small_outbuf;
	t->deflate->next_out = (Bytef *) t->small_outbuf;
	t->deflate->avail_out = SMALL_BUFSIZE;
	t->deflate->avail_in = 0;

	/* Initialize. */
	if ((derr = deflateInit(t->deflate, Z_DEFAULT_COMPRESSION)) != 0) {
		log("SYSERR: deflateEnd returned %d.", derr);
		free(t->deflate);
		t->deflate = NULL;
		return 0;
	}

	if (ver != 2)
		write_to_descriptor(t->descriptor, compress_start_v1, strlen(compress_start_v1));
	else
		write_to_descriptor(t->descriptor, compress_start_v2, strlen(compress_start_v2));

	t->mccp_version = ver;
	return 1;
}


int mccp_end(DESCRIPTOR_DATA * t, int ver)
{
	int derr;
	int prevsize, pending;
	unsigned char tmp[1];

	if (t->deflate == NULL)
		return 1;

	if (t->mccp_version != ver)
		return 0;

	t->deflate->avail_in = 0;
	t->deflate->next_in = tmp;
	prevsize = SMALL_BUFSIZE - t->deflate->avail_out;

	log("SYSERR: about to deflate Z_FINISH.");

	if ((derr = deflate(t->deflate, Z_FINISH)) != Z_STREAM_END) {
		log("SYSERR: deflate returned %d upon Z_FINISH. (in: %d, out: %d)",
		    derr, t->deflate->avail_in, t->deflate->avail_out);
		return 0;
	}

	pending = SMALL_BUFSIZE - t->deflate->avail_out - prevsize;

	if (!write_to_descriptor(t->descriptor, t->small_outbuf + prevsize, pending))
		return 0;

	if ((derr = deflateEnd(t->deflate)) != Z_OK)
		log("SYSERR: deflateEnd returned %d. (in: %d, out: %d)", derr,
		    t->deflate->avail_in, t->deflate->avail_out);

	free(t->deflate);
	t->deflate = NULL;

	return 1;
}
#endif

int toggle_compression(DESCRIPTOR_DATA * t)
{
#if defined(HAVE_ZLIB)
	if (t->mccp_version == 0)
		return 0;
	if (t->deflate == NULL) {
		return mccp_start(t, t->mccp_version) ? 1 : 0;
	} else {
		return mccp_end(t, t->mccp_version) ? 0 : 1;
	}
#endif
	return 0;
}
