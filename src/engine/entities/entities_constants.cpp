/**
 \authors Created by Sventovit
 \date 13.01.2022.
 \brief Константы для персонажей, комнат и предметов и коддля их парсинга.
 \details Код для парсинга названимй констант из конфигурационных файлов.
*/

#include "entities_constants.h"
#include "utils/utils.h"

#include <map>

std::unordered_map<int, std::string> SECTOR_TYPE_BY_VALUE = {
	{ESector::kInside, "inside"},
	{ESector::kCity, "city"},
	{ESector::kField, "field"},
	{ESector::kForest, "forest"},
	{ESector::kHills, "hills"},
	{ESector::kMountain, "mountain"},
	{ESector::kWaterSwim, "swim water"},
	{ESector::kWaterNoswim, "no swim water"},
	{ESector::kOnlyFlying, "flying"},
	{ESector::kUnderwater, "underwater"},
	{ESector::kSecret, "secret"},
	{ESector::kStoneroad, "stone road"},
	{ESector::kRoad, "road"},
	{ESector::kWildroad, "wild road"},
	{ESector::kFieldSnow, "snow field"},
	{ESector::kFieldRain, "rain field"},
	{ESector::kForestSnow, "snow forest"},
	{ESector::kForestRain, "rain forest"},
	{ESector::kHillsSnow, "snow hills"},
	{ESector::kHillsRain, "rain hills"},
	{ESector::kMountainSnow, "snow mountain"},
	{ESector::kThinIce, "thin ice"},
	{ESector::kNormalIce, "normal ice"},
	{ESector::kThickIce, "thick ice"}
};

typedef std::map<EGender, std::string> EGender_name_by_value_t;
typedef std::map<const std::string, EGender> EGender_value_by_name_t;
EGender_name_by_value_t EGender_name_by_value;
EGender_value_by_name_t EGender_value_by_name;

void init_EGender_ITEM_NAMES() {
	EGender_name_by_value.clear();
	EGender_value_by_name.clear();

	EGender_name_by_value[EGender::kNeutral] = "kNeutral";
	EGender_name_by_value[EGender::kMale] = "kMale";
	EGender_name_by_value[EGender::kFemale] = "kFemale";
	EGender_name_by_value[EGender::kPoly] = "kPoly";

	for (const auto &i : EGender_name_by_value) {
		EGender_value_by_name[i.second] = i.first;
	}
}

template<>
EGender ITEM_BY_NAME(const std::string &name) {
	if (EGender_name_by_value.empty()) {
		init_EGender_ITEM_NAMES();
	}
	return EGender_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM(const EGender item) {
	if (EGender_name_by_value.empty()) {
		init_EGender_ITEM_NAMES();
	}
	return EGender_name_by_value.at(item);
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
	EObjFlag_name_by_value[EObjFlag::kZonedecay] = "kZonedecay";
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
	EObjFlag_name_by_value[EObjFlag::kQuestItem] = "kQuestItem";
	EObjFlag_name_by_value[EObjFlag::k2inlaid] = "k2inlaid";
	EObjFlag_name_by_value[EObjFlag::k3inlaid] = "k3inlaid";
	EObjFlag_name_by_value[EObjFlag::kNopour] = "kNopour";
	EObjFlag_name_by_value[EObjFlag::kUnique] = "kUnique";
	EObjFlag_name_by_value[EObjFlag::kTransformed] = "kTransformed";
	EObjFlag_name_by_value[EObjFlag::kNoRentTimer] = "kNoRentTimer";
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

	ENoFlag_name_by_value[ENoFlag::kMono] = "kMono";
	ENoFlag_name_by_value[ENoFlag::kPoly] = "kPoly";
	ENoFlag_name_by_value[ENoFlag::kNeutral] = "kNeutral";
	ENoFlag_name_by_value[ENoFlag::kMage] = "kMage";
	ENoFlag_name_by_value[ENoFlag::kSorcerer] = "kSorcerer";
	ENoFlag_name_by_value[ENoFlag::kThief] = "kThief";
	ENoFlag_name_by_value[ENoFlag::kWarrior] = "kWarrior";
	ENoFlag_name_by_value[ENoFlag::kAssasine] = "kAssasine";
	ENoFlag_name_by_value[ENoFlag::kGuard] = "kGuard";
	ENoFlag_name_by_value[ENoFlag::kPaladine] = "kPaladine";
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

	EAntiFlag_name_by_value[EAntiFlag::kMono] = "kMono";
	EAntiFlag_name_by_value[EAntiFlag::kPoly] = "kPoly";
	EAntiFlag_name_by_value[EAntiFlag::kNeutral] = "kNeutral";
	EAntiFlag_name_by_value[EAntiFlag::kMage] = "kMage";
	EAntiFlag_name_by_value[EAntiFlag::kSorcerer] = "kSorcerer";
	EAntiFlag_name_by_value[EAntiFlag::kThief] = "kThief";
	EAntiFlag_name_by_value[EAntiFlag::kWarrior] = "kWarrior";
	EAntiFlag_name_by_value[EAntiFlag::kAssasine] = "kAssasine";
	EAntiFlag_name_by_value[EAntiFlag::kGuard] = "kGuard";
	EAntiFlag_name_by_value[EAntiFlag::kPaladine] = "kPaladine";
	EAntiFlag_name_by_value[EAntiFlag::kRanger] = "kRanger";
	EAntiFlag_name_by_value[EAntiFlag::kVigilant] = "kVigilant";
	EAntiFlag_name_by_value[EAntiFlag::kMerchant] = "kMerchant";
	EAntiFlag_name_by_value[EAntiFlag::kMagus] = "kMagus";
	EAntiFlag_name_by_value[EAntiFlag::kConjurer] = "kConjurer";
	EAntiFlag_name_by_value[EAntiFlag::kCharmer] = "kCharmer";
	EAntiFlag_name_by_value[EAntiFlag::kWizard] = "kWizard";
	EAntiFlag_name_by_value[EAntiFlag::kNecromancer] = "kNecromancer";
	EAntiFlag_name_by_value[EAntiFlag::kFighter] = "kFighter";
	EAntiFlag_name_by_value[EAntiFlag::kKiller] = "kKiller";
	EAntiFlag_name_by_value[EAntiFlag::kColored] = "kColored";
	EAntiFlag_name_by_value[EAntiFlag::kBattle] = "kBattle";
	EAntiFlag_name_by_value[EAntiFlag::kMale] = "kMale";
	EAntiFlag_name_by_value[EAntiFlag::kFemale] = "kFemale";
	EAntiFlag_name_by_value[EAntiFlag::kCharmice] = "kCharmice";
	EAntiFlag_name_by_value[EAntiFlag::kNoPkClan] = "kNoPkClan";

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

typedef std::map<EPosition, std::string> EPosition_name_by_value_t;
typedef std::map<const std::string, EPosition> EPosition_value_by_name_t;
EPosition_name_by_value_t EPosition_name_by_value;
EPosition_value_by_name_t EPosition_value_by_name;

void init_EPosition_ITEM_NAMES() {
	EPosition_name_by_value.clear();
	EPosition_value_by_name.clear();

	EPosition_name_by_value[EPosition::kUndefined] = "kUndefined";
	EPosition_name_by_value[EPosition::kDead] = "kDead";
	EPosition_name_by_value[EPosition::kPerish] = "kPerish";
	EPosition_name_by_value[EPosition::kIncap] = "kIncap";
	EPosition_name_by_value[EPosition::kStun] = "kStun";
	EPosition_name_by_value[EPosition::kSleep] = "kSleep";
	EPosition_name_by_value[EPosition::kRest] = "kRest";
	EPosition_name_by_value[EPosition::kSit] = "kSit";
	EPosition_name_by_value[EPosition::kFight] = "kFight";
	EPosition_name_by_value[EPosition::kStand] = "kStand";
	EPosition_name_by_value[EPosition::kLast] = "kLast";

	for (const auto &i : EPosition_name_by_value) {
		EPosition_value_by_name[i.second] = i.first;
	}
}

int operator-(EPosition p1,  EPosition p2) {
	return (static_cast<int>(p1) - static_cast<int>(p2));
}

EPosition operator--(const EPosition &p) {
	auto pp = (p == EPosition::kDead) ? EPosition::kDead : static_cast<EPosition>(static_cast<int>(p)-1);
	return pp;
}

template<>
const std::string &NAME_BY_ITEM<EPosition>(const EPosition item) {
	if (EPosition_name_by_value.empty()) {
		init_EPosition_ITEM_NAMES();
	}
	return EPosition_name_by_value.at(item);
}

template<>
EPosition ITEM_BY_NAME<EPosition>(const std::string &name) {
	if (EPosition_name_by_value.empty()) {
		init_EPosition_ITEM_NAMES();
	}
	return EPosition_value_by_name.at(name);
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
	EBaseStat_name_by_value[EBaseStat::kWis] = "kWis";
	EBaseStat_name_by_value[EBaseStat::kInt] = "kInt";
	EBaseStat_name_by_value[EBaseStat::kCha] = "kCha";

	for (const auto &i : EBaseStat_name_by_value) {
		EBaseStat_value_by_name[i.second] = i.first;
	}
}

EBaseStat& operator++(EBaseStat &s) {
	s =  static_cast<EBaseStat>(to_underlying(s) + 1);
	return s;
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
	ESaving_name_by_value[ESaving::kNone] = "kNone";

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

ESaving& operator++(ESaving &s) {
	s =  static_cast<ESaving>(to_underlying(s) + 1);
	return s;
}

typedef std::map<EObjType, std::string> EObjectType_name_by_value_t;
typedef std::map<const std::string, EObjType> EObjectType_value_by_name_t;
EObjectType_name_by_value_t EObjectType_name_by_value;
EObjectType_value_by_name_t EObjectType_value_by_name;
void init_EObjectType_ITEM_NAMES() {
	EObjectType_value_by_name.clear();
	EObjectType_name_by_value.clear();

	EObjectType_name_by_value[EObjType::kLightSource] = "kLight";
	EObjectType_name_by_value[EObjType::kScroll] = "kScroll";
	EObjectType_name_by_value[EObjType::kWand] = "kWand";
	EObjectType_name_by_value[EObjType::kStaff] = "kStaff";
	EObjectType_name_by_value[EObjType::kWeapon] = "kWeapon";
	EObjectType_name_by_value[EObjType::kElementWeapon] = "kElementWeapon";
	EObjectType_name_by_value[EObjType::kMissile] = "kMissile";
	EObjectType_name_by_value[EObjType::kTreasure] = "kTreasure";
	EObjectType_name_by_value[EObjType::kArmor] = "kArmor";
	EObjectType_name_by_value[EObjType::kPotion] = "kPorion";
	EObjectType_name_by_value[EObjType::kWorm] = "kWorm";
	EObjectType_name_by_value[EObjType::kOther] = "kOther";
	EObjectType_name_by_value[EObjType::kTrash] = "kTrash";
	EObjectType_name_by_value[EObjType::kTrap] = "kTrap";
	EObjectType_name_by_value[EObjType::kContainer] = "kContainer";
	EObjectType_name_by_value[EObjType::kNote] = "kNote";
	EObjectType_name_by_value[EObjType::kLiquidContainer] = "kLiquidContainer";
	EObjectType_name_by_value[EObjType::kKey] = "kKey";
	EObjectType_name_by_value[EObjType::kFood] = "kFood";
	EObjectType_name_by_value[EObjType::kMoney] = "kMoney";
	EObjectType_name_by_value[EObjType::kPen] = "kPen";
	EObjectType_name_by_value[EObjType::kBoat] = "kBoat";
	EObjectType_name_by_value[EObjType::kFountain] = "kFounrain";
	EObjectType_name_by_value[EObjType::kBook] = "kBook";
	EObjectType_name_by_value[EObjType::kIngredient] = "kIngredient";
	EObjectType_name_by_value[EObjType::kMagicIngredient] = "kMagicIngredient";
	EObjectType_name_by_value[EObjType::kCraftMaterial] = "kCraftMaterial";
	EObjectType_name_by_value[EObjType::kBandage] = "kBandage";
	EObjectType_name_by_value[EObjType::kLightArmor] = "kLightArmor";
	EObjectType_name_by_value[EObjType::kMediumArmor] = "kMediumArmor";
	EObjectType_name_by_value[EObjType::kHeavyArmor] = "kHeavyArmor";
	EObjectType_name_by_value[EObjType::kEnchant] = "kEnchant";
	EObjectType_name_by_value[EObjType::kMagicMaterial] = "kMagicMaterial";
	EObjectType_name_by_value[EObjType::kMagicArrow] = "kMagicArrow";
	EObjectType_name_by_value[EObjType::kMagicContaner] = "kMagicContaner";
	EObjectType_name_by_value[EObjType::kCraftMaterial2] = "kCraftMaterial2";

	for (const auto &i : EObjectType_name_by_value) {
		EObjectType_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EObjType>(const EObjType item) {
	if (EObjectType_name_by_value.empty()) {
		init_EObjectType_ITEM_NAMES();
	}
	return EObjectType_name_by_value.at(item);
}

template<>
EObjType ITEM_BY_NAME(const std::string &name) {
	if (EObjectType_name_by_value.empty()) {
		init_EObjectType_ITEM_NAMES();
	}
	return EObjectType_value_by_name.at(name);
}

typedef std::map<EObjMaterial, std::string> EObjMaterial_name_by_value_t;
typedef std::map<const std::string, EObjMaterial> EObjMaterial_value_by_name_t;
EObjMaterial_name_by_value_t EObjMaterial_name_by_value;
EObjMaterial_value_by_name_t EObjMaterial_value_by_name;
void init_EObjMaterial_ITEM_NAMES() {
	EObjMaterial_value_by_name.clear();
	EObjMaterial_name_by_value.clear();

	EObjMaterial_name_by_value[EObjMaterial::kMaterialUndefined] = "kMaterialUndefined";
	EObjMaterial_name_by_value[EObjMaterial::kBulat] = "kBulat";
	EObjMaterial_name_by_value[EObjMaterial::kBronze] = "kBronze";
	EObjMaterial_name_by_value[EObjMaterial::kIron] = "kIron";
	EObjMaterial_name_by_value[EObjMaterial::kSteel] = "kSteel";
	EObjMaterial_name_by_value[EObjMaterial::kForgedSteel] = "kForgedSteel";
	EObjMaterial_name_by_value[EObjMaterial::kPreciousMetel] = "kPreciousMetel";
	EObjMaterial_name_by_value[EObjMaterial::kCrystal] = "kCrystal";
	EObjMaterial_name_by_value[EObjMaterial::kWood] = "kWood";
	EObjMaterial_name_by_value[EObjMaterial::kHardWood] = "kSolidWood";
	EObjMaterial_name_by_value[EObjMaterial::kCeramic] = "kCeramic";
	EObjMaterial_name_by_value[EObjMaterial::kGlass] = "kGlass";
	EObjMaterial_name_by_value[EObjMaterial::kStone] = "kStone";
	EObjMaterial_name_by_value[EObjMaterial::kBone] = "kBone";
	EObjMaterial_name_by_value[EObjMaterial::kCloth] = "kCloth";
	EObjMaterial_name_by_value[EObjMaterial::kSkin] = "kSkin";
	EObjMaterial_name_by_value[EObjMaterial::kOrganic] = "kOrganic";
	EObjMaterial_name_by_value[EObjMaterial::kPaper] = "kPaper";
	EObjMaterial_name_by_value[EObjMaterial::kDiamond] = "kDiamond";

	for (const auto &i : EObjMaterial_name_by_value) {
		EObjMaterial_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EObjMaterial>(const EObjMaterial item) {
	if (EObjMaterial_name_by_value.empty()) {
		init_EObjMaterial_ITEM_NAMES();
	}
	return EObjMaterial_name_by_value.at(item);
}

template<>
EObjMaterial ITEM_BY_NAME(const std::string &name) {
	if (EObjMaterial_name_by_value.empty()) {
		init_EObjMaterial_ITEM_NAMES();
	}
	return EObjMaterial_value_by_name.at(name);
}

typedef std::map<EResist, std::string> EResist_name_by_value_t;
typedef std::map<const std::string, EResist> EResist_value_by_name_t;
EResist_name_by_value_t EResist_name_by_value;
EResist_value_by_name_t EResist_value_by_name;

void init_EResist_ITEM_NAMES() {
	EResist_name_by_value.clear();
	EResist_value_by_name.clear();

	EResist_name_by_value[EResist::kFire] = "kFire";
	EResist_name_by_value[EResist::kAir] = "kAir";
	EResist_name_by_value[EResist::kWater] = "kWater";
	EResist_name_by_value[EResist::kEarth] = "kEarth";
	EResist_name_by_value[EResist::kVitality] = "kVitality";
	EResist_name_by_value[EResist::kMind] = "kMind";
	EResist_name_by_value[EResist::kImmunity] = "kImmunity";
	EResist_name_by_value[EResist::kDark] = "kDark";

	for (const auto &i : EResist_name_by_value) {
		EResist_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EResist>(const EResist item) {
	if (EResist_name_by_value.empty()) {
		init_EResist_ITEM_NAMES();
	}
	return EResist_name_by_value.at(item);
}

template<>
EResist ITEM_BY_NAME<EResist>(const std::string &name) {
	if (EResist_name_by_value.empty()) {
		init_EResist_ITEM_NAMES();
	}
	return EResist_value_by_name.at(name);
}

EResist& operator++(EResist &r) {
	r =  static_cast<EResist>(to_underlying(r) + 1);
	return r;
}

EDirection& operator++(EDirection &d) {
	d =  static_cast<EDirection>(to_underlying(d) + 1);
	return d;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :