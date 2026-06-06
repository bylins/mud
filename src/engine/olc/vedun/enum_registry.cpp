/**
 \file enum_registry.cpp - a part of the Bylins engine.
 \brief Vedun editor: runtime enum-by-type-name registry (issue.vedun-editor Phase 2).
*/

#include "enum_registry.h"

#include "gameplay/magic/spells_constants.h"   // ESpell/EElement/EMagic/ETarget/ESpellType + NAMES_OF
#include "gameplay/magic/spell_messages.h"     // ESpellMsg + NAMES_OF
#include "gameplay/skills/skill_messages.h"    // ESkillMsg + NAMES_OF
#include "gameplay/abilities/feat_messages.h"   // EFeat + EFeatMsg + NAMES_OF
#include "gameplay/mechanics/guild_messages.h"   // guilds::EMsg + NAMES_OF
#include "gameplay/fight/fight_messages.h"     // EFightMsg/EDamageSource + NAMES_OF
#include "engine/entities/entities_constants.h" // EPosition + NAMES_OF
#include "engine/structs/info_container.h"      // EItemMode + NAMES_OF
#include "gameplay/affects/affect_contants.h" // EAffFlag + NAMES_OF
#include "gameplay/skills/skills.h"           // ESkill + NAMES_OF

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
	registry.Register<ESkill>("ESkill");
	registry.Register<EBaseStat>("EBaseStat");
	registry.Register<ESaving>("ESaving");
	registry.Register<EResist>("EResist");
	registry.Register<ENpcRace>("ENpcRace");        // issue.npc-races: mob_race scheme
	registry.Register<EMobFlag>("EMobFlag");        // issue.npc-races: <mob_flags>
	registry.Register<ENpcFlag>("ENpcFlag");        // issue.npc-races: <npc_flags>
	registry.Register<EAffect>("EAffect");
	registry.Register<EApply>("EApply");
	registry.Register<ESpellMsg>("ESpellMsg");
	registry.Register<ESkillMsg>("ESkillMsg");
	registry.Register<EFeat>("EFeat");
	registry.Register<EFeatMsg>("EFeatMsg");
	registry.Register<guilds::EMsg>("EGuildMsg");
	registry.Register<fight::EDamageSource>("EDamageSource");
	registry.Register<fight::EFightMsg>("EFightMsg");
	// Inline-strcmp enums (no NAMES_OF map) registered by explicit name list -- keep in sync with
	// the parser in talents_actions.cpp (Actions::ParseAction / the align reader).
	registry.RegisterNames("EActionTarget", {"kTarFightSelf", "kTarFightVict", "kTarGroup", "kTarFoes",
		"kTarRandomFoe", "kTarRandomAlly", "kTarMinions", "kTarSame", "kTarRoomThis"});
	registry.RegisterNames("EActionBase", {"kDamage", "kPoints", "kAffects", "kDispelled", "kCompetence"});
	registry.RegisterNames("EAlign", {"kGood", "kEvil", "kNeutral"});
	// <misc violent> is stored as the literal Y/N/A the spell loader parses into EViolent.
	registry.RegisterNames("EViolent", {"Y", "N", "A"});
}

} // namespace vedun

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
