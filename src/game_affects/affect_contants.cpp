/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы системы аффектов.
*/

//#include "affect_contants.h"

#include "game_magic/spells.h"

const char *affected_bits[] = {"слепота",    // 0
							   "невидимость",
							   "определение наклонностей",
							   "определение невидимости",
							   "определение магии",
							   "чувствовать жизнь",
							   "хождение по воде",
							   "освящение",
							   "состоит в группе",
							   "проклятие",
							   "инфравидение",        // 10
							   "яд",
							   "защита от тьмы",
							   "защита от света",
							   "магический сон",
							   "нельзя выследить",
							   "привязан",
							   "доблесть",
							   "подкрадывается",
							   "прячется",
							   "ярость",        // 20
							   "зачарован",
							   "оцепенение",
							   "летит",
							   "молчание",
							   "настороженность",
							   "мигание",
							   "верхом или под седлом",
							   "не сбежать",
							   "свет",
							   "\n",
							   "освещение",        // 0
							   "затемнение",
							   "определение яда",
							   "под мухой",
							   "отходняк",
							   "декстраплегия",
							   "синистроплегия",
							   "параплегия",
							   "кровотечение",
							   "маскировка",
							   "дышать водой",
							   "медлительность",
							   "ускорение",
							   "защита богов",
							   "воздушный щит",
							   "огненный щит",
							   "ледяной щит",
							   "зеркало магии",
							   "звезды",
							   "каменная рука",
							   "призматическая.аура",
							   "нанят",
							   "силы зла",
							   "воздушная аура",
							   "огненная аура",
							   "ледяная аура",
							   "глухота",
							   "плач",
							   "смирение",
							   "маг параплегия",
							   "\n",
							   "предсмертная ярость",
							   "легкая поступь",
							   "разбитые оковы",
							   "облако стрел",
							   "мантия теней",
							   "блестящая пыль",
							   "испуг",
							   "яд скополии",
							   "яд дурмана",
							   "понижение умений",
							   "не переключается",
							   "яд белены",
							   "прикован",
							   "боевое везение",
							   "перевязка",
							   "не может перевязываться",
							   "превращен",
							   "удушье",
							   "вспоминает заклинания",
							   "регенерация новичка",
							   "вампиризм",
							   "рваные раны",
							   "полководец",
							   "земной поклон",
							   "\n",
							   "\n",
};

typedef std::map<EAffectFlag, std::string> EAffectFlag_name_by_value_t;
typedef std::map<const std::string, EAffectFlag> EAffectFlag_value_by_name_t;
EAffectFlag_name_by_value_t EAffectFlag_name_by_value;
EAffectFlag_value_by_name_t EAffectFlag_value_by_name;
void init_EAffectFlag_ITEM_NAMES() {
	EAffectFlag_value_by_name.clear();
	EAffectFlag_name_by_value.clear();

	EAffectFlag_name_by_value[EAffectFlag::kAfNone]			= "kAfNone";
	EAffectFlag_name_by_value[EAffectFlag::kAfBattledec]	= "kAfBattledec";
	EAffectFlag_name_by_value[EAffectFlag::kAfDeadkeep]		= "kAfDeadkeep";
	EAffectFlag_name_by_value[EAffectFlag::kAfPulsedec]		= "kAfPulsedec";
	EAffectFlag_name_by_value[EAffectFlag::kAfSameTime]		= "kAfSameTime";
	EAffectFlag_name_by_value[EAffectFlag::kAfCurable]		= "kAfCurable";
	EAffectFlag_name_by_value[EAffectFlag::kAfDispelable]	= "kAfDispelable";
	for (const auto &i : EAffectFlag_name_by_value) {
		EAffectFlag_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM(const EAffectFlag item) {
	if (EAffectFlag_name_by_value.empty()) {
		init_EAffectFlag_ITEM_NAMES();
	}
	return EAffectFlag_name_by_value.at(item);
}

template<>
EAffectFlag ITEM_BY_NAME(const std::string &name) {
	if (EAffectFlag_name_by_value.empty()) {
		init_EAffectFlag_ITEM_NAMES();
	}
	return EAffectFlag_value_by_name.at(name);
}

typedef std::map<EAffect, std::string> EAffect_name_by_value_t;
typedef std::map<const std::string, EAffect> EAffect_value_by_name_t;
EAffect_name_by_value_t EAffect_name_by_value;
EAffect_value_by_name_t EAffect_value_by_name;
void init_EAffect_ITEM_NAMES() {
	EAffect_value_by_name.clear();
	EAffect_name_by_value.clear();

	EAffect_name_by_value[EAffect::kUndefinded] = "kUndefinded";
	EAffect_name_by_value[EAffect::kBlind] = "kBlind";
	EAffect_name_by_value[EAffect::kInvisible] = "kInvisible";
	EAffect_name_by_value[EAffect::kDetectAlign] = "kDetectAlign";
	EAffect_name_by_value[EAffect::kDetectInvisible] = "kDetectInvisible";
	EAffect_name_by_value[EAffect::kDetectMagic] = "kDetectMagic";
	EAffect_name_by_value[EAffect::kDetectLife] = "kDetectLife";
	EAffect_name_by_value[EAffect::kWaterWalk] = "kWaterWalk";
	EAffect_name_by_value[EAffect::kSanctuary] = "kSanctuary";
	EAffect_name_by_value[EAffect::kGroup] = "kGroup";
	EAffect_name_by_value[EAffect::kCurse] = "kCurse";
	EAffect_name_by_value[EAffect::kInfravision] = "kInfravision";
	EAffect_name_by_value[EAffect::kPoisoned] = "kPoisoned";
	EAffect_name_by_value[EAffect::kProtectFromDark] = "kProtectFromDark";
	EAffect_name_by_value[EAffect::kProtectFromMind] = "kProtectFromMind";
	EAffect_name_by_value[EAffect::kSleep] = "kSleep";
	EAffect_name_by_value[EAffect::kNoTrack] = "kNoTrack";
	EAffect_name_by_value[EAffect::kTethered] = "kTethered";
	EAffect_name_by_value[EAffect::kBless] = "kBless";
	EAffect_name_by_value[EAffect::kSneak] = "kSneak";
	EAffect_name_by_value[EAffect::kHide] = "kHide";
	EAffect_name_by_value[EAffect::kCourage] = "kCourage";
	EAffect_name_by_value[EAffect::kCharmed] = "kCharmed";
	EAffect_name_by_value[EAffect::kHold] = "kHold";
	EAffect_name_by_value[EAffect::kFly] = "kFly";
	EAffect_name_by_value[EAffect::kSilence] = "kSilence";
	EAffect_name_by_value[EAffect::kAwarness] = "kAwarness";
	EAffect_name_by_value[EAffect::kBlink] = "kBlink";
	EAffect_name_by_value[EAffect::kHorse] = "kHorse";
	EAffect_name_by_value[EAffect::kNoFlee] = "kNoFlee";
	EAffect_name_by_value[EAffect::kSingleLight] = "kSingleLight";
	EAffect_name_by_value[EAffect::kHolyLight] = "kHolyLight";
	EAffect_name_by_value[EAffect::kHolyDark] = "kHolyDark";
	EAffect_name_by_value[EAffect::kDetectPoison] = "kDetectPoison";
	EAffect_name_by_value[EAffect::kDrunked] = "kDrunked";
	EAffect_name_by_value[EAffect::kAbstinent] = "kAbstinent";
	EAffect_name_by_value[EAffect::kStopRight] = "kStopRight";
	EAffect_name_by_value[EAffect::kStopLeft] = "kStopLeft";
	EAffect_name_by_value[EAffect::kStopFight] = "kStopFight";
	EAffect_name_by_value[EAffect::kHaemorrhage] = "kHaemorrhage";
	EAffect_name_by_value[EAffect::kDisguise] = "kDisguise";
	EAffect_name_by_value[EAffect::kWaterBreath] = "kWaterBreath";
	EAffect_name_by_value[EAffect::kSlow] = "kSlow";
	EAffect_name_by_value[EAffect::kHaste] = "kHaste";
	EAffect_name_by_value[EAffect::kGodsShield] = "kGodsShield";
	EAffect_name_by_value[EAffect::kAirShield] = "kAirShield";
	EAffect_name_by_value[EAffect::kFireShield] = "kFireShield";
	EAffect_name_by_value[EAffect::kIceShield] = "kIceShield";
	EAffect_name_by_value[EAffect::kMagicGlass] = "kMagicGlass";
	EAffect_name_by_value[EAffect::kStairs] = "kStairs";
	EAffect_name_by_value[EAffect::kStoneHands] = "kStoneHands";
	EAffect_name_by_value[EAffect::kPrismaticAura] = "kPrismaticAura";
	EAffect_name_by_value[EAffect::kHelper] = "kHelper";
	EAffect_name_by_value[EAffect::kForcesOfEvil] = "kForcesOfEvil";
	EAffect_name_by_value[EAffect::kAirAura] = "kAirAura";
	EAffect_name_by_value[EAffect::kFireAura] = "kFireAura";
	EAffect_name_by_value[EAffect::kIceAura] = "kIceAura";
	EAffect_name_by_value[EAffect::kDeafness] = "kDeafness";
	EAffect_name_by_value[EAffect::kCrying] = "kCrying";
	EAffect_name_by_value[EAffect::kPeaceful] = "kPeaceful";
	EAffect_name_by_value[EAffect::kMagicStopFight] = "kMagicStopFight";
	EAffect_name_by_value[EAffect::kBerserk] = "kBerserk";
	EAffect_name_by_value[EAffect::kLightWalk] = "kLightWalk";
	EAffect_name_by_value[EAffect::kBrokenChains] = "kBrokenChains";
	EAffect_name_by_value[EAffect::kCloudOfArrows] = "kCloudOfArrows";
	EAffect_name_by_value[EAffect::kShadowCloak] = "kShadowCloak";
	EAffect_name_by_value[EAffect::kGlitterDust] = "kGlitterDust";
	EAffect_name_by_value[EAffect::kAffright] = "kAffright";
	EAffect_name_by_value[EAffect::kScopolaPoison] = "kScopolaPoison";
	EAffect_name_by_value[EAffect::kDaturaPoison] = "kDaturaPoison";
	EAffect_name_by_value[EAffect::kSkillReduce] = "kSkillReduce";
	EAffect_name_by_value[EAffect::kNoBattleSwitch] = "kNoBattleSwitch";
	EAffect_name_by_value[EAffect::kBelenaPoison] = "kBelenaPoison";
	EAffect_name_by_value[EAffect::kNoTeleport] = "kNoTeleport";
	EAffect_name_by_value[EAffect::kCombatLuck] = "kCombatLuck";
	EAffect_name_by_value[EAffect::kBandage] = "kBandage";
	EAffect_name_by_value[EAffect::kCannotBeBandaged] = "kCannotBeBandaged";
	EAffect_name_by_value[EAffect::kMorphing] = "kMorphing";
	EAffect_name_by_value[EAffect::kStrangled] = "kStrangled";
	EAffect_name_by_value[EAffect::kMemorizeSpells] = "kMemorizeSpells";
	EAffect_name_by_value[EAffect::kNoobRegen] = "kNoobRegen";
	EAffect_name_by_value[EAffect::kVampirism] = "kVampirism";
	EAffect_name_by_value[EAffect::kCommander] = "kCommander";
	EAffect_name_by_value[EAffect::kEarthAura] = "kEarthAura";
	for (const auto &i : EAffect_name_by_value) {
		EAffect_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM(const EAffect item) {
	if (EAffect_name_by_value.empty()) {
		init_EAffect_ITEM_NAMES();
	}
	return EAffect_name_by_value.at(item);
}

template<>
EAffect ITEM_BY_NAME(const std::string &name) {
	if (EAffect_name_by_value.empty()) {
		init_EAffect_ITEM_NAMES();
	}
	return EAffect_value_by_name.at(name);
}

typedef std::map<EWeaponAffect, std::string> EWeaponAffectFlag_name_by_value_t;
typedef std::map<const std::string, EWeaponAffect> EWeaponAffectFlag_value_by_name_t;
EWeaponAffectFlag_name_by_value_t EWeaponAffectFlag_name_by_value;
EWeaponAffectFlag_value_by_name_t EWeaponAffectFlag_value_by_name;

void init_EWeaponAffectFlag_ITEM_NAMES() {
	EWeaponAffectFlag_name_by_value.clear();
	EWeaponAffectFlag_value_by_name.clear();

	EWeaponAffectFlag_name_by_value[EWeaponAffect::kBlindness] = "kBlindness";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kInvisibility] = "kInvisibility";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kDetectAlign] = "kDetectAlign";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kDetectInvisibility] = "kDetectInvisibility";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kDetectMagic] = "kDetectMagic";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kDetectLife] = "kDetectLife";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kWaterWalk] = "kWaterWalk";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kSanctuary] = "kSanctuary";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kCurse] = "kCurse";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kInfravision] = "kInfravision";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kPoison] = "kPoison";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kProtectFromDark] = "kProtectFromDark";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kProtectFromMind] = "kProtectFromEvil";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kSleep] = "kSleep";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kNoTrack] = "kNoTrack";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kBless] = "kBless";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kSneak] = "kSneak";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kHide] = "kHide";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kHold] = "kHold";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kFly] = "kFly";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kSilence] = "kSilence";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kAwareness] = "kAwareness";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kBlink] = "kBlink";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kNoFlee] = "kNoFlee";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kSingleLight] = "kSingleLight";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kHolyLight] = "kHolyLight";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kHolyDark] = "kHolyDark";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kDetectPoison] = "kDetectPoison";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kSlow] = "kSlow";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kHaste] = "kHaste";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kWaterBreath] = "kWaterBreath";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kHaemorrhage] = "kHaemorrhage";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kDisguising] = "kDisguising";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kShield] = "kShield";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kAirShield] = "kAirShield";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kFireShield] = "kFireShield";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kIceShield] = "kIceShield";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kMagicGlass] = "kMagicGlass";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kStoneHand] = "kStoneHand";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kPrismaticAura] = "kPrismaticAura";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kAirAura] = "kAirAura";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kFireAura] = "kFireAura";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kIceAura] = "kIceAura";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kDeafness] = "kDeafness";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kComamnder] = "kComamnder";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kEarthAura] = "kEarthAura";
	for (const auto &i : EWeaponAffectFlag_name_by_value) {
		EWeaponAffectFlag_value_by_name[i.second] = i.first;
	}
}

template<>
EWeaponAffect ITEM_BY_NAME(const std::string &name) {
	if (EWeaponAffectFlag_name_by_value.empty()) {
		init_EWeaponAffectFlag_ITEM_NAMES();
	}
	return EWeaponAffectFlag_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM(const EWeaponAffect item) {
	if (EWeaponAffectFlag_name_by_value.empty()) {
		init_EWeaponAffectFlag_ITEM_NAMES();
	}
	return EWeaponAffectFlag_name_by_value.at(item);
}

WeaponAffectArray weapon_affect = {
	WeaponAffect{EWeaponAffect::kBlindness, 0, ESpell::kBlindness},
	WeaponAffect{EWeaponAffect::kInvisibility, to_underlying(EAffect::kInvisible), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kDetectAlign, to_underlying(EAffect::kDetectAlign), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kDetectInvisibility, to_underlying(EAffect::kDetectInvisible), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kDetectMagic, to_underlying(EAffect::kDetectMagic), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kDetectLife, to_underlying(EAffect::kDetectLife), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kWaterWalk, to_underlying(EAffect::kWaterWalk), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kSanctuary, to_underlying(EAffect::kSanctuary), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kCurse, to_underlying(EAffect::kCurse), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kInfravision, to_underlying(EAffect::kInfravision), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kPoison, 0, ESpell::kPoison},
	WeaponAffect{EWeaponAffect::kProtectFromDark, to_underlying(EAffect::kProtectFromDark), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kProtectFromMind, to_underlying(EAffect::kProtectFromMind), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kSleep, 0, ESpell::kSleep},
	WeaponAffect{EWeaponAffect::kNoTrack, to_underlying(EAffect::kNoTrack), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kBless, to_underlying(EAffect::kBless), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kSneak, to_underlying(EAffect::kSneak), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kHide, to_underlying(EAffect::kHide), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kHold, 0, ESpell::kHold},
	WeaponAffect{EWeaponAffect::kFly, to_underlying(EAffect::kFly), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kSilence, to_underlying(EAffect::kSilence), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kAwareness, to_underlying(EAffect::kAwarness), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kBlink, to_underlying(EAffect::kBlink), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kNoFlee, to_underlying(EAffect::kNoFlee), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kSingleLight, to_underlying(EAffect::kSingleLight), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kHolyLight, to_underlying(EAffect::kHolyLight), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kHolyDark, to_underlying(EAffect::kHolyDark), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kDetectPoison, to_underlying(EAffect::kDetectPoison), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kSlow, to_underlying(EAffect::kSlow), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kHaste, to_underlying(EAffect::kHaste), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kWaterBreath, to_underlying(EAffect::kWaterBreath), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kHaemorrhage, to_underlying(EAffect::kHaemorrhage), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kDisguising, to_underlying(EAffect::kDisguise), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kShield, to_underlying(EAffect::kGodsShield), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kAirShield, to_underlying(EAffect::kAirShield), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kFireShield, to_underlying(EAffect::kFireShield), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kIceShield, to_underlying(EAffect::kIceShield), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kMagicGlass, to_underlying(EAffect::kMagicGlass), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kStoneHand, to_underlying(EAffect::kStoneHands), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kPrismaticAura, to_underlying(EAffect::kPrismaticAura), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kAirAura, to_underlying(EAffect::kAirAura), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kFireAura, to_underlying(EAffect::kFireAura), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kIceAura, to_underlying(EAffect::kIceAura), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kDeafness, to_underlying(EAffect::kDeafness), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kComamnder, to_underlying(EAffect::kCommander), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kEarthAura, to_underlying(EAffect::kEarthAura), ESpell::kUndefined}
};


// APPLY_x - к чему применяется аффект или бонусы предмета (по сути, урезанные аффекты)

const char *apply_types[] = {"ничего",
							 "сила",
							 "ловкость",
							 "интеллект",
							 "мудрость",
							 "телосложение",
							 "обаяние",
							 "профессия",
							 "уровень",
							 "возраст",
							 "вес",
							 "рост",
							 "запоминание",
							 "макс.жизнь",
							 "макс.энергия",
							 "деньги",
							 "опыт",
							 "защита",
							 "попадание",
							 "повреждение",
							 "воля",
							 "защита.от.стихии.огня",
							 "защита.от.стихии.воздуха",
							 "здоровье",
							 "стойкость",
							 "восст.жизни",
							 "восст.энергии",
							 "слот.1",
							 "слот.2",
							 "слот.3",
							 "слот.4",
							 "слот.5",
							 "слот.6",
							 "слот.7",
							 "слот.8",
							 "слот.9",
							 "размер",
							 "броня",
							 "яд",
							 "реакция",
							 "успех.колдовства",
							 "удача",
							 "инициатива",
							 "религия(НЕ СТАВИТЬ)",
							 "поглощение",
							 "трудолюбие",
							 "защита.от.стихии.воды",
							 "защита.от.стихии.земли",
							 "защита.от.тяжелых.ран",				// живучесть
							 "защита.от.магии.разума",
							 "защита.от.ядов.и.болезней",			// иммунитет
							 "защита.от.чар",
							 "защита.от.магических.повреждений",
							 "яд аконита",
							 "яд скополии",
							 "яд белены",
							 "яд дурмана",
							 "*не используется",
							 "*множитель опыта",
							 "бонус ко всем умениям",
							 "лихорадка",
							 "безумное бормотание",
							 "защита.от.физических.повреждений",
							 "защита.от.магии.тьмы",
							 "роковое предчувствие",
							 "*дополнительный бонус опыта",
							 "бонус физ. повреждений %",
							 "заклинание \"волшебное уклонение\"",
							 "бонус маг. повреждений %",
							 "\n"
};

typedef std::map<EApply, std::string> EApplyLocation_name_by_value_t;
typedef std::map<const std::string, EApply> EApplyLocation_value_by_name_t;
EApplyLocation_name_by_value_t EApplyLocation_name_by_value;
EApplyLocation_value_by_name_t EApplyLocation_value_by_name;

void init_EApplyLocation_ITEM_NAMES() {
	EApplyLocation_name_by_value.clear();
	EApplyLocation_value_by_name.clear();

	EApplyLocation_name_by_value[EApply::kNone] = "kNone";
	EApplyLocation_name_by_value[EApply::kStr] = "kStr";
	EApplyLocation_name_by_value[EApply::kDex] = "kDex";
	EApplyLocation_name_by_value[EApply::kInt] = "kInt";
	EApplyLocation_name_by_value[EApply::kWis] = "kWis";
	EApplyLocation_name_by_value[EApply::kCon] = "kCon";
	EApplyLocation_name_by_value[EApply::kCha] = "kCha";
	EApplyLocation_name_by_value[EApply::kClass] = "kClass";
	EApplyLocation_name_by_value[EApply::kLvl] = "kLvl";
	EApplyLocation_name_by_value[EApply::kAge] = "kAge";
	EApplyLocation_name_by_value[EApply::kWeight] = "kWeight";
	EApplyLocation_name_by_value[EApply::kHeight] = "kHeight";
	EApplyLocation_name_by_value[EApply::kManaRegen] = "kManaRegen";
	EApplyLocation_name_by_value[EApply::kHp] = "kHp";
	EApplyLocation_name_by_value[EApply::kMove] = "kMove";
	EApplyLocation_name_by_value[EApply::kGold] = "kGold";
	EApplyLocation_name_by_value[EApply::kExp] = "kExp";
	EApplyLocation_name_by_value[EApply::kAc] = "kAc";
	EApplyLocation_name_by_value[EApply::kHitroll] = "kHitroll";
	EApplyLocation_name_by_value[EApply::kDamroll] = "kDamroll";
	EApplyLocation_name_by_value[EApply::kSavingWill] = "kSavingWill";
	EApplyLocation_name_by_value[EApply::kResistFire] = "kResistFire";
	EApplyLocation_name_by_value[EApply::kResistAir] = "kResistAir";
	EApplyLocation_name_by_value[EApply::kResistDark] = "kResistDark";
	EApplyLocation_name_by_value[EApply::kSavingCritical] = "kSavingCritical";
	EApplyLocation_name_by_value[EApply::kSavingStability] = "kSavingStability";
	EApplyLocation_name_by_value[EApply::kHpRegen] = "kHpRegen";
	EApplyLocation_name_by_value[EApply::kMoveRegen] = "kMoveRegen";
	EApplyLocation_name_by_value[EApply::kFirstCircle] = "kFirstCircle";
	EApplyLocation_name_by_value[EApply::kSecondCircle] = "kSecondCircle";
	EApplyLocation_name_by_value[EApply::kThirdCircle] = "kThirdCircle";
	EApplyLocation_name_by_value[EApply::kFourthCircle] = "kFourthCircle";
	EApplyLocation_name_by_value[EApply::kFifthCircle] = "kFifthCircle";
	EApplyLocation_name_by_value[EApply::kSixthCircle] = "kSixthCircle";
	EApplyLocation_name_by_value[EApply::kSeventhCircle] = "kSeventhCircle";
	EApplyLocation_name_by_value[EApply::kEighthCircle] = "kEighthCircle";
	EApplyLocation_name_by_value[EApply::kNinthCircle] = "kNinthCircle";
	EApplyLocation_name_by_value[EApply::kSize] = "kSize";
	EApplyLocation_name_by_value[EApply::kArmour] = "kArmour";
	EApplyLocation_name_by_value[EApply::kPoison] = "kPoison";
	EApplyLocation_name_by_value[EApply::kSavingReflex] = "kSavingReflex";
	EApplyLocation_name_by_value[EApply::kCastSuccess] = "kCastSuccess";
	EApplyLocation_name_by_value[EApply::kMorale] = "kMorale";
	EApplyLocation_name_by_value[EApply::kInitiative] = "kInitiative";
	EApplyLocation_name_by_value[EApply::kReligion] = "kReligion";
	EApplyLocation_name_by_value[EApply::kAbsorbe] = "kAbsorbe";
	EApplyLocation_name_by_value[EApply::kLikes] = "kLikes";
	EApplyLocation_name_by_value[EApply::kResistWater] = "kResistWater";
	EApplyLocation_name_by_value[EApply::kResistEarth] = "kResistEarth";
	EApplyLocation_name_by_value[EApply::kResistVitality] = "kResistVitality";
	EApplyLocation_name_by_value[EApply::kResistMind] = "kResistMind";
	EApplyLocation_name_by_value[EApply::kResistImmunity] = "kResistImmunity";
	EApplyLocation_name_by_value[EApply::kAffectResist] = "kAffectResist";
	EApplyLocation_name_by_value[EApply::kMagicResist] = "kMagicResist";
	EApplyLocation_name_by_value[EApply::kAconitumPoison] = "kAconitumPoison";
	EApplyLocation_name_by_value[EApply::kScopolaPoison] = "kScopolaPoison";
	EApplyLocation_name_by_value[EApply::kBelenaPoison] = "kBelenaPoison";
	EApplyLocation_name_by_value[EApply::kDaturaPoison] = "kDaturaPoison";
	EApplyLocation_name_by_value[EApply::kFreeForUse1] = "kFreeForUse1";
	EApplyLocation_name_by_value[EApply::kExpBonus] = "kExpBonus";
	EApplyLocation_name_by_value[EApply::kSkillsBonus] = "kSkillsBonus";
	EApplyLocation_name_by_value[EApply::kPlague] = "kPlague";
	EApplyLocation_name_by_value[EApply::kMadness] = "kMadness";
	EApplyLocation_name_by_value[EApply::kPhysicResist] = "kPhysicResist";
	EApplyLocation_name_by_value[EApply::kResistDark] = "kResistDark";
	EApplyLocation_name_by_value[EApply::kViewDeathTraps] = "kViewDeathTraps";
	EApplyLocation_name_by_value[EApply::kExpPercent] = "kExpPercent";
	EApplyLocation_name_by_value[EApply::kPhysicDamagePercent] = "kPhysicDamagePercent";
	EApplyLocation_name_by_value[EApply::kSpelledBlink] =  "kSpelledBlink";
	EApplyLocation_name_by_value[EApply::kNumberApplies] = "kNumberApplies";
	EApplyLocation_name_by_value[EApply::kMagicDamagePercent] = "kMagicDamagePercent";
	for (const auto &i : EApplyLocation_name_by_value) {
		EApplyLocation_value_by_name[i.second] = i.first;
	}
}

template<>
EApply ITEM_BY_NAME(const std::string &name) {
	if (EApplyLocation_name_by_value.empty()) {
		init_EApplyLocation_ITEM_NAMES();
	}
	return EApplyLocation_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM(const EApply item) {
	if (EApplyLocation_name_by_value.empty()) {
		init_EApplyLocation_ITEM_NAMES();
	}
	return EApplyLocation_name_by_value.at(item);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
