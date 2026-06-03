/**
 \file enum_registry.cpp - a part of the Bylins engine.
 \brief Vedun editor: runtime enum-by-type-name registry (issue.vedun-editor Phase 2).
*/

#include "enum_registry.h"

#include "gameplay/magic/spells_constants.h"   // ESpell/EElement/EMagic/ETarget/ESpellType + NAMES_OF
#include "engine/entities/entities_constants.h" // EPosition + NAMES_OF
#include "engine/structs/info_container.h"      // EItemMode + NAMES_OF
#include "gameplay/affects/affect_contants.h" // EAffFlag + NAMES_OF

namespace vedun {

EnumRegistry &EnumRegistry::Instance() {
	static EnumRegistry instance;
	return instance;
}

bool EnumRegistry::Known(const std::string &type_name) const {
	return registry_.find(type_name) != registry_.end();
}

const std::vector<EnumMember> *EnumRegistry::Members(const std::string &type_name) const {
	const auto it = registry_.find(type_name);
	return it == registry_.end() ? nullptr : &it->second;
}

const std::string *EnumRegistry::NameOf(const std::string &type_name, long value) const {
	if (const auto *members = Members(type_name)) {
		for (const auto &member : *members) {
			if (member.value == value) {
				return &member.name;
			}
		}
	}
	return nullptr;
}

std::optional<long> EnumRegistry::ValueOf(const std::string &type_name, const std::string &member) const {
	if (const auto *members = Members(type_name)) {
		for (const auto &entry : *members) {
			if (entry.name == member) {
				return entry.value;
			}
		}
	}
	return std::nullopt;
}

void RegisterEditorEnums() {
	auto &registry = EnumRegistry::Instance();
	// The enums the spell scheme references. Each needs a NAMES_OF specialization (Phase 0a);
	// extend here (and add the NAMES_OF specialization) as more schemes/enums come online.
	registry.Register<ESpell>("ESpell");
	registry.Register<EElement>("EElement");
	registry.Register<EMagic>("EMagic");
	registry.Register<ETarget>("ETarget");
	registry.Register<ESpellType>("ESpellType");
	registry.Register<EPosition>("EPosition");
	registry.Register<EItemMode>("EItemMode");
	registry.Register<EAffFlag>("EAffFlag");
}

} // namespace vedun

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
