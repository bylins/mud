#ifndef __HEARTBEAT_COMMANDS_HPP__
#define __HEARTBEAT_COMMANDS_HPP__

#include "entities/entities_constants.h"
#include "commands.h"

namespace heartbeat {
const int SCMD_NOTHING = 0;

namespace cmd {
extern const char *HEARTBEAT_COMMAND;

/// Minimal position for heartbeat command
constexpr EPosition MINIMAL_POSITION = EPosition::kSit;

// Minimal level for heartbeat command
constexpr int MINIMAL_LEVEL = kLvlImplementator;

// Probability to stop hide when using heartbeat command
constexpr int UNHIDE_PROBABILITY = 0;    // -1 - always, 0 - never

extern void do_heartbeat(CharData *ch, char *argument, int cmd, int subcmd);
}
}

#endif // __HEARTBEAT_COMMANDS_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/

