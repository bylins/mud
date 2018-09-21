#ifndef CELEBRATES_HPP_INCLUDED
#define CELEBRATES_HPP_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <memory>

class CHAR_DATA;	// forward declaration to avoid inclusion of char.hpp and any dependencies of that header.
class OBJ_DATA;		// forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.

namespace Celebrates
{

const int CLEAN_PERIOD = 10;

typedef std::vector<int> TrigList;
struct ToLoad;

typedef std::shared_ptr<ToLoad> LoadPtr;
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

typedef std::shared_ptr<CelebrateRoom> CelebrateRoomPtr;
typedef std::vector<CelebrateRoomPtr> CelebrateRoomsList;
typedef std::map<int, CelebrateRoomsList> CelebrateZonList;//номер зоны, список комнат

struct CelebrateData
{
	CelebrateData() : is_clean(true) {};
	std::string name;
	bool is_clean;
	CelebrateZonList rooms;
	AttachZonList mobsToAttach;
	AttachZonList objsToAttach;
};

typedef std::shared_ptr<CelebrateData> CelebrateDataPtr;

struct CelebrateDay
{
	CelebrateDay() : last(false), start_at(0), finish_at(24) {};
	bool last;
	int start_at;
	int finish_at;
	CelebrateDataPtr celebrate;
};

typedef std::shared_ptr<CelebrateDay> CelebrateDayPtr;
typedef std::map<int, CelebrateDayPtr> CelebrateList; //номер дня в году, праздник
typedef std::map<long, CHAR_DATA *> CelebrateMobs;
typedef std::map<long, OBJ_DATA *> CelebrateObjs;

CelebrateDataPtr get_mono_celebrate();
CelebrateDataPtr get_poly_celebrate();
CelebrateDataPtr get_real_celebrate();

std::string get_name_mono(int day);
std::string get_name_poly(int day);
std::string get_name_real(int day);

int get_mud_day();
int get_real_day();

void load();
void sanitize();

void add_mob_to_attach_list(long, CHAR_DATA *);
void add_mob_to_load_list(long, CHAR_DATA *);
void add_obj_to_attach_list(long, OBJ_DATA *);
void add_obj_to_load_list(long, OBJ_DATA *);

void remove_from_obj_lists(long uid);
void remove_from_mob_lists(long uid);

};

#endif //CELEBRATES_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
