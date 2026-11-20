/**
 \file affect_triggers.cpp - a part of the Bylins engine.
 \brief Affect-owned actions: per-tick actions of character and room affects, character death and event
        triggers, object-affect weapon hits and triggers, room entry and door triggers, and spending of
        trigger charges.
*/

#include "magic_internal.h"

#include "gameplay/affects/obj_affect_messages.h"
#include "administration/privilege.h"
#include "gameplay/affects/room_affect_messages.h"
#include "gameplay/affects/affect_messages.h"
#include "engine/db/global_objects.h"

// Spend one trigger charge on a character affect after its trigger fired. Decrements every
// instance of the affect type (one per apply, kept in sync); when any hits 0 the whole affect is
// removed. Unlimited charges are never spent. Caller must NOT be mid-iteration over `bearer->affected`.
void SpendCharAffectCharge(CharData *bearer, EAffect affect_type) {
	if (!bearer) {
		return;
	}
	bool depleted = false;
	for (const auto &aff : bearer->affected) {
		if (aff && aff->affect_type == affect_type && aff->charges != kUnlimitedAffectCharges && --aff->charges <= 0) {
			depleted = true;
		}
	}
	if (depleted) {
		// Charges exhausted counts as expiry: fire kExpired before removal, as the timer path does.
		RunCharAffectTrigger(bearer, affect_type, talents_actions::EActionTrigger::kExpired);
		if (bearer->purged()) {
			return;
		}
		RemoveAffectFromChar(bearer, affect_type);
	}
}

// Recursion guard shared by every trigger-launched action chain (kKill / kExpired / kDispell / kPreHit /
// kPostHit / room entry+tick) -- they all run through RunRoomCycledAction. A triggered action that deals
// damage can kill -> ProcessDeath -> another trigger -> back here. Hitting the cap almost always means a
// content loop, so the chain is skipped and logged.
namespace {
constexpr int kMaxTriggerActionDepth = 5;
int g_trigger_action_depth = 0;
struct TriggerDepthGuard {
	bool ok;
	TriggerDepthGuard() : ok(g_trigger_action_depth < kMaxTriggerActionDepth) {
		if (ok) { ++g_trigger_action_depth; }
	}
	~TriggerDepthGuard() { if (ok) { --g_trigger_action_depth; } }
	TriggerDepthGuard(const TriggerDepthGuard &) = delete;
	TriggerDepthGuard &operator=(const TriggerDepthGuard &) = delete;
};
}  // namespace

// Runs one cycled action (action[phase % N]) of `list` on the room; the action's own target marker decides
// whom it hits. Shared by kService tick spells and affect-owned actions.
static ECastResult RunRoomCycledAction(ActionContext &ctx, RoomData *room,
									   const std::vector<talents_actions::Action> &list, int phase) {
	TriggerDepthGuard depth_guard;
	if (!depth_guard.ok) {
		log("SYSERR: trigger-action recursion depth cap (%d) hit -- skipping a triggered action chain.",
			kMaxTriggerActionDepth);
		return ECastResult::kNotCast;
	}
	ctx.rvict = room;
	if (list.empty()) {
		return ECastResult::kNotCast;
	}
	ctx.RewindActions();
	const size_t idx = static_cast<size_t>(phase < 0 ? 0 : phase) % list.size();
	for (size_t i = 0; i < idx; ++i) {
		ctx.NextAction();
	}
	// A manual_cast action runs its hand-coded handler (the kManualHandlers registry), which reads its
	// context (e.g. the tick duration) off ctx.
	if (ctx.action_or_default().Contains(talents_actions::EAction::kManual)) {
		CastManual(ctx);
		return ECastResult::kSuccess;
	}
	const talents_actions::EActionTarget sel = ctx.action_or_default().GetTarget();
	if (sel == talents_actions::EActionTarget::kTarRoomThis) {
		return RunActionOnRoom(ctx);
	}
	const ECastTargets scope = ActionTargetScope(sel, ECastTargets::kFoes);
	std::vector<CharData *> targets = ResolveActionTargets(ctx, sel, {});
	return RunActionOverTargets(ctx, targets, scope, false);
}

// Runs a bearer's ADDITIVE affect talent-patches (op=append/insert) for one affect type as extra actions
// alongside the affect's own. `accept` picks the payload actions matching the runner's trigger. Gated on
// CanUseFeat(ResolvePatchRelative(bearer, relative), feat). Returns true if the bearer was purged.
template <typename TriggerPred>
static bool RunAdditiveAffectPatches(ActionContext &ctx, CharData *ch, RoomData *room,
									 EAffect type, TriggerPred accept) {
	std::vector<talents_actions::Action> extra;
	for (const auto &ref : feats::AffectTalentPatches(type)) {
		if (ref.patch->op != feats::EPatchOp::kAppend && ref.patch->op != feats::EPatchOp::kInsert) {
			continue;
		}
		CharData *holder = feats::ResolvePatchRelative(ch, ref.patch->relative);
		if (!holder || !CanUseFeat(holder, ref.feat)) {
			continue;
		}
		for (const auto &pa : ref.patch->payload.list()) {
			if (accept(pa)) {
				extra.push_back(pa);
			}
		}
	}
	if (extra.empty()) {
		return false;
	}
	ctx.UseExternalActions(&extra);
	for (size_t i = 0; i < extra.size(); ++i) {
		RunRoomCycledAction(ctx, room, extra, i);
		if (ch->purged()) {
			return true;
		}
	}
	return false;
}

// Applies an affect's MUTATING talent-patches (modify / remove / replace / add_effect / replace_all) in
// place to `list`, the per-firing COPY of its matching actions (never the shared config). `accept` filters
// the addressed actions and the installed payload to this trigger. Same feat gate as the additive path.
template <typename TriggerPred>
static void ApplyMutatingAffectPatches(std::vector<talents_actions::Action> &list, CharData *ch,
									   EAffect type, TriggerPred accept) {
	using talents_actions::Action;
	for (const auto &ref : feats::AffectTalentPatches(type)) {
		const auto op = ref.patch->op;
		if (op == feats::EPatchOp::kAppend || op == feats::EPatchOp::kInsert) {
			continue;
		}
		CharData *holder = feats::ResolvePatchRelative(ch, ref.patch->relative);
		if (!holder || !CanUseFeat(holder, ref.feat)) {
			continue;
		}
		const feats::TalentPatch &p = *ref.patch;
		auto payload_for = [&]() {
			std::vector<Action> v;
			for (const auto &pa : p.payload.list()) {
				if (accept(pa)) {
					v.push_back(pa);
				}
			}
			return v;
		};
		auto find_id = [&](const std::string &id) -> long {
			for (size_t k = 0; k < list.size(); ++k) {
				if (list[k].GetId() == id) {
					return static_cast<long>(k);
				}
			}
			return -1;
		};
		switch (op) {
			case feats::EPatchOp::kReplaceAll: {
				list = payload_for();
				break;
			}
			case feats::EPatchOp::kModify: {
				long i = -1;
				if (!p.action_id.empty()) {
					i = find_id(p.action_id);
				} else {
					for (size_t k = 0; k < list.size(); ++k) {
						if (list[k].Contains(p.mod_kind)) { i = static_cast<long>(k); break; }
					}
				}
				if (i < 0) {
					break;
				}
				talents_actions::IAction *man = list[i].CloneManifestation(p.mod_kind);
				if (man) {
					man->ApplyFieldMod(p.mod_field, ToFieldModOp(p.mod_op), p.mod_value);
				}
				break;
			}
			case feats::EPatchOp::kRemove: {
				if (p.action_id.empty()) {
					break;
				}
				long i = find_id(p.action_id);
				if (i < 0) {
					break;
				}
				if (p.effect_id.empty()) {
					list.erase(list.begin() + i);
				} else {
					list[i].EraseManifestationsById(p.effect_id);
				}
				break;
			}
			case feats::EPatchOp::kReplace: {
				if (p.action_id.empty()) {
					break;
				}
				long i = find_id(p.action_id);
				if (i < 0) {
					break;
				}
				if (p.effect_id.empty()) {
					std::vector<Action> pm = payload_for();
					list.erase(list.begin() + i);
					list.insert(list.begin() + i, pm.begin(), pm.end());
				} else {
					list[i].EraseManifestationsById(p.effect_id);
					for (auto &b : payload_for()) {
						list[i].MergeManifestationsFrom(b);
					}
				}
				break;
			}
			case feats::EPatchOp::kAddEffect: {
				if (p.action_id.empty()) {
					break;
				}
				long i = find_id(p.action_id);
				if (i < 0) {
					break;
				}
				for (auto &b : payload_for()) {
					list[i].MergeManifestationsFrom(b);
				}
				break;
			}
			default:
				break;
		}
	}
}

// Runs an affect's own actions on the room each tick. Context (level/potency) comes from the imposing
// spell (ctx_spell); the action list is injected via UseExternalActions. `tick_duration` (if non-null)
// carries the affect's duration in and out, so a manual_cast handler can read or change it.
ECastResult CastRoomTickActionFromActions(CharData *ch, RoomData *room, ESpell ctx_spell,
										  const std::vector<talents_actions::Action> &actions, int phase,
										  int *tick_duration, float fixed_potency,
										  const std::string &aff_dmg_char, const std::string &aff_dmg_vict,
										  const std::string &aff_dmg_room, long damage_author) {
	if (ch == nullptr) {
		return ECastResult::kNotCast;
	}
	ActionContext ctx = BuildActionContext(ch, ctx_spell, GetRealLevel(ch), fixed_potency);
	ctx.UseExternalActions(&actions);
	// Credited author of the tick's damage (e.g. the poisoner).
	ctx.SetDamageAuthorUid(damage_author);
	// Affect-owned damage flavor for <damage> actions (empty -> the generic combat line).
	ctx.SetAffectDamageMsg(aff_dmg_char, aff_dmg_vict, aff_dmg_room);
	if (tick_duration) {
		ctx.SetTickDuration(*tick_duration);
	}
	const ECastResult result = RunRoomCycledAction(ctx, room, actions, phase);
	if (tick_duration) {
		*tick_duration = ctx.GetTickDuration();
	}
	return result;
}

// Runs one character affect's pulse/battle-pulse <actions> on its bearer: kPulse every tick,
// kBattlePulse only while fighting. The bearer is the caster. The affect's duration is threaded through
// so a manual_cast action can end it early. Returns true if any action fired.
bool RunCharAffectTick(CharData *ch, Affect<EApply>::shared_ptr aff) {
	if (!ch || !aff || ch->in_room == kNowhere) {
		return false;
	}
	const bool combat = (ch->GetEnemy() != nullptr);
	std::vector<talents_actions::Action> pulse;
	for (const auto &action : affects::AffectActions(aff->affect_type).list()) {
		const auto &t = action.GetTrigger();
		if (t.test(talents_actions::EActionTrigger::kPulse)
				|| (combat && t.test(talents_actions::EActionTrigger::kBattlePulse))) {
			pulse.push_back(action);
		}
	}
	// Let perks mutate the per-tick actions (on the copy; the affect config stays intact).
	if (!feats::AffectTalentPatches(aff->affect_type).empty()) {
		ApplyMutatingAffectPatches(pulse, ch, aff->affect_type,
				[combat](const talents_actions::Action &a) {
					const auto &tt = a.GetTrigger();
					return tt.test(talents_actions::EActionTrigger::kPulse)
						|| (combat && tt.test(talents_actions::EActionTrigger::kBattlePulse));
				});
	}
	if (pulse.empty()) {
		return false;
	}
	const int phase = aff->apply_time > 0 ? aff->apply_time - 1 : 0;
	int dur = aff->duration;
	// Автора тика ищем по всем узлам этого типа, см. SelectAffectAuthorUid.
	const long author_uid = SelectAffectAuthorUid(ch->affected, aff);
	// Spell-free tick on the affect's stored potency. The bearer is the caster, so a <damage> action must
	// target kTarFightSelf explicitly (a lone action's kTarSame resolves to an empty list). The affect's
	// kDamageTo* flavor is passed, otherwise a DoT deals damage silently.
	CastRoomTickActionFromActions(ch, world[ch->in_room], ESpell::kUndefined, pulse, phase, &dur, aff->potency,
			affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToChar),
			affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToVict),
			affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToRoom),
			author_uid);
	aff->duration = dur;
	// The affect's additive talent-patches matching the pulse triggers (fresh ctx, tick's potency/author).
	if (!ch->purged() && !feats::AffectTalentPatches(aff->affect_type).empty()) {
		ActionContext pctx = BuildActionContext(ch, ESpell::kUndefined, GetRealLevel(ch), aff->potency);
		pctx.SetDamageAuthorUid(author_uid);
		RunAdditiveAffectPatches(pctx, ch, world[ch->in_room], aff->affect_type,
				[combat](const talents_actions::Action &a) {
					const auto &tt = a.GetTrigger();
					return tt.test(talents_actions::EActionTrigger::kPulse)
						|| (combat && tt.test(talents_actions::EActionTrigger::kBattlePulse));
				});
	}
	// A DoT with <charges max=N> lasts exactly N ticks, in or out of combat, independent of the duration.
	// When depleted, mark it expired (duration 0) for the caller's normal removal instead of erasing from
	// ch->affected mid-iteration. Unlimited charges (the default): duration-bound only.
	if (!ch->purged() && aff->charges != kUnlimitedAffectCharges && --aff->charges <= 0) {
		aff->duration = 0;
	}
	return true;
}

// Runs ONE affect type's <actions> matching `trig`, as caster `ch`, with event.actor = `actor` (a
// per-affect lifecycle event; RunCharEventTriggers handles per-hit events across all affects). The caller
// picks `ch`: for kExpired the bearer (actor = null); for kDispell the DISPELLER, so a kTarFightSelf
// retaliation is self-damage, not PvP against the bearer. Recursion is bounded in RunRoomCycledAction.
bool RunCharAffectTrigger(CharData *ch, EAffect affect_type, talents_actions::EActionTrigger trig,
						  CharData *actor, int event_amount, int event_category) {
	if (!ch || ch->in_room == kNowhere) {
		return false;
	}
	std::vector<talents_actions::Action> fired;
	for (const auto &a : affects::AffectActions(affect_type).list()) {
		if (!a.GetTrigger().test(trig)) {
			continue;
		}
		// kPoints category filter: an action pinned to a <trigger category=> fires only for that category.
		// Uncategorized actions and non-kPoints triggers (event_category == -1) always pass.
		if (event_category >= 0 && a.GetTriggerPointsCategory() >= 0
				&& a.GetTriggerPointsCategory() != event_category) {
			continue;
		}
		fired.push_back(a);
	}
	// Let perks mutate the lifecycle actions (on the copy).
	if (!feats::AffectTalentPatches(affect_type).empty()) {
		ApplyMutatingAffectPatches(fired, ch, affect_type,
				[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); });
	}
	if (fired.empty()) {
		return false;
	}
	const int level = ch->IsNpc() ? GetRealLevel(ch) : 1;
	ActionContext ctx = BuildActionContext(ch, ESpell::kUndefined, level);
	ctx.UseExternalActions(&fired);
	// kTarActor resolves off ctx.cvict, so point it at the actor (the dispeller for kDispell, the healer
	// for kPoints); otherwise the retaliation would hit the bearer. Actor-less triggers leave it null.
	ctx.cvict = actor;
	EventContext ev;
	ev.trigger = trig;
	ev.actor = actor;
	ev.amount = event_amount;
	ev.points_category = event_category;
	ctx.SetEvent(ev);
	// The affect's own damage flavor (kDamageTo*) for any <damage> in the chain; empty -> generic line.
	ctx.SetAffectDamageMsg(affects::AffectMsgRaw(affect_type, affects::EAffectMsgType::kDamageToChar),
						   affects::AffectMsgRaw(affect_type, affects::EAffectMsgType::kDamageToVict),
						   affects::AffectMsgRaw(affect_type, affects::EAffectMsgType::kDamageToRoom));
	RunRoomCycledAction(ctx, world[ch->in_room], fired, 0);
	// The affect's additive talent-patches for this trigger.
	RunAdditiveAffectPatches(ctx, ch, world[ch->in_room], affect_type,
			[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); });
	return true;
}

// Fires the DYING char's kDeath actions (from die(), before raw_kill) on the char itself, with
// event.actor = the killer. Returns true if any resolved a block (<trigger return="0"/> or a handler
// override): die() then prevents the death. The preventing affect must heal the char itself -- an
// unhealed block just delays death to the next tick (by design).
bool RunCharDeathTriggers(CharData *ch, CharData *killer) {
	if (!ch || ch->in_room == kNowhere || ch->affected.empty()) {
		return false;
	}
	const int level = ch->IsNpc() ? GetRealLevel(ch) : 1;
	bool prevented = false;
	std::vector<EAffect> seen;
	std::vector<EAffect> spend_after;   // types that actually PREVENTED death -> spend a charge (post-loop)
	for (const auto &aff : ch->affected) {
		if (!aff || std::find(seen.begin(), seen.end(), aff->affect_type) != seen.end()) {
			continue;
		}
		std::vector<talents_actions::Action> fired;
		for (const auto &a : affects::AffectActions(aff->affect_type).list()) {
			if (a.GetTrigger().test(talents_actions::EActionTrigger::kDeath)) {
				fired.push_back(a);
			}
		}
		if (fired.empty()) {
			continue;
		}
		seen.push_back(aff->affect_type);
		ActionContext ctx = BuildActionContext(ch, ESpell::kUndefined, level);
		ctx.UseExternalActions(&fired);
		ctx.cvict = ch;   // self: a <points><heal> / kTarFightSelf lands on the dying char
		EventContext ev;
		ev.trigger = talents_actions::EActionTrigger::kDeath;
		ev.actor = killer;
		ctx.SetEvent(ev);
		ctx.SetAffectDamageMsg(affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToChar),
							   affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToVict),
							   affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDamageToRoom));
		RunRoomCycledAction(ctx, world[ch->in_room], fired, 0);
		if (ch->purged()) {
			return prevented;   // a kDeath action purged the char -- can't prevent what's already gone
		}
		// Optional kDeath flavor, shown after the action (so it follows the heal). $n = the bearer.
		const std::string &to_char = affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDeathToChar);
		if (!to_char.empty()) {
			act(to_char.c_str(), false, ch, nullptr, nullptr, kToChar);
		}
		const std::string &to_room = affects::AffectMsgRaw(aff->affect_type, affects::EAffectMsgType::kDeathToRoom);
		if (!to_room.empty()) {
			act(to_room.c_str(), true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		}
		// Resolve the "prevent death" verdict: a manual_cast handler's ctx override wins, else the
		// action's static <trigger return="0"/> tag. 0 == block (mirrors kEntryTriggerBlock).
		std::optional<int> ret = ctx.GetTriggerReturn();
		if (!ret) {
			for (const auto &a : fired) {
				if (a.GetTriggerReturn()) { ret = a.GetTriggerReturn(); break; }
			}
		}
		if (ret && *ret == 0) {
			prevented = true;
				// A charge is spent only when death is actually prevented. Deferred until after the loop so removing
				// a depleted affect can't invalidate the iteration over ch->affected.
			spend_after.push_back(aff->affect_type);
		}
	}
	for (const EAffect t : spend_after) {
		SpendCharAffectCharge(ch, t);
	}
	return prevented;
}

// Runs every action on the bearer's affects whose trigger matches event.trigger (e.g. kPreHit/kPostHit),
// with `event` on the context for manual_cast handlers. NPCs proc at their real level, PCs at level 1.
// Snapshots the affect TYPES first because a fired side-spell may mutate ch->affected.
bool RunCharEventTriggers(CharData *ch, const EventContext &event) {
	if (!ch || ch->in_room == kNowhere || ch->affected.empty() || !event.valid()) {
		return false;
	}
	const talents_actions::EActionTrigger trig = event.trigger;
	std::vector<EAffect> types;   // unique affect types carrying a matching action (few affects -> linear dedup)
	for (const auto &aff : ch->affected) {
		if (!aff || std::find(types.begin(), types.end(), aff->affect_type) != types.end()) {
			continue;
		}
		for (const auto &a : affects::AffectActions(aff->affect_type).list()) {
			if (a.GetTrigger().test(trig)) {
				types.push_back(aff->affect_type);
				break;
			}
		}
	}
	if (types.empty()) {
		return false;
	}
	const int level = ch->IsNpc() ? GetRealLevel(ch) : 1;
	for (const EAffect t : types) {
		std::vector<talents_actions::Action> fired;
		for (const auto &a : affects::AffectActions(t).list()) {
			if (a.GetTrigger().test(trig)) {
				fired.push_back(a);
			}
		}
		// Let perks mutate the per-hit actions (on the copy).
		if (!feats::AffectTalentPatches(t).empty()) {
			ApplyMutatingAffectPatches(fired, ch, t,
					[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); });
		}
		// Competence-based event actions scale with the firing affect's stored potency (the on-kill kHaste is
		// as strong as the vampirism behind it). base="tag" actions read the event amount instead.
		float aff_potency = 0.0f;
		for (const auto &a : ch->affected) {
			if (a && a->affect_type == t) { aff_potency = a->potency; break; }
		}
		ActionContext ctx = BuildActionContext(ch, ESpell::kUndefined, level, -1.0f, aff_potency);
		ctx.UseExternalActions(&fired);
		ctx.SetEvent(event);              // handlers (and propagated side-spell sub-ctxs) read this
		if (!fired.empty()) {   // a remove/replace_all patch may have emptied the affect's own actions
			RunRoomCycledAction(ctx, world[ch->in_room], fired, 0);
		}
		if (ch->purged()) {
			return true;
		}
		// The affect's additive talent-patches (e.g. kSoulLink's master heal).
		if (RunAdditiveAffectPatches(ctx, ch, world[ch->in_room], t,
				[trig](const talents_actions::Action &a) { return a.GetTrigger().test(trig); })) {
			return true;
		}
		// kPreHit/kPostHit/kKill keep the affect, so charges bound how often it fires. Safe: we iterate the
		// `types` snapshot, not ch->affected.
		SpendCharAffectCharge(ch, t);
	}
	return true;
}

// Obj-affect analog of RunCharEventTriggers: runs a weapon's obj-affect actions on kWeaponHit, with
// cvict = victim (for kTarActor/kTarFightVict) and ovict = the weapon.
bool RunObjAffectWeaponHit(ObjData *weapon, CharData *ch, CharData *victim, int dam) {
	if (!weapon || !ch || !victim || dam <= 0 || ch->in_room == kNowhere) {
		return false;
	}
	if (weapon->get_type() != EObjType::kWeapon || weapon->get_obj_affects().empty()) {
		return false;
	}
	using talents_actions::EActionTrigger;
	std::vector<obj_affects::EObjAffect> types;
	for (const auto &aff : weapon->get_obj_affects()) {
		if (!aff || std::find(types.begin(), types.end(), aff->affect_type) != types.end()) {
			continue;
		}
		for (const auto &a : obj_affects::ObjAffectActions(aff->affect_type).list()) {
			if (a.GetTrigger().test(EActionTrigger::kWeaponHit)) {
				types.push_back(aff->affect_type);
				break;
			}
		}
	}
	if (types.empty()) {
		return false;
	}
	EventContext event;
	event.trigger = EActionTrigger::kWeaponHit;
	event.amount = dam;
	event.weapon = weapon;
	event.actor = victim;
	const int level = GetRealLevel(ch);
	bool any = false;
	for (const obj_affects::EObjAffect t : types) {
		std::vector<talents_actions::Action> fired;
		for (const auto &a : obj_affects::ObjAffectActions(t).list()) {
			if (a.GetTrigger().test(EActionTrigger::kWeaponHit)) {
				fired.push_back(a);
			}
		}
		if (fired.empty()) {
			continue;
		}
		float aff_potency = 0.0f;
		for (const auto &a : weapon->get_obj_affects()) {
			if (a && a->affect_type == t) { aff_potency = a->potency; break; }
		}
		ActionContext ctx = BuildActionContext(ch, ESpell::kUndefined, level, -1.0f, aff_potency);
		ctx.cvict = victim;   // <action target="kTarActor"> resolves off cvict (see RunCharAffectTrigger)
		ctx.ovict = weapon;
		ctx.UseExternalActions(&fired);
		ctx.SetEvent(event);
		RunRoomCycledAction(ctx, world[ch->in_room], fired, 0);
		any = true;
		if (ch->purged() || victim->purged()) {
			return true;
		}
		obj_affects::SpendCharge(weapon, t);   // no-op unless the affect has limited charges
	}
	return any;
}

// Runs one obj-affect trigger action with the actor as the (self-inflicting) caster: emits the trigger
// flavor, runs the action and returns the <trigger return=> verdict (0 = block the actor's action).
static int CastObjTriggerAction(CharData *caster, ObjData *obj, CharData *actor,
		obj_affects::EObjAffect affect_type, const talents_actions::Action &action, float potency) {
	if (actor) {
		const std::string &to_char = obj_affects::ObjAffectMsgRaw(affect_type,
				obj_affects::EObjAffectMsgType::kTriggerToChar);
		if (!to_char.empty()) { act(to_char.c_str(), false, actor, obj, nullptr, kToChar); }
		const std::string &to_room = obj_affects::ObjAffectMsgRaw(affect_type,
				obj_affects::EObjAffectMsgType::kTriggerToRoom);
		if (!to_room.empty()) { act(to_room.c_str(), true, actor, obj, actor, kToRoom | kToArenaListen); }
	}
	const std::vector<talents_actions::Action> single{action};
	ActionContext ctx = BuildActionContext(caster, ESpell::kUndefined, GetRealLevel(caster), potency);
	ctx.cvict = actor;   // <action target="kTarActor"> -> the actor (the trap's victim)
	ctx.ovict = obj;
	// Give a triggered <damage> the affect's own flavor (kDamageTo*) instead of the generic melee-hit line
	// ("X painfully hit you") -- a trap deals impersonal damage, it is not a character attacking.
	ctx.SetAffectDamageMsg(
			obj_affects::ObjAffectMsgRaw(affect_type, obj_affects::EObjAffectMsgType::kDamageToChar),
			obj_affects::ObjAffectMsgRaw(affect_type, obj_affects::EObjAffectMsgType::kDamageToVict),
			obj_affects::ObjAffectMsgRaw(affect_type, obj_affects::EObjAffectMsgType::kDamageToRoom));
	ctx.UseExternalActions(&single);
	RunRoomCycledAction(ctx, world[caster->in_room], single, 0);
	if (const auto h = ctx.GetTriggerReturn()) { return *h; }
	if (const auto t = action.GetTriggerReturn()) { return *t; }
	return 1;   // allow
}

// Fires an object's obj-affect actions matching `trig`. The actor is the (self-inflicting) caster, so a
// trap harms whoever triggers it. Returns false to refuse the actor's action (a <trigger return="0">
// action, e.g. kDartTrap blocking pick/open). Spends trigger charges.
bool RunObjAffectTrigger(ObjData *obj, CharData *actor, talents_actions::EActionTrigger trig) {
	if (!obj || obj->get_obj_affects().empty()) {
		return true;
	}
	// Snapshot so an action that consumes a charge (removing the affect) cannot invalidate iteration.
	struct Pending { obj_affects::EObjAffect type; long caster_id; float potency; };
	std::vector<Pending> pending;
	for (const auto &aff : obj->get_obj_affects()) {
		if (aff) { pending.push_back({aff->affect_type, aff->caster_id, aff->potency}); }
	}
	bool allowed = true;
	for (const auto &p : pending) {
		for (const auto &action : obj_affects::ObjAffectActions(p.type).list()) {
			if (!action.GetTrigger().test(trig)) {
				continue;
			}
			// The affect is SELF-INFLICTING: the actor who triggered it is the caster AND the victim, so
			// the effect (trap damage, poison) never engages combat with whoever SET the affect. With
			// fixed-strength actions (<amount beta="0">) self-sourcing does not weaken it. For an actorless
			// trigger (kDecay) fall back to the setter so the action can still fire. caster_id = provenance.
			CharData *caster = actor ? actor : find_char(p.caster_id);
			if (!caster || caster->in_room == kNowhere) {
				continue;
			}
			const int ret = CastObjTriggerAction(caster, obj, actor, p.type, action, p.potency);
			obj_affects::SpendCharge(obj, p.type);   // spend one trigger charge (removes the affect at 0)
			if (ret == 0) {
				allowed = false;   // refuse the actor's action, but keep firing the rest
			}
			if (actor && actor->purged()) {
				return allowed;   // the actor died from the trap
			}
		}
	}
	return allowed;
}

// On-entry trigger return values: 0 blocks the action that fired the trigger, anything else allows.
// "No value set" means allow, so non-blocking affects need no return attr.
namespace {
constexpr int kEntryTriggerAllow = 1;
constexpr int kEntryTriggerBlock = 0;
}  // namespace

// Spends one trigger charge (no-op for unlimited charges). At 0 the affect is removed from its host
// list (the room's or the exit's) with its kAffExpired line sent to the actor's room.
static void ConsumeAffectCharge(const Affect<room_spells::ERoomApply>::shared_ptr &aff, RoomData *room,
								room_spells::RoomAffects &host, CharData *actor) {
	if (!aff || aff->charges == kUnlimitedAffectCharges) {
		return;
	}
	if (--aff->charges <= 0) {
		// Running out of charges counts as expiry: fire kExpired (in the caster's context) before removal.
		room_spells::RunRoomAffectTrigger(room, find_char(aff->caster_id), aff->affect_type,
										  talents_actions::EActionTrigger::kExpired, aff->potency);
		const std::string &msg =
				room_spells::RoomAffectMsgRaw(aff->affect_type, room_spells::ERoomAffectMsgType::kAffExpiredToRoom);
		if (!msg.empty() && actor && actor->in_room != kNowhere) {
			SendMsgToRoom(msg.c_str(), actor->in_room, 0);
		}
		host.remove(aff);
	}
}

// Run ONE event-triggered (kEnter/kEnterPC) action on the actor. `caster` (the affect's owner) sources
// the cast; `actor` is the target -- carried on ctx.cvict and resolved by EActionTarget::kTarActor.
// Returns the trigger's return value: a manual_cast handler's override (set on ctx) wins, else the
// action's <trigger return=> tag, else "allow". Returns allow without acting if the actor fails the
// action's <target_conditions> (so a filtered-out actor neither gets the effect nor blocks).
static int CastRoomEntryAction(CharData *caster, RoomData *room, CharData *actor,
							   room_spells::ERoomAffect affect_type,
							   const talents_actions::Action &action, float potency, int seal_strength,
							   talents_actions::EActionTrigger ev) {
	if (TargetIsBlocked(actor, action.GetBlocking())
			|| !TargetMeetsRequired(actor, action.GetRequired())) {
		return kEntryTriggerAllow;
	}
	// prob gate: a miss leaves the event untouched (no effect, no block). The chance is the affect's seal
	// strength (Affect::modifier, e.g. kForbidden) if it has a seal_strength formula, else <trigger prob=N>,
	// else always.
	const std::optional<int> prob = room_spells::RoomAffectHasSeal(affect_type)
			? std::optional<int>(seal_strength)
			: action.GetTriggerProb();
	if (prob && number(1, 100) > *prob) {
		return kEntryTriggerAllow;
	}
	// The affect's flavor, shown BEFORE the effect and keyed by which trigger fired, so walking through /
	// opening / picking / unlocking read distinctly. Silent if not authored.
	using room_spells::ERoomAffectMsgType;
	ERoomAffectMsgType char_key = ERoomAffectMsgType::kTriggerOnEntryToChar;
	ERoomAffectMsgType room_key = ERoomAffectMsgType::kTriggerOnEntryToRoom;
	switch (ev) {
		case talents_actions::EActionTrigger::kOpen:
			char_key = ERoomAffectMsgType::kTriggerOnOpenToChar;
			room_key = ERoomAffectMsgType::kTriggerOnOpenToRoom;
			break;
		case talents_actions::EActionTrigger::kPick:
			char_key = ERoomAffectMsgType::kTriggerOnPickToChar;
			room_key = ERoomAffectMsgType::kTriggerOnPickToRoom;
			break;
		case talents_actions::EActionTrigger::kUnlock:
			char_key = ERoomAffectMsgType::kTriggerOnUnlockToChar;
			room_key = ERoomAffectMsgType::kTriggerOnUnlockToRoom;
			break;
		case talents_actions::EActionTrigger::kClose:
			char_key = ERoomAffectMsgType::kTriggerOnCloseToChar;
			room_key = ERoomAffectMsgType::kTriggerOnCloseToRoom;
			break;
		case talents_actions::EActionTrigger::kLock:
			char_key = ERoomAffectMsgType::kTriggerOnLockToChar;
			room_key = ERoomAffectMsgType::kTriggerOnLockToRoom;
			break;
		default:  // kEnter family (move-through) and pulse
			break;
	}
	const std::string &to_char = room_spells::RoomAffectMsgRaw(affect_type, char_key);
	if (!to_char.empty()) { act(to_char.c_str(), false, actor, nullptr, nullptr, kToChar); }
	const std::string &to_room = room_spells::RoomAffectMsgRaw(affect_type, room_key);
	if (!to_room.empty()) { act(to_room.c_str(), true, actor, nullptr, actor, kToRoom | kToArenaListen); }
	const std::vector<talents_actions::Action> single{action};
	ActionContext ctx = BuildActionContext(caster, ESpell::kUndefined, GetRealLevel(caster), potency);
	ctx.cvict = actor;
	ctx.UseExternalActions(&single);
	const ECastResult eff = RunRoomCycledAction(ctx, room, single, 0);
	// A fired trigger with no visible effect (refused cast, or a violent side_spell suppressed by a peaceful
	// room) would look like a silent no-op, so emit the affect's kTriggerNoEffect flash.
	if (action.Contains(talents_actions::EAction::kSideSpell)) {
		bool no_effect = (eff == ECastResult::kNotCast);
		if (!no_effect && ROOM_FLAGGED(actor->in_room, ERoomFlag::kPeaceful)) {
			for (const auto s : action.GetSideSpells()) {
				if (MUD::Spell(s).IsViolent()) { no_effect = true; break; }
			}
		}
		if (no_effect) {
			const std::string &flash = room_spells::RoomAffectMsgRaw(affect_type, ERoomAffectMsgType::kTriggerNoEffect);
			if (!flash.empty()) { SendMsgToRoom(flash.c_str(), actor->in_room, 0); }
		}
	}
	if (const auto h = ctx.GetTriggerReturn()) { return *h; }       // handler override wins
	if (const auto t = action.GetTriggerReturn()) { return *t; }    // else the <trigger return=> tag
	return kEntryTriggerAllow;                                       // else allow
}

bool room_spells::RunRoomEntryTriggers(CharData *actor, RoomData *room,
									   room_spells::EEntryTriggerPhase phase) {
	using room_spells::EEntryTriggerPhase;
	if (!actor || !room || privilege::IsImmortal(actor)) {
		return true;   // affect triggers never fire on (or block) immortals
	}
	const bool actor_is_pc = !actor->IsNpc();
	// Snapshot {affect, caster, potency, strength} so running an action (which could alter
	// room->affected) cannot invalidate the iteration. `strength` is the affect's seal strength
	// (Affect::modifier) -- the block chance for seal-strength affects (kForbidden).
	struct Pending { room_spells::ERoomAffect type; long caster_id; float potency; int strength;
					 Affect<room_spells::ERoomApply>::shared_ptr aff; };
	std::vector<Pending> pending;
	for (const auto &aff : room->affected) {
		if (aff) { pending.push_back({aff->affect_type, aff->caster_id, aff->potency, aff->modifier, aff}); }
	}
	bool allowed = true;
	for (const auto &p : pending) {
		for (const auto &action : room_spells::RoomAffectActions(p.type).list()) {
			const auto &trig = action.GetTrigger();
			const bool fires = trig.test(talents_actions::EActionTrigger::kEnter)
					|| (actor_is_pc && trig.test(talents_actions::EActionTrigger::kEnterPC))
					|| (!actor_is_pc && trig.test(talents_actions::EActionTrigger::kEnterNPC));
			if (!fires) { continue; }
			// An action "can block" iff it yields a non-default return: a <trigger return=> tag or a
			// manual_cast handler that may set one. Such actions run in the BEFORE-placement kBlockCheck
			// pass (where return=0 refuses the move); pure-effect actions (the common case) run in the
			// AFTER-placement effect passes so their effect/messages keep destination-room context.
			const bool can_block = action.GetTriggerReturn().has_value() || !action.GetManualHandler().empty();
			if (phase == EEntryTriggerPhase::kBlockCheck && !can_block) { continue; }
			if (phase == EEntryTriggerPhase::kEffectsNonBlocking && can_block) { continue; }
			// Действию без следствий в проходе за следствиями делать нечего (у печати -- один вердикт
			// <trigger return="0"/>). Принудительный вход (скакун за седоком, телепорт, бегство) вердикта не
			// смотрит, и иначе печать на каждом шаге сообщала бы уже вошедшему, что его не пускают. Вердикт
			// спрашивают в kBlockCheck, пока вход ещё можно отменить.
			if (phase != EEntryTriggerPhase::kBlockCheck && !action.HasEffects()) { continue; }
			CharData *caster = find_char(p.caster_id);
			if (!caster) { continue; }   // no owner to source the cast this entry
			const int ret = CastRoomEntryAction(caster, room, actor, p.type, action, p.potency, p.strength,
					talents_actions::EActionTrigger::kEnter);   // room entry -> the "entry" message pair
			ConsumeAffectCharge(p.aff, room, room->affected, actor);   // spend a trigger charge (no-op if unlimited)
			// Only the block-check pass enforces the verdict; the effect passes ignore it.
			if (phase == EEntryTriggerPhase::kBlockCheck && ret == kEntryTriggerBlock) {
				allowed = false;   // block, but keep running the rest (so all blocking actions still fire)
			}
		}
	}
	return allowed;
}

// Fires door affects when someone acts on the door in direction `dir` from `room`. Like
// RunRoomEntryTriggers, but the host is the exit's affect list and there is a single dispatch point
// (the door command), so no phase split. Returns false to refuse the door action.
bool room_spells::RunDoorTriggers(CharData *actor, RoomData *room, int dir, talents_actions::EActionTrigger ev) {
	if (!actor || !room || dir < 0 || dir >= EDirection::kMaxDirNum || privilege::IsImmortal(actor)) {
		return true;   // affect triggers never fire on (or block) immortals
	}
	const auto near_exit = room->dir_option[dir];
	if (!near_exit) { return true; }
	// Snapshot {affect, caster, potency, strength} so running an action (which could alter the exit's
	// affect list) cannot invalidate the iteration.
	struct Pending { room_spells::ERoomAffect type; long caster_id; float potency; int strength;
					 Affect<room_spells::ERoomApply>::shared_ptr aff; RoomData::exit_data_ptr host; };
	std::vector<Pending> pending;
	auto collect = [&pending](const RoomData::exit_data_ptr &ex) {
		if (!ex) { return; }
		for (const auto &aff : ex->affected) {
			if (aff) { pending.push_back({aff->affect_type, aff->caster_id, aff->potency, aff->modifier, aff, ex}); }
		}
	};
	collect(near_exit);
	// A door affect cast from the far room lives on the reverse exit, so collect that side too (one
	// validated hop, like do_doorcmd's `back`) -- the trap reacts whichever side opens the door.
	if (near_exit->to_room() != kNowhere) {
		static const int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth,
									   EDirection::kEast, EDirection::kDown, EDirection::kUp};
		const auto rev_exit = world[near_exit->to_room()]->dir_option[rev_dir[dir]];
		if (rev_exit && rev_exit->to_room() == actor->in_room) {
			collect(rev_exit);
		}
	}
	const bool actor_is_pc = !actor->IsNpc();
	bool allowed = true;
	for (const auto &p : pending) {
		for (const auto &action : room_spells::RoomAffectActions(p.type).list()) {
			const auto &trig = action.GetTrigger();
			// kEnter here means the enter family (passage move-through), expanded by actor kind like
			// RunRoomEntryTriggers; door verbs are matched verbatim.
			bool fires = trig.test(ev);
			if (ev == talents_actions::EActionTrigger::kEnter) {
				fires = fires || (actor_is_pc && trig.test(talents_actions::EActionTrigger::kEnterPC))
						|| (!actor_is_pc && trig.test(talents_actions::EActionTrigger::kEnterNPC));
			}
			if (!fires) { continue; }
			CharData *caster = find_char(p.caster_id);
			if (!caster) { continue; }   // no owner to source the cast
			const int ret = CastRoomEntryAction(caster, room, actor, p.type, action, p.potency, p.strength, ev);
			ConsumeAffectCharge(p.aff, room, p.host->affected, actor);   // spend a trigger charge (no-op if unlimited)
			if (ret == kEntryTriggerBlock) {
				allowed = false;   // refuse the door action, but keep firing the rest
			}
		}
	}
	return allowed;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
