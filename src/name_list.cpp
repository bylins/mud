// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "name_list.hpp"

#include "obj.hpp"
#include "chars/character.h"
#include "handler.h"
#include "interpreter.h"
#include "utils.h"

#include <limits>
#include <map>
#include <string>
#include <set>

namespace
{

typedef std::map<int /* serial_num */, CHAR_DATA *> CharNodeListType;
typedef std::map<std::string /* имя */, CharNodeListType> CharListType;
CharListType char_list;

typedef std::map<int /* serial_num */, OBJ_DATA *> ObjNodeListType;
typedef std::map<std::string /* имя */, ObjNodeListType> ObjListType;
ObjListType obj_list;

// счетчик чаров/предметов для name_list
int char_serial_num = 0;
int obj_serial_num = 0;

} // namespace

////////////////////////////////////////////////////////////////////////////////

namespace CharacterAlias
{

// * См ObjectAlias::add()
void add(CHAR_DATA *ch)
{
	if (!GET_NAME(ch)) return;

	ch->set_serial_num(++char_serial_num);
	std::string name(GET_NAME(ch)), word;
	lower_convert(name);

	while (!name.empty())
	{
		cut_one_word(name, word);
		if (word.empty())
		{
			return;
		}
		CharListType::iterator it = char_list.find(word);
		if (it != char_list.end())
		{
			it->second.insert(std::make_pair(ch->get_serial_num(), ch));
		}
		else
		{
			CharNodeListType tmp_node;
			tmp_node[ch->get_serial_num()] = ch;
			char_list[word] = tmp_node;
		}
	}
}

// * См ObjectAlias::remove()
void remove(CHAR_DATA *ch)
{
	for (CharListType::iterator it = char_list.begin(); it != char_list.end(); /* empty */)
	{
		CharNodeListType::iterator tmp_it = it->second.find(ch->get_serial_num());
		if (tmp_it != it->second.end())
		{
			it->second.erase(tmp_it);
		}
		// алиасы, оставшиеся пустыми
		if (it->second.empty())
		{
			char_list.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

// * См ObjectAlias::search_by_word()
CHAR_DATA * search_by_word(const char *name, const std::string &search_word)
{
	CHAR_DATA *ch = 0;
	CharListType::iterator i = char_list.lower_bound(search_word);

	while (i != char_list.end())
	{
		if (isname(search_word, i->first))
		{
			for (CharNodeListType::reverse_iterator k = i->second.rbegin(); k != i->second.rend(); ++k)
			{
				if (isname(name, GET_NAME(k->second)))
				{
					if (!ch || (ch && ch->get_serial_num() < k->second->get_serial_num()))
					{
						ch = k->second;
					}
					break;
				}
			}
			++i;
		}
		else
		{
			break;
		}
	}

	return ch;
}

// * См ObjectAlias::get_by_name()
CHAR_DATA * get_by_name(const char *str)
{
	if (!str || !*str)
	{
		return 0;
	}
	std::string name(str), word;
	lower_convert(name);
	cut_one_word(name, word);
	if (word.empty())
	{
		return 0;
	}
	return search_by_word(str, word);
}

} // namespace CharacterAlias

////////////////////////////////////////////////////////////////////////////////

namespace ObjectAlias
{

/**
* Добавление предмета в мап с разделением по каждому из его алиасов
* и сортировкой по порядковому номеру, который тут же и инится.
*/
void add(OBJ_DATA *obj)
{
	if (obj->get_aliases().empty())
	{
		return;
	}

	obj->set_serial_num(++obj_serial_num);
	std::string name(obj->get_aliases());
	std::string word;
	lower_convert(name);

	while (!name.empty())
	{
		cut_one_word(name, word);
		if (word.empty())
		{
			return;
		}

		ObjListType::iterator it = obj_list.find(word);
		if (it != obj_list.end())
		{
			it->second.insert(std::make_pair(obj->get_serial_num(), obj));
		}
		else
		{
			ObjNodeListType tmp_node;
			tmp_node[obj->get_serial_num()] = obj;
			obj_list[word] = tmp_node;
		}
	}
}

// * Удаление предмета из всех полей мапа.
void remove(OBJ_DATA *obj)
{
	for (ObjListType::iterator it = obj_list.begin(); it != obj_list.end(); /* empty */)
	{
		ObjNodeListType::iterator tmp_it = it->second.find(obj->get_serial_num());
		if (tmp_it != it->second.end())
		{
			it->second.erase(tmp_it);
		}
		// алиасы, оставшиеся пустыми
		if (it->second.empty())
		{
			obj_list.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

/**
* Система поиска такая: список предметов сортированы по номеру, поэтому
* всегда достаточно взять последний подходящий по алиасу и он для данного
* поискового слова будет последним добавленным в глобальный список предметом,
* в процессе прохода по словам просто выделяется предмет с наибольшим номером.
*/
OBJ_DATA * search_by_word(const char *name, const std::string &search_word)
{
	OBJ_DATA *obj = 0;
	ObjListType::iterator i = obj_list.lower_bound(search_word);

	while (i != obj_list.end())
	{
		if (isname(search_word, i->first))
		{
			for (ObjNodeListType::reverse_iterator k = i->second.rbegin(); k != i->second.rend(); ++k)
			{
				if (isname(name, k->second->get_aliases()))
				{
					if (!obj || (obj && obj->get_serial_num() < k->second->get_serial_num()))
					{
						obj = k->second;
					}
					break;
				}
			}
			++i;
		}
		else
		{
			break;
		}
	}

	return obj;
}

// * \return последний (технически первый в object_list) предмет или 0 по его алиасу.
OBJ_DATA * get_by_name(const char *str)
{
	if (!str || !*str)
	{
		return 0;
	}
	std::string name(str), word;
	lower_convert(name);
	cut_one_word(name, word);
	if (word.empty())
	{
		return 0;
	}
	return search_by_word(str, word);
}

/**
* Поиск цели для каста локейта, в данном случае нам не важен порядковый номер,
* а просто нужен любой предмет с данным алиасом.
*/
OBJ_DATA * locate_object(const char *str)
{
	if (!str || !*str)
	{
		return 0;
	}

	std::string name(str), word;
	lower_convert(name);
	cut_one_word(name, word);
	if (word.empty())
	{
		return 0;
	}

	ObjListType::iterator i = obj_list.lower_bound(word);
	while (i != obj_list.end())
	{
		if (isname(word, i->first))
		{
			for (ObjNodeListType::reverse_iterator k = i->second.rbegin(); k != i->second.rend(); ++k)
			{
				if (isname(str, k->second->get_aliases()))
				{
					return k->second;
				}
			}
			++i;
		}
		else
		{
			break;
		}
	}
	return 0;
}

} // namespace ObjectAlias

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
