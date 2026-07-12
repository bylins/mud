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
#include "gameplay/classes/classes_constants.h"   // ECharClass + NAME_BY_ITEM
#include "gameplay/economics/currencies.h"         // currency text_ids -> ECurrencyId
#include "gameplay/economics/shop_ext.h"            // shop item-set ids -> ShopItemSetId
#include "engine/db/global_objects.h"              // MUD::Currencies()
#include "gameplay/mechanics/rune_stones.h"      // ERuneStoneMsg + NAMES_OF
#include "engine/entities/entities_constants.h" // EPosition + NAMES_OF
#include "engine/structs/info_container.h"      // EItemMode + NAMES_OF
#include "gameplay/affects/affect_contants.h" // EAffFlag + NAMES_OF
#include "gameplay/affects/affect_messages.h"  // EAffectMsgType + NAMES_OF (affect_msg scheme)
#include "gameplay/magic/magic_rooms.h"       // room_spells::ERoomAffect + NAMES_OF
#include "gameplay/magic/room_affect_messages.h"  // ERoomAffectMsgType + NAMES_OF (room_affect_msg scheme)
#include "gameplay/skills/skills.h"           // ESkill + NAMES_OF
#include "engine/core/config.h"               // ECommonMsg + NAMES_OF

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
	registry.Register<affects::EAffectMsgType>("EAffectMsgType");   // issue.unstable-hotfixes: affect_msg scheme
	registry.Register<ESkillMsg>("ESkillMsg");
	registry.Register<EFeat>("EFeat");
	registry.Register<EFeatMsg>("EFeatMsg");
	registry.Register<guilds::EMsg>("EGuildMsg");
	registry.Register<fight::EDamageSource>("EDamageSource");
	registry.Register<fight::EFightMsg>("EFightMsg");
	registry.Register<room_spells::ERoomAffect>("ERoomAffect");   // issue.room-affect-trigger-improve: room_affects scheme
	registry.Register<room_spells::ERoomAffectMsgType>("ERoomAffectMsgType");   // issue.unstable-hotfixes: room_affect_msg scheme
	// Inline-strcmp enums (no NAMES_OF map) registered by explicit name list -- keep in sync with
	// the parser in talents_actions.cpp (Actions::ParseAction / the align reader).
	registry.RegisterNames("EActionTarget", {"kTarFightSelf", "kTarFightVict", "kTarGroup", "kTarFoes",
		"kTarRandomFoe", "kTarRandomAlly", "kTarMinions", "kTarActor", "kTarSame", "kTarRoomThis"});
	registry.RegisterNames("EActionTrigger",
		{"kPulse", "kBattlePulse", "kEnter", "kEnterPC", "kEnterNPC", "kPick", "kUnlock", "kOpen",
		 "kClose", "kLock"});
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
	registry.Register<specials::EHorseMsg>("EHorseMsg");
	registry.Register<specials::ETorcMsg>("ETorcMsg");
	registry.Register<specials::EMercMsg>("EMercMsg");
	registry.Register<specials::EExchMsg>("EExchMsg");
	registry.Register<specials::ERentMsg>("ERentMsg");
	registry.Register<specials::EShopMsg>("EShopMsg");
	registry.Register<specials::EBoardMsg>("EBoardMsg");
	registry.Register<ECommonMsg>("ECommonMsg");
	registry.Register<ERuneStoneMsg>("ERuneStoneMsg");
	// Player classes (no NAMES_OF map): build the name list 0..kLast so guild <class val=...>
	// flagsets validate against real classes.
	{
		std::vector<std::string> class_names;
		for (int c = 0; c <= to_underlying(ECharClass::kLast); ++c) {
			class_names.push_back(NAME_BY_ITEM<ECharClass>(static_cast<ECharClass>(c)));
		}
		registry.RegisterNames("ECharClass", class_names);
	}
	// Registered currency text_ids (dynamic), so any "currency=" reference (e.g. guild prices)
	// is validated against real currencies -- catches typos like a stale "kGrivna".
	{
		std::vector<std::string> currency_ids;
		// kUndefined = "inherit": a guild talent price set to it falls back to the guild-wide currency
		// (and a guild with no currency of its own falls back to gold).
		currency_ids.emplace_back("kUndefined");
		for (const auto &cur : MUD::Currencies()) {
			if (cur.GetId() >= 0) {
				currency_ids.push_back(cur.GetTextId());
			}
		}
		registry.RegisterNames("ECurrencyId", currency_ids);
	}
	// Registered shop item-set ids (dynamic): a shop's <item_set id=> is validated against --
	// and offered from -- the real catalog (cfg/economics/shop_item_sets.xml).
	{
		std::vector<std::string> item_set_ids;
		for (const auto &set : ShopExt::item_sets()) {
			item_set_ids.push_back(set->_id);
		}
		registry.RegisterNames("ShopItemSetId", item_set_ids);
	}
	registry.RegisterNames("EGender", {"kNeutral", "kMale", "kFemale", "kPoly"});
	registry.RegisterNames("ECase", {"kNom", "kGen", "kDat", "kAcc", "kIns", "kPre"});
}

} // namespace vedun

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
