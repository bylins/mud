// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PRIVILEGE_HPP_INCLUDED
#define PRIVILEGE_HPP_INCLUDED

#include <string>
#include <set>
#include <bitset>
#include <map>

#include "gameplay/magic/spells.h"

class CharData;    // to avoid inclusion of "char.hpp"

namespace privilege {

void Load();
bool IsContainedInGodsList(const std::string &name, long unique);
void LoadGodBoards();
bool HasPrivilege(CharData *ch, const std::string &cmd_name, int cmd_number, int mode, bool check_level = true);
bool CheckFlag(const CharData *ch, int flag);
bool IsSpellPermit(const CharData *ch, ESpell spell_id);
bool CheckSkills(const CharData *ch);

// Level-based privilege predicates (moved off CharData). TODO: drive these from privilege.lst
// membership rather than character level.
[[nodiscard]] bool IsImmortal(const CharData *ch);
[[nodiscard]] bool IsGod(const CharData *ch);
[[nodiscard]] bool IsGrGod(const CharData *ch);
[[nodiscard]] bool IsImpl(const CharData *ch);
[[nodiscard]] bool IsOwner(const CharData *ch);  // issue.privilege-rework: replaces is_head
// issue.privilege-rework: may `ch` edit the Vedun data set `what` (e.g. "spells")? FullAccess/owner -> any.
[[nodiscard]] bool CanEditVedun(const CharData *ch, const std::string &what);

extern const int kBoards;
extern const int kUseSkills;
extern const int kArenaMaster;
extern const int kKroder;
extern const int kFullzedit;
extern const int kTitle;
extern const int kMisprint;
extern const int kSuggest;

} // namespace Privilege

#endif // PRIVILEGE_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
