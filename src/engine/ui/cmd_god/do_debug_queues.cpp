/**
\file do_debug_queues.cpp - a part of the Bylins engine.
\brief The `debug_queues` implementor command, extracted from interpreter.cpp
       (issue.interpreter-cleaning Bucket 5).
*/

#include "engine/ui/cmd_god/do_debug_queues.h"

#include "utils/utils_debug.h"
#include "utils/logger.h"
#include "engine/core/config.h"
#include "engine/structs/structs.h"

#include <sstream>

void do_debug_queues(CharData * /*ch*/, char *argument, int /*cmd*/, int /*subcmd*/) {
	std::stringstream ss;
	if (argument && *argument) {
		debug::log_queue(argument).print_queue(ss, argument);

		return;
	}

	for (const auto &q : debug::log_queues()) {
		q.second.print_queue(ss, q.first);
	}

	mudlog(ss.str().c_str(), DEF, kLvlGod, ERRLOG, true);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
