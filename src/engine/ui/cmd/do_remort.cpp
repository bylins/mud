//
// Created by Sventovit on 03.09.2024.
//

#include "gameplay/core/remort.h"

void DoRemort(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	remort::ProcessRemort(ch, argument, subcmd);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
