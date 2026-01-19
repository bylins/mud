// Part of Bylins http://www.mud.ru
// World checksum calculation for detecting changes during refactoring

#ifndef WORLD_CHECKSUM_H_
#define WORLD_CHECKSUM_H_

#include <cstddef>
#include <cstdint>

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

}

#endif // WORLD_CHECKSUM_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
