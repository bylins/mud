#include "scenario_runner.h"

#include "engine/core/handler.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "engine/db/world_characters.h"
#include "engine/entities/character_builder.h"
#include "engine/entities/char_data.h"
#include "engine/entities/room_data.h"
#include "engine/structs/structs.h"
#include "engine/ui/cmd/do_cast.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/fight/fight.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/magic/spells.h"
#include "utils/utils.h"
#include "utils/logger.h"

#include <fmt/format.h>

#include <chrono>
#include <climits>
#include <stdexcept>
#include <string>
#include <vector>

namespace simulator {

namespace {

// Room rnum used to place participants. rnum 1 is what the in-engine `vstat`
// command uses for the same temp-spawn pattern, so we follow suit.
constexpr RoomRnum kArenaRoom = 1;

// Some real-world rooms have flags that restrict combat or magic
// (kNoMagic, kPeaceful, etc.). For a balance arena we want the cleanest
// possible environment, so we strip such flags from kArenaRoom before each
// run. The flags are restored on cleanup so we do not corrupt the loaded
// world for subsequent runs.
struct ArenaFlagSweep {
	RoomData* room = nullptr;
	bool had_no_magic = false;
	bool had_peaceful = false;

	explicit ArenaFlagSweep(RoomData* r) : room(r) {
		if (!room) {
			return;
		}
		had_no_magic = room->get_flag(ERoomFlag::kNoMagic);
		had_peaceful = room->get_flag(ERoomFlag::kPeaceful);
		if (had_no_magic) {
			room->unset_flag(ERoomFlag::kNoMagic);
		}
		if (had_peaceful) {
			room->unset_flag(ERoomFlag::kPeaceful);
		}
	}

	~ArenaFlagSweep() {
		if (!room) {
			return;
		}
		if (had_no_magic) {
			room->set_flag(ERoomFlag::kNoMagic);
		}
		if (had_peaceful) {
			room->set_flag(ERoomFlag::kPeaceful);
		}
	}
};

// Huge HP pool: both participants must survive `scenario.rounds` battle rounds
// so we can observe the full duel. Headroom away from INT_MAX avoids
// arithmetic overflow inside the engine.
constexpr int kHugeHp = INT_MAX / 4;

std::int64_t NowUnixMs() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}


// Spawn a participant and register it with the global character_list so its
// shared_ptr is owned globally for the duration of the run. Returns the raw
// pointer; ownership stays with character_list until ExtractCharFromWorld is
// called.
CharData* SpawnParticipant(const ParticipantSpec& spec) {
	return std::visit([](auto&& s) -> CharData* {
		using T = std::decay_t<decltype(s)>;
		if constexpr (std::is_same_v<T, PlayerSpec>) {
			const auto cls = FindAvailableCharClassId(s.class_name);
			if (cls == ECharClass::kUndefined) {
				throw ScenarioRunError(fmt::format(
					"unknown player class: '{}'", s.class_name));
			}
			entities::CharacterBuilder b;
			b.make_basic_player(static_cast<short>(cls), s.level);
			auto sp = b.get();
			character_list.push_front(sp);  // global ownership for the duel
			return sp.get();
		} else if constexpr (std::is_same_v<T, MobSpec>) {
			const auto rnum = GetMobRnum(s.vnum);
			if (rnum < 0) {
				throw ScenarioRunError(fmt::format(
					"mob vnum {} not found in world", s.vnum));
			}
			CharData* mob = ReadMobile(rnum, kReal);  // already added to character_list
			if (!mob) {
				throw ScenarioRunError(fmt::format(
					"ReadMobile({}) returned null", s.vnum));
			}
			return mob;
		}
		return static_cast<CharData*>(nullptr);
	}, spec);
}

void AddParticipantAttrs(observability::Event& e, const char* role, const ParticipantSpec& spec) {
	std::visit([&e, role](auto&& s) {
		using T = std::decay_t<decltype(s)>;
		if constexpr (std::is_same_v<T, PlayerSpec>) {
			e.attrs[std::string(role) + "_type"] = std::string("player");
			e.attrs[std::string(role) + "_class"] = observability::EngineStringToUtf8(s.class_name);
			e.attrs[std::string(role) + "_level"] = static_cast<std::int64_t>(s.level);
		} else if constexpr (std::is_same_v<T, MobSpec>) {
			e.attrs[std::string(role) + "_type"] = std::string("mob");
			e.attrs[std::string(role) + "_vnum"] = static_cast<std::int64_t>(s.vnum);
		}
	}, spec);
}

}  // namespace

void RunScenario(const Scenario& scenario, observability::EventSink& sink) {
	if (scenario.rounds <= 0) {
		throw ScenarioRunError("scenario.rounds must be positive");
	}

	// Strip room-level combat/magic restrictions for the arena; restored on
	// scope exit.
	ArenaFlagSweep arena_flags(world[kArenaRoom]);

	// Spawn once. The duel runs continuously; we observe each battle round.
	CharData* attacker = SpawnParticipant(scenario.attacker);
	CharData* victim = SpawnParticipant(scenario.victim);
	PlaceCharToRoom(attacker, kArenaRoom);
	PlaceCharToRoom(victim, kArenaRoom);

	// Both get massive HP so the duel survives all `rounds` battle rounds and
	// we observe per-round damage rather than just kill timing.
	attacker->set_max_hit(kHugeHp);
	attacker->set_hit(kHugeHp);
	victim->set_max_hit(kHugeHp);
	victim->set_hit(kHugeHp);

	const auto* cast = std::get_if<CastAction>(&scenario.action);
	// In both melee and cast scenarios we engage once at the start. Cast spells
	// like kCallLighting need kTarFightVict to resolve a target; SetFighting
	// also makes get_char_vis() find the mob reliably even with multi-word
	// keywords like "kostyanaya gonchaya".
	SetFighting(attacker, victim);

	int prev_hp = victim->get_hit();

	// Per-round event: tick one battle round (kBattleRound pulses == one
	// perform_violence step), measure HP delta, emit. For 'cast' scenarios
	// we additionally invoke DoCast at the start of each round (waiting out
	// kGlobalCooldown if it is still active) and record cast_attempt events.
	for (int r = 0; r < scenario.rounds; ++r) {
		if (cast) {
			// Wait out any leftover global cooldown, then cast. The argument
			// to DoCast is "'spell_name' victim_keyword" (single quotes
			// around the spell name, then the target keyword).
			while (attacker->HasCooldown(ESkill::kGlobalCooldown)) {
				MUD::heartbeat()(0);
				if (attacker->in_room == kNowhere || victim->in_room == kNowhere) {
					break;
				}
			}
			if (attacker->in_room != kNowhere && victim->in_room != kNowhere) {
				// We bypass DoCast and call CastSpell directly: DoCast routes
				// to ch->SetCast() (deferred cast) when there is an enemy,
				// which makes the magic damage land later through the
				// mem_queue heartbeat step and not through Damage::Process
				// (or at level 0). For simulator we want immediate, observable
				// magic damage on every casting round.
				std::string spell_name = cast->spell_name;
				const auto sid = FixNameAndFindSpellId(spell_name);
				if (sid != ESpell::kUndefined) {
					CastSpell(attacker, victim, nullptr, nullptr, sid, sid);
					if (GET_SPELL_MEM(attacker, sid) > 0) {
						GET_SPELL_MEM(attacker, sid)--;
					}
				}
			}
		}
		for (long long p = 0; p < kBattleRound; ++p) {
			MUD::heartbeat()(0);
			if (attacker->in_room == kNowhere || victim->in_room == kNowhere) {
				break;  // somebody died or got extracted
			}
		}
		const bool alive = attacker->in_room != kNowhere && victim->in_room != kNowhere;
		const int hp_now = alive ? victim->get_hit() : 0;
		const int damage = alive ? (prev_hp - hp_now) : prev_hp;

		observability::Event e;
		e.name = "round";
		e.ts_unix_ms = NowUnixMs();
		e.attrs["round"] = static_cast<std::int64_t>(r);
		AddParticipantAttrs(e, "attacker", scenario.attacker);
		AddParticipantAttrs(e, "victim", scenario.victim);
		e.attrs["hp_before"] = static_cast<std::int64_t>(prev_hp);
		e.attrs["hp_after"] = static_cast<std::int64_t>(hp_now);
		e.attrs["damage_observed"] = static_cast<std::int64_t>(damage);
		e.attrs["victim_alive"] = alive;
		sink.Emit(e);

		if (!alive) {
			break;
		}
		prev_hp = hp_now;
	}

	// Cleanup. ExtractCharFromWorld removes from character_list and frees the
	// underlying object.
	if (attacker->in_room != kNowhere) {
		ExtractCharFromWorld(attacker, false);
	}
	if (victim->in_room != kNowhere) {
		ExtractCharFromWorld(victim, false);
	}
	sink.Flush();
}

}  // namespace simulator

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
