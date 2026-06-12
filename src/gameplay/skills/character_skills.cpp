/**
\file character_skills.cpp - a part of the Bylins engine.
\brief issue.chardata-cleaning: CharacterSkills implementation (see character_skills.h).
*/

#include "character_skills.h"

#include "engine/structs/structs.h"   // kBattleRound
#include "engine/db/db.h"             // chardata_cooldown_list

#include <cmath>

void CharacterSkillData::DecreaseCooldown(unsigned value) {
	cooldown = (cooldown > value) ? cooldown - value : 0u;
}

int CharacterSkills::GetLevel(ESkill skill) const {
	const auto it = data_.find(skill);
	return it != data_.end() ? it->second.skill_level : 0;
}

// A zero level is not stored: setting an existing skill to 0 removes its record.
void CharacterSkills::SetLevel(ESkill skill, int percent) {
	const auto it = data_.find(skill);
	if (it != data_.end()) {
		if (percent) {
			it->second.skill_level = percent;
		} else {
			data_.erase(it);
		}
	} else if (percent) {
		data_[skill].skill_level = percent;
	}
}

void CharacterSkills::Clear() {
	data_.clear();
	data_[ESkill::kGlobalCooldown];   // keep the always-present global-cooldown slot
}

void CharacterSkills::SetCooldown(ESkill skill, unsigned cooldown) {
	const auto it = data_.find(skill);
	if (it != data_.end()) {
		it->second.cooldown = cooldown;
		chardata_cooldown_list.insert(owner_);
	}
}

unsigned CharacterSkills::GetCooldown(ESkill skill) const {
	const auto it = data_.find(skill);
	return it != data_.end() ? it->second.cooldown : 0u;
}

int CharacterSkills::GetCooldownInPulses(ESkill skill) const {
	return static_cast<int>(std::ceil(GetCooldown(skill) / (0.0 + kBattleRound)));
}

void CharacterSkills::DecreaseCooldowns(unsigned value) {
	for (auto &kv : data_) {
		kv.second.DecreaseCooldown(value);
	}
}

bool CharacterSkills::DecreaseCooldownsAndCheck() {
	bool has_cooldown = false;
	for (auto &kv : data_) {
		kv.second.DecreaseCooldown(1);
		if (kv.second.cooldown > 0) {
			has_cooldown = true;
		}
	}
	return has_cooldown;
}

void CharacterSkills::ZeroCooldowns() {
	for (auto &kv : data_) {
		kv.second.cooldown = 0u;
	}
}

bool CharacterSkills::HasCooldown(ESkill skill) const {
	const auto it = data_.find(skill);
	return it != data_.end() && it->second.cooldown > 0;
}

bool CharacterSkills::HasActiveCooldown(ESkill skill) const {
	if (GetCooldown(ESkill::kGlobalCooldown) > 0) {
		return true;
	}
	return HasCooldown(skill);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
