// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PARSE_HPP_INCLUDED
#define PARSE_HPP_INCLUDED

#include "conf.h"
#include "pugixml.hpp"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"

#include <string>

namespace TextId
{

enum IdType
{
	CHAR_CLASS,
	OBJ_VALS,
	TEXT_ID_COUNT
};

void init();

int to_num(IdType type, const std::string &str);
std::string to_str(IdType type, int num);

} // namespace TextId

namespace Parse
{

bool valid_obj_vnum(int vnum);

int attr_int(const pugi::xml_node &node, const char *text);
int attr_int_t(const pugi::xml_node &node, const char *text);
int child_value_int(const pugi::xml_node &node, const char *text);

std::string attr_str(const pugi::xml_node &node, const char *text);
std::string child_value_str(const pugi::xml_node &node, const char *text);

pugi::xml_node get_child(const pugi::xml_document &node, const char *name);
pugi::xml_node get_child(const pugi::xml_node &node, const char *name);

} // namespace Parse

#endif // PARSE_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
