// Copyright (c) 2011 Posvist
// Part of Bylins http://www.mud.ru

#ifndef SCRIPTING_HPP_INCLUDED
#define SCRIPTING_HPP_INCLUDED

#include <memory>
#include <string>

namespace scripting
{
void init();
void terminate();
bool send_email(std::string smtp_server, std::string smtp_port, std::string smtp_login, 
					std::string smtp_pass, std::string addr_from, std::string addr_to, 
						std::string msg_text, std::string subject);
bool execute_player_command(CHAR_DATA* ch, const char* command, const char* args);
class Console_impl;

class Console
{
	public:
	Console(CHAR_DATA* ch);
		bool push(const char* line);
		std::string get_prompt();

	private:
		Console(const Console& );
		void operator=(const Console&);
		std::auto_ptr<Console_impl> _impl;
};

}

#endif