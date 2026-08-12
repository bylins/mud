/**
 \file state_manager.h - a part of the Bylins engine.
 \brief issue.misc-migrate: single source of truth for the server-state file locations (state/).
 \details These are the flat, runtime-read/written server-state files (name lists, ban lists, the
          reboot schedule, the global object-id counter, ...) that live under state/. Unlike cfg/
          files they are not XML and are not driven by CfgManager, so StateManager is a small
          parallel registry: it owns their paths and nothing else. Ask MUD::StateManager().Path(...)
          instead of hardcoding "state/..." -- this replaces the old LIB_STATE macro.
*/

#ifndef BYLINS_SRC_ENGINE_BOOT_STATE_MANAGER_H_
#define BYLINS_SRC_ENGINE_BOOT_STATE_MANAGER_H_

#include <array>
#include <string>

namespace state {

// The server-state files kept under state/. Keep kLast_ last (array sizing).
enum class EStateFile {
	kSchedule,          // schedule    : scheduled reboot times
	kGlobalUid,         // globaluid   : global unique object-id counter
	kStopOfftop,        // stop_offtop : off-topic channel block list
	kProxy,             // proxy       : known proxy IP list
	kInvalidNameParts,  // xnames      : invalid name substrings
	kApprovedNames,     // apr_name    : approved names
	kDisallowedNames,   // dis_name    : disallowed names
	kPendingNames,      // new_name    : names awaiting approval
	kUnfreeze,          // unfreeze.lst: scheduled un-freeze list
	kBannedSites,       // badsites    : banned site (host) list
	kBannedProxies,     // badproxy    : banned proxy list
	kLast_
};

// Owns the state/ file locations. The only place that knows where these files live.
class StateManager {
 public:
	StateManager();

	// Path of a state file relative to the world directory (e.g. "state/badsites"). The returned
	// reference is stable for the process lifetime, so `.c_str()` may be handed to C file APIs.
	[[nodiscard]] const std::string &Path(EStateFile file) const;

 private:
	std::array<std::string, static_cast<std::size_t>(EStateFile::kLast_)> paths_;
};

} // namespace state

#endif // BYLINS_SRC_ENGINE_BOOT_STATE_MANAGER_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
