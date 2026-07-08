// Part of Bylins http://www.mud.ru
// SQLite world data source implementation

#ifdef HAVE_SQLITE

#include "sqlite_world_data_source.h"
#include "utils/utils_encoding.h"
#include "db.h"
#include "obj_prototypes.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/utils_string.h"
#include "engine/entities/zone.h"
#include "engine/entities/room_data.h"
#include "engine/entities/char_data.h"
#include "engine/entities/entities_constants.h"
#include "engine/scripting/dg_scripts.h"
#include "engine/db/description.h"
#include "global_objects.h"
#include "engine/structs/extra_description.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/dead_load.h"
#include "engine/scripting/dg_olc.h"
#include "gameplay/affects/affect_contants.h"
#include "gameplay/skills/skills.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <map>
#include <unordered_map>

// External declarations
extern ZoneTable &zone_table;
extern IndexData **trig_index;
extern int top_of_trigt;
extern Rooms &world;
extern RoomRnum top_of_world;
extern IndexData *mob_index;
extern MobRnum top_of_mobt;
extern CharData *mob_proto;

namespace world_loader
{

// ============================================================================
// Flag name to value mappings
// ============================================================================

// Room flags mapping
static std::unordered_map<std::string, Bitvector> room_flag_map = {
	{"kDarked", ERoomFlag::kDarked},
	{"kDeathTrap", ERoomFlag::kDeathTrap},
	{"kNoMob", ERoomFlag::kNoEntryMob},
	{"kNoEntryMob", ERoomFlag::kNoEntryMob},
	{"kIndoors", ERoomFlag::kIndoors},
	{"kPeaceful", ERoomFlag::kPeaceful},
	{"kPeaceFul", ERoomFlag::kPeaceful},
	{"kSoundproof", ERoomFlag::kSoundproof},
	{"kNoTrack", ERoomFlag::kNoTrack},
	{"kNoMagic", ERoomFlag::kNoMagic},
	{"kTunnel", ERoomFlag::kTunnel},
	{"kNoTeleportIn", ERoomFlag::kNoTeleportIn},
	{"kGodRoom", ERoomFlag::kGodsRoom},
	{"kGodsRoom", ERoomFlag::kGodsRoom},
	{"kHouse", ERoomFlag::kHouse},
	{"kHouseCrash", ERoomFlag::kHouseCrash},
	{"kHouseEntry", ERoomFlag::kHouseEntry},
	{"kBfsMark", ERoomFlag::kBfsMark},
	{"kForMages", ERoomFlag::kForMages},
	{"kForSorcerers", ERoomFlag::kForSorcerers},
	{"kForThieves", ERoomFlag::kForThieves},
	{"kForWarriors", ERoomFlag::kForWarriors},
	{"kForAssasines", ERoomFlag::kForAssasines},
	{"kForGuards", ERoomFlag::kForGuards},
	{"kForPaladines", ERoomFlag::kForPaladines},
	{"kForRangers", ERoomFlag::kForRangers},
	{"kForPoly", ERoomFlag::kForPoly},
	{"kForMono", ERoomFlag::kForMono},
	{"kForge", ERoomFlag::kForge},
	{"kForMerchants", ERoomFlag::kForMerchants},
	{"kForMaguses", ERoomFlag::kForMaguses},
	{"kArena", ERoomFlag::kArena},
	{"kNoSummonOut", ERoomFlag::kNoSummonOut},
	{"kNoSummon", ERoomFlag::kNoSummonOut},
	{"kNoTeleportOut", ERoomFlag::kNoTeleportOut},
	{"kNohorse", ERoomFlag::kNohorse},
	{"kNoWeather", ERoomFlag::kNoWeather},
	{"kSlowDeathTrap", ERoomFlag::kSlowDeathTrap},
	{"kIceTrap", ERoomFlag::kIceTrap},
	{"kNoRelocateIn", ERoomFlag::kNoRelocateIn},
	{"kTribune", ERoomFlag::kTribune},
	{"kArenaSend", ERoomFlag::kArenaSend},
	{"kNoBattle", ERoomFlag::kNoBattle},
	{"kAlwaysLit", ERoomFlag::kAlwaysLit},
	{"kMoMapper", ERoomFlag::kMoMapper},
	{"kNoItem", ERoomFlag::kNoItem},
	{"kDominationArena", ERoomFlag::kDominationArena},
	// Additional aliases from database
	{"kNoPk", ERoomFlag::kPeaceful},
	{"kExchangeRoom", ERoomFlag::kHouse},
	{"kNoBirdArrival", ERoomFlag::kNoTeleportIn},
	{"kMoOnlyRoom", ERoomFlag::kGodsRoom},
	{"kDeathIceTrap", ERoomFlag::kIceTrap},
	{"kForestTrap", ERoomFlag::kSlowDeathTrap},
	{"kArenaTrap", ERoomFlag::kArena},
	{"kNoArmor", ERoomFlag::kForge},
	{"kAtrium", ERoomFlag::kHouseEntry},
	{"kAutoQuest", ERoomFlag::kBfsMark},
	{"kDecayedDeathTrap", ERoomFlag::kDeathTrap},
	{"kHoleInTheSky", ERoomFlag::kNoTeleportIn},
};

// Mob action flags mapping
static std::unordered_map<std::string, Bitvector> mob_action_flag_map = {
	// Complete EMobFlag set generated from the enum -- the previous hand list
	// only covered plane 0, so plane 1+ flags (kSwimming/kFlying/... breath/
	// appears/season flags) were silently dropped on save.
	{"kSpec", EMobFlag::kSpec},
	{"kSentinel", EMobFlag::kSentinel},
	{"kScavenger", EMobFlag::kScavenger},
	{"kNpc", EMobFlag::kNpc},
	{"kAware", EMobFlag::kAware},
	{"kAgressive", EMobFlag::kAgressive},
	{"kStayZone", EMobFlag::kStayZone},
	{"kWimpy", EMobFlag::kWimpy},
	{"kAgressiveDay", EMobFlag::kAgressiveDay},
	{"kAggressiveNight", EMobFlag::kAggressiveNight},
	{"kAgressiveFullmoon", EMobFlag::kAgressiveFullmoon},
	{"kMemory", EMobFlag::kMemory},
	{"kHelper", EMobFlag::kHelper},
	{"kNoCharm", EMobFlag::kNoCharm},
	{"kNoSummon", EMobFlag::kNoSummon},
	{"kNoSleep", EMobFlag::kNoSleep},
	{"kNoBash", EMobFlag::kNoBash},
	{"kNoBlind", EMobFlag::kNoBlind},
	{"kMounting", EMobFlag::kMounting},
	{"kNoHold", EMobFlag::kNoHold},
	{"kNoSilence", EMobFlag::kNoSilence},
	{"kAgressiveMono", EMobFlag::kAgressiveMono},
	{"kAgressivePoly", EMobFlag::kAgressivePoly},
	{"kNoFear", EMobFlag::kNoFear},
	{"kNoGroup", EMobFlag::kNoGroup},
	{"kCorpse", EMobFlag::kCorpse},
	{"kLooter", EMobFlag::kLooter},
	{"kProtect", EMobFlag::kProtect},
	{"kMobDeleted", EMobFlag::kMobDeleted},
	{"kMobFreed", EMobFlag::kMobFreed},
	{"kSwimming", EMobFlag::kSwimming},
	{"kFlying", EMobFlag::kFlying},
	{"kOnlySwimming", EMobFlag::kOnlySwimming},
	{"kAgressiveWinter", EMobFlag::kAgressiveWinter},
	{"kAgressiveSpring", EMobFlag::kAgressiveSpring},
	{"kAgressiveSummer", EMobFlag::kAgressiveSummer},
	{"kAgressiveAutumn", EMobFlag::kAgressiveAutumn},
	{"kAppearsDay", EMobFlag::kAppearsDay},
	{"kAppearsNight", EMobFlag::kAppearsNight},
	{"kAppearsFullmoon", EMobFlag::kAppearsFullmoon},
	{"kAppearsWinter", EMobFlag::kAppearsWinter},
	{"kAppearsSpring", EMobFlag::kAppearsSpring},
	{"kAppearsSummer", EMobFlag::kAppearsSummer},
	{"kAppearsAutumn", EMobFlag::kAppearsAutumn},
	{"kNoFight", EMobFlag::kNoFight},
	{"kDecreaseAttack", EMobFlag::kDecreaseAttack},
	{"kHorde", EMobFlag::kHorde},
	{"kClone", EMobFlag::kClone},
	{"kNotKillPunctual", EMobFlag::kNotKillPunctual},
	{"kNoUndercut", EMobFlag::kNoUndercut},
	{"kTutelar", EMobFlag::kTutelar},
	{"kCityGuardian", EMobFlag::kCityGuardian},
	{"kIgnoreForbidden", EMobFlag::kIgnoreForbidden},
	{"kNoBattleExp", EMobFlag::kNoBattleExp},
	{"kNoHammer", EMobFlag::kNoHammer},
	{"kMentalShadow", EMobFlag::kMentalShadow},
	{"kCompanion", EMobFlag::kCompanion},
	{"kUndead", EMobFlag::kUndead},
	{"kSummoned", EMobFlag::kSummoned},
	{"kFireBreath", EMobFlag::kFireBreath},
	{"kGasBreath", EMobFlag::kGasBreath},
	{"kFrostBreath", EMobFlag::kFrostBreath},
	{"kAcidBreath", EMobFlag::kAcidBreath},
	{"kLightingBreath", EMobFlag::kLightingBreath},
	{"kNoSkillTrain", EMobFlag::kNoSkillTrain},
	{"kNoRest", EMobFlag::kNoRest},
	{"kAreaAttack", EMobFlag::kAreaAttack},
	{"kNoOverwhelm", EMobFlag::kNoOverwhelm},
	{"kNoHelp", EMobFlag::kNoHelp},
	{"kOpensDoor", EMobFlag::kOpensDoor},
	{"kIgnoresNoMob", EMobFlag::kIgnoresNoMob},
	{"kIgnoresPeaceRoom", EMobFlag::kIgnoresPeaceRoom},
	{"kResurrected", EMobFlag::kResurrected},
	{"kNoResurrection", EMobFlag::kNoResurrection},
	{"kMobAwake", EMobFlag::kMobAwake},
	{"kIgnoresFormation", EMobFlag::kIgnoresFormation},
};

// Mob affect flags mapping (EAffect values)
static std::unordered_map<std::string, Bitvector> mob_affect_flag_map = {
	// Generated from the full EAffect enum -- the hand list was plane-0 only,
	// so plane-1+ affect flags were dropped on save.
	{"kUndefined", to_underlying(EAffect::kUndefined)},
	{"kBlind", to_underlying(EAffect::kBlind)},
	{"kInvisible", to_underlying(EAffect::kInvisible)},
	{"kDetectAlign", to_underlying(EAffect::kDetectAlign)},
	{"kDetectInvisible", to_underlying(EAffect::kDetectInvisible)},
	{"kDetectMagic", to_underlying(EAffect::kDetectMagic)},
	{"kDetectLife", to_underlying(EAffect::kDetectLife)},
	{"kWaterWalk", to_underlying(EAffect::kWaterWalk)},
	{"kSanctuary", to_underlying(EAffect::kSanctuary)},
	{"kGroup", to_underlying(EAffect::kGroup)},
	{"kCurse", to_underlying(EAffect::kCurse)},
	{"kInfravision", to_underlying(EAffect::kInfravision)},
	{"kPoisoned", to_underlying(EAffect::kPoisoned)},
	{"kProtectFromDark", to_underlying(EAffect::kProtectFromDark)},
	{"kProtectFromMind", to_underlying(EAffect::kProtectFromMind)},
	{"kSleep", to_underlying(EAffect::kSleep)},
	{"kNoTrack", to_underlying(EAffect::kNoTrack)},
	{"kTethered", to_underlying(EAffect::kTethered)},
	{"kBless", to_underlying(EAffect::kBless)},
	{"kSneak", to_underlying(EAffect::kSneak)},
	{"kHide", to_underlying(EAffect::kHide)},
	{"kCourage", to_underlying(EAffect::kCourage)},
	{"kCharmed", to_underlying(EAffect::kCharmed)},
	{"kHold", to_underlying(EAffect::kHold)},
	{"kFly", to_underlying(EAffect::kFly)},
	{"kSilence", to_underlying(EAffect::kSilence)},
	{"kAwarness", to_underlying(EAffect::kAwarness)},
	{"kBlink", to_underlying(EAffect::kBlink)},
	{"kHorse", to_underlying(EAffect::kHorse)},
	{"kNoFlee", to_underlying(EAffect::kNoFlee)},
	{"kSingleLight", to_underlying(EAffect::kSingleLight)},
	{"kHolyLight", to_underlying(EAffect::kHolyLight)},
	{"kHolyDark", to_underlying(EAffect::kHolyDark)},
	{"kDetectPoison", to_underlying(EAffect::kDetectPoison)},
	{"kDrunked", to_underlying(EAffect::kDrunked)},
	{"kAbstinent", to_underlying(EAffect::kAbstinent)},
	{"kStopRight", to_underlying(EAffect::kStopRight)},
	{"kStopLeft", to_underlying(EAffect::kStopLeft)},
	{"kStopFight", to_underlying(EAffect::kStopFight)},
	{"kHaemorrhage", to_underlying(EAffect::kHaemorrhage)},
	{"kDisguise", to_underlying(EAffect::kDisguise)},
	{"kWaterBreath", to_underlying(EAffect::kWaterBreath)},
	{"kSlow", to_underlying(EAffect::kSlow)},
	{"kHaste", to_underlying(EAffect::kHaste)},
	{"kGodsShield", to_underlying(EAffect::kGodsShield)},
	{"kAirShield", to_underlying(EAffect::kAirShield)},
	{"kFireShield", to_underlying(EAffect::kFireShield)},
	{"kIceShield", to_underlying(EAffect::kIceShield)},
	{"kMagicGlass", to_underlying(EAffect::kMagicGlass)},
	{"kStairs", to_underlying(EAffect::kStairs)},
	{"kStoneHands", to_underlying(EAffect::kStoneHands)},
	{"kPrismaticAura", to_underlying(EAffect::kPrismaticAura)},
	{"kHelper", to_underlying(EAffect::kHelper)},
	{"kForcesOfEvil", to_underlying(EAffect::kForcesOfEvil)},
	{"kAirAura", to_underlying(EAffect::kAirAura)},
	{"kFireAura", to_underlying(EAffect::kFireAura)},
	{"kIceAura", to_underlying(EAffect::kIceAura)},
	{"kDeafness", to_underlying(EAffect::kDeafness)},
	{"kCrying", to_underlying(EAffect::kCrying)},
	{"kPeaceful", to_underlying(EAffect::kPeaceful)},
	{"kMagicStopFight", to_underlying(EAffect::kMagicStopFight)},
	{"kBerserk", to_underlying(EAffect::kBerserk)},
	{"kLightWalk", to_underlying(EAffect::kLightWalk)},
	{"kBrokenChains", to_underlying(EAffect::kBrokenChains)},
	{"kCloudOfArrows", to_underlying(EAffect::kCloudOfArrows)},
	{"kShadowCloak", to_underlying(EAffect::kShadowCloak)},
	{"kGlitterDust", to_underlying(EAffect::kGlitterDust)},
	{"kAffright", to_underlying(EAffect::kAffright)},
	{"kScopolaPoison", to_underlying(EAffect::kScopolaPoison)},
	{"kDaturaPoison", to_underlying(EAffect::kDaturaPoison)},
	{"kSkillReduce", to_underlying(EAffect::kSkillReduce)},
	{"kNoBattleSwitch", to_underlying(EAffect::kNoBattleSwitch)},
	{"kBelenaPoison", to_underlying(EAffect::kBelenaPoison)},
	{"kNoTeleport", to_underlying(EAffect::kNoTeleport)},
	{"kCombatLuck", to_underlying(EAffect::kCombatLuck)},
	{"kBandage", to_underlying(EAffect::kBandage)},
	{"kCannotBeBandaged", to_underlying(EAffect::kCannotBeBandaged)},
	{"kMorphing", to_underlying(EAffect::kMorphing)},
	{"kStrangled", to_underlying(EAffect::kStrangled)},
	{"kMemorizeSpells", to_underlying(EAffect::kMemorizeSpells)},
	{"kNoobRegen", to_underlying(EAffect::kNoobRegen)},
	{"kVampirism", to_underlying(EAffect::kVampirism)},
	{"kLacerations", to_underlying(EAffect::kLacerations)},
	{"kCommander", to_underlying(EAffect::kCommander)},
	{"kEarthAura", to_underlying(EAffect::kEarthAura)},
	{"kCloudly", to_underlying(EAffect::kCloudly)},
	{"kConfused", to_underlying(EAffect::kConfused)},
	{"kNoCharge", to_underlying(EAffect::kNoCharge)},
	{"kInjuredLimb", to_underlying(EAffect::kInjuredLimb)},
	{"kFrenzy", to_underlying(EAffect::kFrenzy)},
};

// Object extra flags mapping
static std::unordered_map<std::string, EObjFlag> obj_extra_flag_map = {
	{"kGlow", EObjFlag::kGlow},
	{"kHum", EObjFlag::kHum},
	{"kNorent", EObjFlag::kNorent},
	{"kNodonate", EObjFlag::kNodonate},
	{"kNoinvis", EObjFlag::kNoinvis},
	{"kInvisible", EObjFlag::kInvisible},
	{"kMagic", EObjFlag::kMagic},
	{"kNodrop", EObjFlag::kNodrop},
	{"kBless", EObjFlag::kBless},
	{"kNosell", EObjFlag::kNosell},
	{"kDecay", EObjFlag::kDecay},
	{"kZonedecay", EObjFlag::kZonedecay},
	{"kNodisarm", EObjFlag::kNodisarm},
	{"kNodecay", EObjFlag::kNodecay},
	{"kPoisoned", EObjFlag::kPoisoned},
	{"kSharpen", EObjFlag::kSharpen},
	{"kArmored", EObjFlag::kArmored},
	{"kAppearsDay", EObjFlag::kAppearsDay},
	{"kAppearsNight", EObjFlag::kAppearsNight},
	{"kAppearsFullmoon", EObjFlag::kAppearsFullmoon},
	{"kAppearsWinter", EObjFlag::kAppearsWinter},
	{"kAppearsSpring", EObjFlag::kAppearsSpring},
	{"kAppearsSummer", EObjFlag::kAppearsSummer},
	{"kAppearsAutumn", EObjFlag::kAppearsAutumn},
	{"kSwimming", EObjFlag::kSwimming},
	{"kFlying", EObjFlag::kFlying},
	{"kThrowing", EObjFlag::kThrowing},
	{"kTicktimer", EObjFlag::kTicktimer},
	{"kFire", EObjFlag::kFire},
	{"kRepopDecay", EObjFlag::kRepopDecay},
	{"kNolocate", EObjFlag::kNolocate},
	{"kTimedLvl", EObjFlag::kTimedLvl},
	{"kNoalter", EObjFlag::kNoalter},
	{"kHasOneSlot", EObjFlag::kHasOneSlot},
	{"kHasTwoSlots", EObjFlag::kHasTwoSlots},
	{"kHasThreeSlots", EObjFlag::kHasThreeSlots},
	{"kSetItem", EObjFlag::kSetItem},
	{"kNofail", EObjFlag::kNofail},
	{"kNamed", EObjFlag::kNamed},
	{"kBloody", EObjFlag::kBloody},
	{"kQuestItem", EObjFlag::kQuestItem},
	{"k2inlaid", EObjFlag::k2inlaid},
	{"k3inlaid", EObjFlag::k3inlaid},
	{"kNopour", EObjFlag::kNopour},
	{"kUnique", EObjFlag::kUnique},
	{"kTransformed", EObjFlag::kTransformed},
	{"kNoRentTimer", EObjFlag::kNoRentTimer},
	{"kLimitedTimer", EObjFlag::kLimitedTimer},
	{"kBindOnPurchase", EObjFlag::kBindOnPurchase},
	{"kNotOneInClanChest", EObjFlag::kNotOneInClanChest},
};

// Object wear flags mapping
static std::unordered_map<std::string, EWearFlag> obj_wear_flag_map = {
	{"kTake", EWearFlag::kTake},
	{"kFinger", EWearFlag::kFinger},
	{"kNeck", EWearFlag::kNeck},
	{"kBody", EWearFlag::kBody},
	{"kHead", EWearFlag::kHead},
	{"kLegs", EWearFlag::kLegs},
	{"kFeet", EWearFlag::kFeet},
	{"kHands", EWearFlag::kHands},
	{"kArms", EWearFlag::kArms},
	{"kShield", EWearFlag::kShield},
	{"kShoulders", EWearFlag::kShoulders},
	{"kWaist", EWearFlag::kWaist},
	{"kWrist", EWearFlag::kWrist},
	{"kWield", EWearFlag::kWield},
	{"kHold", EWearFlag::kHold},
	{"kBoth", EWearFlag::kBoth},
	{"kQuiver", EWearFlag::kQuiver},
};

// Object affect flags mapping (EWeaponAffect values)
static std::unordered_map<std::string, EWeaponAffect> obj_affect_flag_map = {
	{"kBlindness", EWeaponAffect::kBlindness},
	{"kInvisibility", EWeaponAffect::kInvisibility},
	{"kDetectAlign", EWeaponAffect::kDetectAlign},
	{"kDetectInvisibility", EWeaponAffect::kDetectInvisibility},
	{"kDetectMagic", EWeaponAffect::kDetectMagic},
	{"kDetectLife", EWeaponAffect::kDetectLife},
	{"kWaterWalk", EWeaponAffect::kWaterWalk},
	{"kSanctuary", EWeaponAffect::kSanctuary},
	{"kCurse", EWeaponAffect::kCurse},
	{"kInfravision", EWeaponAffect::kInfravision},
	{"kPoison", EWeaponAffect::kPoison},
	{"kProtectFromDark", EWeaponAffect::kProtectFromDark},
	{"kProtectFromMind", EWeaponAffect::kProtectFromMind},
	{"kSleep", EWeaponAffect::kSleep},
	{"kNoTrack", EWeaponAffect::kNoTrack},
	{"kBless", EWeaponAffect::kBless},
	{"kSneak", EWeaponAffect::kSneak},
	{"kHide", EWeaponAffect::kHide},
	{"kHold", EWeaponAffect::kHold},
	{"kFly", EWeaponAffect::kFly},
	{"kSilence", EWeaponAffect::kSilence},
	{"kAwareness", EWeaponAffect::kAwareness},
	{"kBlink", EWeaponAffect::kBlink},
	{"kNoFlee", EWeaponAffect::kNoFlee},
	{"kSingleLight", EWeaponAffect::kSingleLight},
	{"kHolyLight", EWeaponAffect::kHolyLight},
	{"kHolyDark", EWeaponAffect::kHolyDark},
	{"kDetectPoison", EWeaponAffect::kDetectPoison},
	{"kSlow", EWeaponAffect::kSlow},
	{"kHaste", EWeaponAffect::kHaste},
	{"kWaterBreath", EWeaponAffect::kWaterBreath},
	{"kHaemorrhage", EWeaponAffect::kHaemorrhage},
	{"kDisguising", EWeaponAffect::kDisguising},
	{"kShield", EWeaponAffect::kShield},
	{"kAirShield", EWeaponAffect::kAirShield},
	{"kFireShield", EWeaponAffect::kFireShield},
	{"kIceShield", EWeaponAffect::kIceShield},
	{"kMagicGlass", EWeaponAffect::kMagicGlass},
	{"kStoneHand", EWeaponAffect::kStoneHand},
	{"kPrismaticAura", EWeaponAffect::kPrismaticAura},
	{"kAirAura", EWeaponAffect::kAirAura},
	{"kFireAura", EWeaponAffect::kFireAura},
	{"kIceAura", EWeaponAffect::kIceAura},
	{"kDeafness", EWeaponAffect::kDeafness},
	{"kComamnder", EWeaponAffect::kComamnder},
	{"kEarthAura", EWeaponAffect::kEarthAura},
	{"kCloudly", EWeaponAffect::kCloudly},
};

// Object anti flags mapping (EAntiFlag values)
static std::unordered_map<std::string, EAntiFlag> obj_anti_flag_map = {
	{"kMono", EAntiFlag::kMono},
	{"kPoly", EAntiFlag::kPoly},
	{"kNeutral", EAntiFlag::kNeutral},
	{"kMage", EAntiFlag::kMage},
	{"kSorcerer", EAntiFlag::kSorcerer},
	{"kThief", EAntiFlag::kThief},
	{"kWarrior", EAntiFlag::kWarrior},
	{"kAssasine", EAntiFlag::kAssasine},
	{"kGuard", EAntiFlag::kGuard},
	{"kPaladine", EAntiFlag::kPaladine},
	{"kRanger", EAntiFlag::kRanger},
	{"kVigilant", EAntiFlag::kVigilant},
	{"kMerchant", EAntiFlag::kMerchant},
	{"kMagus", EAntiFlag::kMagus},
	{"kConjurer", EAntiFlag::kConjurer},
	{"kCharmer", EAntiFlag::kCharmer},
	{"kWizard", EAntiFlag::kWizard},
	{"kNecromancer", EAntiFlag::kNecromancer},
	{"kFighter", EAntiFlag::kFighter},
	{"kKiller", EAntiFlag::kKiller},
	{"kColored", EAntiFlag::kColored},
	{"kBattle", EAntiFlag::kBattle},
	{"kMale", EAntiFlag::kMale},
	{"kFemale", EAntiFlag::kFemale},
	{"kCharmice", EAntiFlag::kCharmice},
	{"kNoPkClan", EAntiFlag::kNoPkClan},
};

// Object no flags mapping (ENoFlag values)
static std::unordered_map<std::string, ENoFlag> obj_no_flag_map = {
	{"kMono", ENoFlag::kMono},
	{"kPoly", ENoFlag::kPoly},
	{"kNeutral", ENoFlag::kNeutral},
	{"kMage", ENoFlag::kMage},
	{"kSorcerer", ENoFlag::kSorcerer},
	{"kThief", ENoFlag::kThief},
	{"kWarrior", ENoFlag::kWarrior},
	{"kAssasine", ENoFlag::kAssasine},
	{"kGuard", ENoFlag::kGuard},
	{"kPaladine", ENoFlag::kPaladine},
	{"kRanger", ENoFlag::kRanger},
	{"kVigilant", ENoFlag::kVigilant},
	{"kMerchant", ENoFlag::kMerchant},
	{"kMagus", ENoFlag::kMagus},
	{"kConjurer", ENoFlag::kConjurer},
	{"kCharmer", ENoFlag::kCharmer},
	{"kWizard", ENoFlag::kWizard},
	{"kNecromancer", ENoFlag::kNecromancer},
	{"kFighter", ENoFlag::kFighter},
	{"kKiller", ENoFlag::kKiller},
	{"kColored", ENoFlag::kColored},
	{"kBattle", ENoFlag::kBattle},
	{"kMale", ENoFlag::kMale},
	{"kFemale", ENoFlag::kFemale},
	{"kCharmice", ENoFlag::kCharmice},
};

// Position mapping
static std::unordered_map<std::string, int> position_map = {
	{"kDead", static_cast<int>(EPosition::kDead)},
	{"kPerish", static_cast<int>(EPosition::kPerish)},
	{"kMortallyw", static_cast<int>(EPosition::kPerish)},
	{"kIncap", static_cast<int>(EPosition::kIncap)},
	{"kStun", static_cast<int>(EPosition::kStun)},
	{"kSleep", static_cast<int>(EPosition::kSleep)},
	{"kRest", static_cast<int>(EPosition::kRest)},
	{"kSit", static_cast<int>(EPosition::kSit)},
	{"kFight", static_cast<int>(EPosition::kFight)},
	{"kStanding", static_cast<int>(EPosition::kStand)},
	{"kStand", static_cast<int>(EPosition::kStand)},
};

// Gender mapping
static std::unordered_map<std::string, int> gender_map = {
	{"kMale", static_cast<int>(EGender::kMale)},
	{"kFemale", static_cast<int>(EGender::kFemale)},
	{"kNeutral", static_cast<int>(EGender::kNeutral)},
	{"kPoly", static_cast<int>(EGender::kPoly)},
};
// Helper: reverse lookup flag name by bit position (template version)
template<typename T>
std::string ReverseLookupFlag(const std::unordered_map<std::string, T> &flag_map, Bitvector bit_value)
{
	for (const auto &[name, value] : flag_map)
	{
		if (static_cast<Bitvector>(value) == bit_value)
		{
			return name;
		}
	}
	return "";  // Not found
}

// EAffect was renumbered to plain 1-based ints, so the packed-bit reconstruction in
// SaveFlagsToTable no longer matches the (enum-valued) affect map. Save affects by testing each
// mapped affect through the typed BitsetFlags API instead.
static void SaveAffectFlags(sqlite3 *db, const std::string &table_name, const std::string &vnum_col,
                            int vnum, const BitsetFlags<EAffect> &flags,
                            const std::unordered_map<std::string, Bitvector> &flag_map,
                            const std::string &category)
{
	const std::string sql = "INSERT INTO " + table_name + " (" + vnum_col
			+ ", flag_category, flag_name) VALUES (?, ?, ?)";
	for (const auto &[name, value] : flag_map)
	{
		if (!flags.test(static_cast<EAffect>(value)))
		{
			continue;
		}
		sqlite3_stmt *stmt = nullptr;
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, vnum);
			sqlite3_bind_text(stmt, 2, category.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}
}

// Enum value <-> text helpers for single-valued enum columns (position, sex).
// An out-of-range value with no symbolic name (e.g. a corrupt default_pos of 15)
// is stored as its raw integer and parsed back verbatim, matching the YAML
// backend's ParseEnum so such values round-trip instead of snapping to a default.
template<typename T>
std::string EnumNameOrInt(const std::unordered_map<std::string, T> &m, int value)
{
	std::string name = ReverseLookupFlag(m, static_cast<Bitvector>(value));
	return name.empty() ? std::to_string(value) : name;
}

inline int EnumIntFromText(const std::unordered_map<std::string, int> &m, const std::string &s, int def)
{
	auto it = m.find(s);
	if (it != m.end())
	{
		return it->second;
	}
	char *end = nullptr;
	const long v = std::strtol(s.c_str(), &end, 10);
	if (!s.empty() && *end == '\0')
	{
		return static_cast<int>(v);
	}
	return def;
}

// Save flags helper (template version)
template<typename T, typename FlagsT>
void SaveFlagsToTable(sqlite3 *db, const std::string &table_name, const std::string &vnum_col,
                      int vnum, const FlagsT &flags,
                      const std::unordered_map<std::string, T> &flag_map,
                      const std::string &category = "")
{
	sqlite3_stmt *stmt = nullptr;
	std::string sql;
	if (category.empty())
	{
		sql = "INSERT INTO " + table_name + " (" + vnum_col + ", flag_name) VALUES (?, ?)";
	}
	else
	{
		sql = "INSERT INTO " + table_name + " (" + vnum_col + ", flag_category, flag_name) VALUES (?, ?, ?)";
	}
	
	for (size_t plane = 0; plane < FlagData::kPlanesNumber; ++plane)
	{
		Bitvector plane_bits = flags.get_plane(plane);
		if (plane_bits == 0) continue;
		
		for (int bit = 0; bit < 30; ++bit)
		{
			if (plane_bits & (1 << bit))
			{
				Bitvector bit_value = (plane << 30) | (1 << bit);
				std::string flag_name = ReverseLookupFlag(flag_map, bit_value);

				if (flag_name.empty())
				{
					// A set bit the enum has no name for (e.g. legacy data bits in
					// the 14-19 gap of the mob act plane). Persist it via the
					// UNUSED_<absolute-bit> convention the loaders understand, so a
					// round-trip is bit-exact instead of silently dropping it.
					flag_name = "UNUSED_" + std::to_string(plane * 30 + bit);
				}

				if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
				{
					int col = 1;
					sqlite3_bind_int(stmt, col++, vnum);
					if (!category.empty())
					{
						BindTextKoi(stmt, col++, category.c_str());
					}
					BindTextKoi(stmt, col++, flag_name.c_str());
					sqlite3_step(stmt);
					sqlite3_finalize(stmt);
				}
			}
		}
	}
}

// ============================================================================
// SqliteWorldDataSource implementation
// ============================================================================

SqliteWorldDataSource::SqliteWorldDataSource(const std::string &db_path)
	: m_db_path(db_path)
	, m_db(nullptr)
{
}

SqliteWorldDataSource::~SqliteWorldDataSource()
{
	CloseDatabase();
}

bool SqliteWorldDataSource::OpenDatabase()
{
	if (m_db)
	{
		return true;
	}

	// READWRITE|CREATE (not READONLY): the engine now writes the world back to
	// SQLite (composite dual-write and stale-source resync), and CREATE lets the
	// first boot build world.db from another source when it does not exist yet.
	int rc = sqlite3_open_v2(m_db_path.c_str(), &m_db,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: Cannot open SQLite database '%s': %s", m_db_path.c_str(), sqlite3_errmsg(m_db));
		sqlite3_close(m_db);
		m_db = nullptr;
		return false;
	}

	log("Opened SQLite database: %s", m_db_path.c_str());
	return true;
}

void SqliteWorldDataSource::CloseDatabase()
{
	if (m_db)
	{
		sqlite3_close(m_db);
		m_db = nullptr;
	}
}

int SqliteWorldDataSource::GetCount(const char *table)
{
	std::string sql = "SELECT COUNT(*) FROM ";
	static const char* enabled_tables[] = {"zones", "rooms", "mobs", "objects", "triggers", nullptr};
	sql += table;
	for (const char** t = enabled_tables; *t; ++t) {
		if (strcmp(table, *t) == 0) {
			sql += " WHERE enabled = 1";
			break;
		}
	}

	sqlite3_stmt *stmt;
	int count = 0;
	if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
	{
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			count = sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}
	return count;
}

bool SqliteWorldDataSource::IsZoneProcessed(int vnum) const
{
	return m_processed_zone_vnums.empty() || m_processed_zone_vnums.count(vnum / 100) > 0;
}

// Helper function to safely convert string to int
static int SafeStoi(const std::string &str, int default_val = 0)
{
	if (str.empty())
	{
		return default_val;
	}
	try
	{
		return std::stoi(str);
	}
	catch (...)
	{
		return default_val;
	}
}

// Helper function to safely convert string to long
static long SafeStol(const std::string &str, long default_val = 0)
{
	if (str.empty())
	{
		return default_val;
	}
	try
	{
		return std::stol(str);
	}
	catch (...)
	{
		return default_val;
	}
}

std::string SqliteWorldDataSource::GetText(sqlite3_stmt *stmt, int col)
{
	const char *text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
	if (!text || !*text)
	{
		return "";
	}
	// Convert UTF-8 from SQLite to KOI8-R
	static char buffer[65536];
	char *input = const_cast<char*>(text);
	char *output = buffer;
	codepages::utf8_to_koi(input, output);
	return buffer;
}

// ============================================================================
// Zone Loading
// ============================================================================

void SqliteWorldDataSource::LoadZones()
{
	log("Loading zones from SQLite database.");

	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for zone loading.");
		return;
	}

	int zone_count = GetCount("zones");
	if (zone_count == 0)
	{
		log("No zones found in SQLite database.");
		return;
	}

	zone_table.reserve(zone_count + dungeons::kNumberOfZoneDungeons);
	zone_table.resize(zone_count);
	log("   %d zones, %zd bytes.", zone_count, sizeof(ZoneData) * zone_count);

	// Load zones
	const char *sql = "SELECT vnum, name, comment, location, author, description, zone_group, "
					  "top_room, lifespan, reset_mode, reset_idle, zone_type, mode, entrance, under_construction "
					  "FROM zones WHERE enabled = 1 ORDER BY vnum";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare zone query: %s", sqlite3_errmsg(m_db));
		return;
	}

	int zone_idx = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW && zone_idx < zone_count)
	{
		ZoneData &zone = zone_table[zone_idx];

		zone.vnum = sqlite3_column_int(stmt, 0);
		zone.name = GetText(stmt, 1);
		int zone_group = sqlite3_column_int(stmt, 6);
		zone.group = (zone_group == 0) ? 1 : zone_group;
		zone.comment = GetText(stmt, 2);
		zone.location = GetText(stmt, 3);
		zone.author = GetText(stmt, 4);
		zone.description = GetText(stmt, 5);
		zone.top = sqlite3_column_int(stmt, 7);
		zone.lifespan = sqlite3_column_int(stmt, 8);
		zone.reset_mode = sqlite3_column_int(stmt, 9);
		zone.reset_idle = sqlite3_column_int(stmt, 10) != 0;
		zone.type = sqlite3_column_int(stmt, 11);
		zone.level = sqlite3_column_int(stmt, 12);
		zone.entrance = sqlite3_column_int(stmt, 13);
		// Initialize runtime fields (uses base class method)
		int under_construction = sqlite3_column_int(stmt, 14);
		InitializeZoneRuntimeFields(zone, under_construction);

		// Load zone commands
		LoadZoneCommands(zone);

		// Load zone groups
		LoadZoneGroups(zone);

		zone_idx++;
	}
	sqlite3_finalize(stmt);

	log("Loaded %d zones from SQLite.", zone_idx - 1);
}

// Runs the four room child sub-loaders (flags/exits/triggers/extra
// descriptions) exactly once, scoped to the set of zones THIS source
// actually loaded (m_processed_zone_vnums). MUST run before
// RosolveWorldDoorToRoomVnumsToRnums() (db.cpp) -- that pass resolves every
// room's exit target from vnum to rnum, so exits must already be populated,
// which is why this is a separate, earlier call from FinalizeZoneEntities().
void SqliteWorldDataSource::FinalizeZoneRooms()
{
	if (m_processed_zone_vnums.empty())
	{
		return;
	}

	// The four sub-loaders take vnum_to_rnum as a parameter, so build it
	// scoped to processed zones only -- otherwise they'd overwrite
	// YAML-sourced rooms with (possibly stale) SQLite room_flags/exits/
	// triggers/extra-description rows.
	std::map<int, int> room_vnum_to_rnum;
	for (RoomRnum i = kFirstRoom; i <= top_of_world; i++)
	{
		if (world[i] && IsZoneProcessed(world[i]->vnum))
		{
			room_vnum_to_rnum[world[i]->vnum] = i;
		}
	}
	LoadRoomFlags(room_vnum_to_rnum);
	LoadRoomExits(room_vnum_to_rnum);
	LoadRoomTriggers(room_vnum_to_rnum);
	LoadRoomExtraDescriptions(room_vnum_to_rnum);
}

// Runs every mob/object child sub-loader exactly once, after every entity
// type is loaded, scoped to the set of zones THIS source actually loaded.
// No downstream vnum-to-rnum resolution pass depends on this data (unlike
// room exits), so it's safe to defer to the very end of boot.
void SqliteWorldDataSource::FinalizeZoneEntities()
{
	if (m_processed_zone_vnums.empty())
	{
		return;
	}

	// Each sub-loader already checks IsZoneProcessed() internally (added at
	// the top of every per-row loop), so no filtered map is needed here.
	LoadMobFlags();
	LoadMobSkills();
	LoadMobTriggers();
	LoadMobResistances();
	LoadMobSaves();
	LoadMobFeats();
	LoadMobSpells();
	LoadMobHelpers();
	LoadMobDestinations();
	LoadMobDeathLoad();

	LoadObjectFlags();
	LoadObjectApplies();
	LoadObjectSkills();
	LoadObjectExtraValues();
	LoadObjectTriggers();
	LoadObjectExtraDescriptions();
}

void SqliteWorldDataSource::LoadZoneCommands(ZoneData &zone)
{
	// Count commands for this zone
	std::string count_sql = "SELECT COUNT(*) FROM zone_commands WHERE zone_vnum = " + std::to_string(zone.vnum);
	sqlite3_stmt *stmt;
	int cmd_count = 0;

	if (sqlite3_prepare_v2(m_db, count_sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
	{
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			cmd_count = sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}

	// Always need at least one command for 'S' terminator
	CREATE(zone.cmd, cmd_count + 1);

	if (cmd_count == 0)
	{
		zone.cmd[0].command = 'S';
		return;
	}

	// Load commands
	std::string sql = "SELECT cmd_type, if_flag, arg_mob_vnum, arg_obj_vnum, arg_room_vnum, "
					  "arg_max, arg_max_world, arg_max_room, arg_load_prob, arg_wear_pos_id, "
					  "arg_direction_id, arg_state, arg_trigger_vnum, arg_trigger_type, "
					  "arg_context, arg_var_name, arg_var_value, arg_container_vnum, "
					  "arg_leader_mob_vnum, arg_follower_mob_vnum "
					  "FROM zone_commands WHERE zone_vnum = " + std::to_string(zone.vnum) +
					  " ORDER BY cmd_order";

	if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
	{
		zone.cmd[0].command = 'S';
		return;
	}

	int idx = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW && idx < cmd_count)
	{
		std::string cmd_type = GetText(stmt, 0);
		struct reset_com &cmd = zone.cmd[idx];

		// Initialize
		cmd.command = '*';
		cmd.if_flag = sqlite3_column_int(stmt, 1);
		cmd.arg1 = 0;
		cmd.arg2 = 0;
		cmd.arg3 = 0;
		cmd.arg4 = -1;
		cmd.sarg1 = nullptr;
		cmd.sarg2 = nullptr;
		cmd.line = 0;

		// Map command type
		if (strcmp(cmd_type.c_str(), "DISABLED") == 0)
		{
			// A resolver-invalidated '*' command preserved verbatim.
			cmd.command = '*';
			cmd.arg1 = sqlite3_column_int(stmt, 2);   // arg_mob_vnum
			cmd.arg2 = sqlite3_column_int(stmt, 3);   // arg_obj_vnum
			cmd.arg3 = sqlite3_column_int(stmt, 4);   // arg_room_vnum
			cmd.arg4 = sqlite3_column_int(stmt, 12);  // arg_trigger_vnum
		}
		else if (strcmp(cmd_type.c_str(), "LOAD_MOB") == 0)
		{
			cmd.command = 'M';
			cmd.arg1 = sqlite3_column_int(stmt, 2);  // mob_vnum
			cmd.arg2 = sqlite3_column_int(stmt, 6);  // max_world
			cmd.arg3 = sqlite3_column_int(stmt, 4);  // room_vnum
			cmd.arg4 = sqlite3_column_int(stmt, 7);  // max_room
		}
		else if (strcmp(cmd_type.c_str(), "LOAD_OBJ") == 0)
		{
			cmd.command = 'O';
			cmd.arg1 = sqlite3_column_int(stmt, 3);  // obj_vnum
			cmd.arg2 = sqlite3_column_int(stmt, 5);  // max
			cmd.arg3 = sqlite3_column_int(stmt, 4);  // room_vnum
			cmd.arg4 = sqlite3_column_int(stmt, 8);  // load_prob
		}
		else if (strcmp(cmd_type.c_str(), "GIVE_OBJ") == 0)
		{
			cmd.command = 'G';
			cmd.arg1 = sqlite3_column_int(stmt, 3);  // obj_vnum
			cmd.arg2 = sqlite3_column_int(stmt, 5);  // max
			cmd.arg3 = -1;
			cmd.arg4 = sqlite3_column_int(stmt, 8);  // load_prob
		}
		else if (strcmp(cmd_type.c_str(), "EQUIP_MOB") == 0)
		{
			cmd.command = 'E';
			cmd.arg1 = sqlite3_column_int(stmt, 3);  // obj_vnum
			cmd.arg2 = sqlite3_column_int(stmt, 5);  // max
			int wear_pos = sqlite3_column_int(stmt, 9);
			cmd.arg3 = wear_pos;
			cmd.arg4 = sqlite3_column_int(stmt, 8);  // load_prob
		}
		else if (strcmp(cmd_type.c_str(), "PUT_OBJ") == 0)
		{
			cmd.command = 'P';
			cmd.arg1 = sqlite3_column_int(stmt, 3);  // obj_vnum
			cmd.arg2 = sqlite3_column_int(stmt, 5);  // max
			cmd.arg3 = sqlite3_column_int(stmt, 17); // container_vnum
			cmd.arg4 = sqlite3_column_int(stmt, 8);  // load_prob
		}
		else if (strcmp(cmd_type.c_str(), "DOOR") == 0)
		{
			cmd.command = 'D';
			cmd.arg1 = sqlite3_column_int(stmt, 4);  // room_vnum
			int zone_dir = sqlite3_column_int(stmt, 10);
			
			cmd.arg2 = zone_dir;
			cmd.arg3 = sqlite3_column_int(stmt, 11); // state
		}
		else if (strcmp(cmd_type.c_str(), "REMOVE_OBJ") == 0)
		{
			cmd.command = 'R';
			cmd.arg1 = sqlite3_column_int(stmt, 4);  // room_vnum
			cmd.arg2 = sqlite3_column_int(stmt, 3);  // obj_vnum
		}
		else if (strcmp(cmd_type.c_str(), "TRIGGER") == 0)
		{
			cmd.command = 'T';
			std::string trig_type = GetText(stmt, 13);
			cmd.arg1 = !trig_type.empty() ? SafeStoi(trig_type) : 0;
			cmd.arg2 = sqlite3_column_int(stmt, 12); // trigger_vnum
			cmd.arg3 = sqlite3_column_int(stmt, 4);  // room_vnum (or -1 for mob/obj)
		}
		else if (strcmp(cmd_type.c_str(), "VARIABLE") == 0)
		{
			cmd.command = 'V';
			std::string trig_type = GetText(stmt, 13);
			cmd.arg1 = !trig_type.empty() ? SafeStoi(trig_type) : 0;
			cmd.arg2 = sqlite3_column_int(stmt, 14); // context
			cmd.arg3 = sqlite3_column_int(stmt, 4);  // room_vnum
			std::string var_name = GetText(stmt, 15);
			std::string var_value = GetText(stmt, 16);
			if (!var_name.empty()) cmd.sarg1 = str_dup(var_name.c_str());
			if (!var_value.empty()) cmd.sarg2 = str_dup(var_value.c_str());
		}
		else if (strcmp(cmd_type.c_str(), "EXTRACT_MOB") == 0)
		{
			cmd.command = 'Q';
			cmd.arg1 = sqlite3_column_int(stmt, 2);  // mob_vnum
		}
		else if (strcmp(cmd_type.c_str(), "FOLLOW") == 0)
		{
			cmd.command = 'F';
			cmd.arg1 = sqlite3_column_int(stmt, 4);  // room_vnum
			cmd.arg2 = sqlite3_column_int(stmt, 18); // leader_mob_vnum
			cmd.arg3 = sqlite3_column_int(stmt, 19); // follower_mob_vnum
		}

		idx++;
	}
	sqlite3_finalize(stmt);

	// Add terminating command
	zone.cmd[idx].command = 'S';
}

void SqliteWorldDataSource::LoadZoneGroups(ZoneData &zone)
{
	sqlite3_stmt *stmt;

	// Count and load typeA
	std::string sql = "SELECT linked_zone_vnum FROM zone_groups WHERE zone_vnum = " +
					  std::to_string(zone.vnum) + " AND group_type = 'A'";

	if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
	{
		// First count
		std::vector<int> typeA_zones;
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			typeA_zones.push_back(sqlite3_column_int(stmt, 0));
		}
		sqlite3_finalize(stmt);

		if (!typeA_zones.empty())
		{
			zone.typeA_count = typeA_zones.size();
			CREATE(zone.typeA_list, zone.typeA_count);
			for (int i = 0; i < zone.typeA_count; i++)
			{
				zone.typeA_list[i] = typeA_zones[i];
			}
		}
	}

	// Count and load typeB
	sql = "SELECT linked_zone_vnum FROM zone_groups WHERE zone_vnum = " +
		  std::to_string(zone.vnum) + " AND group_type = 'B'";

	if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
	{
		std::vector<int> typeB_zones;
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			typeB_zones.push_back(sqlite3_column_int(stmt, 0));
		}
		sqlite3_finalize(stmt);

		if (!typeB_zones.empty())
		{
			zone.typeB_count = typeB_zones.size();
			CREATE(zone.typeB_list, zone.typeB_count);
			CREATE(zone.typeB_flag, zone.typeB_count);
			for (int i = 0; i < zone.typeB_count; i++)
			{
				zone.typeB_list[i] = typeB_zones[i];
				zone.typeB_flag[i] = false;
			}
		}
	}
}

// ============================================================================
// Trigger Loading
// ============================================================================

// The only trigger-loading entry point: `zone_filter` null means every zone.
// One unfiltered query (matches the old bulk behavior), filtered in memory if
// `zone_filter` is given -- a single sqlite3 connection can't usefully run
// concurrent statements anyway, so there's no parallelism to gain by turning
// this into N per-zone queries, and the unfiltered scan is already fast.
// Returns results WITHOUT touching trig_index/top_of_trigt -- the caller
// (GameLoader::BootWorld's orchestrator in db.cpp) does that placement once,
// after possibly combining results from other sources too (composite).
std::vector<LoadedTrigger> SqliteWorldDataSource::LoadTriggers(const std::vector<int> *zone_filter)
{
	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for trigger loading.");
		return {};
	}

	const char *sql = "SELECT t.vnum, t.name, t.attach_type_id, GROUP_CONCAT(ttb.type_char, '') AS type_chars, t.narg, t.arglist, t.script "
					  "FROM triggers t LEFT JOIN trigger_type_bindings ttb ON t.vnum = ttb.trigger_vnum WHERE t.enabled = 1 GROUP BY t.vnum ORDER BY t.vnum";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare trigger query: %s", sqlite3_errmsg(m_db));
		return {};
	}

	std::set<int> filter_set;
	if (zone_filter)
	{
		filter_set.insert(zone_filter->begin(), zone_filter->end());
	}

	std::vector<LoadedTrigger> result;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int vnum = sqlite3_column_int(stmt, 0);
		if (zone_filter && !filter_set.count(vnum / 100))
		{
			continue;
		}
		std::string name = GetText(stmt, 1);
		int attach_type_id = sqlite3_column_int(stmt, 2);
		std::string type_chars = GetText(stmt, 3);
		int narg = sqlite3_column_int(stmt, 4);
		std::string arglist = GetText(stmt, 5);
		std::string script = GetText(stmt, 6);

		byte attach_type = static_cast<byte>(attach_type_id);

		// Compute trigger_type bitmask from type_chars
		long trigger_type = 0;
		for (char ch : type_chars)
		{
			if (ch >= 'a' && ch <= 'z')
				trigger_type |= (1L << (ch - 'a'));
			else if (ch >= 'A' && ch <= 'Z')
				trigger_type |= (1L << (26 + ch - 'A'));
		}

		// rnum is set for real by the orchestrator once this trigger's final
		// position in trig_index is known -- -1 here matches YamlWorldDataSource
		// ::ParseTriggerNode's placeholder convention.
		auto trig = new Trigger(-1, std::move(name), attach_type, trigger_type);
		GET_TRIG_NARG(trig) = narg;
		trig->arglist = arglist;
		ParseTriggerScript(trig, script);

		result.push_back(LoadedTrigger{vnum, trig});
	}
	sqlite3_finalize(stmt);

	log("Loaded %zu triggers from SQLite.", result.size());
	return result;
}

// ============================================================================
// Room Loading
// ============================================================================

// The only room-loading entry point: `zone_filter` null means every zone.
// One unfiltered query (matches the old bulk behavior; `rooms` has a real
// zone_vnum column, but filtering in memory keeps this consistent with
// triggers/mobs/objects, which don't), filtered in memory if `zone_filter`
// is given. Returns results WITHOUT touching world[]/top_of_world or
// zone_table[].RnumRoomsLocation, and WITHOUT running the room_flags/exits/
// triggers/extra-descriptions child sub-loaders -- the caller (GameLoader::
// BootWorld's orchestrator in db.cpp) places rooms in world[] once (after
// possibly combining results from other sources too), then calls
// FinalizeZoneRooms() to run the child sub-loaders, scoped to
// m_processed_zone_vnums (populated below).
std::vector<LoadedRoom> SqliteWorldDataSource::LoadRooms(const std::vector<int> *zone_filter)
{
	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for room loading.");
		return {};
	}

	const char *sql = "SELECT vnum, zone_vnum, name, description, sector_id FROM rooms WHERE enabled = 1 ORDER BY vnum";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare room query: %s", sqlite3_errmsg(m_db));
		return {};
	}

	std::set<int> filter_set;
	if (zone_filter)
	{
		filter_set.insert(zone_filter->begin(), zone_filter->end());
	}

	// Zone rnum - increments as we process rooms in vnum order (same as
	// Legacy); walking forward monotonically stays correct even when some
	// rows are skipped by the in-memory filter below, since rows are still
	// visited in ascending vnum order.
	ZoneRnum zone_rn = 0;
	std::vector<LoadedRoom> result;

	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int vnum = sqlite3_column_int(stmt, 0);
		while (zone_rn < static_cast<ZoneRnum>(zone_table.size()) && vnum > zone_table[zone_rn].top)
		{
			if (++zone_rn >= static_cast<ZoneRnum>(zone_table.size()))
			{
				log("SYSERR: Room %d is outside of any zone.", vnum);
				break;
			}
		}

		if (zone_filter && !filter_set.count(vnum / 100))
		{
			continue;
		}

		std::string name = GetText(stmt, 2);
		std::string description = GetText(stmt, 3);
		int sector_id = sqlite3_column_int(stmt, 4);

		auto room = new RoomData;
		room->vnum = vnum;
		// Apply UPPER to first character (same as Legacy loader)
		if (!name.empty()) { name[0] = UPPER(name[0]); }
		room->set_name(name);

		if (!description.empty())
		{
			room->description_num = GlobalObjects::descriptions().add(description);
		}

		if (zone_rn < static_cast<ZoneRnum>(zone_table.size()))
		{
			room->zone_rn = zone_rn;
		}

		room->sector_type = static_cast<ESector>(sector_id);

		result.push_back(LoadedRoom{vnum, room, {}});
		m_processed_zone_vnums.insert(vnum / 100);
	}
	sqlite3_finalize(stmt);

	log("Loaded %zu rooms from SQLite.", result.size());
	return result;
}

void SqliteWorldDataSource::LoadRoomFlags(const std::map<int, int> &vnum_to_rnum)
{
	const char *sql = "SELECT room_vnum, flag_name FROM room_flags";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare room flags query: %s", sqlite3_errmsg(m_db));
		return;
	}

	int flags_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int room_vnum = sqlite3_column_int(stmt, 0);
		std::string flag_name = GetText(stmt, 1);

		auto room_it = vnum_to_rnum.find(room_vnum);
		if (room_it == vnum_to_rnum.end()) continue;

		auto flag_it = room_flag_map.find(flag_name);
		if (flag_it != room_flag_map.end())
		{
			world[room_it->second]->set_flag(flag_it->second);
			flags_set++;
		}
		else if (flag_name.rfind("UNUSED_", 0) == 0)
		{
			int absbit = std::stoi(flag_name.substr(7));
			Bitvector v = (static_cast<Bitvector>(absbit / 30) << 30) | (static_cast<Bitvector>(1) << (absbit % 30));
			world[room_it->second]->set_flag(static_cast<ERoomFlag>(v));
			flags_set++;
		}
	}
	sqlite3_finalize(stmt);

	log("   Set %d room flags.", flags_set);
}

void SqliteWorldDataSource::LoadRoomExits(const std::map<int, int> &vnum_to_rnum)
{
	const char *sql = "SELECT room_vnum, direction_id, description, keywords, exit_flags, "
					  "key_vnum, to_room, lock_complexity FROM room_exits";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare room exits query: %s", sqlite3_errmsg(m_db));
		return;
	}

	int exits_loaded = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int room_vnum = sqlite3_column_int(stmt, 0);
		int dir = sqlite3_column_int(stmt, 1);
		std::string desc = GetText(stmt, 2);
		std::string keywords = GetText(stmt, 3);
		std::string exit_flags_str = GetText(stmt, 4);
		int exit_flags = !exit_flags_str.empty() ? SafeStoi(exit_flags_str) : 0;
		int key_vnum = sqlite3_column_int(stmt, 5);
		int to_room_vnum = sqlite3_column_int(stmt, 6);
		int lock_complexity = sqlite3_column_int(stmt, 7);

		auto it = vnum_to_rnum.find(room_vnum);
		if (it == vnum_to_rnum.end()) continue;

		RoomData *room = world[it->second];


		if (dir < 0 || dir >= EDirection::kMaxDirNum) continue;

		auto exit_data = std::make_shared<ExitData>();

		// Store vnum - RosolveWorldDoorToRoomVnumsToRnums() will convert to rnum later
		exit_data->to_room(to_room_vnum);

		exit_data->key = key_vnum;
		exit_data->lock_complexity = lock_complexity;
		if (!desc.empty()) exit_data->general_description = desc;
		if (!keywords.empty()) exit_data->set_keywords(keywords);

		// Set exit flags (stored as string in database, parse as integer)
		exit_data->exit_info = exit_flags;

		// Дропаем полностью пустые D-блоки (симметрично с legacy/yaml),
		// см. issue #3272.
		if (exit_data->is_empty()) continue;

		room->dir_option_proto[dir] = exit_data;
		exits_loaded++;
	}
	sqlite3_finalize(stmt);

	log("   Loaded %d room exits.", exits_loaded);
}

void SqliteWorldDataSource::LoadRoomTriggers(const std::map<int, int> &vnum_to_rnum)
{
	const char *sql = "SELECT entity_vnum, trigger_vnum FROM entity_triggers WHERE entity_type = 'room' ORDER BY trigger_order";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int room_vnum = sqlite3_column_int(stmt, 0);
		int trigger_vnum = sqlite3_column_int(stmt, 1);

		auto it = vnum_to_rnum.find(room_vnum);
		if (it == vnum_to_rnum.end()) continue;

		AttachTriggerToRoom(it->second, trigger_vnum, room_vnum);
	}
	sqlite3_finalize(stmt);
}

void SqliteWorldDataSource::LoadRoomExtraDescriptions(const std::map<int, int> &vnum_to_rnum)
{
	// ORDER BY id DESC: the loop below PREPENDS onto room->ex_description --
	// same as the YAML loader, which iterates the YAML list forward and also
	// prepends, so its in-memory order ends up list-reversed. SaveRoomRecord
	// writes rows by walking that same (already-reversed) in-memory list
	// head-to-tail, so the lowest id is the LAST list entry. Re-prepending in
	// id-DESCENDING order reproduces the YAML loader's list-reversed order
	// exactly; id-ASCENDING does not (verified via full-world round-trip
	// checksum testing on a room with 2+ extra descriptions).
	const char *sql = "SELECT entity_vnum, keywords, description FROM extra_descriptions WHERE entity_type = 'room' ORDER BY id DESC";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	int loaded = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int room_vnum = sqlite3_column_int(stmt, 0);
		std::string keywords = GetText(stmt, 1);
		std::string description = GetText(stmt, 2);

		auto it = vnum_to_rnum.find(room_vnum);
		if (it == vnum_to_rnum.end()) continue;

		auto ex_desc = std::make_shared<ExtraDescription>();
		ex_desc->set_keyword(keywords);
		ex_desc->set_description(description);
		ex_desc->next = world[it->second]->ex_description;
		world[it->second]->ex_description = ex_desc;
		loaded++;
	}
	sqlite3_finalize(stmt);

	if (loaded > 0)
	{
		log("   Loaded %d room extra descriptions.", loaded);
	}
}

// ============================================================================
// Mob Loading
// ============================================================================

// Parses one row of the primary `mobs` query into a freshly-allocated
// CharData, WITHOUT touching mob_proto[]/mob_index[]/top_of_mobt/
// zone_table[].RnumMobsLocation -- the caller (LoadMobs) collects these and
// the orchestrator (db.cpp) does that placement once, after possibly
// combining results from other sources too (composite).
LoadedMob SqliteWorldDataSource::LoadMobRow(sqlite3_stmt *stmt)
{
	int vnum = sqlite3_column_int(stmt, 0);
	auto *mob_ptr = new CharData;
	CharData &mob = *mob_ptr;

	// Initialize mob
	mob.player_specials = player_special_data::s_for_mobiles;
	mob.SetNpcAttribute(true);
	mob.player_specials->saved.NameGod = 1001; // Default for Russian name declension
	mob.set_move(100);
	mob.set_max_move(100);

	// Names
	mob.SetCharAliases(GetText(stmt, 1));
	mob.set_npc_name(GetText(stmt, 2));
	mob.player_data.PNames[grammar::ECase::kNom] = GetText(stmt, 2);
	mob.player_data.PNames[grammar::ECase::kGen] = GetText(stmt, 3);
	mob.player_data.PNames[grammar::ECase::kDat] = GetText(stmt, 4);
	mob.player_data.PNames[grammar::ECase::kAcc] = GetText(stmt, 5);
	mob.player_data.PNames[grammar::ECase::kIns] = GetText(stmt, 6);
	mob.player_data.PNames[grammar::ECase::kPre] = GetText(stmt, 7);

	// Descriptions
	mob.player_data.long_descr = utils::colorCAP(GetText(stmt, 8));
	mob.player_data.description = GetText(stmt, 9);

	// Base parameters
	alignment::SetAlignment(&mob, sqlite3_column_int(stmt, 10));

	// Stats
	mob.set_level(sqlite3_column_int(stmt, 12));
	GET_HR(&mob) = sqlite3_column_int(stmt, 13);
	GET_AC(&mob) = sqlite3_column_int(stmt, 14);

	// HP dice
	mob.mem_queue.total = sqlite3_column_int(stmt, 15);  // hp_dice_count
	mob.mem_queue.stored = sqlite3_column_int(stmt, 16);  // hp_dice_size
	int hp_bonus = sqlite3_column_int(stmt, 17);
	mob.set_hit(hp_bonus);
	mob.set_max_hit(0);  // 0 = flag that HP is xdy+z

	// Damage dice
	mob.mob_specials.damnodice = sqlite3_column_int(stmt, 18);
	mob.mob_specials.damsizedice = sqlite3_column_int(stmt, 19);
	mob.real_abils.damroll = sqlite3_column_int(stmt, 20);

	// Gold dice
	mob.mob_specials.GoldNoDs = sqlite3_column_int(stmt, 21);
	mob.mob_specials.GoldSiDs = sqlite3_column_int(stmt, 22);
	currencies::SetHand(mob, currencies::kGold, sqlite3_column_int(stmt, 23));

	// Experience
	mob.set_exp(sqlite3_column_int(stmt, 24));

	// Position (name, or a raw int for out-of-range values like 15)
	std::string default_pos = GetText(stmt, 25);
	std::string start_pos = GetText(stmt, 26);
	mob.mob_specials.default_pos = static_cast<EPosition>(
		EnumIntFromText(position_map, default_pos, static_cast<int>(EPosition::kStand)));
	mob.SetPosition(static_cast<EPosition>(
		EnumIntFromText(position_map, start_pos, static_cast<int>(EPosition::kStand))));

	// Sex
	std::string sex_str = GetText(stmt, 27);
	mob.set_sex(static_cast<EGender>(
		EnumIntFromText(gender_map, sex_str, static_cast<int>(EGender::kMale))));

	// Physical attributes -- bounds mirror MobileFile::interpret_espec
	// (boot_data_files.cpp). Без них старые данные расходятся с легаси.
	GET_SIZE(&mob) = std::clamp<byte>(sqlite3_column_int(stmt, 28), 0, 100);
	GET_HEIGHT(&mob) = std::clamp(sqlite3_column_int(stmt, 29), 0, 200);
	GET_WEIGHT(&mob) = std::clamp(sqlite3_column_int(stmt, 30), 0, 200);

	// Class and race
	mob.set_class(static_cast<ECharClass>(sqlite3_column_int(stmt, 31)));
	mob.player_data.Race = std::clamp(static_cast<ENpcRace>(sqlite3_column_int(stmt, 32)),
									  ENpcRace::kBasic, ENpcRace::kLastNpcRace);

	// Attributes (E-spec)
	mob.set_str(sqlite3_column_int(stmt, 33));
	mob.set_dex(sqlite3_column_int(stmt, 34));
	mob.set_int(sqlite3_column_int(stmt, 35));
	mob.set_wis(sqlite3_column_int(stmt, 36));
	mob.set_con(sqlite3_column_int(stmt, 37));
	mob.set_cha(sqlite3_column_int(stmt, 38));

	// Enhanced E-spec fields (scalar values).
	// Clamps mirror interpret_espec; см. PR #3224.
	mob.set_str_add(sqlite3_column_int(stmt, 39));
	mob.add_abils.hitreg = std::clamp(sqlite3_column_int(stmt, 40), -200, 200);
	mob.add_abils.armour = std::clamp(sqlite3_column_int(stmt, 41), 0, 100);
	mob.add_abils.manareg = std::clamp(sqlite3_column_int(stmt, 42), -200, 200);
	mob.add_abils.cast_success = std::clamp(sqlite3_column_int(stmt, 43), -200, 300);
	mob.add_abils.morale = std::clamp(sqlite3_column_int(stmt, 44), 0, 100);
	mob.add_abils.initiative_add = std::clamp(sqlite3_column_int(stmt, 45), -200, 200);
	mob.add_abils.absorb = std::clamp(sqlite3_column_int(stmt, 46), -200, 200);
	mob.add_abils.aresist = std::clamp(sqlite3_column_int(stmt, 47), 0, 100);
	mob.add_abils.mresist = std::clamp(sqlite3_column_int(stmt, 48), 0, 100);
	mob.add_abils.presist = std::clamp(sqlite3_column_int(stmt, 49), 0, 100);
	mob.mob_specials.attack_type = std::clamp(sqlite3_column_int(stmt, 50), 0, 99);
	mob.mob_specials.like_work = std::clamp<byte>(sqlite3_column_int(stmt, 51), 0, 100);
	mob.mob_specials.MaxFactor = std::clamp<byte>(sqlite3_column_int(stmt, 52), 0, 127);
	mob.mob_specials.extra_attack = std::clamp<byte>(sqlite3_column_int(stmt, 53), 0, 127);
	mob.set_remort(std::clamp<byte>(sqlite3_column_int(stmt, 54), 0, 100));

	// special_bitvector (TEXT - FlagData)
	std::string special_bv = GetText(stmt, 55);
	if (!special_bv.empty())
	{
		mob.mob_specials.npc_flags.from_string((char *)special_bv.c_str());
	}

	// role (TEXT - bitset<9>)
	std::string role_str = GetText(stmt, 56);
	if (!role_str.empty())
	{
		CharData::role_t role(role_str);
		mob.set_role(role);
	}

	// Movement speed: -1 == default cadence (the common 3-field legacy
	// position line). Older DBs without the column read NULL -> -1, the
	// same default the YAML loader uses (issue #3384 class).
	mob.mob_specials.speed = (sqlite3_column_type(stmt, 57) == SQLITE_NULL)
		? -1 : sqlite3_column_int(stmt, 57);

	// Initialize test data if needed
	if (mob.GetLevel() == 0)
		SetTestData(&mob);

	return LoadedMob{vnum, mob_ptr, {}};
}

// The only mob-loading entry point: `zone_filter` null means every zone.
// One unfiltered query (matches the old bulk behavior), filtered in memory
// if `zone_filter` is given. Returns results WITHOUT touching mob_proto[]/
// mob_index[]/top_of_mobt/zone_table[].RnumMobsLocation, and WITHOUT running
// the ten child sub-loaders (flags/skills/triggers/resistances/...) -- the
// caller (GameLoader::BootWorld's orchestrator in db.cpp) places mobs once
// (after possibly combining results from other sources too), then calls
// FinalizeZoneEntities() to run the child sub-loaders, scoped to
// m_processed_zone_vnums (populated below).
std::vector<LoadedMob> SqliteWorldDataSource::LoadMobs(const std::vector<int> *zone_filter)
{
	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for mob loading.");
		return {};
	}

	const char *sql = "SELECT vnum, aliases, name_nom, name_gen, name_dat, name_acc, name_ins, name_pre, "
					  "short_desc, long_desc, alignment, mob_type, level, hitroll_penalty, armor, "
					  "hp_dice_count, hp_dice_size, hp_bonus, dam_dice_count, dam_dice_size, dam_bonus, "
					  "gold_dice_count, gold_dice_size, gold_bonus, experience, default_pos, start_pos, "
					  "sex, size, height, weight, mob_class, race, "
					  "attr_str, attr_dex, attr_int, attr_wis, attr_con, attr_cha, "
					  "attr_str_add, hp_regen, armour_bonus, mana_regen, cast_success, morale, "
					  "initiative_add, absorb, aresist, mresist, presist, bare_hand_attack, "
					  "like_work, max_factor, extra_attack, mob_remort, special_bitvector, role, "
					  "speed "
					  "FROM mobs WHERE enabled = 1 ORDER BY vnum";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare mob query: %s", sqlite3_errmsg(m_db));
		return {};
	}

	std::set<int> filter_set;
	if (zone_filter)
	{
		filter_set.insert(zone_filter->begin(), zone_filter->end());
	}

	std::vector<LoadedMob> result;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int vnum = sqlite3_column_int(stmt, 0);
		if (zone_filter && !filter_set.count(vnum / 100))
		{
			continue;
		}
		result.push_back(LoadMobRow(stmt));
		m_processed_zone_vnums.insert(vnum / 100);
	}
	sqlite3_finalize(stmt);

	log("Loaded %zu mobs from SQLite.", result.size());
	return result;
}

void SqliteWorldDataSource::LoadMobFlags()
{
	const char *sql = "SELECT mob_vnum, flag_category, flag_name FROM mob_flags";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	// Build vnum to rnum map
	std::map<int, int> vnum_to_rnum;
	for (MobRnum i = 0; i <= top_of_mobt; i++)
	{
		vnum_to_rnum[mob_index[i].vnum] = i;
	}

	int flags_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int mob_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(mob_vnum)) continue;
		std::string category = GetText(stmt, 1);
		std::string flag_name = GetText(stmt, 2);

		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;

		CharData &mob = mob_proto[it->second];

		if (strcmp(category.c_str(), "action") == 0)
		{
			auto flag_it = mob_action_flag_map.find(flag_name);
			if (flag_it != mob_action_flag_map.end() && flag_it->second != 0)
			{
				mob.SetFlag(static_cast<EMobFlag>(flag_it->second));
				flags_set++;
			}
			else if (flag_name.rfind("UNUSED_", 0) == 0)
			{
				int absbit = std::stoi(flag_name.substr(7));
				Bitvector v = (static_cast<Bitvector>(absbit / 30) << 30) | (static_cast<Bitvector>(1) << (absbit % 30));
				mob.SetFlag(static_cast<EMobFlag>(v));
				flags_set++;
			}
		}
		else if (strcmp(category.c_str(), "affect") == 0)
		{
			auto flag_it = mob_affect_flag_map.find(flag_name);
			if (flag_it != mob_affect_flag_map.end() && flag_it->second != 0)
			{
				AFF_FLAGS(&mob).set(static_cast<EAffect>(flag_it->second));
				flags_set++;
			}
			else if (flag_name.rfind("UNUSED_", 0) == 0)
			{
				int absbit = std::stoi(flag_name.substr(7));
				AFF_FLAGS(&mob).set_index(static_cast<std::size_t>(absbit));
				flags_set++;
			}
		}
	}
	sqlite3_finalize(stmt);

	log("   Set %d mob flags.", flags_set);
}

void SqliteWorldDataSource::LoadMobSkills()
{
	const char *sql = "SELECT mob_vnum, skill_id, value FROM mob_skills";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	// Build vnum to rnum map
	std::map<int, int> vnum_to_rnum;
	for (MobRnum i = 0; i <= top_of_mobt; i++)
	{
		vnum_to_rnum[mob_index[i].vnum] = i;
	}

	int skills_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int mob_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(mob_vnum)) continue;
		int skill_id = sqlite3_column_int(stmt, 1);
		int value = sqlite3_column_int(stmt, 2);

		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;

		auto skill = static_cast<ESkill>(skill_id);
		SetSkill(&mob_proto[it->second], skill, value);
		skills_set++;
	}
	sqlite3_finalize(stmt);

	if (skills_set > 0)
	{
		log("   Set %d mob skills.", skills_set);
	}
}

void SqliteWorldDataSource::LoadMobTriggers()
{
	const char *sql = "SELECT entity_vnum, trigger_vnum FROM entity_triggers WHERE entity_type = 'mob' ORDER BY trigger_order";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	// Build vnum to rnum map
	std::map<int, int> vnum_to_rnum;
	for (MobRnum i = 0; i <= top_of_mobt; i++)
	{
		vnum_to_rnum[mob_index[i].vnum] = i;
	}

	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int mob_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(mob_vnum)) continue;
		int trigger_vnum = sqlite3_column_int(stmt, 1);

		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;

		AttachTriggerToMob(it->second, trigger_vnum, mob_vnum);
	}
	sqlite3_finalize(stmt);
}

void SqliteWorldDataSource::LoadMobResistances()
{
	const char *sql = "SELECT mob_vnum, resist_type, value FROM mob_resistances";
	
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}
	
	// Build vnum to rnum map
	std::map<int, int> vnum_to_rnum;
	for (MobRnum i = 0; i <= top_of_mobt; i++)
	{
		vnum_to_rnum[mob_index[i].vnum] = i;
	}
	
	int resistances_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int mob_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(mob_vnum)) continue;
		int resist_type = sqlite3_column_int(stmt, 1);
		int value = sqlite3_column_int(stmt, 2);
		
		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;
		
		CharData &mob = mob_proto[it->second];
		if (resist_type >= 0 && resist_type < static_cast<int>(mob.add_abils.apply_resistance.size()))
		{
			mob.add_abils.apply_resistance[resist_type] = std::clamp(value, kMinResistance, kMaxNpcResist);
			resistances_set++;
		}
	}
	sqlite3_finalize(stmt);
	
	if (resistances_set > 0)
	{
		log("   Set %d mob resistances.", resistances_set);
	}
}

void SqliteWorldDataSource::LoadMobSaves()
{
	const char *sql = "SELECT mob_vnum, save_type, value FROM mob_saves";
	
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}
	
	// Build vnum to rnum map
	std::map<int, int> vnum_to_rnum;
	for (MobRnum i = 0; i <= top_of_mobt; i++)
	{
		vnum_to_rnum[mob_index[i].vnum] = i;
	}
	
	int saves_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int mob_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(mob_vnum)) continue;
		int save_type = sqlite3_column_int(stmt, 1);
		int value = sqlite3_column_int(stmt, 2);
		
		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;
		
		CharData &mob = mob_proto[it->second];
		if (save_type >= 0 && save_type < static_cast<int>(mob.add_abils.apply_saving_throw.size()))
		{
			mob.add_abils.apply_saving_throw[save_type] = std::clamp(value, kMinSaving, kMaxSaving);
			saves_set++;
		}
	}
	sqlite3_finalize(stmt);
	
	if (saves_set > 0)
	{
		log("   Set %d mob saves.", saves_set);
	}
}

void SqliteWorldDataSource::LoadMobFeats()
{
	const char *sql = "SELECT mob_vnum, feat_id FROM mob_feats";
	
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}
	
	// Build vnum to rnum map
	std::map<int, int> vnum_to_rnum;
	for (MobRnum i = 0; i <= top_of_mobt; i++)
	{
		vnum_to_rnum[mob_index[i].vnum] = i;
	}
	
	int feats_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int mob_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(mob_vnum)) continue;
		int feat_id = sqlite3_column_int(stmt, 1);
		
		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;
		
		CharData &mob = mob_proto[it->second];
		if (feat_id >= 0 && feat_id < static_cast<int>(mob.real_abils.Feats.size()))
		{
			mob.real_abils.Feats.set(feat_id);
			feats_set++;
		}
	}
	sqlite3_finalize(stmt);
	
	if (feats_set > 0)
	{
		log("   Set %d mob feats.", feats_set);
	}
}

void SqliteWorldDataSource::LoadMobSpells()
{
	const char *sql = "SELECT mob_vnum, spell_id, count FROM mob_spells";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	// Build vnum to rnum map
	std::map<int, int> vnum_to_rnum;
	for (MobRnum i = 0; i <= top_of_mobt; i++)
	{
		vnum_to_rnum[mob_index[i].vnum] = i;
	}

	int spells_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int mob_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(mob_vnum)) continue;
		int spell_id = sqlite3_column_int(stmt, 1);
		int count = sqlite3_column_int(stmt, 2);

		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;

		CharData &mob = mob_proto[it->second];
		if (spell_id >= 0 && spell_id < static_cast<int>(mob.real_abils.SplMem.size()))
		{
			// SplMem holds the memorised count; a known spell implies SplKnw too.
			mob.real_abils.SplMem[spell_id] = count > 0 ? count : 1;
			mob.real_abils.SplKnw[spell_id] = 1;
			spells_set++;
		}
	}
	sqlite3_finalize(stmt);
	
	if (spells_set > 0)
	{
		log("   Set %d mob spells.", spells_set);
	}
}

void SqliteWorldDataSource::LoadMobHelpers()
{
	// ORDER BY helper_order to preserve the original (possibly-repeated,
	// weighted) helper list order -- the table's primary key is now
	// (mob_vnum, helper_order), not (mob_vnum, helper_vnum), so a repeated
	// vnum round-trips correctly instead of being deduplicated.
	const char *sql = "SELECT mob_vnum, helper_vnum FROM mob_helpers ORDER BY mob_vnum, helper_order";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}
	
	// Build vnum to rnum map
	std::map<int, int> vnum_to_rnum;
	for (MobRnum i = 0; i <= top_of_mobt; i++)
	{
		vnum_to_rnum[mob_index[i].vnum] = i;
	}
	
	int helpers_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int mob_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(mob_vnum)) continue;
		int helper_vnum = sqlite3_column_int(stmt, 1);
		
		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;
		
		CharData &mob = mob_proto[it->second];
		mob.summon_helpers.push_back(helper_vnum);
		helpers_set++;
	}
	sqlite3_finalize(stmt);
	
	if (helpers_set > 0)
	{
		log("   Set %d mob helpers.", helpers_set);
	}
}

void SqliteWorldDataSource::LoadMobDestinations()
{
	const char *sql = "SELECT mob_vnum, dest_order, room_vnum FROM mob_destinations ORDER BY mob_vnum, dest_order";
	
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}
	
	// Build vnum to rnum map
	std::map<int, int> vnum_to_rnum;
	for (MobRnum i = 0; i <= top_of_mobt; i++)
	{
		vnum_to_rnum[mob_index[i].vnum] = i;
	}
	
	int destinations_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int mob_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(mob_vnum)) continue;
		int dest_order = sqlite3_column_int(stmt, 1);
		int room_vnum = sqlite3_column_int(stmt, 2);
		
		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;
		
		CharData &mob = mob_proto[it->second];
		if (dest_order >= 0 && dest_order < static_cast<int>(mob.mob_specials.dest.size()))
		{
			mob.mob_specials.dest[dest_order] = room_vnum;
			// dest_count drives GET_DEST / the movement-route logic; rows are
			// ordered by dest_order so this leaves dest_count == number of
			// destinations, matching the legacy parser (issue #3384).
			if (dest_order + 1 > mob.mob_specials.dest_count)
			{
				mob.mob_specials.dest_count = dest_order + 1;
			}
			destinations_set++;
		}
	}
	sqlite3_finalize(stmt);
	
	if (destinations_set > 0)
	{
		log("   Set %d mob destinations.", destinations_set);
	}
}

void SqliteWorldDataSource::LoadMobDeathLoad()
{
	// Table is created lazily on save; an old world.db may not have it yet, in
	// which case prepare fails and we simply load no death-load entries.
	const char *sql = "SELECT mob_vnum, obj_vnum, load_prob, load_type, spec_param "
		"FROM mob_death_load ORDER BY mob_vnum, load_order";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	std::map<int, int> vnum_to_rnum;
	for (MobRnum i = 0; i <= top_of_mobt; i++)
	{
		vnum_to_rnum[mob_index[i].vnum] = i;
	}

	int dl_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int mob_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(mob_vnum)) continue;
		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;

		dead_load::LoadingItem item;
		item.obj_vnum = sqlite3_column_int(stmt, 1);
		item.load_prob = sqlite3_column_int(stmt, 2);
		item.load_type = sqlite3_column_int(stmt, 3);
		item.spec_param = sqlite3_column_int(stmt, 4);
		mob_proto[it->second].dl_list.push_back(item);
		dl_set++;
	}
	sqlite3_finalize(stmt);

	if (dl_set > 0)
	{
		log("   Set %d mob death-load entries.", dl_set);
	}
}

// ============================================================================
// Object Loading
// ============================================================================

// Parses one row of the primary `objects` query into a freshly-built
// CObjectPrototype, WITHOUT touching obj_proto -- the caller (LoadObjects)
// collects these and the orchestrator (db.cpp) does that placement once,
// after possibly combining results from other sources too (composite).
LoadedObject SqliteWorldDataSource::LoadObjectRow(sqlite3_stmt *stmt)
{
	int vnum = sqlite3_column_int(stmt, 0);

	auto obj = std::make_shared<CObjectPrototype>(vnum);

	// Names
	obj->set_aliases(GetText(stmt, 1));
	obj->set_short_description(utils::colorLOW(GetText(stmt, 2)));
	obj->set_PName(grammar::ECase::kNom, utils::colorLOW(GetText(stmt, 2)));
	obj->set_PName(grammar::ECase::kGen, utils::colorLOW(GetText(stmt, 3)));
	obj->set_PName(grammar::ECase::kDat, utils::colorLOW(GetText(stmt, 4)));
	obj->set_PName(grammar::ECase::kAcc, utils::colorLOW(GetText(stmt, 5)));
	obj->set_PName(grammar::ECase::kIns, utils::colorLOW(GetText(stmt, 6)));
	obj->set_PName(grammar::ECase::kPre, utils::colorLOW(GetText(stmt, 7)));
	obj->set_description(utils::colorCAP(GetText(stmt, 8)));
	obj->set_action_description(GetText(stmt, 9));

	// Type
	int obj_type_id = sqlite3_column_int(stmt, 10);
	obj->set_type(static_cast<EObjType>(obj_type_id));

	// Material
	obj->set_material(static_cast<EObjMaterial>(sqlite3_column_int(stmt, 11)));

	// Values
	std::string val0 = GetText(stmt, 12);
	std::string val1 = GetText(stmt, 13);
	std::string val2 = GetText(stmt, 14);
	std::string val3 = GetText(stmt, 15);

	// Parse values - try as numbers

	obj->set_val(0, SafeStol(val0));
	obj->set_val(1, SafeStol(val1));
	obj->set_val(2, SafeStol(val2));
	obj->set_val(3, SafeStol(val3));

	// Physical properties
	obj->set_weight(sqlite3_column_int(stmt, 16));
	// Match Legacy: weight of containers must exceed current quantity
	if (obj->get_type() == EObjType::kLiquidContainer || obj->get_type() == EObjType::kFountain)
	{
		if (obj->get_weight() < obj->get_val(1))
		{
			obj->set_weight(obj->get_val(1) + 5);
		}
	}
	obj->set_cost(sqlite3_column_int(stmt, 17));
	obj->set_rent_off(sqlite3_column_int(stmt, 18));
	obj->set_rent_on(sqlite3_column_int(stmt, 19));
	obj->set_spec_param(sqlite3_column_int(stmt, 20));
	int max_dur = sqlite3_column_int(stmt, 21);
	int cur_dur = sqlite3_column_int(stmt, 22);
	obj->set_maximum_durability(max_dur);
	obj->set_current_durability(std::min(max_dur, cur_dur));  // Match Legacy: MIN(max, cur)
	int timer = sqlite3_column_int(stmt, 23);
	if (timer <= 0) {
		timer = ObjData::SEVEN_DAYS;  // Match Legacy: default timer is 7 days
	}
	if (timer > 99999) timer = 99999;  // Cap timer like Legacy
	obj->set_timer(timer);
	obj->set_spell(sqlite3_column_int(stmt, 24));
	obj->set_level(sqlite3_column_int(stmt, 25));
	obj->set_sex(static_cast<EGender>(sqlite3_column_int(stmt, 26)));
	obj->set_max_in_world(sqlite3_column_type(stmt, 27) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 27));
	obj->set_minimum_remorts(sqlite3_column_int(stmt, 28));

	return LoadedObject{vnum, obj, {}};
}

// The only object-loading entry point: `zone_filter` null means every zone.
// One unfiltered query (matches the old bulk behavior; `objects` has no
// zone_vnum column, so this filters by the vnum/100 convention like the
// others), filtered in memory if `zone_filter` is given. Returns results
// WITHOUT touching obj_proto and WITHOUT running the six child sub-loaders
// (flags/applies/skills/extra-values/triggers/extra-descriptions) -- the
// caller (GameLoader::BootWorld's orchestrator in db.cpp) places objects once
// (after possibly combining results from other sources too), then calls
// FinalizeZoneEntities() to run the child sub-loaders, scoped to
// m_processed_zone_vnums (populated below).
std::vector<LoadedObject> SqliteWorldDataSource::LoadObjects(const std::vector<int> *zone_filter)
{
	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for object loading.");
		return {};
	}

	const char *sql = "SELECT vnum, aliases, name_nom, name_gen, name_dat, name_acc, name_ins, name_pre, "
					  "short_desc, action_desc, obj_type_id, material, value0, value1, value2, value3, "
					  "weight, cost, rent_off, rent_on, spec_param, max_durability, cur_durability, "
					  "timer, spell, level, sex, max_in_world, minimum_remorts "
					  "FROM objects WHERE enabled = 1 ORDER BY vnum";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare object query: %s", sqlite3_errmsg(m_db));
		return {};
	}

	std::set<int> filter_set;
	if (zone_filter)
	{
		filter_set.insert(zone_filter->begin(), zone_filter->end());
	}

	std::vector<LoadedObject> result;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int vnum = sqlite3_column_int(stmt, 0);
		if (zone_filter && !filter_set.count(vnum / 100))
		{
			continue;
		}
		result.push_back(LoadObjectRow(stmt));
		m_processed_zone_vnums.insert(vnum / 100);
	}
	sqlite3_finalize(stmt);

	log("Loaded %zu objects from SQLite.", result.size());
	return result;
}

void SqliteWorldDataSource::LoadObjectFlags()
{
	const char *sql = "SELECT obj_vnum, flag_category, flag_name FROM obj_flags";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	int flags_set = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int obj_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(obj_vnum)) continue;
		std::string category = GetText(stmt, 1);
		std::string flag_name = GetText(stmt, 2);

		int rnum = obj_proto.get_rnum(obj_vnum);
		if (rnum < 0) continue;

		if (strcmp(category.c_str(), "extra") == 0)
		{
			auto flag_it = obj_extra_flag_map.find(flag_name);
			if (flag_it != obj_extra_flag_map.end())
			{
				obj_proto[rnum]->set_extra_flag(flag_it->second);
				flags_set++;
			}
			else if (flag_name.rfind("UNUSED_", 0) == 0)
			{
				// Handle UNUSED_XX flags - extract bit number and set directly
				int bit = std::stoi(flag_name.substr(7));
				size_t plane = bit / 30;
				int bit_in_plane = bit % 30;
				obj_proto[rnum]->toggle_extra_flag(plane, 1 << bit_in_plane);
				flags_set++;
			}
		}
		else if (strcmp(category.c_str(), "wear") == 0)
		{
			auto flag_it = obj_wear_flag_map.find(flag_name);
			if (flag_it != obj_wear_flag_map.end())
			{
				int wear_flags = obj_proto[rnum]->get_wear_flags();
				wear_flags |= to_underlying(flag_it->second);
				obj_proto[rnum]->set_wear_flags(wear_flags);
				flags_set++;
			}
			else if (flag_name.rfind("UNUSED_", 0) == 0)
			{
				// Handle UNUSED_XX flags - extract bit number and set directly
				int bit = std::stoi(flag_name.substr(7));
				int wear_flags = obj_proto[rnum]->get_wear_flags();
				wear_flags |= (1 << bit);
				obj_proto[rnum]->set_wear_flags(wear_flags);
			}
		}
		else if (strcmp(category.c_str(), "no") == 0)
		{
			auto flag_it = obj_no_flag_map.find(flag_name);
			if (flag_it != obj_no_flag_map.end())
			{
				obj_proto[rnum]->set_no_flag(flag_it->second);
				flags_set++;
			}
			else if (flag_name.rfind("UNUSED_", 0) == 0)
			{
				// Handle UNUSED_XX flags - extract bit number and set directly
				int bit = std::stoi(flag_name.substr(7));
				size_t plane = bit / 30;
				int bit_in_plane = bit % 30;
				obj_proto[rnum]->toggle_no_flag(plane, 1 << bit_in_plane);
				flags_set++;
			}
		}
		else if (strcmp(category.c_str(), "anti") == 0)
		{
			auto flag_it = obj_anti_flag_map.find(flag_name);
			if (flag_it != obj_anti_flag_map.end())
			{
				obj_proto[rnum]->set_anti_flag(flag_it->second);
				flags_set++;
			}
			else if (flag_name.rfind("UNUSED_", 0) == 0)
			{
				// Handle UNUSED_XX flags - extract bit number and set directly
				int bit = std::stoi(flag_name.substr(7));
				size_t plane = bit / 30;
				int bit_in_plane = bit % 30;
				obj_proto[rnum]->toggle_anti_flag(plane, 1 << bit_in_plane);
				flags_set++;
			}
		}
		else if (strcmp(category.c_str(), "affect") == 0)
		{
			auto flag_it = obj_affect_flag_map.find(flag_name);
			if (flag_it != obj_affect_flag_map.end())
			{
				obj_proto[rnum]->SetEWeaponAffectFlag(flag_it->second);
				flags_set++;
			}
			else if (flag_name.rfind("UNUSED_", 0) == 0)
			{
				// Handle UNUSED_XX flags - extract bit number and set directly
				int bit = std::stoi(flag_name.substr(7));
				size_t plane = bit / 30;
				int bit_in_plane = bit % 30;
				obj_proto[rnum]->toggle_affect_flag(plane, 1 << bit_in_plane);
				flags_set++;
			}
		}
	}
	sqlite3_finalize(stmt);

	log("   Set %d object flags.", flags_set);
}

void SqliteWorldDataSource::LoadObjectApplies()
{
	// ORDER BY id: the loop below fills the FIRST empty apply slot for each
	// row in turn, so an unordered result set can assign applies to the
	// wrong slot index for an object with 2+ applies (found via full-world
	// round-trip checksum testing).
	const char *sql = "SELECT obj_vnum, location_id, modifier FROM obj_applies ORDER BY id";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	int applies_loaded = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int obj_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(obj_vnum)) continue;
		int location_id = sqlite3_column_int(stmt, 1);
		int modifier = sqlite3_column_int(stmt, 2);

		int rnum = obj_proto.get_rnum(obj_vnum);
		if (rnum < 0) continue;

		int location = location_id;

		// Find first empty apply slot
		for (int i = 0; i < kMaxObjAffect; i++)
		{
			if (obj_proto[rnum]->get_affected(i).location == EApply::kNone)
			{
				obj_proto[rnum]->set_affected(i, static_cast<EApply>(location), modifier);
				applies_loaded++;
				break;
			}
		}
	}
	sqlite3_finalize(stmt);

	log("   Loaded %d object applies.", applies_loaded);
}

void SqliteWorldDataSource::LoadObjectSkills()
{
	const char *sql = "SELECT obj_vnum, skill_id, value FROM obj_skills";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	int skills_loaded = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int obj_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(obj_vnum)) continue;
		int skill_id = sqlite3_column_int(stmt, 1);
		int value = sqlite3_column_int(stmt, 2);

		int rnum = obj_proto.get_rnum(obj_vnum);
		if (rnum < 0) continue;

		// Mirrors the legacy `S` line handler; without it object skill
		// bonuses are silently dropped in the SQLite world (issue #3386).
		obj_proto[rnum]->set_skill(static_cast<ESkill>(skill_id), value);
		skills_loaded++;
	}
	sqlite3_finalize(stmt);

	log("   Loaded %d object skills.", skills_loaded);
}

void SqliteWorldDataSource::LoadObjectExtraValues()
{
	// Lazily-created table (see SaveObjects); an old world.db simply has none.
	const char *sql = "SELECT obj_vnum, vals FROM obj_extra_values";
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	int loaded = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int obj_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(obj_vnum)) continue;
		std::string vals = GetText(stmt, 1);
		int rnum = obj_proto.get_rnum(obj_vnum);
		if (rnum < 0 || vals.empty()) continue;

		// ObjVal::print_to_file prefixes a "Vals:" header line, but init_from_file
		// reads bare "key val" pairs and a leading non-numeric token aborts the
		// whole parse -- strip the header before handing it over.
		const char *body = vals.c_str();
		if (vals.compare(0, 5, "Vals:") == 0)
		{
			auto nl = vals.find('\n');
			if (nl != std::string::npos)
			{
				body += nl + 1;
			}
		}
		obj_proto[rnum]->init_values_from_file(body);
		loaded++;
	}
	sqlite3_finalize(stmt);

	log("   Loaded %d object extra-value sets.", loaded);
}

void SqliteWorldDataSource::LoadObjectTriggers()
{
	const char *sql = "SELECT entity_vnum, trigger_vnum FROM entity_triggers WHERE entity_type = 'obj' ORDER BY trigger_order";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int obj_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(obj_vnum)) continue;
		int trigger_vnum = sqlite3_column_int(stmt, 1);

		int rnum = obj_proto.get_rnum(obj_vnum);
		if (rnum >= 0)
		{
			AttachTriggerToObject(rnum, trigger_vnum, obj_vnum);
		}
	}
	sqlite3_finalize(stmt);
}

void SqliteWorldDataSource::LoadObjectExtraDescriptions()
{
	// ORDER BY id DESC -- see LoadRoomExtraDescriptions for why (same prepend
	// pattern below).
	const char *sql = "SELECT entity_vnum, keywords, description FROM extra_descriptions WHERE entity_type = 'obj' ORDER BY id DESC";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	int loaded = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int obj_vnum = sqlite3_column_int(stmt, 0);
		if (!IsZoneProcessed(obj_vnum)) continue;
		std::string keywords = GetText(stmt, 1);
		std::string description = GetText(stmt, 2);

		int rnum = obj_proto.get_rnum(obj_vnum);
		if (rnum < 0) continue;

		auto ex_desc = std::make_shared<ExtraDescription>();
		ex_desc->set_keyword(keywords);
		ex_desc->set_description(description);
		ex_desc->next = obj_proto[rnum]->get_ex_description();
		obj_proto[rnum]->set_ex_description(ex_desc);
		loaded++;
	}
	sqlite3_finalize(stmt);

	if (loaded > 0)
	{
		log("   Loaded %d object extra descriptions.", loaded);
	}

	// Post-processing to match Legacy loader behavior
	for (size_t i = 0; i < obj_proto.size(); ++i)
	{
		// Clear runtime flags that should not be in prototypes
		obj_proto[i]->unset_extraflag(EObjFlag::kTransformed);
		obj_proto[i]->unset_extraflag(EObjFlag::kTicktimer);

		// Objects with zone decay flags should have unlimited max_in_world
		if (obj_proto[i]->has_flag(EObjFlag::kZonedecay)
			|| obj_proto[i]->has_flag(EObjFlag::kRepopDecay))
		{
			obj_proto[i]->set_max_in_world(-1);
		}
	}
}

// ============================================================================
// Save Methods (read-only)
// ============================================================================

// ============================================================================
// Transaction helpers
// ============================================================================

// Depth 0->1 opens a real transaction; deeper calls open a named SAVEPOINT
// instead, so a BeginBulkWrite() wrapping many per-zone Save* calls collapses
// into a single real BEGIN/COMMIT (avoiding one fsync per zone per entity
// type), while a single nested Save* can still be rolled back on its own via
// RollbackTransaction() without undoing sibling zones already committed
// into the same outer transaction.
bool SqliteWorldDataSource::BeginTransaction()
{
	char *err_msg = nullptr;
	const std::string sql = (m_transaction_depth == 0)
		? "BEGIN TRANSACTION"
		: "SAVEPOINT sp" + std::to_string(m_transaction_depth);
	int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err_msg);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: Failed to begin transaction: %s", err_msg);
		sqlite3_free(err_msg);
		return false;
	}
	++m_transaction_depth;
	return true;
}

bool SqliteWorldDataSource::CommitTransaction()
{
	if (m_transaction_depth == 0)
	{
		log("SYSERR: CommitTransaction called with no active transaction");
		return false;
	}
	--m_transaction_depth;
	char *err_msg = nullptr;
	const std::string sql = (m_transaction_depth == 0)
		? "COMMIT"
		: "RELEASE sp" + std::to_string(m_transaction_depth);
	int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err_msg);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: Failed to commit transaction: %s", err_msg);
		sqlite3_free(err_msg);
		return false;
	}
	return true;
}

bool SqliteWorldDataSource::RollbackTransaction()
{
	if (m_transaction_depth == 0)
	{
		log("SYSERR: RollbackTransaction called with no active transaction");
		return false;
	}
	--m_transaction_depth;
	char *err_msg = nullptr;
	if (m_transaction_depth == 0)
	{
		int rc = sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, &err_msg);
		if (rc != SQLITE_OK)
		{
			log("SYSERR: Failed to rollback transaction: %s", err_msg);
			sqlite3_free(err_msg);
			return false;
		}
		return true;
	}

	// Nested: undo only this savepoint's writes, keep the outer transaction
	// (and any earlier sibling savepoints already released into it) intact.
	const std::string sp = "sp" + std::to_string(m_transaction_depth);
	int rc = sqlite3_exec(m_db, ("ROLLBACK TO " + sp).c_str(), nullptr, nullptr, &err_msg);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: Failed to roll back to savepoint: %s", err_msg);
		sqlite3_free(err_msg);
		return false;
	}
	rc = sqlite3_exec(m_db, ("RELEASE " + sp).c_str(), nullptr, nullptr, &err_msg);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: Failed to release savepoint: %s", err_msg);
		sqlite3_free(err_msg);
		return false;
	}
	return true;
}

bool SqliteWorldDataSource::ExecuteStatement(const std::string &sql, const std::string &operation)
{
	char *err_msg = nullptr;
	int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err_msg);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: Failed to %s: %s", operation.c_str(), err_msg);
		log("SYSERR: SQL: %s", sql.c_str());
		sqlite3_free(err_msg);
		return false;
	}
	return true;
}

// ============================================================================
// Save helper methods
// ============================================================================

void SqliteWorldDataSource::SaveZoneRecord(const ZoneData &zone)
{
	sqlite3_stmt *stmt = nullptr;
	const char *sql = 
		"REPLACE INTO zones (vnum, name, comment, location, author, description, "
		"first_room, top_room, mode, zone_type, zone_group, entrance, "
		"lifespan, reset_mode, reset_idle, under_construction, enabled) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1)";

	int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare zone insert: %s", sqlite3_errmsg(m_db));
		return;
	}

	// Bind values
	sqlite3_bind_int(stmt, 1, zone.vnum);
	BindTextKoi(stmt, 2, zone.name.c_str());
	BindTextKoi(stmt, 3, zone.comment.c_str());
	BindTextKoi(stmt, 4, zone.location.c_str());
	BindTextKoi(stmt, 5, zone.author.c_str());
	BindTextKoi(stmt, 6, zone.description.c_str());
	sqlite3_bind_int(stmt, 7, zone.vnum * 100);  // first_room
	sqlite3_bind_int(stmt, 8, zone.top);  // top_room
	sqlite3_bind_int(stmt, 9, zone.level);  // mode
	sqlite3_bind_int(stmt, 10, zone.type);  // zone_type
	sqlite3_bind_int(stmt, 11, zone.group);  // zone_group
	sqlite3_bind_int(stmt, 12, zone.entrance);
	sqlite3_bind_int(stmt, 13, zone.lifespan);
	sqlite3_bind_int(stmt, 14, zone.reset_mode);
	sqlite3_bind_int(stmt, 15, zone.reset_idle ? 1 : 0);
	sqlite3_bind_int(stmt, 16, zone.under_construction);

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE)
	{
		log("SYSERR: Failed to insert zone %d: %s", zone.vnum, sqlite3_errmsg(m_db));
	}

	sqlite3_finalize(stmt);
}

void SqliteWorldDataSource::SaveZoneGroups(int zone_vnum, const ZoneData &zone)
{
	// Delete existing groups
	std::string delete_sql = "DELETE FROM zone_groups WHERE zone_vnum = " + std::to_string(zone_vnum);
	ExecuteStatement(delete_sql, "delete zone groups");

	// Insert typeA groups
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql = "INSERT INTO zone_groups (zone_vnum, linked_zone_vnum, group_type) VALUES (?, ?, ?)";
	
	for (int i = 0; i < zone.typeA_count; ++i)
	{
		if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, zone_vnum);
			sqlite3_bind_int(stmt, 2, zone.typeA_list[i]);
			BindTextKoi(stmt, 3, "A");
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// Insert typeB groups
	for (int i = 0; i < zone.typeB_count; ++i)
	{
		if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, zone_vnum);
			sqlite3_bind_int(stmt, 2, zone.typeB_list[i]);
			BindTextKoi(stmt, 3, "B");
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}
}

void SqliteWorldDataSource::SaveZoneCommands(int zone_vnum, const struct reset_com *commands)
{
	if (!commands)
	{
		return;
	}

	// Delete existing commands
	std::string delete_sql = "DELETE FROM zone_commands WHERE zone_vnum = " + std::to_string(zone_vnum);
	ExecuteStatement(delete_sql, "delete zone commands");

	// The zone_commands schema is SEMANTIC (cmd_type + typed arg_* columns), and
	// LoadZoneCommands reads it back per command type. This writer is the exact
	// inverse of that loader. ResolveZoneCmdVnumArgsToRnums (db.cpp) also rewrote
	// cmd.argN from on-disk vnums into in-memory rnums at boot, so the entity
	// args are converted rnum->vnum here (like YamlWorldDataSource::SaveZone).
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql =
		"INSERT INTO zone_commands (zone_vnum, cmd_order, cmd_type, if_flag, "
		"arg_mob_vnum, arg_obj_vnum, arg_room_vnum, arg_trigger_vnum, arg_container_vnum, "
		"arg_max, arg_max_world, arg_max_room, arg_load_prob, arg_wear_pos_id, "
		"arg_direction_id, arg_state, arg_trigger_type, arg_context, "
		"arg_var_name, arg_var_value, arg_leader_mob_vnum, arg_follower_mob_vnum) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	auto mob_v = [](int rnum) -> int {
		if (rnum < 0 || rnum >= top_of_mobt) return rnum;
		return mob_index[rnum].vnum;
	};
	auto obj_v = [](int rnum) -> int {
		if (rnum < 0) return rnum;
		auto obj = obj_proto[rnum];
		return obj ? obj->get_vnum() : rnum;
	};
	auto room_v = [](int rnum) -> int {
		if (rnum < 0 || rnum > top_of_world || !world[rnum]) return rnum;
		return world[rnum]->vnum;
	};
	auto trig_v = [](int rnum) -> int {
		if (rnum < 0 || rnum >= top_of_trigt || !trig_index[rnum]) return rnum;
		return trig_index[rnum]->vnum;
	};

	int order = 0;
	for (int i = 0; commands[i].command != 'S'; ++i)
	{
		const struct reset_com &c = commands[i];
		// A/B are zone_groups; '*' (disabled) and unknown types are skipped, as
		// in YamlWorldDataSource::SaveZone, so they don't round-trip as garbage.
		if (c.command == 'A' || c.command == 'B')
		{
			continue;
		}

		const char *cmd_type = nullptr;
		int mobv = 0, objv = 0, roomv = 0, trigv = 0, contv = 0;
		int amax = 0, amaxw = 0, amaxr = 0, prob = 0, wear = 0, dir = 0, state = 0, ctx = 0;
		const char *trig_type = nullptr, *vname = nullptr, *vval = nullptr;
		int leadv = 0, follv = 0;
		char trig_type_buf[16];

		switch (c.command)
		{
		case 'M': cmd_type = "LOAD_MOB"; mobv = mob_v(c.arg1); amaxw = c.arg2;
			roomv = room_v(c.arg3); amaxr = c.arg4; break;
		case 'O': cmd_type = "LOAD_OBJ"; objv = obj_v(c.arg1); amax = c.arg2;
			roomv = (c.arg3 == kNowhere) ? c.arg3 : room_v(c.arg3); prob = c.arg4; break;
		case 'G': cmd_type = "GIVE_OBJ"; objv = obj_v(c.arg1); amax = c.arg2; prob = c.arg4; break;
		case 'E': cmd_type = "EQUIP_MOB"; objv = obj_v(c.arg1); amax = c.arg2; wear = c.arg3;
			prob = c.arg4; break;
		case 'P': cmd_type = "PUT_OBJ"; objv = obj_v(c.arg1); amax = c.arg2; contv = obj_v(c.arg3);
			prob = c.arg4; break;
		case 'D': cmd_type = "DOOR"; roomv = room_v(c.arg1); dir = c.arg2; state = c.arg3; break;
		case 'R': cmd_type = "REMOVE_OBJ"; roomv = room_v(c.arg1); objv = obj_v(c.arg2); break;
		case 'T': cmd_type = "TRIGGER"; trigv = trig_v(c.arg2);
			roomv = (c.arg1 == WLD_TRIGGER && c.arg3 != -1) ? room_v(c.arg3) : c.arg3;
			snprintf(trig_type_buf, sizeof(trig_type_buf), "%d", c.arg1); trig_type = trig_type_buf;
			break;
		case 'V': cmd_type = "VARIABLE"; ctx = c.arg2; roomv = c.arg3; vname = c.sarg1; vval = c.sarg2;
			snprintf(trig_type_buf, sizeof(trig_type_buf), "%d", c.arg1); trig_type = trig_type_buf;
			break;
		case 'Q': cmd_type = "EXTRACT_MOB"; mobv = mob_v(c.arg1); break;
		case 'F': cmd_type = "FOLLOW"; roomv = room_v(c.arg1); leadv = mob_v(c.arg2);
			follv = mob_v(c.arg3); break;
		case '*': // A command the resolver invalidated (unresolved target). Keep
			// it verbatim so the dead reset line round-trips like it does in YAML;
			// args are stored raw (no rnum->vnum conversion -- they never resolved).
			cmd_type = "DISABLED"; mobv = c.arg1; objv = c.arg2; roomv = c.arg3;
			trigv = c.arg4; break;
		default: continue;
		}

		if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, zone_vnum);
			sqlite3_bind_int(stmt, 2, order++);
			BindTextKoi(stmt, 3, cmd_type);
			sqlite3_bind_int(stmt, 4, c.if_flag);
			sqlite3_bind_int(stmt, 5, mobv);
			sqlite3_bind_int(stmt, 6, objv);
			sqlite3_bind_int(stmt, 7, roomv);
			sqlite3_bind_int(stmt, 8, trigv);
			sqlite3_bind_int(stmt, 9, contv);
			sqlite3_bind_int(stmt, 10, amax);
			sqlite3_bind_int(stmt, 11, amaxw);
			sqlite3_bind_int(stmt, 12, amaxr);
			sqlite3_bind_int(stmt, 13, prob);
			sqlite3_bind_int(stmt, 14, wear);
			sqlite3_bind_int(stmt, 15, dir);
			sqlite3_bind_int(stmt, 16, state);
			BindTextKoi(stmt, 17, trig_type);
			sqlite3_bind_int(stmt, 18, ctx);
			BindTextKoi(stmt, 19, vname);
			BindTextKoi(stmt, 20, vval);
			sqlite3_bind_int(stmt, 21, leadv);
			sqlite3_bind_int(stmt, 22, follv);

			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}
}

void SqliteWorldDataSource::SaveZone(int zone_rnum)
{
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveZone", zone_rnum);
		return;
	}

	if (!m_db)
	{
		log("SYSERR: Database not open for SaveZone");
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];

	if (!BeginTransaction())
	{
		return;
	}

	SaveZoneRecord(zone);
	SaveZoneGroups(zone.vnum, zone);
	SaveZoneCommands(zone.vnum, zone.cmd);

	if (!CommitTransaction())
	{
		RollbackTransaction();
		log("SYSERR: Failed to save zone %d", zone.vnum);
		return;
	}

	log("Saved zone %d to SQLite database", zone.vnum);
}


void SqliteWorldDataSource::SaveTriggerRecord(int trig_vnum, const Trigger *trig)
{
	if (!trig)
	{
		return;
	}

	// trigger_type_bindings' schema declares ON DELETE CASCADE from triggers,
	// but PRAGMA foreign_keys is never turned on anywhere in this engine (SQLite
	// defaults FK enforcement to off), so that CASCADE never actually fires --
	// delete it explicitly instead of relying on a constraint that's inert.
	// The primary row itself uses REPLACE (below) instead of DELETE+INSERT, so
	// it needs no separate delete.
	ExecuteStatement("DELETE FROM trigger_type_bindings WHERE trigger_vnum = " + std::to_string(trig_vnum),
					  "del trigger_type_bindings");

	// Insert trigger record. REPLACE, not a separate DELETE+INSERT -- vnum is
	// the primary key, so this atomically overwrites any existing row.
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql =
		"INSERT OR REPLACE INTO triggers (vnum, name, attach_type_id, narg, arglist, script, enabled) "
		"VALUES (?, ?, ?, ?, ?, ?, 1)";

	if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare trigger insert: %s", sqlite3_errmsg(m_db));
		return;
	}

	// Build script text from cmdlist. Mirror YamlWorldDataSource: an empty
	// command line is written as a single space (on reload ParseTriggerScript
	// sees a non-empty line and trims it back to "", preserving the node count)
	// and newlines join lines without a trailing one. Skipping empty lines here
	// dropped blank script lines and changed the command count on reload.
	std::string script_text;
	if (trig->cmdlist)
	{
		for (auto cmd = *trig->cmdlist; cmd; cmd = cmd->next)
		{
			script_text += cmd->cmd.empty() ? std::string(" ") : cmd->cmd;
			if (cmd->next)
			{
				script_text += "\n";
			}
		}
	}

	sqlite3_bind_int(stmt, 1, trig_vnum);
	BindTextKoi(stmt, 2, trig->get_name().c_str());
	sqlite3_bind_int(stmt, 3, trig->get_attach_type());
	sqlite3_bind_int(stmt, 4, GET_TRIG_NARG(trig));
	BindTextKoi(stmt, 5, trig->arglist.c_str());
	BindTextKoi(stmt, 6, script_text.c_str());

	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE)
	{
		log("SYSERR: Failed to insert trigger %d: %s", trig_vnum, sqlite3_errmsg(m_db));
	}
	sqlite3_finalize(stmt);

	// Insert trigger type bindings
	const char *binding_sql = "INSERT INTO trigger_type_bindings (trigger_vnum, type_char) VALUES (?, ?)";
	
	long trigger_type = GET_TRIG_TYPE(trig);
	for (int bit = 0; bit < 52; ++bit)  // 26 lowercase + 26 uppercase
	{
		if (trigger_type & (1L << bit))
		{
			char type_ch = (bit < 26) ? ('a' + bit) : ('A' + (bit - 26));
			
			if (sqlite3_prepare_v2(m_db, binding_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, trig_vnum);
				char ch_str[2] = {type_ch, '\0'};
				BindTextKoi(stmt, 2, ch_str);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}
}

bool SqliteWorldDataSource::SaveTriggers(int zone_rnum, int specific_vnum, int notify_level)
{
	(void)specific_vnum; // SQLite format always saves entire zone
	(void)notify_level; // SQLite saves don't use mudlog - errors go to log file
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveTriggers", zone_rnum);
		return false;
	}

	if (!m_db)
	{
		log("SYSERR: Database not open for SaveTriggers");
		return false;
	}

	const ZoneData &zone = zone_table[zone_rnum];

	if (!BeginTransaction())
	{
		return false;
	}

	// Triggers belong to a zone by vnum/100 (see db.cpp boot). Clear the whole
	// block first so a trigger that exists in the db but not in the source (e.g.
	// a legacy trigger dropped from YAML) is removed -- otherwise the stale row
	// shifts every later trigger rnum and corrupts zone-reset 'T'/'V' command
	// args on reload. The loop below re-inserts the triggers that still exist.
	const int trig_base = zone.vnum * 100;
	ExecuteStatement("DELETE FROM triggers WHERE vnum >= " + std::to_string(trig_base) +
		" AND vnum <= " + std::to_string(trig_base + 99), "del zone triggers");

	// Get trigger range for this zone
	TrgRnum first_trig = zone.RnumTrigsLocation.first;
	TrgRnum last_trig = zone.RnumTrigsLocation.second;

	if (first_trig == -1 || last_trig == -1)
	{
		// No triggers in the source; the block delete above already removed any
		// stale rows, so just commit that.
		CommitTransaction();
		return true;
	}

	int saved_count = 0;
	for (TrgRnum trig_rnum = first_trig; trig_rnum <= last_trig && trig_rnum < top_of_trigt; ++trig_rnum)
	{
		if (!trig_index[trig_rnum])
		{
			continue;
		}

		int trig_vnum = trig_index[trig_rnum]->vnum;
		Trigger *trig = trig_index[trig_rnum]->proto;

		if (!trig)
		{
			continue;
		}

		SaveTriggerRecord(trig_vnum, trig);
		++saved_count;
	}

	if (!CommitTransaction())
	{
		RollbackTransaction();
		log("SYSERR: Failed to save triggers for zone %d", zone.vnum);
		return false;
	}

	log("Saved %d triggers for zone %d", saved_count, zone.vnum);
	return true;
}


void SqliteWorldDataSource::SaveRoomRecord(RoomData *room)
{
	if (!room)
	{
		return;
	}

	int room_vnum = room->vnum;
	int zone_vnum = room->vnum / 100;  // Integer division gives zone vnum

	// Child tables have variable cardinality (0..N rows per room), so they
	// still need an explicit delete before re-inserting the new set -- unlike
	// the primary row (below), a row count mismatch between saves (e.g. an
	// exit removed) can't be expressed as a single REPLACE. FK ON DELETE
	// CASCADE isn't relied on here either way -- PRAGMA foreign_keys is never
	// turned on in this engine, so it wouldn't fire even if it were needed.
	const std::string rvd = std::to_string(room_vnum);
	ExecuteStatement("DELETE FROM room_exits WHERE room_vnum = " + rvd, "del room_exits");
	ExecuteStatement("DELETE FROM room_flags WHERE room_vnum = " + rvd, "del room_flags");
	ExecuteStatement("DELETE FROM entity_triggers WHERE entity_type = 'room' AND entity_vnum = " + rvd, "del room trigs");
	ExecuteStatement("DELETE FROM extra_descriptions WHERE entity_type = 'room' AND entity_vnum = " + rvd, "del room extra descs");

	// Insert room record. REPLACE, not a separate DELETE+INSERT -- vnum is the
	// primary key, so this atomically overwrites any existing row.
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql =
		"INSERT OR REPLACE INTO rooms (vnum, zone_vnum, name, description, sector_id, enabled) "
		"VALUES (?, ?, ?, ?, ?, 1)";

	if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare room insert: %s", sqlite3_errmsg(m_db));
		return;
	}

	sqlite3_bind_int(stmt, 1, room_vnum);
	sqlite3_bind_int(stmt, 2, zone_vnum);
	BindTextKoi(stmt, 3, room->name);
	
	std::string description = GlobalObjects::descriptions().get(room->description_num);
	BindTextKoi(stmt, 4, description.c_str());
	sqlite3_bind_int(stmt, 5, static_cast<int>(room->sector_type));

	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE)
	{
		log("SYSERR: Failed to insert room %d: %s", room_vnum, sqlite3_errmsg(m_db));
	}
	sqlite3_finalize(stmt);

	// Save room flags
	FlagData room_flags = room->read_flags();
	SaveFlagsToTable(m_db, "room_flags", "room_vnum", room_vnum, room_flags, room_flag_map);

	// Save room exits
	const char *exit_sql = 
		"INSERT INTO room_exits (room_vnum, direction_id, description, keywords, exit_flags, key_vnum, to_room, lock_complexity) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

	for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir)
	{
		if (!room->dir_option_proto[dir])
		{
			continue;
		}

		if (sqlite3_prepare_v2(m_db, exit_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, room_vnum);
			sqlite3_bind_int(stmt, 2, dir);
			
			if (!room->dir_option_proto[dir]->general_description.empty())
			{
				BindTextKoi(stmt, 3, room->dir_option_proto[dir]->general_description.c_str());
			}
			else
			{
				sqlite3_bind_null(stmt, 3);
			}

			// keyword and vkeyword (the accusative form used for open/close) are
			// stored joined by '|' -- ExitData::set_keywords splits them back on
			// load. Writing only the keyword lost the accusative form.
			if (room->dir_option_proto[dir]->keyword)
			{
				std::string kw(room->dir_option_proto[dir]->keyword);
				const char *vk = room->dir_option_proto[dir]->vkeyword;
				if (vk && strcmp(room->dir_option_proto[dir]->keyword, vk) != 0)
				{
					kw += '|';
					kw += vk;
				}
				BindTextKoi(stmt, 4, kw.c_str());
			}
			else
			{
				sqlite3_bind_null(stmt, 4);
			}

			// Save exit flags as string (numeric value)
			std::string exit_flags_str = std::to_string(room->dir_option_proto[dir]->exit_info);
			if (!exit_flags_str.empty() && exit_flags_str != "0")
			{
				BindTextKoi(stmt, 5, exit_flags_str.c_str());
			}
			else
			{
				sqlite3_bind_null(stmt, 5);
			}

			sqlite3_bind_int(stmt, 6, room->dir_option_proto[dir]->key);
			sqlite3_bind_int(stmt, 7, room->dir_option_proto[dir]->to_room() != kNowhere ? world[room->dir_option_proto[dir]->to_room()]->vnum : -1);
			sqlite3_bind_int(stmt, 8, room->dir_option_proto[dir]->lock_complexity);

			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// Save extra descriptions
	const char *extra_sql =
		"INSERT INTO extra_descriptions (entity_type, entity_vnum, keywords, description) "
		"VALUES ('room', ?, ?, ?)";

	for (auto exdesc = room->ex_description; exdesc; exdesc = exdesc->next)
	{
		if (sqlite3_prepare_v2(m_db, extra_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, room_vnum);
			BindTextKoi(stmt, 2, exdesc->keyword);
			BindTextKoi(stmt, 3, exdesc->description);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// Save room triggers
	const char *trig_sql = 
		"INSERT INTO entity_triggers (entity_type, entity_vnum, trigger_vnum, trigger_order) "
		"VALUES ('room', ?, ?, ?)";

	int trig_order = 0;
	// Use proto_script (the world-file trigger list), not the runtime
	// script_trig_list which is empty until zone resets attach the triggers --
	// the resync/dual-write runs before any reset. Matches the YAML backend.
	if (room->proto_script && !room->proto_script->empty())
	{
		for (auto trig_vnum : *room->proto_script)
		{
			if (trig_vnum <= 0)
			{
				continue;
			}
			if (sqlite3_prepare_v2(m_db, trig_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, room_vnum);
				sqlite3_bind_int(stmt, 2, trig_vnum);
				sqlite3_bind_int(stmt, 3, trig_order++);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}
}

void SqliteWorldDataSource::SaveRooms(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // SQLite format always saves entire zone
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveRooms", zone_rnum);
		return;
	}

	if (!m_db)
	{
		log("SYSERR: Database not open for SaveRooms");
		return;
	}

	const ZoneData &zone = zone_table[zone_rnum];
	
	// Get room range for this zone  
	RoomRnum first_room = zone.RnumRoomsLocation.first;
	RoomRnum last_room = zone.RnumRoomsLocation.second;

	if (first_room == -1 || last_room == -1)
	{
		log("Zone %d has no rooms to save", zone.vnum);
		return;
	}

	if (!BeginTransaction())
	{
		return;
	}

	int saved_count = 0;
	for (RoomRnum room_rnum = first_room; room_rnum <= last_room && room_rnum <= top_of_world; ++room_rnum)
	{
		RoomData *room = world[room_rnum];
		if (!room || room->vnum < zone.vnum * 100 || room->vnum > zone.top)
		{
			continue;
		}

		SaveRoomRecord(room);
		++saved_count;
	}

	if (!CommitTransaction())
	{
		RollbackTransaction();
		log("SYSERR: Failed to save rooms for zone %d", zone.vnum);
		return;
	}

	log("Saved %d rooms for zone %d", saved_count, zone.vnum);
}


void SqliteWorldDataSource::SaveMobRecord(int mob_vnum, CharData &mob)
{
	// Child tables have variable cardinality (0..N rows per mob), so they
	// still need an explicit delete before re-inserting the new set -- unlike
	// the primary row (below), a row count mismatch between saves can't be
	// expressed as a single REPLACE. FK ON DELETE CASCADE isn't relied on
	// either way -- PRAGMA foreign_keys is never turned on in this engine, so
	// declared CASCADEs never actually fire.
	//
	// mob_feats/mob_helpers/mob_destinations are keyed (mob_vnum, feat_id) /
	// (mob_vnum, helper_order) / (mob_vnum, dest_order) -- previously NOT
	// deleted here at all, so re-saving a mob whose helper/destination list
	// shrank, or whose value at a given order changed, hit a PRIMARY KEY
	// conflict on the unchecked INSERT below and silently kept the OLD row
	// (see SaveMobHelpers/SaveMobDestinations further down -- sqlite3_step's
	// return value there isn't checked). Checksums never caught this because
	// every resync this session re-saved identical content -- the bug only
	// bites when a mob's actual data changes between saves, e.g. via
	// medit/API.
	const std::string mvd = std::to_string(mob_vnum);
	ExecuteStatement("DELETE FROM mob_flags WHERE mob_vnum = " + mvd, "del mob_flags");
	ExecuteStatement("DELETE FROM mob_spells WHERE mob_vnum = " + mvd, "del mob_spells");
	ExecuteStatement("DELETE FROM mob_skills WHERE mob_vnum = " + mvd, "del mob_skills");
	ExecuteStatement("DELETE FROM mob_resistances WHERE mob_vnum = " + mvd, "del mob_resistances");
	ExecuteStatement("DELETE FROM mob_saves WHERE mob_vnum = " + mvd, "del mob_saves");
	ExecuteStatement("DELETE FROM mob_death_load WHERE mob_vnum = " + mvd, "del mob_death_load");
	ExecuteStatement("DELETE FROM mob_feats WHERE mob_vnum = " + mvd, "del mob_feats");
	ExecuteStatement("DELETE FROM mob_helpers WHERE mob_vnum = " + mvd, "del mob_helpers");
	ExecuteStatement("DELETE FROM mob_destinations WHERE mob_vnum = " + mvd, "del mob_destinations");
	ExecuteStatement("DELETE FROM entity_triggers WHERE entity_type = 'mob' AND entity_vnum = " + mvd, "del mob trigs");

	// Insert mob main record. REPLACE, not a separate DELETE+INSERT -- vnum is
	// the primary key, so this atomically overwrites any existing row.
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql =
		"INSERT OR REPLACE INTO mobs (vnum, aliases, name_nom, name_gen, name_dat, name_acc, name_ins, name_pre, "
		"short_desc, long_desc, alignment, mob_type, level, hitroll_penalty, armor, "
		"hp_dice_count, hp_dice_size, hp_bonus, dam_dice_count, dam_dice_size, dam_bonus, "
		"gold_dice_count, gold_dice_size, gold_bonus, experience, default_pos, start_pos, "
		"sex, size, height, weight, mob_class, race, "
		"attr_str, attr_dex, attr_int, attr_wis, attr_con, attr_cha, "
		"attr_str_add, hp_regen, armour_bonus, mana_regen, cast_success, morale, "
		"initiative_add, absorb, aresist, mresist, presist, bare_hand_attack, "
		"like_work, max_factor, extra_attack, mob_remort, special_bitvector, role, speed, enabled) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1)";

	if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare mob insert: %s", sqlite3_errmsg(m_db));
		return;
	}

	int col = 1;
	sqlite3_bind_int(stmt, col++, mob_vnum);
	
	// Names
	BindTextKoi(stmt, col++, GET_PC_NAME(&mob));
	BindTextKoi(stmt, col++, mob.player_data.PNames[grammar::ECase::kNom].c_str());
	BindTextKoi(stmt, col++, mob.player_data.PNames[grammar::ECase::kGen].c_str());
	BindTextKoi(stmt, col++, mob.player_data.PNames[grammar::ECase::kDat].c_str());
	BindTextKoi(stmt, col++, mob.player_data.PNames[grammar::ECase::kAcc].c_str());
	BindTextKoi(stmt, col++, mob.player_data.PNames[grammar::ECase::kIns].c_str());
	BindTextKoi(stmt, col++, mob.player_data.PNames[grammar::ECase::kPre].c_str());
	
	// Descriptions
	BindTextKoi(stmt, col++, mob.player_data.long_descr.c_str());
	BindTextKoi(stmt, col++, mob.player_data.description.c_str());
	
	// Base parameters
	sqlite3_bind_int(stmt, col++, alignment::GetAlignment(&mob));
	
	// Mob type (E or S)
	std::string mob_type = (mob.get_str() > 0) ? "E" : "S";
	BindTextKoi(stmt, col++, mob_type.c_str());
	
	// Stats
	sqlite3_bind_int(stmt, col++, mob.GetLevel());
	sqlite3_bind_int(stmt, col++, GET_HR(&mob));
	sqlite3_bind_int(stmt, col++, GET_AC(&mob));
	
	// HP dice (stored in mem_queue for dice, hit for bonus)
	sqlite3_bind_int(stmt, col++, mob.mem_queue.total);
	sqlite3_bind_int(stmt, col++, mob.mem_queue.stored);
	sqlite3_bind_int(stmt, col++, mob.get_hit());
	
	// Damage dice
	sqlite3_bind_int(stmt, col++, mob.mob_specials.damnodice);
	sqlite3_bind_int(stmt, col++, mob.mob_specials.damsizedice);
	sqlite3_bind_int(stmt, col++, mob.real_abils.damroll);
	
	// Gold dice
	sqlite3_bind_int(stmt, col++, mob.mob_specials.GoldNoDs);
	sqlite3_bind_int(stmt, col++, mob.mob_specials.GoldSiDs);
	sqlite3_bind_int(stmt, col++, currencies::GetHand(mob, currencies::kGold));
	
	// Experience
	sqlite3_bind_int(stmt, col++, mob.get_exp());
	
	// Positions and sex are stored as enum NAMES (the loader looks them up in
	// position_map/gender_map); writing the raw int made every reload fall back
	// to the default position/kMale.
	BindTextKoi(stmt, col++,
		EnumNameOrInt(position_map, static_cast<int>(mob.mob_specials.default_pos)).c_str());
	BindTextKoi(stmt, col++,
		EnumNameOrInt(position_map, static_cast<int>(mob.GetPosition())).c_str());

	BindTextKoi(stmt, col++,
		EnumNameOrInt(gender_map, static_cast<int>(mob.get_sex())).c_str());
	
	// Physical attributes
	sqlite3_bind_int(stmt, col++, GET_SIZE(&mob));
	sqlite3_bind_int(stmt, col++, GET_HEIGHT(&mob));
	sqlite3_bind_int(stmt, col++, GET_WEIGHT(&mob));
	
	// Class and race
	sqlite3_bind_int(stmt, col++, static_cast<int>(mob.GetClass()));
	sqlite3_bind_int(stmt, col++, static_cast<int>(GET_RACE(&mob)));
	
	// Attributes (E-spec)
	sqlite3_bind_int(stmt, col++, mob.get_str());
	sqlite3_bind_int(stmt, col++, mob.get_dex());
	sqlite3_bind_int(stmt, col++, mob.get_int());
	sqlite3_bind_int(stmt, col++, mob.get_wis());
	sqlite3_bind_int(stmt, col++, mob.get_con());
	sqlite3_bind_int(stmt, col++, mob.get_cha());
	
	// Enhanced E-spec fields
	sqlite3_bind_int(stmt, col++, mob.get_str_add());
	sqlite3_bind_int(stmt, col++, mob.add_abils.hitreg);
	sqlite3_bind_int(stmt, col++, mob.add_abils.armour);
	sqlite3_bind_int(stmt, col++, mob.add_abils.manareg);
	sqlite3_bind_int(stmt, col++, mob.add_abils.cast_success);
	sqlite3_bind_int(stmt, col++, mob.add_abils.morale);
	sqlite3_bind_int(stmt, col++, mob.add_abils.initiative_add);
	sqlite3_bind_int(stmt, col++, mob.add_abils.absorb);
	sqlite3_bind_int(stmt, col++, mob.add_abils.aresist);
	sqlite3_bind_int(stmt, col++, mob.add_abils.mresist);
	sqlite3_bind_int(stmt, col++, mob.add_abils.presist);
	sqlite3_bind_int(stmt, col++, mob.mob_specials.attack_type);
	sqlite3_bind_int(stmt, col++, mob.mob_specials.like_work);
	sqlite3_bind_int(stmt, col++, mob.mob_specials.MaxFactor);
	sqlite3_bind_int(stmt, col++, mob.mob_specials.extra_attack);
	sqlite3_bind_int(stmt, col++, mob.get_remort());
	
	// special_bitvector (FlagData as TEXT). FlagData::tascii APPENDS (it does
	// strlen(buf) first), so the buffer MUST start as an empty string -- otherwise
	// it serialises uninitialised stack garbage ahead of the flags. The YAML
	// backend does the same init.
	char special_buf[kMaxStringLength];
	special_buf[0] = '\0';
	mob.mob_specials.npc_flags.tascii(FlagData::kPlanesNumber, special_buf, sizeof(special_buf));
	if (special_buf[0] != '0' || special_buf[1] != 'a')
	{
		BindTextKoi(stmt, col++, special_buf);
	}
	else
	{
		sqlite3_bind_null(stmt, col++);
	}
	
	
	// Role (bitset<9> as TEXT)
	std::string role_str = mob.get_role().to_string();
	if (!role_str.empty() && role_str != "000000000")
	{
		BindTextKoi(stmt, col++, role_str.c_str());
	}
	else
	{
		sqlite3_bind_null(stmt, col++);
	}

	// Movement speed (issue #3384 class)
	sqlite3_bind_int(stmt, col++, mob.mob_specials.speed);

	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE)
	{
		log("SYSERR: Failed to insert mob %d: %s", mob_vnum, sqlite3_errmsg(m_db));
	}
	sqlite3_finalize(stmt);


	// Save mob action flags
	FlagData act_flags = mob.char_specials.saved.act;
	SaveFlagsToTable(m_db, "mob_flags", "mob_vnum", mob_vnum, act_flags, mob_action_flag_map, "action");

	// Save mob affect flags
	SaveAffectFlags(m_db, "mob_flags", "mob_vnum", mob_vnum, mob.char_specials.saved.affected_by, mob_affect_flag_map, "affect");


	// Save mob resistances
	const char *resist_sql = "INSERT INTO mob_resistances (mob_vnum, resist_type, value) VALUES (?, ?, ?)";
	for (size_t i = 0; i < mob.add_abils.apply_resistance.size(); ++i)
	{
		if (GET_RESIST(&mob, i) != 0)
		{
			if (sqlite3_prepare_v2(m_db, resist_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, i);
				sqlite3_bind_int(stmt, 3, GET_RESIST(&mob, i));
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}

	// Save mob saves
	const char *save_sql = "INSERT INTO mob_saves (mob_vnum, save_type, value) VALUES (?, ?, ?)";
	for (size_t i = 0; i < mob.add_abils.apply_saving_throw.size(); ++i)
	{
		if (mob.add_abils.apply_saving_throw[i] != 0)
		{
			if (sqlite3_prepare_v2(m_db, save_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, i);
				sqlite3_bind_int(stmt, 3, mob.add_abils.apply_saving_throw[i]);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}




	// Save mob skills. Use the raw trained-skill map (skill_level), NOT
	// GetSkill(): GetSkill() layers on instance context (equipment, affects,
	// room) and would persist a modified value. Mirrors SerializeMob/YAML.
	const char *skill_sql = "INSERT INTO mob_skills (mob_vnum, skill_id, value) VALUES (?, ?, ?)";
	for (const auto &kv : mob.GetCharSkills())
	{
		if (kv.second.skill_level > 0)
		{
			if (sqlite3_prepare_v2(m_db, skill_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, to_underlying(kv.first));
				sqlite3_bind_int(stmt, 3, kv.second.skill_level);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}

	// Save mob feats
	const char *feat_sql = "INSERT INTO mob_feats (mob_vnum, feat_id) VALUES (?, ?)";
	for (size_t feat_id = 0; feat_id < mob.real_abils.Feats.size(); ++feat_id)
	{
		if (mob.real_abils.Feats.test(feat_id))
		{
			if (sqlite3_prepare_v2(m_db, feat_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, feat_id);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}

	// Save mob spells. The canonical per-mob spell data is the memorised-count
	// array SplMem (this is what the checksum and the YAML backend serialise);
	// the previous code wrote presence from SplKnw and dropped the count.
	const char *spell_sql = "INSERT INTO mob_spells (mob_vnum, spell_id, count) VALUES (?, ?, ?)";
	for (size_t spell_id = 0; spell_id < mob.real_abils.SplMem.size(); ++spell_id)
	{
		int mem = mob.real_abils.SplMem[spell_id];
		if (mem > 0)
		{
			if (sqlite3_prepare_v2(m_db, spell_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, spell_id);
				sqlite3_bind_int(stmt, 3, mem);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}
	// Save mob helpers. helper_order (not helper_vnum) keys the primary key
	// so a repeated vnum (weighted random helper selection in YAML) round-trips
	// instead of silently colliding and being dropped on the second INSERT.
	const char *helper_sql = "INSERT INTO mob_helpers (mob_vnum, helper_order, helper_vnum) VALUES (?, ?, ?)";
	int helper_order = 0;
	for (const auto &helper_vnum : mob.summon_helpers)
	{
		if (helper_vnum != 0)
		{
			if (sqlite3_prepare_v2(m_db, helper_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, helper_order);
				sqlite3_bind_int(stmt, 3, helper_vnum);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
			++helper_order;
		}
	}
	// Save mob destinations. Room vnum 0 is a legitimate destination (issue
	// found via full-world checksum diff: mobs patrolling back to room 0 lost
	// their route on round-trip because this loop used to skip dest[i] == 0
	// as if it were an empty slot). Bound by dest_count, not dest.size(), so
	// unfilled trailing slots are correctly excluded without a value check.
	const char *dest_sql = "INSERT INTO mob_destinations (mob_vnum, dest_order, room_vnum) VALUES (?, ?, ?)";
	for (int i = 0; i < mob.mob_specials.dest_count && i < static_cast<int>(mob.mob_specials.dest.size()); ++i)
	{
		if (sqlite3_prepare_v2(m_db, dest_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, mob_vnum);
			sqlite3_bind_int(stmt, 2, i);
			sqlite3_bind_int(stmt, 3, mob.mob_specials.dest[i]);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// Save mob death-load (loot dropped into the corpse). Ordered, so the load
	// preserves the original sequence via load_order.
	const char *dl_sql = "INSERT INTO mob_death_load "
		"(mob_vnum, load_order, obj_vnum, load_prob, load_type, spec_param) VALUES (?, ?, ?, ?, ?, ?)";
	int dl_order = 0;
	for (const auto &dl : mob.dl_list)
	{
		if (sqlite3_prepare_v2(m_db, dl_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, mob_vnum);
			sqlite3_bind_int(stmt, 2, dl_order++);
			sqlite3_bind_int(stmt, 3, dl.obj_vnum);
			sqlite3_bind_int(stmt, 4, dl.load_prob);
			sqlite3_bind_int(stmt, 5, dl.load_type);
			sqlite3_bind_int(stmt, 6, dl.spec_param);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// Save mob triggers
	const char *trig_sql = 
		"INSERT INTO entity_triggers (entity_type, entity_vnum, trigger_vnum, trigger_order) "
		"VALUES ('mob', ?, ?, ?)";

	int trig_order = 0;
	// proto_script, not the runtime script_trig_list (empty before resets).
	if (mob.proto_script && !mob.proto_script->empty())
	{
		for (auto trig_vnum : *mob.proto_script)
		{
			if (trig_vnum <= 0)
			{
				continue;
			}
			if (sqlite3_prepare_v2(m_db, trig_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, trig_vnum);
				sqlite3_bind_int(stmt, 3, trig_order++);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}
}

void SqliteWorldDataSource::SaveMobs(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // SQLite format always saves entire zone
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveMobs", zone_rnum);
		return;
	}

	if (!m_db)
	{
		log("SYSERR: Database not open for SaveMobs");
		return;
	}

	// The Python converter never emitted a death-load table, so create it on
	// demand (older world.db files simply gain it on first resync).
	ExecuteStatement(
		"CREATE TABLE IF NOT EXISTS mob_death_load ("
		" mob_vnum INTEGER NOT NULL,"
		" load_order INTEGER NOT NULL,"
		" obj_vnum INTEGER NOT NULL,"
		" load_prob INTEGER NOT NULL DEFAULT 0,"
		" load_type INTEGER NOT NULL DEFAULT 0,"
		" spec_param INTEGER NOT NULL DEFAULT 0,"
		" PRIMARY KEY (mob_vnum, load_order))",
		"create mob_death_load");

	const ZoneData &zone = zone_table[zone_rnum];
	
	// Get mob range for this zone
	MobRnum first_mob = zone.RnumMobsLocation.first;
	MobRnum last_mob = zone.RnumMobsLocation.second;

	if (first_mob == -1 || last_mob == -1)
	{
		log("Zone %d has no mobs to save", zone.vnum);
		return;
	}

	if (!BeginTransaction())
	{
		return;
	}

	int saved_count = 0;
	for (MobRnum mob_rnum = first_mob; mob_rnum <= last_mob && mob_rnum <= top_of_mobt; ++mob_rnum)
	{
		if (!mob_index[mob_rnum].vnum)
		{
			continue;
		}

		int mob_vnum = mob_index[mob_rnum].vnum;
		CharData &mob = mob_proto[mob_rnum];

		SaveMobRecord(mob_vnum, mob);
		++saved_count;
	}

	if (!CommitTransaction())
	{
		RollbackTransaction();
		log("SYSERR: Failed to save mobs for zone %d", zone.vnum);
		return;
	}

	log("Saved %d mobs for zone %d", saved_count, zone.vnum);
}


void SqliteWorldDataSource::SaveObjectRecord(int obj_vnum, CObjectPrototype *obj)
{
	if (!obj)
	{
		return;
	}

	// Child tables have variable cardinality (0..N rows per object), so they
	// still need an explicit delete before re-inserting the new set -- unlike
	// the primary row (below), a row count mismatch between saves can't be
	// expressed as a single REPLACE. FK ON DELETE CASCADE isn't relied on
	// either way -- PRAGMA foreign_keys is never turned on in this engine, so
	// declared CASCADEs never actually fire.
	const std::string ov = std::to_string(obj_vnum);
	ExecuteStatement("DELETE FROM obj_applies WHERE obj_vnum = " + ov, "del obj_applies");
	ExecuteStatement("DELETE FROM obj_skills WHERE obj_vnum = " + ov, "del obj_skills");
	ExecuteStatement("DELETE FROM obj_flags WHERE obj_vnum = " + ov, "del obj_flags");
	ExecuteStatement("DELETE FROM obj_extra_values WHERE obj_vnum = " + ov, "del obj_extra_values");
	ExecuteStatement("DELETE FROM entity_triggers WHERE entity_type = 'obj' AND entity_vnum = " + ov, "del obj trigs");
	ExecuteStatement("DELETE FROM extra_descriptions WHERE entity_type = 'obj' AND entity_vnum = " + ov, "del obj extra descs");

	// Insert object main record. REPLACE, not a separate DELETE+INSERT -- vnum
	// is the primary key, so this atomically overwrites any existing row.
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql =
		"INSERT OR REPLACE INTO objects (vnum, aliases, name_nom, name_gen, name_dat, name_acc, name_ins, name_pre, "
		"short_desc, action_desc, obj_type_id, material, value0, value1, value2, value3, "
		"weight, cost, rent_off, rent_on, spec_param, max_durability, cur_durability, "
		"timer, spell, level, sex, max_in_world, minimum_remorts, enabled) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1)";

	if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare object insert: %s", sqlite3_errmsg(m_db));
		return;
	}

	int col = 1;
	sqlite3_bind_int(stmt, col++, obj_vnum);
	
	// Names
	BindTextKoi(stmt, col++, obj->get_aliases().c_str());
	BindTextKoi(stmt, col++, obj->get_PName(grammar::ECase::kNom).c_str());
	BindTextKoi(stmt, col++, obj->get_PName(grammar::ECase::kGen).c_str());
	BindTextKoi(stmt, col++, obj->get_PName(grammar::ECase::kDat).c_str());
	BindTextKoi(stmt, col++, obj->get_PName(grammar::ECase::kAcc).c_str());
	BindTextKoi(stmt, col++, obj->get_PName(grammar::ECase::kIns).c_str());
	BindTextKoi(stmt, col++, obj->get_PName(grammar::ECase::kPre).c_str());
	
	// Descriptions
	BindTextKoi(stmt, col++, obj->get_description().c_str());
	BindTextKoi(stmt, col++, obj->get_action_description().c_str());
	
	// Type and material
	sqlite3_bind_int(stmt, col++, to_underlying(obj->get_type()));
	sqlite3_bind_int(stmt, col++, to_underlying(obj->get_material()));
	
	// Values (store as strings)
	std::string val0 = std::to_string(obj->get_val(0));
	std::string val1 = std::to_string(obj->get_val(1));
	std::string val2 = std::to_string(obj->get_val(2));
	std::string val3 = std::to_string(obj->get_val(3));
	BindTextKoi(stmt, col++, val0.c_str());
	BindTextKoi(stmt, col++, val1.c_str());
	BindTextKoi(stmt, col++, val2.c_str());
	BindTextKoi(stmt, col++, val3.c_str());
	
	// Physical properties
	sqlite3_bind_int(stmt, col++, obj->get_weight());
	sqlite3_bind_int(stmt, col++, obj->get_cost());
	sqlite3_bind_int(stmt, col++, obj->get_rent_off());
	sqlite3_bind_int(stmt, col++, obj->get_rent_on());
	sqlite3_bind_int(stmt, col++, obj->get_spec_param());
	sqlite3_bind_int(stmt, col++, obj->get_maximum_durability());
	sqlite3_bind_int(stmt, col++, obj->get_current_durability());
	sqlite3_bind_int(stmt, col++, obj->get_timer());
	sqlite3_bind_int(stmt, col++, to_underlying(obj->get_spell()));
	sqlite3_bind_int(stmt, col++, obj->get_level());
	sqlite3_bind_int(stmt, col++, to_underlying(obj->get_sex()));
	sqlite3_bind_int(stmt, col++, obj->get_max_in_world());
	sqlite3_bind_int(stmt, col++, obj->get_minimum_remorts());

	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE)
	{
		log("SYSERR: Failed to insert object %d: %s", obj_vnum, sqlite3_errmsg(m_db));
	}
	sqlite3_finalize(stmt);


	// Save object flags by category. Only extra (with an empty category!) and
	// wear used to be written -- anti/no/affect were dropped entirely, so those
	// flags survived a round-trip only by accident via stale converter rows.
	// SaveFlagsToTable now emits UNUSED_<bit> for unnamed bits, making each
	// category lossless.
	SaveFlagsToTable(m_db, "obj_flags", "obj_vnum", obj_vnum, obj->get_extra_flags(), obj_extra_flag_map, "extra");
	SaveFlagsToTable(m_db, "obj_flags", "obj_vnum", obj_vnum, obj->get_anti_flags(), obj_anti_flag_map, "anti");
	SaveFlagsToTable(m_db, "obj_flags", "obj_vnum", obj_vnum, obj->get_no_flags(), obj_no_flag_map, "no");
	SaveFlagsToTable(m_db, "obj_flags", "obj_vnum", obj_vnum, obj->get_affect_flags(), obj_affect_flag_map, "affect");
	// Save object wear flags (a flat 32-bit field, not FlagData planes)
	int wear_flags = obj->get_wear_flags();
	sqlite3_stmt *wear_stmt = nullptr;
	const char *wear_sql = "INSERT INTO obj_flags (obj_vnum, flag_category, flag_name) VALUES (?, 'wear', ?)";
	for (int bit = 0; bit < 32; ++bit)
	{
		if (wear_flags & (1 << bit))
		{
			Bitvector bit_value = (1 << bit);
			std::string flag_name = ReverseLookupFlag(obj_wear_flag_map, bit_value);
			if (flag_name.empty())
			{
				flag_name = "UNUSED_" + std::to_string(bit);
			}
			if (sqlite3_prepare_v2(m_db, wear_sql, -1, &wear_stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(wear_stmt, 1, obj_vnum);
				BindTextKoi(wear_stmt, 2, flag_name.c_str());
				sqlite3_step(wear_stmt);
				sqlite3_finalize(wear_stmt);
			}
		}
	}


	// Save object applies (affects)
	const char *apply_sql = "INSERT INTO obj_applies (obj_vnum, location_id, modifier) VALUES (?, ?, ?)";
	for (int i = 0; i < kMaxObjAffect; ++i)
	{
		if (obj->get_affected(i).location != EApply::kNone)
		{
			if (sqlite3_prepare_v2(m_db, apply_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, obj_vnum);
				sqlite3_bind_int(stmt, 2, to_underlying(obj->get_affected(i).location));
				sqlite3_bind_int(stmt, 3, obj->get_affected(i).modifier);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}

	// Save object skills (legacy `S` lines, issue #3386)
	const char *obj_skill_sql = "INSERT OR REPLACE INTO obj_skills (obj_vnum, skill_id, value) VALUES (?, ?, ?)";
	for (const auto &kv : obj->get_skills())
	{
		if (sqlite3_prepare_v2(m_db, obj_skill_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, obj_vnum);
			sqlite3_bind_int(stmt, 2, to_underlying(kv.first));
			sqlite3_bind_int(stmt, 3, kv.second);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// Save object extra (V-line) values -- the ObjVal map, separate from the four
	// positional value0..3 columns. Stored as the same text the legacy/YAML
	// serialiser produces so init_values_from_file can reload it verbatim.
	{
		std::string vals = obj->serialize_values();
		const char *ev_sql = "INSERT OR REPLACE INTO obj_extra_values (obj_vnum, vals) VALUES (?, ?)";
		if (!vals.empty() && sqlite3_prepare_v2(m_db, ev_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, obj_vnum);
			BindTextKoi(stmt, 2, vals.c_str());
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// Save object extra descriptions
	const char *extra_sql =
		"INSERT INTO extra_descriptions (entity_type, entity_vnum, keywords, description) "
		"VALUES ('obj', ?, ?, ?)";

	for (auto exdesc = obj->get_ex_description(); exdesc; exdesc = exdesc->next)
	{
		if (sqlite3_prepare_v2(m_db, extra_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, obj_vnum);
			BindTextKoi(stmt, 2, exdesc->keyword);
			BindTextKoi(stmt, 3, exdesc->description);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// Save object triggers
	const char *trig_sql = 
		"INSERT INTO entity_triggers (entity_type, entity_vnum, trigger_vnum, trigger_order) "
		"VALUES ('obj', ?, ?, ?)";

	int trig_order = 0;
	for (const auto &trig_vnum : obj->get_proto_script())
	{
		if (trig_vnum > 0)
		{
			if (sqlite3_prepare_v2(m_db, trig_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, obj_vnum);
				sqlite3_bind_int(stmt, 2, trig_vnum);
				sqlite3_bind_int(stmt, 3, trig_order++);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}
}

void SqliteWorldDataSource::SaveObjects(int zone_rnum, int specific_vnum)
{
	(void)specific_vnum; // SQLite format always saves entire zone
	if (zone_rnum < 0 || zone_rnum >= static_cast<int>(zone_table.size()))
	{
		log("SYSERR: Invalid zone_rnum %d for SaveObjects", zone_rnum);
		return;
	}

	if (!m_db)
	{
		log("SYSERR: Database not open for SaveObjects");
		return;
	}

	// Extra (V-line) object values had no home in the converter schema.
	ExecuteStatement(
		"CREATE TABLE IF NOT EXISTS obj_extra_values ("
		" obj_vnum INTEGER PRIMARY KEY,"
		" vals TEXT NOT NULL)",
		"create obj_extra_values");

	const ZoneData &zone = zone_table[zone_rnum];
	int zone_vnum = zone.vnum;

	if (!BeginTransaction())
	{
		return;
	}

	// Iterate through all objects and save those in this zone's vnum range
	int saved_count = 0;
	int start_vnum = zone_vnum * 100;
	int end_vnum = zone.top;

	for (const auto &[obj_vnum, obj_rnum] : obj_proto.vnum2index())
	{
		if (obj_vnum < start_vnum || obj_vnum > end_vnum)
		{
			continue;
		}

		SaveObjectRecord(obj_vnum, obj_proto[obj_rnum].get());
		++saved_count;
	}

	if (!CommitTransaction())
	{
		RollbackTransaction();
		log("SYSERR: Failed to save objects for zone %d", zone_vnum);
		return;
	}

	log("Saved %d objects for zone %d", saved_count, zone_vnum);
}

std::unique_ptr<IWorldDataSource> CreateSqliteDataSource(const std::string &db_path)
{
	return std::make_unique<SqliteWorldDataSource>(db_path);
}

} // namespace world_loader

#endif // HAVE_SQLITE

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
