#ifndef UTILS_TIMESTAMP_PRINT_H_
#define UTILS_TIMESTAMP_PRINT_H_

#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <iostream>
#include <string>

#ifdef WITH_OTEL
#include "logging/log_manager.h"
#endif

// Helper functions to print messages with timestamps to stdout/stderr
// Format: [YYYY-MM-DD HH:MM:SS] message
// With OTEL enabled: also sends to telemetry with log_stream=startup

inline void printf_timestamp(const char* format, ...) {
	char time_buf[32];
	char msg_buf[4096];
	time_t ct = time(0);
	struct tm* local_tm = localtime(&ct);
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", local_tm);

	va_list args;
	va_start(args, format);
	vsnprintf(msg_buf, sizeof(msg_buf), format, args);
	va_end(args);

	// Print to stdout with timestamp
	printf("[%s] %s", time_buf, msg_buf);
	fflush(stdout);

#ifdef WITH_OTEL
	// Also send to OTEL telemetry (startup messages)
	try {
		logging::LogManager::Info(msg_buf, {{"log_stream", "startup"}});
	} catch (const std::exception& e) {
		std::cerr << "WARNING: Failed to send startup message to OTEL: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "WARNING: Failed to send startup message to OTEL (unknown error)" << std::endl;
	}
#endif
}

inline void print_with_timestamp(const std::string& message) {
	char time_buf[32];
	time_t ct = time(0);
	struct tm* local_tm = localtime(&ct);
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", local_tm);
	std::cout << "[" << time_buf << "] " << message << std::endl;
	std::cout.flush();

#ifdef WITH_OTEL
	// Also send to OTEL telemetry (startup messages)
	try {
		logging::LogManager::Info(message, {{"log_stream", "startup"}});
	} catch (const std::exception& e) {
		std::cerr << "WARNING: Failed to send startup message to OTEL: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "WARNING: Failed to send startup message to OTEL (unknown error)" << std::endl;
	}
#endif
}

inline void print_error_with_timestamp(const std::string& message) {
	char time_buf[32];
	time_t ct = time(0);
	struct tm* local_tm = localtime(&ct);
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", local_tm);
	std::cerr << "[" << time_buf << "] " << message << std::endl;
	std::cerr.flush();

#ifdef WITH_OTEL
	// Also send to OTEL telemetry (startup errors)
	try {
		logging::LogManager::Error(message, {{"log_stream", "startup"}});
	} catch (const std::exception& e) {
		std::cerr << "WARNING: Failed to send startup error to OTEL: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "WARNING: Failed to send startup error to OTEL (unknown error)" << std::endl;
	}
#endif
}

#endif // UTILS_TIMESTAMP_PRINT_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
