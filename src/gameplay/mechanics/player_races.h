// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#ifndef PLAYER_RACES_HPP_INCLUDED
#define PLAYER_RACES_HPP_INCLUDED

#include <string>
#include <vector>

#include "third_party_libs/pugixml/pugixml.h"

#include "engine/entities/entities_constants.h"
#include "engine/core/conf.h"
#include "engine/core/sysdep.h"
#include "engine/structs/structs.h"

// issue.kin-remove: the old "kin" (раса) dimension was a never-finished ethnicity layer (only kin 0
// was ever enabled). It has been removed; players now have only a "race" (род). The loader reads
// the races of the enabled kin(s) from playerraces.xml into a single flat list.

const int RACE_UNDEFINED = -1;
#define RACE_NAME_UNDEFINED "RaceUndef"
#define PLAYER_RACE_FILE "playerraces.xml"
#define RACE_MAIN_TAG "races"
#define PLAYER_RACE_ERROR_STR "...players races reading fail"

class PlayerRace;

typedef std::shared_ptr<PlayerRace> PlayerRacePtr;
typedef std::vector<PlayerRacePtr> PlayerRaceListType;

class PlayerRace {
 public:
	static PlayerRaceListType PlayerRaceList;

	void SetRaceNum(int Num) {
		_RaceNum = Num;
	};
	void SetEnabledFlag(bool Flag) {
		_Enabled = Flag;
	};

	void SetRaceMenuStr(std::string MenuStr) {
		_RaceMenuStr = MenuStr;
	};
	void SetRaceItName(std::string Name) {
		_RaceItName = Name;
	};
	void SetRaceHeName(std::string Name) {
		_RaceHeName = Name;
	};
	void SetRaceSheName(std::string Name) {
		_RaceSheName = Name;
	};
	void SetRacePluralName(std::string Name) {
		_RacePluralName = Name;
	};
	int GetRaceNum() {
		return _RaceNum;
	};
	std::string GetMenuStr() {
		return _RaceMenuStr;
	};

	void AddRaceFeature(int feat);
	void AddRaceBirthPlace(int id);
	static void Load(pugi::xml_node XMLSRaceList);
	static PlayerRacePtr GetPlayerRace(int Race);
	static std::vector<int> GetRaceFeatures(int Race);
	static bool FeatureCheck(int Race, int Feat);
	static int GetRaceNumByName(const std::string &RaceName);
	static std::string GetRaceNameByNum(int RaceNum, EGender Sex);
	static std::string ShowRacesMenu();
	static int CheckRace(char *arg);
	static std::vector<int> GetRaceBirthPlaces(int Race);
	static int CheckBirthPlace(int Race, char *arg);

 private:
	int _RaceNum;                // Номер _рода_
	bool _Enabled;               // Флаг доступности
	std::string _RaceMenuStr;   // Название рода в меню
	std::string _RaceItName;    // Название рода в среднем роде
	std::string _RaceHeName;    // Название рода в мужском роде
	std::string _RaceSheName;   // Название рода в женском роде
	std::string _RacePluralName;// Название рода в множественном числе
	std::vector<int> _RaceFeatureList; // Список родовых способностей
	std::vector<int> _RaceBirthPlaceList; // Список "мест рождений" новых персонажей данной расы

};

#endif // PLAYER_RACES_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
