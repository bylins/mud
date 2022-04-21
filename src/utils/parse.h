// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PARSE_HPP_INCLUDED
#define PARSE_HPP_INCLUDED

#include "conf.h"
#include "utils/pugixml/pugixml.h"
#include "sysdep.h"
#include "structs/structs.h"
#include "comm.h"

#include <string>

namespace text_id {

enum EIdType {
	kObjVals,
	kTextIdCount
};

void Init();

int ToNum(EIdType type, const std::string &str);
std::string ToStr(EIdType type, int num);

} // namespace TextId

namespace parse {

bool IsValidObjVnum(int vnum);

int ReadAttrAsInt(const pugi::xml_node &node, const char *text);
int ReadAttrAsIntT(const pugi::xml_node &node, const char *text);
int ReadChildValueAsInt(const pugi::xml_node &node, const char *text);

std::string ReadAattrAsStr(const pugi::xml_node &node, const char *text);
std::string ReadChildValueAsStr(const pugi::xml_node &node, const char *text);

pugi::xml_node GetChild(const pugi::xml_document &node, const char *name);
pugi::xml_node GetChild(const pugi::xml_node &node, const char *name);


const char *ReadAsStr(const char *value);
int ReadAsInt(const char *value);
float ReadAsFloat(const char *value);
double ReadAsDouble(const char *value);

template<typename T>
T ReadAsConstant(const char *value) {
	try {
		return ITEM_BY_NAME<T>(value);
	} catch (const std::out_of_range &) {
		throw std::runtime_error(value);
	}
}

} // namespace Parse

#endif // PARSE_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
