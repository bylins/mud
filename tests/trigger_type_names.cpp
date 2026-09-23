#include <gtest/gtest.h>

#include "engine/db/trigger_type_names.h"

namespace {

// Словарь нового образца: имена с префиксом своего типа плюс общие биты и старые имена,
// оставленные для чтения миров, которые ещё не перезаписаны.
std::unordered_map<std::string, long> NewDictionary() {
	return {
		{"kRandomGlobal", 0}, {"kRandom", 1}, {"kCommand", 2}, {"kAuto", 25},
		{"kMobAct", 4}, {"kMobFight", 10}, {"kMobLoad", 13}, {"kMobTimeChange", 23},
		{"kObjFight", 4}, {"kObjLoad", 13}, {"kObjTimeChange", 20},
		{"kWldEnterPc", 4}, {"kWldTimeChange", 13},
		{"kAct", 4}, {"kFight", 10}, {"kLoad", 13},
	};
}

// Словарь старого образца: только мобовые имена, без префиксов.
std::unordered_map<std::string, long> OldDictionary() {
	return {{"kRandom", 1}, {"kCommand", 2}, {"kAct", 4}, {"kFight", 10}, {"kLoad", 13}};
}

constexpr int kMob = 0;
constexpr int kObj = 1;
constexpr int kWld = 2;

}  // namespace

TEST(TriggerTypeNames, SameBitGetsItsOwnNamePerAttachType) {
	const auto dict = NewDictionary();
	EXPECT_EQ("kMobAct", world_format::PickTriggerTypeName(dict, kMob, 4));
	EXPECT_EQ("kObjFight", world_format::PickTriggerTypeName(dict, kObj, 4));
	EXPECT_EQ("kWldEnterPc", world_format::PickTriggerTypeName(dict, kWld, 4));
}

TEST(TriggerTypeNames, BitThirteenIsLoadForMobAndObjButTimeChangeForRoom) {
	const auto dict = NewDictionary();
	EXPECT_EQ("kMobLoad", world_format::PickTriggerTypeName(dict, kMob, 13));
	EXPECT_EQ("kObjLoad", world_format::PickTriggerTypeName(dict, kObj, 13));
	EXPECT_EQ("kWldTimeChange", world_format::PickTriggerTypeName(dict, kWld, 13));
}

TEST(TriggerTypeNames, SharedBitsKeepTheirPrefixlessName) {
	const auto dict = NewDictionary();
	EXPECT_EQ("kCommand", world_format::PickTriggerTypeName(dict, kMob, 2));
	EXPECT_EQ("kCommand", world_format::PickTriggerTypeName(dict, kObj, 2));
	EXPECT_EQ("kAuto", world_format::PickTriggerTypeName(dict, kWld, 25));
}

TEST(TriggerTypeNames, UnknownBitHasNoName) {
	const auto dict = NewDictionary();
	EXPECT_TRUE(world_format::PickTriggerTypeName(dict, kWld, 31).empty());
}

TEST(TriggerTypeNames, OldDictionaryKeepsTheOldNames) {
	const auto dict = OldDictionary();
	EXPECT_EQ("kAct", world_format::PickTriggerTypeName(dict, kMob, 4));
	EXPECT_EQ("kAct", world_format::PickTriggerTypeName(dict, kObj, 4));
	EXPECT_EQ("kAct", world_format::PickTriggerTypeName(dict, kWld, 4));
}

TEST(TriggerTypeNames, NameOfAnotherAttachTypeIsRejected) {
	EXPECT_TRUE(world_format::TriggerTypeNameFitsAttach("kMobAct", kMob));
	EXPECT_FALSE(world_format::TriggerTypeNameFitsAttach("kMobAct", kWld));
	EXPECT_FALSE(world_format::TriggerTypeNameFitsAttach("kObjFight", kMob));
	// Общие имена и старые имена без префикса подходят любому типу.
	EXPECT_TRUE(world_format::TriggerTypeNameFitsAttach("kCommand", kObj));
	EXPECT_TRUE(world_format::TriggerTypeNameFitsAttach("kAct", kWld));
}

TEST(TriggerTypeNames, PrefixByAttachType) {
	EXPECT_EQ("kMob", world_format::TriggerTypePrefix(kMob));
	EXPECT_EQ("kObj", world_format::TriggerTypePrefix(kObj));
	EXPECT_EQ("kWld", world_format::TriggerTypePrefix(kWld));
	EXPECT_TRUE(world_format::TriggerTypePrefix(7).empty());
}
