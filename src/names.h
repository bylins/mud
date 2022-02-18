// names.hpp
// Copyright (c) 2018 Bylins
// Part of Bylins http://www.bylins.su
#ifndef NAMES_HPP_INCLUDED
#define NAMES_HPP_INCLUDED

class CharData;

// Система одобрения имён
namespace NewNames {
enum {
	AUTO_ALLOW = 0,
	AUTO_BAN = 1,
	NO_DECISION = 2
};
void add(CharData *ch);
void remove(CharData *ch);
void remove(const std::string &name, CharData *actor);
void load();
bool show(CharData *actor);
int auto_authorize(DescriptorData *d);
} // namespace NewNames

void do_name(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

#endif // NAMES_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
