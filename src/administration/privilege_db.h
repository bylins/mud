/**
\file privilege_db.h - a part of the Bylins engine.
\brief issue.privilege-rework (P1): the membership-driven privilege database loaded from
       cfg/privilege.xml.
\details This is the NEW privilege model: privileged status comes from membership in this data file
         (name + uid), not from character level. P1 only INGESTS the file into the in-memory DB; the
         decision API (privilege.{cpp,h}) is NOT yet rewired to consult it (that is P2, gated on the
         `privilege::kLegacyPrivilege` switch below). So at this stage the DB is loaded but unused --
         behaviour is unchanged. The file is server-edited only and is intentionally NOT exposed to
         the Vedun editor (plain ICfgLoader, not IEditableCfgLoader) for security.
*/

#ifndef BYLINS_SRC_ADMINISTRATION_PRIVILEGE_DB_H_
#define BYLINS_SRC_ADMINISTRATION_PRIVILEGE_DB_H_

#include <map>
#include <set>
#include <string>

#include "engine/boot/cfg_manager.h"

namespace privilege {

// Compile-time switch between the legacy (level-based, misc/privilege.lst) and the new
// membership-based (cfg/privilege.xml) privilege systems. P1 ships the new ingestion inert; P2 rewires
// the public decision API to honour this switch. Keep `true` until the new system is verified; flipping
// back to `true` is the instant rollback (the legacy code stays compiled and reachable behind it).
inline constexpr bool kLegacyPrivilege = false;

// Privileged tiers, highest first (declared seniority order in the file is purely for readability).
// Membership in a tier is by name+uid; cumulative nesting (owner > implementator > great-god > god >
// demigod) is applied by the decision API in P2.
enum class EGodTier { kOwner, kImplementator, kGreatGod, kGod, kImmortal, kDemigod, kNone };

// One privileged character as declared in the file. Command access is stored split by kind; group
// references are kept unexpanded here and resolved by the decision API. `flags` are capability tokens
// (e.g. "skills", "boards", "FullAccess").
struct GodEntry {
	std::string name;
	long uid{0};
	EGodTier tier{EGodTier::kNone};
	std::set<std::string> flags;
	std::set<std::string> groups;     // referenced <command_groups> ids
	std::set<std::string> commands;   // plain commands
	std::set<std::string> set_subs;   // `set (...)` subcommands
	std::set<std::string> show_subs;  // `show (...)` subcommands
};

// The parsed privilege database. Entries are keyed by uid (the name is verified on a match by the
// decision API). Command groups keep their raw `commands` string; expansion happens at query time.
class PrivilegeDb {
 public:
	void Clear();
	void AddGroup(const std::string &id, const std::string &raw_commands);
	void AddEntry(GodEntry entry);

	[[nodiscard]] const GodEntry *FindByUid(long uid) const;
	[[nodiscard]] const std::map<long, GodEntry> &entries() const { return entries_; }
	[[nodiscard]] const std::map<std::string, std::string> &groups() const { return groups_; }

 private:
	std::map<long, GodEntry> entries_;
	std::map<std::string, std::string> groups_;
};

// The single process-wide privilege DB (populated by PrivilegeLoader).
[[nodiscard]] const PrivilegeDb &GetDb();

// Two-level command-list parse (shared so the decision API can expand groups the same way in P2):
// split `raw` on '|', then for each token a `set (a b)` / `show (a b)` form contributes subcommands,
// anything else is a plain command. Results are merged into the passed sets.
void ParseCommandList(const std::string &raw, std::set<std::string> &commands,
					  std::set<std::string> &set_subs, std::set<std::string> &show_subs);

// CfgManager loader for cfg/privilege.xml. Deliberately a plain ICfgLoader (NOT IEditableCfgLoader):
// the privilege file must never be editable in-game via Vedun (security).
class PrivilegeLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) override;
	void Reload(parser_wrapper::DataNode data) override;
};

} // namespace privilege

#endif // BYLINS_SRC_ADMINISTRATION_PRIVILEGE_DB_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
