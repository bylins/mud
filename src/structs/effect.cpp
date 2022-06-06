#include "effect.h"

#include "color.h"
#include "entities/char_data.h"
#include "global_objects.h"

namespace effects {

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

void Effects::Print(CharData *ch, std::ostringstream &buffer) const {
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

// ====================================================================================================================
// ====================================================================================================================
// ====================================================================================================================

template<class IdType>
class Mod {
 public:
	using Mods = std::map<IdType, int>;
	Mod() = default;
	explicit Mod(parser_wrapper::DataNode &node) {
		for (const auto &mod: node.Children()) {
			auto val = parse::ReadAsInt(mod.GetValue("val"));
			auto id = parse::ReadAsConstant<IdType>(mod.GetValue("id"));
			mods_.emplace(id, val);
		}
	}

	[[nodiscard]] bool Empty() const {
		return mods_.empty();
	}

	[[nodiscard]] const Mods &GetMods() const {
		return mods_;
	};

	[[nodiscard]] int GetMod(IdType id) const {
		auto it = mods_.find(id);
		if (it != mods_.end()) {
			return it->second;
		}
		return 0;
	};

	void Print(CharData *ch, std::ostringstream &buffer) const {
		table_wrapper::Table table;
		table << table_wrapper::kHeader << "Id" << "Mod" << table_wrapper::kEndRow;
		for (const auto &mod: mods_) {
			table << NAME_BY_ITEM<IdType>(mod.first)
				  << mod.second
				  << table_wrapper::kEndRow;
		}
		table_wrapper::DecorateNoBorderTable(ch, table);
		table_wrapper::PrintTableToStream(buffer, table);
		buffer << std::endl;
	};

 private:
	Mods mods_;
};

class Applies {
	struct ApplyMod {
		EAffect affect{EAffect::kUndefinded};
		int mod{0};
		int cap{0};
		double lvl_bonus{0.0};
		double remort_bonus{0.0};

		ApplyMod() = default;
		ApplyMod(EAffect affect, int mod, int cap, double lvl_bonus, double remort_bonus)
			: affect(affect), mod(mod), cap(cap), lvl_bonus(lvl_bonus), remort_bonus(remort_bonus) {};
	};

	std::unordered_map<EApply, ApplyMod> applies_roster_;

 public:
	Applies() = default;
	explicit Applies(parser_wrapper::DataNode &node);

	void Print(CharData *ch, std::ostringstream &buffer) const;
	void Impose(CharData *ch) const;
};

class PassiveEffects::PassiveEffectsImpl {
	Applies applies_;
	Mod<ESkill> skill_mods_;
	Mod<ESkill> skill_timer_mods_;
	Mod<EFeat> feat_timer_mods_;

	void ParseEffect(parser_wrapper::DataNode &node);
	void ParseApplies(parser_wrapper::DataNode &node);
	void ParseSkillTimerMods(parser_wrapper::DataNode &node);
	void ParseFeatTimerMods(parser_wrapper::DataNode &node);
	void ParseSkillMods(parser_wrapper::DataNode &node);

 public:
	PassiveEffectsImpl() = delete;
	explicit PassiveEffectsImpl(parser_wrapper::DataNode &node);

	void ImposeApplies(CharData *ch) const;
	void ImposeSkillsMods(CharData *ch) const;
	[[nodiscard]] int GetTimerMod(ESkill skill_id) const;
	[[nodiscard]] int GetTimerMod(EFeat feat_id) const;

	void Print(CharData *ch, std::ostringstream &buffer) const;
};

PassiveEffects::PassiveEffectsImpl::PassiveEffectsImpl(parser_wrapper::DataNode &node) {
	for (auto &effect: node.Children()) {
		try {
			ParseEffect(effect);
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
		}
	}
}

void PassiveEffects::PassiveEffectsImpl::ParseEffect(parser_wrapper::DataNode &node) {
	if (strcmp(node.GetName(), "applies") == 0) {
		ParseApplies(node);
	} else if (strcmp(node.GetName(), "skill_timer_mods") == 0) {
		ParseSkillTimerMods(node);
	} else if (strcmp(node.GetName(), "feat_timer_mods") == 0) {
		ParseFeatTimerMods(node);
	} else if (strcmp(node.GetName(), "skill_mods") == 0) {
		ParseSkillMods(node);
	}
}

void PassiveEffects::PassiveEffectsImpl::ParseApplies(parser_wrapper::DataNode &node) {
	applies_ = Applies(node);
}

void PassiveEffects::PassiveEffectsImpl::ParseSkillTimerMods(parser_wrapper::DataNode &node) {
	skill_timer_mods_ = Mod<ESkill>(node);
}

void PassiveEffects::PassiveEffectsImpl::ParseFeatTimerMods(parser_wrapper::DataNode &node) {
	feat_timer_mods_ = Mod<EFeat>(node);
}

void PassiveEffects::PassiveEffectsImpl::ParseSkillMods(parser_wrapper::DataNode &node) {
	auto ptr = std::make_unique<Mod<ESkill> >(node);
	skill_mods_ = Mod<ESkill>(node);
}

void PassiveEffects::PassiveEffectsImpl::ImposeApplies(CharData *ch) const {
	applies_.Impose(ch);
}

void PassiveEffects::PassiveEffectsImpl::ImposeSkillsMods(CharData *ch) const {
	if (skill_mods_.Empty()) {
		return;
	}
	for (const auto &skill_mod : skill_mods_.GetMods()) {
		ch->SetAddSkill(skill_mod.first, skill_mod.second);
	}
}

int PassiveEffects::PassiveEffectsImpl::GetTimerMod(ESkill skill_id) const {
	return skill_timer_mods_.GetMod(skill_id);
}

int PassiveEffects::PassiveEffectsImpl::GetTimerMod(EFeat feat_id) const {
	return feat_timer_mods_.GetMod(feat_id);
}

void PassiveEffects::PassiveEffectsImpl::Print(CharData *ch, std::ostringstream &buffer) const {
	applies_.Print(ch, buffer);
	if (!skill_mods_.Empty()) {
		buffer << " Skill mods:" << std::endl;
		skill_mods_.Print(ch, buffer);
	}
	if (!skill_timer_mods_.Empty()) {
		buffer << " Skills timer mods:" << std::endl;
		skill_timer_mods_.Print(ch, buffer);
	}
	if (!feat_timer_mods_.Empty()) {
		buffer << " Feats timer mods:" << std::endl;
		feat_timer_mods_.Print(ch, buffer);
	}
}

Applies::Applies(parser_wrapper::DataNode &node) {
	for (const auto &apply: node.Children()) {
		try {
			auto location = parse::ReadAsConstant<EApply>(apply.GetValue("location"));
			auto affect = parse::ReadAsConstant<EAffect>(apply.GetValue("affect"));
			auto val = parse::ReadAsInt(apply.GetValue("val"));
			auto lvl_bonus = parse::ReadAsDouble(apply.GetValue("lvl_bonus"));
			auto remort_bonus = parse::ReadAsDouble(apply.GetValue("remort_bonus"));
			auto cap = parse::ReadAsInt(apply.GetValue("cap"));
			applies_roster_.emplace(location, Applies::ApplyMod(affect, val, cap, lvl_bonus, remort_bonus));
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
		}
	}
}

void Applies::Print(CharData *ch, std::ostringstream &buffer) const {
	if (applies_roster_.empty()) {
		return;
	}

	buffer << std::endl << " Applies:" << std::endl;
	table_wrapper::Table table;
	table << table_wrapper::kHeader
		  << "Location" << "Affect" << "Mod" << "Level bonus" << "Remort bonus" << "Cap" << table_wrapper::kEndRow;
	for (const auto &apply: applies_roster_) {
		table << NAME_BY_ITEM<EApply>(apply.first)
			  << NAME_BY_ITEM<EAffect>(apply.second.affect)
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

void Applies::Impose(CharData *ch) const {
	for (const auto &apply: applies_roster_) {
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
		affect_modify(ch, apply.first, mod, apply.second.affect, true);
	}
}

void PassiveEffects::Build(parser_wrapper::DataNode &node) {
	try {
		auto ptr = std::make_unique<PassiveEffectsImpl>(node);
		pimpl_ = std::move(ptr);
	} catch (std::exception &e) {
		err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
	}
}

PassiveEffects::PassiveEffects() = default;
PassiveEffects::~PassiveEffects() = default;

void PassiveEffects::Print(CharData *ch, std::ostringstream &buffer) const {
	if (pimpl_) {
		pimpl_->Print(ch, buffer);
	}
}

void PassiveEffects::ImposeApplies(CharData *ch) const {
	if (pimpl_) {
		pimpl_->ImposeApplies(ch);
	}
}

void PassiveEffects::ImposeSkillsMods(CharData *ch) const {
	if (pimpl_) {
		pimpl_->ImposeSkillsMods(ch);
	}
}

int PassiveEffects::GetTimerMod(ESkill skill_id) const {
	if (pimpl_) {
		return pimpl_->GetTimerMod(skill_id);
	}
	return 0;
}

int PassiveEffects::GetTimerMod(EFeat feat_id) const {
	if (pimpl_) {
		return pimpl_->GetTimerMod(feat_id);
	}
	return 0;
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
