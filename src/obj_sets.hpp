// Copyright (c) 2014 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_SETS_HPP_INCLUDED
#define OBJ_SETS_HPP_INCLUDED

#include "conf.h"
#include <array>
#include <vector>

#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"

/// версия сетовых наборов по принципу навешивания аффектов на чара, а не на
/// конкретные предметы, подменяя их родные статы + как бонус полноценное олц
namespace obj_sets
{

struct idx_node
{
	// индекс сета
	size_t set_idx;
	// предметы на чаре из данного сета
	std::vector<OBJ_DATA *> obj_list;
	// кол-во уже активированных шмоток (для сообщений активации/деактиваций)
	int activated_cnt;
	// показывать или нет сообщения активации
	bool show_msg;
};

/// построение списка предметов с индексами их сетов
/// для сбора сетовых предметов с чара в affect_total()
class WornSets
{
public:
	/// очистка перед каждым использованием
	void clear();
	/// заполнение списка сетов idx_list_ при обходе предметов на чаре
	void add(CHAR_DATA *ch, OBJ_DATA *obj);
	/// проверка активаторов (вся магия здесь)
	void check(CHAR_DATA *ch);

private:
	std::array<idx_node, NUM_WEARS> idx_list_;
};

void load();
void save();
void print_off_msg(CHAR_DATA *ch, OBJ_DATA *obj);
void print_identify(CHAR_DATA *ch, const OBJ_DATA *obj);
void init_xhelp();
std::string get_name(size_t idx);

} // namespace obj_sets

namespace obj_sets_olc
{

void parse_input(CHAR_DATA *ch, const char *arg);

} // namespace obj_sets_olc

ACMD(do_slist);
ACMD(do_sedit);

#endif // OBJ_SETS_HPP_INCLUDED
