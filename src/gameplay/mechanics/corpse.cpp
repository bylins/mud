// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include "corpse.h"

#include "engine/db/world_objects.h"
#include "engine/db/obj_prototypes.h"
#include "engine/entities/char_data.h"
#include "engine/entities/zone.h"
#include "engine/core/handler.h"
#include "third_party_libs/pugixml/pugixml.h"
#include "gameplay/clans/house.h"
#include "utils/parse.h"
#include "utils/random.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/sets_drop.h"

#ifdef HAVE_YAML
#include "gameplay/mechanics/loot_tables/loot_tables.h"
#endif

// see http://stackoverflow.com/questions/20145488/cygwin-g-stdstoi-error-stoi-is-not-a-member-of-std
#if defined __CYGWIN__
#include <cstdlib>
#define stoi(x) strtol(x.c_str(),0,10)
#endif

int get_virtual_race(CharData *mob);

extern int max_npc_corpse_time, max_pc_corpse_time;
extern void obj_to_corpse(ObjData *corpse, CharData *ch, int rnum, bool setload);
extern ObjData::shared_ptr CreateCurrencyObj(long quantity);

namespace GlobalDrop {
std::vector<table_drop> tables_drop;
typedef std::map<int /* vnum */, int /* rnum*/> OlistType;

struct global_drop {
	global_drop()
		: vnum(0),
		  mob_lvl(0),
		  max_mob_lvl(0),
		  count_mob(0),
		  mobs(0),
		  rnum(-1),
		  day_start(-1),
		  day_end(-1),
		  race_mob(-1),
		  chance(-1) {};
	int vnum; // О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ - О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
	int mob_lvl;  // О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫
	int max_mob_lvl; // О©╫О©╫О©╫О©╫. О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ (0 - О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫)
	int count_mob;  // О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫
	int mobs; // О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
	int rnum; // О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫ vnum О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	int day_start; // О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ (О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫) О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫ О©╫ ...
	int day_end; // ... О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫
	int race_mob; // О©╫О©╫О©╫ О©╫О©╫О©╫О©╫, О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ (-1 О©╫О©╫О©╫)
	int chance; // О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ (1..1000)
	// О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ (О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫)
	// О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫
	OlistType olist;
};

// О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
struct global_drop_obj {
	global_drop_obj() : vnum(0), chance(0), day_start(0), day_end(0) {};
	// vnum О©╫О©╫О©╫О©╫О©╫О©╫
	int vnum;
	// drop_chance О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ 0 О©╫О©╫ 1000
	int chance;
	// О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫, О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
	std::list<int> sects;
	int day_start;
	int day_end;
};

table_drop::table_drop(std::vector<int> mbs, int chance_, int count_mobs_, int vnum_obj_) {
	this->mobs = mbs;
	this->chance = chance_;
	this->count_mobs = count_mobs_;
	this->vnum_obj = vnum_obj_;
	this->reload_table();
}
void table_drop::reload_table() {
	this->drop_mobs.clear();
	for (int i = 0; i < this->count_mobs; i++) {
		const int mob_number = number(0, static_cast<int>(this->mobs.size() - 1));
		this->drop_mobs.push_back(this->mobs[mob_number]);
	}
}
// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ true, О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫
bool table_drop::check_mob(int vnum) {
	for (size_t i = 0; i < this->drop_mobs.size(); i++) {
		if (this->drop_mobs[i] == vnum) {
			if (number(0, 1000) < this->chance)
				return true;
		}
	}
	return false;
}
int table_drop::get_vnum() {
	return this->vnum_obj;
}

void reload_tables() {
	for (size_t i = 0; i < tables_drop.size(); i++) {
		tables_drop[i].reload_table();
	}
}

typedef std::vector<global_drop> DropListType;
DropListType drop_list;

std::vector<global_drop_obj> drop_list_obj;

const char *CONFIG_FILE = LIB_MISC"global_drop.xml";
const char *STAT_FILE = LIB_PLRSTUFF"global_drop.tmp";

void init() {
	// О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	drop_list.clear();
	drop_list_obj.clear();
	// О©╫О©╫О©╫О©╫О©╫О©╫
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	pugi::xml_node node_list = doc.child("globaldrop");
	if (!node_list) {
		snprintf(buf, kMaxStringLength, "...<globaldrop> read fail");
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	for (pugi::xml_node node = node_list.child("tdrop"); node; node = node.next_sibling("tdrop")) {
		int chance = parse::ReadAttrAsInt(node, "drop_chance");
		int count_mobs = parse::ReadAttrAsInt(node, "count_mobs");
		int vnum_obj = parse::ReadAttrAsInt(node, "ObjVnum");
		std::vector<int> list_mobs;
		for (pugi::xml_node node_ = node.child("mobs"); node_; node_ = node_.next_sibling("mobs")) {
			list_mobs.push_back(parse::ReadAttrAsInt(node_, "vnum"));
		}
		table_drop tmp(list_mobs, chance, count_mobs, vnum_obj);
		tables_drop.push_back(tmp);
	}
	for (pugi::xml_node node = node_list.child("freedrop_obj"); node; node = node.next_sibling("freedrop_obj")) {
		global_drop_obj tmp;
		int obj_vnum = parse::ReadAttrAsInt(node, "ObjVnum");
		int chance = parse::ReadAttrAsInt(node, "drop_chance");
		int day_start = parse::ReadAttrAsIntT(node, "day_start"); // О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ -1
		int day_end = parse::ReadAttrAsIntT(node, "day_end");
		if (day_start == -1) {
			day_end = 360;
			day_start = 0;
		}
		std::string tmp_str = parse::ReadAattrAsStr(node, "sects");
		std::vector<std::string> strs;
		strs = utils::Split(tmp_str);
		for (const auto &i : strs) {
			tmp.sects.push_back(std::stoi(i));
		}
		tmp.vnum = obj_vnum;
		tmp.chance = chance;
		tmp.day_start = day_start;
		tmp.day_end = day_end;
		drop_list_obj.push_back(tmp);
	}
	for (pugi::xml_node node = node_list.child("drop"); node; node = node.next_sibling("drop")) {
		int obj_vnum = parse::ReadAttrAsInt(node, "ObjVnum");
		int mob_lvl = parse::ReadAttrAsInt(node, "mob_lvl");
		int max_mob_lvl = parse::ReadAttrAsInt(node, "max_mob_lvl");
		int count_mob = parse::ReadAttrAsInt(node, "count_mob");
		int day_start = parse::ReadAttrAsIntT(node, "day_start"); // О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ -1
		int day_end = parse::ReadAttrAsIntT(node, "day_end");
		int race_mob = parse::ReadAttrAsIntT(node, "race_mob");
		int chance = parse::ReadAttrAsIntT(node, "drop_chance");
		if (chance == -1)
			chance = 1000;
		if (day_start == -1)
			day_start = 0;
		if (day_end == -1)
			day_end = 360;
		if (race_mob == -1)
			race_mob = -1; // -1 О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫

		if (obj_vnum == -1 || mob_lvl <= 0 || count_mob <= 0 || max_mob_lvl < 0) {
			snprintf(buf, kMaxStringLength,
					 "...bad drop attributes (ObjVnum=%d, mob_lvl=%d, drop_chance=%d, max_mob_lvl=%d)",
					 obj_vnum, mob_lvl, count_mob, max_mob_lvl);
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			return;
		}
		snprintf(buf,
				 kMaxStringLength,
				 "GLOBALDROP: (ObjVnum=%d, mob_lvl=%d, count_mob=%d, max_mob_lvl=%d, day_start=%d, day_end=%d, race_mob=%d, drop_chance=%d)",
				 obj_vnum,
				 mob_lvl,
				 count_mob,
				 max_mob_lvl,
				 day_start,
				 day_end,
				 race_mob,
				 chance);
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		global_drop tmp_node;
		tmp_node.vnum = obj_vnum;
		tmp_node.mob_lvl = mob_lvl;
		tmp_node.max_mob_lvl = max_mob_lvl;
		tmp_node.count_mob = count_mob;
		tmp_node.day_start = day_start;
		tmp_node.day_end = day_end;
		tmp_node.race_mob = race_mob;
		tmp_node.chance = chance;

		if (obj_vnum >= 0) {
			int obj_rnum = GetObjRnum(obj_vnum);
			if (obj_rnum < 0) {
				snprintf(buf, kMaxStringLength, "...incorrect ObjVnum=%d", obj_vnum);
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				return;
			}
			tmp_node.rnum = obj_rnum;
		} else {
			// О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
			for (pugi::xml_node item = node.child("obj"); item; item = item.next_sibling("obj")) {
				int item_vnum = parse::ReadAttrAsInt(item, "vnum");
				if (item_vnum <= 0) {
					snprintf(buf, kMaxStringLength,
							 "...bad shop attributes (item_vnum=%d)", item_vnum);
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					return;
				}
				// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫
				int item_rnum = GetObjRnum(item_vnum);
				if (item_rnum < 0) {
					snprintf(buf, kMaxStringLength, "...incorrect item_vnum=%d", item_vnum);
					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					return;
				}
				tmp_node.olist[item_vnum] = item_rnum;
			}
			if (tmp_node.olist.empty()) {
				snprintf(buf, kMaxStringLength, "...item list empty (ObjVnum=%d)", obj_vnum);
				mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
				return;
			}
		}
		drop_list.push_back(tmp_node);
	}

	// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
	std::ifstream file(STAT_FILE);
	if (!file.is_open()) {
		log("SYSERROR: О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫: %s", STAT_FILE);
		return;
	}
	int vnum, mobs;
	while (file >> vnum >> mobs) {
		for (DropListType::iterator i = drop_list.begin(); i != drop_list.end(); ++i) {
			if (i->vnum == vnum) {
				i->mobs = mobs;
			}
		}
	}
}

void save() {
	std::ofstream file(STAT_FILE);
	if (!file.is_open()) {
		log("SYSERROR: О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫: %s", STAT_FILE);
		return;
	}
	for (DropListType::iterator i = drop_list.begin(); i != drop_list.end(); ++i) {
		file << i->vnum << " " << i->mobs << "\n";
	}
}

/**
 * О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫.
 * \return rnum
 */
int get_obj_to_drop(DropListType::iterator &i) {
	std::vector<int> tmp_list;
	for (OlistType::iterator k = i->olist.begin(), kend = i->olist.end(); k != kend; ++k) {
		if ((GetObjMIW(k->second) == ObjData::UNLIMITED_GLOBAL_MAXIMUM)
			|| (k->second >= 0
				&& obj_proto.actual_count(k->second) < GetObjMIW(k->second)))
			tmp_list.push_back(k->second);
	}
	if (!tmp_list.empty()) {
		int rnd = number(0, static_cast<int>(tmp_list.size() - 1));
		return tmp_list.at(rnd);
	}
	return -1;
}

/**
 * О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫.
 * О©╫О©╫О©╫О©╫ vnum О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫, О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫.
 */
bool check_mob(ObjData *corpse, CharData *mob) {
	if (mob->IsFlagged(EMobFlag::kMounting))
		return false;
	for (size_t i = 0; i < tables_drop.size(); i++) {
		if (tables_drop[i].check_mob(GET_MOB_VNUM(mob))) {
			int rnum;
			if ((rnum = GetObjRnum(tables_drop[i].get_vnum())) < 0) {
				log("О©╫О©╫О©╫О©╫О©╫О©╫ tdrop. О©╫О©╫О©╫О©╫: %d", tables_drop[i].get_vnum());
				return true;
			}
			act("&GО©╫О©╫О©╫-О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫-О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫.&n", false, mob, 0, 0, kToRoom);
			log("О©╫О©╫О©╫О©╫О©╫О©╫О©╫: О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ %s О©╫ VNUM: %d",
				obj_proto[rnum]->get_short_description().c_str(),
				obj_proto[rnum]->get_vnum());
			obj_to_corpse(corpse, mob, rnum, false);
			return true;
		}
	}
	for (DropListType::iterator i = drop_list.begin(), iend = drop_list.end(); i != iend; ++i) {
		int day = time_info.month * kDaysPerMonth + time_info.day + 1;
		if (GetRealLevel(mob) >= i->mob_lvl
			&& (!i->max_mob_lvl
				|| GetRealLevel(mob) <= i->max_mob_lvl)        // О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫
			&& ((i->race_mob < 0)
				|| (GET_RACE(mob) == i->race_mob)
				|| (get_virtual_race(mob) == i->race_mob))        // О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫
			&& (i->day_start <= day && i->day_end >= day)            // О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
			&& (!NPC_FLAGGED(mob, ENpcFlag::kFreeDrop))  //О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
			&& (!mob->has_master()
				|| mob->get_master()->IsNpc())) // О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫

		{
			++(i->mobs);
			if (i->mobs >= i->count_mob) {
				int obj_rnum = i->vnum > 0 ? i->rnum : get_obj_to_drop(i);
				if (obj_rnum == -1) {
					i->mobs = 0;
					continue;
				}
				if (number(1, 1000) <= i->chance
					&& ((GetObjMIW(obj_rnum) == ObjData::UNLIMITED_GLOBAL_MAXIMUM)
						|| (obj_rnum >= 0
							&& obj_proto.actual_count(obj_rnum) < GetObjMIW(obj_rnum)))) {
					act("&GО©╫О©╫О©╫-О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫-О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫.&n", false, mob, 0, 0, kToRoom);
					sprintf(buf, "О©╫О©╫О©╫О©╫О©╫О©╫О©╫: О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ %s VNUM %d О©╫ О©╫О©╫О©╫О©╫ %s VNUM %d (%d lvl)",
							obj_proto[obj_rnum]->get_short_description().c_str(),
							obj_proto[obj_rnum]->get_vnum(),
							GET_NAME(mob),
							GET_MOB_VNUM(mob), GetRealLevel(mob));
					mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
					obj_to_corpse(corpse, mob, obj_rnum, false);
				}
				i->mobs = 0;
//				return true; О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
			}
		}
	}
	return false;
}

} // namespace GlobalDrop

void make_arena_corpse(CharData *ch, CharData *killer) {
	auto corpse = world_objects.create_blank();
	corpse->set_sex(EGender::kPoly);

	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫О©╫О©╫ %s О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫.", GET_PAD(ch, 1));
	corpse->set_description(buf2);

	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_short_description(buf2);

	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kNom, buf2);
	corpse->set_aliases(buf2);

	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kGen, buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kDat, buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kAcc, buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kIns, buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kPre, buf2);

	corpse->set_type(EObjType::kContainer);
	corpse->set_wear_flag(EWearFlag::kTake);
	corpse->set_extra_flag(EObjFlag::kNodonate);
	corpse->set_extra_flag(EObjFlag::kNosell);
	corpse->set_val(0, 0);    // You can't store stuff in a corpse
	corpse->set_val(2, ch->IsNpc() ? GET_MOB_VNUM(ch) : -1);
	corpse->set_val(3, 1);    // corpse identifier
	corpse->set_weight(GET_WEIGHT(ch));
	corpse->set_rent_off(100000);
	if (ch->IsNpc() && !IS_CHARMICE(ch)) {
		corpse->set_timer(5);
	} else {
		corpse->set_timer(0);
	}
	ExtraDescription::shared_ptr exdesc(new ExtraDescription());
	exdesc->keyword = str_dup(corpse->get_PName(ECase::kNom).c_str());    // О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	if (killer) {
		sprintf(buf, "О©╫О©╫О©╫О©╫%s О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ %s.\r\n", GET_CH_SUF_6(ch), GET_PAD(killer, 4));
	} else {
		sprintf(buf, "О©╫О©╫О©╫О©╫%s О©╫О©╫ О©╫О©╫О©╫О©╫О©╫.\r\n", GET_CH_SUF_4(ch));
	}
	exdesc->description = str_dup(buf);    // О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	exdesc->next = corpse->get_ex_description();
	corpse->set_ex_description(exdesc);
	PlaceObjToRoom(corpse.get(), ch->in_room);
}

ObjData *make_corpse(CharData *ch, CharData *killer) {
	ObjData *obj, *next_obj;
	int i;

	if (ch->IsNpc() && ch->IsFlagged(EMobFlag::kCorpse))
		return nullptr;
	auto corpse = world_objects.create_blank();
	sprintf(buf2, "О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_aliases(buf2);
	corpse->set_sex(EGender::kMale);
	sprintf(buf2, "О©╫О©╫О©╫О©╫ %s О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫.", GET_PAD(ch, 1));
	corpse->set_description(buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_short_description(buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kNom, buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kGen, buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kDat, buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kAcc, buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kIns, buf2);
	sprintf(buf2, "О©╫О©╫О©╫О©╫О©╫ %s", GET_PAD(ch, 1));
	corpse->set_PName(ECase::kPre, buf2);

	corpse->set_type(EObjType::kContainer);
	corpse->set_wear_flag(EWearFlag::kTake);
	corpse->set_extra_flag(EObjFlag::kNodonate);
	corpse->set_extra_flag(EObjFlag::kNosell);
	corpse->set_extra_flag(EObjFlag::kNorent);
	corpse->set_val(0, 0);    // You can't store stuff in a corpse
	corpse->set_val(2, ch->IsNpc() ? GET_MOB_VNUM(ch) : -1);
	corpse->set_val(3, ObjData::CORPSE_INDICATOR);    // corpse identifier
	corpse->set_rent_off(100000);

	if (ch->IsNpc() && !IS_CHARMICE(ch)) {
		corpse->set_timer(max_npc_corpse_time * 2);
		corpse->set_destroyer(max_npc_corpse_time * 2);
	} else {
		corpse->set_destroyer(max_pc_corpse_time * 2);
		corpse->set_timer(max_pc_corpse_time * 2);
	}
	// О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫
	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->get_next_content();
		if (IS_CORPSE(obj)) {
			RemoveObjFromChar(obj);
			PlaceObjToRoom(obj, ch->in_room);
		}
	}
	// transfer character's equipment to the corpse
	for (i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i)) {
			remove_otrigger(GET_EQ(ch, i), ch);
			PlaceObjToInventory(UnequipChar(ch, i, CharEquipFlags()), ch);
		}
	}
	// О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫
	corpse->set_weight(GET_WEIGHT(ch) + ch->GetCarryingWeight());
	// transfer character's inventory to the corpse
	corpse->set_contains(ch->carrying);
	for (obj = corpse->get_contains(); obj != nullptr; obj = obj->get_next_content()) {
		obj->set_in_obj(corpse.get());
	}
	object_list_new_owner(corpse.get(), nullptr);

	// transfer gold
	// following 'if' clause added to fix gold duplication loophole
	if (ch->get_gold() > 0) {
		if (ch->IsNpc()) {
			const auto money = CreateCurrencyObj(ch->get_gold());
			PlaceObjIntoObj(money.get(), corpse.get());
		} else {
			const int amount = ch->get_gold();
			const auto money = CreateCurrencyObj(amount);
			ObjData *purse = nullptr;
			if (amount >= 100) {
				purse = system_obj::create_purse(ch, amount);
				if (purse) {
					PlaceObjIntoObj(money.get(), purse);
					PlaceObjIntoObj(purse, corpse.get());
				}
			}

			if (!purse) {
				PlaceObjIntoObj(money.get(), corpse.get());
			}
		}
		ch->set_gold(0);
	}

	ch->carrying = nullptr;
	IS_CARRYING_N(ch) = 0;
	IS_CARRYING_W(ch) = 0;

	if (ch->IsNpc() && GET_RACE(ch) > ENpcRace::kBasic && !NPC_FLAGGED(ch, ENpcFlag::kNoIngrDrop)
		&& !ROOM_FLAGGED(ch->in_room, ERoomFlag::kHouse)) {
		ObjData *ingr = try_make_ingr(ch, 1000);
		if (ingr) {
			PlaceObjIntoObj(ingr, corpse.get());
		}
	}

#ifdef HAVE_YAML
	// Generate loot from loot tables system
	if (ch->IsNpc() && !IS_CHARMICE(ch) && loot_tables::IsInitialized()) {
		int luck_bonus = 0;
		if (killer && !killer->IsNpc()) {
			// Could add luck calculation based on killer's stats
			luck_bonus = 0;
		}

		auto generated = loot_tables::GenerateMobLoot(ch, killer, luck_bonus);

		// Place generated items into corpse
		for (const auto &[vnum, count] : generated.items) {
			for (int i = 0; i < count; ++i) {
				auto obj = world_objects.create_from_prototype_by_vnum(vnum);
				if (obj) {
					obj->set_vnum_zone_from(GetZoneVnumByCharPlace(ch));
					PlaceObjIntoObj(obj.get(), corpse.get());
				}
			}
		}

		// Place generated currencies into corpse
		for (const auto &[currency_id, amount] : generated.currencies) {
			if (currency_id == loot_tables::currency::kGold && amount > 0) {
				const auto money = CreateCurrencyObj(amount);
				PlaceObjIntoObj(money.get(), corpse.get());
			}
			// Other currencies can be handled here when needed
		}
	}
#endif

	// О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫ О©╫О©╫ О©╫О©╫О©╫О©╫О©╫(О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫ О©╫О©╫) О©╫О©╫ О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫О©╫ О©╫О©╫О©╫О©╫О©╫О©╫О©╫
	if (IS_CHARMICE(ch) && !ch->IsFlagged(EMobFlag::kCorpse)
		&& ((killer && killer->IsFlagged(EPrf::kExecutor)) || (ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena) && !NORENTABLE(ch->get_master())))) {
		if (ch->has_master()) {
			PlaceObjToInventory(corpse.get(), ch->get_master());
		}
		return nullptr;
	} else {
		RoomRnum corpse_room = ch->in_room;

		if (corpse_room == kStrangeRoom
			&& ch->get_was_in_room() != kNowhere) {
			corpse_room = ch->get_was_in_room();
		}
		PlaceObjToRoom(corpse.get(), corpse_room);
		return corpse.get();
	}
}

int get_virtual_race(CharData *mob) {
	if (mob->get_role(static_cast<unsigned>(EMobClass::kBoss))) {
		return kNpcBoss;
	}
	std::map<int, int>::iterator it;
	std::map<int, int> unique_mobs = SetsDrop::get_unique_mob();
	for (it = unique_mobs.begin(); it != unique_mobs.end(); it++) {
		if (GET_MOB_VNUM(mob) == it->first)
			return kNpcUnique;
	}
	return -1;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
