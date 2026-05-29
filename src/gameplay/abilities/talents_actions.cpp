#include "talents_actions.h"

#include "engine/ui/color.h"
#include "engine/core/handler.h"  // EFind (issue.spellcomponents)
#include "engine/entities/char_data.h"
#include "gameplay/magic/spells_constants.h"
#include "fmt/format.h"
#include "utils/random.h"

#include <algorithm>
#include <map>

namespace talents_actions {

namespace {
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
	{"kNoBattle", ERoomFlag::kNoBattle},
	{"kGodsRoom", ERoomFlag::kGodsRoom},
	{"kForMono", ERoomFlag::kForMono},
	{"kForPoly", ERoomFlag::kForPoly},
};

// Parse a `|`-separated list of ESpell names (an any_of/all_of attribute of an
// <unaffect> sub-tag, issue #3342). Empty/absent attribute -> empty list. The
// single-token "*" is the wildcard (handled by the caller, not this helper).
std::vector<ESpell> ParseSpellList(const char *value) {
	std::vector<ESpell> out;
	if (!value || !*value) {
		return out;
	}
	for (const auto &name : utils::Split(value, '|')) {
		try {
			out.push_back(parse::ReadAsConstant<ESpell>(name.c_str()));
		} catch (...) {
			err_log("TalentUnaffect: unknown ESpell '%s' in <unaffect>.", name.c_str());
		}
	}
	return out;
}

// Parse one any_of/all_of attribute, recognising the "*" wildcard token. On a wildcard,
// out_wildcard is set true and the explicit list stays empty. Mixing wildcard with explicit
// names (e.g. "*|kPoison") is rejected with an error: the semantic is ambiguous and the
// explicit names are redundant if "*" is present.
void ParseRemovalAttr(const char *value, std::vector<ESpell> &out_list, bool &out_wildcard,
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
	out_list = ParseSpellList(value);
}
}  // namespace

void Roll::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Potency roll: " << "\r\n"
		   << kColorGrn << "  " << dice_num_
		   << "d" << dice_size_
		   << "+" << dice_add_ << kColorNrm
		   << " Low skill bonus: " << kColorGrn << low_skill_bonus_ << kColorNrm
		   << " Hi skill bonus: " << kColorGrn << hi_skill_bonus_ << kColorNrm << "\r\n";
}

int Roll::RollSkillDices() const {
	return RollDices(dice_num_, dice_size_) + dice_add_;
}

double Roll::CalcSkillCoeff(const CharData *const ch) const {
	auto skill = ch->GetSkill(base_skill_);
	auto low_skill = std::min(skill, abilities::kNoviceSkillThreshold);
	auto hi_skill = std::max(0, skill - abilities::kNoviceSkillThreshold);
	return (low_skill * low_skill_bonus_ + hi_skill * hi_skill_bonus_) / 100.0;
}

double Roll::CalcLowSkillCoeff(const CharData *const ch) const {
	auto low_skill = std::min(ch->GetSkill(base_skill_), abilities::kNoviceSkillThreshold);
	return low_skill * low_skill_bonus_;
}

double Roll::CalcBaseStatCoeff(const CharData *const ch) const {
	return std::max(0, GetRealBaseStat(ch, base_stat_) - base_stat_threshold_) * base_stat_weight_ / 100.0;
}

Roll::Roll(parser_wrapper::DataNode &node) {
	if (node.GoToChild("dices")) {
		// issue.dicerolls: no clamp-to-1. ndice=0 or sdice=0 means "no dice rolled", so
		// <dices ndice="0" sdice="0" adice="N"/> reliably returns N. The previous std::max(1, ...)
		// silently added one to every all-zero spec, which violated the principle of least
		// surprise. RollDices(0, *) and RollDices(*, 0) already short-circuit to 0, so the
		// arithmetic stays correct without any extra guard here.
		dice_num_ = parse::ReadAsInt(node.GetValue("ndice"));
		dice_size_ = parse::ReadAsInt(node.GetValue("sdice"));
		dice_add_ = parse::ReadAsInt(node.GetValue("adice"));
		node.GoToParent();
	}

	if (node.GoToChild("base_skill")) {
		base_skill_ = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
		low_skill_bonus_ = parse::ReadAsDouble(node.GetValue("low_skill_bonus"));
		hi_skill_bonus_ = parse::ReadAsDouble(node.GetValue("hi_skill_bonus"));
		node.GoToParent();
	}

	if (node.GoToChild("base_stat")) {
		base_stat_ = parse::ReadAsConstant<EBaseStat>(node.GetValue("id"));
		base_stat_threshold_ = parse::ReadAsInt(node.GetValue("threshold"));
		base_stat_weight_ = parse::ReadAsDouble(node.GetValue("weight"));
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

// Parses a <components>...</components> spell-level block (issue.spellcomponents).
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
		// CastSpell, process_player_attack and SaySpell consult HasVerbal().
		if (strcmp(name, "verbal") == 0) {
			has_verbal_ = true;
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
		// charge cost. (issue.spellcomponents.)
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
	if (materials_.empty() && !has_verbal_) {
		return;
	}
	buffer << "Components:\r\n";
	if (has_verbal_) {
		buffer << "  Verbal" << "\r\n";
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
		   << " dices_weight: " << kColorGrn << amount_dices_weight_ << kColorNrm
		   << " competencies_weight: " << kColorGrn << amount_competencies_weight_ << kColorNrm << "\r\n";
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
	// <amount> is optional; absent -> keep the defaults (min 0, both weights 1.0). A present tag
	// may still omit individual attributes, which fall back to those same defaults.
	if (node.GoToChild("amount")) {
		const char *amin = node.GetValue("min");
		amount_min_ = (amin && *amin) ? parse::ReadAsDouble(amin) : 0.0;
		const char *adw = node.GetValue("dices_weight");
		amount_dices_weight_ = (adw && *adw) ? parse::ReadAsDouble(adw) : 1.0;
		const char *acw = node.GetValue("competencies_weight");
		amount_competencies_weight_ = (acw && *acw) ? parse::ReadAsDouble(acw) : 1.0;
		node.GoToParent();
	}
	// <hits> is optional (issue.extra-hits); absent -> the spell deals one hit. Individual attrs
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
}

Heal::Heal(parser_wrapper::DataNode &node) {
	const char *npc = node.GetValue("npc_coeff");
	npc_coeff_ = (npc && *npc) ? parse::ReadAsDouble(npc) : 1.0;
	const char *extra = node.GetValue("extra");
	extra_ = (extra && *extra) && parse::ReadAsBool(extra);
	const char *prob = node.GetValue("prob");
	prob_ = (prob && *prob) ? parse::ReadAsInt(prob) : 100;
	if (node.GoToChild("amount")) {
		const char *amin = node.GetValue("min");
		amount_min_ = (amin && *amin) ? parse::ReadAsDouble(amin) : 0.0;
		const char *adw = node.GetValue("dices_weight");
		amount_dices_weight_ = (adw && *adw) ? parse::ReadAsDouble(adw) : 0.0;
		const char *acw = node.GetValue("competencies_weight");
		amount_competencies_weight_ = (acw && *acw) ? parse::ReadAsDouble(acw) : 0.0;
		node.GoToParent();
	}
}

void Heal::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Heal: " << "\r\n"
		   << " NPC coeff: " << kColorGrn << npc_coeff_ << kColorNrm
		   << " Extra: " << kColorGrn << (extra_ ? "yes" : "no") << kColorNrm
		   << " Prob: " << kColorGrn << prob_ << kColorNrm << "\r\n"
		   << " Amount min: " << kColorGrn << amount_min_ << kColorNrm
		   << " dices_weight: " << kColorGrn << amount_dices_weight_ << kColorNrm
		   << " competencies_weight: " << kColorGrn << amount_competencies_weight_ << kColorNrm << "\r\n";
}


void Area::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Area:" << "\r\n"
		   << "  Cast decay: " << kColorGrn << cast_decay << kColorNrm
		   << " Level decay: " << kColorGrn << level_decay << kColorNrm
		   << " Free targets: " << kColorGrn << free_targets << kColorNrm << "\r\n"
		   << "  Skill divisor: " << kColorGrn << skill_divisor << kColorNrm
		   << " Targets dice: " << kColorGrn << targets_dice_size << kColorNrm
		   << " Min targets: " << kColorGrn << min_targets << kColorNrm
		   << " Max targets: " << kColorGrn << max_targets << kColorNrm << "\r\n";
}

int Area::CalcTargetsQuantity(const int skill_level) const {
	auto bonus = RollDices(skill_level / skill_divisor, targets_dice_size);
	return min_targets + std::min(bonus, max_targets);
}

TalentAffect::TalentAffect(parser_wrapper::DataNode &node) {
	spell_ = parse::ReadAsConstant<ESpell>(node.GetValue("type"));
	saving_ = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
	resist_ = parse::ReadAsConstant<EResist>(node.GetValue("resist"));
	const char *prob = node.GetValue("prob");
	prob_ = (prob && *prob) ? parse::ReadAsInt(prob) : 100;

	for (auto &child: node.Children()) {
		const auto name = child.GetName();
		if (strcmp(name, "duration") == 0) {
			// issue.calc-duration: `cnst`/`level_divisor` renamed to `base`/`skill_divisor`.
			// The bonus is now `min(skill, 75) / skill_divisor` (capped, so monsters can't grow
			// affects unbounded), where `skill` is the spell's potency-roll `base_skill`.
			dur_min_ = parse::ReadAsInt(child.GetValue("min"));
			dur_max_ = parse::ReadAsInt(child.GetValue("max"));
			dur_base_ = parse::ReadAsInt(child.GetValue("base"));
			dur_skill_divisor_ = parse::ReadAsInt(child.GetValue("skill_divisor"));
		} else if (strcmp(name, "flags") == 0) {
			flags_ = parse::ReadAsConstantsBitvector<EAffFlag>(child.GetValue("val"));
		} else if (strcmp(name, "apply") == 0) {
			Apply apply;
			apply.id = parse::ReadAsConstant<EAffect>(child.GetValue("id"));
			apply.location = parse::ReadAsConstant<EApply>(child.GetValue("location"));
			// random is optional (default false). parse::ReadAsBool throws on empty input, so
			// guard with the (p && *p) pattern that the other optional attrs use. Without this
			// guard, ANY <apply> tag missing the random= attribute aborts parsing of the whole
			// <affects> block via the exception caught in Actions::Build -- which silently leaves
			// the spell's actions_ empty, so the affect never lands at runtime even though
			// CastAffect's imposition messages still fire (issue.no-affects-bug).
			const char *r = child.GetValue("random");
			apply.random = (r && *r) && parse::ReadAsBool(r);
			if (child.GoToChild("modifier")) {
				apply.min = parse::ReadAsDouble(child.GetValue("min"));
				apply.dices_weight = parse::ReadAsDouble(child.GetValue("dices_weight"));
				apply.competencies_weight = parse::ReadAsDouble(child.GetValue("competencies_weight"));
				apply.factor = parse::ReadAsInt(child.GetValue("factor"));
				// stack: optional max stack count, default 1, clamped to a minimum of 1.
				const char *stack = child.GetValue("stack");
				apply.stack = (stack && *stack) ? std::max(1, parse::ReadAsInt(stack)) : 1;
				// cap: optional upper bound on raw magnitude (issue.modifier-cap). 0 = no cap.
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
		   << "  Spell: " << kColorGrn << NAME_BY_ITEM<ESpell>(spell_) << kColorNrm
		   << " Saving: " << kColorGrn << NAME_BY_ITEM<ESaving>(saving_) << kColorNrm
		   << " Resist: " << kColorGrn << NAME_BY_ITEM<EResist>(resist_) << kColorNrm
		   << " Prob: " << kColorGrn << prob_ << kColorNrm
		   << " Flags: " << kColorGrn << flags_ << kColorNrm << "\r\n"
		   << "  Duration: base=" << kColorGrn << dur_base_ << kColorNrm
		   << " skill_divisor=" << kColorGrn << dur_skill_divisor_ << kColorNrm
		   << " min=" << kColorGrn << dur_min_ << kColorNrm
		   << " max=" << kColorGrn << dur_max_ << kColorNrm << "\r\n";
	for (const auto &apply: applies_) {
		buffer << "  Apply: " << kColorGrn << NAME_BY_ITEM<EAffect>(apply.id) << kColorNrm
			   << " -> " << kColorGrn << NAME_BY_ITEM<EApply>(apply.location) << kColorNrm
			   << (apply.random ? " [random]" : "")
			   << " (min=" << apply.min << " dices_weight=" << apply.dices_weight
			   << " competencies_weight=" << apply.competencies_weight
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
	const char *pw = node.GetValue("potency_weight");
	potency_weight_ = (pw && *pw) ? static_cast<float>(parse::ReadAsDouble(pw)) : 1.0f;
	const char *af = node.GetValue("affect_flags");
	affect_flags_ = (af && *af) ? parse::ReadAsConstantsBitvector<EAffFlag>(af)
								 : (kAfCurable | kAfDispellable);
	const char *prob = node.GetValue("prob");
	prob_ = (prob && *prob) ? parse::ReadAsInt(prob) : 100;
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
			for (const auto s: set.any_of) buffer << NAME_BY_ITEM<ESpell>(s) << " ";
			buffer << kColorNrm;
		}
		if (set.wildcard_all) {
			buffer << " all_of=" << kColorGrn << "*" << kColorNrm;
		} else if (!set.all_of.empty()) {
			buffer << " all_of=" << kColorGrn;
			for (const auto s: set.all_of) buffer << NAME_BY_ITEM<ESpell>(s) << " ";
			buffer << kColorNrm;
		}
		if (set.breaking_by_failure) {
			buffer << " breaking_by_failure=" << kColorGrn << "yes" << kColorNrm;
		}
		buffer << "\r\n";
	};
	buffer << " Unaffect:" << " potency_weight=" << kColorGrn << potency_weight_ << kColorNrm
		   << " affect_flags=" << kColorGrn << affect_flags_ << kColorNrm
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

void Actions::Print(CharData *ch, std::ostringstream &buffer) const {
	PrintFlagCondition("Blocking", blocking_, buffer);
	PrintFlagCondition("Required", required_, buffer);
	PrintFlagCondition("CasterBlocking", caster_blocking_, buffer);
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
	for (const auto &effect: *actions_) {
		effect.second->Print(ch, buffer);
	}
}

void Actions::Build(parser_wrapper::DataNode &node) {
	auto roster = std::make_unique<ActionsRoster>();
	blocking_ = {};
	required_ = {};
	caster_blocking_ = {};
	reflection_ = {};
	for (auto &action: node.Children()) {
		try {
			ParseAction(roster, action);
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
			return;
		}
	}
	actions_ = std::move(roster);
}

/*
 *  Парсеры воздействий
 */

// Parse a <blocking>/<required>/<caster_blocking> tag. Schema: child tags <mob_flags val=>,
// <affect_flags val=>, <room_flags val=>, <align val=>, each with a `|`-separated value
// (or a single token for <align>). Accumulates into the given condition. The child-tag
// form (issue.affect-flags step 2) replaced the previous attribute-list form so the line
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
			// ERoomFlag (issue.affect-flags): blocking by the CASTER's current room flags.
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

void Actions::ParseAction(ActionsRosterPtr &info, parser_wrapper::DataNode node) {
	for (auto &manifestation: node.Children()) {
		if (strcmp(manifestation.GetName(), "damage") == 0) {
			ParseDamage(info, manifestation);
		} else if (strcmp(manifestation.GetName(), "area") == 0) {
			ParseArea(info, manifestation);
		} else if (strcmp(manifestation.GetName(), "heal") == 0) {
			ParseHeal(info, manifestation);
		} else if (strcmp(manifestation.GetName(), "affects") == 0) {
			ParseAffect(info, manifestation);
		} else if (strcmp(manifestation.GetName(), "unaffect") == 0) {
			ParseUnaffect(info, manifestation);
		} else if (strcmp(manifestation.GetName(), "blocking") == 0) {
			ParseFlagCondition(blocking_, manifestation);
		} else if (strcmp(manifestation.GetName(), "required") == 0) {
			ParseFlagCondition(required_, manifestation);
		} else if (strcmp(manifestation.GetName(), "caster_blocking") == 0) {
			ParseFlagCondition(caster_blocking_, manifestation);
		} else if (strcmp(manifestation.GetName(), "reflection") == 0) {
			ParseReflection(reflection_, manifestation);
		}
	}
	node.GoToParent();
}

// <reflection affect_flags="..."  align="kGood|kEvil"  prob="N"/> (issue.cast-dmg-migration).
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
}

void Actions::ParseDamage(ActionsRosterPtr &info, parser_wrapper::DataNode &node) {
	info->emplace(EAction::kDamage, std::make_shared<Damage>(node));
}

void Actions::ParseArea(ActionsRosterPtr &info, parser_wrapper::DataNode &node) {
	auto ptr = std::make_shared<Area>();
	if (node.GoToChild("decay")) {
		ptr->free_targets = std::max(0, parse::ReadAsInt(node.GetValue("free_targets")));
		ptr->level_decay = parse::ReadAsInt(node.GetValue("level"));
		ptr->cast_decay = parse::ReadAsDouble(node.GetValue("cast"));
		node.GoToParent();
	}
	if (node.GoToChild("victims")) {
		ptr->skill_divisor = std::max(1, parse::ReadAsInt(node.GetValue("skill_divisor")));
		ptr->targets_dice_size = std::max(1, parse::ReadAsInt(node.GetValue("dice_size")));
		ptr->min_targets = std::max(1, parse::ReadAsInt(node.GetValue("min_targets")));
		ptr->max_targets = std::max(1, parse::ReadAsInt(node.GetValue("max_targets")));
		node.GoToParent();
	}

	info->insert({EAction::kArea, std::move(ptr)});
}

void Actions::ParseHeal(ActionsRosterPtr &info, parser_wrapper::DataNode &node) {
	info->emplace(EAction::kHeal, std::make_shared<Heal>(node));
}

void Actions::ParseAffect(ActionsRosterPtr &info, parser_wrapper::DataNode &node) {
	info->emplace(EAction::kAffect, std::make_shared<TalentAffect>(node));
}

void Actions::ParseUnaffect(ActionsRosterPtr &info, parser_wrapper::DataNode &node) {
	info->emplace(EAction::kUnaffect, std::make_shared<TalentUnaffect>(node));
}

/*
 *  Геттеры эффектов
 *  Area и Dmg возвпращаются только по одному, потому что для обраьотки их списка надо переписывать всю логику
 *  работы с арекастами, а даже, скорее, переносить ее внутрь эффекта (как сделано с Applies).
 */

bool Actions::Contains(EAction action) const {
	return actions_->contains(action);
}

const Damage &Actions::GetDmg() const {
	if (actions_->contains(EAction::kDamage)) {
		return *std::static_pointer_cast<Damage>(actions_->find(EAction::kDamage)->second);
	} else {
		throw std::runtime_error("Getting damage parameters from talent which has no 'damage' action.");
	}
}

const Area &Actions::GetArea() const {
	if (actions_->contains(EAction::kArea)) {
		return *std::static_pointer_cast<Area>(actions_->find(EAction::kArea)->second);
	} else {
		throw std::runtime_error("Getting area parameters from talent which has no 'area' action.");
	}
}

const Heal &Actions::GetHeal() const {
	if (actions_->contains(EAction::kHeal)) {
		return *std::static_pointer_cast<Heal>(actions_->find(EAction::kHeal)->second);
	} else {
		throw std::runtime_error("Getting heal parameters from talent which has no 'heal' action.");
	}
}

const TalentAffect &Actions::GetAffect() const {
	if (actions_->contains(EAction::kAffect)) {
		return *std::static_pointer_cast<TalentAffect>(actions_->find(EAction::kAffect)->second);
	} else {
		throw std::runtime_error("Getting affect parameters from talent which has no 'affects' action.");
	}
}

const TalentUnaffect &Actions::GetUnaffect() const {
	if (actions_->contains(EAction::kUnaffect)) {
		return *std::static_pointer_cast<TalentUnaffect>(actions_->find(EAction::kUnaffect)->second);
	} else {
		throw std::runtime_error("Getting unaffect parameters from talent which has no 'unaffect' action.");
	}
}

}
