//
// Created by demid on 23.01.2021.
//

#ifndef BYLINS_MINDHALLS_H
#define BYLINS_MINDHALLS_H

#include "structs.h"
#include "obj.hpp"


#include <array>

class CHAR_DATA;
#include <core/json.hpp>

using namespace nlohmann;

namespace nsMindHalls {
    bool initMindHalls();
    enum HRESULT {MH_OK, MH_NOT_FOUND, MH_OVER_LIMIT, MH_ALREADY_STORED};
    void do_mindhalls(CHAR_DATA *ch, char *argument, int, int);
    bool save(CHAR_DATA* owner, FILE* playerFile);
    bool load(CHAR_DATA* owner, char* line);
}

using namespace nsMindHalls;

// перечень рун, занесённых в чертоги
class MindHalls {
private:
    std::unordered_map<int, CObjectPrototype::shared_ptr> _runes;
    u_short _limits = 0;
    OBJ_DATA* operator[](obj_rnum runeId) const;
public:
    MindHalls();
    HRESULT addRune(CHAR_DATA* owner,  char* runeName);
    HRESULT removeRune(char* runeName);
    void erase(u_short remort);
    std::string to_string();
    bool load(CHAR_DATA* owner,  char* runes);
    u_short getFreeStore();
    std::string getContents(CHAR_DATA* owner);
};


#endif //BYLINS_MINDHALLS_H
