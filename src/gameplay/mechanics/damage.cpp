/**
\file damage.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 05.11.2025.
\brief Brief description.
\detail Detail description.
*/

#include "damage.h"
#include "gameplay/core/experience.h"
#include "utils/logger.h"
#include "administration/privilege.h"
#include "utils/grammar/gender.h"
#include "gameplay/mechanics/minions.h"
#include "gameplay/mechanics/mount.h"
#include "gameplay/mechanics/resist.h"
#include "gameplay/mechanics/bonus.h"
#include "engine/entities/char_data.h"
#include "utils/utils_time.h"
#include "engine/db/global_objects.h"
// \TODO Используемые функции из fight.h нужно вынести в механики. Помереть, скажем, можно не только в бою.
#include "gameplay/fight/fight.h"
#include "gameplay/mechanics/equipment.h"
#include "gameplay/clans/house_exp.h"
#include "gameplay/statistics/dps.h"
#include "engine/observability/event_sink.h"
#include "gameplay/core/remort.h"
#include "engine/ui/color.h"
#include "gameplay/core/game_limits.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/affects/affect_handler.h"
#include "gameplay/magic/magic_utils.h"
#include "gameplay/magic/magic.h"   // issue.character-affect-triggers: RunCharEventTriggers / EventContext (kKill)
#include "gameplay/affects/affect_messages.h"   // issue.damage-change: affects::AffectActions
#include "gameplay/abilities/talents_actions.h"  // issue.damage-change: DamageChange / kWardDamage
#include "gameplay/affects/affect_data.h"  // VictimWardAffects (autoaffects-hotfix; TEMPORARY, see unstable.next)
#include "gameplay/ai/spec_procs.h"
#include "gameplay/mechanics/groups.h"
#include "gameplay/mechanics/tutelar.h"
#include "gameplay/mechanics/sight.h"
#include "utils/backtrace.h"

#include <chrono>

#include <fmt/format.h>

void TryRemoveExtrahits(CharData *ch, CharData *victim);

// Estern - нужно разобраться, почему функции работы с опытом распиханы по всем углам
//
void Damage::CalcArmorDmgAbsorption(CharData *victim) {
	// броня на физ дамаг
	if (dam > 0 && dmg_type == fight::kPhysDmg) {
		DamageEquipment(victim, kNowhere, dam, 50);
		if (!flags[fight::kCritHit] && !flags[fight::kIgnoreArmor]) {
			// 50 брони = 50% снижение дамага
			int max_armour = 50;
			if (CanUseFeat(victim, EFeat::kImpregnable) && victim->IsFlagged(EPrf::kAwake)) {
				// непробиваемый в осторожке - до 75 брони
				max_armour = 75;
			}
			if (CanUseFeat(victim, EFeat::kShadowStrike) && victim->IsFlagged(EPrf::kAwake)) {
				// танцующая тень в осторожке - до 60 брони
				max_armour = 60;
			}
			int tmp_dam = dam * std::max(0, std::min(max_armour, GET_ARMOUR(victim))) / 100;
			// ополовинивание брони по флагу скила
			if (tmp_dam >= 2 && flags[fight::kHalfIgnoreArmor]) {
				tmp_dam /= 2;
			}
			dam -= tmp_dam;
			// крит удар умножает дамаг, если жертва без призмы и без лед.щита
		}
	}
}

/**
 * Обработка поглощения физ урона.
 * \return true - полное поглощение
 */
bool Damage::CalcDmgAbsorption(CharData *ch, CharData *victim) {
	if (dmg_type == fight::kPhysDmg
		&& skill_id < ESkill::kFirst
		&& spell_id < ESpell::kUndefined
		&& dam > 0
		&& GET_ABSORBE(victim) > 0) {
		// шансы поглощения: непробиваемый в осторожке 15%, остальные 10%
		int chance = 10 + remort::GetRealRemort(victim) / 3;
		if (CanUseFeat(victim, EFeat::kImpregnable)
			&& victim->IsFlagged(EPrf::kAwake)) {
			chance += 5;
		}
		// физ урон - прямое вычитание из дамага
		if (number(1, 100) <= chance) {
			dam -= GET_ABSORBE(victim) / 2;
			if (dam <= 0) {
				act("Ваши доспехи полностью поглотили удар $n1.",
					false, ch, nullptr, victim, kToVict);
				act("Доспехи $N1 полностью поглотили ваш удар.",
					false, ch, nullptr, victim, kToChar);
				act("Доспехи $N1 полностью поглотили удар $n1.",
					true, ch, nullptr, victim, kToNotVict | kToArenaListen);
				return true;
			}
		}
	}
	if (dmg_type == fight::kMagicDmg
		&& dam > 0
		&& GET_ABSORBE(victim) > 0
		&& !flags[fight::kIgnoreAbsorbe]) {
// маг урон - по 1% за каждые 2 абсорба, максимум 25% (цифры из mag_damage)
		int absorb = std::min(GET_ABSORBE(victim) / 2, 25);
		dam -= dam * absorb / 100;
	}
	return false;
}

// issue.damage-change: does the victim have an active affect that declares a given category property?
// Lets engine damage rules test a declared affect property instead of naming a specific affect id. A
// shield's property counts only when it is the shield chosen for this hit (so ice/air stay selection-
// gated); a non-shield affect (e.g. prismatic aura) counts whenever it is present.
static bool VictimAffectDeclares(int selected_shield, CharData *victim, EAffFlag flag) {
	// autoaffects-hotfix (TEMPORARY -- reverts to victim->affected on unstable.next; see VictimWardAffects):
	// also yields flag-only equipment shields that this branch does not materialize.
	for (const auto &ward : VictimWardAffects(victim)) {
		const EAffect at = ward.first;
		if ((affects::AffectFlagsByType(at) & flag) == 0) {
			continue;
		}
		if (affects::AffectShieldWeight(at) > 0 && static_cast<int>(at) != selected_shield) {
			continue;   // a shield acts only when it is the one chosen for this hit
		}
		return true;
	}
	return false;
}

// issue.damage-change: does the victim have any affect that grants TOTAL damage immunity (kAfFullAbsorb,
// e.g. the "magic cocoon" kGodsShield)? Unlike VictimAffectDeclares this reads the AFF_FLAGS bitset, not
// the affect structs, so it holds for EVERY holder -- cast (affect_total ORs the flag), worn item
// (weapon-affect sets the flag) and bare-flag NPC (vendors/renters) alike -- without needing a struct.
static bool VictimFullyAbsorbs(CharData *victim) {
	for (const EAffect at : affects::FullAbsorbAffects()) {
		if (AFF_FLAGGED(victim, at)) {
			return true;
		}
	}
	return false;
}

void Damage::SendCritHitMsg(CharData *ch, CharData *victim) {
	// issue.damage-change: the ice-shield "sank into the icy veil" crit flavor now lives in the ice
	// crit-absorb <damage_change>'s kTransformCrit* sheaf message (shown on the 94% absorb). This is just
	// the plain crit line for a crit that lands.
	sprintf(buf, "&G&qВаше меткое попадание тяжело ранило %s.&Q&n\r\n",
			sight::PersonName(victim, ch, 3));
	SendMsgToChar(buf, ch);

	sprintf(buf, "&r&qМеткое попадание %s тяжело ранило вас.&Q&n\r\n",
			sight::PersonName(ch, victim, 1));
	SendMsgToChar(buf, victim);
	// Закомментил чтобы не спамило, сделать потом в виде режима
	//act("Меткое попадание $N1 заставило $n3 пошатнуться.", true, victim, nullptr, ch, TO_NOTVICT);
}

void Damage::ProcessBlink(CharData *ch, CharData *victim) {
	if (flags[fight::kIgnoreBlink] || flags[fight::kCritLuck])
		return;
	ubyte blink = 0;
	// даже в случае попадания можно уклониться мигалкой
	// issue.mob-flag-affect-materialization: gate on the miss-chance APPLY, not the AFF flag. kCloudly/
	// kBlink now grant kSpelledBlinkMag/Phys from every source -- cast (affects.xml <apply>), worn item
	// (GetApplyByWeaponAffect) and materialized flag-only mob (BuildMaterializedAffect) -- so the flag
	// check is redundant. NPC bearers still take level+remort; PCs take the apply value.
	if (dmg_type == fight::kMagicDmg) {
		if (victim->add_abils.percent_spell_blink_mag > 0) {
			if (victim->IsNpc()) {
				blink = GetRealLevel(victim) + remort::GetRealRemort(victim);
			} else {
				blink = victim->add_abils.percent_spell_blink_mag;
			}
		}
	} else if(dmg_type == fight::kPhysDmg) {
		if (victim->add_abils.percent_spell_blink_phys > 0) {
			if (victim->IsNpc()) {
				blink = GetRealLevel(victim) + remort::GetRealRemort(victim);
			} else {
				blink = victim->add_abils.percent_spell_blink_phys;
			}
		}
	}
	if(blink < 1)
		return;
//	SendToTC(ch, false, true, false, "Шанс мигалки равен == %d процентов.\r\n", blink);
//	SendToTC(victim, false, true, false, "Шанс мигалки равен == %d процентов.\r\n", blink);
	int bottom = 1;
	if (ch->calc_morale() > number(1, 100)) // удача
		bottom = 10;
	if (number(bottom, blink) >= number(1, 100)) {
		sprintf(buf, "%sНа мгновение вы исчезли из поля зрения противника.%s\r\n",
				kColorBoldBlk, kColorNrm);
		SendMsgToChar(buf, victim);
		act("$n исчез$q из вашего поля зрения.", true, victim, nullptr, ch, kToVict);
		act("$n исчез$q из поля зрения $N1.", true, victim, nullptr, ch, kToNotVict);
		dam = 0;
		return;
	}
}

// issue.damage-change: apply the victim's data-driven incoming-damage modifiers. Each kWardDamage
// <damage_change> whose <conditions> match this Damage (type/element/flags) rolls its prob, then scales
// `dam` by the variation and edits `flags`. Replaces the hardcoded per-affect blocks (kSanctuary,
// kPrismaticAura, ... as they migrate); applied in affect-list order. -1 masks mean "any".
void Damage::ApplyAffectDamageChanges(CharData *ch, CharData *victim, bool late_stage) {
	if (dam <= 0 || !victim) {
		return;
	}
	// autoaffects-hotfix (TEMPORARY -- reverts to victim->affected on unstable.next; see VictimWardAffects):
	// also yields flag-only equipment shields that this branch does not materialize.
	for (const auto &ward : VictimWardAffects(victim)) {
		const EAffect at = ward.first;
		// issue.damage-change: a shield affect acts only when it is the one chosen for this hit.
		if (const int sw = affects::AffectShieldWeight(at);
			sw > 0 && static_cast<int>(at) != selected_shield_) {
			continue;
		}
		for (const auto &action : affects::AffectActions(at).list()) {
			if (!action.GetTrigger().test(talents_actions::EActionTrigger::kWardDamage)) {
				continue;
			}
			const auto &dc = action.GetDamageChange();
			if (!dc.present) {
				continue;
			}
			// stage="late" modifiers (shield reductions) run at a separate, later hook so a hardcoded
			// reflect still reads the pre-reduction damage; the default (early) hook handles the rest.
			if (dc.late != late_stage) {
				continue;
			}
			if (dc.type_mask && !(dc.type_mask & (1u << static_cast<unsigned>(dmg_type)))) {
				continue;
			}
			if (dc.element_mask && !(dc.element_mask & (1u << static_cast<unsigned>(element)))) {
				continue;
			}
			const unsigned long long f = flags.to_ullong();   // re-read: a prior change may have edited flags
			if ((f & dc.flags_present) != dc.flags_present || (f & dc.flags_missing) != 0) {
				continue;
			}
			if (dc.prob < 100 && number(1, 100) > dc.prob) {
				continue;
			}
			if (dc.var_factor != 0) {
				const int pct = (dc.var_min == dc.var_max) ? dc.var_min : number(dc.var_min, dc.var_max);
				dam = dam * (100 + dc.var_factor * pct) / 100;
				if (dam < 0) {
					dam = 0;
				}
			}
			for (int i = 0; i < fight::kHitFlagsNum; ++i) {
				if (dc.flags_add & (1ULL << i)) {
					flags.set(i);
				}
				if (dc.flags_remove & (1ULL << i)) {
					flags.reset(i);
				}
			}
			// The modification applied: show the affect's own transform flavor (e.g. "the shield softened
			// the blow"). The bearer is `victim` ($N); the attacker is `ch` ($n). Optional -- empty = silent.
			if (ch && dc.msg_variant != 2) {
				using EAMT = affects::EAffectMsgType;
				const EAMT sc = (dc.msg_variant == 1) ? EAMT::kTransformCritToChar : EAMT::kTransformToChar;
				const EAMT sv = (dc.msg_variant == 1) ? EAMT::kTransformCritToVict : EAMT::kTransformToVict;
				const EAMT sr = (dc.msg_variant == 1) ? EAMT::kTransformCritToRoom : EAMT::kTransformToRoom;
				const std::string &mc = affects::AffectMsgRaw(at, sc);
				const std::string &mv = affects::AffectMsgRaw(at, sv);
				const std::string &mr = affects::AffectMsgRaw(at, sr);
				if (!mc.empty()) { act(mc.c_str(), false, ch, nullptr, victim, kToChar | kToNoBriefShields); }
				if (!mv.empty()) { act(mv.c_str(), false, ch, nullptr, victim, kToVict | kToNoBriefShields); }
				if (!mr.empty()) {
					act(mr.c_str(), true, ch, nullptr, victim, kToNotVict | kToArenaListen | kToNoBriefShields);
				}
			}
		}
	}
}

void Damage::ApplyRetaliations(CharData *ch, CharData *victim) {
	if (dam <= 0 || !ch || !victim || victim->affected.empty()) {
		return;
	}
	// A reflected hit must never itself retaliate, or two thorns-bearers would bounce forever.
	if (flags[fight::kMagicReflect]) {
		return;
	}
	// autoaffects-hotfix (TEMPORARY -- reverts to victim->affected on unstable.next; see VictimWardAffects):
	// also yields flag-only equipment shields that this branch does not materialize.
	for (const auto &ward : VictimWardAffects(victim)) {
		const EAffect at = ward.first;
		// issue.damage-change: a shield affect retaliates only when it is the one chosen for this hit.
		if (const int sw = affects::AffectShieldWeight(at);
			sw > 0 && static_cast<int>(at) != selected_shield_) {
			continue;
		}
		for (const auto &action : affects::AffectActions(at).list()) {
			if (!action.GetTrigger().test(talents_actions::EActionTrigger::kWardDamage)) {
				continue;
			}
			const auto &rt = action.GetRetaliation();
			if (!rt.present) {
				continue;
			}
			if (rt.type_mask && !(rt.type_mask & (1u << static_cast<unsigned>(dmg_type)))) {
				continue;
			}
			if (rt.element_mask && !(rt.element_mask & (1u << static_cast<unsigned>(element)))) {
				continue;
			}
			const unsigned long long f = flags.to_ullong();
			if ((f & rt.flags_present) != rt.flags_present || (f & rt.flags_missing) != 0) {
				continue;
			}
			if (rt.prob < 100 && number(1, 100) > rt.prob) {
				continue;
			}
			// Percent of the PRE-reduction damage, plus the bearer's (victim's) NPC/boss bonuses.
			int pct = (rt.pct_min == rt.pct_max) ? rt.pct_min : number(rt.pct_min, rt.pct_max);
			if (victim->IsNpc() && !IsCharmice(victim)) {
				pct += rt.npc_bonus;
				if (victim->get_role(static_cast<unsigned>(EMobClass::kBoss))) {
					pct += rt.boss_bonus;
				}
			}
			const int amount = dam * pct / 100;
			if (amount > 0) {
				ReflectHit hit;
				hit.amount = amount;
				hit.type = static_cast<fight::DmgType>((rt.dmg_type >= 0) ? rt.dmg_type : dmg_type);
				hit.element = (rt.element >= 0) ? static_cast<EElement>(rt.element) : element;
				reflect_pool_.push_back(hit);
			}
			// Flag edits (e.g. the kDrawBriefMagMirror HUD glyph) apply whenever the ward reacts.
			for (int i = 0; i < fight::kHitFlagsNum; ++i) {
				if (rt.flags_add & (1ULL << i)) {
					flags.set(i);
				}
				if (rt.flags_remove & (1ULL << i)) {
					flags.reset(i);
				}
			}
		}
	}
}

void Damage::DealReflectPool(CharData *ch, CharData *victim) {
	if (reflect_pool_.empty()
		|| !victim->GetEnemy()
		|| victim->GetPosition() <= EPosition::kStun
		|| victim->in_room == kNowhere) {
		return;
	}
	for (const auto &hit : reflect_pool_) {
		if (hit.amount <= 0) {
			continue;
		}
		// A ward deals no damage of its own: the reflect is credited to the incoming attack (the spell
		// being reflected), so its own combat/death messages are the ones shown.
		Damage dmg(SpellDmg(spell_id), hit.amount, hit.type);
		dmg.element = hit.element;
		dmg.flags.set(fight::kNoFleeDmg);
		dmg.flags.set(fight::kMagicReflect);
		dmg.Process(victim, ch);
	}
	reflect_pool_.clear();
}

void Damage::ProcessDeath(CharData *ch, CharData *victim) const {
	CharData *killer = nullptr;

	if (victim->IsNpc() || victim->desc) {
		if (victim == ch && victim->in_room != kNowhere) {
			// Урон сам себе (тик повреждающего аффекта и т.п.): убийство засчитывается "автору"
			// урона (author_uid -- обычно caster_id аффекта), если он рядом. Нет автора (0) или
			// автор сам victim -- не засчитывается. Любой будущий повреждающий аффект просто
			// выставляет author_uid, отдельная ветка тут не нужна.
			if (author_uid != 0 && author_uid != victim->get_uid()) {
				for (const auto author : world[victim->in_room]->people) {
					if (author != victim && author->get_uid() == author_uid) {
						killer = author;
						break;
					}
				}
			} else if (damage_source == fight::EDamageSource::kSuffering) {
				for (const auto attacker : world[victim->in_room]->people) {
					if (attacker->GetEnemy() == victim) {
						killer = attacker;
					}
				}
			}
		}

		if (ch != victim) {
			killer = ch;
		}
	}
	if (killer) {
		if (AFF_FLAGGED(killer, EAffect::kGroup)) {
			// т.к. помечен флагом AFF_GROUP - точно PC
			experience::group_gain(killer, victim);
		} else if ((killer->IsFlagged(EMobFlag::kCompanion))
			&& killer->has_master())
			// killer - зачармленный NPC с хозяином
		{
			// по логике надо бы сделать, что если хозяина нет в клетке, но
			// кто-то из группы хозяина в клетке, то опыт накинуть согруппам,
			// которые рядом с убившим моба чармисом.
			if (AFF_FLAGGED(killer->get_master(), EAffect::kGroup)
				&& killer->in_room == killer->get_master()->in_room) {
				// Хозяин - PC в группе => опыт группе
				experience::group_gain(killer->get_master(), victim);
			} else if (killer->in_room == killer->get_master()->in_room) {
				experience::perform_group_gain(killer->get_master(), victim, 1, 100);
			}
			// else
			// А хозяина то рядом не оказалось, все чармису - убрано
			// нефиг абьюзить чарм  group::perform_group_gain( killer, victim, 1, 100 );
		} else {
			// Просто NPC или PC сам по себе
			experience::perform_group_gain(killer, victim, 1, 100);
		}
	}

	// в сислог иммам идут только смерти в пк (без арен)
	// в файл пишутся все смерти чаров
	// если чар убит палачем то тоже не спамим
	if (!victim->IsNpc() && !(killer && killer->IsFlagged(EPrf::kExecutor))) {
		UpdatePkLogs(ch, victim);

		for (const auto &ch_vict : world[ch->in_room]->people) {
			//Мобы все кто присутствовал при смерти игрока забывают
			if (privilege::IsImmortal(ch_vict))
				continue;
			if (!HERE(ch_vict))
				continue;
			if (!ch_vict->IsNpc())
				continue;
			if (ch_vict->IsFlagged(EMobFlag::kMemory)) {
				mob_ai::mobForget(ch_vict, victim);
			}
		}

	}

	// issue.character-affect-triggers: kKill trigger -- the killer just dealt the fatal damage. Fire
	// their affects' kKill actions while the victim is still a valid (dead, unpurged) pointer, before
	// die() below turns it into a corpse. Gates: (1) a real character is credited (killer != nullptr;
	// killer != victim is already guaranteed by the resolution above); (2) killer and victim are in the
	// SAME room -- an indirect DoT/spell kill only counts if its credited author is still here; (3) the
	// damage is not server/script-dealt (kTriggerDeath = DG m/w/o-damage -- "the server hit them, not a
	// character"). Recursion (an on-kill effect that itself deals damage) is bounded by the shared
	// trigger-action depth guard inside RunRoomCycledAction. The event exposes the victim as `actor` for
	// reads only -- do NOT cast on it (it is dead); the corpse guards in the cast pipeline log+bail if so.
	if (killer && killer->in_room == victim->in_room
			&& damage_source != fight::EDamageSource::kTriggerDeath) {
		EventContext kill_event;
		kill_event.trigger = talents_actions::EActionTrigger::kKill;
		kill_event.amount = dam;
		kill_event.weapon = wielded;
		kill_event.skill = skill_id;
		kill_event.actor = victim;
		RunCharEventTriggers(killer, kill_event);
		// A kill-trigger side effect (e.g. an explosion) may have purged one of the parties.
		if (victim->purged()) {
			return;   // the death was already fully processed by a nested ProcessDeath/die path
		}
		if (killer->purged()) {
			killer = nullptr;   // fall back to the original attacker for die() below
		}
	}

	if (killer) {
		ch = killer;
	}
	die(victim, ch);
}

/**
 * Разный инит щитов у мобов и чаров.
 * У мобов работают все 3 щита, у чаров только 1 рандомный на текущий удар.
 */
void Damage::SelectMagicShield(CharData *victim) {
	// issue.damage-change: a character may wear several elemental shields, but a given hit passes through
	// exactly ONE, chosen weighted-random by each shield's AffectShieldWeight (roulette-wheel). This
	// unifies PCs and NPCs (both: all shields active, one applies per hit) and is decided up-front so
	// BOTH the retaliation pass and the reduction pass gate on the same choice. The pool is every shield
	// the victim has, independent of the hit -- so e.g. a crit is absorbed only when ice is the pick.
	int total = 0;
	// autoaffects-hotfix (TEMPORARY, see VictimWardAffects): include flag-only equipment shields in the pool.
	const auto wards = VictimWardAffects(victim);
	for (const auto &ward : wards) {
		total += affects::AffectShieldWeight(ward.first);
	}
	if (total <= 0) {
		return;   // no shields -> selected_shield_ stays -1
	}
	int roll = number(1, total);
	int acc = 0;
	for (const auto &ward : wards) {
		const int w = affects::AffectShieldWeight(ward.first);
		if (w <= 0) {
			continue;
		}
		acc += w;
		if (acc >= roll) {
			selected_shield_ = static_cast<int>(ward.first);
			break;
		}
	}
}

void Damage::PerformPostInit(CharData *ch, CharData *victim) {
	if (ch_start_pos == EPosition::kUndefined) {
		ch_start_pos = ch->GetPosition();
	}

	if (victim_start_pos == EPosition::kUndefined) {
		victim_start_pos = victim->GetPosition();
	}

	SelectMagicShield(victim);

	// issue.damage-change: expose the precise-style (точный стиль) crit as a hit-flag so the ice-shield
	// crit-absorb <damage_change> can gate on it -- a punctual crit (dam_critic>0) pierces the ice shield.
	if (dam_critic > 0) {
		flags.set(fight::kPunctualCrit);
	}
}

// обработка щитов, зб, поглощения, сообщения для огн. щита НЕ ЗДЕСЬ
// возвращает сделанный дамаг
int Damage::Process(CharData *ch, CharData *victim) {
	PerformPostInit(ch, victim);
	if (victim->in_room == kNowhere || ch->in_room == kNowhere || ch->in_room != victim->in_room) {
		log("SYSERR: Attempt to damage '%s' in room kNowhere by '%s'.",
			GET_NAME(victim), GET_NAME(ch));
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		return 0;
	}
	if (victim->purged()) { //будем мониторить коредамп
		log("SYSERR: Attempt to damage purged char/mob '%s' in room #%d by '%s'.",
			GET_NAME(victim), GET_ROOM_VNUM(victim->in_room), GET_NAME(ch));
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		return 0;
	}
	if (!check_valid_chars(ch, victim, __FILE__, __LINE__)) {
		log("SYSERR: Attempt to damage purged char/mob ch or vict");
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		return 0;
	}
	if (victim->GetPosition() <= EPosition::kDead) {
		log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
			GET_NAME(victim), GET_ROOM_VNUM(victim->in_room), GET_NAME(ch));
		debug::backtrace(runtime_config.logs(SYSLOG).handle());
		die(victim, nullptr);
		return 0;
	}
	if ((ch->IsNpc() && ch->IsFlagged(EMobFlag::kNoFight))
		|| (victim->IsNpc() && victim->IsFlagged(EMobFlag::kNoFight))) {
		return 0;
	}
	if (dam > 0) {
		if (privilege::IsGod(victim)) {
			dam = 0;
		} else if (privilege::IsImmortal(victim) || GET_GOD_FLAG(victim, EGf::kGodsLike)) {
			dam /= 4;
		} else if (GET_GOD_FLAG(victim, EGf::kGodscurse)) {
			dam *= 2;
		}
	}

	mob_ai::update_mob_memory(ch, victim);

	// If you attack a pet, it hates your guts
	if (!group::same_group(ch, victim)) {
		check_agro_follower(ch, victim);
	}
	if (victim != ch) {
		if (ch->GetPosition() > EPosition::kStun && (ch->GetEnemy() == nullptr)) {
			if (!pk_agro_action(ch, victim)) {
				return 0;
			}
			SetFighting(ch, victim);
			npc_groupbattle(ch);
		}
		// Start the victim fighting the attacker
		if (victim->GetPosition() > EPosition::kDead && (victim->GetEnemy() == nullptr)) {
			SetFighting(victim, ch);
			npc_groupbattle(victim);
		}

		// лошадь сбрасывает седока при уроне
		if (mount::IsOnHorse(ch) && mount::GetHorse(ch) == victim) {
			mount::DropFromHorse(victim);
		} else if (mount::IsOnHorse(victim) && mount::GetHorse(victim) == ch) {
			mount::DropFromHorse(ch);
		}
	}
	sight::Appear(ch);
	sight::Appear(victim);

	if (dam < 0 || ch->in_room == kNowhere || victim->in_room == kNowhere || ch->in_room != victim->in_room) {
		return 0;
	}

	if (ch->GetPosition() <= EPosition::kIncap) {
		return 0;
	}

	if (dam >= 2) {
		// issue.damage-change: data-driven incoming-damage modifiers (kWardDamage <damage_change>).
		// kSanctuary and kPrismaticAura are now affect data (their hardcoded phys/magic scaling lived
		// right here and was removed); kHold and the shields' reduction migrate in later phases.
		ApplyAffectDamageChanges(ch, victim, /*late_stage=*/false);

		if (victim->IsNpc() && Bonus::is_bonus_active(Bonus::EBonusType::BONUS_DAMAGE)) {
			dam *= Bonus::get_mult_bonus();
		}
	}
	//учет резистов для магического урона
	if (dmg_type == fight::kMagicDmg) {
		if (spell_id > ESpell::kUndefined) {
			dam = ApplyResist(victim, GetResistType(spell_id), dam);
		} else {
			dam = ApplyResist(victim, GetResisTypeWithElement(element), dam);
		};
	};

	// учет положения атакующего и жертвы
	// Include a damage multiplier if victim isn't ready to fight:
	// Position sitting  1.5 x normal
	// Position resting  2.0 x normal
	// Position sleeping 2.5 x normal
	// Position stunned  3.0 x normal
	// Position incap    3.5 x normal
	// Position mortally 4.0 x normal
	// Note, this is a hack because it depends on the particular
	// values of the POSITION_XXX constants.

	// физ дамага атакера из сидячего положения
	if (ch_start_pos < EPosition::kFight && dmg_type == fight::kPhysDmg) {
		dam -= dam * (EPosition::kFight - ch_start_pos) / 4;
	}

	// дамаг не увеличивается если:
	// на жертве есть воздушный щит
	// атака - каст моба (в mage_damage увеличение дамага от позиции было только у колдунов)
	if (victim_start_pos < EPosition::kFight
		&& !VictimAffectDeclares(selected_shield_, victim, kAfNoPositionBonus)
		&& !(dmg_type == fight::kMagicDmg
			&& ch->IsNpc())) {
		dam += dam * (EPosition::kFight - victim_start_pos) / 4;
	}

	// прочие множители

	// issue.damage-change: kHold's "held target takes more physical damage" migrated to a data-driven
	// <damage_change> (kWardDamage) on the affect -- applied at the ApplyAffectDamageChanges hook above.
	// Simplified to a flat x1.5 (the old NPC x1.5 / PC x1.25 split was dropped as a pointless distinction).

	if (!victim->IsNpc() && IsCharmice(ch)) {
		dam = dam * 8 / 10;
	}

	if (GET_PR(victim) && dmg_type == fight::kPhysDmg) {
		int ResultDam = dam - (dam * GET_PR(victim) / 100);
		SendToTC(ch, false, true, false,
					   "&CУчет поглощения урона: %d начислено, %d применено.&n\r\n", dam, ResultDam);
		SendToTC(victim, false, true, false,
						   "&CУчет поглощения урона: %d начислено, %d применено.&n\r\n", dam, ResultDam);
		dam = ResultDam;
	}
	if (!privilege::IsImmortal(ch) && VictimFullyAbsorbs(victim)) {
		if (skill_id == ESkill::kBash) {
			SendSkillMessages(dam, ch, victim, skill_id);
		}
		if (ch != victim) {
			act("Магический кокон полностью поглотил удар $N1.", false, victim, nullptr, ch, kToChar);
			act("Магический кокон вокруг $N1 полностью поглотил ваш удар.", false, ch, nullptr, victim, kToChar);
			act("Магический кокон вокруг $N1 полностью поглотил удар $n1.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
		} else {
			act("Магический кокон полностью поглотил повреждения.", false, ch, nullptr, nullptr, kToChar);
			act("Магический кокон вокруг $N1 полностью поглотил повреждения.", true, ch, nullptr, victim, kToNotVict | kToArenaListen);
		}
		return 0;
	}
	// щиты, броня, поглощение
	if (victim != ch) {
		// issue.damage-change: the elemental shields are fully data-driven now (kWardDamage
		// <retaliation>/<damage_change> on the shield affects). Retaliation reads the pre-reduction
		// damage; the late <damage_change> pass applies the one-per-hit reduction (kShieldApplied) and
		// the ice crit-absorb. CalcMagisShieldsDmgAbsoption is retired.
		ApplyRetaliations(ch, victim);
		ApplyAffectDamageChanges(ch, victim, /*late_stage=*/true);
		CalcArmorDmgAbsorption(victim);
		bool armor_full_absorb = CalcDmgAbsorption(ch, victim);
		if (flags[fight::kCritHit] && (GetRealLevel(victim) >= 5 || !ch->IsNpc())
			&& !VictimAffectDeclares(selected_shield_, victim, kAfNoCritBonus)) {
			int tmpdam = std::min(victim->get_real_max_hit() / 8, dam * 2);
			tmpdam = ApplyResist(victim, EResist::kVitality, dam);
			dam = std::max(dam, tmpdam); //крит
		}
		// полное поглощение
		if (armor_full_absorb) {
			return 0;
		}
		if (dam > 0)
			ProcessBlink(ch, victim);
	}

	// Защитная проверка: участники боя могли быть удалены в ходе обработки щитов/поглощения.
	if (!(ch && victim) || (ch->purged() || victim->purged())) {
		log("Death during incoming-damage shield/absorption stage");
		return 0;
	}

	if (victim->IsFlagged(EMobFlag::kProtect)) {
		if (victim != ch) {
			act("$n находится под защитой Богов.", false, victim, nullptr, nullptr, kToRoom);
		}
		return 0;
	}
	// обратка от зеркал/огненного щита
	if (flags[fight::kMagicReflect]) {
		// ограничение для зеркал на 40% от макс хп кастера
		dam = std::min(dam, victim->get_max_hit() * 4 / 10);
		// чтобы не убивало обраткой
		dam = std::min(dam, victim->get_hit() - 1);
	}

	dam = std::clamp(dam, 0, kMaxHits);
	if (dam >= 0) {
		if (dmg_type == fight::kPhysDmg) {
			if (!damage_mtrigger(ch, victim, dam, MUD::Skill(skill_id).GetName(), 1, wielded))
				return 0;
		} else if (dmg_type == fight::kMagicDmg) {
			// spell_id is kUndefined for spell-less magic damage (e.g. mob breath, which
			// is magic melee of an element). Use an empty name rather than looking up an
			// invalid spell.
			const char *dmg_name = (spell_id > ESpell::kUndefined) ? MUD::Spell(spell_id).GetCName() : "";
			if (!damage_mtrigger(ch, victim, dam, dmg_name, 0, wielded))
				return 0;
		} else if (dmg_type == fight::kPoisonDmg) {
			if (!damage_mtrigger(ch, victim, dam, MUD::Spell(spell_id).GetCName(), 2, wielded))
				return 0;
		}
	}
	if (!InTestZone(ch)) {
		experience::gain_battle_exp(ch, victim, dam);
	}

	// real_dam так же идет в обратку от огн.щита
	int real_dam = dam;
	int over_dam = 0;

	if (dam > victim->get_hit() + 11) {
		real_dam = victim->get_hit() + 11;
		over_dam = dam - real_dam;
	}
	// собственно нанесение дамага
	victim->set_hit(victim->get_hit() - dam);
	// Точка инструментации для автономного симулятора баланса (issue #2967):
	// эмитим точные числа на каждый успешный удар. В проде список sink'ов
	// пуст -- HasAnyEventSink() возвращает false, ранний выход без построения
	// Event и без аллокаций.
	if (observability::HasAnyEventSink()) {
		observability::Event ev;
		ev.name = "damage";
		ev.ts_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		ev.attrs["attacker_name"] = observability::EngineStringToUtf8(GET_NAME(ch) ? GET_NAME(ch) : "");
		ev.attrs["victim_name"] = observability::EngineStringToUtf8(GET_NAME(victim) ? GET_NAME(victim) : "");
		ev.attrs["dam"] = static_cast<std::int64_t>(dam);
		ev.attrs["real_dam"] = static_cast<std::int64_t>(real_dam);
		ev.attrs["over_dam"] = static_cast<std::int64_t>(over_dam);
		ev.attrs["victim_hp_after"] = static_cast<std::int64_t>(victim->get_hit());
		ev.attrs["dmg_type"] = static_cast<std::int64_t>(dmg_type);
		ev.attrs["crit"] = flags[fight::kCritHit];
		// spell_id/skill_id выставлены если урон пришёл из заклинания/умения
		// (kUndefined для обычной автоатаки). В виз позволяет отличить
		// melee от cast.
		ev.attrs["spell_id"] = static_cast<std::int64_t>(spell_id);
		ev.attrs["skill_id"] = static_cast<std::int64_t>(skill_id);
		// Чармис/поднятая нежить -- атаковал не сам PC, а его подчинённый.
		// Визуализатору это нужно, чтобы отделить вклад хозяина и слуг.
		ev.attrs["attacker_is_charmie"] = IsCharmice(ch);
		ev.attrs["attacker_master_name"] = observability::EngineStringToUtf8(
			(IsCharmice(ch) && ch->has_master() && GET_NAME(ch->get_master()))
				? GET_NAME(ch->get_master()) : "");
		observability::EmitToAllSinks(ev);
	}
	SendToTC(victim, false, true, true, "&MПолучен урон = %d&n\r\n", dam);
	SendToTC(ch, false, true, true, "&MПрименен урон = %d&n\r\n", dam);
	if (dmg_type == fight::kPhysDmg && GET_GOD_FLAG(ch, EGf::kSkillTester) && skill_id != ESkill::kUndefined) {
		log("SKILLTEST:;%s;skill;%s;damage;%d;Luck;%s", GET_NAME(ch), MUD::Skill(skill_id).GetName(), dam, flags[fight::kCritLuck] ? "yes" : "no");
	}
	// issue.character-affect-triggers: the kVampirism HP-leech moved OUT of the damage pipeline to the
	// affect's data-driven kPostHit <points> heal (affects.xml). (SoulLink master-share dropped for now --
	// re-add as a second action / handler if needed.)
	// запись в дметр фактического и овер дамага
	DpsSystem::UpdateDpsStatistics(ch, real_dam, over_dam);
	// запись дамага в список атакеров
	if (victim->IsNpc()) {
		victim->mark_attacked(ch);
	}
	// попытка спасти жертву через ангела
	CheckTutelarSelfSacrfice(ch, victim);

	// обновление позиции после удара и ангела
	update_pos(victim);
	// если вдруг виктим сдох после этого, то произойдет креш, поэтому вставил тут проверочку
	if (!(ch && victim) || (ch->purged() || victim->purged())) {
		log("Error in fight_hit, function process()\r\n");
		return 0;
	}
	// если у чара есть жатва жизни
	if (CanUseFeat(victim, EFeat::kHarvestOfLife)) {
		if (victim->GetPosition() == EPosition::kDead) {
			int souls = victim->get_souls();
			if (souls >= 10) {
				victim->set_hit(0 + souls * 10);
				update_pos(victim);
				SendMsgToChar("&GДуши спасли вас от смерти!&n\r\n", victim);
				victim->set_souls(0);
			}
		}
	}
	// сбивание надува черноков //
	if (spell_id != ESpell::kPoison && dam > 0 && !flags[fight::kMagicReflect]) {
		TryRemoveExtrahits(ch, victim);
	}

	if (dam && flags[fight::kCritHit] && !dam_critic && spell_id != ESpell::kPoison) {
		SendCritHitMsg(ch, victim);
	}

	// инит Damage::brief_shields_
	if (flags.test(fight::kDrawBriefFireShield)
		|| flags.test(fight::kDrawBriefAirShield)
		|| flags.test(fight::kDrawBriefIceShield)
		|| flags.test(fight::kDrawBriefMagMirror)) {
		char buf_[kMaxInputLength];
		snprintf(buf_, sizeof(buf_), "&n (%s%s%s%s)",
				 flags.test(fight::kDrawBriefFireShield) ? "&R*&n" : "",
				 flags.test(fight::kDrawBriefAirShield) ? "&C*&n" : "",
				 flags.test(fight::kDrawBriefIceShield) ? "&W*&n" : "",
				 flags.test(fight::kDrawBriefMagMirror) ? "&M*&n" : "");
		brief_shields_ = buf_;
	}
	// сообщения об ударах //
	// issue.character-affect-triggers: affect-owned damage flavor (a kDispell trap, a DoT) FULLY
	// replaces the generic combat message AND the char_dam_message severity line -- the affect speaks
	// for itself. $n = ch, $N = victim; a self-damage affect (ch == victim) uses only ToChar/ToRoom.
	if (!aff_msg_char_.empty() || !aff_msg_vict_.empty() || !aff_msg_room_.empty()) {
		if (!aff_msg_char_.empty()) {
			act(aff_msg_char_.c_str(), false, ch, nullptr, victim, kToChar);
		}
		if (ch != victim && !aff_msg_vict_.empty()) {
			act(aff_msg_vict_.c_str(), false, ch, nullptr, victim, kToVict);
		}
		if (!aff_msg_room_.empty()) {
			act(aff_msg_room_.c_str(), true, ch, nullptr, victim, kToNotVict | kToArenaListen);
		}
	} else {
		if (MUD::Skills().IsValid(skill_id)) {
			SendSkillMessages(dam, ch, victim, skill_id, brief_shields_);
		} else if (spell_id > ESpell::kUndefined) {
			SendSkillMessages(dam, ch, victim, spell_id, brief_shields_);
		} else {
			// удар оружием/рукой или серверный урон - всё из контейнера сообщений
			// (для ударов оружием kFightHit* содержит плейсхолдер {intensity}, issue #3322)
			SendSkillMessages(dam, ch, victim, damage_source, brief_shields_);
		}
		/// Use SendMsgToChar -- act() doesn't send message if you are DEAD.
		char_dam_message(dam, ch, victim, flags[fight::kNoFleeDmg]);
	}


	// Проверить, что жертва все еще тут. Может уже сбежала по трусости.
	// Думаю, простой проверки достаточно.
	// Примечание, если сбежал в FIRESHIELD,
	// то обратного повреждения по атакующему не будет
	if (ch->in_room != victim->in_room) {
		return dam;
	}

	// Stop someone from fighting if they're stunned or worse
	/*if ((victim->GetPosition() <= EPosition::kStun)
		&& (victim->GetEnemy() != NULL))
	{
		stop_fighting(victim, victim->GetPosition() <= EPosition::kDead);
	} */

	// жертва умирает //
	if (victim->GetPosition() == EPosition::kDead) {
		ProcessDeath(ch, victim);
		return -1;
	}
	// issue.damage-change: deal the accumulated data-driven thorns (fire/ice/glass retaliation) here, at
	// the post-pipeline point where fire's fs_damage used to be applied -- each as its own kMagicReflect-
	// capped bearer->attacker hit, so the attacker's own defenses transform it.
	DealReflectPool(ch, victim);
	return dam;
}

// * При надуве выше х 1.5 в пк есть 1% того, что весь надув слетит одним ударом.
void TryRemoveExtrahits(CharData *ch, CharData *victim) {
	if (((!ch->IsNpc() && ch != victim)
		|| (ch->has_master()
			&& !ch->get_master()->IsNpc()
			&& ch->get_master() != victim))
		&& !victim->IsNpc()
		&& victim->GetPosition() != EPosition::kDead
		&& victim->get_hit() > victim->get_real_max_hit() * 1.5
		&& number(1, 100) == 5)// пусть будет 5, а то 1 по субъективным ощущениям выпадает как-то часто
	{
		victim->set_hit(victim->get_real_max_hit());
		SendMsgToChar(victim, "%s'Будь%s тощ%s аки прежде' - мелькнула чужая мысль в вашей голове.%s\r\n",
					  kColorWht, grammar::PluralVerbEnding(IsPoly(victim)), grammar::InstrEnding((victim)->get_sex()), kColorNrm);
		act("Вы прервали золотистую нить, питающую $N3 жизнью.", false, ch, nullptr, victim, kToChar);
		act("$n прервал$g золотистую нить, питающую $N3 жизнью.", false, ch, nullptr, victim, kToNotVict | kToArenaListen);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
