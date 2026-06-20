/**
\file do_inventory.cpp - a part of the Bylins engine.
\brief The "inventory" player command. Display logic lives in the inventory mechanic.
*/

#include "do_inventory.h"
#include "gameplay/mechanics/inventory.h"

void DoInventory(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	ShowInventory(ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
