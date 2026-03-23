#ifndef BYLINS_SRC_GAMEPLAY_AFFECTS_PLAYER_AFFECT_UPDATE_PROFILER_H_
#define BYLINS_SRC_GAMEPLAY_AFFECTS_PLAYER_AFFECT_UPDATE_PROFILER_H_

#include <array>
#include <cstddef>
#include <string>

namespace player_affect_update_profiler {

enum class Section : std::size_t {
	kTotal = 0,
	kDescriptorLoop,
	kTunnelDamage,
	kAffectLoop,
	kProcessPoison,
	kDominationAdjust,
	kSetAbstinent,
	kMemQSlots,
	kRecalc,
	kNumSections
};

enum class Counter : std::size_t {
	kPlayers = 0,
	kAffectedPlayers,
	kPurgedPlayers,
	kAffectEntries,
	kPoisonCalls,
	kDominationAffects,
	kAbstinentPlayers,
	kRecalcPlayers,
	kSkippedBattleDecAffects,
	kNumCounters
};

struct RunStats {
	std::array<double, static_cast<std::size_t>(Section::kNumSections)> sections{};
	std::array<std::size_t, static_cast<std::size_t>(Counter::kNumCounters)> counters{};
};

void record_run(const RunStats &stats);
std::string render_report();
void clear();

} // namespace player_affect_update_profiler

#endif // BYLINS_SRC_GAMEPLAY_AFFECTS_PLAYER_AFFECT_UPDATE_PROFILER_H_
