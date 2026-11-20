/**
 \file action_unaffect.cpp - a part of the Bylins engine.
 \brief Unaffect stage of the cast pipeline: dispelling and curing affects on characters, rooms and exits,
        and displacing affects that conflict with a newly imposed one.
*/

#include "magic_internal.h"
#include "spell_trace.h"
#include "gameplay/affects/room_affect_messages.h"
#include "gameplay/affects/affect_messages.h"
#include "utils/grammar/declensions.h"
#include "gameplay/affects/affect_handler.h"
#include "engine/db/global_objects.h"

namespace {
// The <unaffect affect_flags=> set lists the EAffFlag bits (kAfCurable / kAfDispellable) an affect
// must carry to be removable by that unaffect. No matching flag (charm/quest effects, anything
// applied in code) = irremovable.
bool AffectMatchesFlags(const Affect<EApply>::shared_ptr &affect, const AffectFlags &flags) {
	if (!affect) {
		return false;
	}
	// Пакет призванного существа (kAfCharmBond: привязка и бафы, выданные вместе с ней кодом) снятию
	// не подлежит вовсе, какие бы флаги снятия у узлов ни стояли. Разрыв чар снимает пакет напрямую
	// (RemoveCharmBond), минуя эту проверку. Врождённые бафы моба из флагов прототипа
	// (BuildMaterializedAffect) снимаются -- на то и заклинание «разлом».
	if (affect->battleflag.get(kAfCharmBond)) {
		return false;
	}
	return affect->battleflag.intersects(flags);
}

// True if the victim carries a removable affect of the given type (one matching `flags`).
bool HasDispellableAffect(CharData *victim, EAffect affect_type, const AffectFlags &flags) {
	for (const auto &aff : victim->affected) {
		if (aff && aff->affect_type == affect_type && AffectMatchesFlags(aff, flags)) {
			return true;
		}
	}
	return false;
}

// Evaluate a <blocking>/<breaking> condition set by affect identity:
// true if any any_of affect is present, or every all_of affect is present.
bool UnaffectConditionMet(CharData *victim, const talents_actions::TalentUnaffect::Set &set) {
	for (const auto affect_type : set.any_of.chars) {
		if (IsAffected(victim, affect_type)) {
			return true;
		}
	}
	if (!set.all_of.chars.empty()) {
		bool all = true;
		for (const auto affect_type : set.all_of.chars) {
			if (!IsAffected(victim, affect_type)) {
				all = false;
				break;
			}
		}
		if (all) {
			return true;
		}
	}
	return false;
}

// One affect queued for dispel. If the dispel of a candidate with break_on_fail is resisted, the
// whole cast chain breaks.
struct RemovalCandidate {
	bool break_on_fail{false};
	EAffect affect_type{EAffect::kUndefined};                          // char target: the affect to remove
	room_spells::ERoomAffect room_affect_type{room_spells::ERoomAffect::kUndefined};  // room target: the affect to remove
};

// Build the list of affects to dispel for a <remove>/<remove_anyway> set:
//   - any_of (explicit list)   -> the first listed affect that is present and matches `flags`
//   - all_of (explicit list)   -> every listed affect that is present and matches `flags`
//   - wildcard_any (any_of="*")-> ONE eligible affect picked uniformly at random
//   - wildcard_all (all_of="*")-> EVERY eligible affect on the victim
// Each candidate carries the set's breaking_by_failure.
void CollectRemovals(CharData *victim, const talents_actions::TalentUnaffect::Set &set,
					 std::vector<RemovalCandidate> &out, const AffectFlags &flags, bool debuff_only = false) {
	// debuff_only skips clear buffs (EBuff::kYes), so a friendly wildcard cleanse ("unweave")
	// removes debuffs/ambiguous affects without stripping real buffs.
	const auto eligible = [&](const Affect<EApply>::shared_ptr &aff) {
		return AffectMatchesFlags(aff, flags)
			&& !(debuff_only && affects::AffectBuffKind(aff->affect_type) == affects::EBuff::kYes);
	};
	if (set.wildcard_any) {
		EAffect pick_at = EAffect::kUndefined;
		int seen = 0;
		for (const auto &aff : victim->affected) {
			if (eligible(aff) && number(1, ++seen) == 1) {
				pick_at = aff->affect_type;
			}
		}
		if (pick_at != EAffect::kUndefined) {
			out.push_back({.break_on_fail = set.breaking_by_failure, .affect_type = pick_at});
		}
	} else {
		for (const auto affect_type : set.any_of.chars) {
			if (HasDispellableAffect(victim, affect_type, flags)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .affect_type = affect_type});
				break;
			}
		}
	}
	if (set.wildcard_all) {
		for (const auto &aff : victim->affected) {
			if (eligible(aff)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .affect_type = aff->affect_type});
			}
		}
	} else {
		for (const auto affect_type : set.all_of.chars) {
			if (HasDispellableAffect(victim, affect_type, flags)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .affect_type = affect_type});
			}
		}
	}
}

// A successful dispel peels one stack: every removable affect of the type with stacks > 1 loses a
// stack and ~modifier/stacks; if none has more than one stack, the removable instances go entirely.
// Снимаются только экземпляры, прошедшие фильтр снятия: решение «снимать» принимается по одному
// подходящему экземпляру (HasDispellableAffect), и без фильтра вместе с накастованным бафом улетали
// бы врождённый аффект моба (battleflag пуст) и пакет призванного (kAfCharmBond). Вещевой экземпляр
// снимается: он подавляется на предмете (SuppressSourceEquipmentAffect) и вернётся сам.
static const AffectFlags kDispellableInstance{kAfCurable, kAfDispellable};

void ReduceStackOrRemove(CharData *victim, EAffect affect_type) {
	const auto removable = [affect_type](const Affect<EApply>::shared_ptr &aff) {
		return aff && aff->affect_type == affect_type && AffectMatchesFlags(aff, kDispellableInstance);
	};
	bool any_multi = false;
	for (const auto &aff : victim->affected) {
		if (removable(aff) && aff->stacks > 1) {
			any_multi = true;
			break;
		}
	}
	if (!any_multi) {
		auto it = victim->affected.begin();
		while (it != victim->affected.end()) {
			if (removable(*it)) {
				it = RemoveAffect(victim, it);
			} else {
				++it;
			}
		}
		affect_total(victim);
		return;
	}
	std::vector<Affect<EApply>> rebuilt;
	for (const auto &aff : victim->affected) {
		if (removable(aff)) {
			Affect<EApply> peeled = *aff;
			if (peeled.stacks > 1) {
				peeled.modifier = static_cast<int>(
						static_cast<int64_t>(peeled.modifier) * (peeled.stacks - 1) / peeled.stacks);
				peeled.stacks -= 1;
			}
			rebuilt.push_back(peeled);
		}
	}
	// Снимаем только снимаемые экземпляры: врождённый и пакет призванного остаются на месте.
	auto it = victim->affected.begin();
	while (it != victim->affected.end()) {
		if (removable(*it)) {
			it = RemoveAffect(victim, it);
		} else {
			++it;
		}
	}
	for (auto &peeled : rebuilt) {
		affect_to_char(victim, peeled);
	}
	affect_total(victim);
}

// A dispelled affect that a worn item materialized (kAfFromEquipment) is suppressed on the source
// item for this long and re-materializes afterwards. Decremented once per MUD hour
// (obj_point_update), so the unit is GAME HOURS. TODO: scale by f(affect_potency - dispel).
constexpr int kEquipmentAffectSuppressHours = 10;

// displaced -- аффект не развеяли, а вытеснили надетой вещью с несовместимым волшебством. Срок
// возврата обещать нельзя: аффект вернётся лишь когда вытеснившие чары уйдут, поэтому сообщение
// называет причину, а не время.
void SuppressSourceEquipmentAffect(CharData *victim, EAffect affect_type, double competence,
								   bool displaced = false) {
	// A set-conferred affect (kAfFromSet) has no single owning item, so the suppression goes to every
	// worn set piece (one shared timer, survives anchor swaps).
	long src_id = -1;
	bool is_set = false;
	bool found = false;
	for (const auto &aff : victim->affected) {
		if (aff && aff->affect_type == affect_type && IS_SET(aff->battleflag, EAffFlag::kAfFromEquipment)) {
			src_id = aff->caster_id;
			is_set = IS_SET(aff->battleflag, EAffFlag::kAfFromSet);
			found = true;
			break;
		}
	}
	if (!found) {
		return;
	}
	char msg[kMaxStringLength];
	if (is_set) {
		// No extra line: the affect's own dispel narration already told the wearer.
		for (int i = EEquipPos::kFirstEquipPos; i < EEquipPos::kNumEquipPos; ++i) {
			ObjData *obj = GET_EQ(victim, i);
			if (obj && obj_sets::is_set_item(obj)) {
				obj_affects::SuppressEquipAffect(obj, affect_type, kEquipmentAffectSuppressHours, static_cast<float>(competence));
			}
		}
		return;
	}
	for (int i = EEquipPos::kFirstEquipPos; i < EEquipPos::kNumEquipPos; ++i) {
		ObjData *obj = GET_EQ(victim, i);
		if (obj && obj->get_id() == src_id) {
			obj_affects::SuppressEquipAffect(obj, affect_type, kEquipmentAffectSuppressHours, static_cast<float>(competence));
			if (displaced) {
				snprintf(msg, sizeof(msg), "Волшебство $o2 уступило несовместимым чарам.");
			} else {
				snprintf(msg, sizeof(msg), "Волшебство $o2 рассеяно и восстановится через %d %s.",
						 kEquipmentAffectSuppressHours,
						 grammar::GetDeclensionInNumber(kEquipmentAffectSuppressHours, grammar::EWhat::kHour));
			}
			act(msg, false, victim, obj, nullptr, kToChar);
			return;
		}
	}
}

void RemoveAffectAndAnnounce(CharData *ch, CharData *victim, EAffect affect_type, double competence,
							 bool displaced = false) {
	// Fire the affect's kDispell <actions> BEFORE stripping it, with the dispeller as both caster and
	// actor: the retaliation is self-inflicted (no pk-aggro, no revenge rights, the bearer is never
	// dragged into PvP). Only on the announced-dispel path (dispel magic / unaffect).
	RunCharAffectTrigger(ch, affect_type, talents_actions::EActionTrigger::kDispell, ch);
	// An item-conferred affect is suppressed on the item (auto-returns) instead of being lost.
	SuppressSourceEquipmentAffect(victim, affect_type, competence, displaced);
	ReduceStackOrRemove(victim, affect_type);
	// Остался экземпляр этого аффекта (врождённый, пакет призванного) -- эффект никуда не делся,
	// рассказывать про «угасло» нельзя. Истечение по таймеру проверяет то же (NoLiveTwinLeft).
	if (IsAffected(victim, affect_type)) {
		return;
	}
	const std::string &to_vict = affects::AffectDispelMsg(affect_type, /*to_room=*/false);
	if (!to_vict.empty()) {
		act(to_vict.c_str(), false, victim, nullptr, ch, kToChar);
	}
	const std::string &to_room = affects::AffectDispelMsg(affect_type, /*to_room=*/true);
	if (!to_room.empty()) {
		act(to_room.c_str(), true, victim, nullptr, ch, kToRoom | kToArenaListen);
	}
}

// Ceiling on an affect's potency: a negative <unaffect decay> raises it on each failed removal;
// the cap stops unbounded growth (and overflow once it feeds integer modifiers).
constexpr float kMaxAffectPotency = 30000.0f;

// Turns a competence gap (dispeller - affect, skill+stat points) into d100 win-probability
// points. With the competence range ~2..12, 4 gives roughly a +/-40-point swing.
constexpr double kDispelSkillWeight = 4.0;

// On a FAILED removal, shift the surviving affect's potency by `decay` percent of the dispeller's
// strength -- positive weakens, negative strengthens. Clamped to [0, kMaxAffectPotency].
void ApplyDispelDecay(float &affect_potency, float dispel_strength, int decay) {
	if (decay == 0) {
		return;
	}
	const float delta = dispel_strength * static_cast<float>(decay) / 100.0f;
	affect_potency = std::clamp(affect_potency - delta, 0.0f, kMaxAffectPotency);
}

// True if `dispel_spell` removes `affect_type` from victim: a non-violent dispel of a buff always
// succeeds; anything else is a d100 contest against the affect's stored potency (see below).
bool DispelSucceeds(CharData *ch, CharData *victim, ESpell dispel_spell, EAffect affect_type,
					int dispel_bonus, double competence, double area_coeff = 1.0, int decay = 0) {
	float affect_potency = 0.0f;
	float *matched_potency = nullptr;
	for (const auto &aff : victim->affected) {
		if (aff && aff->affect_type == affect_type) {
			affect_potency = aff->potency;
			matched_potency = &aff->potency;
			break;
		}
	}
	// Tester / immortal debug line: one per affect-vs-dispel pair. Reason codes:
	//   buff -- a non-violent dispel of a buff auto-passes (no contest rolled).
	//   roll -- the d100 contest was rolled against the threshold.
	auto emit_debug = [&](int threshold, const char *kind, bool ok) {
		spell_trace::Line(ch, nullptr,
				 "Unaffect: %s [C: %.1f]. Target: %s [p: %.1f]. threshold %d%%. %s (%s).\r\n",
				 MUD::Spell(dispel_spell).GetCName(), competence,
				 affects::AffectMsg(affect_type, affects::EAffectMsgType::kShortDesc).c_str(), affect_potency,
				 threshold, ok ? "Success" : "Fail", kind);
	};
	// A d100 skill contest. The dispeller's competence advantage over the affect (scaled by
	// kDispelSkillWeight) plus the per-spell dispel_bonus and the affect's own dispel_mod set the
	// win threshold; the clamp gives a symmetric 5% upset floor and 5% save ceiling.
	const double raw = kDispelSkillWeight * (area_coeff * competence - affect_potency)
			+ dispel_bonus + affects::AffectDispelMod(affect_type);
	const int threshold = std::clamp(static_cast<int>(std::lround(raw)), 5, 95);
	// A non-violent (per-target) dispel of a buff needs no contest (ally cleansing); an enemy-hand
	// dispel of a buff, or any dispel of a debuff, rolls.
	if (!MUD::Spell(dispel_spell).IsViolentAgainst(ch, victim)
			&& affects::AffectBuffKind(affect_type) == affects::EBuff::kYes) {
		emit_debug(threshold, "buff", true);
		return true;
	}
	const bool ok = number(1, 100) <= threshold;
	emit_debug(threshold, "roll", ok);
	if (!ok && matched_potency) {
		ApplyDispelDecay(*matched_potency, static_cast<float>(kDispelSkillWeight * competence), decay);
	}
	return ok;
}

// === Room-target overloads ===
// Mirror the char helpers above for a RoomData target. Room affects share the Affect template, so
// only the storage and the apply/remove APIs differ. PK gating is intentionally NOT replicated:
// there is no policy for room dispel concurrency yet.

bool AffectMatchesFlags(const Affect<room_spells::ERoomApply>::shared_ptr &affect, const AffectFlags &flags) {
	return affect && affect->battleflag.intersects(flags);
}

bool HasDispellableAffect(RoomData *room, room_spells::ERoomAffect affect_type, const AffectFlags &flags) {
	for (const auto &aff : room->affected) {
		if (aff && aff->affect_type == affect_type && AffectMatchesFlags(aff, flags)) {
			return true;
		}
	}
	return false;
}

bool UnaffectConditionMet(RoomData *room, const talents_actions::TalentUnaffect::Set &set) {
	auto room_has = [room](room_spells::ERoomAffect ra) {
		for (const auto &aff : room->affected) {
			if (aff && aff->affect_type == ra) {
				return true;
			}
		}
		return false;
	};
	for (const auto ra : set.any_of.rooms) {
		if (room_has(ra)) {
			return true;
		}
	}
	if (!set.all_of.rooms.empty()) {
		bool all = true;
		for (const auto ra : set.all_of.rooms) {
			if (!room_has(ra)) {
				all = false;
				break;
			}
		}
		if (all) {
			return true;
		}
	}
	return false;
}

void CollectRemovals(RoomData *room, const talents_actions::TalentUnaffect::Set &set,
					 std::vector<RemovalCandidate> &out, const AffectFlags &flags, bool /*debuff_only*/ = false) {
	// debuff_only is a no-op for room affects (no buff/debuff notion); the parameter exists only so
	// RunCastUnaffects can call this from the shared template.
	if (set.wildcard_any) {
		room_spells::ERoomAffect pick = room_spells::ERoomAffect::kUndefined;
		int seen = 0;
		for (const auto &aff : room->affected) {
			if (AffectMatchesFlags(aff, flags) && number(1, ++seen) == 1) {
				pick = aff->affect_type;
			}
		}
		if (pick != room_spells::ERoomAffect::kUndefined) {
			out.push_back({.break_on_fail = set.breaking_by_failure, .room_affect_type = pick});
		}
	} else {
		for (const auto ra : set.any_of.rooms) {
			if (HasDispellableAffect(room, ra, flags)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .room_affect_type = ra});
				break;
			}
		}
	}
	if (set.wildcard_all) {
		for (const auto &aff : room->affected) {
			if (AffectMatchesFlags(aff, flags)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .room_affect_type = aff->affect_type});
			}
		}
	} else {
		for (const auto ra : set.all_of.rooms) {
			if (HasDispellableAffect(room, ra, flags)) {
				out.push_back({.break_on_fail = set.breaking_by_failure, .room_affect_type = ra});
			}
		}
	}
}

// Strip every affect of `removed` from the room (room affects don't stack, so peeling has no
// meaning here) and emit the room affect's own kAffDispelledTo{Char,Room} narration.
void RemoveAffectAndAnnounce(CharData *ch, RoomData *room, room_spells::ERoomAffect removed) {
	// Fire the room affect's kDispell actions BEFORE the strip, with the DISPELLER as the caster, so
	// the retaliation is self-inflicted (no PvP, same model as char affects).
	float dispel_potency = 0.0f;
	for (const auto &aff : room->affected) {
		if (aff && aff->affect_type == removed) { dispel_potency = aff->potency; break; }
	}
	room_spells::RunRoomAffectTrigger(room, ch, removed, talents_actions::EActionTrigger::kDispell, dispel_potency);
	// Erase directly rather than via room_spells::RoomRemoveAffect: its empty-list guard is redundant
	// inside a live iteration.
	for (auto it = room->affected.begin(); it != room->affected.end(); ) {
		if (*it && (*it)->affect_type == removed) {
			it = room->affected.erase(it);
		} else {
			++it;
		}
	}
	// Sheaf-direct (no kDefault fallback): a room dispel shows only the affect's OWN
	// dispel message, never the char-centric kDefault.
	const auto &to_char = room_spells::RoomAffectMsgRaw(removed, room_spells::ERoomAffectMsgType::kAffDispelledToChar);
	if (!to_char.empty()) {
		act(to_char.c_str(), false, ch, nullptr, nullptr, kToChar);
	}
	const auto &to_room = room_spells::RoomAffectMsgRaw(removed, room_spells::ERoomAffectMsgType::kAffDispelledToRoom);
	if (!to_room.empty()) {
		act(to_room.c_str(), true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
	}
}

bool DispelSucceeds(CharData *ch, RoomData *room, ESpell dispel_spell, room_spells::ERoomAffect affect_type,
					int dispel_bonus, double competence, double area_coeff = 1.0, int decay = 0) {
	float affect_potency = 0.0f;
	long author_uid = 0;
	float *matched_potency = nullptr;
	for (const auto &aff : room->affected) {
		if (aff && aff->affect_type == affect_type) {
			affect_potency = aff->potency;
			author_uid = aff->caster_id;
			matched_potency = &aff->potency;
			break;
		}
	}
	auto emit_debug = [&](int threshold, const char *kind, bool ok) {
		spell_trace::Line(ch, nullptr,
				 "Unaffect: %s [C: %.1f]. Target room: %s [p: %.1f]. threshold %d%%. %s (%s).\r\n",
				 MUD::Spell(dispel_spell).GetCName(), competence,
				 NAME_BY_ITEM<room_spells::ERoomAffect>(affect_type).c_str(), affect_potency,
				 threshold, ok ? "Success" : "Fail", kind);
	};
	// Author/ally-aware room dispel: the affect's author or a live ally dispels for free; anyone else
	// must win the d100 contest (see the char overload), and a player vs a live author commits an
	// aggressive PK act. Dispellability is already filtered upstream by <unaffect affect_flags=>.
	const double raw = kDispelSkillWeight * (area_coeff * competence - affect_potency) + dispel_bonus;
	const int threshold = std::clamp(static_cast<int>(std::lround(raw)), 5, 95);
	const auto access = room_spells::ClassifyRoomAffectAccess(ch, author_uid);
	if (access.free) {
		emit_debug(threshold, "ally", true);
		return true;
	}
	if (!ch->IsNpc() && access.author && !pk_agro_action(ch, access.author)) {
		emit_debug(threshold, "pk", false);
		return false;
	}
	const bool ok = number(1, 100) <= threshold;
	emit_debug(threshold, "roll", ok);
	if (!ok && matched_potency) {
		ApplyDispelDecay(*matched_potency, static_cast<float>(kDispelSkillWeight * competence), decay);
	}
	return ok;
}

// Templated dispel-loop shared by the char and room branches of CastUnaffects. The helpers
// (UnaffectConditionMet / CollectRemovals / DispelSucceeds / RemoveAffectAndAnnounce) are all
// overloaded on the target type, so overload resolution picks the right one at instantiation.
// PK aggro gating is char-only; for a RoomData target the `if constexpr` branch is dropped.
template<typename TTarget>
EStageResult RunCastUnaffects(CharData *ch, TTarget *target, ESpell spell_id,
							  const talents_actions::TalentUnaffect &unaffect, double area_coeff,
							  double &removed_out, double competence) {
	const bool blocking = UnaffectConditionMet(target, unaffect.GetBlocking());
	const bool breaking = UnaffectConditionMet(target, unaffect.GetBreaking());
	bool break_chain = breaking;

	const AffectFlags &flags = unaffect.GetAffectFlags();
	std::vector<RemovalCandidate> to_remove;
	CollectRemovals(target, unaffect.GetRemoveAnyway(), to_remove, flags, unaffect.GetDebuffOnly());
	if (!blocking) {
		CollectRemovals(target, unaffect.GetRemove(), to_remove, flags, unaffect.GetDebuffOnly());
	}
		// An affect with several instances (e.g. kPoisoned on several locations) must be removed and
		// announced ONCE: RemoveAffectAndAnnounce strips ALL instances, so dedup by affect identity.
	{
		std::vector<RemovalCandidate> deduped;
		for (const auto &c : to_remove) {
			bool dup = false;
			for (const auto &e : deduped) {
				const bool same = std::is_same_v<TTarget, CharData> ? (e.affect_type == c.affect_type)
																	: (e.room_affect_type == c.room_affect_type);
				if (same) { dup = true; break; }
			}
			if (!dup) { deduped.push_back(c); }
		}
		to_remove.swap(deduped);
	}

	// A non-violent unaffect (ally cleanse or self-cast) must not touch item-conferred affects --
	// cleansing a friend's debuffs must not strip the buffs their gear grants. Char target only.
	if constexpr (std::is_same_v<TTarget, CharData>) {
		if (!MUD::Spell(spell_id).IsViolentAgainst(ch, target)) {
			std::vector<RemovalCandidate> kept;
			for (const auto &c : to_remove) {
				bool from_equipment = false;
				for (const auto &aff : target->affected) {
					if (aff && aff->affect_type == c.affect_type
							&& IS_SET(aff->battleflag, EAffFlag::kAfFromEquipment)) {
						from_equipment = true;
						break;
					}
				}
				if (!from_equipment) {
					kept.push_back(c);
				}
			}
			to_remove.swap(kept);
		}
	}

	if (!to_remove.empty()) {
		if constexpr (std::is_same_v<TTarget, CharData>) {
			// PK-action check, keyed on the first dispelled affect; a disallowed action aborts the removal.
			// A benign cast (violent="N") that only incidentally strips a buff is not aggression: e.g. a
			// group prismatic aura stripping an ally's sanctuary must not trip pk_agro_action.
			if (ch != target && MUD::Spell(spell_id).IsViolentAgainst(ch, target)) {
				// Classify by the dispelled affect's OWN buff flag. Stripping a buff is aggression against the
				// target; stripping a debuff undoes the work of whoever the target fights, so it aggros that
				// enemy; an ambiguous affect triggers no automatic PK action.
				const auto buff = affects::AffectBuffKind(to_remove.front().affect_type);
				if (buff == affects::EBuff::kYes) {
					if (!pk_agro_action(ch, target)) {
						return EStageResult::kSuccess;
					}
				} else if (buff == affects::EBuff::kNo && target->GetEnemy()) {
					if (!pk_agro_action(ch, target->GetEnemy())) {
						return EStageResult::kSuccess;
					}
				}
			}
		}
		bool removed_any = false;
		bool resisted_any = false;
		for (const auto &cand : to_remove) {
			bool ok;
			double removed_pot = 0.0;
			if constexpr (std::is_same_v<TTarget, CharData>) {
				ok = DispelSucceeds(ch, target, spell_id, cand.affect_type, unaffect.GetDispelBonus(),
								  competence, area_coeff, unaffect.GetDecay());
				if (ok) {
					for (const auto &aff : target->affected) {
						if (aff && aff->affect_type == cand.affect_type) { removed_pot = aff->potency; break; }
					}
				}
			} else {
				ok = DispelSucceeds(ch, target, spell_id, cand.room_affect_type, unaffect.GetDispelBonus(),
								  competence, area_coeff, unaffect.GetDecay());
				if (ok) {
					for (const auto &aff : target->affected) {
						if (aff && aff->affect_type == cand.room_affect_type) { removed_pot = aff->potency; break; }
					}
				}
			}
			if (ok) {
				if constexpr (std::is_same_v<TTarget, CharData>) {
					RemoveAffectAndAnnounce(ch, target, cand.affect_type, competence);
				} else {
					RemoveAffectAndAnnounce(ch, target, cand.room_affect_type);
				}
				removed_any = true;
				removed_out += removed_pot;
			} else {
				resisted_any = true;
				if (cand.break_on_fail) {
					break_chain = true;
				}
			}
		}
		if (!removed_any && resisted_any && !MUD::Spell(spell_id).IsFlagged(kMagAffects)) {
			SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kNoeffect) + "\r\n", ch);
		}
	} else if (breaking
			|| (!MUD::Spell(spell_id).IsFlagged(kMagAffects)
				&& (!unaffect.GetRemove().empty() || !unaffect.GetRemoveAnyway().empty()))) {
		// Nothing on the target matched the <unaffect> filter at all -- distinct from the kNoeffect
		// above (something matched, the dispel roll failed).
		SendMsgToChar(MUD::SpellMessages().GetMessage(spell_id, ESpellMsg::kUnaffectNoMatch) + "\r\n", ch);
	}

	return break_chain ? EStageResult::kBreak : EStageResult::kSuccess;
}

}  // namespace

// Правило берём у одноимённого заклинания аффекта, как это делает материализация. Смотрим оба
// списка запрета: у взаимоисключающих пар они совпадают, но заклинание вправе перечислить в них разное.
std::vector<EAffect> ConflictingAffects(EAffect incoming) {
	std::vector<EAffect> out;
	ESpell spell_id = ESpell::kUndefined;
	try {
		spell_id = ITEM_BY_NAME<ESpell>(NAME_BY_ITEM<EAffect>(incoming));
	} catch (const std::out_of_range &) {
		return out;   // у аффекта нет одноимённого заклинания -- правила взаимоисключения нет
	}
	const auto &actions = MUD::Spell(spell_id).actions;
	if (!actions.Contains(talents_actions::EAction::kUnaffect)) {
		return out;
	}
	const auto &unaffect = actions.GetUnaffect();
	for (const auto *set : {&unaffect.GetBreaking(), &unaffect.GetRemove()}) {
		for (const auto affect_type : set->any_of.chars) {
			if (affect_type == incoming
					|| std::find(out.begin(), out.end(), affect_type) != out.end()) {
				continue;
			}
			out.push_back(affect_type);
		}
	}
	return out;
}

bool HasConflictingAffect(CharData *ch, EAffect incoming) {
	if (!ch) {
		return false;
	}
	for (const auto affect_type : ConflictingAffects(incoming)) {
		if (IsAffected(ch, affect_type)) {
			return true;
		}
	}
	return false;
}

void DisplaceConflictingAffects(CharData *ch, EAffect incoming, double potency) {
	if (!ch) {
		return;
	}
	for (const auto affect_type : ConflictingAffects(incoming)) {
		if (!IsAffected(ch, affect_type)) {
			continue;
		}
		// Тот же путь, которым вытесняет каст: kDispell, подавление вещевого источника (иначе узел
		// вернула бы сверка), снятие узлов, сообщение. Вытесняет волшебство надетого, поэтому
		// снимающим идёт сам носитель.
		RemoveAffectAndAnnounce(ch, ch, affect_type, potency, /*displaced=*/true);
	}
}

// The standard d100 dispel contest against a potency supplied DIRECTLY (obj-affect suppressions are
// not on a char's ->affected). Same math as DispelSucceeds but no per-affect dispel_mod and no buff
// auto-pass. On failure, erodes `target_potency` in place by `decay`% of the caster's dispel strength.
bool DispelContest(float &target_potency, double competence, int dispel_bonus, int decay,
				   CharData *trace_ch, ESpell trace_spell, EAffect trace_affect) {
	const double raw = kDispelSkillWeight * (competence - target_potency) + dispel_bonus;
	const int threshold = std::clamp(static_cast<int>(std::lround(raw)), 5, 95);
	const int roll = number(1, 100);
	const bool ok = roll <= threshold;
	// Tester/immortal debug line, same shape as the DispelSucceeds "Unaffect:" line.
	if (trace_ch) {
		spell_trace::Line(trace_ch, nullptr,
				"Weave restore: %s [C: %.1f]. Suppressed: %s [p: %.1f]. threshold %d%%. %s (roll %d).\r\n",
				MUD::Spell(trace_spell).GetCName(), competence,
				affects::AffectMsg(trace_affect, affects::EAffectMsgType::kShortDesc).c_str(),
				static_cast<double>(target_potency), threshold, ok ? "Success" : "Fail", roll);
	}
	if (!ok) {
		ApplyDispelDecay(target_potency, static_cast<float>(kDispelSkillWeight * competence), decay);
	}
	return ok;
}

// Data-driven dispel stage: a CharData target strips the victim's affects, a RoomData target the
// room's. If both are set, the char path wins.
EStageResult CastUnaffects(ActionContext &ctx) {
	CharData *const ch = ctx.caster();
	CharData *const victim = ctx.cvict;
	RoomData *const room = ctx.rvict;
	const ESpell spell_id = ctx.spell_id();
	if (victim == nullptr && room == nullptr) {
		return EStageResult::kSuccess;
	}

	if (!ctx.action_or_default().Contains(talents_actions::EAction::kUnaffect)) {
		return EStageResult::kSuccess;
	}
	const auto &unaffect = ctx.action_or_default().GetUnaffect();
	const int unaff_prob = unaffect.GetProb();
	if (unaff_prob < 100 && number(1, 100) > unaff_prob) {
		return EStageResult::kSuccess;
	}

	double removed = 0.0;
	const EStageResult r = (victim != nullptr)
			? RunCastUnaffects(ch, victim, spell_id, unaffect, ctx.area_coeff, removed, ctx.CompetenceBase())
			: RunCastUnaffects(ch, room, spell_id, unaffect, ctx.area_coeff, removed, ctx.CompetenceBase());
	ctx.dispelled_potency += removed;
	return r;
}

// Dispels dispellable affects on the passage `dir`, both sides (`развеять магию <dir>`). Same access
// model as the room dispel: the author or a live ally strips for free; an outsider must out-potency it
// (5% lucky override); a player vs a live author commits a PK act. Validation and the overall no-effect
// line belong to the caller (CallMagicToExit). Returns true if anything was dispelled.
bool room_spells::DispelExitAffects(CharData *caster, int dir, ESpell spell_id) {
	if (!caster || caster->in_room == kNowhere || dir < 0 || dir >= EDirection::kMaxDirNum) {
		return false;
	}
	const auto near_exit = world[caster->in_room]->dir_option[dir];
	if (!near_exit) {
		return false;
	}
	std::vector<RoomData::exit_data_ptr> hosts{near_exit};
	if (near_exit->to_room() != kNowhere) {
		static const int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth,
									   EDirection::kEast, EDirection::kDown, EDirection::kUp};
		const auto rev = world[near_exit->to_room()]->dir_option[rev_dir[dir]];
		if (rev && rev->to_room() == caster->in_room) { hosts.push_back(rev); }
	}
	ActionContext ctx = BuildActionContext(caster, spell_id, GetRealLevel(caster));
	bool any = false;
	for (const auto &ex : hosts) {
		for (auto it = ex->affected.begin(); it != ex->affected.end();) {
			const auto af = *it;
			if (!af || !IS_SET(af->battleflag, kAfDispellable)) {
				++it;
				continue;
			}
			const float spell_potency = static_cast<float>(ctx.CompetenceBase());
			const auto access = room_spells::ClassifyRoomAffectAccess(caster, af->caster_id);
			bool ok;
			if (access.free) {
				ok = true;
			} else if (!caster->IsNpc() && access.author && !pk_agro_action(caster, access.author)) {
				ok = false;   // player attacking a live author's ward = a PK act they declined
			} else {
				ok = (number(1, 100) <= 5) || (spell_potency > af->potency);
			}
			if (!ok) {
				++it;
				continue;
			}
			const auto type = af->affect_type;
			const float sting_potency = af->potency;
			const auto &to_char = room_spells::RoomAffectMsgRaw(type, room_spells::ERoomAffectMsgType::kAffDispelledToChar);
			if (!to_char.empty()) { act(to_char.c_str(), false, caster, nullptr, nullptr, kToChar); }
			const auto &to_room = room_spells::RoomAffectMsgRaw(type, room_spells::ERoomAffectMsgType::kAffDispelledToRoom);
			if (!to_room.empty()) { act(to_room.c_str(), true, caster, nullptr, nullptr, kToRoom | kToArenaListen); }
				// Fire the affect's kDispell actions (e.g. the fire trap's sting) on the dispeller before erasing,
				// with the dispeller as caster so kTarFightSelf lands on them (no PvP).
			room_spells::RunRoomAffectTrigger(world[caster->in_room], caster, type,
					talents_actions::EActionTrigger::kDispell, sting_potency);
			it = ex->affected.erase(it);
			any = true;
		}
	}
	return any;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
