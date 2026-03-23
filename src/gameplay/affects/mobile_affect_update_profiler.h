#ifndef BYLINS_SRC_GAMEPLAY_AFFECTS_MOBILE_AFFECT_UPDATE_PROFILER_H_
#define BYLINS_SRC_GAMEPLAY_AFFECTS_MOBILE_AFFECT_UPDATE_PROFILER_H_

#include <array>
#include <cstddef>
#include <string>

namespace mobile_affect_update_profiler {

enum class Section : std::size_t {
	kTotal = 0,
	kCopyAffectedMobs,
	kMobLoop,
	kAffectLoop,
	kProcessPoison,
	kRecalc,
	kTimedSkill,
	kTimedFeat,
	kDeathTrap,
	kStopFollower,
	kNumSections
};

enum class Counter : std::size_t {
	kMobs = 0,
	kRecalcMobs,
	kPurgedMobs,
	kEmptyMobs,
	kAffectEntries,
	kPoisonCalls,
	kTimedSkillEntries,
	kTimedFeatEntries,
	kDeathTrapChecks,
	kCharmStops,
	kNumCounters
};

struct RunStats {
	std::array<double, static_cast<std::size_t>(Section::kNumSections)> sections{};
	std::array<std::size_t, static_cast<std::size_t>(Counter::kNumCounters)> counters{};
};

void record_run(const RunStats &stats);
std::string render_report();
void clear();

} // namespace mobile_affect_update_profiler

#endif // BYLINS_SRC_GAMEPLAY_AFFECTS_MOBILE_AFFECT_UPDATE_PROFILER_H_
