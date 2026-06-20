// Part of Bylins http://www.mud.ru
// issue.player-races-rework: player races (PC tribes) as a cfg-driven, Vedun-editable InfoContainer.

#ifndef PLAYER_RACES_HPP_INCLUDED
#define PLAYER_RACES_HPP_INCLUDED

#include "engine/boot/cfg_manager.h"
#include "engine/structs/info_container.h"
#include "gameplay/abilities/feats_constants.h"   // EFeat

#include <string>
#include <vector>

namespace player_races {

constexpr int kRaceUndefined = -1;

// One PC race (a tribe / "rod"), keyed by an integer vnum. The display name (four gendered forms) lives
// in pc_race_msg.xml (MUD::RaceMessages()); this item holds only the mechanical data.
class PcRaceInfo : public info_container::BaseItem<int> {
	friend class PcRaceInfoBuilder;

	std::vector<EFeat> features_;            // inborn race feats
	std::vector<std::string> birth_cities_;  // place_of_birth: city text-ids (from cities.xml)

 public:
	PcRaceInfo() = default;
	PcRaceInfo(int id, std::string &text_id, EItemMode mode) : BaseItem<int>(id, text_id, mode) {};

	[[nodiscard]] const std::vector<EFeat> &GetFeatures() const { return features_; }
	[[nodiscard]] bool HasFeature(EFeat feat) const {
		for (const EFeat f : features_) {
			if (f == feat) {
				return true;
			}
		}
		return false;
	}
	// The cities (text-ids) where this race may be born -- one per region it spans.
	[[nodiscard]] const std::vector<std::string> &GetBirthCities() const { return birth_cities_; }
};

class PcRaceInfoBuilder : public info_container::IItemBuilder<PcRaceInfo> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemPtr ParseRace(parser_wrapper::DataNode node);
};

using PcRacesInfo = info_container::InfoContainer<int, PcRaceInfo, PcRaceInfoBuilder>;

class PcRacesLoader : virtual public cfg_manager::IEditableCfgLoader {
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

// ---- public API ----------------------------------------------------------------------------------
// Pure data lookups go directly through the registry:
//   MUD::PcRaces()[vnum].GetFeatures() / .HasFeature(feat) / .GetBirthCities()
//   MUD::PcRaces().IsAvailable(vnum)
//   MUD::RaceMessages().GetMessage(vnum, ch->get_sex())   // the gendered name
// Only the helpers carrying real UI logic live here:
// A "<n) name" menu of the selectable races (numbered from 1).
[[nodiscard]] std::string FormatRacesMenu();
// Parse a 1-based menu choice into a race vnum; kRaceUndefined if it is not a selectable race.
[[nodiscard]] int RaceVnumByMenuChoice(const char *arg);

// ---- start-region selection -----------------------------------------------------------------------
// A character is born in a region (the player's choice); the concrete village inside that region is
// fixed by the race. The selectable regions are derived from the race's birth cities (each city
// resolves to a region via the regions registry), de-duplicated, in declaration order.
//
// The ordered, de-duplicated region vnums this race can be born in.
[[nodiscard]] std::vector<int> StartRegionsForRace(int race_vnum);
// A "<n) name" menu of those regions (numbered from 1), names from RegionMessages.
[[nodiscard]] std::string FormatStartRegionsMenu(int race_vnum);
// Parse a player's choice -- a 1-based number over this race's region list, or a region short name
// (matched against RegionMessages kShortDesc) -- into a region vnum; kRaceUndefined if no match.
[[nodiscard]] int StartRegionByMenuChoice(int race_vnum, const char *arg);
// The birth city (text-id) for this race in the given region, or "" if the race has none there.
[[nodiscard]] std::string StartCityForRaceRegion(int race_vnum, int region_vnum);
// The load-room vnum (the chosen city's rent room) for this race + region, or kNowhere if unresolved.
[[nodiscard]] int StartRoomForRaceRegion(int race_vnum, int region_vnum);

} // namespace player_races

#endif // PLAYER_RACES_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
