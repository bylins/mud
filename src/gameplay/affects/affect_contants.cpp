/**
 \authors Created by Sventovit
 \date 18.01.2022.
 \brief Константы системы аффектов.
*/

#include "affect_contants.h"
#include "affects_loader.h"
#include "affect_messages.h"

#include "gameplay/magic/spells.h"
#include "engine/structs/structs.h"   // kIntOne/kIntTwo plane prefixes
#include "engine/structs/msg_container.h"
#include "engine/structs/info_container.h"   // kUndefinedVnum
#include "utils/logger.h"
#include "engine/core/config.h"   // CommonMsg / ECommonMsg

#include <array>
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
	EAffectFlag_name_by_value[EAffect::kInjured] = "kInjured";
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
	for (const auto &i : EAffectFlag_name_by_value) {
		EAffectFlag_value_by_name[i.second] = i.first;
	}
	// issue.ext-affects: the shared "kDefault" sheaf id (used by MsgContainer<EAffect,...> for the
	// affect_msg.xml group scaffolding) resolves to the kUndefined slot, like ESkill/ESpell. Also accept
	// the fixed spelling "kUndefined" alongside the legacy "kUndefinded" (already in the map above).
	EAffectFlag_value_by_name["kDefault"] = EAffect::kUndefined;
	EAffectFlag_value_by_name["kUndefined"] = EAffect::kUndefined;
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
							 "бонус.физ.повреждений %",
							 "волшебное.уклонение.от.физ.урона",
							 "сила.заклинаний.%",
							 "волшебное.уклонение.от.маг.урона",
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
	EApplyLocation_name_by_value[EApply::kSpelledBlinkPhys] =  "kSpelledBlinkPhys";
	EApplyLocation_name_by_value[EApply::kMagicDamagePercent] = "kMagicDamagePercent";
	EApplyLocation_name_by_value[EApply::kSpelledBlinkMag] =  "kSpelledBlinkMag";
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
	};

msg_container::MsgContainer<EAffect, affects::EAffectMsgType> &AffectMsgContainer() {
	static msg_container::MsgContainer<EAffect, affects::EAffectMsgType> container;
	return container;
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
void AffectsLoader::Load(parser_wrapper::DataNode data) { ValidateAffectRegistry(data); }
void AffectsLoader::Reload(parser_wrapper::DataNode data) { ValidateAffectRegistry(data); }
void AffectMessagesLoader::Load(parser_wrapper::DataNode data) {
	AffectMsgContainer().Init(data.Children());
	g_affect_messages_loaded = true;
}
void AffectMessagesLoader::Reload(parser_wrapper::DataNode data) {
	AffectMsgContainer().Reload(data.Children());
	g_affect_messages_loaded = true;
}
}  // namespace affects

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
