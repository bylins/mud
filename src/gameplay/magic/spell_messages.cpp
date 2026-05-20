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
		{ESpellMsg::kTargetUnavailable, "kTargetUnavailable"},
		{ESpellMsg::kCantCastPeaceful, "kCantCastPeaceful"},
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
		{"kTargetUnavailable", ESpellMsg::kTargetUnavailable},
		{"kCantCastPeaceful", ESpellMsg::kCantCastPeaceful},
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
