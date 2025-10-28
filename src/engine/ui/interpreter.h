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

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include "engine/core/conf.h"
#include "engine/network/descriptor_data.h"
#include "engine/entities/entities_constants.h"

#include <string>

class CharData;    // to avoid inclusion of "char.hpp"

void do_move(CharData *ch, char *argument, int cmd, int subcmd);

#define CMD_IS(cmd_name) (!strn_cmp(cmd_name, cmd_info[cmd].command, strlen(cmd_name)))

void command_interpreter(CharData *ch, char *argument);
int search_block(const char *target_string, const char **list, int exact);
int search_block(const std::string &block, const char **list, int exact);
int fill_word(const char *argument);
void half_chop(char const *string, char *arg1, char *arg2);
void nanny(DescriptorData *d, char *argument);

int is_number(const char *str);
int find_command(const char *command);
// блок подобной же фигни для стрингов
void GetOneParam(std::string &buffer, std::string &buffer2);
bool CompareParam(const std::string &buffer, const char *str, bool full = false);
bool CompareParam(const std::string &buffer, const std::string &buffer2, bool full = false);
DescriptorData *DescriptorByUid(long uid);
int GetUniqueByName(std::string_view name, bool god = false);
std::string GetNameByUnique(long unique, bool god = false);
bool IsActiveUser(long unique);
void CreateFileName(std::string &name);
std::string ExpFormat(long long exp);
void name_convert(std::string &text);
int special(CharData *ch, int cmd, char *argument, int fnum);
int find_name(const char *name);

void check_hiding_cmd(CharData *ch, int percent);

char *delete_doubledollar(char *string);
// Cоответствие классов и религий (Кард)
extern const int class_religion[];

struct command_info {
	const char *command;
	EPosition minimum_position;
	void (*command_pointer)(CharData *ch, char *argument, int cmd, int subcmd);
	sh_int minimum_level;
	int subcmd;                ///< Subcommand. See SCMD_* constants.
	int unhide_percent;
};

/*
 * Necessary for CMD_IS macro.  Borland needs the structure defined first
 * so, it has been moved down here.
 */
#ifndef INTERPRETER_CPP_
extern const struct command_info cmd_info[];
#endif

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent of function to function.
 */

// do_spec_com
inline constexpr int SCMD_WHISPER{0};
inline constexpr int SCMD_ASK{1};

// do_gen_com
inline constexpr int SCMD_HOLLER{0};
inline constexpr int SCMD_SHOUT{1};
inline constexpr int SCMD_GOSSIP{2};
inline constexpr int SCMD_AUCTION{3};

// do_date
inline constexpr int SCMD_DATE{0};
inline constexpr int SCMD_UPTIME{1};


// do_commands
inline constexpr int SCMD_COMMANDS{0};
inline constexpr int SCMD_SOCIALS{1};
inline constexpr int SCMD_WIZHELP{2};

// do_drop
inline constexpr int SCMD_DROP{0};

// do_look
inline constexpr int SCMD_LOOK{0};
inline constexpr int SCMD_READ{1};
inline constexpr int SCMD_LOOK_HIDE{2};

// do_pour
inline constexpr int SCMD_POUR{0};
inline constexpr int SCMD_FILL{1};

// do_poof
inline constexpr int SCMD_POOFIN{0};
inline constexpr int SCMD_POOFOUT{1};

// do_hit
inline constexpr int SCMD_HIT{0};
inline constexpr int SCMD_MURDER{1};

// do_drink
inline constexpr int SCMD_DRINK{2};
inline constexpr int SCMD_SIP{3};

// do_echo
inline constexpr int SCMD_ECHO{0};
inline constexpr int SCMD_EMOTE{1};

//. do_olc .
inline constexpr int SCMD_OLC_REDIT{0};
inline constexpr int SCMD_OLC_OEDIT{1};
inline constexpr int SCMD_OLC_ZEDIT{2};
inline constexpr int SCMD_OLC_MEDIT{3};
inline constexpr int SCMD_OLC_TRIGEDIT{4};
inline constexpr int SCMD_OLC_SAVEINFO{5};

//. do_liblist .
inline constexpr int SCMD_OLIST{0};
inline constexpr int SCMD_MLIST{1};
inline constexpr int SCMD_RLIST{2};
inline constexpr int SCMD_ZLIST{3};
inline constexpr int SCMD_CLIST{4};

// do_hchannel
inline constexpr int SCMD_CHANNEL{0};
inline constexpr int SCMD_ACHANNEL{1};

// do_restore
inline constexpr int SCMD_RESTORE_GOD{0};
inline constexpr int SCMD_RESTORE_TRIGGER{1};

// do_throw
inline constexpr int SCMD_PHYSICAL_THROW{0};
inline constexpr int SCMD_SHADOW_THROW{1};

struct SortStruct {
  int sort_pos;
  bool is_social;
};

extern SortStruct *cmd_sort_info;
extern int num_of_cmds;

/**
* copy the first non-fill-word, space-delimited argument of 'argument'
* to 'first_arg'; return a pointer to the remainder of the string.
*/
char *one_argument(char *argument, char *first_arg);
const char *one_argument(const char *argument, char *first_arg);

///
/// same as one_argument except that it doesn't ignore fill words
/// как бы декларируем, что first_arg должен быть не менее kMaxInputLength
///
char *any_one_arg(char *argument, char *first_arg);
const char *any_one_arg(const char *argument, char *first_arg);

/**
* Same as one_argument except that it takes two args and returns the rest;
* ignores fill words
*/
template<typename T>
T two_arguments(T argument, char *first_arg, char *second_arg) {
	return (one_argument(one_argument(argument, first_arg), second_arg));
}

template<typename T>
T three_arguments(T argument, char *first_arg, char *second_arg, char *third_arg) {
	return (one_argument(one_argument(one_argument(argument, first_arg), second_arg), third_arg));
}

// читает все аргументы из arguments в out
void SplitArgument(const char *arguments, std::vector<std::string> &out);
void SplitArgument(const char *arguments, std::vector<short> &out);
void SplitArgument(const char *arguments, std::vector<int> &out);

void SortCommands();

#endif // INTERPRETER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
