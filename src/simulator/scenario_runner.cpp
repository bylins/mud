#include "scenario_runner.h"

#include "engine/core/handler.h"
#include "engine/db/db.h"
#include "engine/db/global_objects.h"
#include "engine/db/world_characters.h"
#include "engine/entities/character_builder.h"
#include "engine/entities/char_data.h"
#include "engine/structs/structs.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/fight/fight.h"
#include "utils/logger.h"
#include "utils/utils.h"

#include <fmt/format.h>

#include <array>
#include <chrono>
#include <climits>
#include <stdexcept>
#include <string>

namespace simulator {

namespace {

// Room rnum used to place participants. rnum 1 is what the in-engine `vstat`
// command uses for the same temp-spawn pattern, so we follow suit.
constexpr RoomRnum kArenaRoom = 1;

// Huge HP pool: both participants must survive `scenario.rounds` battle rounds
// so we can observe the full duel. Headroom away from INT_MAX avoids
// arithmetic overflow inside the engine.
constexpr int kHugeHp = INT_MAX / 4;

std::int64_t NowUnixMs() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// Engine strings (class names, mob descriptions) live in KOI8-R; nlohmann::json
// validates UTF-8 on serialization and rejects KOI8-R bytes. Convert before
// emission. Buffer sized generously: KOI8-R -> UTF-8 expands to 2 bytes per
// non-ASCII char.
std::string Koi8rToUtf8(const std::string& src) {
	std::array<char, 2048> buf{};
	std::string mut = src;  // koi_to_utf8 takes char*, not const
	mut.push_back('\0');
	koi_to_utf8(mut.data(), buf.data());
	return std::string(buf.data());
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
			e.attrs[std::string(role) + "_class"] = Koi8rToUtf8(s.class_name);
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

	SetFighting(attacker, victim);

	int prev_hp = victim->get_hit();

	// Per-round event: tick exactly one battle round (kBattleRound pulses ==
	// one perform_violence step), measure HP delta, emit the event. Per-pulse
	// instrumentation (per-swing roll/hit/miss) is a backlog item; for now
	// the HP delta is the observable.
	for (int r = 0; r < scenario.rounds; ++r) {
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
