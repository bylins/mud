// Headless balance simulator entry point (issue #2967).
//
// MVP step 4: just prove that the engine boots without the network layer and
// that we can drive the heartbeat manually. No scenario, no event sink yet
// (those land in steps 5/6).
//
// CLI (will be replaced with a YAML scenario file in step 5):
//   mud-sim [-d DIR] [--seed UINT] [--rounds N]
//
//   -d DIR        World data directory (default: "lib"). Same semantics as
//                 the corresponding flag of the main `circle` binary.
//   --seed UINT   RNG seed for reproducibility (default: 0).
//   --rounds N    Number of heartbeat pulses to tick before exit (default: 25).

#include "engine/core/config.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "utils/logger.h"
#include "utils/random.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {

struct CliOptions {
	std::string data_dir = "lib";
	unsigned seed = 0;
	int rounds = 25;
};

void PrintUsage(const char* argv0) {
	std::fprintf(stderr,
		"Usage: %s [-d DIR] [--seed UINT] [--rounds N]\n"
		"  -d DIR        World data directory (default: lib)\n"
		"  --seed UINT   RNG seed for reproducibility (default: 0)\n"
		"  --rounds N    Heartbeat pulses to tick before exit (default: 25)\n",
		argv0);
}

bool ParseCli(int argc, char** argv, CliOptions& opts) {
	for (int i = 1; i < argc; ++i) {
		const std::string a = argv[i];
		if (a == "-d") {
			if (++i >= argc) { PrintUsage(argv[0]); return false; }
			opts.data_dir = argv[i];
		} else if (a == "--seed") {
			if (++i >= argc) { PrintUsage(argv[0]); return false; }
			opts.seed = static_cast<unsigned>(std::strtoul(argv[i], nullptr, 10));
		} else if (a == "--rounds") {
			if (++i >= argc) { PrintUsage(argv[0]); return false; }
			opts.rounds = std::atoi(argv[i]);
		} else if (a == "-h" || a == "--help") {
			PrintUsage(argv[0]);
			std::exit(0);
		} else {
			std::fprintf(stderr, "Unknown argument: %s\n", a.c_str());
			PrintUsage(argv[0]);
			return false;
		}
	}
	return true;
}

}  // namespace

int main(int argc, char** argv) {
	CliOptions opts;
	if (!ParseCli(argc, argv, opts)) {
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

	SetRandomSeed(opts.seed);
	log("mud-sim: RNG seed = %u", opts.seed);

	BootMudDataBase();
	log("mud-sim: world booted, ticking %d pulses", opts.rounds);

	for (int i = 0; i < opts.rounds; ++i) {
		MUD::heartbeat()(0);
	}

	log("mud-sim: done");
	return 0;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
