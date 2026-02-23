/* ************************************************************************
*   File: mobact.cpp                                    Part of Bylins    *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */
#include "mobact.h"

#include "gameplay/skills/backstab.h"
#include "gameplay/skills/bash.h"
#include "gameplay/skills/strangle.h"
#include "gameplay/skills/chopoff.h"
#include "gameplay/skills/disarm.h"
#include "gameplay/skills/overhelm.h"
#include "gameplay/skills/throw.h"
#include "gameplay/skills/mighthit.h"
#include "gameplay/skills/protect.h"
#include "gameplay/skills/track.h"
#include "gameplay/skills/kick.h"

#include "gameplay/abilities/abilities_rollsystem.h"
#include "engine/core/action_targeting.h"
#include "engine/core/char_movement.h"
#include "engine/db/world_characters.h"
#include "engine/db/world_objects.h"
#include "engine/core/handler.h"
#include "gameplay/magic/magic.h"
#include "gameplay/mechanics/city_guards.h"
#include "gameplay/fight/pk.h"
#include "utils/random.h"
#include "gameplay/clans/house.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_hit.h"
#include "gameplay/ai/mob_memory.h"
#include "gameplay/mechanics/weather.h"
#include "gameplay/classes/pc_classes.h"
#include "gameplay/mechanics/illumination.h"
#include "gameplay/mechanics/hide.h"

// external structs
extern int no_specials;

int npc_scavenge(CharData *ch);
int npc_loot(CharData *ch);
int npc_move(CharData *ch, int dir, int need_specials_check);
void npc_wield(CharData *ch);
void npc_armor(CharData *ch);
void npc_group(CharData *ch);
void npc_groupbattle(CharData *ch);
int npc_walk(CharData *ch);
int npc_steal(CharData *ch);
void npc_light(CharData *ch);
extern void SetWait(CharData *ch, int waittime, int victim_in_room);
void DropObjOnZoneReset(CharData *ch, ObjData *obj, bool inv, bool zone_reset);


namespace mob_ai {

const short kStupidMob{10};
const short kMiddleAi{30};
const short kHighAi{40};
const short kCharacterHpForMobPriorityAttack{100};

struct Target {
    CharData* ch;
    int weight; 
};

// local functions

CharData* weighted_random_choice(const std::vector<Target>& targets) {
    if (targets.empty()) {
        return nullptr;
    }

    int total_weight = 0;
    for (const auto& target : targets) {
        total_weight += target.weight;
    }

    int random = number(1, total_weight);
    int current_weight = 0;
    for (const auto& target : targets) {
        current_weight += target.weight;
        if (random <= current_weight) {
            return target.ch;
        }
    }
    return targets.back().ch; // На случай ошибок
}

int extra_aggressive(CharData *ch, CharData *victim) {
	int time_ok = false, no_time = true, month_ok = false, no_month = true, agro = false;

	if (!ch->IsNpc())
		return (false);

	if (ch->IsFlagged(EMobFlag::kAgressive))
		return (true);

	if (ch->IsFlagged(EMobFlag::kCityGuardian) && city_guards::MustGuardianAttack(ch, victim))
		return (true);

	if (victim && ch->IsFlagged(EMobFlag::kAgressiveMono) && !victim->IsNpc() && GET_RELIGION(victim) == kReligionMono)
		agro = true;

	if (victim && ch->IsFlagged(EMobFlag::kAgressivePoly) && !victim->IsNpc() && GET_RELIGION(victim) == kReligionPoly)
		agro = true;

	if (ch->IsFlagged(EMobFlag::kAgressiveDay)) {
		no_time = false;
		if (weather_info.sunlight == kSunRise || weather_info.sunlight == kSunLight)
			time_ok = true;
	}

	if (ch->IsFlagged(EMobFlag::kAggressiveNight)) {
		no_time = false;
		if (weather_info.sunlight == kSunDark || weather_info.sunlight == kSunSet)
			time_ok = true;
	}

	if (ch->IsFlagged(EMobFlag::kAgressiveWinter)) {
		no_month = false;
		if (weather_info.season == ESeason::kWinter)
			month_ok = true;
	}

	if (ch->IsFlagged(EMobFlag::kAgressiveSpring)) {
		no_month = false;
		if (weather_info.season == ESeason::kSpring)
			month_ok = true;
	}

	if (ch->IsFlagged(EMobFlag::kAgressiveSummer)) {
		no_month = false;
		if (weather_info.season == ESeason::kSummer)
			month_ok = true;
	}

	if (ch->IsFlagged(EMobFlag::kAgressiveAutumn)) {
		no_month = false;
		if (weather_info.season == ESeason::kAutumn)
			month_ok = true;
	}

	if (ch->IsFlagged(EMobFlag::kAgressiveFullmoon)) {
		no_time = false;
		if (weather_info.moon_day >= 12 && weather_info.moon_day <= 15 &&
			(weather_info.sunlight == kSunDark || weather_info.sunlight == kSunSet))
			time_ok = true;
	}
	if (agro || !no_time || !no_month)
		return ((no_time || time_ok) && (no_month || month_ok));
	else
		return (false);
}

int attack_best(CharData *ch, CharData *victim, bool do_mode) {
	ObjData *wielded = GET_EQ(ch, EEquipPos::kWield);

	if (victim) {
		if (ch->GetSkill(ESkill::kStrangle) && !IsTimedBySkill(ch, ESkill::kStrangle) 
				&& !(IS_UNDEAD(victim) 
						|| victim->player_data.Race == ENpcRace::kFish
						|| victim->player_data.Race == ENpcRace::kPlant
						|| victim->player_data.Race == ENpcRace::kConstruct)) {
			if (do_mode)
				do_strangle(ch, victim);
			else
				go_strangle(ch, victim);
			return (true);
		}
		if ((ch->GetSkill(ESkill::kBackstab) && (!victim->GetEnemy() || CanUseFeat(ch, EFeat::kThieveStrike)) && !IS_CHARMICE(ch))
				|| (IS_CHARMICE(ch) && GET_EQ(ch, EEquipPos::kWield) && ch->GetSkill(ESkill::kBackstab)
						&& (!victim->GetEnemy() || CanUseFeat(ch, EFeat::kThieveStrike)))) {

			if (do_mode)
				do_backstab(ch, victim);
			else
				GoBackstab(ch, victim);
			return (true);
		}
		if ((ch->GetSkill(ESkill::kHammer) && !IS_CHARMICE(ch))
			|| (IS_CHARMICE(ch)
				&& !(GET_EQ(ch, EEquipPos::kWield) || GET_EQ(ch, EEquipPos::kBoths) || GET_EQ(ch, EEquipPos::kHold))
				&& ch->GetSkill(ESkill::kHammer))) {
			if (do_mode)
				DoMighthit(ch, victim);
			else
				GoMighthit(ch, victim);
			return (true);
		}
		if (ch->GetSkill(ESkill::kOverwhelm)) {
			if (do_mode)
				DoOverhelm(ch, victim);
			else
				GoOverhelm(ch, victim);
			return (true);
		}
		if (ch->GetSkill(ESkill::kBash)) {
			if (do_mode) {
				do_bash(ch, victim);
			} else
				go_bash(ch, victim);
			return (true);
		}
		if (ch->GetSkill(ESkill::kKick)) {
			if (do_mode)
				do_kick(ch, victim);
			else
				go_kick(ch, victim);
			return (true);
		}
		if (ch->GetSkill(ESkill::kThrow)
			&& wielded
			&& wielded->get_type() == EObjType::kWeapon
			&& wielded->has_flag(EObjFlag::kThrowing)) {
			if (do_mode)
				DoThrow(ch, victim);
			else
				GoThrow(ch, victim);
		}
		if (ch->GetSkill(ESkill::kDisarm)) {
			if (do_mode)
				do_disarm(ch, victim);
			else
				go_disarm(ch, victim);
		}
		if (ch->GetSkill(ESkill::kChopoff)) {
			if (do_mode)
				do_chopoff(ch, victim);
			else
				go_chopoff(ch, victim);
		}
		if (!ch->GetEnemy()) {
			victim = TryToFindProtector(victim, ch);
			hit(ch, victim, ESkill::kUndefined, fight::kMainHand);
		}
		return (true);
	} else
		return (false);
}

#define KILL_FIGHTING   (1 << 0)
#define CHECK_HITS      (1 << 10)
#define SKIP_HIDING     (1 << 11)
#define SKIP_CAMOUFLAGE (1 << 12)
#define SKIP_SNEAKING   (1 << 13)
#define CHECK_OPPONENT  (1 << 14)
#define GUARD_ATTACK    (1 << 15)

int check_room_tracks(const RoomRnum room, const long victim_id) {
	for (auto track = world[room]->track; track; track = track->next) {
		if (track->who == victim_id) {
			for (int i = 0; i < EDirection::kMaxDirNum; i++) {
				if (IS_SET(track->time_outgone[i], 7)) {
					return i;
				}
			}
		}
	}

	return kBfsError;
}

int find_door(CharData *ch, const bool track_method) {
	bool msg = false;

	for (const auto &vict : character_list) {
		if (CAN_SEE(ch, vict) && vict->in_room != kNowhere) {
			for (auto names = MEMORY(ch); names; names = names->next) {
				if (vict->get_uid() == names->id
					&& (!ch->IsFlagged(EMobFlag::kStayZone)
						|| world[ch->in_room]->zone_rn == world[vict->in_room]->zone_rn)) {
					if (!msg) {
						msg = true;
						act("$n начал$g внимательно искать чьи-то следы.",
							false, ch, nullptr, nullptr, kToRoom);
					}

					const auto door = track_method
									  ? check_room_tracks(ch->in_room, vict->get_uid())
									  : go_track(ch, vict.get(), ESkill::kTrack);

					if (kBfsError != door) {
						return door;
					}
				}
			}
		}
	}

	return kBfsError;
}

int npc_track(CharData *ch) {
	const auto result = find_door(ch, GetRealInt(ch) < number(15, 20));

	return result;
}

CharData *selectRandomSkirmisherFromGroup(CharData *leader) {
	ActionTargeting::FriendsRosterType roster{leader};
	auto isSkirmisher = [](CharData *ch) { return ch->IsFlagged(EPrf::kSkirmisher); };
	int skirmishers = roster.count(isSkirmisher);
	if (skirmishers == 0 || skirmishers == roster.amount()) {
		return nullptr;
	}
	return roster.getRandomItem(isSkirmisher);
}

CharData *selectVictimDependingOnGroupFormation(CharData *assaulter, CharData *initialVictim) {
	if ((initialVictim == nullptr) || !AFF_FLAGGED(initialVictim, EAffect::kGroup)
		|| assaulter->IsFlagged(EMobFlag::kIgnoresFormation)) {
		return initialVictim;
	}

	CharData *leader = initialVictim;
	CharData *newVictim = initialVictim;

	if (initialVictim->has_master()) {
		leader = initialVictim->get_master();
	}
	if (!assaulter->isInSameRoom(leader)) {
		return initialVictim;
	}

	newVictim = selectRandomSkirmisherFromGroup(leader);
	if (!newVictim) {
		return initialVictim;
	}

	abilities_roll::AgainstRivalRoll abilityRoll;
	abilityRoll.Init(leader, abilities::EAbility::kTactician, assaulter);
	bool tacticianFail = !abilityRoll.IsSuccess();
	abilities_roll::AgainstRivalRoll abilityRoll2;
	abilityRoll2.Init(newVictim, abilities::EAbility::kScirmisher, assaulter);
	if (tacticianFail || !abilityRoll2.IsSuccess()) {
		return initialVictim;
	}

	act("Вы героически приняли удар $n1 на себя!",
		false, assaulter, nullptr, newVictim, kToVict | kToNoBriefShields);
	act("$n попытал$u ворваться в ваши ряды, но $N героически принял$G удар на себя!",
		false, assaulter, nullptr, newVictim, kToNotVict | kToNoBriefShields);
	return newVictim;
}

CharData *find_best_stupidmob_victim(CharData *ch, int extmode) {
	CharData *victim, *use_light = nullptr, *min_hp = nullptr, *min_lvl = nullptr, *caster = nullptr, *best = nullptr;
	int kill_this, extra_aggr = 0;

	victim = ch->GetEnemy();

	for (const auto vict : world[ch->in_room]->people) {
		if ((vict->IsNpc() && !IS_SET(extmode, CHECK_OPPONENT) && !IS_CHARMICE(vict))
			|| (IS_CHARMICE(vict) && !vict->GetEnemy()) // чармиса агрим только если он уже с кем-то сражается
			|| vict->IsFlagged(EPrf::kNohassle)
			|| !MAY_SEE(ch, ch, vict)
			|| (IS_SET(extmode, CHECK_OPPONENT) && ch != vict->GetEnemy())
			|| (!may_kill_here(ch, vict, NoArgument) && !IS_SET(extmode, GUARD_ATTACK)))//старжники агрят в мирках
		{
			continue;
		}

		kill_this = false;

		// Mobile too damage //обработка флага ТРУС
		if (IS_SET(extmode, CHECK_HITS)
			&& ch->IsFlagged(EMobFlag::kWimpy)
			&& AWAKE(vict)
			&& ch->get_hit() * 2 < ch->get_real_max_hit()) {
			continue;
		}

		// Mobile helpers... //ассист
		if (IS_SET(extmode, KILL_FIGHTING)
			&& vict->GetEnemy()
			&& vict->GetEnemy() != ch
			&& vict->GetEnemy()->IsNpc()
			&& !AFF_FLAGGED(vict->GetEnemy(), EAffect::kCharmed)
			&& SAME_ALIGN(ch, vict->GetEnemy())) {
			kill_this = true;
		} else {
			// ... but no aggressive for this char
			if (!(extra_aggr = extra_aggressive(ch, vict))
				&& !IS_SET(extmode, GUARD_ATTACK)) {
				continue;
			}
		}

		// skip sneaking, hiding and camouflaging pc
		if (IS_SET(extmode, SKIP_SNEAKING)) {
			SkipSneaking(vict, ch);
			if ((EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK))) {
				AFF_FLAGS(vict).unset(EAffect::kSneak);
			}
			if (AFF_FLAGGED(vict, EAffect::kSneak))
				continue;
		}

		if (IS_SET(extmode, SKIP_HIDING)) {
			SkipHiding(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE)) {
				AFF_FLAGS(vict).unset(EAffect::kHide);
			}
		}

		if (IS_SET(extmode, SKIP_CAMOUFLAGE)) {
			SkipCamouflage(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE)) {
				AFF_FLAGS(vict).unset(EAffect::kDisguise);
			}
		}
		if (!CAN_SEE(ch, vict)) {
			continue;
		}
		// Mobile aggresive
		if (!kill_this && extra_aggr) {
			if (CanUseFeat(vict, EFeat::kSilverTongue)) {
				const int number1 = number(1, GetRealLevel(vict) * GetRealCha(vict));
				const int range = ((GetRealLevel(ch) > 30)
								   ? (GetRealLevel(ch) * 2 * GetRealInt(ch) + GetRealInt(ch) * 20)
								   : (GetRealLevel(ch) * GetRealInt(ch)));
				const int number2 = number(1, range);
				const bool do_continue = number1 > number2;
				if (do_continue) {
					continue;
				}
			}
			kill_this = true;
		}

		if (!kill_this)
			continue;

		// define victim if not defined
		if (!victim)
			victim = vict;

		if (IsDefaultDark(ch->in_room)
			&& ((GET_EQ(vict, EObjType::kLightSource)
				&& GET_OBJ_VAL(GET_EQ(vict, EObjType::kLightSource), 2))
				|| (!AFF_FLAGGED(vict, EAffect::kHolyDark)
					&& (AFF_FLAGGED(vict, EAffect::kSingleLight)
						|| AFF_FLAGGED(vict, EAffect::kHolyLight))))
			&& (!use_light
				|| GetRealCha(use_light) > GetRealCha(vict))) {
			use_light = vict;
		}

		if (!min_hp
			|| vict->get_hit() + GetRealCha(vict) * 10 < min_hp->get_hit() + GetRealCha(min_hp) * 10) {
			min_hp = vict;
		}

		if (!min_lvl
			|| GetRealLevel(vict) + number(1, GetRealCha(vict))
				< GetRealLevel(min_lvl) + number(1, GetRealCha(min_lvl))) {
			min_lvl = vict;
		}

		if (IsCaster(vict) &&
			(!caster || caster->caster_level * GetRealCha(vict) < GetRealCha(caster) * vict->caster_level)) {
			caster = vict;
		}
	}

	if (GetRealInt(ch) < 5 + number(1, 6))
		best = victim;
	else if (GetRealInt(ch) < 10 + number(1, 6))
		best = use_light ? use_light : victim;
	else if (GetRealInt(ch) < 15 + number(1, 6))
		best = min_lvl ? min_lvl : (use_light ? use_light : victim);
	else if (GetRealInt(ch) < 25 + number(1, 6))
		best = caster ? caster : (min_lvl ? min_lvl : (use_light ? use_light : victim));
	else
		best = min_hp ? min_hp : (caster ? caster : (min_lvl ? min_lvl : (use_light ? use_light : victim)));

	if (best && !ch->GetEnemy() && ch->IsFlagged(EMobFlag::kAgressiveMono) &&
		!best->IsNpc() && GET_RELIGION(best) == kReligionMono) {
		act("$n закричал$g: 'Умри, христианская собака!' и набросил$u на вас.", false, ch, nullptr, best, kToVict);
		act("$n закричал$g: 'Умри, христианская собака!' и набросил$u на $N3.", false, ch, nullptr, best, kToNotVict);
	}

	if (best && !ch->GetEnemy() && ch->IsFlagged(EMobFlag::kAgressivePoly) &&
		!best->IsNpc() && GET_RELIGION(best) == kReligionPoly) {
		act("$n закричал$g: 'Умри, грязный язычник!' и набросил$u на вас.", false, ch, nullptr, best, kToVict);
		act("$n закричал$g: 'Умри, грязный язычник!' и набросил$u на $N3.", false, ch, nullptr, best, kToNotVict);
	}

	return selectVictimDependingOnGroupFormation(ch, best);
}
// TODO invert and rename for clarity: -> isStrayCharmice(), to return true if a charmice, and master is absent =II
bool find_master_charmice(CharData *charmice) {
	// проверяем на спелл чарма, ищем хозяина и сравниваем румы
	if (!IS_CHARMICE(charmice) || !charmice->has_master()) {
		return true;
	}

	if (charmice->in_room == charmice->get_master()->in_room) {
		return true;
	}

	return false;
}

bool filter_victim (CharData *ch, CharData *vict, int extmode) {
	bool kill_this = false;
	if ((vict->IsNpc() && !IS_CHARMICE(vict))
		|| (IS_CHARMICE(vict) && !vict->GetEnemy()
			&& find_master_charmice(vict)) // чармиса агрим только если нет хозяина в руме.
		|| vict->IsFlagged(EPrf::kNohassle)
		|| !MAY_SEE(ch, ch, vict) // если не видим цель,
		|| (IS_SET(extmode, CHECK_OPPONENT) && ch != vict->GetEnemy())
		|| (!may_kill_here(ch, vict, NoArgument) && !IS_SET(extmode, GUARD_ATTACK)))//старжники агрят в мирках
	{
		return false;
	}

	// Mobile too damage //обработка флага ТРУС
	if (IS_SET(extmode, CHECK_HITS)
		&& ch->IsFlagged(EMobFlag::kWimpy)
		&& AWAKE(vict) && ch->get_hit() * 2 < ch->get_real_max_hit()) {
		return false;
	}

	int extra_aggr = extra_aggressive(ch, vict);
	// Mobile helpers... //ассист
	if ((vict->GetEnemy())
		&& (vict->GetEnemy() != ch)
		&& vict->GetEnemy()->IsNpc()
		&& (!AFF_FLAGGED(vict->GetEnemy(), EAffect::kCharmed))) {
		kill_this = true;
	} else {
		// ... but no aggressive for this char
		if (!extra_aggr && !IS_SET(extmode, GUARD_ATTACK)) {
			return false;
		}
	}
	if (IS_SET(extmode, SKIP_SNEAKING)) {
		SkipSneaking(vict, ch);
		if (EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK)) {
			AFF_FLAGS(vict).unset(EAffect::kSneak);
		}

		if (AFF_FLAGGED(vict, EAffect::kSneak)) {
			return false;
		}
	}
	if (IS_SET(extmode, SKIP_HIDING)) {
		SkipHiding(vict, ch);
		if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE)) {
			AFF_FLAGS(vict).unset(EAffect::kHide);
		}
	}

	if (IS_SET(extmode, SKIP_CAMOUFLAGE)) {
		SkipCamouflage(vict, ch);
		if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE)) {
			AFF_FLAGS(vict).unset(EAffect::kDisguise);
		}
	}
	if (!CAN_SEE(ch, vict)) {
		return false;
	}
	if (!kill_this && extra_aggr) {
		if (CanUseFeat(vict, EFeat::kSilverTongue)
			&& number(1, GetRealLevel(vict) * GetRealCha(vict)) > number(1, GetRealLevel(ch) * GetRealInt(ch))) {
			return false;
		}
		kill_this = true;
	}
	return kill_this;
}

CharData *find_best_mob_victim(CharData *ch, int extmode) {
	int mobINT = GetRealInt(ch);
	if (mobINT < kStupidMob) {
		return find_best_stupidmob_victim(ch, extmode);
	}

	CharData *currentVictim = ch->GetEnemy();
  std::vector<Target> casters, druids, clers, charmmages, others;

	if (currentVictim && !currentVictim->IsNpc()) {
		if (IsCaster(currentVictim)) {
			return currentVictim;
		}
	}
	// проходим по всем чарам в комнате
	int base_weight = 10; // Базовый вес
	for (const auto vict : world[ch->in_room]->people) {
		if (!filter_victim(ch, vict, extmode)) {
			continue;
		}

		if (vict->get_hit() <= kCharacterHpForMobPriorityAttack) {
			return selectVictimDependingOnGroupFormation(ch, vict);
		}
		// Распределяем цели по категориям
		if (vict->GetClass() == ECharClass::kMagus) {
			druids.push_back({vict, base_weight});
		} else if (vict->GetClass() == ECharClass::kSorcerer) {
			clers.push_back({vict, base_weight});
		} else if (vict->GetClass() == ECharClass::kCharmer) {
			charmmages.push_back({vict, base_weight});
		} else if (IsCaster(vict)) {
			casters.push_back({vict, base_weight});
		} else {
			others.push_back({vict, base_weight});
		}
	}

// Определяем веса в зависимости от интеллекта моба
	int druid_weight = 0;
	int cler_weight = 0;
	int charmmage_weight = 0;
	int caster_weight = 0, other_weight = base_weight;

	if (mobINT < kMiddleAi) {
		// Глупый моб: равномерные веса
	} else if (mobINT < kHighAi) {
		druid_weight = 50;
		cler_weight = 50;
		charmmage_weight = 30;
		caster_weight = 30;
		other_weight = 10;
	} else {
		druid_weight = 900;
		cler_weight = 400;
		charmmage_weight = 200;
		caster_weight = 125;
		other_weight = 25;
	}

	std::vector<Target> all_targets;
	for (auto& target : druids) {
		target.weight = druid_weight;
		all_targets.push_back(target);
	}
	for (auto& target : clers) {
		target.weight = cler_weight;
		all_targets.push_back(target);
	}
	for (auto& target : charmmages) {
		target.weight = charmmage_weight;
		all_targets.push_back(target);
	}
	for (auto& target : casters) {
		target.weight = caster_weight;
		all_targets.push_back(target);
	}
	for (auto& target : others) {
		target.weight = other_weight;
		all_targets.push_back(target);
	}
	for (auto& target : all_targets) {
		if (target.ch->IsLeader()) {
			target.weight = target.weight * 3 / 2;
		}
	}

	CharData* selected = weighted_random_choice(all_targets);
	if (!selected) {
		selected = currentVictim; 
	}
	return selectVictimDependingOnGroupFormation(ch, selected);
}

int perform_best_mob_attack(CharData *ch, int extmode) {
	CharData *best;
	int clone_number = 0;
	best = find_best_mob_victim(ch, extmode);

	if (best) {
		// если у игрока стоит олц на зону, в ней его не агрят
		if (best->player_specials->saved.olc_zone == GET_MOB_VNUM(ch) / 100) {
			SendMsgToChar(best, "&GАгромоб, атака остановлена.\r\n");
			return (false);
		}
		if (ch->GetPosition() < EPosition::kFight && ch->GetPosition() > EPosition::kSleep) {
			act("$n вскочил$g.", false, ch, nullptr, nullptr, kToRoom);
			ch->SetPosition(EPosition::kStand);
		}

		if (IS_SET(extmode, KILL_FIGHTING) && best->GetEnemy()) {
			if (best->GetEnemy()->IsFlagged(EMobFlag::kNoHelp))
				return (false);
			act("$n вступил$g в битву на стороне $N1.", false, ch, nullptr, best->GetEnemy(), kToRoom);
		}

		if (IS_SET(extmode, GUARD_ATTACK)) {
			act("'$N - за грехи свои ты заслуживаешь смерти!', сурово проговорил$g $n.",
				false, ch, nullptr, best, kToRoom);
			act("'Как страж этого города, я намерен$g привести приговор в исполнение немедленно. Защищайся!'",
				false, ch, nullptr, best, kToRoom);
		}

		if (!best->IsNpc()) {
			struct FollowerType *f;
			// поиск клонов и отработка атаки в клона персонажа
			for (f = best->followers; f; f = f->next)
				if (f->follower->IsFlagged(EMobFlag::kClone))
					clone_number++;
			for (f = best->followers; f; f = f->next)
				if (f->follower->IsNpc() && f->follower->IsFlagged(EMobFlag::kClone)
					&& f->follower->in_room == best->in_room) {
					if (number(0, clone_number) == 1)
						break;
					if ((GetRealInt(ch) < 20) && number(0, clone_number))
						break;
					if (GetRealInt(ch) >= 30)
						break;
					if ((GetRealInt(ch) >= 20)
						&& number(1, 10 + VPOSI((35 - GetRealInt(ch)), 0, 15) * clone_number) <= 10)
						break;
					best = f->follower;
					break;
				}
		}
		if (!start_fight_mtrigger(ch, best)) {
			return false;
		}
		if (!attack_best(ch, best) && !ch->GetEnemy()) {
			hit(ch, best, ESkill::kUndefined, fight::kMainHand);
		}
		return (true);
	}
	return (false);
}

int perform_best_horde_attack(CharData *ch, int extmode) {
	if (perform_best_mob_attack(ch, extmode)) {
		return (true);
	}

	for (const auto vict : world[ch->in_room]->people) {
		if (!vict->IsNpc() || !MAY_SEE(ch, ch, vict) || vict->IsFlagged(EMobFlag::kProtect) || vict->IsFlagged(EMobFlag::kNoFight)) {
			continue;
		}
		if (!SAME_ALIGN(ch, vict)) {
			if (ch->GetPosition() < EPosition::kFight && ch->GetPosition() > EPosition::kSleep) {
				act("$n вскочил$g.", false, ch, nullptr, nullptr, kToRoom);
				ch->SetPosition(EPosition::kStand);
			}
			if (!start_fight_mtrigger(ch, vict)) {
				return false;
			}
			if (!attack_best(ch, vict) && !ch->GetEnemy()) {
				hit(ch, vict, ESkill::kUndefined, fight::kMainHand);
			}
			return (true);
		}
	}
	return (false);
}

int perform_mob_switch(CharData *ch) {
	CharData *best;
	best = find_best_mob_victim(ch, SKIP_HIDING | SKIP_CAMOUFLAGE | SKIP_SNEAKING | CHECK_OPPONENT);

	if (!best)
		return false;

	best = TryToFindProtector(best, ch);
	if (best == ch->GetEnemy())
		return false;

	// переключаюсь на best
	stop_fighting(ch, false);
	SetFighting(ch, best);
	SetWait(ch, 2, false);

	if (ch->GetSkill(ESkill::kHammer) && IsArmedWithMighthitWeapon(ch)) {
		ch->battle_affects.set(kEafHammer);
	} else if (ch->GetSkill(ESkill::kOverwhelm)) {
		ch->battle_affects.set(ESkill::kOverwhelm);
	}

	return true;
}

void do_aggressive_mob(CharData *ch, int check_sneak) {
	if (!ch || ch->in_room == kNowhere || !ch->IsNpc() || !MAY_ATTACK(ch) || AFF_FLAGGED(ch, EAffect::kBlind)) {
		return;
	}

	int mode = check_sneak ? SKIP_SNEAKING : 0;
	// ****************  Horde
	if (ch->IsFlagged(EMobFlag::kHorde)) {
		perform_best_horde_attack(ch, mode | SKIP_HIDING | SKIP_CAMOUFLAGE);
		return;
	}

	// ****************  Aggressive Mobs
	if (extra_aggressive(ch, nullptr)) {
		const auto &room = world[ch->in_room];
		for (auto affect_it = room->affected.begin(); affect_it != room->affected.end(); ++affect_it) {
			if (affect_it->get()->type == ESpell::kRuneLabel && (affect_it != room->affected.end())) {
				act("$n шаркнул$g несколько раз по светящимся рунам, полностью их уничтожив.",
					false,
					ch,
					nullptr,
					nullptr,
					kToRoom | kToArenaListen);
				room_spells::RoomRemoveAffect(world[ch->in_room], affect_it);
				break;
			}
		}
		perform_best_mob_attack(ch, mode | SKIP_HIDING | SKIP_CAMOUFLAGE | CHECK_HITS);
		return;
	}

	if (ch->IsFlagged(EMobFlag::kCityGuardian)) {
		perform_best_mob_attack(ch, SKIP_HIDING | SKIP_CAMOUFLAGE | SKIP_SNEAKING | GUARD_ATTACK);
		return;
	}

	if (ch->IsFlagged(EMobFlag::kMemory)) {
		auto victim = FimdRememberedEnemyInRoom(ch, check_sneak);
		AttackToRememberedVictim(ch, victim);
		return;
	}

	// ****************  Helper Mobs
	if (ch->IsFlagged(EMobFlag::kHelper)) {
		perform_best_mob_attack(ch, mode | KILL_FIGHTING | CHECK_HITS);
		return;
	}
}

/**
* Примечание: сам ch после этой функции уже может быть спуржен
* в результате агра на себя кого-то в комнате и начале атаки
* например с глуша.
*/
void do_aggressive_room(CharData *ch, int check_sneak) {
	if (!ch || ch->in_room == kNowhere) {
		return;
	}

	const auto people =
		world[ch->in_room]->people;    // сделать копию people, т. к. оно может измениться в теле цикла и итераторы будут испорчены
	for (const auto &vict : people) {
		// здесь не надо преварително запоминать next_in_room, потому что как раз
		// он то и может быть спуржен по ходу do_aggressive_mob, а вот атакующий нет
		do_aggressive_mob(vict, check_sneak);
	}
}

/**
 * Проверка на наличие в комнате мобов с таким же спешиалом, что и входящий.
 * \param ch - входящий моб
 * \return true - можно войти, false - нельзя
 */
bool allow_enter(RoomData *room, CharData *ch) {
	if (!ch->IsNpc() || !GET_MOB_SPEC(ch)) {
		return true;
	}

	for (const auto vict : room->people) {
		if (vict->IsNpc()
			&& GET_MOB_SPEC(vict) == GET_MOB_SPEC(ch)) {
			return false;
		}
	}

	return true;
}


void mobile_activity(int activity_level, int missed_pulses) {
//	int door, max, was_in = -1, activity_lev, i, ch_activity;
//	int std_lev = activity_level % kPulseMobile;

	for (auto &ch : character_list) {
	  int door, max, was_in = -1, activity_lev, i, ch_activity;
	  auto std_lev = activity_level % kPulseMobile;

	  if (ch->purged()  || !IS_MOB(ch) || !ch->in_used_zone()) {
		continue;
	  }
	  UpdateAffectOnPulse(ch.get(), missed_pulses);
	  if (ch->punctual_wait > 0)
		  ch->punctual_wait -= missed_pulses;
	  else
		  ch->punctual_wait = 0;

	  if (ch->punctual_wait < 0)
		  ch->punctual_wait = 0;

	  if (ch->mob_specials.speed <= 0) {
		  activity_lev = std_lev;
	  } else {
		  activity_lev = activity_level % (ch->mob_specials.speed * kRealSec);
	  }

	  ch_activity = GET_ACTIVITY(ch);

// на случай вызова mobile_activity() не каждый пульс
	  // TODO: by WorM а где-то используется это mob_specials.speed ???
	  if (ch_activity - activity_lev < missed_pulses && ch_activity - activity_lev >= 0) {
		  ch_activity = activity_lev;
	  }
	  if (ch_activity != activity_lev
		  || (was_in = ch->in_room) == kNowhere
		  || GET_ROOM_VNUM(ch->in_room) % 100 == 99) {
		  continue;
	  }

	  // Examine call for special procedure
	  if (ch->IsFlagged(EMobFlag::kSpec) && !no_specials) {
		  if (mob_index[GET_MOB_RNUM(ch)].func == nullptr) {
			  log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
				  GET_NAME(ch), GET_MOB_VNUM(ch));
			  ch->UnsetFlag(EMobFlag::kSpec);
		  } else {
			  buf2[0] = '\0';
			  if ((mob_index[GET_MOB_RNUM(ch)].func)(ch.get(), ch.get(), 0, buf2)) {
				  continue;    // go to next char
			  }
		  }
	  }
	  // Extract free horses
	if (AFF_FLAGGED(ch, EAffect::kHorse) && ch->IsFlagged(EMobFlag::kMounting) && !ch->has_master()) {
		act("Возникший как из-под земли цыган ловко вскочил на $n3 и унесся прочь.",
		false, ch.get(), nullptr, nullptr, kToRoom);
		character_list.AddToExtractedList(ch.get());
		continue;
	}
	  // Extract uncharmed mobs
	  if (ch->extract_timer > 0) {
		  if (ch->has_master()) {
			  ch->extract_timer = 0;
		  } else {
			  --(ch->extract_timer);
			  if (!(ch->extract_timer)) {
				  extract_charmice(ch.get(), true);
				  continue;
			  }
		  }
	  }
	  // If the mob has no specproc, do the default actions
	  if (ch->GetEnemy() ||
		  ch->GetPosition() <= EPosition::kStun ||
		  ch->get_wait() > 0 ||
		  AFF_FLAGGED(ch, EAffect::kCharmed) ||
		  AFF_FLAGGED(ch, EAffect::kHold) || AFF_FLAGGED(ch, EAffect::kMagicStopFight) ||
		  AFF_FLAGGED(ch, EAffect::kStopFight) || AFF_FLAGGED(ch, EAffect::kSleep)) {
		  continue;
	  }

	  if (IS_HORSE(ch)) {
		  if (ch->GetPosition() < EPosition::kFight) {
			  ch->SetPosition(EPosition::kStand);
		  }

		  continue;
	  }

	  if (ch->GetPosition() == EPosition::kSleep && GET_DEFAULT_POS(ch) > EPosition::kSleep) {
		  ch->SetPosition(GET_DEFAULT_POS(ch));
		  act("$n проснул$u.", false, ch.get(), nullptr, nullptr, kToRoom);
	  }

	  if (!AWAKE(ch)) {
		  continue;
	  }

	  max = false;
	  bool found = false;
	  for (const auto vict : world[ch->in_room]->people) {
		  if (ch.get() == vict) {
			  continue;
		  }

		  if (vict->GetEnemy() == ch.get()) {
			  continue;        // Mob is under attack
		  }

		  if (!vict->IsNpc()
			  && CAN_SEE(ch, vict)) {
			  max = true;
		  }
	  }

	  // Mob attemp rest if it is not an angel
	  if (!max && !ch->IsFlagged(EMobFlag::kNoRest) 
		  && !ch->IsFlagged(EMobFlag::kHorde) 
		  && ch->get_hit() < ch->get_real_max_hit() 
		  && !ch->IsFlagged(EMobFlag::kTutelar)
		  && !ch->IsFlagged(EMobFlag::kMentalShadow)
		  && !ch->IsOnHorse()
		  && ch->GetPosition() > EPosition::kRest) {
		  act("$n присел$g отдохнуть.", false, ch.get(), nullptr, nullptr, kToRoom);
		  ch->SetPosition(EPosition::kRest);
	  }

	  // Mob continue to default pos if full rested or if it is an angel
	  if ((ch->get_hit() >= ch->get_real_max_hit()
		  && ch->GetPosition() != GET_DEFAULT_POS(ch))
		  || ((ch->IsFlagged(EMobFlag::kTutelar)
			  || ch->IsFlagged(EMobFlag::kMentalShadow))
			  && ch->GetPosition() != GET_DEFAULT_POS(ch))) {
		  switch (GET_DEFAULT_POS(ch)) {
			  case EPosition::kStand: act("$n поднял$u.", false, ch.get(), nullptr, nullptr, kToRoom);
				  ch->SetPosition(EPosition::kStand);
				  break;
			  case EPosition::kSit: act("$n сел$g.", false, ch.get(), nullptr, nullptr, kToRoom);
				  ch->SetPosition(EPosition::kSit);
				  break;
			  case EPosition::kRest: act("$n присел$g отдохнуть.", false, ch.get(), nullptr, nullptr, kToRoom);
				  ch->SetPosition(EPosition::kRest);
				  break;
			  case EPosition::kSleep: act("$n уснул$g.", false, ch.get(), nullptr, nullptr, kToRoom);
				  ch->SetPosition(EPosition::kSleep);
				  break;
			  default: break;
		  }
	  }
	  // continue, if the mob is an angel
	  // если моб ментальная тень или ангел он не должен проявлять активность
	  if ((ch->IsFlagged(EMobFlag::kTutelar))
		  || (ch->IsFlagged(EMobFlag::kMentalShadow))) {
		  continue;
	  }

	  // look at room before moving
	  do_aggressive_mob(ch.get(), false);

	  // if mob attack something
	  if (ch->GetEnemy()
		  || ch->get_wait() > 0) {
		  continue;
	  }

	  // Scavenger (picking up objects)
	  // От одного до трех предметов за раз
	  i = number(1, 3);
	  while (i) {
		  npc_scavenge(ch.get());
		  i--;
	  }

	  if (ch->extract_timer == 0) {
		  //чармисы, собирающиеся уходить - не лутят! (Купала)
		  //Niker: LootCR// Start
		  //Не уверен, что рассмотрены все случаи, когда нужно снимать флаги с моба
		  //Реализация для лута и воровства
		  int grab_stuff = false;
		  // Looting the corpses

		  grab_stuff += npc_loot(ch.get());
		  grab_stuff += npc_steal(ch.get());

		  if (grab_stuff) {
			  ch->UnsetFlag(EMobFlag::kAppearsDay);    //Взял из make_horse
			  ch->UnsetFlag(EMobFlag::kAppearsNight);
			  ch->UnsetFlag(EMobFlag::kAppearsFullmoon);
			  ch->UnsetFlag(EMobFlag::kAppearsWinter);
			  ch->UnsetFlag(EMobFlag::kAppearsSpring);
			  ch->UnsetFlag(EMobFlag::kAppearsSummer);
			  ch->UnsetFlag(EMobFlag::kAppearsAutumn);
		  }
		  //Niker: LootCR// End
	  }
	  npc_wield(ch.get());
	  npc_armor(ch.get());

	  if (ch->GetPosition() == EPosition::kStand && NPC_FLAGGED(ch, ENpcFlag::kInvis)) {
		  ch->set_affect(EAffect::kInvisible);
	  }

	  if (ch->GetPosition() == EPosition::kStand && NPC_FLAGGED(ch, ENpcFlag::kMoveFly)) {
		  ch->set_affect(EAffect::kFly);
	  }

	  if (ch->GetPosition() == EPosition::kStand && NPC_FLAGGED(ch, ENpcFlag::kSneaking)) {
		  if (CalcCurrentSkill(ch.get(), ESkill::kSneak, nullptr) >= number(0, 100)) {
			  ch->set_affect(EAffect::kSneak);
		  } else {
			  ch->remove_affect(EAffect::kSneak);
		  }
		  affect_total(ch.get());
	  }

	  if (ch->GetPosition() == EPosition::kStand && NPC_FLAGGED(ch, ENpcFlag::kDisguising)) {
		  if (CalcCurrentSkill(ch.get(), ESkill::kDisguise, nullptr) >= number(0, 100)) {
			  ch->set_affect(EAffect::kDisguise);
		  } else {
			  ch->remove_affect(EAffect::kDisguise);
		  }

		  affect_total(ch.get());
	  }

	  door = kBfsError;

	  // Helpers go to some dest
	  if (ch->IsFlagged(EMobFlag::kHelper)
		  && !ch->IsFlagged(EMobFlag::kSentinel)
		  && !AFF_FLAGGED(ch, EAffect::kBlind)
		  && !ch->has_master()
		  && ch->GetPosition() == EPosition::kStand) {
		  for (found = false, door = 0; door < EDirection::kMaxDirNum; door++) {
			  RoomData::exit_data_ptr rdata = EXIT(ch, door);
			  if (!rdata
				  || rdata->to_room() == kNowhere
				  || !IsCorrectDirection(ch.get(), door, true, false)
				  || (is_room_forbidden(world[rdata->to_room()])
					  && !ch->IsFlagged(EMobFlag::kIgnoreForbidden))
				  || is_dark(rdata->to_room())
				  || (ch->IsFlagged(EMobFlag::kStayZone)
					  && world[ch->in_room]->zone_rn != world[rdata->to_room()]->zone_rn)) {
				  continue;
			  }

			  const auto room = world[rdata->to_room()];
			  for (auto first : room->people) {
				  if (first->IsNpc()
					  && !AFF_FLAGGED(first, EAffect::kCharmed)
					  && !IS_HORSE(first)
					  && CAN_SEE(ch, first)
					  && first->GetEnemy()
					  && SAME_ALIGN(ch, first)) {
					  found = true;
					  break;
				  }
			  }

			  if (found) {
				  break;
			  }
		  }

		  if (!found) {
			  door = kBfsError;
		  }
	  }

	  if (GET_DEST(ch) != kNowhere
		  && ch->GetPosition() > EPosition::kFight
		  && door == kBfsError) {
		  npc_group(ch.get());
		  door = npc_walk(ch.get());
	  }

	  if (MEMORY(ch) && door == kBfsError && ch->GetPosition() > EPosition::kFight && ch->GetSkill(ESkill::kTrack))
		  door = npc_track(ch.get());

	  if (door == kBfsAlreadyThere) {
		  do_aggressive_mob(ch.get(), false);
		  continue;
	  }

	  if (door == kBfsError) {
		  door = number(0, 18);
	  }

	  // Mob Movement
	  if (!ch->IsFlagged(EMobFlag::kSentinel)
		  && ch->GetPosition() == EPosition::kStand
		  && (door >= 0 && door < EDirection::kMaxDirNum)
		  && EXIT(ch, door)
		  && EXIT(ch, door)->to_room() != kNowhere
		  && IsCorrectDirection(ch.get(), door, true, false)
		  && (!is_room_forbidden(world[EXIT(ch, door)->to_room()]) || ch->IsFlagged(EMobFlag::kIgnoreForbidden))
		  && (!ch->IsFlagged(EMobFlag::kStayZone)
			  || world[EXIT(ch, door)->to_room()]->zone_rn == world[ch->in_room]->zone_rn)
		  && allow_enter(world[EXIT(ch, door)->to_room()], ch.get())) {
		  // После хода нпц уже может не быть, т.к. ушел в дт, я не знаю почему
		  // оно не валится на муд.ру, но на цигвине у меня падало стабильно,
		  // т.к. в ch уже местами мусор после фри-чара // Krodo
		  if (npc_move(ch.get(), door, 1)) {
			  npc_group(ch.get());
			  npc_groupbattle(ch.get());
		  } else {
			  continue;
		  }
	  }
	  npc_light(ch.get());
	  // *****************  Mob Memory
	  if (ch->IsFlagged(EMobFlag::kMemory)
		  && MEMORY(ch)
		  && ch->GetPosition() > EPosition::kSleep
		  && !AFF_FLAGGED(ch, EAffect::kBlind)
		  && !ch->GetEnemy()) {
		  // Find memory in world
		  for (auto names = MEMORY(ch); names && (GET_SPELL_MEM(ch, ESpell::kSummon) > 0
			  || GET_SPELL_MEM(ch, ESpell::kRelocate) > 0); names = names->next) {
			  for (const auto &vict : character_list) {
				  if (names->id == vict->get_uid()
					  && CAN_SEE(ch, vict) && !vict->IsFlagged(EPrf::kNohassle)) {
					  if (GET_SPELL_MEM(ch, ESpell::kSummon) > 0) {
						  CastSpell(ch.get(), vict.get(), nullptr, nullptr, ESpell::kSummon, ESpell::kSummon);
						  break;
					  } else if (GET_SPELL_MEM(ch, ESpell::kRelocate) > 0) {
						  CastSpell(ch.get(), vict.get(), nullptr, nullptr, ESpell::kRelocate, ESpell::kRelocate);
						  break;
					  }
				  }
			  }
		  }
	  }
	  // Add new mobile actions here
	  if (was_in != ch->in_room) {
		  do_aggressive_room(ch.get(), false);
	  }
	}
}
ObjData *create_charmice_box(CharData *ch) {
	const auto obj = world_objects.create_blank();

	obj->set_aliases("узелок вещами");
	const std::string descr = std::string("узелок с вещами ") + ch->get_pad(1);
	obj->set_short_description(descr);
	obj->set_description("Туго набитый узел лежит тут.");
	obj->set_ex_description(descr.c_str(), "Кто-то сильно торопился, когда набивал этот узелок.");
	obj->set_PName(ECase::kNom, "узелок");
	obj->set_PName(ECase::kGen, "узелка");
	obj->set_PName(ECase::kDat, "узелку");
	obj->set_PName(ECase::kAcc, "узелок");
	obj->set_PName(ECase::kIns, "узелком");
	obj->set_PName(ECase::kPre, "узелке");
	obj->set_sex(EGender::kMale);
	obj->set_type(EObjType::kContainer);
	obj->set_wear_flags(to_underlying(EWearFlag::kTake));
	obj->set_weight(1);
	obj->set_cost(1);
	obj->set_rent_off(1);
	obj->set_rent_on(1);
	obj->set_timer(9999);

	obj->set_extra_flag(EObjFlag::kNosell);
	obj->set_extra_flag(EObjFlag::kNolocate);
	obj->set_extra_flag(EObjFlag::kNodecay);
	obj->set_extra_flag(EObjFlag::kSwimming);
	obj->set_extra_flag(EObjFlag::kFlying);

	return obj.get();
}

void extract_charmice(CharData *ch, bool on_ground) {
	std::vector<ObjData *> objects;
	for (int i = 0; i < EEquipPos::kNumEquipPos; ++i) {
		if (GET_EQ(ch, i)) {
			ObjData *obj = UnequipChar(ch, i, CharEquipFlags());
			if (obj) {
				remove_otrigger(obj, ch);
				objects.push_back(obj);
			}
		}
	}

	while (ch->carrying) {
		ObjData *obj = ch->carrying;
		RemoveObjFromChar(obj);
		objects.push_back(obj);
	}

	if (!objects.empty()) {
		ObjData *charmice_box = create_charmice_box(ch);
		for (auto &object : objects) {
			PlaceObjIntoObj(object, charmice_box);
		}
		if (on_ground || !ch->get_master()) {
			DropObjOnZoneReset(ch, charmice_box, true, false);
		} else {
			SendMsgToChar(ch->get_master(), "&YВолшебный узелок с вещами появился у вас в инвентаре.&n\r\n");
			PlaceObjToInventory(charmice_box, ch->get_master());
		}
	}
	character_list.AddToExtractedList(ch);
}

} // namespace mob_ai

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
