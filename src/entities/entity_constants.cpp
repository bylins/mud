/**
 \authors Created by Sventovit
 \date 13.01.2022.
 \brief Константы для персонажей, комнат и предметов и коддля их парсинга.
 \details Код для парсинга названимй констант из конфигурационных файлов.
*/

#include "entity_constants.h"

#include <map>

std::unordered_map<int, std::string> SECTOR_TYPE_BY_VALUE = {
	{kSectInside, "inside"},
	{kSectCity, "city"},
	{kSectField, "field"},
	{kSectForest, "forest"},
	{kSectHills, "hills"},
	{kSectMountain, "mountain"},
	{kSectWaterSwim, "swim water"},
	{kSectWaterNoswim, "no swim water"},
	{kSectOnlyFlying, "flying"},
	{kSectUnderwater, "underwater"},
	{kSectSecret, "secret"},
	{kSectStoneroad, "stone road"},
	{kSectRoad, "road"},
	{kSectWildroad, "wild road"},
	{kSectFieldSnow, "snow field"},
	{kSectFieldRain, "rain field"},
	{kSectForestSnow, "snow forest"},
	{kSectForestRain, "rain forest"},
	{kSectHillsSnow, "snow hills"},
	{kSectHillsRain, "rain hills"},
	{kSectMountainSnow, "snow mountain"},
	{kSectThinIce, "thin ice"},
	{kSectNormalIce, "normal ice"},
	{kSectThickIce, "thick ice"}
};

typedef std::map<ESex, std::string> ESex_name_by_value_t;
typedef std::map<const std::string, ESex> ESex_value_by_name_t;
ESex_name_by_value_t ESex_name_by_value;
ESex_value_by_name_t ESex_value_by_name;

void init_ESex_ITEM_NAMES() {
	ESex_name_by_value.clear();
	ESex_value_by_name.clear();

	ESex_name_by_value[ESex::kNeutral] = "NEUTRAL";
	ESex_name_by_value[ESex::kMale] = "MALE";
	ESex_name_by_value[ESex::kFemale] = "FEMALE";
	ESex_name_by_value[ESex::kPoly] = "POLY";

	for (const auto &i : ESex_name_by_value) {
		ESex_value_by_name[i.second] = i.first;
	}
}

template<>
ESex ITEM_BY_NAME(const std::string &name) {
	if (ESex_name_by_value.empty()) {
		init_ESex_ITEM_NAMES();
	}
	return ESex_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM(const ESex item) {
	if (ESex_name_by_value.empty()) {
		init_ESex_ITEM_NAMES();
	}
	return ESex_name_by_value.at(item);
}

typedef std::map<EWearFlag, std::string> EWearFlag_name_by_value_t;
typedef std::map<const std::string, EWearFlag> EWearFlag_value_by_name_t;
EWearFlag_name_by_value_t EWearFlag_name_by_value;
EWearFlag_value_by_name_t EWearFlag_value_by_name;

void init_EWearFlag_ITEM_NAMES() {
	EWearFlag_name_by_value.clear();
	EWearFlag_value_by_name.clear();

	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_TAKE] = "ITEM_WEAR_TAKE";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_FINGER] = "ITEM_WEAR_FINGER";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_NECK] = "ITEM_WEAR_NECK";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_BODY] = "ITEM_WEAR_BODY";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_HEAD] = "ITEM_WEAR_HEAD";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_LEGS] = "ITEM_WEAR_LEGS";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_FEET] = "ITEM_WEAR_FEET";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_HANDS] = "ITEM_WEAR_HANDS";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_ARMS] = "ITEM_WEAR_ARMS";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_SHIELD] = "ITEM_WEAR_SHIELD";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_ABOUT] = "ITEM_WEAR_ABOUT";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_WAIST] = "ITEM_WEAR_WAIST";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_WRIST] = "ITEM_WEAR_WRIST";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_WIELD] = "ITEM_WEAR_WIELD";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_HOLD] = "ITEM_WEAR_HOLD";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_BOTHS] = "ITEM_WEAR_BOTHS";
	EWearFlag_name_by_value[EWearFlag::ITEM_WEAR_QUIVER] = "ITEM_WEAR_QUIVER";

	for (const auto &i : EWearFlag_name_by_value) {
		EWearFlag_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EWearFlag>(const EWearFlag item) {
	if (EWearFlag_name_by_value.empty()) {
		init_EWearFlag_ITEM_NAMES();
	}
	return EWearFlag_name_by_value.at(item);
}

template<>
EWearFlag ITEM_BY_NAME<EWearFlag>(const std::string &name) {
	if (EWearFlag_name_by_value.empty()) {
		init_EWearFlag_ITEM_NAMES();
	}
	return EWearFlag_value_by_name.at(name);
}

typedef std::map<EExtraFlag, std::string> EExtraFlag_name_by_value_t;
typedef std::map<const std::string, EExtraFlag> EExtraFlag_value_by_name_t;
EExtraFlag_name_by_value_t EExtraFlag_name_by_value;
EExtraFlag_value_by_name_t EExtraFlag_value_by_name;

void init_EExtraFlag_ITEM_NAMES() {
	EExtraFlag_name_by_value.clear();
	EExtraFlag_value_by_name.clear();

	EExtraFlag_name_by_value[EExtraFlag::ITEM_GLOW] = "ITEM_GLOW";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_HUM] = "ITEM_HUM";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NORENT] = "ITEM_NORENT";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NODONATE] = "ITEM_NODONATE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOINVIS] = "ITEM_NOINVIS";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_INVISIBLE] = "ITEM_INVISIBLE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_MAGIC] = "ITEM_MAGIC";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NODROP] = "ITEM_NODROP";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_BLESS] = "ITEM_BLESS";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOSELL] = "ITEM_NOSELL";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_DECAY] = "ITEM_DECAY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_ZONEDECAY] = "ITEM_ZONEDECAY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NODISARM] = "ITEM_NODISARM";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NODECAY] = "ITEM_NODECAY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_POISONED] = "ITEM_POISONED";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_SHARPEN] = "ITEM_SHARPEN";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_ARMORED] = "ITEM_ARMORED";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_DAY] = "ITEM_DAY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NIGHT] = "ITEM_NIGHT";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_FULLMOON] = "ITEM_FULLMOON";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_WINTER] = "ITEM_WINTER";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_SPRING] = "ITEM_SPRING";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_SUMMER] = "ITEM_SUMMER";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_AUTUMN] = "ITEM_AUTUMN";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_SWIMMING] = "ITEM_SWIMMING";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_FLYING] = "ITEM_FLYING";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_THROWING] = "ITEM_THROWING";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_TICKTIMER] = "ITEM_TICKTIMER";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_FIRE] = "ITEM_FIRE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_REPOP_DECAY] = "ITEM_REPOP_DECAY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOLOCATE] = "ITEM_NOLOCATE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_TIMEDLVL] = "ITEM_TIMEDLVL";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOALTER] = "ITEM_NOALTER";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_WITH1SLOT] = "ITEM_WITH1SLOT";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_WITH2SLOTS] = "ITEM_WITH2SLOTS";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_WITH3SLOTS] = "ITEM_WITH3SLOTS";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_SETSTUFF] = "ITEM_SETSTUFF";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NO_FAIL] = "ITEM_NO_FAIL";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NAMED] = "ITEM_NAMED";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_BLOODY] = "ITEM_BLOODY";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_1INLAID] = "ITEM_1INLAID";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_2INLAID] = "ITEM_2INLAID";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_3INLAID] = "ITEM_3INLAID";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOPOUR] = "ITEM_NOPOUR";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_UNIQUE] = "ITEM_UNIQUE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_TRANSFORMED] = "ITEM_TRANSFORMED";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOT_UNLIMIT_TIMER] = "ITEM_NOT_UNLIMIT_TIMER";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_UNIQUE_WHEN_PURCHASE] = "ITEM_UNIQUE_WHEN_PURCHASE";
	EExtraFlag_name_by_value[EExtraFlag::ITEM_NOT_ONE_CLANCHEST] = "ITEM_NOT_ONE_CLANCHEST";

	for (const auto &i : EExtraFlag_name_by_value) {
		EExtraFlag_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM(const EExtraFlag item) {
	if (EExtraFlag_name_by_value.empty()) {
		init_EExtraFlag_ITEM_NAMES();
	}
	return EExtraFlag_name_by_value.at(item);
}

template<>
EExtraFlag ITEM_BY_NAME(const std::string &name) {
	if (EExtraFlag_name_by_value.empty()) {
		init_EExtraFlag_ITEM_NAMES();
	}
	return EExtraFlag_value_by_name.at(name);
}

typedef std::map<ENoFlag, std::string> ENoFlag_name_by_value_t;
typedef std::map<const std::string, ENoFlag> ENoFlag_value_by_name_t;
ENoFlag_name_by_value_t ENoFlag_name_by_value;
ENoFlag_value_by_name_t ENoFlag_value_by_name;
void init_ENoFlag_ITEM_NAMES() {
	ENoFlag_value_by_name.clear();
	ENoFlag_name_by_value.clear();

	ENoFlag_name_by_value[ENoFlag::ITEM_NO_MONO] = "ITEM_NO_MONO";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_POLY] = "ITEM_NO_POLY";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_NEUTRAL] = "ITEM_NO_NEUTRAL";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_MAGIC_USER] = "ITEM_NO_MAGIC_USER";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_CLERIC] = "ITEM_NO_CLERIC";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_THIEF] = "ITEM_NO_THIEF";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_WARRIOR] = "ITEM_NO_WARRIOR";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_ASSASINE] = "ITEM_NO_ASSASINE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_GUARD] = "ITEM_NO_GUARD";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_PALADINE] = "ITEM_NO_PALADINE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_RANGER] = "ITEM_NO_RANGER";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_SMITH] = "ITEM_NO_SMITH";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_MERCHANT] = "ITEM_NO_MERCHANT";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_DRUID] = "ITEM_NO_DRUID";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_BATTLEMAGE] = "ITEM_NO_BATTLEMAGE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_CHARMMAGE] = "ITEM_NO_CHARMMAGE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_DEFENDERMAGE] = "ITEM_NO_DEFENDERMAGE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_NECROMANCER] = "ITEM_NO_NECROMANCER";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_FIGHTER_USER] = "ITEM_NO_FIGHTER_USER";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_KILLER] = "ITEM_NO_KILLER";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_COLORED] = "ITEM_NO_COLORED";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_BD] = "ITEM_NO_BD";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_MALE] = "ITEM_NO_MALE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_FEMALE] = "ITEM_NO_FEMALE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_CHARMICE] = "ITEM_NO_CHARMICE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_POLOVCI] = "ITEM_NO_POLOVCI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_PECHENEGI] = "ITEM_NO_PECHENEGI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_MONGOLI] = "ITEM_NO_MONGOLI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_YIGURI] = "ITEM_NO_YIGURI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_KANGARI] = "ITEM_NO_KANGARI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_XAZARI] = "ITEM_NO_XAZARI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_SVEI] = "ITEM_NO_SVEI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_DATCHANE] = "ITEM_NO_DATCHANE";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_GETTI] = "ITEM_NO_GETTI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_UTTI] = "ITEM_NO_UTTI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_XALEIGI] = "ITEM_NO_XALEIGI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_NORVEZCI] = "ITEM_NO_NORVEZCI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_RUSICHI] = "ITEM_NO_RUSICHI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_STEPNYAKI] = "ITEM_NO_STEPNYAKI";
	ENoFlag_name_by_value[ENoFlag::ITEM_NO_VIKINGI] = "ITEM_NO_VIKINGI";

	for (const auto &i : ENoFlag_name_by_value) {
		ENoFlag_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ENoFlag>(const ENoFlag item) {
	if (ENoFlag_name_by_value.empty()) {
		init_ENoFlag_ITEM_NAMES();
	}
	return ENoFlag_name_by_value.at(item);
}

template<>
ENoFlag ITEM_BY_NAME<ENoFlag>(const std::string &name) {
	if (ENoFlag_name_by_value.empty()) {
		init_ENoFlag_ITEM_NAMES();
	}
	return ENoFlag_value_by_name.at(name);
}

typedef std::map<EAntiFlag, std::string> EAntiFlag_name_by_value_t;
typedef std::map<const std::string, EAntiFlag> EAntiFlag_value_by_name_t;
EAntiFlag_name_by_value_t EAntiFlag_name_by_value;
EAntiFlag_value_by_name_t EAntiFlag_value_by_name;
void init_EAntiFlag_ITEM_NAMES() {
	EAntiFlag_value_by_name.clear();
	EAntiFlag_name_by_value.clear();

	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_MONO] = "ITEM_AN_MONO";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_POLY] = "ITEM_AN_POLY";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_NEUTRAL] = "ITEM_AN_NEUTRAL";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_MAGIC_USER] = "ITEM_AN_MAGIC_USER";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_CLERIC] = "ITEM_AN_CLERIC";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_THIEF] = "ITEM_AN_THIEF";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_WARRIOR] = "ITEM_AN_WARRIOR";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_ASSASINE] = "ITEM_AN_ASSASINE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_GUARD] = "ITEM_AN_GUARD";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_PALADINE] = "ITEM_AN_PALADINE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_RANGER] = "ITEM_AN_RANGER";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_SMITH] = "ITEM_AN_SMITH";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_MERCHANT] = "ITEM_AN_MERCHANT";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_DRUID] = "ITEM_AN_DRUID";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_BATTLEMAGE] = "ITEM_AN_BATTLEMAGE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_CHARMMAGE] = "ITEM_AN_CHARMMAGE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_DEFENDERMAGE] = "ITEM_AN_DEFENDERMAGE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_NECROMANCER] = "ITEM_AN_NECROMANCER";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_FIGHTER_USER] = "ITEM_AN_FIGHTER_USER";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_KILLER] = "ITEM_AN_KILLER";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_COLORED] = "ITEM_AN_COLORED";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_BD] = "ITEM_AN_BD";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_SEVERANE] = "ITEM_AN_SEVERANE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_POLANE] = "ITEM_AN_POLANE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_KRIVICHI] = "ITEM_AN_KRIVICHI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_VATICHI] = "ITEM_AN_VATICHI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_VELANE] = "ITEM_AN_VELANE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_DREVLANE] = "ITEM_AN_DREVLANE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_MALE] = "ITEM_AN_MALE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_FEMALE] = "ITEM_AN_FEMALE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_CHARMICE] = "ITEM_AN_CHARMICE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_POLOVCI] = "ITEM_AN_POLOVCI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_PECHENEGI] = "ITEM_AN_PECHENEGI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_MONGOLI] = "ITEM_AN_MONGOLI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_YIGURI] = "ITEM_AN_YIGURI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_KANGARI] = "ITEM_AN_KANGARI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_XAZARI] = "ITEM_AN_XAZARI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_SVEI] = "ITEM_AN_SVEI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_DATCHANE] = "ITEM_AN_DATCHANE";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_GETTI] = "ITEM_AN_GETTI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_UTTI] = "ITEM_AN_UTTI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_XALEIGI] = "ITEM_AN_XALEIGI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_NORVEZCI] = "ITEM_AN_NORVEZCI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_RUSICHI] = "ITEM_AN_RUSICHI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_STEPNYAKI] = "ITEM_AN_STEPNYAKI";
	EAntiFlag_name_by_value[EAntiFlag::ITEM_AN_VIKINGI] = "ITEM_AN_VIKINGI";

	for (const auto &i : EAntiFlag_name_by_value) {
		EAntiFlag_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EAntiFlag>(const EAntiFlag item) {
	if (EAntiFlag_name_by_value.empty()) {
		init_EAntiFlag_ITEM_NAMES();
	}
	return EAntiFlag_name_by_value.at(item);
}

template<>
EAntiFlag ITEM_BY_NAME(const std::string &name) {
	if (EAntiFlag_name_by_value.empty()) {
		init_EAntiFlag_ITEM_NAMES();
	}
	return EAntiFlag_value_by_name.at(name);
}

typedef std::map<EBaseStat, std::string> EBaseStat_name_by_value_t;
typedef std::map<const std::string, EBaseStat> EBaseStat_value_by_name_t;
EBaseStat_name_by_value_t EBaseStat_name_by_value;
EBaseStat_value_by_name_t EBaseStat_value_by_name;

void init_EBaseStat_ITEM_NAMES() {
	EBaseStat_name_by_value.clear();
	EBaseStat_value_by_name.clear();

	EBaseStat_name_by_value[EBaseStat::kStr] = "kStr";
	EBaseStat_name_by_value[EBaseStat::kDex] = "kDex";
	EBaseStat_name_by_value[EBaseStat::kCon] = "kCon";
	EBaseStat_name_by_value[EBaseStat::kWis] = "kWin";
	EBaseStat_name_by_value[EBaseStat::kInt] = "kInt";
	EBaseStat_name_by_value[EBaseStat::kCha] = "kCha";

	for (const auto &i : EBaseStat_name_by_value) {
		EBaseStat_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EBaseStat>(const EBaseStat item) {
	if (EBaseStat_name_by_value.empty()) {
		init_EBaseStat_ITEM_NAMES();
	}
	return EBaseStat_name_by_value.at(item);
}

template<>
EBaseStat ITEM_BY_NAME<EBaseStat>(const std::string &name) {
	if (EBaseStat_name_by_value.empty()) {
		init_EBaseStat_ITEM_NAMES();
	}
	return EBaseStat_value_by_name.at(name);
}

// Все, связанное с религиями, нужно вынести в отдельный модуль
const religion_names_t religion_name = {
	religion_genders_t{"Язычник", "Язычник", "Язычница", "Язычники"},
	religion_genders_t{"Христианин", "Христианин", "Христианка", "Христиане"},
	religion_genders_t{"", "", "", ""}        // for undefined religion
};

int operator-(EPosition p1,  EPosition p2) {
	return (static_cast<int>(p1) - static_cast<int>(p2));
}

EPosition operator--(const EPosition &p) {
	auto pp = (p == EPosition::kDead) ? EPosition::kDead : static_cast<EPosition>(static_cast<int>(p)-1);
	return pp;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :