//
// Created by Sventovit on 03.09.2024.
//

#include "structs/structs.h"

#include <unordered_map>

#ifndef BYLINS_SRC_GAME_MECHANICS_TOWN_GUARDS_H_
#define BYLINS_SRC_GAME_MECHANICS_TOWN_GUARDS_H_

namespace city_guards {

struct CityGuardian {
  int max_wars_allow{};
  bool agro_killers{};
  bool agro_all_agressors{};
  std::vector<ZoneVnum> agro_argressors_in_zones{};
};

using GuardianRosterType = std::unordered_map<MobVnum, CityGuardian>;

extern GuardianRosterType guardian_roster;

void LoadGuardians();
bool MustGuardianAttack(CharData *ch, CharData *vict);

} // namespace city_guards

#endif //BYLINS_SRC_GAME_MECHANICS_TOWN_GUARDS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
