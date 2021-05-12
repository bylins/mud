// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#ifndef PLAYER_RACES_HPP_INCLUDED
#define PLAYER_RACES_HPP_INCLUDED

#define RACE_UNDEFINED -1
#define KIN_UNDEFINED -1
#define RACE_NAME_UNDEFINED "RaceUndef"
#define KIN_NAME_UNDEFINED "KinUndef"
#define PLAYER_RACE_FILE "playerraces.xml"
#define RACE_MAIN_TAG "races"
#define PLAYER_RACE_ERROR_STR "...players races reading fail"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

#include <string>
#include <vector>

class PlayerKin;
class PlayerRace;

typedef std::shared_ptr<PlayerRace> PlayerRacePtr;
typedef std::vector<PlayerRacePtr> PlayerRaceListType;
typedef std::shared_ptr<PlayerKin> PlayerKinPtr;
typedef std::vector<PlayerKinPtr> PlayerKinListType;

class PlayerKin {
 public:
  PlayerRaceListType PlayerRaceList;

  PlayerKin();
  //	void ShowMenu(CHAR_DATA *ch);

  int KinNum;                    // Номер расы
  bool Enabled;               // Флаг доступности для создания персонажей
  std::string KinMenuStr;        // Название расы в меню выбора
  std::string KinItName;        // Название расы в среднем роде
  std::string KinHeName;      // Название расы в мужском роде
  std::string KinSheName;     // Название расы в женском роде
  std::string KinPluralName;  // Название расы в множественном числе
};

class PlayerRace {
 public:
  static PlayerKinListType PlayerKinList;

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
  //static void Load(const char *PathToFile);
  static void Load(pugi::xml_node XMLSRaceList);
  static PlayerKinPtr GetPlayerKin(int Kin);
  static PlayerRacePtr GetPlayerRace(int Kin, int Race);
  static std::vector<int> GetRaceFeatures(int Kin, int Race);
  static void GetKinNamesList(CHAR_DATA *ch);
  static bool FeatureCheck(int Kin, int Race, int Feat);
  static int GetKinNumByName(const std::string &KinName);
  static int GetRaceNumByName(int Kin, const std::string &RaceName);
  static std::string GetKinNameByNum(int KinNum, const ESex Sex);
  static std::string GetRaceNameByNum(int KinNum, int RaceNum, const ESex Sex);
  static std::string ShowRacesMenu(int KinNum);
  static int CheckRace(int KinNum, char *arg);
  static std::string ShowKinsMenu();
  static int CheckKin(char *arg);
  static std::vector<int> GetRaceBirthPlaces(int Kin, int Race);
  static int CheckBirthPlace(int Kin, int Race, char *arg);

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
