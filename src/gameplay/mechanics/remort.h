//
// Created by Sventovit on 11.06.2026.
//

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

#ifndef MUD_SRC_GAMEPLAY_MECHANICS_REMORT_H_
#define MUD_SRC_GAMEPLAY_MECHANICS_REMORT_H_

#include <memory>

class CharData;

namespace remort {

void ProcessRemort(CharData *ch, char *argument, int subcmd);

} // namespace remort

int GetRealRemort(const CharData *ch);
int GetRealRemort(const std::shared_ptr<CharData> &ch);

#endif //MUD_SRC_GAMEPLAY_MECHANICS_REMORT_H_
