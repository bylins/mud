// Part of Bylins http://www.mud.ru
// Unit tests for the fight (hit type) message registration added in issue #3311:
//   * the "kDefault" -> EDamageSource::kUndefined alias used by the default sheaf id;
//   * the EDamageSource <-> name and EFightMsg <-> name mappings consumed by MsgSheafBuilder.
// The MsgContainer/MsgSheaf parsing logic itself is covered by msg_container.cpp.

#include "gameplay/fight/fight_messages.h"
#include "gameplay/fight/fight_constants.h"
#include "utils/parse.h"

#include <gtest/gtest.h>
#include <stdexcept>

// The default sheaf in hit_msg.xml uses id="kDefault"; MsgSheafBuilder resolves it
// via ReadAsConstant<EDamageSource>, so "kDefault" must map to EDamageSource::kUndefined.
TEST(FightMessages, DefaultSheafIdAliasesToUndefined) {
	EXPECT_EQ(fight::EDamageSource::kUndefined, ITEM_BY_NAME<fight::EDamageSource>("kDefault"));
	EXPECT_EQ(fight::EDamageSource::kUndefined, parse::ReadAsConstant<fight::EDamageSource>("kDefault"));
}

// Adding the alias must not change kUndefined's canonical (reverse) name.
TEST(FightMessages, UndefinedKeepsCanonicalName) {
	EXPECT_EQ("kUndefined", NAME_BY_ITEM<fight::EDamageSource>(fight::EDamageSource::kUndefined));
}

TEST(FightMessages, EDamageSourceNameRoundTrip) {
	for (const auto type : {fight::EDamageSource::kHit, fight::EDamageSource::kSkin,
							fight::EDamageSource::kWhip, fight::EDamageSource::kSlash,
							fight::EDamageSource::kBite, fight::EDamageSource::kBludgeon,
							fight::EDamageSource::kCrush, fight::EDamageSource::kPound,
							fight::EDamageSource::kClaw, fight::EDamageSource::kMaul,
							fight::EDamageSource::kThrash, fight::EDamageSource::kPierce,
							fight::EDamageSource::kBlast, fight::EDamageSource::kPunch,
							fight::EDamageSource::kStab, fight::EDamageSource::kPick,
							fight::EDamageSource::kSting,
							fight::EDamageSource::kTriggerDeath, fight::EDamageSource::kTunnelDeath,
							fight::EDamageSource::kUnderwaterDeathTrap, fight::EDamageSource::kSlowDeathTrap,
							fight::EDamageSource::kSuffering}) {
		EXPECT_EQ(type, ITEM_BY_NAME<fight::EDamageSource>(NAME_BY_ITEM<fight::EDamageSource>(type)));
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
	EXPECT_THROW(ITEM_BY_NAME<fight::EDamageSource>("kNopeNotAType"), std::out_of_range);
	EXPECT_THROW(ITEM_BY_NAME<fight::EFightMsg>("kNopeNotAMsg"), std::out_of_range);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
