/**
\authors Created by Sventovit
\date 29.05.2022.
\brief Модуль способностей.
\details Структура с мнформацией о способностях и функцими для работы с ними - какого типа, какие эффектиы вызывают,
 при каких условиях могут быть изучены и так далее.
*/
#ifndef FILE_FEATURES_H_INCLUDED
#define FILE_FEATURES_H_INCLUDED

#include "abilities_items_set.h"
#include "abilities_constants.h"
#include "gameplay/skills/skills.h"
#include "talents_effects.h"
#include "engine/structs/structs.h"
#include "engine/core/conf.h"
#include "gameplay/classes/classes_constants.h"
#include "engine/boot/cfg_manager.h"
#include "engine/structs/info_container.h"
#include "gameplay/magic/spells_constants.h"   // issue.perk-action-patching: ESpell (patch target)
#include "talents_actions.h"                    // issue.perk-action-patching: Actions (patch payload)

#include <array>
#include <bitset>

struct TimedFeat {
	EFeat feat{EFeat::kUndefined};	// Used feature //
	ubyte time{0};				// Time for next using //
};

int CalcMaxFeatSlotPerLvl(const CharData *ch);
int CalcFeatSlotsAmountPerRemort(CharData *ch);
EFeat FindWeaponMasterFeat(ESkill skill);

void UnsetInaccessibleFeats(CharData *ch);
void SetRaceFeats(CharData *ch);
void UnsetRaceFeats(CharData *ch);
void SetInbornAndRaceFeats(CharData *ch);
bool CanUseFeat(const CharData *ch, EFeat feat_id);
bool CanGetFeat(CharData *ch, EFeat feat);
bool TryFlipActivatedFeature(CharData *ch, char *argument);
EPrf GetPrfWithFeatNumber(EFeat feat_id);

namespace feats {

using DataNode = parser_wrapper::DataNode;

// issue.perk-action-patching: a perk's declared modification of a spell's <action> chain. The payload
// holds perk-owned <action> block(s) parsed by the normal action parser; they live for the process
// lifetime, so the per-cast materializer can point at them (non-owning) without copying.
enum class EPatchOp { kAppend, kInsert, kReplace, kRemove, kAddEffect, kReplaceAll, kModify };

struct TalentPatch {
	enum class EScope { kCaster, kTarget };   // whose feat is tested (kTarget = future: the spell's victim)
	// issue.soullink-affect-patching: for AFFECT patches, whose feat gates the patch, relative to the affect
	// OWNER (bearer). kSelf = the bearer; kMaster = the bearer's charm master (kSoulLink); kGroupLeader.
	enum class ERelative { kSelf, kMaster, kGroupLeader };
	// Selector -- which spells this patch targets. All present predicates are AND-combined; an absent one
	// is vacuously true. Resolved to concrete spells ONCE at boot (BuildTalentPatchIndex), so class
	// selectors cost nothing at cast time. A patch with no selector at all is a load error (never global
	// by accident -- use all="true" to mean every spell).
	ESpell target_spell{ESpell::kUndefined};   // a specific spell
	EElement element{EElement::kUndefined};    // every spell of this element
	ESkill base_skill{ESkill::kUndefined};     // every spell whose potency OR success base skill matches
	Bitvector flag_sel{0};                     // every spell carrying this EMagic flag (0 = unused)
	bool match_all{false};                     // explicit opt-in: every spell
	talents_actions::EAction has_effect_kind{talents_actions::EAction::kDamage};   // spells whose chain
	bool has_has_effect{false};                //   contains this manifestation kind (has_effect="kArea")
	std::string category;                      // spells tagged with this category (<tags>); category="curse"
	EAffect target_affect{EAffect::kUndefined};// issue.soullink-affect-patching: patch an AFFECT's action
	                                           //   chain instead of a spell's (affect="kVampirism")
	[[nodiscard]] bool HasSelector() const {
		return target_spell != ESpell::kUndefined || element != EElement::kUndefined
			|| base_skill != ESkill::kUndefined || flag_sel != 0 || match_all || has_has_effect
			|| !category.empty() || target_affect != EAffect::kUndefined;
	}
	EPatchOp op{EPatchOp::kAppend};
	EScope scope{EScope::kCaster};
	ERelative relative{ERelative::kSelf};   // affect patches: gate on bearer's self/master/group-leader feat
	std::string action_id;    // target block id (replace / remove / add_effect)
	std::string effect_id;    // target manifestation id within that block (manifestation-level ops)
	std::string anchor_id;    // insert anchor block id
	bool anchor_before{false};
	// op="modify": scale/offset/set a numeric field of a manifestation (addressed by kind via effect=).
	talents_actions::EAction mod_kind{talents_actions::EAction::kArea};
	std::string mod_field;                     // e.g. "decay"
	enum class EModOp { kMul, kAdd, kSet } mod_op{EModOp::kMul};
	double mod_value{1.0};
	talents_actions::Actions payload;   // perk-provided <action> block(s) (unused for op="modify")
};

class FeatsLoader : virtual public cfg_manager::ICfgLoader {
 public:
	void Load(DataNode data) final;
	void Reload(DataNode data) final;
};

class FeatInfo : public info_container::BaseItem<EFeat> {
	friend class FeatInfoBuilder;

	std::string name_;

 public:
	FeatInfo() = default;
	FeatInfo(EFeat id, EItemMode mode)
		: BaseItem<EFeat>(id, mode) {};

	talents_effects::Effects effects;

	// issue.perk-action-patching: this feat's spell-action patches (empty for most feats).
	std::vector<TalentPatch> talent_patches;

	[[nodiscard]] const std::string &GetName() const { return name_; };
	[[nodiscard]] const char *GetCName() const { return name_.c_str(); };

	void Print(CharData *ch, std::ostringstream &buffer) const;
};

class FeatInfoBuilder : public info_container::IItemBuilder<FeatInfo> {
 public:
	ItemPtr Build(DataNode &node) final;
 private:
	static ItemPtr ParseFeat(DataNode &node);
	static ItemPtr ParseHeader(DataNode &node);
	static void ParseEffects(ItemPtr &info, DataNode &node);
};

using FeatsInfo = info_container::InfoContainer<EFeat, FeatInfo, FeatInfoBuilder>;

// issue.perk-action-patching: after feats + spells load, bucket every feat's patches onto the target
// SpellInfo (SpellInfo::talent_patches) and validate their target ids. Idempotent; called once from boot.
void BuildTalentPatchIndex();

// issue.soullink-affect-patching: an affect-targeting patch (which feat + the patch). Patches with an
// affect="kX" selector are bucketed by affect type at boot (parallel to SpellInfo::talent_patches).
struct AffectPatchRef { EFeat feat{EFeat::kUndefined}; const TalentPatch *patch{nullptr}; };
// The affect patches for `affect_type` (empty for affects with none). Consumed by the affect-trigger runner.
const std::vector<AffectPatchRef> &AffectTalentPatches(EAffect affect_type);
// Resolve a patch's ERelative against the affect owner (bearer): self / charm master / group leader.
// Returns nullptr if the relative does not exist. Defined in feats.cpp.
CharData *ResolvePatchRelative(CharData *owner, TalentPatch::ERelative relative);

}

#endif // __FEATURES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
