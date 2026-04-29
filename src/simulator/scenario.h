#ifndef BYLINS_SIMULATOR_SCENARIO_H
#define BYLINS_SIMULATOR_SCENARIO_H

#include "engine/structs/structs.h"

#include <string>
#include <variant>

namespace simulator {

// Class name is kept as a string (not resolved to ECharClass) at scenario load
// time, because class resolution requires the world to be booted. The runner
// resolves it when creating the participant.
struct PlayerSpec {
	std::string class_name;
	int level = 1;
};

struct MobSpec {
	MobVnum vnum = 0;
};

using ParticipantSpec = std::variant<PlayerSpec, MobSpec>;

struct Scenario {
	unsigned seed = 0;
	int rounds = 100;
	std::string output;
	ParticipantSpec attacker;
	ParticipantSpec victim;
};

}  // namespace simulator

#endif  // BYLINS_SIMULATOR_SCENARIO_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
