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
