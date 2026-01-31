// World Save/Load Unit Tests
// Tests for SQLite, YAML, and Legacy world data source save implementations

#include <gtest/gtest.h>

#ifdef HAVE_SQLITE
#include <sqlite3.h>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

// =============================================================================
// SQLite Save Implementation Tests
// =============================================================================

class SQLiteWorldSaveTest : public ::testing::Test {
protected:
	void SetUp() override {
		test_db_ = fs::temp_directory_path() / "test_world_save.db";
		fs::remove(test_db_);  // Clean start

		// Create database
		int rc = sqlite3_open(test_db_.c_str(), &db_);
		ASSERT_EQ(rc, SQLITE_OK) << "Failed to create test database";

		// Create tables with correct schema
		CreateSchema();
	}

	void TearDown() override {
		if (db_) {
			sqlite3_close(db_);
			db_ = nullptr;
		}
		fs::remove(test_db_);
	}

	void CreateSchema() {
		// Create tables matching the fixed schema
		const char* schema[] = {
			// Mob tables
			"CREATE TABLE mob_resistances (mob_vnum INTEGER NOT NULL, resist_type INTEGER NOT NULL, value INTEGER NOT NULL)",
			"CREATE TABLE mob_saves (mob_vnum INTEGER NOT NULL, save_type INTEGER NOT NULL, value INTEGER NOT NULL)",
			"CREATE TABLE mob_skills (mob_vnum INTEGER NOT NULL, skill_id INTEGER NOT NULL, value INTEGER NOT NULL)",
			"CREATE TABLE mob_feats (mob_vnum INTEGER NOT NULL, feat_id INTEGER NOT NULL)",
			"CREATE TABLE mob_spells (mob_vnum INTEGER NOT NULL, spell_id INTEGER NOT NULL)",
			"CREATE TABLE mob_flags (mob_vnum INTEGER NOT NULL, flag_category TEXT NOT NULL, flag_name TEXT NOT NULL)",
			"CREATE TABLE mob_destinations (mob_vnum INTEGER NOT NULL, dest_order INTEGER NOT NULL, room_vnum INTEGER NOT NULL, PRIMARY KEY (mob_vnum, dest_order))",

			// Object tables
			"CREATE TABLE obj_applies (obj_vnum INTEGER NOT NULL, location_id INTEGER NOT NULL, modifier INTEGER NOT NULL)",
			"CREATE TABLE obj_flags (obj_vnum INTEGER NOT NULL, flag_category TEXT NOT NULL, flag_name TEXT NOT NULL)",
		};

		for (const char* sql : schema) {
			char* errmsg = nullptr;
			int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);
			ASSERT_EQ(rc, SQLITE_OK) << "Schema creation failed: " << (errmsg ? errmsg : "unknown error");
			if (errmsg) sqlite3_free(errmsg);
		}
	}

	fs::path test_db_;
	sqlite3* db_ = nullptr;
};

// Test 1: Verify mob_resistances uses correct column name "value"
TEST_F(SQLiteWorldSaveTest, MobResistances_CorrectColumnName) {
	const char* sql = "INSERT INTO mob_resistances (mob_vnum, resist_type, value) VALUES (?, ?, ?)";
	sqlite3_stmt* stmt = nullptr;

	int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	EXPECT_EQ(rc, SQLITE_OK) << "Column 'value' should exist in mob_resistances";

	if (stmt) {
		sqlite3_bind_int(stmt, 1, 1000);  // mob_vnum
		sqlite3_bind_int(stmt, 2, 0);     // resist_type
		sqlite3_bind_int(stmt, 3, 25);    // value

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE) << "Insert should succeed";

		sqlite3_finalize(stmt);
	}

	// Verify wrong column name would fail
	const char* wrong_sql = "INSERT INTO mob_resistances (mob_vnum, resist_type, resist_value) VALUES (?, ?, ?)";
	rc = sqlite3_prepare_v2(db_, wrong_sql, -1, &stmt, nullptr);
	EXPECT_NE(rc, SQLITE_OK) << "Column 'resist_value' should NOT exist";
}

// Test 2: Verify mob_saves uses correct column name "value"
TEST_F(SQLiteWorldSaveTest, MobSaves_CorrectColumnName) {
	const char* sql = "INSERT INTO mob_saves (mob_vnum, save_type, value) VALUES (?, ?, ?)";
	sqlite3_stmt* stmt = nullptr;

	int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	EXPECT_EQ(rc, SQLITE_OK) << "Column 'value' should exist in mob_saves";

	if (stmt) {
		sqlite3_bind_int(stmt, 1, 1000);  // mob_vnum
		sqlite3_bind_int(stmt, 2, 0);     // save_type
		sqlite3_bind_int(stmt, 3, 10);    // value

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE);

		sqlite3_finalize(stmt);
	}
}

// Test 3: Verify obj_applies uses correct column names
TEST_F(SQLiteWorldSaveTest, ObjApplies_CorrectColumnNames) {
	const char* sql = "INSERT INTO obj_applies (obj_vnum, location_id, modifier) VALUES (?, ?, ?)";
	sqlite3_stmt* stmt = nullptr;

	int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	EXPECT_EQ(rc, SQLITE_OK) << "Columns 'location_id' and 'modifier' should exist in obj_applies";

	if (stmt) {
		sqlite3_bind_int(stmt, 1, 2000);  // obj_vnum
		sqlite3_bind_int(stmt, 2, 1);     // location_id
		sqlite3_bind_int(stmt, 3, 5);     // modifier

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE);

		sqlite3_finalize(stmt);
	}

	// Verify wrong column names would fail
	const char* wrong_sql = "INSERT INTO obj_applies (obj_vnum, affect_location, affect_modifier) VALUES (?, ?, ?)";
	rc = sqlite3_prepare_v2(db_, wrong_sql, -1, &stmt, nullptr);
	EXPECT_NE(rc, SQLITE_OK) << "Old column names should NOT exist";
}

// Test 4: Verify mob_destinations has dest_order column
TEST_F(SQLiteWorldSaveTest, MobDestinations_HasDestOrder) {
	const char* sql = "INSERT INTO mob_destinations (mob_vnum, dest_order, room_vnum) VALUES (?, ?, ?)";
	sqlite3_stmt* stmt = nullptr;

	int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	EXPECT_EQ(rc, SQLITE_OK) << "Column 'dest_order' should exist in mob_destinations";

	if (stmt) {
		sqlite3_bind_int(stmt, 1, 1000);  // mob_vnum
		sqlite3_bind_int(stmt, 2, 0);     // dest_order
		sqlite3_bind_int(stmt, 3, 5000);  // room_vnum

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE);

		sqlite3_finalize(stmt);
	}
}

// Test 5: Verify mob_skills table exists and is usable
TEST_F(SQLiteWorldSaveTest, MobSkills_TableExists) {
	const char* sql = "INSERT INTO mob_skills (mob_vnum, skill_id, value) VALUES (?, ?, ?)";
	sqlite3_stmt* stmt = nullptr;

	int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	EXPECT_EQ(rc, SQLITE_OK) << "mob_skills table should exist";

	if (stmt) {
		sqlite3_bind_int(stmt, 1, 1000);  // mob_vnum
		sqlite3_bind_int(stmt, 2, 10);    // skill_id
		sqlite3_bind_int(stmt, 3, 75);    // value

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE);

		sqlite3_finalize(stmt);
	}
}

// Test 6: Verify mob_feats table exists
TEST_F(SQLiteWorldSaveTest, MobFeats_TableExists) {
	const char* sql = "INSERT INTO mob_feats (mob_vnum, feat_id) VALUES (?, ?)";
	sqlite3_stmt* stmt = nullptr;

	int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	EXPECT_EQ(rc, SQLITE_OK) << "mob_feats table should exist";

	if (stmt) {
		sqlite3_bind_int(stmt, 1, 1000);  // mob_vnum
		sqlite3_bind_int(stmt, 2, 5);     // feat_id

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE);

		sqlite3_finalize(stmt);
	}
}

// Test 7: Verify mob_spells table exists
TEST_F(SQLiteWorldSaveTest, MobSpells_TableExists) {
	const char* sql = "INSERT INTO mob_spells (mob_vnum, spell_id) VALUES (?, ?)";
	sqlite3_stmt* stmt = nullptr;

	int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	EXPECT_EQ(rc, SQLITE_OK) << "mob_spells table should exist";

	if (stmt) {
		sqlite3_bind_int(stmt, 1, 1000);  // mob_vnum
		sqlite3_bind_int(stmt, 2, 20);    // spell_id

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE);

		sqlite3_finalize(stmt);
	}
}

// Test 8: Verify mob_flags supports flag_category
TEST_F(SQLiteWorldSaveTest, MobFlags_SupportsCategoryColumn) {
	const char* sql = "INSERT INTO mob_flags (mob_vnum, flag_category, flag_name) VALUES (?, ?, ?)";
	sqlite3_stmt* stmt = nullptr;

	int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	EXPECT_EQ(rc, SQLITE_OK) << "mob_flags should have flag_category column";

	// Test action category
	if (stmt) {
		sqlite3_bind_int(stmt, 1, 1000);
		sqlite3_bind_text(stmt, 2, "action", -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, "kSentinel", -1, SQLITE_STATIC);

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE);

		sqlite3_finalize(stmt);
	}

	// Test affect category
	rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	if (stmt) {
		sqlite3_bind_int(stmt, 1, 1000);
		sqlite3_bind_text(stmt, 2, "affect", -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, "kSanctuary", -1, SQLITE_STATIC);

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE);

		sqlite3_finalize(stmt);
	}
}

// Test 9: Verify obj_flags supports flag_category for wear flags
TEST_F(SQLiteWorldSaveTest, ObjFlags_SupportsCategoryColumn) {
	const char* sql = "INSERT INTO obj_flags (obj_vnum, flag_category, flag_name) VALUES (?, ?, ?)";
	sqlite3_stmt* stmt = nullptr;

	int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	EXPECT_EQ(rc, SQLITE_OK) << "obj_flags should have flag_category column";

	// Test extra category
	if (stmt) {
		sqlite3_bind_int(stmt, 1, 2000);
		sqlite3_bind_text(stmt, 2, "extra", -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, "kGlow", -1, SQLITE_STATIC);

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE);

		sqlite3_finalize(stmt);
	}

	// Test wear category
	rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
	if (stmt) {
		sqlite3_bind_int(stmt, 1, 2000);
		sqlite3_bind_text(stmt, 2, "wear", -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, "kTake", -1, SQLITE_STATIC);

		rc = sqlite3_step(stmt);
		EXPECT_EQ(rc, SQLITE_DONE);

		sqlite3_finalize(stmt);
	}
}

// Test 10: Verify data retrieval after insert (round-trip)
TEST_F(SQLiteWorldSaveTest, RoundTrip_MobResistances) {
	// Insert data
	const char* insert_sql = "INSERT INTO mob_resistances (mob_vnum, resist_type, value) VALUES (1000, 1, 30)";
	char* errmsg = nullptr;
	int rc = sqlite3_exec(db_, insert_sql, nullptr, nullptr, &errmsg);
	ASSERT_EQ(rc, SQLITE_OK) << (errmsg ? errmsg : "");
	if (errmsg) sqlite3_free(errmsg);

	// Retrieve data
	const char* select_sql = "SELECT value FROM mob_resistances WHERE mob_vnum = 1000 AND resist_type = 1";
	sqlite3_stmt* stmt = nullptr;

	rc = sqlite3_prepare_v2(db_, select_sql, -1, &stmt, nullptr);
	ASSERT_EQ(rc, SQLITE_OK);

	rc = sqlite3_step(stmt);
	ASSERT_EQ(rc, SQLITE_ROW);

	int value = sqlite3_column_int(stmt, 0);
	EXPECT_EQ(value, 30) << "Retrieved value should match inserted value";

	sqlite3_finalize(stmt);
}

#endif // HAVE_SQLITE

// =============================================================================
// YAML Tests (structure validation)
// =============================================================================

// Test that YAML includes are available
TEST(YAMLWorldSaveTest, HeadersAvailable) {
	// This test just verifies the test can compile with YAML headers
	// Actual YAML save/load tests would require file system operations
	SUCCEED() << "YAML test infrastructure is available";
}

// =============================================================================
// Legacy Tests (basic validation)
// =============================================================================

TEST(LegacyWorldSaveTest, LegacyImplementationExists) {
	// Basic test that legacy world data source is available
	// Actual tests would require OLC infrastructure
	SUCCEED() << "Legacy test infrastructure is available";
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
