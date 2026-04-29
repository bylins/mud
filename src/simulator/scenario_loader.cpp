#include "scenario_loader.h"

#include <yaml-cpp/yaml.h>

#include <fmt/format.h>

#include <fstream>
#include <sstream>

namespace simulator {

namespace {

std::vector<PetSpec> ParsePets(const YAML::Node& node, const char* role);
std::vector<int> ParseInventory(const YAML::Node& node, const char* role);

StatOverrides ParseStatOverrides(const YAML::Node& node) {
	StatOverrides o;
	if (!node) {
		return o;
	}
	if (node["str"]) o.str = node["str"].as<int>();
	if (node["dex"]) o.dex = node["dex"].as<int>();
	if (node["con"]) o.con = node["con"].as<int>();
	if (node["int"]) o.intel = node["int"].as<int>();
	if (node["wis"]) o.wis = node["wis"].as<int>();
	if (node["cha"]) o.cha = node["cha"].as<int>();
	if (node["max_hit"]) o.max_hit = node["max_hit"].as<int>();
	return o;
}

ParticipantSpec ParseParticipant(const YAML::Node& node, const char* role) {
	if (!node || !node.IsMap()) {
		throw ScenarioLoadError(fmt::format("scenario.{}: must be a map", role));
	}
	const auto type = node["type"];
	if (!type) {
		throw ScenarioLoadError(fmt::format("scenario.{}.type: required field is missing", role));
	}
	const auto type_str = type.as<std::string>();
	if (type_str == "player") {
		PlayerSpec p;
		const auto cls = node["class"];
		const auto level = node["level"];
		if (!cls) {
			throw ScenarioLoadError(fmt::format("scenario.{}.class: required for player", role));
		}
		if (!level) {
			throw ScenarioLoadError(fmt::format("scenario.{}.level: required for player", role));
		}
		p.class_name = cls.as<std::string>();
		p.level = level.as<int>();
		p.overrides = ParseStatOverrides(node["stats"]);
		p.pets = ParsePets(node["pets"], role);
		p.inventory = ParseInventory(node["inventory"], role);
		return p;
	}
	if (type_str == "mob") {
		MobSpec m;
		const auto vnum = node["vnum"];
		if (!vnum) {
			throw ScenarioLoadError(fmt::format("scenario.{}.vnum: required for mob", role));
		}
		m.vnum = vnum.as<int>();
		m.overrides = ParseStatOverrides(node["stats"]);
		m.pets = ParsePets(node["pets"], role);
		m.inventory = ParseInventory(node["inventory"], role);
		return m;
	}
	throw ScenarioLoadError(fmt::format(
		"scenario.{}.type: must be 'player' or 'mob' (got '{}')", role, type_str));
}

std::vector<int> ParseInventory(const YAML::Node& node, const char* role) {
	std::vector<int> out;
	if (!node) {
		return out;
	}
	if (!node.IsSequence()) {
		throw ScenarioLoadError(fmt::format(
			"scenario.{}.inventory: must be a sequence of object vnums", role));
	}
	for (std::size_t i = 0; i < node.size(); ++i) {
		out.push_back(node[i].as<int>());
	}
	return out;
}

std::vector<PetSpec> ParsePets(const YAML::Node& node, const char* role) {
	std::vector<PetSpec> out;
	if (!node) {
		return out;
	}
	if (!node.IsSequence()) {
		throw ScenarioLoadError(fmt::format(
			"scenario.{}.pets: must be a sequence", role));
	}
	for (std::size_t i = 0; i < node.size(); ++i) {
		const auto pet_node = node[i];
		if (!pet_node.IsMap()) {
			throw ScenarioLoadError(fmt::format(
				"scenario.{}.pets[{}]: must be a map", role, i));
		}
		PetSpec p;
		const auto vnum = pet_node["vnum"];
		if (!vnum) {
			throw ScenarioLoadError(fmt::format(
				"scenario.{}.pets[{}].vnum: required", role, i));
		}
		p.vnum = vnum.as<int>();
		p.overrides = ParseStatOverrides(pet_node["stats"]);
		out.push_back(p);
	}
	return out;
}

Scenario ParseScenario(const YAML::Node& root) {
	if (!root || !root.IsMap()) {
		throw ScenarioLoadError("scenario: top-level node must be a map");
	}
	Scenario s;
	if (root["seed"]) {
		s.seed = root["seed"].as<unsigned>();
	}
	if (root["rounds"]) {
		s.rounds = root["rounds"].as<int>();
	}
	if (root["output"]) {
		s.output = root["output"].as<std::string>();
	} else {
		throw ScenarioLoadError("scenario.output: required field is missing");
	}
	s.attacker = ParseParticipant(root["attacker"], "attacker");
	s.victim = ParseParticipant(root["victim"], "victim");
	if (root["action"]) {
		const auto action_node = root["action"];
		if (!action_node.IsMap()) {
			throw ScenarioLoadError("scenario.action: must be a map");
		}
		const auto type = action_node["type"];
		if (!type) {
			throw ScenarioLoadError("scenario.action.type: required field is missing");
		}
		const auto type_str = type.as<std::string>();
		if (type_str == "melee") {
			s.action = MeleeAction{};
		} else if (type_str == "cast") {
			const auto spell_node = action_node["spell"];
			if (!spell_node) {
				throw ScenarioLoadError("scenario.action.spell: required for cast");
			}
			s.action = CastAction{spell_node.as<std::string>()};
		} else {
			throw ScenarioLoadError(fmt::format(
				"scenario.action.type: must be 'melee' or 'cast' (got '{}')", type_str));
		}
	}
	if (s.rounds <= 0) {
		throw ScenarioLoadError(fmt::format("scenario.rounds: must be positive (got {})", s.rounds));
	}
	return s;
}

}  // namespace

Scenario LoadScenario(const std::string& path) {
	std::ifstream in(path);
	if (!in) {
		throw ScenarioLoadError(fmt::format("cannot open scenario file: {}", path));
	}
	std::ostringstream ss;
	ss << in.rdbuf();
	return LoadScenarioFromString(ss.str());
}

Scenario LoadScenarioFromString(const std::string& yaml_text) {
	YAML::Node root;
	try {
		root = YAML::Load(yaml_text);
	} catch (const YAML::Exception& e) {
		throw ScenarioLoadError(fmt::format("YAML parse error: {}", e.what()));
	}
	return ParseScenario(root);
}

}  // namespace simulator

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
