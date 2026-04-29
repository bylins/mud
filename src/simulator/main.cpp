// Headless balance simulator entry point (issue #2967).
//
// MVP step 5: CLI now takes a YAML scenario file plus a world directory.
// All run parameters (seed, rounds, output, attacker, victim) live in the
// scenario file. The scenario itself is not yet executed (that lands in
// step 6, RunAutoAttack). This step only proves: load scenario -> seed RNG
// -> boot world -> tick the requested number of pulses -> exit.
//
// CLI:
//   mud-sim --config PATH -d DIR
//
//   --config PATH   Path to the YAML scenario file (required).
//   -d DIR          World data directory (required, same semantics as for
//                   the main `circle` binary).

#include "engine/core/config.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "simulator/scenario.h"
#include "simulator/scenario_loader.h"
#include "utils/logger.h"
#include "utils/random.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace {

struct CliOptions {
	std::string data_dir;
	std::string config_path;
};

void PrintUsage(const char* argv0) {
	std::fprintf(stderr,
		"Usage: %s --config PATH -d DIR\n"
		"  --config PATH   YAML scenario file (required)\n"
		"  -d DIR          World data directory (required)\n",
		argv0);
}

bool ParseCli(int argc, char** argv, CliOptions& opts) {
	for (int i = 1; i < argc; ++i) {
		const std::string a = argv[i];
		if (a == "-d") {
			if (++i >= argc) { PrintUsage(argv[0]); return false; }
			opts.data_dir = argv[i];
		} else if (a == "--config") {
			if (++i >= argc) { PrintUsage(argv[0]); return false; }
			opts.config_path = argv[i];
		} else if (a == "-h" || a == "--help") {
			PrintUsage(argv[0]);
			std::exit(0);
		} else {
			std::fprintf(stderr, "Unknown argument: %s\n", a.c_str());
			PrintUsage(argv[0]);
			return false;
		}
	}
	if (opts.data_dir.empty() || opts.config_path.empty()) {
		std::fprintf(stderr, "Both --config and -d are required.\n");
		PrintUsage(argv[0]);
		return false;
	}
	return true;
}

}  // namespace

int main(int argc, char** argv) {
	CliOptions opts;
	if (!ParseCli(argc, argv, opts)) {
		return 1;
	}

	simulator::Scenario scenario;
	try {
		scenario = simulator::LoadScenario(opts.config_path);
	} catch (const std::exception& e) {
		std::fprintf(stderr, "mud-sim: failed to load scenario %s: %s\n",
			opts.config_path.c_str(), e.what());
		return 1;
	}

	const std::string config_path = opts.data_dir + "/misc/configuration.xml";
	runtime_config.load(config_path.c_str());
	runtime_config.setup_logs();
	logfile = runtime_config.logs(SYSLOG).handle();

	if (chdir(opts.data_dir.c_str()) < 0) {
		std::perror("mud-sim: cannot chdir to data directory");
		return 1;
	}

	SetRandomSeed(scenario.seed);
	log("mud-sim: scenario loaded from %s, seed = %u, rounds = %d",
		opts.config_path.c_str(), scenario.seed, scenario.rounds);

	BootMudDataBase();
	log("mud-sim: world booted, ticking %d pulses (scenario runner: step 6)",
		scenario.rounds);

	for (int i = 0; i < scenario.rounds; ++i) {
		MUD::heartbeat()(0);
	}

	log("mud-sim: done");
	return 0;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
