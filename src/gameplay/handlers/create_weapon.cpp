/**
 \file create_weapon.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: CreateWeapon object-creation handler.
 \details Plumbing stub: the base vnum is loaded by CastCreationAction; the stat/type customization
          (TODO) goes here. The requested weapon type is parsed from the cast argument here now,
          instead of being special-cased in FindCastTarget (issue.spell-pipeline-cleaning #2).
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/magic/spells.h"        // cast_argument
#include "engine/ui/interpreter.h"        // search_block

extern const char *what_weapon[];         // defined in magic_utils.cpp (shared with dg scripts)

namespace handlers {

void CreateWeapon(CharData *ch, ObjData *obj, const ActionContext &ctx) {
	(void) ch; (void) obj; (void) ctx;
	// TODO (kCreateWeapon is unfinished): resolve the requested weapon type from the cast argument
	// and shape the loaded base object accordingly. If the caster named no/unknown type, that is
	// their problem -- the base object is still created.
	const int weapon_type = cast_argument[0] ? search_block(cast_argument, what_weapon, false) : -1;
	(void) weapon_type;
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
