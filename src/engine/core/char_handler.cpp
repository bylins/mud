/**
\file char_handler.cpp - a part of the Bylins engine.
\brief issue.handler-cleaning: split out of handler.cpp.
*/

#include "engine/core/char_handler.h"
#include "engine/core/obj_handler.h"
#include "administration/privilege.h"

#include "engine/scripting/dg_scripts.h"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/equipment.h"
#include "utils/grammar/declensions.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/affects/affect_data.h"   // issue.mob-flag-affect-materialization: spawn-hook
#include "gameplay/fight/fight_stuff.h"
#include "gameplay/economics/auction.h"
#include "utils/backtrace.h"
#include "utils_char_obj.inl"
#include "engine/core/target_resolver.h"
#include "engine/entities/char_player.h"
#include "engine/db/world_characters.h"
#include "gameplay/economics/exchange.h"
#include "gameplay/fight/fight.h"
#include "gameplay/clans/house.h"
#include "gameplay/magic/magic.h"
#include "engine/db/obj_prototypes.h"
#include "engine/ui/color.h"
#include "gameplay/classes/classes_spell_slots.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/dungeons.h"
#include "gameplay/mechanics/cities.h"
#include "gameplay/mechanics/player_races.h"
#include "gameplay/core/remort.h"
#include "gameplay/magic/magic_rooms.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/core/base_stats.h"
#include "utils/utils_time.h"
#include "gameplay/classes/pc_classes.h"

using classes::CalcCircleSlotsAmount;

// local functions //
int apply_ac(CharData *ch, int eq_pos);
int apply_armour(CharData *ch, int eq_pos);
void UpdateObject(ObjData *obj, int use);
void UpdateCharObjects(CharData *ch);

// external functions //
void PerformDropGold(CharData *ch, int amount);
int invalid_unique(CharData *ch, const ObjData *obj);
void do_entergame(DescriptorData *d);
void DoReturn(CharData *ch, char *argument, int cmd, int subcmd);
//extern std::vector<City> Cities;
extern int global_uid;
extern void change_leader(CharData *ch, CharData *vict);
const char *sight::find_exdesc(const char *word, const std::vector<ExtraDescription> &list);
extern void SetSkillCooldown(CharData *ch, ESkill skill, int pulses);




// move a player out of a room

void RemoveCharFromRoom(CharData *ch) {
	if (ch == nullptr || ch->in_room == kNowhere) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: NULL character or kNowhere in %s, RemoveCharFromRoom", __FILE__);
		return;
	}

	if (ch->GetEnemy() != nullptr)
		stop_fighting(ch, true);

	if (!ch->IsNpc())
		ch->set_from_room(ch->in_room);

	CheckLight(ch, kLightNo, kLightNo, kLightNo, kLightNo, -1);

	auto &people = world[ch->in_room]->people;
	people.erase(std::find(people.begin(), people.end(), ch));

	ch->in_room = kNowhere;
	ch->track_dirs = 0;
}

void PlaceCharToRoom(CharData *ch, RoomRnum room, bool process_entry_affects) {
	if (ch == nullptr || room < kNowhere + 1 || room > top_of_world) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p", room, top_of_world, ch);
		return;
	}

	if (!ch->IsNpc() && !Clan::MayEnter(ch, room, kHousePortal)) {
		room = ch->get_from_room();
	}

	if (!ch->IsNpc()
			&& NORENTABLE(ch)
			&& ROOM_FLAGGED(room, ERoomFlag::kArena)
			&& !privilege::IsImmortal(ch)) {
		SendMsgToChar("Вы не можете попасть на арену в состоянии боевых действий!\r\n", ch);
		room = ch->get_from_room();
	}
	world[room]->people.push_front(ch);

	ch->in_room = room;
	CheckLight(ch, kLightNo, kLightNo, kLightNo, kLightNo, 1);
	ch->Temporary.unset(ECharExtraFlag::kFailHide);
	ch->Temporary.unset(ECharExtraFlag::kFailSneak);
	ch->Temporary.unset(ECharExtraFlag::kFailCamouflage);
	if (ch->IsFlagged(EPrf::kCoderinfo)) {
		sprintf(buf,
				"%sКомната=%s%d %sСвет=%s%d %sОсвещ=%s%d %sКостер=%s%d %sЛед=%s%d "
				"%sТьма=%s%d %sСолнце=%s%d %sНебо=%s%d %sЛуна=%s%d%s.\r\n",
				kColorNrm, kColorBoldBlk, room,
				kColorRed, kColorBoldRed, world[room]->light,
				kColorGrn, kColorBoldGrn, world[room]->glight,
				kColorYel, kColorBoldYel, world[room]->fires,
				kColorYel, kColorBoldYel, world[room]->ices,
				kColorBlu, kColorBoldBlu, world[room]->gdark,
				kColorMag, kColorBoldCyn, weather_info.sky,
				kColorWht, kColorBoldBlk, weather_info.sunlight,
				kColorYel, kColorBoldYel, weather_info.moon_day, kColorNrm);
		SendMsgToChar(buf, ch);
	}
	// Stop fighting now, if we left.
	if (ch->GetEnemy() && ch->in_room != ch->GetEnemy()->in_room) {
		stop_fighting(ch->GetEnemy(), false);
		stop_fighting(ch, true);
	}

	if (!ch->IsNpc() && !GET_INVIS_LEV(ch)) {
		MarkZoneUsed(world[room]->zone_rn);   // wake the zone (materializes flag-only NPC buffs on the edge)
		zone_table[world[room]->zone_rn].activity++;
	} else if (process_entry_affects) {
		//sventovit: здесь обрабатываются только неписи, чтобы игрок успел увидеть комнату
		//как сделать красивей я не придумал, т.к. look_at_room вызывается в act.movement а не тут
		room_spells::ProcessRoomAffectsOnEntry(ch, ch->in_room);
	}
	// issue.mob-flag-affect-materialization: a mob placed into an ALREADY-AWAKE zone (script mobload,
	// summon, reset under a present player) materializes its intrinsic buff flags here -- the wake-hook
	// only catches mobs present when the zone first wakes. Placement is bounded (spawns/relocations, not
	// routine wandering, which uses char_to_room), and MaterializeMobFlagAffects no-ops if the mob has no
	// such buffs or already has them.
	if (ch->IsNpc() && ch->in_room != kNowhere && zone_table[world[ch->in_room]->zone_rn].used) {
		MaterializeMobFlagAffects(ch);
	}
	cities::CheckCityVisit(ch, room);
}

void change_npc_leader(CharData *ch) {
	std::vector<CharData *> tmp_list;

	for (auto *i : ch->followers) {
		if (i->IsNpc()
			&& !IsCharmice(i)
			&& i->get_master() == ch) {
			tmp_list.push_back(i);
		}
	}
	if (tmp_list.empty()) {
		return;
	}

	CharData *leader = nullptr;
	for (auto i : tmp_list) {
		if (follow::StopFollower(i, follow::kSfSilence)) {
			continue;
		}
		if (!leader) {
			leader = i;
		} else {
			follow::AddFollowerSilently(leader, i);
		}
	}
}

void DropEquipment(CharData *ch, bool zone_reset) {
	for (auto i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i)) {
			ObjData *obj_eq = UnequipChar(ch, i, CharEquipFlags());
			if (!obj_eq) {
				continue;
			}
			remove_otrigger(obj_eq, ch);
			DropObjOnZoneReset(ch, obj_eq, false, zone_reset);
		}
	}
}

void DropInventory(CharData *ch, bool zone_reset) {
	while (ch->carrying) {
		ObjData *obj = ch->carrying;
		RemoveObjFromChar(obj);
		DropObjOnZoneReset(ch, obj, true, zone_reset);
	}
}

void ExtractCharFromWorld(CharData *ch, int clear_objs, bool zone_reset) {
	timechange_unregister_mob(ch);
	if (ch->purged()) {
		log("SYSERROR: double extract_char (%s:%d)", __FILE__, __LINE__);
		return;
	}

	if (ch->IsFlagged(EMobFlag::kMobFreed) || ch->IsFlagged(EMobFlag::kMobDeleted)) {
		return;
	}

	// issue.npc-races: every resurrected mob must also be marked undead (kResurrected => kUndead).
	// A kResurrected mob without kUndead means a game designer set the flag by mistake -- log it.
	if (ch->IsFlagged(EMobFlag::kResurrected) && !ch->IsFlagged(EMobFlag::kUndead)) {
		log("SYSERR: mob VNUM %d has kResurrected without kUndead (designer error: resurrected mobs must be undead).",
			GET_MOB_VNUM(ch));
	}

	std::string name = GET_NAME(ch);
	DescriptorData *t_desc;
	utils::CExecutionTimer timer;

	log("[Extract char] Start function for char %s VNUM: %d", name.c_str(), GET_MOB_VNUM(ch));
	if (!ch->IsNpc() && !ch->desc) {
//		log("[Extract char] Extract descriptors");
		for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next) {
			if (t_desc->original.get() == ch) {
				DoReturn(t_desc->character.get(), nullptr, 0, 0);
			}
		}
	}

	// Forget snooping, if applicable
//	log("[Extract char] Stop snooping");
	if (ch->desc) {
		if (ch->desc->snooping) {
			ch->desc->snooping->snoop_by = nullptr;
			ch->desc->snooping = nullptr;
		}

		if (ch->desc->snoop_by) {
			iosystem::write_to_output("Ваша жертва теперь недоступна.\r\n", ch->desc->snoop_by);
			ch->desc->snoop_by->snooping = nullptr;
			ch->desc->snoop_by = nullptr;
		}
	}

	// transfer equipment to room, if any
//	log("[Extract char] Drop equipment & inventory");
	if (ch->in_room != kNowhere) {
		DropEquipment(ch, zone_reset);
		DropInventory(ch, zone_reset);
	}

	if (!ch->IsNpc()
		&& !ch->has_master()
		&& !ch->followers.empty()
		&& AFF_FLAGGED(ch, EAffect::kGroup)) {
//		log("[Extract char] Change group leader");
		change_leader(ch, nullptr);
	} else if (ch->IsNpc()
		&& !IsCharmice(ch)
		&& !ch->has_master()
		&& !ch->followers.empty()) {
//		log("[Extract char] Changing NPC leader");
		change_npc_leader(ch);
	}

//	log("[Extract char] Die followers");
	if (!ch->followers.empty() || ch->has_master()) {
		follow::DieFollower(ch);
	}
//	log("[Extract char] Stop all fight for opponee");
	ChangeFighting(ch, true);

//	log("[Extract char] Remove char from room");
	if (ch->in_room != kNowhere) {
		RemoveCharFromRoom(ch);
	}

	// pull the char from the list
	ch->SetFlag(EMobFlag::kMobDeleted);

	if (ch->desc && ch->desc->original) {
		DoReturn(ch, nullptr, 0, 0);
	}

	const bool is_npc = ch->IsNpc();
	if (!is_npc) {
//		log("[Extract char] All save for PC");
		check_auction(ch, nullptr);
		ch->save_char();
		//удаляются рент-файлы, если только персонаж не ушел в ренту
		Crash_delete_crashfile(ch);
	} else {
//		log("[Extract char] All clear for NPC");
		// A warlock-revived corpse (kResurrected) reuses a real mob's vnum but is loaded without
		// bumping total_online (see ReadMobile is_corpse), so it must not decrement it either --
		// otherwise reviving e.g. a zone boss would block that boss's normal respawn.
		if ((ch->get_rnum() >= 0) && !ch->IsFlagged(EMobFlag::kResurrected)) {
			mob_index[ch->get_rnum()].total_online--;
		}
	}
	chardata_by_uid.erase(ch->get_uid());
	bool left_in_game = false;
	if (!is_npc
		&& ch->desc != nullptr) {
		ch->desc->state = EConState::kMenu;
		ShowMainMenu(ch->desc);
		if (!ch->IsNpc() && NORENTABLE(ch) && clear_objs) {
			ch->zero_wait();
			do_entergame(ch->desc);
			left_in_game = true;
		}
	}
	if (ch->get_protecting()) {
		ch->remove_protecting();
	}
	if (!ch->who_protecting.empty()) {
		auto protecting_copy = ch->who_protecting;
		for (auto &it : protecting_copy) {
			it->remove_protecting();
		}
	}
	if (!left_in_game) {
		character_list.remove(ch);
	}

	log("[Extract char] Stop function for char %s, delta %f", name.c_str(), timer.delta().count());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
