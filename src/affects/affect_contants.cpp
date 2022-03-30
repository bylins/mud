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
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kProtectedFromEvil] = "kProtectedFromEvil";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kProtectedFromGood] = "kProtectedFromGood";
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

weapon_affect_t weapon_affect = {
	WeaponAffect{EWeaponAffect::kBlindness, 0, kSpellBlindness},
	WeaponAffect{EWeaponAffect::kInvisibility, to_underlying(EAffect::kInvisible), 0},
	WeaponAffect{EWeaponAffect::kDetectAlign, to_underlying(EAffect::kDetectAlign), 0},
	WeaponAffect{EWeaponAffect::kDetectInvisibility, to_underlying(EAffect::kDetectInvisible), 0},
	WeaponAffect{EWeaponAffect::kDetectMagic, to_underlying(EAffect::kDetectMagic), 0},
	WeaponAffect{EWeaponAffect::kDetectLife, to_underlying(EAffect::kDetectLife), 0},
	WeaponAffect{EWeaponAffect::kWaterWalk, to_underlying(EAffect::kWaterWalk), 0},
	WeaponAffect{EWeaponAffect::kSanctuary, to_underlying(EAffect::kSanctuary), 0},
	WeaponAffect{EWeaponAffect::kCurse, to_underlying(EAffect::kCurse), 0},
	WeaponAffect{EWeaponAffect::kInfravision, to_underlying(EAffect::kInfravision), 0},
	WeaponAffect{EWeaponAffect::kPoison, 0, kSpellPoison},
	WeaponAffect{EWeaponAffect::kProtectedFromEvil, to_underlying(EAffect::kProtectedFromEvil), 0},
	WeaponAffect{EWeaponAffect::kProtectedFromGood, to_underlying(EAffect::kProtectedFromGood), 0},
	WeaponAffect{EWeaponAffect::kSleep, 0, kSpellSleep},
	WeaponAffect{EWeaponAffect::kNoTrack, to_underlying(EAffect::kNoTrack), 0},
	WeaponAffect{EWeaponAffect::kBless, to_underlying(EAffect::kBless), 0},
	WeaponAffect{EWeaponAffect::kSneak, to_underlying(EAffect::kSneak), 0},
	WeaponAffect{EWeaponAffect::kHide, to_underlying(EAffect::kHide), 0},
	WeaponAffect{EWeaponAffect::kHold, 0, kSpellHold},
	WeaponAffect{EWeaponAffect::kFly, to_underlying(EAffect::kFly), 0},
	WeaponAffect{EWeaponAffect::kSilence, to_underlying(EAffect::kSilence), 0},
	WeaponAffect{EWeaponAffect::kAwareness, to_underlying(EAffect::kAwarness), 0},
	WeaponAffect{EWeaponAffect::kBlink, to_underlying(EAffect::kBlink), 0},
	WeaponAffect{EWeaponAffect::kNoFlee, to_underlying(EAffect::kNoFlee), 0},
	WeaponAffect{EWeaponAffect::kSingleLight, to_underlying(EAffect::kSingleLight), 0},
	WeaponAffect{EWeaponAffect::kHolyLight, to_underlying(EAffect::kHolyLight), 0},
	WeaponAffect{EWeaponAffect::kHolyDark, to_underlying(EAffect::kHolyDark), 0},
	WeaponAffect{EWeaponAffect::kDetectPoison, to_underlying(EAffect::kDetectPoison), 0},
	WeaponAffect{EWeaponAffect::kSlow, to_underlying(EAffect::kSlow), 0},
	WeaponAffect{EWeaponAffect::kHaste, to_underlying(EAffect::kHaste), 0},
	WeaponAffect{EWeaponAffect::kWaterBreath, to_underlying(EAffect::kWaterBreath), 0},
	WeaponAffect{EWeaponAffect::kHaemorrhage, to_underlying(EAffect::kHaemorrhage), 0},
	WeaponAffect{EWeaponAffect::kDisguising, to_underlying(EAffect::kDisguise), 0},
	WeaponAffect{EWeaponAffect::kShield, to_underlying(EAffect::kShield), 0},
	WeaponAffect{EWeaponAffect::kAirShield, to_underlying(EAffect::kAirShield), 0},
	WeaponAffect{EWeaponAffect::kFireShield, to_underlying(EAffect::kFireShield), 0},
	WeaponAffect{EWeaponAffect::kIceShield, to_underlying(EAffect::kIceShield), 0},
	WeaponAffect{EWeaponAffect::kMagicGlass, to_underlying(EAffect::kMagicGlass), 0},
	WeaponAffect{EWeaponAffect::kStoneHand, to_underlying(EAffect::kStoneHands), 0},
	WeaponAffect{EWeaponAffect::kPrismaticAura, to_underlying(EAffect::kPrismaticAura), 0},
	WeaponAffect{EWeaponAffect::kAirAura, to_underlying(EAffect::kAitAura), 0},
	WeaponAffect{EWeaponAffect::kFireAura, to_underlying(EAffect::kFireAura), 0},
	WeaponAffect{EWeaponAffect::kIceAura, to_underlying(EAffect::kIceAura), 0},
	WeaponAffect{EWeaponAffect::kDeafness, to_underlying(EAffect::kDeafness), 0},
	WeaponAffect{EWeaponAffect::kComamnder, to_underlying(EAffect::kCommander), 0},
	WeaponAffect{EWeaponAffect::kEarthAura, to_underlying(EAffect::kEarthAura), 0}
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
