// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#ifndef CORPSE_HPP_INCLUDED
#define CORPSE_HPP_INCLUDED

#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"

void make_arena_corpse(CharData *ch, CharData *killer);
ObjData *make_corpse(CharData *ch, CharData *killer = nullptr);

namespace GlobalDrop {
void init();
void save();
bool check_mob(ObjData *corpse, CharData *ch);
void reload_tables();

// период сохранения временного файла с мобами (в минутах)
const int SAVE_PERIOD = 10;
// для генерации книги улучшения умения
const ObjVnum kSkillUpgradeBookVnum = 1920;
// для генерации энчантов
const ObjVnum WARR1_ENCHANT_VNUM = 1921;
const ObjVnum WARR2_ENCHANT_VNUM = 1922;
const ObjVnum WARR3_ENCHANT_VNUM = 1923;

//для генерации знаков зачарования
const ObjVnum MAGIC1_ENCHANT_VNUM = 1930;
const ObjVnum MAGIC2_ENCHANT_VNUM = 1931;
const ObjVnum MAGIC3_ENCHANT_VNUM = 1932;

class table_drop {
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
