#include "engine/db/global_objects.h"
#include "engine/ui/color.h"
#include "backtrace.h"

#include <iostream>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <format>

#if defined(__clang__) || defined(__CYGWIN__)
#define HAS_TIME_ZONE 0
#else
#define HAS_TIME_ZONE 1
#include <ctime>
#endif

/**
* Файл персонального лога терь открывается один раз за каждый вход плеера в игру.
* Дескриптор открытого файла у плеера же и хранится (закрывает при con_close).
*/
// дескрипторы открытых файлов логов для сброса буфера при креше
std::list<FILE *> opened_files;

void pers_log(CharData *ch, const char *format, ...) {
	if (!ch) {
		log("NULL character resieved! (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}

	if (!format) {
		format = "SYSERR: pers_log received a NULL format.";
	}

	if (!ch->desc->pers_log) {
		char filename[128], name[64], *ptr;
		strcpy(name, GET_NAME(ch));
		for (ptr = name; *ptr; ptr++) {
			*ptr = LOWER(AtoL(*ptr));
		}
		sprintf(filename, "../log/perslog/%s.log", name);
		ch->desc->pers_log = fopen(filename, "a");
		if (!ch->desc->pers_log) {
			log("SYSERR: error open %s (%s %s %d)", filename, __FILE__, __func__, __LINE__);
			return;
		}
		opened_files.push_back(ch->desc->pers_log);
	}

	write_time(ch->desc->pers_log);
	va_list args;
	va_start(args, format);
	vfprintf(ch->desc->pers_log, format, args);
	va_end(args);
	fprintf(ch->desc->pers_log, "\n");
}

// Файл для вывода
FILE *logfile = nullptr;

std::size_t vlog_buffer(char *buffer, const std::size_t buffer_size, const char *format, va_list args) {
    std::size_t result = ~0u;
    int timestamp_length = -1;

#if HAS_TIME_ZONE
    // Реализация с использованием std::chrono::time_zone
    const std::chrono::time_zone* time_zone;
    try {
        time_zone = std::chrono::current_zone();
    }
    catch(const std::runtime_error&) {
        puts("SYSERR: failed to get local timezone.");
        return result;
    }

    const auto utc_now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    const auto now = std::chrono::zoned_time{time_zone, utc_now};
    const auto str = std::format("{:%Y-%m-%d %T}", now);
    timestamp_length = snprintf(buffer, buffer_size, "%s :: ", str.c_str());
#else
    // Реализация без std::chrono::time_zone, используем std::chrono::local_time
    const auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        const auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto* local_tm = std::localtime(&time_t_now);

        if (!local_tm) {
            puts("SYSERR: failed to get local time.");
            return result;
        }

        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        char time_str[64];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_tm);
        std::snprintf(time_str + std::strlen(time_str), sizeof(time_str) - std::strlen(time_str), ".%03ld", ms.count());
        const std::string str = time_str;
        timestamp_length = snprintf(buffer, buffer_size, "%s :: ", str.c_str());
    #endif

    if (0 > timestamp_length) {
        puts("SYSERR: failed to print timestamp inside log() function.");
        return result;
    }

    va_list args_copy;
    va_copy(args_copy, args);
    const int length = vsnprintf(buffer + timestamp_length, buffer_size - timestamp_length, format, args_copy);
    va_end(args_copy);

    if (0 > length) {
        puts("SYSERR: failed to print message contents inside log() function.");
        return result;
    }

    result = timestamp_length + length;
    if (buffer_size <= result) {
        const char truncated_suffix[] = "[TRUNCATED]";
        snprintf(buffer, buffer_size - sizeof(truncated_suffix), "%s", truncated_suffix);
    }

    return result;
}

void vlog(const char *format, va_list args, FILE *logfile) {
	if (!runtime_config.logging_enabled()) {
		return;
	}

	if (logfile == nullptr) {
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		puts("SYSERR: Using log() before stream was initialized!");
		return;
	}

	if (format == nullptr) {
		format = "SYSERR: log() received a NULL format.";
	}

	if (!runtime_config.output_thread()
		&& runtime_config.log_stderr().empty()) {
		const time_t ct = time(0);
		const char *time_s = asctime(localtime(&ct));

		fprintf(logfile, "%-15.15s :: ", time_s + 4);
		vfprintf(logfile, format, args);
		fprintf(logfile, "\n");
	} else {
		constexpr std::size_t BUFFER_SIZE = 4096;
		std::shared_ptr<char> buffer(new char[BUFFER_SIZE], [](char *p) { delete[] p; });
		const std::size_t length = vlog_buffer(buffer.get(), BUFFER_SIZE, format, args);
		GlobalObjects::output_thread().output(OutputThread::message_t{buffer, length, logfile});
	}
}

void vlog(const char *format, va_list args) {
	vlog(format, args, ::logfile);
}

void vlog(const EOutputStream steam, const char *format, va_list rargs) {
	va_list args;
	va_copy(args, rargs);

	const auto log = runtime_config.logs(steam).handle();
	vlog(format, args, log);

	va_end(args);
}

void log(std::string format) {
	log("%s", format.c_str());
} 

void log(FILE *log, const char *format, ...) {
	va_list args;
	va_start(args, format);
	vlog(format, args, log);
	va_end(args);
}

void log(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vlog(format, args, ::logfile);
	va_end(args);
}

void shop_log(const char *format, ...) {
	const char *filename = "../log/shop.log";

	FILE *file = fopen(filename, "a");
	if (!file) {
		log("SYSERR: can't open %s!", filename);
		return;
	}

	if (!format)
		format = "SYSERR: olc_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");

	fclose(file);
}

void olc_log(const char *format, ...) {
	const char *filename = "../log/olc.log";

	FILE *file = fopen(filename, "a");
	if (!file) {
		log("SYSERR: can't open %s!", filename);
		return;
	}

	if (!format)
		format = "SYSERR: olc_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");

	fclose(file);
}

void imm_log(const char *format, ...) {
	const char *filename = "../log/imm.log";

	FILE *file = fopen(filename, "a");
	if (!file) {
		log("SYSERR: can't open %s!", filename);
		return;
	}

	if (!format)
		format = "SYSERR: imm_log received a NULL format.";

	write_time(file);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");

	fclose(file);
}

void err_log(const char *format, ...) {
	static char buf_[kMaxRawInputLength];
	int cnt = snprintf(buf_, sizeof(buf_), "SYSERROR: ");

	va_list args;
	va_start(args, format);
	vsnprintf(buf_ + cnt, sizeof(buf_) - cnt, format, args);
	va_end(args);

	mudlog(buf_, LGH, kLvlImmortal, SYSLOG, true);
}

void ip_log(const char *ip) {
	FILE *iplog;

	if (!(iplog = fopen("../log/ip.log", "a"))) {
		log("SYSERR: ../log/ip.log");
		return;
	}

	fprintf(iplog, "%s\n", ip);
	fclose(iplog);
}

/*
 * Перегрузка функции mudlog, которая первым параметром вместо char *, принимает строку
 */
void mudlog(std::string str, LogMode type, int level, EOutputStream channel, bool file) {
	mudlog(str.c_str(), type, level, channel, file);
}

/*
* mudlog -- log mud messages to a file & to online imm's syslogs
* file - номер файла для вывода (0..NLOG), -1 не выводить в файл
*/
void mudlog(const char *str, LogMode type, int level, EOutputStream channel, bool file) {
	char tmpbuf[kMaxStringLength];
	DescriptorData *i;

	if (str == nullptr) {
		return;        // eh, oh well.
	}

	if (channel < 0 || channel > LAST_LOG) {
		return;
	}

	if (file) {
		const auto log = runtime_config.logs(channel).handle();
		::log(log, "%s", str);
	}

	if (level < 0) {
		return;
	}

	char time_buf[20];
	time_t ct = time(0);
	strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M:%S", localtime(&ct));
	snprintf(tmpbuf, sizeof(tmpbuf), "[%s][ %s ]\r\n", time_buf, str);
	for (i = descriptor_list; i; i = i->next) {
		if  (i->state != EConState::kPlaying || i->character->IsNpc())    // switch
			continue;
		if (GET_LOGS(i->character)[channel] < type && type != DEF)
			continue;
		if (type == DEF && GetRealLevel(i->character) < kLvlImmortal && !i->character->IsFlagged(EPrf::kCoderinfo))
			continue;
		if (GetRealLevel(i->character) < level && !i->character->IsFlagged(EPrf::kCoderinfo))
			continue;
		if (i->character->IsFlagged(EPlrFlag::kWriting) || i->character->IsFlagged(EPlrFlag::kFrozen))
			continue;

		SendMsgToChar(kColorGrn, i->character.get());
		SendMsgToChar(tmpbuf, i->character.get());
		SendMsgToChar(kColorNrm, i->character.get());
	}
}

void mudlog_python(const std::string &str, LogMode type, int level, const EOutputStream channel, int file) {
	mudlog(str.c_str(), type, level, channel, file);
}

void hexdump(FILE *file, const char *ptr, size_t buflen, const char *title/* = nullptr*/) {
	unsigned char *buf = (unsigned char *) ptr;
	size_t i, j;

	if (nullptr != title) {
		fprintf(file, "%s\n", title);
	}

	fprintf(file, "        | 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
	fprintf(file, "--------+------------------------------------------------\n");

	for (i = 0; i < buflen; i += 16) {
		fprintf(file, "%06zx: | ", i);
		for (j = 0; j < 16; j++) {
			if (i + j < buflen) {
				fprintf(file, "%02x ", buf[i + j]);
			} else {
				fprintf(file, "   ");
			}
		}

		fprintf(file, " ");
		for (j = 0; j < 16; j++) {
			if (i + j < buflen) {
				fprintf(file, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
			}
		}
		fprintf(file, "\n");
	}
}

// * Чтобы не дублировать создание даты в каждом виде лога.
void write_time(FILE *file) {
	char time_buf[20];
	time_t ct = time(0);
	strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M:%S", localtime(&ct));
	fprintf(file, "%s :: ", time_buf);
}

void Logger::operator()(const char *format, ...) {
	const size_t BUFFER_SIZE = 4096;

	va_list args;
	va_start(args, format);
	char buffer[BUFFER_SIZE];
	char *p = buffer;
	size_t free_space = BUFFER_SIZE;

	std::string prefix;
	for (const auto &part : m_prefix) {
		prefix += part;
	}
	const size_t plength = std::min(BUFFER_SIZE, prefix.length());
	strncpy(p, prefix.c_str(), plength - 1);
	free_space -= plength;
	p += plength;

	const size_t length = vsnprintf(p, free_space, format, args);
	va_end(args);

	if (free_space <= length) {
		const char truncated[] = "[TRUNCATED]\n";
		strncpy(buffer + BUFFER_SIZE - sizeof(truncated), truncated, (size_t)sizeof(truncated));
	}

	// Use the following line to redirect crafts log into syslog:
	if (false) {
		::log("%s", buffer);
	} else {
		// instead of output just onto console:
		const auto syslog_converter = runtime_config.syslog_converter();
		if (syslog_converter) {
			syslog_converter(buffer, static_cast<int>(length));
		}

		std::cerr << buffer;
	}
}

OutputThread::OutputThread(const std::size_t queue_size) :
	m_output_queue(queue_size),
	m_destroying(false) {
	m_thread.reset(new std::thread(&OutputThread::output_loop, this));
}

OutputThread::~OutputThread() {
	log("~OutputThread()");
	m_destroying = true;
	m_output_queue.destroy();

	m_thread->join();
}

namespace {
void output_message(const char *message, FILE *file) {
	fputs(message, file);
	fputs("\n", file);
}
}

void OutputThread::output_loop() {
	while (!m_destroying || !m_output_queue.empty()) {
		message_t message;
		if (m_output_queue.pop(message)) {
			if (!runtime_config.log_stderr().empty()) {
				const auto syslog_converter = runtime_config.syslog_converter();
				if (syslog_converter) {
					syslog_converter(message.text.get(), static_cast<int>(message.size));
				}
			}

			output_message(message.text.get(), message.channel);
		}
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
