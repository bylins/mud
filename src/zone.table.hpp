#ifndef __ZONE_TABLE_HPP__
#define __ZONE_TABLE_HPP__

#include "structs.h"

#include <vector>

// zone definition structure. for the 'zone-table'
class ZoneData
{
public:
	ZoneData();

	ZoneData(const ZoneData&) = delete;
	ZoneData(ZoneData&&) = default;

	ZoneData& operator=(const ZoneData&) = delete;
	ZoneData& operator=(ZoneData&&) = default;

	~ZoneData();

	char *name;		// название зоны
	char *comment;
	char *author;
	int traffic;
	int level;	// level of this zone (is used in ingredient loading)
	int type;	// the surface type of this zone (is used in ingredient loading)

	int lifespan;		// how long between resets (minutes)
	int age;		// current age of this zone (minutes)
	room_vnum top;		// upper limit for rooms in this zone

	/**
	 * Conditions for reset.
	 *
	 * Values:
	 *   0: Don't reset, and don't update age.
	 *   1: Reset if no PC's are located in zone.
	 *   2: Just reset.
	 *   3: Multi reset.
	 */
	int reset_mode;

	zone_vnum number;	// virtual number of this zone
	// Местоположение зоны
	char *location;
	// Описание зоны
	char *description;
	struct reset_com *cmd;	// command table for reset

	int typeA_count;
	int *typeA_list;	// список номеров зон, которые сбрасываются одновременно с этой
	int typeB_count;
	int *typeB_list;	// список номеров зон, которые сбрасываются независимо, но они должны быть сброшены до сброса зон типа А
	bool *typeB_flag;	// флаги, были ли сброшены зоны типа В
	int under_construction;	// зона в процессе тестирования
	bool locked;
	bool reset_idle;	// очищать ли зону, в которой никто не бывал
	bool used;		// был ли кто-то в зоне после очистки
	unsigned long long activity;	// параметр активности игроков в зоне
	// <= 1 - обычная зона (соло), >= 2 - зона для группы из указанного кол-ва человек
	int group;
	// средний уровень мобов в зоне
	int mob_level;
	// является ли зона городом
	bool is_town;
	// показывает количество репопов зоны, при условии, что в зону ходят
	int count_reset;
};

using zone_table_t = std::vector<ZoneData>;
extern zone_table_t& zone_table;

#endif // __ZONE_TABLE_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

