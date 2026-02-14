#ifndef __ZONE_TABLE_HPP__
#define __ZONE_TABLE_HPP__

#include "engine/structs/structs.h"

#include <vector>

struct ZoneCategory {
	char *name = nullptr;            // type name //
	int ingr_qty = 0;        // quantity of ingredient types //
	int *ingr_types = nullptr;    // types of ingredients, which are loaded in zones of this type //
};

extern struct ZoneCategory *zone_types;

// zone definition structure. for the 'zone-table'
class ZoneData {
 public:
	ZoneData();

	ZoneData(const ZoneData &) = delete;
	ZoneData(ZoneData &&) noexcept;

	ZoneData &operator=(const ZoneData &) = delete;
	ZoneData &operator=(ZoneData &&) noexcept;

	~ZoneData();

	std::string name;        // название зоны
	std::string comment;
	std::string author;
	int traffic;
	int level;    // level of this zone (is used in ingredient loading)
	int type;    // the surface type of this zone (is used in ingredient loading)
	std::string first_enter;
	int lifespan;        // how long between resets (minutes)
	int age;        // current age of this zone (minutes)
	time_t time_awake;
	RoomVnum top;        // upper limit for rooms in this zone
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

	ZoneVnum vnum;    // virtual number of this zone
	ZoneVnum copy_from_zone;
	// Местоположение зоны
	std::string location;
	// Описание зоны
	std::string description;
	struct reset_com *cmd;    // command table for reset

	int typeA_count;
	int *typeA_list;    // список номеров зон, которые сбрасываются одновременно с этой
	int typeB_count;
	int *
		typeB_list;    // список номеров зон, которые сбрасываются независимо, но они должны быть сброшены до сброса зон типа А
	bool *typeB_flag;    // флаги, были ли сброшены зоны типа В
	int under_construction;    // зона в процессе тестирования
	bool locked;
	bool reset_idle;    // очищать ли зону, в которой никто не бывал
	bool used;        // был ли кто-то в зоне после очистки
	unsigned long long activity;    // параметр активности игроков в зоне
	// <= 1 - обычная зона (соло), >= 2 - зона для группы из указанного кол-ва человек
	int group;
	// средний уровень мобов в зоне
	int mob_level;
	// является ли зона городом
	bool is_town;
	// показывает количество репопов зоны, при условии, что в зону ходят
	int count_reset;
	RoomRnum entrance;
	std::pair<TrgRnum, TrgRnum> RnumTrigsLocation;
	std::pair<RoomRnum, RoomRnum> RnumRoomsLocation;
	std::pair<MobRnum, MobRnum> RnumMobsLocation;
};

using ZoneTable = std::vector<ZoneData>;
extern ZoneTable &zone_table;

ZoneVnum GetZoneVnumByCharPlace(CharData *ch);

#endif // __ZONE_TABLE_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

