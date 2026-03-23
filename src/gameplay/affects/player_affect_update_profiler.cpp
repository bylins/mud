#include "gameplay/affects/player_affect_update_profiler.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <deque>
#include <limits>
#include <string>
#include <vector>

#include "../subprojects/fmt/include/fmt/format.h"
#include "utils/logger.h"

namespace player_affect_update_profiler {
namespace {

constexpr std::size_t kWindowSize = 256;
constexpr std::size_t kReportEveryNRuns = 10;

constexpr std::array<const char *, static_cast<std::size_t>(Section::kNumSections)> kSectionNames{
	"total",
	"descriptor_loop",
	"tunnel_damage",
	"affect_loop",
	"poison",
	"domination",
	"set_abstinent",
	"memq_slots",
	"recalc"
};

constexpr std::array<const char *, static_cast<std::size_t>(Counter::kNumCounters)> kCounterNames{
	"players",
	"affected_players",
	"purged_players",
	"affect_entries",
	"poison_calls",
	"domination_affects",
	"abstinent_players",
	"recalc_players",
	"skipped_battledec"
};

class Distribution {
 public:
	void add(const double value) {
		m_sum += value;
		++m_count;
		m_min = std::min(m_min, value);
		m_max = std::max(m_max, value);
		m_window.push_back(value);
		if (m_window.size() > kWindowSize) {
			m_window.pop_front();
		}
	}

	bool empty() const {
		return m_count == 0;
	}

	double avg() const {
		return m_count ? m_sum / static_cast<double>(m_count) : 0.0;
	}

	double min() const {
		return empty() ? 0.0 : m_min;
	}

	double max() const {
		return empty() ? 0.0 : m_max;
	}

	double percentile(const double p) const {
		if (m_window.empty()) {
			return 0.0;
		}

		std::vector<double> sorted(m_window.begin(), m_window.end());
		std::sort(sorted.begin(), sorted.end());
		const auto index = static_cast<std::size_t>(std::ceil((sorted.size() - 1) * p));
		return sorted[std::min(index, sorted.size() - 1)];
	}

	std::size_t count() const {
		return m_count;
	}

 private:
	double m_sum = 0.0;
	std::size_t m_count = 0;
	double m_min = std::numeric_limits<double>::max();
	double m_max = 0.0;
	std::deque<double> m_window;
};

class Profiler {
 public:
	void add_run(const RunStats &stats) {
		++m_runs;
		for (std::size_t i = 0; i < m_sections.size(); ++i) {
			m_sections[i].add(stats.sections[i]);
		}
		for (std::size_t i = 0; i < m_counters.size(); ++i) {
			m_counters[i].add(static_cast<double>(stats.counters[i]));
		}

		if (m_runs % kReportEveryNRuns == 0) {
			report();
		}
	}

	std::string render() const {
		std::string out = fmt::format("Player affect update profile: runs={} window={}\r\n", m_runs, kWindowSize);
		if (m_runs == 0) {
			out += "No stats collected yet.\r\n";
			return out;
		}

		out += "\r\nSections:\r\n";
		for (std::size_t i = 0; i < m_sections.size(); ++i) {
			out += fmt::format(
				"  {:<16} avg={:.6f}s min={:.6f}s p50={:.6f}s p95={:.6f}s p99={:.6f}s max={:.6f}s n={}\r\n",
				kSectionNames[i],
				m_sections[i].avg(),
				m_sections[i].min(),
				m_sections[i].percentile(0.50),
				m_sections[i].percentile(0.95),
				m_sections[i].percentile(0.99),
				m_sections[i].max(),
				m_sections[i].count());
		}

		out += "\r\nCounters:\r\n";
		for (std::size_t i = 0; i < m_counters.size(); ++i) {
			out += fmt::format(
				"  {:<18} avg={:.2f} min={:.0f} p50={:.0f} p95={:.0f} p99={:.0f} max={:.0f} n={}\r\n",
				kCounterNames[i],
				m_counters[i].avg(),
				m_counters[i].min(),
				m_counters[i].percentile(0.50),
				m_counters[i].percentile(0.95),
				m_counters[i].percentile(0.99),
				m_counters[i].max(),
				m_counters[i].count());
		}

		return out;
	}

	void clear() {
		m_runs = 0;
		m_sections = {};
		m_counters = {};
	}

 private:
	void report() const {
		log("%s", fmt::format("player_affect_update profile: runs={} window={}", m_runs, kWindowSize).c_str());
		for (std::size_t i = 0; i < m_sections.size(); ++i) {
			log("%s",
				fmt::format(
					"player_affect_update profile section={} avg={:.6f}s min={:.6f}s p50={:.6f}s p95={:.6f}s p99={:.6f}s max={:.6f}s n={}",
					kSectionNames[i],
					m_sections[i].avg(),
					m_sections[i].min(),
					m_sections[i].percentile(0.50),
					m_sections[i].percentile(0.95),
					m_sections[i].percentile(0.99),
					m_sections[i].max(),
					m_sections[i].count()).c_str());
		}
		for (std::size_t i = 0; i < m_counters.size(); ++i) {
			log("%s",
				fmt::format(
					"player_affect_update profile counter={} avg={:.2f} min={:.0f} p50={:.0f} p95={:.0f} p99={:.0f} max={:.0f} n={}",
					kCounterNames[i],
					m_counters[i].avg(),
					m_counters[i].min(),
					m_counters[i].percentile(0.50),
					m_counters[i].percentile(0.95),
					m_counters[i].percentile(0.99),
					m_counters[i].max(),
					m_counters[i].count()).c_str());
		}
	}

	std::size_t m_runs = 0;
	std::array<Distribution, static_cast<std::size_t>(Section::kNumSections)> m_sections;
	std::array<Distribution, static_cast<std::size_t>(Counter::kNumCounters)> m_counters;
};

Profiler &profiler() {
	static Profiler instance;
	return instance;
}

} // namespace

void record_run(const RunStats &stats) {
	profiler().add_run(stats);
}

std::string render_report() {
	return profiler().render();
}

void clear() {
	profiler().clear();
}

} // namespace player_affect_update_profiler
