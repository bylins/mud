#include "spells_info.h"

#include "color.h"
#include "spells.h"
#include "structs/global_objects.h"

/*
 * Эта структура и все с ней связанное - часть нерабочей, но недовырезанной системы то ли создания свитков,
 * посохов и так далее, то ли их применения. Сейчас на нее завязано применение рун. По уму, после переписывания
 * системы рун надо или вырезать остатки, или наоборот - довести до рабочео состояния и подключить.
 */
std::unordered_map<ESpell, SpellCreate> spell_create;

namespace spells {

void InitSpellCreate(ESpell spell_id) {
	int i;

	for (i = 0; i < 3; i++) {
		spell_create[spell_id].wand.items[i] = -1;
		spell_create[spell_id].scroll.items[i] = -1;
		spell_create[spell_id].potion.items[i] = -1;
		spell_create[spell_id].items.items[i] = -1;
		spell_create[spell_id].runes.items[i] = -1;
	}

	spell_create[spell_id].wand.rnumber = -1;
	spell_create[spell_id].scroll.rnumber = -1;
	spell_create[spell_id].potion.rnumber = -1;
	spell_create[spell_id].items.rnumber = -1;
	spell_create[spell_id].runes.rnumber = -1;

	spell_create[spell_id].wand.min_caster_level = kLvlGreatGod;
	spell_create[spell_id].scroll.min_caster_level = kLvlGreatGod;
	spell_create[spell_id].potion.min_caster_level = kLvlGreatGod;
	spell_create[spell_id].items.min_caster_level = kLvlGreatGod;
	spell_create[spell_id].runes.min_caster_level = kLvlGreatGod;
}

void InitSpellsCreate() {
	for (const auto &spell : MUD::Spells()) {
		InitSpellCreate(spell.GetId());
	}
}

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
	auto id{ESpell::kUndefined};
	try {
		id = parse::ReadAsConstant<ESpell>(node.GetValue("id"));
	} catch (std::exception &e) {
		err_log("Incorrect spell id (%s).", e.what());
		return nullptr;
	}
	auto mode = SpellInfoBuilder::ParseItemMode(node, EItemMode::kEnabled);
	auto info = std::make_shared<SpellInfo>(id, mode);

	try {
		info->element_ = parse::ReadAsConstant<EElement>(node.GetValue("element"));
	} catch (std::exception &) {
	}

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

	if (node.GoToChild("targets")) {
		try {
			info->targets_ = parse::ReadAsConstantsBitvector<ETarget>(node.GetValue("val"));
		} catch (std::exception &e) {
			err_log("Incorrect 'mana' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}

	if (node.GoToChild("flags")) {
		try {
			info->flags_ = parse::ReadAsConstantsBitvector<EMagic>(node.GetValue("val"));
		} catch (std::exception &e) {
			err_log("Incorrect 'mana' section (spell: %s, value: %s).",
					NAME_BY_ITEM(info->GetId()).c_str(), e.what());
		}
		node.GoToParent();
	}

	return info;
}

bool SpellInfo::IsFlagged(Bitvector flag) const {
	return IS_SET(flags_, flag);
}

bool SpellInfo::AllowTarget(Bitvector target_type) const {
	return IS_SET(targets_, target_type);
}

void SpellInfo::Print(std::ostringstream &buffer) const {
	buffer << "Print spell:" << std::endl
		   << " Id: " << KGRN << NAME_BY_ITEM<ESpell>(GetId()) << KNRM
		   << " Mode: " << KGRN << NAME_BY_ITEM<EItemMode>(GetMode()) << KNRM << std::endl
		   << " Name (rus): " << KGRN << name_ << KNRM << std::endl
		   << " Name (eng): " << KGRN << name_eng_ << KNRM << std::endl
		   << " Element: " << KGRN << NAME_BY_ITEM<EElement>(element_) << KNRM << std::endl
		   << " Min position: " << KGRN << NAME_BY_ITEM<EPosition>(min_position_) << KNRM << std::endl
		   << " Violent: " << KGRN << (violent_ ? "Yes" : "No") << KNRM << std::endl
		   << " Danger: " << KGRN << danger_ << KNRM << std::endl
		   << " Mana min: " << KGRN << min_mana_ << KNRM
		   << " Mana max: " << KGRN << max_mana_ << KNRM
		   << " Mana change: " << KGRN << mana_change_ << KNRM << std::endl
		   << " Flags: " << KGRN << flags_ << KNRM << std::endl
		   << " Targets: " << KGRN << targets_ << KNRM << std::endl;
}

}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
