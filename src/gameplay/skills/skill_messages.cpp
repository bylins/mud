/**
\file skill_messages.cpp - a part of the Bylins engine.
\authors Created by Claude (issue #3310).
\brief In-game message container for skills: loader + ESkillMsg name registration.
*/

#include "skill_messages.h"

#include "engine/db/global_objects.h"

#include <map>

template<>
const std::string &NAME_BY_ITEM<ESkillMsg>(const ESkillMsg item) {
	static const std::map<ESkillMsg, std::string> kMap{
		{ESkillMsg::kUndefined, "kUndefined"},
		{ESkillMsg::kDontKnowSkill, "kDontKnowSkill"},
		{ESkillMsg::kOnCooldown, "kOnCooldown"},
		{ESkillMsg::kCantWhileMounted, "kCantWhileMounted"},
		{ESkillMsg::kMustBeMounted, "kMustBeMounted"},
		{ESkillMsg::kGetOnFeet, "kGetOnFeet"},
		{ESkillMsg::kCantFightNow, "kCantFightNow"},
		{ESkillMsg::kNotFighting, "kNotFighting"},
		{ESkillMsg::kPeacefulRoom, "kPeacefulRoom"},
		{ESkillMsg::kNoTarget, "kNoTarget"},
		{ESkillMsg::kCantTargetSelf, "kCantTargetSelf"},
		{ESkillMsg::kNeedWeapon, "kNeedWeapon"},
		{ESkillMsg::kWrongWeapon, "kWrongWeapon"},
	};
	return kMap.at(item);
}

template<>
ESkillMsg ITEM_BY_NAME<ESkillMsg>(const std::string &name) {
	static const std::map<std::string, ESkillMsg> kMap{
		{"kUndefined", ESkillMsg::kUndefined},
		{"kDontKnowSkill", ESkillMsg::kDontKnowSkill},
		{"kOnCooldown", ESkillMsg::kOnCooldown},
		{"kCantWhileMounted", ESkillMsg::kCantWhileMounted},
		{"kMustBeMounted", ESkillMsg::kMustBeMounted},
		{"kGetOnFeet", ESkillMsg::kGetOnFeet},
		{"kCantFightNow", ESkillMsg::kCantFightNow},
		{"kNotFighting", ESkillMsg::kNotFighting},
		{"kPeacefulRoom", ESkillMsg::kPeacefulRoom},
		{"kNoTarget", ESkillMsg::kNoTarget},
		{"kCantTargetSelf", ESkillMsg::kCantTargetSelf},
		{"kNeedWeapon", ESkillMsg::kNeedWeapon},
		{"kWrongWeapon", ESkillMsg::kWrongWeapon},
	};
	return kMap.at(name); // throws std::out_of_range on unknown name
}

namespace skills {

void SkillMessagesLoader::Load(parser_wrapper::DataNode data) {
	MUD::SkillMessages().Init(data.Children());
}

void SkillMessagesLoader::Reload(parser_wrapper::DataNode data) {
	MUD::SkillMessages().Reload(data.Children());
}

} // namespace skills

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
