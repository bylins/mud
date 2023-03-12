/**
\authors Created by Sventovit
\date 27.04.2022.
\brief Константы системы заклинаний.
*/

#include "spells_constants.h"
#include "entities/entities_constants.h"

#include <map>
#include <sstream>

typedef std::map<ESpell, std::string> ESpell_name_by_value_t;
typedef std::map<const std::string, ESpell> ESpell_value_by_name_t;
ESpell_name_by_value_t ESpell_name_by_value;
ESpell_value_by_name_t ESpell_value_by_name;
void init_ESpell_ITEM_NAMES() {
	ESpell_value_by_name.clear();
	ESpell_name_by_value.clear();

	ESpell_name_by_value[ESpell::kUndefined] = "kUndefined";
	ESpell_name_by_value[ESpell::kArmor] = "kArmor";
	ESpell_name_by_value[ESpell::kTeleport] = "kTeleport";
	ESpell_name_by_value[ESpell::kBless] = "kBless";
	ESpell_name_by_value[ESpell::kBlindness] = "kBlindness";
	ESpell_name_by_value[ESpell::kBurningHands] = "kBurningHands";
	ESpell_name_by_value[ESpell::kCallLighting] = "kCallLighting";
	ESpell_name_by_value[ESpell::kCharm] = "kCharm";
	ESpell_name_by_value[ESpell::kChillTouch] = "kChillTouch";
	ESpell_name_by_value[ESpell::kClone] = "kClone";
	ESpell_name_by_value[ESpell::kIceBolts] = "kIceBolts";
	ESpell_name_by_value[ESpell::kControlWeather] = "kControlWeather";
	ESpell_name_by_value[ESpell::kCreateFood] = "kCreateFood";
	ESpell_name_by_value[ESpell::kCreateWater] = "kCreateWater";
	ESpell_name_by_value[ESpell::kCureBlind] = "kCureBlind";
	ESpell_name_by_value[ESpell::kCureCritic] = "kCureCritic";
	ESpell_name_by_value[ESpell::kCureLight] = "kCureLight";
	ESpell_name_by_value[ESpell::kCurse] = "kCurse";
	ESpell_name_by_value[ESpell::kDetectAlign] = "kDetectAlign";
	ESpell_name_by_value[ESpell::kDetectInvis] = "kDetectInvis";
	ESpell_name_by_value[ESpell::kDetectMagic] = "kDetectMagic";
	ESpell_name_by_value[ESpell::kDetectPoison] = "kDetectPoison";
	ESpell_name_by_value[ESpell::kDispelEvil] = "kDispelEvil";
	ESpell_name_by_value[ESpell::kEarthquake] = "kEarthquake";
	ESpell_name_by_value[ESpell::kEnchantWeapon] = "kEnchantWeapon";
	ESpell_name_by_value[ESpell::kEnergyDrain] = "kEnergyDrain";
	ESpell_name_by_value[ESpell::kFireball] = "kFireball";
	ESpell_name_by_value[ESpell::kHarm] = "kHarm";
	ESpell_name_by_value[ESpell::kHeal] = "kHeal";
	ESpell_name_by_value[ESpell::kInvisible] = "kInvisible";
	ESpell_name_by_value[ESpell::kLightingBolt] = "kLightingBolt";
	ESpell_name_by_value[ESpell::kLocateObject] = "kLocateObject";
	ESpell_name_by_value[ESpell::kMagicMissile] = "kMagicMissile";
	ESpell_name_by_value[ESpell::kPoison] = "kPoison";
	ESpell_name_by_value[ESpell::kProtectFromEvil] = "kProtectFromEvil";
	ESpell_name_by_value[ESpell::kRemoveCurse] = "kRemoveCurse";
	ESpell_name_by_value[ESpell::kSanctuary] = "kSanctuary";
	ESpell_name_by_value[ESpell::kShockingGasp] = "kShockingGasp";
	ESpell_name_by_value[ESpell::kSleep] = "kSleep";
	ESpell_name_by_value[ESpell::kStrength] = "kStrength";
	ESpell_name_by_value[ESpell::kSummon] = "kSummon";
	ESpell_name_by_value[ESpell::kPatronage] = "kPatronage";
	ESpell_name_by_value[ESpell::kWorldOfRecall] = "kWorldOfRecall";
	ESpell_name_by_value[ESpell::kRemovePoison] = "kRemovePoison";
	ESpell_name_by_value[ESpell::kSenseLife] = "kSenseLife";
	ESpell_name_by_value[ESpell::kAnimateDead] = "kAnimateDead";
	ESpell_name_by_value[ESpell::kDispelGood] = "kDispelGood";
	ESpell_name_by_value[ESpell::kGroupArmor] = "kGroupArmor";
	ESpell_name_by_value[ESpell::kGroupHeal] = "kGroupHeal";
	ESpell_name_by_value[ESpell::kGroupRecall] = "kGroupRecall";
	ESpell_name_by_value[ESpell::kInfravision] = "kInfravision";
	ESpell_name_by_value[ESpell::kWaterwalk] = "kWaterwalk";
	ESpell_name_by_value[ESpell::kCureSerious] = "kCureSerious";
	ESpell_name_by_value[ESpell::kGroupStrength] = "kGroupStrength";
	ESpell_name_by_value[ESpell::kHold] = "kHold";
	ESpell_name_by_value[ESpell::kPowerHold] = "kPowerHold";
	ESpell_name_by_value[ESpell::kMassHold] = "kMassHold";
	ESpell_name_by_value[ESpell::kFly] = "kFly";
	ESpell_name_by_value[ESpell::kBrokenChains] = "kBrokenChains";
	ESpell_name_by_value[ESpell::kNoflee] = "kNoflee";
	ESpell_name_by_value[ESpell::kCreateLight] = "kCreateLight";
	ESpell_name_by_value[ESpell::kDarkness] = "kDarkness";
	ESpell_name_by_value[ESpell::kStoneSkin] = "kStoneSkin";
	ESpell_name_by_value[ESpell::kCloudly] = "kCloudly";
	ESpell_name_by_value[ESpell::kSilence] = "kSilence";
	ESpell_name_by_value[ESpell::kLight] = "kLight";
	ESpell_name_by_value[ESpell::kChainLighting] = "kChainLighting";
	ESpell_name_by_value[ESpell::kFireBlast] = "kFireBlast";
	ESpell_name_by_value[ESpell::kGodsWrath] = "kGodsWrath";
	ESpell_name_by_value[ESpell::kWeaknes] = "kWeaknes";
	ESpell_name_by_value[ESpell::kGroupInvisible] = "kGroupInvisible";
	ESpell_name_by_value[ESpell::kShadowCloak] = "kShadowCloak";
	ESpell_name_by_value[ESpell::kAcid] = "kAcid";
	ESpell_name_by_value[ESpell::kRepair] = "kRepair";
	ESpell_name_by_value[ESpell::kEnlarge] = "kEnlarge";
	ESpell_name_by_value[ESpell::kFear] = "kFear";
	ESpell_name_by_value[ESpell::kSacrifice] = "kSacrifice";
	ESpell_name_by_value[ESpell::kWeb] = "kWeb";
	ESpell_name_by_value[ESpell::kBlink] = "kBlink";
	ESpell_name_by_value[ESpell::kRemoveHold] = "kRemoveHold";
	ESpell_name_by_value[ESpell::kCamouflage] = "kCamouflage";
	ESpell_name_by_value[ESpell::kPowerBlindness] = "kPowerBlindness";
	ESpell_name_by_value[ESpell::kMassBlindness] = "kMassBlindness";
	ESpell_name_by_value[ESpell::kPowerSilence] = "kPowerSilence";
	ESpell_name_by_value[ESpell::kExtraHits] = "kExtraHits";
	ESpell_name_by_value[ESpell::kResurrection] = "kResurrection";
	ESpell_name_by_value[ESpell::kMagicShield] = "kMagicShield";
	ESpell_name_by_value[ESpell::kForbidden] = "kForbidden";
	ESpell_name_by_value[ESpell::kMassSilence] = "kMassSilence";
	ESpell_name_by_value[ESpell::kRemoveSilence] = "kRemoveSilence";
	ESpell_name_by_value[ESpell::kDamageLight] = "kDamageLight";
	ESpell_name_by_value[ESpell::kDamageSerious] = "kDamageSerious";
	ESpell_name_by_value[ESpell::kDamageCritic] = "kDamageCritic";
	ESpell_name_by_value[ESpell::kMassCurse] = "kMassCurse";
	ESpell_name_by_value[ESpell::kArmageddon] = "kArmageddon";
	ESpell_name_by_value[ESpell::kGroupFly] = "kGroupFly";
	ESpell_name_by_value[ESpell::kGroupBless] = "kGroupBless";
	ESpell_name_by_value[ESpell::kResfresh] = "kResfresh";
	ESpell_name_by_value[ESpell::kStunning] = "kStunning";
	ESpell_name_by_value[ESpell::kHide] = "kHide";
	ESpell_name_by_value[ESpell::kSneak] = "kSneak";
	ESpell_name_by_value[ESpell::kDrunked] = "kDrunked";
	ESpell_name_by_value[ESpell::kAbstinent] = "kAbstinent";
	ESpell_name_by_value[ESpell::kFullFeed] = "kFullFeed";
	ESpell_name_by_value[ESpell::kColdWind] = "kColdWind";
	ESpell_name_by_value[ESpell::kBattle] = "kBattle";
	ESpell_name_by_value[ESpell::kHaemorrhage] = "kHaemorrhage";
	ESpell_name_by_value[ESpell::kCourage] = "kCourage";
	ESpell_name_by_value[ESpell::kWaterbreath] = "kWaterbreath";
	ESpell_name_by_value[ESpell::kSlowdown] = "kSlowdown";
	ESpell_name_by_value[ESpell::kHaste] = "kHaste";
	ESpell_name_by_value[ESpell::kMassSlow] = "kMassSlow";
	ESpell_name_by_value[ESpell::kGroupHaste] = "kGroupHaste";
	ESpell_name_by_value[ESpell::kGodsShield] = "kGodsShield";
	ESpell_name_by_value[ESpell::kFever] = "kFever";
	ESpell_name_by_value[ESpell::kCureFever] = "kCureFever";
	ESpell_name_by_value[ESpell::kAwareness] = "kAwareness";
	ESpell_name_by_value[ESpell::kReligion] = "kReligion";
	ESpell_name_by_value[ESpell::kAirShield] = "kAirShield";
	ESpell_name_by_value[ESpell::kPortal] = "kPortal";
	ESpell_name_by_value[ESpell::kDispellMagic] = "kDispellMagic";
	ESpell_name_by_value[ESpell::kSummonKeeper] = "kSummonKeeper";
	ESpell_name_by_value[ESpell::kFastRegeneration] = "kFastRegeneration";
	ESpell_name_by_value[ESpell::kCreateWeapon] = "kCreateWeapon";
	ESpell_name_by_value[ESpell::kFireShield] = "kFireShield";
	ESpell_name_by_value[ESpell::kRelocate] = "kRelocate";
	ESpell_name_by_value[ESpell::kSummonFirekeeper] = "kSummonFirekeeper";
	ESpell_name_by_value[ESpell::kIceShield] = "kIceShield";
	ESpell_name_by_value[ESpell::kIceStorm] = "kIceStorm";
	ESpell_name_by_value[ESpell::kLessening] = "kLessening";
	ESpell_name_by_value[ESpell::kShineFlash] = "kShineFlash";
	ESpell_name_by_value[ESpell::kMadness] = "kMadness";
	ESpell_name_by_value[ESpell::kGroupMagicGlass] = "kGroupMagicGlass";
	ESpell_name_by_value[ESpell::kCloudOfArrows] = "kCloudOfArrows";
	ESpell_name_by_value[ESpell::kVacuum] = "kVacuum";
	ESpell_name_by_value[ESpell::kMeteorStorm] = "kMeteorStorm";
	ESpell_name_by_value[ESpell::kStoneHands] = "kStoneHands";
	ESpell_name_by_value[ESpell::kMindless] = "kMindless";
	ESpell_name_by_value[ESpell::kPrismaticAura] = "kPrismaticAura";
	ESpell_name_by_value[ESpell::kEviless] = "kEviless";
	ESpell_name_by_value[ESpell::kAirAura] = "kAirAura";
	ESpell_name_by_value[ESpell::kFireAura] = "kFireAura";
	ESpell_name_by_value[ESpell::kIceAura] = "kIceAura";
	ESpell_name_by_value[ESpell::kShock] = "kShock";
	ESpell_name_by_value[ESpell::kMagicGlass] = "kMagicGlass";
	ESpell_name_by_value[ESpell::kGroupSanctuary] = "kGroupSanctuary";
	ESpell_name_by_value[ESpell::kGroupPrismaticAura] = "kGroupPrismaticAura";
	ESpell_name_by_value[ESpell::kDeafness] = "kDeafness";
	ESpell_name_by_value[ESpell::kPowerDeafness] = "kPowerDeafness";
	ESpell_name_by_value[ESpell::kRemoveDeafness] = "kRemoveDeafness";
	ESpell_name_by_value[ESpell::kMassDeafness] = "kMassDeafness";
	ESpell_name_by_value[ESpell::kDustStorm] = "kDustStorm";
	ESpell_name_by_value[ESpell::kEarthfall] = "kEarthfall";
	ESpell_name_by_value[ESpell::kSonicWave] = "kSonicWave";
	ESpell_name_by_value[ESpell::kHolystrike] = "kHolystrike";
	ESpell_name_by_value[ESpell::kSumonAngel] = "kSumonAngel";
	ESpell_name_by_value[ESpell::kMassFear] = "kMassFear";
	ESpell_name_by_value[ESpell::kFascination] = "kFascination";
	ESpell_name_by_value[ESpell::kCrying] = "kCrying";
	ESpell_name_by_value[ESpell::kOblivion] = "kOblivion";
	ESpell_name_by_value[ESpell::kBurdenOfTime] = "kBurdenOfTime";
	ESpell_name_by_value[ESpell::kGroupRefresh] = "kGroupRefresh";
	ESpell_name_by_value[ESpell::kPeaceful] = "kPeaceful";
	ESpell_name_by_value[ESpell::kMagicBattle] = "kMagicBattle";
	ESpell_name_by_value[ESpell::kBerserk] = "kBerserk";
	ESpell_name_by_value[ESpell::kStoneBones] = "kStoneBones";
	ESpell_name_by_value[ESpell::kRoomLight] = "kRoomLight";
	ESpell_name_by_value[ESpell::kDeadlyFog] = "kDeadlyFog";
	ESpell_name_by_value[ESpell::kThunderstorm] = "kThunderstorm";
	ESpell_name_by_value[ESpell::kLightWalk] = "kLightWalk";
	ESpell_name_by_value[ESpell::kFailure] = "kFailure";
	ESpell_name_by_value[ESpell::kClanPray] = "kClanPray";
	ESpell_name_by_value[ESpell::kGlitterDust] = "kGlitterDust";
	ESpell_name_by_value[ESpell::kScream] = "kScream";
	ESpell_name_by_value[ESpell::kCatGrace] = "kCatGrace";
	ESpell_name_by_value[ESpell::kBullBody] = "kBullBody";
	ESpell_name_by_value[ESpell::kSnakeWisdom] = "kSnakeWisdom";
	ESpell_name_by_value[ESpell::kGimmicry] = "kGimmicry";
	ESpell_name_by_value[ESpell::kWarcryOfChallenge] = "kWarcryOfChallenge";
	ESpell_name_by_value[ESpell::kWarcryOfMenace] = "kWarcryOfMenace";
	ESpell_name_by_value[ESpell::kWarcryOfRage] = "kWarcryOfRage";
	ESpell_name_by_value[ESpell::kWarcryOfMadness] = "kWarcryOfMadness";
	ESpell_name_by_value[ESpell::kWarcryOfThunder] = "kWarcryOfThunder";
	ESpell_name_by_value[ESpell::kWarcryOfDefence] = "kWarcryOfDefence";
	ESpell_name_by_value[ESpell::kWarcryOfBattle] = "kWarcryOfBattle";
	ESpell_name_by_value[ESpell::kWarcryOfPower] = "kWarcryOfPower";
	ESpell_name_by_value[ESpell::kWarcryOfBless] = "kWarcryOfBless";
	ESpell_name_by_value[ESpell::kWarcryOfCourage] = "kWarcryOfCourage";
	ESpell_name_by_value[ESpell::kWarcryOfExperience] = "kWarcryOfExperience";
	ESpell_name_by_value[ESpell::kWarcryOfLuck] = "kWarcryOfLuck";
	ESpell_name_by_value[ESpell::kWarcryOfPhysdamage] = "kWarcryOfPhysdamage";
	ESpell_name_by_value[ESpell::kRuneLabel] = "kRuneLabel";
	ESpell_name_by_value[ESpell::kAconitumPoison] = "kAconitumPoison";
	ESpell_name_by_value[ESpell::kScopolaPoison] = "kScopolaPoison";
	ESpell_name_by_value[ESpell::kBelenaPoison] = "kBelenaPoison";
	ESpell_name_by_value[ESpell::kDaturaPoison] = "kDaturaPoison";
	ESpell_name_by_value[ESpell::kTimerRestore] = "kTimerRestore";
	ESpell_name_by_value[ESpell::kCombatLuck] = "kCombatLuck";
	ESpell_name_by_value[ESpell::kBandage] = "kBandage";
	ESpell_name_by_value[ESpell::kNoBandage] = "kNoBandage";
	ESpell_name_by_value[ESpell::kCapable] = "kCapable";
	ESpell_name_by_value[ESpell::kStrangle] = "kStrangle";
	ESpell_name_by_value[ESpell::kRecallSpells] = "kRecallSpells";
	ESpell_name_by_value[ESpell::kHypnoticPattern] = "kHypnoticPattern";
	ESpell_name_by_value[ESpell::kSolobonus] = "kSolobonus";
	ESpell_name_by_value[ESpell::kVampirism] = "kVampirism";
	ESpell_name_by_value[ESpell::kRestoration] = "kRestoration";
	ESpell_name_by_value[ESpell::kDeathAura] = "kDeathAura";
	ESpell_name_by_value[ESpell::kRecovery] = "kRecovery";
	ESpell_name_by_value[ESpell::kMassRecovery] = "kMassRecovery";
	ESpell_name_by_value[ESpell::kAuraOfEvil] = "kAuraOfEvil";
	ESpell_name_by_value[ESpell::kMentalShadow] = "kMentalShadow";
	ESpell_name_by_value[ESpell::kBlackTentacles] = "kBlackTentacles";
	ESpell_name_by_value[ESpell::kWhirlwind] = "kWhirlwind";
	ESpell_name_by_value[ESpell::kIndriksTeeth] = "kIndriksTeeth";
	ESpell_name_by_value[ESpell::kAcidArrow] = "kAcidArrow";
	ESpell_name_by_value[ESpell::kThunderStone] = "kThunderStone";
	ESpell_name_by_value[ESpell::kClod] = "kClod";
	ESpell_name_by_value[ESpell::kExpedient] = "kExpedient";
	ESpell_name_by_value[ESpell::kSightOfDarkness] = "kSightOfDarkness";
	ESpell_name_by_value[ESpell::kGroupSincerity] = "kGroupSincerity";
	ESpell_name_by_value[ESpell::kMagicalGaze] = "kMagicalGaze";
	ESpell_name_by_value[ESpell::kAllSeeingEye] = "kAllSeeingEye";
	ESpell_name_by_value[ESpell::kEyeOfGods] = "kEyeOfGods";
	ESpell_name_by_value[ESpell::kBreathingAtDepth] = "kBreathingAtDepth";
	ESpell_name_by_value[ESpell::kGeneralRecovery] = "kGeneralRecovery";
	ESpell_name_by_value[ESpell::kCommonMeal] = "kCommonMeal";
	ESpell_name_by_value[ESpell::kStoneWall] = "kStoneWall";
	ESpell_name_by_value[ESpell::kSnakeEyes] = "kSnakeEyes";
	ESpell_name_by_value[ESpell::kEarthAura] = "kEarthAura";
	ESpell_name_by_value[ESpell::kGroupProtectFromEvil] = "kGroupProtectFromEvil";
	ESpell_name_by_value[ESpell::kArrowsFire] = "kArrowsFire";
	ESpell_name_by_value[ESpell::kArrowsWater] = "kArrowsWater";
	ESpell_name_by_value[ESpell::kArrowsEarth] = "kArrowsEarth";
	ESpell_name_by_value[ESpell::kArrowsAir] = "kArrowsAir";
	ESpell_name_by_value[ESpell::kArrowsDeath] = "kArrowsDeath";
	ESpell_name_by_value[ESpell::kPaladineInspiration] = "kPaladineInspiration";
	ESpell_name_by_value[ESpell::kDexterity] = "kDexterity";
	ESpell_name_by_value[ESpell::kGroupBlink] = "kGroupBlink";
	ESpell_name_by_value[ESpell::kGroupCloudly] = "kGroupCloudly";
	ESpell_name_by_value[ESpell::kGroupAwareness] = "kGroupAwareness";
	ESpell_name_by_value[ESpell::kMassFailure] = "kMassFailure";
	ESpell_name_by_value[ESpell::kSnare] = "kSnare";
	ESpell_name_by_value[ESpell::kExpedientFail] = "kExpedientFail";
	ESpell_name_by_value[ESpell::kFireBreath] = "kFireBreath";
	ESpell_name_by_value[ESpell::kGasBreath] = "kGasBreath";
	ESpell_name_by_value[ESpell::kFrostBreath] = "kFrostBreath";
	ESpell_name_by_value[ESpell::kAcidBreath] = "kAcidBreath";
	ESpell_name_by_value[ESpell::kLightingBreath] = "kLightingBreath";
	ESpell_name_by_value[ESpell::kIdentify] = "kIdentify";
	ESpell_name_by_value[ESpell::kFullIdentify] = "kFullIdentify";
	ESpell_name_by_value[ESpell::kQUest] = "kQUest";
	ESpell_name_by_value[ESpell::kPortalTimer] = "kPortalTimer";

	for (const auto &i: ESpell_name_by_value) {
		ESpell_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ESpell>(const ESpell item) {
	if (ESpell_name_by_value.empty()) {
		init_ESpell_ITEM_NAMES();
	}
	return ESpell_name_by_value.at(item);
}

template<>
ESpell ITEM_BY_NAME(const std::string &name) {
	if (ESpell_name_by_value.empty()) {
		init_ESpell_ITEM_NAMES();
	}
	return ESpell_value_by_name.at(name);
}

const ESpell &operator++(ESpell &s) {
	s = static_cast<ESpell>(to_underlying(s) + 1);
	return s;
}

std::ostream &operator<<(std::ostream &os, const ESpell &s) {
	os << to_underlying(s) << " (" << NAME_BY_ITEM<ESpell>(s) << ")";
	return os;
};

typedef std::map<EElement, std::string> EElement_name_by_value_t;
typedef std::map<const std::string, EElement> EElement_value_by_name_t;
EElement_name_by_value_t EElement_name_by_value;
EElement_value_by_name_t EElement_value_by_name;
void init_EElement_ITEM_NAMES() {
	EElement_value_by_name.clear();
	EElement_name_by_value.clear();

	EElement_name_by_value[EElement::kUndefined] = "kUndefined";
	EElement_name_by_value[EElement::kAir] = "kAir";
	EElement_name_by_value[EElement::kFire] = "kFire";
	EElement_name_by_value[EElement::kWater] = "kWater";
	EElement_name_by_value[EElement::kEarth] = "kEarth";
	EElement_name_by_value[EElement::kLight] = "kLight";
	EElement_name_by_value[EElement::kDark] = "kDark";
	EElement_name_by_value[EElement::kMind] = "kMind";
	EElement_name_by_value[EElement::kLife] = "kLife";

	for (const auto &i: EElement_name_by_value) {
		EElement_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EElement>(const EElement item) {
	if (EElement_name_by_value.empty()) {
		init_EElement_ITEM_NAMES();
	}
	return EElement_name_by_value.at(item);
}

template<>
EElement ITEM_BY_NAME(const std::string &name) {
	if (EElement_name_by_value.empty()) {
		init_EElement_ITEM_NAMES();
	}
	return EElement_value_by_name.at(name);
}

typedef std::map<ESpellType, std::string> ESpellType_name_by_value_t;
typedef std::map<const std::string, ESpellType> ESpellType_value_by_name_t;
ESpellType_name_by_value_t ESpellType_name_by_value;
ESpellType_value_by_name_t ESpellType_value_by_name;
void init_ESpellType_ITEM_NAMES() {
	ESpellType_value_by_name.clear();
	ESpellType_name_by_value.clear();

	ESpellType_name_by_value[ESpellType::kKnow] = "kKnow";
	ESpellType_name_by_value[ESpellType::kTemp] = "kTemp";
/*	ESpellType_name_by_value[ESpellType::kRunes] = "kRunes";
	ESpellType_name_by_value[ESpellType::kItemCast] = "kItemCast";
	ESpellType_name_by_value[ESpellType::kPotionCast] = "kPotionCast";
	ESpellType_name_by_value[ESpellType::kWandCast] = "kWandCast";*/

	for (const auto &i: ESpellType_name_by_value) {
		ESpellType_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ESpellType>(const ESpellType item) {
	if (ESpellType_name_by_value.empty()) {
		init_ESpellType_ITEM_NAMES();
	}
	return ESpellType_name_by_value.at(item);
}

template<>
ESpellType ITEM_BY_NAME(const std::string &name) {
	if (ESpellType_name_by_value.empty()) {
		init_ESpellType_ITEM_NAMES();
	}
	return ESpellType_value_by_name.at(name);
}

typedef std::map<EMagic, std::string> EMagic_name_by_value_t;
typedef std::map<const std::string, EMagic> EMagic_value_by_name_t;
EMagic_name_by_value_t EMagic_name_by_value;
EMagic_value_by_name_t EMagic_value_by_name;
void init_EMagic_ITEM_NAMES() {
	EMagic_value_by_name.clear();
	EMagic_name_by_value.clear();

	EMagic_name_by_value[EMagic::kMagNone] = "kMagNone";
	EMagic_name_by_value[EMagic::kMagDamage] = "kMagDamage";
	EMagic_name_by_value[EMagic::kMagAffects] = "kMagAffects";
	EMagic_name_by_value[EMagic::kMagUnaffects] = "kMagUnaffects";
	EMagic_name_by_value[EMagic::kMagPoints] = "kMagPoints";
	EMagic_name_by_value[EMagic::kMagAlterObjs] = "kMagAlterObjs";
	EMagic_name_by_value[EMagic::kMagGroups] = "kMagGroups";
	EMagic_name_by_value[EMagic::kMagMasses] = "kMagMasses";
	EMagic_name_by_value[EMagic::kMagAreas] = "kMagAreas";
	EMagic_name_by_value[EMagic::kMagSummons] = "kMagSummons";
	EMagic_name_by_value[EMagic::kMagCreations] = "kMagCreations";
	EMagic_name_by_value[EMagic::kMagManual] = "kMagManual";
	EMagic_name_by_value[EMagic::kMagWarcry] = "kMagWarcry";
	EMagic_name_by_value[EMagic::kMagNeedControl] = "kMagNeedControl";
	EMagic_name_by_value[EMagic::kNpcDamagePc] = "kNpcDamagePc";
	EMagic_name_by_value[EMagic::kNpcDamagePcMinhp] = "kNpcDamagePcMinhp";
	EMagic_name_by_value[EMagic::kNpcAffectPc] = "kNpcAffectPc";
	EMagic_name_by_value[EMagic::kNpcAffectPcCaster] = "kNpcAffectPcCaster";
	EMagic_name_by_value[EMagic::kNpcAffectNpc] = "kNpcAffectNpc";
	EMagic_name_by_value[EMagic::kNpcUnaffectNpc] = "kNpcUnaffectNpc";
	EMagic_name_by_value[EMagic::kNpcUnaffectNpcCaster] = "kNpcUnaffectNpcCaster";
	EMagic_name_by_value[EMagic::kNpcDummy] = "kNpcDummy";
	EMagic_name_by_value[EMagic::kMagRoom] = "kMagRoom";
	EMagic_name_by_value[EMagic::kMagCasterInroom] = "kMagCasterInroom";
	EMagic_name_by_value[EMagic::kMagCasterInworld] = "kMagCasterInworld";
	EMagic_name_by_value[EMagic::kMagCasterAnywhere] = "kMagCasterAnywhere";
	EMagic_name_by_value[EMagic::kMagCasterInworldDelay] = "kMagCasterInworldDelay";

	for (const auto &i: EMagic_name_by_value) {
		EMagic_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EMagic>(const EMagic item) {
	if (EMagic_name_by_value.empty()) {
		init_EMagic_ITEM_NAMES();
	}
	return EMagic_name_by_value.at(item);
}

template<>
EMagic ITEM_BY_NAME(const std::string &name) {
	if (EMagic_name_by_value.empty()) {
		init_EMagic_ITEM_NAMES();
	}
	return EMagic_value_by_name.at(name);
}

typedef std::map<ETarget, std::string> ETarget_name_by_value_t;
typedef std::map<const std::string, ETarget> ETarget_value_by_name_t;
ETarget_name_by_value_t ETarget_name_by_value;
ETarget_value_by_name_t ETarget_value_by_name;
void init_ETarget_ITEM_NAMES() {
	ETarget_value_by_name.clear();
	ETarget_name_by_value.clear();

	ETarget_name_by_value[ETarget::kTarNone] = "kTarNone";
	ETarget_name_by_value[ETarget::kTarIgnore] = "kTarIgnore";
	ETarget_name_by_value[ETarget::kTarCharRoom] = "kTarCharRoom";
	ETarget_name_by_value[ETarget::kTarCharWorld] = "kTarCharWorld";
	ETarget_name_by_value[ETarget::kTarFightSelf] = "kTarFightSelf";
	ETarget_name_by_value[ETarget::kTarFightVict] = "kTarFightVict";
	ETarget_name_by_value[ETarget::kTarSelfOnly] = "kTarSelfOnly";
	ETarget_name_by_value[ETarget::kTarNotSelf] = "kTarNotSelf";
	ETarget_name_by_value[ETarget::kTarObjInv] = "kTarObjInv";
	ETarget_name_by_value[ETarget::kTarObjRoom] = "kTarObjRoom";
	ETarget_name_by_value[ETarget::kTarObjWorld] = "kTarObjWorld";
	ETarget_name_by_value[ETarget::kTarObjEquip] = "kTarObjEquip";
	ETarget_name_by_value[ETarget::kTarRoomThis] = "kTarRoomThis";
	ETarget_name_by_value[ETarget::kTarRoomDir] = "kTarRoomDir";
	ETarget_name_by_value[ETarget::kTarRoomWorld] = "kTarRoomWorld";

	for (const auto &i: ETarget_name_by_value) {
		ETarget_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ETarget>(const ETarget item) {
	if (ETarget_name_by_value.empty()) {
		init_ETarget_ITEM_NAMES();
	}
	return ETarget_name_by_value.at(item);
}

template<>
ETarget ITEM_BY_NAME(const std::string &name) {
	if (ETarget_name_by_value.empty()) {
		init_ETarget_ITEM_NAMES();
	}
	return ETarget_value_by_name.at(name);
}

typedef std::map<ESpellMsg, std::string> ESpellMsg_name_by_value_t;
typedef std::map<const std::string, ESpellMsg> ESpellMsg_value_by_name_t;
ESpellMsg_name_by_value_t ESpellMsg_name_by_value;
ESpellMsg_value_by_name_t ESpellMsg_value_by_name;
void init_ESpellMsg_ITEM_NAMES() {
	ESpellMsg_value_by_name.clear();
	ESpellMsg_name_by_value.clear();

	ESpellMsg_name_by_value[ESpellMsg::kFailedForChar] = "kFailedForChar";
	ESpellMsg_name_by_value[ESpellMsg::kFailedForRoom] = "kFailedForRoom";
	ESpellMsg_name_by_value[ESpellMsg::kCastPhrasePoly] = "kCastPhrasePoly";
	ESpellMsg_name_by_value[ESpellMsg::kCastPhraseChrist] = "kCastPhraseChrist";
	ESpellMsg_name_by_value[ESpellMsg::kAreaForChar] = "kAreaForChar";
	ESpellMsg_name_by_value[ESpellMsg::kAreaForRoom] = "kAreaForRoom";
	ESpellMsg_name_by_value[ESpellMsg::kAreaForVict] = "kAreaForVict";
	ESpellMsg_name_by_value[ESpellMsg::kComponentUse] = "kComponentUse";
	ESpellMsg_name_by_value[ESpellMsg::kComponentMissing] = "kComponentMissing";
	ESpellMsg_name_by_value[ESpellMsg::kComponentExhausted] = "kComponentExhausted";
	ESpellMsg_name_by_value[ESpellMsg::kExpired] = "kExpired";
	ESpellMsg_name_by_value[ESpellMsg::kImposedForChar] = "kImposedForChar";
	ESpellMsg_name_by_value[ESpellMsg::kImposedForVict] = "kImposedForVict";
	ESpellMsg_name_by_value[ESpellMsg::kImposedForRoom] = "kImposedForRoom";

	for (const auto &i: ESpellMsg_name_by_value) {
		ESpellMsg_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<ESpellMsg>(const ESpellMsg item) {
	if (ESpellMsg_name_by_value.empty()) {
		init_ESpellMsg_ITEM_NAMES();
	}
	return ESpellMsg_name_by_value.at(item);
}

template<>
ESpellMsg ITEM_BY_NAME(const std::string &name) {
	if (ESpellMsg_name_by_value.empty()) {
		init_ESpellMsg_ITEM_NAMES();
	}
	return ESpellMsg_value_by_name.at(name);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
