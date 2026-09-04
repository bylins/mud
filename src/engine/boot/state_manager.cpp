/**
 \file state_manager.cpp - a part of the Bylins engine.
 \brief issue.misc-migrate: the state/ file-path registry. See state_manager.h.
*/

#include "state_manager.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

#include "utils/logger.h"
#include "utils/native_text.h"

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
	put(EStateFile::kStopOfftop, "stop_offtop.lst");
	put(EStateFile::kProxy, "proxy.lst");
	put(EStateFile::kInvalidNameParts, "xnames.lst");
	put(EStateFile::kApprovedNames, "apr_name.lst");
	put(EStateFile::kDisallowedNames, "dis_name.lst");
	put(EStateFile::kPendingNames, "new_name.lst");
	put(EStateFile::kUnfreeze, "unfreeze.lst");
	put(EStateFile::kBannedSites, "badsites.lst");
	put(EStateFile::kBannedProxies, "badproxy.lst");
	put(EStateFile::kTitles, "titles.lst");
	put(EStateFile::kRegisteredEmails, "registered-email.lst");
	put(EStateFile::kSaveChecksums, "player_save_checksums.lst");
	put(EStateFile::kMobStat, "statistics/mob_stat_new.xml");
	put(EStateFile::kMobStatBin, "statistics/mob_stat.bin");
	put(EStateFile::kZoneTraffic, "statistics/zone_traffic.xml");
	put(EStateFile::kGlobalDropStat, "statistics/global_drop.tmp");
	put(EStateFile::kSpellStat, "statistics/spellstat.txt");
	put(EStateFile::kUniqueMobs, "unique_mobs.xml");
	put(EStateFile::kDropTable, "sets_drop_generated_table.lst");
}

const std::string &StateManager::Path(EStateFile file) const {
	return paths_[static_cast<std::size_t>(file)];
}

std::vector<std::string> StateManager::LoadLines(EStateFile file) const {
	std::vector<std::string> lines;
	std::ifstream in(Path(file));
	if (!in.is_open()) {
		return lines;   // absent file == empty list (normal on first boot)
	}
	std::string line;
	while (std::getline(in, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();   // tolerate CRLF files
		}
		// Граница чтения: строка принимается в любой из двух кодировок -- валидный UTF-8
		// считается уже нативным, всё остальное разбирается как KOI8-R. Так файл, переведённый
		// не целиком, читается правильно, и старые списки живут рядом с новыми (issue #3787).
		lines.push_back(native_text::from_disk_line(line.c_str()));
	}
	return lines;
}

std::string StateManager::LoadText(EStateFile file) const {
	std::ifstream in(Path(file), std::ios::binary);
	if (!in.is_open()) {
		return {};   // absent file == empty (normal on first boot)
	}
	const std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	// Граница чтения, та же что в LoadLines, но на весь файл сразу (issue #3787).
	return native_text::from_disk_text(raw);
}

bool StateManager::SaveLines(EStateFile file, const std::vector<std::string> &lines) const {
	std::string body;
	for (const auto &l : lines) {
		body += l;   // граница записи, зеркало к LoadLines: пишем нативное как есть
		body += '\n';
	}
	return SaveText(file, body);
}

bool StateManager::SaveText(EStateFile file, const std::string &text) const {
	const std::string &path = Path(file);
	const std::string tmp = path + ".tmp";
	{
		std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
		if (!out.is_open()) {
			log("SYSERR: StateManager: cannot open '%s' for writing", tmp.c_str());
			return false;
		}
		out.write(text.data(), static_cast<std::streamsize>(text.size()));
		out.flush();
		if (!out.good()) {
			log("SYSERR: StateManager: write error on '%s'", tmp.c_str());
			return false;
		}
	}   // close before rename
	// std::filesystem::rename atomically REPLACES an existing destination on every
	// platform (POSIX rename() and, on Windows, MoveFileExW/MOVEFILE_REPLACE_EXISTING).
	// Plain std::rename() from <cstdio> fails on Windows when the destination exists,
	// which would break every overwrite save (bans, name lists, proxy).
	std::error_code ec;
	std::filesystem::rename(tmp, path, ec);
	if (ec) {
		log("SYSERR: StateManager: cannot rename '%s' -> '%s': %s",
			tmp.c_str(), path.c_str(), ec.message().c_str());
		return false;
	}
	return true;
}

bool StateManager::AppendLine(EStateFile file, const std::string &line) const {
	const std::string &path = Path(file);
	std::ofstream out(path, std::ios::app);
	if (!out.is_open()) {
		log("SYSERR: StateManager: cannot open '%s' for append", path.c_str());
		return false;
	}
	out << line << '\n';   // граница записи, зеркало к LoadLines: пишем нативное как есть
	return out.good();
}

bool StateManager::RewriteDropping(EStateFile file,
		const std::function<bool(const std::string &)> &drop) const {
	auto lines = LoadLines(file);
	std::vector<std::string> kept;
	kept.reserve(lines.size());
	for (auto &l : lines) {
		if (!drop(l)) {
			kept.push_back(std::move(l));
		}
	}
	return SaveLines(file, kept);
}

} // namespace state

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
