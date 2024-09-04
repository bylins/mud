#ifndef CELEBRATES_HPP_INCLUDED
#define CELEBRATES_HPP_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <memory>

class CharData;
class ObjData;

namespace celebrates {

extern const int kCleanPeriod;

using TrigList = std::vector<int>;
struct ToLoad;

using LoadPtr = std::shared_ptr<ToLoad>;
using LoadList = std::vector<LoadPtr>;

struct ToLoad {
  TrigList triggers;
  int vnum;
  int max;
  LoadList objects;
};

using AttachList = std::map<MobVnum, TrigList>;
using AttachZonList = std::map<ZoneVnum, AttachList>;

struct CelebrateRoom {
  int vnum;
  TrigList triggers;
  LoadList mobs;
  LoadList objects;
};

typedef std::shared_ptr<CelebrateRoom> CelebrateRoomPtr;
typedef std::vector<CelebrateRoomPtr> CelebrateRoomsList;
typedef std::map<ZoneVnum, CelebrateRoomsList> CelebrateZonList;

struct CelebrateData {
  CelebrateData() : is_clean(true) {};
  std::string name;
  bool is_clean;
  CelebrateZonList rooms{};
  AttachZonList mobsToAttach{};
  AttachZonList objsToAttach{};
};

using CelebrateDataPtr = std::shared_ptr<CelebrateData>;

struct CelebrateDay {
  CelebrateDay() : last(false), start_at(0), finish_at(24) {};
  bool last;
  int start_at;
  int finish_at;
  CelebrateDataPtr celebrate;
};

using CelebrateDayPtr = std::shared_ptr<CelebrateDay>;
using CelebrateList = std::map<int, CelebrateDayPtr>; //номер дня в году, праздник
using CelebrateMobs = std::map<long, CharData *>;
using CelebrateObjs = std::map<long, ObjData *>;

std::string GetNameMono(int day);
std::string GetNamePoly(int day);
std::string GetNameReal(int day);
int GetMudDay();
int GetRealDay();
void Load();
void Sanitize();
void RemoveFromObjLists(long uid);
void RemoveFromMobLists(long uid);
void ProcessCelebrates(int vnum);

};

#endif //CELEBRATES_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
