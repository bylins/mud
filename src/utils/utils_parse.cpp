// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "utils_parse.h"

#include "third_party_libs/pugixml/pugixml.h"
#include "utils/parser_wrapper.h"   // issue.xml-parse-cleaning: AttrInt/AttrStr over DataNode

#include "engine/db/obj_prototypes.h"
#include "engine/db/db.h"
#include "utils/utils.h"   // a_isdigit
#include "utils/utils_string.h"   // str_cmp/strn_cmp (search_block)

//extern ObjRnum GetObjRnum(ObjVnum vnum) { return obj_proto.rnum(vnum);

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
	// issue.magic-items: unified magic-item payload. The FIRST string per key is canonical (Add keeps the
	// first num->str, so ToStr writes it); the trailing POTION_*/SPELLITEM_* strings are READ-ALIASES so
	// every existing potion/scroll/wand/staff still loads and is silently re-saved in the new short form.
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kSpell1Num), "SPELL1_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kSpell1Num), "POTION_SPELL1_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kSpell1Num), "SPELLITEM_SPELL1_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kSpell2Num), "SPELL2_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kSpell2Num), "POTION_SPELL2_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kSpell2Num), "SPELLITEM_SPELL2_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kSpell3Num), "SPELL3_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kSpell3Num), "POTION_SPELL3_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kSpell3Num), "SPELLITEM_SPELL3_NUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kMakerSkill), "MAKER_SKILL");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kMakerSkill), "POTION_SKILL");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kMakerSkill), "SPELLITEM_SKILL");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kMakerStat), "MAKER_STAT");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kMakerStat), "POTION_STAT");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kMakerStat), "SPELLITEM_STAT");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kMaxCharges), "MAX_CHARGES");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kMaxCharges), "SPELLITEM_MAX_CHARGES");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kCurCharges), "CUR_CHARGES");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kCurCharges), "SPELLITEM_CUR_CHARGES");
	// potion / drink-specific keys (names unchanged)
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kPotionSpell1Lvl), "POTION_SPELL1_LVL");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kPotionSpell2Lvl), "POTION_SPELL2_LVL");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kPotionSpell3Lvl), "POTION_SPELL3_LVL");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kPotionProtoVnum), "POTION_PROTO_VNUM");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kPotionPotency), "POTION_POTENCY");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kPotionBrewRoll), "POTION_BREW_ROLL");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kLiquidTimer), "LIQUID_TIMER");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kLiquidPoison), "LIQUID_POISON");
	// issue.magic-items-hotfix: drink-container/fountain liquid core keys
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kLiquidCapacity), "LIQUID_CAPACITY");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kLiquidCurrent), "LIQUID_CURRENT");
	text_id_list.at(kObjVals).Add(to_underlying(ObjVal::EValueKey::kLiquidType), "LIQUID_TYPE");
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
	char buf[kMaxStringLength];
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
	if (GetObjRnum(vnum) < 0) {
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
 * Прочитать значение value как массив int, разделенный 'I' и записать знаения в num_set.
 */
void ReadAsIntSet(std::unordered_set<int> &num_set, const char *value) {
	if (strcmp(value, "") == 0) {
		throw std::runtime_error("string is empty");
	}

	for (const auto &str : utils::Split(value, '|')) {
		try {
			num_set.emplace(std::stoi(str, nullptr));
		} catch (...) {
			err_log("value '%s' cannot be converted into num.", str.c_str());
		}
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

// issue.xml-parse-cleaning: единые толерантные читатели атрибутов DataNode (заменяют
// локальные AttrInt/AttrStr, ранее продублированные в загрузчиках). Пустой/отсутствующий/
// неразобранный атрибут -> def.
int AttrInt(const parser_wrapper::DataNode &node, const char *key, int def) {
	const char *v = node.GetValue(key);
	if (!v || !*v) {
		return def;
	}
	try {
		return ReadAsInt(v);
	} catch (const std::exception &) {
		return def;
	}
}

std::string AttrStr(const parser_wrapper::DataNode &node, const char *key, const char *def) {
	const char *v = node.GetValue(key);
	return (v && *v) ? std::string(v) : std::string(def);
}

} // namespace parse


// issue.handler-cleaning: argument index parsing ('2.sword') moved from handler.
int get_number(char **name) {
	int i, res;
	char *ppos;

	if ((ppos = strchr(*name, '.')) != nullptr) {
		for (i = 0; *name + i != ppos; i++) {
			if (!a_isdigit(*(*name + i))) {
				return 1;
			}
		}
		res = atoi(*name);
		memmove(*name, ppos + 1, strlen(ppos));
		return res;
	}
	return 1;
}

int get_number(std::string &name) {
	std::string::size_type pos = name.find('.');

	if (pos != std::string::npos) {
		for (std::string::size_type i = 0; i != pos; i++)
			if (!a_isdigit(name[i]))
				return (1);
		int res = atoi(name.substr(0, pos).c_str());
		name.erase(0, pos + 1);
		return (res);
	}
	return (1);
}


// issue.handler-cleaning: first keyword of a name list (moved from handler).
char *fname(const char *namelist) {
	static char holder[30];
	char *point;

	for (point = holder; a_isalpha(*namelist); namelist++, point++)
		*point = *namelist;

	*point = '\0';

	return (holder);
}

// issue.interpreter-cleaning: generic argument/token parsing helpers moved from interpreter.cpp.
int search_block(const char *target_string, const char **list, int exact) {
	int i;
	size_t l = strlen(target_string);

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++) {
			if (!str_cmp(target_string, *(list + i))) {
				return i;
			}
		}
	} else {
		if (0 == l) {
			l = 1;    // Avoid "" to match the first available string
		}
		for (i = 0; **(list + i) != '\n'; i++) {
			if (!strn_cmp(target_string, *(list + i), l)) {
				return i;
			}
		}
	}

	return -1;
}

int search_block(const std::string &block, const char **list, int exact) {
	int i;
	std::string::size_type l = block.length();

	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++)
			if (!str_cmp(block, *(list + i)))
				return (i);
	} else {
		if (!l)
			l = 1;    // Avoid "" to match the first available string
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strn_cmp(block, *(list + i), l))
				return (i);
	}

	return (-1);
}

void GetOneParam(std::string &in_buffer, std::string &out_buffer) {
	std::string::size_type beg_idx = 0, end_idx = 0;
	beg_idx = in_buffer.find_first_not_of(' ');

	if (beg_idx != std::string::npos) {
		// случай с кавычками
		if (in_buffer[beg_idx] == '\'') {
			if (std::string::npos != (beg_idx = in_buffer.find_first_not_of('\'', beg_idx))) {
				if (std::string::npos == (end_idx = in_buffer.find_first_of('\'', beg_idx))) {
					out_buffer = in_buffer.substr(beg_idx);
					in_buffer.clear();
				} else {
					out_buffer = in_buffer.substr(beg_idx, end_idx - beg_idx);
					in_buffer.erase(0, ++end_idx);
				}
			}
			// случай с одним параметром через пробел
		} else {
			if (std::string::npos != (beg_idx = in_buffer.find_first_not_of(' ', beg_idx))) {
				if (std::string::npos == (end_idx = in_buffer.find_first_of(' ', beg_idx))) {
					out_buffer = in_buffer.substr(beg_idx);
					in_buffer.clear();
				} else {
					out_buffer = in_buffer.substr(beg_idx, end_idx - beg_idx);
					in_buffer.erase(0, end_idx);
				}
			}
		}
		return;
	}

	in_buffer.clear();
	out_buffer.clear();
}

// регистронезависимое сравнение двух строк по длине первой, флаг - для учета длины строк (неравенство)
bool CompareParam(const std::string &buffer, const char *str, bool full) {
	if (!str || !*str || buffer.empty() || (full && buffer.length() != strlen(str))) {
		return false;
	}

	std::string::size_type i;
	for (i = 0; i != buffer.length() && *str; ++i, ++str) {
		if (LOWER(buffer[i]) != LOWER(*str)) {
			return false;
		}
	}

	if (i == buffer.length()) {
		return true;
	} else {
		return false;
	}
}

// тоже самое с обоими аргументами стринг
bool CompareParam(const std::string &buffer, const std::string &buffer2, bool full) {
	if (buffer.empty() || buffer2.empty()
		|| (full && buffer.length() != buffer2.length())) {
		return false;
	}

	std::string::size_type i;
	for (i = 0; i != buffer.length() && i != buffer2.length(); ++i) {
		if (LOWER(buffer[i]) != LOWER(buffer2[i])) {
			return false;
		}
	}

	if (i == buffer.length()) {
		return true;
	} else {
		return false;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
