#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include "structs.h"

#include <cstdio>
#include <array>

namespace pugi
{
	class xml_node;
}

enum EOutputStream
{
	SYSLOG = 0,
	ERRLOG = 1,
	IMLOG = 2,
	LAST_LOG = IMLOG
};

class CLogInfo
{
private:
	CLogInfo() {}
	CLogInfo& operator=(const CLogInfo&);

public:
	enum EBuffered
	{
		EB_NO = _IONBF,
		EB_LINE = _IOLBF,
		EB_FULL = _IOFBF
	};

	CLogInfo(const char* filename, const char* human_readable_name) :
		m_handle(nullptr),
		m_filename(filename),
		m_human_readable_name(human_readable_name),
		m_buffered(EB_LINE)
	{
	}
	CLogInfo(const CLogInfo& from) :
		m_handle(nullptr),
		m_filename(from.m_filename),
		m_human_readable_name(from.m_human_readable_name),
		m_buffered(from.m_buffered)
	{
	}

	bool open();

	void buffered(const EBuffered _) { m_buffered = _; }
	void handle(FILE* _) { m_handle = _; }
	void filename(const char* _) { m_filename = _; }

	const std::string& filename() const { return m_filename; }
	const std::string& title() const { return m_human_readable_name; }
	auto handle() const { return m_handle; }

private:
	static constexpr size_t BUFFER_SIZE = 1024;

	FILE *m_handle;
	std::string m_filename;
	std::string m_human_readable_name;
	EBuffered m_buffered;

	char m_buffer[BUFFER_SIZE];
};

template <> CLogInfo::EBuffered ITEM_BY_NAME<CLogInfo::EBuffered>(const std::string& name);
template <> const std::string& NAME_BY_ITEM<CLogInfo::EBuffered>(const CLogInfo::EBuffered spell);

class runtime_config
{
private:
	static const char* CONFIGURATION_FILE_NAME;

	using logs_t = std::array<CLogInfo, 1 + LAST_LOG>;

	runtime_config();
	runtime_config(const runtime_config&);
	runtime_config& operator=(const runtime_config&);

	static void load_from_file(const char* filename);
	static void load_stream_config(CLogInfo& log, const pugi::xml_node* node);

	static logs_t m_logs;
	static std::string m_log_stderr;

public:
	static void load(const char* filename = CONFIGURATION_FILE_NAME) { load_from_file(filename); }
	static bool open_log(const EOutputStream stream);
	static const CLogInfo& logs(EOutputStream id) { return m_logs[static_cast<size_t>(id)]; }
	static void handle(const EOutputStream stream, FILE * handle);
	static const std::string& log_stderr() { return m_log_stderr; }
};

#endif // __CONFIG_HPP__