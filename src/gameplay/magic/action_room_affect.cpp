/**
 \file action_room_affect.cpp - a part of the Bylins engine.
 \brief Room-targeted casting: imposing a spell's affect on a room or an exit (CastRoomAffect,
        CallMagicToRoom, CallMagicToExit).
*/

#include "magic_internal.h"

#include "engine/db/global_objects.h"
#include "gameplay/affects/room_affect_messages.h"
#include "gameplay/affects/room_affects.h"
#include "gameplay/magic/magic_utils.h"

namespace room_affects {

namespace {
// Look up `key` in `spell`'s sheaf (with kDefault fallback) and send to `ch`,
// substituting any {name} placeholder with `spell`'s canonical name.
void SendSpellNameMsg(CharData *ch, ESpell spell, ESpellMsg key) {
	std::string msg = MUD::SpellMessages().GetMessage(spell, key);
	const auto pos = msg.find("{name}");
	if (pos != std::string::npos) {
		msg.replace(pos, std::strlen("{name}"), MUD::Spell(spell).GetCName());
	}
	SendMsgToChar(msg + "\r\n", ch);
}
}  // namespace

// Impose the spell's room affect on ctx.rvict from the CURRENT action's <affects> block. Pure
// data-driven, no per-spell switch: duration, potency and charges come from the spell's <affects>;
// behaviour flags and seal strength (modifier) from the room affect's own room_affects.xml entry;
// narration from the room affect's messages, falling back to the spell's sheaf.
// Material-component and room-flag gates are NOT checked here -- CallMagicToRoom does that.
// Reads ctx.action_or_default(): action[0] for the CallMagicToRoom entry, or the cursor action
// when driven from the per-action loop -- so a kTarRoomThis action can scale the room affect off a
// prior action's result via ctx.CompetenceBase(). kSuccess on impose, kNotCast on no-effect.
// Duration unit: room-affect durations are in RAW ROOM-TICK PULSES, NOT the hours used by char
// affects: base + skill_bonus is used directly, without the kSecsPerMudHour multiplier that
// CalcDuration applies for PCs, so sub-hour values (kDeadlyFog 8 pulses, kMeteorStorm 3) fit in
// integers.
ECastResult CastRoomAffect(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	RoomData *const room = ctx.rvict;
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr || room == nullptr) {
		return ECastResult::kNotCast;
	}
	// The affect this spell imposes is exactly what its <affects id=...> declares.
	const ERoomAffect room_affect = ctx.action_or_default().Contains(talents_actions::EAction::kAffect)
		? ctx.action_or_default().GetAffect().GetRoomAffect() : ERoomAffect::kUndefined;
	// Default-init the affect array. kMaxSpellAffects slots; only slot 0 is
	// populated from the XML today, but the impose loop still walks them all
	// so multi-apply room spells stay forward-compatible.
	Affect<ERoomApply> af[kMaxSpellAffects];
	for (int i = 0; i < kMaxSpellAffects; i++) {
		af[i].affect_type = static_cast<ERoomAffect>(0);
		af[i].modifier = 0;
		af[i].battleflag.clear();
		af[i].location = kNone;
		af[i].caster_id = 0;
		af[i].apply_time = 0;
		af[i].duration = 0;
	}

	// Fills af[0] with type, caster, battleflag, duration, potency, charges and modifier. The
	// helpers CalcCastPotency / ComputeApplyModifier are shared with CastAffect so both record
	// the same values for the same cast roll.
	if (ctx.action_or_default().Contains(talents_actions::EAction::kAffect)) {
		const auto &talent = ctx.action_or_default().GetAffect();
		af[0].affect_type = room_affect;
		af[0].caster_id = ch->get_uid();
		// Flags come from room_affects.xml by affect_type (so the update-gate below reads the right
		// kAfUpdateDuration/kAfUpdateMod).
		af[0].battleflag = RoomAffectFlagsByType(af[0].affect_type);
		const ESkill dur_skill = MUD::Spell(spell_id).GetPotencyRoll().GetBaseSkill();
		int skill_bonus = (talent.GetDurationSkillDivisor() > 0 && dur_skill != ESkill::kUndefined)
			? CalcNoviceSkillBonus(ch, dur_skill, talent.GetDurationSkillDivisor()) : 0;
		if (talent.GetDurationMin() > 0) skill_bonus = std::max(skill_bonus, talent.GetDurationMin());
		if (talent.GetDurationMax() > 0) skill_bonus = std::min(skill_bonus, talent.GetDurationMax());
		af[0].duration = talent.GetDurationBase() + static_cast<unsigned>(skill_bonus);
		// Stored potency scaled by <affects potency_weight=> (default 1.0).
		// Symmetric with the char-affect path in TryApplyAffectTalent.
		af[0].potency = CalcCastPotency(ctx.potency()) * talent.GetPotencyWeight();
		af[0].charges = talent.GetChargesMax();
		// The seal strength (modifier) is the ROOM AFFECT's own property (room_affects.xml
		// <seal_strength>), not the casting spell's apply: the spell names the affect via a bare
		// <affect id=> ref carrying no coefficients. The kForbidden ward (number(1,100) <= modifier)
		// and the sight.cpp star rating read it.
		if (RoomAffectHasSeal(af[0].affect_type)) {
			af[0].modifier = ComputeApplyModifier(RoomAffectSeal(af[0].affect_type),
												  ctx.CompetenceBase(), ctx.potency());
		}
	}

	// Refresh / dedup policy. kMagNeedControl spells cancel the caster's previous controlled
	// affect. kAfUnique drops any prior copy from this caster. Otherwise an existing copy is
	// updated in place if af[0] carries kAfUpdateDuration and/or kAfUpdateMod, else the cast fails.
	bool success = true;
	bool handled_update = false;   // existing room affect updated in place -> skip impose loop
	if (MUD::Spell(spell_id).IsFlagged(kMagNeedControl)) {
		// Show the caster's interrupt notice FIRST (keyed on the OLD affect),
		// THEN remove it (which narrates the effect ceasing), THEN the new spell announces itself.
		const ERoomAffect controlled = FindControlledRoomAffect(ch);
		if (controlled != ERoomAffect::kUndefined) {
			const auto &imsg = RoomAffectMsg(controlled, ERoomAffectMsgType::kAffInterruptedToChar);
			if (!imsg.empty()) {
				act(imsg.c_str(), false, ch, nullptr, nullptr, kToChar);
			}
		}
		if (RemoveControlledRoomAffect(ch) != ERoomAffect::kUndefined) {
			SendSpellNameMsg(ch, spell_id, ESpellMsg::kCastPreparedToChar);
		}
	} else {
		auto RoomAffect_i = FindAffect(room, room_affect);
		if (RoomAffect_i != room->affected.end() && !IS_SET(af[0].battleflag, kAfUnique)) {
			// Re-cast over an existing (non per-caster-unique) room affect.
			// Per-field gating: kAfUpdateDuration -> duration, kAfUpdateMod -> modifier;
			// neither -> not updatable.
			const auto &existing = *RoomAffect_i;
			const bool can_dur = IS_SET(af[0].battleflag, kAfUpdateDuration);
			const bool can_mod = IS_SET(af[0].battleflag, kAfUpdateMod);
			if (!can_dur && !can_mod) {
				success = false;
			} else {
				// Non-dispellable room buffs (e.g. room light) update freely. A dispellable ward
				// (e.g. kForbidden) is updated for free by its author/live ally (buff OR weaken);
				// an outsider must win a strength contest (5% luck or higher potency), and a
				// player outsider vs a live author commits an aggressive PK act.
				bool may = true;
				if (IS_SET(af[0].battleflag, kAfDispellable)) {
					const auto access = ClassifyRoomAffectAccess(ch, existing->caster_id);
					may = access.free;
					if (!access.free) {
						if (!ch->IsNpc() && access.author && !pk_agro_action(ch, access.author)) {
							success = false;
						} else if (number(1, 100) <= 5 || af[0].potency > existing->potency) {
							may = true;
						} else {
							success = false;
						}
					}
				}
				if (success && may) {
					if (can_dur) {
						existing->duration =
							CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, af[0].duration);
					}
					if (can_mod) {
						existing->modifier = af[0].modifier;
					}
					existing->caster_id = ch->get_uid();   // authorship transfer
					AddRoomToAffected(room);
					handled_update = true;
				}
			}
		} else if (IS_SET(af[0].battleflag, kAfUnique)) {
			RemoveSingleRoomAffect(ch->get_uid(), room_affect);
		}
	}

	// Impose loop. Each affect's battleflag drives the join policy: kAfUpdateDuration ->
	// affect_room_join_fspell (merge into an existing copy, keeping the larger modifier and
	// duration); otherwise affect_room_join with kAfAccumulateDuration deciding whether durations
	// stack. Empty slots (no duration, no location) are skipped.
	for (int i = 0; !handled_update && success && i < kMaxSpellAffects; i++) {
		if (af[i].duration
			|| af[i].location != kNone) {
			af[i].duration = CalcComplexSpellMod(ch, spell_id, GAPPLY_SPELL_EFFECT, af[i].duration);
			if (IS_SET(af[i].battleflag, kAfUpdateDuration)) {
				affect_room_join_fspell(room, af[i]);
			} else {
				const bool accum_duration = IS_SET(af[i].battleflag, kAfAccumulateDuration);
				affect_room_join(room, af[i], accum_duration, false, false, false);
			}
			AddRoomToAffected(room);
		}
	}

	if (success) {
		// Imposition narration: the room affect's own messages, falling back to the spell's sheaf.
		// Both lookups skip the kDefault fallback, so a missing key stays silent.
		const ERoomAffect ra = room_affect;
		const auto &sheaf = MUD::SpellMessages()[spell_id];
		const std::string &room_to_room = RoomAffectMsgRaw(ra, ERoomAffectMsgType::kAffImposedToRoom);
		const std::string &to_room = !room_to_room.empty() ? room_to_room
				: sheaf.GetMessage(ESpellMsg::kAffImposedToRoom);
		if (!to_room.empty()) {
			act(to_room.c_str(), true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		const std::string &room_to_char = RoomAffectMsgRaw(ra, ERoomAffectMsgType::kAffImposedToChar);
		const std::string &to_char = !room_to_char.empty() ? room_to_char
				: sheaf.GetMessage(ESpellMsg::kAffImposedToChar);
		if (!to_char.empty()) {
			act(to_char.c_str(), true, ch, nullptr, nullptr, kToChar);
		}
		return ECastResult::kSuccess;
	}

	SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
	return ECastResult::kNotCast;
}

ECastResult CallMagicToRoom(CharData *ch, RoomData *room, ActionContext roll) {
	const ESpell spell_id = roll.spell_id();   // roll.level is unused for room casts
	roll.cvict = nullptr;
	roll.rvict = room;

	if (room == nullptr || ch == nullptr || ch->in_room == kNowhere) {
		return ECastResult::kNotCast;
	}

	// Material component: silently abort if the cast requires one and ch
	// can't provide it. No-op for spells without a configured component.
	if (ProcessMatComponents(ch, ch, spell_id) == EStageResult::kBreak) {
		return ECastResult::kNotCast;
	}

	// Room block, same mechanism as in CallMagic. The kDefault sheaf covers the generic
	// fizzle narration, per-spell sheaves override.
	if (IsRoomBlocked(world[ch->in_room], MUD::Spell(spell_id).actions.GetBlocking())
			&& !MayCastInForbiddenRoom(ch)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastForbiddenToChar) + "\r\n", ch);
		const auto &fizzle_room = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastForbiddenToRoom);
		if (!fizzle_room.empty()) {
			act(fizzle_room.c_str(), false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		return ECastResult::kNotCast;
	}

	// A kTarRoomThis spell carrying an <unaffect> block strips room affects here, mirroring
	// CastOnTarget's "Damage -> Unaffect -> Affect" ordering. CastUnaffects emits its own dispel /
	// no-effect narration. For a pure-dispel spell (no <affects>) the impose
	// loop below then runs as a no-op; for a dual spell (kMagAffects + <unaffect>) the affect
	// imposition continues normally.
	if (CastUnaffects(roll) == EStageResult::kBreak) {
		return ECastResult::kNotCast;
	}

	return CastRoomAffect(roll);
}

// Host a room affect on an EXIT instead of the room.
// Same Affect<ERoomApply> type, same impose -- only the host list differs. Behaviour flags come from
// room_affects.xml by affect_type, exactly like affect_to_room.
void affect_to_exit(const RoomData::exit_data_ptr &exit, const Affect<ERoomApply> &af) {
	Affect<ERoomApply>::shared_ptr new_affect(new Affect<ERoomApply>(af));
	if (RoomAffectFlagsLoaded() && af.affect_type != ERoomAffect::kUndefined) {
		new_affect->battleflag = RoomAffectFlagsByType(af.affect_type);
		if (af.battleflag.get(kAfFailed)) {
			new_affect->battleflag.set(kAfFailed);
		}
	}
	exit->affected.push_front(new_affect);
}

// Cast a kMagRoom spell onto the exit in direction `dir` from the caster's room (a kTarDirection cast).
// Mirrors CastRoomAffect's affect build but imposes on the exit/door. No dedup/PK contest yet:
// a repeat cast adds another copy. Expiry is ticked by UpdateExitsAffects.
ECastResult CallMagicToExit(CharData *ch, int dir, ActionContext roll) {
	const ESpell spell_id = roll.spell_id();
	roll.cvict = nullptr;
	if (ch == nullptr || ch->in_room == kNowhere || dir < 0 || dir >= EDirection::kMaxDirNum) {
		return ECastResult::kNotCast;
	}
	const auto exit = world[ch->in_room]->dir_option[dir];
	if (!exit || exit->to_room() == kNowhere) {
		SendMsgToChar("В этом направлении нет прохода.\r\n", ch);
		return ECastResult::kNotCast;
	}
	if (ProcessMatComponents(ch, ch, spell_id) == EStageResult::kBreak) {
		return ECastResult::kNotCast;
	}
	// A direction cast runs ONLY the passage-meaningful stages of the spell -- impose an affect on the
	// door and/or dispel affects already on it. Char-targeted stages (damage / points / summon / ...) are
	// intentionally skipped: they are pointless on a passage. This is the general mechanism -- the spell
	// just declares <targets ...kTarDirection> and carries <affects>/<unaffect>; no per-spell id check.
	const auto &action = roll.action_or_default();
	bool did = false;

	// Dispel stage: strip dispellable affects on the passage. This is how `развеять магию <dir>` works
	// (kDispellMagic's <unaffect> block).
	if (action.Contains(talents_actions::EAction::kUnaffect)) {
		if (DispelExitAffects(ch, dir, spell_id)) { did = true; }
	}

	// Affect-impose stage: build the affect from the spell's <affects> block (same fields CastRoomAffect
	// fills on af[0]) and host it on the exit.
	if (action.Contains(talents_actions::EAction::kAffect)) {
		const auto &talent = action.GetAffect();
		Affect<ERoomApply> af;
		af.affect_type = talent.GetRoomAffect();
		af.caster_id = ch->get_uid();
		af.location = kNone;
		af.battleflag = RoomAffectFlagsByType(af.affect_type);
		const ESkill dur_skill = MUD::Spell(spell_id).GetPotencyRoll().GetBaseSkill();
		int skill_bonus = (talent.GetDurationSkillDivisor() > 0 && dur_skill != ESkill::kUndefined)
			? CalcNoviceSkillBonus(ch, dur_skill, talent.GetDurationSkillDivisor()) : 0;
		if (talent.GetDurationMin() > 0) skill_bonus = std::max(skill_bonus, talent.GetDurationMin());
		if (talent.GetDurationMax() > 0) skill_bonus = std::min(skill_bonus, talent.GetDurationMax());
		af.duration = talent.GetDurationBase() + static_cast<unsigned>(skill_bonus);
		af.potency = CalcCastPotency(roll.potency()) * talent.GetPotencyWeight();
		af.charges = talent.GetChargesMax();
		if (RoomAffectHasSeal(af.affect_type)) {
			af.modifier = ComputeApplyModifier(RoomAffectSeal(af.affect_type), roll.CompetenceBase(), roll.potency());
		}
		affect_to_exit(exit, af);
		AddExitToAffected(world[ch->in_room], dir);   // register for the timer tick (UpdateExitsAffects)
		const auto &to_char = RoomAffectMsg(af.affect_type, ERoomAffectMsgType::kAffImposedToChar);
		if (!to_char.empty()) { act(to_char.c_str(), false, ch, nullptr, nullptr, kToChar); }
		const auto &to_room = RoomAffectMsg(af.affect_type, ERoomAffectMsgType::kAffImposedToRoom);
		if (!to_room.empty()) { act(to_room.c_str(), false, ch, nullptr, nullptr, kToRoom | kToArenaListen); }
		did = true;
	}

	if (!did) {
		// No passage-meaningful stage (a pure damage / heal / summon spell) -- nothing happens on a
		// doorway. The spell's own no-effect line says so.
		const auto &m = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect);
		SendMsgToChar((m.empty() ? "Это бессмысленно проделывать с проходом." : m) + "\r\n", ch);
		return ECastResult::kNotCast;
	}
	return ECastResult::kSuccess;
}

}  // namespace room_affects

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
