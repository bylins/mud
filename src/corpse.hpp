// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#ifndef CORPSE_HPP_INCLUDED
#define CORPSE_HPP_INCLUDED

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

void make_arena_corpse(CHAR_DATA * ch, CHAR_DATA * killer);
OBJ_DATA *make_corpse(CHAR_DATA * ch, CHAR_DATA * killer = NULL);

namespace GlobalDrop
{
void init();
void save();
bool check_mob(OBJ_DATA *corpse, CHAR_DATA *ch);
void reload_tables();

// период сохранения временного файла с мобами (в минутах)
const int SAVE_PERIOD = 10;
// для генерации книги улучшения умения
const int BOOK_UPGRD_VNUM = 1920;
// для генерации энчантов
const int WARR1_ENCHANT_VNUM = 1921;
const int WARR2_ENCHANT_VNUM = 1922;
const int WARR3_ENCHANT_VNUM = 1923;

//для генерации знаков зачарования
const int MAGIC1_ENCHANT_VNUM = 1930;
const int MAGIC2_ENCHANT_VNUM = 1931;
const int MAGIC3_ENCHANT_VNUM = 1932;

class table_drop
{
private:
	// внумы мобов
	std::vector<int> mobs;
	// шанс выпадения 0 от 1000
	int chance;
	// сколько мобов брать из списка для таблицы дропа
	int count_mobs;
	// список мобов, с которых в данный момент будет падать дроп
	std::vector<int> drop_mobs;
	// предмет, который будет падать с мобов
	int vnum_obj;
public:
	table_drop(std::vector<int> mbs, int chance_, int count_mobs_, int vnum_obj_);
	void reload_table();
	// возвратит true, если моб найден в таблице и прошел шанс
	bool check_mob(int vnum);
	int get_vnum();

};


} // namespace GlobalDrop

#endif // CORPSE_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
