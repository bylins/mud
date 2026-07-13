#include <filesystem>
#include "gameplay/affects/affect_messages.h"
#include "gameplay/abilities/feats.h"   // issue.perk-action-patching: BuildTalentPatchIndex
#include "utils/utils_encoding.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/mechanics/portal.h"

#include "third_party_libs/pugixml/pugixml.h"

#include "administration/accounts.h"
#include "administration/ban.h"
#include "administration/karma.h"
#include "gameplay/communication/boards/boards.h"
#include "engine/boot/boot_data_files.h"
#include "engine/boot/boot_index.h"
#include "gameplay/communication/social.h"
#include "gameplay/crafting/jewelry.h"
#include "gameplay/crafting/mining.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/mechanics/corpse.h"
#include "gameplay/mechanics/celebrates.h"
#include "gameplay/mechanics/deathtrap.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/city_guards.h"
#include "description.h"
#include "gameplay/mechanics/depot.h"
#include "gameplay/mechanics/bonus.h"
#include "utils/file_crc.h"
#include "global_objects.h"
#include "gameplay/mechanics/rune_stones.h"   // issue.runestones phase 3: SpawnStones()
#include "gameplay/mechanics/glory.h"
#include "gameplay/mechanics/glory_const.h"
#include "gameplay/mechanics/glory_misc.h"
#include "engine/core/char_equip_flags.h"
#include "engine/core/char_handler.h"
#include "engine/core/char_movement.h"
#include "engine/core/obj_handler.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/inventory.h"
#include "engine/core/target_resolver.h"
#include "help.h"
#include "gameplay/clans/house.h"
#include "gameplay/crafting/item_creation.h"
#include "gameplay/mechanics/liquid.h"
#include "gameplay/communication/mail.h"
#include "gameplay/statistics/mob_stat.h"
#include "engine/ui/modify.h"
#include "gameplay/mechanics/named_stuff.h"
#include "administration/names.h"
#include "gameplay/mechanics/noob.h"
#include "obj_prototypes.h"
#include "engine/olc/olc.h"
#include "engine/olc/vedun/vedun.h"
#include "engine/observability/helpers.h"
#include "engine/observability/metrics.h"
#include "utils/tracing/trace_manager.h"
#include "gameplay/communication/offtop.h"
#include "gameplay/communication/parcel.h"
#include "administration/privilege.h"
#include "gameplay/mechanics/sets_drop.h"
#include "gameplay/mechanics/stable_objs.h"
#include "gameplay/economics/shop_ext.h"
#include "gameplay/mechanics/stuff.h"
#include "gameplay/mechanics/title.h"
#include "gameplay/statistics/top.h"
#include "utils/backtrace.h"
#include "engine/scripting/dg_db_scripts.h"
#include "gameplay/mechanics/mob_races.h"
#include "gameplay/mechanics/treasure_cases.h"
#include "gameplay/mechanics/cities.h"
#include "gameplay/ai/mobact.h"
#include "administration/proxy.h"
#include "utils/utils_time.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/ai/spec_procs.h"
#include "gameplay/communication/social.h"
#include "player_index.h"
#include "world_checksum.h"
#include "legacy_world_data_source.h"
#include "world_data_source_base.h"
#ifdef HAVE_SQLITE
#include "sqlite_world_data_source.h"
#endif
#ifdef HAVE_YAML
#include "yaml_world_data_source.h"
#endif
#include "world_data_source_manager.h"
#include "composite_world_data_source.h"
#include "engine/core/config.h"


#include <fmt/format.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <memory>

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/
#if defined(CIRCLE_MACINTOSH)
const long kBeginningOfTime = -1561789232;
#else
const long kBeginningOfTime = 650336715;
#endif

char buf[kMaxStringLength];
char buf1[kMaxStringLength];
char buf2[kMaxStringLength];
char arg[kMaxInputLength];
char smallBuf[kMaxRawInputLength];

Rooms &world = GlobalObjects::world();

RoomRnum top_of_world = 0;    // ref to top element of world

void AddTrigIndexEntry(int nr, Trigger *trig) {
	IndexData *index;
	CREATE(index, 1);
	index->vnum = nr;
	index->total_online = 0;
	index->func = nullptr;
	index->proto = trig;

	trig_index[top_of_trigt++] = index;
}

IndexData **trig_index;    // index table for triggers
int top_of_trigt = 0;        // top of trigger index table

IndexData *mob_index;        // index table for mobile file
MobRnum top_of_mobt = 0;    // top of mobile index table
std::map<long, CharData *> chardata_by_uid;
std::unordered_set<CharData *> chardata_wait_list;
std::unordered_set<CharData *> chardata_cooldown_list;
int global_uid = 0;
long top_idnum = 0;        // highest idnum in use

int circle_restrict = 0;    // level of game restriction
bool enable_world_checksum = false;	// enable world checksum calculation
const char *g_force_world_source = nullptr;	// -F <kind>: force one-shot source, see CompositeWorldDataSource::ForceSource
RoomRnum r_mortal_start_room;    // rnum of mortal start room
RoomRnum r_immort_start_room;    // rnum of immort start room
RoomRnum r_frozen_start_room;    // rnum of frozen start room
RoomRnum r_helled_start_room;
RoomRnum r_named_start_room;
RoomRnum r_unreg_start_room;


char *help{nullptr};        // help screen

TimeInfoData time_info;
ResetQueue reset_q;

const FlagData clear_flags;

const char *ZONE_TRAFFIC_FILE = LIB_PLRSTUFF"zone_traffic.xml";
time_t zones_stat_date;

GameLoader game_loader;

// local functions
void LoadGlobalUid();
void AssignMobiles();
void AssignObjects();
void AssignRooms();
int ReadFileToBuffer(const char *name, char *destination_buf);
void CheckStartRooms();
void AddVirtualRoomsToAllZones();
void CalculateFirstAndLastRooms();
void CalculateFirstAndLastMobs();
void RosolveWorldDoorToRoomVnumsToRnums();
void ResolveZoneTableCmdVnumArgsToRnums();
void LogZoneError(const ZoneData &zone_data, int cmd_no, const char *message);
void ResetGameWorldTime();
int CountMobsInRoom(int m_num, int r_num);
void SetZoneRnumForObjects();
void SetZoneRnumForMobiles();
void SetZoneRnumForTriggers();
int ReadCrashTimerFile(std::size_t index, int temp);
int LoadExchange();
void SetPrecipitations(int *wtype, int startvalue, int chance1, int chance2, int chance3);
void CalcEaster();

// external
extern int no_specials;
extern RoomVnum mortal_start_room;
extern RoomVnum immort_start_room;
extern RoomVnum frozen_start_room;
extern RoomVnum helled_start_room;
extern RoomVnum named_start_room;
extern RoomVnum unreg_start_room;
extern struct MonthTemperature year_temp[];

extern void ExtractTrigger(Trigger *trig);
extern ESkill FixNameAndFindSkillId(char *name);
extern void CopyMobilePrototypeForMedit(CharData *dst, CharData *src, bool partial_copy);

// Separate a 4-character id tag from the data it precedes
void ExtractTagFromArgument(char *argument, char *tag) {
	char *tmp = argument, *ttag = tag, *wrt = argument;
	int i;

	for (i = 0; i < 4; i++)
		*(ttag++) = *(tmp++);
	*ttag = '\0';

	while (*tmp == ':' || *tmp == ' ')
		tmp++;

	while (*tmp)
		*(wrt++) = *(tmp++);
	*wrt = '\0';
}

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

void LoadSheduledReboot() {
	FILE *sch;
	int day = 0, hour = 0, minutes = 0, numofreaded = 0, timeOffset = 0;
	char str[10];
	int offsets[7];

	time_t currTime;

	sch = fopen(LIB_MISC "schedule", "r");
	if (!sch) {
		log("Error opening schedule");
		return;
	}
	time(&currTime);
	for (int i = 0; i < 7; i++) {
		if (fgets(str, 10, sch) == nullptr) {
			break;
		}
		numofreaded = sscanf(str, "%i %i:%i", &day, &hour, &minutes);
		if (numofreaded < 1) {
			continue;
		}
		if (day != i) {
			break;
		}
		if (numofreaded == 1) {
			offsets[i] = 24 * 60;
			continue;
		}
		offsets[i] = hour * 60 + minutes;
	}
	fclose(sch);

	timeOffset = (int) difftime(currTime, shutdown_parameters.get_boot_time());

	//set reboot_uptime equal to current uptime and align to the start of the day
	shutdown_parameters.set_reboot_uptime(
		timeOffset / 60 - localtime(&currTime)->tm_min - 60 * (localtime(&currTime)->tm_hour));
	const auto boot_time = shutdown_parameters.get_boot_time();
	const auto local_boot_time = localtime(&boot_time);
	for (int i = local_boot_time->tm_wday; i < 7; i++) {
		//7 empty days was cycled - break with default uptime
		if (shutdown_parameters.get_reboot_uptime() - 1440 * 7 >= 0) {
			shutdown_parameters.set_reboot_uptime(DEFAULT_REBOOT_UPTIME);    //2 days reboot bu default if no schedule
			break;
		}

		//if we get non-1-full-day offset, but server will reboot too early (36 hour is minimum)
		//we are going to find another scheduled day
		if (offsets[i] < 24 * 60 && (shutdown_parameters.get_reboot_uptime() + offsets[i]) < UPTIME_THRESHOLD * 60) {
			shutdown_parameters.set_reboot_uptime(shutdown_parameters.get_reboot_uptime() + 24 * 60);
		}
		//we've found next point of reboot! :) break cycle
		if (offsets[i] < 24 * 60
			&& (shutdown_parameters.get_reboot_uptime() + offsets[i]) > UPTIME_THRESHOLD * 60) {
			shutdown_parameters.set_reboot_uptime(shutdown_parameters.get_reboot_uptime() + offsets[i]);
			break;
		}
		//empty day - add 24 hour and go next
		if (offsets[i] == 24 * 60) {
			shutdown_parameters.set_reboot_uptime(shutdown_parameters.get_reboot_uptime() + offsets[i]);
		}
		// jump to 1st day of the week
		if (i == 6) {
			i = -1;
		}
	}
	log("Setting up reboot_uptime: %i", shutdown_parameters.get_reboot_uptime());
}

/// конверт поля GET_OBJ_SKILL в емкостях TODO: 12.2013
int ConvertDrinkconSkillField(CObjectPrototype *obj, bool proto) {
	if (obj->get_spec_param() > 0
		&& (obj->get_type() == EObjType::kLiquidContainer
			|| obj->get_type() == EObjType::kFountain)) {
		log("obj_skill: %d - %s (%d)", obj->get_spec_param(), obj->get_PName(grammar::ECase::kNom).c_str(), GET_OBJ_VNUM(obj));
		// если емскости уже просетили какие-то заклы, то зелье
		// из обж-скилл их не перекрывает, а просто удаляется
		if (obj->GetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum) < 0) {
			const auto potion = world_objects.create_from_prototype_by_vnum(obj->get_spec_param());
			if (potion
				&& potion->get_type() == EObjType::kPotion) {
				drinkcon::copy_potion_values(potion.get(), obj);
				if (proto) {
					// copy_potion_values сетит до кучи и внум из пошена,
					// поэтому уточним здесь, что зелье не перелито
					// емкости из read_one_object_new идут как перелитые
					obj->SetPotionValueKey(ObjVal::EValueKey::kPotionProtoVnum, 0);
				}
			}
		}
		obj->set_spec_param(to_underlying(ESkill::kUndefined));

		return 1;
	}
	return 0;
}

// issue.potion-hotfix P2: migrate a kPotion's legacy m_vals payload into the ObjVal
// EValueKey map so it no longer relies on the old array. val[1..3] were the three spells;
// the pre-P1 brew overwrote val[3] with the potency, so an INSTANCE whose val[3] differs
// from its prototype's is treated as brewed (val[3] -> kPotionPotency; its 3rd spell was
// already lost -- the rare old 3-spell brew is a hand-fix). Prototypes always keep val[3]
// as the real 3rd spell. Idempotent: a potion that already has any potion key is skipped
// (P1 brews and prior converter runs). m_vals is left in place for the fallback until P3.
int ConvertPotionToEValueKey(CObjectPrototype *obj, bool proto) {
	if (obj->get_type() != EObjType::kPotion) {
		return 0;
	}
	if (obj->GetPotionValueKey(ObjVal::EValueKey::kPotionSpell1Num) >= 0
		|| obj->GetPotionValueKey(ObjVal::EValueKey::kPotionSpell2Num) >= 0
		|| obj->GetPotionValueKey(ObjVal::EValueKey::kPotionSpell3Num) >= 0
		|| obj->GetPotionValueKey(ObjVal::EValueKey::kPotionPotency) >= 0) {
		return 0;  // already migrated
	}
	const int v1 = obj->get_val(1);
	const int v2 = obj->get_val(2);
	const int v3 = obj->get_val(3);
	// A potion spell is any positive value; -1/0 is the "no spell" sentinel. We must NOT call
	// MUD::Spell().IsValid() here -- ConvertObjValues runs before the spell registry is populated, so
	// it would reject every spell. It isn't needed either: an out-of-range/undefined spell number is
	// silently ignored at cast, exactly as the m_vals path already does.
	const auto set_spell = [obj](ObjVal::EValueKey key, int num) {
		if (num > 0) {
			obj->SetPotionValueKey(key, num);
		}
	};
	set_spell(ObjVal::EValueKey::kPotionSpell1Num, v1);
	set_spell(ObjVal::EValueKey::kPotionSpell2Num, v2);

	bool brewed = false;
	if (!proto && v3 > 0) {
		const auto rnum = obj->get_rnum();
		if (rnum >= 0 && static_cast<size_t>(rnum) < obj_proto.size()) {
			brewed = (v3 != obj_proto[rnum]->get_val(3));  // differs from proto -> brewed potency
		}
	}
	if (brewed) {
		obj->SetPotionValueKey(ObjVal::EValueKey::kPotionPotency, v3);
	} else {
		set_spell(ObjVal::EValueKey::kPotionSpell3Num, v3);
	}
	return 1;
}

/// конверт параметров прототипов ПОСЛЕ лоада всех файлов с прототипами
// issue.potion-hotfix: split the historically overloaded drink/food val[3] into named keys. val[3]
// classically meant "poisoned" at exactly 1; the (revived) freshness code also counted it DOWN, so a
// value > 1 was a freshness countdown. Move poison -> kLiquidPoison, any countdown -> kLiquidTimer, and
// zero val[3] (now unused for drinks). Idempotent: once val[3] is 0 there is nothing left to migrate.
// Runs AFTER ConvertDrinkconSkillField (which may reset the liquid keys while planting a potion).
int ConvertDrinkPoisonField(CObjectPrototype *obj, bool /*proto*/) {
	const auto type = obj->get_type();
	if (type != EObjType::kLiquidContainer && type != EObjType::kFountain && type != EObjType::kFood) {
		return 0;
	}
	const int v3 = obj->get_val(3);
	if (v3 == 1) {
		obj->SetPotionValueKey(ObjVal::EValueKey::kLiquidPoison, 1);
		obj->set_val(3, 0);
		return 1;
	}
	if (v3 > 1) {
		obj->SetPotionValueKey(ObjVal::EValueKey::kLiquidTimer, v3);
		obj->set_val(3, 0);
		return 1;
	}
	return 0;
}

void ConvertObjValues() {
	int save = 0;
	for (const auto &i : obj_proto) {
		save = std::max(save, ConvertDrinkconSkillField(i.get(), true));
		save = std::max(save, ConvertDrinkPoisonField(i.get(), true));
		save = std::max(save, ConvertPotionToEValueKey(i.get(), true));
		if (i->has_flag(EObjFlag::k2inlaid)) {
			i->unset_extraflag(EObjFlag::k2inlaid);
			save = 1;
		}
		if (i->has_flag(EObjFlag::k3inlaid)) {
			i->unset_extraflag(EObjFlag::k3inlaid);
			save = 1;
		}
		// ...
		if (save) {
			olc_add_to_save_list(i->get_vnum() / 100, OLC_SAVE_OBJ);
			save = 0;
		}
	}
}

namespace {

// Build a single world data source from a backend name used in
// <world_loader><sources>. Returns nullptr (with a logged error) for an
// unknown name or a backend not compiled in.
std::unique_ptr<world_loader::IWorldDataSource> CreateWorldSourceByName(const std::string &name) {
	if (name == "yaml") {
#ifdef HAVE_YAML
		return world_loader::CreateYamlDataSource("world");
#else
		log("SYSERR: world source 'yaml' configured but YAML backend is not compiled in");
		return nullptr;
#endif
	}
	if (name == "sqlite") {
#ifdef HAVE_SQLITE
		return world_loader::CreateSqliteDataSource("world.db");
#else
		log("SYSERR: world source 'sqlite' configured but SQLite backend is not compiled in");
		return nullptr;
#endif
	}
	if (name == "legacy") {
		return world_loader::CreateLegacyDataSource();
	}
	log("SYSERR: unknown world source '%s' in <world_loader><sources>", name.c_str());
	return nullptr;
}

// Save one zone's zone-record/rooms/objects/mobs/triggers to `saver`, in the
// fixed order the loaders expect on the way back in. Logs and reports failure
// instead of letting an exception escape -- both call sites (SaveLoadedWorldTo
// below, and GameLoader::ResaveWorld) need to keep saving the remaining zones
// after one zone fails, they just differ in what a failure means for their own
// bookkeeping (resync error count + skip vs. a full converter dump's error
// count). `context` is only used for the log line's prefix.
bool SaveZoneAllEntities(world_loader::IWorldDataSource &saver, int zone_rnum,
						  int zone_vnum, const char *context) {
	try {
		saver.SaveZone(zone_rnum);
		saver.SaveRooms(zone_rnum);
		saver.SaveObjects(zone_rnum);
		saver.SaveMobs(zone_rnum);
		saver.SaveTriggers(zone_rnum, -1, 0);
		return true;
	} catch (const std::exception &e) {
		log("SYSERR: %s failed on zone %d (vnum=%d): %s", context, zone_rnum, zone_vnum, e.what());
		return false;
	}
}

// Write the fully-loaded in-memory world into one data source, scoped to
// `zone_vnums_to_resync` -- the composite's per-source dirty-zone list (see
// CompositeWorldDataSource::StaleZonesBySource()), already excluding
// dungeon-instance vnums. Not all zone_table -- resyncing only the zones a
// source actually lost the freshness comparison for is the whole point of
// per-zone selection (see composite_world_data_source.h for why). Stamps
// each zone (and the index) with the content version reported by
// `version_src` -- the source we read from -- so the rewritten backend ends
// up EQUAL in freshness to it, not newer; that equality is what stops the
// two backends trading "stale" roles each boot.
int SaveLoadedWorldTo(world_loader::IWorldDataSource &saver,
					  const world_loader::IWorldDataSource *version_src,
					  const std::vector<int> &zone_vnums_to_resync) {
	int errors = 0;
	saver.BeginBulkWrite();
	for (int vnum : zone_vnums_to_resync) {
		const int z = GetZoneRnum(vnum);
		if (z < 0) {
			log("SYSERR: resync: zone vnum %d not found in zone_table", vnum);
			++errors;
			continue;
		}
		if (SaveZoneAllEntities(saver, z, vnum, "resync")) {
			const world_loader::Freshness ver =
				version_src ? version_src->GetZoneFreshness(vnum) : 0;
			saver.MarkZoneSynced(z, ver);
		} else {
			++errors;
		}
	}
	saver.EndBulkWrite();
	if (version_src) {
		saver.MarkIndexSynced(version_src->GetIndexFreshness());
	}
	try {
		saver.FinalizeResave();
	} catch (const std::exception &e) {
		log("SYSERR: resync finalize failed: %s", e.what());
		++errors;
	}
	return errors;
}

// Entity-loading orchestration: the ONE path for every scenario (plain
// single-source boot, composite with one winning source per zone, flat or
// per-file YAML underneath) -- ds_ptr->LoadX(nullptr) already fans out to
// every source internally if ds_ptr is a composite (see
// CompositeWorldDataSource::LoadTriggers/Rooms/Mobs/Objects), so there is
// nothing left for BootWorld to branch on. These functions do the one thing
// that must happen exactly once regardless of source count: sort the
// (possibly multi-source) results by vnum and place them in the global
// tables (trig_index/world/mob_proto+mob_index/obj_proto), then attach
// triggers now that every entity has its final rnum.
void LoadTriggersUnified(world_loader::IWorldDataSource *ds_ptr) {
	auto loaded = ds_ptr->LoadTriggers(nullptr);
	if (!ds_ptr->SupportsZoneFilter()) {
		// Self-managing source (the legacy backend): its LoadTriggers() call
		// above already populated trig_index/top_of_trigt directly through
		// its own internal boot path (GameLoader::BootIndex), exactly as it
		// always has -- there's nothing left to place here.
		return;
	}
	std::sort(loaded.begin(), loaded.end(),
		[](const auto &a, const auto &b) { return a.vnum < b.vnum; });

	CREATE(trig_index, loaded.size());
	top_of_trigt = 0;
	for (auto &lt : loaded) {
		lt.trig->set_rnum(top_of_trigt);
		world_loader::WorldDataSourceBase::CreateTriggerIndex(lt.vnum, lt.trig);
	}
	log("Loaded %d triggers.", top_of_trigt);
}

void LoadRoomsUnified(world_loader::IWorldDataSource *ds_ptr) {
	if (!ds_ptr->SupportsZoneFilter()) {
		// Self-managing source (the legacy backend): its own internal boot
		// path (GameLoader::BootIndex) pushes the dummy room 0 and populates
		// world[]/top_of_world itself, exactly as it always has -- doing it
		// again here would leave two dummy rooms.
		ds_ptr->LoadRooms(nullptr);
		return;
	}

	// Dummy room 0 (kNowhere), same as every source used to create up front.
	world.push_back(new RoomData);
	top_of_world = kNowhere;

	auto loaded = ds_ptr->LoadRooms(nullptr);
	std::sort(loaded.begin(), loaded.end(),
		[](const auto &a, const auto &b) { return a.vnum < b.vnum; });

	for (auto &lr : loaded) {
		world.push_back(lr.room);
	}
	top_of_world = static_cast<RoomRnum>(world.size() - 1);

	// Attach room triggers (needs world[]/top_of_world updated above so
	// GetRoomRnum can find the rooms just appended).
	for (auto &lr : loaded) {
		if (lr.triggers.empty()) continue;
		const int room_rnum = GetRoomRnum(lr.vnum);
		if (room_rnum < 0) continue;
		for (int trigger_vnum : lr.triggers) {
			world_loader::WorldDataSourceBase::AttachTriggerToRoom(room_rnum, trigger_vnum, lr.vnum);
		}
	}
	log("Loaded %d rooms.", top_of_world);
}

void LoadMobsUnified(world_loader::IWorldDataSource *ds_ptr) {
	auto loaded = ds_ptr->LoadMobs(nullptr);
	if (!ds_ptr->SupportsZoneFilter()) {
		// Self-managing source (the legacy backend): already populated
		// mob_proto[]/mob_index[]/top_of_mobt/RnumMobsLocation itself.
		return;
	}
	std::sort(loaded.begin(), loaded.end(),
		[](const auto &a, const auto &b) { return a.vnum < b.vnum; });

	mob_proto = new CharData[loaded.size()];
	CREATE(mob_index, loaded.size());
	top_of_mobt = 0;
	for (auto &lm : loaded) {
		const MobRnum rnum = world_loader::WorldDataSourceBase::AppendMobIndex(lm.vnum);
		mob_proto[rnum] = std::move(*lm.mob);
		mob_proto[rnum].set_rnum(rnum);
		delete lm.mob;

		// CalculateFirstAndLastMobs() is a no-op stub (unlike rooms' real
		// post-pass) -- this is the only place RnumMobsLocation gets set.
		const int zone_rn = GetZoneRnum(lm.vnum / 100);
		if (zone_rn >= 0) {
			if (zone_table[zone_rn].RnumMobsLocation.first == -1) {
				zone_table[zone_rn].RnumMobsLocation.first = rnum;
			}
			zone_table[zone_rn].RnumMobsLocation.second = rnum;
		}

		for (int trigger_vnum : lm.triggers) {
			world_loader::WorldDataSourceBase::AttachTriggerToMob(rnum, trigger_vnum, lm.vnum);
		}
	}
	// top_of_mobt should be last valid index, not count -- same one-time
	// conversion the bulk loaders always did after their own fill loop.
	if (top_of_mobt > 0) {
		top_of_mobt--;
	}
	log("Loaded %d mobs.", top_of_mobt + 1);
}

void LoadObjectsUnified(world_loader::IWorldDataSource *ds_ptr) {
	auto loaded = ds_ptr->LoadObjects(nullptr);
	if (!ds_ptr->SupportsZoneFilter()) {
		// Self-managing source (the legacy backend): already populated
		// obj_proto itself.
		return;
	}
	std::sort(loaded.begin(), loaded.end(),
		[](const auto &a, const auto &b) { return a.vnum < b.vnum; });

	for (auto &lo : loaded) {
		obj_proto.add(lo.obj, lo.vnum);
	}
	for (const auto &lo : loaded) {
		if (lo.triggers.empty()) continue;
		const int rnum = obj_proto.get_rnum(lo.vnum);
		if (rnum < 0) continue;
		for (int trigger_vnum : lo.triggers) {
			world_loader::WorldDataSourceBase::AttachTriggerToObject(rnum, trigger_vnum, lo.vnum);
		}
	}
	log("Loaded %zu objects.", loaded.size());
}

} // namespace

void GameLoader::BootWorld(std::unique_ptr<world_loader::IWorldDataSource> data_source) {
	utils::CSteppedProfiler boot_profiler("World booting", 1.1);

	// Initialise the int<->name table for ObjVal keys before anything that
	// might serialise an object touches it. Normally BootMudDataBase()
	// (comm.cpp:807) does this for the running-server path, but `-S` /
	// `scheck` return early -- between BootWorld and ResaveWorld -- and
	// never get there. `text_id::Init()` uses unordered_map::insert so it's
	// idempotent if BootMudDataBase later runs.
	text_id::Init();

	// issue.ext-affects: affected_bits[] is data-driven -- rebuilt from each affect's kShortDesc when
	// affect_messages loads -- and object/mob parsing below renders affect flags through it (sprintbits),
	// so it must be populated before the world loads. affects (the id registry) is validated alongside.
	// Guarded like the skills load just below; the normal running-server boot reaches here too.
	if (!affects::MessagesLoaded()) {
		MUD::CfgManager().LoadCfg("affects");
		MUD::CfgManager().LoadCfg("affect_msg");
		// issue.common-msg: nothing_string (CommonMsg(kNothing)) is used by sprintbits during the
		// object/mob load below, so common_messages must be ready before the world parses.
		MUD::CfgManager().LoadCfg("common_msg");
	}

	// CharData::set_skill() / CObjectPrototype::set_skill() drop any skill
	// whose id is invalid, and a skill is "invalid" until the skills config is
	// loaded (MUD::Skills().IsInvalid). BootMudDataBase loads it before
	// BootWorld for the running server, but `-S` / `scheck` call BootWorld
	// directly and skip it -- so without this, every mob and object skill is
	// silently dropped while loading the world to resave it, and the round
	// trip loses them (issue #3391). Guarded so the normal boot, which already
	// loaded the config, does not reload it.
	if (!MUD::Skills().IsInitizalized())
	{
		MUD::CfgManager().LoadCfg("skill_msg");   // issue.thing-names: names/abbr before skills
		MUD::CfgManager().LoadCfg("skills");
	}

	// Create data source if none provided. Runtime selection comes from
	// <world_loader><sources> (ordered = priority); several sources combine into
	// a CompositeWorldDataSource. An empty/absent block falls back to the
	// compile-time default single source, so existing setups are unaffected.
	if (!data_source)
	{
		const auto &source_names = runtime_config.world_sources();
		if (!source_names.empty())
		{
			std::vector<std::unique_ptr<world_loader::IWorldDataSource>> sources;
			for (const auto &name : source_names)
			{
				auto src = CreateWorldSourceByName(name);
				if (src)
				{
					sources.push_back(std::move(src));
				}
			}
			if (sources.size() == 1)
			{
				data_source = std::move(sources.front());
			}
			else if (sources.size() > 1)
			{
				data_source = world_loader::CreateCompositeDataSource(std::move(sources));
			}
		}
	}
	if (!data_source)
	{
#ifdef HAVE_YAML
		data_source = world_loader::CreateYamlDataSource("world");
#elif defined(HAVE_SQLITE)
		data_source = world_loader::CreateSqliteDataSource("world.db");
#else
		data_source = world_loader::CreateLegacyDataSource();
#endif
	}
	log("Using data source: %s", data_source->GetName().c_str());

	// Register data source in manager for OLC access
	auto* ds_ptr = data_source.get();
	world_loader::WorldDataSourceManager::Instance().SetDataSource(std::move(data_source));

	// -F <kind>: one-shot override, e.g. rebuilding a YAML tree from a
	// SQLite world.db that was copied in from another machine, where mtime-
	// based freshness comparison isn't meaningful. Must fail loudly (not
	// silently boot in normal cache mode) if the request can't be honored --
	// see CompositeWorldDataSource::ForceSource.
	if (g_force_world_source)
	{
		auto *composite_for_force = dynamic_cast<world_loader::CompositeWorldDataSource *>(ds_ptr);
		if (!composite_for_force)
		{
			fatal_log("FATAL: -F %s requires a composite <world_loader><sources> with 2+ entries "
				"(single-source config has nothing to force/rebuild).", g_force_world_source);
		}
		if (!composite_for_force->ForceSource(g_force_world_source))
		{
			fatal_log("FATAL: -F %s: no configured world source has that kind.", g_force_world_source);
		}
	}

	boot_profiler.next_step("Loading zone table");
	ds_ptr->LoadZones();

	boot_profiler.next_step("Create blank zoness for dungeons");
	log("Create zones for dungeons.");
	dungeons::CreateBlankZoneDungeon();

	boot_profiler.next_step("Loading triggers");
	LoadTriggersUnified(ds_ptr);

	boot_profiler.next_step("Create blank triggers for dungeons");
	log("Create triggers for dungeons.");
	dungeons::CreateBlankTrigsDungeon();

	boot_profiler.next_step("Loading rooms");
	LoadRoomsUnified(ds_ptr);

	boot_profiler.next_step("Create blank rooms for dungeons");
	log("Create blank rooms for dungeons.");
	dungeons::CreateBlankRoomDungeon();

	boot_profiler.next_step("Adding virtual rooms to all zones");
	log("Adding virtual rooms to all zones.");
	AddVirtualRoomsToAllZones();

	boot_profiler.next_step("Calculate first end last room into zones");
	log("Calculate first and last room into zones.");
	CalculateFirstAndLastRooms();

	// Deferred room child-table sub-loaders (flags/exits/triggers/extra
	// descriptions) that per-zone backends postponed -- see
	// SqliteWorldDataSource::FinalizeZoneRooms. MUST run before the door-rnum
	// resolution pass just below, which needs every room's exits already
	// populated. No-op for a plain single-source boot and for YAML (which
	// does everything inline per zone already). AddVirtualRoomsToAllZones()
	// has already run (it inserts into world[], shifting rnums), and rooms
	// are otherwise stable by this point, so vnum->rnum lookups done inside
	// FinalizeZoneRooms() stay valid through RosolveWorldDoorToRoomVnumsToRnums.
	ds_ptr->FinalizeZoneRooms();

	boot_profiler.next_step("Renumbering rooms");
	log("Renumbering rooms.");
	RosolveWorldDoorToRoomVnumsToRnums();

	boot_profiler.next_step("Checking start rooms");
	log("Checking start rooms.");
	CheckStartRooms();

	boot_profiler.next_step("Loading mobs and regerating index");
	LoadMobsUnified(ds_ptr);

	boot_profiler.next_step("Counting mob's levels");
	log("Count mob quantity by level");
	MobMax::init();

	boot_profiler.next_step("Create blank mob for dungeons");
	log("Create blank mob for dungeons.");
	dungeons::CreateBlankMobsDungeon();

	boot_profiler.next_step("Calculate first end last mob into zones");
	log("Calculate first and last mob into zones.");
//	CalculateFirstAndLastMobs();

	boot_profiler.next_step("Loading objects");
	LoadObjectsUnified(ds_ptr);

	// Deferred mob/object child-table sub-loaders (flags/skills/applies/
	// triggers/...) that per-zone backends postponed -- see
	// SqliteWorldDataSource::FinalizeZoneEntities. Room sub-loaders already
	// ran earlier via FinalizeZoneRooms(), before door-rnum resolution.
	// No-op for a plain single-source boot and for YAML (which does
	// everything inline per zone already).
	ds_ptr->FinalizeZoneEntities();

	boot_profiler.next_step("Create blank obj for dungeons");
	log("Create blank obj for dungeons.");
	dungeons::CreateBlankObjsDungeon();

	boot_profiler.next_step("Calculate first end last mob into zones");
	log("Calculate first and last mob into zones.");
	CalculateFirstAndLastMobs();

	boot_profiler.next_step("Converting deprecated obj values");
	log("Converting deprecated obj values.");
	ConvertObjValues();

	boot_profiler.next_step("enumbering zone table");
	log("Renumbering zone table.");
	ResolveZoneTableCmdVnumArgsToRnums();

	boot_profiler.next_step("Renumbering Obj_zone");
	log("Renumbering Obj_zone.");
	SetZoneRnumForObjects();

	boot_profiler.next_step("Renumbering Mob_zone");
	log("Renumbering Mob_zone.");
	SetZoneRnumForMobiles();

	boot_profiler.next_step("Calculating trigger locations for zones");
	log("Calculating trigger locations for zones.");
	SetZoneRnumForTriggers();

	boot_profiler.next_step("Initialization of object rnums");
	log("Init system_obj rnums.");
	system_obj::init();

	log("Init global_drop_obj.");

	// Self-heal: after loading, rewrite each source's LOST zones (not the
	// whole zone_table) so every backend converges to the loaded world. Per
	// composite_world_data_source.h, the per-zone freshness comparison
	// already excluded dungeon-instance vnums, so zone_vnums here needs no
	// further filtering.
	if (auto *composite = dynamic_cast<world_loader::CompositeWorldDataSource *>(ds_ptr))
	{
		for (const auto &[src, zone_vnums] : composite->StaleZonesBySource())
		{
			if (zone_vnums.empty())
			{
				continue;
			}
			if (!src->IsWritable())
			{
				log("World source '%s' has %zu stale zone(s) but is not writable; skipping resync",
					src->GetName().c_str(), zone_vnums.size());
				continue;
			}
			boot_profiler.next_step("Resyncing stale world source");
			log("Resyncing %zu stale zone(s) into world source '%s' from loaded world...",
				zone_vnums.size(), src->GetName().c_str());
			const int errs = SaveLoadedWorldTo(*src, composite, zone_vnums);
			log("Resync of '%s' done, errors=%d", src->GetName().c_str(), errs);
		}
	}

	if (enable_world_checksum)
	{
		boot_profiler.next_step("Calculating world checksums");
		log("Calculating world checksums...");
		auto checksums = WorldChecksum::Calculate();
		WorldChecksum::LogResult(checksums);
		WorldChecksum::SaveDetailedChecksums("checksums_detailed.txt", checksums);
		if (!WorldChecksum::SaveDetailedBuffers("checksums_buffers"))
		{
			log("WARNING: Failed to save detailed buffers (see errors above)");
		}

		// If BASELINE_DIR is set, compare with baseline checksums
		const char *baseline_dir = getenv("BASELINE_DIR");
		if (baseline_dir)
		{
			log("Comparing with baseline from: %s", baseline_dir);
			WorldChecksum::CompareWithBaseline(baseline_dir);
		}
	}
}


void SetZoneMobLevel() {
	for (auto &i : zone_table) {
		int level = 0;
		int count = 0;

		for (int nr = 0; nr <= top_of_mobt; ++nr) {
			if (mob_index[nr].vnum >= i.vnum * 100
				&& mob_index[nr].vnum <= i.vnum * 100 + 99) {
				level += mob_proto[nr].GetLevel();
				++count;
			}
		}

		i.mob_level = count ? level / count : 0;
	}
}

void SetZonesTownFlags() {
	const auto zones_count = static_cast<ZoneRnum>(zone_table.size());
	for (ZoneRnum i = 0; i < zones_count; ++i) {
		zone_table[i].is_town = false;
		int rnum_start = 0, rnum_end = 0;
		if (!GetZoneRooms(i, &rnum_start, &rnum_end)) {
			continue;
		}

		bool rent_flag = false, bank_flag = false, post_flag = false;
		for (int k = rnum_start; k <= rnum_end; ++k) {
			for (const auto ch : world[k]->people) {
				if (specials::IsRentkeeper(ch)) {
					rent_flag = true;
				} else if (specials::IsBankkeeper(ch)) {
					bank_flag = true;
				} else if (specials::IsPostkeeper(ch)) {
					post_flag = true;
				}
			}
		}

		zone_table[i].is_town = (rent_flag && bank_flag && post_flag);
	}
}

void ZoneTrafficSave() {
	pugi::xml_document doc;
	doc.append_child().set_name("zone_traffic");
	pugi::xml_node node_list = doc.child("zone_traffic");
	pugi::xml_node time_node = node_list.append_child();
	time_node.set_name("time");
	time_node.append_attribute("date") = static_cast<unsigned long long>(zones_stat_date);

	for (auto &i : zone_table) {
		pugi::xml_node zone_node = node_list.append_child();
		zone_node.set_name("zone");
		zone_node.append_attribute("vnum") = i.vnum;
		zone_node.append_attribute("traffic") = i.traffic;
	}

	doc.save_file(ZONE_TRAFFIC_FILE);
}
void zone_traffic_load() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(ZONE_TRAFFIC_FILE);
	if (!result) {
		snprintf(buf, kMaxStringLength, "...%s", result.description());
		mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
		return;
	}
	pugi::xml_node node_list = doc.child("zone_traffic");
	pugi::xml_node time_node = node_list.child("time");
	pugi::xml_attribute xml_date = time_node.attribute("date");
	if (xml_date) {
		zones_stat_date = static_cast<time_t>(xml_date.as_ullong());
	}
	for (pugi::xml_node node = node_list.child("zone"); node; node = node.next_sibling("zone")) {
		const int zone_vnum = atoi(node.attribute("vnum").value());
		ZoneRnum zrn;
		zrn = GetZoneRnum(zone_vnum);
		int num = atoi(node.attribute("traffic").value());
		if (zrn == kNoZone) {
			snprintf(buf, kMaxStringLength,
					 "zone_traffic: несуществующий номер зоны %d ее траффик %d ",
					 zone_vnum, num);
			mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
			continue;
		}
		zone_table[zrn].traffic = atoi(node.attribute("traffic").value());
	}
}

// body of the booting system
void BootMudDataBase() {
	auto boot_start = std::chrono::high_resolution_clock::now();
	utils::CSteppedProfiler boot_profiler("MUD booting", 1.1);

	log("Boot db -- BEGIN.");

	boot_profiler.next_step("Creating directories");
	struct stat st{};
#define MKDIR(dir) if (stat((dir), &st) != 0) \
        mkdir((dir), 0744)
#define MKLETTERS(BASE) MKDIR(#BASE); \
        MKDIR(#BASE "/A-E"); \
        MKDIR(#BASE "/F-J"); \
        MKDIR(#BASE "/K-O"); \
        MKDIR(#BASE "/P-T"); \
        MKDIR(#BASE "/U-Z"); \
        MKDIR(#BASE "/ZZZ")

	MKDIR("etc");
	MKDIR("etc/board");
	MKLETTERS(plralias);
	MKLETTERS(plrobjs);
	MKLETTERS(plrs);
	MKLETTERS(plrvars);
	MKDIR("plrstuff");
	MKDIR("plrstuff/depot");
	MKLETTERS(plrstuff / depot);
	MKDIR("plrstuff/house");
	MKDIR("stat");

#undef MKLETTERS
#undef MKDIR

	boot_profiler.next_step("Initialization of text IDs");
	log("Init TextId list.");
	text_id::Init();

	boot_profiler.next_step("Resetting the game time");
	log("Resetting the game time:");
	ResetGameWorldTime();

	boot_profiler.next_step("Reading credits, help, bground, info & motds.");
	log("Reading credits, help, bground, info & motds.");
	AllocateBufferForFile(HELP_PAGE_FILE, &help);
	MUD::CfgManager().LoadCfg("system_msg");

	boot_profiler.next_step("Loading currencies cfg.");
	log("Loading currencies cfg.");
	// issue.thing-names: currency names (currency_msg.xml) load before currencies.xml so the builder
	// can read each currency's display name from the message file.
	MUD::CfgManager().LoadCfg("currency_msg");
	MUD::CfgManager().LoadCfg("currencies");

	// issue.thing-names: skill messages (which now hold skill names + abbreviations) load before skills.
	boot_profiler.next_step("Loading skill messages cfg.");
	log("Loading skill messages cfg.");
	MUD::CfgManager().LoadCfg("skill_msg");

	boot_profiler.next_step("Loading skills cfg.");
	log("Loading skills cfg.");
	MUD::CfgManager().LoadCfg("skills");

	boot_profiler.next_step("Loading feats cfg.");
	log("Loading feats cfg.");
	MUD::CfgManager().LoadCfg("feat_msg");   // issue.thing-names: names before feats
	MUD::CfgManager().LoadCfg("feats");

	// issue.thing-names: spell messages (which now hold the Russian display names) load BEFORE
	// spells, so SpellInfoBuilder::ParseName can read each spell's name from the message container.
	boot_profiler.next_step("Loading spell messages cfg.");
	log("Loading spell messages cfg.");
	MUD::CfgManager().LoadCfg("spell_msg");

	boot_profiler.next_step("Loading spells cfg.");
	log("Loading spells cfg.");
	MUD::CfgManager().LoadCfg("spells");

	// issue.perk-action-patching: feats + spells are both loaded now -- bucket every talent patch
	// onto its target SpellInfo and validate its action ids.
	feats::BuildTalentPatchIndex();

	// issue.affect-migration: room-affect registry + messages (flags/triggers/actions and the
	// per-room-affect display/lifecycle text). After spells, since room affects reference spell ids.
	boot_profiler.next_step("Loading room affects cfg.");
	log("Loading room affects cfg.");
	MUD::CfgManager().LoadCfg("room_affect_msg");
	MUD::CfgManager().LoadCfg("room_affects");

	boot_profiler.next_step("Linting editor schemes.");
	vedun::LintSchemes();

	boot_profiler.next_step("Loading points intensity cfg.");
	log("Loading points intensity cfg.");
	MUD::CfgManager().LoadCfg("points_intensity");


    boot_profiler.next_step("Loading hit type messages cfg.");
    log("Loading hit type messages cfg.");
    MUD::CfgManager().LoadCfg("hit_msg");

	boot_profiler.next_step("Loading abilities definitions");
	log("Loading abilities.");
	MUD::CfgManager().LoadCfg("abilities");

	boot_profiler.next_step("Loading daily quests");
	log("Loading daily quests.");
	MUD::CfgManager().LoadCfg("daily_quest");   // issue.daily-quest: cfg/quests/daily_quest.xml

	boot_profiler.next_step("Loading undecayable object criterions");
	log("Loading undecayable object criterions.");
	MUD::CfgManager().LoadCfg("stable_objs");

	boot_profiler.next_step("Loading ingredients magic");
	log("Booting IM");
	initIngredientsMagic();

	boot_profiler.next_step("Assigning character classs info.");
	log("Assigning character classs info.");
	MUD::CfgManager().LoadCfg("class_msg");   // issue.thing-names: names/abbr before classes
	MUD::CfgManager().LoadCfg("pc_classes");
	MUD::CfgManager().LoadCfg("experience_table");   // issue.experience-table

	boot_profiler.next_step("Loading rune spells cfg");
	log("Loading rune spells cfg.");
	MUD::CfgManager().LoadCfg("rune_spells");

	boot_profiler.next_step("Loading zone types and ingredient for each zone type");
	log("Booting zone types and ingredient types for each zone type.");
	MUD::CfgManager().LoadCfg("entity_names");   // issue.thing-names: names before mob_classes/races/zone_types
	MUD::CfgManager().LoadCfg("zone_types");

	boot_profiler.next_step("Loading gurdians");
	log("Load guardians.");
	MUD::CfgManager().LoadCfg("guards");   // issue.guards: cfg/mechanics/guards.xml

	boot_profiler.next_step("Loading world");
	GameLoader::BootWorld();

	boot_profiler.next_step("Loading stuff load table");
	log("Booting stuff load table.");
	oload_table.init();

	boot_profiler.next_step("Loading item levels");
	log("Init item levels.");
	ObjSystem::init_item_levels();

	boot_profiler.next_step("Loading help entries");
	log("Loading help entries.");
	GameLoader::BootIndex(DB_BOOT_HLP);

	boot_profiler.next_step("Loading socials");
	log("Loading socials.");
	MUD::CfgManager().LoadCfg("social_msg");

	boot_profiler.next_step("Loading players index");
	log("Generating player index.");
	BuildPlayerIndexNew();

	// хэши читать после генерации плеер-таблицы
	boot_profiler.next_step("Loading CRC system");
	log("Loading file crc system.");
	FileCRC::load();

	boot_profiler.next_step("Assigning function pointers");
	log("Assigning function pointers:");
	if (!no_specials) {
		log("   Mobiles.");
		AssignMobiles();
		log("   Objects.");
		AssignObjects();
		log("   Rooms.");
		AssignRooms();
	}

	boot_profiler.next_step("Reading skills variables.");
	log("Reading skills variables.");
	MUD::CfgManager().LoadCfg("digging");
	MUD::CfgManager().LoadCfg("jewelry");

	boot_profiler.next_step("Sorting command list");
	log("Sorting command list.");
	SortCommands();

	boot_profiler.next_step("Reading banned site, proxy, privileges and invalid-name list.");
	log("Reading banned site, proxy, privileges and invalid-name list.");
	ban = new BanList();
	ReadCharacterInvalidNamesList();

	boot_profiler.next_step("Loading rented objects info");
	log("Booting rented objects info");
	for (std::size_t i = 0; i < player_table.size(); i++) {
		player_table[i].timer = nullptr;
		ReadCrashTimerFile(i, false);
	}

	// последовательность лоада кланов/досок не менять
	boot_profiler.next_step("Loading boards");
	log("Booting boards");
	Boards::Static::BoardInit();

	boot_profiler.next_step("loading clans");
	log("Booting clans");
	Clan::ClanLoad();

	// загрузка списка именных вещей
	boot_profiler.next_step("Loading named stuff");
	log("Load named stuff");
	NamedStuff::load();

	boot_profiler.next_step("Loading basic values");
	log("Booting basic values");
	MUD::CfgManager().LoadCfg("basic");

	boot_profiler.next_step("Loading grouping parameters");
	log("Booting grouping parameters");
	MUD::CfgManager().LoadCfg("group_exp_handicap");
	if (!grouping.loaded()) {
		fatal_log("Failed to load grouping parameters");
	}

	boot_profiler.next_step("Loading special assignments");
	log("Booting special assignment");
	MUD::CfgManager().LoadCfg("specials");
	MUD::CfgManager().LoadCfg("special_msg");   // issue.specials Phase 2: spec-proc messages
	MUD::CfgManager().LoadCfg("bank_msg");
	MUD::CfgManager().LoadCfg("mail_msg");
	MUD::CfgManager().LoadCfg("horse_msg");
	MUD::CfgManager().LoadCfg("torc_msg");
	MUD::CfgManager().LoadCfg("mercenary_msg");
	MUD::CfgManager().LoadCfg("exchange_msg");
	MUD::CfgManager().LoadCfg("rent_msg");
	MUD::CfgManager().LoadCfg("shop_msg");
	MUD::CfgManager().LoadCfg("board_msg");
	// "affects" + "affect_messages" load earlier, at the top of BootWorld (affected_bits must exist
	// before the world's objects/mobs are parsed) -- see GameLoader::BootWorld.

	boot_profiler.next_step("Assigning guilds info.");
	log("Assigning guilds info.");
	MUD::CfgManager().LoadCfg("guild_msg");   // issue.thing-names: messages before guilds
	MUD::CfgManager().LoadCfg("guilds");

	boot_profiler.next_step("Assigning mob classes info.");
	log("Assigning mob classes info.");
	MUD::CfgManager().LoadCfg("mob_classes");

	boot_profiler.next_step("Loading mob races");
	log("Load mob races.");
	MUD::CfgManager().LoadCfg("mob_races");
	MUD::CfgManager().LoadCfg("cities_msg");   // issue.cities: names before cities
	MUD::CfgManager().LoadCfg("cities");
	MUD::CfgManager().LoadCfg("region_msg");   // issue.regions: messages before regions
	MUD::CfgManager().LoadCfg("regions");
	MUD::CfgManager().LoadCfg("pc_race_msg");   // issue.player-races-rework: names before races
	MUD::CfgManager().LoadCfg("pc_races");

	boot_profiler.next_step("Loading runestones for 'town portal' spell");
	log("Booting runestones for 'town portal' spell");
	MUD::CfgManager().LoadCfg("rune_stone_msg");   // issue.runestones: names before the registry
	MUD::CfgManager().LoadCfg("rune_stones");
	MUD::Runestones().SpawnStones();   // phase 3: place the physical stone object into each room

	boot_profiler.next_step("Loading made items");
	log("Booting maked items");
	MUD::CfgManager().LoadCfg("item_creation");

	boot_profiler.next_step("Loading exchange");
	log("Booting exchange");
	LoadExchange();

	boot_profiler.next_step("Loading scheduled reboot time");
	log("Load schedule reboot time");
	LoadSheduledReboot();

	boot_profiler.next_step("Loading proxies list");
	log("Load proxy list");
	RegisterSystem::LoadProxyList();

	boot_profiler.next_step("Loading new_name list");
	log("Load new_name list");
	NewNames::load();

	boot_profiler.next_step("Loading global UID timer");
	log("Load global uid counter");
	LoadGlobalUid();

	boot_profiler.next_step("Initializing DT list");
	log("Init DeathTrap list.");
	deathtrap::load();

	boot_profiler.next_step("Loading titles list");
	log("Load Title list.");
	TitleSystem::load_title_list();

	boot_profiler.next_step("Loading emails list");
	log("Load registered emails list.");
	RegisterSystem::load();

	boot_profiler.next_step("Loading privileges and gods list");
	log("Load privilege and god list.");
	privilege::Load();
	MUD::CfgManager().LoadCfg("privilege");  // issue.privilege-rework P1: membership DB (inert until P2)

	// должен идти до резета зон
	boot_profiler.next_step("Initializing depot system");
	log("Init Depot system.");
	Depot::init_depot();

	boot_profiler.next_step("Loading Parcel system");
	log("Init Parcel system.");
	Parcel::load();

	boot_profiler.next_step("Loading celebrates");
	log("Load Celebrates.");
	MUD::CfgManager().LoadCfg("celebrates");   // issue.celebrates: cfg/mechanics/celebrates.xml

	// Триггера комнат должны быть подключены ДО первого ResetZone, иначе
	// reset_wtrigger / random_wtrigger не найдут их в SCRIPT(room) и не
	// сработают на boot-ресете - и зоны с WTRIG_RESET (например 974, где
	// мобы лоадятся через триггер 97433) выходят из бута пустыми. Для
	// legacy-загрузчика двойной аттач не случится: TriggersList::add
	// дедуп по rnum, dg_read_trigger уже подключил инстансы при чтении
	// wld-файла. Для YAML/SQLite это единственное место, где триггеры
	// комнат попадают в SCRIPT(room).
	boot_profiler.next_step("Assigning triggers to rooms");
	world_loader::WorldDataSourceBase::AssignTriggersToLoadedRooms();

	// резет должен идти после лоада всех шмоток вне зон (хранилища и т.п.)
	boot_profiler.next_step("Resetting zones");
	for (ZoneRnum i = 0; i < static_cast<ZoneRnum>(zone_table.size()); i++) {
		log("Resetting %s (rooms %d-%d).", zone_table[i].name.c_str(),
			(i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);
		ResetZone(i);
	}
	reset_q.clear();


	// делается после резета зон, см камент к функции
	boot_profiler.next_step("Loading depot chests");
	log("Load depot chests.");
	Depot::load_chests();

	boot_profiler.next_step("Loading glory");
	log("Load glory.");
	Glory::load_glory();
	log("Load const glory.");
	GloryConst::load();
	log("Load glory log.");
	GloryMisc::load_log();

	log("Load zone traffic.");
	zone_traffic_load();

	boot_profiler.next_step("Initializing global drop list");
	log("Init global drop list.");
	GlobalDrop::init();

	boot_profiler.next_step("Loading offtop block list");
	log("Init offtop block list.");
	offtop_system::Init();

	boot_profiler.next_step("Loading shop_ext list");
	log("load shop_ext list start.");
	MUD::CfgManager().LoadCfg("shop_item_sets");   // issue.shops-ext: catalog before shops
	ShopExt::load(false);
	log("load shop_ext list stop.");

	boot_profiler.next_step("Loading zone average mob_level");
	log("Set zone average mob_level.");
	SetZoneMobLevel();

	boot_profiler.next_step("Setting zone town");
	log("Set zone town.");
	SetZonesTownFlags();

	boot_profiler.next_step("Initializing town shop_keepers");
	log("Init town shop_keepers.");
	town_shop_keepers();

	/* Commented out until full implementation
	boot_profiler.next_step("Loading crafts system");
	log("Starting crafts system.");
	if (!crafts::start())
	{
		log("ERROR: Failed to start crafts system.\n");
	}
	*/

	boot_profiler.next_step("Loading big sets in rent");
	log("Check big sets in rent.");
	SetSystem::check_rented();

	// сначала сеты, стата мобов, потом дроп сетов
	boot_profiler.next_step("Loading object sets/mob_stat/drop_sets lists");
	MUD::CfgManager().LoadCfg("obj_sets");   // issue.obj-sets: cfg/mechanics/obj_sets.xml
	log("Load mob_stat.xml");
	mob_stat::Load();
	log("Init SetsDrop lists.");
	SetsDrop::init();

	boot_profiler.next_step("Loading noob.xml");
	log("Load noob.xml");
	MUD::CfgManager().LoadCfg("noob");

	boot_profiler.next_step("Loading reset_stats.xml");
	log("Load reset_stats.xml");
	MUD::CfgManager().LoadCfg("reset_stats");

	boot_profiler.next_step("Loading remort.xml");
	log("Load remort.xml");
	MUD::CfgManager().LoadCfg("remort");

	boot_profiler.next_step("Loading mail.xml");
	log("Load mail.xml");
	mail::load();

	// загрузка кейсов
	boot_profiler.next_step("Loading treasure cases");
	log("Loading treasure cases.");
	MUD::CfgManager().LoadCfg("cases");   // issue.lib-template: cfg/mechanics/cases.xml

	// справка должна иниться после всего того, что может в нее что-то добавить
	boot_profiler.next_step("Reloading help system");
	log("Loading help system.");
	HelpSystem::reload_all();

	boot_profiler.next_step("Loading bonus log");
	log("Loading bonus log.");
	Bonus::bonus_log_load();

	boot_profiler.next_step("Loading cities cfg");
	log("Loading cities cfg.");

	shutdown_parameters.mark_boot_time();
	log("Boot db -- DONE.");

	auto boot_end = std::chrono::high_resolution_clock::now();
	auto boot_duration = std::chrono::duration<double>(boot_end - boot_start).count();
	log("Boot db total time: %.3f seconds", boot_duration);

}

// reset the time in the game from file
void ResetGameWorldTime() {
	time_info = *CalcMudTimePassed(time(nullptr), kBeginningOfTime);
	// Calculate moon day
	weather_info.moon_day =
		((time_info.year * kMonthsPerYear + time_info.month) * kDaysPerMonth + time_info.day) % kMoonCycle;
	weather_info.week_day_mono =
		((time_info.year * kMonthsPerYear + time_info.month) * kDaysPerMonth + time_info.day) % kWeekCycle;
	weather_info.week_day_poly =
		((time_info.year * kMonthsPerYear + time_info.month) * kDaysPerMonth + time_info.day) % kPolyWeekCycle;
	// Calculate Easter
	CalcEaster();

	if (time_info.hours < sunrise[time_info.month][0]) {
		weather_info.sunlight = kSunDark;
	} else if (time_info.hours == sunrise[time_info.month][0]) {
		weather_info.sunlight = kSunRise;
	} else if (time_info.hours < sunrise[time_info.month][1]) {
		weather_info.sunlight = kSunLight;
	} else if (time_info.hours == sunrise[time_info.month][1]) {
		weather_info.sunlight = kSunSet;
	} else {
		weather_info.sunlight = kSunDark;
	}

	log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours, time_info.day, time_info.month, time_info.year);

	weather_info.temperature = (year_temp[time_info.month].med * 4 +
		number(year_temp[time_info.month].min, year_temp[time_info.month].max)) / 5;
	weather_info.pressure = 960;
	if ((time_info.month >= EMonth::kMay) && (time_info.month <= EMonth::kNovember))
		weather_info.pressure += RollDices(1, 50);
	else
		weather_info.pressure += RollDices(1, 80);

	weather_info.change = 0;
	weather_info.weather_type = 0;

	if ((time_info.month >= EMonth::kApril && time_info.month <= EMonth::kMay) ||
		(time_info.month == EMonth::kMarch && weather_info.temperature >= 3)) {
		weather_info.season = ESeason::kSpring;
	} else if (time_info.month >= EMonth::kJune && time_info.month <= EMonth::kAugust) {
		weather_info.season = ESeason::kSummer;
	} else if ((time_info.month >= EMonth::kSeptember && time_info.month <= EMonth::kOctober) ||
		(time_info.month == EMonth::kNovember && weather_info.temperature >= 3)) {
		weather_info.season = ESeason::kAutumn;
	} else {
		weather_info.season = ESeason::kWinter;
	}

	if (weather_info.pressure <= 980)
		weather_info.sky = kSkyLightning;
	else if (weather_info.pressure <= 1000) {
		weather_info.sky = kSkyRaining;
		if (time_info.month >= EMonth::kApril && time_info.month <= EMonth::kOctober)
			SetPrecipitations(&weather_info.weather_type, kWeatherLightrain, 40, 40, 20);
		else if (time_info.month >= EMonth::kDecember || time_info.month <= EMonth::kFebruary)
			SetPrecipitations(&weather_info.weather_type, kWeatherLightsnow, 50, 40, 10);
		else if (time_info.month == EMonth::kNovember || time_info.month == EMonth::kMarch) {
			if (weather_info.temperature >= 3)
				SetPrecipitations(&weather_info.weather_type, kWeatherLightrain, 70, 30, 0);
			else
				SetPrecipitations(&weather_info.weather_type, kWeatherLightsnow, 80, 20, 0);
		}
	} else if (weather_info.pressure <= 1020)
		weather_info.sky = kSkyCloudy;
	else
		weather_info.sky = kSkyCloudless;
}

int GameLoader::ResaveWorld(const std::string &target_dir, const std::string &target_format) {
	namespace fs = std::filesystem;

	// Resolve target_format: empty / "auto" -> compile-time default.
	std::string fmt = target_format;
	if (fmt.empty() || fmt == "auto") {
#if defined(HAVE_YAML)
		fmt = "yaml";
#elif defined(HAVE_SQLITE)
		fmt = "sqlite";
#else
		fmt = "legacy";
#endif
	}

	std::unique_ptr<world_loader::IWorldDataSource> saver;

	if (fmt == "yaml") {
#ifdef HAVE_YAML
		// YamlWorldDataSource's constructor refuses to instantiate without a
		// readable world_config.yaml under the data root. For a save-only
		// instance, copy the original config (and dictionaries) from the load
		// location -- which BootWorld used at the compile-time default of
		// "world" -- before constructing. SaveZone/Save* rebuild their
		// index.yaml files themselves now, so we don't mirror any indexes here.
		const std::string load_dir = "world";
		try {
			fs::create_directories(target_dir);
			fs::copy_file(load_dir + "/world_config.yaml",
						  target_dir + "/world_config.yaml",
						  fs::copy_options::overwrite_existing);
			if (fs::exists(load_dir + "/dictionaries")) {
				fs::copy(load_dir + "/dictionaries",
						 target_dir + "/dictionaries",
						 fs::copy_options::recursive
						 | fs::copy_options::overwrite_existing);
			}
		} catch (const std::exception &e) {
			log("SYSERR: ResaveWorld bootstrap failed: %s", e.what());
			return 1;
		}
		saver = world_loader::CreateYamlDataSource(target_dir);
#else
		log("SYSERR: ResaveWorld: yaml backend requested but not compiled in");
		return 1;
#endif
	} else if (fmt == "sqlite") {
#ifdef HAVE_SQLITE
		saver = world_loader::CreateSqliteDataSource(target_dir);
#else
		log("SYSERR: ResaveWorld: sqlite backend requested but not compiled in");
		return 1;
#endif
	} else if (fmt == "legacy") {
		// Legacy save uses OLC functions that write to "world/{wld,mob,...}/"
		// relative to cwd. LegacyWorldDataSource owns its world_dir and
		// chdir's into it for each Save* call (creating the layout first), so
		// the saved tree lands under <target_dir>/world/. Symmetric with the
		// YAML/SQLite factories below.
		saver = world_loader::CreateLegacyDataSource(target_dir);
	} else {
		log("SYSERR: ResaveWorld: unknown target format '%s'", fmt.c_str());
		return 1;
	}

	log("ResaveWorld: target=%s, format=%s, zones=%zu",
		target_dir.c_str(), fmt.c_str(), zone_table.size());
	int errors = 0;
	int skipped = 0;
	saver->BeginBulkWrite();
	for (size_t z = 0; z < zone_table.size(); ++z) {
		// Dungeon zones (CreateBlankZoneDungeon, vnum >= kZoneStartDungeons)
		// are generated in-memory and never persisted -- matches legacy's
		// medit_save_to_disk guard at olc/medit.cpp:532. `under_construction`
		// alone is not the right filter: ordinary prod zones may carry it as
		// a "work in progress" marker (zone 73 "Светлый лес" etc.).
		if (zone_table[z].vnum >= dungeons::kZoneStartDungeons) {
			++skipped;
			continue;
		}
		if (!SaveZoneAllEntities(*saver, static_cast<int>(z), zone_table[z].vnum, "ResaveWorld")) {
			++errors;
		}
	}
	saver->EndBulkWrite();

	// Let the backend finalize after the full resave: legacy rebuilds its
	// boot indexes here (YAML/SQLite already maintain them inside Save*).
	try {
		saver->FinalizeResave();
	} catch (const std::exception &e) {
		log("SYSERR: ResaveWorld: finalize failed: %s", e.what());
		++errors;
	}

	log("ResaveWorld: done, errors=%d, skipped_dungeon=%d", errors, skipped);
	return errors;
}

void GameLoader::BootIndex(const EBootType mode) {
	log("Index booting %d", mode);

	auto index = IndexFileFactory::get_index(mode);
	if (!index->open()) {
		fatal_log("Failed to open index file (mode=%d)", static_cast<int>(mode));
	}
	const int rec_count = index->load();

	PrepareGlobalStructures(mode, rec_count);

	const auto data_file_factory = DataFileFactory::create();
	for (const auto &entry : *index) {
		auto data_file = data_file_factory->get_file(mode, entry);
		if (!data_file->open()) {
			continue;    // TODO: we need to react somehow.
		}
		if (!data_file->load()) {
			// TODO: do something
		}
		// brackets to suppress define
		data_file->close();
	}
}

void GameLoader::PrepareGlobalStructures(const EBootType mode, const int rec_count) {
	// * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
	switch (mode) {
		case DB_BOOT_TRG: CREATE(trig_index, rec_count);
			break;

		case DB_BOOT_WLD: {
			// Creating empty world with kNowhere room.
			world.push_back(new RoomData);
			top_of_world = kNowhere;
			const size_t rooms_bytes = sizeof(RoomData) * rec_count;
			log("   %d rooms, %zd bytes.", rec_count, rooms_bytes);
		}
			break;

		case DB_BOOT_MOB: {
			mob_proto = new CharData[rec_count]; // TODO: переваять на вектор (+в medit)
			CREATE(mob_index, rec_count);
			const size_t index_size = sizeof(IndexData) * rec_count;
			const size_t characters_size = sizeof(CharData) * rec_count;
			log("   %d mobs, %zd bytes in index, %zd bytes in prototypes.", rec_count, index_size, characters_size);
		}
			break;

		case DB_BOOT_OBJ:
			log("   %d objs, ~%zd bytes in index, ~%zd bytes in prototypes.",
				rec_count,
				obj_proto.index_size(),
				obj_proto.prototypes_size());
			break;

		case DB_BOOT_ZON: {
			zone_table.reserve(rec_count + dungeons::kNumberOfZoneDungeons);
			zone_table.resize(rec_count);
			const size_t zones_size = sizeof(ZoneData) * rec_count;
			log("   %d zones, %zd bytes.", rec_count, zones_size);
		}
			break;

		case DB_BOOT_HLP: break;

	}
}

void CheckRoomForIncompatibleFlags(int rnum) {
	if (deathtrap::IsSlowDeathtrap(rnum)) {
		// снятие номагик и прочих флагов, запрещающих чару выбраться из комнаты без выходов при наличии медленного дт
		world[rnum]->unset_flag(ERoomFlag::kNoMagic);
		world[rnum]->unset_flag(ERoomFlag::kNoTeleportOut);
		world[rnum]->unset_flag(ERoomFlag::kNoSummonOut);
	}
	if (world[rnum]->get_flag(ERoomFlag::kHouse)
		&& (SECT(rnum) == ESector::kMountain || SECT(rnum) == ESector::kHills)) {
		// шоб в замках умные не копали
		SECT(rnum) = ESector::kInside;
	}
}

// make sure the start rooms exist & resolve their vnums to rnums
void CheckStartRooms() {
	if ((r_mortal_start_room = GetRoomRnum(mortal_start_room)) == kNowhere) {
		fatal_log("SYSERR:  Mortal start room does not exist.  Change in config.c. %d", mortal_start_room);
	}

	if ((r_immort_start_room = GetRoomRnum(immort_start_room)) == kNowhere) {
		log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
		r_immort_start_room = r_mortal_start_room;
	}

	if ((r_frozen_start_room = GetRoomRnum(frozen_start_room)) == kNowhere) {
		log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
		r_frozen_start_room = r_mortal_start_room;
	}

	if ((r_helled_start_room = GetRoomRnum(helled_start_room)) == kNowhere) {
		log("SYSERR:  Warning: Hell start room does not exist.  Change in config.c.");
		r_helled_start_room = r_mortal_start_room;
	}

	if ((r_named_start_room = GetRoomRnum(named_start_room)) == kNowhere) {
		log("SYSERR:  Warning: NAME start room does not exist.  Change in config.c.");
		r_named_start_room = r_mortal_start_room;
	}

	if ((r_unreg_start_room = GetRoomRnum(unreg_start_room)) == kNowhere) {
		log("SYSERR:  Warning: UNREG start room does not exist.  Change in config.c.");
		r_unreg_start_room = r_mortal_start_room;
	}
}

void RestoreRoomExitData(RoomRnum rrn) {
	for (int dir = 0; dir < EDirection::kMaxDirNum; ++dir) {
		const auto &from = world[rrn]->dir_option_proto[dir];

		if (world[rrn]->dir_option[dir]) {
			world[rrn]->dir_option[dir].reset();
		}
		if (from) {
			world[rrn]->dir_option[dir] = std::make_shared<ExitData>();
			world[rrn]->dir_option[dir]->to_room(from->to_room());
			if (!from->general_description.empty()) {
				world[rrn]->dir_option[dir]->general_description = from->general_description;
			}
			if (from->keyword) {
				world[rrn]->dir_option[dir]->set_keyword(from->keyword);
			}
			if (from->vkeyword) {
				world[rrn]->dir_option[dir]->set_vkeyword(from->vkeyword);
			}
			world[rrn]->dir_option[dir]->exit_info = from->exit_info;
			world[rrn]->dir_option[dir]->key = from->key;
			world[rrn]->dir_option[dir]->lock_complexity = from->lock_complexity;
		}
	}
}

void CalculateFirstAndLastMobs() {
}

void CalculateFirstAndLastRooms() {
	int current = world[1]->zone_rn;
	RoomRnum rn;
	ZoneRnum zrn = 0;

	zone_table[0].RnumRoomsLocation.first = 1;
	for (rn = 1; rn <= static_cast<RoomRnum>(world.size() - 1); rn++) {
		zrn = world[rn]->zone_rn;
		if (current != zrn) {
			zone_table[zrn].RnumRoomsLocation.first = rn;
			zone_table[current].RnumRoomsLocation.second = rn - 1;
			current = zrn;
		}
	}
	zone_table[zrn].RnumRoomsLocation.first = zone_table[zrn - 1].RnumRoomsLocation.second + 1;
	zone_table[zrn].RnumRoomsLocation.second = rn - 1;
	for (size_t i = 0; i < zone_table.size(); ++i) {
		auto &zone_data = zone_table[i];
		zone_data.RnumRoomsLocation.second--; //уберем виртуалки
		if (zone_data.entrance == 0) {  //если в зонфайле не указана стартовая комната
			log("Отсутствует стартовая комната для зоны %d", zone_data.vnum);
			zone_data.entrance = world[zone_data.RnumRoomsLocation.first]->vnum;
		}
	}
}

void AddVirtualRoomsToAllZones() {
	for (auto it = zone_table.begin(); it < zone_table.end(); ++it) {
		const auto first_room = it->vnum * 100;
		const auto last_room = first_room + 99;

		const RoomVnum virtual_vnumum = (it->vnum * 100) + 99;
		const auto vroom_it = std::find_if(world.cbegin(), world.cend(), [virtual_vnumum](RoomData *room) {
		  return room->vnum == virtual_vnumum;
		});
		if (vroom_it != world.cend()) {
			log("Zone %d already contains virtual room.", it->vnum);
			continue;
		}
		const auto rnum = static_cast<ZoneRnum>(std::distance(zone_table.begin(), it));

		// ищем место для вставки новой комнаты с конца, чтобы потом не вычитать 1 из результата
		auto insert_reverse_position = std::find_if(world.rbegin(), world.rend(), [rnum](RoomData *room) {
		  return rnum >= room->zone_rn;
		});
		auto insert_position =
			(insert_reverse_position == world.rend()) ? world.begin() : insert_reverse_position.base();

		top_of_world++;
		auto *new_room = new RoomData;
		world.insert(insert_position, new_room);
		new_room->zone_rn = rnum;
		new_room->vnum = last_room;
		new_room->set_name(std::string("Виртуальная комната"));
		new_room->description_num = GlobalObjects::descriptions().add(std::string("Похоже, здесь вам делать нечего."));
		new_room->clear_flags();
		new_room->sector_type = ESector::kSecret;

		new_room->func = nullptr;
		new_room->contents.clear();
		new_room->track = nullptr;
		new_room->light = 0;
		new_room->fires = 0;
		new_room->gdark = 0;
		new_room->glight = 0;
		new_room->proto_script = std::make_shared<ObjData::triggers_list_t>();
	}
}

void RosolveWorldDoorToRoomVnumsToRnums() {
	for (auto room = kFirstRoom; room <= top_of_world; room++) {
		for (auto door = 0; door < EDirection::kMaxDirNum; door++) {
			if (world[room]->dir_option_proto[door]) {
				if (world[room]->dir_option_proto[door]->to_room() != kNowhere) {
					const auto to_room = GetRoomRnum(world[room]->dir_option_proto[door]->to_room());
					world[room]->dir_option_proto[door]->to_room(to_room);
				}
			}
		}
	}
}

void SetZoneRnumForObjects() {
	for (size_t i = 0; i < obj_proto.size(); ++i) {
		obj_proto.zone(i, get_zone_rnum_by_obj_vnum(obj_proto[i]->get_vnum()));
	}
}

void SetZoneRnumForMobiles() {
	int i;
	for (i = 0; i <= top_of_mobt; ++i) {
		mob_index[i].zone = get_zone_rnum_by_mob_vnum(mob_index[i].vnum);
	}
}

void SetZoneRnumForTriggers() {
	// Calculate RnumTrigsLocation (first and last trigger rnum) for each zone
	// This works for all data source formats (Legacy, YAML, SQLite)

	// First pass: find first and last trigger for each zone
	// NOTE: top_of_trigt is the COUNT, not the last index (unlike top_of_mobt)
	for (int i = 0; i < top_of_trigt; ++i) {
		if (!trig_index[i]) {
			continue;
		}

		int trig_vnum = trig_index[i]->vnum;
		ZoneVnum zone_vnum = trig_vnum / 100;
		ZoneRnum zone_rnum = GetZoneRnum(zone_vnum);

		if (zone_rnum == kNoZone) {
			log("FATAL: Trigger %d belongs to non-existent zone %d (zone_table has %zu zones)",
				trig_vnum, zone_vnum, zone_table.size());
			log("FATAL: This indicates broken world data or initialization order bug.");
			fatal_log("FATAL: Boot aborted. Triggers cannot function without valid zone assignment.");
		}

		// Set first trigger for this zone (if not set yet)
		if (zone_table[zone_rnum].RnumTrigsLocation.first == -1) {
			zone_table[zone_rnum].RnumTrigsLocation.first = i;
		}

		// Always update last trigger for this zone
		zone_table[zone_rnum].RnumTrigsLocation.second = i;
	}
}

void ResolveZoneCmdVnumArgsToRnums(ZoneData &zone_data) {
	int cmd_no, a, b, c, olda, oldb, oldc;
	char local_buf[128];
	int i;
	for (i = 0; i < zone_data.typeA_count; i++) {
		if (GetZoneRnum(zone_data.typeA_list[i]) == kNoZone) {
			sprintf(local_buf,
					"SYSERROR: некорректное значение в typeA (%d) для зоны: %d",
					zone_data.typeA_list[i],
					zone_data.vnum);
			mudlog(local_buf, CMP, kLvlGreatGod, SYSLOG, true);
		}
	}
	for (i = 0; i < zone_data.typeB_count; i++) {
		if (GetZoneRnum(zone_data.typeB_list[i]) == kNoZone) {
			sprintf(local_buf,
					"SYSERROR: некорректное значение в typeB (%d) для зоны: %d",
					zone_data.typeB_list[i],
					zone_data.vnum);
			mudlog(local_buf, CMP, kLvlGreatGod, SYSLOG, true);
		}
	}

	if (!zone_data.cmd) {
		return;
	}
	for (cmd_no = 0; zone_data.cmd[cmd_no].command != 'S'; cmd_no++) {
		auto &reset_cmd = zone_data.cmd[cmd_no];
		a = b = c = 0;
		olda = reset_cmd.arg1;
		oldb = reset_cmd.arg2;
		oldc = reset_cmd.arg3;
		switch (reset_cmd.command) {
			case 'M': a = reset_cmd.arg1 = GetMobRnum(reset_cmd.arg1);
				if (reset_cmd.arg2 <= 0 || reset_cmd.arg1 <= 0) {
					auto msg = fmt::format("SYSERROR: incorrect M cmd: zone {} rnum {}, stored {} room {}",
										   zone_data.vnum,
										   reset_cmd.arg1,
										   reset_cmd.arg2,
										   reset_cmd.arg3);
					mudlog(msg, CMP, kLvlGreatGod, SYSLOG, true);
					reset_cmd.command = '*';
				} else {
					if (mob_index[reset_cmd.arg1].stored < reset_cmd.arg2) {
						mob_index[reset_cmd.arg1].stored = reset_cmd.arg2;
					}
					c = reset_cmd.arg3 = GetRoomRnum(reset_cmd.arg3);
				}
				break;
			case 'F': a = reset_cmd.arg1 = GetRoomRnum(reset_cmd.arg1);
				b = reset_cmd.arg2 = GetMobRnum(reset_cmd.arg2);
				c = reset_cmd.arg3 = GetMobRnum(reset_cmd.arg3);
				break;
			case 'O': a = reset_cmd.arg1 = GetObjRnum(reset_cmd.arg1);
				if (reset_cmd.arg3 != kNowhere)
					c = reset_cmd.arg3 = GetRoomRnum(reset_cmd.arg3);
				break;
			case 'G': [[fallthrough]];
			case 'E': a = reset_cmd.arg1 = GetObjRnum(reset_cmd.arg1);
				break;
			case 'P': a = reset_cmd.arg1 = GetObjRnum(reset_cmd.arg1);
				c = reset_cmd.arg3 = GetObjRnum(reset_cmd.arg3);
				break;
			case 'D': a = reset_cmd.arg1 = GetRoomRnum(reset_cmd.arg1);
				break;
			case 'R':    // rem obj from room
				a = reset_cmd.arg1 = GetRoomRnum(reset_cmd.arg1);
				b = reset_cmd.arg2 = GetObjRnum(reset_cmd.arg2);
				break;
			case 'Q': a = reset_cmd.arg1 = GetMobRnum(reset_cmd.arg1);
				break;
			case 'T':    // a trigger
				// designer's choice: convert this later
				// b = reset_cmd.arg2 = GetTriggerRnum(reset_cmd.arg2);
				b = reset_cmd.arg2 = GetTriggerRnum(reset_cmd.arg2);
				if (reset_cmd.arg1 == WLD_TRIGGER)
					c = reset_cmd.arg3 = GetRoomRnum(reset_cmd.arg3);
				break;
			case 'V':    // trigger variable assignment
				if (reset_cmd.arg1 == WLD_TRIGGER)
					b = reset_cmd.arg2 = GetRoomRnum(reset_cmd.arg2);
				break;
		}
		if (a < 0 || b < 0 || c < 0) {
			sprintf(local_buf, "Invalid vnum %d, cmd disabled", (a < 0) ? olda : ((b < 0) ? oldb : oldc));
			LogZoneError(zone_data, cmd_no, local_buf);
			reset_cmd.command = '*';
		}
	}
}

// resolve vnums into rnums in the zone reset tables
void ResolveZoneTableCmdVnumArgsToRnums() {
	for (auto &zone : zone_table) {
		ResolveZoneCmdVnumArgsToRnums(zone);
	}
}

namespace {

int test_levels[] = {
	100,
	300,
	600,
	1000,
	1500,
	2100,
	2900,
	3900,
	5100,
	6500, // 10
	8100,
	10100,
	12500,
	15300,
	18500,
	22500,
	27300,
	32900,
	39300,
	46500, // 20
	54500,
	64100,
	75300,
	88100,
	102500,
	117500,
	147500,
	192500,
	252500,
	327500, // 30
	417500,
	522500,
	642500,
	777500,
	927500,
	1127500,
	1427500,
	1827500,
	2327500,
	2927500, // 40
	3627500,
	4427500,
	5327500,
	6327500,
	7427500,
	8627500,
	9927500,
	11327500,
	12827500,
	14427500
};

} // namespace

int calc_boss_value(CharData *mob, int num) {
	if (mob->get_role(static_cast<unsigned>(EMobClass::kBoss))) {
		num += num * 25 / 100;
	}
	return num;
}

void SetTestData(CharData *mob) {
	if (!mob) {
		log("SYSERROR: null mob (%s %s %d)", __FILE__, __func__, __LINE__);
		return;
	}
	if (mob->get_exp() == 0) {
		return;
	}

	if (GetRealLevel(mob) <= 50) {
		if (mob->get_exp() > test_levels[49]) {
			// log("test1: %s - %d -> %d", mob->get_name(), mob->get_level(), 50);
			mob->set_level(50);
		} else {
			if (mob->GetLevel() == 0) {
				for (int i = 0; i < 50; ++i) {
					if (test_levels[i] >= mob->get_exp()) {
						// log("test2: %s - %d -> %d", mob->get_name(), mob->get_level(), i + 1);
						mob->set_level(i + 1);

						// -10..-86
						int min_save = -(10 + 4 * (mob->GetLevel() - 31));
						min_save = calc_boss_value(mob, min_save);

						for (auto s = ESaving::kFirst; s <= ESaving::kLast; ++s) {
							if (GetSave(mob, s) > min_save) {
								SetSave(mob, s, min_save);
							}
						}
						// 20..77
						int min_cast = 20 + 3 * (mob->GetLevel() - 31);
						min_cast = calc_boss_value(mob, min_cast);

						if (GET_CAST_SUCCESS(mob) < min_cast) {
							//log("test4: %s - %d -> %d", mob->get_name(), GET_CAST_SUCCESS(mob), min_cast);
							GET_CAST_SUCCESS(mob) = min_cast;
						}

						int min_absorbe = calc_boss_value(mob, mob->GetLevel());
						if (GET_ABSORBE(mob) < min_absorbe) {
							GET_ABSORBE(mob) = min_absorbe;
						}

						break;
					}
				}
			}
		}
	}
}

/*************************************************************************
*  procedures for resetting, both play-time and boot-time                *
*************************************************************************/

namespace {

int test_hp[] = {
	20,
	40,
	60,
	80,
	100,
	120,
	140,
	160,
	180,
	200,
	220,
	240,
	260,
	280,
	300,
	340,
	380,
	420,
	460,
	520,
	580,
	640,
	700,
	760,
	840,
	1000,
	1160,
	1320,
	1480,
	1640,
	1960,
	2280,
	2600,
	2920,
	3240,
	3880,
	4520,
	5160,
	5800,
	6440,
	7720,
	9000,
	10280,
	11560,
	12840,
	15400,
	17960,
	20520,
	23080,
	25640
};

} // namespace

int get_test_hp(int lvl) {
	if (lvl > 0 && lvl <= 50) {
		return test_hp[lvl - 1];
	}
	return 1;
}

// create a new mobile from a prototype
CharData *ReadMobile(MobVnum nr, int type) {                // and MobRnum
	int is_corpse = 0;
	MobRnum i;

	if (nr < 0) {
		is_corpse = 1;
		nr = -nr;
	}

	if (type == kVirtual) {
		if ((i = GetMobRnum(nr)) < 0) {
			log("WARNING: Mobile vnum %d does not exist in database.", nr);
			return (nullptr);
		}
	} else {
		i = nr;
	}
	if (i > top_of_mobt) {
		log("WARNING: Mobile rnum %d is above the list 'top of mobt' (%s %s %d)", i, __FILE__, __func__, __LINE__);
		return (nullptr);
	}
	CharData *mob = new CharData(mob_proto[i]); //чет мне кажется что конструкции типа этой не принесут нам щастья...
	mob->proto_script = std::make_shared<ObjData::triggers_list_t>();
	mob->script = std::make_shared<Script>();    //fill it in assign_triggers from proto_script
	character_list.push_front(mob);

	// issue.npc-races: stamp this instance with the flags declared for its race. Race flags are
	// added (OR) to the mob's own flags -- they never live on the prototype, only on the loaded mob.
	{
		const auto &race_info = MUD::MobRaces()[GET_RACE(mob)];
		mob->char_specials.saved.act += race_info.GetMobFlags();
		mob->mob_specials.npc_flags += race_info.GetNpcFlags();
	}

	if (!mob->points.max_hit) {
		mob->points.max_hit = std::max(1, RollDices(mob->mem_queue.total, mob->mem_queue.stored) + mob->points.hit);
	} else {
		mob->points.max_hit = std::max(1, number(mob->points.hit, mob->mem_queue.total));
	}

	int mob_test_hp = get_test_hp(GetRealLevel(mob));
	if (mob->get_exp() > 0 && mob->points.max_hit < mob_test_hp) {
		mob->points.max_hit = mob_test_hp;
	}

	mob->points.hit = mob->points.max_hit;
	mob->mem_queue.total = mob->mem_queue.stored = 0;
	GET_HORSESTATE(mob) = 800;
	GET_LASTROOM(mob) = kNowhere;
	if (mob->mob_specials.speed <= -1)
		GET_ACTIVITY(mob) = number(0, kPulseMobile - 1);
	else
		GET_ACTIVITY(mob) = number(0, mob->mob_specials.speed);
	mob->extract_timer = 0;
	mob->points.move = mob->points.max_move;
	currencies::AddHand(*mob, currencies::kGold, RollDices(GET_GOLD_NoDs(mob), GET_GOLD_SiDs(mob)));

	mob->player_data.time.birth = time(nullptr);
	mob->player_data.time.played = 0;
	mob->player_data.time.logon = time(nullptr);

	mob->set_uid(max_id.allocate());
	if (!is_corpse) {
		mob_index[i].total_online++;
		assign_triggers(mob, MOB_TRIGGER);
	} else {
		// summoned/revived instance (negative vnum): an ally, loaded without bumping total_online
		mob->SetFlag(EMobFlag::kCompanion);
	}
	chardata_by_uid[mob->get_uid()] = mob;
	i = mob_index[i].zone;
	if (i != -1 && zone_table[i].under_construction) {
		// mobile принадлежит тестовой зоне
		mob->SetFlag(EMobFlag::kNoSummon);
	}

	if (city_guards::guardian_roster.contains(GET_MOB_VNUM(mob))) {
		mob->SetFlag(EMobFlag::kCityGuardian);
	}

	return (mob);
}

/**
// никакая это не копия, строковые и остальные поля с выделением памяти остаются общими
// мы просто отдаем константный указатель на прототип
 * \param type по дефолту VIRTUAL
 */
CObjectPrototype::shared_ptr GetObjectPrototype(ObjVnum nr, int type) {
	unsigned i = nr;
	if (type == kVirtual) {
		const int rnum = GetObjRnum(nr);
		if (rnum < 0) {
			log("Object (V) %d does not exist in database.", nr);
			return nullptr;
		} else {
			i = rnum;
		}
	}

	if (i > obj_proto.size()) {
		return nullptr;
	}
	return obj_proto[i];
}

void after_reset_zone(ZoneRnum nr_zone) {
	for (auto d = descriptor_list; d; d = d->next) {
		// Чар должен быть в игре
		if (d->state == EConState::kPlaying) {
			if (world[d->character->in_room]->zone_rn == nr_zone) {
				MarkZoneUsed(nr_zone);   // player present when zone reset -> wake + materialize fresh mobs
				return;
			}
			for (auto *k : d->character->followers) {
				if (IsCharmice(k) && world[k->in_room]->zone_rn == nr_zone) {
					MarkZoneUsed(nr_zone);   // charmice present when zone reset -> wake + materialize
					return;
				}
			}
		}
	}
}

const int ZO_DEAD{999999};

// Zone name is KOI8-R; OtelMetrics auto-converts to UTF-8.
class ZoneResetMetrics {
public:
	explicit ZoneResetMetrics(ZoneRnum zone_rnum)
		: m_zone_name(zone_table[zone_rnum].name) {}

	void RecordReset(double duration_seconds) {
		observability::OtelMetrics::RecordHistogram("zone.reset.duration", duration_seconds,
			{{"zone", m_zone_name}});
	}

	void RecordResetCount() {
		observability::OtelMetrics::RecordCounter("zone.reset.total", 1,
			{{"zone", m_zone_name}});
	}

	void RecordZoneCmdQ(double duration_seconds, MobVnum vnum) {
		observability::OtelMetrics::RecordHistogram("zone.command.Q.duration", duration_seconds,
			{{"zone", m_zone_name}, {"vnum", std::to_string(vnum)}});
	}

private:
	std::string m_zone_name; // KOI8-R; auto-converted by OtelMetrics
};

// update zone ages, queue for reset if necessary, and dequeue when possible
void ZoneUpdate() {
	int k = 0;
	static int timer = 0;
	utils::CExecutionTimer timer_count;
	// OpenTelemetry: Track zone updates
	auto zone_span = tracing::TraceManager::Instance().StartSpan("Zone Update");
	observability::ScopedMetric zone_metric("zone.update.duration");
	
	int zones_reset_count = 0;
	if (((++timer * kPulseZone) / kPassesPerSec) >= 60)    // one minute has passed
	{
		/*
		 * NOT accurate unless kPulseZone is a multiple of kPassesPerSec or a
		 * factor of 60
		 */
		timer = 0;
		for (std::size_t i = 0; i < zone_table.size(); i++) {
			if (zone_table[i].age < zone_table[i].lifespan 
					&& zone_table[i].reset_mode 
					&& (zone_table[i].reset_idle || zone_table[i].used)) {
				zone_table[i].age++;
			}
			if (zone_table[i].age >= zone_table[i].lifespan
					&& zone_table[i].age < ZO_DEAD
					&& zone_table[i].reset_mode
					&& (zone_table[i].reset_idle || zone_table[i].used)) {
				reset_q.push_back({static_cast<ZoneRnum>(i)});
				zone_table[i].age = ZO_DEAD;
			}
		}
	}
	for (auto it = reset_q.begin(); it != reset_q.end(); ) {
		const ZoneRnum zone = it->zone_to_reset;
		if (it->force_reset
			|| zone_table[zone].reset_mode == 2
			|| (zone_table[zone].reset_mode != 3 && IsZoneEmpty(zone))
			|| CanBeReset(zone)) {
			UniqueList<ZoneRnum> zone_repop_list;
			zone_repop_list.push_back(zone);
			std::stringstream out;
			out << "Auto zone reset: " << zone_table[zone].name << " (" << zone_table[zone].vnum << ")";
			// Collect typeA zones to queue individually (only for normal complex resets, not force_reset).
			// Queuing one zone per tick instead of resetting all at once avoids heartbeat spikes.
			std::vector<ZoneRnum> typeA_to_queue;
			if (!it->force_reset && zone_table[zone].reset_mode == 3) {
				for (auto i = 0; i < zone_table[zone].typeA_count; i++) {
					for (ZoneRnum j = 0; j < static_cast<ZoneRnum>(zone_table.size()); j++) {
						if (zone_table[j].vnum == zone_table[zone].typeA_list[i]) {
							typeA_to_queue.push_back(j);
							out << " ]\r\n[ Queued for reset: " << zone_table[j].name
								<< " (" << zone_table[j].vnum << ")";
							break;
						}
					}
				}
			}
			DecayObjectsOnRepop(zone_repop_list);
			// Логируем попадание в список репопа до самого сброса, иначе строка
			// печатается уже после "Stop zone" этой зоны (см. issue #3380).
			mudlog(fmt::format("В списке репопа: {} ", zone_table[zone].vnum), LGH, kLvlGod, SYSLOG, true);
			if (zone_table[zone].vnum < dungeons::kZoneStartDungeons) {
				ResetZone(zone);
				zones_reset_count++;
				ZoneResetMetrics(zone).RecordResetCount();
			} else {
				log("Закрываю брошенный dungeon %d", zone);
				dungeons::DungeonReset(zone);
				zone_table[zone].age = 0;
				zone_table[zone].used = false;
				zone_table[zone].time_awake = time(nullptr);
				zone_table[zone].first_enter.clear();
			}
			out << " ]\r\n[ Time reset: " << fmt::format("{:.3f} ms", timer_count.delta().count() * 1000.0);
			mudlog(out.str(), LGH, kLvlGod, SYSLOG, true);
			it = reset_q.erase(it);
			// Queue typeA zones at the front (in reverse so typeA[0] is processed first).
			// Also remove any existing normal queue entry to prevent double resets.
			for (auto rn = typeA_to_queue.rbegin(); rn != typeA_to_queue.rend(); ++rn) {
				for (auto s = reset_q.begin(); s != reset_q.end(); ++s) {
					if (s->zone_to_reset == *rn && !s->force_reset) {
						reset_q.erase(s);
						break;
					}
				}
				reset_q.push_front({*rn, true});
			}
			k++;
			if (k >= kZonesReset)
				break;
		} else {
			++it;
		}
	}
	
	// OpenTelemetry: Record total zones reset
	zone_span->SetAttribute("zones_reset_count", static_cast<int64_t>(zones_reset_count));
}

bool CanBeReset(ZoneRnum zone) {
	if (zone_table[zone].reset_mode != 3)
		return false;
// проверяем себя
	if (!IsZoneEmpty(zone))
		return false;
// проверяем список B
	for (auto i = 0; i < zone_table[zone].typeB_count; i++) {
		//Ищем ZoneRnum по vnum
		for (ZoneRnum j = 0; j < static_cast<ZoneRnum>(zone_table.size()); j++) {
			if (zone_table[j].vnum == zone_table[zone].typeB_list[i]) {
				if (!zone_table[zone].typeB_flag[i] || !IsZoneEmpty(j)) {
					return false;
				}
				break;
			}
		}
	}
// проверяем список A
	for (auto i = 0; i < zone_table[zone].typeA_count; i++) {
		//Ищем ZoneRnum по vnum
		for (ZoneRnum j = 0; j < static_cast<ZoneRnum>(zone_table.size()); j++) {
			if (zone_table[j].vnum == zone_table[zone].typeA_list[i]) {
				if (!IsZoneEmpty(j)) {
					return false;
				}
				break;
			}
		}
	}
	return true;
}

void paste_mob(CharData *ch, RoomRnum room) {
	if (!ch->IsNpc() || ch->GetEnemy() || ch->GetPosition() < EPosition::kStun)
		return;
	if (IsCharmice(ch)
		|| AFF_FLAGGED(ch, EAffect::kHorse)
		|| AFF_FLAGGED(ch, EAffect::kHold)
		|| (ch->extract_timer > 0)) {
		return;
	}
//	if (ch->IsFlagged(MOB_CORPSE))
//		return;
	if (room == kNowhere)
		return;
	bool time_ok = false;
	bool month_ok = false;
	bool need_move = false;
	bool no_month = true;
	bool no_time = true;
	const bool seasonal_mob =
		ch->IsFlagged(EMobFlag::kAppearsWinter)
		|| ch->IsFlagged(EMobFlag::kAppearsSpring)
		|| ch->IsFlagged(EMobFlag::kAppearsSummer)
		|| ch->IsFlagged(EMobFlag::kAppearsAutumn);



	if (ch->IsFlagged(EMobFlag::kAppearsDay)) {
		if (weather_info.sunlight == kSunRise || weather_info.sunlight == kSunLight)
			time_ok = true;
		need_move = true;
		no_time = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsNight)) {
		if (weather_info.sunlight == kSunSet || weather_info.sunlight == kSunDark)
			time_ok = true;
		need_move = true;
		no_time = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsFullmoon)) {
		if ((weather_info.sunlight == kSunSet ||
			weather_info.sunlight == kSunDark) &&
			(weather_info.moon_day >= 12 && weather_info.moon_day <= 15))
			time_ok = true;
		need_move = true;
		no_time = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsWinter)) {
		if (weather_info.season == ESeason::kWinter)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsSpring)) {
		if (weather_info.season == ESeason::kSpring)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsSummer)) {
		if (weather_info.season == ESeason::kSummer)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (ch->IsFlagged(EMobFlag::kAppearsAutumn)) {
		if (weather_info.season == ESeason::kAutumn)
			month_ok = true;
		need_move = true;
		no_month = false;
	}
	if (need_move)
	{
		month_ok |= no_month;
		time_ok |= no_time;
		if (month_ok && time_ok)
		{
			if (world[room]->vnum != zone_table[world[room]->zone_rn].top)
			{
				return;
			}

			if (GET_LASTROOM(ch) == kNowhere)
			{
				character_list.AddToExtractedList(ch);
				return;
			}

			RemoveCharFromRoom(ch);
			PlaceCharToRoom(ch, GET_LASTROOM(ch));
		}
		else
		{
			if (world[room]->vnum == zone_table[world[room]->zone_rn].top)
			{
				return;
			}

			if (seasonal_mob)
			{
				if (mob_ai::drop_mob_objects_to_box(ch))
				{
					char log_buf[kMaxInputLength];
					snprintf(log_buf, sizeof(log_buf), "Сезонный моб %s [%d] сложил вещи в узелок в комнате %d перед уходом в виртуальную комнату.",
						GET_NAME(ch), GET_MOB_VNUM(ch), world[room]->vnum);
					mudlog(log_buf, NRM, kLvlGod, SYSLOG, true);
				}
			}

			GET_LASTROOM(ch) = room;
			RemoveCharFromRoom(ch);
			room = GetRoomRnum(zone_table[world[room]->zone_rn].top);

			if (room == kNowhere)
				room = GET_LASTROOM(ch);

			PlaceCharToRoom(ch, room);
		}
	}
}

void paste_obj(ObjData *obj, RoomRnum room) {
	if (obj->get_carried_by()
		|| obj->get_worn_by()
		|| room == kNowhere) {
		return;
	}
	if (!zone_table[world[room]->zone_rn].used)
		return;
	bool time_ok = false;
	bool month_ok = false;
	bool need_move = false;
	bool no_time = true;
	bool no_month = true;

	if (obj->has_flag(EObjFlag::kAppearsDay)) {
		if (weather_info.sunlight == kSunRise
			|| weather_info.sunlight == kSunLight) {
			time_ok = true;
		}
		need_move = true;
		no_time = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsNight)) {
		if (weather_info.sunlight == kSunSet
			|| weather_info.sunlight == kSunDark) {
			time_ok = true;
		}
		need_move = true;
		no_time = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsFullmoon)) {
		if ((weather_info.sunlight == kSunSet
			|| weather_info.sunlight == kSunDark)
			&& weather_info.moon_day >= 12
			&& weather_info.moon_day <= 15) {
			time_ok = true;
		}
		need_move = true;
		no_time = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsWinter)) {
		if (weather_info.season == ESeason::kWinter) {
			month_ok = true;
		}
		need_move = true;
		no_month = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsSpring)) {
		if (weather_info.season == ESeason::kSpring) {
			month_ok = true;
		}
		need_move = true;
		no_month = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsSummer)) {
		if (weather_info.season == ESeason::kSummer) {
			month_ok = true;
		}
		need_move = true;
		no_month = false;
	}
	if (obj->has_flag(EObjFlag::kAppearsAutumn)) {
		if (weather_info.season == ESeason::kAutumn) {
			month_ok = true;
		}
		need_move = true;
		no_month = false;
	}
	if (need_move) {
		month_ok |= no_month;
		time_ok |= no_time;
		if (month_ok && time_ok) {
			if (world[room]->vnum != zone_table[world[room]->zone_rn].top) {
				return;
			}
			if (obj->get_room_was_in() == kNowhere) {
				world_objects.AddToExtractedList(obj);
				return;
			}
			RemoveObjFromRoom(obj);
			PlaceObjToRoom(obj, GetRoomRnum(obj->get_room_was_in()));
		} else {
			if (world[room]->vnum == zone_table[world[room]->zone_rn].top) {
				return;
			}
			// зачем сезонные переносить в виртуалку? спуржить нафиг
			if (!month_ok) {
//				ExtractObjFromWorld(obj);
				world_objects.AddToExtractedList(obj);
				return;
			}
			obj->set_room_was_in(GET_ROOM_VNUM(room));
			RemoveObjFromRoom(obj);
			room = GetRoomRnum(zone_table[world[room]->zone_rn].top);
			if (room == kNowhere) {
				room = GetRoomRnum(obj->get_room_was_in());
			}
			PlaceObjToRoom(obj, room);
		}
	}
}

void PasteMobiles() {
	utils::CExecutionTimer time;

	for (auto &it : character_list) {
	  paste_mob(it.get(), it->in_room);
	}
	log("Paste Mobiles() finished, time %f", time.delta().count());
	for (auto &it : world_objects) {
	  paste_obj(it.get(), it->get_in_room());
	}
	log("Paste obj() finished, time %f", time.delta().count());

}

void paste_on_reset(RoomData *to_room) {
	const auto people_copy = to_room->people;

	for (const auto &ch : people_copy) {
		paste_mob(ch, ch->in_room);
	}
	for (auto obj : to_room->contents) {
		paste_obj(obj, obj->get_in_room());
	}
}

void LogZoneError(const ZoneData &zone_data, int cmd_no, const char *message) {
	char local_buf[256];

	sprintf(local_buf, "SYSERR: zone file %d.zon: %s", zone_data.vnum, message);
	mudlog(local_buf, NRM, kLvlGod, SYSLOG, true);

	sprintf(local_buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
			zone_data.cmd[cmd_no].command, zone_data.vnum, zone_data.cmd[cmd_no].line);
	mudlog(local_buf, NRM, kLvlGod, SYSLOG, true);
}

const int CHECK_SUCCESS{1};	// Выполить команду, только если предыдущая успешна
const int FLAG_PERSIST{2};	// Команда не должна изменить флаг

class ZoneReset {
 public:
  explicit ZoneReset(const ZoneRnum zone) : m_zone_rnum(zone) {}

  void Reset();

 private:
  [[nodiscard]] bool HandleZoneCmdQ(MobRnum rnum) const;

  // execute the reset command table of a given zone
  void ResetZoneEssential();

  ZoneRnum m_zone_rnum;
};

void ZoneReset::Reset() {
	utils::CExecutionTimer timer;

	ResetZoneEssential();
	const auto execution_time = timer.delta();

	ZoneResetMetrics(m_zone_rnum).RecordReset(execution_time.count());
}

bool ZoneReset::HandleZoneCmdQ(const MobRnum rnum) const {
	utils::CExecutionTimer overall_timer;
	bool extracted = false;

	utils::CExecutionTimer get_mobs_timer;
	Characters::list_t mobs;
	if (rnum > top_of_mobt) {
		log("WARNING: Mobile rnum %d is above the list 'top of mobt' (%s %s %d)", rnum, __FILE__, __func__, __LINE__);
		return extracted;
	}
	character_list.get_mobs_by_vnum(mob_index[rnum].vnum, mobs);
	const auto get_mobs_time = get_mobs_timer.delta();

	utils::CExecutionTimer extract_timer;
	for (const auto &mob : mobs) {
		if (!mob->IsFlagged(EMobFlag::kResurrected)) {
			ExtractCharFromWorld(mob.get(), false, true);
			extracted = true;
		}
	}
	const auto extract_time = extract_timer.delta();

	const auto execution_time = overall_timer.delta();

	ZoneResetMetrics(m_zone_rnum).RecordZoneCmdQ(execution_time.count(), mob_index[rnum].vnum);

	return extracted;
}

void ZoneReset::ResetZoneEssential() {
	int cmd_no;
	int cmd_tmp, obj_in_room_max, obj_in_room = 0;
	CharData *mob = nullptr, *leader = nullptr;
	ObjData *obj_to;
	CharData *tmob = nullptr;    // for trigger assignment
	ObjData *tobj = nullptr;    // for trigger assignment
	int last_state, curr_state;    // статус завершения последней и текущей команды

	log("[Reset] Start zone (%d) %s", zone_table[m_zone_rnum].vnum, zone_table[m_zone_rnum].name.c_str());
	if (!zone_table[m_zone_rnum].cmd) {
		log("No cmd, skiped");
		return;
	}
	for (auto rrn = zone_table[m_zone_rnum].RnumRoomsLocation.first;
		 rrn <= zone_table[m_zone_rnum].RnumRoomsLocation.second; rrn++) {
		RestoreRoomExitData(rrn);
	}
	//----------------------------------------------------------------------------
	last_state = 1;        // для первой команды считаем, что все ок

	for (cmd_no = 0; zone_table[m_zone_rnum].cmd != nullptr; cmd_no++) {
		const auto &zone_data = zone_table[m_zone_rnum];
		zone_data.cmd[cmd_no].line = cmd_no + 1;
		auto &reset_cmd = zone_table[m_zone_rnum].cmd[cmd_no];
		if (reset_cmd.command == '*') {
			continue;
		}
		if (reset_cmd.command == 'S') {
			break;
		}
		curr_state = 0;    // по умолчанию - неудачно
		if (!(reset_cmd.if_flag & CHECK_SUCCESS) || last_state) {
			// Выполняем команду, если предыдущая успешна или не нужно проверять
			switch (reset_cmd.command) {
				case 'M':
					// read a mobile
					// 'M' <flag> <MobVnum> <max_in_world> <RoomVnum> <max_in_room|-1>

					if (reset_cmd.arg3 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка загрузить моба в 0 комнату. (VNUM = %d, ZONE = %d)",
								mob_index[reset_cmd.arg1].vnum, zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}

					mob = nullptr;    //Добавлено Ладником
					if (mob_index[reset_cmd.arg1].total_online < reset_cmd.arg2 &&
						(reset_cmd.arg4 < 0 || CountMobsInRoom(reset_cmd.arg1, reset_cmd.arg3) < reset_cmd.arg4)) {
						mob = ReadMobile(reset_cmd.arg1, kReal);
						if (!mob) {
							sprintf(buf,
									"ZRESET: ошибка! моб %d  в зоне %d не существует",
									reset_cmd.arg1,
									zone_table[m_zone_rnum].vnum);
							mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
							return;
						}
						if (!(mob_proto[mob->get_rnum()].get_role_bits().any() || ROOM_FLAGGED(reset_cmd.arg3, ERoomFlag::kArena))) {
							int rndlev = mob->GetLevel();
							rndlev += number(-2, +2);
							mob->set_level(std::clamp(rndlev, 1, kMaxMobLevel));
						}
						PlaceCharToRoom(mob, reset_cmd.arg3);
						load_mtrigger(mob);
						tmob = mob;
						curr_state = 1;
					}
					tobj = nullptr;
					break;

				case 'F':
					// Follow mobiles
					// 'F' <flag> <RoomVnum> <leader_vnum> <MobVnum>
					leader = nullptr;
					if (reset_cmd.arg1 >= kFirstRoom && reset_cmd.arg1 <= top_of_world) {
						for (const auto ch : world[reset_cmd.arg1]->people) {
							if (ch->IsNpc() && ch->get_rnum() == reset_cmd.arg2) {
								leader = ch;
							}
						}

						if (leader) {
							for (const auto ch : world[reset_cmd.arg1]->people) {
								if (ch->IsNpc()
									&& ch->get_rnum() == reset_cmd.arg3
									&& leader != ch
									&& !follow::MakesLoop(ch, leader)) {
									if (IsCharmice(ch)) {
										continue;
									}
									if (ch->has_master()) {
										follow::StopFollower(ch, follow::kSfEmpty);
									}
									if (ch->purged() || ch->in_room == kNowhere)
										continue;
									follow::AddFollower(leader, ch);
									curr_state = 1;
								}
							}
						}
					}
					break;

				case 'Q':
					if (HandleZoneCmdQ(reset_cmd.arg1)) {
						curr_state = 1;
					}

					tobj = nullptr;
					tmob = nullptr;
					break;

				case 'O':
					// read an object
					// 'O' <flag> <ObjVnum> <max_in_world> <RoomVnum|-1> <load%|-1>
					// Проверка  - сколько всего таких же обьектов надо на эту клетку
					if (reset_cmd.arg3 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка загрузить объект в 0 комнату. (VNUM = %d, ZONE = %d)",
								obj_proto[reset_cmd.arg1]->get_vnum(), zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}
					for (cmd_tmp = 0, obj_in_room_max = 0; zone_data.cmd[cmd_tmp].command != 'S'; cmd_tmp++)
						if ((zone_data.cmd[cmd_tmp].command == 'O')
							&& (reset_cmd.arg1 == zone_data.cmd[cmd_tmp].arg1)
							&& (reset_cmd.arg3 == zone_data.cmd[cmd_tmp].arg3))
							obj_in_room_max++;
					// Теперь считаем склько их на текущей клетке
					obj_in_room = 0;
					for (auto obj_room : world[reset_cmd.arg3]->contents) {
						if (reset_cmd.arg1 == obj_room->get_rnum()) {
							obj_in_room++;
						}
					}
					// Теперь грузим обьект если надо
					if ((obj_proto.actual_count(reset_cmd.arg1) < GetObjMIW(reset_cmd.arg1)
						|| GetObjMIW(reset_cmd.arg1) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| stable_objs::IsTimerUnlimited(obj_proto[reset_cmd.arg1].get()))
						&& (reset_cmd.arg4 <= 0
							|| number(1, 100) <= reset_cmd.arg4)
						&& (obj_in_room < obj_in_room_max)) {
						const auto obj = world_objects.create_from_prototype_by_rnum(reset_cmd.arg1);
						obj->set_vnum_zone_from(zone_table[world[reset_cmd.arg3]->zone_rn].vnum);

						if (!PlaceObjToRoom(obj.get(), reset_cmd.arg3)) {
							ExtractObjFromWorld(obj.get());
							break;
						}
						load_otrigger(obj.get());

						tobj = obj.get();
						curr_state = 1;

						if (!obj->has_flag(EObjFlag::kNodecay)) {
							sprintf(buf, "&YВНИМАНИЕ&G На землю загружен объект без флага NODECAY : %s (VNUM=%d)",
									obj->get_PName(grammar::ECase::kNom).c_str(), obj->get_vnum());
							mudlog(buf, BRF, kLvlBuilder, ERRLOG, true);
						}
					}
					tmob = nullptr;
					break;

				case 'P':
					// object to object
					// 'P' <flag> <ObjVnum> <room or 0> <target_vnum> <load%|-1>
					if ((obj_proto.actual_count(reset_cmd.arg1) < GetObjMIW(reset_cmd.arg1)
						|| GetObjMIW(reset_cmd.arg1) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| stable_objs::IsTimerUnlimited(obj_proto[reset_cmd.arg1].get()))
						&& (reset_cmd.arg4 <= 0
							|| number(1, 100) <= reset_cmd.arg4)) {
						if (reset_cmd.arg2 > 0) {
							RoomRnum arg2 = GetRoomRnum(reset_cmd.arg2);
							if (arg2 == kNowhere) {
								LogZoneError(zone_data, cmd_no, "room in arg2 not found, command omited");
								break;
							}
							if (!(obj_to = GetObjByRnumInContent(reset_cmd.arg3, world[arg2]->contents))) {
								break;
							}
						} else {
							if (!(obj_to = target_resolver::FindObjByRnum(reset_cmd.arg3))) {
								LogZoneError(zone_data, cmd_no, "target obj not found in word, command omited");
								break;
							}
						}
						if (obj_to->get_type() != EObjType::kContainer) {
							LogZoneError(zone_data, cmd_no, "attempt put obj to non container, omited");
							reset_cmd.command = '*';
							break;
						}
						const auto obj = world_objects.create_from_prototype_by_rnum(reset_cmd.arg1);
						if (obj_to->get_in_room() != kNowhere) {
							obj->set_vnum_zone_from(zone_table[world[obj_to->get_in_room()]->zone_rn].vnum);
						} else if (obj_to->get_worn_by()) {
							obj->set_vnum_zone_from(zone_table[world[obj_to->get_worn_by()->in_room]->zone_rn].vnum);
						} else if (obj_to->get_carried_by()) {
							obj->set_vnum_zone_from(zone_table[world[obj_to->get_carried_by()->in_room]->zone_rn].vnum);
						}
						PlaceObjIntoObj(obj.get(), obj_to);
						load_otrigger(obj.get());
						tobj = obj.get();
						curr_state = 1;
					}
					tmob = nullptr;
					break;

				case 'G':
					// obj_to_char
					// 'G' <flag> <ObjVnum> <max_in_world> <-> <load%|-1>
					if (!mob)
						//Изменено Ладником
					{
						// ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
						// reset_cmd.command = '*';
						break;
					}
					if ((obj_proto.actual_count(reset_cmd.arg1) < GetObjMIW(reset_cmd.arg1)
						|| GetObjMIW(reset_cmd.arg1) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| stable_objs::IsTimerUnlimited(obj_proto[reset_cmd.arg1].get()))
						&& (reset_cmd.arg4 <= 0
							|| number(1, 100) <= reset_cmd.arg4)) {
						const auto obj = world_objects.create_from_prototype_by_rnum(reset_cmd.arg1);
						PlaceObjToInventory(obj.get(), mob);
						obj->set_vnum_zone_from(zone_table[world[mob->in_room]->zone_rn].vnum);
						tobj = obj.get();
						load_otrigger(obj.get());
						curr_state = 1;
					}
					tmob = nullptr;
					break;

				case 'E':
					// object to equipment list
					// 'E' <flag> <ObjVnum> <max_in_world> <wear_pos> <load%|-1>
					//Изменено Ладником
					if (!mob) {
						//ZONE_ERROR("trying to equip non-existant mob, command disabled");
						// reset_cmd.command = '*';
						break;
					}
					if ((obj_proto.actual_count(reset_cmd.arg1) < obj_proto[reset_cmd.arg1]->get_max_in_world()
						|| GetObjMIW(reset_cmd.arg1) == ObjData::UNLIMITED_GLOBAL_MAXIMUM
						|| stable_objs::IsTimerUnlimited(obj_proto[reset_cmd.arg1].get()))
						&& (reset_cmd.arg4 <= 0
							|| number(1, 100) <= reset_cmd.arg4)) {
						if (reset_cmd.arg3 < 0
							|| reset_cmd.arg3 >= EEquipPos::kNumEquipPos) {
							LogZoneError(zone_data, cmd_no, "invalid equipment pos number");
						} else {
							const auto obj = world_objects.create_from_prototype_by_rnum(reset_cmd.arg1);
							obj->set_vnum_zone_from(zone_table[world[mob->in_room]->zone_rn].vnum);
							obj->set_in_room(mob->in_room);
							load_otrigger(obj.get());
							if (wear_otrigger(obj.get(), mob, reset_cmd.arg3)) {
								obj->set_in_room(kNowhere);
								EquipObj(mob, obj.get(), reset_cmd.arg3, CharEquipFlags());
							} else {
								PlaceObjToInventory(obj.get(), mob);
							}
							if (obj->get_carried_by() != mob && obj->get_worn_by() != mob) {
								ExtractObjFromWorld(obj.get(), false);
								tobj = nullptr;
							} else {
								tobj = obj.get();
								curr_state = 1;
							}
						}
					}
					tmob = nullptr;
					break;

				case 'R':
					// rem obj from room
					// 'R' <flag> <RoomVnum> <ObjVnum>

					if (reset_cmd.arg1 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка удалить объект из 0 комнаты. (VNUM = %d, ZONE = %d)",
								obj_proto[reset_cmd.arg2]->get_vnum(), zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}

					if (const auto obj = GetObjByRnumInContent(reset_cmd.arg2, world[reset_cmd.arg1]->contents)) {
						ExtractObjFromWorld(obj);
						curr_state = 1;
					}
					tmob = nullptr;
					tobj = nullptr;
					break;

				case 'D':
					// set state of door
					// 'D' <flag> <RoomVnum> <door_pos> <door_state>

					if (reset_cmd.arg1 < kFirstRoom) {
						sprintf(buf, "&YВНИМАНИЕ&G Попытка установить двери в 0 комнате. (ZONE = %d)",
								zone_table[m_zone_rnum].vnum);
						mudlog(buf, BRF, kLvlBuilder, SYSLOG, true);
						break;
					}
					if (reset_cmd.arg2 < 0 || reset_cmd.arg2 >= EDirection::kMaxDirNum ||
						(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2] == nullptr)) {
						LogZoneError(zone_data, cmd_no, "door does not exist, command disabled");
						reset_cmd.command = '*';
					} else {
						REMOVE_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->exit_info,
								   EExitFlag::kBrokenLock);
						switch (reset_cmd.arg3) {
							case 0:
								REMOVE_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->
									exit_info, EExitFlag::kLocked);
								REMOVE_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->
									exit_info, EExitFlag::kClosed);
								break;
							case 1:
								SET_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->exit_info,
										EExitFlag::kClosed);
								REMOVE_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->
									exit_info, EExitFlag::kLocked);
								break;
							case 2:
								SET_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->exit_info,
										EExitFlag::kLocked);
								SET_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->exit_info,
										EExitFlag::kClosed);
								break;
							case 3:
								SET_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->exit_info,
										EExitFlag::kHidden);
								break;
							case 4:
								REMOVE_BIT(world[reset_cmd.arg1]->dir_option[reset_cmd.arg2]->
									exit_info, EExitFlag::kHidden);
								break;
						}
						curr_state = 1;
					}
					tmob = nullptr;
					tobj = nullptr;
					break;

				case 'T':
					// trigger command; details to be filled in later
					// 'T' <flag> <trigger_type> <trigger_vnum> <RoomVnum, для WLD_TRIGGER>
					if (reset_cmd.arg1 == MOB_TRIGGER && tmob) {
						auto trig = read_trigger(reset_cmd.arg2);
						if (add_trigger(SCRIPT(tmob).get(), trig, -1)) {
							add_trig_to_owner(-1, trig_index[reset_cmd.arg2]->vnum, GET_MOB_VNUM(tmob));
						} else {
							ExtractTrigger(trig);
						}
						curr_state = 1;
					} else if (reset_cmd.arg1 == OBJ_TRIGGER && tobj) {
						auto trig = read_trigger(reset_cmd.arg2);
						if (add_trigger(tobj->get_script().get(), trig, -1)) {
							add_trig_to_owner(-1, trig_index[reset_cmd.arg2]->vnum, GET_OBJ_VNUM(tobj));
							world_objects.update_obj_indices(tobj);
						} else {
							ExtractTrigger(trig);
						}
						curr_state = 1;
					} else if (reset_cmd.arg1 == WLD_TRIGGER) {
						if (reset_cmd.arg3 > kNowhere) {
							auto trig = read_trigger(reset_cmd.arg2);
							if (add_trigger(world[reset_cmd.arg3]->script.get(), trig, -1)) {
								add_trig_to_owner(-1, trig_index[reset_cmd.arg2]->vnum, world[reset_cmd.arg3]->vnum);
							} else {
								ExtractTrigger(trig);
							}
							curr_state = 1;
						}
					}
					break;

				case 'V':
					// 'V' <flag> <trigger_type> <RoomVnum> <context> <var_name> <var_value>
					if (reset_cmd.arg1 == MOB_TRIGGER && tmob) {
						if (!SCRIPT(tmob)->has_triggers()) {
							LogZoneError(zone_data, cmd_no, "Attempt to give variable to scriptless mobile");
						} else {
							add_var_cntx(SCRIPT(tmob)->global_vars, reset_cmd.sarg1, reset_cmd.sarg2, reset_cmd.arg3);
							curr_state = 1;
						}
					} else if (reset_cmd.arg1 == OBJ_TRIGGER && tobj) {
						if (!tobj->get_script()->has_triggers()) {
							LogZoneError(zone_data, cmd_no, "Attempt to give variable to scriptless object");
						} else {
							add_var_cntx(tobj->get_script()->global_vars,
										 reset_cmd.sarg1,
										 reset_cmd.sarg2,
										 reset_cmd.arg3);
							curr_state = 1;
						}
					} else if (reset_cmd.arg1 == WLD_TRIGGER) {
						if (reset_cmd.arg2 < kFirstRoom || reset_cmd.arg2 > top_of_world) {
							LogZoneError(zone_data, cmd_no, "Invalid room number in variable assignment");
						} else {
							if (!SCRIPT(world[reset_cmd.arg2])->has_triggers()) {
								LogZoneError(zone_data, cmd_no, "Attempt to give variable to scriptless room");
							} else {
								add_var_cntx(world[reset_cmd.arg2]->script->global_vars,
											 reset_cmd.sarg1,
											 reset_cmd.sarg2,
											 reset_cmd.arg3);
								curr_state = 1;
							}
						}
					}
					break;

				default: LogZoneError(zone_data, cmd_no, "unknown cmd in reset table; cmd disabled");
					reset_cmd.command = '*';
					break;
			}
		}

		if (!(reset_cmd.if_flag & FLAG_PERSIST)) {
			// команда изменяет флаг
			last_state = curr_state;
		}
	}
	if (zone_table[m_zone_rnum].used) {
		zone_table[m_zone_rnum].count_reset++;
	}

	zone_table[m_zone_rnum].age = 0;
	zone_table[m_zone_rnum].used = false;
	zone_table[m_zone_rnum].time_awake = time(nullptr);
	zone_table[m_zone_rnum].first_enter.clear();
	celebrates::ProcessCelebrates(zone_table[m_zone_rnum].vnum);
	int rnum_start = 0;
	int rnum_stop = 0;

	if (GetZoneRooms(m_zone_rnum, &rnum_start, &rnum_stop)) {
		// все внутренние резеты комнат зоны теперь идут за один цикл
		// резет порталов теперь тут же и переписан, чтобы не гонять по всем румам, ибо жрал половину времени резета -- Krodo
		log("Paste mob&obj on reset");
		for (int rnum = rnum_start; rnum <= rnum_stop; rnum++) {
			RoomData *room = world[rnum];
			reset_wtrigger(room);
			int sect = real_sector(GetRoomRnum(room->vnum));
			if (!(sect == ESector::kWaterSwim || sect == ESector::kWaterNoswim || sect == ESector::kOnlyFlying)) {
				im_reset_room(room, zone_table[m_zone_rnum].level, zone_table[m_zone_rnum].type);
			}
			while (room_spells::RoomHasPortal(world[rnum])) {
				RemovePortalGate(rnum);
			}
			paste_on_reset(room);
		}
	}
	for (rnum_start = 0; rnum_start < static_cast<int>(zone_table.size()); rnum_start++) {
		// проверяем, не содержится ли текущая зона в чьем-либо typeB_list
		for (curr_state = zone_table[rnum_start].typeB_count; curr_state > 0; curr_state--) {
			if (zone_table[rnum_start].typeB_list[curr_state - 1] == zone_table[m_zone_rnum].vnum) {
				zone_table[rnum_start].typeB_flag[curr_state - 1] = true;

				break;
			}
		}
	}
	//Если это ведущая зона, то при ее сбросе обнуляем typeB_flag
	for (rnum_start = zone_table[m_zone_rnum].typeB_count; rnum_start > 0; rnum_start--)
		zone_table[m_zone_rnum].typeB_flag[rnum_start - 1] = false;
	log("[Reset] Stop zone (%d) %s", zone_table[m_zone_rnum].vnum, zone_table[m_zone_rnum].name.c_str());
	after_reset_zone(m_zone_rnum);
}

void ResetZone(ZoneRnum zone) {
	ZoneReset zreset(zone);
	zreset.Reset();
}

// Ищет RNUM первой и последней комнаты зоны
// Еси возвращает 0 - комнат в зоне нету
int GetZoneRooms(ZoneRnum zrn, int *first, int *last) {
	*first = zone_table[zrn].RnumRoomsLocation.first;
	*last = zone_table[zrn].RnumRoomsLocation.second;

	if (*first <= 0)
		return 0;
	return 1;
}

// for use in ResetZone; return true if zone 'nr' is free of PC's
bool IsZoneEmpty(ZoneRnum zone_nr, bool debug) {
	int rnum_start, rnum_stop;

	for (auto i = descriptor_list; i; i = i->next) {
		if  (i->state != EConState::kPlaying)
			continue;
		if (i->character->in_room == kNowhere)
			continue;
		if (GetRealLevel(i->character) >= kLvlImmortal && GET_INVIS_LEV(i->character) > 0)
			continue;
		if (world[i->character->in_room]->zone_rn != zone_nr)
			continue;
		return false;
	}
	// Поиск link-dead игроков в зонах комнаты zone_nr
	if (!GetZoneRooms(zone_nr, &rnum_start, &rnum_stop)) {
		return true;    // в зоне нет комнат :)
	}
	for (; rnum_start <= rnum_stop; rnum_start++) {
// num_pc_in_room() использовать нельзя, т.к. считает вместе с иммами.
		for (const auto c : world[rnum_start]->people) {
			if (!c->IsNpc() && (GetRealLevel(c) < kLvlImmortal)) {
				return false;
			}
		}
	}
// теперь проверю всех товарищей в void комнате STRANGE_ROOM
	for (const auto c : world[kStrangeRoom]->people) {
		const int was = c->get_was_in_room();

		if (was == kNowhere
			|| GetRealLevel(c) >= kLvlImmortal
			|| world[was]->zone_rn != zone_nr) {
			continue;
		}
		return false;
	}

	if (room_spells::IsZoneRoomAffected(zone_nr, room_spells::ERoomAffect::kRuneLabel)) {
		return false;
	}
	if (debug) {
		sprintf(buf, "is_empty чек по клеткам зоны. Зона %d в зоне НИКОГО!!!", zone_table[zone_nr].vnum);
		mudlog(buf, CMP, kLvlGreatGod, SYSLOG, true);
	}
	return true;
}

int CountMobsInRoom(int m_num, int r_num) {
	int count = 0;

	for (const auto &ch : world[r_num]->people) {
		if (m_num == ch->get_rnum()
			&& !ch->IsFlagged(EMobFlag::kResurrected)) {
			count++;
		}
	}
	return count;
}

/**
* Можно канеш просто в get_skill иммам возвращать 100 или там 200, но тут зато можно
* потестить че-нить с возможностью покачать скилл во время игры иммом.
*/
void SetGodSkills(CharData *ch) {
	for (auto i = ESkill::kFirst; i <= ESkill::kLast; ++i) {
		if (MUD::Skills().IsValid(i)) {
			SetSkill(ch, i, 200);
		}
	}
}

// по умолчанию reboot = 0 (пользуется только при ребуте)
int LoadPlayerCharacter(const char *name, CharData *char_element, int load_flags) {
	const auto player_i = char_element->load_char_ascii(name, load_flags);
	// OpenTelemetry: Track player loading
	auto load_span = tracing::TraceManager::Instance().StartSpan("Load Player");
	load_span->SetAttribute("character_name", std::string(name));
	
	observability::ScopedMetric load_metric("player.load.duration");
	
	if (player_i > -1) {
		char_element->set_pfilepos(player_i);
	}
	return (player_i);
}

/************************************************************************
*  funcs of a (more or less) general utility nature                     *
************************************************************************/

/*
 * Steps:
 *   1: Make sure no one is using the pointer in paging.
 *   2: Read contents of a text file.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * str_dup()'d copy on ->showstr_head, and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector, but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 */
int AllocateBufferForFile(const char *name, char **destination_buf) {
	char temp[kMaxExtendLength];
	DescriptorData *in_use;

	for (in_use = descriptor_list; in_use; in_use = in_use->next) {
		if (in_use->showstr_vector && *in_use->showstr_vector == *destination_buf) {
			return (-1);
		}
	}

	// Lets not free() what used to be there unless we succeeded.
	if (ReadFileToBuffer(name, temp) < 0) {
		return (-1);
	}

	if (*destination_buf) {
		free(*destination_buf);
	}

	*destination_buf = str_dup(temp);
	return (0);
}

// read contents of a text file, and place in destination_buf
int ReadFileToBuffer(const char *name, char *destination_buf) {
	FILE *fl;
	char tmp[READ_SIZE + 3];

	*destination_buf = '\0';

	if (!(fl = fopen(name, "r"))) {
		log("SYSERR: reading %s: %s", name, strerror(errno));
		sprintf(destination_buf + strlen(destination_buf), "Error: file '%s' is empty.\r\n", name);
		return (0);
	}
	do {
		const char *dummy = fgets(tmp, READ_SIZE, fl);
		UNUSED_ARG(dummy);

		tmp[strlen(tmp) - 1] = '\0';    // take off the trailing \n
		strcat(tmp, "\r\n");

		if (!feof(fl)) {
			if (strlen(destination_buf) + strlen(tmp) + 1 > kMaxExtendLength) {
				log("SYSERR: %s: string too big (%d max)", name, kMaxStringLength);
				*destination_buf = '\0';
				return (-1);
			}
			strcat(destination_buf, tmp);
		}
	} while (!feof(fl));

	fclose(fl);

	return (0);
}

ObjRnum GetObjRnum(ObjVnum vnum) {
	return obj_proto.get_rnum(vnum);
}

ZoneRnum GetZoneRnum(ZoneVnum vnum) {
	ZoneRnum bot, top, mid;

	bot = 0;
	top = static_cast<ZoneRnum>(zone_table.size() - 1);

	for (;;) {
		mid = (bot + top) / 2;

		if (zone_table[mid].vnum == vnum)
			return (mid);
		if (bot >= top)
			return (kNoZone);
		if (zone_table[mid].vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

// returns the real number of the room with given virtual number
RoomRnum GetRoomRnum(RoomVnum vnum) {
	RoomRnum bot, top, mid;
/*	for (int i = 0; i <= top_of_world; i++) {
		if (world[i]->vnum == vnum)
			return (i);
	}
	return 0;
*/
	bot = 1;        // 0 - room is kNowhere
	top = top_of_world;
	// perform binary search on world-table
	for (;;) {
		mid = (bot + top) / 2;
		if (world[mid]->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (kNowhere);
		if (world[mid]->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

TrgRnum GetTriggerRnum(TrgVnum vnum) {
/*	int rnum;

	for (rnum = 0; rnum <= top_of_trigt; rnum++) {
		if (trig_index[rnum]->vnum == vnum)
			break;
	}

	if (rnum > top_of_trigt)
		rnum = -1;
	return (rnum);
*/
	TrgRnum bot, top, mid;
	bot = 0;
	top = top_of_trigt - 1;
	// perform binary search on world-table
	for (;;) {
		mid = (bot + top) / 2;
		if (trig_index[mid]->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (-1);
		if (trig_index[mid]->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

// returns the real number of the monster with given virtual number
MobRnum GetMobRnum(MobVnum vnum) {
//	log("real mobile vnum %d top %d", vnum, top_of_mobt);
	MobRnum bot, top, mid;

	bot = 0;
	top = top_of_mobt;
	// perform binary search on mob-table
	for (;;) {
		mid = (bot + top) / 2;
		if ((mob_index + mid)->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (-1);
		if ((mob_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

void LoadGlobalUid() {
	FILE *guid;
	char buffer[256];

	global_uid = 0;

	if (!(guid = fopen(LIB_MISC "globaluid", "r"))) {
		log("Can't open global uid file...");
		return;
	}
	get_line(guid, buffer);
	global_uid = atoi(buffer);
	fclose(guid);
}

void SaveGlobalUID() {
	FILE *guid;

	if (!(guid = fopen(LIB_MISC "globaluid", "w"))) {
		log("Can't write global uid file...");
		return;
	}

	fprintf(guid, "%d\n", global_uid);
	fclose(guid);
}

Rooms::~Rooms() {
	log("~Rooms()");
	for (auto &i : *this) {
		delete i;
	}
}

int get_filename(const char *orig_name, char *filename, int mode) {
	const char *prefix, *middle, *suffix;
	char name[64], *ptr;

	if (orig_name == nullptr || *orig_name == '\0' || filename == nullptr) {
		log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.", orig_name, filename);
		return (0);
	}

	switch (mode) {
		case kAliasFile: prefix = LIB_PLRALIAS;
			suffix = SUF_ALIAS;
			break;
		case kScriptVarsFile: prefix = LIB_PLRVARS;
			suffix = SUF_MEM;
			break;
		case kPlayersFile: prefix = LIB_PLRS;
			suffix = SUF_PLAYER;
			break;
		case kTextCrashFile: prefix = LIB_PLROBJS;
			suffix = TEXT_SUF_OBJS;
			break;
		case kTimeCrashFile: prefix = LIB_PLROBJS;
			suffix = TIME_SUF_OBJS;
			break;
		case kPersDepotFile: prefix = LIB_DEPOT;
			suffix = SUF_PERS_DEPOT;
			break;
		case kShareDepotFile: prefix = LIB_DEPOT;
			suffix = SUF_SHARE_DEPOT;
			break;
		case kPurgeDepotFile: prefix = LIB_DEPOT;
			suffix = SUF_PURGE_DEPOT;
			break;
		default: return (0);
	}

	strcpy(name, orig_name);
	for (ptr = name; *ptr; ptr++) {
		if (*ptr == 'Ё' || *ptr == 'ё')
			*ptr = '9';
		else
			*ptr = LOWER(codepages::AtoL(*ptr));
	}

	switch (LOWER(*name)) {
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e': middle = LIB_A;
			break;
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j': middle = LIB_F;
			break;
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o': middle = LIB_K;
			break;
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't': middle = LIB_P;
			break;
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z': middle = LIB_U;
			break;
		default: middle = LIB_Z;
			break;
	}

	sprintf(filename, "%s%s" SLASH "%s.%s", prefix, middle, name, suffix);
	return (1);
}

CharData *find_char(long uid) {
	auto it = chardata_by_uid.find(uid);
	if (it != chardata_by_uid.end()) {
		return it->second;
	}
	return nullptr;
}

CharData *find_pc(long uid) {
	for (auto d = descriptor_list; d; d = d->next) {
		if (d->state == EConState::kPlaying && d->character->get_uid() == uid) {
			return d->character.get();
		}
	}
	return nullptr;
}
void CharTimerUpdate() {
	std::list<CharData *> wait_list;
	std::list<CharData *> cooldown_list;

	for (auto it : chardata_cooldown_list) {
		if (!it->Skills().DecreaseCooldownsAndCheck()) {
			cooldown_list.push_back(it);
		}
	}
	for (auto &it : chardata_wait_list) {
		it->wait_dec();
		if (it->get_wait() == 0) {
			wait_list.push_back(it);
		}
	}
	for (auto &it : wait_list) {
		chardata_wait_list.erase(it);
	}
	for (auto &it : cooldown_list) {
		chardata_cooldown_list.erase(it);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
