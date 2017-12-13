#include "logger.hpp"

#include "config.hpp"
#include "screen.h"
#include "comm.h"
#include "utils.h"
#include "char.hpp"

#include <iostream>

// Файл для вывода
FILE *logfile = nullptr;

void log(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vlog(format, args);
	va_end(args);
}

/*
* New variable argument log() function.  Works the same as the old for
* previously written code but is very nice for new code.
*/
void vlog(const char *format, va_list args)
{
	if (!runtime_config.logging_enabled())
	{
		return;
	}

	if (logfile == NULL)
	{
		puts("SYSERR: Using log() before stream was initialized!");
		return;
	}

	if (format == NULL)
	{
		format = "SYSERR: log() received a NULL format.";
	}

	time_t ct = time(0);
	char *time_s = asctime(localtime(&ct));

	time_s[strlen(time_s) - 1] = '\0';
	fprintf(logfile, "%-15.15s :: ", time_s + 4);

	if (!runtime_config.log_stderr().empty())
	{
		fprintf(stderr, "%-15.15s :: ", time_s + 4);
		const size_t BUFFER_SIZE = 4096;
		char buffer[BUFFER_SIZE];
		char* p = buffer;

		va_list args_copy;
		va_copy(args_copy, args);
		const size_t length = vsnprintf(p, BUFFER_SIZE, format, args_copy);
		va_end(args_copy);

		if (BUFFER_SIZE <= length)
		{
			fputs("TRUNCATED: ", stderr);
			p[BUFFER_SIZE - 1] = '\0';
		}

		const auto syslog_converter = runtime_config.syslog_converter();
		if (syslog_converter)
		{
			syslog_converter(buffer, static_cast<int>(length));
		}

		fputs(p, stderr);
	}

	vfprintf(logfile, format, args);
	fprintf(logfile, "\n");

	if (!runtime_config.log_stderr().empty())
	{
		fprintf(stderr, "\n");
	}
}

void vlog(const EOutputStream steam, const char* format, va_list rargs)
{
	va_list args;
	va_copy(args, rargs);

	const auto prev = logfile;
	logfile = runtime_config.logs(steam).handle();
	vlog(format, args);
	logfile = prev;

	va_end(args);
}

void shop_log(const char *format, ...)
{
	const char *filename = "../log/shop.log";

	FILE *file = fopen(filename, "a");
	if (!file)
	{
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

void olc_log(const char *format, ...)
{
	const char *filename = "../log/olc.log";

	FILE *file = fopen(filename, "a");
	if (!file)
	{
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

void imm_log(const char *format, ...)
{
	const char *filename = "../log/imm.log";

	FILE *file = fopen(filename, "a");
	if (!file)
	{
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

void err_log(const char *format, ...)
{
	static char buf_[MAX_RAW_INPUT_LENGTH];
	int cnt = snprintf(buf_, sizeof(buf_), "SYSERROR: ");

	va_list args;
	va_start(args, format);
	vsnprintf(buf_ + cnt, sizeof(buf_) - cnt, format, args);
	va_end(args);

	mudlog(buf_, DEF, LVL_IMMORT, SYSLOG, TRUE);
}

void ip_log(const char *ip)
{
	FILE *iplog;

	if (!(iplog = fopen("../log/ip.log", "a")))
	{
		log("SYSERR: ../log/ip.log");
		return;
	}

	fprintf(iplog, "%s\n", ip);
	fclose(iplog);
}

/*
* mudlog -- log mud messages to a file & to online imm's syslogs
* based on syslog by Fen Jul 3, 1992
* file - номер файла для вывода (0..NLOG), -1 не выводить в файл
*/
void mudlog(const char *str, int type, int level, EOutputStream channel, int file)
{
	char tmpbuf[MAX_STRING_LENGTH];
	DESCRIPTOR_DATA *i;

	if (str == NULL)
	{
		return;		// eh, oh well.
	}

	if (channel < 0 || channel > LAST_LOG)
	{
		return;
	}

	if (file)
	{
		logfile = runtime_config.logs(channel).handle();
		log("%s", str);
		logfile = runtime_config.logs(SYSLOG).handle();
	}

	if (level < 0)
	{
		return;
	}

	char time_buf[20];
	time_t ct = time(0);
	strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M:%S", localtime(&ct));
	snprintf(tmpbuf, sizeof(tmpbuf), "[%s][ %s ]\r\n", time_buf, str);
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) != CON_PLAYING || IS_NPC(i->character))	// switch
			continue;
		if (GET_LOGS(i->character)[channel] < type && type != DEF)
			continue;
		if (type == DEF && GET_LEVEL(i->character) < LVL_IMMORT && !PRF_FLAGGED(i->character, PRF_CODERINFO))
			continue;
		if (GET_LEVEL(i->character) < level && !PRF_FLAGGED(i->character, PRF_CODERINFO))
			continue;
		if (PLR_FLAGGED(i->character, PLR_WRITING) || PLR_FLAGGED(i->character, PLR_FROZEN))
			continue;

		send_to_char(CCGRN(i->character, C_NRM), i->character.get());
		send_to_char(tmpbuf, i->character.get());
		send_to_char(CCNRM(i->character, C_NRM), i->character.get());
	}
}

void mudlog_python(const std::string& str, int type, int level, const EOutputStream channel, int file)
{
	mudlog(str.c_str(), type, level, channel, file);
}

void hexdump(FILE* file, const char *ptr, size_t buflen, const char* title/* = nullptr*/)
{
	unsigned char *buf = (unsigned char*)ptr;
	size_t i, j;

	if (nullptr != title)
	{
		fprintf(file, "%s\n", title);
	}

	fprintf(file, "        | 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
	fprintf(file, "--------+------------------------------------------------\n");

	for (i = 0; i < buflen; i += 16)
	{
		fprintf(file, "%06zx: | ", i);
		for (j = 0; j < 16; j++)
		{
			if (i + j < buflen)
			{
				fprintf(file, "%02x ", buf[i + j]);
			}
			else
			{
				fprintf(file, "   ");
			}
		}

		fprintf(file, " ");
		for (j = 0; j < 16; j++)
		{
			if (i + j < buflen)
			{
				fprintf(file, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
			}
		}
		fprintf(file, "\n");
	}
}

// * Чтобы не дублировать создание даты в каждом виде лога.
void write_time(FILE *file)
{
	char time_buf[20];
	time_t ct = time(0);
	strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M:%S", localtime(&ct));
	fprintf(file, "%s :: ", time_buf);
}

void Logger::operator()(const char* format, ...)
{
	const size_t BUFFER_SIZE = 4096;

	va_list args;
	va_start(args, format);
	char buffer[BUFFER_SIZE];
	char* p = buffer;
	size_t free_space = BUFFER_SIZE;

	std::string prefix;
	for (const auto& part : m_prefix)
	{
		prefix += part;
	}
	const size_t plength = std::min(BUFFER_SIZE, prefix.length());
	strncpy(p, prefix.c_str(), plength);
	free_space -= plength;
	p += plength;

	const size_t length = vsnprintf(p, free_space, format, args);
	va_end(args);

	if (free_space <= length)
	{
		const char truncated[] = " ...<TRUNCATED>\n";
		strncpy(buffer + BUFFER_SIZE - sizeof(truncated), truncated, sizeof(truncated));
	}

	// Use the following line to redirect craft log into syslog:
	if (false)
	{
		::log("%s", buffer);
	}
	else
	{
		// instead of output just onto console:
		const auto syslog_converter = runtime_config.syslog_converter();
		if (syslog_converter)
		{
			syslog_converter(buffer, static_cast<int>(length));
		}

		std::cerr << buffer;
	}
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
