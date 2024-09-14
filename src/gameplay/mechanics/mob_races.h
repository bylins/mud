//
// Created by Sventovit on 04.09.2024.
//

#ifndef BYLINS_SRC_GAME_MECHANICS_MOB_RACES_H_
#define BYLINS_SRC_GAME_MECHANICS_MOB_RACES_H_

#include "engine/structs/structs.h"

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mob_races {

struct ingredient {
  int imtype;
  std::string imname;
  std::array<int, kMaxMobLevel + 1> prob; // вероятность загрузки для каждого уровня моба
};

class MobRace {
 public:
  MobRace();
  ~MobRace();
  std::string race_name;
  std::vector<ingredient> ingrlist;
};

using MobRacePtr = std::shared_ptr<MobRace>;
using MobRaceListType = std::map<int, MobRacePtr>;

extern MobRaceListType mobraces_list;

void LoadMobraces();

} // namespace mob_races

#endif //BYLINS_SRC_GAME_MECHANICS_MOB_RACES_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
