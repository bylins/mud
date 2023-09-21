#include "feats_constants.h"

//#include <map>

typedef std::unordered_map<EFeat, std::string> EFeat_name_by_value_t;
typedef std::unordered_map<std::string, EFeat> EFeat_value_by_name_t;
EFeat_name_by_value_t EFeat_name_by_value;
EFeat_value_by_name_t EFeat_value_by_name;

void init_EFeat_ITEM_NAMES() {
	EFeat_name_by_value.clear();
	EFeat_value_by_name.clear();

	EFeat_name_by_value[EFeat::kUndefined] = "kUndefined";
	EFeat_name_by_value[EFeat::kBerserker] = "kBerserker";
	EFeat_name_by_value[EFeat::kParryArrow] = "kParryArrow";
	EFeat_name_by_value[EFeat::kBlindFight] = "kBlindFight";
	EFeat_name_by_value[EFeat::kImpregnable] = "kImpregnable";
	EFeat_name_by_value[EFeat::kNightVision] = "kNightVision";
	EFeat_name_by_value[EFeat::kDefender] = "kDefender";
	EFeat_name_by_value[EFeat::kDodger] = "kDodger";
	EFeat_name_by_value[EFeat::kLightWalk] = "kLightWalk";
	EFeat_name_by_value[EFeat::kWriggler] = "kWriggler";
	EFeat_name_by_value[EFeat::kSpellSubstitute] = "kSpellSubstitute";
	EFeat_name_by_value[EFeat::kPowerAttack] = "kPowerAttack";
	EFeat_name_by_value[EFeat::kWoodenSkin] = "kWoodenSkin";
	EFeat_name_by_value[EFeat::kIronSkin] = "kIronSkin";
	EFeat_name_by_value[EFeat::kConnoiseur] = "kConnoiseur";
	EFeat_name_by_value[EFeat::kExorcist] = "kExorcist";
	EFeat_name_by_value[EFeat::kHealer] = "kHealer";
	EFeat_name_by_value[EFeat::kLightingReflex] = "kLightingReflex";
	EFeat_name_by_value[EFeat::kDrunkard] = "kDrunkard";
	EFeat_name_by_value[EFeat::kPowerMagic] = "kPowerMagic";
	EFeat_name_by_value[EFeat::kEndurance] = "kEndurance";
	EFeat_name_by_value[EFeat::kGreatFortitude] = "kGreatFortitude";
	EFeat_name_by_value[EFeat::kFastRegen] = "kFastRegen";
	EFeat_name_by_value[EFeat::kStealthy] = "kStealthy";
	EFeat_name_by_value[EFeat::kWolfScent] = "kWolfScent";
	EFeat_name_by_value[EFeat::kSplendidHealth] = "kSplendidHealth";
	EFeat_name_by_value[EFeat::kTracker] = "kTracker";
	EFeat_name_by_value[EFeat::kWeaponFinesse] = "kWeaponFinesse";
	EFeat_name_by_value[EFeat::kCombatCasting] = "kCombatCasting";
	EFeat_name_by_value[EFeat::kPunchMaster] = "kPunchMaster";
	EFeat_name_by_value[EFeat::kClubsMaster] = "kClubsMaster";
	EFeat_name_by_value[EFeat::kAxesMaster] = "kAxesMaster";
	EFeat_name_by_value[EFeat::kLongsMaster] = "kLongsMaster";
	EFeat_name_by_value[EFeat::kShortsMaster] = "kShortsMaster";
	EFeat_name_by_value[EFeat::kNonstandartsMaster] = "kNonstandartsMaster";
	EFeat_name_by_value[EFeat::kTwohandsMaster] = "kTwohandsMaster";
	EFeat_name_by_value[EFeat::kPicksMaster] = "kPicksMaster";
	EFeat_name_by_value[EFeat::kSpadesMaster] = "kSpadesMaster";
	EFeat_name_by_value[EFeat::kBowsMaster] = "kBowsMaster";
	EFeat_name_by_value[EFeat::kForestPath] = "kForestPath";
	EFeat_name_by_value[EFeat::kMountainPath] = "kMountainPath";
	EFeat_name_by_value[EFeat::kLuckyGuy] = "kLuckyGuy";
	EFeat_name_by_value[EFeat::kWarriorSpirit] = "kWarriorSpirit";
	EFeat_name_by_value[EFeat::kReliableHealth] = "kReliableHealth";
	EFeat_name_by_value[EFeat::kExcellentMemory] = "kExcellentMemory";
	EFeat_name_by_value[EFeat::kAnimalDextery] = "kAnimalDextery";
	EFeat_name_by_value[EFeat::kLegibleWritting] = "kLegibleWritting";
	EFeat_name_by_value[EFeat::kIronMuscles] = "kIronMuscles";
	EFeat_name_by_value[EFeat::kMagicSign] = "kMagicSign";
	EFeat_name_by_value[EFeat::kGreatEndurance] = "kGreatEndurance";
	EFeat_name_by_value[EFeat::kBestDestiny] = "kBestDestiny";
	EFeat_name_by_value[EFeat::kHerbalist] = "kHerbalist";
	EFeat_name_by_value[EFeat::kJuggler] = "kJuggler";
	EFeat_name_by_value[EFeat::kNimbleFingers] = "kNimbleFingers";
	EFeat_name_by_value[EFeat::kGreatPowerAttack] = "kGreatPowerAttack";
	EFeat_name_by_value[EFeat::kStrongImmunity] = "kStrongImmunity";
	EFeat_name_by_value[EFeat::kMobility] = "kMobility";
	EFeat_name_by_value[EFeat::kNaturalStr] = "kNaturalStr";
	EFeat_name_by_value[EFeat::kNaturalDex] = "kNaturalDex";
	EFeat_name_by_value[EFeat::kNaturalInt] = "kNaturalInt";
	EFeat_name_by_value[EFeat::kNaturalWis] = "kNaturalWis";
	EFeat_name_by_value[EFeat::kNaturalCon] = "kNaturalCon";
	EFeat_name_by_value[EFeat::kNaturalCha] = "kNaturalCha";
	EFeat_name_by_value[EFeat::kMnemonicEnhancer] = "kMnemonicEnhancer";
	EFeat_name_by_value[EFeat::kMagneticPersonality] = "kMagneticPersonality";
	EFeat_name_by_value[EFeat::kDamrollBonus] = "kDamrollBonus";
	EFeat_name_by_value[EFeat::kHitrollBonus] = "kHitrollBonus";
	EFeat_name_by_value[EFeat::kInjure] = "kInjure";
	EFeat_name_by_value[EFeat::kPunchFocus] = "kPunchFocus";
	EFeat_name_by_value[EFeat::kClubsFocus] = "kClubsFocus";
	EFeat_name_by_value[EFeat::kAxesFocus] = "kAxesFocus";
	EFeat_name_by_value[EFeat::kLongsFocus] = "kLongsFocus";
	EFeat_name_by_value[EFeat::kShortsFocus] = "kShortsFocus";
	EFeat_name_by_value[EFeat::kNonstandartsFocus] = "kNonstandartsFocus";
	EFeat_name_by_value[EFeat::kTwohandsFocus] = "kTwohandsFocus";
	EFeat_name_by_value[EFeat::kPicksFocus] = "kPicksFocus";
	EFeat_name_by_value[EFeat::kSpadesFocus] = "kSpadesFocus";
	EFeat_name_by_value[EFeat::kBowsFocus] = "kBowsFocus";
	EFeat_name_by_value[EFeat::kAimingAttack] = "kAimingAttack";
	EFeat_name_by_value[EFeat::kGreatAimingAttack] = "kGreatAimingAttack";
	EFeat_name_by_value[EFeat::kDoubleShot] = "kDoubleShot";
	EFeat_name_by_value[EFeat::kPorter] = "kPorter";
	EFeat_name_by_value[EFeat::kSecretRunes] = "kSecretRunes";
	EFeat_name_by_value[EFeat::kToFitItem] = "kToFitItem";
	EFeat_name_by_value[EFeat::kToFitClotches] = "kToFitClotches";
	EFeat_name_by_value[EFeat::kStrengthConcentration] = "kStrengthConcentration";
	EFeat_name_by_value[EFeat::kDarkReading] = "kDarkReading";
	EFeat_name_by_value[EFeat::kSpellCapabler] = "kSpellCapabler";
	EFeat_name_by_value[EFeat::kWearingLightArmor] = "kWearingLightArmor";
	EFeat_name_by_value[EFeat::kWearingMediumArmor] = "kWearingMediumArmor";
	EFeat_name_by_value[EFeat::kWearingHeavyArmor] = "kWearingHeavyArmor";
	EFeat_name_by_value[EFeat::kGemsInlay] = "kGemsInlay";
	EFeat_name_by_value[EFeat::kWarriorStrength] = "kWarriorStrength";
	EFeat_name_by_value[EFeat::kRelocate] = "kRelocate";
	EFeat_name_by_value[EFeat::kSilverTongue] = "kSilverTongue";
	EFeat_name_by_value[EFeat::kBully] = "kBully";
	EFeat_name_by_value[EFeat::kThieveStrike] = "kThieveStrike";
	EFeat_name_by_value[EFeat::kJeweller] = "kJeweller";
	EFeat_name_by_value[EFeat::kSkilledTrader] = "kSkilledTrader";
	EFeat_name_by_value[EFeat::kZombieDrover] = "kZombieDrover";
	EFeat_name_by_value[EFeat::kEmployer] = "kEmployer";
	EFeat_name_by_value[EFeat::kMagicUser] = "kMagicUser";
	EFeat_name_by_value[EFeat::kGoldTongue] = "kGoldTongue";
	EFeat_name_by_value[EFeat::kCalmness] = "kCalmness";
	EFeat_name_by_value[EFeat::kRetreat] = "kRetreat";
	EFeat_name_by_value[EFeat::kShadowStrike] = "kShadowStrike";
	EFeat_name_by_value[EFeat::kThrifty] = "kThrifty";
	EFeat_name_by_value[EFeat::kCynic] = "kCynic";
	EFeat_name_by_value[EFeat::kPartner] = "kPartner";
	EFeat_name_by_value[EFeat::kFavorOfDarkness] = "kFavorOfDarkness";
	EFeat_name_by_value[EFeat::kFuryOfDarkness] = "kFuryOfDarkness";
	EFeat_name_by_value[EFeat::kRegenOfDarkness] = "kRegenOfDarkness";
	EFeat_name_by_value[EFeat::kSoulLink] = "kSoulLink";
	EFeat_name_by_value[EFeat::kStrongClutch] = "kStrongClutch";
	EFeat_name_by_value[EFeat::kMagicArrows] = "kMagicArrows";
	EFeat_name_by_value[EFeat::kSoulsCollector] = "kSoulsCollector";
	EFeat_name_by_value[EFeat::kDarkPact] = "kDarkPact";
	EFeat_name_by_value[EFeat::kCorruption] = "kCorruption";
	EFeat_name_by_value[EFeat::kHarvestOfLife] = "kHarvestOfLife";
	EFeat_name_by_value[EFeat::kLoyalAssist] = "kLoyalAssist";
	EFeat_name_by_value[EFeat::kHauntingSpirit] = "kHauntingSpirit";
	EFeat_name_by_value[EFeat::kSnakeRage] = "kSnakeRage";
	EFeat_name_by_value[EFeat::kElderTaskmaster] = "kElderTaskmaster";
	EFeat_name_by_value[EFeat::kLordOfUndeads] = "kLordOfUndeads";
	EFeat_name_by_value[EFeat::kWarlock] = "kWarlock";
	EFeat_name_by_value[EFeat::kElderPriest] = "kElderPriest";
	EFeat_name_by_value[EFeat::kHighLich] = "kHighLich";
	EFeat_name_by_value[EFeat::kDarkRitual] = "kDarkRitual";
	EFeat_name_by_value[EFeat::kTeamsterOfUndead] = "kTeamsterOfUndead";
	EFeat_name_by_value[EFeat::kScirmisher] = "kScirmisher";
	EFeat_name_by_value[EFeat::kTactician] = "kTactician";
	EFeat_name_by_value[EFeat::kLiveShield] = "kLiveShield";
	EFeat_name_by_value[EFeat::kSerratedBlade] = "kSerratedBlade";
	EFeat_name_by_value[EFeat::kEvasion] = "kEvasion";
	EFeat_name_by_value[EFeat::kCutting] = "kCutting";
	EFeat_name_by_value[EFeat::kFInesseShot] = "kFInesseShot";
	EFeat_name_by_value[EFeat::kObjectEnchanter] = "kObjectEnchanter";
	EFeat_name_by_value[EFeat::kDeftShooter] = "kDeftShooter";
	EFeat_name_by_value[EFeat::kMagicShooter] = "kMagicShooter";
	EFeat_name_by_value[EFeat::kShadowThrower] = "kShadowThrower";
	EFeat_name_by_value[EFeat::kShadowDagger] = "kShadowDagger";
	EFeat_name_by_value[EFeat::kShadowSpear] = "kShadowSpear";
	EFeat_name_by_value[EFeat::kShadowClub] = "kShadowClub";
	EFeat_name_by_value[EFeat::kDoubleThrower] = "kDoubleThrower";
	EFeat_name_by_value[EFeat::kTripleThrower] = "kTripleThrower";
	EFeat_name_by_value[EFeat::kPowerThrow] = "kPowerThrow";
	EFeat_name_by_value[EFeat::kDeadlyThrow] = "kDeadlyThrow";
	EFeat_name_by_value[EFeat::kMultipleCast] = "kMultipleCast";
	EFeat_name_by_value[EFeat::kMagicalShield] = "kMagicalShield";
	EFeat_name_by_value[EFeat::kAnimalMaster] = "kAnimalMaster";
	EFeat_name_by_value[EFeat::kSlashMaster] = "kSlashMaster";
	EFeat_name_by_value[EFeat::kPhysicians] = "kPhysicians";

	for (const auto &i : EFeat_name_by_value) {
		EFeat_value_by_name[i.second] = i.first;
	}
}

template<>
const std::string &NAME_BY_ITEM<EFeat>(const EFeat item) {
	if (EFeat_name_by_value.empty()) {
		init_EFeat_ITEM_NAMES();
	}
	return EFeat_name_by_value.at(item);
}

template<>
EFeat ITEM_BY_NAME<EFeat>(const std::string &name) {
	if (EFeat_name_by_value.empty()) {
		init_EFeat_ITEM_NAMES();
	}
	return EFeat_value_by_name.at(name);
}

EFeat& operator++(EFeat &f) {
	f =  static_cast<EFeat>(to_underlying(f) + 1);
	return f;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :

