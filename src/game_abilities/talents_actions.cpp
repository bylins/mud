#include "talents_actions.h"

#include "color.h"
#include "entities/char_data.h"
#include "utils/random.h"

namespace talents_actions {

void Roll::ParseRoll(parser_wrapper::DataNode &node) {
	if (node.GoToChild("dices")) {
		ParseDices(node);
		node.GoToParent();
	}

	if (node.GoToChild("base_skill")) {
		ParseBaseSkill(node);
		node.GoToParent();
	}

	if (node.GoToChild("base_stat")) {
		ParseBaseStat(node);
		node.GoToParent();
	}
}

int Roll::RollDices() const {
	return ::RollDices(dice_num_, dice_size_) + dice_add_;
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

void Roll::ParseDices(parser_wrapper::DataNode &node) {
	dice_num_ = std::max(1, parse::ReadAsInt(node.GetValue("ndice")));
	dice_size_ = std::max(1, parse::ReadAsInt(node.GetValue("sdice")));
	dice_add_ = parse::ReadAsInt(node.GetValue("adice"));
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

void Roll::Print(std::ostringstream &buffer) const {
	buffer << "  Roll: " << KGRN << dice_num_ << "d" << dice_size_ << "+" << dice_add_ << KNRM << "\r\n"
		   << "   Skill: " << KGRN << NAME_BY_ITEM<ESkill>(base_skill_) << KNRM
		   << " Low skill bonus: " << KGRN << low_skill_bonus_ << KNRM
		   << " Hi skill bonus: " << KGRN << hi_skill_bonus_ << KNRM << "\r\n"
		   << "   Stat: " << NAME_BY_ITEM<EBaseStat>(base_stat_)
		   << " Threshold: " << KGRN << base_stat_threshold_ << KNRM
		   << " Weight: " << KGRN << base_stat_weight_ << KNRM << "\r\n";
}

// =====================================================================================================================

TalentEffect::TalentEffect(parser_wrapper::DataNode &node) {
	ParseSaving(node);
	if (node.GoToChild("roll")) {
		power_roll_.ParseRoll(node);
		node.GoToParent();
	}
}

void TalentEffect::ParseSaving(parser_wrapper::DataNode &node) {
	saving_ = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
}

void TalentEffect::Print(std::ostringstream &buffer) const {
	buffer << "  Saving: " << KGRN << NAME_BY_ITEM<ESaving>(saving_) << KNRM << "\r\n";
	power_roll_.Print(buffer);
}

// =====================================================================================================================

//Affect::Affect(parser_wrapper::DataNode &node) {
//	saving_ = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));
//}

// =====================================================================================================================

void Damage::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Damage: \r\n";
	TalentEffect::Print(buffer);
}

Damage::Damage(parser_wrapper::DataNode &node)
	: TalentEffect(node) { }

// =====================================================================================================================

Area::Area(parser_wrapper::DataNode &node)
	: TalentEffect(node) {
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
	buffer << " Area:" << "\r\n"
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

void Actions::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	for (const auto &effect: *actions_) {
		effect.second->Print(buffer);
	}
}

void Actions::Build(parser_wrapper::DataNode &node) {
	auto roster = std::make_unique<ActionsRoster>();
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

void Actions::ParseAction(ActionsRosterPtr &info, parser_wrapper::DataNode node) {
	for (auto &manifestation: node.Children()) {
		if (strcmp(manifestation.GetName(), "damage") == 0) {
			ParseDamage(info, manifestation);
		} else if (strcmp(manifestation.GetName(), "area") == 0) {
			ParseArea(info, manifestation);
		} else if (strcmp(manifestation.GetName(), "affect") == 0) {
			ParseAffect(info, manifestation);
		}
	}
	node.GoToParent();
}

void Actions::ParseDamage(ActionsRosterPtr &info, parser_wrapper::DataNode &node) {
	info->emplace(ETalentEffect::kDamage, std::make_shared<Damage>(node));
}

void Actions::ParseAffect(ActionsRosterPtr &/*info*/, parser_wrapper::DataNode &/*node*/)
{
//	info->emplace(ETalentEffect::kAffect, std::make_shared<Affect>(node));
}

void Actions::ParseArea(ActionsRosterPtr &info, parser_wrapper::DataNode &node) {
	info->emplace(ETalentEffect::kArea, std::make_shared<Area>(node));
}

/*
 *  Геттеры эффектов
 *  Area и Dmg возвпращаются только по одному, потому что для обработки их списка надо переписывать всю логику
 *  работы с арекастами, а даже, скорее, переносить ее внутрь эффекта (как сделано с Applies).
 */

const Damage &Actions::GetDmg() const {
	if (actions_->contains(ETalentEffect::kDamage)) {
		return *std::static_pointer_cast<Damage>(actions_->find(ETalentEffect::kDamage)->second);
	} else {
		throw std::runtime_error("Getting damage parameters from talent which has no 'damage' action.");
	}
}

const Area &Actions::GetArea() const {
	if (actions_->contains(ETalentEffect::kArea)) {
		return *std::static_pointer_cast<Area>(actions_->find(ETalentEffect::kArea)->second);
	} else {
		throw std::runtime_error("Getting area parameters from talent which has no 'area' action.");
	}
}

}
