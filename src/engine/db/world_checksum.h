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
	uint32_t zones;
	uint32_t rooms;
	uint32_t mobs;
	uint32_t objects;
	uint32_t triggers;
	uint32_t combined;

	size_t zones_count;
	size_t rooms_count;
	size_t mobs_count;
	size_t objects_count;
	size_t triggers_count;
};

ChecksumResult Calculate();
void LogResult(const ChecksumResult &result);
void SaveDetailedChecksums(const char *filename);

// Extended functions for debugging SQLite vs Legacy differences
// Save buffers with labeled fields for comparison
void SaveDetailedBuffers(const char *dir);

// Load baseline checksums from file
// Returns map of "TYPE VNUM" -> checksum
std::map<std::string, uint32_t> LoadBaselineChecksums(const char *filename);

// Compare current state with baseline and print first N mismatches per type
// baseline_dir contains files from SaveDetailedBuffers()
void CompareWithBaseline(const char *baseline_dir, int max_mismatches_per_type = 3);

}

#endif // WORLD_CHECKSUM_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
