// Copyright (c) 2005 Krodo
// Part of Bylins http://www.mud.ru

#ifndef _BOARDS_H_
#define _BOARDS_H_

#include "boards.message.hpp"
#include "boards.types.hpp"
#include "db.h"
#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include "conf.h"
#include "sysdep.h"

#include <boost/array.hpp>

#include <string>
#include <vector>
#include <bitset>

namespace Boards
{
extern std::string dg_script_text;

class Loader
{
public:
	static void LoginInfo(CHAR_DATA * ch);
	static void BoardInit();
	static void ClanInit();
	static int Special(CHAR_DATA*, void*, int, char*);
	static std::string print_stats(CHAR_DATA *ch, Board::shared_ptr board, int num);
	static void reload_all();
	static void clear_god_boards();
	static void init_god_board(long uid, std::string name);
	static void do_list(CHAR_DATA* ch, const Board::shared_ptr board);

	static bool can_see(CHAR_DATA *ch, const Board::shared_ptr board);
	static bool can_read(CHAR_DATA *ch, const Board::shared_ptr board);
	static bool can_write(CHAR_DATA *ch, const Board::shared_ptr board);
	static bool full_access(CHAR_DATA *ch, const Board::shared_ptr board);
	static void clan_delete_message(const std::string &name, int vnum);
	static void new_message_notify(const Board::shared_ptr board);

private:
	static void create_board(BoardTypes type, const std::string &name,
		const std::string &desc, const std::string &file);

	static std::bitset<ACCESS_NUM> get_access(CHAR_DATA *ch, const Board::shared_ptr board);
};

void report_on_board(CHAR_DATA* ch, char* argument, int cmd, int subcmd);
void DoBoard(CHAR_DATA* ch, char* argument, int cmd, int subcmd);
void DoBoardList(CHAR_DATA* ch, char* argument, int cmd, int subcmd);

} // namespace BoardSystem

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
