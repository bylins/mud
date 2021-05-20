#ifndef __SHUTDOWN_PARAMETERS_HPP__
#define __SHUTDOWN_PARAMETERS_HPP__

#include <ctime>

class ShutdownParameters
{
public:
	constexpr static int DEFAULT_REBOOT_TIMEOUT = 30;

	ShutdownParameters();

	void reboot() { reboot(DEFAULT_REBOOT_TIMEOUT); }
	void reboot(const int timeout);
	void shutdown(const int timeout);
	void shutdown_now();
	void cancel_shutdown();

	bool no_shutdown() const { return ES_DO_NOT_SHUTDOWN == circle_shutdown; }
	bool need_normal_shutdown() const { return ES_NORMAL_SHUTDOWN == circle_shutdown; }

	bool reboot_after_shutdown() const { return 0 != circle_reboot; }
	bool reboot_is_2() const { return 2 == circle_reboot; }

	const auto get_shutdown_timeout() const { return m_shutdown_time; }
	void set_shutdown_timeout(const int timeout) { m_shutdown_time = timeout; }

	const auto get_reboot_uptime() const { return m_reboot_uptime; }
	void set_reboot_uptime(const int reboot_uptime) { m_reboot_uptime = reboot_uptime; }

	void mark_boot_time() { m_boot_time = time(nullptr); }
	const auto get_boot_time() const { return m_boot_time; }

private:
	enum EShutdown
	{
		ES_DO_NOT_SHUTDOWN = 0,
		ES_SHUTDOWN_WITH_RENT_CRASH = 1,
		ES_NORMAL_SHUTDOWN = 2
	};

	EShutdown circle_shutdown;	// clean shutdown

	int circle_reboot;			// reboot the game after a shutdown
	int m_shutdown_time;		// reboot at this time
	int m_reboot_uptime;		// uptime until reboot in minutes
	time_t m_boot_time;			// time of mud boot
};

extern ShutdownParameters& shutdown_parameters;

#endif // __SHUTDOWN_PARAMETERS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
