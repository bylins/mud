// Part of Bylins http://www.mud.ru
// Unit tests for the fight (hit type) message registration added in issue #3311:
//   * the "kDefault" -> EAttackType::kUndefined alias used by the default sheaf id;
//   * the EAttackType <-> name and EFightMsg <-> name mappings consumed by MsgSheafBuilder.
// The MsgContainer/MsgSheaf parsing logic itself is covered by msg_container.cpp.

#include "gameplay/fight/fight_messages.h"
#include "gameplay/fight/fight_constants.h"
#include "utils/parse.h"

#include <gtest/gtest.h>
#include <stdexcept>

// The default sheaf in hit_msg.xml uses id="kDefault"; MsgSheafBuilder resolves it
// via ReadAsConstant<EAttackType>, so "kDefault" must map to EAttackType::kUndefined.
TEST(FightMessages, DefaultSheafIdAliasesToUndefined) {
	EXPECT_EQ(fight::EAttackType::kUndefined, ITEM_BY_NAME<fight::EAttackType>("kDefault"));
	EXPECT_EQ(fight::EAttackType::kUndefined, parse::ReadAsConstant<fight::EAttackType>("kDefault"));
}

// Adding the alias must not change kUndefined's canonical (reverse) name.
TEST(FightMessages, UndefinedKeepsCanonicalName) {
	EXPECT_EQ("kUndefined", NAME_BY_ITEM<fight::EAttackType>(fight::EAttackType::kUndefined));
}

TEST(FightMessages, EAttackTypeNameRoundTrip) {
	for (const auto type : {fight::EAttackType::kHit, fight::EAttackType::kSkin,
							fight::EAttackType::kWhip, fight::EAttackType::kSlash,
							fight::EAttackType::kBite, fight::EAttackType::kBludgeon,
							fight::EAttackType::kCrush, fight::EAttackType::kPound,
							fight::EAttackType::kClaw, fight::EAttackType::kMaul,
							fight::EAttackType::kThrash, fight::EAttackType::kPierce,
							fight::EAttackType::kBlast, fight::EAttackType::kPunch,
							fight::EAttackType::kStab, fight::EAttackType::kPick,
							fight::EAttackType::kSting}) {
		EXPECT_EQ(type, ITEM_BY_NAME<fight::EAttackType>(NAME_BY_ITEM<fight::EAttackType>(type)));
	}
}

TEST(FightMessages, EFightMsgNameRoundTrip) {
	EXPECT_EQ(fight::EFightMsg::kFightHitToChar, parse::ReadAsConstant<fight::EFightMsg>("kFightHitToChar"));
	for (const auto type : {fight::EFightMsg::kFightDeathToChar, fight::EFightMsg::kFightDeathToVict,
							fight::EFightMsg::kFightDeathToRoom, fight::EFightMsg::kFightMissToChar,
							fight::EFightMsg::kFightMissToVict, fight::EFightMsg::kFightMissToRoom,
							fight::EFightMsg::kFightHitToChar, fight::EFightMsg::kFightHitToVict,
							fight::EFightMsg::kFightHitToRoom, fight::EFightMsg::kFightGodToChar,
							fight::EFightMsg::kFightGodToVict, fight::EFightMsg::kFightGodToRoom}) {
		EXPECT_EQ(type, ITEM_BY_NAME<fight::EFightMsg>(NAME_BY_ITEM<fight::EFightMsg>(type)));
	}
}

TEST(FightMessages, UnknownNamesThrow) {
	EXPECT_THROW(ITEM_BY_NAME<fight::EAttackType>("kNopeNotAType"), std::out_of_range);
	EXPECT_THROW(ITEM_BY_NAME<fight::EFightMsg>("kNopeNotAMsg"), std::out_of_range);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
