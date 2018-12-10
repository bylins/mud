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

#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "conf.h"
#include "structs.h"

#include <string>

class CHAR_DATA;	// to avoid inclusion of "char.hpp"

void do_move(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

#define CMD_NAME (cmd_info[cmd].command)
#define CMD_IS(cmd_name) (!strn_cmp(cmd_name, cmd_info[cmd].command, strlen(cmd_name)))
#define IS_MOVE(cmdnum) (cmd_info[cmdnum].command_pointer == do_move)

void command_interpreter(CHAR_DATA * ch, char *argument);
int search_block(const char *arg, const char **list, int exact);
int search_block(const std::string &arg, const char **list, int exact);
int fill_word(const char *argument);
void half_chop(char const *string, char *arg1, char *arg2);
void nanny(DESCRIPTOR_DATA * d, char *arg);

/**
* returns 1 if arg1 is an abbreviation of arg2
*/
int is_abbrev(const char *arg1, const char *arg2);
inline int is_abbrev(const std::string& arg1, const char *arg2) { return is_abbrev(arg1.c_str(), arg2); }

int is_number(const char *str);
int find_command(const char *command);
// блок подобной же фигни для стрингов
void GetOneParam(std::string & buffer, std::string & buffer2);
bool CompareParam(const std::string & buffer, const char *arg, bool full = 0);
bool CompareParam(const std::string & buffer, const std::string & buffer2, bool full = 0);
DESCRIPTOR_DATA *DescByUID(int uid);
DESCRIPTOR_DATA* get_desc_by_id(long id, bool playing = 1);
long GetUniqueByName(const std::string & name, bool god = 0);
std::string GetNameByUnique(long unique, bool god = 0);
void CreateFileName(std::string &name);
std::string ExpFormat(long long exp);
void lower_convert(std::string& text);
void lower_convert(char* text);
void name_convert(std::string& text);
void god_work_invoice();
int special(CHAR_DATA * ch, int cmd, char *arg, int fnum);
int find_name(const char *name);

void check_hiding_cmd(CHAR_DATA * ch, int percent);
void do_aggressive_room(CHAR_DATA *ch, int check_sneak);

char *delete_doubledollar(char *string);
// Cоответствие классов и религий (Кард)
extern const int class_religion[];

// for compatibility with 2.20:
#define argument_interpreter(a, b, c) two_arguments(a, b, c)

struct command_info
{
	const char *command;
	byte minimum_position;
	void (*command_pointer)(CHAR_DATA * ch, char *argument, int cmd, int subcmd);
	sh_int minimum_level;
	int subcmd;				///< Subcommand. See SCMD_* constants.
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
struct alias_data
{
	char *alias;
	char *replacement;
	int type;
	struct alias_data *next;
};

#define ALIAS_SIMPLE 0
#define ALIAS_COMPLEX   1

#define ALIAS_SEP_CHAR  '+'
#define ALIAS_VAR_CHAR  '='
#define ALIAS_GLOB_CHAR '*'

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

// directions
#define SCMD_NORTH   1
#define SCMD_EAST 2
#define SCMD_SOUTH   3
#define SCMD_WEST 4
#define SCMD_UP      5
#define SCMD_DOWN 6

// do_gen_ps
#define SCMD_INFO       0
#define SCMD_HANDBOOK   1
#define SCMD_CREDITS    2
// free
// free
#define SCMD_POLICIES   5
#define SCMD_VERSION    6
#define SCMD_IMMLIST    7
#define SCMD_MOTD       8
#define SCMD_RULES      9
#define SCMD_CLEAR     10
#define SCMD_WHOAMI    11

// do_gen_tog
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
//#define SCMD_COLOR      17  теперь свободно
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
#define SCMD_NOEXCHANGE 30
#define SCMD_NOCLONES	31
#define SCMD_NOINVISTELL 32
#define SCMD_LENGTH      33
#define SCMD_WIDTH       34
#define SCMD_SCREEN      35
#define SCMD_NEWS_MODE   36
#define SCMD_BOARD_MODE  37
#define SCMD_CHEST_MODE  38
#define SCMD_PKL_MODE    39
#define SCMD_POLIT_MODE  40
#define SCMD_PKFORMAT_MODE 41
#define SCMD_WORKMATE_MODE 42
#define SCMD_OFFTOP_MODE   43
#define SCMD_ANTIDC_MODE   44
#define SCMD_NOINGR_MODE   45
#define SCMD_REMEMBER      46
#define SCMD_NOTIFY_EXCH   47
#define SCMD_DRAW_MAP      48
#define SCMD_ENTER_ZONE    49
#define SCMD_MISPRINT      50
#define SCMD_BRIEF_SHIELDS 51
#define SCMD_AUTO_NOSUMMON 52
#define SCMD_SDEMIGOD 53
#define SCMD_BLIND 54
#define SCMD_MAPPER 55
#define SCMD_TESTER 56
#define SCMD_IPCONTROL 57

// do_wizutil
#define SCMD_REROLL     0
#define SCMD_NOTITLE    1
#define SCMD_SQUELCH    2
#define SCMD_FREEZE     3
#define SCMD_UNAFFECT   4
#define SCMD_HELL       5
#define SCMD_NAME       6
#define SCMD_REGISTER   7
#define SCMD_MUTE       8
#define SCMD_DUMB       9
#define SCMD_UNREGISTER 10

// do_spec_com
#define SCMD_WHISPER 0
#define SCMD_ASK  1

// do_gen_com
#define SCMD_HOLLER  0
#define SCMD_SHOUT   1
#define SCMD_GOSSIP  2
#define SCMD_AUCTION 3

// do_shutdown
#define SCMD_SHUTDOW 0
#define SCMD_SHUTDOWN   1

// do_quit
#define SCMD_QUI  0
#define SCMD_QUIT 1

// do_date
#define SCMD_DATE 0
#define SCMD_UPTIME  1


// do_commands
#define SCMD_COMMANDS   0
#define SCMD_SOCIALS 1
#define SCMD_WIZHELP 2

// do_fit
#define SCMD_DO_ADAPT 0
#define SCMD_MAKE_OVER 1

//  do helpee
#define SCMD_BUYHELPEE  0
#define SCMD_FREEHELPEE 1

// do_drop
#define SCMD_DROP 0
#define SCMD_DONATE 1

// do_pray
#define SCMD_PRAY   0

// do_look
#define SCMD_LOOK    0
#define SCMD_READ    1
#define SCMD_LOOK_HIDE 2

// do_pour
#define SCMD_POUR 0
#define SCMD_FILL 1

// do_poof
#define SCMD_POOFIN  0
#define SCMD_POOFOUT 1

// do_hit
#define SCMD_HIT  0
#define SCMD_MURDER  1

// do_eat
#define SCMD_EAT  0
#define SCMD_TASTE   1
#define SCMD_DRINK   2
#define SCMD_SIP  3
#define SCMD_DEVOUR 4

// do_use
#define SCMD_USE  0
#define SCMD_QUAFF   1
#define SCMD_RECITE  2

// do_echo
#define SCMD_ECHO 0
#define SCMD_EMOTE   1

// do_gen_door
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4

// do_mixture
#define SCMD_ITEMS      0
#define SCMD_RUNES      1

//. do_olc .
#define SCMD_OLC_REDIT  0
#define SCMD_OLC_OEDIT  1
#define SCMD_OLC_ZEDIT  2
#define SCMD_OLC_MEDIT  3
#define SCMD_OLC_TRIGEDIT  4
#define SCMD_OLC_SAVEINFO  5

#define SCMD_RECIPE        1

//. do_liblist .
#define SCMD_OLIST      0
#define SCMD_MLIST      1
#define SCMD_RLIST      2
#define SCMD_ZLIST      3
#define SCMD_CLIST		4

// do_wake
#define SCMD_WAKE 0
#define SCMD_WAKEUP  1

// do_hchannel
#define SCMD_CHANNEL 0
#define SCMD_ACHANNEL 1

// do_restore
#define SCMD_RESTORE_GOD 0
#define SCMD_RESTORE_TRIGGER 1

/**
* copy the first non-fill-word, space-delimited argument of 'argument'
* to 'first_arg'; return a pointer to the remainder of the string.
*/
char* one_argument(char* argument, char *first_arg);
const char* one_argument(const char* argument, char *first_arg);

///
/// same as one_argument except that it doesn't ignore fill words
/// как бы декларируем, что first_arg должен быть не менее MAX_INPUT_LENGTH
///
char* any_one_arg(char* argument, char* first_arg);
const char* any_one_arg(const char* argument, char* first_arg);

/**
* Same as one_argument except that it takes two args and returns the rest;
* ignores fill words
*/
template <typename T>
T two_arguments(T argument, char *first_arg, char *second_arg)
{
	return (one_argument(one_argument(argument, first_arg), second_arg));
}

template <typename T>
T three_arguments(T argument, char *first_arg, char *second_arg, char *third_arg)
{
	return (one_argument(one_argument(one_argument(argument, first_arg), second_arg), third_arg));
}

// константы для спам-контроля команды кто
// если кто захочет и сможет вынести их во внешний конфиг, то почет ему и слава

// максимум маны
#define WHO_MANA_MAX  6000
// расход на одно выполнение с выводом полного списка
#define WHO_COST  180
// расход на одно выполнение с поиском по имени
#define WHO_COST_NAME  30
// расход на вывод списка по кланам
#define WHO_COST_CLAN  120
// скорость восстановления
#define WHO_MANA_REST_PER_SECOND  9
// режимы выполнения
#define WHO_LISTALL 0
#define WHO_LISTNAME 1
#define WHO_LISTCLAN 2

bool login_change_invoice(CHAR_DATA* ch);
bool who_spamcontrol(CHAR_DATA *, unsigned short int);

#endif // _INTERPRETER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
