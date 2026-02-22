// Part of Bylins http://www.mud.ru
// SQLite world data source implementation

#ifdef HAVE_SQLITE

#include "sqlite_world_data_source.h"
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
#include "engine/scripting/dg_olc.h"
#include "gameplay/affects/affect_contants.h"
#include "gameplay/skills/skills.h"

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
	{"kSpec", EMobFlag::kSpec},
	{"kSentinel", EMobFlag::kSentinel},
	{"kScavenger", EMobFlag::kScavenger},
	{"kIsNpc", EMobFlag::kNpc},
	{"kNpc", EMobFlag::kNpc},
	{"kAware", EMobFlag::kAware},
	{"kAggressive", EMobFlag::kAgressive},
	{"kAgressive", EMobFlag::kAgressive},
	{"kStayZone", EMobFlag::kStayZone},
	{"kWimpy", EMobFlag::kWimpy},
	{"kAgressiveDay", EMobFlag::kAgressiveDay},
	{"kAggressiveNight", EMobFlag::kAggressiveNight},
	{"kAgressiveFullmoon", EMobFlag::kAgressiveFullmoon},
	{"kMemory", EMobFlag::kMemory},
	{"kHelper", EMobFlag::kHelper},
	{"kNoCharm", EMobFlag::kNoCharm},
	{"kNoSummoned", EMobFlag::kNoSummon},
	{"kNoSummon", EMobFlag::kNoSummon},
	{"kNoSleep", EMobFlag::kNoSleep},
	{"kNoBash", EMobFlag::kNoBash},
	{"kNoBlind", EMobFlag::kNoBlind},
	{"kMounting", EMobFlag::kMounting},
	{"kNoHolder", EMobFlag::kNoHold},
	{"kNoHold", EMobFlag::kNoHold},
	{"kNoSilence", EMobFlag::kNoSilence},
	{"kAgressiveMono", EMobFlag::kAgressiveMono},
	{"kAgressivePoly", EMobFlag::kAgressivePoly},
	{"kNoFear", EMobFlag::kNoFear},
	{"kIgnoresFear", EMobFlag::kNoFear},
	{"kNoGroup", EMobFlag::kNoGroup},
	{"kCorpse", EMobFlag::kCorpse},
	{"kLooter", EMobFlag::kLooter},
	{"kLooting", EMobFlag::kLooter},
	{"kProtected", EMobFlag::kProtect},
	{"kProtect", EMobFlag::kProtect},
	{"kDeleted", EMobFlag::kMobDeleted},
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
	{"kHorde", EMobFlag::kHorde},
	{"kClone", EMobFlag::kClone},
	{"kTutelar", EMobFlag::kTutelar},
	{"kMentalShadow", EMobFlag::kMentalShadow},
	{"kSummoner", EMobFlag::kSummoned},
	{"kFireCreature", EMobFlag::kFireBreath},
	{"kWaterCreature", EMobFlag::kSwimming},
	{"kEarthCreature", EMobFlag::kNoBash},
	{"kAirCreature", EMobFlag::kFlying},
	{"kNoTrack", EMobFlag::kAware},
	{"kNoTerrainAttack", EMobFlag::kNoFight},
	{"kFreemaker", EMobFlag::kSpec},
	{"kProgrammedLootGroup", EMobFlag::kLooter},
	{"kScrStay", EMobFlag::kSentinel},
	{"kRacing", EMobFlag::kSwimming},
	{"kAggressive_Mob", EMobFlag::kAgressive},
};

// Mob affect flags mapping (EAffect values)
static std::unordered_map<std::string, Bitvector> mob_affect_flag_map = {
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
	{"kSneak", to_underlying(EAffect::kSneak)},
	{"kHide", to_underlying(EAffect::kHide)},
	{"kCharmed", to_underlying(EAffect::kCharmed)},
	{"kHold", to_underlying(EAffect::kHold)},
	{"kFly", to_underlying(EAffect::kFly)},
	{"kFlying", to_underlying(EAffect::kFly)},
	{"kSilence", to_underlying(EAffect::kSilence)},
	{"kAwarness", to_underlying(EAffect::kAwarness)},
	{"kBlink", to_underlying(EAffect::kBlink)},
	{"kHorse", to_underlying(EAffect::kHorse)},
	{"kNoFlee", to_underlying(EAffect::kNoFlee)},
	{"kHelper", to_underlying(EAffect::kHelper)},
	// Aliases from database to appropriate flags
	{"kAggressive", to_underlying(EAffect::kNoFlee)},
	{"kScavenger", to_underlying(EAffect::kDetectLife)},
	{"kIsNpc", 0},  // Not an affect
	{"kProtected", to_underlying(EAffect::kSanctuary)},
	{"kNoFear", to_underlying(EAffect::kNoFlee)},
	{"kAware", to_underlying(EAffect::kAwarness)},
	{"kScrStay", 0},
	{"kStayZone", 0},
	{"kWimpy", 0},
	{"kNoSummoned", 0},
	{"kNoSleep", 0},
	{"kNoBlind", 0},
	{"kNoCharm", 0},
	{"kSentinel", 0},
	{"kSpec", 0},
	{"kDeleted", 0},
	{"kSwimming", to_underlying(EAffect::kWaterWalk)},
	{"kWaterCreature", to_underlying(EAffect::kWaterWalk)},
	{"kFireCreature", 0},
	{"kEarthCreature", 0},
	{"kAirCreature", to_underlying(EAffect::kFly)},
	{"kMounting", to_underlying(EAffect::kHorse)},
	{"kMemory", 0},
	{"kNoHolder", 0},
	{"kNoSilence", 0},
	{"kClone", 0},
	{"kFreemaker", 0},
	{"kProgrammedLootGroup", 0},
	{"kNoMagicTerrainAttack", 0},
	{"kRacing", 0},
	{"kAggressive_Mob", 0},
	{"kIgnoresFear", 0},
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
	{"kSetItem", EObjFlag::KSetItem},
	{"kNofail", EObjFlag::KNofail},
	{"kNamed", EObjFlag::kNamed},
	{"kBloody", EObjFlag::kBloody},
	{"kQuestItem", EObjFlag::kQuestItem},
	{"k2inlaid", EObjFlag::k2inlaid},
	{"k3inlaid", EObjFlag::k3inlaid},
	{"kNopour", EObjFlag::kNopour},
	{"kUnique", EObjFlag::kUnique},
	{"kTransformed", EObjFlag::kTransformed},
	{"kNoRentTimer", EObjFlag::kNoRentTimer},
	{"kLimitedTimer", EObjFlag::KLimitedTimer},
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
	{"kWizard", ENoFlag::kWIzard},
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

// Save flags helper (template version)
template<typename T>
void SaveFlagsToTable(sqlite3 *db, const std::string &table_name, const std::string &vnum_col,
                      int vnum, const FlagData &flags,
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
				
				if (!flag_name.empty())
				{
					if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
					{
						int col = 1;
						sqlite3_bind_int(stmt, col++, vnum);
						if (!category.empty())
						{
							sqlite3_bind_text(stmt, col++, category.c_str(), -1, SQLITE_TRANSIENT);
						}
						sqlite3_bind_text(stmt, col++, flag_name.c_str(), -1, SQLITE_TRANSIENT);
						sqlite3_step(stmt);
						sqlite3_finalize(stmt);
					}
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

	int rc = sqlite3_open_v2(m_db_path.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr);
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
	utf8_to_koi(input, output);
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
		if (strcmp(cmd_type.c_str(), "LOAD_MOB") == 0)
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

void SqliteWorldDataSource::LoadTriggers()
{
	log("Loading triggers from SQLite database.");

	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for trigger loading.");
		return;
	}

	int trig_count = GetCount("triggers");
	if (trig_count == 0)
	{
		log("No triggers found in SQLite database.");
		return;
	}

	// Allocate trig_index
	CREATE(trig_index, trig_count);
	log("   %d triggers.", trig_count);

	const char *sql = "SELECT t.vnum, t.name, t.attach_type_id, GROUP_CONCAT(ttb.type_char, '') AS type_chars, t.narg, t.arglist, t.script "
					  "FROM triggers t LEFT JOIN trigger_type_bindings ttb ON t.vnum = ttb.trigger_vnum WHERE t.enabled = 1 GROUP BY t.vnum ORDER BY t.vnum";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare trigger query: %s", sqlite3_errmsg(m_db));
		return;
	}

	top_of_trigt = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int vnum = sqlite3_column_int(stmt, 0);
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
		

		// Create trigger with proper constructor (keep empty name same as Legacy)
		auto trig = new Trigger(top_of_trigt, std::move(name), attach_type, trigger_type);
		GET_TRIG_NARG(trig) = narg;
		trig->arglist = arglist;


		// Parse script into cmdlist (uses base class method)
		ParseTriggerScript(trig, script);

		// Create index entry (uses base class method)
		CreateTriggerIndex(vnum, trig);

	}
	sqlite3_finalize(stmt);

	log("Loaded %d triggers from SQLite.", top_of_trigt);
}

// ============================================================================
// Room Loading
// ============================================================================

void SqliteWorldDataSource::LoadRooms()
{
	log("Loading rooms from SQLite database.");

	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for room loading.");
		return;
	}

	int room_count = GetCount("rooms");
	if (room_count == 0)
	{
		log("No rooms found in SQLite database.");
		return;
	}

	// Create kNowhere room first
	world.push_back(new RoomData);
	top_of_world = kNowhere;

	log("   %d rooms, %zd bytes.", room_count, sizeof(RoomData) * room_count);

	const char *sql = "SELECT vnum, zone_vnum, name, description, sector_id FROM rooms WHERE enabled = 1 ORDER BY vnum";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare room query: %s", sqlite3_errmsg(m_db));
		return;
	}

	// Zone rnum - increments as we process rooms in vnum order (same as Legacy)
	ZoneRnum zone_rn = 0;

	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int vnum = sqlite3_column_int(stmt, 0);
		[[maybe_unused]] int zone_vnum = sqlite3_column_int(stmt, 1);
		std::string name = GetText(stmt, 2);
		std::string description = GetText(stmt, 3);
		int sector_id = sqlite3_column_int(stmt, 4);

		auto room = new RoomData;
		room->vnum = vnum;
		// Apply UPPER to first character (same as Legacy loader)
		if (!name.empty()) { name[0] = UPPER(name[0]); }
		room->set_name(name);

		// Set description
		if (!description.empty())
		{
			room->description_num = GlobalObjects::descriptions().add(description);
		}

		// Set zone_rn by finding zone that contains this vnum (same as Legacy)
		while (vnum > zone_table[zone_rn].top)
		{
			if (++zone_rn >= static_cast<ZoneRnum>(zone_table.size()))
			{
				log("SYSERR: Room %d is outside of any zone.", vnum);
				break;
			}
		}
		if (zone_rn < static_cast<ZoneRnum>(zone_table.size()))
		{
			room->zone_rn = zone_rn;
			if (zone_table[zone_rn].RnumRoomsLocation.first == -1)
			{
				zone_table[zone_rn].RnumRoomsLocation.first = top_of_world + 1;
			}
			zone_table[zone_rn].RnumRoomsLocation.second = top_of_world + 1;
		}

		room->sector_type = static_cast<ESector>(sector_id);

		world.push_back(room);
		top_of_world++;
	}
	sqlite3_finalize(stmt);

	// Build room vnum to rnum map for exits
	std::map<int, int> room_vnum_to_rnum;
	for (RoomRnum i = kFirstRoom; i <= top_of_world; i++)
	{
		room_vnum_to_rnum[world[i]->vnum] = i;
	}

	// Load room flags
	LoadRoomFlags(room_vnum_to_rnum);

	// Load room exits
	LoadRoomExits(room_vnum_to_rnum);

	// Load room triggers
	LoadRoomTriggers(room_vnum_to_rnum);

	// Load room extra descriptions
	LoadRoomExtraDescriptions(room_vnum_to_rnum);


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
	const char *sql = "SELECT entity_vnum, keywords, description FROM extra_descriptions WHERE entity_type = 'room'";

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

void SqliteWorldDataSource::LoadMobs()
{
	log("Loading mobs from SQLite database.");

	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for mob loading.");
		return;
	}

	int mob_count = GetCount("mobs");
	if (mob_count == 0)
	{
		log("No mobs found in SQLite database.");
		return;
	}

	// Allocate like PrepareGlobalStructures
	mob_proto = new CharData[mob_count];
	CREATE(mob_index, mob_count);
	log("   %d mobs, %zd bytes in index, %zd bytes in prototypes.",
		mob_count, sizeof(IndexData) * mob_count, sizeof(CharData) * mob_count);

	// Build zone vnum to rnum map
	std::map<int, int> zone_vnum_to_rnum;
	for (size_t i = 0; i < zone_table.size(); i++)
	{
		zone_vnum_to_rnum[zone_table[i].vnum] = i;
	}

	const char *sql = "SELECT vnum, aliases, name_nom, name_gen, name_dat, name_acc, name_ins, name_pre, "
					  "short_desc, long_desc, alignment, mob_type, level, hitroll_penalty, armor, "
					  "hp_dice_count, hp_dice_size, hp_bonus, dam_dice_count, dam_dice_size, dam_bonus, "
					  "gold_dice_count, gold_dice_size, gold_bonus, experience, default_pos, start_pos, "
					  "sex, size, height, weight, mob_class, race, "
					  "attr_str, attr_dex, attr_int, attr_wis, attr_con, attr_cha, "
					  "attr_str_add, hp_regen, armour_bonus, mana_regen, cast_success, morale, "
					  "initiative_add, absorb, aresist, mresist, presist, bare_hand_attack, "
					  "like_work, max_factor, extra_attack, mob_remort, special_bitvector, role "
					  "FROM mobs WHERE enabled = 1 ORDER BY vnum";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare mob query: %s", sqlite3_errmsg(m_db));
		return;
	}

	top_of_mobt = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int vnum = sqlite3_column_int(stmt, 0);
		CharData &mob = mob_proto[top_of_mobt];

		// Initialize mob
		mob.player_specials = player_special_data::s_for_mobiles;
		mob.SetNpcAttribute(true);
		mob.player_specials->saved.NameGod = 1001; // Default for Russian name declension
		mob.set_move(100);
		mob.set_max_move(100);

		// Names
		mob.SetCharAliases(GetText(stmt, 1));
		mob.set_npc_name(GetText(stmt, 2));
		mob.player_data.PNames[ECase::kNom] = GetText(stmt, 2);
		mob.player_data.PNames[ECase::kGen] = GetText(stmt, 3);
		mob.player_data.PNames[ECase::kDat] = GetText(stmt, 4);
		mob.player_data.PNames[ECase::kAcc] = GetText(stmt, 5);
		mob.player_data.PNames[ECase::kIns] = GetText(stmt, 6);
		mob.player_data.PNames[ECase::kPre] = GetText(stmt, 7);

		// Descriptions
		mob.player_data.long_descr = utils::colorCAP(GetText(stmt, 8));
		mob.player_data.description = GetText(stmt, 9);

		// Base parameters
		GET_ALIGNMENT(&mob) = sqlite3_column_int(stmt, 10);

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
		mob.set_gold(sqlite3_column_int(stmt, 23));

		// Experience
		mob.set_exp(sqlite3_column_int(stmt, 24));

		// Position
		std::string default_pos = GetText(stmt, 25);
		std::string start_pos = GetText(stmt, 26);
		auto pos_it = position_map.find(default_pos);
		mob.mob_specials.default_pos = pos_it != position_map.end() ?
			static_cast<EPosition>(pos_it->second) : EPosition::kStand;
		pos_it = position_map.find(start_pos);
		mob.SetPosition(pos_it != position_map.end() ?
			static_cast<EPosition>(pos_it->second) : EPosition::kStand);

		// Sex
		std::string sex_str = GetText(stmt, 27);
		auto gender_it = gender_map.find(sex_str);
		mob.set_sex(gender_it != gender_map.end() ?
			static_cast<EGender>(gender_it->second) : EGender::kMale);

		// Physical attributes
		GET_SIZE(&mob) = sqlite3_column_int(stmt, 28);
		GET_HEIGHT(&mob) = sqlite3_column_int(stmt, 29);
		GET_WEIGHT(&mob) = sqlite3_column_int(stmt, 30);

		// Class and race
		mob.set_class(static_cast<ECharClass>(sqlite3_column_int(stmt, 31)));
		mob.player_data.Race = static_cast<ENpcRace>(sqlite3_column_int(stmt, 32));

		// Attributes (E-spec)
		mob.set_str(sqlite3_column_int(stmt, 33));
		mob.set_dex(sqlite3_column_int(stmt, 34));
		mob.set_int(sqlite3_column_int(stmt, 35));
		mob.set_wis(sqlite3_column_int(stmt, 36));
		mob.set_con(sqlite3_column_int(stmt, 37));
		mob.set_cha(sqlite3_column_int(stmt, 38));

		// Enhanced E-spec fields (scalar values)
		mob.set_str_add(sqlite3_column_int(stmt, 39));
		mob.add_abils.hitreg = sqlite3_column_int(stmt, 40);
		mob.add_abils.armour = sqlite3_column_int(stmt, 41);
		mob.add_abils.manareg = sqlite3_column_int(stmt, 42);
		mob.add_abils.cast_success = sqlite3_column_int(stmt, 43);
		mob.add_abils.morale = sqlite3_column_int(stmt, 44);
		mob.add_abils.initiative_add = sqlite3_column_int(stmt, 45);
		mob.add_abils.absorb = sqlite3_column_int(stmt, 46);
		mob.add_abils.aresist = sqlite3_column_int(stmt, 47);
		mob.add_abils.mresist = sqlite3_column_int(stmt, 48);
		mob.add_abils.presist = sqlite3_column_int(stmt, 49);
		mob.mob_specials.attack_type = sqlite3_column_int(stmt, 50);
		mob.mob_specials.like_work = sqlite3_column_int(stmt, 51);
		mob.mob_specials.MaxFactor = sqlite3_column_int(stmt, 52);
		mob.mob_specials.extra_attack = sqlite3_column_int(stmt, 53);
		mob.set_remort(sqlite3_column_int(stmt, 54));
		
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

		// Set NPC flag

		// Setup index
		int zone_vnum = vnum / 100;
		auto zone_it = zone_vnum_to_rnum.find(zone_vnum);
		if (zone_it != zone_vnum_to_rnum.end())
		{
			if (zone_table[zone_it->second].RnumMobsLocation.first == -1)
			{
				zone_table[zone_it->second].RnumMobsLocation.first = top_of_mobt;
			}
			zone_table[zone_it->second].RnumMobsLocation.second = top_of_mobt;
		}

		mob_index[top_of_mobt].vnum = vnum;
		mob_index[top_of_mobt].total_online = 0;
		mob_index[top_of_mobt].stored = 0;
		mob_index[top_of_mobt].func = nullptr;
		mob_index[top_of_mobt].farg = nullptr;
		mob_index[top_of_mobt].proto = nullptr;
		mob_index[top_of_mobt].set_idx = -1;

		// Initialize test data if needed
		if (mob.GetLevel() == 0)
			SetTestData(&mob);
		mob.set_rnum(top_of_mobt);

		top_of_mobt++;
	}
	sqlite3_finalize(stmt);

	// top_of_mobt should be last valid index, not count
	if (top_of_mobt > 0)
	{
		top_of_mobt--;
	}

	// Load mob flags
	LoadMobFlags();

	// Load mob skills
	LoadMobSkills();

	// Load mob triggers
	LoadMobTriggers();

	// Load Enhanced mob fields (arrays)
	LoadMobResistances();
	LoadMobSaves();
	LoadMobFeats();
	LoadMobSpells();
	LoadMobHelpers();
	LoadMobDestinations();

	log("Loaded %d mobs from SQLite.", top_of_mobt + 1);
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
		}
		else if (strcmp(category.c_str(), "affect") == 0)
		{
			auto flag_it = mob_affect_flag_map.find(flag_name);
			if (flag_it != mob_affect_flag_map.end() && flag_it->second != 0)
			{
				AFF_FLAGS(&mob).set(static_cast<Bitvector>(flag_it->second));
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
		int skill_id = sqlite3_column_int(stmt, 1);
		int value = sqlite3_column_int(stmt, 2);

		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;

		auto skill = static_cast<ESkill>(skill_id);
		mob_proto[it->second].set_skill(skill, value);
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
		int resist_type = sqlite3_column_int(stmt, 1);
		int value = sqlite3_column_int(stmt, 2);
		
		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;
		
		CharData &mob = mob_proto[it->second];
		if (resist_type >= 0 && resist_type < static_cast<int>(mob.add_abils.apply_resistance.size()))
		{
			mob.add_abils.apply_resistance[resist_type] = value;
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
		int save_type = sqlite3_column_int(stmt, 1);
		int value = sqlite3_column_int(stmt, 2);
		
		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;
		
		CharData &mob = mob_proto[it->second];
		if (save_type >= 0 && save_type < static_cast<int>(mob.add_abils.apply_saving_throw.size()))
		{
			mob.add_abils.apply_saving_throw[save_type] = value;
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
	const char *sql = "SELECT mob_vnum, spell_id FROM mob_spells";
	
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
		int spell_id = sqlite3_column_int(stmt, 1);
		
		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;
		
		CharData &mob = mob_proto[it->second];
		if (spell_id >= 0 && spell_id < static_cast<int>(mob.real_abils.SplKnw.size()))
		{
			mob.real_abils.SplKnw[spell_id] = 1;  // Mark spell as known
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
	const char *sql = "SELECT mob_vnum, helper_vnum FROM mob_helpers";
	
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
		int dest_order = sqlite3_column_int(stmt, 1);
		int room_vnum = sqlite3_column_int(stmt, 2);
		
		auto it = vnum_to_rnum.find(mob_vnum);
		if (it == vnum_to_rnum.end()) continue;
		
		CharData &mob = mob_proto[it->second];
		if (dest_order >= 0 && dest_order < static_cast<int>(mob.mob_specials.dest.size()))
		{
			mob.mob_specials.dest[dest_order] = room_vnum;
			destinations_set++;
		}
	}
	sqlite3_finalize(stmt);
	
	if (destinations_set > 0)
	{
		log("   Set %d mob destinations.", destinations_set);
	}
}

// ============================================================================
// Object Loading
// ============================================================================

void SqliteWorldDataSource::LoadObjects()
{
	log("Loading objects from SQLite database.");

	if (!OpenDatabase())
	{
		log("SYSERR: Failed to open SQLite database for object loading.");
		return;
	}

	int obj_count = GetCount("objects");
	if (obj_count == 0)
	{
		log("No objects found in SQLite database.");
		return;
	}

	log("   %d objs.", obj_count);

	const char *sql = "SELECT vnum, aliases, name_nom, name_gen, name_dat, name_acc, name_ins, name_pre, "
					  "short_desc, action_desc, obj_type_id, material, value0, value1, value2, value3, "
					  "weight, cost, rent_off, rent_on, spec_param, max_durability, cur_durability, "
					  "timer, spell, level, sex, max_in_world, minimum_remorts "
					  "FROM objects WHERE enabled = 1 ORDER BY vnum";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare object query: %s", sqlite3_errmsg(m_db));
		return;
	}

	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int vnum = sqlite3_column_int(stmt, 0);

		auto obj = std::make_shared<CObjectPrototype>(vnum);

		// Names
		obj->set_aliases(GetText(stmt, 1));
		obj->set_short_description(utils::colorLOW(GetText(stmt, 2)));
		obj->set_PName(ECase::kNom, utils::colorLOW(GetText(stmt, 2)));
		obj->set_PName(ECase::kGen, utils::colorLOW(GetText(stmt, 3)));
		obj->set_PName(ECase::kDat, utils::colorLOW(GetText(stmt, 4)));
		obj->set_PName(ECase::kAcc, utils::colorLOW(GetText(stmt, 5)));
		obj->set_PName(ECase::kIns, utils::colorLOW(GetText(stmt, 6)));
		obj->set_PName(ECase::kPre, utils::colorLOW(GetText(stmt, 7)));
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

		obj_proto.add(obj, vnum);
	}
	sqlite3_finalize(stmt);

	// Load object flags
	LoadObjectFlags();

	// Load object applies
	LoadObjectApplies();

	// Load object triggers
	LoadObjectTriggers();

	// Load object extra descriptions
	LoadObjectExtraDescriptions();

	log("Loaded %zu objects from SQLite.", obj_proto.size());
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
	const char *sql = "SELECT obj_vnum, location_id, modifier FROM obj_applies";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	int applies_loaded = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int obj_vnum = sqlite3_column_int(stmt, 0);
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
	const char *sql = "SELECT entity_vnum, keywords, description FROM extra_descriptions WHERE entity_type = 'obj'";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		return;
	}

	int loaded = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int obj_vnum = sqlite3_column_int(stmt, 0);
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

bool SqliteWorldDataSource::BeginTransaction()
{
	char *err_msg = nullptr;
	int rc = sqlite3_exec(m_db, "BEGIN TRANSACTION", nullptr, nullptr, &err_msg);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: Failed to begin transaction: %s", err_msg);
		sqlite3_free(err_msg);
		return false;
	}
	return true;
}

bool SqliteWorldDataSource::CommitTransaction()
{
	char *err_msg = nullptr;
	int rc = sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, &err_msg);
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
	char *err_msg = nullptr;
	int rc = sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, &err_msg);
	if (rc != SQLITE_OK)
	{
		log("SYSERR: Failed to rollback transaction: %s", err_msg);
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
	sqlite3_bind_text(stmt, 2, zone.name.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, zone.comment.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, zone.location.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 5, zone.author.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 6, zone.description.c_str(), -1, SQLITE_TRANSIENT);
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
			sqlite3_bind_text(stmt, 3, "A", -1, SQLITE_STATIC);
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
			sqlite3_bind_text(stmt, 3, "B", -1, SQLITE_STATIC);
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

	// Insert commands
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql = 
		"INSERT INTO zone_commands (zone_vnum, command_order, command, if_flag, "
		"arg1, arg2, arg3, arg4, sarg1, sarg2) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	int order = 0;
	for (int i = 0; commands[i].command != 'S'; ++i)
	{
		// Skip A and B commands - they're in zone_groups
		if (commands[i].command == 'A' || commands[i].command == 'B')
		{
			continue;
		}

		if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, zone_vnum);
			sqlite3_bind_int(stmt, 2, order++);
			
			char cmd_str[2] = {commands[i].command, '\0'};
			sqlite3_bind_text(stmt, 3, cmd_str, -1, SQLITE_TRANSIENT);
			sqlite3_bind_int(stmt, 4, commands[i].if_flag);
			sqlite3_bind_int(stmt, 5, commands[i].arg1);
			sqlite3_bind_int(stmt, 6, commands[i].arg2);
			sqlite3_bind_int(stmt, 7, commands[i].arg3);
			sqlite3_bind_int(stmt, 8, commands[i].arg4);
			
			if (commands[i].sarg1)
			{
				sqlite3_bind_text(stmt, 9, commands[i].sarg1, -1, SQLITE_TRANSIENT);
			}
			else
			{
				sqlite3_bind_null(stmt, 9);
			}
			
			if (commands[i].sarg2)
			{
				sqlite3_bind_text(stmt, 10, commands[i].sarg2, -1, SQLITE_TRANSIENT);
			}
			else
			{
				sqlite3_bind_null(stmt, 10);
			}

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

	// Delete existing trigger and bindings (CASCADE will handle trigger_type_bindings)
	std::string delete_sql = "DELETE FROM triggers WHERE vnum = " + std::to_string(trig_vnum);
	ExecuteStatement(delete_sql, "delete trigger");

	// Insert trigger record
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql = 
		"INSERT INTO triggers (vnum, name, attach_type_id, narg, arglist, script, enabled) "
		"VALUES (?, ?, ?, ?, ?, ?, 1)";

	if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare trigger insert: %s", sqlite3_errmsg(m_db));
		return;
	}

	// Build script text from cmdlist
	std::string script_text;
	if (trig->cmdlist)
	{
		for (auto cmd = *trig->cmdlist; cmd; cmd = cmd->next)
		{
			if (!cmd->cmd.empty())
			{
				script_text += cmd->cmd;
				script_text += "\n";
			}
		}
	}

	sqlite3_bind_int(stmt, 1, trig_vnum);
	sqlite3_bind_text(stmt, 2, trig->get_name().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 3, trig->get_attach_type());
	sqlite3_bind_int(stmt, 4, GET_TRIG_NARG(trig));
	sqlite3_bind_text(stmt, 5, trig->arglist.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 6, script_text.c_str(), -1, SQLITE_TRANSIENT);

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
				sqlite3_bind_text(stmt, 2, ch_str, -1, SQLITE_TRANSIENT);
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
	
	// Get trigger range for this zone
	TrgRnum first_trig = zone.RnumTrigsLocation.first;
	TrgRnum last_trig = zone.RnumTrigsLocation.second;

	if (first_trig == -1 || last_trig == -1)
	{
		log("Zone %d has no triggers to save", zone.vnum);
		return true;
	}

	if (!BeginTransaction())
	{
		return false;
	}

	int saved_count = 0;
	for (TrgRnum trig_rnum = first_trig; trig_rnum <= last_trig && trig_rnum <= top_of_trigt; ++trig_rnum)
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

	// Delete existing room data (CASCADE will handle related tables)
	std::string delete_sql = "DELETE FROM rooms WHERE vnum = " + std::to_string(room_vnum);
	ExecuteStatement(delete_sql, "delete room");

	// Insert room record
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql = 
		"INSERT INTO rooms (vnum, zone_vnum, name, description, sector_id, enabled) "
		"VALUES (?, ?, ?, ?, ?, 1)";

	if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare room insert: %s", sqlite3_errmsg(m_db));
		return;
	}

	sqlite3_bind_int(stmt, 1, room_vnum);
	sqlite3_bind_int(stmt, 2, zone_vnum);
	sqlite3_bind_text(stmt, 3, room->name, -1, SQLITE_TRANSIENT);
	
	std::string description = GlobalObjects::descriptions().get(room->description_num);
	sqlite3_bind_text(stmt, 4, description.c_str(), -1, SQLITE_TRANSIENT);
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
		if (!room->dir_option[dir])
		{
			continue;
		}

		if (sqlite3_prepare_v2(m_db, exit_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, room_vnum);
			sqlite3_bind_int(stmt, 2, dir);
			
			if (!room->dir_option[dir]->general_description.empty())
			{
				sqlite3_bind_text(stmt, 3, room->dir_option[dir]->general_description.c_str(), -1, SQLITE_TRANSIENT);
			}
			else
			{
				sqlite3_bind_null(stmt, 3);
			}

			if (room->dir_option[dir]->keyword)
			{
				sqlite3_bind_text(stmt, 4, room->dir_option[dir]->keyword, -1, SQLITE_TRANSIENT);
			}
			else
			{
				sqlite3_bind_null(stmt, 4);
			}

			// Save exit flags as string (numeric value)
			std::string exit_flags_str = std::to_string(room->dir_option[dir]->exit_info);
			if (!exit_flags_str.empty() && exit_flags_str != "0")
			{
				sqlite3_bind_text(stmt, 5, exit_flags_str.c_str(), -1, SQLITE_TRANSIENT);
			}
			else
			{
				sqlite3_bind_null(stmt, 5);
			}

			sqlite3_bind_int(stmt, 6, room->dir_option[dir]->key);
			sqlite3_bind_int(stmt, 7, room->dir_option[dir]->to_room() != kNowhere ? world[room->dir_option[dir]->to_room()]->vnum : -1);
			sqlite3_bind_int(stmt, 8, room->dir_option[dir]->lock_complexity);

			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// Save extra descriptions
	const char *extra_sql = 
		"INSERT INTO extra_descriptions (entity_type, entity_vnum, keyword, description) "
		"VALUES ('room', ?, ?, ?)";

	for (auto exdesc = room->ex_description; exdesc; exdesc = exdesc->next)
	{
		if (sqlite3_prepare_v2(m_db, extra_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, room_vnum);
			sqlite3_bind_text(stmt, 2, exdesc->keyword, -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 3, exdesc->description, -1, SQLITE_TRANSIENT);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	// Save room triggers
	const char *trig_sql = 
		"INSERT INTO entity_triggers (entity_type, entity_vnum, trigger_vnum, trigger_order) "
		"VALUES ('room', ?, ?, ?)";

	int trig_order = 0;
	if (room->script && !room->script->script_trig_list.empty())
	{
		for (auto trig : room->script->script_trig_list)
		{
			if (sqlite3_prepare_v2(m_db, trig_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, room_vnum);
				sqlite3_bind_int(stmt, 2, GET_TRIG_VNUM(trig));
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
	// Delete existing mob data (CASCADE will handle related tables)
	std::string delete_sql = "DELETE FROM mobs WHERE vnum = " + std::to_string(mob_vnum);
	ExecuteStatement(delete_sql, "delete mob");

	// Insert mob main record
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql = 
		"INSERT INTO mobs (vnum, aliases, name_nom, name_gen, name_dat, name_acc, name_ins, name_pre, "
		"short_desc, long_desc, alignment, mob_type, level, hitroll_penalty, armor, "
		"hp_dice_count, hp_dice_size, hp_bonus, dam_dice_count, dam_dice_size, dam_bonus, "
		"gold_dice_count, gold_dice_size, gold_bonus, experience, default_pos, start_pos, "
		"sex, size, height, weight, mob_class, race, "
		"attr_str, attr_dex, attr_int, attr_wis, attr_con, attr_cha, "
		"attr_str_add, hp_regen, armour_bonus, mana_regen, cast_success, morale, "
		"initiative_add, absorb, aresist, mresist, presist, bare_hand_attack, "
		"like_work, max_factor, extra_attack, mob_remort, special_bitvector, role, enabled) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1)";

	if (sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		log("SYSERR: Failed to prepare mob insert: %s", sqlite3_errmsg(m_db));
		return;
	}

	int col = 1;
	sqlite3_bind_int(stmt, col++, mob_vnum);
	
	// Names
	sqlite3_bind_text(stmt, col++, GET_PC_NAME(&mob), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, mob.player_data.PNames[ECase::kNom].c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, mob.player_data.PNames[ECase::kGen].c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, mob.player_data.PNames[ECase::kDat].c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, mob.player_data.PNames[ECase::kAcc].c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, mob.player_data.PNames[ECase::kIns].c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, mob.player_data.PNames[ECase::kPre].c_str(), -1, SQLITE_TRANSIENT);
	
	// Descriptions
	sqlite3_bind_text(stmt, col++, mob.player_data.long_descr.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, mob.player_data.description.c_str(), -1, SQLITE_TRANSIENT);
	
	// Base parameters
	sqlite3_bind_int(stmt, col++, GET_ALIGNMENT(&mob));
	
	// Mob type (E or S)
	std::string mob_type = (mob.get_str() > 0) ? "E" : "S";
	sqlite3_bind_text(stmt, col++, mob_type.c_str(), -1, SQLITE_TRANSIENT);
	
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
	sqlite3_bind_int(stmt, col++, mob.get_gold());
	
	// Experience
	sqlite3_bind_int(stmt, col++, mob.get_exp());
	
	// Position (need to convert enum to string)
	// For now use numeric values, TODO: add lookup
	sqlite3_bind_int(stmt, col++, static_cast<int>(mob.mob_specials.default_pos));
	sqlite3_bind_int(stmt, col++, static_cast<int>(mob.GetPosition()));
	
	// Sex (need to convert enum to string)
	sqlite3_bind_int(stmt, col++, static_cast<int>(mob.get_sex()));
	
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
	
	// special_bitvector (FlagData as TEXT)
	char special_buf[kMaxStringLength];
	mob.mob_specials.npc_flags.tascii(FlagData::kPlanesNumber, special_buf);
	if (special_buf[0] != '0' || special_buf[1] != 'a')
	{
		sqlite3_bind_text(stmt, col++, special_buf, -1, SQLITE_TRANSIENT);
	}
	else
	{
		sqlite3_bind_null(stmt, col++);
	}
	
	
	// Role (bitset<9> as TEXT)
	std::string role_str = mob.get_role().to_string();
	if (!role_str.empty() && role_str != "000000000")
	{
		sqlite3_bind_text(stmt, col++, role_str.c_str(), -1, SQLITE_TRANSIENT);
	}
	else
	{
		sqlite3_bind_null(stmt, col++);
	}

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
	FlagData affect_flags = mob.char_specials.saved.affected_by;
	SaveFlagsToTable(m_db, "mob_flags", "mob_vnum", mob_vnum, affect_flags, mob_affect_flag_map, "affect");


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




	// Save mob skills
	const char *skill_sql = "INSERT INTO mob_skills (mob_vnum, skill_id, value) VALUES (?, ?, ?)";
	for (ESkill skill = ESkill::kFirst; skill <= ESkill::kLast; ++skill)
	{
		int value = mob.GetSkill(skill);
		if (value > 0)
		{
			if (sqlite3_prepare_v2(m_db, skill_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, to_underlying(skill));
				sqlite3_bind_int(stmt, 3, value);
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

	// Save mob spells
	const char *spell_sql = "INSERT INTO mob_spells (mob_vnum, spell_id) VALUES (?, ?)";
	for (size_t spell_id = 0; spell_id < mob.real_abils.SplKnw.size(); ++spell_id)
	{
		if (mob.real_abils.SplKnw[spell_id] > 0)
		{
			if (sqlite3_prepare_v2(m_db, spell_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, spell_id);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}
	// Save mob helpers
	const char *helper_sql = "INSERT INTO mob_helpers (mob_vnum, helper_vnum) VALUES (?, ?)";
	for (const auto &helper_vnum : mob.summon_helpers)
	{
		if (helper_vnum != 0)
		{
			if (sqlite3_prepare_v2(m_db, helper_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, helper_vnum);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
			}
		}
	}
	// Save mob destinations
	const char *dest_sql = "INSERT INTO mob_destinations (mob_vnum, dest_order, room_vnum) VALUES (?, ?, ?)";
	for (size_t i = 0; i < mob.mob_specials.dest.size(); ++i)
	{
		if (mob.mob_specials.dest[i] != 0)
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
	}

	// Save mob triggers
	const char *trig_sql = 
		"INSERT INTO entity_triggers (entity_type, entity_vnum, trigger_vnum, trigger_order) "
		"VALUES ('mob', ?, ?, ?)";

	int trig_order = 0;
	if (mob.script && !mob.script->script_trig_list.empty())
	{
		for (auto trig : mob.script->script_trig_list)
		{
			if (sqlite3_prepare_v2(m_db, trig_sql, -1, &stmt, nullptr) == SQLITE_OK)
			{
				sqlite3_bind_int(stmt, 1, mob_vnum);
				sqlite3_bind_int(stmt, 2, GET_TRIG_VNUM(trig));
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

	// Delete existing object data (CASCADE will handle related tables)
	std::string delete_sql = "DELETE FROM objects WHERE vnum = " + std::to_string(obj_vnum);
	ExecuteStatement(delete_sql, "delete object");

	// Insert object main record
	sqlite3_stmt *stmt = nullptr;
	const char *insert_sql = 
		"INSERT INTO objects (vnum, aliases, name_nom, name_gen, name_dat, name_acc, name_ins, name_pre, "
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
	sqlite3_bind_text(stmt, col++, obj->get_aliases().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, obj->get_PName(ECase::kNom).c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, obj->get_PName(ECase::kGen).c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, obj->get_PName(ECase::kDat).c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, obj->get_PName(ECase::kAcc).c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, obj->get_PName(ECase::kIns).c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, obj->get_PName(ECase::kPre).c_str(), -1, SQLITE_TRANSIENT);
	
	// Descriptions
	sqlite3_bind_text(stmt, col++, obj->get_description().c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, obj->get_action_description().c_str(), -1, SQLITE_TRANSIENT);
	
	// Type and material
	sqlite3_bind_int(stmt, col++, to_underlying(obj->get_type()));
	sqlite3_bind_int(stmt, col++, to_underlying(obj->get_material()));
	
	// Values (store as strings)
	std::string val0 = std::to_string(obj->get_val(0));
	std::string val1 = std::to_string(obj->get_val(1));
	std::string val2 = std::to_string(obj->get_val(2));
	std::string val3 = std::to_string(obj->get_val(3));
	sqlite3_bind_text(stmt, col++, val0.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, val1.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, val2.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, col++, val3.c_str(), -1, SQLITE_TRANSIENT);
	
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


	// Save object extra flags
	FlagData obj_flags = obj->get_extra_flags();
	SaveFlagsToTable(m_db, "obj_flags", "obj_vnum", obj_vnum, obj_flags, obj_extra_flag_map);
	// Save object wear flags
	int wear_flags = obj->get_wear_flags();
	sqlite3_stmt *wear_stmt = nullptr;
	const char *wear_sql = "INSERT INTO obj_flags (obj_vnum, flag_category, flag_name) VALUES (?, 'wear', ?)";
	for (int bit = 0; bit < 32; ++bit)
	{
		if (wear_flags & (1 << bit))
		{
			Bitvector bit_value = (1 << bit);
			std::string flag_name = ReverseLookupFlag(obj_wear_flag_map, bit_value);
			if (!flag_name.empty())
			{
				if (sqlite3_prepare_v2(m_db, wear_sql, -1, &wear_stmt, nullptr) == SQLITE_OK)
				{
					sqlite3_bind_int(wear_stmt, 1, obj_vnum);
					sqlite3_bind_text(wear_stmt, 2, flag_name.c_str(), -1, SQLITE_TRANSIENT);
					sqlite3_step(wear_stmt);
					sqlite3_finalize(wear_stmt);
				}
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

	// Save object extra descriptions
	const char *extra_sql = 
		"INSERT INTO extra_descriptions (entity_type, entity_vnum, keyword, description) "
		"VALUES ('obj', ?, ?, ?)";

	for (auto exdesc = obj->get_ex_description(); exdesc; exdesc = exdesc->next)
	{
		if (sqlite3_prepare_v2(m_db, extra_sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, obj_vnum);
			sqlite3_bind_text(stmt, 2, exdesc->keyword, -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 3, exdesc->description, -1, SQLITE_TRANSIENT);
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
