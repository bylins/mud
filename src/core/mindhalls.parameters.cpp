//
// Created by demid on 24.01.2021.
//

#include "mindhalls.parameters.h"
#include "logger.hpp"

#include <iostream>
#include "json.hpp"
#include <bitset>

#define MAX_HALLS_VALUE 9999999

using namespace std;

using json = nlohmann::json;

MindHallsParams::MindHallsParams() {
    for (short i= 0; i < MAX_REMORT; ++i)
        _hallsCapacity[i]= -1;
};

bool MindHallsParams::load() {
    std::ifstream ifs(_paramFile, std::ios::binary);
    if (!ifs.is_open()) {
//        log("MindHallsParams::load: ошибка - не найден файл с параметрами");
        return false;
    }
    // define parser callback
    json::parser_callback_t cb = [](int, json::parse_event_t event, json & parsed) {
        if (event == json::parse_event_t::key and parsed == json("Comment")) {
            return false;
        }
        else {
            return true;
        }
    };
    json p;
    try {
         p = json::parse(ifs, cb);
    }
    catch (const std::exception&error){
        return false;
    }
    auto node = p.find("RunePower");
    if(node == p.end()) {
        log("MindHallsParams::load: ошибка - не найдена секция RunePower.");
        return false;
    }
    short vnum = 0;
    u_short value = 0;
    for (const auto& rp : node->items()) {
        vnum = (short)std::stoi(rp.key());
        value = (u_short)rp.value();
        _runePower.emplace(std::make_pair(vnum , value));
    }
    node = p.find("HallsCapacity");
    if(node == p.end()) {
        log("MindHallsParams::load: ошибка - не найдена секция RemortCapacity.");
        return false;
    }
    auto capNode = node.value().find("Capacity");
    if (capNode == node.value().end()) {
        log("MindHallsParams::load: ошибка - не найдена секция Capacity.");
        return false;
    }
    u_short i = 0;

    for (const auto& rp : capNode->items()) {
        if (i==MAX_REMORT){
          log("MindHallsParams::load: ошибка - массив RemortCapacity больше лимита ремортов.");
            return false;
        }
        _hallsCapacity[i] = rp.value();
        ++i;
    }
    if (i != MAX_REMORT) {
        log("MindHallsParams::load: ошибка - массив RemortCapacity не для всех ремортов.");
        return false;
    }
    return true;
}

// возвращает силу руны по vnum прототипа
// если руна не указана - возвращает MAX_HALLS_VALUE
int MindHallsParams::getRunePower(u_short runeVnum) {
    if (runeVnum < 1)
        return MAX_HALLS_VALUE;
    auto it = _runePower.find(runeVnum);
    if (it != _runePower.end())
        return it->second;
    return MAX_HALLS_VALUE;
}

// возвращает размер чертогов из параметров для указанного реморта
// если не найдено или неправильные параметры возвращает -1
int MindHallsParams::getHallsCapacity(u_short remort) {
    if (remort > MAX_REMORT) {
        mudlog("MindHallsParams::getHallsCapacity - невалидное значение реморта", BRF, -1, SYSLOG, 1);
        return -1;
    }
    return _hallsCapacity[remort];
}
