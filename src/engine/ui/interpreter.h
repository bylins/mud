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
#include "utils/utils_parse.h"   // issue.interpreter-cleaning: parsing helpers moved here (transitional bridge)

#include <string>

class CharData;    // to avoid inclusion of "char.hpp"

void DoMove(CharData *ch, char *, int, int subcmd);

#define CMD_IS(cmd_name) (!strn_cmp(cmd_name, cmd_info[cmd].command, strlen(cmd_name)))

void command_interpreter(CharData *ch, char *argument);
// fill_word, half_chop moved to mud_string.h
void nanny(DescriptorData *d, char *argument);

// is_number moved to utils_string.h
int find_command(const char *command);
// блок подобной же фигни для стрингов
DescriptorData *DescriptorByUid(long uid);
int GetUniqueByName(std::string name, bool god = false);
std::string GetNameByUnique(long unique, bool god = false);
bool IsActiveUser(long unique);
void CreateFileName(std::string &name);
std::string ExpFormat(long long exp);
void name_convert(std::string &text);
int special(CharData *ch, int cmd, char *argument, int fnum);
int GetSpecprocFnum();
void do_specproc(CharData *ch, char *argument, int cmd, int subcmd);
void do_specword(CharData *ch, char *argument, int cmd, int subcmd);
void do_receive(CharData *ch, char *argument, int cmd, int subcmd);
int find_name(const char *name);

void check_hiding_cmd(CharData *ch, int percent);

// delete_doubledollar moved to utils_string.h
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
inline constexpr int kScmdWhisper{0};
inline constexpr int kScmdAsk{1};

// do_date
inline constexpr int kScmdDate{0};
inline constexpr int kScmdUptime{1};

// do_commands
inline constexpr int kScmdCommands{0};
inline constexpr int kScmdSocials{1};
inline constexpr int kScmdWizhelp{2};

// do_look
inline constexpr int kScmdLook{0};
inline constexpr int kScmdRead{1};
inline constexpr int kScmdLookHide{2};

// do_hit
inline constexpr int kScmdHit{0};
inline constexpr int kScmdMurder{1};

// do_echo
inline constexpr int kScmdEcho{0};
inline constexpr int kScmdEmote{1};

// do_hchannel
inline constexpr int kScmdChannel{0};
inline constexpr int kScmdAchannel{1};

// do_restore
inline constexpr int kScmdRestoreGod{0};
inline constexpr int kScmdRestoreTrigger{1};

struct SortStruct {
  int sort_pos;
  bool is_social;
};

extern SortStruct *cmd_sort_info;
extern int num_of_cmds;

// one_argument, any_one_arg, two_arguments, three_arguments, SplitArgument moved to mud_string.h

void SortCommands();

#endif // INTERPRETER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
