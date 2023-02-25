// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef PARSE_HPP_INCLUDED
#define PARSE_HPP_INCLUDED

#include <string>
#include <set>

#include "conf.h"
#include "third_party_libs/pugixml/pugixml.h"
#include "sysdep.h"
#include "structs/structs.h"
#include "comm.h"
#include "utils/logger.h"

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
void ReadAsIntSet(std::unordered_set<int> &num_set, const char *value);
float ReadAsFloat(const char *value);
double ReadAsDouble(const char *value);
bool ReadAsBool(const char *value);

template<typename T>
T ReadAsConstant(const char *value) {
	try {
		return ITEM_BY_NAME<T>(value);
	} catch (const std::out_of_range &) {
		throw std::runtime_error(value);
	}
}

template<typename T>
void ReadAsConstantsSet(std::unordered_set<T> &roster, const char *value) {
	if (strcmp(value, "") == 0) {
		throw std::runtime_error("string is empty");
	}
	std::vector<std::string> str_array;
	utils::Split(str_array, value, '|');
	for (const auto &str: str_array) {
		try {
			roster.emplace(ITEM_BY_NAME<T>(str));
		} catch (...) {
			err_log("value '%s' is incorrcect constant in this context.", str.c_str());
		}
	}
}

template<typename T>
Bitvector ReadAsConstantsBitvector(const char *value) {
	if (strcmp(value, "") == 0) {
		throw std::runtime_error("string is empty");
	}
	std::vector<std::string> str_array;
	utils::Split(str_array, value, '|');
	Bitvector result{0u};
	for (const auto &str: str_array) {
		try {
			result |= ITEM_BY_NAME<T>(str);
		} catch (...) {
			err_log("value '%s' is incorrcect constant in this context.", str.c_str());
		}
	}

	return result;
}

/*
 * Конвертирует битвектор в набор строковых констант, разделенных символом "|".
 * ВНИМАНИЕ! Не используйте эту функцию для наборов констант, длиннее 29.
 * Из-за особенности хранения флаговв битвекторах обратная конвертация для
 * констант с номерами выше 29 (т.е. имеющими первые три бита, отличные от нуля)
 * работает некорректно.
 * Если нужно хранить набор таких констант, используйте std::set.
 */
template<typename T>
std::string BitvectorToString(Bitvector bits) {
	if (bits == 0u) {
		try {
			return NAME_BY_ITEM<T>(static_cast<T>(bits));
		} catch (...) {
			return "None";
		}
	}

	Bitvector flag;
	Bitvector bit_number = 0u;
	std::ostringstream buffer;
	while (bits != 0u) {
		auto bit = bits & 1u;
		if (bit) {
			flag = (1u << bit_number);
			try {
				buffer << NAME_BY_ITEM<T>(static_cast<T>(flag));
			} catch (...) {
				err_log("value '%dl' is incorrcect constant in this context.", flag);
			}
		}
		bits >>= 1;
		if (bit && bits != 0u) {
			buffer << "|";
		}
		++bit_number;
	}
	return buffer.str();
}

template<typename T>
std::string ConstantsSetToString(const std::unordered_set<T> &constants) {
	if (constants.empty()) {
		try {
			return NAME_BY_ITEM<T>(static_cast<T>(0u));
		} catch (...) {
			return "None";
		}
	}

	std::ostringstream buffer;
	for (const auto constant: constants) {
		try {
			buffer << NAME_BY_ITEM<T>(static_cast<T>(constant)) << "|";
		} catch (...) {
			err_log("value '%ud' is incorrcect constant in this context.", to_underlying(constant));
		}
	}
	auto out = buffer.str();
	out.pop_back();
	return out;
}

} // namespace Parse

#endif // PARSE_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
