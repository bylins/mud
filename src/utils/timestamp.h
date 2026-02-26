#ifndef BYLINS_UTILS_TIMESTAMP_H
#define BYLINS_UTILS_TIMESTAMP_H

#include <chrono>
#include <ctime>
#include <cstdio>
#include <string>

namespace utils {

/**
 * Current timestamp as "YYYY-MM-DD HH:MM:SS.mmm".
 * Used to prefix startup messages printed to stdout.
 */
inline std::string NowTs() {
	auto now = std::chrono::system_clock::now();
	auto t = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	struct tm tm_buf;
	localtime_r(&t, &tm_buf);
	char result[32];
	std::snprintf(result, sizeof(result), "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
		tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
		tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, (long long)ms.count());
	return result;
}

} // namespace utils

#endif // BYLINS_UTILS_TIMESTAMP_H

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
