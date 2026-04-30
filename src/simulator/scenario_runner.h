#ifndef BYLINS_SIMULATOR_SCENARIO_RUNNER_H
#define BYLINS_SIMULATOR_SCENARIO_RUNNER_H

#include "scenario.h"

namespace simulator {

class ScenarioRunError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

// Runs the scenario on a booted world and emits events through the global
// sink registry (observability::RegisterEventSink). Each round recreates both
// participants from scratch (clean affects, clean HP, clean cooldowns) so the
// per-round numbers are independent.
//
// The attacker is given an absurdly large HP pool so it does not die mid-run
// and skew the dps measurement. The victim is observed: its HP before/after
// and the resulting damage_observed are emitted.
//
// Pre-condition: BootMudDataBase() has been called and at least one sink is
// registered (otherwise nothing is emitted -- the run is silent).
void RunScenario(const Scenario& scenario);

}  // namespace simulator

#endif  // BYLINS_SIMULATOR_SCENARIO_RUNNER_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
