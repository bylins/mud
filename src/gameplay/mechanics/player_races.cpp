// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include "utils/utils.h"
#include "third_party_libs/pugixml/pugixml.h"

#include "player_races.h"

PlayerRaceListType PlayerRace::PlayerRaceList;

static void LoadRace(pugi::xml_node RaceNode);

// Создаем новый _род_ и распихиваем по его полям значения из файла
static void LoadRace(pugi::xml_node RaceNode) {
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
	// Add race features
	for (CurNode = RaceNode.child("feature"); CurNode; CurNode = CurNode.next_sibling("feature")) {
		TmpRace->AddRaceFeature(CurNode.attribute("featnum").as_int());
	}
	// Добавляем родовые места "появления на свет" новых персонажей
	for (CurNode = RaceNode.child("birthplace"); CurNode; CurNode = CurNode.next_sibling("birthplace")) {
		TmpRace->AddRaceBirthPlace(CurNode.attribute("id").as_int());
	}
	// Add new race in the flat list
	PlayerRace::PlayerRaceList.push_back(TmpRace);
}

// Загрузка параметров родов. issue.kin-remove: the XML still groups races under <kin> wrappers; we
// read the races of every enabled kin into one flat list (in practice only kin 0 is enabled).
void PlayerRace::Load(pugi::xml_node XMLSRaceList) {
	for (pugi::xml_node KinNode = XMLSRaceList.child("kin"); KinNode; KinNode = KinNode.next_sibling("kin")) {
		if (!KinNode.attribute("enabled").as_bool()) {
			continue;
		}
		pugi::xml_node KinRaces = KinNode.child("kinraces");
		for (pugi::xml_node CurNode = KinRaces.child("race"); CurNode; CurNode = CurNode.next_sibling("race")) {
			LoadRace(CurNode);
		}
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

// Получение указателя на род PC
PlayerRacePtr PlayerRace::GetPlayerRace(int Race) {
	PlayerRacePtr RacePtr;
	for (PlayerRaceListType::iterator it = PlayerRaceList.begin(); it != PlayerRaceList.end(); ++it)
		if ((*it)->_RaceNum == Race)
			RacePtr = *it;
	return RacePtr;
};

// Проверка наличия у данного рода способности с указанным номером
bool PlayerRace::FeatureCheck(int Race, int Feat) {
	PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Race);
	if (RacePtr == nullptr)
		return false;
	auto RaceFeature = find(RacePtr->_RaceFeatureList.begin(), RacePtr->_RaceFeatureList.end(), Feat);
	if (RaceFeature != RacePtr->_RaceFeatureList.end())
		return true;

	return false;
};

// Получение всего списка способностей для указанного рода
std::vector<int> PlayerRace::GetRaceFeatures(int Race) {
	std::vector<int> RaceFeatures;
	PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Race);
	if (RacePtr != nullptr) {
		RaceFeatures = RacePtr->_RaceFeatureList;
	}
	return RaceFeatures;
}

// Получение номера рода по названию
int PlayerRace::GetRaceNumByName(const std::string &RaceName) {
	for (PlayerRaceListType::const_iterator it = PlayerRaceList.begin(); it != PlayerRaceList.end(); ++it) {
		if ((*it)->_RaceMenuStr == RaceName) {
			return (*it)->_RaceNum;
		}
	}

	return RACE_UNDEFINED;
};

// Получение названия рода по номеру и полу
std::string PlayerRace::GetRaceNameByNum(int RaceNum, EGender Sex) {
	PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(RaceNum);
	if (RacePtr != nullptr) {
		switch (Sex) {
			case EGender::kNeutral: return RacePtr->_RaceItName;
			case EGender::kMale: return RacePtr->_RaceHeName;
			case EGender::kFemale: return RacePtr->_RaceSheName;
			case EGender::kPoly: return RacePtr->_RacePluralName;
			default: return RacePtr->_RaceHeName;
		}
	}

	return RACE_NAME_UNDEFINED;
};

// Вывод списка родов в виде меню
std::string PlayerRace::ShowRacesMenu() {
	std::ostringstream buffer;
	for (PlayerRaceListType::iterator it = PlayerRaceList.begin(); it != PlayerRaceList.end(); ++it)
		buffer << " " << (*it)->_RaceNum + 1 << ") " << (*it)->_RaceMenuStr << "\r\n";

	return buffer.str();
};

// Проверяем наличие рода с введенным номером. Возвращаем номер рода или "неопределено".
// Меню формируется ShowRacesMenu с нумерацией от 1, поэтому введенное число - это индекс+1.
int PlayerRace::CheckRace(char *arg) {
	unsigned RaceNum = atoi(arg);
	if (!RaceNum || (RaceNum < 1) || (RaceNum > PlayerRaceList.size())
		|| !PlayerRaceList[RaceNum - 1]->_Enabled) {
		return RACE_UNDEFINED;
	}

	return PlayerRaceList[RaceNum - 1]->_RaceNum;
};

std::vector<int> PlayerRace::GetRaceBirthPlaces(int Race) {
	std::vector<int> BirthPlaces;
	PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Race);
	if (RacePtr != nullptr)
		BirthPlaces = RacePtr->_RaceBirthPlaceList;

	return BirthPlaces;
}

int PlayerRace::CheckBirthPlace(int Race, char *arg) {
	int BirthPlaceNum = atoi(arg);
	PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Race);
	if (BirthPlaceNum && RacePtr != nullptr
		&& (BirthPlaceNum > 0)
		&& (static_cast<unsigned>(BirthPlaceNum) <= RacePtr->_RaceBirthPlaceList.size())) {
		return RacePtr->_RaceBirthPlaceList[BirthPlaceNum - 1];
	}

	return RACE_UNDEFINED;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
