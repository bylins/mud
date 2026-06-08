/**
 \file enum_registry.cpp - a part of the Bylins engine.
 \brief Vedun editor: runtime enum-by-type-name registry (issue.vedun-editor Phase 2).
*/

#include "enum_registry.h"

#include "gameplay/magic/spells_constants.h"   // ESpell/EElement/EMagic/ETarget/ESpellType + NAMES_OF
#include "gameplay/magic/magic.h"               // issue.vedun-hotfix #5: *HandlerNames() registries
#include "gameplay/magic/spell_messages.h"     // ESpellMsg + NAMES_OF
#include "gameplay/skills/skill_messages.h"    // ESkillMsg + NAMES_OF
#include "gameplay/abilities/feat_messages.h"   // EFeat + EFeatMsg + NAMES_OF
#include "gameplay/mechanics/guild_messages.h"   // guilds::EMsg + NAMES_OF
#include "gameplay/ai/special_messages.h"          // specials::ESpecialMsg/EBankMsg + NAMES_OF
#include "gameplay/communication/social.h"        // ESocialMsg + NAMES_OF
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
	registry.Register<ESocialMsg>("ESocialMsg");
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
	// issue.vedun-hotfix #4: ERoomFlag has no NAMES_OF map (multi-int Bitvector with bit-group markers
	// + exit/sector tokens mixed in), so register the room-flag subset by name for the <room_flags>
	// flagset menu/validation (flagsets key on member names, not values).
	registry.RegisterNames("ERoomFlag", {"kDarked", "kDeathTrap", "kNoEntryMob", "kIndoors", "kPeaceful",
		"kSoundproof", "kNoTrack", "kNoMagic", "kTunnel", "kNoTeleportIn", "kGodsRoom", "kLevelGod",
		"kHouse", "kHouseCrash", "kHouseEntry", "kOlc", "kBfsMark", "kForMages", "kForSorcerers",
		"kForThieves", "kForWarriors", "kForAssasines", "kForGuards", "kForPaladines", "kForRangers",
		"kForPoly", "kForMono", "kForge", "kForMerchants", "kForMaguses", "kArena", "kNoSummonOut",
		"kNoTeleportOut", "kNohorse", "kNoWeather", "kSlowDeathTrap", "kIceTrap", "kNoRelocateIn",
		"kTribune", "kArenaSend", "kNoBattle", "kAlwaysLit", "kMoMapper", "kNoItem", "kDominationArena"});
	// issue.vedun-hotfix #5: registered spell action handlers, so the editor offers a pick-list (and
	// rejects unknown values) for <manual_cast/summon/obj_creation/alter_obj handler="...">.
	registry.RegisterNames("ManualHandler", ManualHandlerNames());
	registry.RegisterNames("SummonHandler", SummonHandlerNames());
	registry.RegisterNames("AlterObjHandler", AlterObjHandlerNames());
	registry.RegisterNames("ObjCreationHandler", ObjCreationHandlerNames());
	// issue.specials: the <special id=> tokens that may be assigned to a mob in specials.xml (the
	// mob-handler subset of ESpecial; kShop/kGuild/kBoard come from other sources, not the file).
	registry.RegisterNames("ESpecial", {"kRent", "kMail", "kBank", "kHorse", "kExchange", "kTorc",
		"kMercenary", "kOutfit", "kPuff"});
	// issue.specials Phase 2: special-procedure message enums (special_msg.xml / bank_msg.xml).
	registry.Register<specials::ESpecialMsg>("ESpecialMsg");
	registry.Register<specials::EBankMsg>("EBankMsg");
	registry.Register<specials::EMailMsg>("EMailMsg");
}

} // namespace vedun

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
