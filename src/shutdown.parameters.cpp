#include "shutdown.parameters.hpp"

#include "global.objects.hpp"
#include "sysdep.h"

ShutdownParameters::ShutdownParameters() :
	circle_shutdown(ES_DO_NOT_SHUTDOWN),
	circle_reboot(0),
	m_shutdown_time(0),
	m_reboot_uptime(0),
	m_boot_time(0)
{
}

void ShutdownParameters::reboot(const int timeout)
{
	circle_shutdown = ES_NORMAL_SHUTDOWN;
	circle_reboot = 1;
	m_shutdown_time = time(nullptr) + timeout;
}

void ShutdownParameters::shutdown(const int timeout)
{
	circle_shutdown = ES_NORMAL_SHUTDOWN;
	circle_reboot = 0;
	m_shutdown_time = time(nullptr) + timeout;
}

void ShutdownParameters::shutdown_now()
{
	circle_shutdown = ES_SHUTDOWN_WITH_RENT_CRASH;
	circle_reboot = 2;
	m_shutdown_time = 0;
}

void ShutdownParameters::cancel_shutdown()
{
	circle_shutdown = ES_DO_NOT_SHUTDOWN;
	circle_reboot = 0;
	m_shutdown_time = 0;
}

ShutdownParameters& shutdown_parameters = GlobalObjects::shutdown_parameters();

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
