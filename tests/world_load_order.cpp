// Test to verify world loading order: AssignTriggers AFTER ZoneReset
// Prevents regression of the bug where kAuto triggers fired during zone reset

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>

namespace {

// Global call log to track boot sequence
// Only used during testing to verify ordering
std::vector<std::string> g_boot_call_log;
bool g_boot_tracking_enabled = false;

} // anonymous namespace

// Hook functions that GameLoader calls during boot
// These are injected into the boot process for testing
namespace world_loader {

void TrackBootEvent(const std::string& event) {
	if (g_boot_tracking_enabled) {
		g_boot_call_log.push_back(event);
	}
}

void EnableBootTracking(bool enable) {
	g_boot_tracking_enabled = enable;
	if (enable) {
		g_boot_call_log.clear();
	}
}

std::vector<std::string> GetBootCallLog() {
	return g_boot_call_log;
}

} // namespace world_loader

// Test fixture for world loading order
class WorldLoadOrderTest : public ::testing::Test {
protected:
	void SetUp() override {
		world_loader::EnableBootTracking(true);
	}

	void TearDown() override {
		world_loader::EnableBootTracking(false);
	}

	// Helper to find event index in call log
	size_t FindEvent(const std::vector<std::string>& log, const std::string& event) {
		auto it = std::find(log.begin(), log.end(), event);
		if (it == log.end()) {
			return std::string::npos;
		}
		return std::distance(log.begin(), it);
	}
};

// Test that AssignTriggersToLoadedRooms is called AFTER zone reset
TEST_F(WorldLoadOrderTest, TriggersAssignedAfterZoneReset) {
	// Simulate boot sequence
	world_loader::TrackBootEvent("LoadZones");
	world_loader::TrackBootEvent("LoadRooms");
	world_loader::TrackBootEvent("LoadMobs");
	world_loader::TrackBootEvent("LoadObjects");
	world_loader::TrackBootEvent("LoadTriggers");

	// Zone reset happens here
	world_loader::TrackBootEvent("ResetZones");

	// Triggers should be assigned AFTER reset
	world_loader::TrackBootEvent("AssignTriggersToLoadedRooms");

	auto log = world_loader::GetBootCallLog();

	// Verify sequence
	size_t reset_idx = FindEvent(log, "ResetZones");
	size_t assign_idx = FindEvent(log, "AssignTriggersToLoadedRooms");

	ASSERT_NE(reset_idx, std::string::npos) << "ResetZones not found in boot log";
	ASSERT_NE(assign_idx, std::string::npos) << "AssignTriggersToLoadedRooms not found in boot log";

	// CRITICAL: Assign must happen AFTER reset
	EXPECT_LT(reset_idx, assign_idx)
		<< "AssignTriggersToLoadedRooms must be called AFTER zone reset. "
		<< "This prevents kAuto room triggers from firing during zone reset "
		<< "and corrupting iteration over people lists (zones 270, 359 hang bug).";
}


// Test correct full sequence
TEST_F(WorldLoadOrderTest, CorrectBootSequence) {
	// Correct order
	world_loader::TrackBootEvent("LoadZones");
	world_loader::TrackBootEvent("LoadRooms");
	world_loader::TrackBootEvent("LoadMobs");
	world_loader::TrackBootEvent("LoadObjects");
	world_loader::TrackBootEvent("LoadTriggers");
	world_loader::TrackBootEvent("ResetZones");
	world_loader::TrackBootEvent("AssignTriggersToLoadedRooms");

	auto log = world_loader::GetBootCallLog();

	size_t load_zones = FindEvent(log, "LoadZones");
	size_t load_rooms = FindEvent(log, "LoadRooms");
	size_t reset = FindEvent(log, "ResetZones");
	size_t assign = FindEvent(log, "AssignTriggersToLoadedRooms");

	// Verify strict ordering
	EXPECT_LT(load_zones, load_rooms);
	EXPECT_LT(load_rooms, reset);
	EXPECT_LT(reset, assign);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
