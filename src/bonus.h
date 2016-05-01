// Copyright (c) 2016 bodrich
// Part of Bylins http://www.bylins.su


#include <string>
#include <vector>
#include "interpreter.h"

namespace Bonus
{
	void do_bonus(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
	void do_bonus_info(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
	bool is_bonus(int type);
	void timer_bonus();
	std::string bonus_end();
	std::string str_type_bonus();
	void bonus_log_add(std::string name);
	int get_mult_bonus();
	void bonus_log_load();
	void show_log(CHAR_DATA *ch);
}