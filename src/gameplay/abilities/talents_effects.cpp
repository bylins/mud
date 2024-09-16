#include "talents_effects.h"

//#include "engine/ui/color.h"
#include "engine/entities/char_data.h"
#include "engine/db/global_objects.h"

namespace talents_effects {

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
		buffer << "\r\n";
	};

 private:
	Mods mods_;
};

class Applies {
	struct ApplyMod {
		EApply location{EApply::kNone};
		EAffect affect{EAffect::kUndefinded};
		int mod{0};
		int cap{0};
		double lvl_bonus{0.0};
		double remort_bonus{0.0};

		ApplyMod() = default;
		ApplyMod(EApply location, EAffect affect, int mod, int cap, double lvl_bonus, double remort_bonus)
			: location(location),
			  affect(affect),
			  mod(mod),
			  cap(cap),
			  lvl_bonus(lvl_bonus),
			  remort_bonus(remort_bonus) {};
	};

	std::vector<ApplyMod> applies_;

 public:
	Applies() = default;
	explicit Applies(parser_wrapper::DataNode &node);

	void Print(CharData *ch, std::ostringstream &buffer) const;
	void Impose(CharData *ch) const;
};

class Effects::PassiveEffectsImpl {
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

Effects::PassiveEffectsImpl::PassiveEffectsImpl(parser_wrapper::DataNode &node) {
	for (auto &effect: node.Children()) {
		try {
			ParseEffect(effect);
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
		}
	}
}

void Effects::PassiveEffectsImpl::ParseEffect(parser_wrapper::DataNode &node) {
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

void Effects::PassiveEffectsImpl::ParseApplies(parser_wrapper::DataNode &node) {
	applies_ = Applies(node);
}

void Effects::PassiveEffectsImpl::ParseSkillTimerMods(parser_wrapper::DataNode &node) {
	skill_timer_mods_ = Mod<ESkill>(node);
}

void Effects::PassiveEffectsImpl::ParseFeatTimerMods(parser_wrapper::DataNode &node) {
	feat_timer_mods_ = Mod<EFeat>(node);
}

void Effects::PassiveEffectsImpl::ParseSkillMods(parser_wrapper::DataNode &node) {
	auto ptr = std::make_unique<Mod<ESkill> >(node);
	skill_mods_ = Mod<ESkill>(node);
}

void Effects::PassiveEffectsImpl::ImposeApplies(CharData *ch) const {
	applies_.Impose(ch);
}

void Effects::PassiveEffectsImpl::ImposeSkillsMods(CharData *ch) const {
	if (skill_mods_.Empty()) {
		return;
	}
	for (const auto &skill_mod: skill_mods_.GetMods()) {
		ch->SetAddSkill(skill_mod.first, skill_mod.second);
	}
}

int Effects::PassiveEffectsImpl::GetTimerMod(ESkill skill_id) const {
	return skill_timer_mods_.GetMod(skill_id);
}

int Effects::PassiveEffectsImpl::GetTimerMod(EFeat feat_id) const {
	return feat_timer_mods_.GetMod(feat_id);
}

void Effects::PassiveEffectsImpl::Print(CharData *ch, std::ostringstream &buffer) const {
	applies_.Print(ch, buffer);
	if (!skill_mods_.Empty()) {
		buffer << " Skill mods:" << "\r\n";
		skill_mods_.Print(ch, buffer);
	}
	if (!skill_timer_mods_.Empty()) {
		buffer << " Skills timer mods:" << "\r\n";
		skill_timer_mods_.Print(ch, buffer);
	}
	if (!feat_timer_mods_.Empty()) {
		buffer << " Feats timer mods:" << "\r\n";
		feat_timer_mods_.Print(ch, buffer);
	}
}

Applies::Applies(parser_wrapper::DataNode &node) {
	for (const auto &apply: node.Children()) {
		try {
			auto location = parse::ReadAsConstant<EApply>(apply.GetValue("location"));
			auto affect = parse::ReadAsConstant<EAffect>(apply.GetValue("affect"));
			auto mod = parse::ReadAsInt(apply.GetValue("val"));
			auto cap = parse::ReadAsInt(apply.GetValue("cap"));
			auto lvl_bonus = parse::ReadAsDouble(apply.GetValue("lvl_bonus"));
			auto remort_bonus = parse::ReadAsDouble(apply.GetValue("remort_bonus"));
			applies_.emplace_back(Applies::ApplyMod(location, affect, mod, cap, lvl_bonus, remort_bonus));
		} catch (std::exception &e) {
			err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
		}
	}
}

void Applies::Print(CharData *ch, std::ostringstream &buffer) const {
	if (applies_.empty()) {
		return;
	}

	buffer << "\r\n" << " Applies:" << "\r\n";
	table_wrapper::Table table;
	table << table_wrapper::kHeader
		  << "Location" << "Affect" << "Mod" << "Level bonus" << "Remort bonus" << "Cap" << table_wrapper::kEndRow;
	for (const auto &apply: applies_) {
		table << NAME_BY_ITEM<EApply>(apply.location)
			  << NAME_BY_ITEM<EAffect>(apply.affect)
			  << apply.mod
			  << apply.lvl_bonus
			  << apply.remort_bonus
			  << apply.cap
			  << table_wrapper::kEndRow;
	}
	table_wrapper::DecorateNoBorderTable(ch, table);
	table_wrapper::PrintTableToStream(buffer, table);
	buffer << "\r\n";
}

void Applies::Impose(CharData *ch) const {
	for (const auto &apply: applies_) {
		auto mod = static_cast<int>(apply.mod);
		if (IsNegativeApply(apply.location))
			mod -= apply.lvl_bonus * GetRealLevel(ch) + apply.remort_bonus * GetRealRemort(ch);
		else
			mod += apply.lvl_bonus * GetRealLevel(ch) + apply.remort_bonus * GetRealRemort(ch);
		if (apply.cap) {
			if (apply.cap > 0) {
				mod = std::min(mod, apply.cap);
			} else {
				mod = std::max(mod, apply.cap);
			}
		}
		affect_modify(ch, apply.location, mod, apply.affect, true);
	}
}

void Effects::Build(parser_wrapper::DataNode &node) {
	try {
		auto ptr = std::make_unique<PassiveEffectsImpl>(node);
		pimpl_ = std::move(ptr);
	} catch (std::exception &e) {
		err_log("Incorrect value '%s' in '%s'.", e.what(), node.GetName());
	}
}

Effects::Effects() = default;
Effects::~Effects() = default;

void Effects::Print(CharData *ch, std::ostringstream &buffer) const {
	if (pimpl_) {
		pimpl_->Print(ch, buffer);
	}
}

void Effects::ImposeApplies(CharData *ch) const {
	if (pimpl_) {
		pimpl_->ImposeApplies(ch);
	}
}

void Effects::ImposeSkillsMods(CharData *ch) const {
	if (pimpl_) {
		pimpl_->ImposeSkillsMods(ch);
	}
}

int Effects::GetTimerMod(ESkill skill_id) const {
	if (pimpl_) {
		return pimpl_->GetTimerMod(skill_id);
	}
	return 0;
}

int Effects::GetTimerMod(EFeat feat_id) const {
	if (pimpl_) {
		return pimpl_->GetTimerMod(feat_id);
	}
	return 0;
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
