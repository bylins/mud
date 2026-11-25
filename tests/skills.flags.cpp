// issue #2689: skill flags from cfg/skills.xml (flags="kHidden|kObligatory|..."), the deprecated
// self_train attribute, and the room check ImproveSkill makes with the flags.

#include "gameplay/skills/skills.h"
#include "gameplay/skills/skills_info.h"
#include "engine/entities/room_data.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

const char *kSkillSrc = "skills_flags_test.xml";

SkillInfoBuilder::ItemPtr ParseSkill(const std::string &attributes) {
	{
		std::ofstream f(kSkillSrc);
		f << R"(<skills><skill id="kBash" saving="kStability" difficulty="200" cap="1000" )" << attributes
		  << "/></skills>";
	}
	parser_wrapper::DataNode doc(kSkillSrc);
	doc.GoToChild("skill");
	SkillInfoBuilder builder;
	auto skill = builder.Build(doc);
	std::remove(kSkillSrc);
	return skill;
}

SkillInfo MakeSkill(bool train_in_peaceful, bool train_in_house) {
	SkillInfo skill(ESkill::kBash, EItemMode::kEnabled);
	if (train_in_peaceful) {
		skill.flags.set(ESkillFlag::kTrainInPeaceful);
	}
	if (train_in_house) {
		skill.flags.set(ESkillFlag::kTrainInHouse);
	}
	return skill;
}

RoomData MakeRoom(std::initializer_list<ERoomFlag> flags) {
	RoomData room;
	for (const auto flag : flags) {
		room.set_flag(flag);
	}
	return room;
}

} // namespace

const ESkillFlag kAllFlags[] = {ESkillFlag::kSelfTrain, ESkillFlag::kHidden, ESkillFlag::kTrainInPeaceful,
								  ESkillFlag::kTrainInHouse, ESkillFlag::kObligatory, ESkillFlag::kKeepOnRemort};

TEST(SkillFlags, OffByDefault) {
	const auto skill = ParseSkill("");
	ASSERT_TRUE(skill);
	for (const auto flag : kAllFlags) {
		EXPECT_FALSE(skill->IsFlagged(flag)) << NAME_BY_ITEM<ESkillFlag>(flag);
	}
}

TEST(SkillFlags, NamesRoundTrip) {
	for (const auto flag : kAllFlags) {
		EXPECT_EQ(ITEM_BY_NAME<ESkillFlag>(NAME_BY_ITEM<ESkillFlag>(flag)), flag);
	}
	EXPECT_THROW(ITEM_BY_NAME<ESkillFlag>("kNoSuchFlag"), std::out_of_range);
}

TEST(SkillFlags, ParsedFromList) {
	const auto skill = ParseSkill(R"(flags="kSelfTrain|kHidden|kTrainInPeaceful|kTrainInHouse|kObligatory|kKeepOnRemort")");
	ASSERT_TRUE(skill);
	for (const auto flag : kAllFlags) {
		EXPECT_TRUE(skill->IsFlagged(flag)) << NAME_BY_ITEM<ESkillFlag>(flag);
	}
}

TEST(SkillFlags, ParsedSeparately) {
	const auto skill = ParseSkill(R"(flags="kHidden|kObligatory")");
	ASSERT_TRUE(skill);
	EXPECT_TRUE(skill->IsFlagged(ESkillFlag::kHidden));
	EXPECT_TRUE(skill->IsFlagged(ESkillFlag::kObligatory));
	EXPECT_FALSE(skill->IsFlagged(ESkillFlag::kSelfTrain));
	EXPECT_FALSE(skill->IsFlagged(ESkillFlag::kTrainInPeaceful));
	EXPECT_FALSE(skill->IsFlagged(ESkillFlag::kTrainInHouse));
}

// A misspelled flag is dropped (and logged); the rest of the list still counts.
TEST(SkillFlags, UnknownFlagSkipped) {
	const auto skill = ParseSkill(R"(flags="kHidden|kObligatroy")");
	ASSERT_TRUE(skill);
	EXPECT_TRUE(skill->IsFlagged(ESkillFlag::kHidden));
	EXPECT_FALSE(skill->IsFlagged(ESkillFlag::kObligatory));
}

// The deprecated self_train attribute still works, alone or next to the list.
TEST(SkillFlags, DeprecatedSelfTrain) {
	EXPECT_TRUE(ParseSkill(R"(self_train="1")")->IsFlagged(ESkillFlag::kSelfTrain));
	EXPECT_FALSE(ParseSkill(R"(self_train="0")")->IsFlagged(ESkillFlag::kSelfTrain));
	const auto both = ParseSkill(R"(flags="kHidden" self_train="1")");
	EXPECT_TRUE(both->IsFlagged(ESkillFlag::kHidden));
	EXPECT_TRUE(both->IsFlagged(ESkillFlag::kSelfTrain));
}

TEST(SkillTrainableInRoom, NoRoomNever) {
	EXPECT_FALSE(IsSkillTrainableInRoom(MakeSkill(true, true), nullptr));
}

TEST(SkillTrainableInRoom, OrdinaryRoomAlways) {
	const auto room = MakeRoom({});
	EXPECT_TRUE(IsSkillTrainableInRoom(MakeSkill(false, false), &room));
	EXPECT_TRUE(IsSkillTrainableInRoom(MakeSkill(true, true), &room));
}

TEST(SkillTrainableInRoom, ArenaNever) {
	const auto room = MakeRoom({ERoomFlag::kArena});
	EXPECT_FALSE(IsSkillTrainableInRoom(MakeSkill(true, true), &room));
}

TEST(SkillTrainableInRoom, PeacefulOnlyWithFlag) {
	const auto room = MakeRoom({ERoomFlag::kPeaceful});
	EXPECT_FALSE(IsSkillTrainableInRoom(MakeSkill(false, false), &room));
	EXPECT_FALSE(IsSkillTrainableInRoom(MakeSkill(false, true), &room));
	EXPECT_TRUE(IsSkillTrainableInRoom(MakeSkill(true, false), &room));
}

TEST(SkillTrainableInRoom, HouseOnlyWithFlag) {
	for (const auto flag : {ERoomFlag::kHouse, ERoomFlag::kHouseEntry}) {
		const auto room = MakeRoom({flag});
		EXPECT_FALSE(IsSkillTrainableInRoom(MakeSkill(false, false), &room));
		EXPECT_FALSE(IsSkillTrainableInRoom(MakeSkill(true, false), &room));
		EXPECT_TRUE(IsSkillTrainableInRoom(MakeSkill(false, true), &room));
	}
}

TEST(SkillTrainableInRoom, PeacefulHouseNeedsBothFlags) {
	const auto room = MakeRoom({ERoomFlag::kPeaceful, ERoomFlag::kHouse});
	EXPECT_FALSE(IsSkillTrainableInRoom(MakeSkill(true, false), &room));
	EXPECT_FALSE(IsSkillTrainableInRoom(MakeSkill(false, true), &room));
	EXPECT_TRUE(IsSkillTrainableInRoom(MakeSkill(true, true), &room));
}

// The shipped config: piety (kReligion) is hidden, grows in temples (peaceful rooms), is obligatory (so
// every class must list it -- otherwise it is not saved for that class's characters) and survives the
// remort skill reset.
TEST(SkillFlags, ShippedReligionSkill) {
	parser_wrapper::DataNode doc(std::string("cfg/skills.xml"));
	bool found = false;
	for (auto node : doc.Children("skill")) {
		if (std::string(node.GetValue("id")) != "kReligion") {
			continue;
		}
		SkillInfoBuilder builder;
		const auto skill = builder.Build(node);
		ASSERT_TRUE(skill);
		EXPECT_TRUE(skill->IsAvailable());
		EXPECT_TRUE(skill->IsFlagged(ESkillFlag::kHidden));
		EXPECT_TRUE(skill->IsFlagged(ESkillFlag::kTrainInPeaceful));
		EXPECT_TRUE(skill->IsFlagged(ESkillFlag::kObligatory));
		EXPECT_TRUE(skill->IsFlagged(ESkillFlag::kKeepOnRemort));
		found = true;
	}
	EXPECT_TRUE(found);
}

// The shipped config uses only the flag list: no skill keeps the deprecated self_train attribute, and
// the eight magic skills train on self.
TEST(SkillFlags, ShippedSelfTrain) {
	parser_wrapper::DataNode doc(std::string("cfg/skills.xml"));
	int self_train = 0;
	for (auto node : doc.Children("skill")) {
		const char *legacy = node.GetValue("self_train");
		EXPECT_TRUE(!legacy || !*legacy) << node.GetValue("id");
		SkillInfoBuilder builder;
		const auto skill = builder.Build(node);
		if (skill && skill->IsFlagged(ESkillFlag::kSelfTrain)) {
			++self_train;
		}
	}
	EXPECT_EQ(self_train, 8);
}

TEST(SkillFlags, EveryClassListsReligion) {
	const std::array<const char *, 14> kClassFiles{
		"pc_assasine", "pc_charmer", "pc_conjurer", "pc_guard", "pc_magus", "pc_merchant", "pc_necromancer",
		"pc_paladine", "pc_ranger", "pc_sorcerer", "pc_thief", "pc_vigilant", "pc_warrior", "pc_wizard"};
	for (const auto *file : kClassFiles) {
		parser_wrapper::DataNode doc(std::string("cfg/classes/") + file + ".xml");
		ASSERT_TRUE(doc.GoToChild("skills")) << file;
		bool found = false;
		for (auto node : doc.Children("skill")) {
			if (std::string(node.GetValue("id")) == "kReligion") {
				found = true;
			}
		}
		EXPECT_TRUE(found) << file;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
