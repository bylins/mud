#include "effect.h"

//#include <sstream>

#include "color.h"
#include "entities/char_data.h"
//#include "utils/parse.h"
#include "utils/table_wrapper.h"

namespace effects {

void Damage::Print(std::ostringstream &buffer) const {
	buffer << " Damage: " << std::endl
		   << KGRN << "  " << dice_num
		   << "d" << dice_size
		   << "+" << dice_add << KNRM
		   << " Low skill bonus: " << KGRN << low_skill_bonus << KNRM
		   << " Hi skill bonus: " << KGRN << hi_skill_bonus << KNRM
		   << " Saving: " << KGRN << NAME_BY_ITEM<ESaving>(saving) << KNRM << std::endl;
}

void Area::Print(std::ostringstream &buffer) const {
	buffer << " Area:" << std::endl
		   << "  Cast decay: " << KGRN << cast_decay << KNRM
		   << " Level decay: " << KGRN << level_decay << KNRM
		   << " Free targets: " << KGRN << free_targets << KNRM << std::endl
		   << "  Skill divisor: " << KGRN << skill_divisor << KNRM
		   << " Targets dice: " << KGRN << targets_dice_size << KNRM
		   << " Min targets: " << KGRN << min_targets << KNRM
		   << " Max targets: " << KGRN << max_targets << KNRM << std::endl;
}

void Applies::Print(std::ostringstream &buffer) const {
	buffer << " Applies:" << std::endl;
	table_wrapper::Table table;
	table << table_wrapper::kHeader
		<< "Location" << "Mod" << "Level bonus" << "Remort bonus" << "Cap" << table_wrapper::kEndRow;
	for (const auto &apply: applies_roster) {
		table << NAME_BY_ITEM<EApply>(apply.first)
			  << apply.second.mod
			  << apply.second.lvl_bonus
			  << apply.second.remort_bonus
			  << apply.second.cap
			  << table_wrapper::kEndRow;
	}
	// \todo Сделать тут оформление таблицы
	table_wrapper::PrintTableToStream(buffer, table);
	buffer << std::endl;
}

void Effects::Print(std::ostringstream &buffer) const {
	for (const auto &effect: *effects_) {
		effect.second->Print(buffer);
	}
}

void Effects::Build(parser_wrapper::DataNode &node) {
	auto roster = std::make_unique<EffectsRoster>();
	for (auto &effect: node.Children()) {
		try {
			ParseEffect(roster, effect);
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in '%s' effect.", e.what(), node.GetName());
			return;
		}
	}
	effects_ = std::move(roster);
}

/*
 *  Парсеры эффектов
 */

void Effects::ParseEffect(EffectsRosterPtr &info, parser_wrapper::DataNode node) {
	if (strcmp(node.GetName(), "damage") == 0) {
		ParseDamageEffect(info, node);
	} else if (strcmp(node.GetName(), "area") == 0) {
		ParseAreaEffect(info, node);
	} else if (strcmp(node.GetName(), "applies") == 0) {
		ParseAppliesEffect(info, node);
	}
}

void Effects::ParseDamageEffect(EffectsRosterPtr &info, parser_wrapper::DataNode &node) {
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

	info->insert({EEffect::kDamage, std::move(ptr)});
}

void Effects::ParseAreaEffect(EffectsRosterPtr &info, parser_wrapper::DataNode &node) {
	auto ptr = std::make_shared<effects::Area>();
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

	info->insert({EEffect::kArea, std::move(ptr)});
}

void Effects::ParseAppliesEffect(EffectsRosterPtr &info, parser_wrapper::DataNode &node) {
	auto ptr = std::make_shared<effects::Applies>();
	for (const auto &apply: node.Children("apply")) {
		try {
			auto location = parse::ReadAsConstant<EApply>(apply.GetValue("location"));
			auto val = parse::ReadAsInt(apply.GetValue("val"));
			auto lvl_bonus = parse::ReadAsDouble(apply.GetValue("lvl_bonus"));
			auto remort_bonus = parse::ReadAsDouble(apply.GetValue("remort_bonus"));
			auto cap = parse::ReadAsInt(apply.GetValue("cap"));
			ptr->applies_roster.emplace(location, Applies::Mod(val, cap, lvl_bonus, remort_bonus));
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in apply effect.", e.what());
		}
	}

	info->insert({EEffect::kApply, std::move(ptr)});
}

/*
 *  Геттеры эффектов
 */

std::optional<Damage> Effects::GetDmg() const {
	if (effects_->contains(EEffect::kDamage)) {
		return *std::dynamic_pointer_cast<Damage>(effects_->find(EEffect::kDamage)->second);
	} else {
		err_log("getting damage parameters from talent has no 'damage' effect section.");
		return std::nullopt;
	}
}

std::optional<Area> Effects::GetArea() const {
	if (effects_->contains(EEffect::kArea)) {
		return *std::dynamic_pointer_cast<Area>(effects_->find(EEffect::kArea)->second);
	} else {
		err_log("getting area parameters from talent has no 'area' effect section.");
		return std::nullopt;
	}
}

void Effects::ImposeApplies(CharData *ch) const {
	if (effects_->contains(effects::EEffect::kApply)) {
		auto ramge = effects_->equal_range(EEffect::kApply);
		for (auto it = ramge.first; it != ramge.second; ++it) {
			std::dynamic_pointer_cast<Applies>(it->second)->ImposeApplies(ch);
		}
	}
};

void Applies::ImposeApplies(CharData *ch) const {
	for (const auto &apply: applies_roster) {
		auto mod = static_cast<int>(apply.second.mod +
			apply.second.lvl_bonus * ch->GetLevel() +
			apply.second.remort_bonus * ch->get_remort());
		if (apply.second.cap) {
			if (apply.second.cap > 0) {
				mod = std::min(mod, apply.second.cap);
			} else {
				mod = std::max(mod, apply.second.cap);
			}
		}
		affect_modify(ch, apply.first, mod, EAffect::kUndefinded, true);
	}
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
