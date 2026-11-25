// issue #2689: skill flags from cfg/skills.xml -- hidden, train_in_peaceful, train_in_house, obligatory --
// and the room check ImproveSkill makes with them.

#include "gameplay/skills/skills.h"
#include "gameplay/skills/skills_info.h"
#include "engine/entities/room_data.h"
#include "utils/parser_wrapper.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <fstream>
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
	skill.train_in_peaceful = train_in_peaceful;
	skill.train_in_house = train_in_house;
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

TEST(SkillFlags, OffByDefault) {
	const auto skill = ParseSkill("");
	ASSERT_TRUE(skill);
	EXPECT_FALSE(skill->hidden);
	EXPECT_FALSE(skill->train_in_peaceful);
	EXPECT_FALSE(skill->train_in_house);
	EXPECT_FALSE(skill->obligatory);
}

TEST(SkillFlags, ParsedFromAttributes) {
	const auto skill = ParseSkill(R"(hidden="1" train_in_peaceful="Y" train_in_house="true" obligatory="1")");
	ASSERT_TRUE(skill);
	EXPECT_TRUE(skill->hidden);
	EXPECT_TRUE(skill->train_in_peaceful);
	EXPECT_TRUE(skill->train_in_house);
	EXPECT_TRUE(skill->obligatory);
}

TEST(SkillFlags, ParsedSeparately) {
	const auto skill = ParseSkill(R"(hidden="1" obligatory="0")");
	ASSERT_TRUE(skill);
	EXPECT_TRUE(skill->hidden);
	EXPECT_FALSE(skill->train_in_peaceful);
	EXPECT_FALSE(skill->train_in_house);
	EXPECT_FALSE(skill->obligatory);
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

// The shipped config: piety (kReligion) is hidden, grows in temples (peaceful rooms) and is obligatory,
// so every class must list it -- otherwise it is not saved for that class's characters.
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
		EXPECT_TRUE(skill->hidden);
		EXPECT_TRUE(skill->train_in_peaceful);
		EXPECT_TRUE(skill->obligatory);
		found = true;
	}
	EXPECT_TRUE(found);
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
