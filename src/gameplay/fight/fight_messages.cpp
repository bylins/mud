/**
\file fight_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue #3311).
\brief Weapon attack (hit) type message container: loader + name registration.
*/

#include "fight_messages.h"

#include "engine/db/global_objects.h"

#include <map>
#include <vector>

namespace {
// issue.vedun-msg-editor: file-scope so NAMES_OF can expose it to the editor.
const std::map<fight::EDamageSource, std::string> kEDamageSourceNames{
		{fight::EDamageSource::kUndefined, "kUndefined"},
		{fight::EDamageSource::kHit, "kHit"},
		{fight::EDamageSource::kSkin, "kSkin"},
		{fight::EDamageSource::kWhip, "kWhip"},
		{fight::EDamageSource::kSlash, "kSlash"},
		{fight::EDamageSource::kBite, "kBite"},
		{fight::EDamageSource::kBludgeon, "kBludgeon"},
		{fight::EDamageSource::kCrush, "kCrush"},
		{fight::EDamageSource::kPound, "kPound"},
		{fight::EDamageSource::kClaw, "kClaw"},
		{fight::EDamageSource::kMaul, "kMaul"},
		{fight::EDamageSource::kThrash, "kThrash"},
		{fight::EDamageSource::kPierce, "kPierce"},
		{fight::EDamageSource::kBlast, "kBlast"},
		{fight::EDamageSource::kPunch, "kPunch"},
		{fight::EDamageSource::kStab, "kStab"},
		{fight::EDamageSource::kPick, "kPick"},
		{fight::EDamageSource::kSting, "kSting"},
		{fight::EDamageSource::kFireBreath, "kFireBreath"},
		{fight::EDamageSource::kGasBreath, "kGasBreath"},
		{fight::EDamageSource::kFrostBreath, "kFrostBreath"},
		{fight::EDamageSource::kAcidBreath, "kAcidBreath"},
		{fight::EDamageSource::kLightingBreath, "kLightingBreath"},
		{fight::EDamageSource::kTriggerDeath, "kTriggerDeath"},
		{fight::EDamageSource::kTunnelDeath, "kTunnelDeath"},
		{fight::EDamageSource::kUnderwaterDeathTrap, "kUnderwaterDeathTrap"},
		{fight::EDamageSource::kSlowDeathTrap, "kSlowDeathTrap"},
		{fight::EDamageSource::kSuffering, "kSuffering"},
	};
}  // namespace

template<>
const std::string &NAME_BY_ITEM<fight::EDamageSource>(const fight::EDamageSource item) {
	return kEDamageSourceNames.at(item);
}

template<>
const std::map<fight::EDamageSource, std::string> &NAMES_OF<fight::EDamageSource>() {
	return kEDamageSourceNames;
}

template<>
fight::EDamageSource ITEM_BY_NAME<fight::EDamageSource>(const std::string &name) {
	static const std::map<std::string, fight::EDamageSource> kMap{
		{"kUndefined", fight::EDamageSource::kUndefined},
		{"kDefault", fight::EDamageSource::kUndefined}, // default sheaf id in hit_msg.xml
		{"kHit", fight::EDamageSource::kHit},
		{"kSkin", fight::EDamageSource::kSkin},
		{"kWhip", fight::EDamageSource::kWhip},
		{"kSlash", fight::EDamageSource::kSlash},
		{"kBite", fight::EDamageSource::kBite},
		{"kBludgeon", fight::EDamageSource::kBludgeon},
		{"kCrush", fight::EDamageSource::kCrush},
		{"kPound", fight::EDamageSource::kPound},
		{"kClaw", fight::EDamageSource::kClaw},
		{"kMaul", fight::EDamageSource::kMaul},
		{"kThrash", fight::EDamageSource::kThrash},
		{"kPierce", fight::EDamageSource::kPierce},
		{"kBlast", fight::EDamageSource::kBlast},
		{"kPunch", fight::EDamageSource::kPunch},
		{"kStab", fight::EDamageSource::kStab},
		{"kPick", fight::EDamageSource::kPick},
		{"kSting", fight::EDamageSource::kSting},
		{"kFireBreath", fight::EDamageSource::kFireBreath},
		{"kGasBreath", fight::EDamageSource::kGasBreath},
		{"kFrostBreath", fight::EDamageSource::kFrostBreath},
		{"kAcidBreath", fight::EDamageSource::kAcidBreath},
		{"kLightingBreath", fight::EDamageSource::kLightingBreath},
		{"kTriggerDeath", fight::EDamageSource::kTriggerDeath},
		{"kTunnelDeath", fight::EDamageSource::kTunnelDeath},
		{"kUnderwaterDeathTrap", fight::EDamageSource::kUnderwaterDeathTrap},
		{"kSlowDeathTrap", fight::EDamageSource::kSlowDeathTrap},
		{"kSuffering", fight::EDamageSource::kSuffering},
	};
	return kMap.at(name); // throws std::out_of_range on unknown name
}

namespace {
// issue.vedun-msg-editor: file-scope so NAMES_OF can expose it to the editor.
const std::map<fight::EFightMsg, std::string> kEFightMsgNames{
		{fight::EFightMsg::kUndefined, "kUndefined"},
		{fight::EFightMsg::kFightDeathToChar, "kFightDeathToChar"},
		{fight::EFightMsg::kFightDeathToVict, "kFightDeathToVict"},
		{fight::EFightMsg::kFightDeathToRoom, "kFightDeathToRoom"},
		{fight::EFightMsg::kFightMissToChar, "kFightMissToChar"},
		{fight::EFightMsg::kFightMissToVict, "kFightMissToVict"},
		{fight::EFightMsg::kFightMissToRoom, "kFightMissToRoom"},
		{fight::EFightMsg::kFightHitToChar, "kFightHitToChar"},
		{fight::EFightMsg::kFightHitToVict, "kFightHitToVict"},
		{fight::EFightMsg::kFightHitToRoom, "kFightHitToRoom"},
		{fight::EFightMsg::kFightGodToChar, "kFightGodToChar"},
		{fight::EFightMsg::kFightGodToVict, "kFightGodToVict"},
		{fight::EFightMsg::kFightGodToRoom, "kFightGodToRoom"},
		{fight::EFightMsg::kDescription, "kDescription"},
	};
}  // namespace

template<>
const std::string &NAME_BY_ITEM<fight::EFightMsg>(const fight::EFightMsg item) {
	return kEFightMsgNames.at(item);
}

template<>
const std::map<fight::EFightMsg, std::string> &NAMES_OF<fight::EFightMsg>() {
	return kEFightMsgNames;
}

template<>
fight::EFightMsg ITEM_BY_NAME<fight::EFightMsg>(const std::string &name) {
	static const std::map<std::string, fight::EFightMsg> kMap{
		{"kUndefined", fight::EFightMsg::kUndefined},
		{"kFightDeathToChar", fight::EFightMsg::kFightDeathToChar},
		{"kFightDeathToVict", fight::EFightMsg::kFightDeathToVict},
		{"kFightDeathToRoom", fight::EFightMsg::kFightDeathToRoom},
		{"kFightMissToChar", fight::EFightMsg::kFightMissToChar},
		{"kFightMissToVict", fight::EFightMsg::kFightMissToVict},
		{"kFightMissToRoom", fight::EFightMsg::kFightMissToRoom},
		{"kFightHitToChar", fight::EFightMsg::kFightHitToChar},
		{"kFightHitToVict", fight::EFightMsg::kFightHitToVict},
		{"kFightHitToRoom", fight::EFightMsg::kFightHitToRoom},
		{"kFightGodToChar", fight::EFightMsg::kFightGodToChar},
		{"kFightGodToVict", fight::EFightMsg::kFightGodToVict},
		{"kFightGodToRoom", fight::EFightMsg::kFightGodToRoom},
		{"kDescription", fight::EFightMsg::kDescription},
	};
	return kMap.at(name); // throws std::out_of_range on unknown name
}

namespace fight {

const std::string &GetAttackTypeDescription(int attack_type) {
	// Weapon attack types occupy EDamageSource kHit..kSting; anything else uses the
	// kDefault sheaf description.
	const auto src = (attack_type >= static_cast<int>(EDamageSource::kHit)
					  && attack_type <= static_cast<int>(EDamageSource::kSting))
		? static_cast<EDamageSource>(attack_type)
		: EDamageSource::kUndefined;
	return MUD::FightMessages().GetMessage(src, EFightMsg::kDescription);
}

void FightMessagesLoader::Load(parser_wrapper::DataNode data) {
	MUD::FightMessages().Init(data.Children());
}

void FightMessagesLoader::Reload(parser_wrapper::DataNode data) {
	MUD::FightMessages().Reload(data.Children());
}

// issue.vedun-msg-editor: editor discovery + safe-commit validation.
std::string FightMessagesLoader::EditableWhat() const {
	return "hitmsg";
}

std::vector<cfg_manager::EditableElement> FightMessagesLoader::ListElements() const {
	std::vector<cfg_manager::EditableElement> out;
	for (const auto &sheaf : MUD::FightMessages()) {
		const EDamageSource id = sheaf.GetId();
		const std::string id_str = (id == EDamageSource::kUndefined) ? "kDefault" : NAME_BY_ITEM<EDamageSource>(id);
		out.push_back({id_str, ""});
	}
	return out;
}

cfg_manager::ValidationResult FightMessagesLoader::Validate(parser_wrapper::DataNode &doc) const {
	if (MUD::FightMessages().Validate(doc.Children())) {
		return {true, ""};
	}
	return {false, "Hit-message data failed to parse (see syslog for the offending sheaf/message)."};
}

bool FightMessagesLoader::IsValidElementId(const std::string &id) const {
	try {
		return ITEM_BY_NAME<EDamageSource>(id) != EDamageSource::kUndefined;
	} catch (const std::exception &) {
		return false;
	}
}

parser_wrapper::DataNode FightMessagesLoader::CreateElementNode(parser_wrapper::DataNode root, const std::string &id) const {
	// An empty sheaf for `id`; the editor then adds <message> children.
	auto node = root.AddChild("msg_sheaf");
	node.SetValue("id", id);
	return node;
}

} // namespace fight

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
