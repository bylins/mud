// Part of Bylins http://www.mud.ru

#include "world_revision.h"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace world_revision {

namespace {

namespace fs = std::filesystem;

// Столько же знаков, сколько берёт для движка `git rev-parse --short HEAD`.
constexpr std::size_t kShortSha = 9;
constexpr std::size_t kMaxRevisionLen = 64;
// Полная перезапись мира ("oedit save all", resync) кладёт в список все зоны
// сразу: тогда в "ver" уходит начало списка и счётчик остатка.
constexpr std::size_t kMaxZonesListed = 12;

std::string g_revision;

struct ZoneEdit {
	std::vector<std::string> kinds;
	std::time_t when{0};
};
std::map<int, ZoneEdit> g_edits;

// Строка приходит из файла в каталоге мира и уходит игроку в "ver", поэтому
// пропускаем только печатный ASCII и срезаем '&': это код цвета, и чужая строка
// не должна красить чужой вывод.
std::string Sanitize(std::string_view in) {
	std::string out;
	for (const char c : in) {
		if (out.size() >= kMaxRevisionLen) {
			break;
		}
		const auto byte = static_cast<unsigned char>(c);
		if (byte < 0x20 || byte > 0x7e || c == '&') {
			continue;
		}
		out += c;
	}
	return out;
}

bool ReadFirstLine(const fs::path &path, std::string &out) {
	std::ifstream file(path);
	if (!file) {
		return false;
	}
	std::string line;
	if (!std::getline(file, line)) {
		return false;
	}
	while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
		line.pop_back();
	}
	out = line;
	return !out.empty();
}

bool LooksLikeSha(std::string_view str) {
	if (str.size() < kShortSha) {
		return false;
	}
	return std::all_of(str.begin(), str.end(), [](const char c) {
		return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
	});
}

std::string ShortSha(std::string_view sha) {
	return std::string(sha.substr(0, kShortSha));
}

// Ссылка лежит либо файлом <git>/<ref>, либо строкой "<sha> <ref>" в packed-refs
// (у нас она как раз упакована, см. .git/packed-refs в репозитории мира).
bool ResolveRef(const fs::path &git_dir, const std::string &ref, std::string &sha) {
	if (ReadFirstLine(git_dir / ref, sha) && LooksLikeSha(sha)) {
		return true;
	}
	std::ifstream packed(git_dir / "packed-refs");
	if (!packed) {
		return false;
	}
	std::string line;
	while (std::getline(packed, line)) {
		// '#' -- заголовок файла, '^' -- хеш объекта под аннотированным тегом.
		if (line.empty() || line[0] == '#' || line[0] == '^') {
			continue;
		}
		const auto space = line.find(' ');
		if (space == std::string::npos) {
			continue;
		}
		std::string name = line.substr(space + 1);
		while (!name.empty() && (name.back() == '\r' || name.back() == ' ')) {
			name.pop_back();
		}
		if (name != ref) {
			continue;
		}
		sha = line.substr(0, space);
		return LooksLikeSha(sha);
	}
	return false;
}

// Читаем git сами, без libgit2 и без запуска процесса: нужны ровно два файла.
std::string FromGit(const fs::path &world_dir) {
	std::error_code error;
	fs::path git = world_dir / ".git";
	if (fs::is_regular_file(git, error)) {
		// Worktree или submodule: ".git" -- не каталог, а файл со строкой
		// "gitdir: <путь>", и дальше работаем уже с тем путём.
		std::string line;
		if (!ReadFirstLine(git, line)) {
			return {};
		}
		constexpr std::string_view kGitDirPrefix = "gitdir: ";
		if (line.compare(0, kGitDirPrefix.size(), kGitDirPrefix) != 0) {
			return {};
		}
		const fs::path dir(line.substr(kGitDirPrefix.size()));
		git = dir.is_absolute() ? dir : world_dir / dir;
	}
	if (!fs::is_directory(git, error)) {
		return {};
	}

	std::string head;
	if (!ReadFirstLine(git / "HEAD", head)) {
		return {};
	}

	constexpr std::string_view kRefPrefix = "ref: ";
	if (head.compare(0, kRefPrefix.size(), kRefPrefix) != 0) {
		// Отцепленный HEAD: в файле лежит сам хеш, ветки нет.
		return LooksLikeSha(head) ? ShortSha(head) : std::string{};
	}
	const std::string ref = head.substr(kRefPrefix.size());

	std::string sha;
	bool found = ResolveRef(git, ref, sha);
	if (!found) {
		// У worktree свой HEAD, а ссылки лежат в общем каталоге (commondir).
		std::string common;
		if (ReadFirstLine(git / "commondir", common)) {
			const fs::path common_dir(common);
			found = ResolveRef(common_dir.is_absolute() ? common_dir : git / common_dir, ref, sha);
		}
	}
	if (!found) {
		return {};
	}

	constexpr std::string_view kHeadsPrefix = "refs/heads/";
	const std::string branch =
		(ref.compare(0, kHeadsPrefix.size(), kHeadsPrefix) == 0) ? ref.substr(kHeadsPrefix.size()) : ref;
	return ShortSha(sha) + " (" + branch + ")";
}

std::string FormatWhen(std::time_t when) {
	struct tm broken {};
#ifdef _WIN32
	localtime_s(&broken, &when);
#else
	localtime_r(&when, &broken);
#endif
	return fmt::format("{:02d}.{:02d} {:02d}:{:02d}",
					   broken.tm_mday, broken.tm_mon + 1, broken.tm_hour, broken.tm_min);
}

}  // namespace

void Load(const std::string &world_dir) {
	g_revision = Sanitize(FromGit(world_dir));
	if (!g_revision.empty()) {
		return;
	}
	// git недоступен -- мир развёрнут не репозиторием. Тогда берём строку,
	// которую положила выкладка: то же запасное правило, по которому движок
	// берёт свою ревизию из окружения (tools/meson/generate_version.py).
	std::string stamp;
	if (ReadFirstLine(fs::path(world_dir) / "REVISION", stamp)) {
		g_revision = Sanitize(stamp);
	}
}

const std::string &Get() {
	return g_revision;
}

void NoteZoneSaved(int zone_vnum, const char *what) {
	auto &edit = g_edits[zone_vnum];
	edit.when = std::time(nullptr);
	if (!what || !*what) {
		return;
	}
	const std::string kind(what);
	if (std::find(edit.kinds.begin(), edit.kinds.end(), kind) == edit.kinds.end()) {
		edit.kinds.push_back(kind);
	}
}

std::string FormatEdits() {
	if (g_edits.empty()) {
		return {};
	}
	std::vector<std::string> parts;
	for (const auto &[zone_vnum, edit] : g_edits) {
		if (parts.size() >= kMaxZonesListed) {
			parts.push_back(fmt::format("и ещё {}", g_edits.size() - parts.size()));
			break;
		}
		parts.push_back(fmt::format("{} ({}, {})", zone_vnum, fmt::join(edit.kinds, "+"), FormatWhen(edit.when)));
	}
	return fmt::format("{}", fmt::join(parts, ", "));
}

}  // namespace world_revision

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
