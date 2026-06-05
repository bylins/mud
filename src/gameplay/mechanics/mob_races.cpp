//
// Created by Sventovit on 04.09.2024.
// issue.npc-races: ENpcRace name tables + info_container-based mob-race loader.
//

#include "mob_races.h"

#include "engine/db/global_objects.h"
#include "engine/entities/char_data.h"             // CharData::IsNpc
#include "engine/entities/entities_constants.h"   // EMobFlag, ENpcFlag
#include "utils/parse.h"
#include "utils/utils.h"                           // GET_RACE
#include "utils/utils_string.h"                    // utils::Split, str_cmp

// =============================================================================================
//  ENpcRace name table (meta_enum)
// =============================================================================================

typedef std::map<ENpcRace, std::string> ENpcRace_name_by_value_t;
typedef std::map<const std::string, ENpcRace> ENpcRace_value_by_name_t;
ENpcRace_name_by_value_t ENpcRace_name_by_value;
ENpcRace_value_by_name_t ENpcRace_value_by_name;

void init_ENpcRace_ITEM_NAMES() {
	ENpcRace_name_by_value.clear();
	ENpcRace_value_by_name.clear();

	ENpcRace_name_by_value[ENpcRace::kBasic] = "kBasic";
	ENpcRace_name_by_value[ENpcRace::kHuman] = "kHuman";
	ENpcRace_name_by_value[ENpcRace::kBeastman] = "kBeastman";
	ENpcRace_name_by_value[ENpcRace::kBird] = "kBird";
	ENpcRace_name_by_value[ENpcRace::kAnimal] = "kAnimal";
	ENpcRace_name_by_value[ENpcRace::kReptile] = "kReptile";
	ENpcRace_name_by_value[ENpcRace::kFish] = "kFish";
	ENpcRace_name_by_value[ENpcRace::kInsect] = "kInsect";
	ENpcRace_name_by_value[ENpcRace::kPlant] = "kPlant";
	ENpcRace_name_by_value[ENpcRace::kConstruct] = "kConstruct";
	ENpcRace_name_by_value[ENpcRace::kZombie] = "kZombie";
	ENpcRace_name_by_value[ENpcRace::kGhost] = "kGhost";
	ENpcRace_name_by_value[ENpcRace::kBoggart] = "kBoggart";
	ENpcRace_name_by_value[ENpcRace::kSpirit] = "kSpirit";
	ENpcRace_name_by_value[ENpcRace::kMagicCreature] = "kMagicCreature";

	for (const auto &i : ENpcRace_name_by_value) {
		ENpcRace_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ENpcRace>(const ENpcRace item) {
	if (ENpcRace_name_by_value.empty()) {
		init_ENpcRace_ITEM_NAMES();
	}
	return ENpcRace_name_by_value.at(item);
}

template<>
const std::map<ENpcRace, std::string> &NAMES_OF<ENpcRace>() {
	if (ENpcRace_name_by_value.empty()) {
		init_ENpcRace_ITEM_NAMES();
	}
	return ENpcRace_name_by_value;
}

template<>
ENpcRace ITEM_BY_NAME<ENpcRace>(const std::string &name) {
	if (ENpcRace_name_by_value.empty()) {
		init_ENpcRace_ITEM_NAMES();
	}
	return ENpcRace_value_by_name.at(name);
}

namespace mob_races {

using DataNode = parser_wrapper::DataNode;
using ItemPtr = MobRaceBuilder::ItemPtr;

// Set every flag named in a "kA|kB|kC" value string into `flags`, decoding each token via the
// enum's meta_enum table (so multi-plane flags land in the right plane). Unknown tokens are logged
// and skipped. An empty/absent value leaves `flags` untouched.
template<typename FlagEnum>
static void ParseFlagSet(const char *value, FlagData &flags) {
	if (!value || !*value) {
		return;
	}
	for (const auto &token : utils::Split(value, '|')) {
		try {
			flags.set(ITEM_BY_NAME<FlagEnum>(token));
		} catch (const std::exception &) {
			err_log("mob_races: unknown flag constant '%s'.", token.c_str());
		}
	}
}

ItemPtr MobRaceBuilder::Build(DataNode &node) {
	try {
		return ParseRace(node);
	} catch (std::exception &e) {
		err_log("Mob race parsing error (incorrect value '%s').", e.what());
		return nullptr;
	}
}

ItemPtr MobRaceBuilder::ParseRace(DataNode node) {
	// id is the ENpcRace token (e.g. "kHuman"); its integer value is the container key (vnum).
	std::string text_id = parse::ReadAsStr(node.GetValue("id"));
	const int race_num = ITEM_BY_NAME<ENpcRace>(text_id);
	auto mode = MobRaceBuilder::ParseItemMode(node, EItemMode::kEnabled);
	std::string name{"undefined"};
	try {
		name = parse::ReadAsStr(node.GetValue("name"));
	} catch (...) {}

	auto race = std::make_shared<MobRace>(race_num, text_id, name, mode);

	if (node.GoToChild("imlist")) {
		for (auto &im : node.Children("im")) {
			ingredient tmp_ingr;
			tmp_ingr.imtype = parse::ReadAsInt(im.GetValue("type"));
			tmp_ingr.imname = std::string(im.GetValue("name"));
			utils::Trim(tmp_ingr.imname);

			int cur_lvl = 1;
			int prob_value = 1;
			for (auto &prob : im.Children("prob")) {
				const int next_lvl = parse::ReadAsInt(prob.GetValue("lvl"));
				if (next_lvl <= 0) {
					err_log("mob_races: bad level lvl=%d for ingredient '%s' of race '%s'.",
							next_lvl, tmp_ingr.imname.c_str(), name.c_str());
					return nullptr;
				}
				for (int lvl = cur_lvl; lvl < next_lvl; lvl++) {
					tmp_ingr.prob[lvl - 1] = prob_value;
				}
				prob_value = parse::ReadAsInt(prob.GetValue("val"));
				cur_lvl = next_lvl;
			}
			for (int lvl = cur_lvl; lvl <= kMaxMobLevel; lvl++) {
				tmp_ingr.prob[lvl - 1] = prob_value;
			}
			race->ingrlist_.push_back(std::move(tmp_ingr));
		}
		node.GoToParent();
	}

	if (node.GoToChild("mob_flags")) {
		ParseFlagSet<EMobFlag>(node.GetValue("val"), race->mob_flags_);
		node.GoToParent();
	}

	if (node.GoToChild("tzs_flags")) {
		ParseFlagSet<ENpcFlag>(node.GetValue("val"), race->npc_flags_);
		node.GoToParent();
	}

	if (node.GoToChild("traits")) {
		race->vocal_ = node.HaveChild("vocal");
		race->respiration_ = node.HaveChild("respiration");
		race->skinnable_ = node.HaveChild("skinnable");
		node.GoToParent();
	}

	return race;
}

// --- editable cfg loader (Vedun + hot reload) -------------------------------------------------

void MobRacesLoader::Load(DataNode data) {
	MUD::MobRaces().Init(data.Children());
}

void MobRacesLoader::Reload(DataNode data) {
	MUD::MobRaces().Reload(data.Children());
}

std::vector<cfg_manager::EditableElement> MobRacesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &race : MUD::MobRaces()) {
		out.push_back({race.GetTextId(), race.GetName()});
	}
	return out;
}

cfg_manager::ValidationResult MobRacesLoader::Validate(DataNode &doc) const {
	if (MUD::MobRaces().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Mob race data failed to parse (see syslog for the offending race/attribute)."};
}

std::string MobRacesLoader::CanonicalElementId(const std::string &id) const {
	for (const auto &[value, name] : NAMES_OF<ENpcRace>()) {
		if (str_cmp(name, id) == 0) {
			return name;
		}
	}
	return "";
}

} // namespace mob_races

bool CanBreathe(CharData *ch) {
	if (!ch->IsNpc()) {
		return true;
	}
	return MUD::MobRaces()[GET_RACE(ch)].CanBreathe();
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
