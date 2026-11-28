// Команда зоны V (переменная триггера) проходит через SQLite без потерь (#4267).
//
// У V поле arg2 -- комната (только у триггера комнаты; в памяти это rnum), arg3 -- контекст
// переменной. SaveZone писал arg2 как есть, то есть rnum комнаты вместо vnum, и к тому же клал
// arg2 в колонку arg_context, а arg3 -- в arg_room_vnum. Загрузка разбирала колонки в том же
// перепутанном порядке, поэтому по позициям всё сходилось, но rnum после загрузки принимался за
// vnum: команда отключалась или ставила переменную в чужой комнате.
//
// Только для сборки с SQLite: без HAVE_SQLITE файл пустой.

#ifdef HAVE_SQLITE

#include "engine/db/db.h"
#include "engine/db/sqlite_world_data_source.h"
#include "engine/entities/room_data.h"
#include "engine/entities/zone.h"
#include "engine/scripting/dg_scripts.h"
#include "utils/utils.h"

#include <sqlite3.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace sqlite_zone_cmd_var_test {

constexpr int kZoneVnum = 970;
constexpr int kRoomVnum = 97001;
constexpr int kRoomContext = 7;
constexpr int kMobContext = 3;

class SqliteZoneCmdVarTest : public ::testing::Test {
 protected:
	void SetUp() override {
		const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
		dir_ = std::filesystem::temp_directory_path()
			/ (std::string("sqlite_zone_cmd_var_") + info->name() + "_"
				+ std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		std::filesystem::remove_all(dir_);
		std::filesystem::create_directories(dir_);
		db_path_ = (dir_ / "world.db").string();

		// Свой мир и своя таблица зон: LoadZones переписывает zone_table целиком.
		saved_zones_.swap(zone_table);
		saved_world_.swap(world);
		saved_top_of_world_ = top_of_world;

		world.push_back(new RoomData());    // rnum 0 -- kNowhere
		auto *room = new RoomData();
		room->vnum = kRoomVnum;
		world.push_back(room);
		room_rnum_ = static_cast<int>(world.size() - 1);
		top_of_world = room_rnum_;
	}

	void TearDown() override {
		FreeVarStrings();
		zone_table.clear();
		for (auto *room : world) {
			delete room;
		}
		world.clear();
		saved_zones_.swap(zone_table);
		saved_world_.swap(world);
		top_of_world = saved_top_of_world_;
		std::filesystem::remove_all(dir_);
	}

	// Зона в том виде, в каком её держит движок после загрузки: у триггера комнаты arg2 -- rnum.
	void BuildZone() {
		ZoneData zone;
		zone.vnum = kZoneVnum;
		zone.name = "sqlite zone cmd var test";
		zone.top = kZoneVnum * 100 + 99;
		CREATE(zone.cmd, 3);

		auto &room_var = zone.cmd[0];
		room_var.command = 'V';
		room_var.arg1 = WLD_TRIGGER;
		room_var.arg2 = room_rnum_;
		room_var.arg3 = kRoomContext;
		room_var.arg4 = -1;
		room_var.sarg1 = str_dup("quest");
		room_var.sarg2 = str_dup("started now");

		auto &mob_var = zone.cmd[1];
		mob_var.command = 'V';
		mob_var.arg1 = MOB_TRIGGER;
		mob_var.arg2 = 0;
		mob_var.arg3 = kMobContext;
		mob_var.arg4 = -1;
		mob_var.sarg1 = str_dup("counter");
		mob_var.sarg2 = str_dup("5");

		zone.cmd[2].command = 'S';
		zone_table.push_back(std::move(zone));
	}

	// ZoneData освобождает cmd, но не строки команд V.
	void FreeVarStrings() {
		for (auto &zone : zone_table) {
			if (!zone.cmd) {
				continue;
			}
			for (int i = 0; zone.cmd[i].command != 'S'; ++i) {
				free(zone.cmd[i].sarg1);
				free(zone.cmd[i].sarg2);
				zone.cmd[i].sarg1 = nullptr;
				zone.cmd[i].sarg2 = nullptr;
			}
		}
	}

	// (arg_room_vnum, arg_context) каждой команды зоны, по порядку.
	std::vector<std::pair<int, int>> ReadRawColumns() const {
		std::vector<std::pair<int, int>> rows;
		sqlite3 *db = nullptr;
		if (sqlite3_open_v2(db_path_.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
			sqlite3_close(db);
			return rows;
		}
		const std::string sql = "SELECT arg_room_vnum, arg_context FROM zone_commands WHERE zone_vnum = "
			+ std::to_string(kZoneVnum) + " ORDER BY cmd_order";
		sqlite3_stmt *stmt = nullptr;
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				rows.emplace_back(sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1));
			}
		}
		sqlite3_finalize(stmt);
		sqlite3_close(db);
		return rows;
	}

	std::filesystem::path dir_;
	std::string db_path_;
	int room_rnum_{0};

 private:
	ZoneTable saved_zones_;
	std::vector<RoomData *> saved_world_;
	RoomRnum saved_top_of_world_{0};
};

TEST_F(SqliteZoneCmdVarTest, SaveWritesRoomVnumAndContextToTheirColumns) {
	BuildZone();
	world_loader::SqliteWorldDataSource source(db_path_);
	ASSERT_TRUE(source.IsWritable());    // заодно разворачивает схему в пустой базе
	source.SaveZone(0);

	const auto rows = ReadRawColumns();
	ASSERT_EQ(rows.size(), 2u);
	// Триггер комнаты: vnum комнаты, а не её rnum, и в своей колонке.
	EXPECT_EQ(rows[0].first, kRoomVnum);
	EXPECT_EQ(rows[0].second, kRoomContext);
	// Триггер моба: комнаты нет, контекст в своей колонке.
	EXPECT_EQ(rows[1].second, kMobContext);
}

TEST_F(SqliteZoneCmdVarTest, RoundTripKeepsRoomAndContext) {
	BuildZone();
	{
		world_loader::SqliteWorldDataSource source(db_path_);
		ASSERT_TRUE(source.IsWritable());
		source.SaveZone(0);
	}
	FreeVarStrings();
	zone_table.clear();

	world_loader::SqliteWorldDataSource source(db_path_);
	source.LoadZones();
	ASSERT_EQ(zone_table.size(), 1u);
	const auto &zone = zone_table[0];
	ASSERT_EQ(zone.vnum, kZoneVnum);
	ASSERT_NE(zone.cmd, nullptr);

	// До резолвера команды зоны держат vnum -- его загрузка и должна вернуть.
	const auto &room_var = zone.cmd[0];
	EXPECT_EQ(room_var.command, 'V');
	EXPECT_EQ(room_var.arg1, WLD_TRIGGER);
	EXPECT_EQ(room_var.arg2, kRoomVnum);
	EXPECT_EQ(room_var.arg3, kRoomContext);
	ASSERT_NE(room_var.sarg1, nullptr);
	ASSERT_NE(room_var.sarg2, nullptr);
	EXPECT_STREQ(room_var.sarg1, "quest");
	EXPECT_STREQ(room_var.sarg2, "started now");

	const auto &mob_var = zone.cmd[1];
	EXPECT_EQ(mob_var.command, 'V');
	EXPECT_EQ(mob_var.arg1, MOB_TRIGGER);
	EXPECT_EQ(mob_var.arg3, kMobContext);
	ASSERT_NE(mob_var.sarg1, nullptr);
	EXPECT_STREQ(mob_var.sarg1, "counter");

	EXPECT_EQ(zone.cmd[2].command, 'S');
}

}   // namespace sqlite_zone_cmd_var_test

#endif  // HAVE_SQLITE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
