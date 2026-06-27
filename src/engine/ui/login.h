/**
 \file login.h - a part of the Bylins engine.
 \brief issue.interpreter-cleaning: the pre-game connection dialogue (login -> menu -> character
        creation), extracted from interpreter.cpp. Was the CircleMUD "nanny".
*/
#ifndef BYLINS_ENGINE_UI_LOGIN_H_
#define BYLINS_ENGINE_UI_LOGIN_H_

class DescriptorData;

// Drive one line of input for a connection that is NOT yet in the game (login / menus / char creation).
void ProcessLoginInput(DescriptorData *d, char *argument);   // was nanny()
// Move a fully created/authenticated character into the world.
void do_entergame(DescriptorData *d);
// Validate/normalise a proposed character name (0 = rejected). Used by login and do_set.
int _parse_name(char *argument, char *name);

#endif // BYLINS_ENGINE_UI_LOGIN_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
