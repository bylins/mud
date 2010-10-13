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

enum { ALL, PERSONAL, CLAN, ALLY, GROUP, GOSSIP, OFFTOP, PRAY, PRAY_PERSONAL };
// кол-во запоминаемых строк в каждом списке
const unsigned int MAX_REMEMBER_NUM = 100;
// кол-во выводимых стсрок по умолчанию
const unsigned int DEF_REMEMBER_NUM = 15;
typedef std::list<std::string> RememberListType;

std::string time_format();
std::string format_gossip(CHAR_DATA *ch, CHAR_DATA *vict, int cmd, const char *argument);

} // namespace Remember

class CharRemember
{
public:
	CharRemember() : num_str_(Remember::DEF_REMEMBER_NUM) {};
	void add_str(std::string text, int flag);
	std::string get_text(int flag) const;
	void reset();
	bool set_num_str(unsigned int num);
	unsigned int get_num_str() const;

private:
	unsigned int num_str_; // кол-во выводимых строк (режим вспомнить)
	Remember::RememberListType all_; // все запоминаемые каналы + воззвания (TODO: теллы в клетку и крики?), включая собственные
	Remember::RememberListType personal_; // теллы
	Remember::RememberListType pray_; // воззвания для мортала
	Remember::RememberListType group_; // added by WorM  групптелы 2010.10.13
};

#endif // REMEMBER_HPP_INCLUDED
