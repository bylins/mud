#include "utils_time.h"

#include "logger.h"

#include <sstream>
#include <iomanip>

namespace utils {

CSteppedProfiler::~CSteppedProfiler() {
	if (0 < m_steps.size()) {
		m_steps.back()->stop();
	}
	report();
}

void CSteppedProfiler::next_step(const std::string &step_name) {
	if (0 < m_steps.size()) {
		m_steps.back()->stop();
	}
	m_steps.push_back(step_t(new CExecutionStepProfiler(step_name)));
}

void CSteppedProfiler::report() const {
	FILE *flog;
	std::stringstream ss;
	if (m_time_probe == 0 || m_timer.delta().count() > m_time_probe) {
		const time_t ct = time(0);
		const char *time_s = asctime(localtime(&ct));

		ss << time_s;
		ss.seekp(-1, std::ios_base::end);
		ss << " Stepped profiler report for scope '" << m_scope_name << "'";
		if (0 == m_steps.size()) {
			ss << ": it took ";
		} else {
			ss << ":" << "\r\n";

			size_t number = 0;
			auto steps = m_steps;
			steps.sort(step_t__less);
			for (const auto &step : steps) {
				++number;
				ss << "  " << number << ". Step '" << step->name() << "' took "
				   << std::fixed << std::setprecision(6) << step->duration().count() << " second(s) ("
				   << std::fixed << std::setprecision(6) << (100 * step->duration().count() / m_timer.delta().count())
				   << "%)"
				   << ";" << "\r\n";
			}

			ss << "Whole scope took ";
		}
		ss << std::fixed << std::setprecision(6) << "time probe: " << m_time_probe << " total time: "<< m_timer.delta().count() << " second(s)\n";
// спам сислога, кому надо уберите
//		log("INFO: %s\n", ss.str().c_str());

		flog = fopen(LOAD_LOG_FOLDER LOAD_LOG_FILE, "a");
		if (!flog) {
				log("ERROR: Can't open file %s", LOAD_LOG_FOLDER LOAD_LOG_FILE);
			return;
		}

		fprintf(flog, "%s", ss.str().c_str());
		fclose(flog);
	}
}

void CSteppedProfiler::CExecutionStepProfiler::stop() {
	if (m_stopped) {
		return;
	}
	m_duration = m_timer.delta();
	m_stopped = true;
}
} // namespace utils

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
