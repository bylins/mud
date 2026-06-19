//
// Created by Sventovit on 04.09.2024.
// issue.npc-races: ENpcRace moved here from entities_constants.h; mob-race data migrated to an
// info_container with per-race flags (EMobFlag/ENpcFlag) and boolean traits.
//

#ifndef BYLINS_SRC_GAME_MECHANICS_MOB_RACES_H_
#define BYLINS_SRC_GAME_MECHANICS_MOB_RACES_H_

#include "engine/structs/structs.h"
#include "engine/structs/meta_enum.h"

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

class CharData;

// --- ENpcRace --------------------------------------------------------------------------------
// Defined BEFORE the heavy info_container/cfg_manager includes below: those transitively re-enter
// entities_constants.h / char_data.h, which use ENpcRace, so the enum must already be visible.
/**
 * NPC races. Values are wire-stable (stored as integers in mob world data); do not renumber.
 */
enum ENpcRace : int {
	kBasic = 100,
	kHuman = 101,
	kBeastman = 102,
	kBird = 103,
	kAnimal = 104,
	kReptile = 105,
	kFish = 106,
	kInsect = 107,
	kPlant = 108,
	kConstruct = 109,
	kZombie = 110,
	kGhost = 111,
	kBoggart = 112,
	kSpirit = 113,
	kMagicCreature = 114,
	kLastNpcRace = kMagicCreature // Don't forget to update when adding new races.
};

/**
 * Virtual NPC races.
 */
const int kNpcBoss = 200;
const int kNpcUnique = 201;

template<>
ENpcRace ITEM_BY_NAME<ENpcRace>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<ENpcRace>(ENpcRace item);
template<>
const std::map<ENpcRace, std::string> &NAMES_OF<ENpcRace>();

// --- mob-race data container -----------------------------------------------------------------
#include "engine/structs/flag_data.h"
#include "engine/structs/info_container.h"
#include "engine/boot/cfg_manager.h"

namespace mob_races {

// One ingredient a mob of this race can drop, with a per-level drop probability table.
struct ingredient {
	int imtype{0};
	std::string imname;
	std::array<int, kMaxMobLevel + 1> prob{}; // drop probability for each mob level
};

// One race's description: drop list + flags/traits applied to a loaded mob of this race.
// int-keyed (vnum == the ENpcRace integer value); the XML id is the ENpcRace token (e.g. kHuman),
// resolved to the integer key at load and kept as the element's text id.
class MobRace : public info_container::BaseItem<int> {
	friend class MobRaceBuilder;

	std::string race_name_{"!undefined!"};
	std::vector<ingredient> ingrlist_;
	FlagData mob_flags_;	// EMobFlag set OR'd onto a freshly loaded instance (issue.npc-races item 8)
	FlagData npc_flags_;	// ENpcFlag set OR'd onto a freshly loaded instance (the <npc_flags> tag)
	bool vocal_{false};			// race can speak/incant (replaces the old per-race speech switch)
	bool respiration_{false};	// race breathes (drives the can-breathe / strangle check)
	bool skinnable_{false};		// corpses of this race can be skinned

 public:
	MobRace() = default;
	MobRace(int id, std::string &text_id, std::string &name, EItemMode mode)
		: BaseItem<int>(id, text_id, mode), race_name_{name} {};

	[[nodiscard]] const std::string &GetName() const { return race_name_; }
	[[nodiscard]] const std::vector<ingredient> &GetIngredients() const { return ingrlist_; }
	[[nodiscard]] const FlagData &GetMobFlags() const { return mob_flags_; }
	[[nodiscard]] const FlagData &GetNpcFlags() const { return npc_flags_; }
	[[nodiscard]] bool IsVocal() const { return vocal_; }
	[[nodiscard]] bool CanBreathe() const { return respiration_; }
	[[nodiscard]] bool IsSkinnable() const { return skinnable_; }
};

class MobRaceBuilder : public info_container::IItemBuilder<MobRace> {
 public:
	ItemPtr Build(parser_wrapper::DataNode &node) final;
 private:
	static ItemPtr ParseRace(parser_wrapper::DataNode node);
};

using MobRacesInfo = info_container::InfoContainer<int, MobRace, MobRaceBuilder>;

// issue.npc-races: editable cfg loader (Vedun + hot reload). Registered in cfg_manager as "mob_races".
class MobRacesLoader : public cfg_manager::IEditableCfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;

	[[nodiscard]] std::string EditableWhat() const final { return "mobrace"; }
	[[nodiscard]] std::vector<cfg_manager::EditableElement> ListElements() const final;
	[[nodiscard]] cfg_manager::ValidationResult Validate(parser_wrapper::DataNode &doc) const final;
	[[nodiscard]] std::string CanonicalElementId(const std::string &id) const final;
};

} // namespace mob_races

// issue.npc-races: true if `ch` breathes -- players always do; an NPC does iff its race carries the
// <respiration/> trait. Drives the strangle "can-breathe" gate (replaces the old per-race switch).
bool CanBreathe(CharData *ch);

#endif //BYLINS_SRC_GAME_MECHANICS_MOB_RACES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
