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
// substituting any {name} placeholder with `spell`'s canonical name. Trailing
// "\r\n" matches the SendMsgToChar convention.
void SendSpellNameMsg(CharData *ch, ESpell spell, ESpellMsg key) {
	std::string msg = MUD::SpellMessages().GetMessage(spell, key);
	const auto pos = msg.find("{name}");
	if (pos != std::string::npos) {
		msg.replace(pos, std::strlen("{name}"), MUD::Spell(spell).GetCName());
	}
	SendMsgToChar(msg + "\r\n", ch);
}
}  // namespace

// Apply a room-targeted spell to ch's current room. Pure data-driven: every
// affect parameter -- duration, modifier, potency, debuff flag, refresh /
// dedup policy, imposition narration -- comes from the spell's XML
// <talent_actions>/<affects> block and spell_msg.xml. No per-spell switch or
// code-set messages remain. New room spells need only their XML rows; this
// function is unchanged.
// Three universal gates run before any affect data is computed:
//   1. Material component (ProcessMatComponents). Missing -> abort silently.
//   2. <blocking><room_flags val>. Room carries a blocking flag and the caster
//      is not exempt (MayCastInForbiddenRoom) -> emit kCastForbidden* and
//      abort. This is the kRuneLabel/kForbidden/kNoMagic fizzle path.
//   3. (no third gate -- everything else is data-driven inside the impose loop)
// Duration unit: room-affect durations are in RAW ROOM-TICK PULSES, NOT the
// hours used by char affects. The reader uses base + skill_bonus directly,
// without the kSecsPerMudHour multiplier that CalcDuration applies for PCs.
// This preserves the OLD pulse-direct semantics of kDeadlyFog (8 pulses),
// kMeteorStorm (3 pulses), etc. -- sub-hour values that hours-based duration
// can't express in integers.
// Impose the spell's room affect on ctx.rvict from the CURRENT action's <affects> block.
// Reads ctx.action_or_default(): action[0] for the legacy CallMagicToRoom entry, or the cursor
// action when driven from the per-action loop -- so a kTarRoomThis action can scale the room
// affect off a prior action's result via ctx.CompetenceBase(). kSuccess on impose, kNotCast on no-effect.
ECastResult CastRoomAffect(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	RoomData *const room = ctx.rvict;
	const ESpell spell_id = ctx.spell_id();
	if (ch == nullptr || room == nullptr) {
		return ECastResult::kNotCast;
	}
	// issue.affects-improve: the affect this spell imposes is exactly what its <affects id=...> declares.
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

	// Data-driven defaults from the spell's <talent_actions>/<affects> block.
	// Fills af[0] with duration, battleflag, modifier, potency, debuff -- the
	// same fields CastAffect populates on a per-target affect (the helpers
	// CalcCastPotency / ComputeApplyModifier are shared with CastAffect's
	// apply_one path so both record the same values for the same cast roll).
	if (ctx.action_or_default().Contains(talents_actions::EAction::kAffect)) {
		const auto &talent = ctx.action_or_default().GetAffect();
		af[0].affect_type = room_affect;
		af[0].caster_id = ch->get_uid();
		// issue.affect-migration: flags come from room_affects.xml by affect_type (so the update-gate
		// below reads the right kAfUpdateDuration/kAfUpdateMod).
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
		// issue.affects-improve (P1 fix): the seal strength (modifier) is the ROOM AFFECT's own
		// property (room_affects.xml <seal_strength>), not the casting spell's apply. The spell now
		// names the affect via a bare <affect id=> ref carrying no coefficients, so reading
		// talent.GetApplies()[0] here yielded modifier 0 -- which silently disabled the kForbidden
		// ward (gated on number(1,100) <= af->modifier) and zeroed the sight.cpp star rating.
		if (RoomAffectHasSeal(af[0].affect_type)) {
			af[0].modifier = ComputeApplyModifier(RoomAffectSeal(af[0].affect_type),
												  ctx.CompetenceBase(), ctx.potency());
		}
	}

	/*
	// The hack for displaying the sealing level has been removed. The code is
	// left in case we need to quickly revert it.
	if (spell_id == ESpell::kForbidden) {
		if (af[0].modifier > 99) {
			// top tier -> spell_msg.xml kForbidden/kAffImposed*
		} else if (af[0].modifier > 79) {
			to_char = "Вы почти полностью запечатали магией все входы.";
			to_room = "$n почти полностью запечатал$g магией все входы.";
		} else {
			to_char = "Вы очень плохо запечатали магией все входы.";
			to_room = "$n очень плохо запечатал$g магией все входы.";
		}
	}
	*/

	// Refresh / dedup policy. kMagNeedControl spells cancel any other
	// controlled affect; otherwise a duplicate cast is allowed iff the
	// affect's first slot carries kAfUpdateDuration. kAfUnique drops any
	// prior copy from this caster.
	bool success = true;
	bool handled_update = false;   // existing room affect updated in place -> skip impose loop
	if (MUD::Spell(spell_id).IsFlagged(kMagNeedControl)) {
		// issue.affects-improve: show the caster's interrupt notice FIRST (keyed on the OLD affect),
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
			// issue.dispellbug: re-cast over an existing (non per-caster-unique) room affect.
			// Per-field gating: kAfUpdateDuration -> duration, kAfUpdateMod -> modifier;
			// neither -> not updatable. Author/live-ally update for free (buff OR weaken);
			// an outsider must win a strength contest, and a player outsider vs a live
			// author commits an aggressive PK act.
			const auto &existing = *RoomAffect_i;
			const bool can_dur = IS_SET(af[0].battleflag, kAfUpdateDuration);
			const bool can_mod = IS_SET(af[0].battleflag, kAfUpdateMod);
			if (!can_dur && !can_mod) {
				success = false;
			} else {
				// Only dispellable wards (e.g. kForbidden) require an author/ally + strength
				// contest to update; non-dispellable room buffs (e.g. room light) update
				// freely for now (issue.dispellbug).
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

	// Impose loop. Each affect's battleflag drives the join policy: kAfUpdate
	// Duration -> affect_room_join_fspell (replace by spell id); otherwise
	// affect_room_join with kAfAccumulateDuration deciding whether durations
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
		// Imposition narration: pure spell_msg.xml lookup, sheaf-direct so a
		// missing key stays silent (same convention as CastAffect's
		// EmitImpositionEffects).
		// Prefer the room-affect message system (keyed by the affect's ERoomAffect identity);
		// fall back to the spell sheaf for anything not yet migrated.
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

	// Data-driven room block: same mechanism as in
	// CallMagic. Fizzle narration lives in spell_msg.xml; the kDefault
	// sheaf covers the generic case, per-spell sheaves override.
	if (IsRoomBlocked(world[ch->in_room], MUD::Spell(spell_id).actions.GetBlocking())
			&& !MayCastInForbiddenRoom(ch)) {
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastForbiddenToChar) + "\r\n", ch);
		const auto &fizzle_room = MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kCastForbiddenToRoom);
		if (!fizzle_room.empty()) {
			act(fizzle_room.c_str(), false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		return ECastResult::kNotCast;
	}

	// Data-driven dispel. A kTarRoomThis spell carrying an <unaffect>
	// block strips room affects here, mirroring CastToSingleTarget's "Damage -> Unaffect ->
	// Affect" ordering. CastUnaffects emits its own dispel / no-effect narration via the
	// kAffDispelledTo{Char,Room} sheaves. For a pure-dispel spell (no <affects>) the impose
	// loop below then runs as a no-op; for a dual spell (kMagAffects + <unaffect>) the affect
	// imposition continues normally.
	if (CastUnaffects(roll) == EStageResult::kBreak) {
		return ECastResult::kNotCast;
	}

	return CastRoomAffect(roll);
}

// issue.room-affect-trigger-improve (door affects): host a room affect on an EXIT instead of the room.
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
// Mirrors CallMagicToRoom's affect build but imposes on the exit/door. NOTE (first cut): no dedup/PK
// contest and no expiry tick yet -- a door affect persists until dispelled; both are follow-ups.
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
	// (kDispellMagic's <unaffect> block) -- it is no longer a kDispellMagic special-case in CallMagic.
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
