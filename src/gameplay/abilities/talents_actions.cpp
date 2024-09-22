#include "talents_actions.h"

#include "engine/ui/color.h"
#include "engine/entities/char_data.h"
#include "fmt/format.h"
#include "utils/random.h"

namespace talents_actions {

void Damage::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Damage: " << "\r\n"
		   << kColorGrn << "  " << dice_num_
		   << "d" << dice_size_
		   << "+" << dice_add_ << kColorNrm
		   << " Low skill bonus: " << kColorGrn << low_skill_bonus_ << kColorNrm
		   << " Hi skill bonus: " << kColorGrn << hi_skill_bonus_ << kColorNrm
		   << " Saving: " << kColorGrn << NAME_BY_ITEM<ESaving>(saving_) << kColorNrm << "\r\n";
}

int Damage::RollSkillDices() const {
	return RollDices(dice_num_, dice_size_) + dice_add_;
}

double Damage::CalcSkillCoeff(const CharData *const ch) const {
	auto skill = ch->GetSkill(base_skill_);
	auto low_skill = std::min(skill, abilities::kNoviceSkillThreshold);
	auto hi_skill = std::max(0, skill - abilities::kNoviceSkillThreshold);
	return (low_skill * low_skill_bonus_ + hi_skill * hi_skill_bonus_) / 100.0;
}

double Damage::CalcBaseStatCoeff(const CharData *const ch) const {
	return std::max(0, GetRealBaseStat(ch, base_stat_) - base_stat_threshold_) * base_stat_weight_ / 100.0;
}

Damage::Damage(parser_wrapper::DataNode &node) {
	saving_ = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));

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

Heal::Heal(parser_wrapper::DataNode &node) : Damage(node) {
	npc_coeff_ = parse::ReadAsDouble(node.GetValue("npc_coeff"));
}

double Heal::GetNpcCoeff() const {
	return npc_coeff_;
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

void Actions::Print(CharData *ch, std::ostringstream &buffer) const {
	for (const auto &effect: *actions_) {
		effect.second->Print(ch, buffer);
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
		} else if (strcmp(manifestation.GetName(), "heal") == 0) {
			ParseHeal(info, manifestation);
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

/*
 *  Геттеры эффектов
 *  Area и Dmg возвпращаются только по одному, потому что для обраьотки их списка надо переписывать всю логику
 *  работы с арекастами, а даже, скорее, переносить ее внутрь эффекта (как сделано с Applies).
 */

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

}
