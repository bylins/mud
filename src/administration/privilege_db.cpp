/**
\file privilege_db.cpp - a part of the Bylins engine.
\brief issue.privilege-rework (P1): parse cfg/privilege.xml into the membership privilege DB.
\details Ingestion only at this stage -- the decision API does not yet consult this DB (P2). See
         privilege_db.h.
*/

#include "privilege_db.h"
#include "privilege.h"

#include <cstdlib>
#include <cstring>

#include "utils/logger.h"
#include "utils/utils_string.h"   // utils::Split, utils::Trim

namespace privilege {

namespace {

PrivilegeDb g_db;

// Map a <god_list> section tag to its tier (highest first). kNone = not a known section.
EGodTier TierFromSection(const char *tag) {
	if (!std::strcmp(tag, "owner")) return EGodTier::kOwner;
	if (!std::strcmp(tag, "implementators")) return EGodTier::kImplementator;
	if (!std::strcmp(tag, "great_gods")) return EGodTier::kGreatGod;
	if (!std::strcmp(tag, "gods")) return EGodTier::kGod;
	if (!std::strcmp(tag, "immortals")) return EGodTier::kImmortal;
	if (!std::strcmp(tag, "demigods")) return EGodTier::kDemigod;
	return EGodTier::kNone;
}

std::string Lower(std::string s) {
	for (auto &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return s;
}

// Split a pipe-delimited attribute into trimmed, non-empty tokens.
std::set<std::string> SplitTokens(const std::string &raw) {
	std::set<std::string> out;
	for (auto &t : utils::Split(raw, '|')) {
		utils::Trim(t);
		if (!t.empty()) out.insert(t);
	}
	return out;
}

} // namespace

void ParseCommandList(const std::string &raw, std::set<std::string> &commands,
					  std::set<std::string> &set_subs, std::set<std::string> &show_subs) {
	for (auto &token : utils::Split(raw, '|')) {
		utils::Trim(token);
		if (token.empty()) {
			continue;
		}
		const auto paren = token.find('(');
		if (paren == std::string::npos) {
			commands.insert(token);
			continue;
		}
		// `head (sub1 sub2 ...)` -- subcommand grant. Only set/show carry subcommands.
		std::string head = token.substr(0, paren);
		utils::Trim(head);
		const auto close = token.rfind(')');
		std::string inside = (close != std::string::npos && close > paren)
				? token.substr(paren + 1, close - paren - 1) : std::string{};
		std::set<std::string> *bucket = nullptr;
		const std::string head_l = Lower(head);
		if (head_l == "set") {
			bucket = &set_subs;
		} else if (head_l == "show") {
			bucket = &show_subs;
		} else {
			log("SYSERR: privilege.xml: unexpected subcommand spec '%s' (only set/show take "
				"subcommands); treating '%s' as a plain command.", token.c_str(), head.c_str());
			if (!head.empty()) commands.insert(head);
			continue;
		}
		for (auto &sub : utils::Split(inside, ' ')) {
			utils::Trim(sub);
			if (!sub.empty()) bucket->insert(sub);
		}
	}
}

void PrivilegeDb::Clear() {
	entries_.clear();
	groups_.clear();
}

void PrivilegeDb::AddGroup(const std::string &id, const std::string &raw_commands) {
	groups_[id] = raw_commands;
}

void PrivilegeDb::AddEntry(GodEntry entry) {
	entries_[entry.uid] = std::move(entry);
}

const GodEntry *PrivilegeDb::FindByUid(long uid) const {
	const auto it = entries_.find(uid);
	return it == entries_.end() ? nullptr : &it->second;
}

const PrivilegeDb &GetDb() {
	return g_db;
}

namespace {

void ParseEntry(parser_wrapper::DataNode entry_node, EGodTier tier) {
	GodEntry e;
	e.tier = tier;
	e.name = entry_node.GetValue("name");
	e.uid = std::atol(entry_node.GetValue("uid"));
	for (auto &child : entry_node.Children()) {
		const char *tag = child.GetName();
		if (!std::strcmp(tag, "flags")) {
			e.flags = SplitTokens(child.GetValue("list"));
		} else if (!std::strcmp(tag, "groups")) {
			e.groups = SplitTokens(child.GetValue("list"));
		} else if (!std::strcmp(tag, "commands")) {
			ParseCommandList(child.GetValue("list"), e.commands, e.set_subs, e.show_subs);
		} else if (!std::strcmp(tag, "vedun")) {
			e.vedun = SplitTokens(child.GetValue("list"));
		}
	}
	g_db.AddEntry(std::move(e));
}

void DoLoad(parser_wrapper::DataNode data) {
	g_db.Clear();
	int groups = 0, gods = 0;
	for (auto &top : data.Children()) {
		const char *name = top.GetName();
		if (!std::strcmp(name, "command_groups")) {
			for (auto &grp : top.Children()) {
				if (std::strcmp(grp.GetName(), "group") != 0) continue;
				g_db.AddGroup(grp.GetValue("id"), grp.GetValue("commands"));
				++groups;
			}
		} else if (!std::strcmp(name, "god_list")) {
			for (auto &section : top.Children()) {
				const EGodTier tier = TierFromSection(section.GetName());
				if (tier == EGodTier::kNone) {
					log("SYSERR: privilege.xml: unknown god_list section <%s>, skipped.",
						section.GetName());
					continue;
				}
				for (auto &entry : section.Children()) {
					if (std::strcmp(entry.GetName(), "entry") != 0) continue;
					ParseEntry(entry, tier);
					++gods;
				}
			}
		}
	}
	log("privilege.xml loaded: %d command groups, %d privileged characters.", groups, gods);
	if constexpr (!kLegacyPrivilege) {
		privilege::LoadGodBoards();
	}
}

} // namespace

void PrivilegeLoader::Load(parser_wrapper::DataNode data) {
	DoLoad(data);
}

void PrivilegeLoader::Reload(parser_wrapper::DataNode data) {
	DoLoad(data);
}

} // namespace privilege

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
