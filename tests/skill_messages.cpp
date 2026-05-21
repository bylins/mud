// Part of Bylins http://www.mud.ru
// Unit tests for the skill message registration added in issue #3310:
//   * the "kDefault" -> ESkill::kUndefined alias used by the default sheaf id;
//   * the ESkillMsg <-> name mapping consumed by MsgSheafBuilder.
// The MsgContainer/MsgSheaf parsing logic itself is covered by msg_container.cpp.

#include "gameplay/skills/skill_messages.h"
#include "gameplay/skills/skills.h"
#include "utils/parse.h"

#include <gtest/gtest.h>
#include <stdexcept>

// The default sheaf in skill_msg.xml uses id="kDefault"; MsgSheafBuilder resolves
// it via ReadAsConstant<ESkill>, so "kDefault" must map to ESkill::kUndefined.
TEST(SkillMessages, DefaultSheafIdAliasesToUndefined) {
	EXPECT_EQ(ESkill::kUndefined, ITEM_BY_NAME<ESkill>("kDefault"));
	EXPECT_EQ(ESkill::kUndefined, parse::ReadAsConstant<ESkill>("kDefault"));
}

// Adding the alias must not change kUndefined's canonical (reverse) name.
TEST(SkillMessages, UndefinedKeepsCanonicalName) {
	EXPECT_EQ("kUndefined", NAME_BY_ITEM<ESkill>(ESkill::kUndefined));
}

TEST(SkillMessages, ESkillMsgNameRoundTrip) {
	EXPECT_EQ(ESkillMsg::kDontKnowSkill, ITEM_BY_NAME<ESkillMsg>("kDontKnowSkill"));
	EXPECT_EQ(ESkillMsg::kDontKnowSkill, parse::ReadAsConstant<ESkillMsg>("kDontKnowSkill"));
	EXPECT_EQ("kDontKnowSkill", NAME_BY_ITEM<ESkillMsg>(ESkillMsg::kDontKnowSkill));

	for (const auto type : {ESkillMsg::kDontKnowSkill, ESkillMsg::kOnCooldown,
							ESkillMsg::kCantWhileMounted, ESkillMsg::kMustBeMounted,
							ESkillMsg::kGetOnFeet, ESkillMsg::kCantFightNow,
							ESkillMsg::kPeacefulRoom, ESkillMsg::kNoTarget,
							ESkillMsg::kCantTargetSelf, ESkillMsg::kNeedWeapon,
							ESkillMsg::kWrongWeapon}) {
		EXPECT_EQ(type, ITEM_BY_NAME<ESkillMsg>(NAME_BY_ITEM<ESkillMsg>(type)));
	}
}

TEST(SkillMessages, UnknownESkillMsgNameThrows) {
	EXPECT_THROW(ITEM_BY_NAME<ESkillMsg>("kNopeNotARealType"), std::out_of_range);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
