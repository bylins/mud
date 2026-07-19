#include "talents_actions.h"

#include "engine/ui/color.h"
#include "engine/core/target_resolver.h"
#include "engine/entities/char_data.h"
#include "gameplay/magic/spells_constants.h"
#include "gameplay/fight/fight_constants.h"   // issue.damage-change: fight::DmgType + hit-flag values
#include "gameplay/magic/magic_rooms.h"  // ERoomAffect, ITEM_BY_NAME/NAME_BY_ITEM<ERoomAffect>
#include "gameplay/skills/skills.h"  // NAME_BY_ITEM<ESkill>
#include "fmt/format.h"
#include "utils/random.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>

namespace talents_actions {

namespace {
// issue.vedun-print: token names for the internal action enums (mirror the parser below and the
// enum order in talents_actions.h) so "show spell" prints e.g. kTarMinions rather than the raw 7.
const char *ActionTargetName(EActionTarget t) {
	switch (t) {
		case EActionTarget::kTarSame: return "kTarSame";
		case EActionTarget::kTarFightSelf: return "kTarFightSelf";
		case EActionTarget::kTarFightVict: return "kTarFightVict";
		case EActionTarget::kTarGroup: return "kTarGroup";
		case EActionTarget::kTarFoes: return "kTarFoes";
		case EActionTarget::kTarRandomFoe: return "kTarRandomFoe";
		case EActionTarget::kTarRandomAlly: return "kTarRandomAlly";
		case EActionTarget::kTarMinions: return "kTarMinions";
		case EActionTarget::kTarActor: return "kTarActor";
		case EActionTarget::kTarRoomThis: return "kTarRoomThis";
		case EActionTarget::kTarMaster: return "kTarMaster";
	}
	return "?";
}

const char *ActionBaseName(EActionBase b) {
	switch (b) {
		case EActionBase::kCompetence: return "kCompetence";
		case EActionBase::kDamage: return "kDamage";
		case EActionBase::kPoints: return "kPoints";
		case EActionBase::kAffects: return "kAffects";
		case EActionBase::kDispelled: return "kDispelled";
		case EActionBase::kTag: return "tag";
	}
	return "?";
}

const char *AreaDistributionName(EAreaDistribution d) {
	switch (d) {
		case EAreaDistribution::kUniform: return "kUniform";
		case EAreaDistribution::kLinear: return "kLinear";
		case EAreaDistribution::kStepped: return "kStepped";
	}
	return "?";
}

// Mob flags (EMobFlag) addressable from the <blocking>/<required>/<caster_blocking>
// <mob_flags val=...> child tag. A focused table rather than a full EMobFlag name registry:
// only flags meaningful as a cast gate (immunities for <blocking>, target requirements like
// kCorpse for <required>) live here.
const std::map<std::string, EMobFlag> kBlockingFlagByName{
	{"kNoBlind", EMobFlag::kNoBlind},
	{"kNoSleep", EMobFlag::kNoSleep},
	{"kNoSilence", EMobFlag::kNoSilence},
	{"kNoHold", EMobFlag::kNoHold},
	{"kNoBash", EMobFlag::kNoBash},
	{"kNoCharm", EMobFlag::kNoCharm},
	{"kNoSummon", EMobFlag::kNoSummon},
	{"kNoFear", EMobFlag::kNoFear},
	{"kCorpse", EMobFlag::kCorpse},
	{"kResurrected", EMobFlag::kResurrected},
	{"kMounting", EMobFlag::kMounting},
	{"kHelper", EMobFlag::kHelper},
	{"kClone", EMobFlag::kClone},
	{"kIgnoreForbidden", EMobFlag::kIgnoreForbidden},   // issue.room-affect-trigger-improve: kForbidden seal exempts these
	{"kCompanion", EMobFlag::kCompanion},   // any NPC ally (charm/summon/clone/keeper); kForbidden lets minions follow
};

// issue.room-affect-trigger-improve: <trigger val=...> token -> event flag. Single source of truth for
// the trigger names (replaces a hand-rolled if/else chain). A `|`-separated val sets several flags --
// an action may fire on more than one event (e.g. "kPulse|kEnter").
const std::map<std::string, EActionTrigger> kActionTriggerByName{
	{"kPulse", EActionTrigger::kPulse},
	{"kBattlePulse", EActionTrigger::kBattlePulse},
	{"kEnter", EActionTrigger::kEnter},
	{"kEnterPC", EActionTrigger::kEnterPC},
	{"kEnterNPC", EActionTrigger::kEnterNPC},
	{"kPick", EActionTrigger::kPick},
	{"kUnlock", EActionTrigger::kUnlock},
	{"kOpen", EActionTrigger::kOpen},
	{"kClose", EActionTrigger::kClose},
	{"kLock", EActionTrigger::kLock},
	{"kPreHit", EActionTrigger::kPreHit},
	{"kPostHit", EActionTrigger::kPostHit},
	{"kWardAttack", EActionTrigger::kWardAttack},
	{"kKill", EActionTrigger::kKill},
	{"kExpired", EActionTrigger::kExpired},
	{"kDispell", EActionTrigger::kDispell},
	{"kPoints", EActionTrigger::kPoints},
	{"kDeath", EActionTrigger::kDeath},
	{"kWardDamage", EActionTrigger::kWardDamage},
};

// issue.damage-change: name -> value maps for <damage_change> conditions/flags. DmgType and the fight
// hit-flags aren't meta-enum-registered, so we map by name here (values mirror fight_constants.h). EElement
// IS registered (ITEM_BY_NAME<EElement>), so it's parsed directly. The masks are stored decoupled (ints).
const std::map<std::string, int> kDmgTypeByName{
	{"kUndefDmg", fight::kUndefDmg}, {"kPhysDmg", fight::kPhysDmg}, {"kMagicDmg", fight::kMagicDmg},
	{"kPoisonDmg", fight::kPoisonDmg}, {"kPureDmg", fight::kPureDmg},
};
// Hit-flags referenceable from <damage_change> conditions/edits. Besides the author-facing kIgnore*/
// kCrit* set, the kDrawBrief* display flags and kShieldApplied/kPunctualCrit are exposed so shield
// reductions (kWardDamage <damage_change>) can light the brief-shield HUD, enforce one-shield-per-hit,
// and let the ice crit-absorb skip precise-style crits. (Which shield is active is decided in code by
// the weighted SelectMagicShield, not via a flag.)
const std::map<std::string, int> kHitFlagByName{
	{"kIgnoreSanct", fight::kIgnoreSanct}, {"kIgnorePrism", fight::kIgnorePrism},
	{"kIgnoreArmor", fight::kIgnoreArmor}, {"kHalfIgnoreArmor", fight::kHalfIgnoreArmor},
	{"kIgnoreAbsorbe", fight::kIgnoreAbsorbe}, {"kNoFleeDmg", fight::kNoFleeDmg},
	{"kCritHit", fight::kCritHit}, {"kCritLuck", fight::kCritLuck},
	{"kIgnoreFireShield", fight::kIgnoreFireShield}, {"kMagicReflect", fight::kMagicReflect},
	{"kIgnoreBlink", fight::kIgnoreBlink},
	{"kDrawBriefFireShield", fight::kDrawBriefFireShield}, {"kDrawBriefAirShield", fight::kDrawBriefAirShield},
	{"kDrawBriefIceShield", fight::kDrawBriefIceShield}, {"kDrawBriefMagMirror", fight::kDrawBriefMagMirror},
	{"kShieldApplied", fight::kShieldApplied}, {"kPunctualCrit", fight::kPunctualCrit},
};

// issue.damage-change: parse a `|`-separated name list into a mask (bit = enum value). Unknown => err_log.
static unsigned ParseDmgTypeMask(const char *val) {
	unsigned mask = 0;
	if (!val || !*val) { return mask; }
	for (const auto &name : utils::Split(val, '|')) {
		const auto it = kDmgTypeByName.find(name);
		if (it != kDmgTypeByName.end()) { mask |= (1u << static_cast<unsigned>(it->second)); }
		else { err_log("Actions: unknown <damage_change type='%s'>.", name.c_str()); }
	}
	return mask;
}
static unsigned ParseElementMask(const char *val) {
	unsigned mask = 0;
	if (!val || !*val) { return mask; }
	for (const auto &name : utils::Split(val, '|')) {
		const EElement e = ITEM_BY_NAME<EElement>(name);
		mask |= (1u << static_cast<unsigned>(e));   // kUndefined(0) for an unknown name -> harmless bit
	}
	return mask;
}
static unsigned long long ParseHitFlagMask(const char *val) {
	unsigned long long mask = 0;
	if (!val || !*val) { return mask; }
	for (const auto &name : utils::Split(val, '|')) {
		const auto it = kHitFlagByName.find(name);
		if (it != kHitFlagByName.end()) { mask |= (1ULL << static_cast<unsigned>(it->second)); }
		else { err_log("Actions: unknown <damage_change flag '%s'>.", name.c_str()); }
	}
	return mask;
}

// issue.character-affect-triggers: <trigger val="kPoints" category="..."/> -> a points_intensity::ECategory
// value (kept as int here to avoid a magic-subsystem include; MUST mirror that enum's values). kDamage
// is intentionally absent -- it is weapon-hit narration, not a restoration a kPoints trigger reacts to.
const std::map<std::string, int> kPointsCategoryByName{
	{"kHeal", 0}, {"kMoves", 1}, {"kThirst", 2}, {"kFull", 3},
};

// issue.room-affect-trigger-improve: <action target=...> token -> EActionTarget, replacing a
// hand-rolled if/else chain. (The reverse, EActionTarget -> token, stays in ActionTargetName's
// switch so a new enumerator trips -Wswitch.)
const std::map<std::string, EActionTarget> kActionTargetByName{
	{"kTarSame", EActionTarget::kTarSame},
	{"kTarFightSelf", EActionTarget::kTarFightSelf},
	{"kTarFightVict", EActionTarget::kTarFightVict},
	{"kTarGroup", EActionTarget::kTarGroup},
	{"kTarFoes", EActionTarget::kTarFoes},
	{"kTarRandomFoe", EActionTarget::kTarRandomFoe},
	{"kTarRandomAlly", EActionTarget::kTarRandomAlly},
	{"kTarMinions", EActionTarget::kTarMinions},
	{"kTarActor", EActionTarget::kTarActor},
	{"kTarRoomThis", EActionTarget::kTarRoomThis},
	{"kTarMaster", EActionTarget::kTarMaster},
};

// Room flags (ERoomFlag) addressable from the <blocking>/<required>/<caster_blocking>
// <room_flags val=...> child tag. A focused table -- only flags meaningful as a cast gate
// live here (kNoMagic is the universal "magic doesn't work here" gate for all non-warcry
// spells; the others are spell-specific blockers like kRuneLabel's room-flag fizzle path).
const std::map<std::string, ERoomFlag> kBlockingRoomFlagByName{
	{"kNoMagic", ERoomFlag::kNoMagic},
	{"kPeaceful", ERoomFlag::kPeaceful},
	{"kTunnel", ERoomFlag::kTunnel},
	{"kNoTeleportIn", ERoomFlag::kNoTeleportIn},
	{"kNoTeleportOut", ERoomFlag::kNoTeleportOut},
	{"kNoBattle", ERoomFlag::kNoBattle},
	{"kGodsRoom", ERoomFlag::kGodsRoom},
	{"kForMono", ERoomFlag::kForMono},
	{"kForPoly", ERoomFlag::kForPoly},
};

// Resolve a `|`-separated list of AFFECT names (an any_of/all_of attribute of an <unaffect>
// sub-tag) into char-affect (EAffect) and room-affect (ERoomAffect) ids. issue.affect-migration:
// the tags name the AFFECT, not the casting spell; a name resolves in exactly one domain (the
// EAffect / ERoomAffect namespaces are disjoint), so each token lands in either out.chars or
// out.rooms. Empty/absent attribute -> empty refs. "*" is the wildcard (handled by the caller).
void ParseAffectRefs(const char *value, TalentUnaffect::AffectRefs &out) {
	if (!value || !*value) {
		return;
	}
	for (const auto &name : utils::Split(value, '|')) {
		try {
			out.chars.push_back(ITEM_BY_NAME<EAffect>(name));
			continue;
		} catch (const std::out_of_range &) {}
		try {
			out.rooms.push_back(ITEM_BY_NAME<room_spells::ERoomAffect>(name));
		} catch (const std::out_of_range &) {
			err_log("TalentUnaffect: unknown affect '%s' in <unaffect>.", name.c_str());
		}
	}
}

// Parse one any_of/all_of attribute, recognising the "*" wildcard token. On a wildcard,
// out_wildcard is set true and the explicit refs stay empty. Mixing wildcard with explicit
// names (e.g. "*|kCurse") is rejected with an error: the semantic is ambiguous and the
// explicit names are redundant if "*" is present.
void ParseRemovalAttr(const char *value, TalentUnaffect::AffectRefs &out_refs, bool &out_wildcard,
					  const char *tag_name, const char *attr_name) {
	if (!value || !*value) {
		return;
	}
	const auto parts = utils::Split(value, '|');
	const bool has_wild = std::any_of(parts.begin(), parts.end(),
		[](const std::string &s) { return s == "*"; });
	if (has_wild) {
		if (parts.size() > 1) {
			err_log("TalentUnaffect: \"*\" wildcard cannot be combined with explicit names in "
					"<%s %s=\"%s\">; treating as wildcard alone.", tag_name, attr_name, value);
		}
		out_wildcard = true;
		return;
	}
	ParseAffectRefs(value, out_refs);
}
}  // namespace

void Roll::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Potency roll: " << "\r\n"
		   << " Skill: " << kColorGrn << NAME_BY_ITEM<ESkill>(base_skill_) << kColorNrm
		   << " (low " << low_skill_bonus_ << " / hi " << hi_skill_bonus_ << ")"
		   << " Stat: " << kColorGrn << NAME_BY_ITEM<EBaseStat>(base_stat_) << kColorNrm
		   << " (>" << base_stat_threshold_ << " x" << base_stat_weight_ << ")\r\n";
}

void SuccessRoll::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Success roll: " << "\r\n"
		   << kColorGrn << "  Skill: " << NAME_BY_ITEM<ESkill>(base_skill_) << kColorNrm
		   << " (low " << low_skill_bonus_ << " / hi " << hi_skill_bonus_ << ")"
		   << " Stat: " << kColorGrn << NAME_BY_ITEM<EBaseStat>(base_stat_) << kColorNrm
		   << " (>" << base_stat_threshold_ << " x" << base_stat_weight_ << ")"
		   << " bonus[roll " << bonus_roll_ << " pvp " << bonus_pvp_ << " pve " << bonus_pve_
		   << " evp " << bonus_evp_ << " eve " << bonus_eve_ << "]"
		   << " crit[" << critsuccess_ << "/" << critfail_ << "]\r\n";
}

double Roll::CalcSkillCoeff(const CharData *const ch) const {
	auto skill = GetSkill(ch, base_skill_);
	auto low_skill = std::min(skill, abilities::kNoviceSkillThreshold);
	auto hi_skill = std::max(0, skill - abilities::kNoviceSkillThreshold);
	return (low_skill * low_skill_bonus_ + hi_skill * hi_skill_bonus_) / 100.0;
}

// As CalcSkillCoeff, but with an explicit skill value instead of the caster's GetSkill(base_skill_).
// Used by ingredient-magic brewing to compute a potion's potency from the recipe (brewing) skill.
double Roll::CalcSkillCoeffForValue(int skill) const {
	auto low_skill = std::min(skill, abilities::kNoviceSkillThreshold);
	auto hi_skill = std::max(0, skill - abilities::kNoviceSkillThreshold);
	return (low_skill * low_skill_bonus_ + hi_skill * hi_skill_bonus_) / 100.0;
}

double Roll::CalcLowSkillCoeff(const CharData *const ch) const {
	auto low_skill = std::min(GetSkill(ch, base_skill_), abilities::kNoviceSkillThreshold);
	return low_skill * low_skill_bonus_;
}

double Roll::CalcBaseStatCoeff(const CharData *const ch) const {
	return std::max(0, GetRealBaseStat(ch, base_stat_) - base_stat_threshold_) * base_stat_weight_ / 100.0;
}

// As CalcBaseStatCoeff, but with an explicit key-stat value instead of GetRealBaseStat(ch).
// issue.potion-hotfix P3: gives a non-crafted potion a potency as if brewed at a fixed key-stat.
double Roll::CalcBaseStatCoeffForValue(int stat) const {
	return std::max(0, stat - base_stat_threshold_) * base_stat_weight_ / 100.0;
}

// Editor-friendly reads: a <potency_roll>/<success_roll> built in the Vedun editor may leave some
// dice/skill/stat attributes unset; treat a missing/empty attribute as its neutral default (0 /
// kUndefined) rather than throwing "incorrect value ''" on save.
namespace {
int RollIntOr(const char *v, int def) { return (v && *v) ? parse::ReadAsInt(v) : def; }
double RollDoubleOr(const char *v, double def) { return (v && *v) ? parse::ReadAsDouble(v) : def; }
}  // namespace

Roll::Roll(parser_wrapper::DataNode &node) {
	if (node.GoToChild("noise")) {
		// issue.potency-noise: the spell's random-draw parameters (replaces <dices>). sigma = base
		// relative spread of the shared z; trunc = clamp of z in sd units. Absent <noise> -> sigma 0,
		// i.e. a deterministic spell unless a manifestation opts into noise via its own weight.
		noise_sigma_ = RollDoubleOr(node.GetValue("sigma"), 0.0);
		noise_trunc_ = RollDoubleOr(node.GetValue("trunc"), 2.0);
		node.GoToParent();
	}

	if (node.GoToChild("base_skill")) {
		const char *skill_id = node.GetValue("id");
		base_skill_ = (skill_id && *skill_id) ? parse::ReadAsConstant<ESkill>(skill_id) : ESkill::kUndefined;
		low_skill_bonus_ = RollDoubleOr(node.GetValue("low_skill_bonus"), 1.0);
		hi_skill_bonus_ = RollDoubleOr(node.GetValue("hi_skill_bonus"), 7.0);
		node.GoToParent();
	}

	if (node.GoToChild("base_stat")) {
		const char *stat_id = node.GetValue("id");
		if (stat_id && *stat_id) {
			base_stat_ = parse::ReadAsConstant<EBaseStat>(stat_id);
		}
		base_stat_threshold_ = RollIntOr(node.GetValue("threshold"), 0);
		base_stat_weight_ = RollDoubleOr(node.GetValue("weight"), 18.0);
		node.GoToParent();
	}
}

// issue.success-roll: parse <success_roll> -- base_skill/base_stat like potency_roll (NO <dices>),
// plus the optional <bonus> and <thresholds> blocks. Stored only; not yet consumed.
SuccessRoll::SuccessRoll(parser_wrapper::DataNode &node) {
	if (node.GoToChild("base_skill")) {
		const char *skill_id = node.GetValue("id");
		base_skill_ = (skill_id && *skill_id) ? parse::ReadAsConstant<ESkill>(skill_id) : ESkill::kUndefined;
		low_skill_bonus_ = RollDoubleOr(node.GetValue("low_skill_bonus"), 0.0);
		hi_skill_bonus_ = RollDoubleOr(node.GetValue("hi_skill_bonus"), 0.0);
		node.GoToParent();
	}
	if (node.GoToChild("base_stat")) {
		const char *stat_id = node.GetValue("id");
		if (stat_id && *stat_id) {
			base_stat_ = parse::ReadAsConstant<EBaseStat>(stat_id);
		}
		base_stat_threshold_ = RollIntOr(node.GetValue("threshold"), 0);
		base_stat_weight_ = RollDoubleOr(node.GetValue("weight"), 0.0);
		node.GoToParent();
	}
	if (node.GoToChild("bonus")) {
		bonus_roll_ = RollIntOr(node.GetValue("roll"), 0);
		bonus_pvp_ = RollIntOr(node.GetValue("pvp"), 0);
		bonus_pve_ = RollIntOr(node.GetValue("pve"), 0);
		bonus_evp_ = RollIntOr(node.GetValue("evp"), 0);
		bonus_eve_ = RollIntOr(node.GetValue("eve"), 0);
		node.GoToParent();
	}
	if (node.GoToChild("thresholds")) {
		critsuccess_ = RollIntOr(node.GetValue("critsuccess"), 6);
		critfail_ = RollIntOr(node.GetValue("critfail"), 95);
		node.GoToParent();
	}
}

// EFind names addressable from the <material where=...> attribute. A focused
// table -- only the three location flags used by ProcessMatComponents'
// equip->inventory->room search live here. Other EFind bits (kCharInRoom,
// kObjWorld, ...) are deliberately rejected; allowing them would invite
// material-component searches that ProcessMatComponents can't honour.
const std::map<std::string, EFind> kComponentWhereByName{
	{"kObjEquip",     EFind::kObjEquip},
	{"kObjInventory", EFind::kObjInventory},
	{"kObjRoom",      EFind::kObjRoom},
};

static constexpr Bitvector kDefaultComponentWhere =
		EFind::kObjEquip | EFind::kObjInventory | EFind::kObjRoom;

// Parses a <components>...</components> spell-level block.
// Each <material> child becomes one Material entry. Attribute defaults:
//   any_of / all_of: empty (parser warns -- material can never be satisfied).
//   where: kObjEquip|kObjInventory|kObjRoom.
// Unknown EFind names in `where` are dropped with an error log; unparseable
// vnums (non-integer tokens in any_of/all_of) raise the underlying parse
// exception, which Components callers must catch in the same way SpellInfoBuilder
// catches Roll/Actions errors today.
Components::Components(parser_wrapper::DataNode &node) {
	for (auto &child : node.Children()) {
		const auto name = child.GetName();
		// <verbal/>: presence-only marker -- no attributes today. Spells with
		// this child require speech; the kSilence-aware guards in do_cast,
		// CastSpell, process_player_attack and SaySpell consult HasVerbalComponent().
		if (strcmp(name, "verbal") == 0) {
			has_verbal_ = true;
			continue;
		}
		// <weave/>: presence-only marker -- no attributes today.
		// Spells with this child are "actual magic" and cannot be cast in a kNoMagic
		// room. The CallMagic-side gate (magic_utils.cpp) consults HasWeaveComponent() and
		// emits the same kCastForbidden* narration the legacy data-driven blocking
		// used to.
		if (strcmp(name, "weave") == 0) {
			has_weave_ = true;
			continue;
		}
		// <sight/>: presence-only marker. Casting requires the caster to see; the CallMagic-side
		// gate (magic_utils.cpp) blocks a blind caster via HasSightComponent() (issue.sight-component).
		if (strcmp(name, "sight") == 0) {
			has_sight_ = true;
			continue;
		}
		if (strcmp(name, "material") != 0) {
			continue;
		}
		Material mat;
		const char *any_of = child.GetValue("any_of");
		if (any_of && *any_of) {
			for (const auto &tok : utils::Split(any_of, '|')) {
				mat.any_of.push_back(parse::ReadAsInt(tok.c_str()));
			}
		}
		const char *all_of = child.GetValue("all_of");
		if (all_of && *all_of) {
			for (const auto &tok : utils::Split(all_of, '|')) {
				mat.all_of.push_back(parse::ReadAsInt(tok.c_str()));
			}
		}
		const char *where = child.GetValue("where");
		if (where && *where) {
			Bitvector w = 0;
			for (const auto &tok : utils::Split(where, '|')) {
				const auto it = kComponentWhereByName.find(tok);
				if (it != kComponentWhereByName.end()) {
					w |= it->second;
				} else {
					err_log("Components: unknown EFind '%s' in <material where>.", tok.c_str());
				}
			}
			mat.where = w;
		} else {
			mat.where = kDefaultComponentWhere;
		}
		// cost: optional, default 1 (single-charge consumption, matching the
		// legacy dec_val(2) behaviour). 0 = presence-only focus; -1 = consumed
		// whole in one cast. Parsed via ReadAsInt -- any other integer is a
		// charge cost.
		const char *cost = child.GetValue("cost");
		if (cost && *cost) {
			mat.cost = parse::ReadAsInt(cost);
		}
		if (mat.any_of.empty() && mat.all_of.empty()) {
			err_log("Components: <material> has neither any_of nor all_of -- "
					"the requirement is meaningless and will be skipped.");
		}
		materials_.push_back(std::move(mat));
	}
}

void Components::Print(std::ostringstream &buffer) const {
	if (materials_.empty() && !has_verbal_ && !has_weave_) {
		return;
	}
	buffer << "Components:\r\n";
	if (has_verbal_) {
		buffer << "  Verbal" << "\r\n";
	}
	if (has_weave_) {
		buffer << "  Weave" << "\r\n";
	}
	for (const auto &mat : materials_) {
		buffer << "  Material:";
		if (!mat.any_of.empty()) {
			buffer << " any_of=" << kColorGrn;
			for (size_t i = 0; i < mat.any_of.size(); ++i) {
				buffer << (i ? "|" : "") << mat.any_of[i];
			}
			buffer << kColorNrm;
		}
		if (!mat.all_of.empty()) {
			buffer << " all_of=" << kColorGrn;
			for (size_t i = 0; i < mat.all_of.size(); ++i) {
				buffer << (i ? "|" : "") << mat.all_of[i];
			}
			buffer << kColorNrm;
		}
		buffer << " where=" << kColorGrn;
		bool first = true;
		for (const auto &[name, value] : kComponentWhereByName) {
			if (mat.where & value) {
				buffer << (first ? "" : "|") << name;
				first = false;
			}
		}
		buffer << kColorNrm << " cost=" << kColorGrn << mat.cost << kColorNrm << "\r\n";
	}
}

void Damage::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Damage: " << "\r\n"
		   << " Saving: " << kColorGrn << NAME_BY_ITEM<ESaving>(saving_) << kColorNrm
		   << " Prob: " << kColorGrn << prob_ << kColorNrm << "\r\n"
		   << " Amount min: " << kColorGrn << amount_min_ << kColorNrm
		   << " beta: " << kColorGrn << amount_beta_ << kColorNrm << "\r\n";
	if (has_hits_) {
		buffer << " Hits: skill_divisor=" << kColorGrn << hits_skill_divisor_ << kColorNrm
			   << " max=" << kColorGrn << hits_max_ << kColorNrm
			   << " prob=" << kColorGrn << hits_prob_ << kColorNrm << "\r\n";
	}
}

Damage::Damage(parser_wrapper::DataNode &node) {
	saving_ = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
	const char *prob = node.GetValue("prob");
	prob_ = (prob && *prob) ? parse::ReadAsInt(prob) : 100;
	// issue.damage-over-time: <damage source="poison"> reproduces ProcessPoisonDmg's kPoison branch.
	const char *src = node.GetValue("source");
	if (src && strcmp(src, "poison") == 0) { source_ = EDamageSource::kPoison; }
	// <amount> is optional; absent -> keep the defaults (min 0, both weights 1.0). A present tag
	// may still omit individual attributes, which fall back to those same defaults.
	if (node.GoToChild("amount")) {
		const char *amin = node.GetValue("min");
		amount_min_ = (amin && *amin) ? parse::ReadAsDouble(amin) : 0.0;
		const char *ab = node.GetValue("beta");
		amount_beta_ = (ab && *ab) ? parse::ReadAsDouble(ab) : 1.0;
		const char *awt = node.GetValue("weight");
		amount_weight_ = (awt && *awt) ? parse::ReadAsDouble(awt) : 0.0;  // issue.potency-noise (stage 1, additive): weight on the shared draw
		node.GoToParent();
	}
	// <hits> is optional; absent -> the spell deals one hit. Individual attrs
	// also fall back to their member defaults (skill_divisor=25, max=1, prob=20).
	if (node.GoToChild("hits")) {
		has_hits_ = true;
		const char *hsd = node.GetValue("skill_divisor");
		hits_skill_divisor_ = (hsd && *hsd) ? parse::ReadAsInt(hsd) : hits_skill_divisor_;
		const char *hmx = node.GetValue("max");
		hits_max_ = (hmx && *hmx) ? parse::ReadAsInt(hmx) : hits_max_;
		const char *hpr = node.GetValue("prob");
		hits_prob_ = (hpr && *hpr) ? parse::ReadAsInt(hpr) : hits_prob_;
		node.GoToParent();
	}
	// issue.instant-death: <instant_death prob="N"/> (default 100) -- presence enables the lethal strike.
	if (node.GoToChild("instant_death")) {
		has_instant_death_ = true;
		const char *idp = node.GetValue("prob");
		instant_death_prob_ = (idp && *idp) ? parse::ReadAsInt(idp) : 100;
		node.GoToParent();
	}
}

// Parse one child amount tag (heal/moves/thirst/cond) into `a`. The shared
// schema is (min, dices_weight, alpha, beta); npc_coeff is parsed only
// when `with_npc` is set (heal only).
static void ParsePointsAmount(parser_wrapper::DataNode &node, const char *tag,
							  Points::Amount &a, bool with_npc) {
	if (!node.GoToChild(tag)) {
		return;
	}
	a.present = true;
	const char *amin = node.GetValue("min");
	a.min = (amin && *amin) ? parse::ReadAsDouble(amin) : 0.0;
	const char *ab = node.GetValue("beta");
	a.beta = (ab && *ab) ? parse::ReadAsDouble(ab) : 0.0;
	const char *awt2 = node.GetValue("weight");
	a.weight = (awt2 && *awt2) ? parse::ReadAsDouble(awt2) : 0.0;  // issue.potency-noise (stage 1, additive): weight on the shared draw
	if (with_npc) {
		// 1.0 default mirrors the legacy <heal npc_coeff/> default: NPC casters
		// get a *2 boost on heal points unless the tag overrides it.
		const char *npc = node.GetValue("npc_coeff");
		a.npc_coeff = (npc && *npc) ? parse::ReadAsDouble(npc) : 1.0;
	}
	node.GoToParent();
}

Points::Points(parser_wrapper::DataNode &node) {
	const char *extra = node.GetValue("extra");
	extra_ = (extra && *extra) ? std::max(0, parse::ReadAsInt(extra)) : 0;
	const char *prob = node.GetValue("prob");
	prob_ = (prob && *prob) ? parse::ReadAsInt(prob) : 100;
	ParsePointsAmount(node, "heal",   heal_,   /*with_npc=*/true);
	ParsePointsAmount(node, "moves",  moves_,  false);
	ParsePointsAmount(node, "thirst", thirst_, false);
	ParsePointsAmount(node, "full",   full_,   false);
}

static void PrintAmount(std::ostringstream &buffer, const char *label,
						const Points::Amount &a, bool with_npc) {
	if (!a.present) return;
	buffer << "  " << label << ": min=" << kColorGrn << a.min << kColorNrm
		   << " beta=" << kColorGrn << a.beta << kColorNrm;
	if (with_npc) {
		buffer << " npc_coeff=" << kColorGrn << a.npc_coeff << kColorNrm;
	}
	buffer << "\r\n";
}

void Points::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Points: extra=" << kColorGrn << extra_ << "%" << kColorNrm
		   << " prob=" << kColorGrn << prob_ << kColorNrm << "\r\n";
	PrintAmount(buffer, "Heal",   heal_,   /*with_npc=*/true);
	PrintAmount(buffer, "Moves",  moves_,  false);
	PrintAmount(buffer, "Thirst", thirst_, false);
	PrintAmount(buffer, "Full",   full_,   false);
}


void Area::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Area:" << "\r\n"
		   << "  Targets: skill_divisor=" << kColorGrn << skill_divisor << kColorNrm
		   << " dice_size=" << kColorGrn << dice_size << kColorNrm
		   << " min=" << kColorGrn << min_targets << kColorNrm
		   << " max=" << kColorGrn << max_targets << kColorNrm
		   << " stat_weight=" << kColorGrn << stat_weight << kColorNrm << "\r\n"
		   << "  Distribution: type=" << kColorGrn << AreaDistributionName(distribution) << kColorNrm
		   << " decay=" << kColorGrn << decay << kColorNrm
		   << " free_targets=" << kColorGrn << free_targets << kColorNrm << "\r\n";
}

void Summon::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Summon: base_fail=" << kColorGrn << base_fail << kColorNrm
		   << " min_fail=" << kColorGrn << min_fail << kColorNrm
		   << " handler=" << kColorGrn << (handler.empty() ? "-" : handler) << kColorNrm << "\r\n"
		   << "  Mob: vnum=" << kColorGrn << mob_vnum << kColorNrm
		   << " competence_weight=" << kColorGrn << competence_weight << kColorNrm
		   << " extra=" << kColorGrn << (extra ? "Y" : "N") << kColorNrm
		   << " keeper=" << kColorGrn << (keeper ? "Y" : "N") << kColorNrm
		   << " from_corpse=" << kColorGrn << (from_corpse ? "Y" : "N") << kColorNrm << "\r\n";
}

void Creation::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Creation: vnum=" << kColorGrn << vnum << kColorNrm
		   << " handler=" << kColorGrn << (handler.empty() ? "-" : handler) << kColorNrm << "\r\n";
}

void AlterObj::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " AlterObj: handler=" << kColorGrn << (handler.empty() ? "-" : handler) << kColorNrm
		   << (collateral_on_damage ? " collateral=on_damage" : "") << "\r\n";
}

// historical skill-scaled-dice count + an optional secondary-stat nudge.
// stat_coeff is the cast potency roll's stat coefficient; stat_weight=0 -> exactly the old count.
int Area::CalcTargetsQuantity(const int skill, const double stat_coeff) const {
	const int bonus = RollDices(skill / skill_divisor, dice_size)
			+ static_cast<int>(std::ceil(stat_weight * stat_coeff));
	// max <= 0 -> no upper limit on the bonus.
	return min_targets + (max_targets > 0 ? std::min(bonus, max_targets) : bonus);
}

// per-target falloff coefficient (1-based j of n). decay_eff is the
// effective (possibly kMultipleCast-softened) step rate. Result is always in [0,1].
double Area::DistributionCoeff(const int j, const int n, const double decay_eff) const {
	switch (distribution) {
		case EAreaDistribution::kLinear:
			return n > 0 ? static_cast<double>(n - j + 1) / n : 1.0;
		case EAreaDistribution::kStepped:
			return j <= free_targets
					? 1.0
					: std::max(0.0, 1.0 - decay_eff * (j - free_targets));
		case EAreaDistribution::kUniform:
		default:
			return 1.0;
	}
}

TalentAffect::TalentAffect(parser_wrapper::DataNode &node) {
	// Both OPTIONAL -- guard the reads so an absent attribute keeps the member default instead of throwing.
	// An unguarded ReadAsConstant("") threw -> Actions::Build dropped the WHOLE payload, silently disabling
	// any <affects> that omitted saving/resist (issue.perk-action-patching; same lesson as the <damage>
	// saving guard from issue.damage-over-time).
	if (const char *s = node.GetValue("saving"); s && *s) { saving_ = parse::ReadAsConstant<ESaving>(s); }
	if (const char *r = node.GetValue("resist"); r && *r) { resist_ = parse::ReadAsConstant<EResist>(r); }
	const char *prob = node.GetValue("prob");
	prob_ = (prob && *prob) ? parse::ReadAsInt(prob) : 100;
	// potency_weight: optional scale on the cast
	// potency stored on each Affect this block imposes. Default 1.0 (no change).
	// Mirrors TalentUnaffect::potency_weight_, which scales the dispel's roll
	// in the opposite direction.
	const char *pw = node.GetValue("potency_weight");
	potency_weight_ = (pw && *pw) ? static_cast<float>(parse::ReadAsDouble(pw)) : 1.0f;
	// issue.vampirism-haste: optional per-application battle flags OR'd onto each imposed affect. Lets an
	// action grant an affect with kAfBattledec (round-decrementing) though the affect TYPE is tick-based;
	// with kAfBattledec set, the <duration> below is read as raw combat rounds (no PC hour->tick scaling).
	const char *bf = node.GetValue("battleflag");
	extra_battleflags_ = (bf && *bf) ? parse::ReadAsConstantsBitvector<EAffFlag>(bf) : 0;
	for (auto &child: node.Children()) {
		const auto name = child.GetName();
		if (strcmp(name, "duration") == 0) {
			// `cnst`/`level_divisor` renamed to `base`/`skill_divisor`.
			// The bonus is now `min(skill, 75) / skill_divisor` (capped, so monsters can't grow
			// affects unbounded), where `skill` is the spell's potency-roll `base_skill`.
			dur_min_ = parse::ReadAsInt(child.GetValue("min"));
			dur_max_ = parse::ReadAsInt(child.GetValue("max"));
			dur_base_ = parse::ReadAsInt(child.GetValue("base"));
			dur_skill_divisor_ = parse::ReadAsInt(child.GetValue("skill_divisor"));
			// issue.drunked-migration (Gap A): optional skill whose novice-bonus scales the duration -- lets a
			// triggered affect-action (no casting spell) scale like a real cast would.
			if (const char *sk = child.GetValue("skill"); sk && *sk) { dur_skill_ = parse::ReadAsConstant<ESkill>(sk); }
		} else if (strcmp(name, "charges") == 0) {
			// issue.room-affect-trigger-improve: <charges max="N"/> -- how many times a TRIGGERED affect
			// may fire before it is removed. -1 (or absent) = unlimited. Modelled on <hits max=>; the
			// count is deterministic (decremented by one per trigger execution), so no prob attr.
			const char *mx = child.GetValue("max");
			charges_max_ = (mx && *mx) ? parse::ReadAsInt(mx) : -1;
		} else if (strcmp(name, "affect") == 0) {
			// issue.affects-improve (P3e): a bare affect reference -- the canonical grammar. The affect
			// OWNS its applies (affects.xml); the spell only names which affect(s) to impose, plus an
			// optional `random` flag marking it a member of the spell's random-one-of pool. The id may
			// resolve as an EAffect (char affect -> applies_) and/or an ERoomAffect (room affect ->
			// room_affect_); kForbidden is intentionally BOTH, so we try both enums and let each cast
			// path (magic.cpp / magic_rooms.cpp) consume the kind it understands.
			const char *idv = child.GetValue("id");
			if (!idv || !*idv) {
				err_log("talent_actions <affect>: missing id attribute.");
			} else {
				bool resolved = false;
				try {
					Apply apply;
					apply.id = ITEM_BY_NAME<EAffect>(idv);
					apply.location = EApply::kNone;  // the affect owns its location(s)/modifier(s)
					const char *r = child.GetValue("random");
					apply.random = (r && *r) && parse::ReadAsBool(r);
					applies_.push_back(apply);
					resolved = true;
				} catch (const std::out_of_range &) {}
				try {
					room_affect_ = ITEM_BY_NAME<room_spells::ERoomAffect>(idv);
					resolved = true;
				} catch (const std::out_of_range &) {}
				if (!resolved) {
					err_log("talent_actions <affect id='%s'>: unknown affect (not an EAffect or ERoomAffect).",
							idv);
				}
			}
		} else if (strcmp(name, "apply") == 0) {
			// issue.affects-improve: LEGACY spell-owned apply (location + <modifier>). Retained only for
			// affects whose data has not yet moved into affects.xml (the kPoisoned poison family). New
			// data uses the bare <affect id=...> form above; this branch retires once poison migrates.
			Apply apply;
			apply.id = parse::ReadAsConstant<EAffect>(child.GetValue("id"));
			// issue.affects-improve (P3c): location is optional now that affects.xml owns it; a bare
			// <apply id=".."/> ref (no location/modifier) parses as kNone (content comes from the affect).
			const char *loc = child.GetValue("location");
			apply.location = (loc && *loc) ? parse::ReadAsConstant<EApply>(loc) : EApply::kNone;
			// random is optional (default false). parse::ReadAsBool throws on empty input, so
			// guard with the (p && *p) pattern that the other optional attrs use. Without this
			// guard, ANY <apply> tag missing the random= attribute aborts parsing of the whole
			// <affects> block via the exception caught in Actions::Build -- which silently leaves
			// the spell's actions_ empty, so the affect never lands at runtime even though
			// CastAffect's imposition messages still fire.
			const char *r = child.GetValue("random");
			apply.random = (r && *r) && parse::ReadAsBool(r);
			if (child.GoToChild("modifier")) {
				apply.min = parse::ReadAsDouble(child.GetValue("min"));
				{ const char *w = child.GetValue("weight"); apply.weight = (w && *w) ? parse::ReadAsDouble(w) : 0.0; }  // issue.potency-noise (stage 1, additive): weight on the shared draw
				apply.beta = parse::ReadAsDouble(child.GetValue("beta"));
				apply.factor = parse::ReadAsInt(child.GetValue("factor"));
				// stack: optional max stack count, default 1, clamped to a minimum of 1.
				const char *stack = child.GetValue("stack");
				apply.stack = (stack && *stack) ? std::max(1, parse::ReadAsInt(stack)) : 1;
				// cap: optional upper bound on raw magnitude. 0 = no cap.
				// Same (p && *p) guard as random/stack so missing attribute keeps the default.
				const char *cap = child.GetValue("cap");
				apply.cap = (cap && *cap) ? std::max(0, parse::ReadAsInt(cap)) : 0;
				child.GoToParent();
			}
			applies_.push_back(apply);
		} else if (strcmp(name, "lag") == 0) {
			has_lag_ = true;
			lag_base_ = parse::ReadAsInt(child.GetValue("base"));
			lag_bonus_divisor_ = parse::ReadAsDouble(child.GetValue("bonus_divisor"));
		} else if (strcmp(name, "reposition") == 0) {
			has_reposition_ = true;
			const char *p = child.GetValue("pos");
			if (p && *p) {
				reposition_pos_ = parse::ReadAsConstant<EPosition>(p);
			}
			// stop_fight is optional (default false). Same guard as random above; without it,
			// any <reposition pos=...> tag that omits stop_fight= aborts the affect parse.
			const char *sf = child.GetValue("stop_fight");
			reposition_stop_fight_ = (sf && *sf) && parse::ReadAsBool(sf);
		}
	}
}

void TalentAffect::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Affect:" << "\r\n"
		   << " Saving: " << kColorGrn << NAME_BY_ITEM<ESaving>(saving_) << kColorNrm
		   << " Resist: " << kColorGrn << NAME_BY_ITEM<EResist>(resist_) << kColorNrm
		   << " Prob: " << kColorGrn << prob_ << kColorNrm
		   << " potency_weight=" << kColorGrn << potency_weight_ << kColorNrm << "\r\n"
		   << "  Duration: base=" << kColorGrn << dur_base_ << kColorNrm
		   << " skill_divisor=" << kColorGrn << dur_skill_divisor_ << kColorNrm
		   << " min=" << kColorGrn << dur_min_ << kColorNrm
		   << " max=" << kColorGrn << dur_max_ << kColorNrm << "\r\n";
	for (const auto &apply: applies_) {
		buffer << "  Apply: " << kColorGrn << NAME_BY_ITEM<EAffect>(apply.id) << kColorNrm
			   << " -> " << kColorGrn << NAME_BY_ITEM<EApply>(apply.location) << kColorNrm
			   << (apply.random ? " [random]" : "")
			   << " (min=" << apply.min << " beta=" << apply.beta
			   << " factor=" << apply.factor << " stack=" << apply.stack
			   << " cap=" << apply.cap << ")\r\n";
	}
	if (has_lag_) {
		buffer << "  Lag: base=" << kColorGrn << lag_base_ << kColorNrm
			   << " bonus_divisor=" << kColorGrn << lag_bonus_divisor_ << kColorNrm << "\r\n";
	}
	if (has_reposition_) {
		buffer << "  Reposition: pos=" << kColorGrn << NAME_BY_ITEM<EPosition>(reposition_pos_) << kColorNrm
			   << " stop_fight=" << kColorGrn << (reposition_stop_fight_ ? "yes" : "no") << kColorNrm << "\r\n";
	}
}

TalentUnaffect::TalentUnaffect(parser_wrapper::DataNode &node) {
	const char *db = node.GetValue("dispel_bonus");
	dispel_bonus_ = (db && *db) ? parse::ReadAsInt(db) : 50;
	const char *af = node.GetValue("affect_flags");
	affect_flags_ = (af && *af) ? parse::ReadAsConstantsBitvector<EAffFlag>(af)
								 : (kAfCurable | kAfDispellable);
	const char *prob = node.GetValue("prob");
	prob_ = (prob && *prob) ? parse::ReadAsInt(prob) : 100;
	const char *dec = node.GetValue("decay");
	decay_ = (dec && *dec) ? parse::ReadAsInt(dec) : 0;
	// issue.new-unaffect-spells: restrict a wildcard <remove any_of="*"> to DEBUFFS only
	// (so a friendly cleanse like "unweave" never strips the target's own buffs).
	const char *donly = node.GetValue("debuff_only");
	debuff_only_ = (donly && *donly) && parse::ReadAsBool(donly);
	for (auto &child: node.Children()) {
		const auto name = child.GetName();
		Set *set = nullptr;
		if (strcmp(name, "blocking") == 0) {
			set = &blocking_;
		} else if (strcmp(name, "breaking") == 0) {
			set = &breaking_;
		} else if (strcmp(name, "remove_anyway") == 0) {
			set = &remove_anyway_;
		} else if (strcmp(name, "remove") == 0) {
			set = &remove_;
		}
		if (set) {
			ParseRemovalAttr(child.GetValue("any_of"), set->any_of, set->wildcard_any, name, "any_of");
			ParseRemovalAttr(child.GetValue("all_of"), set->all_of, set->wildcard_all, name, "all_of");
			const char *bf = child.GetValue("breaking_by_failure");
			set->breaking_by_failure = (bf && *bf) && parse::ReadAsBool(bf);
		}
	}
}

void TalentUnaffect::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	auto print_set = [&buffer](const char *label, const Set &set) {
		if (set.empty()) {
			return;
		}
		buffer << "  " << label << ":";
		if (set.wildcard_any) {
			buffer << " any_of=" << kColorGrn << "*" << kColorNrm;
		} else if (!set.any_of.empty()) {
			buffer << " any_of=" << kColorGrn;
			for (const auto a: set.any_of.chars) buffer << NAME_BY_ITEM<EAffect>(a) << " ";
			for (const auto r: set.any_of.rooms) buffer << NAME_BY_ITEM<room_spells::ERoomAffect>(r) << " ";
			buffer << kColorNrm;
		}
		if (set.wildcard_all) {
			buffer << " all_of=" << kColorGrn << "*" << kColorNrm;
		} else if (!set.all_of.empty()) {
			buffer << " all_of=" << kColorGrn;
			for (const auto a: set.all_of.chars) buffer << NAME_BY_ITEM<EAffect>(a) << " ";
			for (const auto r: set.all_of.rooms) buffer << NAME_BY_ITEM<room_spells::ERoomAffect>(r) << " ";
			buffer << kColorNrm;
		}
		if (set.breaking_by_failure) {
			buffer << " breaking_by_failure=" << kColorGrn << "yes" << kColorNrm;
		}
		buffer << "\r\n";
	};
	buffer << " Unaffect:" << " dispel_bonus=" << kColorGrn << dispel_bonus_ << kColorNrm
		   << " affect_flags=" << kColorGrn << affect_flags_ << kColorNrm
		   << " decay=" << kColorGrn << decay_ << kColorNrm
		   << " debuff_only=" << kColorGrn << (debuff_only_ ? "yes" : "no") << kColorNrm
		   << " prob=" << kColorGrn << prob_ << kColorNrm << "\r\n";
	print_set("blocking", blocking_);
	print_set("breaking", breaking_);
	print_set("remove_anyway", remove_anyway_);
	print_set("remove", remove_);
}

namespace {
// Print one FlagCondition (mob flags by their kBlockingFlagByName name, affect flags by NAME_BY_ITEM).
void PrintFlagCondition(const char *label, const FlagCondition &cond, std::ostringstream &buffer) {
	if (cond.empty()) {
		return;
	}
	buffer << "  " << label << ":";
	for (const auto flag : cond.mob_flags) {
		std::string flag_name = "?";
		for (const auto &[name, value] : kBlockingFlagByName) {
			if (value == flag) {
				flag_name = name;
				break;
			}
		}
		buffer << " " << kColorGrn << flag_name << kColorNrm;
	}
	for (const auto flag : cond.affect_flags) {
		buffer << " " << kColorGrn << NAME_BY_ITEM<EAffect>(flag) << kColorNrm;
	}
	for (const auto flag : cond.room_flags) {
		std::string flag_name = "?";
		for (const auto &[name, value] : kBlockingRoomFlagByName) {
			if (value == flag) {
				flag_name = name;
				break;
			}
		}
		buffer << " room:" << kColorGrn << flag_name << kColorNrm;
	}
	if (cond.align == EAlign::kGood) {
		buffer << " align=" << kColorGrn << "kGood" << kColorNrm;
	} else if (cond.align == EAlign::kEvil) {
		buffer << " align=" << kColorGrn << "kEvil" << kColorNrm;
	} else if (cond.align == EAlign::kNeutral) {
		buffer << " align=" << kColorGrn << "kNeutral" << kColorNrm;
	}
	buffer << "\r\n";
}
}  // namespace

void Action::Print(CharData *ch, std::ostringstream &buffer) const {
	if (!manual_handler_.empty()) {
		buffer << "  Manual: " << kColorGrn << manual_handler_ << kColorNrm << "\r\n";
	}
	for (const auto s : side_spells_) {
		buffer << "  SideSpell: " << kColorGrn << NAME_BY_ITEM<ESpell>(s) << kColorNrm << "\r\n";
	}
	buffer << "  Target: " << kColorGrn << ActionTargetName(target_) << kColorNrm
		   << " Base: " << kColorGrn << ActionBaseName(base_) << kColorNrm
		   << " Reset: " << kColorGrn << (reset_ ? "Y" : "N") << kColorNrm << "\r\n";
	PrintFlagCondition("Blocking", blocking_, buffer);
	PrintFlagCondition("Required", required_, buffer);
	if (!reflection_.empty()) {
		buffer << "  Reflection: prob=" << kColorGrn << reflection_.prob << kColorNrm;
		for (const auto aff : reflection_.affect_flags) {
			buffer << " " << kColorGrn << NAME_BY_ITEM<EAffect>(aff) << kColorNrm;
		}
		if (reflection_.align == EAlign::kGood) {
			buffer << " align=" << kColorGrn << "kGood" << kColorNrm;
		} else if (reflection_.align == EAlign::kEvil) {
			buffer << " align=" << kColorGrn << "kEvil" << kColorNrm;
		} else if (reflection_.align == EAlign::kNeutral) {
			buffer << " align=" << kColorGrn << "kNeutral" << kColorNrm;
		}
		buffer << "\r\n";
	}
	for (const auto &effect: manifestations_) {
		effect.second->Print(ch, buffer);
	}
}

void Actions::Print(CharData *ch, std::ostringstream &buffer) const {
	for (size_t i = 0; i < list_.size(); ++i) {
		if (list_.size() > 1) {
			buffer << " Action " << kColorGrn << (i + 1) << kColorNrm << ":\r\n";
		}
		list_[i].Print(ch, buffer);
	}
}

void Actions::Build(parser_wrapper::DataNode &node) {
	std::vector<Action> list;
	for (auto &action: node.Children()) {
		try {
			Action parsed;
			ParseAction(parsed, action);
			list.push_back(std::move(parsed));
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
			return;
		}
	}
	list_ = std::move(list);
}

/*
 *  Парсеры воздействий
 */

// Parse a <blocking>/<required>/<caster_blocking> tag. Schema: child tags <mob_flags val=>,
// <affect_flags val=>, <room_flags val=>, <align val=>, each with a `|`-separated value
// (or a single token for <align>). Accumulates into the given condition. The child-tag
// form replaced the previous attribute-list form so the line
// stays readable when several axes are combined (kNoMagic blocking now lives on most spells).
void Actions::ParseFlagCondition(FlagCondition &cond, parser_wrapper::DataNode &node) {
	for (auto &child : node.Children()) {
		const auto name = child.GetName();
		const char *val = child.GetValue("val");
		if (!val || !*val) {
			continue;
		}
		if (strcmp(name, "mob_flags") == 0) {
			for (const auto &flag_name : utils::Split(val, '|')) {
				const auto it = kBlockingFlagByName.find(flag_name);
				if (it != kBlockingFlagByName.end()) {
					cond.mob_flags.push_back(it->second);
				} else {
					err_log("Actions: unknown EMobFlag '%s' in <mob_flags>.", flag_name.c_str());
				}
			}
		} else if (strcmp(name, "affect_flags") == 0) {
			for (const auto &flag_name : utils::Split(val, '|')) {
				cond.affect_flags.push_back(parse::ReadAsConstant<EAffect>(flag_name.c_str()));
			}
		} else if (strcmp(name, "room_flags") == 0) {
			// ERoomFlag: blocking by the CASTER's current room flags.
			// Drives the kNoMagic blocking for all non-warcry spells and per-spell
			// blockers like kRuneLabel's kPeaceful/kTunnel/kNoTeleportIn.
			for (const auto &flag_name : utils::Split(val, '|')) {
				const auto it = kBlockingRoomFlagByName.find(flag_name);
				if (it != kBlockingRoomFlagByName.end()) {
					cond.room_flags.push_back(it->second);
				} else {
					err_log("Actions: unknown ERoomFlag '%s' in <room_flags>.", flag_name.c_str());
				}
			}
		} else if (strcmp(name, "align") == 0) {
			// kGood / kEvil / kNeutral; absent = no alignment check.
			if (strcmp(val, "kGood") == 0) {
				cond.align = EAlign::kGood;
			} else if (strcmp(val, "kEvil") == 0) {
				cond.align = EAlign::kEvil;
			} else if (strcmp(val, "kNeutral") == 0) {
				cond.align = EAlign::kNeutral;
			} else {
				err_log("Actions: unknown EAlign '%s' in <align>.", val);
			}
		}
	}
}

// <target_conditions>: wraps the action's <blocking> and
// <required> blocks. Each is a FlagCondition filled by ParseFlagCondition. Strict:
// only these two children are recognised; bare <blocking>/<required> directly under
// <action> is no longer accepted (the XML was migrated in the same issue).
void Actions::ParseTargetConditions(Action &out, parser_wrapper::DataNode &node) {
	for (auto &child : node.Children()) {
		if (strcmp(child.GetName(), "blocking") == 0) {
			ParseFlagCondition(out.blocking_, child);
		} else if (strcmp(child.GetName(), "required") == 0) {
			ParseFlagCondition(out.required_, child);
		}
	}
}

// <caster_conditions>: spell-level caster gate.
// <blocking>/<required> sections, each with <align>/<affect_flags> child tags --
// reuses ParseFlagCondition (it honours those axes; mob_flags/room_flags are simply
// not used for casters today). Lives at SpellInfo level, parsed by SpellInfoBuilder.
void Actions::ParseCasterConditions(CasterConditions &out, parser_wrapper::DataNode &node) {
	for (auto &child : node.Children()) {
		if (strcmp(child.GetName(), "blocking") == 0) {
			ParseFlagCondition(out.blocking, child);
		} else if (strcmp(child.GetName(), "required") == 0) {
			ParseFlagCondition(out.required, child);
		}
	}
}

void Actions::ParseAction(Action &out, parser_wrapper::DataNode node) {
	// optional per-action target selector (non-first actions). Default kTarSame.
	if (const char *tgt = node.GetValue("target"); tgt && *tgt) {
		const auto it = kActionTargetByName.find(tgt);
		if (it != kActionTargetByName.end()) {
			out.target_ = it->second;
		} else {
			err_log("Actions: unknown <action target='%s'>.", tgt);
		}
	}
	// optional formula base + reset (non-first actions). Default kCompetence/false.
	const char *bs = node.GetValue("base");
	if (bs && *bs) {
		if (strcmp(bs, "kDamage") == 0) { out.base_ = EActionBase::kDamage; }
		else if (strcmp(bs, "kPoints") == 0) { out.base_ = EActionBase::kPoints; }
		else if (strcmp(bs, "kAffects") == 0) { out.base_ = EActionBase::kAffects; }
		else if (strcmp(bs, "kDispelled") == 0) { out.base_ = EActionBase::kDispelled; }
		else if (strcmp(bs, "kCompetence") == 0) { out.base_ = EActionBase::kCompetence; }
		// issue.character-affect-triggers: base="tag" tag="NAME" -- the formula base is the event tag NAME
		// (read off the ActionContext's EventContext, cast to a number).
		else if (strcmp(bs, "tag") == 0) {
			out.base_ = EActionBase::kTag;
			const char *tn = node.GetValue("tag");
			if (tn && *tn) { out.tag_name_ = tn; }
			else { err_log("Actions: <action base='tag'> without a tag='NAME'."); }
		}
		else { err_log("Actions: unknown <action base='%s'>.", bs); }
	}
	const char *rs = node.GetValue("reset");
	out.reset_ = (rs && (strcmp(rs, "true") == 0 || strcmp(rs, "Y") == 0 || strcmp(rs, "1") == 0));
	// issue.perk-action-patching: optional <action id="Name"> -- the block's stable id for perk patches.
	if (const char *idv = node.GetValue("id"); idv && *idv) { out.id_ = idv; }
	for (auto &manifestation: node.Children()) {
		if (strcmp(manifestation.GetName(), "damage") == 0) {
			ParseDamage(out, manifestation);
		} else if (strcmp(manifestation.GetName(), "area") == 0) {
			ParseArea(out, manifestation);
		} else if (strcmp(manifestation.GetName(), "summon") == 0) {
			ParseSummon(out, manifestation);
		} else if (strcmp(manifestation.GetName(), "obj_creation") == 0) {
			ParseCreation(out, manifestation);
		} else if (strcmp(manifestation.GetName(), "alter_obj") == 0) {
			ParseAlterObj(out, manifestation);
		} else if (strcmp(manifestation.GetName(), "points") == 0) {
			ParsePoints(out, manifestation);
		} else if (strcmp(manifestation.GetName(), "affects") == 0) {
			ParseAffect(out, manifestation);
		} else if (strcmp(manifestation.GetName(), "unaffect") == 0) {
			ParseUnaffect(out, manifestation);
		} else if (strcmp(manifestation.GetName(), "target_conditions") == 0) {
			ParseTargetConditions(out, manifestation);
		} else if (strcmp(manifestation.GetName(), "reflection") == 0) {
			ParseReflection(out.reflection_, manifestation);
		} else if (strcmp(manifestation.GetName(), "absorption") == 0) {
			ParseAbsorption(out.absorption_, manifestation);
		} else if (strcmp(manifestation.GetName(), "damage_change") == 0) {
			ParseDamageChange(out.damage_change_, manifestation);
		} else if (strcmp(manifestation.GetName(), "retaliation") == 0) {
			ParseRetaliation(out.retaliation_, manifestation);
		} else if (strcmp(manifestation.GetName(), "manual_cast") == 0) {
			// <manual_cast handler="SpellX"/>.
			const char *hv = manifestation.GetValue("handler");
			if (hv && *hv) { out.manual_handler_ = hv; }
		} else if (strcmp(manifestation.GetName(), "side_spell") == 0) {
			// <side_spell id="kHold"/>.
			const char *idv = manifestation.GetValue("id");
			if (idv && *idv) { out.side_spells_.push_back(parse::ReadAsConstant<ESpell>(idv)); }
		} else if (strcmp(manifestation.GetName(), "trigger") == 0) {
			// issue.affect-migration: <trigger val="kPulse|kBattlePulse"/> -- the event(s) that fire
			// this action. No trigger => the action runs inline in the normal cast.
			// issue.room-affect-trigger-improve: kEnter/kEnterPC fire on a character entering the room;
			// the optional return="N" int is the value the trigger yields the firing event (0 = block).
			const char *tv = manifestation.GetValue("val");
			if (tv && *tv) {
				// `|`-separated -> several flags; an action can fire on more than one event.
				for (const auto &name : utils::Split(tv, '|')) {
					const auto it = kActionTriggerByName.find(name);
					if (it != kActionTriggerByName.end()) {
						out.trigger_.set(it->second);
					} else {
						err_log("Actions: unknown <trigger val='%s'>.", name.c_str());
					}
				}
			}
			if (const char *rv = manifestation.GetValue("return"); rv && *rv) {
				out.trigger_return_ = parse::ReadAsInt(rv);
			}
			// issue.character-affect-triggers: optional <trigger category="kHeal"/> for kPoints -- restrict
			// the trigger to one restoration category. Unknown name => no filter (react to any).
			if (const char *cv = manifestation.GetValue("category"); cv && *cv) {
				const auto it = kPointsCategoryByName.find(cv);
				if (it != kPointsCategoryByName.end()) {
					out.trigger_points_category_ = it->second;
				} else {
					err_log("Actions: unknown <trigger category='%s'> (kHeal|kMoves|kThirst|kFull).", cv);
				}
			}
			if (const char *pv = manifestation.GetValue("prob"); pv && *pv) {
				out.trigger_prob_ = parse::ReadAsInt(pv);
			}
		}
	}
	node.GoToParent();
}

// <reflection affect_flags="..."  align="kGood|kEvil"  prob="N"/>.
// prob defaults to 20 if the attribute is absent; align defaults to kAny (no alignment check).
void Actions::ParseReflection(Reflection &refl, parser_wrapper::DataNode &node) {
	const char *aff = node.GetValue("affect_flags");
	if (aff && *aff) {
		for (const auto &flag_name : utils::Split(aff, '|')) {
			refl.affect_flags.push_back(parse::ReadAsConstant<EAffect>(flag_name.c_str()));
		}
	}
	const char *align = node.GetValue("align");
	if (align && *align) {
		if (strcmp(align, "kGood") == 0) {
			refl.align = EAlign::kGood;
		} else if (strcmp(align, "kEvil") == 0) {
			refl.align = EAlign::kEvil;
		} else if (strcmp(align, "kNeutral") == 0) {
			refl.align = EAlign::kNeutral;
		} else {
			err_log("Actions: unknown EAlign '%s' in <reflection align>.", align);
		}
	}
	const char *prob = node.GetValue("prob");
	if (prob && *prob) {
		refl.prob = parse::ReadAsInt(prob);
	}
	// issue.attack-ward: potency-contest bounds (max>0 => contest mode in RunAttackWards, else fixed prob).
	const char *mn = node.GetValue("min");
	if (mn && *mn) { refl.min = parse::ReadAsInt(mn); }
	const char *mx = node.GetValue("max");
	if (mx && *mx) { refl.max = parse::ReadAsInt(mx); }
	refl.present = true;   // issue.attack-ward: marks a defender Magic-Mirror ward (vs a default Reflection)
}

// issue.attack-ward: <absorption scope="all|damage|affect" [chance="kMagicResist"] [prob="N"]/>.
// chance names a resist apply whose capped GET_<apply>(target) is the absorb % (scales with the stat);
// when absent, the fixed prob is used. scope defaults to all; prob to 0.
static EWardScope ParseWardScope(const char *scope, const char *ctx) {
	if (!scope || !*scope || strcmp(scope, "all") == 0) { return EWardScope::kAll; }
	if (strcmp(scope, "damage") == 0) { return EWardScope::kDamage; }
	if (strcmp(scope, "affect") == 0) { return EWardScope::kAffect; }
	err_log("Actions: unknown <%s scope='%s'> (all|damage|affect).", ctx, scope);
	return EWardScope::kAll;
}

void Actions::ParseAbsorption(Absorption &absorb, parser_wrapper::DataNode &node) {
	absorb.present = true;
	absorb.scope = ParseWardScope(node.GetValue("scope"), "absorption");
	const char *chance = node.GetValue("chance");
	if (chance && *chance) {
		absorb.chance = parse::ReadAsConstant<EApply>(chance);
	}
	const char *prob = node.GetValue("prob");
	if (prob && *prob) {
		absorb.prob = parse::ReadAsInt(prob);
	}
}

void Actions::ParseDamageChange(DamageChange &dc, parser_wrapper::DataNode &node) {
	dc.present = true;
	if (const char *p = node.GetValue("prob"); p && *p) { dc.prob = parse::ReadAsInt(p); }
	if (const char *s = node.GetValue("stage"); s && strcmp(s, "late") == 0) { dc.late = true; }
	if (const char *m = node.GetValue("msg"); m && *m) {
		if (strcmp(m, "crit") == 0) { dc.msg_variant = 1; }
		else if (strcmp(m, "none") == 0) { dc.msg_variant = 2; }
	}
	for (auto &child : node.Children()) {
		const auto cn = child.GetName();
		if (strcmp(cn, "conditions") == 0) {
			for (auto &c : child.Children()) {
				const auto n = c.GetName();
				if (strcmp(n, "type") == 0) {
					dc.type_mask |= ParseDmgTypeMask(c.GetValue("val"));
				} else if (strcmp(n, "element") == 0) {
					dc.element_mask |= ParseElementMask(c.GetValue("val"));
				} else if (strcmp(n, "flags") == 0) {
					dc.flags_present |= ParseHitFlagMask(c.GetValue("present"));
					dc.flags_missing |= ParseHitFlagMask(c.GetValue("missing"));
				}
			}
		} else if (strcmp(cn, "variation") == 0) {
			if (const char *v = child.GetValue("min"); v && *v) { dc.var_min = parse::ReadAsInt(v); }
			if (const char *v = child.GetValue("max"); v && *v) { dc.var_max = parse::ReadAsInt(v); }
			if (const char *v = child.GetValue("factor"); v && *v) { dc.var_factor = parse::ReadAsInt(v); }
		} else if (strcmp(cn, "flags") == 0) {
			dc.flags_add |= ParseHitFlagMask(child.GetValue("add"));
			dc.flags_remove |= ParseHitFlagMask(child.GetValue("remove"));
		}
	}
}

void Actions::ParseRetaliation(Retaliation &rt, parser_wrapper::DataNode &node) {
	rt.present = true;
	if (const char *p = node.GetValue("prob"); p && *p) { rt.prob = parse::ReadAsInt(p); }
	for (auto &child : node.Children()) {
		const auto cn = child.GetName();
		if (strcmp(cn, "conditions") == 0) {
			for (auto &c : child.Children()) {
				const auto n = c.GetName();
				if (strcmp(n, "type") == 0) {
					rt.type_mask |= ParseDmgTypeMask(c.GetValue("val"));
				} else if (strcmp(n, "element") == 0) {
					rt.element_mask |= ParseElementMask(c.GetValue("val"));
				} else if (strcmp(n, "flags") == 0) {
					rt.flags_present |= ParseHitFlagMask(c.GetValue("present"));
					rt.flags_missing |= ParseHitFlagMask(c.GetValue("missing"));
				}
			}
		} else if (strcmp(cn, "percent") == 0) {
			if (const char *v = child.GetValue("min"); v && *v) { rt.pct_min = parse::ReadAsInt(v); }
			if (const char *v = child.GetValue("max"); v && *v) { rt.pct_max = parse::ReadAsInt(v); }
			if (const char *v = child.GetValue("npc_bonus"); v && *v) { rt.npc_bonus = parse::ReadAsInt(v); }
			if (const char *v = child.GetValue("boss_bonus"); v && *v) { rt.boss_bonus = parse::ReadAsInt(v); }
		} else if (strcmp(cn, "element") == 0) {
			// The reflected damage's element/type. Absent => same as the incoming hit (-1 sentinel).
			if (const char *v = child.GetValue("val"); v && *v) {
				rt.element = static_cast<int>(ITEM_BY_NAME<EElement>(v));
			}
			if (const char *v = child.GetValue("type"); v && *v) {
				const auto it = kDmgTypeByName.find(v);
				if (it != kDmgTypeByName.end()) { rt.dmg_type = static_cast<int>(it->second); }
				else { err_log("Actions: unknown <retaliation><element type='%s'>.", v); }
			}
		} else if (strcmp(cn, "flags") == 0) {
			rt.flags_add |= ParseHitFlagMask(child.GetValue("add"));
			rt.flags_remove |= ParseHitFlagMask(child.GetValue("remove"));
		}
	}
}

void Actions::ParseDamage(Action &out, parser_wrapper::DataNode &node) {
	auto it = out.manifestations_.emplace(EAction::kDamage, std::make_shared<Damage>(node));
	if (const char *idv = node.GetValue("id"); idv && *idv) { it->second->id = idv; }
}

void Actions::ParseArea(Action &out, parser_wrapper::DataNode &node) {
	auto ptr = std::make_shared<Area>();
	if (node.GoToChild("targets")) {
		// Every attribute is optional: a missing one must keep the Area member default, NOT abort
		// the whole action parse. ReadAsInt/ReadAsDouble throw runtime_error("") on an empty string
		// (std::stoi/stod on ""), and that exception is swallowed by Actions::Build, which then
		// drops the spell's entire action list -- so an absent `stat_weight` silently disabled every
		// area/mass/group spell. Guard each read like Damage/Affect do.
		const char *sd = node.GetValue("skill_divisor");
		if (sd && *sd) { ptr->skill_divisor = std::max(1, parse::ReadAsInt(sd)); }
		const char *ds = node.GetValue("dice_size");
		if (ds && *ds) { ptr->dice_size = std::max(1, parse::ReadAsInt(ds)); }
		const char *mn = node.GetValue("min");
		if (mn && *mn) { ptr->min_targets = std::max(1, parse::ReadAsInt(mn)); }
		// max <= 0 means "no upper limit" (hit every target in the roster); don't clamp it.
		const char *mx = node.GetValue("max");
		if (mx && *mx) { ptr->max_targets = parse::ReadAsInt(mx); }
		// stat_weight defaults to 0 (exactly the historical count) and is absent everywhere today.
		const char *sw = node.GetValue("stat_weight");
		if (sw && *sw) { ptr->stat_weight = parse::ReadAsDouble(sw); }
		node.GoToParent();
	}
	if (node.GoToChild("distribution")) {
		const char *type = node.GetValue("type");
		if (type && strcmp(type, "kLinear") == 0) {
			ptr->distribution = EAreaDistribution::kLinear;
		} else if (type && strcmp(type, "kStepped") == 0) {
			ptr->distribution = EAreaDistribution::kStepped;
		} else {
			ptr->distribution = EAreaDistribution::kUniform;
		}
		// decay / free_targets matter only for kStepped and are absent otherwise (e.g. kUniform);
		// guard them so a missing attribute keeps the default instead of throwing.
		const char *dc = node.GetValue("decay");
		if (dc && *dc) { ptr->decay = parse::ReadAsDouble(dc); }
		const char *ft = node.GetValue("free_targets");
		if (ft && *ft) { ptr->free_targets = std::max(0, parse::ReadAsInt(ft)); }
		node.GoToParent();
	}
	auto it = out.manifestations_.insert({EAction::kArea, std::move(ptr)});
	if (const char *idv = node.GetValue("id"); idv && *idv) { it->second->id = idv; }
}

void Actions::ParseSummon(Action &out, parser_wrapper::DataNode &node) {
	auto ptr = std::make_shared<Summon>();
	const char *bf = node.GetValue("base_fail");
	if (bf && *bf) { ptr->base_fail = parse::ReadAsInt(bf); }
	const char *mf = node.GetValue("min_fail");
	if (mf && *mf) { ptr->min_fail = parse::ReadAsInt(mf); }
	const char *h = node.GetValue("handler");
	if (h && *h) { ptr->handler = h; }
	if (node.GoToChild("mob")) {
		const char *vn = node.GetValue("vnum");
		if (vn && *vn) { ptr->mob_vnum = parse::ReadAsInt(vn); }
		const char *cw = node.GetValue("competence_weight");
		if (cw && *cw) { ptr->competence_weight = parse::ReadAsDouble(cw); }
		const char *ex = node.GetValue("extra");
		ptr->extra = (ex && *ex) && parse::ReadAsBool(ex);
		const char *kp = node.GetValue("keeper");
		ptr->keeper = (kp && *kp) && parse::ReadAsBool(kp);
		const char *fc = node.GetValue("from_corpse");
		ptr->from_corpse = (fc && *fc) && parse::ReadAsBool(fc);
		node.GoToParent();
	}
	auto it = out.manifestations_.insert({EAction::kSummon, std::move(ptr)});
	if (const char *idv = node.GetValue("id"); idv && *idv) { it->second->id = idv; }
}

void Actions::ParseCreation(Action &out, parser_wrapper::DataNode &node) {
	auto ptr = std::make_shared<Creation>();
	const char *vn = node.GetValue("vnum");
	if (vn && *vn) { ptr->vnum = parse::ReadAsInt(vn); }
	const char *h = node.GetValue("handler");
	if (h && *h) { ptr->handler = h; }
	auto it = out.manifestations_.insert({EAction::kCreation, std::move(ptr)});
	if (const char *idv = node.GetValue("id"); idv && *idv) { it->second->id = idv; }
}

void Actions::ParseAlterObj(Action &out, parser_wrapper::DataNode &node) {
	auto ptr = std::make_shared<AlterObj>();
	const char *h = node.GetValue("handler");
	if (h && *h) { ptr->handler = h; }
	const char *col = node.GetValue("collateral");
	ptr->collateral_on_damage = (col && strcmp(col, "on_damage") == 0);
	auto it = out.manifestations_.insert({EAction::kAlterObj, std::move(ptr)});
	if (const char *idv = node.GetValue("id"); idv && *idv) { it->second->id = idv; }
}

void Actions::ParsePoints(Action &out, parser_wrapper::DataNode &node) {
	auto it = out.manifestations_.emplace(EAction::kPoints, std::make_shared<Points>(node));
	if (const char *idv = node.GetValue("id"); idv && *idv) { it->second->id = idv; }
}

void Actions::ParseAffect(Action &out, parser_wrapper::DataNode &node) {
	auto it = out.manifestations_.emplace(EAction::kAffect, std::make_shared<TalentAffect>(node));
	if (const char *idv = node.GetValue("id"); idv && *idv) { it->second->id = idv; }
}

void Actions::ParseUnaffect(Action &out, parser_wrapper::DataNode &node) {
	auto it = out.manifestations_.emplace(EAction::kUnaffect, std::make_shared<TalentUnaffect>(node));
	if (const char *idv = node.GetValue("id"); idv && *idv) { it->second->id = idv; }
}

/*
 *  Геттеры эффектов
 *  Area и Dmg возвпращаются только по одному, потому что для обраьотки их списка надо переписывать всю логику
 *  работы с арекастами, а даже, скорее, переносить ее внутрь эффекта (как сделано с Applies).
 */

// ---- Action: per-action manifestation accessors (the real logic) ----

bool Action::Contains(EAction action) const {
	// kManual / kSideSpell have no IAction in manifestations_ -- they are carried as a handler
	// name / a spell list, so answer from those fields instead of the manifestation map.
	if (action == EAction::kManual) {
		return !manual_handler_.empty();
	}
	if (action == EAction::kSideSpell) {
		return !side_spells_.empty();
	}
	return manifestations_.contains(action);
}

// issue.perk-action-patching: op="modify" support -- replace this block's shared manifestation of `kind`
// with a private deep copy and hand it back mutable. Only the kinds modify actually edits are cloned.
// issue.affect-action-patch-improve (Phase B): the mul/add/set arithmetic shared by ApplyFieldMod overrides.
static double ApplyModOp(double cur, EFieldModOp op, double v) {
	switch (op) {
		case EFieldModOp::kMul: return cur * v;
		case EFieldModOp::kAdd: return cur + v;
		case EFieldModOp::kSet: return v;
	}
	return cur;
}

void Area::ApplyFieldMod(const std::string &field, EFieldModOp op, double value) {
	if (field == "decay") { decay = ApplyModOp(decay, op, value); }
	else if (field == "free_targets") { free_targets = static_cast<int>(ApplyModOp(free_targets, op, value)); }
	else if (field == "max_targets") { max_targets = static_cast<int>(ApplyModOp(max_targets, op, value)); }
	else if (field == "min_targets") { min_targets = static_cast<int>(ApplyModOp(min_targets, op, value)); }
	else if (field == "skill_divisor") { skill_divisor = std::max(1, static_cast<int>(ApplyModOp(skill_divisor, op, value))); }
	else { err_log("ApplyFieldMod: unknown Area field [%s].", field.c_str()); }
}

void Damage::ApplyFieldMod(const std::string &field, EFieldModOp op, double value) {
	if (field == "beta") { amount_beta_ = ApplyModOp(amount_beta_, op, value); }
	else if (field == "min") { amount_min_ = ApplyModOp(amount_min_, op, value); }
	else if (field == "prob") { prob_ = static_cast<int>(ApplyModOp(prob_, op, value)); }
	else if (field == "hits_max") { hits_max_ = static_cast<int>(ApplyModOp(hits_max_, op, value)); }
	else { err_log("ApplyFieldMod: unknown Damage field [%s].", field.c_str()); }
}

IAction *Action::CloneManifestation(EAction kind) {
	auto it = manifestations_.find(kind);
	if (it == manifestations_.end() || !it->second) {
		return nullptr;
	}
	switch (kind) {
		case EAction::kArea:
			it->second = std::make_shared<Area>(*std::static_pointer_cast<Area>(it->second));
			break;
		case EAction::kDamage:
			it->second = std::make_shared<Damage>(*std::static_pointer_cast<Damage>(it->second));
			break;
		default:
			return nullptr;   // kind not supported by op="modify" yet
	}
	return it->second.get();
}

const Damage &Action::GetDmg() const {
	if (manifestations_.contains(EAction::kDamage)) {
		return *std::static_pointer_cast<Damage>(manifestations_.find(EAction::kDamage)->second);
	} else {
		throw std::runtime_error("Getting damage parameters from action which has no 'damage'.");
	}
}

const Area &Action::GetArea() const {
	if (manifestations_.contains(EAction::kArea)) {
		return *std::static_pointer_cast<Area>(manifestations_.find(EAction::kArea)->second);
	} else {
		throw std::runtime_error("Getting area parameters from action which has no 'area'.");
	}
}

const Points &Action::GetPoints() const {
	if (manifestations_.contains(EAction::kPoints)) {
		return *std::static_pointer_cast<Points>(manifestations_.find(EAction::kPoints)->second);
	} else {
		throw std::runtime_error("Getting points parameters from action which has no 'points'.");
	}
}

const TalentAffect &Action::GetAffect() const {
	if (manifestations_.contains(EAction::kAffect)) {
		return *std::static_pointer_cast<TalentAffect>(manifestations_.find(EAction::kAffect)->second);
	} else {
		throw std::runtime_error("Getting affect parameters from action which has no 'affects'.");
	}
}

const TalentUnaffect &Action::GetUnaffect() const {
	if (manifestations_.contains(EAction::kUnaffect)) {
		return *std::static_pointer_cast<TalentUnaffect>(manifestations_.find(EAction::kUnaffect)->second);
	} else {
		throw std::runtime_error("Getting unaffect parameters from action which has no 'unaffect'.");
	}
}

// ---- Actions: back-compat single-action API, delegates to the first action ----
// (Every spell has exactly one action today. EmptyAction() supplies the old
// "default-constructed gates / no manifestation" behaviour for spells with none.)

const Action &Actions::EmptyAction() {
	static const Action empty{};
	return empty;
}

bool Actions::Contains(EAction action) const {
	return !list_.empty() && list_.front().Contains(action);
}

const Damage &Actions::GetDmg() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetDmg();
}

const Area &Actions::GetArea() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetArea();
}

const Summon &Action::GetSummon() const {
	if (manifestations_.contains(EAction::kSummon)) {
		return *std::static_pointer_cast<Summon>(manifestations_.find(EAction::kSummon)->second);
	} else {
		throw std::runtime_error("Getting summon parameters from action which has no 'summon'.");
	}
}

const Summon &Actions::GetSummon() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetSummon();
}

const Creation &Action::GetCreation() const {
	if (manifestations_.contains(EAction::kCreation)) {
		return *std::static_pointer_cast<Creation>(manifestations_.find(EAction::kCreation)->second);
	} else {
		throw std::runtime_error("Getting creation parameters from action which has no 'obj_creation'.");
	}
}

const Creation &Actions::GetCreation() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetCreation();
}

const AlterObj &Action::GetAlterObj() const {
	if (manifestations_.contains(EAction::kAlterObj)) {
		return *std::static_pointer_cast<AlterObj>(manifestations_.find(EAction::kAlterObj)->second);
	} else {
		throw std::runtime_error("Getting alter_obj parameters from action which has no 'alter_obj'.");
	}
}

const AlterObj &Actions::GetAlterObj() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetAlterObj();
}

const Points &Actions::GetPoints() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetPoints();
}

const TalentAffect &Actions::GetAffect() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetAffect();
}

const TalentUnaffect &Actions::GetUnaffect() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetUnaffect();
}

const FlagCondition &Actions::GetBlocking() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetBlocking();
}

const FlagCondition &Actions::GetRequired() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetRequired();
}

const Reflection &Actions::GetReflection() const {
	return (list_.empty() ? EmptyAction() : list_.front()).GetReflection();
}

}
