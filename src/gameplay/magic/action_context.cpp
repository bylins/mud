/**
 \file action_context.cpp - a part of the Bylins engine.
 \brief ActionContext: the cast's action cursor, competence base and talent-patch application.
*/

#include "magic_internal.h"

#include "engine/db/global_objects.h"

double ActionContext::CompetenceBase() const {
	const double real = potency_.skill_coeff + potency_.stat_coeff;
	if (is_entry_action) {
		return real;
	}
	switch (action_or_default().GetBase()) {
		case talents_actions::EActionBase::kDamage:    return damage_count;
		case talents_actions::EActionBase::kPoints:    return points_count;
		case talents_actions::EActionBase::kAffects:   return affects_potency;
		case talents_actions::EActionBase::kDispelled: return dispelled_potency;
		// base="tag": the event tag's numeric value (e.g. damage amount).
		case talents_actions::EActionBase::kTag:
			return static_cast<double>(event_.GetTag(action_or_default().GetTagName()));
		case talents_actions::EActionBase::kCompetence:
		default:                                       return real;
	}
}

const talents_actions::Action &ActionContext::action_or_default() const {
	// Cursor active (loop driving) -> the current action; otherwise (bypass
	// callers / rooms / area setup that never rewound) -> the spell's primary.
	const talents_actions::Action *a = action();
	if (a) {
		return *a;
	}
	// A patched cast whose cursor ran past the end still reads from the view.
	if (patched_ && !action_view_.empty()) {
		return *action_view_.front();
	}
	if (external_actions_ && !external_actions_->empty()) {
		return (*external_actions_)[0];
	}
	return MUD::Spell(spell_id_).actions.primary();
}

// --- ActionContext action cursor (multi-action) ---
// Defined here (not the header) so it can reach MUD::Spell for the action list.
// A spell with NO <action> blocks (flag-only spells: kDazzle/kCombatLuck/...,
// plus the summon/creation/manual stages) still has flagged stages that run their
// effect in code. To keep them firing, an empty action list yields exactly ONE
// loop iteration with action() == nullptr; the data-reading stages then fall back
// to the spell-id getters (which return the empty-action default). So the loop runs
// max(1, action_count) times.
void ActionContext::RewindActions() {
	actions_ = external_actions_ ? external_actions_ : &MUD::Spell(spell_id_).actions.list();
	action_idx_ = 0;
}

void ActionContext::NextAction() {
	++action_idx_;
}

const talents_actions::Action *ActionContext::action() const {
	// A per-cast patched chain wins over the spell's own list. patched_ (not emptiness) is the
	// selector, so a perk that removed every block still overrides.
	if (patched_) {
		return action_idx_ < action_view_.size() ? action_view_[action_idx_] : nullptr;
	}
	if (!actions_ || action_idx_ >= actions_->size()) {
		return nullptr;
	}
	return &(*actions_)[action_idx_];
}

bool ActionContext::HasPendingActions() const {
	const size_t count = patched_ ? action_view_.size() : (actions_ ? actions_->size() : 0);
	return action_idx_ < std::max<size_t>(1, count);
}

// Materialize this cast's patched action chain (once per cast, in CallMagic). A spell without talent
// patches (nearly all) returns at once and the cast walks the spell's own action list; a per-cast
// view is built only when the spell has patches AND the caster can use the matching perk.
void ActionContext::ApplyTalentPatches() {
	if (!caster_) {
		return;
	}
	const auto &patches = MUD::Spell(spell_id_).talent_patches;
	if (patches.empty()) {
		return;
	}
	const auto &own = MUD::Spell(spell_id_).actions.list();
	action_view_.clear();
	action_view_.reserve(own.size() + patches.size());
	for (const auto &blk : own) {
		action_view_.push_back(&blk);
	}
	patched_ = true;
	// patches arrive in EFeat-enum order (MUD::Feats() is an ordered map), so application is deterministic.
	for (const auto &ref : patches) {
		const feats::TalentPatch &p = *ref.patch;
		// Only caster-held perks are materialized here (once per cast). Target-held perks must be resolved
		// per victim inside the target loop -- not wired yet, so skip them to avoid misfiring.
		if (p.scope != feats::TalentPatch::EScope::kCaster) {
			continue;
		}
		if (!CanUseFeat(caster_, ref.feat)) {
			continue;
		}
		ApplyOnePatch(p);
	}
}

// Bridge the patch's mod op to the manifestation-side enum: op="modify" arithmetic lives in
// IAction::ApplyFieldMod, shared by spell and affect action chains.
talents_actions::EFieldModOp ToFieldModOp(feats::TalentPatch::EModOp op) {
	switch (op) {
		case feats::TalentPatch::EModOp::kMul: return talents_actions::EFieldModOp::kMul;
		case feats::TalentPatch::EModOp::kAdd: return talents_actions::EFieldModOp::kAdd;
		case feats::TalentPatch::EModOp::kSet: return talents_actions::EFieldModOp::kSet;
	}
	return talents_actions::EFieldModOp::kSet;
}

void ActionContext::ApplyOnePatch(const feats::TalentPatch &p) {
	using talents_actions::Action;
	using Diff = std::vector<const Action *>::difference_type;
	const auto &payload = p.payload.list();

	auto find_idx = [&](const std::string &id) -> long {
		for (size_t i = 0; i < action_view_.size(); ++i) {
			if (action_view_[i]->GetId() == id) {
				return static_cast<long>(i);
			}
		}
		return -1;
	};
	auto payload_ptrs = [&]() {
		std::vector<const Action *> v;
		v.reserve(payload.size());
		for (const auto &b : payload) {
			v.push_back(&b);
		}
		return v;
	};

	switch (p.op) {
		case feats::EPatchOp::kReplaceAll: {
			action_view_.clear();
			const auto v = payload_ptrs();
			action_view_.insert(action_view_.end(), v.begin(), v.end());
			break;
		}
		case feats::EPatchOp::kAppend: {
			const auto v = payload_ptrs();
			action_view_.insert(action_view_.end(), v.begin(), v.end());
			break;
		}
		case feats::EPatchOp::kInsert: {
			const long i = p.anchor_id.empty() ? -1 : find_idx(p.anchor_id);
			const size_t at = (i < 0) ? action_view_.size()
									  : (p.anchor_before ? static_cast<size_t>(i) : static_cast<size_t>(i) + 1);
			const auto v = payload_ptrs();
			action_view_.insert(action_view_.begin() + static_cast<Diff>(at), v.begin(), v.end());
			break;
		}
		case feats::EPatchOp::kRemove: {
			const long i = find_idx(p.action_id);
			if (i < 0) {
				break;
			}
			if (p.effect_id.empty()) {
				action_view_.erase(action_view_.begin() + static_cast<Diff>(i));
			} else {
				patch_scratch_.push_back(*action_view_[i]);
				patch_scratch_.back().EraseManifestationsById(p.effect_id);
				action_view_[i] = &patch_scratch_.back();
			}
			break;
		}
		case feats::EPatchOp::kReplace: {
			const long i = find_idx(p.action_id);
			if (i < 0) {
				break;
			}
			if (p.effect_id.empty()) {
				action_view_.erase(action_view_.begin() + static_cast<Diff>(i));
				const auto v = payload_ptrs();
				action_view_.insert(action_view_.begin() + static_cast<Diff>(i), v.begin(), v.end());
			} else {
				patch_scratch_.push_back(*action_view_[i]);
				Action &copy = patch_scratch_.back();
				copy.EraseManifestationsById(p.effect_id);
				for (const auto &b : payload) {
					copy.MergeManifestationsFrom(b);
				}
				action_view_[i] = &copy;
			}
			break;
		}
		case feats::EPatchOp::kAddEffect: {
			const long i = find_idx(p.action_id);
			if (i < 0) {
				break;
			}
			patch_scratch_.push_back(*action_view_[i]);
			Action &copy = patch_scratch_.back();
			for (const auto &b : payload) {
				copy.MergeManifestationsFrom(b);
			}
			action_view_[i] = &copy;
			break;
		}
		case feats::EPatchOp::kModify: {
			// find the block carrying the target manifestation kind (by action id if given, else first match)
			long i = -1;
			if (!p.action_id.empty()) {
				i = find_idx(p.action_id);
			} else {
				for (size_t k = 0; k < action_view_.size(); ++k) {
					if (action_view_[k]->Contains(p.mod_kind)) { i = static_cast<long>(k); break; }
				}
			}
			if (i < 0) {
				break;
			}
			patch_scratch_.push_back(*action_view_[i]);
			Action &copy = patch_scratch_.back();
			talents_actions::IAction *man = copy.CloneManifestation(p.mod_kind);
			if (man) {
				man->ApplyFieldMod(p.mod_field, ToFieldModOp(p.mod_op), p.mod_value);
			}
			action_view_[i] = &copy;
			break;
		}
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
