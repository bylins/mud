#ifndef __HEARTBEAT_COMMANDS_HPP__
#define __HEARTBEAT_COMMANDS_HPP__

#include "structs.h"
#include "commands.hpp"

namespace heartbeat
{
	const int SCMD_NOTHING = 0;

	namespace cmd
	{
		extern const char* HEARTBEAT_COMMAND;

		/// Minimal position for heartbeat command
		constexpr int MINIMAL_POSITION = POS_SITTING;

		// Minimal level for heartbeat command
		constexpr int MINIMAL_LEVEL = LVL_IMPL;

		// Probability to stop hide when using heartbeat command
		constexpr int UNHIDE_PROBABILITY = 0;	// -1 - always, 0 - never

		extern void do_heartbeat(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
	}
}

#endif // __HEARTBEAT_COMMANDS_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/

