#include "talents_actions.h"

#include "color.h"
#include "utils/parse.h"

namespace talents_actions {

void Damage::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Damage: " << std::endl
		   << KGRN << "  " << dice_num
		   << "d" << dice_size
		   << "+" << dice_add << KNRM
		   << " Low skill bonus: " << KGRN << low_skill_bonus << KNRM
		   << " Hi skill bonus: " << KGRN << hi_skill_bonus << KNRM
		   << " Saving: " << KGRN << NAME_BY_ITEM<ESaving>(saving) << KNRM << std::endl;
}

void Area::Print(CharData */*ch*/, std::ostringstream &buffer) const {
	buffer << " Area:" << std::endl
		   << "  Cast decay: " << KGRN << cast_decay << KNRM
		   << " Level decay: " << KGRN << level_decay << KNRM
		   << " Free targets: " << KGRN << free_targets << KNRM << std::endl
		   << "  Skill divisor: " << KGRN << skill_divisor << KNRM
		   << " Targets dice: " << KGRN << targets_dice_size << KNRM
		   << " Min targets: " << KGRN << min_targets << KNRM
		   << " Max targets: " << KGRN << max_targets << KNRM << std::endl;
}

void Actions::Print(CharData *ch, std::ostringstream &buffer) const {
	for (const auto &effect: *actions_) {
		effect.second->Print(ch, buffer);
	}
}

void Actions::Build(parser_wrapper::DataNode &node) {
	auto roster = std::make_unique<ActionsRoster>();
	for (auto &effect: node.Children()) {
		try {
			ParseAction(roster, effect);
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
	if (strcmp(node.GetName(), "damage") == 0) {
		ParseDamage(info, node);
	} else if (strcmp(node.GetName(), "area") == 0) {
		ParseArea(info, node);
	}
}

void Actions::ParseDamage(ActionsRosterPtr &info, parser_wrapper::DataNode &node) {
	auto ptr = std::make_shared<Damage>();
	ptr->saving = parse::ReadAsConstant<ESaving>(node.GetValue("saving"));

	if (node.GoToChild("amount")) {
		ptr->dice_num = std::max(1, parse::ReadAsInt(node.GetValue("ndice")));
		ptr->dice_size = std::max(1, parse::ReadAsInt(node.GetValue("sdice")));
		ptr->dice_add = parse::ReadAsInt(node.GetValue("adice"));
		ptr->low_skill_bonus = parse::ReadAsDouble(node.GetValue("low_skill_bonus"));
		ptr->hi_skill_bonus = parse::ReadAsDouble(node.GetValue("hi_skill_bonus"));
		node.GoToParent();
	}

	info->insert({EAction::kDamage, std::move(ptr)});
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

/*
 *  Геттеры эффектов
 *  Area и Dmg возвпращаются только по одному, потому что для обраьотки их списка надо переписывать всю логику
 *  работы с арекастами, а даже, скорее, переносить ее внутрь эффекта (как сделано с Applies).
 */

std::optional<Damage> Actions::GetDmg() const {
	if (actions_->contains(EAction::kDamage)) {
		return *std::static_pointer_cast<Damage>(actions_->find(EAction::kDamage)->second);
	} else {
		err_log("getting damage parameters from talent has no 'damage' effect section.");
		return std::nullopt;
	}
}

std::optional<Area> Actions::GetArea() const {
	if (actions_->contains(EAction::kArea)) {
		return *std::static_pointer_cast<Area>(actions_->find(EAction::kArea)->second);
	} else {
		err_log("getting area parameters from talent has no 'area' effect section.");
		return std::nullopt;
	}
}

}