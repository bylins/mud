// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef NOOB_HPP_INCLUDED
#define NOOB_HPP_INCLUDED

#include "engine/structs/structs.h"
#include "engine/boot/cfg_manager.h"

#include <string>
#include <vector>

class CharData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

namespace Noob {

// Loads cfg/mechanics/noob.xml (noob max level + per-class start outfit) via cfg_manager:
// boot + hot reload (reload noobhelp).
class NoobLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

int outfit(CharData *ch, void *me, int cmd, char *argument);

bool is_noob(const CharData *ch);
std::string print_start_outfit(CharData *ch);
std::vector<int> get_start_outfit(CharData *ch);
// City start outfit (cities.xml <start_item>); call only AFTER the char is placed in their room.
void give_city_start_outfit(CharData *ch);
void check_help_message(CharData *ch);
void equip_start_outfit(CharData *ch, ObjData *obj);

} // namespace Noob

#endif // NOOB_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
