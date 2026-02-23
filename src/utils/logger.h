#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include "engine/core/config.h"
#include "engine/core/sysdep.h"

#include <list>
#include <string>
#include <atomic>
#include <thread>

extern FILE *logfile;
extern std::list<FILE *> opened_files;

void pers_log(CharData *ch, const char *format, ...) __attribute__((format(gnu_printf, 2, 3)));

void log(std::string format);
void log(const char *format, ...) __attribute__((format(gnu_printf, 1, 2)));
void vlog(const char *format, va_list args) __attribute__((format(gnu_printf, 1, 0)));
void vlog(const EOutputStream steam, const char *format, va_list args) __attribute__((format(gnu_printf, 2, 0)));
void shop_log(const char *format, ...) __attribute__((format(gnu_printf, 1, 2)));
void olc_log(const char *format, ...) __attribute__((format(gnu_printf, 1, 2)));
void imm_log(const char *format, ...) __attribute__((format(gnu_printf, 1, 2)));
void err_log(const char *format, ...) __attribute__((format(gnu_printf, 1, 2)));
void ip_log(const char *ip);

// defines for mudlog() //
enum LogMode : int {
	OFF = 0,
	CMP = 1,
	BRF = 2,
	NRM = 3,
	LGH = 4,
	DEF = 5
};

void mudlog(const char *str, LogMode type = CMP, int level = 31, EOutputStream channel = SYSLOG, bool file = true);
void mudlog(std::string str, LogMode type = CMP, int level = 31, EOutputStream channel = SYSLOG, bool file = true);

void mudlog_python(const std::string &str, LogMode type, int level, const EOutputStream channel, int file);

void hexdump(FILE *file, const char *ptr, size_t buflen, const char *title = nullptr);
inline void hexdump(const EOutputStream stream, const char *ptr, size_t buflen, const char *title = nullptr) {
	hexdump(runtime_config.logs(stream).handle(), ptr, buflen, title);
}

void write_time(FILE *file);

class AbstractLogger {
 public:
	~AbstractLogger() {}

	virtual void operator()(const char *format, ...) __attribute__((format(gnu_printf, 2, 3))) = 0;
};

class Logger : public AbstractLogger {
 public:
	class CPrefix {
	 public:
		CPrefix(Logger &logger, const char *prefix) : m_logger(logger) { logger.push_prefix(prefix); }
		~CPrefix() { m_logger.pop_prefix(); }
		void change_prefix(const char *new_prefix) { m_logger.change_prefix(new_prefix); }

	 private:
		Logger &m_logger;
	};

	void operator()(const char *format, ...) override
	__attribute__((
	format(gnu_printf,
	2, 3)));

 private:
	void push_prefix(const char *prefix) { m_prefix.push_back(prefix); }
	void change_prefix(const char *new_prefix);
	void pop_prefix();

	std::list<std::string> m_prefix;
};

inline void Logger::change_prefix(const char *new_prefix) {
	if (!m_prefix.empty()) {
		m_prefix.back() = new_prefix;
	}
}

inline void Logger::pop_prefix() {
	if (!m_prefix.empty()) {
		m_prefix.pop_back();
	}
}

class OutputThread {
 public:
	using message_t = struct {
		std::shared_ptr<char> text;
		std::size_t size;
		FILE *channel;
	};
	using output_queue_t = BlockingQueue<message_t>;

	OutputThread(const std::size_t queue_size);
	~OutputThread();

	void output(const message_t &message) { m_output_queue.push(message); }

 private:
	void output_loop();

	output_queue_t m_output_queue;
	std::shared_ptr<std::thread> m_thread;
	std::atomic_bool m_destroying;
};

#endif // __LOGGER_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
