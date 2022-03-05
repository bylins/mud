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

	if (!IS_NPC(ch))
		return (false);

	if (MOB_FLAGGED(ch, MOB_AGGRESSIVE))
		return (true);

	if (MOB_FLAGGED(ch, MOB_GUARDIAN) && guardian_attack(ch, victim))
		return (true);

	if (victim && MOB_FLAGGED(ch, MOB_AGGRMONO) && !IS_NPC(victim) && GET_RELIGION(victim) == kReligionMono)
		agro = true;

	if (victim && MOB_FLAGGED(ch, MOB_AGGRPOLY) && !IS_NPC(victim) && GET_RELIGION(victim) == kReligionPoly)
		agro = true;

//Пока что убрал обработку флагов, тем более что персов кроме русичей и нет
//Поскольку расы и рода убраны из кода то так вот в лоб этот флаг не сделать,
//надо или по названию расы смотреть или еще что придумывать
	/*if (victim && MOB_FLAGGED(ch, MOB_AGGR_RUSICHI) && !IS_NPC(victim) && GET_KIN(victim) == KIN_RUSICHI)
		agro = true;

	if (victim && MOB_FLAGGED(ch, MOB_AGGR_VIKINGI) && !IS_NPC(victim) && GET_KIN(victim) == KIN_VIKINGI)
		agro = true;

	if (victim && MOB_FLAGGED(ch, MOB_AGGR_STEPNYAKI) && !IS_NPC(victim) && GET_KIN(victim) == KIN_STEPNYAKI)
		agro = true; */

	if (MOB_FLAGGED(ch, MOB_AGGR_DAY)) {
		no_time = false;
		if (weather_info.sunlight == SUN_RISE || weather_info.sunlight == SUN_LIGHT)
			time_ok = true;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_NIGHT)) {
		no_time = false;
		if (weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET)
			time_ok = true;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_WINTER)) {
		no_month = false;
		if (weather_info.season == SEASON_WINTER)
			month_ok = true;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_SPRING)) {
		no_month = false;
		if (weather_info.season == SEASON_SPRING)
			month_ok = true;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_SUMMER)) {
		no_month = false;
		if (weather_info.season == SEASON_SUMMER)
			month_ok = true;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_AUTUMN)) {
		no_month = false;
		if (weather_info.season == SEASON_AUTUMN)
			month_ok = true;
	}

	if (MOB_FLAGGED(ch, MOB_AGGR_FULLMOON)) {
		no_time = false;
		if (weather_info.moon_day >= 12 && weather_info.moon_day <= 15 &&
			(weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET))
			time_ok = true;
	}
	if (agro || !no_time || !no_month)
		return ((no_time || time_ok) && (no_month || month_ok));
	else
		return (false);
}

int attack_best(CharData *ch, CharData *victim) {
	ObjData *wielded = GET_EQ(ch, WEAR_WIELD);
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
			&& wielded->get_extra_flag(EExtraFlag::ITEM_THROWING)) {
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
			for (int i = 0; i < kDirMaxNumber; i++) {
				if (IS_SET(track->time_outgone[i], 7)) {
					return i;
				}
			}
		}
	}

	return BFS_ERROR;
}

int find_door(CharData *ch, const bool track_method) {
	bool msg = false;

	for (const auto &vict : character_list) {
		if (CAN_SEE(ch, vict) && IN_ROOM(vict) != kNowhere) {
			for (auto names = MEMORY(ch); names; names = names->next) {
				if (GET_IDNUM(vict) == names->id
					&& (!MOB_FLAGGED(ch, MOB_STAY_ZONE)
						|| world[ch->in_room]->zone_rn == world[IN_ROOM(vict)]->zone_rn)) {
					if (!msg) {
						msg = true;
						act("$n начал$g внимательно искать чьи-то следы.",
							false, ch, nullptr, nullptr, kToRoom);
					}

					const auto door = track_method
									  ? check_room_tracks(ch->in_room, GET_IDNUM(vict))
									  : go_track(ch, vict.get(), ESkill::kTrack);

					if (BFS_ERROR != door) {
						return door;
					}
				}
			}
		}
	}

	return BFS_ERROR;
}

int npc_track(CharData *ch) {
	const auto result = find_door(ch, GET_REAL_INT(ch) < number(15, 20));

	return result;
}

CharData *selectRandomSkirmisherFromGroup(CharData *leader) {
	ActionTargeting::FriendsRosterType roster{leader};
	auto isSkirmisher = [](CharData *ch) { return PRF_FLAGGED(ch, PRF_SKIRMISHER); };
	int skirmishers = roster.count(isSkirmisher);
	if (skirmishers == 0 || skirmishers == roster.amount()) {
		return nullptr;
	}
	return roster.getRandomItem(isSkirmisher);
}

CharData *selectVictimDependingOnGroupFormation(CharData *assaulter, CharData *initialVictim) {
	if ((initialVictim == nullptr) || !AFF_FLAGGED(initialVictim, EAffectFlag::AFF_GROUP) || MOB_FLAGGED(assaulter, MOB_IGNORE_FORMATION)) {
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
	abilityRoll.Init(leader, TACTICIAN_FEAT, assaulter);
	bool tacticianFail = !abilityRoll.IsSuccess();
	abilityRoll.Init(newVictim, SKIRMISHER_FEAT, assaulter);
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
		if ((IS_NPC(vict) && !IS_SET(extmode, CHECK_OPPONENT) && !IS_CHARMICE(vict))
			|| (IS_CHARMICE(vict) && !vict->get_fighting()) // чармиса агрим только если он уже с кем-то сражается
			|| PRF_FLAGGED(vict, PRF_NOHASSLE)
			|| !MAY_SEE(ch, ch, vict)
			|| (IS_SET(extmode, CHECK_OPPONENT) && ch != vict->get_fighting())
			|| (!may_kill_here(ch, vict, NoArgument) && !IS_SET(extmode, GUARD_ATTACK)))//старжники агрят в мирках
		{
			continue;
		}

		kill_this = false;

		// Mobile too damage //обработка флага ТРУС
		if (IS_SET(extmode, CHECK_HITS)
			&& MOB_FLAGGED(ch, MOB_WIMPY)
			&& AWAKE(vict)
			&& GET_HIT(ch) * 2 < GET_REAL_MAX_HIT(ch)) {
			continue;
		}

		// Mobile helpers... //ассист
		if (IS_SET(extmode, KILL_FIGHTING)
			&& vict->get_fighting()
			&& vict->get_fighting() != ch
			&& IS_NPC(vict->get_fighting())
			&& !AFF_FLAGGED(vict->get_fighting(), EAffectFlag::AFF_CHARM)
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
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_SNEAK);
			}
			if (AFF_FLAGGED(vict, EAffectFlag::AFF_SNEAK))
				continue;
		}

		if (IS_SET(extmode, SKIP_HIDING)) {
			skip_hiding(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE)) {
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_HIDE);
			}
		}

		if (IS_SET(extmode, SKIP_CAMOUFLAGE)) {
			skip_camouflage(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE)) {
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_CAMOUFLAGE);
			}
		}

		if (!CAN_SEE(ch, vict))
			continue;

		// Mobile aggresive
		if (!kill_this && extra_aggr) {
			if (can_use_feat(vict, SILVER_TONGUED_FEAT)) {
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
				|| (!AFF_FLAGGED(vict, EAffectFlag::AFF_HOLYDARK)
					&& (AFF_FLAGGED(vict, EAffectFlag::AFF_SINGLELIGHT)
						|| AFF_FLAGGED(vict, EAffectFlag::AFF_HOLYLIGHT))))
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

	if (best && !ch->get_fighting() && MOB_FLAGGED(ch, MOB_AGGRMONO) &&
		!IS_NPC(best) && GET_RELIGION(best) == kReligionMono) {
		act("$n закричал$g: 'Умри, христианская собака!' и набросил$u на вас.", false, ch, nullptr, best, kToVict);
		act("$n закричал$g: 'Умри, христианская собака!' и набросил$u на $N3.", false, ch, nullptr, best, kToNotVict);
	}

	if (best && !ch->get_fighting() && MOB_FLAGGED(ch, MOB_AGGRPOLY) &&
		!IS_NPC(best) && GET_RELIGION(best) == kReligionPoly) {
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
	if (currentVictim && !IS_NPC(currentVictim)) {
		if (IS_CASTER(currentVictim)) {
			return currentVictim;
		}
	}

	// проходим по всем чарам в комнате
	for (const auto vict : world[ch->in_room]->people) {
		if ((IS_NPC(vict) && !IS_CHARMICE(vict))
			|| (IS_CHARMICE(vict) && !vict->get_fighting()
				&& find_master_charmice(vict)) // чармиса агрим только если нет хозяина в руме.
			|| PRF_FLAGGED(vict, PRF_NOHASSLE)
			|| !MAY_SEE(ch, ch, vict) // если не видим цель,
			|| (IS_SET(extmode, CHECK_OPPONENT) && ch != vict->get_fighting())
			|| (!may_kill_here(ch, vict, NoArgument) && !IS_SET(extmode, GUARD_ATTACK)))//старжники агрят в мирках
		{
			continue;
		}

		kill_this = false;
		// Mobile too damage //обработка флага ТРУС
		if (IS_SET(extmode, CHECK_HITS)
			&& MOB_FLAGGED(ch, MOB_WIMPY)
			&& AWAKE(vict) && GET_HIT(ch) * 2 < GET_REAL_MAX_HIT(ch)) {
			continue;
		}

		// Mobile helpers... //ассист
		if ((vict->get_fighting())
			&& (vict->get_fighting() != ch)
			&& (IS_NPC(vict->get_fighting()))
			&& (!AFF_FLAGGED(vict->get_fighting(), EAffectFlag::AFF_CHARM))) {
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
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_SNEAK);
			}

			if (AFF_FLAGGED(vict, EAffectFlag::AFF_SNEAK)) {
				continue;
			}
		}

		if (IS_SET(extmode, SKIP_HIDING)) {
			skip_hiding(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE)) {
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_HIDE);
			}
		}

		if (IS_SET(extmode, SKIP_CAMOUFLAGE)) {
			skip_camouflage(vict, ch);
			if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE)) {
				AFF_FLAGS(vict).unset(EAffectFlag::AFF_CAMOUFLAGE);
			}
		}
		if (!CAN_SEE(ch, vict))
			continue;

		if (!kill_this && extra_aggr) {
			if (can_use_feat(vict, SILVER_TONGUED_FEAT)
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
			if (MOB_FLAGGED(best->get_fighting(), MOB_NOHELPS))
				return (false);
			act("$n вступил$g в битву на стороне $N1.", false, ch, nullptr, best->get_fighting(), kToRoom);
		}

		if (IS_SET(extmode, GUARD_ATTACK)) {
			act("'$N - за грехи свои ты заслуживаешь смерти!', сурово проговорил$g $n.",
				false, ch, nullptr, best, kToRoom);
			act("'Как страж этого города, я намерен$g привести приговор в исполнение немедленно. Защищайся!'",
				false, ch, nullptr, best, kToRoom);
		}

		if (!IS_NPC(best)) {
			struct Follower *f;
			// поиск клонов и отработка атаки в клона персонажа
			for (f = best->followers; f; f = f->next)
				if (MOB_FLAGGED(f->ch, MOB_CLONE))
					clone_number++;
			for (f = best->followers; f; f = f->next)
				if (IS_NPC(f->ch) && MOB_FLAGGED(f->ch, MOB_CLONE)
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
		if (!IS_NPC(vict) || !MAY_SEE(ch, ch, vict) || MOB_FLAGGED(vict, MOB_PROTECT)) {
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
	if (!ch || ch->in_room == kNowhere || !IS_NPC(ch) || !MAY_ATTACK(ch) || AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)) {
		return;
	}

	int mode = check_sneak ? SKIP_SNEAKING : 0;

	// ****************  Horde
	if (MOB_FLAGGED(ch, MOB_HORDE)) {
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
	if (MOB_FLAGGED(ch, MOB_GUARDIAN)) {
		perform_best_mob_attack(ch, SKIP_HIDING | SKIP_CAMOUFLAGE | SKIP_SNEAKING | GUARD_ATTACK);
		return;
	}

	// *****************  Mob Memory
	if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch)) {
		CharData *victim = nullptr;
		// Find memory in room
		const auto people_copy = world[ch->in_room]->people;
		for (auto vict_i = people_copy.begin(); vict_i != people_copy.end() && !victim; ++vict_i) {
			const auto vict = *vict_i;

			if (IS_NPC(vict)
				|| PRF_FLAGGED(vict, PRF_NOHASSLE)) {
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
							AFF_FLAGS(vict).unset(EAffectFlag::AFF_SNEAK);
						}
						if (AFF_FLAGGED(vict, EAffectFlag::AFF_SNEAK))
							continue;
					}
					skip_hiding(vict, ch);
					if (EXTRA_FLAGGED(vict, EXTRA_FAILHIDE)) {
						AFF_FLAGS(vict).unset(EAffectFlag::AFF_HIDE);
					}
					skip_camouflage(vict, ch);
					if (EXTRA_FLAGGED(vict, EXTRA_FAILCAMOUFLAGE)) {
						AFF_FLAGS(vict).unset(EAffectFlag::AFF_CAMOUFLAGE);
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
			if (GET_RACE(ch) != NPC_RACE_HUMAN) {
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
	if (MOB_FLAGGED(ch, MOB_HELPER)) {
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
	if (!IS_NPC(ch) || !GET_MOB_SPEC(ch)) {
		return true;
	}

	for (const auto vict : room->people) {
		if (IS_NPC(vict)
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
	obj->set_wear_flags(to_underlying(EWearFlag::ITEM_WEAR_TAKE));
	obj->set_weight(1);
	obj->set_cost(1);
	obj->set_rent_off(1);
	obj->set_rent_on(1);
	obj->set_timer(9999);

	obj->set_extra_flag(EExtraFlag::ITEM_NOSELL);
	obj->set_extra_flag(EExtraFlag::ITEM_NOLOCATE);
	obj->set_extra_flag(EExtraFlag::ITEM_NODECAY);
	obj->set_extra_flag(EExtraFlag::ITEM_SWIMMING);
	obj->set_extra_flag(EExtraFlag::ITEM_FLYING);

	return obj.get();
}

void extract_charmice(CharData *ch) {
	std::vector<ObjData *> objects;
	for (int i = 0; i < NUM_WEARS; ++i) {
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
		if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
			if (mob_index[GET_MOB_RNUM(ch)].func == nullptr) {
				log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
					GET_NAME(ch), GET_MOB_VNUM(ch));
				MOB_FLAGS(ch).unset(MOB_SPEC);
			} else {
				buf2[0] = '\0';
				if ((mob_index[GET_MOB_RNUM(ch)].func)(ch.get(), ch.get(), 0, buf2)) {
					return;    // go to next char
				}
			}
		}
		// Extract free horses
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_HORSE)
			&& MOB_FLAGGED(ch, MOB_MOUNTING)
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
			AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) ||
			AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD) || AFF_FLAGGED(ch, EAffectFlag::AFF_MAGICSTOPFIGHT) ||
			AFF_FLAGGED(ch, EAffectFlag::AFF_STOPFIGHT) || AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP)) {
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

			if (!IS_NPC(vict)
				&& CAN_SEE(ch, vict)) {
				max = true;
			}
		}

		// Mob attemp rest if it is not an angel
		if (!max && !MOB_FLAGGED(ch, MOB_NOREST) && !MOB_FLAGGED(ch, MOB_HORDE) &&
			GET_HIT(ch) < GET_REAL_MAX_HIT(ch) && !MOB_FLAGGED(ch, MOB_ANGEL) && !MOB_FLAGGED(ch, MOB_GHOST)
			&& GET_POS(ch) > EPosition::kRest) {
			act("$n присел$g отдохнуть.", false, ch.get(), 0, 0, kToRoom);
			GET_POS(ch) = EPosition::kRest;
		}

		// Mob return to default pos if full rested or if it is an angel
		if ((GET_HIT(ch) >= GET_REAL_MAX_HIT(ch)
			&& GET_POS(ch) != GET_DEFAULT_POS(ch))
			|| ((MOB_FLAGGED(ch, MOB_ANGEL)
				|| MOB_FLAGGED(ch, MOB_GHOST))
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
		if ((MOB_FLAGGED(ch, MOB_ANGEL))
			|| (MOB_FLAGGED(ch, MOB_GHOST))) {
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
				MOB_FLAGS(ch).unset(MOB_LIKE_DAY);    //Взял из make_horse
				MOB_FLAGS(ch).unset(MOB_LIKE_NIGHT);
				MOB_FLAGS(ch).unset(MOB_LIKE_FULLMOON);
				MOB_FLAGS(ch).unset(MOB_LIKE_WINTER);
				MOB_FLAGS(ch).unset(MOB_LIKE_SPRING);
				MOB_FLAGS(ch).unset(MOB_LIKE_SUMMER);
				MOB_FLAGS(ch).unset(MOB_LIKE_AUTUMN);
			}
			//Niker: LootCR// End
		}
		npc_wield(ch.get());
		npc_armor(ch.get());

		if (GET_POS(ch) == EPosition::kStand && NPC_FLAGGED(ch, NPC_INVIS)) {
			ch->set_affect(EAffectFlag::AFF_INVISIBLE);
		}

		if (GET_POS(ch) == EPosition::kStand && NPC_FLAGGED(ch, NPC_MOVEFLY)) {
			ch->set_affect(EAffectFlag::AFF_FLY);
		}

		if (GET_POS(ch) == EPosition::kStand && NPC_FLAGGED(ch, NPC_SNEAK)) {
			if (CalcCurrentSkill(ch.get(), ESkill::kSneak, 0) >= number(0, 100)) {
				ch->set_affect(EAffectFlag::AFF_SNEAK);
			} else {
				ch->remove_affect(EAffectFlag::AFF_SNEAK);
			}
			affect_total(ch.get());
		}

		if (GET_POS(ch) == EPosition::kStand && NPC_FLAGGED(ch, NPC_CAMOUFLAGE)) {
			if (CalcCurrentSkill(ch.get(), ESkill::kDisguise, 0) >= number(0, 100)) {
				ch->set_affect(EAffectFlag::AFF_CAMOUFLAGE);
			} else {
				ch->remove_affect(EAffectFlag::AFF_CAMOUFLAGE);
			}

			affect_total(ch.get());
		}

		door = BFS_ERROR;

		// Helpers go to some dest
		if (MOB_FLAGGED(ch, MOB_HELPER)
			&& !MOB_FLAGGED(ch, MOB_SENTINEL)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)
			&& !ch->has_master()
			&& GET_POS(ch) == EPosition::kStand) {
			for (found = false, door = 0; door < kDirMaxNumber; door++) {
				RoomData::exit_data_ptr rdata = EXIT(ch, door);
				if (!rdata
					|| rdata->to_room() == kNowhere
					|| !legal_dir(ch.get(), door, true, false)
					|| (is_room_forbidden(world[rdata->to_room()])
						&& !MOB_FLAGGED(ch, MOB_IGNORE_FORBIDDEN))
					|| IS_DARK(rdata->to_room())
					|| (MOB_FLAGGED(ch, MOB_STAY_ZONE)
						&& world[ch->in_room]->zone_rn != world[rdata->to_room()]->zone_rn)) {
					continue;
				}

				const auto room = world[rdata->to_room()];
				for (auto first : room->people) {
					if (IS_NPC(first)
						&& !AFF_FLAGGED(first, EAffectFlag::AFF_CHARM)
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
				door = BFS_ERROR;
			}
		}

		if (GET_DEST(ch) != kNowhere
			&& GET_POS(ch) > EPosition::kFight
			&& door == BFS_ERROR) {
			npc_group(ch.get());
			door = npc_walk(ch.get());
		}

		if (MEMORY(ch) && door == BFS_ERROR && GET_POS(ch) > EPosition::kFight && ch->get_skill(ESkill::kTrack))
			door = npc_track(ch.get());

		if (door == BFS_ALREADY_THERE) {
			do_aggressive_mob(ch.get(), false);
			return;
		}

		if (door == BFS_ERROR) {
			door = number(0, 18);
		}

		// Mob Movement
		if (!MOB_FLAGGED(ch, MOB_SENTINEL)
			&& GET_POS(ch) == EPosition::kStand
			&& (door >= 0 && door < kDirMaxNumber)
			&& EXIT(ch, door)
			&& EXIT(ch, door)->to_room() != kNowhere
			&& legal_dir(ch.get(), door, true, false)
			&& (!is_room_forbidden(world[EXIT(ch, door)->to_room()]) || MOB_FLAGGED(ch, MOB_IGNORE_FORBIDDEN))
			&& (!MOB_FLAGGED(ch, MOB_STAY_ZONE)
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
		if (MOB_FLAGGED(ch, MOB_MEMORY)
			&& MEMORY(ch)
			&& GET_POS(ch) > EPosition::kSleep
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)
			&& !ch->get_fighting()) {
			// Find memory in world
			for (auto names = MEMORY(ch); names && (GET_SPELL_MEM(ch, kSpellSummon) > 0
				|| GET_SPELL_MEM(ch, kSpellRelocate) > 0); names = names->next) {
				for (const auto &vict : character_list) {
					if (names->id == GET_IDNUM(vict)
						&& CAN_SEE(ch, vict) && !PRF_FLAGGED(vict, PRF_NOHASSLE)) {
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

	if (!IS_NPC(ch) ||
		IS_NPC(victim) ||
		PRF_FLAGGED(victim, PRF_NOHASSLE) ||
		!MOB_FLAGGED(ch, MOB_MEMORY) ||
		AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		return;

	for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
		if (tmp->id == GET_IDNUM(victim)) {
			if (tmp->time > 0)
				tmp->time = time(nullptr) + MOB_MEM_KOEFF * GET_REAL_INT(ch);
			present = true;
		}

	if (!present) {
		CREATE(tmp, 1);
		tmp->next = MEMORY(ch);
		tmp->id = GET_IDNUM(victim);
		tmp->time = time(nullptr) + MOB_MEM_KOEFF * GET_REAL_INT(ch);
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
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
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

	if (!IS_NPC(ch) || !vict || !MOB_FLAGGED(ch, MOB_GUARDIAN))
		return false;
//на всякий случай проверим, а вдруг моб да подевался куда из списка...
	auto it = guardian_list.find(GET_MOB_VNUM(ch));
	if (it == guardian_list.end())
		return false;

	tmp_guard = guardian_list[GET_MOB_VNUM(ch)];

	if ((tmp_guard.agro_all_agressors && AGRESSOR(vict)) ||
		(tmp_guard.agro_killers && PLR_FLAGGED(vict, PLR_KILLER)))
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
