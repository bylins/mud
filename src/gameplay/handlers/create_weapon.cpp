/**
 \file create_weapon.cpp - a part of the Bylins engine.
 \brief issue.spellhandlers: CreateWeapon object-creation handler (extracted from magic.cpp).
 \details Plumbing stub: the base vnum is loaded by the CastCreationAction skeleton; the
          stat/type customization (TODO) goes here.
*/

#include "gameplay/handlers/spell_handlers.h"

namespace handlers {

void CreateWeapon(CharData *ch, ObjData *obj, const CastContext &ctx) {
	(void) ch; (void) obj; (void) ctx;   // TODO: shape the loaded base object into a weapon
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
