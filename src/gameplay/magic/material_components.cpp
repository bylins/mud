/**
 \file material_components.cpp - a part of the Bylins engine.
 \brief Material components: items an action requires and may consume, looked up in the actor's
        equipment, inventory and room.
*/

#include "magic_internal.h"

#include "engine/core/obj_handler.h"
#include "engine/core/target_resolver.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/inventory.h"

// A material component matches by val(1) (the ingredient's prototype-vnum field), not by the
// object's own vnum, so several ingredient prototypes can satisfy one requirement.
static inline bool IsMaterialFor(const ObjData *o, int vnum) {
	return o != nullptr
		&& o->get_type() == EObjType::kMagicIngredient
		&& o->get_val(1) == vnum;
}

static ObjData *FindMatInEquip(CharData *caster, int vnum) {
	for (int i = 0; i < EEquipPos::kNumEquipPos; ++i) {
		ObjData *o = GET_EQ(caster, i);
		if (IsMaterialFor(o, vnum)) {
			return o;
		}
	}
	return nullptr;
}

static ObjData *FindMatInInventory(CharData *caster, int vnum) {
	for (ObjData *o = caster->carrying; o; o = o->get_next_content()) {
		if (IsMaterialFor(o, vnum)) {
			return o;
		}
	}
	return nullptr;
}

static ObjData *FindMatInRoom(CharData *caster, int vnum) {
	if (caster->in_room == kNowhere) {
		return nullptr;
	}
	for (ObjData *o : world[caster->in_room]->contents) {
		if (IsMaterialFor(o, vnum)) {
			return o;
		}
	}
	return nullptr;
}

// Searches the locations enabled in `where`: equipment, then inventory, then room.
static ObjData *FindMatInLocations(CharData *caster, int vnum, Bitvector where) {
	if ((where & EFind::kObjEquip) != 0) {
		if (ObjData *o = FindMatInEquip(caster, vnum)) return o;
	}
	if ((where & EFind::kObjInventory) != 0) {
		if (ObjData *o = FindMatInInventory(caster, vnum)) return o;
	}
	if ((where & EFind::kObjRoom) != 0) {
		if (ObjData *o = FindMatInRoom(caster, vnum)) return o;
	}
	return nullptr;
}

// Spend a matched material item's Material::cost: emit `use`, take the charges, and emit `exhausted`
// when the item is destroyed. kCostNone does nothing at all (no narration, no charge spent).
// Unlinking dispatches on where the item actually lives (worn / carried /
// room) so destruction works in any of the three component locations.
static void ConsumeMatComponent(CharData *caster, ObjData *obj, int cost,
								const char *use, const char *exhausted) {
	if (cost == talents_actions::Material::kCostNone) {
		return;
	}
	if (use) {
		act(use, false, caster, obj, nullptr, kToChar);
	}
	bool destroy = false;
	if (cost == talents_actions::Material::kCostConsumeWhole) {
		destroy = true;
	} else {
		obj->set_val(2, GET_OBJ_VAL(obj, 2) - cost);
		if (GET_OBJ_VAL(obj, 2) < 1) {
			destroy = true;
		}
	}
	if (destroy) {
		if (exhausted) {
			act(exhausted, false, caster, obj, nullptr, kToChar);
		}
		// Unlink from wherever the item lives (worn / carried / room) before deferring its free.
		if (obj->get_worn_by()) {
			for (int i = 0; i < EEquipPos::kNumEquipPos; ++i) {
				if (GET_EQ(obj->get_worn_by(), i) == obj) {
					UnequipChar(obj->get_worn_by(), i, CharEquipFlags{});
					break;
				}
			}
		} else if (obj->get_carried_by()) {
			RemoveObjFromChar(obj);
		} else if (obj->get_in_room() != kNowhere) {
			RemoveObjFromRoom(obj);
		}
		// Deferred free (at heartbeat end): a target object that doubled as a consumed component is left
		// flagged purged(), not dangling.
		world_objects.AddToExtractedList(obj);
	}
}

// Checks and consumes the spell's <components>/<material> requirements; returns kBreak if any
// all_of/any_of can't be satisfied. A missing narration key in spell_msg.xml stays silent. The message
// strings outlive this call (SpellMessages isn't mutated mid-cast), so passing c_str() is safe.
EStageResult ProcessMatComponents(CharData *caster, CharData *victim, ESpell spell_id) {
	const auto &components = MUD::Spell(spell_id).GetComponents();
	if (components.empty()) {
		return EStageResult::kSuccess;
	}
	const auto &sheaf = MUD::SpellMessages()[spell_id];
	const std::string &use_str       = sheaf.GetMessage(ESpellMsg::kComponentUse);
	const std::string &missing_str   = sheaf.GetMessage(ESpellMsg::kComponentMissing);
	const std::string &exhausted_str = sheaf.GetMessage(ESpellMsg::kComponentExhausted);
	const char *use       = use_str.empty()       ? nullptr : use_str.c_str();
	const char *missing   = missing_str.empty()   ? nullptr : missing_str.c_str();
	const char *exhausted = exhausted_str.empty() ? nullptr : exhausted_str.c_str();

	for (const auto &mat : components.GetMaterials()) {
		// Only eq/inv/room are searchable; a <material> naming none of them can't be honoured.
		const Bitvector allowed =
				mat.where & (EFind::kObjEquip | EFind::kObjInventory | EFind::kObjRoom);
		if (allowed == 0) {
			log("SYSERR: spell %s: <material where> has no eq/inv/room flag "
				"(where=%lu); cast aborted.",
				NAME_BY_ITEM<ESpell>(spell_id).c_str(),
				static_cast<unsigned long>(mat.where));
			return EStageResult::kBreak;
		}

		// The parser already warned about a material with neither any_of nor all_of; skip it.
		if (mat.any_of.empty() && mat.all_of.empty()) {
			continue;
		}

		// Items to consume once every check has passed. Built up before any
		// dec_val so a partial match doesn't half-spend the requirement.
		std::vector<ObjData *> consume;

		for (int vnum : mat.all_of) {
			ObjData *o = FindMatInLocations(caster, vnum, allowed);
			if (!o) {
				if (missing) {
					act(missing, false, victim, nullptr, caster, kToChar);
				}
				return EStageResult::kBreak;
			}
			consume.push_back(o);
		}
		if (!mat.any_of.empty()) {
			ObjData *found = nullptr;
			for (int vnum : mat.any_of) {
				if ((found = FindMatInLocations(caster, vnum, allowed)) != nullptr) {
					break;
				}
			}
			if (!found) {
				if (missing) {
					act(missing, false, victim, nullptr, caster, kToChar);
				}
				return EStageResult::kBreak;
			}
			consume.push_back(found);
		}

		for (ObjData *o : consume) {
			ConsumeMatComponent(caster, o, mat.cost, use, exhausted);
		}
	}
	return EStageResult::kSuccess;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
