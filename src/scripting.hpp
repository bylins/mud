// Copyright (c) 2011 Posvist
// Part of Bylins http://www.mud.ru

#include "structs.h"

#include <memory>
#include <string>

namespace scripting
{
	const unsigned HEARTBEAT_PASSES = PASSES_PER_SEC/2;
	void init();
	void terminate();
	bool execute_player_command(CHAR_DATA* ch, const char* command, const char* args);
	void heartbeat();
}
// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
