// title.hpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#ifndef TITLE_HPP_INCLUDED
#define TITLE_HPP_INCLUDED

class CHAR_DATA;

// * Система титулов: команда титул, ведение списка на одобрение, сохранение, лоад.
namespace TitleSystem
{

void do_title(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
bool show_title_list(CHAR_DATA* ch);
void load_title_list();
void save_title_list();

} // namespace TitleSystem

#endif // TITLE_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
