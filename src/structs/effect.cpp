//#include "effect.h"

#include "color.h"
#include "entities/char_data.h"
#include "utils/table_wrapper.h"
#include "global_objects.h"

namespace effects {

void TimerMod::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << " Timer mods:" << std::endl;
	table_wrapper::Table table;
	table << table_wrapper::kHeader
		  << "Type" << "Id" << "Name" << "Mod" << table_wrapper::kEndRow;
	for (const auto &timer_mod: skill_timer_mods) {
		table << "Skill"
			  << NAME_BY_ITEM<ESkill>(timer_mod.first)
			  << MUD::Skill(timer_mod.first).GetName()
			  << timer_mod.second
			  << table_wrapper::kEndRow;
	}
	for (const auto &timer_mod: feat_timer_mods) {
		table << "Feat"
			  << NAME_BY_ITEM<EFeat>(timer_mod.first)
			  << MUD::Feat(timer_mod.first).GetName()
			  << timer_mod.second
			  << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(buffer, table);
	buffer << std::endl;
}

void SkillMod::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << " Skill mods:" << std::endl;
	table_wrapper::Table table;
	table << table_wrapper::kHeader
		  << "Id" << "Name" << "Mod" << table_wrapper::kEndRow;
	for (const auto &mod: skill_mods) {
		table << NAME_BY_ITEM<ESkill>(mod.first)
			  << MUD::Skill(mod.first).GetName()
			  << mod.second
			  << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(buffer, table);
	buffer << std::endl;
}

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

void Applies::Print(CharData *ch, std::ostringstream &buffer) const {
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
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(buffer, table);
	buffer << std::endl;
}

void Effects::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << " Effect IDs: ";
	for (const auto &effect: *effects_) {
		buffer << to_underlying(effect.first) << " "; // ABYRVALG
	}
	buffer << std::endl;

	for (const auto &effect: *effects_) {
		effect.second->Print(ch, buffer);
	}
}

void Effects::Build(parser_wrapper::DataNode &node) {
	auto roster = std::make_unique<EffectsRoster>();
	for (auto &effect: node.Children()) {
		try {
			ParseEffect(roster, effect);
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
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
	} else if (strcmp(node.GetName(), "timer_mods") == 0) {
		ParseTimerMods(info, node);
	} else if (strcmp(node.GetName(), "skill_mods") == 0) {
		ParseSkillMods(info, node);
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

	info->insert({EEffect::kArea, std::move(ptr)});
}

void Effects::ParseAppliesEffect(EffectsRosterPtr &info, parser_wrapper::DataNode &node) {
	auto ptr = std::make_shared<Applies>();
	for (const auto &apply: node.Children("apply")) {
		auto location = parse::ReadAsConstant<EApply>(apply.GetValue("location"));
		auto val = parse::ReadAsInt(apply.GetValue("val"));
		auto lvl_bonus = parse::ReadAsDouble(apply.GetValue("lvl_bonus"));
		auto remort_bonus = parse::ReadAsDouble(apply.GetValue("remort_bonus"));
		auto cap = parse::ReadAsInt(apply.GetValue("cap"));
		ptr->applies_roster.emplace(location, Applies::Mod(val, cap, lvl_bonus, remort_bonus));
	}

	info->insert({EEffect::kApply, std::move(ptr)});
}

void Effects::ParseTimerMods(EffectsRosterPtr &info, parser_wrapper::DataNode node) {
	auto ptr = std::make_shared<TimerMod>();
	for (const auto &timer_mod: node.Children()) {
		auto mod = parse::ReadAsInt(timer_mod.GetValue("val"));
		if (strcmp(timer_mod.GetName(), "skill_timer") == 0) {
			auto id = parse::ReadAsConstant<ESkill>(timer_mod.GetValue("id"));
			ptr->skill_timer_mods.emplace(id, mod);
		} else if (strcmp(timer_mod.GetName(), "feat_timer") == 0) {
			auto id = parse::ReadAsConstant<EFeat>(timer_mod.GetValue("id"));
			ptr->feat_timer_mods.emplace(id, mod);
		}
	}

	if (!ptr->skill_timer_mods.empty() || !ptr->feat_timer_mods.empty()) {
		info->insert({EEffect::kTimerMod, std::move(ptr)});
	}
}

void Effects::ParseSkillMods(EffectsRosterPtr &info, parser_wrapper::DataNode node) {
	auto ptr = std::make_shared<SkillMod>();
	for (const auto &skill_mod: node.Children("skill")) {
		auto id = parse::ReadAsConstant<ESkill>(skill_mod.GetValue("id"));
		auto mod = parse::ReadAsInt(skill_mod.GetValue("val"));
		ptr->skill_mods.emplace(id, mod);
	}
	info->insert({EEffect::kSkillMod, std::move(ptr)});
}

/*
 *  Геттеры эффектов
 *  Area и Dmg возвпращаются только по одному, потому что для обраьотки их списка надо переписывать всю логику
 *  работы с арекастами, а даже, скорее, переносить ее внутрь эффекта (как сделано с Applies).
 */

std::optional<Damage> Effects::GetDmg() const {
	if (effects_->contains(EEffect::kDamage)) {
		return *std::static_pointer_cast<Damage>(effects_->find(EEffect::kDamage)->second);
	} else {
		err_log("getting damage parameters from talent has no 'damage' effect section.");
		return std::nullopt;
	}
}

std::optional<Area> Effects::GetArea() const {
	if (effects_->contains(EEffect::kArea)) {
		return *std::static_pointer_cast<Area>(effects_->find(EEffect::kArea)->second);
	} else {
		err_log("getting area parameters from talent has no 'area' effect section.");
		return std::nullopt;
	}
}

void Effects::ImposeApplies(CharData *ch) const {
	if (effects_->contains(EEffect::kApply)) {
		auto range = effects_->equal_range(EEffect::kApply);
		for (auto it = range.first; it != range.second; ++it) {
			std::static_pointer_cast<Applies>(it->second)->ImposeApplies(ch);
		}
	}
}

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

int Effects::GetSkillMod(ESkill skill_id) const {
	int mod{0};
	if (effects_->contains(EEffect::kSkillMod)) {
		auto range = effects_->equal_range(EEffect::kSkillMod);
		for (auto it = range.first; it != range.second; ++it) {
			mod += std::static_pointer_cast<SkillMod>(it->second)->GetMod(skill_id);
		}
	}
	return mod;
}

int SkillMod::GetMod(ESkill skill_id) const {
	//err_log("Get mod for skill '%s'.\r\n", NAME_BY_ITEM<ESkill>(skill_id).c_str());
	if (skill_mods.contains(skill_id)) {
		return skill_mods.at(skill_id);
	}
	return 0;
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
