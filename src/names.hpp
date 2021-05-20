// names.hpp
// Copyright (c) 2018 Bylins
// Part of Bylins http://www.bylins.su
#ifndef NAMES_HPP_INCLUDED
#define NAMES_HPP_INCLUDED

class CHAR_DATA;

// Система одобрения имён
namespace NewNames
{
	enum {
			AUTO_ALLOW = 0,
			AUTO_BAN = 1,
			NO_DECISION = 2
	};
	void add(CHAR_DATA * ch);
	void remove(CHAR_DATA * ch);
	void remove(const std::string& name, CHAR_DATA * actor);
	void load();
	bool show(CHAR_DATA * actor);
	int auto_authorize(DESCRIPTOR_DATA * d);
} // namespace NewNames

void do_name(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif // NAMES_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
