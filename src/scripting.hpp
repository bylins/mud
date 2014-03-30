// Copyright (c) 2011 Posvist
// Part of Bylins http://www.mud.ru

#ifndef SCRIPTING_HPP_INCLUDED
#define SCRIPTING_HPP_INCLUDED

#include <memory>
#include <string>


namespace scripting
{
const unsigned HEARTBEAT_PASSES = PASSES_PER_SEC/2;
void init();
void terminate();
bool execute_player_command(CHAR_DATA* ch, const char* command, const char* args);
void heartbeat();
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