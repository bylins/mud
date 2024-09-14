//
// Created by Sventovit on 07.09.2024.
//

#ifndef BYLINS_SRC_GAME_MECHANICS_CITIES_H_
#define BYLINS_SRC_GAME_MECHANICS_CITIES_H_

#include "engine/structs/structs.h"

class CharData;

namespace cities {

extern std::string default_str_cities;

void CheckCityVisit(CharData *ch, RoomRnum room_rnum);
void LoadCities();
void DoCities(CharData *ch, char *, int, int);
bool IsCharInCity(CharData *ch);
std::size_t CountCities();

} // namespace cities

#endif //BYLINS_SRC_GAME_MECHANICS_CITIES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
