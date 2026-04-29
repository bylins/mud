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
// Returns the max_hit override if set in the spec, -1 otherwise.
int GetMaxHitOverride(const ParticipantSpec& spec) {
	return std::visit([](auto&& s) { return s.overrides.max_hit; }, spec);
}

// Apply YAML stat overrides to a freshly-spawned character. Each negative
// override is left alone (engine default kept).
void ApplyStatOverrides(CharData* ch, const StatOverrides& o) {
	if (o.str >= 0) ch->set_str(o.str);
	if (o.dex >= 0) ch->set_dex(o.dex);
	if (o.con >= 0) ch->set_con(o.con);
	if (o.intel >= 0) ch->set_int(o.intel);
	if (o.wis >= 0) ch->set_wis(o.wis);
	if (o.cha >= 0) ch->set_cha(o.cha);
	if (o.max_hit >= 0) {
		ch->set_max_hit(o.max_hit);
		ch->set_hit(o.max_hit);
	}
}

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
			character_list.push_front(sp);
			ApplyStatOverrides(sp.get(), s.overrides);
			return sp.get();
		} else if constexpr (std::is_same_v<T, MobSpec>) {
			const auto rnum = GetMobRnum(s.vnum);
			if (rnum < 0) {
				throw ScenarioRunError(fmt::format(
					"mob vnum {} not found in world", s.vnum));
			}
			CharData* mob = ReadMobile(rnum, kReal);
			if (!mob) {
				throw ScenarioRunError(fmt::format(
					"ReadMobile({}) returned null", s.vnum));
			}
			ApplyStatOverrides(mob, s.overrides);
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

// Periodic snapshot of a participant's state for replay-mode reconstruction.
// Emits hp/move/position; the full per-affect timeline is already covered by
// affect_added/removed events from src/gameplay/affects/affect_data.cpp.
void EmitCharState(observability::EventSink& sink, const char* role,
		const CharData* ch, int round_no) {
	observability::Event e;
	e.name = "char_state";
	e.ts_unix_ms = NowUnixMs();
	e.attrs["round"] = static_cast<std::int64_t>(round_no);
	e.attrs["role"] = std::string(role);
	e.attrs["target_name"] = observability::EngineStringToUtf8(
		GET_NAME(ch) ? GET_NAME(ch) : "");
	e.attrs["hp"] = static_cast<std::int64_t>(ch->get_hit());
	e.attrs["max_hp"] = static_cast<std::int64_t>(ch->get_max_hit());
	e.attrs["move"] = static_cast<std::int64_t>(ch->get_move());
	e.attrs["max_move"] = static_cast<std::int64_t>(ch->get_max_move());
	e.attrs["position"] = static_cast<std::int64_t>(ch->GetPosition());
	e.attrs["in_room"] = static_cast<std::int64_t>(ch->in_room);
	sink.Emit(e);
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

	// By default both get massive HP so the duel survives all rounds. If a
	// participant has an explicit max_hit override in the YAML scenario, that
	// override wins (it was already applied inside SpawnParticipant and we
	// must not overwrite it here).
	if (GetMaxHitOverride(scenario.attacker) < 0) {
		attacker->set_max_hit(kHugeHp);
		attacker->set_hit(kHugeHp);
	}
	if (GetMaxHitOverride(scenario.victim) < 0) {
		victim->set_max_hit(kHugeHp);
		victim->set_hit(kHugeHp);
	}

	const auto* cast = std::get_if<CastAction>(&scenario.action);
	// In both melee and cast scenarios we engage once at the start. Cast spells
	// like kCallLighting need kTarFightVict to resolve a target; SetFighting
	// also makes get_char_vis() find the mob reliably even with multi-word
	// keywords like "kostyanaya gonchaya".
	SetFighting(attacker, victim);

	// Initial snapshot before the first round, so replay can reconstruct
	// starting state.
	EmitCharState(sink, "attacker", attacker, -1);
	EmitCharState(sink, "victim", victim, -1);

	int prev_hp = victim->get_hit();

	// Per-round event: tick one battle round (kBattleRound pulses == one
	// perform_violence step), measure HP delta, emit. For 'cast' scenarios
	// we additionally invoke DoCast at the start of each round (waiting out
	// kGlobalCooldown if it is still active) and record cast_attempt events.
	for (int r = 0; r < scenario.rounds; ++r) {
		if (cast) {
			// Wait out any leftover global cooldown AND wait_state (cast time
			// for the previous spell). Cap the wait so a misconfigured spell
			// can't hang the run.
			constexpr long long kMaxWaitPulses = kBattleRound * 4;
			long long waited = 0;
			while ((attacker->HasCooldown(ESkill::kGlobalCooldown) ||
					attacker->get_wait() > 0) &&
					waited < kMaxWaitPulses) {
				MUD::heartbeat()(0);
				++waited;
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
					// DoCast выставил бы wait_state на kBattleRound при
					// нормальном пути; для прямого CastSpell (минуем DoCast,
					// см. комментарий выше) делаем то же руками, иначе
					// spell-каст в каждом раунде = переоценённый dpr.
					SetWaitState(attacker, kBattleRound);
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

		// Per-round snapshot for both participants. Replay tooling consumes
		// these to reconstruct any participant's state at any round without
		// having to fold all damage/affect events.
		if (alive) {
			EmitCharState(sink, "attacker", attacker, r);
			EmitCharState(sink, "victim", victim, r);
		}

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
