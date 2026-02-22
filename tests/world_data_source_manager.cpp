// Part of Bylins http://www.mud.ru
// Unit tests for WorldDataSourceManager

#include "engine/db/world_data_source_manager.h"

#include <gtest/gtest.h>
#include <string>

using namespace world_loader;

// Minimal mock: no-op implementation of IWorldDataSource
struct MockDataSource : public IWorldDataSource {
	explicit MockDataSource(std::string name = "mock") : name_(std::move(name)) {}

	std::string GetName() const override { return name_; }
	void LoadZones() override {}
	void LoadTriggers() override {}
	void LoadRooms() override {}
	void LoadMobs() override {}
	void LoadObjects() override {}
	void SaveZone(int) override {}
	bool SaveTriggers(int, int, int) override { return true; }
	void SaveRooms(int, int) override {}
	void SaveMobs(int, int) override {}
	void SaveObjects(int, int) override {}

private:
	std::string name_;
};

// Fixture that resets singleton state before/after each test
class WorldDataSourceManagerTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Reset to empty state (nullptr unique_ptr)
		WorldDataSourceManager::Instance().SetDataSource(nullptr);
	}

	void TearDown() override {
		WorldDataSourceManager::Instance().SetDataSource(nullptr);
	}
};

// After reset, HasDataSource returns false
TEST_F(WorldDataSourceManagerTest, HasDataSource_WhenEmpty_ReturnsFalse) {
	EXPECT_FALSE(WorldDataSourceManager::Instance().HasDataSource());
}

// After setting a source, HasDataSource returns true
TEST_F(WorldDataSourceManagerTest, HasDataSource_AfterSet_ReturnsTrue) {
	WorldDataSourceManager::Instance().SetDataSource(
		std::make_unique<MockDataSource>());
	EXPECT_TRUE(WorldDataSourceManager::Instance().HasDataSource());
}

// GetDataSource returns the same pointer that was set
TEST_F(WorldDataSourceManagerTest, GetDataSource_AfterSet_ReturnsSamePointer) {
	auto* raw = new MockDataSource("test-source");
	WorldDataSourceManager::Instance().SetDataSource(
		std::unique_ptr<IWorldDataSource>(raw));

	EXPECT_EQ(raw, WorldDataSourceManager::Instance().GetDataSource());
}

// GetName on the stored source returns the expected string
TEST_F(WorldDataSourceManagerTest, GetName_ReturnsExpectedString) {
	WorldDataSourceManager::Instance().SetDataSource(
		std::make_unique<MockDataSource>("my-backend"));

	EXPECT_EQ("my-backend",
		WorldDataSourceManager::Instance().GetDataSource()->GetName());
}

// Setting a second source replaces the first one
TEST_F(WorldDataSourceManagerTest, SetDataSource_Twice_ReplacesOldSource) {
	WorldDataSourceManager::Instance().SetDataSource(
		std::make_unique<MockDataSource>("first"));
	EXPECT_EQ("first",
		WorldDataSourceManager::Instance().GetDataSource()->GetName());

	WorldDataSourceManager::Instance().SetDataSource(
		std::make_unique<MockDataSource>("second"));
	EXPECT_EQ("second",
		WorldDataSourceManager::Instance().GetDataSource()->GetName());
}

// After replacing with nullptr, HasDataSource returns false again
TEST_F(WorldDataSourceManagerTest, SetDataSource_Nullptr_ClearsSource) {
	WorldDataSourceManager::Instance().SetDataSource(
		std::make_unique<MockDataSource>());
	ASSERT_TRUE(WorldDataSourceManager::Instance().HasDataSource());

	WorldDataSourceManager::Instance().SetDataSource(nullptr);
	EXPECT_FALSE(WorldDataSourceManager::Instance().HasDataSource());
}

// Instance() always returns the same singleton object
TEST_F(WorldDataSourceManagerTest, Instance_ReturnsSameSingleton) {
	EXPECT_EQ(&WorldDataSourceManager::Instance(),
		&WorldDataSourceManager::Instance());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
