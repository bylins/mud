#ifndef CELEBRATES_HPP_INCLUDED
#define CELEBRATES_HPP_INCLUDED
#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

namespace Celebrates
{
typedef std::vector<int> TrigList;
struct ToLoad;

typedef boost::shared_ptr<ToLoad> LoadPtr;
typedef std::vector<LoadPtr> LoadList;

struct ToLoad
{
	TrigList triggers;
	int vnum;
	int max;
	LoadList objects;
};

typedef std::map<int, TrigList> AttachList; //mob vnum, списк триггеров
typedef std::map<int, AttachList> AttachZonList; //zone_num, список для аттача

struct CelebrateRoom
{
	int vnum;
	TrigList triggers;
	LoadList mobs;
	LoadList objects;
};

typedef boost::shared_ptr<CelebrateRoom> CelebrateRoomPtr;
typedef std::vector<CelebrateRoomPtr> CelebrateRoomsList;
typedef std::map<int, CelebrateRoomsList> CelebrateZonList;//номер зоны, список комнат

struct CelebrateData
{
	std::string name;
	CelebrateZonList rooms;
	AttachZonList mobsToAttach;
	AttachZonList objsToAttach;
};

typedef boost::shared_ptr<CelebrateData> CelebrateDataPtr;

struct CelebrateDay
{
	int start_at;
	int finish_at;
	CelebrateDataPtr celebrate;
};

typedef boost::shared_ptr<CelebrateDay> CelebrateDayPtr;
typedef std::map<int, CelebrateDayPtr> CelebrateList; //номер дня в году, праздник

CelebrateDataPtr get_mono_celebrate();
CelebrateDataPtr get_poly_celebrate();
CelebrateDataPtr get_real_celebrate();

std::string get_name_mono(int day);
std::string get_name_poly(int day);
std::string get_name_real(int day);

int get_mud_day();
int get_real_day();

void load();
};

#endif //CELEBRATES_HPP_INCLUDED