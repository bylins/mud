#ifndef BYLINS_SRC_ENGINE_BOOT_LEGACY_FILES_H_
#define BYLINS_SRC_ENGINE_BOOT_LEGACY_FILES_H_

#include "boot_constants.h"

#include <filesystem>
#include <string>

// Раскладка файлов legacy-загрузчика (#3301). Только этот класс знает, где под корнем
// лежат файлы каждого типа и как они называются: загрузчик (BootIndex) и OLC-сохранение
// берут пути отсюда и сами их не склеивают. Корень задаёт тот, кто создаёт объект, --
// явно, а не через текущий каталог процесса.
class LegacyFiles
{
public:
	// Мир: <root>/{wld,mob,obj,zon,trg}/<vnum>.<тип>, в каждом подкаталоге свой index.
	static LegacyFiles World(std::filesystem::path root);
	// Справка: <root>/index и перечисленные в нём файлы, без подкаталогов.
	static LegacyFiles Help(std::filesystem::path root);

	[[nodiscard]] const std::filesystem::path& Root() const { return m_root; }
	// Каталог файлов этого типа. Пустой путь -- тип не из этой раскладки.
	[[nodiscard]] std::filesystem::path Dir(EBootType kind) const;
	[[nodiscard]] std::filesystem::path Index(EBootType kind) const;
	// Файл, названный строкой индекса.
	[[nodiscard]] std::filesystem::path Entry(EBootType kind, const std::string& name) const;
	// Файл зоны (<vnum>.wld и т.п.) и временный файл, куда его пишет сохранение (<vnum>.new).
	[[nodiscard]] std::filesystem::path ZoneFile(EBootType kind, int zone_vnum) const;
	[[nodiscard]] std::filesystem::path TempFile(EBootType kind, int zone_vnum) const;

	// Создать подкаталоги мира, если их нет: fopen каталогов не создаёт.
	void EnsureLayout() const;
	// Поставить записанный TempFile на место ZoneFile.
	bool CommitZoneFile(EBootType kind, int zone_vnum) const;
	// Переписать index по файлам каталога, в числовом порядке. Бросает при ошибке записи.
	void RebuildIndex(EBootType kind) const;
	// Вставить файл зоны в index по порядку номеров, если его там ещё нет.
	bool AddToIndex(EBootType kind, int zone_vnum) const;

private:
	enum class ELayout
	{
		kWorld,
		kHelp
	};

	LegacyFiles(ELayout layout, std::filesystem::path root);

	ELayout m_layout;
	std::filesystem::path m_root;
};

#endif // BYLINS_SRC_ENGINE_BOOT_LEGACY_FILES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
