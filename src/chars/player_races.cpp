// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include <algorithm>
#include <sstream>

#include "conf.h"
#include "sysdep.h"
#include "utils.h"
#include "comm.h"
#include "player_races.hpp"
#include "pugixml.hpp"

PlayerKinListType PlayerRace::PlayerKinList;

void LoadRace(pugi::xml_node RaceNode, PlayerKinPtr KinPtr);
void LoadKin(pugi::xml_node KinNode);

PlayerKin::PlayerKin() {
//Create kin
};

//Создаем новый _род_ и распихиваем по его полям значения из файла
void LoadRace(pugi::xml_node RaceNode, PlayerKinPtr KinPtr) {
  pugi::xml_node CurNode;
  PlayerRacePtr TmpRace(new PlayerRace);

  TmpRace->SetRaceNum(RaceNode.attribute("racenum").as_int());
  TmpRace->SetEnabledFlag(RaceNode.attribute("enabled").as_bool());
  TmpRace->SetRaceMenuStr(RaceNode.child("menu").child_value());
  CurNode = RaceNode.child("racename");
  TmpRace->SetRaceItName(CurNode.child("itname").child_value());
  TmpRace->SetRaceHeName(CurNode.child("hename").child_value());
  TmpRace->SetRaceSheName(CurNode.child("shename").child_value());
  TmpRace->SetRacePluralName(CurNode.child("pluralname").child_value());
  //Add race features
  for (CurNode = RaceNode.child("feature"); CurNode; CurNode = CurNode.next_sibling("feature")) {
    TmpRace->AddRaceFeature(CurNode.attribute("featnum").as_int());
  }
  //Добавляем родовые места "появления на свет" новых персонажей
  for (CurNode = RaceNode.child("birthplace"); CurNode; CurNode = CurNode.next_sibling("birthplace")) {
    TmpRace->AddRaceBirthPlace(CurNode.attribute("id").as_int());
  }
  //Add new race in list
  KinPtr->PlayerRaceList.push_back(TmpRace);
}

//Создаем новую расу и заполняем ее поля значениями из файлда
void LoadKin(pugi::xml_node KinNode) {
  pugi::xml_node CurNode;
  PlayerKinPtr TmpKin(new PlayerKin);

  //Parse kin's parameters
  TmpKin->KinNum = KinNode.attribute("kinnum").as_int();
  TmpKin->Enabled = KinNode.attribute("enabled").as_bool();
  CurNode = KinNode.child("menu");
  TmpKin->KinMenuStr = CurNode.child_value();
  CurNode = KinNode.child("kinname");
  TmpKin->KinItName = CurNode.child("itname").child_value();
  TmpKin->KinHeName = CurNode.child("hename").child_value();
  TmpKin->KinSheName = CurNode.child("shename").child_value();
  TmpKin->KinPluralName = CurNode.child("pluralname").child_value();

  //Parce kin races
  CurNode = KinNode.child("kinraces");
  for (CurNode = CurNode.child("race"); CurNode; CurNode = CurNode.next_sibling("race")) {
    LoadRace(CurNode, TmpKin);
  }
  //Add new kin in kin list
  PlayerRace::PlayerKinList.push_back(TmpKin);
}

//Загрузка параметров родов и рас
void PlayerRace::Load(pugi::xml_node XMLSRaceList) {
  pugi::xml_node CurNode;

  for (CurNode = XMLSRaceList.child("kin"); CurNode; CurNode = CurNode.next_sibling("kin")) {
    LoadKin(CurNode);
  }
}

// Добавление новой родовой способности в список
void PlayerRace::AddRaceFeature(int feat) {
  std::vector<int>::iterator RaceFeature = find(_RaceFeatureList.begin(), _RaceFeatureList.end(), feat);
  if (RaceFeature == _RaceFeatureList.end())
    _RaceFeatureList.push_back(feat);
};

// Добавление нового места рождения персонажей
void PlayerRace::AddRaceBirthPlace(int id) {
  std::vector<int>::iterator BirthPlace = find(_RaceBirthPlaceList.begin(), _RaceBirthPlaceList.end(), id);
  if (BirthPlace == _RaceBirthPlaceList.end())
    _RaceBirthPlaceList.push_back(id);
};

//Получение указателя на расу PC
PlayerKinPtr PlayerRace::GetPlayerKin(int Kin) {
  PlayerKinPtr KinPtr;
  for (PlayerKinListType::iterator it = PlayerKinList.begin(); it != PlayerKinList.end(); ++it)
    if ((*it)->KinNum == Kin)
      KinPtr = *it;
  return KinPtr;
};

//Получение указателя на род PC
PlayerRacePtr PlayerRace::GetPlayerRace(int Kin, int Race) {
  PlayerRacePtr RacePtr;
  PlayerKinPtr KinPtr = PlayerRace::GetPlayerKin(Kin);

  if (KinPtr != NULL)
    for (PlayerRaceListType::iterator it = KinPtr->PlayerRaceList.begin(); it != KinPtr->PlayerRaceList.end(); ++it)
      if ((*it)->_RaceNum == Race)
        RacePtr = *it;
  return RacePtr;
};

//Проверка наличия у данного рода+расы способности с указанным номером
bool PlayerRace::FeatureCheck(int Kin, int Race, int Feat) {
  PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Kin, Race);
  if (RacePtr == NULL)
    return false;
  std::vector<int>::iterator
      RaceFeature = find(RacePtr->_RaceFeatureList.begin(), RacePtr->_RaceFeatureList.end(), Feat);
  if (RaceFeature != RacePtr->_RaceFeatureList.end())
    return true;

  return false;
};

void PlayerRace::GetKinNamesList(CHAR_DATA * /*ch*/) {
  //char buf[MAX_INPUT_LENGTH];
  //snprintf(buf, MAX_STRING_LENGTH, " %d \r\n", PlayerKinList[0]->PlayerRaceList[0]->GetFeatNum());
  //send_to_char(buf, ch);
  //for (PlayerKinListType::iterator it = PlayerKinList.begin();it != PlayerKinList.end();++it)
  //{
  //	snprintf(buf, MAX_STRING_LENGTH, " %s \r\n", (*it)->KinHeName.c_str());
  //	send_to_char(buf, ch);
  //}
  //test message
  //char buf33[MAX_INPUT_LENGTH];
  //snprintf(buf33, MAX_STRING_LENGTH, "!==!...%s", CurNode.child("shename").child_value());
  //mudlog(buf33, CMP, LVL_IMMORT, SYSLOG, TRUE);
}

//Получение всего списка способностей для указанного рода+расы
std::vector<int> PlayerRace::GetRaceFeatures(int Kin, int Race) {
  std::vector<int> RaceFeatures;
  PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Kin, Race);
  if (RacePtr != NULL) {
    RaceFeatures = RacePtr->_RaceFeatureList;
  }
  return RaceFeatures;
}

//Получение номера расы по названию
int PlayerRace::GetKinNumByName(const std::string &KinName) {
  for (PlayerKinListType::const_iterator it = PlayerKinList.begin(); it != PlayerKinList.end(); ++it) {
    if ((*it)->KinMenuStr == KinName) {
      return (*it)->KinNum;
    }
  }

  return RACE_UNDEFINED;
};

//Получение номера рода по названию
int PlayerRace::GetRaceNumByName(int Kin, const std::string &RaceName) {
  PlayerKinPtr KinPtr = PlayerRace::GetPlayerKin(Kin);
  if (KinPtr != NULL) {
    for (PlayerRaceListType::const_iterator it = KinPtr->PlayerRaceList.begin(); it != KinPtr->PlayerRaceList.end();
         ++it) {
      if ((*it)->_RaceMenuStr == RaceName) {
        return (*it)->_RaceNum;
      }
    }
  }

  return RACE_UNDEFINED;
};

//Получение названия расы по номеру и полу
std::string PlayerRace::GetKinNameByNum(int KinNum, const ESex Sex) {
  for (PlayerKinListType::iterator it = PlayerKinList.begin(); it != PlayerKinList.end(); ++it) {
    if ((*it)->KinNum == KinNum) {
      switch (Sex) {
        case ESex::SEX_NEUTRAL: return PlayerRace::PlayerKinList[KinNum]->KinItName;
          break;

        case ESex::SEX_MALE: return PlayerRace::PlayerKinList[KinNum]->KinHeName;
          break;

        case ESex::SEX_FEMALE: return PlayerRace::PlayerKinList[KinNum]->KinSheName;
          break;

        case ESex::SEX_POLY: return PlayerRace::PlayerKinList[KinNum]->KinPluralName;
          break;

        default: return PlayerRace::PlayerKinList[KinNum]->KinHeName;
      }
    }
  }

  return KIN_NAME_UNDEFINED;
};

//Получение названия рода по номеру и полу
std::string PlayerRace::GetRaceNameByNum(int KinNum, int RaceNum, const ESex Sex) {
  //static char out_str[MAX_STRING_LENGTH];
  //*out_str = '\0';
  //sprintf(out_str, "Число рас %d %d", KinNum, RaceNum);
  //return out_str; //PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RaceHeName;
  PlayerKinPtr KinPtr;
  if ((KinNum > KIN_UNDEFINED) && (static_cast<unsigned>(KinNum) < PlayerRace::PlayerKinList.size())) {
    KinPtr = PlayerRace::PlayerKinList[KinNum];
    for (PlayerRaceListType::iterator it = KinPtr->PlayerRaceList.begin(); it != KinPtr->PlayerRaceList.end(); ++it) {
      if ((*it)->_RaceNum == RaceNum) {
        switch (Sex) {
          case ESex::SEX_NEUTRAL: return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RaceItName;
            break;

          case ESex::SEX_MALE: return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RaceHeName;
            break;

          case ESex::SEX_FEMALE: return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RaceSheName;
            break;

          case ESex::SEX_POLY: return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RacePluralName;
            break;

          default: return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RaceHeName;
        };
      }
    }
  }

  return RACE_NAME_UNDEFINED;
};

//Вывод списка родов в виде меню
std::string PlayerRace::ShowRacesMenu(int KinNum) {
  std::ostringstream buffer;
  PlayerKinPtr KinPtr = PlayerRace::GetPlayerKin(KinNum);

  if (KinPtr != NULL)
    for (PlayerRaceListType::iterator it = KinPtr->PlayerRaceList.begin(); it != KinPtr->PlayerRaceList.end(); ++it)
      buffer << " " << (*it)->_RaceNum + 1 << ") " << (*it)->_RaceMenuStr << "\r\n";

  return buffer.str();
};

//Проверяем наличие рода с введнным номером
//Возвращаем номер рода или "неопределено"
int PlayerRace::CheckRace(int KinNum, char *arg) {
  unsigned RaceNum = atoi(arg);
  //Поскольку меню формируется в этом же модуле функцией ShowRacesMenu
  //То для доступа к конкретному _роду_ можно использовать просто само введенное число
  //как индекс массива (с поправкой на единицу). Нужно только убедиться, что введено вменяемое значение.
  //То есть от единицы до максимального индекса+1 (ибо в меню нумерация с 1)
  //Но вот возвращать надо номер _рода_ а не индекс, ибо теоретически они могут и не совпадать
  if (!RaceNum || (RaceNum < 1) ||
      (RaceNum > PlayerRace::PlayerKinList[KinNum]->PlayerRaceList.size()) ||
      !PlayerKinList[KinNum]->PlayerRaceList[RaceNum - 1]->_Enabled) {
    return RACE_UNDEFINED;
  }

  if ((KinNum > RACE_UNDEFINED) && (static_cast<unsigned>(KinNum) < PlayerRace::PlayerKinList.size())) {
    return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum - 1]->_RaceNum;
  }

  return RACE_UNDEFINED;
};

//Вывод списка рас в виде меню
std::string PlayerRace::ShowKinsMenu() {
  std::ostringstream buffer;
  for (PlayerKinListType::iterator it = PlayerKinList.begin(); it != PlayerKinList.end(); ++it)
    buffer << " " << (*it)->KinNum + 1 << ") " << (*it)->KinMenuStr << "\r\n";

  return buffer.str();
};

//Проверяем наличие расы с введенным номером
int PlayerRace::CheckKin(char *arg) {
  int KinNum = atoi(arg);
  if (!KinNum || (KinNum < 1) ||
      (static_cast<unsigned>(KinNum) > PlayerRace::PlayerKinList.size()) ||
      !PlayerRace::PlayerKinList[KinNum - 1]->Enabled)
    return KIN_UNDEFINED;

  return PlayerRace::PlayerKinList[KinNum - 1]->KinNum;
};

std::vector<int> PlayerRace::GetRaceBirthPlaces(int Kin, int Race) {
  std::vector<int> BirthPlaces;
  PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Kin, Race);
  if (RacePtr != NULL)
    BirthPlaces = RacePtr->_RaceBirthPlaceList;

  return BirthPlaces;
}

int PlayerRace::CheckBirthPlace(int Kin, int Race, char *arg) {
  int BirthPlaceNum = atoi(arg);
  if (BirthPlaceNum &&
      ((Kin > RACE_UNDEFINED) && (static_cast<unsigned>(Kin) < PlayerRace::PlayerKinList.size())) &&
      (Race > RACE_UNDEFINED) && (static_cast<unsigned>(Race) < PlayerRace::PlayerKinList[Kin]->PlayerRaceList.size())
      &&
          ((BirthPlaceNum > 0) && (static_cast<unsigned>(BirthPlaceNum)
              <= PlayerRace::PlayerKinList[Kin]->PlayerRaceList[Race]->_RaceBirthPlaceList.size())))
    return PlayerRace::PlayerKinList[Kin]->PlayerRaceList[Race]->_RaceBirthPlaceList[BirthPlaceNum - 1];

  return RACE_UNDEFINED;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
