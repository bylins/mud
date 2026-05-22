/**
\file fight_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue #3311).
\brief Weapon attack (hit) type message container: loader + name registration.
*/

#include "fight_messages.h"

#include "engine/db/global_objects.h"

#include <map>

template<>
const std::string &NAME_BY_ITEM<fight::EAttackType>(const fight::EAttackType item) {
	static const std::map<fight::EAttackType, std::string> kMap{
		{fight::EAttackType::kUndefined, "kUndefined"},
		{fight::EAttackType::kHit, "kHit"},
		{fight::EAttackType::kSkin, "kSkin"},
		{fight::EAttackType::kWhip, "kWhip"},
		{fight::EAttackType::kSlash, "kSlash"},
		{fight::EAttackType::kBite, "kBite"},
		{fight::EAttackType::kBludgeon, "kBludgeon"},
		{fight::EAttackType::kCrush, "kCrush"},
		{fight::EAttackType::kPound, "kPound"},
		{fight::EAttackType::kClaw, "kClaw"},
		{fight::EAttackType::kMaul, "kMaul"},
		{fight::EAttackType::kThrash, "kThrash"},
		{fight::EAttackType::kPierce, "kPierce"},
		{fight::EAttackType::kBlast, "kBlast"},
		{fight::EAttackType::kPunch, "kPunch"},
		{fight::EAttackType::kStab, "kStab"},
		{fight::EAttackType::kPick, "kPick"},
		{fight::EAttackType::kSting, "kSting"},
	};
	return kMap.at(item);
}

template<>
fight::EAttackType ITEM_BY_NAME<fight::EAttackType>(const std::string &name) {
	static const std::map<std::string, fight::EAttackType> kMap{
		{"kUndefined", fight::EAttackType::kUndefined},
		{"kDefault", fight::EAttackType::kUndefined}, // default sheaf id in hit_msg.xml
		{"kHit", fight::EAttackType::kHit},
		{"kSkin", fight::EAttackType::kSkin},
		{"kWhip", fight::EAttackType::kWhip},
		{"kSlash", fight::EAttackType::kSlash},
		{"kBite", fight::EAttackType::kBite},
		{"kBludgeon", fight::EAttackType::kBludgeon},
		{"kCrush", fight::EAttackType::kCrush},
		{"kPound", fight::EAttackType::kPound},
		{"kClaw", fight::EAttackType::kClaw},
		{"kMaul", fight::EAttackType::kMaul},
		{"kThrash", fight::EAttackType::kThrash},
		{"kPierce", fight::EAttackType::kPierce},
		{"kBlast", fight::EAttackType::kBlast},
		{"kPunch", fight::EAttackType::kPunch},
		{"kStab", fight::EAttackType::kStab},
		{"kPick", fight::EAttackType::kPick},
		{"kSting", fight::EAttackType::kSting},
	};
	return kMap.at(name); // throws std::out_of_range on unknown name
}

template<>
const std::string &NAME_BY_ITEM<fight::EFightMsg>(const fight::EFightMsg item) {
	static const std::map<fight::EFightMsg, std::string> kMap{
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
	};
	return kMap.at(item);
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
	};
	return kMap.at(name); // throws std::out_of_range on unknown name
}

namespace fight {

void FightMessagesLoader::Load(parser_wrapper::DataNode data) {
	MUD::FightMessages().Init(data.Children());
}

void FightMessagesLoader::Reload(parser_wrapper::DataNode data) {
	MUD::FightMessages().Reload(data.Children());
}

} // namespace fight

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
