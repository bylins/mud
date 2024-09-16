/**
\file mob_memory.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 13.09.2024.
\brief description.
*/

#include "mob_memory.h"

#include "gameplay/ai/mobact.h"
#include "gameplay/fight/fight_hit.h"
#include "engine/entities/char_data.h"
#include "engine/core/handler.h"
#include "gameplay/fight/pk.h"
#include "act_movement.h"
#include "gameplay/fight/fight_constants.h"
#include "gameplay/mechanics/weather.h"

namespace mob_ai {

const int kMobMemKoeff = kSecsPerMudHour;

void mobRemember(CharData *ch, CharData *victim) {
	struct TimedSkill timed{};
	MemoryRecord *tmp;
	bool present = false;

	if (!ch->IsNpc() ||
		victim->IsNpc() ||
		victim->IsFlagged(EPrf::kNohassle) ||
		!ch->IsFlagged(EMobFlag::kMemory) ||
		AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
		if (tmp->id == victim->get_uid()) {
			if (tmp->time > 0)
				tmp->time = time(nullptr) + kMobMemKoeff * GetRealInt(ch);
			present = true;
		}

	if (!present) {
		CREATE(tmp, 1);
		tmp->next = MEMORY(ch);
		tmp->id = victim->get_uid();
		tmp->time = time(nullptr) + kMobMemKoeff * GetRealInt(ch);
		MEMORY(ch) = tmp;
	}
	if (!IsTimedBySkill(victim, ESkill::kHideTrack) && victim->GetSkill(ESkill::kHideTrack)) {
		timed.skill = ESkill::kHideTrack;
		timed.time = ch->GetSkill(ESkill::kTrack) ? 6 : 3;
		ImposeTimedSkill(victim, &timed);
	}
}

// make ch forget victim
void mobForget(CharData *ch, CharData *victim) {
	MemoryRecord *curr, *prev = nullptr;

	// Момент спорный, но думаю, что так правильнее
	if (AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	if (!(curr = MEMORY(ch)))
		return;

	while (curr && curr->id != victim->get_uid()) {
		prev = curr;
		curr = curr->next;
	}

	if (!curr)
		return;        // person wasn't there at all.

	if (curr == MEMORY(ch))
		MEMORY(ch) = curr->next;
	else
		prev->next = curr->next;

	free(curr);
}

// erase ch's memory
// Можно заметить, что функция вызывается только при extract char/mob
// Удаляется все подряд
void clearMemory(CharData *ch) {
	MemoryRecord *curr, *next;

	curr = MEMORY(ch);

	while (curr) {
		next = curr->next;
		free(curr);
		curr = next;
	}
	MEMORY(ch) = nullptr;
}

CharData *FimdRememberedEnemyInRoom(CharData *mob, int check_sneak) {
	if (!mob->mob_specials.memory) {
		return nullptr;
	}

	CharData *victim{nullptr};
	const auto people_copy = world[mob->in_room]->people;
	for (auto vict_i = people_copy.begin(); vict_i != people_copy.end() && !victim; ++vict_i) {
		const auto vict = *vict_i;

		if (vict->IsNpc() || vict->IsFlagged(EPrf::kNohassle)) {
			continue;
		}
		for (MemoryRecord *names = MEMORY(mob); names && !victim; names = names->next) {
			if (names->id == vict->get_uid()) {
				if (!MAY_SEE(mob, mob, vict) || !may_kill_here(mob, vict, NoArgument)) {
					continue;
				}
				if (check_sneak) {
					skip_sneaking(vict, mob);
					if (EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK)) {
						AFF_FLAGS(vict).unset(EAffect::kSneak);
					}
					if (AFF_FLAGGED(vict, EAffect::kSneak))
						continue;
				}
				skip_hiding(vict, mob);
				if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE)) {
					AFF_FLAGS(vict).unset(EAffect::kHide);
				}
				skip_camouflage(vict, mob);
				if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE)) {
					AFF_FLAGS(vict).unset(EAffect::kDisguise);
				}
				if (CAN_SEE(mob, vict)) {
					victim = vict;
				}
			}
		}
	}

	return victim;
}

void AttackToRememberedVictim(CharData *mob, CharData *victim) {
	if (victim && mob->get_wait() <= 0) {
		if (mob->GetPosition() < EPosition::kFight && mob->GetPosition() > EPosition::kSleep) {
			act("$n вскочил$g.", false, mob, nullptr, nullptr, kToRoom);
			mob->SetPosition(EPosition::kStand);
		}
		if (GET_RACE(mob) != ENpcRace::kHuman) {
			act("$n вспомнил$g $N3.", false, mob, nullptr, victim, kToRoom);
		} else {
			act("'$N - ты пытал$U убить меня! Попал$U! Умри!!!', воскликнул$g $n.",
				false, mob, nullptr, victim, kToRoom);
		}
		if (!start_fight_mtrigger(mob, victim)) {
			return;
		}
		if (!attack_best(mob, victim)) {
			hit(mob, victim, ESkill::kUndefined, fight::kMainHand);
		}
	}
}

// запоминание.
// чара-обидчика моб помнит всегда, если он его бьет непосредственно.
// если бьют клоны (проверка на MOB_CLONE), тоже помнит всегда.
// если бьют храны или чармис (все остальные под чармом), то только если
// моб может видеть их хозяина.
void update_mob_memory(CharData *ch, CharData *victim) {
	// первое -- бьют моба, он запоминает обидчика
	if (victim->IsNpc() && victim->IsFlagged(EMobFlag::kMemory)) {
		if (!ch->IsNpc()) {
			mobRemember(victim, ch);
		} else if (AFF_FLAGGED(ch, EAffect::kCharmed)
			&& ch->has_master()
			&& !ch->get_master()->IsNpc()) {
			if (ch->IsFlagged(EMobFlag::kClone)) {
				mobRemember(victim, ch->get_master());
			} else if (ch->get_master()->in_room == victim->in_room
				&& CAN_SEE(victim, ch->get_master())) {
				mobRemember(victim, ch->get_master());
			}
		}
	}

	// второе -- бьет сам моб и запоминает, кого потом добивать :)
	if (ch->IsNpc() && ch->IsFlagged(EMobFlag::kMemory)) {
		if (!victim->IsNpc()) {
			mobRemember(ch, victim);
		} else if (AFF_FLAGGED(victim, EAffect::kCharmed)
			&& victim->has_master()
			&& !victim->get_master()->IsNpc()) {
			if (victim->IsFlagged(EMobFlag::kClone)) {
				mobRemember(ch, victim->get_master());
			} else if (victim->get_master()->in_room == ch->in_room
				&& CAN_SEE(ch, victim->get_master())) {
				mobRemember(ch, victim->get_master());
			}
		}
	}
}

} // namespace mob_ai

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
