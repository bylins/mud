/**
 \file state_manager.cpp - a part of the Bylins engine.
 \brief issue.misc-migrate: the state/ file-path registry. See state_manager.h.
*/

#include "state_manager.h"

namespace state {

namespace {
// The world-relative directory these files live in (was the LIB_STATE macro).
constexpr const char *kStateDir = "state/";
}  // namespace

StateManager::StateManager() {
	const auto put = [this](EStateFile file, const char *name) {
		paths_[static_cast<std::size_t>(file)] = std::string(kStateDir) + name;
	};
	put(EStateFile::kSchedule, "schedule");
	put(EStateFile::kGlobalUid, "globaluid");
	put(EStateFile::kStopOfftop, "stop_offtop");
	put(EStateFile::kProxy, "proxy");
	put(EStateFile::kInvalidNameParts, "xnames");
	put(EStateFile::kApprovedNames, "apr_name");
	put(EStateFile::kDisallowedNames, "dis_name");
	put(EStateFile::kPendingNames, "new_name");
	put(EStateFile::kUnfreeze, "unfreeze.lst");
	put(EStateFile::kBannedSites, "badsites");
	put(EStateFile::kBannedProxies, "badproxy");
}

const std::string &StateManager::Path(EStateFile file) const {
	return paths_[static_cast<std::size_t>(file)];
}

} // namespace state

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
