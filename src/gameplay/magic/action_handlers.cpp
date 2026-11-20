/**
 \file action_handlers.cpp - a part of the Bylins engine.
 \brief Handler-driven stages of the cast pipeline: object alteration, object creation and hand-coded
        manual spells, with the registries that map handler names from the spell data to code.
*/

#include "magic_internal.h"

#include "gameplay/handlers/spell_handlers.h"
#include "engine/core/obj_handler.h"
#include "magic_utils.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/inventory.h"

// Alter-obj handlers. Each runs on ctx.ovict (resolved + kNoalter-guarded by CastToAlterObjs), does
// its OWN messaging, and returns kSuccess when it acted (or chose to stay silent) / kFail to ask
// for the generic "no effect" line.
namespace handlers {
EStageResult AlterMsg(ActionContext &ctx, ESpellMsg key) {
	act(MUD::SpellMessages().GetMessage(ctx.spell_id(), key).c_str(), true, ctx.caster(), ctx.ovict, nullptr, kToChar);
	return EStageResult::kSuccess;
}

}  // namespace handlers

static const std::map<std::string, std::function<EStageResult(ActionContext &)>> kAlterObjHandlers = {
	{"AlterBless", handlers::AlterBless},
	{"AlterCurse", handlers::AlterCurse},
	{"AlterInvisible", handlers::AlterInvisible},
	{"AlterPoison", handlers::AlterPoison},
	{"AlterRemoveCurse", handlers::AlterRemoveCurse},
	{"AlterEnchantWeapon", handlers::AlterEnchantWeapon},
	{"AlterRemovePoison", handlers::AlterRemovePoison},
	{"AlterFly", handlers::AlterFly},
	{"AlterAcid", handlers::AlterAcid},
	{"AlterRepair", handlers::AlterRepair},
	{"AlterTimerRestore", handlers::AlterTimerRestore},
	{"AlterRestoration", handlers::AlterRestoration},
	{"AlterLight", handlers::AlterLight},
	{"AlterDarkness", handlers::AlterDarkness},
	{"AlterWeaveRestore", handlers::AlterWeaveRestore},
};

// Data-driven object transform: resolve the target object (an explicit ovict, else a collateral
// item of the victim), guard kNoalter, then dispatch the per-spell handler (kAlterObjHandlers);
// kFail from the handler => the generic "no effect" line.
EStageResult CastToAlterObjs(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	CharData *const victim = ctx.cvict;
	ObjData *obj = ctx.ovict;
	const ESpell spell_id = ctx.spell_id();
	// A prior action / component consumption may have destroyed the target (deferred-extracted,
	// flagged purged()); treat a purged target as absent.
	if (obj && obj->purged()) {
		obj = nullptr;
		ctx.ovict = nullptr;
	}
	// Collateral random item: opt-in per spell via <alter_obj collateral="on_damage">, and only when
	// damage was actually dealt this cast (acid corroding gear; kAcid/kAcidArrow cannot target an
	// object directly). Other alter_obj spells never alter the items of a character they hit.
	if (obj == nullptr && victim != nullptr
			&& ctx.action_or_default().GetAlterObj().collateral_on_damage
			&& ctx.result.damage != 0) {
		int rand = number(1, 50);
		if (rand <= EEquipPos::kBoths) {
			obj = GET_EQ(victim, rand);
		} else {
			for (rand -= EEquipPos::kBoths, obj = victim->carrying; rand && obj;
				 rand--, obj = obj->get_next_content());
		}
	}
	// <alter_obj collateral="first_suppressed"> on a char cast with no explicit object: pick a random
	// worn item that carries a suppression.
	if (obj == nullptr && victim != nullptr && ctx.action_or_default().GetAlterObj().find_suppressed) {
		std::vector<ObjData *> suppressed = SuppressedWornItems(victim);
		if (!suppressed.empty()) {
			obj = suppressed[number(0, static_cast<int>(suppressed.size()) - 1)];
		}
	}
	if (obj == nullptr) {
		return EStageResult::kSuccess;
	}
	if (obj->has_flag(EObjFlag::kNoalter) && !ctx.action_or_default().GetAlterObj().bypass_noalter) {
		act(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kObjResist).c_str(), true, ch, obj, nullptr, kToChar);
		return EStageResult::kSuccess;
	}
	ctx.ovict = obj;
	const auto &a = ctx.action_or_default().GetAlterObj();
	const auto it = kAlterObjHandlers.find(a.handler);
	if (it == kAlterObjHandlers.end()) {
		err_log("CastToAlterObjs: unknown alter_obj handler '%s' for %s.",
				a.handler.c_str(), NAME_BY_ITEM<ESpell>(spell_id).c_str());
		return EStageResult::kSuccess;
	}
	if (it->second(ctx) == EStageResult::kFail) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
	}
	return EStageResult::kSuccess;
}

// Post-load creation handlers that customize the created object, signature (ch, created obj, ctx).
// Plain food/light spells need no handler. CreateWeapon/CreateArmor are stubs: the base vnum is
// loaded by CastCreationAction; the stat/type customization goes in these bodies.
static const std::map<std::string, std::function<void(CharData *, ObjData *, const ActionContext &)>>
		kCreationHandlers = {
	{"CreateWeapon", handlers::CreateWeapon},
	{"CreateArmor",  handlers::CreateArmor},
};

// Data-driven object creation: load <obj_creation vnum>, run the optional post-load handler,
// narrate, then place in inventory (or drop to the room when over-encumbered).
EStageResult CastCreationAction(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr) {
		return EStageResult::kSuccess;
	}
	const auto &s = ctx.action_or_default().GetCreation();
	const auto tobj = world_objects.create_from_prototype_by_vnum(s.vnum);
	if (!tobj) {
		// Player-facing narration through the spell's sheaf, plus a SYSERR for builders.
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kItemNoPrototype) + "\r\n", ch);
		log("SYSERR: spell_creations, spell %d, obj %d: obj not found", to_underlying(spell_id), s.vnum);
		return EStageResult::kSuccess;
	}
	// Optional post-load customizer (kCreationHandlers): runs before narration/placement so it
	// can shape the freshly-loaded base object (weapon/armor). No handler => plain creation.
	if (!s.handler.empty()) {
		const auto it = kCreationHandlers.find(s.handler);
		if (it != kCreationHandlers.end()) {
			it->second(ch, tobj.get(), ctx);
		} else {
			err_log("CastCreationAction: unknown creation handler '%s' for %s.",
					s.handler.c_str(), NAME_BY_ITEM<ESpell>(spell_id).c_str());
		}
	}

	const auto &item_room_msg =
			MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kItemCreatedToRoom);
	act(item_room_msg.c_str(), false, ch, tobj.get(), nullptr, kToRoom | kToArenaListen);
	const auto &item_char_msg =
			MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kItemCreatedToChar);
	act(item_char_msg.c_str(), false, ch, tobj.get(), nullptr, kToChar);
	load_otrigger(tobj.get());

	if (ch->GetCarryingQuantity() >= CAN_CARRY_N(ch)) {
		SendMsgToChar("Вы не сможете унести столько предметов.\r\n", ch);
		PlaceObjToRoom(tobj.get(), ch->in_room);
		CheckObjDecay(tobj.get());
	} else if (ch->GetCarryingWeight() + tobj->get_weight() > CAN_CARRY_W(ch)) {
		SendMsgToChar("Вы не сможете унести такой вес.\r\n", ch);
		PlaceObjToRoom(tobj.get(), ch->in_room);
		CheckObjDecay(tobj.get());
	} else {
		PlaceObjToInventory(tobj.get(), ch);
	}

	return EStageResult::kSuccess;
}

// Hand-coded handlers in spells.cpp for kMagManual spells, selected by name (= function name) via
// <manual_cast><handler val="..."/>. std::function so future Lua/closure handlers can register too.
static const std::map<std::string, std::function<EStageResult(ActionContext &)>> kManualHandlers = {
	{"SpellControlWeather", handlers::SpellControlWeather},
	{"SpellCreateWater",    handlers::SpellCreateWater},
	{"SpellLocateObject",   handlers::SpellLocateObject},
	{"SpellCharm",          handlers::SpellCharm},
	{"SpellEnergydrain",    handlers::SpellEnergydrain},
	{"SpellFear",           handlers::SpellFear},
	{"SpellIdentify",       handlers::SpellIdentify},
	{"SpellFullIdentify",   handlers::SpellFullIdentify},
	{"SpellHolystrike",     handlers::SpellHolystrike},
	{"SpellRecall",         handlers::SpellRecall},
	{"SpellTeleport",       handlers::SpellTeleport},
	{"SpellSummon",         handlers::SpellSummon},
	{"SpellPortal",         handlers::SpellPortal},
	{"SpellRelocate",       handlers::SpellRelocate},
	{"SummonTutelar",       handlers::SummonTutelar},
	{"SpellMentalShadow",   handlers::SpellMentalShadow},
	{"SpellAnimateDead",    SpellAnimateDead},
	{"SpellAnimateSkeleton", SpellAnimateSkeleton},
	{"SpellResurrection",   SpellResurrection},
	{"HandleThunderstormTick", handlers::HandleThunderstormTick},
	// kWeaponHit obj-affect handler: a poisoned weapon poisons its strike victim.
	{"WeaponPoisonHit", handlers::WeaponPoisonHit},
	{"SkeletonHit", handlers::SkeletonHit},
};

bool IsManualHandlerRegistered(const std::string &name) {
	return kManualHandlers.count(name) > 0;
}

// Registered handler names (sorted), so the Vedun editor can offer a pick-list and reject unknown
// values for the <alter_obj/obj_creation/manual_cast handler="..."> attributes.
std::vector<std::string> AlterObjHandlerNames() {
	std::vector<std::string> v; for (const auto &p : kAlterObjHandlers) v.push_back(p.first); return v;
}

std::vector<std::string> ObjCreationHandlerNames() {
	std::vector<std::string> v; for (const auto &p : kCreationHandlers) v.push_back(p.first); return v;
}

std::vector<std::string> ManualHandlerNames() {
	std::vector<std::string> v; for (const auto &p : kManualHandlers) v.push_back(p.first); return v;
}

EStageResult CastManual(ActionContext &ctx) {
	const std::string &name = ctx.action_or_default().GetManualHandler();
	if (name.empty()) {
		return EStageResult::kSuccess;
	}
	const auto it = kManualHandlers.find(name);
	if (it == kManualHandlers.end()) {
		err_log("CastManual: unknown handler '%s' for spell %s", name.c_str(),
				NAME_BY_ITEM<ESpell>(ctx.spell_id()).c_str());
		return EStageResult::kSuccess;
	}
	return it->second(ctx);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
