#ifndef BYLINS_SIMULATOR_SCENARIO_LOADER_H
#define BYLINS_SIMULATOR_SCENARIO_LOADER_H

#include "scenario.h"

#include <stdexcept>
#include <string>

namespace simulator {

class ScenarioLoadError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

// Parses a YAML scenario file. Throws ScenarioLoadError on any problem.
// Class names are resolved through FindAvailableCharClassId(); resolution
// against the engine's class table requires that the world has been booted
// before LoadScenario() is called.
Scenario LoadScenario(const std::string& path);

// Same as above but reads the YAML from an in-memory string. Used by tests.
Scenario LoadScenarioFromString(const std::string& yaml_text);

}  // namespace simulator

#endif  // BYLINS_SIMULATOR_SCENARIO_LOADER_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
