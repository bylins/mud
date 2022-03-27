/**
 \authors Created by Sventovit
 \date 13.01.2022.
 \brief Константы для персонажей, комнат и предметов и коддля их парсинга.
 \details Код для парсинга названимй констант из конфигурационных файлов.
*/

#include "entities_constants.h"

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

	EWearFlag_name_by_value[EWearFlag::kTake] = "kTake";
	EWearFlag_name_by_value[EWearFlag::kFinger] = "kFinger";
	EWearFlag_name_by_value[EWearFlag::kNeck] = "kNeck";
	EWearFlag_name_by_value[EWearFlag::kBody] = "kBody";
	EWearFlag_name_by_value[EWearFlag::kHead] = "kHead";
	EWearFlag_name_by_value[EWearFlag::kLegs] = "kLegs";
	EWearFlag_name_by_value[EWearFlag::kFeet] = "kFeet";
	EWearFlag_name_by_value[EWearFlag::kHands] = "kHands";
	EWearFlag_name_by_value[EWearFlag::kArms] = "kArms";
	EWearFlag_name_by_value[EWearFlag::kShield] = "kShield";
	EWearFlag_name_by_value[EWearFlag::kShoulders] = "kShoulders";
	EWearFlag_name_by_value[EWearFlag::kWaist] = "kWaist";
	EWearFlag_name_by_value[EWearFlag::kWrist] = "kWrist";
	EWearFlag_name_by_value[EWearFlag::kWield] = "kWield";
	EWearFlag_name_by_value[EWearFlag::kHold] = "kHold";
	EWearFlag_name_by_value[EWearFlag::kBoth] = "kBoth";
	EWearFlag_name_by_value[EWearFlag::kQuiver] = "kQuiver";

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

typedef std::map<EObjFlag, std::string> EObjFlag_name_by_value_t;
typedef std::map<const std::string, EObjFlag> EObjFlag_value_by_name_t;
EObjFlag_name_by_value_t EObjFlag_name_by_value;
EObjFlag_value_by_name_t EObjFlag_value_by_name;

void init_EObjFlag_ITEM_NAMES() {
	EObjFlag_name_by_value.clear();
	EObjFlag_value_by_name.clear();

	EObjFlag_name_by_value[EObjFlag::kGlow] = "kGlow";
	EObjFlag_name_by_value[EObjFlag::kHum] = "kHum";
	EObjFlag_name_by_value[EObjFlag::kNorent] = "kNorent";
	EObjFlag_name_by_value[EObjFlag::kNodonate] = "kNodonate";
	EObjFlag_name_by_value[EObjFlag::kNoinvis] = "kNoinvis";
	EObjFlag_name_by_value[EObjFlag::kInvisible] = "kInvisible";
	EObjFlag_name_by_value[EObjFlag::kMagic] = "kMagic";
	EObjFlag_name_by_value[EObjFlag::kNodrop] = "kNodrop";
	EObjFlag_name_by_value[EObjFlag::kBless] = "kBless";
	EObjFlag_name_by_value[EObjFlag::kNosell] = "kNosell";
	EObjFlag_name_by_value[EObjFlag::kDecay] = "kDecay";
	EObjFlag_name_by_value[EObjFlag::kZonedacay] = "kZonedacay";
	EObjFlag_name_by_value[EObjFlag::kNodisarm] = "kNodisarm";
	EObjFlag_name_by_value[EObjFlag::kNodecay] = "kNodecay";
	EObjFlag_name_by_value[EObjFlag::kPoisoned] = "kPoisoned";
	EObjFlag_name_by_value[EObjFlag::kSharpen] = "kSharpen";
	EObjFlag_name_by_value[EObjFlag::kArmored] = "kArmored";
	EObjFlag_name_by_value[EObjFlag::kAppearsDay] = "kAppearsDay";
	EObjFlag_name_by_value[EObjFlag::kAppearsNight] = "kAppearsNight";
	EObjFlag_name_by_value[EObjFlag::kAppearsFullmoon] = "kAppearsFullmoon";
	EObjFlag_name_by_value[EObjFlag::kAppearsWinter] = "kAppearsWinter";
	EObjFlag_name_by_value[EObjFlag::kAppearsSpring] = "kAppearsSpring";
	EObjFlag_name_by_value[EObjFlag::kAppearsSummer] = "kAppearsSummer";
	EObjFlag_name_by_value[EObjFlag::kAppearsAutumn] = "kAppearsAutumn";
	EObjFlag_name_by_value[EObjFlag::kSwimming] = "kSwimming";
	EObjFlag_name_by_value[EObjFlag::kFlying] = "kFlying";
	EObjFlag_name_by_value[EObjFlag::kThrowing] = "kThrowing";
	EObjFlag_name_by_value[EObjFlag::kTicktimer] = "kTicktimer";
	EObjFlag_name_by_value[EObjFlag::kFire] = "kFire";
	EObjFlag_name_by_value[EObjFlag::kRepopDecay] = "kRepopDecay";
	EObjFlag_name_by_value[EObjFlag::kNolocate] = "kNolocate";
	EObjFlag_name_by_value[EObjFlag::kTimedLvl] = "kTimedLvl";
	EObjFlag_name_by_value[EObjFlag::kNoalter] = "kNoalter";
	EObjFlag_name_by_value[EObjFlag::kHasOneSlot] = "kHasOneSlot";
	EObjFlag_name_by_value[EObjFlag::kHasTwoSlots] = "kHasTwoSlots";
	EObjFlag_name_by_value[EObjFlag::kHasThreeSlots] = "kHasThreeSlots";
	EObjFlag_name_by_value[EObjFlag::KSetItem] = "KSetItem";
	EObjFlag_name_by_value[EObjFlag::KNofail] = "KNofail";
	EObjFlag_name_by_value[EObjFlag::kNamed] = "kNamed";
	EObjFlag_name_by_value[EObjFlag::kBloody] = "kBloody";
	EObjFlag_name_by_value[EObjFlag::k1inlaid] = "k1inlaid";
	EObjFlag_name_by_value[EObjFlag::k2inlaid] = "k2inlaid";
	EObjFlag_name_by_value[EObjFlag::k3inlaid] = "k3inlaid";
	EObjFlag_name_by_value[EObjFlag::kNopour] = "kNopour";
	EObjFlag_name_by_value[EObjFlag::kUnique] = "kUnique";
	EObjFlag_name_by_value[EObjFlag::kTransformed] = "kTransformed";
	EObjFlag_name_by_value[EObjFlag::KLimitedTimer] = "KLimitedTimer";
	EObjFlag_name_by_value[EObjFlag::kBindOnPurchase] = "kBindOnPurchase";
	EObjFlag_name_by_value[EObjFlag::kNotOneInClanChest] = "kNotOneInClanChest";

	for (const auto &i : EObjFlag_name_by_value) {
		EObjFlag_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM(const EObjFlag item) {
	if (EObjFlag_name_by_value.empty()) {
		init_EObjFlag_ITEM_NAMES();
	}
	return EObjFlag_name_by_value.at(item);
}

template<>
EObjFlag ITEM_BY_NAME(const std::string &name) {
	if (EObjFlag_name_by_value.empty()) {
		init_EObjFlag_ITEM_NAMES();
	}
	return EObjFlag_value_by_name.at(name);
}

typedef std::map<ENoFlag, std::string> ENoFlag_name_by_value_t;
typedef std::map<const std::string, ENoFlag> ENoFlag_value_by_name_t;
ENoFlag_name_by_value_t ENoFlag_name_by_value;
ENoFlag_value_by_name_t ENoFlag_value_by_name;
void init_ENoFlag_ITEM_NAMES() {
	ENoFlag_value_by_name.clear();
	ENoFlag_name_by_value.clear();

	ENoFlag_name_by_value[ENoFlag::kMono] = "ITEM_NO_MONO";
	ENoFlag_name_by_value[ENoFlag::kPoly] = "ITEM_NO_POLY";
	ENoFlag_name_by_value[ENoFlag::kNeutral] = "ITEM_NO_NEUTRAL";
	ENoFlag_name_by_value[ENoFlag::kMage] = "ITEM_NO_MAGIC_USER";
	ENoFlag_name_by_value[ENoFlag::kSorcerer] = "ITEM_NO_CLERIC";
	ENoFlag_name_by_value[ENoFlag::kThief] = "ITEM_NO_THIEF";
	ENoFlag_name_by_value[ENoFlag::kWarrior] = "ITEM_NO_WARRIOR";
	ENoFlag_name_by_value[ENoFlag::kAssasine] = "ITEM_NO_ASSASINE";
	ENoFlag_name_by_value[ENoFlag::kGuard] = "ITEM_NO_GUARD";
	ENoFlag_name_by_value[ENoFlag::kPaladine] = "ITEM_NO_PALADINE";
	ENoFlag_name_by_value[ENoFlag::kRanger] = "kRanger";
	ENoFlag_name_by_value[ENoFlag::kVigilant] = "kVigilant";
	ENoFlag_name_by_value[ENoFlag::kMerchant] = "kMerchant";
	ENoFlag_name_by_value[ENoFlag::kMagus] = "kMagus";
	ENoFlag_name_by_value[ENoFlag::kConjurer] = "kConjurer";
	ENoFlag_name_by_value[ENoFlag::kCharmer] = "kCharmer";
	ENoFlag_name_by_value[ENoFlag::kWIzard] = "kWIzard";
	ENoFlag_name_by_value[ENoFlag::kNecromancer] = "kNecromancer";
	ENoFlag_name_by_value[ENoFlag::kFighter] = "kFighter";
	ENoFlag_name_by_value[ENoFlag::kKiller] = "kKiller";
	ENoFlag_name_by_value[ENoFlag::kColored] = "kColored";
	ENoFlag_name_by_value[ENoFlag::kBattle] = "kBattle";
	ENoFlag_name_by_value[ENoFlag::kMale] = "kMale";
	ENoFlag_name_by_value[ENoFlag::kFemale] = "kFemale";
	ENoFlag_name_by_value[ENoFlag::kCharmice] = "kCharmice";

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

typedef std::map<ESaving, std::string> ESaving_name_by_value_t;
typedef std::map<const std::string, ESaving> ESaving_value_by_name_t;
ESaving_name_by_value_t ESaving_name_by_value;
ESaving_value_by_name_t ESaving_value_by_name;

void init_ESaving_ITEM_NAMES() {
	ESaving_name_by_value.clear();
	ESaving_value_by_name.clear();

	ESaving_name_by_value[ESaving::kWill] = "kWill";
	ESaving_name_by_value[ESaving::kStability] = "kStability";
	ESaving_name_by_value[ESaving::kCritical] = "kCritical";
	ESaving_name_by_value[ESaving::kReflex] = "kReflex";

	for (const auto &i : ESaving_name_by_value) {
		ESaving_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ESaving>(const ESaving item) {
	if (ESaving_name_by_value.empty()) {
		init_ESaving_ITEM_NAMES();
	}
	return ESaving_name_by_value.at(item);
}

template<>
ESaving ITEM_BY_NAME<ESaving>(const std::string &name) {
	if (ESaving_name_by_value.empty()) {
		init_ESaving_ITEM_NAMES();
	}
	return ESaving_value_by_name.at(name);
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

ESaving& operator++(ESaving &s) {
	s =  static_cast<ESaving>(to_underlying(s) + 1);
	return s;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :