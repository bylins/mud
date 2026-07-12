/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы системы аффектов.
*/

#include "affect_contants.h"
#include "affects_loader.h"
#include "affect_messages.h"
#include "utils/utils_string.h"   // str_cmp (Vedun CanonicalElementId)
#include "utils/utils_parse.h"   // parse::ReadAsConstantsBitvector

#include "gameplay/magic/spells.h"
#include "engine/structs/structs.h"   // kIntOne/kIntTwo plane prefixes
#include "engine/structs/msg_container.h"
#include "engine/structs/info_container.h"   // kUndefinedVnum
#include "utils/logger.h"
#include "engine/core/config.h"   // CommonMsg / ECommonMsg
#include "gameplay/abilities/talents_actions.h"   // issue.character-affect-triggers: per-affect <actions>

#include <array>
#include <algorithm>
#include <bit>
#include <set>

// issue.affect-migration: affect short names are queried straight from the affect system
// (affects::DescribeActive / AffectMsg) -- no affected_bits[] projection. This flag only tracks
// whether the affect_messages cfg has loaded yet (the boot guard in db.cpp uses it).
namespace {
bool g_affect_messages_loaded = false;
}  // namespace

typedef std::map<EAffect, std::string> EAffectFlag_name_by_value_t;
typedef std::map<const std::string, EAffect> EAffectFlag_value_by_name_t;
EAffectFlag_name_by_value_t EAffectFlag_name_by_value;
EAffectFlag_value_by_name_t EAffectFlag_value_by_name;
void init_EAffectFlag_ITEM_NAMES() {
	EAffectFlag_value_by_name.clear();
	EAffectFlag_name_by_value.clear();

	// Keep the legacy data token "kUndefinded" (the historical typo) so existing spells.xml/talent data
	// that uses it as the "no flag" value still parses; the C++ enumerator is the fixed kUndefined.
	EAffectFlag_name_by_value[EAffect::kUndefined] = "kUndefinded";
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
	EAffectFlag_name_by_value[EAffect::kProtectFromDark] = "kProtectFromDark";
	EAffectFlag_name_by_value[EAffect::kProtectFromMind] = "kProtectFromMind";
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
	EAffectFlag_name_by_value[EAffect::kGodsShield] = "kGodsShield";
	EAffectFlag_name_by_value[EAffect::kAirShield] = "kAirShield";
	EAffectFlag_name_by_value[EAffect::kFireShield] = "kFireShield";
	EAffectFlag_name_by_value[EAffect::kIceShield] = "kIceShield";
	EAffectFlag_name_by_value[EAffect::kMagicGlass] = "kMagicGlass";
	EAffectFlag_name_by_value[EAffect::kStairs] = "kStairs";
	EAffectFlag_name_by_value[EAffect::kStoneHands] = "kStoneHands";
	EAffectFlag_name_by_value[EAffect::kPrismaticAura] = "kPrismaticAura";
	EAffectFlag_name_by_value[EAffect::kHelper] = "kHelper";
	EAffectFlag_name_by_value[EAffect::kForcesOfEvil] = "kForcesOfEvil";
	EAffectFlag_name_by_value[EAffect::kAirAura] = "kAirAura";
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
	EAffectFlag_name_by_value[EAffect::kCombatLuck] = "kCombatLuck";
	EAffectFlag_name_by_value[EAffect::kBandage] = "kBandage";
	EAffectFlag_name_by_value[EAffect::kCannotBeBandaged] = "kCannotBeBandaged";
	EAffectFlag_name_by_value[EAffect::kMorphing] = "kMorphing";
	EAffectFlag_name_by_value[EAffect::kStrangled] = "kStrangled";
	EAffectFlag_name_by_value[EAffect::kMemorizeSpells] = "kMemorizeSpells";
	EAffectFlag_name_by_value[EAffect::kNoobRegen] = "kNoobRegen";
	EAffectFlag_name_by_value[EAffect::kVampirism] = "kVampirism";
	EAffectFlag_name_by_value[EAffect::kLacerations] = "kLacerations";
	EAffectFlag_name_by_value[EAffect::kCommander] = "kCommander";
	EAffectFlag_name_by_value[EAffect::kEarthAura] = "kEarthAura";
	EAffectFlag_name_by_value[EAffect::kCloudly] = "kCloudly";
	EAffectFlag_name_by_value[EAffect::kConfused] = "kConfused";
	EAffectFlag_name_by_value[EAffect::kNoCharge] = "kNoCharge";
	EAffectFlag_name_by_value[EAffect::kInjuredLimb] = "kInjuredLimb";
	EAffectFlag_name_by_value[EAffect::kFrenzy] = "kFrenzy";
	EAffectFlag_name_by_value[EAffect::kMagicArmor] = "kMagicArmor";
	EAffectFlag_name_by_value[EAffect::kShivering] = "kShivering";
	EAffectFlag_name_by_value[EAffect::kDespodency] = "kDespodency";
	EAffectFlag_name_by_value[EAffect::kMagicStrength] = "kMagicStrength";
	EAffectFlag_name_by_value[EAffect::kPatronage] = "kPatronage";
	EAffectFlag_name_by_value[EAffect::kStoneSkin] = "kStoneSkin";
	EAffectFlag_name_by_value[EAffect::kMagicWeaknes] = "kMagicWeaknes";
	EAffectFlag_name_by_value[EAffect::kEnlarge] = "kEnlarge";
	EAffectFlag_name_by_value[EAffect::kMagicShield] = "kMagicShield";
	EAffectFlag_name_by_value[EAffect::kForbidden] = "kForbidden";
	EAffectFlag_name_by_value[EAffect::kFrostbite] = "kFrostbite";
	EAffectFlag_name_by_value[EAffect::kSlowdown] = "kSlowdown";
	EAffectFlag_name_by_value[EAffect::kFever] = "kFever";
	EAffectFlag_name_by_value[EAffect::kFastRegeneration] = "kFastRegeneration";
	EAffectFlag_name_by_value[EAffect::kLessening] = "kLessening";
	EAffectFlag_name_by_value[EAffect::kMadness] = "kMadness";
	EAffectFlag_name_by_value[EAffect::kMindless] = "kMindless";
	EAffectFlag_name_by_value[EAffect::kFascination] = "kFascination";
	EAffectFlag_name_by_value[EAffect::kContusion] = "kContusion";
	EAffectFlag_name_by_value[EAffect::kStoneBones] = "kStoneBones";
	EAffectFlag_name_by_value[EAffect::kFailure] = "kFailure";
	EAffectFlag_name_by_value[EAffect::kCatGrace] = "kCatGrace";
	EAffectFlag_name_by_value[EAffect::kBullBody] = "kBullBody";
	EAffectFlag_name_by_value[EAffect::kSnakeWisdom] = "kSnakeWisdom";
	EAffectFlag_name_by_value[EAffect::kGimmicry] = "kGimmicry";
	EAffectFlag_name_by_value[EAffect::kIndecision] = "kIndecision";
	EAffectFlag_name_by_value[EAffect::kInsanity] = "kInsanity";
	EAffectFlag_name_by_value[EAffect::kRousing] = "kRousing";
	EAffectFlag_name_by_value[EAffect::kPassion] = "kPassion";
	EAffectFlag_name_by_value[EAffect::kSurgeOfPower] = "kSurgeOfPower";
	EAffectFlag_name_by_value[EAffect::kSurgeOfValour] = "kSurgeOfValour";
	EAffectFlag_name_by_value[EAffect::kBattleCourage] = "kBattleCourage";
	EAffectFlag_name_by_value[EAffect::kInspiration] = "kInspiration";
	EAffectFlag_name_by_value[EAffect::kConcentration] = "kConcentration";
	EAffectFlag_name_by_value[EAffect::kBattleLuck] = "kBattleLuck";
	EAffectFlag_name_by_value[EAffect::kPhysdamageBonus] = "kPhysdamageBonus";
	EAffectFlag_name_by_value[EAffect::kSuspiciousness] = "kSuspiciousness";
	EAffectFlag_name_by_value[EAffect::kSevereWound] = "kSevereWound";
	EAffectFlag_name_by_value[EAffect::kWellFed] = "kWellFed";
	EAffectFlag_name_by_value[EAffect::kPrayerful] = "kPrayerful";
	EAffectFlag_name_by_value[EAffect::kPietas] = "kPietas";
	EAffectFlag_name_by_value[EAffect::kEvade] = "kEvade";
	EAffectFlag_name_by_value[EAffect::kWitchery] = "kWitchery";
	EAffectFlag_name_by_value[EAffect::kAconitumPoison] = "kAconitumPoison";
	EAffectFlag_name_by_value[EAffect::kCapable] = "kCapable";
	EAffectFlag_name_by_value[EAffect::kWebbed] = "kWebbed";
	EAffectFlag_name_by_value[EAffect::kInsidiousWound] = "kInsidiousWound";
	EAffectFlag_name_by_value[EAffect::kBurning] = "kBurning";
	for (const auto &i : EAffectFlag_name_by_value) {
		EAffectFlag_value_by_name[i.second] = i.first;
	}
	// issue.ext-affects: the shared "kDefault" sheaf id (used by MsgContainer<EAffect,...> for the
	// affect_msg.xml group scaffolding) resolves to the kUndefined slot, like ESkill/ESpell. Also accept
	// the fixed spelling "kUndefined" alongside the legacy "kUndefinded" (already in the map above).
	EAffectFlag_value_by_name["kDefault"] = EAffect::kUndefined;
	EAffectFlag_value_by_name["kUndefined"] = EAffect::kUndefined;
	// issue.witchery-typo: accept the legacy misspelling "kWirchery" alongside the fixed "kWitchery"
	// so old player saves / world scripts / data still resolve on load.
	EAffectFlag_value_by_name["kWirchery"] = EAffect::kWitchery;
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
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kProtectFromDark] = "kProtectFromDark";
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kProtectFromMind] = "kProtectFromMind";
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
	EWeaponAffectFlag_name_by_value[EWeaponAffect::kCloudly] = "kCloudly";
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
	WeaponAffect{EWeaponAffect::kEarthAura, to_underlying(EAffect::kEarthAura), ESpell::kUndefined},
	WeaponAffect{EWeaponAffect::kCloudly, to_underlying(EAffect::kCloudly), ESpell::kUndefined}
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
							 "*множитель.опыта",
							 "бонус.ко.всем.умениям",
							 "лихорадка",
							 "безумное.бормотание",
							 "защита.от.физических.повреждений",
							 "защита.от.магии.тьмы",
							 "роковое.предчувствие",
							 "*дополнительный.бонус.опыта",
							 "бонус.физ.повреждений.%",
							 "волшебное.уклонение.от.физ.урона",
							 "сила.заклинаний.%",
							 "волшебное.уклонение.от.маг.урона",
							 "путы",
							 "отравление",
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
	// issue.affects-improve (P3d): EApply::kScopolaPoison RETIRED (dead -- no affect_modify case);
	// enum value 54 kept reserved (objects persist EApply by value, never renumber), but not
	// name-mapped so it can no longer be referenced from XML.
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
	EApplyLocation_name_by_value[EApply::kSpelledBlinkPhys] =  "kSpelledBlinkPhys";
	EApplyLocation_name_by_value[EApply::kMagicDamagePercent] = "kMagicDamagePercent";
	EApplyLocation_name_by_value[EApply::kSpelledBlinkMag] =  "kSpelledBlinkMag";
	EApplyLocation_name_by_value[EApply::kBind] = "kBind";
	EApplyLocation_name_by_value[EApply::kPoisoned] = "kPoisoned";
	EApplyLocation_name_by_value[EApply::kNumberApplies] = "kNumberApplies";
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

typedef std::map<EAffFlag, std::string> EAffFlag_name_by_value_t;
typedef std::map<const std::string, EAffFlag> EAffFlag_value_by_name_t;
EAffFlag_name_by_value_t EAffFlag_name_by_value;
EAffFlag_value_by_name_t EAffFlag_value_by_name;

void init_EAffFlag_ITEM_NAMES() {
	EAffFlag_name_by_value.clear();
	EAffFlag_value_by_name.clear();

	EAffFlag_name_by_value[EAffFlag::kAfBattledec] = "kAfBattledec";
	EAffFlag_name_by_value[EAffFlag::kAfDeadkeep] = "kAfDeadkeep";
	EAffFlag_name_by_value[EAffFlag::kAfPulsedec] = "kAfPulsedec";
	EAffFlag_name_by_value[EAffFlag::kAfSameTime] = "kAfSameTime";
	EAffFlag_name_by_value[EAffFlag::kAfUpdateDuration] = "kAfUpdateDuration";
	EAffFlag_name_by_value[EAffFlag::kAfAccumulateDuration] = "kAfAccumulateDuration";
	EAffFlag_name_by_value[EAffFlag::kAfUpdateMod] = "kAfUpdateMod";
	EAffFlag_name_by_value[EAffFlag::kAfDispellable] = "kAfDispellable";
	EAffFlag_name_by_value[EAffFlag::kAfCurable] = "kAfCurable";
	EAffFlag_name_by_value[EAffFlag::kAfMustBeHandled] = "kAfMustBeHandled";
	EAffFlag_name_by_value[EAffFlag::kAfUnique] = "kAfUnique";
	EAffFlag_name_by_value[EAffFlag::kAfFailed] = "kAfFailed";
	EAffFlag_name_by_value[EAffFlag::kAfPoison] = "kAfPoison";
	EAffFlag_name_by_value[EAffFlag::kAfEntanglement] = "kAfEntanglement";
	EAffFlag_name_by_value[EAffFlag::kAfCharmBond] = "kAfCharmBond";
	EAffFlag_name_by_value[EAffFlag::kAfNeedControl] = "kAfNeedControl";
	EAffFlag_name_by_value[EAffFlag::kAfCasterInRoom] = "kAfCasterInRoom";
	EAffFlag_name_by_value[EAffFlag::kAfCasterInWorld] = "kAfCasterInWorld";
	EAffFlag_name_by_value[EAffFlag::kAfCasterInWorldDelay] = "kAfCasterInWorldDelay";
	EAffFlag_name_by_value[EAffFlag::kAfIgnoreBlink] = "kAfIgnoreBlink";
	EAffFlag_name_by_value[EAffFlag::kAfNoPositionBonus] = "kAfNoPositionBonus";
	EAffFlag_name_by_value[EAffFlag::kAfNoCritBonus] = "kAfNoCritBonus";
	EAffFlag_name_by_value[EAffFlag::kAfFullAbsorb] = "kAfFullAbsorb";
	EAffFlag_name_by_value[EAffFlag::kAfMaterialize] = "kAfMaterialize";
	EAffFlag_name_by_value[EAffFlag::kAfBoon] = "kAfBoon";
	EAffFlag_name_by_value[EAffFlag::kAfWarding] = "kAfWarding";
	EAffFlag_name_by_value[EAffFlag::kAfAegis] = "kAfAegis";

	for (const auto &i : EAffFlag_name_by_value) {
		EAffFlag_value_by_name[i.second] = i.first;
	}
}

template<>
EAffFlag ITEM_BY_NAME<EAffFlag>(const std::string &name) {
	if (EAffFlag_name_by_value.empty()) {
		init_EAffFlag_ITEM_NAMES();
	}
	return EAffFlag_value_by_name.at(name);
}

template<>
const std::string &NAME_BY_ITEM<EAffFlag>(const EAffFlag item) {
	if (EAffFlag_name_by_value.empty()) {
		init_EAffFlag_ITEM_NAMES();
	}
	return EAffFlag_name_by_value.at(item);
}

template<>
const std::map<EAffFlag, std::string> &NAMES_OF<EAffFlag>() {
	if (EAffFlag_name_by_value.empty()) {
		init_EAffFlag_ITEM_NAMES();
	}
	return EAffFlag_name_by_value;
}

template<>
const std::map<EAffect, std::string> &NAMES_OF<EAffect>() {
	if (EAffectFlag_name_by_value.empty()) {
		init_EAffectFlag_ITEM_NAMES();
	}
	return EAffectFlag_name_by_value;
}

template<>
const std::map<EApply, std::string> &NAMES_OF<EApply>() {
	if (EApplyLocation_name_by_value.empty()) {
		init_EApplyLocation_ITEM_NAMES();
	}
	return EApplyLocation_name_by_value;
}

// --- issue.ext-affects: affect short names + aura look text (cfg/messages/ru/affect_msg.xml) ---------
// The message container is keyed BY AFFECT (one <msg_sheaf id="<EAffect>"> per affect, like spell/skill
// messages) + a shared "kDefault" sheaf for the merged shield/aura group scaffolding. EAffectMsgType is
// the per-sheaf slot. affected_bits (the short names sprintbits walks) is rebuilt from each affect's
// kShortDesc after the file loads, so it is data too -- no hand-kept positional array.
namespace {
const std::map<affects::EAffectMsgType, std::string> kAffectMsgTypeNames{
		{affects::EAffectMsgType::kUndefined, "kUndefined"},
		{affects::EAffectMsgType::kShortDesc, "kShortDesc"},
		{affects::EAffectMsgType::kLook, "kLook"},
		{affects::EAffectMsgType::kLookPoly, "kLookPoly"},
		{affects::EAffectMsgType::kShieldFrame, "kShieldFrame"},
		{affects::EAffectMsgType::kShieldNoun, "kShieldNoun"},
		{affects::EAffectMsgType::kShieldNounMany, "kShieldNounMany"},
		{affects::EAffectMsgType::kAuraFrame, "kAuraFrame"},
		{affects::EAffectMsgType::kAuraNoun, "kAuraNoun"},
		{affects::EAffectMsgType::kAuraNounMany, "kAuraNounMany"},
		{affects::EAffectMsgType::kAffImposedToRoom, "kAffImposedToRoom"},
		{affects::EAffectMsgType::kAffImposedToChar, "kAffImposedToChar"},
		{affects::EAffectMsgType::kAffImposedToVict, "kAffImposedToVict"},
		{affects::EAffectMsgType::kAffImposeFailToChar, "kAffImposeFailToChar"},
		{affects::EAffectMsgType::kAffImposeFailToVict, "kAffImposeFailToVict"},
		{affects::EAffectMsgType::kAffImposeFailToRoom, "kAffImposeFailToRoom"},
		{affects::EAffectMsgType::kAffDispelledToRoom, "kAffDispelledToRoom"},
		{affects::EAffectMsgType::kAffDispelledToChar, "kAffDispelledToChar"},
		{affects::EAffectMsgType::kAffExpiredToChar, "kAffExpiredToChar"},
		{affects::EAffectMsgType::kAffExpiredToRoom, "kAffExpiredToRoom"},
		{affects::EAffectMsgType::kAffExpireSoon, "kAffExpireSoon"},
		{affects::EAffectMsgType::kWardToChar, "kWardToChar"},
		{affects::EAffectMsgType::kWardToVict, "kWardToVict"},
		{affects::EAffectMsgType::kWardToRoom, "kWardToRoom"},
		{affects::EAffectMsgType::kDamageToChar, "kDamageToChar"},
		{affects::EAffectMsgType::kDamageToVict, "kDamageToVict"},
		{affects::EAffectMsgType::kDamageToRoom, "kDamageToRoom"},
		{affects::EAffectMsgType::kDeathToChar, "kDeathToChar"},
		{affects::EAffectMsgType::kDeathToRoom, "kDeathToRoom"},
		{affects::EAffectMsgType::kTransformToChar, "kTransformToChar"},
		{affects::EAffectMsgType::kTransformToVict, "kTransformToVict"},
		{affects::EAffectMsgType::kTransformToRoom, "kTransformToRoom"},
		{affects::EAffectMsgType::kTransformCritToChar, "kTransformCritToChar"},
		{affects::EAffectMsgType::kTransformCritToVict, "kTransformCritToVict"},
		{affects::EAffectMsgType::kTransformCritToRoom, "kTransformCritToRoom"},
	};

msg_container::MsgContainer<EAffect, affects::EAffectMsgType> &AffectMsgContainer() {
	static msg_container::MsgContainer<EAffect, affects::EAffectMsgType> container;
	return container;
}

// issue.affect-migration: per-affect_type behavior flags (kAfCurable/kAfDispellable/kAfBattledec/...)
// loaded from affects.xml. affects.xml is the SOURCE OF TRUTH for what an effect does; the casting
// source (spell/skill/item) only sets strength + duration. Indexed by to_underlying(EAffect)
// (EAffect is 1-based; index 0 = kUndefined = no flags).
constexpr std::size_t kAffectFlagTableSize = 138;  // EAffect max (kBurning=137) + 1
std::array<Bitvector, kAffectFlagTableSize> g_affect_flags{};
std::array<affects::EBuff, kAffectFlagTableSize> g_affect_buff{};
// issue.affects-improve (P2): per-affect stat-change applies (location + modifier formula) from
// affects.xml. Built here but NOT yet read by the impose path (that switch is P3).
std::array<std::vector<affects::AffectApply>, kAffectFlagTableSize> g_affect_applies{};
// issue.character-affect-triggers: per-affect pulse/battle-pulse <actions> (same talents_actions grammar
// as room_affects), so a char affect can carry data-driven periodic effects (DoT). Mirrors
// g_room_affect_actions. Indexed by to_underlying(EAffect).
std::array<talents_actions::Actions, kAffectFlagTableSize> g_affect_actions{};
// issue.damage-change: per-affect "magic shield" selection weight (<shield weight="N"/>). >0 marks the
// affect as one of the mutually-exclusive elemental shields; a hit passes through exactly one, picked
// weighted-random by these values. 0 = not a shield.
std::array<int, kAffectFlagTableSize> g_affect_shield_weight{};
// issue.random-noise-rework: per-affect additive dispel modifier (<affect dispel_mod="N"/>), in dispel-
// contest threshold percentage points. Negative -> harder to remove (critical buffs). 0 = neutral.
std::array<int, kAffectFlagTableSize> g_affect_dispel_mod{};
// issue.mob-flag-affect-materialization: affect types flagged kAfMaterialize, collected once at load so
// the zone-wake materializer can walk a short list instead of scanning every EAffect per mob.
std::vector<EAffect> g_materializable_affects{};
// issue.damage-change: affect types flagged kAfFullAbsorb, collected once at load so the total-immunity
// block can scan a short list of AFF flags instead of hard-coding the kGodsShield affect id.
std::vector<EAffect> g_full_absorb_affects{};
bool g_affect_flags_loaded = false;

// Parse an <affect buff="Y|N|A"> attribute into EBuff (the affect-side analog of ParseViolent). Absent
// or unrecognised -> kAmbiguous (classification undetermined).
static affects::EBuff ParseAffectBuff(const char *raw) {
	if (!raw || !*raw) return affects::EBuff::kAmbiguous;
	if (*raw == 'Y' || *raw == 'y' || *raw == '1') return affects::EBuff::kYes;
	if (*raw == 'N' || *raw == 'n' || *raw == '0') return affects::EBuff::kNo;
	return affects::EBuff::kAmbiguous;
}

void BuildAffectFlagTable(parser_wrapper::DataNode data) {
	g_affect_flags.fill(0);
	g_affect_buff.fill(affects::EBuff::kAmbiguous);
	g_affect_shield_weight.fill(0);
	g_affect_dispel_mod.fill(0);
	for (auto &v : g_affect_applies) { v.clear(); }
	for (auto &a : g_affect_actions) { a = talents_actions::Actions{}; }
	for (auto &node : data.Children("affect")) {
		const char *id = node.GetValue("id");
		if (!id || !*id) {
			continue;
		}
		EAffect affect;
		try {
			affect = ITEM_BY_NAME<EAffect>(id);
		} catch (const std::out_of_range &) {
			continue;  // unknown id already reported by ValidateAffectRegistry
		}
		const auto idx = static_cast<std::size_t>(to_underlying(affect));
		if (idx >= kAffectFlagTableSize) {
			continue;
		}
		// issue.affect-migration: effect flags live in a <flags val="..."> child tag (unified with the
		// spells.xml format); read off a COPY of the node (the obj_sets pattern).
		auto fnode = node;
		if (fnode.GoToChild("flags")) {
			if (const char *flags = fnode.GetValue("val"); flags && *flags) {
				g_affect_flags[idx] = parse::ReadAsConstantsBitvector<EAffFlag>(flags);
			}
		}
		// issue.affect-migration: the affect's own buff/debuff/ambiguous classification.
		g_affect_buff[idx] = ParseAffectBuff(node.GetValue("buff"));
		// issue.random-noise-rework: additive dispel-threshold modifier (percentage points), default 0.
		if (const char *dm = node.GetValue("dispel_mod"); dm && *dm) {
			g_affect_dispel_mod[idx] = parse::ReadAsInt(dm);
		}
		// issue.damage-change: <shield weight="N"/> -- marks a mutually-exclusive elemental shield and its
		// weighted-random selection weight (read off a COPY of the node, the obj_sets pattern).
		{
			auto snode = node;
			if (snode.GoToChild("shield")) {
				if (const char *w = snode.GetValue("weight"); w && *w) {
					g_affect_shield_weight[idx] = std::max(0, parse::ReadAsInt(w));
				}
			}
		}
		// issue.affects-improve (P2): the affect's stat-change applies (location + <modifier>), same
		// shape as a spell's <apply>. The affect IS the id, so applies carry only location + formula.
		for (auto &ap : node.Children("apply")) {
			affects::AffectApply a;
			a.location = parse::ReadAsConstant<EApply>(ap.GetValue("location"));
			const char *r = ap.GetValue("random");
			a.random = (r && *r) && parse::ReadAsBool(r);
			auto mn = ap;
			if (mn.GoToChild("modifier")) {
				a.min = parse::ReadAsDouble(mn.GetValue("min"));
				a.beta = parse::ReadAsDouble(mn.GetValue("beta"));
				{ const char *w = mn.GetValue("weight"); a.weight = (w && *w) ? parse::ReadAsDouble(w) : 0.0; }  // issue.potency-noise
				a.factor = parse::ReadAsInt(mn.GetValue("factor"));
				const char *st = mn.GetValue("stack");
				a.stack = (st && *st) ? std::max(1, parse::ReadAsInt(st)) : 1;
				const char *cp = mn.GetValue("cap");
				a.cap = (cp && *cp) ? std::max(0, parse::ReadAsInt(cp)) : 0;
			}
			g_affect_applies[idx].push_back(a);
		}
		// issue.character-affect-triggers: the affect's own pulse/battle-pulse <actions> (same grammar as
		// a spell's <talent_actions> / room_affects). Read off a COPY of the node (the obj_sets pattern).
		{
			auto act_node = node;
			if (act_node.GoToChild("actions")) {
				g_affect_actions[idx].Build(act_node);
			}
		}
	}
	// issue.mob-flag-affect-materialization: cache the kAfMaterialize affect types.
	g_materializable_affects.clear();
	// issue.damage-change: cache the kAfFullAbsorb affect types (total-immunity flag scan).
	g_full_absorb_affects.clear();
	for (std::size_t i = 0; i < kAffectFlagTableSize; ++i) {
		if (g_affect_flags[i] & to_underlying(EAffFlag::kAfMaterialize)) {
			g_materializable_affects.push_back(static_cast<EAffect>(i));
		}
		if (g_affect_flags[i] & to_underlying(EAffFlag::kAfFullAbsorb)) {
			g_full_absorb_affects.push_back(static_cast<EAffect>(i));
		}
	}
	g_affect_flags_loaded = true;
}

// affects.xml is the affect registry (id-only for now; grows per-affect handler/action data later).
// Validate every <affect id> is a known affect and every affect has a row.
void ValidateAffectRegistry(parser_wrapper::DataNode data) {
	std::set<std::string> seen;
	for (auto &node : data.Children("affect")) {
		const char *id = node.GetValue("id");
		if (!id || !*id) {
			log("SYSERROR: affects.xml: <affect> without an id -- skipped.");
			continue;
		}
		try {
			(void) ITEM_BY_NAME<EAffect>(id);
		} catch (const std::out_of_range &) {
			log("SYSERROR: affects.xml: unknown affect id '%s' -- skipped.", id);
			continue;
		}
		seen.insert(id);
	}
	for (const auto &[affect, token] : NAMES_OF<EAffect>()) {
		if (affect != EAffect::kUndefined && seen.find(token) == seen.end()) {
			log("SYSERROR: affects.xml: missing <affect id=\"%s\">.", token.c_str());
		}
	}
}
}  // namespace

template<>
const std::string &NAME_BY_ITEM<affects::EAffectMsgType>(const affects::EAffectMsgType item) {
	return kAffectMsgTypeNames.at(item);
}
template<>
const std::map<affects::EAffectMsgType, std::string> &NAMES_OF<affects::EAffectMsgType>() {
	return kAffectMsgTypeNames;
}
template<>
affects::EAffectMsgType ITEM_BY_NAME<affects::EAffectMsgType>(const std::string &name) {
	static std::map<std::string, affects::EAffectMsgType> by_name;
	if (by_name.empty()) {
		for (const auto &[value, token] : kAffectMsgTypeNames) {
			by_name.emplace(token, value);
		}
	}
	return by_name.at(name);
}

namespace affects {
const std::string &AffectMsg(EAffect affect, EAffectMsgType slot) {
	return AffectMsgContainer().GetMessage(affect, slot);
}

const std::string &AffectMsgRaw(EAffect affect, EAffectMsgType slot) {
	static const std::string kEmpty;
	auto &c = AffectMsgContainer();
	return c.IsKnown(affect) ? c[affect].GetMessage(slot) : kEmpty;
}

const std::string &AffectMsgWeapon(EAffect affect, EAffectMsgType slot, bool has_weapon) {
	static const std::string kEmpty;
	auto &c = AffectMsgContainer();
	if (!c.IsKnown(affect)) return kEmpty;
	const auto &all = c[affect].GetMessages(slot);
	if (all.empty()) return kEmpty;
	std::vector<const std::string *> pool;
	if (has_weapon) {
		for (const auto &m : all) if (m.find("$o") != std::string::npos) pool.push_back(&m);
		if (pool.empty()) for (const auto &m : all) pool.push_back(&m);
	} else {
		for (const auto &m : all) if (m.find("$o") == std::string::npos) pool.push_back(&m);
	}
	if (pool.empty()) return kEmpty;
	return *pool[number(0, static_cast<int>(pool.size()) - 1)];
}

bool MessagesLoaded() { return g_affect_messages_loaded; }

// Affects in NAMES_OF order (kUndefined excluded) -- the ordered list the OLC affect editor
// numbers and toggles by index.
const std::vector<EAffect> &MenuOrder() {
	static std::vector<EAffect> order;
	if (order.empty()) {
		for (const auto &[affect, token] : NAMES_OF<EAffect>()) {
			if (affect != EAffect::kUndefined) {
				order.push_back(affect);
			}
		}
	}
	return order;
}

// issue.affect-migration: the affect's intrinsic behavior flags (cure/dispel/refresh/decrement/...)
// from affects.xml, keyed by affect_type. 0 for affects with no row/flags. Not yet wired into the
// apply path (Phase 1 -- table built, sourcing switched in Phase 2).
Bitvector AffectFlagsByType(EAffect affect_type) {
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kAffectFlagTableSize ? g_affect_flags[idx] : Bitvector{0};
}

// issue.character-affect-triggers: the affect's own pulse/battle-pulse <actions> from affects.xml.
const talents_actions::Actions &AffectActions(EAffect affect_type) {
	static const talents_actions::Actions kEmpty;
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kAffectFlagTableSize ? g_affect_actions[idx] : kEmpty;
}

// issue.affect-migration: the affect's buff/debuff/ambiguous classification from affects.xml.
EBuff AffectBuffKind(EAffect affect_type) {
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kAffectFlagTableSize ? g_affect_buff[idx] : EBuff::kAmbiguous;
}

// issue.damage-change: the affect's magic-shield selection weight (0 = not a shield).
int AffectShieldWeight(EAffect affect_type) {
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kAffectFlagTableSize ? g_affect_shield_weight[idx] : 0;
}

// issue.random-noise-rework: the affect's additive dispel modifier (0 = neutral).
int AffectDispelMod(EAffect affect_type) {
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kAffectFlagTableSize ? g_affect_dispel_mod[idx] : 0;
}

// issue.mob-flag-affect-materialization: affect types flagged kAfMaterialize (buffs to realize as real
// affects on flag-only NPCs when their zone wakes).
const std::vector<EAffect> &MaterializableAffects() {
	return g_materializable_affects;
}

// issue.damage-change: affect types flagged kAfFullAbsorb (grant total damage immunity).
const std::vector<EAffect> &FullAbsorbAffects() {
	return g_full_absorb_affects;
}

// issue.affects-improve (P2): the affect's stat-change applies from affects.xml (empty if none).
const std::vector<AffectApply> &AffectApplies(EAffect affect_type) {
	static const std::vector<AffectApply> kEmpty;
	const auto idx = static_cast<std::size_t>(to_underlying(affect_type));
	return idx < kAffectFlagTableSize ? g_affect_applies[idx] : kEmpty;
}

// True once affects.xml flags have been loaded; before that (unit tests, early boot) callers keep
// their own battleflag instead of sourcing from the (empty) table.
bool AffectFlagsLoaded() { return g_affect_flags_loaded; }

// The flat bit index of an active affect back to its EAffect (the inverse of the 1-based
// flag_index_mapping<EAffect>: enum value = index + 1). kUndefined if it is not a known affect.
EAffect AffectByIndex(std::size_t flat_index) {
	const EAffect affect = static_cast<EAffect>(flat_index + 1);
	return NAMES_OF<EAffect>().count(affect) ? affect : EAffect::kUndefined;
}

// Joined short descriptions of all active affects, queried straight from the affect system (no
// affected_bits[] projection). Returns CommonMsg(kNothing) when none are set.
std::string DescribeActive(const BitsetFlags<EAffect> &flags, const char *div) {
	std::string result;
	flags.for_each_set([&](std::size_t i) {
		const EAffect affect = AffectByIndex(i);
		if (affect == EAffect::kUndefined) {
			return;
		}
		const std::string &name = AffectMsg(affect, EAffectMsgType::kShortDesc);
		if (name.empty()) {
			return;
		}
		if (!result.empty()) {
			result += div;
		}
		result += name;
	});
	return result.empty() ? CommonMsg(ECommonMsg::kNothing) : result;
}

// Resolve an affect by its short description (prefix match, mirroring the old ext_search_block on
// affected_bits). Iterates in EAffect order for deterministic first-match.
bool FindByShortDesc(const std::string &name, EAffect &out) {
	if (name.empty()) {
		return false;
	}
	for (const auto &[affect, token] : NAMES_OF<EAffect>()) {
		if (affect == EAffect::kUndefined) {
			continue;
		}
		const std::string &sd = AffectMsg(affect, EAffectMsgType::kShortDesc);
		if (!sd.empty() && sd.size() >= name.size() && sd.compare(0, name.size(), name) == 0) {
			out = affect;
			return true;
		}
	}
	return false;
}
void AffectsLoader::Load(parser_wrapper::DataNode data) { ValidateAffectRegistry(data); BuildAffectFlagTable(data); }
void AffectsLoader::Reload(parser_wrapper::DataNode data) { ValidateAffectRegistry(data); BuildAffectFlagTable(data); }

// issue.vedun-editor: in-game editing of cfg/affects.xml.
std::string AffectsLoader::EditableWhat() const { return "affects"; }

std::vector<cfg_manager::EditableElement> AffectsLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &[affect, token] : NAMES_OF<EAffect>()) {
		if (affect == EAffect::kUndefined) {
			continue;
		}
		const std::string &label = affects::AffectMsg(affect, affects::EAffectMsgType::kShortDesc);
		out.push_back({token, label.empty() ? token : label});
	}
	return out;
}

// Dry-run: re-check every <affect> the editor is about to save against the same enum/range rules the
// loader applies, so an invalid id / location / flag / buff / modifier is rejected BEFORE the file is
// written (the .scheme already constrains the editor's pick-lists; this is the on-save safety net).
cfg_manager::ValidationResult AffectsLoader::Validate(parser_wrapper::DataNode &doc) const {
	for (auto &affect : doc.Children("affect")) {
		const char *id = affect.GetValue("id");
		if (!id || !*id) {
			return {false, "<affect> without an id."};
		}
		try {
			(void) ITEM_BY_NAME<EAffect>(id);
		} catch (const std::out_of_range &) {
			return {false, std::string("unknown affect id '") + id + "'."};
		}
		if (const char *b = affect.GetValue("buff"); b && *b && strcmp(b, "Y") != 0 && strcmp(b, "N") != 0) {
			return {false, std::string("affect '") + id + "': buff must be Y or N."};
		}
		if (auto fnode = affect; fnode.GoToChild("flags")) {
			if (const char *fv = fnode.GetValue("val"); fv && *fv) {
				try {
					(void) parse::ReadAsConstantsBitvector<EAffFlag>(fv);
				} catch (const std::exception &) {
					return {false, std::string("affect '") + id + "': unknown flag in '" + fv + "'."};
				}
			}
		}
		for (auto &ap : affect.Children("apply")) {
			if (const char *loc = ap.GetValue("location"); loc && *loc) {
				try {
					(void) ITEM_BY_NAME<EApply>(loc);
				} catch (const std::out_of_range &) {
					return {false, std::string("affect '") + id + "': unknown apply location '" + loc + "'."};
				}
			}
		}
	}
	return {true, ""};
}
void AffectMessagesLoader::Load(parser_wrapper::DataNode data) {
	AffectMsgContainer().Init(data.Children());
	g_affect_messages_loaded = true;
}
void AffectMessagesLoader::Reload(parser_wrapper::DataNode data) {
	AffectMsgContainer().Reload(data.Children());
	g_affect_messages_loaded = true;
}

// issue.unstable-hotfixes: Vedun in-game editing of affect messages (`vedun affectmsg`). Mirrors
// SpellMessagesLoader -- keyed by the <msg_sheaf id=> (EAffect token; the shared sheaf is "kDefault").
std::string AffectMessagesLoader::EditableWhat() const {
	return "affectmsg";
}

std::vector<cfg_manager::EditableElement> AffectMessagesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &sheaf : AffectMsgContainer()) {
		const EAffect id = sheaf.GetId();
		// The default sheaf is stored under kUndefined but its XML id is "kDefault".
		const std::string id_str = (id == EAffect::kUndefined) ? "kDefault" : NAME_BY_ITEM<EAffect>(id);
		out.push_back({id_str, sheaf.GetMessage(EAffectMsgType::kShortDesc)});   // label = short desc
	}
	return out;
}

cfg_manager::ValidationResult AffectMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (AffectMsgContainer().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Affect-message data failed to parse (see syslog for the offending sheaf/message)."};
}

std::string AffectMessagesLoader::CanonicalElementId(const std::string &id) const {
	for (const auto &[value, name] : NAMES_OF<EAffect>()) {
		if (value != EAffect::kUndefined && str_cmp(name, id) == 0) {
			return name;
		}
	}
	return "";
}

parser_wrapper::DataNode AffectMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	// An empty sheaf for `id`; the editor then adds <message> children.
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}
}  // namespace affects

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
