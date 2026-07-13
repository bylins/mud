// Part of Bylins http://www.mud.ru
// Unit tests for the spell message registration added in issue #3304:
//   * the "kDefault" -> ESpell::kUndefined alias used by the default sheaf id;
//   * the ESpellMsg <-> name mapping consumed by MsgSheafBuilder.
// The MsgContainer/MsgSheaf parsing logic itself is covered by msg_container.cpp.

#include "gameplay/magic/spell_messages.h"
#include "gameplay/magic/spells_constants.h"
#include "utils/utils_parse.h"

#include <gtest/gtest.h>
#include <stdexcept>

// The default sheaf in spell_msg.xml uses id="kDefault"; MsgSheafBuilder resolves
// it via ReadAsConstant<ESpell>, so "kDefault" must map to ESpell::kUndefined.
TEST(SpellMessages, DefaultSheafIdAliasesToUndefined) {
	EXPECT_EQ(ESpell::kUndefined, ITEM_BY_NAME<ESpell>("kDefault"));
	EXPECT_EQ(ESpell::kUndefined, parse::ReadAsConstant<ESpell>("kDefault"));
}

// Adding the alias must not change kUndefined's canonical (reverse) name.
TEST(SpellMessages, UndefinedKeepsCanonicalName) {
	EXPECT_EQ("kUndefined", NAME_BY_ITEM<ESpell>(ESpell::kUndefined));
}

TEST(SpellMessages, ESpellMsgNameRoundTrip) {
	// issue.point-bugs #2: kPointsToVict replaced by per-category keys.
	EXPECT_EQ(ESpellMsg::kHealToVict, ITEM_BY_NAME<ESpellMsg>("kHealToVict"));
	EXPECT_EQ(ESpellMsg::kHealToVict, parse::ReadAsConstant<ESpellMsg>("kHealToVict"));
	EXPECT_EQ("kHealToVict", NAME_BY_ITEM<ESpellMsg>(ESpellMsg::kHealToVict));

	for (const auto type : {ESpellMsg::kAreaToChar, ESpellMsg::kAreaToRoom, ESpellMsg::kAreaToVict,
							ESpellMsg::kCantCastSleeping, ESpellMsg::kCantCastResting,
							ESpellMsg::kCantCastSitting, ESpellMsg::kCantCastFighting,
							ESpellMsg::kCantCastPosition, ESpellMsg::kCantCastMaster,
							ESpellMsg::kCantCastSelfOnly, ESpellMsg::kCantCastNotSelf,
							ESpellMsg::kTargetUnavailable, ESpellMsg::kCantCastPeaceful,
							ESpellMsg::kObjResist, ESpellMsg::kAlterObjToChar,
							ESpellMsg::kEnchantNotWeapon, ESpellMsg::kEnchantMagic,
							ESpellMsg::kEnchantSetItem, ESpellMsg::kEnchantMono,
							ESpellMsg::kEnchantPoly, ESpellMsg::kEnchantOther,
							ESpellMsg::kRemovePoisonUnknown, ESpellMsg::kRemovePoisonRotten,
							ESpellMsg::kSummonToRoom, ESpellMsg::kSummonFail,
							ESpellMsg::kSummonNoCorpse, ESpellMsg::kSummonCharmed,
							ESpellMsg::kSummonNoProto, ESpellMsg::kSummonWarhorse,
							ESpellMsg::kResurrectBadCorpse, ESpellMsg::kResurrectConsecrated,
							ESpellMsg::kResurrectNoPower, ESpellMsg::kResurrectProtected,
							ESpellMsg::kLaggedToRoom, ESpellMsg::kLaggedToChar,
							ESpellMsg::kKnockdownToRoom, ESpellMsg::kKnockdownToChar,
							ESpellMsg::kAcidCorrodeObj,
							ESpellMsg::kCastPhraseHeathen, ESpellMsg::kCastPhraseChristian,
							ESpellMsg::kFightDeathToChar, ESpellMsg::kFightDeathToVict,
							ESpellMsg::kFightDeathToRoom, ESpellMsg::kFightMissToChar,
							ESpellMsg::kFightMissToVict, ESpellMsg::kFightMissToRoom,
							ESpellMsg::kFightHitToChar, ESpellMsg::kFightHitToVict,
							ESpellMsg::kFightHitToRoom, ESpellMsg::kFightGodToChar,
							ESpellMsg::kFightGodToVict, ESpellMsg::kFightGodToRoom,
							ESpellMsg::kReflectedToChar, ESpellMsg::kReflectedToVict,
							ESpellMsg::kReflectedToRoom, ESpellMsg::kAffExpiredToChar,
							ESpellMsg::kAffExpiredToRoom,
							ESpellMsg::kCastForbiddenToChar, ESpellMsg::kCastForbiddenToRoom,
							ESpellMsg::kRoomAffectVisible, ESpellMsg::kRoomAffectInvisible,
							ESpellMsg::kRoomAffectSelfInvisible,
							ESpellMsg::kRoomAffectPkVisible, ESpellMsg::kRoomAffectPkInvisible,
							ESpellMsg::kComponentUse, ESpellMsg::kComponentMissing,
							ESpellMsg::kComponentExhausted,
							ESpellMsg::kCantCastSilenced}) {
		EXPECT_EQ(type, ITEM_BY_NAME<ESpellMsg>(NAME_BY_ITEM<ESpellMsg>(type)));
	}
}

TEST(SpellMessages, UnknownESpellMsgNameThrows) {
	EXPECT_THROW(ITEM_BY_NAME<ESpellMsg>("kNopeNotARealType"), std::out_of_range);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
