#include "talents_actions.h"

#include "color.h"
#include "entities/char_data.h"
#include "utils/random.h"
#include "utils/table_wrapper.h"

namespace talents_actions {

void Roll::ParseRoll(parser_wrapper::DataNode &node) {
	resist_weight_ = parse::ReadAsInt(node.GetValue("resist_weight"));
	if (node.GoToChild("base_skill")) {
		ParseBaseSkill(node);
		node.GoToParent();
	}
	if (node.GoToChild("base_stat")) {
		ParseBaseStat(node);
		node.GoToParent();
	}
}

void Roll::ParseBaseSkill(parser_wrapper::DataNode &node) {
	base_skill_ = parse::ReadAsConstant<ESkill>(node.GetValue("id"));
	low_skill_bonus_ = parse::ReadAsDouble(node.GetValue("low_skill_bonus"));
	hi_skill_bonus_ = parse::ReadAsDouble(node.GetValue("hi_skill_bonus"));
}

void Roll::ParseBaseStat(parser_wrapper::DataNode &node) {
	base_stat_ = parse::ReadAsConstant<EBaseStat>(node.GetValue("id"));
	base_stat_threshold_ = parse::ReadAsInt(node.GetValue("threshold"));
	base_stat_weight_ = parse::ReadAsDouble(node.GetValue("weight"));
}

double Roll::CalcSkillCoeff(const CharData *const ch) const {
	auto skill = ch->GetSkill(base_skill_);
	auto low_skill = std::min(skill, abilities::kNoviceSkillThreshold);
	auto hi_skill = std::max(0, skill - abilities::kNoviceSkillThreshold);
	return (low_skill * low_skill_bonus_ + hi_skill * hi_skill_bonus_) / 100.0;
}

double Roll::CalcBaseStatCoeff(const CharData *const ch) const {
	return (std::max(0, GetRealBaseStat(ch, base_stat_) - base_stat_threshold_)) * base_stat_weight_ / 100.0;
}

void Roll::Print(std::ostringstream &buffer) const {
	buffer << "   Skill: " << KGRN << NAME_BY_ITEM<ESkill>(base_skill_) << KNRM
		   << " Low skill bonus: " << KGRN << low_skill_bonus_ << KNRM
		   << " Hi skill bonus: " << KGRN << hi_skill_bonus_ << KNRM << "\r\n"
		   << "   Stat: "  << KGRN << NAME_BY_ITEM<EBaseStat>(base_stat_) << KNRM
		   << " Threshold: " << KGRN << base_stat_threshold_ << KNRM
		   << " Weight: " << KGRN << base_stat_weight_ << KNRM << "\r\n";
}

// =====================================================================================================================

void PowerRoll::ParseRoll(parser_wrapper::DataNode &node) {
	if (node.GoToChild("dices")) {
		ParseDices(node);
		node.GoToParent();
	}
	Roll::ParseRoll(node);
}

int PowerRoll::RollDices() const {
	return ::RollDices(dice_num_, dice_size_) + dice_add_;
}

void PowerRoll::ParseDices(parser_wrapper::DataNode &node) {
	dice_num_ = std::max(1, parse::ReadAsInt(node.GetValue("ndice")));
	dice_size_ = std::max(1, parse::ReadAsInt(node.GetValue("sdice")));
	dice_add_ = parse::ReadAsInt(node.GetValue("adice"));
}

int PowerRoll::DoRoll(const CharData *ch) const {
	auto roll = RollDices();
	auto skill_coeff = CalcSkillCoeff(ch);
	auto base_stat_coeff = CalcBaseStatCoeff(ch);
	return static_cast<int>((1.0 + skill_coeff + base_stat_coeff)*roll);
}

void PowerRoll::Print(std::ostringstream &buffer) const {
	buffer << "  Roll: " << KGRN << dice_num_ << "d" << dice_size_ << "+" << dice_add_ << KNRM << "\r\n";
	Roll::Print(buffer);
}

// =====================================================================================================================

void SuccessRoll::ParseRoll(parser_wrapper::DataNode &node) {
	if (node.GoToChild("thresholds")) {
		critsuccess_threshold_ = parse::ReadAsInt(node.GetValue("critsuccess"));
		critfail_threshold_ = parse::ReadAsInt(node.GetValue("critfail"));
		node.GoToParent();
	}
	if (node.GoToChild("roll_mods")) {
		roll_bonus_ = parse::ReadAsInt(node.GetValue("bonus"));
		pvp_penalty_ = parse::ReadAsInt(node.GetValue("pvp"));
		pve_penalty_ = parse::ReadAsInt(node.GetValue("pve"));
		evp_penalty_ = parse::ReadAsInt(node.GetValue("evp"));
		node.GoToParent();
	}
	Roll::ParseRoll(node);
}

void SuccessRoll::Print(std::ostringstream &buffer) const {
	buffer << "   Bonuses:  Roll: " << KGRN << roll_bonus_ << KNRM
		   << " PvP: " << KGRN << pvp_penalty_ << KNRM
		   << " PvE: " << KGRN << pve_penalty_ << KNRM
		   << " EvP: " << KGRN << evp_penalty_ << KNRM << "\r\n"
		   << "   Thresholds:  Critsuccess: " << KGRN << critsuccess_threshold_ << KNRM
		   << " Critfail: " << KGRN << critfail_threshold_ << KNRM << "\r\n";
	Roll::Print(buffer);
}

// =====================================================================================================================

Duration::Duration(parser_wrapper::DataNode &node) {
	min_ = std::max(0, parse::ReadAsInt(node.GetValue("min")));
	cap_ = parse::ReadAsInt(node.GetValue("cap"));
	accumulate_ = parse::ReadAsBool(node.GetValue("accumulate"));
	if (node.GoToChild("success_roll")) {
		success_roll_.ParseRoll(node);
		node.GoToParent();
	}
}

void Duration::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << "\r\n Duration:\r\n";
	buffer << "  Min: " << KGRN << min_ << KNRM
		   << " Cap: " << KGRN << cap_ << KNRM
		   << " Accumulate: " << KGRN << (accumulate_ ? "Yes" : "No") << KNRM << "\r\n";
	success_roll_.Print(buffer);
}

// =====================================================================================================================

Affect::Affect(parser_wrapper::DataNode &node) {
	saving_ = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
	if (node.GoToChild("power_roll")) {
		power_roll_.ParseRoll(node);
		node.GoToParent();
	}
	if (node.GoToChild("flags")) {
		flags_ = parse::ReadAsConstantsBitvector<EAffectFlag>(node.GetValue("val"));
		node.GoToParent();
	}
	if (node.GoToChild("apply")) {
		location_ = parse::ReadAsConstant<EApply>(node.GetValue("location"));
		mod_ = parse::ReadAsInt(node.GetValue("val"));
		cap_ = parse::ReadAsInt(node.GetValue("cap"));
		accumulate_ = parse::ReadAsBool(node.GetValue("accumulate"));
		node.GoToParent();
	}
	if (node.GoToChild("applies_affects")) {
		parse::ReadAsConstantsSet<EAffect>(applies_affects_, node.GetValue("val"));
		for (const auto bit: applies_affects_) {
			appplies_bits_ |= to_underlying(bit);
		}
		node.GoToParent();
	}
	if (node.GoToChild("removes_affects")) {
		parse::ReadAsConstantsSet<ESpell>(removes_spell_affects_, node.GetValue("val"));
		node.GoToParent();
	}
	if (node.GoToChild("replaces_affects")) {
		parse::ReadAsConstantsSet<ESpell>(replaces_spell_affects_, node.GetValue("val"));
		node.GoToParent();
	}
	if (node.GoToChild("blocked")) {
		parse::ReadAsConstantsSet<EAffect>(blocked_by_affects_, node.GetValue("affects"));
		parse::ReadAsConstantsSet<EMobFlag>(blocked_by_mob_flags_, node.GetValue("flags"));
		node.GoToParent();
	}
}

void Affect::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << "\r\n Affect:\r\n";
	buffer << "  Saving: " << KGRN << NAME_BY_ITEM<ESaving>(saving_) << KNRM << "\r\n";
	buffer << "  Affect flags: " << KGRN << parse::BitvectorToString<EAffectFlag>(flags_) << KNRM << "\r\n";
	buffer << "  Applies affects: " << KGRN << parse::ConstantsSetToString<EAffect>(applies_affects_) << KNRM << "\r\n";
	buffer << "  Removes spell affects: " << KGRN << parse::ConstantsSetToString<ESpell>(removes_spell_affects_) << KNRM << "\r\n";
	buffer << "  Replaces spell affects: " << KGRN << parse::ConstantsSetToString<ESpell>(replaces_spell_affects_) << KNRM << "\r\n";
	buffer << "  Blocked by affects: " << KGRN << parse::ConstantsSetToString<EAffect>(blocked_by_affects_) << KNRM << "\r\n";
	buffer << "  Blocked by mob flag: " << KGRN << parse::ConstantsSetToString<EMobFlag>(blocked_by_mob_flags_) << KNRM << "\r\n";
	buffer << "\r\n  Applies:\r\n";
	table_wrapper::Table table;
	table.set_left_margin(3);
	table << table_wrapper::kHeader << "Location" << "Mod" << "Cap" << "Accumulate" << table_wrapper::kEndRow;
	table << NAME_BY_ITEM<EApply>(location_) << mod_ << cap_ << (accumulate_ ? "Yes" : "No") << table_wrapper::kEndRow;
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(buffer, table);
	buffer << "\r\n";
	power_roll_.Print(buffer);
}

// =====================================================================================================================

Damage::Damage(parser_wrapper::DataNode &node) {
	saving_ = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
	if (node.GoToChild("power_roll")) {
		power_roll_.ParseRoll(node);
		node.GoToParent();
	}
}

void Damage::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << "\r\n Damage:\r\n";
	buffer << "  Saving: " << KGRN << NAME_BY_ITEM<ESaving>(saving_) << KNRM << "\r\n";
	power_roll_.Print(buffer);
}

// =====================================================================================================================

Area::Area(parser_wrapper::DataNode &node) {
	if (node.GoToChild("decay")) {
		free_targets = std::max(0, parse::ReadAsInt(node.GetValue("free_targets")));
		level_decay = parse::ReadAsInt(node.GetValue("level"));
		cast_decay = parse::ReadAsDouble(node.GetValue("cast"));
		node.GoToParent();
	}
	if (node.GoToChild("victims")) {
		skill_divisor = std::max(1, parse::ReadAsInt(node.GetValue("skill_divisor")));
		targets_dice_size = std::max(1, parse::ReadAsInt(node.GetValue("dice_size")));
		min_targets = std::max(1, parse::ReadAsInt(node.GetValue("min_targets")));
		max_targets = std::max(1, parse::ReadAsInt(node.GetValue("max_targets")));
		node.GoToParent();
	}
}

void Area::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << "\r\n Area:\r\n"
		   << "  Cast decay: " << KGRN << cast_decay << KNRM
		   << " Level decay: " << KGRN << level_decay << KNRM
		   << " Free targets: " << KGRN << free_targets << KNRM << "\r\n"
		   << "  Skill divisor: " << KGRN << skill_divisor << KNRM
		   << " Targets dice: " << KGRN << targets_dice_size << KNRM
		   << " Min targets: " << KGRN << min_targets << KNRM
		   << " Max targets: " << KGRN << max_targets << KNRM << "\r\n";
}

int Area::CalcTargetsQuantity(const int skill_level) const {
	auto bonus = ::RollDices(skill_level / skill_divisor, targets_dice_size);
	return min_targets + std::min(bonus, max_targets);
}

// =====================================================================================================================

TalentAction::TalentAction(parser_wrapper::DataNode &node) {
	for (auto &manifestation: node.Children()) {
		if (strcmp(manifestation.GetName(), "damage") == 0) {
			damage_ = std::make_unique<Damage>(manifestation);
		} else if (strcmp(manifestation.GetName(), "area") == 0) {
			area_ = std::make_unique<Area>(manifestation);
		} else if (strcmp(manifestation.GetName(), "duration") == 0) {
			duration_ = std::make_unique<Duration>(manifestation);
		} else if (strcmp(manifestation.GetName(), "affect") == 0) {
			affects_.emplace_back(Affect(manifestation));
		}
	}
}

void TalentAction::Print(CharData *ch, std::ostringstream &buffer) const {
	if (area_) {
		area_->Print(ch, buffer);
	}
	if (duration_) {
		duration_->Print(ch, buffer);
	}
	if (damage_) {
		damage_->Print(ch, buffer);
	}
	for (const auto &affect:  affects_) {
		affect.Print(ch, buffer);
	}
}

const Damage &TalentAction::GetDmg() const {
	if (damage_) {
		return *damage_;
	} else {
		throw std::runtime_error("Getting damage parameters from a talent action without a 'damage' section.");
	}
}

const Area &TalentAction::GetArea() const {
	if (area_) {
		return *area_;
	} else {
		throw std::runtime_error("Getting area parameters from a talent action without a 'area' section.");
	}
}

const Duration &TalentAction::GetDuration() const {
	if (duration_) {
		return *duration_;
	} else {
		throw std::runtime_error("Getting duration parameters from a talent action without a 'duration' section.");
	}
}

const TalentAction::Affects &TalentAction::GetAffects() const {
	return affects_;
}

// =====================================================================================================================

void Actions::Print(CharData *ch, std::ostringstream &buffer) const {
	int count{1};
	for (const auto &action: *actions_) {
		buffer << " Action " << count << ":\r\n";
		action.Print(ch, buffer);
		++count;
	}
}

void Actions::Build(parser_wrapper::DataNode &node) {
	auto roster = std::make_unique<ActionsRoster>();
	for (auto &action: node.Children()) {
		try {
			ParseSingeAction(roster, action);
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
			return;
		}
	}
	actions_ = std::move(roster);
}

void Actions::ParseSingeAction(ActionsRosterPtr &info, parser_wrapper::DataNode node) {
	info->emplace_back(TalentAction(node));
//	node.GoToParent();
}

/*
 *  Геттеры эффектов
 */

const Damage &Actions::GetDmg() const {
	return actions_->at(0).GetDmg();
}

const Area &Actions::GetArea() const {
	return actions_->at(0).GetArea();
}

const Duration &Actions::GetDuration() const {
	return actions_->at(0).GetDuration();
}

const TalentAction::Affects &Actions::GetAffects() const {
	return actions_->at(0).GetAffects();
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :