// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PRIVILEGE_HPP_INCLUDED
#define PRIVILEGE_HPP_INCLUDED

#include <string>
#include <set>
#include <bitset>
#include <map>

class CHAR_DATA;	// to avoid inclusion of "char.hpp"

namespace Privilege
{

void load();
bool god_list_check(const std::string &name, long unique);
void load_god_boards();
bool can_do_priv(CHAR_DATA *ch, const std::string &cmd_name, int cmd_number, int mode, bool check_level = true);
bool check_flag(const CHAR_DATA *ch, int flag);
bool check_spells(const CHAR_DATA *ch, int spellnum);
bool check_skills(const CHAR_DATA *ch);

extern const int BOARDS;
extern const int USE_SKILLS;
extern const int ARENA_MASTER;
extern const int KRODER;
extern const int FULLZEDIT;
extern const int TITLE;
extern const int MISPRINT;
extern const int SUGGEST;

} // namespace Privilege

#endif // PRIVILEGE_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
