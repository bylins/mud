#include "command.shutdown.hpp"

#include "logger.hpp"
#include "chars/character.h"
#include "comm.h"
#include "interpreter.h"
#include "utils.h"
#include "structs.h"

namespace commands
{
	Shutdown::Shutdown(CHAR_DATA* character, const char* argument, ShutdownParameters& shutdown_parameters):
		m_character(character),
		m_argument(argument),
		m_timeout(0),
		m_shutdown_parameters(shutdown_parameters)
	{
	}

	bool Shutdown::parse_arguments()
{
		char buffer[MAX_STRING_LENGTH];

		two_arguments(m_argument, m_argument_buffer, buffer);

		if (!*m_argument_buffer)
		{
			send_to_char(HELP_MESSAGE, m_character);
			return false;
		}

		if (*buffer)
		{
			m_timeout = atoi(buffer);
		}

		return true;
	}

	void Shutdown::reboot() const
	{
		const auto timeout = MAX(30, m_timeout);
		sprintf(buf, "[ПЕРЕЗАГРУЗКА через %d %s]\r\n", timeout, desc_count(timeout, WHAT_SEC));
		send_to_all(buf);
		log("(GC) Reboot by %s.", GET_NAME(m_character));
		imm_log("Reboot by %s.", GET_NAME(m_character));
		touch(FASTBOOT_FILE);
		m_shutdown_parameters.reboot(timeout);
	}

	void Shutdown::die() const
	{
		const auto timeout = MAX(30, m_timeout);
		sprintf(buf, "[ОСТАНОВКА через %d %s]\r\n", timeout, desc_count(timeout, WHAT_SEC));
		send_to_all(buf);
		log("(GC) Shutdown die by %s.", GET_NAME(m_character));
		imm_log("Shutdown die by %s.", GET_NAME(m_character));
		touch(KILLSCRIPT_FILE);
		m_shutdown_parameters.shutdown(m_timeout);
	}

	void Shutdown::pause() const
	{
		const auto timeout = MAX(30, m_timeout);
		sprintf(buf, "[ОСТАНОВКА через %d %s]\r\n", timeout, desc_count(timeout, WHAT_SEC));
		send_to_all(buf);
		log("(GC) Shutdown pause by %s.", GET_NAME(m_character));
		imm_log("Shutdown pause by %s.", GET_NAME(m_character));
		touch(PAUSE_FILE);
		m_shutdown_parameters.shutdown(m_timeout);
	}

	void Shutdown::shutdown_now() const
	{
		sprintf(buf, "(GC) Shutdown NOW by %s.", GET_NAME(m_character));
		log("%s", buf);
		imm_log("Shutdown NOW by %s.", GET_NAME(m_character));
		send_to_all("ПЕРЕЗАГРУЗКА.. Вернетесь через пару минут.\r\n");
		m_shutdown_parameters.shutdown_now();
	}

	void Shutdown::schedule_shutdown() const
	{
		const auto boot_time = m_shutdown_parameters.get_boot_time();
		if (m_timeout <= 0)
		{
			const auto tmp_time = boot_time + (time_t)(60 * m_shutdown_parameters.get_reboot_uptime());
			send_to_char(m_character, "Сервер будет автоматически перезагружен в %s\r\n", rustime(localtime(&tmp_time)));
			return;
		}

		const auto uptime = time(0) - boot_time;
		m_shutdown_parameters.set_reboot_uptime(uptime/60 + m_timeout);
		m_shutdown_parameters.cancel_shutdown();

		const auto tmp_time = boot_time + (time_t)(60 * m_shutdown_parameters.get_reboot_uptime());
		send_to_char(m_character, "Сервер будет автоматически перезагружен в %s\r\n", rustime(localtime(&tmp_time)));
		log("(GC) Shutdown scheduled by %s.", GET_NAME(m_character));
		imm_log("Shutdown scheduled by %s.", GET_NAME(m_character));
	}

	void Shutdown::cancel_shutdown() const
	{
		log("(GC) Shutdown canceled by %s.", GET_NAME(m_character));
		imm_log("Shutdown canceled by %s.", GET_NAME(m_character));
		send_to_all("ПЕРЕЗАГРУЗКА ОТМЕНЕНА.\r\n");
		m_shutdown_parameters.cancel_shutdown();
	}

	void Shutdown::execute() const
	{
		void (Shutdown::*handler)(void) const = nullptr;

		if (!str_cmp(m_argument_buffer, "reboot") && 0 < m_timeout)
		{
			handler = &Shutdown::reboot;
		}
		else if (!str_cmp(m_argument_buffer, "die") && 0 < m_timeout)
		{
			handler = &Shutdown::die;
		}
		else if (!str_cmp(m_argument_buffer, "pause") && 0 < m_timeout)
		{
			handler = &Shutdown::pause;
		}
		else if (!str_cmp(m_argument_buffer, "now"))
		{
			handler = &Shutdown::shutdown_now;
		}
		else if (!str_cmp(m_argument_buffer, "schedule"))
		{
			handler = &Shutdown::schedule_shutdown;
		}
		else if (!str_cmp(m_argument_buffer, "cancel"))
		{
			handler = &Shutdown::cancel_shutdown;
		}

		if (nullptr != handler)
		{
			(this->*handler)();
		}
		else
		{
			send_to_char(HELP_MESSAGE, m_character);
		}
	}

	char const *Shutdown::HELP_MESSAGE =
		"Формат команды shutdown [reboot|die|pause] кол-во секунд\r\n"
		"               shutdown schedule кол-во минут\r\n"
		"               shutdown now|cancel|schedule";
}

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
