/**
\file character_skills.h - a part of the Bylins engine.
\brief issue.chardata-cleaning: a character's learned-skills container.
\detail Replaces the loose `CharacterSkillDataType` struct + `CharSkillsType` map that lived in
        char_data.h. Encapsulates the std::map<ESkill, CharacterSkillData> and owns the per-skill
        cooldown logic that used to be a set of CharData methods. The global-cooldown slot is always
        present (was added by CharData's constructor). `owner_` is the owning character, needed to
        register active cooldowns in the global chardata_cooldown_list.
*/

#ifndef BYLINS_SRC_GAMEPLAY_SKILLS_CHARACTER_SKILLS_H_
#define BYLINS_SRC_GAMEPLAY_SKILLS_CHARACTER_SKILLS_H_

#include "gameplay/skills/skills.h"   // ESkill

#include <map>

class CharData;

// One learned skill: its trained percentage and its current cooldown.
struct CharacterSkillData {
	int skill_level{0};
	unsigned cooldown{0};

	void DecreaseCooldown(unsigned value);
};

// A character's learned skills, keyed by ESkill.
class CharacterSkills {
 public:
	using Map = std::map<ESkill, CharacterSkillData>;

	CharacterSkills() { data_[ESkill::kGlobalCooldown]; }   // the global-cooldown slot is always present

	void SetOwner(CharData *owner) { owner_ = owner; }

	// --- trained level ---
	[[nodiscard]] int GetLevel(ESkill skill) const;
	void SetLevel(ESkill skill, int percent);   // sets the level, or erases the record when percent == 0
	[[nodiscard]] int Count() const { return static_cast<int>(data_.size()); }
	void Clear();
	[[nodiscard]] const Map &data() const { return data_; }
	[[nodiscard]] Map &data() { return data_; }

	// --- cooldowns ---
	void SetCooldown(ESkill skill, unsigned cooldown);
	[[nodiscard]] unsigned GetCooldown(ESkill skill) const;
	[[nodiscard]] int GetCooldownInPulses(ESkill skill) const;
	void DecreaseCooldowns(unsigned value);
	bool DecreaseCooldownsAndCheck();                            // was CharData::HaveDecreaseCooldowns
	void ZeroCooldowns();
	[[nodiscard]] bool HasCooldown(ESkill skill) const;          // was CharData::haveSkillCooldown
	[[nodiscard]] bool HasActiveCooldown(ESkill skill) const;    // was CharData::HasCooldown (incl. global)

 private:
	Map data_;
	CharData *owner_{nullptr};
};

#endif  // BYLINS_SRC_GAMEPLAY_SKILLS_CHARACTER_SKILLS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
