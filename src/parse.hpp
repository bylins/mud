// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PARSE_HPP_INCLUDED
#define PARSE_HPP_INCLUDED

#include "conf.h"
#include <string>
#include "pugixml.hpp"

#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"

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
int child_value_int(const pugi::xml_node &node, const char *text);

std::string attr_str(const pugi::xml_node &node, const char *text);
std::string child_value_str(const pugi::xml_node &node, const char *text);

template <class T>
pugi::xml_node get_child(const T &node, const char *name)
{
    pugi::xml_node tmp_node = node.child(name);
    if (!tmp_node)
    {
    	char tmp[100];
		snprintf(tmp, sizeof(tmp), "...<%s> read fail", name);
		mudlog(tmp, CMP, LVL_IMMORT, SYSLOG, TRUE);
    }
    return tmp_node;
}

} // namespace Parse

time_t parse_asctime(const std::string &str);

#endif // PARSE_HPP_INCLUDED
