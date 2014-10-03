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
// Events aka События
// умер игрок
void on_pc_dead(CHAR_DATA* ch, CHAR_DATA* killer, OBJ_DATA* corpse);
// умер нпс
void on_npc_dead(CHAR_DATA* ch, CHAR_DATA* killer, OBJ_DATA* corpse);
// чар вошел в комнату
void on_enter_pc(CHAR_DATA* ch);
// чар что-то сказал
void on_say_pc(CHAR_DATA* ch, char *argument);
// чар ввел какую-то команду
void on_input_command(CHAR_DATA* ch, char *argument, int cmd, int subcmd);
// чар дал предмет мобу
void on_give_obj_to_mob(CHAR_DATA* ch, CHAR_DATA *mob, OBJ_DATA* obj);


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

/* engine events:

	pc_dead: ch, killer, corpse
	npc_dead: ch, killer, corpse
*/

#define EVENT_PC_DEAD "pc_dead"
#define EVENT_NPC_DEAD "npc_dead"
}

#endif