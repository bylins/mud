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

#include "game_skills/backstab.h"
#include "game_skills/bash.h"
#include "game_skills/strangle.h"
#include "game_skills/chopoff.h"
#include "game_skills/disarm.h"
#include "game_skills/stupor.h"
#include "game_skills/throw.h"
#include "game_skills/mighthit.h"
#include "game_skills/protect.h"
#include "game_skills/track.h"

#include "abilities/abilities_rollsystem.h"
#include "action_targeting.h"
#include "act_movement.h"
#include "entities/world_characters.h"
#include "world_objects.h"
#include "handler.h"
#include "game_magic/magic.h"
#include "fightsystem/pk.h"
#include "utils/random.h"
#include "house.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"

const int kMobMemKoeff = kSecsPerMudHour;

// external structs
extern int no_specials;
extern int guild_poly(CharData *, void *, int, char *);
extern guardian_type guardian_list;

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
bool guardian_attack(CharData *ch, CharData *vict);
void drop_obj_on_zreset(CharData *ch, ObjData *obj, bool inv, bool zone_reset);

// local functions

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

int extra_aggressive(CharData *ch, CharData *victim) {
	int time_ok = false, no_time = true, month_ok = false, no_month = true, agro = false;

	if (!ch->is_npc())
		return (false);

	if (MOB_FLAGGED(ch, EMobFlag::kAgressive))
		return (true);

	if (MOB_FLAGGED(ch, EMobFlag::kCityGuardian) && guardian_attack(ch, victim))
		return (true);

	if (victim && MOB_FLAGGED(ch, EMobFlag::kAgressiveMono) && !victim->is_npc() && GET_RELIGION(victim) == kReligionMono)
		agro = true;

	if (victim && MOB_FLAGGED(ch, EMobFlag::kAgressivePoly) && !victim->is_npc() && GET_RELIGION(victim) == kReligionPoly)
		agro = true;

	if (MOB_FLAGGED(ch, EMobFlag::kAgressiveDay)) {
		no_time = false;
		if (weather_info.sunlight == kSunRise || weather_info.sunlight == kSunLight)
			time_ok = true;
	}

	if (MOB_FLAGGED(ch, EMobFlag::kAggressiveNight)) {
		no_time = false;
		if (weather_info.sunlight == kSunDark || weather_info.sunlight == kSunSet)
			time_ok = true;
	}

	if (MOB_FLAGGED(ch, EMobFlag::kAgressiveWinter)) {
		no_month = false;
		if (weather_info.season == SEASON_WINTER)
			month_ok = true;
	}

	if (MOB_FLAGGED(ch, EMobFlag::kAgressiveSpring)) {
		no_month = false;
		if (weather_info.season == SEASON_SPRING)
			month_ok = true;
	}

	if (MOB_FLAGGED(ch, EMobFlag::kAgressiveSummer)) {
		no_month = false;
		if (weather_info.season == SEASON_SUMMER)
			month_ok = true;
	}

	if (MOB_FLAGGED(ch, EMobFlag::kAgressiveAutumn)) {
		no_month = false;
		if (weather_info.season == SEASON_AUTUMN)
			month_ok = true;
	}

	if (MOB_FLAGGED(ch, EMobFlag::kAgressiveFullmoon)) {
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

int attack_best(CharData *ch, CharData *victim) {
	ObjData *wielded = GET_EQ(ch, EEquipPos::kWield);
	if (victim) {
		if (ch->get_skill(ESkill::kStrangle) && !IsTimedBySkill(ch, ESkill::kStrangle)) {
			go_strangle(ch, victim);
			return (true);
		}
		if (ch->get_skill(ESkill::kBackstab) && !victim->get_fighting()) {
			go_backstab(ch, victim);
			return (true);
		}
		if (ch->get_skill(ESkill::kHammer)) {
			go_mighthit(ch, victim);
			return (true);
		}
		if (ch->get_skill(ESkill::kOverwhelm)) {
			go_stupor(ch, victim);
			return (true);
		}
		if (ch->get_skill(ESkill::kBash)) {
			go_bash(ch, victim);
			return (true);
		}
		if (ch->get_skill(ESkill::kThrow)
			&& wielded
			&& GET_OBJ_TYPE(wielded) == ObjData::ITEM_WEAPON
			&& wielded->has_flag(EObjFlag::kThrowing)) {
			go_throw(ch, victim);
		}
		if (ch->get_skill(ESkill::kDisarm)) {
			go_disarm(ch, victim);
		}
		if (ch->get_skill(ESkill::kUndercut)) {
			go_chopoff(ch, victim);
		}
		if (!ch->get_fighting()) {
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
		if (CAN_SEE(ch, vict) && IN_ROOM(vict) != kNowhere) {
			for (auto names = MEMORY(ch); names; names = names->next) {
				if (GET_IDNUM(vict) == names->id
					&& (!MOB_FLAGGED(ch, EMobFlag::kStayZone)
						|| world[ch->in_room]->zone_rn == world[IN_ROOM(vict)]->zone_rn)) {
					if (!msg) {
						msg = true;
						act("$n начал$g внимательно искать чьи-то следы.",
							false, ch, nullptr, nullptr, kToRoom);
					}

					const auto door = track_method
									  ? check_room_tracks(ch->in_room, GET_IDNUM(vict))
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
	const auto result = find_door(ch, GET_REAL_INT(ch) < number(15, 20));

	return result;
}

CharData *selectRandomSkirmisherFromGroup(CharData *leader) {
	ActionTargeting::FriendsRosterType roster{leader};
	auto isSkirmisher = [](CharData *ch) { return PRF_FLAGGED(ch, EPrf::kSkirmisher); };
	int skirmishers = roster.count(isSkirmisher);
	if (skirmishers == 0 || skirmishers == roster.amount()) {
		return nullptr;
	}
	return roster.getRandomItem(isSkirmisher);
}

CharData *selectVictimDependingOnGroupFormation(CharData *assaulter, CharData *initialVictim) {
	if ((initialVictim == nullptr) || !AFF_FLAGGED(initialVictim, EAffect::kGroup) || MOB_FLAGGED(assaulter, EMobFlag::kIgnoresFormation)) {
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

	AbilitySystem::AgainstRivalRoll abilityRoll;
	abilityRoll.Init(leader, EFeat::kTactician, assaulter);
	bool tacticianFail = !abilityRoll.IsSuccess();
	abilityRoll.Init(newVictim, EFeat::kScirmisher, assaulter);
	if (tacticianFail || !abilityRoll.IsSuccess()) {
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

	victim = ch->get_fighting();

	for (const auto vict : world[ch->in_room]->people) {
		if ((vict->is_npc() && !IS_SET(extmode, CHECK_OPPONENT) && !IS_CHARMICE(vict))
			|| (IS_CHARMICE(vict) && !vict->get_fighting()) // чармиса агрим только если он уже с кем-то сражается
			|| PRF_FLAGGED(vict, EPrf::kNohassle)
			|| !MAY_SEE(ch, ch, vict)
			|| (IS_SET(extmode, CHECK_OPPONENT) && ch != vict->get_fighting())
			|| (!may_kill_here(ch, vict, NoArgument) && !IS_SET(extmode, GUARD_ATTACK)))//старжники агрят в мирках
		{
			continue;
		}

		kill_this = false;

		// Mobile too damage //обработка флага ТРУС
		if (IS_SET(extmode, CHECK_HITS)
			&& MOB_FLAGGED(ch, EMobFlag::kWimpy)
			&& AWAKE(vict)
			&& GET_HIT(ch) * 2 < GET_REAL_MAX_HIT(ch)) {
			continue;
		}

		// Mobile helpers... //ассист
		if (IS_SET(extmode, KILL_FIGHTING)
			&& vict->get_fighting()
			&& vict->get_fighting() != ch
			&& vict->get_fighting()->is_npc()
			&& !AFF_FLAGGED(vict->get_fighting(), EAffect::kCharmed)
			&& SAME_ALIGN(ch, vict->get_fighting())) {
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
			skip_sneaking(vict, ch);
			if ((EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK))) {
				AFF_FLAGS(vict).unset(EAffect::kSneak);
			}
			if (AFF_FLAGGED(vict, EAffect::kSneak))
				continue;
		}

		if (IS_SET(extmode, SKIP_HIDING)) {
			skip_hiding(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE)) {
				AFF_FLAGS(vict).unset(EAffect::kHide);
			}
		}

		if (IS_SET(extmode, SKIP_CAMOUFLAGE)) {
			skip_camouflage(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE)) {
				AFF_FLAGS(vict).unset(EAffect::kDisguise);
			}
		}

		if (!CAN_SEE(ch, vict))
			continue;

		// Mobile aggresive
		if (!kill_this && extra_aggr) {
			if (IsAbleToUseFeat(vict, EFeat::kSilverTongue)) {
				const int number1 = number(1, GetRealLevel(vict) * GET_REAL_CHA(vict));
				const int range = ((GetRealLevel(ch) > 30)
								   ? (GetRealLevel(ch) * 2 * GET_REAL_INT(ch) + GET_REAL_INT(ch) * 20)
								   : (GetRealLevel(ch) * GET_REAL_INT(ch)));
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

		if (IS_DEFAULTDARK(ch->in_room)
			&& ((GET_EQ(vict, ObjData::ITEM_LIGHT)
				&& GET_OBJ_VAL(GET_EQ(vict, ObjData::ITEM_LIGHT), 2))
				|| (!AFF_FLAGGED(vict, EAffect::kHolyDark)
					&& (AFF_FLAGGED(vict, EAffect::kSingleLight)
						|| AFF_FLAGGED(vict, EAffect::kHolyLight))))
			&& (!use_light
				|| GET_REAL_CHA(use_light) > GET_REAL_CHA(vict))) {
			use_light = vict;
		}

		if (!min_hp
			|| GET_HIT(vict) + GET_REAL_CHA(vict) * 10 < GET_HIT(min_hp) + GET_REAL_CHA(min_hp) * 10) {
			min_hp = vict;
		}

		if (!min_lvl
			|| GetRealLevel(vict) + number(1, GET_REAL_CHA(vict))
				< GetRealLevel(min_lvl) + number(1, GET_REAL_CHA(min_lvl))) {
			min_lvl = vict;
		}

		if (IS_CASTER(vict)
			&& (!caster
				|| GET_CASTER(caster) * GET_REAL_CHA(vict) < GET_CASTER(vict) * GET_REAL_CHA(caster))) {
			caster = vict;
		}
	}

	if (GET_REAL_INT(ch) < 5 + number(1, 6))
		best = victim;
	else if (GET_REAL_INT(ch) < 10 + number(1, 6))
		best = use_light ? use_light : victim;
	else if (GET_REAL_INT(ch) < 15 + number(1, 6))
		best = min_lvl ? min_lvl : (use_light ? use_light : victim);
	else if (GET_REAL_INT(ch) < 25 + number(1, 6))
		best = caster ? caster : (min_lvl ? min_lvl : (use_light ? use_light : victim));
	else
		best = min_hp ? min_hp : (caster ? caster : (min_lvl ? min_lvl : (use_light ? use_light : victim)));

	if (best && !ch->get_fighting() && MOB_FLAGGED(ch, EMobFlag::kAgressiveMono) &&
		!best->is_npc() && GET_RELIGION(best) == kReligionMono) {
		act("$n закричал$g: 'Умри, христианская собака!' и набросил$u на вас.", false, ch, nullptr, best, kToVict);
		act("$n закричал$g: 'Умри, христианская собака!' и набросил$u на $N3.", false, ch, nullptr, best, kToNotVict);
	}

	if (best && !ch->get_fighting() && MOB_FLAGGED(ch, EMobFlag::kAgressivePoly) &&
		!best->is_npc() && GET_RELIGION(best) == kReligionPoly) {
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

// пока тестово
CharData *find_best_mob_victim(CharData *ch, int extmode) {
	CharData *currentVictim, *caster = nullptr, *best = nullptr;
	CharData *druid = nullptr, *cler = nullptr, *charmmage = nullptr;
	int extra_aggr = 0;
	bool kill_this;

	int mobINT = GET_REAL_INT(ch);
	if (mobINT < kStupidMod) {
		return find_best_stupidmob_victim(ch, extmode);
	}

	currentVictim = ch->get_fighting();
	if (currentVictim && !currentVictim->is_npc()) {
		if (IS_CASTER(currentVictim)) {
			return currentVictim;
		}
	}

	// проходим по всем чарам в комнате
	for (const auto vict : world[ch->in_room]->people) {
		if ((vict->is_npc() && !IS_CHARMICE(vict))
			|| (IS_CHARMICE(vict) && !vict->get_fighting()
				&& find_master_charmice(vict)) // чармиса агрим только если нет хозяина в руме.
			|| PRF_FLAGGED(vict, EPrf::kNohassle)
			|| !MAY_SEE(ch, ch, vict) // если не видим цель,
			|| (IS_SET(extmode, CHECK_OPPONENT) && ch != vict->get_fighting())
			|| (!may_kill_here(ch, vict, NoArgument) && !IS_SET(extmode, GUARD_ATTACK)))//старжники агрят в мирках
		{
			continue;
		}

		kill_this = false;
		// Mobile too damage //обработка флага ТРУС
		if (IS_SET(extmode, CHECK_HITS)
			&& MOB_FLAGGED(ch, EMobFlag::kWimpy)
			&& AWAKE(vict) && GET_HIT(ch) * 2 < GET_REAL_MAX_HIT(ch)) {
			continue;
		}

		// Mobile helpers... //ассист
		if ((vict->get_fighting())
			&& (vict->get_fighting() != ch)
			&& vict->get_fighting()->is_npc()
			&& (!AFF_FLAGGED(vict->get_fighting(), EAffect::kCharmed))) {
			kill_this = true;
		} else {
			// ... but no aggressive for this char
			if (!(extra_aggr = extra_aggressive(ch, vict))
				&& !IS_SET(extmode, GUARD_ATTACK)) {
				continue;
			}
		}
		if (IS_SET(extmode, SKIP_SNEAKING)) {
			skip_sneaking(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK)) {
				AFF_FLAGS(vict).unset(EAffect::kSneak);
			}

			if (AFF_FLAGGED(vict, EAffect::kSneak)) {
				continue;
			}
		}

		if (IS_SET(extmode, SKIP_HIDING)) {
			skip_hiding(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE)) {
				AFF_FLAGS(vict).unset(EAffect::kHide);
			}
		}

		if (IS_SET(extmode, SKIP_CAMOUFLAGE)) {
			skip_camouflage(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE)) {
				AFF_FLAGS(vict).unset(EAffect::kDisguise);
			}
		}
		if (!CAN_SEE(ch, vict))
			continue;

		if (!kill_this && extra_aggr) {
			if (IsAbleToUseFeat(vict, EFeat::kSilverTongue)
				&& number(1, GetRealLevel(vict) * GET_REAL_CHA(vict)) > number(1, GetRealLevel(ch) * GET_REAL_INT(ch))) {
				continue;
			}
			kill_this = true;
		}

		if (!kill_this)
			continue;
		// волхв
		if (GET_CLASS(vict) == ECharClass::kMagus) {
			druid = vict;
			caster = vict;
			continue;
		}
		// лекарь
		if (GET_CLASS(vict) == ECharClass::kSorcerer) {
			cler = vict;
			caster = vict;
			continue;
		}
		// кудес
		if (GET_CLASS(vict) == ECharClass::kCharmer) {
			charmmage = vict;
			caster = vict;
			continue;
		}

		if (GET_HIT(vict) <= kCharacterHPForMobPriorityAttack) {
			return vict;
		}
		if (IS_CASTER(vict)) {
			caster = vict;
			continue;
		}
		best = vict;
	}

	if (!best) {
		best = currentVictim;
	}

	if (mobINT < kMiddleAI) {
		int rand = number(0, 2);
		if (caster) {
			best = caster;
		}
		if ((rand == 0) && (druid)) {
			best = druid;
		}
		if ((rand == 1) && (cler)) {
			best = cler;
		}
		if ((rand == 2) && (charmmage)) {
			best = charmmage;
		}
		return selectVictimDependingOnGroupFormation(ch, best);
	}

	if (mobINT < kHighAI) {
		int rand = number(0, 1);
		if (caster)
			best = caster;
		if (charmmage)
			best = charmmage;
		if ((rand == 0) && (druid))
			best = druid;
		if ((rand == 1) && (cler))
			best = cler;

		return selectVictimDependingOnGroupFormation(ch, best);
	}

	//  и если >= 40 инты
	if (caster)
		best = caster;
	if (charmmage)
		best = charmmage;
	if (cler)
		best = cler;
	if (druid)
		best = druid;

	return selectVictimDependingOnGroupFormation(ch, best);
}

int perform_best_mob_attack(CharData *ch, int extmode) {
	CharData *best;
	int clone_number = 0;
	best = find_best_mob_victim(ch, extmode);

	if (best) {
		// если у игрока стоит олц на зону, в ней его не агрят
		if (best->player_specials->saved.olc_zone == GET_MOB_VNUM(ch) / 100) {
			send_to_char(best, "&GАгромоб, атака остановлена.\r\n");
			return (false);
		}
		if (GET_POS(ch) < EPosition::kFight && GET_POS(ch) > EPosition::kSleep) {
			act("$n вскочил$g.", false, ch, nullptr, nullptr, kToRoom);
			GET_POS(ch) = EPosition::kStand;
		}

		if (IS_SET(extmode, KILL_FIGHTING) && best->get_fighting()) {
			if (MOB_FLAGGED(best->get_fighting(), EMobFlag::kNoHelp))
				return (false);
			act("$n вступил$g в битву на стороне $N1.", false, ch, nullptr, best->get_fighting(), kToRoom);
		}

		if (IS_SET(extmode, GUARD_ATTACK)) {
			act("'$N - за грехи свои ты заслуживаешь смерти!', сурово проговорил$g $n.",
				false, ch, nullptr, best, kToRoom);
			act("'Как страж этого города, я намерен$g привести приговор в исполнение немедленно. Защищайся!'",
				false, ch, nullptr, best, kToRoom);
		}

		if (!best->is_npc()) {
			struct Follower *f;
			// поиск клонов и отработка атаки в клона персонажа
			for (f = best->followers; f; f = f->next)
				if (MOB_FLAGGED(f->ch, EMobFlag::kClone))
					clone_number++;
			for (f = best->followers; f; f = f->next)
				if (f->ch->is_npc() && MOB_FLAGGED(f->ch, EMobFlag::kClone)
					&& IN_ROOM(f->ch) == IN_ROOM(best)) {
					if (number(0, clone_number) == 1)
						break;
					if ((GET_REAL_INT(ch) < 20) && number(0, clone_number))
						break;
					if (GET_REAL_INT(ch) >= 30)
						break;
					if ((GET_REAL_INT(ch) >= 20)
						&& number(1, 10 + VPOSI((35 - GET_REAL_INT(ch)), 0, 15) * clone_number) <= 10)
						break;
					best = f->ch;
					break;
				}
		}
		if (!start_fight_mtrigger(ch, best)) {
			return false;
		}
		if (!attack_best(ch, best) && !ch->get_fighting())
			hit(ch, best, ESkill::kUndefined, fight::kMainHand);
		return (true);
	}
	return (false);
}

int perform_best_horde_attack(CharData *ch, int extmode) {
	if (perform_best_mob_attack(ch, extmode)) {
		return (true);
	}

	for (const auto vict : world[ch->in_room]->people) {
		if (!vict->is_npc() || !MAY_SEE(ch, ch, vict) || MOB_FLAGGED(vict, EMobFlag::kProtect)) {
			continue;
		}

		if (!SAME_ALIGN(ch, vict)) {
			if (GET_POS(ch) < EPosition::kFight && GET_POS(ch) > EPosition::kSleep) {
				act("$n вскочил$g.", false, ch, nullptr, nullptr, kToRoom);
				GET_POS(ch) = EPosition::kStand;
			}
			if (!start_fight_mtrigger(ch, vict)) {
				return false;
			}
			if (!attack_best(ch, vict) && !ch->get_fighting()) {
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
	if (best == ch->get_fighting())
		return false;

	// переключаюсь на best
	stop_fighting(ch, false);
	set_fighting(ch, best);
	SetWait(ch, 2, false);

	if (ch->get_skill(ESkill::kHammer)
		&& check_mighthit_weapon(ch)) {
		SET_AF_BATTLE(ch, kEafHammer);
	} else if (ch->get_skill(ESkill::kOverwhelm)) {
		SET_AF_BATTLE(ch, ESkill::kOverwhelm);
	}

	return true;
}

void do_aggressive_mob(CharData *ch, int check_sneak) {
	if (!ch || ch->in_room == kNowhere || !ch->is_npc() || !MAY_ATTACK(ch) || AFF_FLAGGED(ch, EAffect::kBlind)) {
		return;
	}

	int mode = check_sneak ? SKIP_SNEAKING : 0;

	// ****************  Horde
	if (MOB_FLAGGED(ch, EMobFlag::kHorde)) {
		perform_best_horde_attack(ch, mode | SKIP_HIDING | SKIP_CAMOUFLAGE);
		return;
	}

	// ****************  Aggressive Mobs
	if (extra_aggressive(ch, nullptr)) {
		const auto &room = world[ch->in_room];
		for (auto affect_it = room->affected.begin(); affect_it != room->affected.end(); ++affect_it) {
			if (affect_it->get()->type == kSpellRuneLabel && (affect_it != room->affected.end())) {
				act("$n шаркнул$g несколько раз по светящимся рунам, полностью их уничтожив.",
					false,
					ch,
					nullptr,
					nullptr,
					kToRoom | kToArenaListen);
				room_spells::RemoveAffect(world[ch->in_room], affect_it);
				break;
			}
		}
		perform_best_mob_attack(ch, mode | SKIP_HIDING | SKIP_CAMOUFLAGE | CHECK_HITS);
		return;
	}
	//Polud стражники
	if (MOB_FLAGGED(ch, EMobFlag::kCityGuardian)) {
		perform_best_mob_attack(ch, SKIP_HIDING | SKIP_CAMOUFLAGE | SKIP_SNEAKING | GUARD_ATTACK);
		return;
	}

	// *****************  Mob Memory
	if (MOB_FLAGGED(ch, EMobFlag::kMemory) && MEMORY(ch)) {
		CharData *victim = nullptr;
		// Find memory in room
		const auto people_copy = world[ch->in_room]->people;
		for (auto vict_i = people_copy.begin(); vict_i != people_copy.end() && !victim; ++vict_i) {
			const auto vict = *vict_i;

			if (vict->is_npc()
				|| PRF_FLAGGED(vict, EPrf::kNohassle)) {
				continue;
			}
			for (MemoryRecord *names = MEMORY(ch); names && !victim; names = names->next) {
				if (names->id == GET_IDNUM(vict)) {
					if (!MAY_SEE(ch, ch, vict) || !may_kill_here(ch, vict, NoArgument)) {
						continue;
					}
					if (check_sneak) {
						skip_sneaking(vict, ch);
						if (EXTRA_FLAGGED(vict, EXTRA_FAILSNEAK)) {
							AFF_FLAGS(vict).unset(EAffect::kSneak);
						}
						if (AFF_FLAGGED(vict, EAffect::kSneak))
							continue;
					}
					skip_hiding(vict, ch);
					if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE)) {
						AFF_FLAGS(vict).unset(EAffect::kHide);
					}
					skip_camouflage(vict, ch);
					if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE)) {
						AFF_FLAGS(vict).unset(EAffect::kDisguise);
					}
					if (CAN_SEE(ch, vict)) {
						victim = vict;
					}
				}
			}
		}

		// Is memory found ?
		if (victim && !CHECK_WAIT(ch)) {
			if (GET_POS(ch) < EPosition::kFight && GET_POS(ch) > EPosition::kSleep) {
				act("$n вскочил$g.", false, ch, nullptr, nullptr, kToRoom);
				GET_POS(ch) = EPosition::kStand;
			}
			if (GET_RACE(ch) != ENpcRace::kHuman) {
				act("$n вспомнил$g $N3.", false, ch, nullptr, victim, kToRoom);
			} else {
				act("'$N - ты пытал$U убить меня ! Попал$U ! Умри !!!', воскликнул$g $n.",
					false, ch, nullptr, victim, kToRoom);
			}
			if (!start_fight_mtrigger(ch, victim)) {
				return;
			}
			if (!attack_best(ch, victim)) {
				hit(ch, victim, ESkill::kUndefined, fight::kMainHand);
			}
			return;
		}
	}

	// ****************  Helper Mobs
	if (MOB_FLAGGED(ch, EMobFlag::kHelper)) {
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
	for (const auto &vict: people) {
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
	if (!ch->is_npc() || !GET_MOB_SPEC(ch)) {
		return true;
	}

	for (const auto vict : room->people) {
		if (vict->is_npc()
			&& GET_MOB_SPEC(vict) == GET_MOB_SPEC(ch)) {
			return false;
		}
	}

	return true;
}

namespace {
ObjData *create_charmice_box(CharData *ch) {
	const auto obj = world_objects.create_blank();

	obj->set_aliases("узелок вещами");
	const std::string descr = std::string("узелок с вещами ") + ch->get_pad(1);
	obj->set_short_description(descr);
	obj->set_description("Туго набитый узел лежит тут.");
	obj->set_ex_description(descr.c_str(), "Кто-то сильно торопился, когда набивал этот узелок.");
	obj->set_PName(0, "узелок");
	obj->set_PName(1, "узелка");
	obj->set_PName(2, "узелку");
	obj->set_PName(3, "узелок");
	obj->set_PName(4, "узелком");
	obj->set_PName(5, "узелке");
	obj->set_sex(ESex::kMale);
	obj->set_type(ObjData::ITEM_CONTAINER);
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

void extract_charmice(CharData *ch) {
	std::vector<ObjData *> objects;
	for (int i = 0; i < EEquipPos::kNumEquipPos; ++i) {
		if (GET_EQ(ch, i)) {
			ObjData *obj = unequip_char(ch, i, CharEquipFlags());
			if (obj) {
				remove_otrigger(obj, ch);
				objects.push_back(obj);
			}
		}
	}

	while (ch->carrying) {
		ObjData *obj = ch->carrying;
		obj_from_char(obj);
		objects.push_back(obj);
	}

	if (!objects.empty()) {
		ObjData *charmice_box = create_charmice_box(ch);
		for (auto it = objects.begin(); it != objects.end(); ++it) {
			obj_to_obj(*it, charmice_box);
		}
		drop_obj_on_zreset(ch, charmice_box, true, false);
	}

	extract_char(ch, false);
}
}

void mobile_activity(int activity_level, int missed_pulses) {
	int door, max, was_in = -1, activity_lev, i, ch_activity;
	int std_lev = activity_level % kPulseMobile;

	character_list.foreach_on_copy([&](const CharData::shared_ptr &ch) {
		if (!IS_MOB(ch)
			|| !ch->in_used_zone()) {
			return;
		}

		i = missed_pulses;
		while (i--) {
			pulse_affect_update(ch.get());
		}

		ch->wait_dec(missed_pulses);
		ch->decreaseSkillsCooldowns(missed_pulses);

		if (GET_PUNCTUAL_WAIT(ch) > 0)
			GET_PUNCTUAL_WAIT(ch) -= missed_pulses;
		else
			GET_PUNCTUAL_WAIT(ch) = 0;

		if (GET_PUNCTUAL_WAIT(ch) < 0)
			GET_PUNCTUAL_WAIT(ch) = 0;

		if (ch->mob_specials.speed <= 0) {
			activity_lev = std_lev;
		} else {
			activity_lev = activity_level % (ch->mob_specials.speed*kRealSec);
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
			return;
		}

		// Examine call for special procedure
		if (MOB_FLAGGED(ch, EMobFlag::kSpec) && !no_specials) {
			if (mob_index[GET_MOB_RNUM(ch)].func == nullptr) {
				log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
					GET_NAME(ch), GET_MOB_VNUM(ch));
				MOB_FLAGS(ch).unset(EMobFlag::kSpec);
			} else {
				buf2[0] = '\0';
				if ((mob_index[GET_MOB_RNUM(ch)].func)(ch.get(), ch.get(), 0, buf2)) {
					return;    // go to next char
				}
			}
		}
		// Extract free horses
		if (AFF_FLAGGED(ch, EAffect::kHorse)
			&& MOB_FLAGGED(ch, EMobFlag::kMounting)
			&& !ch->has_master()) // если скакун, под седлом но нет хозяина
		{
			act("Возникший как из-под земли цыган ловко вскочил на $n3 и унесся прочь.",
				false, ch.get(), nullptr, nullptr, kToRoom);
			extract_char(ch.get(), false);
			return;
		}
		// Extract uncharmed mobs
		if (EXTRACT_TIMER(ch) > 0) {
			if (ch->has_master()) {
				EXTRACT_TIMER(ch) = 0;
			} else {
				EXTRACT_TIMER(ch)--;
				if (!EXTRACT_TIMER(ch)) {
					extract_charmice(ch.get());
					return;
				}
			}
		}
		// If the mob has no specproc, do the default actions
		if (ch->get_fighting() ||
			GET_POS(ch) <= EPosition::kStun ||
			GET_WAIT(ch) > 0 ||
			AFF_FLAGGED(ch, EAffect::kCharmed) ||
			AFF_FLAGGED(ch, EAffect::kHold) || AFF_FLAGGED(ch, EAffect::kMagicStopFight) ||
			AFF_FLAGGED(ch, EAffect::kStopFight) || AFF_FLAGGED(ch, EAffect::kSleep)) {
			return;
		}

		if (IS_HORSE(ch)) {
			if (GET_POS(ch) < EPosition::kFight) {
				GET_POS(ch) = EPosition::kStand;
			}

			return;
		}

		if (GET_POS(ch) == EPosition::kSleep && GET_DEFAULT_POS(ch) > EPosition::kSleep) {
			GET_POS(ch) = GET_DEFAULT_POS(ch);
			act("$n проснул$u.", false, ch.get(), 0, 0, kToRoom);
		}

		if (!AWAKE(ch)) {
			return;
		}

		max = false;
		bool found = false;
		for (const auto vict : world[ch->in_room]->people) {
			if (ch.get() == vict) {
				continue;
			}

			if (vict->get_fighting() == ch.get()) {
				return;        // Mob is under attack
			}

			if (!vict->is_npc()
				&& CAN_SEE(ch, vict)) {
				max = true;
			}
		}

		// Mob attemp rest if it is not an angel
		if (!max && !MOB_FLAGGED(ch, EMobFlag::kNoRest) && !MOB_FLAGGED(ch, EMobFlag::kHorde) &&
			GET_HIT(ch) < GET_REAL_MAX_HIT(ch) && !MOB_FLAGGED(ch, EMobFlag::kTutelar) && !MOB_FLAGGED(ch, EMobFlag::kMentalShadow)
			&& GET_POS(ch) > EPosition::kRest) {
			act("$n присел$g отдохнуть.", false, ch.get(), 0, 0, kToRoom);
			GET_POS(ch) = EPosition::kRest;
		}

		// Mob return to default pos if full rested or if it is an angel
		if ((GET_HIT(ch) >= GET_REAL_MAX_HIT(ch)
			&& GET_POS(ch) != GET_DEFAULT_POS(ch))
			|| ((MOB_FLAGGED(ch, EMobFlag::kTutelar)
				|| MOB_FLAGGED(ch, EMobFlag::kMentalShadow))
				&& GET_POS(ch) != GET_DEFAULT_POS(ch))) {
			switch (GET_DEFAULT_POS(ch)) {
				case EPosition::kStand: act("$n поднял$u.", false, ch.get(), 0, 0, kToRoom);
					GET_POS(ch) = EPosition::kStand;
					break;
				case EPosition::kSit: act("$n сел$g.", false, ch.get(), 0, 0, kToRoom);
					GET_POS(ch) = EPosition::kSit;
					break;
				case EPosition::kRest: act("$n присел$g отдохнуть.", false, ch.get(), 0, 0, kToRoom);
					GET_POS(ch) = EPosition::kRest;
					break;
				case EPosition::kSleep: act("$n уснул$g.", false, ch.get(), 0, 0, kToRoom);
					GET_POS(ch) = EPosition::kSleep;
					break;
				default:
					break;
			}
		}
		// continue, if the mob is an angel
		// если моб ментальная тень или ангел он не должен проявлять активность
		if ((MOB_FLAGGED(ch, EMobFlag::kTutelar))
			|| (MOB_FLAGGED(ch, EMobFlag::kMentalShadow))) {
			return;
		}

		// look at room before moving
		do_aggressive_mob(ch.get(), false);

		// if mob attack something
		if (ch->get_fighting()
			|| GET_WAIT(ch) > 0) {
			return;
		}

		// Scavenger (picking up objects)
		// От одного до трех предметов за раз
		i = number(1, 3);
		while (i) {
			npc_scavenge(ch.get());
			i--;
		}

		if (EXTRACT_TIMER(ch) == 0) {
			//чармисы, собирающиеся уходить - не лутят! (Купала)
			//Niker: LootCR// Start
			//Не уверен, что рассмотрены все случаи, когда нужно снимать флаги с моба
			//Реализация для лута и воровства
			int grab_stuff = false;
			// Looting the corpses

			grab_stuff += npc_loot(ch.get());
			grab_stuff += npc_steal(ch.get());

			if (grab_stuff) {
				MOB_FLAGS(ch).unset(EMobFlag::kAppearsDay);    //Взял из make_horse
				MOB_FLAGS(ch).unset(EMobFlag::kAppearsNight);
				MOB_FLAGS(ch).unset(EMobFlag::kAppearsFullmoon);
				MOB_FLAGS(ch).unset(EMobFlag::kAppearsWinter);
				MOB_FLAGS(ch).unset(EMobFlag::kAppearsSpring);
				MOB_FLAGS(ch).unset(EMobFlag::kAppearsSummer);
				MOB_FLAGS(ch).unset(EMobFlag::kAppearsAutumn);
			}
			//Niker: LootCR// End
		}
		npc_wield(ch.get());
		npc_armor(ch.get());

		if (GET_POS(ch) == EPosition::kStand && NPC_FLAGGED(ch, ENpcFlag::kInvis)) {
			ch->set_affect(EAffect::kInvisible);
		}

		if (GET_POS(ch) == EPosition::kStand && NPC_FLAGGED(ch, ENpcFlag::kMoveFly)) {
			ch->set_affect(EAffect::kFly);
		}

		if (GET_POS(ch) == EPosition::kStand && NPC_FLAGGED(ch, ENpcFlag::kSneaking)) {
			if (CalcCurrentSkill(ch.get(), ESkill::kSneak, 0) >= number(0, 100)) {
				ch->set_affect(EAffect::kSneak);
			} else {
				ch->remove_affect(EAffect::kSneak);
			}
			affect_total(ch.get());
		}

		if (GET_POS(ch) == EPosition::kStand && NPC_FLAGGED(ch, ENpcFlag::kDisguising)) {
			if (CalcCurrentSkill(ch.get(), ESkill::kDisguise, 0) >= number(0, 100)) {
				ch->set_affect(EAffect::kDisguise);
			} else {
				ch->remove_affect(EAffect::kDisguise);
			}

			affect_total(ch.get());
		}

		door = kBfsError;

		// Helpers go to some dest
		if (MOB_FLAGGED(ch, EMobFlag::kHelper)
			&& !MOB_FLAGGED(ch, EMobFlag::kSentinel)
			&& !AFF_FLAGGED(ch, EAffect::kBlind)
			&& !ch->has_master()
			&& GET_POS(ch) == EPosition::kStand) {
			for (found = false, door = 0; door < EDirection::kMaxDirNum; door++) {
				RoomData::exit_data_ptr rdata = EXIT(ch, door);
				if (!rdata
					|| rdata->to_room() == kNowhere
					|| !legal_dir(ch.get(), door, true, false)
					|| (is_room_forbidden(world[rdata->to_room()])
						&& !MOB_FLAGGED(ch, EMobFlag::kIgnoreForbidden))
					|| IS_DARK(rdata->to_room())
					|| (MOB_FLAGGED(ch, EMobFlag::kStayZone)
						&& world[ch->in_room]->zone_rn != world[rdata->to_room()]->zone_rn)) {
					continue;
				}

				const auto room = world[rdata->to_room()];
				for (auto first : room->people) {
					if (first->is_npc()
						&& !AFF_FLAGGED(first, EAffect::kCharmed)
						&& !IS_HORSE(first)
						&& CAN_SEE(ch, first)
						&& first->get_fighting()
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
			&& GET_POS(ch) > EPosition::kFight
			&& door == kBfsError) {
			npc_group(ch.get());
			door = npc_walk(ch.get());
		}

		if (MEMORY(ch) && door == kBfsError && GET_POS(ch) > EPosition::kFight && ch->get_skill(ESkill::kTrack))
			door = npc_track(ch.get());

		if (door == kBfsAlreadyThere) {
			do_aggressive_mob(ch.get(), false);
			return;
		}

		if (door == kBfsError) {
			door = number(0, 18);
		}

		// Mob Movement
		if (!MOB_FLAGGED(ch, EMobFlag::kSentinel)
			&& GET_POS(ch) == EPosition::kStand
			&& (door >= 0 && door < EDirection::kMaxDirNum)
			&& EXIT(ch, door)
			&& EXIT(ch, door)->to_room() != kNowhere
			&& legal_dir(ch.get(), door, true, false)
			&& (!is_room_forbidden(world[EXIT(ch, door)->to_room()]) || MOB_FLAGGED(ch, EMobFlag::kIgnoreForbidden))
			&& (!MOB_FLAGGED(ch, EMobFlag::kStayZone)
				|| world[EXIT(ch, door)->to_room()]->zone_rn == world[ch->in_room]->zone_rn)
			&& allow_enter(world[EXIT(ch, door)->to_room()], ch.get())) {
			// После хода нпц уже может не быть, т.к. ушел в дт, я не знаю почему
			// оно не валится на муд.ру, но на цигвине у меня падало стабильно,
			// т.к. в ch уже местами мусор после фри-чара // Krodo
			if (npc_move(ch.get(), door, 1)) {
				npc_group(ch.get());
				npc_groupbattle(ch.get());
			} else {
				return;
			}
		}

		npc_light(ch.get());

		// *****************  Mob Memory
		if (MOB_FLAGGED(ch, EMobFlag::kMemory)
			&& MEMORY(ch)
			&& GET_POS(ch) > EPosition::kSleep
			&& !AFF_FLAGGED(ch, EAffect::kBlind)
			&& !ch->get_fighting()) {
			// Find memory in world
			for (auto names = MEMORY(ch); names && (GET_SPELL_MEM(ch, kSpellSummon) > 0
				|| GET_SPELL_MEM(ch, kSpellRelocate) > 0); names = names->next) {
				for (const auto &vict : character_list) {
					if (names->id == GET_IDNUM(vict)
						&& CAN_SEE(ch, vict) && !PRF_FLAGGED(vict, EPrf::kNohassle)) {
						if (GET_SPELL_MEM(ch, kSpellSummon) > 0) {
							CastSpell(ch.get(), vict.get(), 0, 0, kSpellSummon, kSpellSummon);

							break;
						} else if (GET_SPELL_MEM(ch, kSpellRelocate) > 0) {
							CastSpell(ch.get(), vict.get(), 0, 0, kSpellRelocate, kSpellRelocate);

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
	});            // end for()
}

// Mob Memory Routines
// 11.07.2002 - у зачармленных мобов не работает механизм памяти на время чарма

// make ch remember victim
void mobRemember(CharData *ch, CharData *victim) {
	struct TimedSkill timed{};
	MemoryRecord *tmp;
	bool present = false;

	if (!ch->is_npc() ||
		victim->is_npc() ||
		PRF_FLAGGED(victim, EPrf::kNohassle) ||
		!MOB_FLAGGED(ch, EMobFlag::kMemory) ||
		AFF_FLAGGED(ch, EAffect::kCharmed))
		return;

	for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
		if (tmp->id == GET_IDNUM(victim)) {
			if (tmp->time > 0)
				tmp->time = time(nullptr) + kMobMemKoeff * GET_REAL_INT(ch);
			present = true;
		}

	if (!present) {
		CREATE(tmp, 1);
		tmp->next = MEMORY(ch);
		tmp->id = GET_IDNUM(victim);
		tmp->time = time(nullptr) + kMobMemKoeff * GET_REAL_INT(ch);
		MEMORY(ch) = tmp;
	}

	if (!IsTimedBySkill(victim, ESkill::kHideTrack)) {
		timed.skill = ESkill::kHideTrack;
		timed.time = ch->get_skill(ESkill::kTrack) ? 6 : 3;
		timed_to_char(victim, &timed);
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

	while (curr && curr->id != GET_IDNUM(victim)) {
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
//Polud Функция проверяет, является ли моб ch стражником (описан в файле guards.xml)
//и должен ли он сагрить на эту жертву vict
bool guardian_attack(CharData *ch, CharData *vict) {
	struct mob_guardian tmp_guard;
	int num_wars_vict = 0;

	if (!ch->is_npc() || !vict || !MOB_FLAGGED(ch, EMobFlag::kCityGuardian))
		return false;
//на всякий случай проверим, а вдруг моб да подевался куда из списка...
	auto it = guardian_list.find(GET_MOB_VNUM(ch));
	if (it == guardian_list.end())
		return false;

	tmp_guard = guardian_list[GET_MOB_VNUM(ch)];

	if ((tmp_guard.agro_all_agressors && AGRESSOR(vict)) ||
		(tmp_guard.agro_killers && PLR_FLAGGED(vict, EPlrFlag::kKiller)))
		return true;

	if (CLAN(vict)) {
		num_wars_vict = Clan::GetClanWars(vict);
		int clan_town_vnum = CLAN(vict)->GetOutRent() / 100; //Polud подскажите мне другой способ определить vnum зоны
		int mob_town_vnum = GET_MOB_VNUM(ch) / 100;          //по vnum комнаты, не перебирая все комнаты и зоны мира
		if (num_wars_vict && num_wars_vict > tmp_guard.max_wars_allow && clan_town_vnum != mob_town_vnum)
			return true;
	}
	if (AGRESSOR(vict))
		for (int & agro_argressors_in_zone : tmp_guard.agro_argressors_in_zones) {
			if (agro_argressors_in_zone == AGRESSOR(vict) / 100) return true;
		}

	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
