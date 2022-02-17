#if !defined __CRAFT_COMMANDS_HPP__
#define __CRAFT_COMMANDS_HPP__

#include "commands.h"
#include "entities/entities_constants.h"

#include <memory>

namespace craft {
// Subcommands for the "crafts" command
const int SCMD_NOTHING = 0;

/// Defines for the "crafts" command (base crafts command)
namespace cmd {
extern const char *CRAFT_COMMAND;

/// Minimal position for base crafts command
constexpr EPosition MINIMAL_POSITION = EPosition::kSit;

// Minimal level for base crafts command
constexpr int MINIMAL_LEVEL = 0;

// Probability to stop hide when using base crafts command
constexpr int UNHIDE_PROBABILITY = 0;    // -1 - always, 0 - never

extern void do_craft(CharData *ch, char *argument, int cmd, int subcmd);
}
}

#endif // __CRAFT_COMMANDS_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
