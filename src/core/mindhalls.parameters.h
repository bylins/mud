//
// Created by demid on 24.01.2021.
//

#ifndef BYLINS_MINDHALLS_PARAMETERS_H
#define BYLINS_MINDHALLS_PARAMETERS_H

#include "boot.constants.hpp"
#include "structs.h"

#include <map>

class MindHallsParams {
private:
    const char* _paramFile = LIB_MISC"mindhalls.json";
    std::map<short, u_short> _runePower;
    short _hallsCapacity[MAX_REMORT-1];
public:
    MindHallsParams();
    bool load();
    int getRunePower(u_short runeVnum);
    int getHallsCapacity(u_short remort);
};


#endif //BYLINS_MINDHALLS_PARAMETERS_H
