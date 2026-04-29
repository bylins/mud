#include "scenario_runner.h"

#include "engine/core/handler.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "engine/entities/character_builder.h"
#include "engine/entities/char_data.h"
#include "engine/structs/structs.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/fight/fight.h"
#include "utils/logger.h"

#include <fmt/format.h>

#include <chrono>
#include <climits>
#include <stdexcept>
#include <string>

namespace simulator {

namespace {

// Room rnum used to place participants. rnum 1 is what the in-engine `vstat`
// command uses for the same temp-spawn pattern, so we follow suit.
constexpr RoomRnum kArenaRoom = 1;

// Huge HP pool for the attacker so its own death does not cut the run short.
// We do not max it to INT_MAX to leave headroom for any internal arithmetic.
constexpr int kAttackerHpHuge = INT_MAX / 4;

std::int64_t NowUnixMs() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
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
			b.set_max_hit(kAttackerHpHuge);
			b.set_hit(kAttackerHpHuge);
			// Note: we keep the shared_ptr alive via global character_list once
			// the engine takes ownership in PlaceCharToRoom; for now return raw.
			return b.get().get();
		} else if constexpr (std::is_same_v<T, MobSpec>) {
			const auto rnum = GetMobRnum(s.vnum);
			if (rnum < 0) {
				throw ScenarioRunError(fmt::format(
					"victim mob vnum {} not found in world", s.vnum));
			}
			CharData* mob = ReadMobile(rnum, kReal);
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
			e.attrs[std::string(role) + "_class"] = s.class_name;
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

	for (int r = 0; r < scenario.rounds; ++r) {
		CharData* attacker = SpawnParticipant(scenario.attacker);
		CharData* victim = SpawnParticipant(scenario.victim);
		PlaceCharToRoom(attacker, kArenaRoom);
		PlaceCharToRoom(victim, kArenaRoom);

		const int hp_before = victim->get_hit();

		SetFighting(attacker, victim);

		// One battle round = kBattleRound pulses (~2 wall-clock seconds at the
		// real heartbeat rate, but here we tick as fast as possible).
		for (long long p = 0; p < kBattleRound; ++p) {
			MUD::heartbeat()(0);
			if (!attacker->in_room || !victim->in_room) {
				// One of the participants left the room (death extracts to NOWHERE).
				break;
			}
		}

		const int hp_after = victim->in_room ? victim->get_hit() : 0;
		const bool victim_alive = victim->in_room && hp_after > 0;

		observability::Event e;
		e.name = "hit";
		e.ts_unix_ms = NowUnixMs();
		e.attrs["round"] = static_cast<std::int64_t>(r);
		AddParticipantAttrs(e, "attacker", scenario.attacker);
		AddParticipantAttrs(e, "victim", scenario.victim);
		e.attrs["hp_before"] = static_cast<std::int64_t>(hp_before);
		e.attrs["hp_after"] = static_cast<std::int64_t>(hp_after);
		e.attrs["damage_observed"] = static_cast<std::int64_t>(hp_before - hp_after);
		e.attrs["victim_alive"] = victim_alive;
		sink.Emit(e);

		// Cleanup: attacker is built by CharacterBuilder (shared_ptr lifetime
		// auto-cleans on scope exit, but it was placed into the room, extract
		// it first). Victim is owned by character_list; ExtractCharFromWorld
		// removes it from world and frees it.
		if (attacker->in_room != kNowhere) {
			ExtractCharFromWorld(attacker, false);
		}
		if (victim->in_room != kNowhere) {
			ExtractCharFromWorld(victim, false);
		}
	}

	sink.Flush();
}

}  // namespace simulator

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
