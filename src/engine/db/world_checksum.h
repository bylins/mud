// Part of Bylins http://www.mud.ru
// World checksum calculation for detecting changes during refactoring

#ifndef WORLD_CHECKSUM_H_
#define WORLD_CHECKSUM_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <map>

namespace WorldChecksum
{

struct ChecksumResult
{
	// Static data (loaded from disk)
	uint32_t zones;
	uint32_t rooms;
	uint32_t mobs;
	uint32_t objects;
	uint32_t triggers;
	uint32_t combined;

	// Runtime data (after initialization)
	uint32_t room_scripts;      // Actual Script objects attached to rooms
	uint32_t mob_scripts;        // Actual Script objects attached to mobs
	uint32_t obj_scripts;        // Actual Script objects attached to objects
	uint32_t door_rnums;         // Room door vnums converted to rnums
	uint32_t zone_rnums_mobs;    // zone_rnum field in mob_proto
	uint32_t zone_rnums_objects; // zone_rnum field in obj_proto
	uint32_t zone_cmd_rnums;     // Zone commands with vnum->rnum conversions
	uint32_t runtime_combined;   // Combined runtime checksum

	size_t zones_count;
	size_t rooms_count;
	size_t mobs_count;
	size_t objects_count;
	size_t triggers_count;
	
	size_t rooms_with_scripts;   // Count of rooms with actual scripts
	size_t mobs_with_scripts;    // Count of mobs with actual scripts
	size_t objects_with_scripts; // Count of objects with actual scripts
};

ChecksumResult Calculate();
void LogResult(const ChecksumResult &result);
void SaveDetailedChecksums(const char *filename, const ChecksumResult &checksums);

// Extended functions for debugging SQLite vs Legacy differences
// Save buffers with labeled fields for comparison
// Returns true on success, false on error (logged to syslog)
bool SaveDetailedBuffers(const char *dir);

// Load baseline checksums from file
// Returns map of "TYPE VNUM" -> checksum
std::map<std::string, uint32_t> LoadBaselineChecksums(const char *filename);

// Compare current state with baseline and print first N mismatches per type
// baseline_dir contains files from SaveDetailedBuffers()
void CompareWithBaseline(const char *baseline_dir, int max_mismatches_per_type = 3);

}

#endif // WORLD_CHECKSUM_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
