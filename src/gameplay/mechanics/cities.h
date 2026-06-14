/**
\file cities.h - a part of the Bylins engine.
\authors Created by Sventovit, reworked for issue.cities.
\brief City registry: a vnum-keyed InfoContainer of in-game cities.
\details Each city has an integer vnum (identity/key), a string text id, a rent room vnum, and a
		 list of zones (inner + suburb). Display names live in cfg/messages/ru/cities_msg.xml
		 (MUD::CityMessages()), looked up by the city vnum at build time. XML source is
		 cfg/mechanics/cities.xml, loaded through cfg_manager and exposed via MUD::Cities().
*/

#ifndef BYLINS_SRC_GAME_MECHANICS_CITIES_H_
#define BYLINS_SRC_GAME_MECHANICS_CITIES_H_

#include "engine/boot/cfg_manager.h"
#include "engine/structs/info_container.h"
#include "engine/structs/structs.h"

#include <string>
#include <unordered_set>
#include <vector>

class CharData;

namespace cities {

// One zone belonging to a city. suburb=false => the inner city; suburb=true => an outer/suburb zone.
// More attributes may be added later (the reason this is a struct, not a bare vnum).
struct CityZone {
	int vnum{0};
	bool suburb{false};
};

class CityInfo : public info_container::BaseItem<int> {
	friend class CityInfoBuilder;

	std::string name_{"undefined"};
	std::unordered_set<int> rent_vnums_;   // a city may have several rent rooms
	std::vector<CityZone> zones_;

 public:
	CityInfo() = default;
	CityInfo(int id, std::string &text_id, std::string &name, EItemMode mode)
		: BaseItem<int>(id, text_id, mode), name_{name} {};

	[[nodiscard]] const std::string &GetName() const { return name_; }
	[[nodiscard]] const std::unordered_set<int> &GetRentVnums() const { return rent_vnums_; }
	[[nodiscard]] bool HasRentVnum(int vnum) const { return rent_vnums_.count(vnum) > 0; }
	[[nodiscard]] const std::vector<CityZone> &GetZones() const { return zones_; }
	[[nodiscard]] bool HasZone(int zone_vnum) const {
		for (const auto &z : zones_) {
			if (z.vnum == zone_vnum) {
				return true;
			}
		}
		return false;
	}
};

class CityInfoBuilder : public info_container::IItemBuilder<CityInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemPtr ParseCity(parser_wrapper::DataNode node);
};

using CitiesInfo = info_container::InfoContainer<int, CityInfo, CityInfoBuilder>;

class CitiesLoader : virtual public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
	[[nodiscard]] std::string EditableWhat() const final;
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] parser_wrapper::DataNode FindElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
	[[nodiscard]] parser_wrapper::DataNode CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const final;
};

// Marks the city (by its text id) as visited when ch enters that city's rent room.
void CheckCityVisit(CharData *ch, RoomRnum room_rnum);
// True if ch's current zone is one of any city's zones (inner or suburb).
bool IsCharInCity(CharData *ch);
// The "goroda" (cities) command: lists cities and whether the player has visited each.
void DoCities(CharData *ch, char *, int, int);

} // namespace cities

#endif //BYLINS_SRC_GAME_MECHANICS_CITIES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
