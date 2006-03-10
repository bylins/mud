/* ************************************************************************
*   File: interpreter.h                                 Part of Bylins    *
*  Usage: header file: public procs, macro defs, subcommand defines       *
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

#include <string>

#define ACMD(name)  \
   void name(CHAR_DATA *ch, char *argument, int cmd, int subcmd)

ACMD(do_move);

#define CMD_NAME (cmd_info[cmd].command)
#define CMD_IS(cmd_name) (!strn_cmp(cmd_name, cmd_info[cmd].command, strlen(cmd_name)))
#define IS_MOVE(cmdnum) (cmd_info[cmdnum].command_pointer == do_move)

void command_interpreter(CHAR_DATA * ch, char *argument);
int search_block(char *arg, const char **list, int exact);
char lower(char c);
char *one_argument(char *argument, char *first_arg);
char *one_word(char *argument, char *first_arg);
char *any_one_arg(char *argument, char *first_arg);
char *two_arguments(char *argument, char *first_arg, char *second_arg);
char *three_arguments(char *argument, char *first_arg, char *second_arg, char *third_arg);
int fill_word(char *argument);
void half_chop(char *string, char *arg1, char *arg2);
void nanny(DESCRIPTOR_DATA * d, char *arg);
int is_abbrev(const char *arg1, const char *arg2);
int is_number(const char *str);
int find_command(const char *command);
//подобная фигня для стрингов
void GetOneParam(std::string & buffer, std::string & buffer2);
bool CompareParam(const std::string & buffer, const char *arg, bool full = 0);
void SkipSpaces(std::string & buffer);

char *delete_doubledollar(char *string);
/** Cоответствие классов и религий (Кард)*/
extern const int class_religion[];
/****/

/* for compatibility with 2.20: */
#define argument_interpreter(a, b, c) two_arguments(a, b, c)


struct command_info {
	const char *command;
	byte minimum_position;
	void (*command_pointer)
	 (CHAR_DATA * ch, char *argument, int cmd, int subcmd);
	sh_int minimum_level;
	int subcmd;
	int unhide_percent;
};

/*
 * Necessary for CMD_IS macro.  Borland needs the structure defined first
 * so it has been moved down here.
 */
#ifndef __INTERPRETER_C__
extern const struct command_info cmd_info[];
#endif

/*
 * Alert! Changed from 'struct alias' to 'struct alias_data' in bpl15
 * because a Windows 95 compiler gives a warning about it having similiar
 * named member.
 */
struct alias_data {
	char *alias;
	char *replacement;
	int type;
	struct alias_data *next;
};

#define ALIAS_SIMPLE 0
#define ALIAS_COMPLEX   1

#define ALIAS_SEP_CHAR  ';'
#define ALIAS_VAR_CHAR  '$'
#define ALIAS_GLOB_CHAR '*'

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

/* directions */
#define SCMD_NORTH   1
#define SCMD_EAST 2
#define SCMD_SOUTH   3
#define SCMD_WEST 4
#define SCMD_UP      5
#define SCMD_DOWN 6

/* do_gen_ps */
#define SCMD_INFO       0
#define SCMD_HANDBOOK   1
#define SCMD_CREDITS    2
#define SCMD_NEWS       3
// ╙╫╧┬╧─╬╧
#define SCMD_POLICIES   5
#define SCMD_VERSION    6
#define SCMD_IMMLIST    7
#define SCMD_MOTD       8
#define SCMD_RULES      9
#define SCMD_CLEAR     10
#define SCMD_WHOAMI    11
#define SCMD_GODNEWS   12

/* do_gen_tog */
#define SCMD_NOSUMMON   0
#define SCMD_NOHASSLE   1
#define SCMD_BRIEF      2
#define SCMD_COMPACT    3
#define SCMD_NOTELL  4
#define SCMD_NOAUCTION  5
#define SCMD_NOHOLLER   6
#define SCMD_NOGOSSIP   7
#define SCMD_NOGRATZ 8
#define SCMD_NOWIZ   9
#define SCMD_QUEST      10
#define SCMD_ROOMFLAGS  11
#define SCMD_NOREPEAT   12
#define SCMD_HOLYLIGHT  13
#define SCMD_SLOWNS  14
#define SCMD_AUTOEXIT   15
#define SCMD_TRACK   16
#define SCMD_COLOR      17
#define SCMD_CODERINFO  18
#define SCMD_AUTOMEM    19
#define SCMD_COMPRESS   20
#define SCMD_AUTOZLIB   21
#define SCMD_NOSHOUT    22
#define SCMD_GOAHEAD    23
#define SCMD_SHOWGROUP  24
#define SCMD_AUTOASSIST 25
#define SCMD_AUTOLOOT   26
#define SCMD_AUTOSPLIT  27
#define SCMD_AUTOMONEY  28
#define SCMD_NOARENA    29
//F@N|
#define SCMD_NOEXCHANGE 30
//shapirus
#define SCMD_NOCLONES	31
#define SCMD_NOINVISTELL	32

/* do_wizutil */
#define SCMD_REROLL     0
#define SCMD_PARDON     1
#define SCMD_NOTITLE    2
#define SCMD_SQUELCH    3
#define SCMD_FREEZE     4
#define SCMD_THAW       5
#define SCMD_UNAFFECT   6
#define SCMD_HELL       7
#define SCMD_NAME       8
#define SCMD_REGISTER   9
#define SCMD_MUTE       10
#define SCMD_DUMB       11
#define SCMD_UNREGISTER   12

/* do_spec_com */
#define SCMD_WHISPER 0
#define SCMD_ASK  1

/* do_gen_com */
#define SCMD_HOLLER  0
#define SCMD_SHOUT   1
#define SCMD_GOSSIP  2
#define SCMD_AUCTION 3

/* do_shutdown */
#define SCMD_SHUTDOW 0
#define SCMD_SHUTDOWN   1

/* do_quit */
#define SCMD_QUI  0
#define SCMD_QUIT 1

/* do_date */
#define SCMD_DATE 0
#define SCMD_UPTIME  1


/* do_commands */
#define SCMD_COMMANDS   0
#define SCMD_SOCIALS 1
#define SCMD_WIZHELP 2

/*  do helpee */
#define SCMD_BUYHELPEE  0
#define SCMD_FREEHELPEE 1

/* do_drop */
#define SCMD_DROP 0
#define SCMD_JUNK 1
#define SCMD_DONATE  2

/* do_pray */
#define SCMD_PRAY   0

/* do_gen_write */
#define SCMD_BUG  0
#define SCMD_TYPO 1
#define SCMD_IDEA 2

/* do_look */
#define SCMD_LOOK    0
#define SCMD_READ    1
#define SCMD_LOOK_HIDE 2

/* do_pour */
#define SCMD_POUR 0
#define SCMD_FILL 1

/* do_poof */
#define SCMD_POOFIN  0
#define SCMD_POOFOUT 1

/* do_hit */
#define SCMD_HIT  0
#define SCMD_MURDER  1

/* do_eat */
#define SCMD_EAT  0
#define SCMD_TASTE   1
#define SCMD_DRINK   2
#define SCMD_SIP  3
#define SCMD_DEVOUR 4

/* do_use */
#define SCMD_USE  0
#define SCMD_QUAFF   1
#define SCMD_RECITE  2

/* do_echo */
#define SCMD_ECHO 0
#define SCMD_EMOTE   1

/* do_gen_door */
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4

/* do_mixture */
#define SCMD_ITEMS      0
#define SCMD_RUNES      1

/*. do_olc .*/
#define SCMD_OLC_REDIT  0
#define SCMD_OLC_OEDIT  1
#define SCMD_OLC_ZEDIT  2
#define SCMD_OLC_MEDIT  3
#define SCMD_OLC_SEDIT  4
#define SCMD_OLC_TRIGEDIT  5
#define SCMD_OLC_SAVEINFO  6

#define SCMD_RECIPE        1

/*. do_liblist .*/
#define SCMD_OLIST      0
#define SCMD_MLIST      1
#define SCMD_RLIST      2
#define SCMD_ZLIST      3

/* do_wake*/
#define SCMD_WAKE 0
#define SCMD_WAKEUP  1

/* do_set_politics */
#define SCMD_NEUTRAL 0
#define SCMD_WAR 1
#define SCMD_ALLIANCE 2

/* do_hchannel */
#define SCMD_CHANNEL 0
#define SCMD_ACHANNEL 1
