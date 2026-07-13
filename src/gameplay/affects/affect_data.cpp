#include "affect_data.h"
#include "gameplay/affects/affect_messages.h"
#include "gameplay/abilities/talents_actions.h"   // issue.character-affect-triggers: kExpired trigger
#include <set>   // issue.character-affect-triggers: per-round dedup of multi-instance affects
#include "administration/privilege.h"
#include "gameplay/affects/affect_handler.h"
#include "gameplay/mechanics/condition.h"
#include "gameplay/mechanics/follow.h"
#include "gameplay/mechanics/mount.h"
#include "engine/observability/event_sink.h"
#include "gameplay/affects/mobile_affect_update_profiler.h"
#include "gameplay/affects/player_affect_update_profiler.h"
#include "engine/entities/char_player.h"
#include "engine/db/world_characters.h"
#include "engine/db/db.h"          // world, top_of_world (zone-wake materialization)
#include "engine/entities/zone.h"  // zone_table
#include "gameplay/fight/fight_hit.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/mechanics/deathtrap.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/poison.h"
#include "gameplay/skills/death_rage.h"
#include "engine/db/global_objects.h"
#include "engine/core/char_equip_flags.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/mechanics/illumination.h"
#include "gameplay/mechanics/inventory.h"
#include "utils/utils_time.h"
#include "gameplay/core/genchar.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/mechanics/glory_const.h"
#include "engine/ui/cmd/do_equip.h"
#include "gameplay/core/base_stats.h"
#include "gameplay/abilities/abilities_constants.h"   // issue.duration-scale: kNoviceSkillThreshold
#include "gameplay/fight/fight.h"
#include "utils/backtrace.h"

#include <chrono>
#include "gameplay/core/remort.h"

std::unordered_set<CharData *> affected_mobs;

namespace {

// Эмиссия 'affect_added'/'affect_removed' для replay-режима симулятора
// баланса (issue #2967). В проде список sink'ов пуст -- ранний выход до
// построения Event.
void EmitAffectEvent(const char *kind, const CharData *ch,
		const Affect<EApply> &af) {
	if (!observability::HasAnyEventSink()) return;
	observability::Event ev;
	ev.name = kind;
	ev.ts_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
	ev.attrs["target_name"] = observability::EngineStringToUtf8(
		GET_NAME(ch) ? GET_NAME(ch) : "");
	ev.attrs["duration"] = static_cast<std::int64_t>(af.duration);
	ev.attrs["modifier"] = static_cast<std::int64_t>(af.modifier);
	ev.attrs["location"] = static_cast<std::int64_t>(af.location);
	ev.attrs["affect_type"] = static_cast<std::int64_t>(af.affect_type);
	observability::EmitToAllSinks(ev);
}

// issue.affect-migration: an affect announces its natural expiry if it has an IDENTITY -- its
// affect_type. (Affect::type is gone; affect_type is the sole identity.)
[[nodiscard]] bool AffectHasIdentity(const Affect<EApply>::shared_ptr &af) {
	return af->affect_type != EAffect::kUndefined;
}
}  // namespace

// "Same affect" for the multi-slot dedup (one spell -> several slots announces once): same affect_type,
// or same legacy type for affects that still lack an affect_type. Exposed (declared in affect_data.h)
// for callers that dedup or look up affects by their identity rather than by the casting spell
// (remove_random_affects, the do_affects display).
bool SameAffectIdentity(const Affect<EApply>::shared_ptr &a, const Affect<EApply>::shared_ptr &b) {
	return a->affect_type == b->affect_type;
}

int apply_ac(CharData *ch, int eq_pos);
int apply_armour(CharData *ch, int eq_pos);
void apply_natural_affects(CharData *ch);
extern std::array<EAffect, 2> char_saved_aff;
extern std::array<EAffect, 3> char_stealth_aff;

bool no_bad_affects(ObjData *obj) {
	static std::list<EWeaponAffect> bad_waffects =
		{
			EWeaponAffect::kHold,
			EWeaponAffect::kSanctuary,
			EWeaponAffect::kPrismaticAura,
			EWeaponAffect::kPoison,
			EWeaponAffect::kSilence,
			EWeaponAffect::kDeafness,
			EWeaponAffect::kHaemorrhage,
			EWeaponAffect::kBlindness,
			EWeaponAffect::kSleep,
			EWeaponAffect::kHolyDark
		};
	for (const auto wa : bad_waffects) {
		if (OBJ_AFFECT(obj, wa)) {
			return false;
		}
	}
	return true;
}

// Return the effect of a piece of armor in position eq_pos
int apply_ac(CharData *ch, int eq_pos) {
	int factor = 1;

	if (GET_EQ(ch, eq_pos) == nullptr) {
		log("SYSERR: apply_ac(%s,%d) when no equip...", GET_NAME(ch), eq_pos);
		// core_dump();
		return (0);
	}

	if (!ObjSystem::is_armor_type(GET_EQ(ch, eq_pos))) {
		return (0);
	}

	switch (eq_pos) {
		case EEquipPos::kBody:factor = 3;
			break;        // 30% //
		case EEquipPos::kHead:
		case EEquipPos::kLegs:factor = 2;
			break;        // 20% //
		default:factor = 1;
			break;        // all others 10% //
	}

	if (ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed))
		factor *= kMobAcMult;

	return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
}

int apply_armour(CharData *ch, int eq_pos) {
	int factor = 1;
	ObjData *obj = GET_EQ(ch, eq_pos);

	if (!obj) {
		log("SYSERR: apply_armor(%s,%d) when no equip...", GET_NAME(ch), eq_pos);
		// core_dump();
		return (0);
	}

	if (!ObjSystem::is_armor_type(obj))
		return (0);

	switch (eq_pos) {
		case EEquipPos::kBody: factor = 3;
			break;        // 30% //
		case EEquipPos::kHead:
		case EEquipPos::kLegs: factor = 2;
			break;        // 20% //
		default: factor = 1;
			break;        // all others 10% //
	}

	if (ch->IsNpc() && !AFF_FLAGGED(ch, EAffect::kCharmed))
		factor *= kMobArmourMult;

	// чтобы не плюсовать левую броню на стафе с текущей прочностью выше максимальной
	int cur_dur = MIN(obj->get_maximum_durability(), obj->get_current_durability());
	return (factor * GET_OBJ_VAL(obj, 1) * cur_dur / MAX(1, obj->get_maximum_durability()));
}

///
/// Сет чару аффектов, которые должны висеть постоянно (через affect_total)
///
// Была ошибка, у нубов реген хитов был всегда 50, хотя с 26 по 30, должен быть 60.
// Теперь аффект регенерация новичка держится 3 реморта, с каждыи ремортом все слабее и слабее
void apply_natural_affects(CharData *ch) {
	if (remort::GetRealRemort(ch) <= 3 && !privilege::IsImmortal(ch)) {
		affect_modify(ch, EApply::kHpRegen, 60 - (remort::GetRealRemort(ch) * 10), EAffect::kNoobRegen, true);
		affect_modify(ch, EApply::kMoveRegen, 100, EAffect::kNoobRegen, true);
		affect_modify(ch, EApply::kManaRegen, 100 - (remort::GetRealRemort(ch) * 20), EAffect::kNoobRegen, true);
	}
}

std::array<EAffect, 2> char_saved_aff =
	{
		EAffect::kGroup,
		EAffect::kHorse
	};

std::array<EAffect, 3> char_stealth_aff =
	{
		EAffect::kHide,
		EAffect::kSneak,
		EAffect::kDisguise
	};

template<>
bool Affect<EApply>::removable() const {
	// issue.affect-migration: curability is the kAfCurable battleflag (single source of truth),
	// replacing the hardcoded ESpell list.
	return IS_SET(battleflag, kAfCurable);
}
// 
// для мобов раз в 10 пульсов
void UpdateAffectOnPulse(CharData *ch, int count) {
	bool pulse_aff = false;

	if (ch->GetEnemy()) {
		return;
	}

	if (ch->affected.empty()) {
		return;
	}

	auto affect_i = ch->affected.begin();
	while (affect_i != ch->affected.end()) {
		const auto &affect = *affect_i;

		if (!IS_SET(affect->battleflag, kAfPulsedec)) {
			++affect_i;
			continue;
		}
		pulse_aff = true;

		if (affect->duration == 0) { //-1 вечный таймер по задумке
			// issue.character-affect-triggers: kExpired -- timer ran out; fire the affect's kExpired
			// <actions> on the bearer before the strip (natural end -> no dispeller actor).
			RunCharAffectTrigger(ch, affect->affect_type, talents_actions::EActionTrigger::kExpired);
			affect_i = RemoveAffect(ch, affect_i);
		} else {
			if (affect->duration > 0) {
				affect->duration -= count;
				affect->duration = std::max(0, affect->duration);
			}
			++affect_i;
		}
	}
	if (pulse_aff) {
		affect_total(ch);
	}
}

void player_timed_update() {
	for (auto d = descriptor_list; d; d = d->next) {
		if (d->state != EConState::kPlaying)
			continue;
		const auto ch = d->get_character().get();

		for (auto timed = ch->timed_skill.begin(); timed != ch->timed_skill.end();) {
/*			if (timed->first == ESkill::kTownportal) {
				mudlog(fmt::format("таймер == {} time0 {} time {}" , (timed->second - time(0) - 1) / 60 + 1, time(0), timed->second));
			}
*/
			if (time(0) >= timed->second) {
				timed = ch->timed_skill.erase(timed);
			} else {
				++timed;
			}
		}
		for (auto timed = ch->timed_feat.begin(); timed != ch->timed_feat.end();) {
			if (timed->second >= 1) {
				timed->second--;
				++timed;
			} else {
				timed = ch->timed_feat.erase(timed);
			}
		}
	}
}

// игроки раз в 2 секунды
void player_affect_update() {
	using player_affect_update_profiler::Counter;
	using player_affect_update_profiler::RunStats;
	using player_affect_update_profiler::Section;

	RunStats profile;
	utils::CExecutionTimer total_timer;
	for (auto d = descriptor_list; d; d = d->next) {
		if (d->state != EConState::kPlaying)
			continue;

		utils::CExecutionTimer descriptor_timer;
		const auto i = d->get_character();
		++profile.counters[static_cast<std::size_t>(Counter::kPlayers)];
				
		// на всякий случай проверка на пурж, в целом пурж чармисов внутри
		// такого цикла сейчас выглядит безопасным, чармисы если и есть, то они
		// добавлялись в чар-лист в начало списка и идут до самого чара

		{
			utils::CExecutionTimer tunnel_timer;
			if (i->purged() || deathtrap::tunnel_damage(i.get())) {
				profile.sections[static_cast<std::size_t>(Section::kTunnelDamage)] += tunnel_timer.delta().count();
				++profile.counters[static_cast<std::size_t>(Counter::kPurgedPlayers)];
				profile.sections[static_cast<std::size_t>(Section::kDescriptorLoop)] += descriptor_timer.delta().count();
				profile.sections[static_cast<std::size_t>(Section::kTotal)] = total_timer.delta().count();
				player_affect_update_profiler::record_run(profile);
				return;
			}
			profile.sections[static_cast<std::size_t>(Section::kTunnelDamage)] += tunnel_timer.delta().count();
		}

		if (!i->affected.empty()) {
			++profile.counters[static_cast<std::size_t>(Counter::kAffectedPlayers)];
		}
		bool was_purged = false;
		bool need_recalc = false;
		// issue.damage-over-time: an out-of-combat data-driven DoT (kPulse actions) ticks once per affect
		// type per pass (multi-instance affects share one tick, like poison in battle_affect_update).
		std::set<EAffect> ticked_types;
		// issue.drunked-migration (Gap B): kExpired likewise fires at most once per affect TYPE per pass.
		std::set<EAffect> expired_types;
		auto affect_i = i->affected.begin();

		while (affect_i != i->affected.end()) {
			utils::CExecutionTimer affect_timer;
			++profile.counters[static_cast<std::size_t>(Counter::kAffectEntries)];
			const auto &affect = *affect_i;

			// нечего тикать аффектам вне раунда боя
			if (IS_SET(affect->battleflag, kAfBattledec) && i->GetEnemy()) {
				++profile.counters[static_cast<std::size_t>(Counter::kSkippedBattleDecAffects)];
				++affect_i;
				profile.sections[static_cast<std::size_t>(Section::kAffectLoop)] += affect_timer.delta().count();
				continue;
			}
			if (affect->duration == 0) {
				if (AffectHasIdentity(affect)) {
					auto next_affect_i = affect_i;

					++next_affect_i;
					if (next_affect_i == i->affected.end()	//костыль на спадение 1 закла накладывающего несколько аффектов
							|| !SameAffectIdentity(affect, *next_affect_i)
							|| (*next_affect_i)->duration > 0) {
						//чтобы не выдавалось, "что теперь вы можете сражаться",
						//хотя на самом деле не можете :)
						// issue.affect-migration: suppress the "you can fight again" line while the OTHER stun
						// still holds; keyed on the stun affect_type now (ability-to-act), not the kBattle marker.
						if (!(affect->affect_type == EAffect::kMagicStopFight
								&& AFF_FLAGGED(i, EAffect::kStopFight))) {
							if (!(affect->affect_type == EAffect::kStopFight
									&& AFF_FLAGGED(i, EAffect::kMagicStopFight))) {
								ShowAffExpiredMsg(affect->affect_type, i.get());
							}
						}
					}
				}
				// issue.character-affect-triggers: kExpired (see UpdateAffectOnPulse) -- natural timeout.
				// issue.drunked-migration (Gap B): once per affect TYPE per pass, so a multi-instance affect
				// (e.g. the 3-apply kDrunked) runs its on-expire action a single time. kDrunked's hangover is
				// now data: its <trigger val="kExpired"> action imposes kAbstinent (was hardcoded set_abstinent).
				if (expired_types.insert(affect->affect_type).second) {
					RunCharAffectTrigger(i.get(), affect->affect_type, talents_actions::EActionTrigger::kExpired);
				}
				affect_i = RemoveAffect(i.get(), affect_i);
				need_recalc = true;
			} else {
				if (affect->duration > 0) {
					// issue.damage-over-time: out of combat, a data-driven DoT (has <actions>) ticks its
					// kPulse actions here -- the out-of-combat counterpart of battle_affect_update, so a
					// burn/DoT keeps damaging even when the bearer is not fighting (once per affect type). In
					// combat this is skipped (guarded by !GetEnemy); battle_affect_update ticks it per round.
					// Poison/aconite are DoTs with <actions> now, so the old hardcoded ProcessPoisonDmg path
					// is gone; a kAfSameTime affect WITHOUT actions is a pure debuff and simply ticks nothing.
					if (IS_SET(affect->battleflag, kAfSameTime) && !i->GetEnemy()
							&& !affects::AffectActions(affect->affect_type).list().empty()
							&& ticked_types.insert(affect->affect_type).second) {
						RunCharAffectTick(i.get(), affect);
						if (i->purged()) {
							was_purged = true;
							++profile.counters[static_cast<std::size_t>(Counter::kPurgedPlayers)];
							break;
						}
					}
//					sprintf(buf, "ЧАР: Спелл %s висит на %s длительносить %d\r\n", MUD::Spell(affect->type).GetCName(), GET_PAD(i, 5), affect->duration);
//					mudlog(buf, CMP, kLvlImmortal, SYSLOG, true);
					if (ROOM_FLAGGED(i->in_room, ERoomFlag::kDominationArena)) {
						utils::CExecutionTimer domination_timer;
						++profile.counters[static_cast<std::size_t>(Counter::kDominationAffects)];
						if (IS_SET(affect->battleflag, kAfCurable)) {
							affect->duration -= 15;
						}
						if (IS_SET(affect->battleflag, kAfPulsedec))
							affect->duration -= MIN(affect->duration, kSecsPerPlayerAffect * kPassesPerSec);
						else
							affect->duration--;
						profile.sections[static_cast<std::size_t>(Section::kDominationAdjust)] += domination_timer.delta().count();
					} else {
						if (IS_SET(affect->battleflag, kAfPulsedec))
							affect->duration -= MIN(affect->duration, kSecsPerPlayerAffect * kPassesPerSec);
						else
							affect->duration--;
					}
					affect->duration = std::max(0, affect->duration);
				}
				++affect_i;
			}
			profile.sections[static_cast<std::size_t>(Section::kAffectLoop)] += affect_timer.delta().count();
		}
		if (!was_purged && need_recalc) {
			{
				utils::CExecutionTimer memq_timer;
				MemQ_slots(i.get());    // сколько каких слотов занято (с коррекцией)
				profile.sections[static_cast<std::size_t>(Section::kMemQSlots)] += memq_timer.delta().count();
			}
			{
				utils::CExecutionTimer recalc_timer;
				affect_total(i.get());
				profile.sections[static_cast<std::size_t>(Section::kRecalc)] += recalc_timer.delta().count();
			}
			++profile.counters[static_cast<std::size_t>(Counter::kRecalcPlayers)];
		}
		profile.sections[static_cast<std::size_t>(Section::kDescriptorLoop)] += descriptor_timer.delta().count();
	}
	profile.sections[static_cast<std::size_t>(Section::kTotal)] = total_timer.delta().count();
	player_affect_update_profiler::record_run(profile);
}

// This file update battle affects only
void battle_affect_update(CharData *ch) {
	bool need_recalc = false;
	// issue.character-affect-triggers: a data-driven combat-DoT affect ticks once per affect TYPE per
	// round, even if it has several stacked applies (e.g. poison's kPoison + kStr) -- mirrors how the
	// hardcoded ProcessPoisonDmg damages once (its location gate).
	std::set<EAffect> ticked_types;
	if (ch->purged()) {
		char tmpbuf[256];
		sprintf(tmpbuf,"WARNING: battle_affect_update ch purged. Name %s vnum %d", GET_NAME(ch), GET_MOB_VNUM(ch));
		mudlog(tmpbuf, LGH, kLvlImmortal, SYSLOG, true);
		return;
	}
	if (ch->affected.empty()) {
		return;
	}
	auto affect_i = ch->affected.begin();
	while (affect_i != ch->affected.end()) {
		const auto &affect = *affect_i;

		if (!IS_SET((*affect_i)->battleflag, kAfBattledec) && !IS_SET((*affect_i)->battleflag, kAfSameTime)) {
			++affect_i;
			continue;
		}
		// issue.character-affect-triggers: legacy NPC poison is still deferred to the slow mob loop, BUT a
		// poison affect migrated to the data-driven <actions> mechanism is NOT skipped here -- it ticks per
		// combat round like a player's (fixing the "mob combat DoT is too slow" problem).
		if (ch->IsNpc() && (*affect_i)->location == EApply::kPoison
				&& affects::AffectActions((*affect_i)->affect_type).list().empty()) {
			++affect_i;
			continue;
		}
		if (affect->duration == 0) {
			if (AffectHasIdentity(affect)) {
				auto next_affect_i = affect_i;

				++next_affect_i;
				if (next_affect_i == ch->affected.end()
						|| !SameAffectIdentity(affect, *next_affect_i)
						|| (*next_affect_i)->duration > 0) {
					ShowAffExpiredMsg(affect->affect_type, ch);
				}
			}
			// issue.character-affect-triggers: kExpired (see UpdateAffectOnPulse) -- natural timeout in combat.
			RunCharAffectTrigger(ch, affect->affect_type, talents_actions::EActionTrigger::kExpired);
			affect_i = RemoveAffect(ch, affect_i);
			need_recalc = true;
		} else {
			if (affect->duration > 0) {
				// issue.character-affect-triggers/damage-over-time: data-driven combat DoT -- run the
				// affect's pulse/battle-pulse <actions> on the bearer (once per type this round). A
				// kAfSameTime affect WITHOUT actions is a pure debuff and ticks nothing; poison/aconite are
				// data-driven DoTs now, so the hardcoded ProcessPoisonDmg path is retired.
				if (IS_SET(affect->battleflag, kAfSameTime)
						&& !affects::AffectActions(affect->affect_type).list().empty()) {
					if (ticked_types.insert(affect->affect_type).second) {
						RunCharAffectTick(ch, affect);
					}
					if (ch->purged()) {
						// issue #3544: the bearer can be killed/purged by its own affect tick this round --
						// that is normal, so just stop (the implementor-log message here was pure spam).
						return;
					}
				}
				if (ch->IsNpc())
					--affect->duration;
				else {
					affect->duration -= std::min(affect->duration, kSecsPerMudHour / kSecsPerPlayerAffect);
				}
				affect->duration = std::max(0, affect->duration);
			}
			++affect_i;
		}
	}
	if (need_recalc) {
		affect_total(ch);
	}
}

// раз в минуту
void mobile_affect_update() {
	using mobile_affect_update_profiler::Counter;
	using mobile_affect_update_profiler::RunStats;
	using mobile_affect_update_profiler::Section;

	RunStats profile;
	utils::CExecutionTimer total_timer;

	utils::CExecutionTimer copy_timer;
	auto copy = affected_mobs;
	profile.sections[static_cast<std::size_t>(Section::kCopyAffectedMobs)] += copy_timer.delta().count();

	for (auto it = copy.begin(); it != copy.end(); ++it) {
		utils::CExecutionTimer mob_timer;
		const auto &ch = *it;
		int was_charmed = false;
		bool was_purged = false;
		bool need_recalc = false;
		// issue.damage-over-time: out-of-combat data-driven DoT ticks once per affect type per pass.
		std::set<EAffect> ticked_types;
		++profile.counters[static_cast<std::size_t>(Counter::kMobs)];
//		if (!ch->in_used_zone()) {
//			return;
//		}

		auto affect_i = ch->affected.begin();

		if (ch->affected.empty()) {
			++profile.counters[static_cast<std::size_t>(Counter::kEmptyMobs)];
			log(fmt::format("ERROR!!! Проверка счетчика аффектов у очищенного моба {} #{}", ch->get_name(), GET_MOB_VNUM(ch)));
			auto it_erase = affected_mobs.find(ch);
			if (it_erase != affected_mobs.end()) {
				affected_mobs.erase(it_erase);
				profile.sections[static_cast<std::size_t>(Section::kMobLoop)] += mob_timer.delta().count();
				continue;
			}
		}

		while (affect_i != ch->affected.end()) {
			utils::CExecutionTimer affect_timer;
			++profile.counters[static_cast<std::size_t>(Counter::kAffectEntries)];
			const auto &affect = *affect_i;

			if (affect->duration == 0) {
				if (AffectHasIdentity(affect)) {
					if (IS_SET(affect->battleflag, kAfCharmBond)) {
						was_charmed = true;
					}
					auto next_affect_i = affect_i;
					++next_affect_i;
					if (next_affect_i == ch->affected.end()
							|| !SameAffectIdentity(affect, *next_affect_i)
							|| (*next_affect_i)->duration > 0) {
						ShowAffExpiredMsg(affect->affect_type, ch);
					}
				}
				// issue.character-affect-triggers: kExpired (see UpdateAffectOnPulse) -- natural timeout (mob).
				RunCharAffectTrigger(ch, affect->affect_type, talents_actions::EActionTrigger::kExpired);
				affect_i = RemoveAffect(ch, affect_i);
				need_recalc = true;
			} else {
				if (affect->duration > 0) {
					// issue.character-affect-triggers/damage-over-time: a DoT with <actions> ticks per combat
					// round in battle_affect_update while fighting; here (the slow mob loop) it ticks only
					// OUT of combat -- so a burn keeps damaging a non-fighting mob without double-hitting in
					// combat. A kAfSameTime affect without actions is a pure debuff and ticks nothing;
					// poison/aconite are data-driven DoTs now, so ProcessPoisonDmg is retired.
					if (IS_SET(affect->battleflag, kAfSameTime)
							&& !affects::AffectActions(affect->affect_type).list().empty()) {
						if (!ch->GetEnemy() && ticked_types.insert(affect->affect_type).second) {
							RunCharAffectTick(ch, affect);
							if (ch->purged()) {
								was_purged = true;
								++profile.counters[static_cast<std::size_t>(Counter::kPurgedMobs)];
								break;
							}
						}
					}
					affect->duration--;
					if (affect->duration == 0) {
						const std::string &expire_soon =
								affects::AffectMsgRaw(affect->affect_type, affects::EAffectMsgType::kAffExpireSoon);
						if (!expire_soon.empty()) {
							act(expire_soon.c_str(), false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
						}
					}
				}
				++affect_i;
			}
			profile.sections[static_cast<std::size_t>(Section::kAffectLoop)] += affect_timer.delta().count();
		}

		if (!was_purged) {
			if (need_recalc) {
				utils::CExecutionTimer recalc_timer;
				affect_total(ch);
				profile.sections[static_cast<std::size_t>(Section::kRecalc)] += recalc_timer.delta().count();
				++profile.counters[static_cast<std::size_t>(Counter::kRecalcMobs)];
			}
			{
				utils::CExecutionTimer timed_skill_timer;
				for (auto timed = ch->timed_skill.begin(); timed != ch->timed_skill.end();) {
					++profile.counters[static_cast<std::size_t>(Counter::kTimedSkillEntries)];
					if (timed->second >= 1) {
						timed->second--;
						++timed;
					} else {
						timed = ch->timed_skill.erase(timed);
					}
				}
				profile.sections[static_cast<std::size_t>(Section::kTimedSkill)] += timed_skill_timer.delta().count();
			}
			{
				utils::CExecutionTimer timed_feat_timer;
				for (auto timed = ch->timed_feat.begin(); timed != ch->timed_feat.end();) {
					++profile.counters[static_cast<std::size_t>(Counter::kTimedFeatEntries)];
					if (timed->second >= 1) {
						timed->second--;
						++timed;
					} else {
						timed = ch->timed_feat.erase(timed);
					}
				}
				profile.sections[static_cast<std::size_t>(Section::kTimedFeat)] += timed_feat_timer.delta().count();
			}
			{
				utils::CExecutionTimer deathtrap_timer;
				++profile.counters[static_cast<std::size_t>(Counter::kDeathTrapChecks)];
				if (deathtrap::check_death_trap(ch)) {
					profile.sections[static_cast<std::size_t>(Section::kDeathTrap)] += deathtrap_timer.delta().count();
					profile.sections[static_cast<std::size_t>(Section::kMobLoop)] += mob_timer.delta().count();
					profile.sections[static_cast<std::size_t>(Section::kTotal)] = total_timer.delta().count();
					mobile_affect_update_profiler::record_run(profile);
					return;
				}
				profile.sections[static_cast<std::size_t>(Section::kDeathTrap)] += deathtrap_timer.delta().count();
			}
			if (was_charmed) {
				utils::CExecutionTimer stop_follower_timer;
				follow::StopFollower(ch, follow::kSfCharmlost);
				profile.sections[static_cast<std::size_t>(Section::kStopFollower)] += stop_follower_timer.delta().count();
				++profile.counters[static_cast<std::size_t>(Counter::kCharmStops)];
			}
		}

		// issue.mob-flag-affect-materialization: drop the mob from affected_mobs once none of its
		// surviving affects need per-tick processing (all permanent -1, or none left) -- so a mob left
		// with only materialized buffs after a timed affect expired stops being scanned. The -1 affects
		// remain on ch->affected (the damage system reads that), and a future timed affect re-registers it.
		bool needs_update = false;
		for (const auto &a : ch->affected) {
			if (a && a->duration != -1) {
				needs_update = true;
				break;
			}
		}
		if (!needs_update) {
			auto it_erase = affected_mobs.find(ch);
			if (it_erase != affected_mobs.end()) {
				affected_mobs.erase(it_erase);
			}
		}

		profile.sections[static_cast<std::size_t>(Section::kMobLoop)] += mob_timer.delta().count();
	}

	profile.sections[static_cast<std::size_t>(Section::kTotal)] = total_timer.delta().count();
	mobile_affect_update_profiler::record_run(profile);
}

// Affect-keyed counterparts (issue.affect-migration): remove every affect with a given
// affect_type. Removes affect *structs* only; innate flags (mob proto / innate abilities)
// are reapplied by affect_total and are not touched here.
void RemoveAffectFromChar(CharData *ch, EAffect affect_type) {
	if (affect_type == EAffect::kUndefined) {
		return;
	}
	auto it = ch->affected.begin();
	while (it != ch->affected.end()) {
		Affect<EApply>::shared_ptr affect = *it;
		if (affect->affect_type == affect_type) {
			EmitAffectEvent("affect_removed", ch, *affect);
			it = RemoveAffect(ch, it);
		} else {
			++it;
		}
	}
}

void RemoveAffectFromCharAndRecalculate(CharData *ch, EAffect affect_type) {
	RemoveAffectFromChar(ch, affect_type);
	affect_total(ch);
}

// issue.affect-migration: break the charm "package". Removes every affect of the package (bond + the
// companion buffs that share the per-instance kAfCharmBond flag) and, for an NPC, schedules its
// extraction and clears the hire price -- the behavior the old RemoveAffectFromChar(ESpell::kCharm)
// carried. Replaces that call for un-charming.
void RemoveCharmBond(CharData *ch, bool recalculate) {
	auto it = ch->affected.begin();
	while (it != ch->affected.end()) {
		if (IS_SET((*it)->battleflag, kAfCharmBond)) {
			EmitAffectEvent("affect_removed", ch, **it);
			it = RemoveAffect(ch, it);
		} else {
			++it;
		}
	}
	if (ch->IsNpc()) {
		ch->extract_timer = 5;
		ch->mob_specials.hire_price = 0;
	}
	if (recalculate) {
		affect_total(ch);
	}
}

// issue.affect-migration: remove every curable affect (kAfCurable). Replaces the old
// GetRemovableSpellId sweep.
void RemoveCurableAffects(CharData *ch) {
	auto it = ch->affected.begin();
	while (it != ch->affected.end()) {
		const auto af = *it;
		if (IS_SET(af->battleflag, kAfCurable)) {
			EmitAffectEvent("affect_removed", ch, *af);
			it = RemoveAffect(ch, it);
		} else {
			++it;
		}
	}
}

std::pair<EApply, int>  GetApplyByWeaponAffect(EWeaponAffect element, CharData *ch) {
	int value;
	if (ch) //чтоб не было варнинга, ch передаю на будущее
		value = 2;
	switch (element) {
		case EWeaponAffect::kFireAura:
			return std::pair<EApply, int>(EApply::kResistFire, value);
			break;
		case EWeaponAffect::kAirAura:
			return std::pair<EApply, int>(EApply::kResistAir, value);
			break;
		case EWeaponAffect::kIceAura:
			return std::pair<EApply, int>(EApply::kResistWater, value);
			break;
		case EWeaponAffect::kEarthAura:
			return std::pair<EApply, int>(EApply::kResistEarth, value);
			break;
		case EWeaponAffect::kProtectFromDark:
			return std::pair<EApply, int>(EApply::kResistDark, value);
			break;
		case EWeaponAffect::kProtectFromMind:
			return std::pair<EApply, int>(EApply::kResistMind, value);
			break;
		// issue.mob-flag-affect-materialization: worn cloudly/blink must grant the miss-chance APPLY,
		// not just the flag -- ProcessBlink now gates on the apply, not AFF_FLAGGED. Flat 10 preserves
		// the pre-change PC value (the old hardcoded flag path defaulted a flagged PC to blink 10); NPC
		// bearers still take level+remort from ProcessBlink regardless of magnitude.
		case EWeaponAffect::kCloudly:
			return std::pair<EApply, int>(EApply::kSpelledBlinkMag, 10);
			break;
		case EWeaponAffect::kBlink:
			return std::pair<EApply, int>(EApply::kSpelledBlinkPhys, 10);
			break;
		default:
			return std::pair<EApply, int>(EApply::kNone, 0);
			break;
	}
}

// This updates a character by subtracting everything he is affected by
// restoring original abilities, and then affecting all again
void affect_total(CharData *ch) {
	if (ch->purged()) {
		return;
	}
	if (ch->in_room == kNowhere) {
		return;
	}
	bool domination = false;
/*
	if (!ch->IsNpc()) {
		mudlog("Enter in affect_total");
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
	}
*/
	if (!ch->IsNpc() && ch->in_room != kNowhere && ch->in_room >= 0
			&& static_cast<size_t>(ch->in_room) < world.size()
			&& ROOM_FLAGGED(ch->in_room, ERoomFlag::kDominationArena)) {
		domination = true;
	}
	ObjData *obj;

	BitsetFlags<EAffect> saved;

	// Init struct
	saved.clear();
	ch->clear_add_apply_affects();
	// PC's clear all affects, because recalc one
	{
		saved = ch->char_specials.saved.affected_by;
		if (ch->IsNpc()) {
			ch->char_specials.saved.affected_by = mob_proto[ch->get_rnum()].char_specials.saved.affected_by;
		} else {
			ch->char_specials.saved.affected_by.clear();
		}
		for (const auto &i : char_saved_aff) {
			if (saved.get(i)) {
				AFF_FLAGS(ch).set(i);
			}
		}
	}
	if (domination) {
		ch->set_remort_add(24 - ch->get_remort());
		ch->set_level_add(30 - ch->GetLevel());
		ch->set_str_add(ch->get_start_stat(G_STR) + 24 - ch->get_str());
		ch->set_dex_add(ch->get_start_stat(G_DEX) + 24 - ch->get_dex());
		ch->set_con_add(ch->get_start_stat(G_CON) + 24 - ch->get_con());
		ch->set_int_add(ch->get_start_stat(G_INT) + 24 - ch->get_int());
		ch->set_wis_add(ch->get_start_stat(G_WIS) + 24 - ch->get_wis());
		ch->set_cha_add(ch->get_start_stat(G_CHA) + 24 - ch->get_cha());
		double add_hp_per_level = MUD::Class(ch->GetClass()).applies.base_con
				+ (ClampBaseStat(ch, EBaseStat::kCon, GetRealCon(ch)) - MUD::Class(ch->GetClass()).applies.base_con)
				* MUD::Class(ch->GetClass()).applies.koef_con / 100.0 + 3;
		double hiton30lvl = 10 + add_hp_per_level * 30;
		ch->set_hit_add(hiton30lvl - ch->get_max_hit());
//		SendMsgToChar(ch, "max_hit: %d, add per level: %f, hitadd: %d, start_con: %d, level: %d\r\n", 
//			GET_REAL_MAX_HIT(ch), add_hp_per_level, GET_HIT_ADD(ch), ch->get_start_stat(G_CON), GetRealLevel(ch));
	}
	if (ch->IsNpc()) {
		(ch)->add_abils = (&mob_proto[ch->get_rnum()])->add_abils;
	}
	// move object modifiers
	for (int i = EEquipPos::kFirstEquipPos; i < EEquipPos::kNumEquipPos; i++) {
		if ((obj = GET_EQ(ch, i))) {
			if (ObjSystem::is_armor_type(obj)) {
				GET_AC_ADD(ch) -= apply_ac(ch, i);
				GET_ARMOUR(ch) += apply_armour(ch, i);
			}
			// Update weapon applies
			for (int j = 0; j < kMaxObjAffect; j++) {
				affect_modify(ch, GET_EQ(ch, i)->get_affected(j).location,
							  GET_EQ(ch, i)->get_affected(j).modifier, static_cast<EAffect>(0), true);
			}
			// Update weapon bitvectors
			for (const auto &j : weapon_affect) {
				// То же самое, но переформулировал
				if (j.aff_bitvector == 0 || !obj->GetEWeaponAffect(j.aff_pos)) {
					continue;
				}
				affect_modify(ch, GetApplyByWeaponAffect(j.aff_pos, ch).first, GetApplyByWeaponAffect(j.aff_pos, ch).second, static_cast<EAffect>(j.aff_bitvector), true);
			}
		}
	}
	ch->obj_bonus().apply_affects(ch);

	for (auto feat_id = EFeat::kFirst; feat_id <= EFeat::kLast; ++feat_id) {
		if (ch->HaveFeat(feat_id) && CanUseFeat(ch, feat_id)) {
			MUD::Feat(feat_id).effects.ImposeApplies(ch);
			MUD::Feat(feat_id).effects.ImposeSkillsMods(ch);
		}
	}

	if (!ch->IsNpc()) {
		if (!domination) // мы не на новой арене и не ПК
			GloryConst::apply_modifiers(ch);
		apply_natural_affects(ch);
	}

	// move affect modifiers
	for (const auto &af : ch->affected) {
		// Failed-attempt markers (kAfFailed) keep the success affect_type for identity/display,
		// but must NOT raise the affected_by flag bit -- otherwise a botched hide/berserk would
		// read as the real effect everywhere AFF_FLAGGED is checked. Apply the modifier (a no-op
		// for these markers: location kNone) without the flag.
		const EAffect bitv = IS_SET(af->battleflag, kAfFailed) ? EAffect::kUndefined : af->affect_type;
		affect_modify(ch, af->location, af->modifier, bitv, true);
	}

	// move race and class modifiers
	if (!ch->IsNpc()) {
		const unsigned wdex = PlayerSystem::weight_dex_penalty(ch);
		if (wdex != 0) {
			ch->set_dex_add(ch->get_dex_add() - wdex);
		}
			GET_DR_ADD(ch) += GetExtraDamroll(ch->GetClass(), GetRealLevel(ch));
		if (!AFF_FLAGGED(ch, EAffect::kNoobRegen)) {
			ch->set_hitreg(ch->get_hitreg() + (GetRealLevel(ch) + 4) / 5 * 10);
		}
		if (CanUseFeat(ch, EFeat::kRegenOfDarkness)) {
			ch->set_hitreg(ch->get_hitreg() + ch->get_hitreg() * 0.2);
		}
		if (GET_CON_ADD(ch)) {
			ch->set_hit_add(ch->get_hit_add() + PlayerSystem::con_add_hp(ch));
			int i = ch->get_real_max_hit();
			if (i < 1) {
				ch->set_hit_add(ch->get_hit_add() - (i - 1));
			}
		}
		if (!privilege::IsImmortal(ch) && mount::IsOnHorse(ch)) {
			AFF_FLAGS(ch).unset(EAffect::kHide);
			AFF_FLAGS(ch).unset(EAffect::kSneak);
			AFF_FLAGS(ch).unset(EAffect::kDisguise);
			AFF_FLAGS(ch).unset(EAffect::kInvisible);
		}
	}

	// correctize all weapon
	if (!privilege::IsImmortal(ch)) {
		if ((obj = GET_EQ(ch, EEquipPos::kBoths)) && !CanBeTakenInBothHands(ch, obj)) {
			if (!ch->IsNpc()) {
				act("Вам слишком тяжело держать $o3 в обоих руках!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_BOTH_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kBoths, CharEquipFlag::skip_total), ch);
		}
		if ((obj = GET_EQ(ch, EEquipPos::kWield)) && !CanBeTakenInMajorHand(ch, obj)) {
			if (!ch->IsNpc()) {
				act("Вам слишком тяжело держать $o3 в правой руке!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_WIELD_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kWield, CharEquipFlag::skip_total), ch);
			// если пушку можно вооружить в обе руки и эти руки свободны
			if (CAN_WEAR(obj, EWearFlag::kBoth)
				&& CanBeTakenInBothHands(ch, obj)
				&& !GET_EQ(ch, EEquipPos::kHold)
				&& !GET_EQ(ch, EEquipPos::kLight)
				&& !GET_EQ(ch, EEquipPos::kShield)
				&& !GET_EQ(ch, EEquipPos::kWield)
				&& !GET_EQ(ch, EEquipPos::kBoths)) {
				EquipObj(ch, obj, EEquipPos::kBoths, CharEquipFlag::skip_total);
			}
		}
		if ((obj = GET_EQ(ch, EEquipPos::kHold)) && !CanBeTakenInMinorHand(ch, obj)) {
			if (!ch->IsNpc()) {
				act("Вам слишком тяжело держать $o3 в левой руке!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_HOLD_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kHold, CharEquipFlag::skip_total), ch);
		}
		if ((obj = GET_EQ(ch, EEquipPos::kShield)) && !CanBeWearedAsShield(ch, obj)) {
			if (!ch->IsNpc()) {
				act("Вам слишком тяжело держать $o3 на левой руке!", false, ch, obj, nullptr, kToChar);
				message_str_need(ch, obj, STR_SHIELD_W);
			}
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kShield, CharEquipFlag::skip_total), ch);
		}
		if (!ch->IsNpc() && (obj = GET_EQ(ch, EEquipPos::kQuiver)) && !GET_EQ(ch, EEquipPos::kBoths)) {
			SendMsgToChar("Нет лука, нет и стрел.\r\n", ch);
			act("$n прекратил$g использовать $o3.", false, ch, obj, nullptr, kToRoom);
			PlaceObjToInventory(UnequipChar(ch, EEquipPos::kQuiver, CharEquipFlag::skip_total), ch);
		}
	}

	// calculate DAMAGE value
	// походу для выбора цели переключения в бою
	ch->damage_level = (str_bonus(GetRealStr(ch), STR_TO_DAM) + GetRealDamroll(ch)) * 2;
	if ((obj = GET_EQ(ch, EEquipPos::kBoths))
		&& obj->get_type() == EObjType::kWeapon) {
		ch->damage_level += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1)))
			>> 1; // правильный расчет среднего у оружия
	} else {
		if ((obj = GET_EQ(ch, EEquipPos::kWield))
			&& obj->get_type() == EObjType::kWeapon) {
			ch->damage_level += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1))) >> 1;
		}
		if ((obj = GET_EQ(ch, EEquipPos::kHold))
			&& obj->get_type() == EObjType::kWeapon) {
			ch->damage_level += (GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 1))) >> 1;
		}
	}

	// бонусы от морта
	if (remort::GetRealRemort(ch) >= 20) {
		ch->add_abils.mresist += remort::GetRealRemort(ch) - 19;
		ch->add_abils.presist += remort::GetRealRemort(ch) - 19;
	}

	//капы
	ch->add_abils.mresist = std::min(ch->IsNpc() ? kMaxNpcResist : kMaxPcResist, ch->add_abils.mresist);
	ch->add_abils.presist = std::min(ch->IsNpc() ? kMaxNpcResist : kMaxPcResist, ch->add_abils.presist);

	{
		// Calculate CASTER value
		auto spell_id{ESpell::kFirst};
		for (ch->caster_level = 0; !ch->IsNpc() && spell_id <= ESpell::kLast; ++spell_id) {
			if (IS_SET(GET_SPELL_TYPE(ch, spell_id), ESpellType::kKnow | ESpellType::kTemp)) {
				ch->caster_level += MUD::Spell(spell_id).GetDanger()*GET_SPELL_MEM(ch, spell_id);
			}
		}
	}

	{
		// Check steal affects
		for (const auto &i : char_stealth_aff) {
			if (saved.get(i)
				&& !AFF_FLAGS(ch).get(i)) {
				ch->check_aggressive = true;
			}
		}
	}
	if (!ch->IsNpc())
		CheckDeathRage(ch);
	if (ch->GetEnemy() || IsAffected(ch, EAffect::kGlitterDust)) {
		AFF_FLAGS(ch).unset(EAffect::kHide);
		AFF_FLAGS(ch).unset(EAffect::kSneak);
		AFF_FLAGS(ch).unset(EAffect::kDisguise);
		AFF_FLAGS(ch).unset(EAffect::kInvisible);
	}
	update_pos(ch);
	int was_lgt = AFF_FLAGGED(ch, EAffect::kSingleLight) ? kLightYes : kLightNo;
	long was_hlgt = AFF_FLAGGED(ch, EAffect::kHolyLight) ? kLightYes : kLightNo;
	long was_hdrk = AFF_FLAGGED(ch, EAffect::kHolyDark) ? kLightYes : kLightNo;

	CheckLight(ch, kLightUndef, was_lgt, was_hlgt, was_hdrk, 1);
}

void ImposeAffect(CharData *ch, const Affect<EApply> &af) {
	for (const auto &affect : ch->affected) {
		// issue.affect-migration: re-application is keyed on affect_type (the effect identity); fall back
		// to the legacy ESpell type only for affects that have no affect_type yet.
		const bool same_id = affect->affect_type == af.affect_type;
		const bool same_affect = (af.location == EApply::kNone) && (affect->affect_type == af.affect_type);
		const bool same_type = (af.location != EApply::kNone) && same_id && (affect->location == af.location);
		if (same_affect || same_type) {
			if (affect->modifier < af.modifier) {
				affect->modifier = af.modifier;
			}
			if (affect->duration < af.duration) {
				affect->duration = af.duration;
			}
			affect_total(ch);
			return;
		}
	}
	affect_to_char(ch, af);
}

void ImposeAffect(CharData *ch, Affect<EApply> &af, bool add_dur, bool max_dur, bool add_mod, bool max_mod) {
	if (af.location) {
		auto it = ch->affected.begin();

		while (it != ch->affected.end()) {
			const auto &affect = *it;
			// issue.affect-migration: merge by affect_type (effect identity), type fallback if none yet.
			const bool same_id = affect->affect_type == af.affect_type;
			if (same_id
				&& affect->location == af.location) {
				if (add_dur) {
					af.duration += affect->duration;
				} else if (max_dur) {
					af.duration = std::max(af.duration, affect->duration);
				}
				if (add_mod) {
					af.modifier += affect->modifier;
				} else if (max_mod) {
					af.modifier = std::max(af.modifier, affect->modifier);
				}
				RemoveAffect(ch, it);
				affect_to_char(ch, af);
				return;
			} else {
				++it;
			}
		}
	}
	affect_to_char(ch, af);
}


/* Insert an affect_type in a char_data structure
   Automatically sets appropriate bits and apply's */
void affect_to_char(CharData *ch, const Affect<EApply> &af, Bitvector extra_battleflag) {
	Affect<EApply>::shared_ptr affected_alloc(new Affect<EApply>(af));
	// issue.affect-migration Phase 2: effect behavior flags are sourced from affects.xml by
	// affect_type; the caller only contributes the per-instance kAfFailed bit. Guarded on the table
	// being loaded (unit tests / pre-cfg boot keep caller flags); kUndefined affects have no row.
	if (affects::AffectFlagsLoaded() && af.affect_type != EAffect::kUndefined) {
		affected_alloc->battleflag = affects::AffectFlagsByType(af.affect_type)
				| (af.battleflag & static_cast<Bitvector>(kAfFailed | kAfCharmBond))
				| extra_battleflag;   // issue.vampirism-haste: action-requested per-instance flags (e.g. kAfBattledec)
	}

	// issue.mob-flag-affect-materialization: only register mobs that need per-tick affect processing.
	// A permanent (duration -1) affect -- e.g. a materialized intrinsic buff -- never ticks or expires,
	// so it does not belong in the per-tick affected_mobs set (the damage system reads ch->affected
	// directly, so the buff still works). A later timed/pulsing affect re-registers the mob here.
	if (ch->IsNpc() && af.duration != -1) {
		affected_mobs.insert(ch);
	}
	ch->affected.push_front(affected_alloc);

	if (af.affect_type != EAffect::kUndefined)
		affect_modify(ch, af.location, af.modifier, af.affect_type, true);
	//log("[AFFECT_TO_CHAR->AFFECT_TOTAL] Start");
	affect_total(ch);
	EmitAffectEvent("affect_added", ch, af);
}

// Same as affect_to_char but without affect_total() recalculation.
// Caller MUST call affect_total(ch) after all affects are applied.
void affect_to_char_no_recalc(CharData *ch, const Affect<EApply> &af) {
	Affect<EApply>::shared_ptr affected_alloc(new Affect<EApply>(af));
	// issue.affect-migration Phase 2: effect behavior flags are sourced from affects.xml by
	// affect_type; the caller only contributes the per-instance kAfFailed bit. Guarded on the table
	// being loaded (unit tests / pre-cfg boot keep caller flags); kUndefined affects have no row.
	if (affects::AffectFlagsLoaded() && af.affect_type != EAffect::kUndefined) {
		affected_alloc->battleflag = affects::AffectFlagsByType(af.affect_type)
				| (af.battleflag & static_cast<Bitvector>(kAfFailed | kAfCharmBond));
	}

	// issue.mob-flag-affect-materialization: only register mobs that need per-tick affect processing.
	// A permanent (duration -1) affect -- e.g. a materialized intrinsic buff -- never ticks or expires,
	// so it does not belong in the per-tick affected_mobs set (the damage system reads ch->affected
	// directly, so the buff still works). A later timed/pulsing affect re-registers the mob here.
	if (ch->IsNpc() && af.duration != -1) {
		affected_mobs.insert(ch);
	}
	ch->affected.push_front(affected_alloc);

	if (af.affect_type != EAffect::kUndefined)
		affect_modify(ch, af.location, af.modifier, af.affect_type, true);
}

// Same as ImposeAffect but without affect_total() recalculation.
// Caller MUST call affect_total(ch) after all affects are applied.
void ImposeAffectNoRecalc(CharData *ch, const Affect<EApply> &af) {
	for (const auto &affect : ch->affected) {
		// issue.affect-migration: re-application is keyed on affect_type (the effect identity); fall back
		// to the legacy ESpell type only for affects that have no affect_type yet.
		const bool same_id = affect->affect_type == af.affect_type;
		const bool same_affect = (af.location == EApply::kNone) && (affect->affect_type == af.affect_type);
		const bool same_type = (af.location != EApply::kNone) && same_id && (affect->location == af.location);
		if (same_affect || same_type) {
			if (affect->modifier < af.modifier) {
				affect->modifier = af.modifier;
			}
			if (affect->duration < af.duration) {
				affect->duration = af.duration;
			}
			// Refresh the dispel potency/nature too (issue): a stronger re-cast raises the
			// recorded potency; the nature follows the (re-)casting spell.
			if (affect->potency < af.potency) {
				affect->potency = af.potency;
			}
			return;
		}
	}
	affect_to_char_no_recalc(ch, af);
}

// Same as ImposeAffect (with accumulation) but without affect_total() recalculation.
// Caller MUST call affect_total(ch) after all affects are applied.
void ImposeAffectNoRecalc(CharData *ch, Affect<EApply> &af, bool add_dur, bool max_dur, bool add_mod, bool max_mod) {
	if (af.location) {
		auto it = ch->affected.begin();

		while (it != ch->affected.end()) {
			const auto &affect = *it;
			// issue.affect-migration: merge by affect_type (effect identity), type fallback if none yet.
			const bool same_id = affect->affect_type == af.affect_type;
			if (same_id
				&& affect->location == af.location) {
				if (add_dur) {
					af.duration += affect->duration;
				} else if (max_dur) {
					af.duration = std::max(af.duration, affect->duration);
				}
				if (add_mod) {
					af.modifier += affect->modifier;
				} else if (max_mod) {
					af.modifier = std::max(af.modifier, affect->modifier);
				}
				RemoveAffect(ch, it);
				affect_to_char_no_recalc(ch, af);
				return;
			} else {
				++it;
			}
		}
	}
	affect_to_char_no_recalc(ch, af);
}

void affect_modify(CharData *ch, EApply loc, int mod, const EAffect bitv, bool add) {
	if (add) {
		AFF_FLAGS(ch).set(bitv);
	} else {
		AFF_FLAGS(ch).unset(bitv);
		mod = -mod;
	}
	switch (loc) {
		case EApply::kNone: break;
		case EApply::kStr: ch->set_str_add(ch->get_str_add() + mod);
			break;
		case EApply::kDex: ch->set_dex_add(ch->get_dex_add() + mod);
			break;
		case EApply::kInt: ch->set_int_add(ch->get_int_add() + mod);
			break;
		case EApply::kWis: ch->set_wis_add(ch->get_wis_add() + mod);
			break;
		case EApply::kCon: ch->set_con_add(ch->get_con_add() + mod);
			break;
		case EApply::kCha: ch->set_cha_add(ch->get_cha_add() + mod);
			break;
		case EApply::kClass:
		case EApply::kLvl: break;
		case EApply::kAge: GET_AGE_ADD(ch) += mod;
			break;
		case EApply::kWeight: GET_WEIGHT_ADD(ch) += mod;
			break;
		case EApply::kHeight: GET_HEIGHT_ADD(ch) += mod;
			break;
		case EApply::kManaRegen: GET_MANAREG(ch) += mod;
			break;
		case EApply::kHp: ch->set_hit_add(ch->get_hit_add() + mod);
			break;
		case EApply::kMove: ch->set_move_add(ch->get_move_add() + mod);
			break;
		case EApply::kGold:
		case EApply::kExp: break;
		case EApply::kAc: GET_AC_ADD(ch) += mod;
			break;
		case EApply::kHitroll: GET_HR_ADD(ch) += mod;
			break;
		case EApply::kDamroll: GET_DR_ADD(ch) += mod;
			break;
		case EApply::kResistFire: GET_RESIST(ch, EResist::kFire) += mod;
			break;
		case EApply::kResistAir: GET_RESIST(ch, EResist::kAir) += mod;
			break;
		case EApply::kResistDark: GET_RESIST(ch, EResist::kDark) += mod;
			break;
		case EApply::kSavingWill: SetSave(ch, ESaving::kWill, GetSave(ch, ESaving::kWill) +  mod);
			break;
		case EApply::kSavingCritical: SetSave(ch, ESaving::kCritical, GetSave(ch, ESaving::kCritical) +  mod);
			break;
		case EApply::kSavingStability: SetSave(ch, ESaving::kStability, GetSave(ch, ESaving::kStability) +  mod);
			break;
		case EApply::kSavingReflex: SetSave(ch, ESaving::kReflex, GetSave(ch, ESaving::kReflex) +  mod);
			break;
		case EApply::kHpRegen:  ch->set_hitreg(ch->get_hitreg() + mod);
			break;
		case EApply::kMoveRegen: ch->set_movereg(ch->get_movereg() + mod);
			break;
		case EApply::kFirstCircle:
		case EApply::kSecondCircle:
		case EApply::kThirdCircle:
		case EApply::kFourthCircle:
		case EApply::kFifthCircle:
		case EApply::kSixthCircle:
		case EApply::kSeventhCircle:
		case EApply::kEighthCircle:
		case EApply::kNinthCircle: ch->add_obj_slot(loc - EApply::kFirstCircle, mod);
			break;
		case EApply::kSize: GET_SIZE_ADD(ch) += mod;
			break;
		case EApply::kArmour: GET_ARMOUR(ch) += mod;
			break;
		case EApply::kPoison: GET_POISON(ch) += mod;
			break;
		case EApply::kCastSuccess: GET_CAST_SUCCESS(ch) += mod;
			break;
		case EApply::kMorale: GET_MORALE(ch) += mod;
			break;
		case EApply::kInitiative: GET_INITIATIVE(ch) += mod;
			break;
		case EApply::kReligion:
			if (add) {
				GET_PRAY(ch) |= mod;
			} else {
				GET_PRAY(ch) &= mod;
			}
			break;
		case EApply::kAbsorbe: GET_ABSORBE(ch) += mod;
			break;
		case EApply::kLikes: GET_LIKES(ch) += mod;
			break;
		case EApply::kResistWater: GET_RESIST(ch, EResist::kWater) += mod;
			break;
		case EApply::kResistEarth: GET_RESIST(ch, EResist::kEarth) += mod;
			break;
		case EApply::kResistVitality: GET_RESIST(ch, EResist::kVitality) += mod;
			break;
		case EApply::kResistMind: GET_RESIST(ch, EResist::kMind) += mod;
			break;
		case EApply::kResistImmunity: GET_RESIST(ch, EResist::kImmunity) += mod;
			break;
		case EApply::kAffectResist: GET_AR(ch) += mod;
			break;
		case EApply::kMagicResist: GET_MR(ch) += mod;
			break;
		// issue.damage-over-time: aconite contributes to the shared GET_POISON DoT at 1/4 rate (it is also
		// a heavy debuff, so its DoT share is 4x weaker than a plain poison). TEST of kvirund's model:
		// no separate aconite damage mechanic -- the poison tick deals it via GET_POISON, once per round.
		case EApply::kAconitumPoison: GET_POISON(ch) += mod / 4;
			break;
		case EApply::kBelenaPoison: GET_SKILL_REDUCE(ch) += mod;
			break;
		case EApply::kDaturaPoison: GET_SKILL_REDUCE(ch) += mod;
			break;
		case EApply::kPhysicResist: GET_PR(ch) += mod; //скиллрезист
			break;
		case EApply::kPhysicDamagePercent: ch->add_abils.percent_physdam_add += mod;
			break;
		case EApply::kMagicDamagePercent: 
			ch->add_abils.percent_spellpower_add += mod;
			break;
		case EApply::kExpPercent: ch->add_abils.percent_exp_add += mod;
			break;
		case EApply::kSpelledBlinkPhys: ch->add_abils.percent_spell_blink_phys += mod;
			break;
			case EApply::kSpelledBlinkMag: ch->add_abils.percent_spell_blink_mag += mod;
				break;
		case EApply::kPoisoned: GET_POISON(ch) += mod;
			break;
		case EApply::kBind: GET_BIND(ch) += mod;
			break;
		case EApply::kSkillsBonus: {
			ch->set_skill_bonus(ch->get_skill_bonus() + mod);
		}
		default:break;
	}            // switch
}

// issue.mob-flag-affect-materialization: realize a flag-only NPC's kAfMaterialize buff flags as real
// duration=-1 (permanent, dispellable) affects, so the data-driven affect system works for it. Skips
// flags that already have a matching affect. One recalc at the end. Each buff's real node(s) -- bare
// flag-carrier for actions-only buffs (sanct/shields/prism/glass), or one node per <apply> (kCloudly ->
// blink+AC, kBlink -> blink) with a mob-scaled modifier and potency -- are built by
// BuildMaterializedAffect (magic), which rolls the buff's same-named spell for this mob.
// issue.autoaffects-hotfix: see affect_data.h. Real affects first (their potency wins on dedup),
// then any flagged kAfMaterialize affect not already present (equipment shields on a PC), potency 0.
// TEMPORARY bridge -- superseded by full equipment-affect materialization on unstable.next (then remove).
std::vector<std::pair<EAffect, float>> VictimWardAffects(CharData *ch) {
	std::vector<std::pair<EAffect, float>> out;
	for (const auto &aff : ch->affected) {
		if (aff) {
			out.emplace_back(aff->affect_type, aff->potency);
		}
	}
	for (const EAffect at : affects::MaterializableAffects()) {
		if (!AFF_FLAGGED(ch, at)) {
			continue;
		}
		bool present = false;
		for (const auto &p : out) {
			if (p.first == at) {
				present = true;
				break;
			}
		}
		if (!present) {
			out.emplace_back(at, 0.0f);
		}
	}
	return out;
}

void MaterializeMobFlagAffects(CharData *mob) {
	if (!mob || !mob->IsNpc() || !affects::AffectFlagsLoaded() || mob->get_rnum() < 0) {
		return;
	}
	// Read the INTRINSIC (prototype) buff flags, not the live mob's: a dispelled buff is gone from the
	// live flags but must still be restorable, and on a fresh wake the live flags equal the prototype's.
	const CharData *proto = &mob_proto[mob->get_rnum()];
	bool added = false;
	for (const EAffect at : affects::MaterializableAffects()) {
		if (!AFF_FLAGGED(proto, at)) {
			continue;
		}
		bool has_struct = false;
		for (const auto &aff : mob->affected) {
			if (aff && aff->affect_type == at) {
				has_struct = true;
				break;
			}
		}
		if (has_struct) {
			continue;
		}
		for (auto &af : BuildMaterializedAffect(mob, at)) {
			affect_to_char_no_recalc(mob, af);
			added = true;
		}
	}
	if (added) {
		affect_total(mob);
	}
}

void MaterializeZoneMobAffects(ZoneRnum zrn) {
	if (zrn < 0 || static_cast<std::size_t>(zrn) >= zone_table.size()
		|| affects::MaterializableAffects().empty()) {
		return;
	}
	const RoomRnum first = zone_table[zrn].RnumRoomsLocation.first;
	const RoomRnum last = zone_table[zrn].RnumRoomsLocation.second;
	if (first < 0 || last < first) {
		return;
	}
	for (RoomRnum rrn = first; rrn <= last && rrn <= top_of_world; ++rrn) {
		if (rrn < 0 || !world[rrn]) {
			continue;
		}
		for (const auto vict : world[rrn]->people) {
			if (vict && vict->IsNpc()) {
				MaterializeMobFlagAffects(vict);
			}
		}
	}
}

void MarkZoneUsed(ZoneRnum zrn) {
	if (zrn < 0 || static_cast<std::size_t>(zrn) >= zone_table.size()) {
		return;
	}
	if (!zone_table[zrn].used) {          // dormant -> awake edge
		zone_table[zrn].used = true;
		MaterializeZoneMobAffects(zrn);
	}
}

// * Снятие аффектов с чара при смерти/уходе в дт.
void reset_affects(CharData *ch) {
	auto af = ch->affected.begin();

	while (af != ch->affected.end()) {
		const auto &affect = *af;

		if (!IS_SET(affect->battleflag, kAfDeadkeep)) {
			af = RemoveAffect(ch, af);
		} else {
			++af;
		}
	}
	GET_COND(ch, condition::kDrunk) = 0; // Чтобы не шатало без аффекта "под мухой"
	affect_total(ch);
}


// Same as reset_affects but without affect_total() recalculation.
// Used in raw_kill for PCs Б─■ mob is about to be deleted, PC will recalc on re-enter.
void reset_affects_no_recalc(CharData *ch) {
	auto af = ch->affected.begin();

	while (af != ch->affected.end()) {
		const auto &affect = *af;

		if (!IS_SET(affect->battleflag, kAfDeadkeep)) {
			af = RemoveAffect(ch, af);
		} else {
			++af;
		}
	}
	GET_COND(ch, condition::kDrunk) = 0;
}
bool IsAffectedWithCasterId(CharData *ch, CharData *vict, EAffect affect_type) {
	for (const auto &affect : vict->affected) {
		if (affect->affect_type == affect_type && affect->caster_id == ch->get_uid()
				&& !IS_SET(affect->battleflag, kAfFailed)) {
			return true;
		}
	}
	return false;
}

bool IsAffected(CharData *ch, EAffect affect_type) {
	for (const auto &affect : ch->affected) {
		if (affect->affect_type == affect_type && !IS_SET(affect->battleflag, kAfFailed)) {
			return true;
		}
	}
	return false;
}

bool IsAffectedOrAttempting(CharData *ch, EAffect affect_type) {
	for (const auto &affect : ch->affected) {
		if (affect->affect_type == affect_type) {
			return true;
		}
	}
	return false;
}

bool IsAffectedWithFlag(CharData *ch, EAffFlag flag) {
	for (const auto &affect : ch->affected) {
		if (IS_SET(affect->battleflag, flag) && !IS_SET(affect->battleflag, kAfFailed)) {
			return true;
		}
	}
	return false;
}

bool IsNegativeApply(EApply location) {
	for (auto elem : apply_negative) {
		if (location == elem.location)
			return true;
	}
	return false;
}

bool GetAffectNumByName(const std::string &affName, EAffect &result) {
	return affects::FindByShortDesc(affName, result);
}

// issue.calc-duration: skill-based duration. `caster` provides the skill. issue.duration-scale: the
// skill is uncapped; min/max bound the TOTAL duration, and an unset max (0) defaults to the old
// skill-75 value (base + 75/divisor) so un-retuned effects still cap at skill 75. `victim` decides
// the unit (PC: convert hours to player-affect ticks; NPC: raw).
// skill_id == kUndefined skips the skill bonus -- used for flat durations and for spells without
// a <potency_roll>. min/max keep the OLD-style "0 means no clamp on that side" semantics.
int CalcDuration(CharData *caster, CharData *victim, ESkill skill_id,
				 unsigned base, unsigned skill_divisor, int min, int max, int skill_override, bool raw_rounds) {
	// issue.vampirism-haste: raw_rounds keeps the value in combat-round units (as for NPCs) instead of
	// converting hours->ticks for PCs -- used by battle-decrementing affects, which are round-measured.
	const bool no_convert = victim->IsNpc() || raw_rounds;
	if (skill_divisor == 0 && min == 0 && max == 0) {
		return (no_convert ? base : (base * kSecsPerMudHour / kSecsPerPlayerAffect));
	}
	// issue.potion-hotfix: skill_override >= 0 (a potion) scales off the potion MAKER's stored skill;
	// override < 0 = a live cast (the caster's skill); kUndefined skill = flat (no scaling).
	const int skill_bonus =
		(skill_id == ESkill::kUndefined) ? 0
		: (skill_override >= 0) ? CalcDurationSkillBonusForValue(skill_override, skill_divisor)
		: CalcDurationSkillBonus(caster, skill_id, skill_divisor);
	int total = static_cast<int>(base) + skill_bonus;   // Option A: min/max bound the TOTAL
	const int default_max = (skill_divisor == 0)
		? static_cast<int>(base)
		: static_cast<int>(base) + abilities::kNoviceSkillThreshold / static_cast<int>(skill_divisor);
	const int hi = std::max(min, (max > 0) ? max : default_max);   // never below the min floor
	if (min > 0) {
		total = std::max(total, min);
	}
	total = std::min(total, hi);
	const auto duration = static_cast<unsigned>(std::max(0, total));
	return (no_convert ? duration : (duration * kSecsPerMudHour / kSecsPerPlayerAffect));
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
