// description.h
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#ifndef _DESCRIPTION_H_INCLUDED
#define _DESCRIPTION_H_INCLUDED

#include "conf.h"
#include <string>
#include <vector>
#include <map>

/**
* глобальный класс-контейнер уникальных описаний комнат, тупо экономит 70+мб памяти.
* действия: грузим описания комнат, попутно отсекая дубликаты. в комнату пишется
* не само описание, а его порядковый номер в глобальном массиве этих описаний.
* если у вас возникнет мысль делать тоже самое построчно, то не мучайтесь,
* т.к. вы ничего на этом не сэкономите, по крайней мере с текущим форматом зон.
* при редактировании в олц старые описания остаются в массиве, т.к. это все херня

* \todo В последствии нужно переместить в class room, а потом вообще убрать из кода,
* т.к. есть куда более прикольная тема с шаблонами в файлах зон.
*/
class RoomDescription
{
public:
	static size_t add_desc(const std::string &text);
	static const std::string& show_desc(size_t desc_num);

private:
	RoomDescription();
	~RoomDescription();
	// отсюда дергаем описания при работе мада
	static std::vector<std::string> _desc_list;
	// а это чтобы мад не грузился пол часа. из-за оптимизации копирования строк мап
	// проще оставлять на все время работы мада для олц, а мож и дальнейшего релоада описаний
	typedef std::map<std::string, size_t> reboot_map_t;
	static reboot_map_t _reboot_map;
};

#endif // _DESCRIPTION_H_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
