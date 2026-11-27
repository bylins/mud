// Part of Bylins http://www.mud.ru

#include "world_revision.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace world_revision {

namespace {

namespace fs = std::filesystem;

// Столько же знаков, сколько берёт для движка `git rev-parse --short HEAD`.
constexpr std::size_t kShortSha = 9;
constexpr std::size_t kFullSha = 40;
constexpr std::size_t kMaxRevisionLen = 64;
// Полная перезапись мира ("oedit save all", resync) кладёт в список все зоны
// сразу: тогда в "ver" уходит начало списка и счётчик остатка.
constexpr std::size_t kMaxZonesListed = 12;

std::string g_revision;
std::string g_date;
std::string g_edits_path;

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

void TrimRight(std::string &line) {
	while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
		line.pop_back();
	}
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
	TrimRight(line);
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
// (у репозитория мира она как раз упакована).
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
		TrimRight(name);
		if (name != ref) {
			continue;
		}
		sha = line.substr(0, space);
		return LooksLikeSha(sha);
	}
	return false;
}

std::string FormatWhen(std::time_t when, const char *format) {
	struct tm broken {};
#ifdef _WIN32
	localtime_s(&broken, &when);
#else
	localtime_r(&when, &broken);
#endif
	char out[64];
	const std::size_t len = std::strftime(out, sizeof(out), format, &broken);
	return std::string(out, len);
}

// Когда дерево встало на этот хеш, по собственной записи git: .git/logs/HEAD,
// обычный текст "<старый> <новый> Кто <почта> <время> <зона>\tчто сделали".
// Берём последнюю подряд идущую запись, у которой новый хеш -- наш: на боевом
// переключений ветки не бывает, так что это момент выкладки (git pull).
//
// Почему reflog, а не mtime файла ссылки: mtime сбивает любое копирование дерева
// (а бут-тест мира именно копирует его tar'ом) и перепаковка ссылок `git gc`, а
// текст reflog переживает и то, и другое.
std::string ReflogDate(const fs::path &git_dir, const std::string &sha) {
	std::ifstream log(git_dir / "logs" / "HEAD");
	if (!log) {
		return {};
	}
	std::vector<std::time_t> run;    // времена записей, подряд приводящих к sha
	std::string line;
	while (std::getline(log, line)) {
		const auto tab = line.find('\t');
		std::string head = (tab == std::string::npos) ? line : line.substr(0, tab);
		TrimRight(head);
		std::vector<std::string> fields;
		std::istringstream stream(head);
		std::string field;
		while (stream >> field) {
			fields.push_back(field);
		}
		// <старый> <новый> ... <время> <зона>: кто и почта между ними, со пробелами.
		if (fields.size() < 4 || fields[1].size() != kFullSha) {
			continue;
		}
		if (fields[1] != sha) {
			run.clear();
			continue;
		}
		run.push_back(static_cast<std::time_t>(std::atoll(fields[fields.size() - 2].c_str())));
	}
	if (run.empty() || run.front() <= 0) {
		return {};
	}
	return FormatWhen(run.front(), "%d.%m.%Y %H:%M");
}

// Читаем git сами, без libgit2 и без запуска процесса: нужны ровно два файла.
// Возвращает полный хеш HEAD (пусто = не вышло) и ветку, если HEAD не отцеплен.
std::string FromGit(const fs::path &world_dir, std::string &branch, std::string &date) {
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

	std::string sha;
	constexpr std::string_view kRefPrefix = "ref: ";
	if (head.compare(0, kRefPrefix.size(), kRefPrefix) != 0) {
		// Отцепленный HEAD: в файле лежит сам хеш, ветки нет.
		if (!LooksLikeSha(head)) {
			return {};
		}
		sha = head;
	} else {
		const std::string ref = head.substr(kRefPrefix.size());
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
		branch = (ref.compare(0, kHeadsPrefix.size(), kHeadsPrefix) == 0) ? ref.substr(kHeadsPrefix.size()) : ref;
	}

	if (sha.size() == kFullSha) {
		date = ReflogDate(git, sha);
	}
	return sha;
}

// --- Список правленных зон, который переживает перезагрузки ---------------
//
// Формат файла: строка "rev <ревизия>", дальше по строке на зону --
// "<внум> <время> <вид> [<вид> ...]". Ревизия в заголовке и есть вся логика
// обнуления: выложили другую -- прежний список больше не про этот мир.

constexpr std::string_view kRevLinePrefix = "rev ";

void SaveEdits() {
	if (g_edits_path.empty()) {
		return;
	}
	const fs::path path(g_edits_path);
	std::error_code error;
	if (path.has_parent_path()) {
		fs::create_directories(path.parent_path(), error);
	}
	const fs::path temp = fs::path(g_edits_path + ".tmp");
	{
		std::ofstream out(temp, std::ios::trunc);
		if (!out) {
			return;
		}
		out << "# Зоны, записанные на диск после выкладки этой ревизии мира (см. ver).\n";
		out << kRevLinePrefix << g_revision << "\n";
		for (const auto &[zone_vnum, edit] : g_edits) {
			out << zone_vnum << ' ' << static_cast<long long>(edit.when);
			for (const auto &kind : edit.kinds) {
				out << ' ' << kind;
			}
			out << "\n";
		}
	}
	fs::rename(temp, path, error);
	if (error) {
		fs::remove(temp, error);
	}
}

void LoadEdits() {
	g_edits.clear();
	if (g_edits_path.empty()) {
		return;
	}
	std::ifstream in(g_edits_path);
	if (!in) {
		return;    // файла нет -- нормально, мир ещё не правили
	}
	std::string stored_revision;
	std::map<int, ZoneEdit> edits;
	std::string line;
	while (std::getline(in, line)) {
		TrimRight(line);
		if (line.empty() || line[0] == '#') {
			continue;
		}
		if (line.compare(0, kRevLinePrefix.size(), kRevLinePrefix) == 0) {
			stored_revision = line.substr(kRevLinePrefix.size());
			continue;
		}
		std::istringstream stream(line);
		int zone_vnum = 0;
		long long when = 0;
		if (!(stream >> zone_vnum >> when)) {
			continue;
		}
		ZoneEdit edit;
		edit.when = static_cast<std::time_t>(when);
		std::string kind;
		while (stream >> kind) {
			edit.kinds.push_back(kind);
		}
		edits[zone_vnum] = std::move(edit);
	}
	if (stored_revision != g_revision) {
		// Выложили другую ревизию -- прежние расхождения уехали вместе с ней.
		SaveEdits();
		return;
	}
	g_edits = std::move(edits);
}

}  // namespace

void Load(const std::string &world_dir, const std::string &edits_state_path) {
	std::string branch;
	std::string date;
	const std::string sha = FromGit(world_dir, branch, date);
	if (sha.empty()) {
		// git недоступен -- мир развёрнут не репозиторием. Тогда берём строку,
		// которую положила выкладка (она может написать туда и дату): то же
		// запасное правило, по которому движок берёт свою ревизию из окружения
		// (tools/meson/generate_version.py).
		std::string stamp;
		g_revision = ReadFirstLine(fs::path(world_dir) / "REVISION", stamp) ? Sanitize(stamp) : std::string{};
		g_date.clear();
	} else {
		g_revision = Sanitize(branch.empty() ? ShortSha(sha) : ShortSha(sha) + " (" + branch + ")");
		g_date = date;
	}
	g_edits_path = edits_state_path;
	LoadEdits();
}

const std::string &Get() {
	return g_revision;
}

const std::string &Date() {
	return g_date;
}

void NoteZoneSaved(int zone_vnum, const char *what) {
	auto &edit = g_edits[zone_vnum];
	edit.when = std::time(nullptr);
	const std::string kind = what ? what : "";
	if (!kind.empty() && std::find(edit.kinds.begin(), edit.kinds.end(), kind) == edit.kinds.end()) {
		edit.kinds.push_back(kind);
	}
	SaveEdits();
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
		parts.push_back(fmt::format("{} ({}, {})", zone_vnum, fmt::join(edit.kinds, "+"),
								   FormatWhen(edit.when, "%d.%m %H:%M")));
	}
	return fmt::format("{}", fmt::join(parts, ", "));
}

}  // namespace world_revision

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
