#include "legacy_files.h"

#include "utils/logger.h"
#include "utils/utils.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace
{

// Подкаталог и расширение файлов зоны для типа мира; nullptr -- не мировой тип.
const char* WorldSubdir(EBootType kind)
{
	switch (kind)
	{
		case DB_BOOT_WLD:
			return "wld";

		case DB_BOOT_MOB:
			return "mob";

		case DB_BOOT_OBJ:
			return "obj";

		case DB_BOOT_ZON:
			return "zon";

		case DB_BOOT_TRG:
			return "trg";

		default:
			return nullptr;
	}
}

constexpr EBootType kWorldKinds[] = {DB_BOOT_WLD, DB_BOOT_MOB, DB_BOOT_OBJ, DB_BOOT_ZON, DB_BOOT_TRG};

} // namespace

LegacyFiles::LegacyFiles(ELayout layout, std::filesystem::path root)
	: m_layout(layout),
	m_root(std::move(root))
{
}

LegacyFiles LegacyFiles::World(std::filesystem::path root)
{
	return LegacyFiles(ELayout::kWorld, std::move(root));
}

LegacyFiles LegacyFiles::Help(std::filesystem::path root)
{
	return LegacyFiles(ELayout::kHelp, std::move(root));
}

std::filesystem::path LegacyFiles::Dir(EBootType kind) const
{
	if (m_layout == ELayout::kHelp)
	{
		return kind == DB_BOOT_HLP ? m_root : std::filesystem::path();
	}
	const char* subdir = WorldSubdir(kind);
	return subdir ? m_root / subdir : std::filesystem::path();
}

std::filesystem::path LegacyFiles::Index(EBootType kind) const
{
	const auto dir = Dir(kind);
	return dir.empty() ? dir : dir / INDEX_FILE;
}

std::filesystem::path LegacyFiles::Entry(EBootType kind, const std::string& name) const
{
	const auto dir = Dir(kind);
	return dir.empty() ? dir : dir / name;
}

std::filesystem::path LegacyFiles::ZoneFile(EBootType kind, int zone_vnum) const
{
	const char* ext = m_layout == ELayout::kWorld ? WorldSubdir(kind) : nullptr;
	if (!ext)
	{
		return {};
	}
	return Dir(kind) / fmt::format("{}.{}", zone_vnum, ext);
}

std::filesystem::path LegacyFiles::TempFile(EBootType kind, int zone_vnum) const
{
	const auto dir = m_layout == ELayout::kWorld ? Dir(kind) : std::filesystem::path();
	if (dir.empty())
	{
		return dir;
	}
	return dir / fmt::format("{}.new", zone_vnum);
}

void LegacyFiles::EnsureLayout() const
{
	if (m_layout != ELayout::kWorld)
	{
		return;
	}
	for (const auto kind : kWorldKinds)
	{
		std::error_code ec;
		std::filesystem::create_directories(Dir(kind), ec);
		if (ec)
		{
			log("SYSERR: cannot create %s: %s", Dir(kind).c_str(), ec.message().c_str());
		}
	}
}

bool LegacyFiles::CommitZoneFile(EBootType kind, int zone_vnum) const
{
	const auto tmp = TempFile(kind, zone_vnum);
	const auto final_path = ZoneFile(kind, zone_vnum);
	if (tmp.empty() || final_path.empty())
	{
		log("SYSERR: CommitZoneFile: type %d is not a world file type", static_cast<int>(kind));
		return false;
	}
	// rename поверх существующего файла заменяет его одним шагом.
	std::error_code ec;
	std::filesystem::rename(tmp, final_path, ec);
	if (ec)
	{
		log("SYSERR: cannot rename %s to %s: %s", tmp.c_str(), final_path.c_str(), ec.message().c_str());
		return false;
	}
	return true;
}

void LegacyFiles::RebuildIndex(EBootType kind) const
{
	const auto dir = Dir(kind);
	const char* ext = WorldSubdir(kind);
	if (dir.empty() || !ext || !std::filesystem::is_directory(dir))
	{
		return;
	}
	const std::string extension = std::string(".") + ext;
	std::vector<std::string> names;
	for (const auto& entry : std::filesystem::directory_iterator(dir))
	{
		if (entry.is_regular_file() && entry.path().extension() == extension)
		{
			names.push_back(entry.path().filename().string());
		}
	}
	// По номеру зоны в начале имени, при равенстве -- по имени.
	std::sort(names.begin(), names.end(),
		[](const std::string& a, const std::string& b)
		{
			const long va = std::strtol(a.c_str(), nullptr, 10);
			const long vb = std::strtol(b.c_str(), nullptr, 10);
			if (va != vb)
			{
				return va < vb;
			}
			return a < b;
		});
	const auto tmp = dir / "index.new";
	{
		std::ofstream os(tmp, std::ios::trunc);
		if (!os)
		{
			throw std::runtime_error("cannot open " + tmp.string());
		}
		for (const auto& name : names)
		{
			os << name << "\n";
		}
		os << "$\n";
	}
	std::filesystem::rename(tmp, Index(kind));
}

bool LegacyFiles::AddToIndex(EBootType kind, int zone_vnum) const
{
	const auto zone_file = ZoneFile(kind, zone_vnum);
	if (zone_file.empty())
	{
		log("SYSERR: AddToIndex: type %d is not a world file type", static_cast<int>(kind));
		return false;
	}
	const auto old_name = Index(kind);
	const auto new_name = Dir(kind) / "newindex";
	FILE* oldfile = fopen(old_name.c_str(), "r");
	if (!oldfile)
	{
		log("SYSERR: AddToIndex: failed to open %s", old_name.c_str());
		return false;
	}
	FILE* newfile = fopen(new_name.c_str(), "w");
	if (!newfile)
	{
		log("SYSERR: AddToIndex: failed to open %s", new_name.c_str());
		fclose(oldfile);
		return false;
	}

	// Индекс упорядочен по номерам: ищем место, вставляем файл зоны, остальное
	// переписываем как было.
	const std::string entry = zone_file.filename().string();
	bool found = false;
	int num = 0;
	// get_line читает строку через буфер в 256 байт и копирует её сюда целиком.
	char line[256];
	while (get_line(oldfile, line))
	{
		if (*line == '$')
		{
			if (!found)
			{
				fprintf(newfile, "%s\n", entry.c_str());
			}
			fprintf(newfile, "$\n");
			break;
		}
		else if (!found)
		{
			sscanf(line, "%d", &num);
			if (num == zone_vnum)
			{
				found = true;
			}
			else if (num > zone_vnum)
			{
				found = true;
				fprintf(newfile, "%s\n", entry.c_str());
			}
		}
		fprintf(newfile, "%s\n", line);
	}
	fclose(newfile);
	fclose(oldfile);

	std::error_code ec;
	std::filesystem::rename(new_name, old_name, ec);
	if (ec)
	{
		log("SYSERR: AddToIndex: cannot rename %s to %s: %s", new_name.c_str(), old_name.c_str(),
			ec.message().c_str());
		return false;
	}
	return true;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
