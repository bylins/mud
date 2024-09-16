//#include "spells_info.h"

#include "engine/ui/color.h"
//#include "spells.h"
#include "engine/db/global_objects.h"

/*
 * Эта структура и все с ней связанное - часть нерабочей, но недовырезанной системы то ли создания свитков,
 * посохов и так далее, то ли их применения. Сейчас на нее завязано применение рун. По уму, после переписывания
 * системы рун надо или вырезать остатки, или наоборот - довести до рабочего состояния и подключить.
 */
std::map<ESpell, SpellCreate> spell_create;

namespace spells {

using ItemPtr = SpellInfoBuilder::ItemPtr;

void SpellsLoader::Load(DataNode data) {
	MUD::Spells().Init(data.Children());
}

void SpellsLoader::Reload(DataNode data) {
	MUD::Spells().Reload(data.Children());
}

ItemPtr SpellInfoBuilder::Build(DataNode &node) {
	try {
		return ParseSpell(node);
	} catch (std::exception &e) {
		err_log("Spell parsing error (incorrect value '%s')", e.what());
		return nullptr;
	}
}

ItemPtr SpellInfoBuilder::ParseSpell(DataNode node) {
	auto info = ParseHeader(node);

	ParseName(info, node);
	ParseMisc(info, node);
	ParseMana(info, node);
	ParseTargets(info, node);
	ParseFlags(info, node);
	ParseActions(info, node);

	return info;
}

ItemPtr SpellInfoBuilder::ParseHeader(DataNode &node) {
	auto id{ESpell::kUndefined};
	try {
		id = parse::ReadAsConstant<ESpell>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect spell id (%s).", e.what());
		throw;
	}
	auto mode = SpellInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);

	auto info = std::make_shared<SpellInfo>(id, mode);
	try {
		info->element_ = parse::ReadAsConstant<EElement>(node.GetValue("element"));
	} catch (std::exception &) {
	}

	return info;
}

void SpellInfoBuilder::ParseName(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("name")) {
		try {
			info->name_ = parse::ReadAsStr(node.GetValue("rus"));
			info->name_eng_ = parse::ReadAsStr(node.GetValue("eng"));
		} catch (std::exception &e) {
			err_log("Incorrect 'name' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
}

void SpellInfoBuilder::ParseMisc(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("misc")) {
		try {
			info->min_position_ = parse::ReadAsConstant<EPosition>(node.GetValue("pos"));
			info->violent_ = parse::ReadAsBool(node.GetValue("violent"));
			info->danger_ = parse::ReadAsInt(node.GetValue("danger"));
		} catch (std::exception &e) {
			err_log("Incorrect 'misc' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
}

void SpellInfoBuilder::ParseMana(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("mana")) {
		try {
			info->min_mana_ = parse::ReadAsInt(node.GetValue("min"));
			info->max_mana_ = parse::ReadAsInt(node.GetValue("max"));
			info->mana_change_ = parse::ReadAsInt(node.GetValue("change"));
		} catch (std::exception &e) {
			err_log("Incorrect 'mana' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
}

void SpellInfoBuilder::ParseTargets(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("targets")) {
		try {
			info->targets_ = parse::ReadAsConstantsBitvector<ETarget>(node.GetValue("val"));
		} catch (std::exception &e) {
			err_log("Incorrect 'targets' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
}

void SpellInfoBuilder::ParseFlags(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("flags")) {
		try {
			info->flags_ = parse::ReadAsConstantsBitvector<EMagic>(node.GetValue("val"));
		} catch (std::exception &e) {
			err_log("Incorrect 'flags' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}
}

void SpellInfoBuilder::ParseActions(ItemPtr &info, DataNode &node) {
	if (node.GoToChild("talent_actions")) {
		info->actions.Build(node);
		node.GoToParent();
	}
}

bool SpellInfo::IsFlagged(const Bitvector flag) const {
	return IS_SET(flags_, flag);
}

bool SpellInfo::AllowTarget(const Bitvector target_type) const {
	return IS_SET(targets_, target_type);
}

void SpellInfo::Print(CharData *ch, std::ostringstream &buffer) const {
	buffer << "Print spell:" << "\r\n"
		   << " Id: " << kColorGrn << NAME_BY_ITEM<ESpell>(GetId()) << kColorNrm
		   << " Mode: " << kColorGrn << NAME_BY_ITEM<EItemMode>(GetMode()) << kColorNrm << "\r\n"
		   << " Name (rus): " << kColorGrn << name_ << kColorNrm << "\r\n"
		   << " Name (eng): " << kColorGrn << name_eng_ << kColorNrm << "\r\n"
		   << " Element: " << kColorGrn << NAME_BY_ITEM<EElement>(element_) << kColorNrm << "\r\n"
		   << " Min position: " << kColorGrn << NAME_BY_ITEM<EPosition>(min_position_) << kColorNrm << "\r\n"
		   << " Violent: " << kColorGrn << (violent_ ? "Yes" : "No") << kColorNrm << "\r\n"
		   << " Danger: " << kColorGrn << danger_ << kColorNrm << "\r\n"
		   << " Mana min: " << kColorGrn << min_mana_ << kColorNrm
		   << " Mana max: " << kColorGrn << max_mana_ << kColorNrm
		   << " Mana change: " << kColorGrn << mana_change_ << kColorNrm << "\r\n"
		   << " Flags: " << kColorGrn << parse::BitvectorToString<EMagic>(flags_) << kColorNrm << "\r\n"
		   << " Targets: " << kColorGrn << parse::BitvectorToString<ETarget>(targets_) << kColorNrm << "\r\n\r\n";

	actions.Print(ch, buffer);
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
