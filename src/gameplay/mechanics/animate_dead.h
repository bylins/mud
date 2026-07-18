/**
\file animate_dead.h - a part of the Bylins engine.
\brief Warlock construct (kAnimateDead) parameters, moved to data (cfg/mechanics/animate_dead.xml).
\details A necromancer's undead are described in one file: the control budget, the tier ladder
		 (each tier = a mob vnum + skill-weight + corpse-level band + caster-rating gate + pick odds),
		 and per-tier stat scaling off the cast competence C. Loaded via CfgManager ("animate_dead"),
		 exposed through MUD::AnimateDead(). See claude_1/OUTPUT for the design.
*/

#ifndef GAMEPLAY_MECHANICS_ANIMATE_DEAD_H_
#define GAMEPLAY_MECHANICS_ANIMATE_DEAD_H_

#include "engine/boot/cfg_manager.h"
#include "gameplay/skills/skills.h"   // ESkill

#include <string>
#include <vector>

class CharData;

namespace animate_dead {

enum class EScaleMode { kAdd, kMult };

// One scaled stat: stat = add ? proto + round(beta*C) : proto * (1 + beta*C), then bounded by cap.
struct StatScale {
	double beta = 0.0;
	EScaleMode mode = EScaleMode::kAdd;
	bool has_cap = false;
	int cap = 0;
};

// Per-tier scaling. 'luck' is intentionally absent (mobs have no luck stat).
struct CreatureScaling {
	StatScale hp, damage_dice, hitroll, ac, skills, saving, armor, morale, initiative;
};

struct CreatureInfo {
	int vnum = 0;              // config index (weakest->strongest ladder order)
	std::string id;            // e.g. "kSkeleton"
	int proto_vnum = 0;        // the summoned mob prototype
	int weight = 0;            // control-budget cost
	int corpse_max_level = 0;  // highest source-corpse level that yields this tier
	int min_rating = 0;        // smallest caster rating (level+remort+4) allowed
	int pick = 0;              // relative odds within a shared top corpse band (0 = the single tier)
	CreatureScaling scaling;
};

class AnimateDeadInfo {
	ESkill control_skill_ = ESkill::kDarkMagic;
	int budget_cap_ = 75;
	std::vector<CreatureInfo> creatures_;   // ladder order, weakest -> strongest

 public:
	void Load(parser_wrapper::DataNode data);

	[[nodiscard]] ESkill ControlSkill() const { return control_skill_; }
	[[nodiscard]] int BudgetCap() const { return budget_cap_; }
	[[nodiscard]] const std::vector<CreatureInfo> &Creatures() const { return creatures_; }
	[[nodiscard]] bool empty() const { return creatures_.empty(); }
	// Tier lookups keyed by the summoned mob's proto vnum.
	[[nodiscard]] const CreatureInfo *ByProtoVnum(int proto_vnum) const;
	[[nodiscard]] int WeightOf(int proto_vnum) const;   // 0 if not a configured tier
	[[nodiscard]] int LadderIndex(int proto_vnum) const;
};

class AnimateDeadLoader : public cfg_manager::ICfgLoader {
 public:
	void Load(parser_wrapper::DataNode data) final;
	void Reload(parser_wrapper::DataNode data) final;
};

// The construct VNUM for a corpse of `corpse_mob_level`, cast by `ch`: the corpse-level band, then
// the caster-rating gate (demote down the ladder), then the random pick within a shared top band.
// Returns 0 when nothing is configured.
[[nodiscard]] int PickTier(CharData *ch, int corpse_mob_level);

// The strongest configured tier (vnum) not exceeding `desired` that fits the caster's remaining
// control budget, demoting down the ladder as needed. 0 when even the weakest tier does not fit.
[[nodiscard]] int FitUndeadTier(CharData *ch, int desired);

// Scale a freshly-read construct's stats off the caster's cast competence C, per the tier's
// <scaling>. No-op for a mob whose vnum is not a configured tier.
void SetupUndeadStats(CharData *ch, CharData *mob, double competence);

}  // namespace animate_dead

#endif  // GAMEPLAY_MECHANICS_ANIMATE_DEAD_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
