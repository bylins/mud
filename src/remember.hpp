// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#ifndef REMEMBER_HPP_INCLUDED
#define REMEMBER_HPP_INCLUDED

#include <list>
#include <string>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

namespace Remember
{

enum { ALL, PERSONAL, GROUP, PRAY };

} // namespace Remember

class CharRemember
{
public:
	CharRemember() : answer_id_(NOBODY), num_str_(15) {};
	void add_str(std::string text, int flag);
	std::string get_text(int flag) const;
	void set_answer_id(int id);
	int get_answer_id() const;
	void reset();
private:
	typedef std::list<std::string> RememberListType;
	RememberListType personal_; // теллы
	RememberListType group_; // группа
	RememberListType pray_; // воззвания (личные)
	long answer_id_; // id последнего телявшего (для ответа)
	unsigned int num_str_; // кол-во выводимых строк (режим вспомнить)
	std::string last_tell_; // последняя введенная строка (от спама)
};

#endif // REMEMBER_HPP_INCLUDED
