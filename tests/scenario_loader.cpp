#include <gtest/gtest.h>

#include "simulator/scenario.h"
#include "simulator/scenario_loader.h"

#include <variant>

namespace {

constexpr const char* kMinimalScenario = R"(
seed: 42
rounds: 100
output: /tmp/sim.jsonl
attacker:
  type: player
  class: sorcerer
  level: 30
victim:
  type: mob
  vnum: 1234
)";

}  // namespace

TEST(ScenarioLoader, ParsesMinimalScenario) {
	const auto s = simulator::LoadScenarioFromString(kMinimalScenario);
	EXPECT_EQ(s.seed, 42u);
	EXPECT_EQ(s.rounds, 100);
	EXPECT_EQ(s.output, "/tmp/sim.jsonl");
	ASSERT_TRUE(std::holds_alternative<simulator::PlayerSpec>(s.attacker));
	const auto& a = std::get<simulator::PlayerSpec>(s.attacker);
	EXPECT_EQ(a.class_name, "sorcerer");
	EXPECT_EQ(a.level, 30);
	ASSERT_TRUE(std::holds_alternative<simulator::MobSpec>(s.victim));
	EXPECT_EQ(std::get<simulator::MobSpec>(s.victim).vnum, 1234);
}

TEST(ScenarioLoader, MobAsAttackerAndPlayerAsVictim) {
	const auto s = simulator::LoadScenarioFromString(R"(
rounds: 5
output: /tmp/x.jsonl
attacker:
  type: mob
  vnum: 7
victim:
  type: player
  class: warrior
  level: 12
)");
	ASSERT_TRUE(std::holds_alternative<simulator::MobSpec>(s.attacker));
	EXPECT_EQ(std::get<simulator::MobSpec>(s.attacker).vnum, 7);
	ASSERT_TRUE(std::holds_alternative<simulator::PlayerSpec>(s.victim));
	const auto& v = std::get<simulator::PlayerSpec>(s.victim);
	EXPECT_EQ(v.class_name, "warrior");
	EXPECT_EQ(v.level, 12);
}

TEST(ScenarioLoader, PlayerVsPlayerWithCastAction) {
	const auto s = simulator::LoadScenarioFromString(R"(
seed: 7
rounds: 20
output: /tmp/pvp.jsonl
attacker:
  type: player
  class: koldun
  level: 30
victim:
  type: player
  class: bogatyr
  level: 30
action:
  type: cast
  spell: lightning
)");
	ASSERT_TRUE(std::holds_alternative<simulator::PlayerSpec>(s.attacker));
	ASSERT_TRUE(std::holds_alternative<simulator::PlayerSpec>(s.victim));
	const auto& a = std::get<simulator::PlayerSpec>(s.attacker);
	const auto& v = std::get<simulator::PlayerSpec>(s.victim);
	EXPECT_EQ(a.class_name, "koldun");
	EXPECT_EQ(v.class_name, "bogatyr");
	ASSERT_TRUE(std::holds_alternative<simulator::CastAction>(s.action));
	EXPECT_EQ(std::get<simulator::CastAction>(s.action).spell_name, "lightning");
}

TEST(ScenarioLoader, StatOverridesApplied) {
	const auto s = simulator::LoadScenarioFromString(R"(
output: /tmp/x.jsonl
attacker:
  type: player
  class: koldun
  level: 30
  stats:
    str: 30
    dex: 35
    con: 40
    int: 50
    wis: 60
    cha: 25
    max_hit: 1500
victim: { type: mob, vnum: 7 }
)");
	const auto& a = std::get<simulator::PlayerSpec>(s.attacker);
	EXPECT_EQ(a.overrides.str, 30);
	EXPECT_EQ(a.overrides.dex, 35);
	EXPECT_EQ(a.overrides.con, 40);
	EXPECT_EQ(a.overrides.intel, 50);
	EXPECT_EQ(a.overrides.wis, 60);
	EXPECT_EQ(a.overrides.cha, 25);
	EXPECT_EQ(a.overrides.max_hit, 1500);
}

TEST(ScenarioLoader, StatOverridesPartial) {
	const auto s = simulator::LoadScenarioFromString(R"(
output: /tmp/x.jsonl
attacker:
  type: player
  class: koldun
  level: 30
  stats:
    wis: 50
victim: { type: mob, vnum: 7 }
)");
	const auto& a = std::get<simulator::PlayerSpec>(s.attacker);
	EXPECT_EQ(a.overrides.wis, 50);
	EXPECT_EQ(a.overrides.str, -1);
	EXPECT_EQ(a.overrides.max_hit, -1);
}

TEST(ScenarioLoader, MobMaxHitOverride) {
	const auto s = simulator::LoadScenarioFromString(R"(
output: /tmp/x.jsonl
attacker: { type: mob, vnum: 1 }
victim:
  type: mob
  vnum: 102
  stats:
    max_hit: 5000
)");
	const auto& v = std::get<simulator::MobSpec>(s.victim);
	EXPECT_EQ(v.overrides.max_hit, 5000);
}

TEST(ScenarioLoader, MeleeActionDefault) {
	const auto s = simulator::LoadScenarioFromString(R"(
output: /tmp/x.jsonl
attacker: { type: mob, vnum: 1 }
victim:   { type: mob, vnum: 2 }
)");
	EXPECT_TRUE(std::holds_alternative<simulator::MeleeAction>(s.action));
}

TEST(ScenarioLoader, ExplicitMeleeAction) {
	const auto s = simulator::LoadScenarioFromString(R"(
output: /tmp/x.jsonl
attacker: { type: mob, vnum: 1 }
victim:   { type: mob, vnum: 2 }
action:   { type: melee }
)");
	EXPECT_TRUE(std::holds_alternative<simulator::MeleeAction>(s.action));
}

TEST(ScenarioLoader, CastActionWithoutSpellIsFatal) {
	EXPECT_THROW(simulator::LoadScenarioFromString(R"(
output: /tmp/x.jsonl
attacker: { type: mob, vnum: 1 }
victim:   { type: mob, vnum: 2 }
action:   { type: cast }
)"), simulator::ScenarioLoadError);
}

TEST(ScenarioLoader, UnknownActionTypeIsFatal) {
	EXPECT_THROW(simulator::LoadScenarioFromString(R"(
output: /tmp/x.jsonl
attacker: { type: mob, vnum: 1 }
victim:   { type: mob, vnum: 2 }
action:   { type: dance }
)"), simulator::ScenarioLoadError);
}

TEST(ScenarioLoader, DefaultsSeedAndRounds) {
	const auto s = simulator::LoadScenarioFromString(R"(
output: /tmp/x.jsonl
attacker: { type: mob, vnum: 1 }
victim:   { type: mob, vnum: 2 }
)");
	EXPECT_EQ(s.seed, 0u);
	EXPECT_EQ(s.rounds, 100);
}

TEST(ScenarioLoader, MissingOutputIsFatal) {
	EXPECT_THROW(simulator::LoadScenarioFromString(R"(
attacker: { type: mob, vnum: 1 }
victim:   { type: mob, vnum: 2 }
)"), simulator::ScenarioLoadError);
}

TEST(ScenarioLoader, MissingAttackerIsFatal) {
	EXPECT_THROW(simulator::LoadScenarioFromString(R"(
output: /tmp/x.jsonl
victim: { type: mob, vnum: 2 }
)"), simulator::ScenarioLoadError);
}

TEST(ScenarioLoader, UnknownParticipantTypeIsFatal) {
	EXPECT_THROW(simulator::LoadScenarioFromString(R"(
output: /tmp/x.jsonl
attacker: { type: ghost }
victim:   { type: mob, vnum: 2 }
)"), simulator::ScenarioLoadError);
}

TEST(ScenarioLoader, NonPositiveRoundsIsFatal) {
	EXPECT_THROW(simulator::LoadScenarioFromString(R"(
rounds: 0
output: /tmp/x.jsonl
attacker: { type: mob, vnum: 1 }
victim:   { type: mob, vnum: 2 }
)"), simulator::ScenarioLoadError);
}

TEST(ScenarioLoader, MalformedYamlIsFatal) {
	EXPECT_THROW(simulator::LoadScenarioFromString("seed: ["),
		simulator::ScenarioLoadError);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
