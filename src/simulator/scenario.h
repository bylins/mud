#ifndef BYLINS_SIMULATOR_SCENARIO_H
#define BYLINS_SIMULATOR_SCENARIO_H

#include "engine/structs/structs.h"

#include <string>
#include <variant>

namespace simulator {

// Optional stat overrides for a participant. Default (-1) means "leave whatever
// the engine assigned" - for player it's set by CharacterBuilder, for mob it's
// the prototype value.
struct StatOverrides {
	int str = -1;
	int dex = -1;
	int con = -1;
	int intel = -1;  // 'int' is a keyword
	int wis = -1;
	int cha = -1;
	int max_hit = -1;  // base max HP
};

// Class name is kept as a string (not resolved to ECharClass) at scenario load
// time, because class resolution requires the world to be booted. The runner
// resolves it when creating the participant.
struct PlayerSpec {
	std::string class_name;
	int level = 1;
	StatOverrides overrides;
};

struct MobSpec {
	MobVnum vnum = 0;
	StatOverrides overrides;
};

using ParticipantSpec = std::variant<PlayerSpec, MobSpec>;

// What the attacker does each battle round. Default is plain melee
// (auto-attack via SetFighting + heartbeat). Cast invokes DoCast with the
// given Russian spell name on the victim each time the global cooldown
// allows.
struct MeleeAction {};
struct CastAction {
	std::string spell_name;
};
using AttackerAction = std::variant<MeleeAction, CastAction>;

struct Scenario {
	unsigned seed = 0;
	int rounds = 100;
	std::string output;
	ParticipantSpec attacker;
	ParticipantSpec victim;
	AttackerAction action;  // default: MeleeAction
};

}  // namespace simulator

#endif  // BYLINS_SIMULATOR_SCENARIO_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
