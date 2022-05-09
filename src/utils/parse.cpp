// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "parse.h"

#include "obj_prototypes.h"
#include "utils/utils.h"

namespace text_id {

///
/// Содержит списки соответствия номер=строка/строка=номер для конверта
/// внутри-игровых констант в строковые ИД и обратно.
/// Нужно для независимости файлов от изменения номеров констант в коде.
///
class TextIdNode {
 public:
	void Add(int num, std::string str);

	std::string ToStr(int num) const;
	int ToNum(const std::string &str) const;

 private:
	std::unordered_map<int, std::string> num_to_str;
	std::unordered_map<std::string, int> str_to_num;
};

// общий список конвертируемых констант
std::array<TextIdNode, kTextIdCount> text_id_list;

///
/// Инит текстовых ИД параметров предметов для сохранения в файл.
///
void InitObjVals() {
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::POTION_SPELL1_NUM), "POTION_SPELL1_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::POTION_SPELL1_LVL), "POTION_SPELL1_LVL");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::POTION_SPELL2_NUM), "POTION_SPELL2_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::POTION_SPELL2_LVL), "POTION_SPELL2_LVL");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::POTION_SPELL3_NUM), "POTION_SPELL3_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::POTION_SPELL3_LVL), "POTION_SPELL3_LVL");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::POTION_PROTO_VNUM), "POTION_PROTO_VNUM");
}

///
/// Общий инит системы текстовых ИД, дергается при старте мада.
///
void Init() {
	/// OBJ_VALS
	InitObjVals();
	/// ...
}

///
/// Конвертирование текстового ИД константы в ее значение в коде.
/// \return значение константы или -1, если ничего не было найдено
///
int ToNum(EIdType type, const std::string &str) {
	if (type < kTextIdCount) {
		return text_id_list.at(type).ToNum(str);
	}
	return -1;
}

///
/// Конвертирование значения константы в ее текстовый ИД.
/// \return текстовый ИД константы или пустая строка, если ничего не было найдено
///
std::string ToStr(EIdType type, int num) {
	if (type < kTextIdCount) {
		return text_id_list.at(type).ToStr(num);
	}
	return "";
}

///
/// Добавление соответствия значение=константа/константа=значение
///
void TextIdNode::Add(int num, std::string str) {
	num_to_str.insert(std::make_pair(num, str));
	str_to_num.insert(std::make_pair(str, num));
}

///
/// Конвертирование значение -> текстовый ИД
///
std::string TextIdNode::ToStr(int num) const {
	auto i = num_to_str.find(num);
	if (i != num_to_str.end()) {
		return i->second;
	}
	return "";
}

///
/// Конвертирование текстовый ИД -> значение
///
int TextIdNode::ToNum(const std::string &str) const {
	auto i = str_to_num.find(str);
	if (i != str_to_num.end()) {
		return i->second;
	}
	return -1;
}

} // namespace text_id

namespace parse {

///
/// Попытка конвертирования \param text в <int> с перехватом исключения
/// \return число или -1, в случае неудачи
///
int CastToInt(const char *text) {
	int result = -1;

	try {
		result = std::stoi(text, nullptr, 10);
	} catch (...) {
		snprintf(buf, kMaxStringLength, "...lexical_cast<int> fail (value='%s')", text);
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
	}

	return result;
}

///
/// Обертка на pugixml для чтения числового аттрибута
/// с логирование в имм- и сислог
/// В конфиге это выглядит как <param value="1234" />
/// \return -1 в случае неудачи
int ReadAttrAsInt(const pugi::xml_node &node, const char *text) {
	pugi::xml_attribute attr = node.attribute(text);
	if (!attr) {
		snprintf(buf, kMaxStringLength, "...%s read fail", text);
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
	}
	return CastToInt(attr.value());
}

// тоже самое, что и attr_int, только, если элемента нет, возвращает -1
int ReadAttrAsIntT(const pugi::xml_node &node, const char *text) {
	pugi::xml_attribute attr = node.attribute(text);
	if (!attr) {
		return -1;
	}
	return CastToInt(attr.value());
}

///
/// Тоже, что и ReadAttrAsInt, только для чтения child_value()
/// В конфиге это выглядит как <param>1234<param>
///
int ReadChildValueAsInt(const pugi::xml_node &node, const char *text) {
	pugi::xml_node child_node = node.child(text);
	if (!child_node) {
		snprintf(buf, kMaxStringLength, "...%s read fail", text);
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
	}
	return CastToInt(child_node.child_value());
}

///
/// Аналог ReadAttrAsInt, \return строку со значением или пустую строку
///
std::string ReadAattrAsStr(const pugi::xml_node &node, const char *text) {
	pugi::xml_attribute attr = node.attribute(text);
	if (!attr) {
		snprintf(buf, kMaxStringLength, "...%s read fail", text);
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
	} else {
		return attr.value();
	}
	return "";
}

///
/// Аналог ReadChildValueAsInt, \return строку со значением или пустую строку
///
std::string ReadChildValueAsStr(const pugi::xml_node &node, const char *text) {
	pugi::xml_node child_node = node.child(text);
	if (!child_node) {
		snprintf(buf, kMaxStringLength, "...%s read fail", text);
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
	} else {
		return child_node.child_value();
	}
	return "";
}

template<class T>
pugi::xml_node GetChildTemplate(const T &node, const char *name) {
	pugi::xml_node tmp_node = node.child(name);
	if (!tmp_node) {
		char tmp[100];
		snprintf(tmp, sizeof(tmp), "...<%s> read fail", name);
		mudlog(tmp, CMP, kLvlImmortal, SYSLOG, true);
	}
	return tmp_node;
}

pugi::xml_node GetChild(const pugi::xml_document &node, const char *name) {
	return GetChildTemplate(node, name);
}

pugi::xml_node GetChild(const pugi::xml_node &node, const char *name) {
	return GetChildTemplate(node, name);
}

///
/// проверка валидности внума объекта с логированием ошибки в имм и сислог
/// \return true - если есть прототип объекта (рнум) с данным внумом
///
bool IsValidObjVnum(int vnum) {
	if (real_object(vnum) < 0) {
		snprintf(buf, sizeof(buf), "...bad obj vnum (%d)", vnum);
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return false;
	}

	return true;
}

// =====================================================================================================================

/**
 * Прочитать значение value как строку.
 * Ecxeption: если строка пуста, сообщение "string is empty";
 */
const char *ReadAsStr(const char *value) {
	if (strcmp(value, "") == 0) {
		throw std::runtime_error("string is empty");
	}
	return value;
}

/**
 * Прочитать значение value как int.
 * Ecxeption: при неудаче, сообщение - содержимое value.
 */
int ReadAsInt(const char *value) {
	try {
		return std::stoi(value, nullptr);
	} catch (std::exception &) {
		throw std::runtime_error(value);
	}
}

/**
 * Прочитать значение value как float.
 * Ecxeption: при неудаче, сообщение - содержимое value.
 */
float ReadAsFloat(const char *value) {
	try {
		return std::stof(value, nullptr);
	} catch (std::exception &) {
		throw std::runtime_error(value);
	}
}

/**
 * Прочитать значение value как double.
 * Ecxeption: при неудаче, сообщение - содержимое value.
 */
double ReadAsDouble(const char *value) {
	try {
		return std::stod(value, nullptr);
	} catch (std::exception &) {
		throw std::runtime_error(value);
	}
}

/**
 * Прочитать значение value как bool.
 * Возвращает true, если значение "1", "true", "T", "t", "Y" или "y" и false в ином случае.
 * Ecxeption: если value пусто, сообщение - "value is empty".
 */
bool ReadAsBool(const char *value) {
	if (strcmp(value, "") == 0) {
		throw std::runtime_error("value is empty");
	}
	return (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 || strcmp(value, "t") == 0
				|| strcmp(value, "T") == 0 || strcmp(value, "y") == 0 || strcmp(value, "Y") == 0);
}

} // namespace parse

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
