#ifndef __COMMAND_SHUTDOWN_HPP__
#define __COMMAND_SHUTDOWN_HPP__

#include "administration/shutdown_parameters.h"

#include "engine/structs/structs.h"

class CharData;    // to avoid inclusion of "char.hpp"

namespace commands {
class Shutdown {
 public:
	Shutdown(CharData *character, const char *argument, ShutdownParameters &shutdown_parameters);

	bool parse_arguments();
	void reboot() const;
	void die() const;
	void pause() const;
	void shutdown_now() const;
	void schedule_shutdown() const;
	void cancel_shutdown() const;
	void execute() const;

 private:
	static char const *HELP_MESSAGE;

	CharData *m_character;
	const char *m_argument;
	int m_timeout;
	ShutdownParameters &m_shutdown_parameters;
	char m_argument_buffer[kMaxStringLength];
};
}

#endif // __COMMAND_SHUTDOWN_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
