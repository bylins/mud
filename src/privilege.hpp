// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PRIVILEGE_HPP_INCLUDED
#define PRIVILEGE_HPP_INCLUDED

#include <string>
#include <set>
#include <bitset>
#include <map>

namespace Privilege {

void load();
bool god_list_check(const std::string &name, long unique);
void load_god_boards();
bool can_do_priv(CHAR_DATA *ch, const std::string &cmd_name, int cmd_number, int mode);
bool check_flag(CHAR_DATA *ch, int flag);
bool check_spells(CHAR_DATA *ch, int spellnum);
bool check_skills(CHAR_DATA *ch, int skills);

extern const int NEWS_MAKER;
extern const int USE_SKILLS;
extern const int ARENA_MASTER;
extern const int KRODER;
extern const int FULLZEDIT;
extern const int TITLE;

} // namespace Privilege

#endif // PRIVILEGE_HPP_INCLUDED
