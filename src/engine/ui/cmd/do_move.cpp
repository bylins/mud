/**
\file do_move.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 29.10.2025.
\brief Brief description.
\detail Detail description.
*/

#include "engine/entities/char_data.h"
#include "engine/core/char_movement.h"

void DoMove(CharData *ch, char * /*argument*/, int/* cmd*/, int subcmd) {
	PerformMove(ch, subcmd - 1, 0, true, nullptr);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
