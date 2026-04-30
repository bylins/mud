// Headless balance simulator entry point (issue #2967).
//
// MVP step 6: full pipeline. CLI takes a YAML scenario file plus a world
// directory; the scenario describes attacker, victim, seed, rounds and
// output path. Each round both participants are recreated, the attacker
// engages in melee, heartbeat ticks one battle round, and the resulting
// HP delta is emitted as one JSON line into the output file.
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
#include "engine/observability/event_sink.h"
#include "simulator/scenario.h"
#include "simulator/scenario_loader.h"
#include "simulator/scenario_runner.h"
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
	log("mud-sim: world booted, running %d rounds, output=%s",
		scenario.rounds, scenario.output.c_str());

	auto sink = observability::MakeFileEventSink(scenario.output);
	// Register so engine-side instrumentation (combat, magic, affects, etc.)
	// can emit events through EmitToAllSinks without threading the sink
	// through every call signature.
	observability::RegisterEventSink(sink.get());
	try {
		simulator::RunScenario(scenario);
	} catch (const std::exception& e) {
		observability::FlushAllSinks();
		observability::UnregisterEventSink(sink.get());
		std::fprintf(stderr, "mud-sim: scenario run failed: %s\n", e.what());
		return 1;
	}
	observability::FlushAllSinks();
	observability::UnregisterEventSink(sink.get());

	log("mud-sim: done, %d rounds emitted to %s",
		scenario.rounds, scenario.output.c_str());
	return 0;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
