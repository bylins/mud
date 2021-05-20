// Copyright (c) 2016 bodrich
// Part of Bylins http://www.bylins.su
#include "bonus.types.hpp"

#include <string>

class CHAR_DATA;	// to avoid inclusion of "char.hpp"

namespace Bonus
{
	void setup_bonus(int duration_in_seconds, int multilpier, EBonusType type);

	void do_bonus_by_character(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
	void do_bonus_info(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
	bool is_bonus(int type);
	void timer_bonus();
	std::string bonus_end();
	std::string str_type_bonus();
	int get_mult_bonus();
	void bonus_log_load();
	void show_log(CHAR_DATA *ch);
	void dg_do_bonus(char *cmd);
}