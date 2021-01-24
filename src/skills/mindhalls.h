//
// Created by demid on 23.01.2021.
//

#ifndef BYLINS_MINDHALLS_H
#define BYLINS_MINDHALLS_H

#include "structs.h"


#include <array>

class CHAR_DATA;
class OBJ_DATA;
#include <core/json.hpp>

using namespace nlohmann;

namespace nsMindHalls {
    bool initMindHalls();
    enum HRESULT {MH_OK, MH_NOT_FOUND, MH_OVER_LIMIT, MH_ALREADY_STORED};
    void do_mindhalls(CHAR_DATA *ch, char *argument, int, int);
}

using namespace nsMindHalls;

// перечень рун, занесённых в чертоги
class MindHalls {
    using obj_sptr = std::shared_ptr<OBJ_DATA>;
private:
    std::unordered_map<int, obj_sptr> _runes;
    u_short _limits = 0;
    OBJ_DATA* operator[](obj_rnum runeId) const;
public:
    MindHalls(CHAR_DATA* owner, char* runes);
    HRESULT addRune(CHAR_DATA* owner,  char* runeName);
    HRESULT removeRune(char* runeName);
    void erase(u_short remort);
    std::string to_string();
    u_short getFreeStore();
};


#endif //BYLINS_MINDHALLS_H
