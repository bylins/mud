// Part of Bylins http://www.mud.ru

#include "ingr_chest_saver.h"

#include "house.h"

#include "engine/db/db.h"
#include "engine/db/obj_save.h"
#include "engine/entities/obj_data.h"
#include "engine/entities/room_data.h"
#include "utils/logger.h"
#include "utils/thread_pool.h"
#include "utils/utils.h"
#include "utils/utils_time.h"

#include <third_party_libs/fmt/include/fmt/format.h>

#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <future>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace ClanSystem {

namespace {

// Сериализация одного сундука. Читает ObjData, не мутирует. Размер
// буфера заранее резервируется под предыдущий размер файла +10%, чтобы
// не было серии realloc в stringbuf при росте.
bool save_one_chest(ObjData *chest, const std::string &filename) {
	std::size_t prealloc = 64 * 1024;
	struct stat st {};
	if (::stat(filename.c_str(), &st) == 0 && st.st_size > 0) {
		prealloc = static_cast<std::size_t>(st.st_size)
			+ static_cast<std::size_t>(st.st_size / 10);
	}

	std::stringstream out;
	out.str(std::string(prealloc, '\0'));
	out.seekp(0);
	out << "* Items file\n";
	for (ObjData *temp = chest->get_contains(); temp; temp = temp->get_next_content()) {
		write_one_object(out, temp, 0);
	}
	out << "\n$\n$\n";

	std::ofstream file(filename.c_str());
	if (!file.is_open()) {
		return false;
	}
	// Преаллокация оставляет нули в хвосте буфера, write() пишет только
	// реально заполненную часть по tellp().
	const auto written = out.tellp();
	const auto contents = out.str();
	file.write(contents.data(), static_cast<std::streamsize>(written));
	file.close();
	return true;
}

}  // namespace

class IngrChestSaver::Impl {
 public:
	Impl() : pool(std::max<std::size_t>(1, std::thread::hardware_concurrency())) {}

	utils::ThreadPool pool;
	// Клан попадает сюда, когда содержимое его сундука изменилось.
	// Обращения из главного потока, синхронизация не требуется.
	std::unordered_set<Clan *> dirty;
};

IngrChestSaver::IngrChestSaver() : m_impl(std::make_unique<Impl>()) {}

IngrChestSaver::~IngrChestSaver() = default;

void IngrChestSaver::mark_dirty(Clan *clan) {
	if (clan) {
		m_impl->dirty.insert(clan);
	}
}

void IngrChestSaver::run() {
	struct Job {
		Clan *clan;
		ObjData *chest;
		std::string filename;
		int objcount;
	};

	struct Result {
		Clan *clan;
		std::string abbrev;
		double seconds;
		bool success;
	};

	std::vector<Job> jobs;
	jobs.reserve(m_impl->dirty.size());
	for (const auto &clan : Clan::ClanList) {
		Clan *raw = clan.get();
		if (!m_impl->dirty.count(raw)) {
			continue;
		}
		if (!raw->ingr_chest_active()) {
			m_impl->dirty.erase(raw);
			continue;
		}
		const auto file_abbrev = raw->get_file_abbrev();
		std::string filename = LIB_HOUSE + file_abbrev + "/" + file_abbrev + ".ing";
		for (auto chest : world[raw->get_ingr_chest_room_rnum()]->contents) {
			if (!ClanSystem::is_ingr_chest(chest)) {
				continue;
			}
			Job job;
			job.clan = raw;
			job.chest = chest;
			job.filename = std::move(filename);
			job.objcount = raw->get_ingr_chest_objcount();
			jobs.push_back(std::move(job));
			break;
		}
	}

	// Снимаем dirty до постановки в очередь: параллельный put_ingr_chest
	// взведёт флаг заново, и следующий вызов run() поднимет обновлённое
	// состояние. При ошибке записи флаг будет восстановлен ниже.
	for (const auto &job : jobs) {
		m_impl->dirty.erase(job.clan);
	}

	if (jobs.empty()) {
		return;
	}

	// Longest Processing Time. Время сериализации одного сундука линейно
	// зависит от числа объектов в нём; используем это как вес задачи,
	// чтобы самая тяжёлая стартовала первой и wall time был близок к
	// теоретическому минимуму max(longest, total / threads).
	std::sort(jobs.begin(), jobs.end(),
		[](const Job &a, const Job &b) { return a.objcount > b.objcount; });

	utils::CExecutionTimer wall_timer;

	std::vector<std::future<Result>> futures;
	futures.reserve(jobs.size());
	for (const auto &job : jobs) {
		futures.push_back(m_impl->pool.Enqueue([job]() -> Result {
			utils::CExecutionTimer timer;
			const bool ok = save_one_chest(job.chest, job.filename);
			return Result{
				job.clan,
				std::string(job.clan->GetAbbrev()),
				timer.delta().count(),
				ok,
			};
		}));
	}

	for (auto &f : futures) {
		const Result r = f.get();
		if (r.success) {
			log(fmt::format("saving clan chest {} done, timer {:.10f}",
				r.abbrev, r.seconds));
		} else {
			// Сундук остаётся изменённым: следующий run() повторит попытку.
			m_impl->dirty.insert(r.clan);
			log("Error saving clan chest %s: cannot open file", r.abbrev.c_str());
		}
	}

	const double wall = wall_timer.delta().count();
	log(fmt::format("save_ingr_chests: {} chests on {} threads, wall {:.10f}",
		jobs.size(), m_impl->pool.NumThreads(), wall));
}

}  // namespace ClanSystem

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
