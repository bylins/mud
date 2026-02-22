#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include "engine/structs/blocking_queue.h"
#include "gameplay/mechanics/birthplaces.h"
#include "engine/structs/structs.h"
#include "engine/structs/meta_enum.h"

#include <cstdio>
#include <array>
#include <functional>

/*
* Should the game automatically save people?  (i.e., save player data
* every 4 kills (on average), and Crash-save as defined below.  This
* option has an added meaning past bpl13.  If auto_save is YES, then
* the 'save' command will be disabled to prevent item duplication via
* game crashes.
*/
constexpr bool AUTO_SAVE = true;

/*
* if auto_save (above) is yes, how often (in minutes) should the MUD
* Crash-save people's objects?   Also, this number indicates how often
* the MUD will Crash-save players' houses.
*/
constexpr int AUTOSAVE_TIME = 5;

namespace pugi {
class xml_node;
}

enum EOutputStream {
	SYSLOG = 0,
	ERRLOG = 1,
	IMLOG = 2,
	MSDP_LOG = 3,
	MONEY_LOG = 4,
	LAST_LOG = MONEY_LOG
};

template<>
EOutputStream ITEM_BY_NAME<EOutputStream>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<EOutputStream>(const EOutputStream spell);

/* PCCleanCriteria структура которая определяет через какой время
   неактивности будет удален чар
*/
struct PCCleanCriteria {
	int level = 0;	// max уровень для этого временного лимита //
	int days = 0;	// временной лимит в днях        //
};

extern struct PCCleanCriteria pclean_criteria[];

class CLogInfo {
 private:
	CLogInfo() {}
	CLogInfo &operator=(const CLogInfo &);

 public:
	static constexpr umask_t UMASK_DEFAULT = -1;

	enum EBuffered {
		EB_NO = _IONBF,
		EB_LINE = _IOLBF,
		EB_FULL = _IOFBF
	};

	enum EMode {
		EM_REWRITE,
		EM_APPEND
	};

	CLogInfo(const char *filename, const char *human_readable_name) :
		m_handle(nullptr),
		m_filename(filename),
		m_title(human_readable_name),
		m_buffered(EB_LINE),
		m_mode(EM_REWRITE),
		m_umask(UMASK_DEFAULT) {
	}
	CLogInfo(const CLogInfo &from) :
		m_handle(nullptr),
		m_filename(from.m_filename),
		m_title(from.m_title),
		m_buffered(from.m_buffered),
		m_mode(from.m_mode),
		m_umask(from.m_umask) {
	}

	bool open();

	void buffered(const EBuffered _) { m_buffered = _; }
	void handle(FILE *_) { m_handle = _; }
	void filename(const char *_) { m_filename = _; }
	void mode(const EMode _) { m_mode = _; }
	void umask(const int _) { m_umask = _; }

	auto buffered() const { return m_buffered; }
	const std::string &filename() const { return m_filename; }
	const std::string &title() const { return m_title; }
	FILE *handle() const { return m_handle; }
	auto mode() const { return m_mode; }
	auto umask() const { return m_umask; }

 private:
	static constexpr size_t BUFFER_SIZE = 1024;

	FILE *m_handle;
	std::string m_filename;
	std::string m_title;
	EBuffered m_buffered;
	EMode m_mode;
	umask_t m_umask;

	char m_buffer[BUFFER_SIZE];
};

template<>
CLogInfo::EBuffered ITEM_BY_NAME<CLogInfo::EBuffered>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<CLogInfo::EBuffered>(const CLogInfo::EBuffered mode);

template<>
CLogInfo::EMode ITEM_BY_NAME<CLogInfo::EMode>(const std::string &name);
template<>
const std::string &NAME_BY_ITEM<CLogInfo::EMode>(const CLogInfo::EMode mode);

class RuntimeConfiguration {
 public:
	using logs_t = std::array<CLogInfo, 1 + LAST_LOG>;

	class StatisticsConfiguration {
	 public:
		static constexpr unsigned short DEFAULT_PORT = 8089;

		StatisticsConfiguration(const std::string &host = "", const unsigned short port = DEFAULT_PORT)
			: m_host(host), m_port(port) {}

		const auto &host() const { return m_host; }
		const auto &port() const { return m_port; }

	 private:
		std::string m_host;
		unsigned short m_port;
	};

	static constexpr std::size_t OUTPUT_QUEUE_SIZE = 256;

	RuntimeConfiguration();

	void load(const char *filename = CONFIGURATION_FILE_NAME) { load_from_file(filename); }
	bool open_log(const EOutputStream stream);
	const CLogInfo &logs(EOutputStream id) { return m_logs[static_cast<size_t>(id)]; }
	void handle(const EOutputStream stream, FILE *handle);
	const std::string &log_stderr() { return m_log_stderr; }
	const std::string &log_dir() const { return m_log_dir; }
	auto output_thread() const { return m_output_thread; }
	auto output_queue_size() const { return m_output_queue_size; }

	void setup_logs();
	auto syslog_converter() const { return m_syslog_converter; }

	void enable_logging() { m_logging_enabled = true; }
	void disable_logging() { m_logging_enabled = false; }
	bool logging_enabled() const { return m_logging_enabled; }

	auto msdp_disabled() const { return m_msdp_disabled; }
	auto msdp_debug() const { return m_msdp_debug; }

	const auto &changelog_file_name() const { return m_changelog_file_name; }
	const auto &changelog_format() const { return m_changelog_format; }

	const auto &external_reboot_trigger_file_name() const { return m_external_reboot_trigger_file_name; }

	const auto &statistics() const { return m_statistics; }

	size_t yaml_threads() const { return m_yaml_threads; }

#ifdef ENABLE_ADMIN_API
	const auto &admin_socket_path() const { return m_admin_socket_path; }
	bool admin_api_enabled() const { return m_admin_api_enabled; }
	bool admin_require_auth() const { return m_admin_require_auth; }
	int admin_max_connections() const { return m_admin_max_connections; }
#endif

 private:
	static const char *CONFIGURATION_FILE_NAME;

	using converter_t = std::function<void(char *, int)>;

	RuntimeConfiguration(const RuntimeConfiguration &);
	RuntimeConfiguration &operator=(const RuntimeConfiguration &);

	void load_from_file(const char *filename);
	void load_stream_config(CLogInfo &log, const pugi::xml_node *node);
	void setup_converters();
	void load_logging_configuration(const pugi::xml_node *root);
	void load_features_configuration(const pugi::xml_node *root);
	void load_msdp_configuration(const pugi::xml_node *msdp);
	void load_boards_configuration(const pugi::xml_node *root);
	void load_external_triggers(const pugi::xml_node *root);
	void load_statistics_configuration(const pugi::xml_node *root);
	void load_world_loader_configuration(const pugi::xml_node *root);
#ifdef ENABLE_ADMIN_API
	void load_admin_api_configuration(const pugi::xml_node *root);
#endif

	logs_t m_logs;
	std::string m_log_stderr;
	std::string m_log_dir;
	bool m_output_thread;
	std::size_t m_output_queue_size;
	converter_t m_syslog_converter;
	bool m_logging_enabled;
	bool m_msdp_disabled;
	bool m_msdp_debug;
	std::string m_changelog_file_name;
	std::string m_changelog_format;
	std::string m_external_reboot_trigger_file_name;

	StatisticsConfiguration m_statistics;

	size_t m_yaml_threads;

#ifdef ENABLE_ADMIN_API
	std::string m_admin_socket_path{"admin_api.sock"};
	bool m_admin_api_enabled{false};
	bool m_admin_require_auth{true};
	int m_admin_max_connections{1};
#endif
};

extern RuntimeConfiguration runtime_config;

int calc_loadroom(const CharData *ch, int bplace_mode = kBirthplaceUndefined);

extern char const *OK;
extern char const *NOPERSON;
extern char const *NOEFFECT;
extern const char *nothing_string;

#endif // __CONFIG_HPP__
