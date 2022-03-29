/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы системы аффектов.
*/

#include "affect_contants.h"

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
							   "\n",	// не используется, место свободно
							   "полководец",
							   "земной поклон",
							   "\n",
							   "\n",
};

typedef std::map<EAffect, std::string> EAffectFlag_name_by_value_t;
typedef std::map<const std::string, EAffect> EAffectFlag_value_by_name_t;
EAffectFlag_name_by_value_t EAffectFlag_name_by_value;
EAffectFlag_value_by_name_t EAffectFlag_value_by_name;
void init_EAffectFlag_ITEM_NAMES() {
	EAffectFlag_value_by_name.clear();
	EAffectFlag_name_by_value.clear();

	EAffectFlag_name_by_value[EAffect::kBlind] = "kBlind";
	EAffectFlag_name_by_value[EAffect::kInvisible] = "kInvisible";
	EAffectFlag_name_by_value[EAffect::kDetectAlign] = "kDetectAlign";
	EAffectFlag_name_by_value[EAffect::kDetectInvisible] = "kDetectInvisible";
	EAffectFlag_name_by_value[EAffect::kDetectMagic] = "kDetectMagic";
	EAffectFlag_name_by_value[EAffect::kDetectLife] = "kDetectLife";
	EAffectFlag_name_by_value[EAffect::kWaterWalk] = "kWaterWalk";
	EAffectFlag_name_by_value[EAffect::kSanctuary] = "kSanctuary";
	EAffectFlag_name_by_value[EAffect::kGroup] = "kGroup";
	EAffectFlag_name_by_value[EAffect::kCurse] = "kCurse";
	EAffectFlag_name_by_value[EAffect::kInfravision] = "kInfravision";
	EAffectFlag_name_by_value[EAffect::kPoisoned] = "kPoisoned";
	EAffectFlag_name_by_value[EAffect::kProtectedFromEvil] = "kProtectedFromEvil";
	EAffectFlag_name_by_value[EAffect::kProtectedFromGood] = "kProtectedFromGood";
	EAffectFlag_name_by_value[EAffect::kSleep] = "kSleep";
	EAffectFlag_name_by_value[EAffect::kNoTrack] = "kNoTrack";
	EAffectFlag_name_by_value[EAffect::kTethered] = "kTethered";
	EAffectFlag_name_by_value[EAffect::kBless] = "kBless";
	EAffectFlag_name_by_value[EAffect::kSneak] = "kSneak";
	EAffectFlag_name_by_value[EAffect::kHide] = "kHide";
	EAffectFlag_name_by_value[EAffect::kCourage] = "kCourage";
	EAffectFlag_name_by_value[EAffect::kCharmed] = "kCharmed";
	EAffectFlag_name_by_value[EAffect::kHold] = "kHold";
	EAffectFlag_name_by_value[EAffect::kFly] = "kFly";
	EAffectFlag_name_by_value[EAffect::kSilence] = "kSilence";
	EAffectFlag_name_by_value[EAffect::kAwarness] = "kAwarness";
	EAffectFlag_name_by_value[EAffect::kBlink] = "kBlink";
	EAffectFlag_name_by_value[EAffect::kHorse] = "kHorse";
	EAffectFlag_name_by_value[EAffect::kNoFlee] = "kNoFlee";
	EAffectFlag_name_by_value[EAffect::kSingleLight] = "kSingleLight";
	EAffectFlag_name_by_value[EAffect::kHolyLight] = "kHolyLight";
	EAffectFlag_name_by_value[EAffect::kHolyDark] = "kHolyDark";
	EAffectFlag_name_by_value[EAffect::kDetectPoison] = "kDetectPoison";
	EAffectFlag_name_by_value[EAffect::kDrunked] = "kDrunked";
	EAffectFlag_name_by_value[EAffect::kAbstinent] = "kAbstinent";
	EAffectFlag_name_by_value[EAffect::kStopRight] = "kStopRight";
	EAffectFlag_name_by_value[EAffect::kStopLeft] = "kStopLeft";
	EAffectFlag_name_by_value[EAffect::kStopFight] = "kStopFight";
	EAffectFlag_name_by_value[EAffect::kHaemorrhage] = "kHaemorrhage";
	EAffectFlag_name_by_value[EAffect::kDisguise] = "kDisguise";
	EAffectFlag_name_by_value[EAffect::kWaterBreath] = "kWaterBreath";
	EAffectFlag_name_by_value[EAffect::kSlow] = "kSlow";
	EAffectFlag_name_by_value[EAffect::kHaste] = "kHaste";
	EAffectFlag_name_by_value[EAffect::kShield] = "kShield";
	EAffectFlag_name_by_value[EAffect::kAirShield] = "kAirShield";
	EAffectFlag_name_by_value[EAffect::kFireShield] = "kFireShield";
	EAffectFlag_name_by_value[EAffect::kIceShield] = "kIceShield";
	EAffectFlag_name_by_value[EAffect::kMagicGlass] = "kMagicGlass";
	EAffectFlag_name_by_value[EAffect::kStairs] = "kStairs";
	EAffectFlag_name_by_value[EAffect::kStoneHands] = "kStoneHands";
	EAffectFlag_name_by_value[EAffect::kPrismaticAura] = "kPrismaticAura";
	EAffectFlag_name_by_value[EAffect::kHelper] = "kHelper";
	EAffectFlag_name_by_value[EAffect::kForcesOfEvil] = "kForcesOfEvil";
	EAffectFlag_name_by_value[EAffect::kAitAura] = "kAitAura";
	EAffectFlag_name_by_value[EAffect::kFireAura] = "kFireAura";
	EAffectFlag_name_by_value[EAffect::kIceAura] = "kIceAura";
	EAffectFlag_name_by_value[EAffect::kDeafness] = "kDeafness";
	EAffectFlag_name_by_value[EAffect::kCrying] = "kCrying";
	EAffectFlag_name_by_value[EAffect::kPeaceful] = "kPeaceful";
	EAffectFlag_name_by_value[EAffect::kMagicStopFight] = "kMagicStopFight";
	EAffectFlag_name_by_value[EAffect::kBerserk] = "kBerserk";
	EAffectFlag_name_by_value[EAffect::kLightWalk] = "kLightWalk";
	EAffectFlag_name_by_value[EAffect::kBrokenChains] = "kBrokenChains";
	EAffectFlag_name_by_value[EAffect::kCloudOfArrows] = "kCloudOfArrows";
	EAffectFlag_name_by_value[EAffect::kShadowCloak] = "kShadowCloak";
	EAffectFlag_name_by_value[EAffect::kGlitterDust] = "kGlitterDust";
	EAffectFlag_name_by_value[EAffect::kAffright] = "kAffright";
	EAffectFlag_name_by_value[EAffect::kScopolaPoison] = "kScopolaPoison";
	EAffectFlag_name_by_value[EAffect::kDaturaPoison] = "kDaturaPoison";
	EAffectFlag_name_by_value[EAffect::kSkillReduce] = "kSkillReduce";
	EAffectFlag_name_by_value[EAffect::kNoBattleSwitch] = "kNoBattleSwitch";
	EAffectFlag_name_by_value[EAffect::kBelenaPoison] = "kBelenaPoison";
	EAffectFlag_name_by_value[EAffect::kNoTeleport] = "kNoTeleport";
	EAffectFlag_name_by_value[EAffect::kLacky] = "kLacky";
	EAffectFlag_name_by_value[EAffect::kBandage] = "kBandage";
	EAffectFlag_name_by_value[EAffect::kCannotBeBandaged] = "kCannotBeBandaged";
	EAffectFlag_name_by_value[EAffect::kMorphing] = "kMorphing";
	EAffectFlag_name_by_value[EAffect::kStrangled] = "kStrangled";
	EAffectFlag_name_by_value[EAffect::kMemorizeSpells] = "kMemorizeSpells";
	EAffectFlag_name_by_value[EAffect::kNoobRegen] = "kNoobRegen";
	EAffectFlag_name_by_value[EAffect::kVampirism] = "kVampirism";
	EAffectFlag_name_by_value[EAffect::kCommander] = "kCommander";
	EAffectFlag_name_by_value[EAffect::kEarthAura] = "kEarthAura";
	for (const auto &i : EAffectFlag_name_by_value) {
		EAffectFlag_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM(const EAffect item) {
	if (EAffectFlag_name_by_value.empty()) {
		init_EAffectFlag_ITEM_NAMES();
	}
	return EAffectFlag_name_by_value.at(item);
}

template<>
EAffect ITEM_BY_NAME(const std::string &name) {
	if (EAffectFlag_name_by_value.empty()) {
		init_EAffectFlag_ITEM_NAMES();
	}
	return EAffectFlag_value_by_name.at(name);
}

typedef std::map<EWeaponAffectFlag, std::string> EWeaponAffectFlag_name_by_value_t;
typedef std::map<const std::string, EWeaponAffectFlag> EWeaponAffectFlag_value_by_name_t;
EWeaponAffectFlag_name_by_value_t EWeaponAffectFlag_name_by_value;
EWeaponAffectFlag_value_by_name_t EWeaponAffectFlag_value_by_name;

void init_EWeaponAffectFlag_ITEM_NAMES() {
	EWeaponAffectFlag_name_by_value.clear();
	EWeaponAffectFlag_value_by_name.clear();

	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_BLINDNESS] = "WAFF_BLINDNESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_INVISIBLE] = "WAFF_INVISIBLE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_ALIGN] = "WAFF_DETECT_ALIGN";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_INVISIBLE] = "WAFF_DETECT_INVISIBLE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_MAGIC] = "WAFF_DETECT_MAGIC";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SENSE_LIFE] = "WAFF_SENSE_LIFE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_WATER_WALK] = "WAFF_WATER_WALK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SANCTUARY] = "WAFF_SANCTUARY";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_CURSE] = "WAFF_CURSE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_INFRAVISION] = "WAFF_INFRAVISION";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_POISON] = "WAFF_POISON";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_PROTECT_EVIL] = "WAFF_PROTECT_EVIL";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_PROTECT_GOOD] = "WAFF_PROTECT_GOOD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SLEEP] = "WAFF_SLEEP";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_NOTRACK] = "WAFF_NOTRACK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_BLESS] = "WAFF_BLESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SNEAK] = "WAFF_SNEAK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HIDE] = "WAFF_HIDE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HOLD] = "WAFF_HOLD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_FLY] = "WAFF_FLY";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SILENCE] = "WAFF_SILENCE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_AWARENESS] = "WAFF_AWARENESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_BLINK] = "WAFF_BLINK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_NOFLEE] = "WAFF_NOFLEE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SINGLE_LIGHT] = "WAFF_SINGLE_LIGHT";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HOLY_LIGHT] = "WAFF_HOLY_LIGHT";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HOLY_DARK] = "WAFF_HOLY_DARK";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DETECT_POISON] = "WAFF_DETECT_POISON";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SLOW] = "WAFF_SLOW";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HASTE] = "WAFF_HASTE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_WATER_BREATH] = "WAFF_WATER_BREATH";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_HAEMORRAGIA] = "WAFF_HAEMORRAGIA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_CAMOUFLAGE] = "WAFF_CAMOUFLAGE";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_SHIELD] = "WAFF_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_AIR_SHIELD] = "WAFF_AIR_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_FIRE_SHIELD] = "WAFF_FIRE_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_ICE_SHIELD] = "WAFF_ICE_SHIELD";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_MAGIC_GLASS] = "WAFF_MAGIC_GLASS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_STONE_HAND] = "WAFF_STONE_HAND";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_PRISMATIC_AURA] = "WAFF_PRISMATIC_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_AIR_AURA] = "WAFF_AIR_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_FIRE_AURA] = "WAFF_FIRE_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_ICE_AURA] = "WAFF_ICE_AURA";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_DEAFNESS] = "WAFF_DEAFNESS";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_COMMANDER] = "WAFF_COMMANDER";
	EWeaponAffectFlag_name_by_value[EWeaponAffectFlag::WAFF_EARTHAURA] = "WAFF_EARTHAURA";
	for (const auto &i : EWeaponAffectFlag_name_by_value) {
		EWeaponAffectFlag_value_by_name[i.second] = i.first;
	}
}

template<>
EWeaponAffectFlag ITEM_BY_NAME(const std::string &name) {
	if (EWeaponAffectFlag_name_by_value.empty()) {
		init_EWeaponAffectFlag_ITEM_NAMES();
	}
	return EWeaponAffectFlag_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM(const EWeaponAffectFlag item) {
	if (EWeaponAffectFlag_name_by_value.empty()) {
		init_EWeaponAffectFlag_ITEM_NAMES();
	}
	return EWeaponAffectFlag_name_by_value.at(item);
}

weapon_affect_t weapon_affect = {
	WeaponAffect{EWeaponAffectFlag::WAFF_BLINDNESS, 0, kSpellBlindness},
	WeaponAffect{EWeaponAffectFlag::WAFF_INVISIBLE, to_underlying(EAffect::kInvisible), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_DETECT_ALIGN, to_underlying(EAffect::kDetectAlign), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_DETECT_INVISIBLE, to_underlying(EAffect::kDetectInvisible), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_DETECT_MAGIC, to_underlying(EAffect::kDetectMagic), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_SENSE_LIFE, to_underlying(EAffect::kDetectLife), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_WATER_WALK, to_underlying(EAffect::kWaterWalk), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_SANCTUARY, to_underlying(EAffect::kSanctuary), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_CURSE, to_underlying(EAffect::kCurse), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_INFRAVISION, to_underlying(EAffect::kInfravision), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_POISON, 0, kSpellPoison},
	WeaponAffect{EWeaponAffectFlag::WAFF_PROTECT_EVIL, to_underlying(EAffect::kProtectedFromEvil), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_PROTECT_GOOD, to_underlying(EAffect::kProtectedFromGood), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_SLEEP, 0, kSpellSleep},
	WeaponAffect{EWeaponAffectFlag::WAFF_NOTRACK, to_underlying(EAffect::kNoTrack), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_BLESS, to_underlying(EAffect::kBless), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_SNEAK, to_underlying(EAffect::kSneak), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_HIDE, to_underlying(EAffect::kHide), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_HOLD, 0, kSpellHold},
	WeaponAffect{EWeaponAffectFlag::WAFF_FLY, to_underlying(EAffect::kFly), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_SILENCE, to_underlying(EAffect::kSilence), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_AWARENESS, to_underlying(EAffect::kAwarness), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_BLINK, to_underlying(EAffect::kBlink), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_NOFLEE, to_underlying(EAffect::kNoFlee), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_SINGLE_LIGHT, to_underlying(EAffect::kSingleLight), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_HOLY_LIGHT, to_underlying(EAffect::kHolyLight), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_HOLY_DARK, to_underlying(EAffect::kHolyDark), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_DETECT_POISON, to_underlying(EAffect::kDetectPoison), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_SLOW, to_underlying(EAffect::kSlow), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_HASTE, to_underlying(EAffect::kHaste), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_WATER_BREATH, to_underlying(EAffect::kWaterBreath), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_HAEMORRAGIA, to_underlying(EAffect::kHaemorrhage), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_CAMOUFLAGE, to_underlying(EAffect::kDisguise), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_SHIELD, to_underlying(EAffect::kShield), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_AIR_SHIELD, to_underlying(EAffect::kAirShield), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_FIRE_SHIELD, to_underlying(EAffect::kFireShield), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_ICE_SHIELD, to_underlying(EAffect::kIceShield), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_MAGIC_GLASS, to_underlying(EAffect::kMagicGlass), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_STONE_HAND, to_underlying(EAffect::kStoneHands), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_PRISMATIC_AURA, to_underlying(EAffect::kPrismaticAura), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_AIR_AURA, to_underlying(EAffect::kAitAura), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_FIRE_AURA, to_underlying(EAffect::kFireAura), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_ICE_AURA, to_underlying(EAffect::kIceAura), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_DEAFNESS, to_underlying(EAffect::kDeafness), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_COMMANDER, to_underlying(EAffect::kCommander), 0},
	WeaponAffect{EWeaponAffectFlag::WAFF_EARTHAURA, to_underlying(EAffect::kEarthAura), 0}
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
							 "не используется",
							 "множитель опыта",
							 "бонус ко всем умениям",
							 "лихорадка",
							 "безумное бормотание",
							 "защита.от.физических.повреждений",
							 "защита.от.магии.тьмы",
							 "роковое предчувствие",
							 "дополнительный бонус опыта",
							 "бонус физ. повреждений %",
							 "заклинание \"волшебное уклонение\"",
							 "бонус маг. повреждений %",
							 "\n"
};

typedef std::map<EApplyLocation, std::string> EApplyLocation_name_by_value_t;
typedef std::map<const std::string, EApplyLocation> EApplyLocation_value_by_name_t;
EApplyLocation_name_by_value_t EApplyLocation_name_by_value;
EApplyLocation_value_by_name_t EApplyLocation_value_by_name;

void init_EApplyLocation_ITEM_NAMES() {
	EApplyLocation_name_by_value.clear();
	EApplyLocation_value_by_name.clear();

	EApplyLocation_name_by_value[EApplyLocation::APPLY_NONE] = "APPLY_NONE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_STR] = "APPLY_STR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_DEX] = "APPLY_DEX";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_INT] = "APPLY_INT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_WIS] = "APPLY_WIS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CON] = "APPLY_CON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CHA] = "APPLY_CHA";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CLASS] = "APPLY_CLASS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_LEVEL] = "APPLY_LEVEL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_AGE] = "APPLY_AGE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CHAR_WEIGHT] = "APPLY_CHAR_WEIGHT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CHAR_HEIGHT] = "APPLY_CHAR_HEIGHT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MANAREG] = "APPLY_MANAREG";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_HIT] = "APPLY_HIT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MOVE] = "APPLY_MOVE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_GOLD] = "APPLY_GOLD";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_EXP] = "APPLY_EXP";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_AC] = "APPLY_AC";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_HITROLL] = "APPLY_HITROLL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_DAMROLL] = "APPLY_DAMROLL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_WILL] = "APPLY_SAVING_WILL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_FIRE] = "APPLY_RESIST_FIRE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_AIR] = "APPLY_RESIST_AIR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_DARK] = "APPLY_RESIST_DARK";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_CRITICAL] = "APPLY_SAVING_CRITICAL";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_STABILITY] = "APPLY_SAVING_STABILITY";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_HITREG] = "APPLY_HITREG";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MOVEREG] = "APPLY_MOVEREG";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C1] = "APPLY_C1";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C2] = "APPLY_C2";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C3] = "APPLY_C3";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C4] = "APPLY_C4";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C5] = "APPLY_C5";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C6] = "APPLY_C6";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C7] = "APPLY_C7";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C8] = "APPLY_C8";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_C9] = "APPLY_C9";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SIZE] = "APPLY_SIZE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_ARMOUR] = "APPLY_ARMOUR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_POISON] = "APPLY_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SAVING_REFLEX] = "APPLY_SAVING_REFLEX";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_CAST_SUCCESS] = "APPLY_CAST_SUCCESS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MORALE] = "APPLY_MORALE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_INITIATIVE] = "APPLY_INITIATIVE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RELIGION] = "APPLY_RELIGION";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_ABSORBE] = "APPLY_ABSORBE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_LIKES] = "APPLY_LIKES";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_WATER] = "APPLY_RESIST_WATER";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_EARTH] = "APPLY_RESIST_EARTH";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_VITALITY] = "APPLY_RESIST_VITALITY";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_MIND] = "APPLY_RESIST_MIND";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_IMMUNITY] = "APPLY_RESIST_IMMUNITY";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_AR] = "APPLY_AR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MR] = "APPLY_MR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_ACONITUM_POISON] = "APPLY_ACONITUM_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SCOPOLIA_POISON] = "APPLY_SCOPOLIA_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_BELENA_POISON] = "APPLY_BELENA_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_DATURA_POISON] = "APPLY_DATURA_POISON";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_FREE_FOR_USE] = "APPLY_FREE_FOR_USE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_BONUS_EXP] = "APPLY_BONUS_EXP";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_BONUS_SKILLS] = "APPLY_BONUS_SKILLS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PLAQUE] = "APPLY_PLAQUE";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_MADNESS] = "APPLY_MADNESS";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PR] = "APPLY_PR";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_RESIST_DARK] = "APPLY_RESIST_DARK";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_VIEW_DT] = "APPLY_VIEW_DT";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PERCENT_EXP] = "APPLY_PERCENT_EXP";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PERCENT_PHYSDAM] = "APPLY_PERCENT_PHYSDAM";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_SPELL_BLINK] = "APPLY_SPELL_BLINK";
	EApplyLocation_name_by_value[EApplyLocation::NUM_APPLIES] = "NUM_APPLIES";
	EApplyLocation_name_by_value[EApplyLocation::APPLY_PERCENT_MAGDAM] = "APPLY_PERCENT_MAGDAM";
	for (const auto &i : EApplyLocation_name_by_value) {
		EApplyLocation_value_by_name[i.second] = i.first;
	}
}

template<>
EApplyLocation ITEM_BY_NAME(const std::string &name) {
	if (EApplyLocation_name_by_value.empty()) {
		init_EApplyLocation_ITEM_NAMES();
	}
	return EApplyLocation_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM(const EApplyLocation item) {
	if (EApplyLocation_name_by_value.empty()) {
		init_EApplyLocation_ITEM_NAMES();
	}
	return EApplyLocation_name_by_value.at(item);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
