/**
\file spell_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue #3304).
\brief In-game message container for spells: loader + ESpellMsg name registration.
*/

#include "spell_messages.h"

#include "engine/db/global_objects.h"

#include <map>

template<>
const std::string &NAME_BY_ITEM<ESpellMsg>(const ESpellMsg item) {
	static const std::map<ESpellMsg, std::string> kMap{
		{ESpellMsg::kUndefined, "kUndefined"},
		{ESpellMsg::kPointsToVict, "kPointsToVict"},
		{ESpellMsg::kAreaToChar, "kAreaToChar"},
		{ESpellMsg::kAreaToRoom, "kAreaToRoom"},
		{ESpellMsg::kAreaToVict, "kAreaToVict"},
		{ESpellMsg::kCantCastSleeping, "kCantCastSleeping"},
		{ESpellMsg::kCantCastResting, "kCantCastResting"},
		{ESpellMsg::kCantCastSitting, "kCantCastSitting"},
		{ESpellMsg::kCantCastFighting, "kCantCastFighting"},
		{ESpellMsg::kCantCastPosition, "kCantCastPosition"},
		{ESpellMsg::kCantCastMaster, "kCantCastMaster"},
		{ESpellMsg::kCantCastSelfOnly, "kCantCastSelfOnly"},
		{ESpellMsg::kCantCastNotSelf, "kCantCastNotSelf"},
		{ESpellMsg::kCantCastNotAlly, "kCantCastNotAlly"},
		{ESpellMsg::kCantCastNotMinion, "kCantCastNotMinion"},
		{ESpellMsg::kNoeffect, "kNoeffect"},
		{ESpellMsg::kTargetUnavailable, "kTargetUnavailable"},
		{ESpellMsg::kCantCastPeaceful, "kCantCastPeaceful"},
		{ESpellMsg::kObjResist, "kObjResist"},
		{ESpellMsg::kAlterObjToChar, "kAlterObjToChar"},
		{ESpellMsg::kEnchantNotWeapon, "kEnchantNotWeapon"},
		{ESpellMsg::kEnchantMagic, "kEnchantMagic"},
		{ESpellMsg::kEnchantSetItem, "kEnchantSetItem"},
		{ESpellMsg::kEnchantMono, "kEnchantMono"},
		{ESpellMsg::kEnchantPoly, "kEnchantPoly"},
		{ESpellMsg::kEnchantOther, "kEnchantOther"},
		{ESpellMsg::kRemovePoisonUnknown, "kRemovePoisonUnknown"},
		{ESpellMsg::kRemovePoisonRotten, "kRemovePoisonRotten"},
		{ESpellMsg::kSummonToRoom, "kSummonToRoom"},
		{ESpellMsg::kSummonFail, "kSummonFail"},
		{ESpellMsg::kSummonNoCorpse, "kSummonNoCorpse"},
		{ESpellMsg::kSummonCharmed, "kSummonCharmed"},
		{ESpellMsg::kSummonNoProto, "kSummonNoProto"},
		{ESpellMsg::kSummonWarhorse, "kSummonWarhorse"},
		{ESpellMsg::kResurrectBadCorpse, "kResurrectBadCorpse"},
		{ESpellMsg::kResurrectConsecrated, "kResurrectConsecrated"},
		{ESpellMsg::kResurrectNoPower, "kResurrectNoPower"},
		{ESpellMsg::kResurrectProtected, "kResurrectProtected"},
		{ESpellMsg::kLaggedToRoom, "kLaggedToRoom"},
		{ESpellMsg::kLaggedToChar, "kLaggedToChar"},
		{ESpellMsg::kKnockdownToRoom, "kKnockdownToRoom"},
		{ESpellMsg::kKnockdownToChar, "kKnockdownToChar"},
		{ESpellMsg::kAcidCorrodeObj, "kAcidCorrodeObj"},
		{ESpellMsg::kCastPhraseHeathen, "kCastPhraseHeathen"},
		{ESpellMsg::kCastPhraseChristian, "kCastPhraseChristian"},
		{ESpellMsg::kFightDeathToChar, "kFightDeathToChar"},
		{ESpellMsg::kFightDeathToVict, "kFightDeathToVict"},
		{ESpellMsg::kFightDeathToRoom, "kFightDeathToRoom"},
		{ESpellMsg::kFightMissToChar, "kFightMissToChar"},
		{ESpellMsg::kFightMissToVict, "kFightMissToVict"},
		{ESpellMsg::kFightMissToRoom, "kFightMissToRoom"},
		{ESpellMsg::kFightHitToChar, "kFightHitToChar"},
		{ESpellMsg::kFightHitToVict, "kFightHitToVict"},
		{ESpellMsg::kFightHitToRoom, "kFightHitToRoom"},
		{ESpellMsg::kFightGodToChar, "kFightGodToChar"},
		{ESpellMsg::kFightGodToVict, "kFightGodToVict"},
		{ESpellMsg::kFightGodToRoom, "kFightGodToRoom"},
		{ESpellMsg::kAffImposedToRoom, "kAffImposedToRoom"},
		{ESpellMsg::kAffImposedToChar, "kAffImposedToChar"},
		{ESpellMsg::kAffDispelledToRoom, "kAffDispelledToRoom"},
		{ESpellMsg::kAffDispelledToChar, "kAffDispelledToChar"},
	};
	return kMap.at(item);
}

template<>
ESpellMsg ITEM_BY_NAME<ESpellMsg>(const std::string &name) {
	static const std::map<std::string, ESpellMsg> kMap{
		{"kUndefined", ESpellMsg::kUndefined},
		{"kPointsToVict", ESpellMsg::kPointsToVict},
		{"kAreaToChar", ESpellMsg::kAreaToChar},
		{"kAreaToRoom", ESpellMsg::kAreaToRoom},
		{"kAreaToVict", ESpellMsg::kAreaToVict},
		{"kCantCastSleeping", ESpellMsg::kCantCastSleeping},
		{"kCantCastResting", ESpellMsg::kCantCastResting},
		{"kCantCastSitting", ESpellMsg::kCantCastSitting},
		{"kCantCastFighting", ESpellMsg::kCantCastFighting},
		{"kCantCastPosition", ESpellMsg::kCantCastPosition},
		{"kCantCastMaster", ESpellMsg::kCantCastMaster},
		{"kCantCastSelfOnly", ESpellMsg::kCantCastSelfOnly},
		{"kCantCastNotSelf", ESpellMsg::kCantCastNotSelf},
		{"kCantCastNotAlly", ESpellMsg::kCantCastNotAlly},
		{"kCantCastNotMinion", ESpellMsg::kCantCastNotMinion},
		{"kNoeffect", ESpellMsg::kNoeffect},
		{"kTargetUnavailable", ESpellMsg::kTargetUnavailable},
		{"kCantCastPeaceful", ESpellMsg::kCantCastPeaceful},
		{"kObjResist", ESpellMsg::kObjResist},
		{"kAlterObjToChar", ESpellMsg::kAlterObjToChar},
		{"kEnchantNotWeapon", ESpellMsg::kEnchantNotWeapon},
		{"kEnchantMagic", ESpellMsg::kEnchantMagic},
		{"kEnchantSetItem", ESpellMsg::kEnchantSetItem},
		{"kEnchantMono", ESpellMsg::kEnchantMono},
		{"kEnchantPoly", ESpellMsg::kEnchantPoly},
		{"kEnchantOther", ESpellMsg::kEnchantOther},
		{"kRemovePoisonUnknown", ESpellMsg::kRemovePoisonUnknown},
		{"kRemovePoisonRotten", ESpellMsg::kRemovePoisonRotten},
		{"kSummonToRoom", ESpellMsg::kSummonToRoom},
		{"kSummonFail", ESpellMsg::kSummonFail},
		{"kSummonNoCorpse", ESpellMsg::kSummonNoCorpse},
		{"kSummonCharmed", ESpellMsg::kSummonCharmed},
		{"kSummonNoProto", ESpellMsg::kSummonNoProto},
		{"kSummonWarhorse", ESpellMsg::kSummonWarhorse},
		{"kResurrectBadCorpse", ESpellMsg::kResurrectBadCorpse},
		{"kResurrectConsecrated", ESpellMsg::kResurrectConsecrated},
		{"kResurrectNoPower", ESpellMsg::kResurrectNoPower},
		{"kResurrectProtected", ESpellMsg::kResurrectProtected},
		{"kLaggedToRoom", ESpellMsg::kLaggedToRoom},
		{"kLaggedToChar", ESpellMsg::kLaggedToChar},
		{"kKnockdownToRoom", ESpellMsg::kKnockdownToRoom},
		{"kKnockdownToChar", ESpellMsg::kKnockdownToChar},
		{"kAcidCorrodeObj", ESpellMsg::kAcidCorrodeObj},
		{"kCastPhraseHeathen", ESpellMsg::kCastPhraseHeathen},
		{"kCastPhraseChristian", ESpellMsg::kCastPhraseChristian},
		{"kFightDeathToChar", ESpellMsg::kFightDeathToChar},
		{"kFightDeathToVict", ESpellMsg::kFightDeathToVict},
		{"kFightDeathToRoom", ESpellMsg::kFightDeathToRoom},
		{"kFightMissToChar", ESpellMsg::kFightMissToChar},
		{"kFightMissToVict", ESpellMsg::kFightMissToVict},
		{"kFightMissToRoom", ESpellMsg::kFightMissToRoom},
		{"kFightHitToChar", ESpellMsg::kFightHitToChar},
		{"kFightHitToVict", ESpellMsg::kFightHitToVict},
		{"kFightHitToRoom", ESpellMsg::kFightHitToRoom},
		{"kFightGodToChar", ESpellMsg::kFightGodToChar},
		{"kFightGodToVict", ESpellMsg::kFightGodToVict},
		{"kFightGodToRoom", ESpellMsg::kFightGodToRoom},
		{"kAffImposedToRoom", ESpellMsg::kAffImposedToRoom},
		{"kAffImposedToChar", ESpellMsg::kAffImposedToChar},
		{"kAffDispelledToRoom", ESpellMsg::kAffDispelledToRoom},
		{"kAffDispelledToChar", ESpellMsg::kAffDispelledToChar},
	};
	return kMap.at(name); // throws std::out_of_range on unknown name
}

namespace spells {

void SpellMessagesLoader::Load(parser_wrapper::DataNode data) {
	MUD::SpellMessages().Init(data.Children());
}

void SpellMessagesLoader::Reload(parser_wrapper::DataNode data) {
	MUD::SpellMessages().Reload(data.Children());
}

} // namespace spells

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
