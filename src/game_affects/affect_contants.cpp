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

	EAffectFlag_name_by_value[EAffectFlag::kAfNone] = "kAfNone";
	EAffectFlag_name_by_value[EAffectFlag::kAfBattledec] = "kAfBattledec";
	EAffectFlag_name_by_value[EAffectFlag::kAfDeadkeep] = "kAfDeadkeep";
	EAffectFlag_name_by_value[EAffectFlag::kAfPulsedec] = "kAfPulsedec";
	EAffectFlag_name_by_value[EAffectFlag::kAfSameTime] = "kAfSameTime";
	EAffectFlag_name_by_value[EAffectFlag::kAfCurable] = "kAfCurable";
	EAffectFlag_name_by_value[EAffectFlag::kAfDispelable] = "kAfDispelable";
	for (const auto &i: EAffectFlag_name_by_value) {
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
	for (const auto &i: EAffect_name_by_value) {
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
	for (const auto &i: EWeaponAffectFlag_name_by_value) {
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
							 "защита.от.тяжелых.ран",                // живучесть
							 "защита.от.магии.разума",
							 "защита.от.ядов.и.болезней",            // иммунитет
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
	EApplyLocation_name_by_value[EApply::kSpelledBlink] = "kSpelledBlink";
	EApplyLocation_name_by_value[EApply::kNumberApplies] = "kNumberApplies";
	EApplyLocation_name_by_value[EApply::kMagicDamagePercent] = "kMagicDamagePercent";
	for (const auto &i: EApplyLocation_name_by_value) {
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

enum class EAffectMsg {
	kName,
	kDecs,
	kExpired,
	kImposedForActor,
	kImposedForVict,
	kImposedForRoom
};

using AffectMsgSet = std::map<EAffectMsg, std::string>;

const std::string &GetAffectMsg(EAffect affect_id, EAffectMsg msg_id) {
	static const std::unordered_map<EAffect, AffectMsgSet> affects_msg = {
			{EAffect::kUndefinded, {
				{EAffectMsg::kName, "Нечто непонятное, сообщите богам!"},
				{EAffectMsg::kExpired, ""},
				{EAffectMsg::kImposedForRoom, ""},
				{EAffectMsg::kImposedForVict, "Произошло нечто непонятное, сообщите богам!"}}},
			{EAffect::kBlind, {
				{EAffectMsg::kName, "слепота"},
				{EAffectMsg::kDecs, "...слеп$a"},
				{EAffectMsg::kExpired, "Вы вновь можете видеть."},
				{EAffectMsg::kImposedForRoom, "$n0 ослеп$q!"},
				{EAffectMsg::kImposedForVict, "Вы ослепли!"}}},
			{EAffect::kInvisible, {
				{EAffectMsg::kName, "невидимость"},
				{EAffectMsg::kDecs, "(невидим%s)"},
				{EAffectMsg::kExpired, "Вы вновь видимы."},
				{EAffectMsg::kImposedForVict, "Вы стали невидимы для окружающих."},
				{EAffectMsg::kImposedForRoom, "$n медленно растворил$u в пустоте."}}},
			{EAffect::kDetectAlign, {
				{EAffectMsg::kName, "определение наклонностей"},
				{EAffectMsg::kExpired, "Вы более не можете определять наклонности."},
				{EAffectMsg::kImposedForVict, "Ваши глаза приобрели зеленый оттенок."},
				{EAffectMsg::kImposedForRoom, "Глаза $n1 приобрели зеленый оттенок."}}},
			{EAffect::kDetectInvisible, {
				{EAffectMsg::kName, "определение невидимости"},
				{EAffectMsg::kExpired, "Вы не в состоянии больше видеть невидимых."},
				{EAffectMsg::kImposedForVict, "Ваши глаза приобрели золотистый оттенок."},
				{EAffectMsg::kImposedForRoom, "Глаза $n1 приобрели золотистый оттенок."}}},
			{EAffect::kDetectMagic, {
				{EAffectMsg::kName, "определение магии"},
				{EAffectMsg::kExpired, "Вы не в состоянии более определять магию."},
				{EAffectMsg::kImposedForVict, "Ваши глаза приобрели желтый оттенок."},
				{EAffectMsg::kImposedForRoom, "Глаза $n1 приобрели желтый оттенок."}}},
			{EAffect::kDetectLife, {
				{EAffectMsg::kName, "чувствовать жизнь"},
				{EAffectMsg::kExpired, "Вы больше не можете чувствовать жизнь."},
				{EAffectMsg::kImposedForVict, "Вы способны разглядеть даже микроба."},
				{EAffectMsg::kImposedForRoom, "$n0 начал$g замечать любые движения."}}},
			{EAffect::kWaterWalk, {
				{EAffectMsg::kName, "хождение по воде"},
				{EAffectMsg::kExpired, "Вы больше не можете ходить по воде."},
				{EAffectMsg::kImposedForVict, "На рыбалку вы можете отправляться без лодки."}}},
			{EAffect::kSanctuary, {
				{EAffectMsg::kName, "освящение"},
				{EAffectMsg::kExpired, "Белая аура вокруг вашего тела угасла."},
				{EAffectMsg::kImposedForVict, "Белая аура мгновенно окружила вас."},
				{EAffectMsg::kImposedForRoom, "Белая аура покрыла $n3 с головы до пят."}}},
			{EAffect::kGroup, {
				{EAffectMsg::kName, "состоит в группе"},
				{EAffectMsg::kExpired, "Вы больше не состоите в группе."},
				{EAffectMsg::kImposedForActor, "$N принят$A в члены вашего кружка (тьфу-ты, группы :)."},
				{EAffectMsg::kImposedForVict, "Вы приняты в группу $n1."},
				{EAffectMsg::kImposedForRoom, "$N принят$A в группу $n1."}}},
			{EAffect::kCurse, {
				{EAffectMsg::kName, "проклятие"},
				{EAffectMsg::kExpired, "Вы почувствовали себя более уверенно."},
				{EAffectMsg::kImposedForRoom, "Красное сияние вспыхнуло над $n4 и тут же погасло!"},
				{EAffectMsg::kImposedForVict, "Боги сурово поглядели на вас."}}},
			{EAffect::kInfravision, {
				{EAffectMsg::kName, "инфравидение"},
				{EAffectMsg::kExpired, "Вы больше не можете видеть ночью."},
				{EAffectMsg::kImposedForVict, "Ваши глаза приобрели красный оттенок."},
				{EAffectMsg::kImposedForRoom, "Глаза $n1 приобрели красный оттенок."}}},
			{EAffect::kPoisoned, {
				{EAffectMsg::kName, "отравление"},
				{EAffectMsg::kExpired, "В вашей крови не осталось ни капельки яда."},
				{EAffectMsg::kImposedForVict, "Вы почувствовали себя отравленным."},
				{EAffectMsg::kImposedForRoom, "$n позеленел$g от действия яда."}}},
			{EAffect::kProtectFromDark, {
				{EAffectMsg::kName, "защита от света"},
				{EAffectMsg::kExpired, "Вы вновь ощущаете страх перед тьмой."},
				{EAffectMsg::kImposedForVict, "Вы подавили в себе страх к тьме."},
				{EAffectMsg::kImposedForRoom, "$n подавил$g в себе страх к тьме."}}},
			{EAffect::kProtectFromMind, { // Похоже, это не используется. Удалить?
				{EAffectMsg::kName, "безмозглость"},
				{EAffectMsg::kExpired, "Вы слегка поумнели."},
				{EAffectMsg::kImposedForVict, "Ыыг-угуу?"},
				{EAffectMsg::kImposedForRoom, "$n резко поглупел$g... хотя казалось бы, куда?"}}},
			{EAffect::kSleep, {
				{EAffectMsg::kName, "беспробудный сон"},
				{EAffectMsg::kExpired, "Вы не чувствуете сонливости."},
				{EAffectMsg::kImposedForVict, "Вы слишком устали... Спать... Спа..."},
				{EAffectMsg::kImposedForRoom, "$n прилег$q подремать."}}},
			{EAffect::kNoTrack, {
				{EAffectMsg::kName, "нельзя выследить"}}},
			{EAffect::kTethered, {
				{EAffectMsg::kName, "привязан"},
				{EAffectMsg::kExpired, "Привязь распустилась."},
				{EAffectMsg::kImposedForActor, "$N принят$A в члены вашего кружка (тьфу-ты, группы :)."},
				{EAffectMsg::kImposedForVict, "Вы слишком устали... Спать... Спа..."},
				{EAffectMsg::kImposedForRoom, "$n прилег$q подремать."}}},
			{EAffect::kBless, {
				{EAffectMsg::kName, "доблесть"},
				{EAffectMsg::kExpired, "Вы почувствовали себя менее доблестно."},
				{EAffectMsg::kImposedForActor, "Вы благословили $N1."},
				{EAffectMsg::kImposedForVict, "Боги одарили вас своей улыбкой."},
				{EAffectMsg::kImposedForRoom, "$n осветил$u на миг неземным светом."}}},
			{EAffect::kHide, {
				{EAffectMsg::kName, "прячется"},
				{EAffectMsg::kExpired, "Вы стали заметны окружающим."}}},
			{EAffect::kSneak, {
				{EAffectMsg::kName, "подкрадывается"},
				{EAffectMsg::kExpired, "Ваши передвижения стали заметны."}}},
			{EAffect::kCourage, {
				{EAffectMsg::kName, "ярость"},
				{EAffectMsg::kExpired, "Вы успокоились."}}},
			{EAffect::kCharmed, {
				{EAffectMsg::kName, "зачарован"},
				{EAffectMsg::kExpired, "Вы подчиняетесь теперь только себе."},
				{EAffectMsg::kImposedForRoom, "$n0 как лунатик двинул$u следом за $N4."},
				{EAffectMsg::kImposedForVict, "Ваша воля склонилась перед $N4."}}},
			{EAffect::kHold, {
				{EAffectMsg::kName, "оцепенение"},
				{EAffectMsg::kExpired, "К вам вернулась способность двигаться."},
				{EAffectMsg::kImposedForRoom, "$n0 замер$q на месте!"},
				{EAffectMsg::kImposedForVict, "Вы замерли на месте, не в силах пошевельнуться."}}},
			{EAffect::kFly, {
				{EAffectMsg::kName, "летит"},
				{EAffectMsg::kExpired, "Вы приземлились на землю."},
				{EAffectMsg::kImposedForRoom, "$n0 медленно поднял$u в воздух."},
				{EAffectMsg::kImposedForVict, "Вы медленно поднялись в воздух."}}},
			{EAffect::kSilence, {
				{EAffectMsg::kName, "молчание"},
				{EAffectMsg::kExpired, "Теперь вы можете болтать, все что думаете."},
				{EAffectMsg::kImposedForRoom, "$n0 прикусил$g язык!"},
				{EAffectMsg::kImposedForVict, "Вы не в состоянии вымолвить ни слова."}}},
			{EAffect::kAwarness, {  // Ошибка, переименовать в kAwareness
				{EAffectMsg::kName, "настороженность"},
				{EAffectMsg::kExpired, "Вы стали менее внимательны."},
				{EAffectMsg::kImposedForVict, "Вы стали более внимательны к окружающему."},
				{EAffectMsg::kImposedForRoom, "$n начал$g внимательно осматриваться по сторонам."}}},
			{EAffect::kBlink, {
				{EAffectMsg::kName, "мигание"},
				{EAffectMsg::kExpired, "Вы перестали мигать."},
				{EAffectMsg::kImposedForRoom, "$n начал$g мигать."},
				{EAffectMsg::kImposedForVict, "Вы начали мигать."}}},
			{EAffect::kHorse, {
				{EAffectMsg::kName, "верхом или под седлом"},
				{EAffectMsg::kExpired, "Вы успокоились."}}},
			{EAffect::kNoFlee, {
				{EAffectMsg::kName, "не сбежать"},
				{EAffectMsg::kExpired, "Вы опять можете сбежать с поля боя."},
				{EAffectMsg::kImposedForRoom, "$n0 теперь прикован$a к $N2."},
				{EAffectMsg::kImposedForVict, "Вы не сможете покинуть $N3."}}},
			{EAffect::kSingleLight, { //  Что-то непонятное, разобраться
				{EAffectMsg::kName, "освещение"},
				{EAffectMsg::kExpired, "Вы успокоились."}}},
			{EAffect::kHolyLight, {
				{EAffectMsg::kName, "свет"},
				{EAffectMsg::kExpired, "Ваше тело перестало светиться."},
				{EAffectMsg::kImposedForRoom, "$n0 начал$g светиться ярким светом."},
				{EAffectMsg::kImposedForVict, "Вы засветились, освещая комнату."}}},
			{EAffect::kHolyDark, {
				{EAffectMsg::kName, "затемнение"},
				{EAffectMsg::kExpired, "Облако тьмы, окружающее вас, спало."},
				{EAffectMsg::kImposedForVict, "Ваши глаза приобрели красный оттенок."},
				{EAffectMsg::kImposedForRoom, "Глаза $n1 приобрели красный оттенок."}}},
			{EAffect::kDetectPoison, {
				{EAffectMsg::kName, "определение яда"},
				{EAffectMsg::kExpired, "Вы не в состоянии более определять яды."},
				{EAffectMsg::kImposedForVict, "Ваши глаза приобрели карий оттенок."},
				{EAffectMsg::kImposedForRoom, "Глаза $n1 приобрели карий оттенок."}}},
			{EAffect::kDrunked, {
				{EAffectMsg::kName, "под мухой"},
				{EAffectMsg::kExpired, "Кураж прошел. Мама, лучше бы я умер$q вчера."}}},
			{EAffect::kAbstinent, {
				{EAffectMsg::kName, "отходняк"},
				{EAffectMsg::kExpired, "А головка ваша уже не болит."}}},
			{EAffect::kStopRight, {
				{EAffectMsg::kName, "декстраплегия"},
				{EAffectMsg::kExpired, "А головка ваша уже не болит."}}},
			{EAffect::kStopLeft, {
				{EAffectMsg::kName, "синистроплегия"},
				{EAffectMsg::kExpired, "А головка ваша уже не болит."}}},
			{EAffect::kStopFight, {
				{EAffectMsg::kName, "параплегия"},
				{EAffectMsg::kExpired, "А головка ваша уже не болит."}}},
			{EAffect::kHaemorrhage, {
				{EAffectMsg::kName, "кровотечение"},
				{EAffectMsg::kExpired, "Ваши кровоточащие раны затянулись."}}},
			{EAffect::kDisguise, {
				{EAffectMsg::kName, "маскировка"},
				{EAffectMsg::kExpired, "Вы прекратили маскироваться."}}},
			{EAffect::kWaterBreath, { // а заклинание Waterbreath... переименовать?
				{EAffectMsg::kName, "дышать водой"},
				{EAffectMsg::kExpired, "Вы более не способны дышать водой."},
				{EAffectMsg::kImposedForVict, "У вас выросли жабры."},
				{EAffectMsg::kImposedForRoom, "У $n1 выросли жабры."}}},
			{EAffect::kSlow, {
				{EAffectMsg::kName, "медлительность"},
				{EAffectMsg::kExpired, "Медлительность исчезла."},
				{EAffectMsg::kImposedForRoom, "Движения $n1 заметно замедлились."},
				{EAffectMsg::kImposedForVict, "Ваши движения заметно замедлились."}}},
			{EAffect::kHaste, {
				{EAffectMsg::kName, "ускорение"},
				{EAffectMsg::kExpired, "Вы стали более медлительны."},
				{EAffectMsg::kImposedForVict, "Вы начали двигаться быстрее."},
				{EAffectMsg::kImposedForRoom, "$n начал$g двигаться заметно быстрее."}}},
			{EAffect::kGodsShield, {
				{EAffectMsg::kName, "защита богов"},
				{EAffectMsg::kExpired, "Голубой кокон вокруг вашего тела угас."},
				{EAffectMsg::kImposedForVict, "Вас покрыл голубой кокон."},
				{EAffectMsg::kImposedForRoom, "$n покрыл$u сверкающим коконом."}}},
			{EAffect::kAirShield, {
				{EAffectMsg::kName, "воздушный щит"},
				{EAffectMsg::kExpired, "Ваш воздушный щит исчез."},
				{EAffectMsg::kImposedForVict, "Вас окутал воздушный щит."},
				{EAffectMsg::kImposedForRoom, "$n3 окутал воздушный щит."}}},
			{EAffect::kFireShield, {
				{EAffectMsg::kName, "огненный щит"},
				{EAffectMsg::kExpired, "Огненный щит вокруг вашего тела исчез."},
				{EAffectMsg::kImposedForVict, "Вас окутал огненный щит."},
				{EAffectMsg::kImposedForRoom, "$n3 окутал огненный щит."}}},
			{EAffect::kIceShield, {
				{EAffectMsg::kName, "ледяной щит"},
				{EAffectMsg::kExpired, "Ледяной щит вокруг вашего тела исчез."},
				{EAffectMsg::kImposedForVict, "Вас окутал ледяной щит."},
				{EAffectMsg::kImposedForRoom, "$n3 окутал ледяной щит."}}},
			{EAffect::kMagicGlass, {
				{EAffectMsg::kName, "зеркало магии"},
				{EAffectMsg::kExpired, "Вы вновь чувствительны к магическим поражениям."},
				{EAffectMsg::kImposedForVict, "Вас покрыло зеркало магии."},
				{EAffectMsg::kImposedForRoom, "$n3 покрыла зеркальная пелена."}}},
			{EAffect::kStairs, { // Похоже тоже не используется
				{EAffectMsg::kName, "звезды"},
				{EAffectMsg::kExpired, "звезды угасли"}}},
			{EAffect::kStoneHands, {
				{EAffectMsg::kName, "каменная рука"},
				{EAffectMsg::kExpired, "Ваши руки вернулись к прежнему состоянию."},
				{EAffectMsg::kImposedForVict, "Ваши руки задубели."},
				{EAffectMsg::kImposedForRoom, "Руки $n1 задубели."}}},
			{EAffect::kPrismaticAura, {
				{EAffectMsg::kName, "призматическая.аура"},
				{EAffectMsg::kExpired, "Призматическая аура вокруг вашего тела угасла."},
				{EAffectMsg::kImposedForVict, "Вас покрыла призматическая аура."},
				{EAffectMsg::kImposedForRoom, "$n3 покрыла призматическая аура."}}},
			{EAffect::kHelper, {
				{EAffectMsg::kName, "нанят"},
				{EAffectMsg::kExpired, "Вы больше не служите нанимателю."}}},
			{EAffect::kForcesOfEvil, {
				{EAffectMsg::kName, "силы зла"},
				{EAffectMsg::kExpired, "Силы зла оставили вас."},
				{EAffectMsg::kImposedForVict, "Черное облако покрыло вас."},
				{EAffectMsg::kImposedForRoom, "Черное облако покрыло $n3 с головы до пят."}}},
			{EAffect::kAirAura, {
				{EAffectMsg::kName, "воздушная аура"},
				{EAffectMsg::kExpired, "Воздушная аура вокруг вас исчезла."},
				{EAffectMsg::kImposedForVict, "Вас окружила воздушная аура."},
				{EAffectMsg::kImposedForRoom, "$n3 окружила воздушная аура."}}},
			{EAffect::kFireAura, {
				{EAffectMsg::kName, "огненная аура"},
				{EAffectMsg::kExpired, "Огненная аура вокруг вас исчезла."},
				{EAffectMsg::kImposedForVict, "Вас окружила огненная аура."},
				{EAffectMsg::kImposedForRoom, "$n3 окружила огненная аура."}}},
			{EAffect::kIceAura, {
				{EAffectMsg::kName, "ледяная аура"},
				{EAffectMsg::kExpired, "Ледяная аура вокруг вас исчезла."},
				{EAffectMsg::kImposedForVict, "Вас окружила ледяная аура."},
				{EAffectMsg::kImposedForRoom, "$n3 окружила ледяная аура."}}},
			{EAffect::kDeafness, {
				{EAffectMsg::kName, "глухота"},
				{EAffectMsg::kExpired, "Вы вновь можете слышать."},
				{EAffectMsg::kImposedForRoom, "$n0 оглох$q!"},
				{EAffectMsg::kImposedForVict, "Вы оглохли."}}},
			{EAffect::kCrying, {
				{EAffectMsg::kName, "плач"},
				{EAffectMsg::kExpired, "Ваша душа успокоилась."},
				{EAffectMsg::kImposedForRoom, "$n0 издал$g протяжный стон."},
				{EAffectMsg::kImposedForVict, "Вы впали в уныние."}}},
			{EAffect::kPeaceful, {
				{EAffectMsg::kName, "смирение"},
				{EAffectMsg::kExpired, "Смирение в вашей душе вдруг куда-то исчезло."},
				{EAffectMsg::kImposedForRoom, "Взгляд $n1 потускнел, а сам он успокоился."},
				{EAffectMsg::kImposedForVict, "Ваша душа очистилась от зла и странно успокоилась."}}},
			{EAffect::kMagicStopFight, {
				{EAffectMsg::kName, "маг параплегия"},
				{EAffectMsg::kExpired, "Ваши кровоточащие раны затянулись."}}},
			{EAffect::kBerserk, {
				{EAffectMsg::kName, "предсмертная ярость"},
				{EAffectMsg::kExpired, "Неистовство оставило вас."}}},
			{EAffect::kLightWalk, {
				{EAffectMsg::kName, "легкая поступь"},
				{EAffectMsg::kExpired, "Ваши следы вновь стали заметны."}}},
			{EAffect::kBrokenChains, {
				{EAffectMsg::kName, "разбитые оковы"},
				{EAffectMsg::kExpired, "Вы вновь стали уязвимы для оцепенения."},
				{EAffectMsg::kImposedForRoom, "Ярко-синий ореол вспыхнул вокруг $n1 и тут же угас."},
				{EAffectMsg::kImposedForVict, "Волна ярко-синего света омыла вас с головы до ног."}}},
			{EAffect::kCloudOfArrows, {
				{EAffectMsg::kName, "облако стрел"},
				{EAffectMsg::kExpired, "Облако стрел вокруг вас рассеялось."},
				{EAffectMsg::kImposedForVict, "Вас окружило облако летающих огненных стрел."},
				{EAffectMsg::kImposedForRoom, "$n3 окружило облако летающих огненных стрел."}}},
			{EAffect::kShadowCloak, {
				{EAffectMsg::kName, "мантия теней"},
				{EAffectMsg::kExpired, "Ваша теневая мантия замерцала и растаяла."},
				{EAffectMsg::kImposedForVict, "Густые тени окутали вас."},
				{EAffectMsg::kImposedForRoom, "$n скрыл$u в густой тени."}}},
			{EAffect::kGlitterDust, {
				{EAffectMsg::kName, "блестящая пыль"},
				{EAffectMsg::kExpired, "Покрывавшая вас блестящая пыль осыпалась и растаяла в воздухе."},
				{EAffectMsg::kImposedForRoom, "Облако ярко блестящей пыли накрыло $n3."},
				{EAffectMsg::kImposedForVict, "Липкая блестящая пыль покрыла вас с головы до пят."}}},
			{EAffect::kAffright, {
				{EAffectMsg::kName, "испуг"},
				{EAffectMsg::kExpired, "Леденящий душу испуг отпустил вас."},
				{EAffectMsg::kImposedForRoom, "$n0 побледнел$g и задрожал$g от страха."},
				{EAffectMsg::kImposedForVict, "Страх сжал ваше сердце ледяными когтями."}}},
			{EAffect::kScopolaPoison, {
				{EAffectMsg::kName, "яд скополии"},
				{EAffectMsg::kExpired, "В вашей крови не осталось ни капельки яда."},
				{EAffectMsg::kImposedForVict, "Вы почувствовали себя отравленным."},
				{EAffectMsg::kImposedForRoom, "$n позеленел$g от действия яда."}}},
			{EAffect::kDaturaPoison, {
				{EAffectMsg::kName, "яд дурмана"},
				{EAffectMsg::kExpired, "В вашей крови не осталось ни капельки яда."},
				{EAffectMsg::kImposedForVict, "Вы почувствовали себя отравленным."},
				{EAffectMsg::kImposedForRoom, "$n позеленел$g от действия яда."}}},
			{EAffect::kSkillReduce, {
				{EAffectMsg::kName, "понижение умений"},
				{EAffectMsg::kExpired, "В вашей крови не осталось ни капельки яда."},
				{EAffectMsg::kImposedForVict, "Вы почувствовали себя отравленным."},
				{EAffectMsg::kImposedForRoom, "$n позеленел$g от действия яда."}}},
			{EAffect::kNoBattleSwitch, {
				{EAffectMsg::kName, "не переключается"},
				{EAffectMsg::kExpired, "Вы снова можете атаковать кого вздумается."}}},
			{EAffect::kBelenaPoison, {
				{EAffectMsg::kName, "яд белены"},
				{EAffectMsg::kExpired, "Неистовство оставило вас."}}},
			{EAffect::kNoTeleport, {
				{EAffectMsg::kName, "прикован"},
				{EAffectMsg::kExpired, "Сковавшие вас колдовские путы растаяли."}}},
			{EAffect::kCombatLuck, {
				{EAffectMsg::kName, "боевое везение"},
				{EAffectMsg::kImposedForRoom, "$n вдохновенно выпятил$g грудь."},
				{EAffectMsg::kImposedForVict, "Вы почувствовали вдохновение."}}},
			{EAffect::kBandage, {
				{EAffectMsg::kName, "перевязан"},
				{EAffectMsg::kExpired, "Вы аккуратно перевязали свои раны."}}},
			{EAffect::kCannotBeBandaged, {
				{EAffectMsg::kName, "не может перевязываться"},
				{EAffectMsg::kExpired, "Вы снова можете перевязывать свои раны."}}},
			{EAffect::kMorphing, { // Тоже что-то недоделанное похоже
				{EAffectMsg::kName, "превращен"}}},
			{EAffect::kStrangled, {
				{EAffectMsg::kName, "удушье"},
				{EAffectMsg::kExpired, "!SPELL_CombatLuck!"}}},
			{EAffect::kMemorizeSpells, {
				{EAffectMsg::kName, "вспоминает заклинания"},
				{EAffectMsg::kExpired, "!SPELL_CombatLuck!"}}},
			{EAffect::kNoobRegen, {
				{EAffectMsg::kName, "регенерация новичка"},
				{EAffectMsg::kExpired, "!SPELL_CombatLuck!"}}},
			{EAffect::kVampirism, {
				{EAffectMsg::kName, "вампиризм"},
				{EAffectMsg::kImposedForRoom, "Зрачки $n3 приобрели красный оттенок."},
				{EAffectMsg::kImposedForVict, "Ваши зрачки приобрели красный оттенок."}}},
			{EAffect::kLacerations, {
				{EAffectMsg::kName, "рваные раны"},
				{EAffectMsg::kExpired, "Ваши раны слегка затянулись."}}},
			{EAffect::kLacerations, {
				{EAffectMsg::kName, "...да защитит он себя."},
				{EAffectMsg::kExpired, "!SPELL_CombatLuck!"}}},
			{EAffect::kCommander, {
				{EAffectMsg::kName, "полководец"},
				{EAffectMsg::kExpired, "Вы сняли флажок со своего шлема."}}},
			{EAffect::kEarthAura, {
				{EAffectMsg::kName, "аура земли"},
				{EAffectMsg::kExpired, "Аура земли вокруг вас развеялась."},
				{EAffectMsg::kImposedForVict, "Вас окружила аура земли."},
				{EAffectMsg::kImposedForRoom, "$n3 окружила аура земли."}}}
		};

	static const std::string empty_str;
	try {
		return affects_msg.at(affect_id).at(msg_id);
	} catch (std::out_of_range &) {
		return empty_str;
	}
}


// ===============================================================================================
/*
		 {EAffect::kChillTouch, {
			 {EAffectMsg::kName, "... которые черны от льда."},
			 {EAffectMsg::kExpired, "Вы отметили, что силы вернулись к вам."},
			 {EAffectMsg::kImposedForVict, "Вы почувствовали себя слабее!"},
			 {EAffectMsg::kImposedForRoom, "Боевой пыл $n1 несколько остыл."}}},
		 {EAffect::kDispelEvil, {
			 {EAffectMsg::kName, "... грешников преследует зло, а праведникам воздается добром."},
			 {EAffectMsg::kExpired, "!Dispel Evil!"}}},
		 {EAffect::kEnergyDrain, {
			 {EAffectMsg::kName, "... да иссякнут соки, питающие тело."},
			 {EAffectMsg::kExpired, "!Energy Drain!"},
			 {EAffectMsg::kImposedForVict, "Вы почувствовали себя слабее!"},
			 {EAffectMsg::kImposedForRoom, "$n стал$g немного слабее."}}},
		 {EAffect::kStrength, {
			 {EAffectMsg::kName, "... и человек разумный укрепляет силу свою."},
			 {EAffectMsg::kExpired, "Вы чувствуете себя немного слабее."},
			 {EAffectMsg::kImposedForVict, "Вы почувствовали себя сильнее."},
			 {EAffectMsg::kImposedForRoom, "Мышцы $n1 налились силой."}}},
		 {EAffect::kPatronage, {
			 {EAffectMsg::kName, "... ибо спасет людей Своих от грехов их."},
			 {EAffectMsg::kExpired, "Вы утратили покровительство высших сил."},
			 {EAffectMsg::kImposedForVict, "Исходящий с небес свет на мгновение озарил вас."},
			 {EAffectMsg::kImposedForRoom, "Исходящий с небес свет на мгновение озарил $n3."}}},
		 {EAffect::kGroupArmor, {
			 {EAffectMsg::kName, "... ибо кто Бог, кроме Господа, и кто защита, кроме Бога нашего?"},
			 {ESpellMsg::kAreaForChar, "Вы создали защитную сферу, которая окутала вас и пространство рядом с вами.\r\n"},
			 {EAffectMsg::kExpired, "Вы почувствовали себя менее защищенно."},
			 {EAffectMsg::kImposedForVict, "Вы почувствовали вокруг себя невидимую защиту."},
			 {EAffectMsg::kImposedForRoom, "Вокруг $n1 вспыхнул белый щит и тут же погас."}}},
		 {EAffect::kGroupStrength, {
			 {EAffectMsg::kName, "... и даст нам Господь силу."},
			 {ESpellMsg::kAreaForChar, "Вы наделили союзников силой медведя.\r\n"},
			 {EAffectMsg::kExpired, "Вы чувствуете себя немного слабее."},
			 {EAffectMsg::kImposedForVict, "Вы почувствовали себя сильнее."},
			 {EAffectMsg::kImposedForRoom, "Мышцы $n1 налились силой."}}},
		 {EAffect::kStoneSkin, {
			 {EAffectMsg::kName, "... твердость ли камней твердость твоя?"},
			 {EAffectMsg::kExpired, "Ваша кожа вновь стала мягкой и бархатистой."},
			 {EAffectMsg::kImposedForVict, "Вы стали менее чувствительны к ударам."},
			 {EAffectMsg::kImposedForRoom, "Кожа $n1 покрылась каменными пластинами."}}},
		 {EAffect::kCloudly, {
			 {EAffectMsg::kName, "... будут как утренний туман."},
			 {EAffectMsg::kExpired, "Ваши очертания приобрели отчетливость."},
			 {EAffectMsg::kImposedForVict, "Ваше тело стало прозрачным, как туман."},
			 {EAffectMsg::kImposedForRoom, "Очертания $n1 расплылись и стали менее отчетливыми."}}},
		 {EAffect::kWeaknes, {
			 {EAffectMsg::kName, "... и силу могучих ослабляет."},
			 {EAffectMsg::kExpired, "Силы вернулись к вам."},
			 {EAffectMsg::kImposedForVict, "Вы почувствовали себя слабее!"},
			 {EAffectMsg::kImposedForRoom, "$n стал$g немного слабее."}}},
		 {EAffect::kEnlarge, {
			 {EAffectMsg::kName, "... и плоть выросла."},
			 {EAffectMsg::kExpired, "Ваши размеры стали прежними."},
			 {EAffectMsg::kImposedForVict, "Вы стали крупнее."},
			 {EAffectMsg::kImposedForRoom, "$n начал$g расти, как на дрожжах."}}},
		 {EAffect::kSacrifice, {
			 {EAffectMsg::kName, "... плоть твоя и тело твое будут истощены."},
			 {EAffectMsg::kExpired, "!SPELL SACRIFICE!"}}},
		 {EAffect::kWeb, {
			 {EAffectMsg::kName, "... терны и сети на пути коварного."},
			 {EAffectMsg::kExpired, "Магическая сеть, покрывавшая вас, исчезла."},
			 {EAffectMsg::kImposedForRoom, "$n3 покрыла невидимая паутина, сковывая $s движения!"},
			 {EAffectMsg::kImposedForVict, "Вас покрыла невидимая паутина!"}}},
		 {EAffect::kCamouflage, {
			 {EAffectMsg::kExpired, "Вы стали вновь похожи сами на себя."}}},
		 {EAffect::kPowerBlindness, {
			 {EAffectMsg::kName, "... поразит тебя Господь слепотою навечно."},
			 {EAffectMsg::kExpired, "!SPELL POWER BLINDNESS!"}}},
		 {EAffect::kPowerSilence, {
			 {EAffectMsg::kName, "... исходящее из уст твоих, да не осквернит слуха."},
			 {EAffectMsg::kExpired, "Теперь вы можете болтать, все что думаете."},
			 {EAffectMsg::kImposedForRoom, "$n0 прикусил$g язык!"},
			 {EAffectMsg::kImposedForVict, "Вы не в состоянии вымолвить ни слова."}}},
		 {EAffect::kExtraHits, {
			 {EAffectMsg::kName, "... крепкое тело лучше несметного богатства."},
			 {EAffectMsg::kExpired, "!SPELL EXTRA HITS!"}}},
		 {EAffect::kResurrection, {

			 {EAffectMsg::kName, "... оживут мертвецы Твои, восстанут мертвые тела!"},
			 {EAffectMsg::kExpired, "!SPELL RESSURECTION!"}}},
		 {EAffect::kMagicShield, {

			 {EAffectMsg::kName, "... руками своими да защитит он себя"},
			 {EAffectMsg::kExpired, "Ваш волшебный щит рассеялся."},
			 {EAffectMsg::kImposedForRoom, "Сверкающий щит вспыхнул вокруг $n1 и угас."},
			 {EAffectMsg::kImposedForVict, "Сверкающий щит вспыхнул вокруг вас и угас."}}},
		 {EAffect::kForbidden, {
			 {EAffectMsg::kName, "... ибо положена печать, и никто не возвращается."},
			 {EAffectMsg::kExpired, "Магия, запечатывающая входы, пропала."}}},
		 {EAffect::kStunning, {
			 {EAffectMsg::kName, "... и проклял его именем Господним."},
			 {EAffectMsg::kExpired, "Каменное проклятие отпустило вас."}}},
		 {EAffect::kFullFeed, {
			 {EAffectMsg::kName, "... душа больше пищи, и тело - одежды."},
			 {EAffectMsg::kExpired, "Вам снова захотелось жареного, да с дымком."}}},
		 {EAffect::kColdWind, {
			 {EAffectMsg::kName, "... подует северный холодный ветер."},
			 {EAffectMsg::kExpired, "Вы согрелись и подвижность вернулась к вам."},
			 {EAffectMsg::kImposedForVict, "Вы покрылись серебристым инеем."},
			 {EAffectMsg::kImposedForRoom, "$n покрыл$u красивым серебристым инеем."}}},
		 {EAffect::kBattle, {
			 {EAffectMsg::kExpired, "К вам вернулась способность нормально сражаться."}}},
		 {EAffect::kFever, {
			 {EAffectMsg::kName, "... и сделаются жестокие и кровавые язвы."},
			 {EAffectMsg::kExpired, "Лихорадка прекратилась."},
			 {EAffectMsg::kImposedForVict, "Вас скрутило в жестокой лихорадке."},
			 {EAffectMsg::kImposedForRoom, "$n3 скрутило в жестокой лихорадке."}}},
		 {EAffect::kReligion, {
			 {EAffectMsg::kExpired, "Вы утратили расположение Богов."}}},
		 {EAffect::kFastRegeneration, {
			 {EAffectMsg::kName, "... нет богатства лучше телесного здоровья."},
			 {EAffectMsg::kExpired, "Живительная сила покинула вас."},
			 {EAffectMsg::kImposedForVict, "Вас наполнила живительная сила."},
			 {EAffectMsg::kImposedForRoom, "$n расцвел$g на ваших глазах."}}},
		 {EAffect::kIceStorm, {
			 {EAffectMsg::kName, "... и град, величиною в талант, падет с неба."},
			 {ESpellMsg::kAreaForChar, "Вы воздели руки к небу, и тысячи мелких льдинок хлынули вниз!"},
			 {ESpellMsg::kAreaForRoom, "$n воздел$g руки к небу, и тысячи мелких льдинок хлынули вниз!"},
			 {EAffectMsg::kExpired, "Ваши мышцы оттаяли и вы снова можете двигаться."},
			 {EAffectMsg::kImposedForRoom, "$n3 оглушило."},
			 {EAffectMsg::kImposedForVict, "Вас оглушило."}}},
		 {EAffect::kLessening, {
			 {EAffectMsg::kName, "... плоть на нем пропадает."},
			 {EAffectMsg::kExpired, "Ваши размеры вновь стали прежними."},
			 {EAffectMsg::kImposedForVict, "Вы стали мельче."},
			 {EAffectMsg::kImposedForRoom, "$n скукожил$u."}}},
		 {EAffect::kMadness, {
			 {EAffectMsg::kName, "... и ярость его загорелась в нем."},
			 {EAffectMsg::kExpired, "Безумие отпустило вас."}}},
		 {EAffect::kGroupMagicGlass, {
			 {EAffectMsg::kName, "... воздай им по делам их, по злым поступкам их."},
			 {ESpellMsg::kAreaForChar, "Вы произнесли несколько резких слов, и все вокруг засеребрилось.\r\n"},
			 {EAffectMsg::kExpired, "Зеркало магии истаяло."},
			 {EAffectMsg::kImposedForVict, "Вас покрыло зеркало магии."},
			 {EAffectMsg::kImposedForRoom, "$n3 покрыла зеркальная пелена."}}},
		 {EAffect::kVacuum, {
			 {EAffectMsg::kName, "... и услышав слова сии - пал бездыханен."},
			 {EAffectMsg::kExpired, "Пустота вокруг вас исчезла."}}},
		 {EAffect::kMeteorStorm, {
			 {EAffectMsg::kName, "... и камни, величиною в талант, падут с неба."},
			 {EAffectMsg::kExpired, "Последний громовой камень грянул в землю и все стихло."}}},
		 {EAffect::kMindless, {
			 {EAffectMsg::kName, "... и безумие его с ним."},
			 {EAffectMsg::kExpired, "Ваш разум просветлел."},
			 {EAffectMsg::kImposedForVict, "Ваш разум помутился!"},
			 {EAffectMsg::kImposedForRoom, "$n0 стал$g слаб$g на голову!"}}},
		 {EAffect::kShock, {
			 {EAffectMsg::kName, "... кто делает или глухим, или слепым."},
			 {ESpellMsg::kAreaForChar, "Яркая вспышка слетела с кончиков ваших пальцев и с оглушительным грохотом взорвалась в воздухе."},
			 {ESpellMsg::kAreaForRoom, "Выпущенная $n1 яркая вспышка с оглушительным грохотом взорвалась в воздухе."},
			 {EAffectMsg::kExpired, "Ваше сознание прояснилось."},
			 {EAffectMsg::kImposedForRoom, "$n1 в шоке!"},
			 {EAffectMsg::kImposedForVict, "Вас повергло в шок."}}},
		 {EAffect::kEarthfall, {
			 {EAffectMsg::kName, "... и обрушатся камни с небес."},
			 {ESpellMsg::kAreaForChar, "Вы высоко подбросили комок земли и он, увеличиваясь на глазах, обрушился вниз."},
			 {ESpellMsg::kAreaForRoom, "$n высоко подбросил$g комок земли, который, увеличиваясь на глазах, стал падать вниз."},
			 {EAffectMsg::kExpired, "Громкий звон в голове несколько поутих."},
			 {EAffectMsg::kImposedForRoom, "$n3 оглушило."},
			 {EAffectMsg::kImposedForVict, "Вас оглушило."}}},
		 {EAffect::kHolystrike, {
			 {EAffectMsg::kName, "... и предоставь мертвым погребать своих мертвецов."},
			 {EAffectMsg::kExpired, "!SPELL_HOLYSTRIKE!"},
			 {EAffectMsg::kImposedForRoom, "$n0 замер$q на месте!"},
			 {EAffectMsg::kImposedForVict, "Вы замерли на месте, не в силах пошевельнуться."}}},
		 {EAffect::kFascination, {
			 {EAffectMsg::kName, "... и омолодил он, и украсил их."},
			 {EAffectMsg::kExpired, "Ваша красота куда-то пропала."},
			 {EAffectMsg::kImposedForVict, "Вы попытались вспомнить уроки старой цыганки, что учила вас людям головы морочить."},
			 {EAffectMsg::kImposedForRoom, "$n0 достал$g из маленькой сумочки какие-то вонючие порошки и отвернул$u, бормоча под нос \r\n\"..так это на ресницы надо, кажется... Эх, только бы не перепутать...\" \r\n"}}},
		 {EAffect::kOblivion, {
			 {EAffectMsg::kName, "... опадет на тебя чернь страшная."},
			 {EAffectMsg::kExpired, "Тёмная вода забвения схлынула из вашего раузма."},
			 {EAffectMsg::kImposedForRoom, "Облако забвения окружило $n3."},
			 {EAffectMsg::kImposedForVict, "Ваш разум помутился."}}},
		 {EAffect::kBurdenOfTime, {
			 {EAffectMsg::kName, "... и время не властно над ними."},
			 {ESpellMsg::kAreaForChar, "Вы скрестили руки на груди, вызвав яркую вспышку синего света."},
			 {ESpellMsg::kAreaForRoom, "$n0 скрестил$g руки на груди, вызвав яркую вспышку синего света."},
			 {EAffectMsg::kExpired, "Иновременное небытие больше не властно над вами."},
			 {EAffectMsg::kImposedForRoom, "Облако забвения окружило $n3."},
			 {EAffectMsg::kImposedForVict, "Ваш разум помутился."}}},
		 {EAffect::kMagicBattle, {
			 {EAffectMsg::kExpired, "К вам вернулась способность нормально сражаться."}}},
		 {EAffect::kStoneBones, {
			 {EAffectMsg::kName, "...и тот, кто упадет на камень сей, разобьется."},
			 {EAffectMsg::kExpired, "Читить вредно!"},
			 {EAffectMsg::kImposedForVict, "Читим-с?! Нехорошо! Инцидент запротоколирован."},
			 {EAffectMsg::kImposedForRoom, "Кости $n1 обрели твердость кремня."}}},
		 {EAffect::kRoomLight, {
			 {EAffectMsg::kName, "...ибо сказал МОНТЕР !!!"},
			 {EAffectMsg::kExpired, "Колдовской свет угас."}}},
		 {EAffect::kDeadlyFog, {
			 {EAffectMsg::kName, "...и смерть явись в тумане его."},
			 {EAffectMsg::kExpired, "Порыв ветра развеял туман смерти."}}},
		 {EAffect::kThunderstorm, {
			 {EAffectMsg::kName, "...творит молнии при дожде, изводит ветер из хранилищ Своих."},
			 {EAffectMsg::kExpired, "Ветер прогнал грозовые тучи."}}},
		 {EAffect::kFailure, {
			 {EAffectMsg::kName, ".. и несчастен, и жалок, и нищ."},
			 {EAffectMsg::kExpired, "Удача вновь вернулась к вам."},
			 {EAffectMsg::kImposedForRoom, "Тяжелое бурое облако сгустилось над $n4."},
			 {EAffectMsg::kImposedForVict, "Тяжелые тучи сгустились над вами, и вы почувствовали, что удача покинула вас."}}},
		 {EAffect::kClanPray, {
			 {EAffectMsg::kExpired, "Магические чары ослабели со временем и покинули вас."}}},
		 {EAffect::kCatGrace, {
			 {EAffectMsg::kName, "...и не уязвит враг того, кто скор."},
			 {EAffectMsg::kExpired, "Ваши движения утратили прежнюю колдовскую ловкость."},
			 {EAffectMsg::kImposedForVict, "Ваши движения обрели невиданную ловкость."},
			 {EAffectMsg::kImposedForRoom, "Движения $n1 обрели невиданную ловкость."}}},
		 {EAffect::kBullBody, {
			 {EAffectMsg::kName, "...и мощь звериная жила в теле его."},
			 {EAffectMsg::kExpired, "Ваше телосложение вновь стало обычным."},
			 {EAffectMsg::kImposedForVict, "Ваше тело налилось звериной мощью."},
			 {EAffectMsg::kImposedForRoom, "Плечи $n1 раздались вширь, а тело налилось звериной мощью."}}},
		 {EAffect::kSnakeWisdom, {
			 {EAffectMsg::kName, "...и даровал мудрость ему."},
			 {EAffectMsg::kExpired, "Вы утратили навеянную магией мудрость."},
			 {EAffectMsg::kImposedForVict, "Шелест змеиной чешуи коснулся вашего сознания, и вы стали мудрее."},
			 {EAffectMsg::kImposedForRoom, "$n спокойно и мудро посмотрел$g вокруг."}}},
		 {EAffect::kGimmicry, {
			 {EAffectMsg::kName, "...ибо кто познал ум Господень?"},
			 {EAffectMsg::kExpired, "Навеянная магией хитрость покинула вас."},
			 {EAffectMsg::kImposedForVict, "Вы почувствовали, что для вашего ума более нет преград."},
			 {EAffectMsg::kImposedForRoom, "$n хитро прищурил$u и поглядел$g по сторонам."}}},
		 {EAffect::kWarcryOfChallenge, {
			 {EAffectMsg::kName, "Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!"},
			 {ESpellMsg::kAreaForRoom, "Вы не стерпели насмешки, и бросились на $n1!"},
			 {EAffectMsg::kExpired, "!SPELL_WC_OF_CHALLENGE!"}}},
		 {EAffect::kWarcryOfMenace, {
			 {EAffectMsg::kName, "Покрошу-изувечу, душу выну и в блины закатаю!"},
			 {EAffectMsg::kExpired, "Похоже, к вам вернулась удача."},
			 {EAffectMsg::kImposedForVict, "Похоже, сегодня не ваш день."},
			 {EAffectMsg::kImposedForRoom, "Удача покинула $n3."}}},
		 {EAffect::kWarcryOfRage, {
			 {EAffectMsg::kName, "Не отступим, други, они наше сало сперли!"},
			 {EAffectMsg::kExpired, "!SPELL_WC_OF_RAGE!"},
			 {EAffectMsg::kImposedForRoom, "$n0 оглох$q!"},
			 {EAffectMsg::kImposedForVict, "Вы оглохли."}}},
		 {EAffect::kWarcryOfMadness, {
			 {ESpellMsg::kCastPoly, "Всех убью, а сам$g останусь!"},
			 {EAffectMsg::kName, "Всех убью, а сам$g останусь!"},
			 {EAffectMsg::kExpired, "Рассудок вернулся к вам."},
			 {EAffectMsg::kImposedForRoom, "$n0 потерял$g рассудок."},
			 {EAffectMsg::kImposedForVict, "Вас обуяло безумие!"}}},
		 {EAffect::kWarcryOfThunder, {
			 {EAffectMsg::kName, "Шоб вас приподняло да шлепнуло!!!"},
			 {EAffectMsg::kExpired, "!SPELL_WC_OF_THUNDER!"},
			 {EAffectMsg::kImposedForRoom, "$n0 оглох$q!"},
			 {EAffectMsg::kImposedForVict, "Вы оглохли."}}},
		 {EAffect::kWarcryOfDefence, {
			 {EAffectMsg::kName, "В строй други, защитим животами Русь нашу!"},
			 {EAffectMsg::kExpired, "Действие клича 'призыв к обороне' закончилось."}}},
		 {EAffect::kWarcryOfBattle, {
			 {EAffectMsg::kName, "Дер-ржать строй, волчьи хвосты!"},
			 {EAffectMsg::kExpired, "Действие клича битвы закончилось."}}},
		 {EAffect::kWarcryOfPower, {
			 {EAffectMsg::kName, "Сарынь на кичку!"},
			 {EAffectMsg::kExpired, "Действие клича мощи закончилось."}}},
		 {EAffect::kWarcryOfBless, {
			 {EAffectMsg::kName, "Стоять крепко! За нами Киев, Подол и трактир с пивом!!!"},
			 {EAffectMsg::kExpired, "Действие клича доблести закончилось."}}},
		 {EAffect::kWarcryOfCourage, {
			 {EAffectMsg::kName, "Орлы! Будем биться как львы!"},
			 {EAffectMsg::kExpired, "Действие клича отваги закончилось."}}},
		 {EAffect::kRuneLabel, {
			 {EAffectMsg::kName, "...и Сам отошел от них на вержение камня."},
			 {EAffectMsg::kExpired, "Магические письмена на земле угасли."}}},
		 {EAffect::kAconitumPoison, {
			 {EAffectMsg::kName, "... и пошлю на них зубы зверей и яд ползающих по земле."},
			 {EAffectMsg::kExpired, "В вашей крови не осталось ни капельки яда."},
			 {EAffectMsg::kImposedForVict, "Вы почувствовали себя отравленным."},
			 {EAffectMsg::kImposedForRoom, "$n позеленел$g от действия яда."}}},
		 {EAffect::kCapable, {
			 {EAffectMsg::kExpired, "!SPELL_CAPABLE!"}}},
		 {EAffect::kStrangle, {
			 {EAffectMsg::kExpired, "Удушье отпустило вас, и вы вздохнули полной грудью."}}},
		 {EAffect::kRecallSpells, {
			 {EAffectMsg::kExpired, "Вам стало не на чем концентрироваться."}}},
		 {EAffect::kHypnoticPattern, {
			 {EAffectMsg::kName, "...и утроба его приготовляет обман."},
			 {EAffectMsg::kExpired, "Плывший в воздухе огненный узор потускнел и растаял струйками дыма."}}},
		 {EAffect::kSolobonus, {
			 {EAffectMsg::kExpired, "Одна из наград прекратила действовать."}}},
		 {EAffect::kDeathAura, {
			 {EAffectMsg::kName, "...налякай ворогов наших и покарай их немощью."},
			 {EAffectMsg::kExpired, "Силы нави покинули вас."}}},
		 {EAffect::kRecovery, {
			 {EAffectMsg::kName, "... прости Господи грехи, верни плоть созданию."},
			 {EAffectMsg::kExpired, "!SPELL_RECOVERY!"}}},
		 {EAffect::kMassRecovery, {
			 {EAffectMsg::kName, "... прости Господи грехи, верни плоть созданиям."},
			 {EAffectMsg::kExpired, "!SPELL_MASS_RECOVERY!"}}},
		 {EAffect::kAuraOfEvil, {
			 {EAffectMsg::kName, "Надели силой злою во благо."},
			 {EAffectMsg::kExpired, "Аура зла больше не помогает вам."}}},
		 {EAffect::kBlackTentacles, {
			 {EAffectMsg::kName, "И он не знает, что мертвецы там и что в глубине..."},
			 {EAffectMsg::kExpired, "Жуткие черные руки побледнели и расплылись зловонной дымкой."}}},
		 {EAffect::kIndriksTeeth, {
			 {EAffectMsg::kName, "Есть род, у которого зубы - мечи и челюсти - ножи..."},
			 {EAffectMsg::kExpired, "Каменные зубы исчезли, возвратив способность двигаться."},
			 {EAffectMsg::kImposedForRoom, "$n0 теперь прикован$a к $N2."},
			 {EAffectMsg::kImposedForVict, "Вы не сможете покинуть $N3."}}},
		 {EAffect::kExpedient, {
			 {EAffectMsg::kName, "!Применил боевой прием!"},
			 {EAffectMsg::kExpired, "Эффект боевого приема завершился."}}},
		 {EAffect::kSightOfDarkness, {
			 {EAffectMsg::kName, "Станьте зрячи в тьме кромешной!"},
			 {EAffectMsg::kExpired, "Вы больше не можете видеть ночью."},
			 {EAffectMsg::kImposedForRoom, "$n0 погрузил$g комнату во мрак."},
			 {EAffectMsg::kImposedForVict, "Вы погрузили комнату в непроглядную тьму."}}},
		 {EAffect::kGroupSincerity, {
			 {EAffectMsg::kName, "И узрим братья намерения окружающих."},
			 {EAffectMsg::kExpired, "Вы не в состоянии больше видеть невидимых."},
			 {EAffectMsg::kImposedForVict, "Ваши глаза приобрели зеленый оттенок."},
			 {EAffectMsg::kImposedForRoom, "Глаза $n1 приобрели зеленый оттенок."}}},
		 {EAffect::kMagicalGaze, {
			 {EAffectMsg::kName, "Покажи, Спаситель, магические силы братии."},
			 {EAffectMsg::kExpired, "Вы не в состоянии более определять магию."},
			 {EAffectMsg::kImposedForVict, "Ваши глаза приобрели желтый оттенок."},
			 {EAffectMsg::kImposedForRoom, "Глаза $n1 приобрели желтый оттенок."}}},
		 {EAffect::kAllSeeingEye, {
			 {EAffectMsg::kName, "Не спрячется, не скроется, ни заяц, ни блоха."},
			 {EAffectMsg::kExpired, "Вы более не можете определять наклонности."},
			 {EAffectMsg::kImposedForVict, "Ваши глаза приобрели золотистый оттенок."},
			 {EAffectMsg::kImposedForRoom, "Глаза $n1 приобрели золотистый оттенок."}}},
		 {EAffect::kEyeOfGods, {
			 {EAffectMsg::kName, "Да не скроется от взора вашего, ни одна живая душа."},
			 {EAffectMsg::kExpired, "Вы больше не можете чувствовать жизнь."},
			 {EAffectMsg::kImposedForVict, "Вы способны разглядеть даже микроба."},
			 {EAffectMsg::kImposedForRoom, "$n0 начал$g замечать любые движения."}}},
		 {EAffect::kBreathingAtDepth, {
			 {EAffectMsg::kName, "Что в воде, что на земле, воздух свежим будет."},
			 {EAffectMsg::kExpired, "Вы более не способны дышать водой."},
			 {EAffectMsg::kImposedForVict, "У вас выросли жабры."},
			 {EAffectMsg::kImposedForRoom, "У $n1 выросли жабры."}}},
		 {EAffect::kSnakeEyes, {
			 {EAffectMsg::kName, "...и самый сильный яд станет вам виден."},
			 {EAffectMsg::kExpired, "Вы не в состоянии более определять яды."},
			 {EAffectMsg::kImposedForVict, "Ваши глаза приобрели карий оттенок."},
			 {EAffectMsg::kImposedForRoom, "Глаза $n1 приобрели карий оттенок."}}},
		 {EAffect::kPaladineInspiration, {
			 {ESpellMsg::kAreaForChar, "Ваш точный удар воодушевил и придал новых сил!"},
			 {ESpellMsg::kAreaForRoom, "Точный удар $n1 воодушевил и придал новых сил!"},
			 {EAffectMsg::kExpired, "*Боевое воодушевление угасло, а с ним и вся жажда подвигов!"}}},
		 {EAffect::kDexterity, {
			 {EAffectMsg::kName, "... и человек разумный укрепляет ловкость свою."},
			 {EAffectMsg::kExpired, "Вы стали менее шустрым."},
			 {EAffectMsg::kImposedForVict, "Вы почувствовали себя более шустрым."},
			 {EAffectMsg::kImposedForRoom, "$n0 будет двигаться более шустро."}}},
		 {EAffect::kWarcryOfExperience, {
			 {EAffectMsg::kName, "найдем новизну в рутине сражений!"},
			 {ESpellMsg::kAreaForChar, "Вы приготовились к обретению нового опыта."},
			 {EAffectMsg::kExpired, "Действие клича 'обучение' закончилось."}}},
		 {EAffect::kWarcryOfLuck, {
			 {EAffectMsg::kName, "и пусть удача будет нашей спутницей!"},
			 {ESpellMsg::kAreaForChar, "Вы ощутили, что вам улыбнулась удача."},
			 {EAffectMsg::kExpired, "Действие клича 'везение' закончилось."}}},
		 {EAffect::kWarcryOfPhysdamage, {
			 {EAffectMsg::kName, "бей в глаз, не порти шкуру."},
			 {ESpellMsg::kAreaForChar, "Боевой клич придал вам сил!"},
			 {EAffectMsg::kExpired, "Действие клича 'точность' закончилось."}}},
		 {EAffect::kMassFailure, {
			 {EAffectMsg::kName, "...надежда тщетна: не упадешь ли от одного взгляда его?"},
			 {ESpellMsg::kAreaForChar, "Вняв вашему призыву, Змей Велес коснулся недобрым взглядом ваших супостатов.\r\n"},
			 {ESpellMsg::kAreaForVict, "$n провыл$g несколько странно звучащих слов и от тяжелого взгляда из-за края мира у вас подкосились ноги."},
			 {EAffectMsg::kExpired, "Удача снова повернулась к вам лицом... и залепила пощечину."},
			 {EAffectMsg::kImposedForRoom, "Тяжелое бурое облако сгустилось над $n4."},
			 {EAffectMsg::kImposedForVict, "Тяжелые тучи сгустились над вами, и вы почувствовали, что удача покинула вас."}}},
		 {EAffect::kSnare, {
			 {EAffectMsg::kName, "...будет трапеза их сетью им, и мирное пиршество их - западнею."},
			 {ESpellMsg::kAreaForChar, "Вы соткали магические тенета, опутавшие ваших врагов.\r\n"},
			 {ESpellMsg::kAreaForVict, "$n что-то прошептал$g, странно скрючив пальцы, и взлетевшие откуда ни возьмись ловчие сети опутали вас"},
			 {EAffectMsg::kExpired, "Покрывавшие вас сети колдовской западни растаяли."},
			 {EAffectMsg::kImposedForRoom, "$n0 теперь прикован$a к $N2."},
			 {EAffectMsg::kImposedForVict, "Вы не сможете покинуть $N3."}}},
		 {EAffect::kQUest, {
			 {EAffectMsg::kExpired, "Наложенные на вас чары рассеялись."}}},
		 {EAffect::kExpedientFail, {
			 {EAffectMsg::kName, "!Провалил боевой прием!"},
			 {EAffectMsg::kExpired, "Вы восстановили равновесие."}}},
		 {EAffect::kPortalTimer, {
			 {EAffectMsg::kExpired, "Пентаграмма медленно растаяла."}}
		 }
		}; */

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
