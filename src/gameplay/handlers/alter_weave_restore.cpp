/**
 \file alter_weave_restore.cpp - a part of the Bylins engine.
 \brief issue.affect-suppression-dispell: AlterWeaveRestore alter-object handler for the
        !восстановить плетение! / !weave restoration! spell -- contest+lift one magic-suppression
        (kSuppressed obj-affect) from the target item; on success the equipment affect auto-returns.
*/

#include "gameplay/handlers/spell_handlers.h"
#include "gameplay/affects/obj_affects.h"
#include "gameplay/affects/affect_contants.h"
#include "engine/entities/obj_data.h"
#include "gameplay/abilities/talents_actions.h"
#include "gameplay/magic/magic.h"
#include "gameplay/magic/spell_messages.h"

namespace handlers {

EStageResult AlterWeaveRestore(ActionContext &ctx) {
	ObjData *obj = ctx.ovict;
	auto sup = obj_affects::FirstSuppression(obj);
	if (!sup) {
		return EStageResult::kFail;   // no suppression on this item -> generic "no effect"
	}
	const EAffect aff = static_cast<EAffect>(sup->modifier);
	const auto &a = ctx.action_or_default().GetAlterObj();
	// Same d100 contest as a normal dispel: caster competence vs the suppression's stored potency (the
	// suppressing dispel's competence) + this spell's dispel_bonus. A failed roll erodes the stored
	// potency in place (decay), so repeated attempts wear the suppression down.
	if (DispelContest(sup->potency, ctx.CompetenceBase(), a.dispel_bonus, a.decay, ctx.caster(), ctx.spell_id(), aff)) {
		obj->release_suppression(aff);   // erase the kSuppressed + re-materialize the affect if worn
		return AlterMsg(ctx, ESpellMsg::kAlterObjToChar);
	}
	return AlterMsg(ctx, ESpellMsg::kObjResist);   // the weave resisted (spell-specific message)
}

}  // namespace handlers

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
