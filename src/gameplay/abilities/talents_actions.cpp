#include "talents_actions.h"

#include "engine/ui/color.h"
#include "engine/entities/char_data.h"
#include "gameplay/magic/spells_constants.h"
#include "fmt/format.h"
#include "utils/random.h"

#include <map>

namespace talents_actions {

namespace {
// Mob flags (EMobFlag) addressable from the <blocking>/<required> mob_flags="..." tags. A
// focused table rather than a full EMobFlag name registry: only flags meaningful as a cast
// gate (immunities for <blocking>, target requirements like kCorpse for <required>) live here.
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

// Parse a `|`-separated list of ESpell names (an any_of/all_of attribute of an
// <unaffect> sub-tag, issue #3342). Empty/absent attribute -> empty list.
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
		dice_num_ = std::max(1, parse::ReadAsInt(node.GetValue("ndice")));
		dice_size_ = std::max(1, parse::ReadAsInt(node.GetValue("sdice")));
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

void Damage::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Damage: " << "\r\n"
		   << " Saving: " << kColorGrn << NAME_BY_ITEM<ESaving>(saving_) << kColorNrm << "\r\n";
}

Damage::Damage(parser_wrapper::DataNode &node) {
	saving_ = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
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

	for (auto &child: node.Children()) {
		const auto name = child.GetName();
		if (strcmp(name, "duration") == 0) {
			dur_min_ = parse::ReadAsInt(child.GetValue("min"));
			dur_max_ = parse::ReadAsInt(child.GetValue("max"));
			dur_const_ = parse::ReadAsInt(child.GetValue("cnst"));
			dur_level_divisor_ = parse::ReadAsInt(child.GetValue("level_divisor"));
		} else if (strcmp(name, "flags") == 0) {
			flags_ = parse::ReadAsConstantsBitvector<EAffFlag>(child.GetValue("val"));
		} else if (strcmp(name, "apply") == 0) {
			Apply apply;
			apply.id = parse::ReadAsConstant<EAffect>(child.GetValue("id"));
			apply.location = parse::ReadAsConstant<EApply>(child.GetValue("location"));
			apply.random = parse::ReadAsBool(child.GetValue("random"));
			if (child.GoToChild("modifier")) {
				apply.min = parse::ReadAsDouble(child.GetValue("min"));
				apply.dices_weight = parse::ReadAsDouble(child.GetValue("dices_weight"));
				apply.competencies_weight = parse::ReadAsDouble(child.GetValue("competencies_weight"));
				apply.factor = parse::ReadAsInt(child.GetValue("factor"));
				// stack: optional max stack count, default 1, clamped to a minimum of 1.
				const char *stack = child.GetValue("stack");
				apply.stack = (stack && *stack) ? std::max(1, parse::ReadAsInt(stack)) : 1;
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
			reposition_stop_fight_ = parse::ReadAsBool(child.GetValue("stop_fight"));
		}
	}
}

void TalentAffect::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Affect:" << "\r\n"
		   << "  Spell: " << kColorGrn << NAME_BY_ITEM<ESpell>(spell_) << kColorNrm
		   << " Saving: " << kColorGrn << NAME_BY_ITEM<ESaving>(saving_) << kColorNrm
		   << " Resist: " << kColorGrn << NAME_BY_ITEM<EResist>(resist_) << kColorNrm
		   << " Flags: " << kColorGrn << flags_ << kColorNrm << "\r\n"
		   << "  Duration: cnst=" << kColorGrn << dur_const_ << kColorNrm
		   << " level_divisor=" << kColorGrn << dur_level_divisor_ << kColorNrm
		   << " min=" << kColorGrn << dur_min_ << kColorNrm
		   << " max=" << kColorGrn << dur_max_ << kColorNrm << "\r\n";
	for (const auto &apply: applies_) {
		buffer << "  Apply: " << kColorGrn << NAME_BY_ITEM<EAffect>(apply.id) << kColorNrm
			   << " -> " << kColorGrn << NAME_BY_ITEM<EApply>(apply.location) << kColorNrm
			   << (apply.random ? " [random]" : "")
			   << " (min=" << apply.min << " dices_weight=" << apply.dices_weight
			   << " competencies_weight=" << apply.competencies_weight
			   << " factor=" << apply.factor << " stack=" << apply.stack << ")\r\n";
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
			set->any_of = ParseSpellList(child.GetValue("any_of"));
			set->all_of = ParseSpellList(child.GetValue("all_of"));
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
		if (!set.any_of.empty()) {
			buffer << " any_of=" << kColorGrn;
			for (const auto s: set.any_of) buffer << NAME_BY_ITEM<ESpell>(s) << " ";
			buffer << kColorNrm;
		}
		if (!set.all_of.empty()) {
			buffer << " all_of=" << kColorGrn;
			for (const auto s: set.all_of) buffer << NAME_BY_ITEM<ESpell>(s) << " ";
			buffer << kColorNrm;
		}
		if (set.breaking_by_failure) {
			buffer << " breaking_by_failure=" << kColorGrn << "yes" << kColorNrm;
		}
		buffer << "\r\n";
	};
	buffer << " Unaffect:" << " potency_weight=" << kColorGrn << potency_weight_ << kColorNrm << "\r\n";
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
	buffer << "\r\n";
}
}  // namespace

void Actions::Print(CharData *ch, std::ostringstream &buffer) const {
	PrintFlagCondition("Blocking", blocking_, buffer);
	PrintFlagCondition("Required", required_, buffer);
	for (const auto &effect: *actions_) {
		effect.second->Print(ch, buffer);
	}
}

void Actions::Build(parser_wrapper::DataNode &node) {
	auto roster = std::make_unique<ActionsRoster>();
	blocking_ = {};
	required_ = {};
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

// Parse a <blocking>/<required> tag: mob_flags (EMobFlag, from the focused name table) and
// affect_flags (EAffect) -- both `|`-separated lists. Accumulates into the given condition.
void Actions::ParseFlagCondition(FlagCondition &cond, parser_wrapper::DataNode &node) {
	const char *mob = node.GetValue("mob_flags");
	if (mob && *mob) {
		for (const auto &flag_name : utils::Split(mob, '|')) {
			const auto it = kBlockingFlagByName.find(flag_name);
			if (it != kBlockingFlagByName.end()) {
				cond.mob_flags.push_back(it->second);
			} else {
				err_log("Actions: unknown EMobFlag '%s' in <blocking/required mob_flags>.", flag_name.c_str());
			}
		}
	}
	const char *aff = node.GetValue("affect_flags");
	if (aff && *aff) {
		for (const auto &flag_name : utils::Split(aff, '|')) {
			cond.affect_flags.push_back(parse::ReadAsConstant<EAffect>(flag_name.c_str()));
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
		}
	}
	node.GoToParent();
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
