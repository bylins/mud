/**
\file spell_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue #3304).
\brief In-game message container for spells: loader + ESpellMsg name registration.
*/

#include "spell_messages.h"
#include "utils/utils_string.h"   // str_cmp

#include "engine/db/global_objects.h"

#include <map>
#include <string>
#include <vector>

namespace {
// issue.vedun-msg-editor: file-scope so NAMES_OF can expose it for the editor pick-list.
const std::map<ESpellMsg, std::string> kESpellMsgNames{
		{ESpellMsg::kUndefined, "kUndefined"},
		{ESpellMsg::kHealToVict, "kHealToVict"},
		{ESpellMsg::kMovesToVict, "kMovesToVict"},
		{ESpellMsg::kThirstToVict, "kThirstToVict"},
		{ESpellMsg::kFullToVict, "kFullToVict"},
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
		{ESpellMsg::kReflectedToChar, "kReflectedToChar"},
		{ESpellMsg::kReflectedToVict, "kReflectedToVict"},
		{ESpellMsg::kReflectedToRoom, "kReflectedToRoom"},
		{ESpellMsg::kAffExpiredToChar, "kAffExpiredToChar"},
		{ESpellMsg::kAffExpiredToRoom, "kAffExpiredToRoom"},
		{ESpellMsg::kCastForbiddenToChar, "kCastForbiddenToChar"},
		{ESpellMsg::kCastForbiddenToRoom, "kCastForbiddenToRoom"},
		{ESpellMsg::kRoomAffectVisible, "kRoomAffectVisible"},
		{ESpellMsg::kRoomAffectInvisible, "kRoomAffectInvisible"},
		{ESpellMsg::kRoomAffectSelfInvisible, "kRoomAffectSelfInvisible"},
		{ESpellMsg::kRoomAffectPkVisible, "kRoomAffectPkVisible"},
		{ESpellMsg::kRoomAffectPkInvisible, "kRoomAffectPkInvisible"},
		{ESpellMsg::kComponentUse, "kComponentUse"},
		{ESpellMsg::kComponentMissing, "kComponentMissing"},
		{ESpellMsg::kComponentExhausted, "kComponentExhausted"},
		{ESpellMsg::kCantCastSilenced, "kCantCastSilenced"},
		{ESpellMsg::kNoTarget, "kNoTarget"},
		{ESpellMsg::kCantCastHereToChar, "kCantCastHereToChar"},
		{ESpellMsg::kCantCastHereToRoom, "kCantCastHereToRoom"},
		{ESpellMsg::kCastIncantToChar, "kCastIncantToChar"},
		{ESpellMsg::kItemNoPrototype, "kItemNoPrototype"},
		{ESpellMsg::kItemCreatedToChar, "kItemCreatedToChar"},
		{ESpellMsg::kItemCreatedToRoom, "kItemCreatedToRoom"},
		{ESpellMsg::kCastSayToSelf, "kCastSayToSelf"},
		{ESpellMsg::kCastSayToOther, "kCastSayToOther"},
		{ESpellMsg::kCastSayToObj, "kCastSayToObj"},
		{ESpellMsg::kCastSayToSomething, "kCastSayToSomething"},
		{ESpellMsg::kCastSayDamageeToVict, "kCastSayDamageeToVict"},
		{ESpellMsg::kCastSayHelpeeToVict, "kCastSayHelpeeToVict"},
		{ESpellMsg::kCastSaySound, "kCastSaySound"},
		{ESpellMsg::kCastInterruptedToChar, "kCastInterruptedToChar"},
		{ESpellMsg::kCastPreparedToChar, "kCastPreparedToChar"},
		{ESpellMsg::kAfDispelledToOwner, "kAfDispelledToOwner"},
		{ESpellMsg::kItemCreationFailToChar, "kItemCreationFailToChar"},
		{ESpellMsg::kWrongTarget, "kWrongTarget"},
		{ESpellMsg::kCustomMsgOne, "kCustomMsgOne"},
		{ESpellMsg::kCustomMsgTwo, "kCustomMsgTwo"},
		{ESpellMsg::kCustomMsgThree, "kCustomMsgThree"},
		{ESpellMsg::kCustomMsgFour, "kCustomMsgFour"},
		{ESpellMsg::kCustomMsgFive, "kCustomMsgFive"},
		{ESpellMsg::kCustomMsgSix, "kCustomMsgSix"},
		{ESpellMsg::kCustomMsgSeven, "kCustomMsgSeven"},
		{ESpellMsg::kCustomMsgEight, "kCustomMsgEight"},
		{ESpellMsg::kCustomMsgNine, "kCustomMsgNine"},
		{ESpellMsg::kCustomMsgTen, "kCustomMsgTen"},
		{ESpellMsg::kCastDisappearToRoom, "kCastDisappearToRoom"},
		{ESpellMsg::kCastAppearToRoom, "kCastAppearToRoom"},
};
}  // namespace

template<>
const std::string &NAME_BY_ITEM<ESpellMsg>(const ESpellMsg item) {
	return kESpellMsgNames.at(item);
}

template<>
const std::map<ESpellMsg, std::string> &NAMES_OF<ESpellMsg>() {
	return kESpellMsgNames;
}

template<>
ESpellMsg ITEM_BY_NAME<ESpellMsg>(const std::string &name) {
	static const std::map<std::string, ESpellMsg> kMap{
		{"kUndefined", ESpellMsg::kUndefined},
		{"kHealToVict", ESpellMsg::kHealToVict},
		{"kMovesToVict", ESpellMsg::kMovesToVict},
		{"kThirstToVict", ESpellMsg::kThirstToVict},
		{"kFullToVict", ESpellMsg::kFullToVict},
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
		{"kReflectedToChar", ESpellMsg::kReflectedToChar},
		{"kReflectedToVict", ESpellMsg::kReflectedToVict},
		{"kReflectedToRoom", ESpellMsg::kReflectedToRoom},
		{"kAffExpiredToChar", ESpellMsg::kAffExpiredToChar},
		{"kAffExpiredToRoom", ESpellMsg::kAffExpiredToRoom},
		{"kCastForbiddenToChar", ESpellMsg::kCastForbiddenToChar},
		{"kCastForbiddenToRoom", ESpellMsg::kCastForbiddenToRoom},
		{"kRoomAffectVisible", ESpellMsg::kRoomAffectVisible},
		{"kRoomAffectInvisible", ESpellMsg::kRoomAffectInvisible},
		{"kRoomAffectSelfInvisible", ESpellMsg::kRoomAffectSelfInvisible},
		{"kRoomAffectPkVisible", ESpellMsg::kRoomAffectPkVisible},
		{"kRoomAffectPkInvisible", ESpellMsg::kRoomAffectPkInvisible},
		{"kComponentUse", ESpellMsg::kComponentUse},
		{"kComponentMissing", ESpellMsg::kComponentMissing},
		{"kComponentExhausted", ESpellMsg::kComponentExhausted},
		{"kCantCastSilenced", ESpellMsg::kCantCastSilenced},
		{"kNoTarget", ESpellMsg::kNoTarget},
		{"kCantCastHereToChar", ESpellMsg::kCantCastHereToChar},
		{"kCantCastHereToRoom", ESpellMsg::kCantCastHereToRoom},
		{"kCastIncantToChar", ESpellMsg::kCastIncantToChar},
		{"kItemNoPrototype", ESpellMsg::kItemNoPrototype},
		{"kItemCreatedToChar", ESpellMsg::kItemCreatedToChar},
		{"kItemCreatedToRoom", ESpellMsg::kItemCreatedToRoom},
		{"kCastSayToSelf", ESpellMsg::kCastSayToSelf},
		{"kCastSayToOther", ESpellMsg::kCastSayToOther},
		{"kCastSayToObj", ESpellMsg::kCastSayToObj},
		{"kCastSayToSomething", ESpellMsg::kCastSayToSomething},
		{"kCastSayDamageeToVict", ESpellMsg::kCastSayDamageeToVict},
		{"kCastSayHelpeeToVict", ESpellMsg::kCastSayHelpeeToVict},
		{"kCastSaySound", ESpellMsg::kCastSaySound},
		{"kCastInterruptedToChar", ESpellMsg::kCastInterruptedToChar},
		{"kCastPreparedToChar", ESpellMsg::kCastPreparedToChar},
		{"kAfDispelledToOwner", ESpellMsg::kAfDispelledToOwner},
		{"kItemCreationFailToChar", ESpellMsg::kItemCreationFailToChar},
		{"kWrongTarget", ESpellMsg::kWrongTarget},
		{"kCustomMsgOne", ESpellMsg::kCustomMsgOne},
		{"kCustomMsgTwo", ESpellMsg::kCustomMsgTwo},
		{"kCustomMsgThree", ESpellMsg::kCustomMsgThree},
		{"kCustomMsgFour", ESpellMsg::kCustomMsgFour},
		{"kCustomMsgFive", ESpellMsg::kCustomMsgFive},
		{"kCustomMsgSix", ESpellMsg::kCustomMsgSix},
		{"kCustomMsgSeven", ESpellMsg::kCustomMsgSeven},
		{"kCustomMsgEight", ESpellMsg::kCustomMsgEight},
		{"kCustomMsgNine", ESpellMsg::kCustomMsgNine},
		{"kCustomMsgTen", ESpellMsg::kCustomMsgTen},
		{"kCastDisappearToRoom", ESpellMsg::kCastDisappearToRoom},
		{"kCastAppearToRoom", ESpellMsg::kCastAppearToRoom},
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

// issue.vedun-msg-editor: editor discovery + safe-commit validation (mirrors SpellsLoader).
std::string SpellMessagesLoader::EditableWhat() const {
	return "spellmsg";
}

std::vector<cfg_manager::EditableElement> SpellMessagesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &sheaf : MUD::SpellMessages()) {
		const ESpell id = sheaf.GetId();
		// The default sheaf is stored under kUndefined but its XML id is "kDefault".
		const std::string id_str = (id == ESpell::kUndefined) ? "kDefault" : NAME_BY_ITEM<ESpell>(id);
		std::string label;
		if (id != ESpell::kUndefined && MUD::Spells().IsKnown(id)) {
			label = MUD::Spells()[id].GetName();
		}
		out.push_back({id_str, label});
	}
	return out;
}

cfg_manager::ValidationResult SpellMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::SpellMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Spell-message data failed to parse (see syslog for the offending sheaf/message)."};
}

std::string SpellMessagesLoader::CanonicalElementId(const std::string &id) const {
	for (const auto &[value, name] : NAMES_OF<ESpell>()) {
		if (value != ESpell::kUndefined && str_cmp(name, id) == 0) {
			return name;
		}
	}
	return "";
}

parser_wrapper::DataNode SpellMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	// An empty sheaf for `id`; the editor then adds <message> children.
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}

} // namespace spells

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
