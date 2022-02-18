/* ************************************************************************
*   File: act.movement.cpp                              Part of Bylins    *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */
#include "act_movement.h"

#include "game_mechanics/deathtrap.h"
#include "entities/entities_constants.h"
#include "fightsystem/fight.h"
#include "fightsystem/pk.h"
#include "fightsystem/mobact.h"
#include "handler.h"
#include "house.h"
#include "game_mechanics/named_stuff.h"
#include "obj_prototypes.h"
#include "administration/privilege.h"
#include "color.h"
#include "skills/pick.h"
#include "utils/random.h"
#include "structs/global_objects.h"

#include <cmath>


// external functs
void SetWait(CharData *ch, int waittime, int victim_in_room);
int find_eq_pos(CharData *ch, ObjData *obj, char *arg);
// local functions
void check_ice(int room);

const int Reverse[kDirMaxNumber] = {2, 3, 0, 1, 5, 4};
const char *DirIs[] =
	{
		"север",
		"восток",
		"юг",
		"запад",
		"вверх",
		"вниз",
		"\n"
	};

// check ice in room
int check_death_ice(int room, CharData * /*ch*/) {
	int sector, mass = 0, result = false;

	if (room == kNowhere)
		return (false);
	sector = SECT(room);
	if (sector != kSectWaterSwim && sector != kSectWaterNoswim)
		return (false);
	if ((sector = real_sector(room)) != kSectThinIce && sector != kSectNormalIce)
		return (false);

	for (const auto vict : world[room]->people) {
		if (!IS_NPC(vict)
			&& !AFF_FLAGGED(vict, EAffectFlag::AFF_FLY)) {
			mass += GET_WEIGHT(vict) + IS_CARRYING_W(vict);
		}
	}

	if (!mass) {
		return (false);
	}

	if ((sector == kSectThinIce && mass > 500) || (sector == kSectNormalIce && mass > 1500)) {
		const auto first_in_room = world[room]->first_character();

		act("Лед проломился под вашей тяжестью.", false, first_in_room, nullptr, nullptr, kToRoom);
		act("Лед проломился под вашей тяжестью.", false, first_in_room, nullptr, nullptr, kToChar);

		world[room]->weather.icelevel = 0;
		world[room]->ices = 2;
		GET_ROOM(room)->set_flag(ROOM_ICEDEATH);
		DeathTrap::add(world[room]);
	} else {
		return (false);
	}

	return (result);
}

// simple function to determine if char can walk on water
int has_boat(CharData *ch) {
	ObjData *obj;
	int i;

	//if (ROOM_IDENTITY(ch->in_room) == DEAD_SEA)
	//	return (1);

	if (IS_IMMORTAL(ch))
		return (true);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_WATERWALK))
		return (true);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_WATERBREATH))
		return (true);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_FLY))
		return (true);

	// non-wearable boats in inventory will do it
	for (obj = ch->carrying; obj; obj = obj->get_next_content()) {
		if (GET_OBJ_TYPE(obj) == ObjData::ITEM_BOAT
			&& (find_eq_pos(ch, obj, nullptr) < 0)) {
			return true;
		}
	}

	// and any boat you're wearing will do it too
	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i)
			&& GET_OBJ_TYPE(GET_EQ(ch, i)) == ObjData::ITEM_BOAT) {
			return true;
		}
	}

	return false;
}

void make_visible(CharData *ch, const EAffectFlag affect) {
	char to_room[kMaxStringLength], to_char[kMaxStringLength];

	*to_room = *to_char = 0;

	switch (affect) {
		case EAffectFlag::AFF_HIDE: strcpy(to_char, "Вы прекратили прятаться.\r\n");
			strcpy(to_room, "$n прекратил$g прятаться.\r\n");
			break;

		case EAffectFlag::AFF_CAMOUFLAGE: strcpy(to_char, "Вы прекратили маскироваться.\r\n");
			strcpy(to_room, "$n прекратил$g маскироваться.\r\n");
			break;

		default: break;
	}
	AFF_FLAGS(ch).unset(affect);
	CHECK_AGRO(ch) = true;
	if (*to_char)
		send_to_char(to_char, ch);
	if (*to_room)
		act(to_room, false, ch, nullptr, nullptr, kToRoom);
}

int skip_hiding(CharData *ch, CharData *vict) {
	int percent, prob;

	if (MAY_SEE(ch, vict, ch) && (AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE) || affected_by_spell(ch, kSpellHide))) {
		if (awake_hide(ch))    //if (affected_by_spell(ch, SPELL_HIDE))
		{
			send_to_char("Вы попытались спрятаться, но ваша экипировка выдала вас.\r\n", ch);
			affect_from_char(ch, kSpellHide);
			make_visible(ch, EAffectFlag::AFF_HIDE);
			EXTRA_FLAGS(ch).set(EXTRA_FAILHIDE);
		} else if (affected_by_spell(ch, kSpellHide)) {
			percent = number(1, 82 + GET_REAL_INT(vict));
			prob = CalcCurrentSkill(ch, ESkill::kHide, vict);
			if (percent > prob) {
				affect_from_char(ch, kSpellHide);
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE)) {
					ImproveSkill(ch, ESkill::kHide, false, vict);
					act("Вы не сумели остаться незаметным.", false, ch, nullptr, vict, kToChar);
				}
			} else {
				ImproveSkill(ch, ESkill::kHide, true, vict);
				act("Вам удалось остаться незаметным.\r\n", false, ch, nullptr, vict, kToChar);
				return (true);
			}
		}
	}
	return (false);
}

int skip_camouflage(CharData *ch, CharData *vict) {
	int percent, prob;

	if (MAY_SEE(ch, vict, ch)
		&& (AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE)
			|| affected_by_spell(ch, kSpellCamouflage))) {
		if (awake_camouflage(ch))    //if (affected_by_spell(ch,SPELL_CAMOUFLAGE))
		{
			send_to_char("Вы попытались замаскироваться, но ваша экипировка выдала вас.\r\n", ch);
			affect_from_char(ch, kSpellCamouflage);
			make_visible(ch, EAffectFlag::AFF_CAMOUFLAGE);
			EXTRA_FLAGS(ch).set(EXTRA_FAILCAMOUFLAGE);
		} else if (affected_by_spell(ch, kSpellCamouflage)) {
			percent = number(1, 82 + GET_REAL_INT(vict));
			prob = CalcCurrentSkill(ch, ESkill::kDisguise, vict);
			if (percent > prob) {
				affect_from_char(ch, kSpellCamouflage);
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE)) {
					ImproveSkill(ch, ESkill::kDisguise, false, vict);
					act("Вы не сумели правильно замаскироваться.", false, ch, nullptr, vict, kToChar);
				}
			} else {
				ImproveSkill(ch, ESkill::kDisguise, true, vict);
				act("Ваша маскировка оказалась на высоте.\r\n", false, ch, nullptr, vict, kToChar);
				return (true);
			}
		}
	}
	return (false);
}

int skip_sneaking(CharData *ch, CharData *vict) {
	int percent, prob, absolute_fail;
	bool try_fail;

	if (MAY_SEE(ch, vict, ch) && (AFF_FLAGGED(ch, EAffectFlag::AFF_SNEAK) || affected_by_spell(ch, kSpellSneak))) {
		if (awake_sneak(ch))    //if (affected_by_spell(ch,SPELL_SNEAK))
		{
			send_to_char("Вы попытались подкрасться, но ваша экипировка выдала вас.\r\n", ch);
			affect_from_char(ch, kSpellSneak);
			if (affected_by_spell(ch, kSpellHide))
				affect_from_char(ch, kSpellHide);
			make_visible(ch, EAffectFlag::AFF_SNEAK);
			EXTRA_FLAGS(ch).get(EXTRA_FAILSNEAK);
		} else if (affected_by_spell(ch, kSpellSneak)) {
			//if (can_use_feat(ch, STEALTHY_FEAT)) //тать или наем
			//percent = number(1, 140 + GET_REAL_INT(vict));
			//else
			percent = number(1,
							 (can_use_feat(ch, STEALTHY_FEAT) ? 102 : 112)
								 + (GET_REAL_INT(vict) * (vict->get_role(MOB_ROLE_BOSS) ? 3 : 1))
								 + (GET_REAL_LEVEL(vict) > 30 ? GET_REAL_LEVEL(vict) : 0));
			prob = CalcCurrentSkill(ch, ESkill::kSneak, vict);

			int catch_level = (GET_REAL_LEVEL(vict) - GET_REAL_LEVEL(ch));
			if (catch_level > 5) {
				//5% шанс фэйла при prob==200 всегда, при prob = 100 - 10%, если босс, шанс множим на 5
				absolute_fail = ((200 - prob) / 20 + 5) * (vict->get_role(MOB_ROLE_BOSS) ? 5 : 1);
				try_fail = number(1, 100) < absolute_fail;
			} else
				try_fail = false;

			if ((percent > prob) || try_fail) {
				affect_from_char(ch, kSpellSneak);
				if (affected_by_spell(ch, kSpellHide))
					affect_from_char(ch, kSpellHide);
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_SNEAK)) {
					ImproveSkill(ch, ESkill::kSneak, false, vict);
					act("Вы не сумели пробраться незаметно.", false, ch, nullptr, vict, kToChar);
				}
			} else {
				ImproveSkill(ch, ESkill::kSneak, true, vict);
				act("Вам удалось прокрасться незаметно.\r\n", false, ch, nullptr, vict, kToChar);
				return (true);
			}
		}
	}
	return (false);
}

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
/*
 * Check for special routines (North is 1 in command list, but 0 here) Note
 * -- only check if following; this avoids 'double spec-proc' bug
 */

int real_forest_paths_sect(int sect) {
	switch (sect) {
		case kSectForest: return kSectField;
			break;
		case kSectForestSnow: return kSectFieldSnow;
			break;
		case kSectForestRain: return kSectFieldRain;
			break;
	}
	return sect;
}

int real_mountains_paths_sect(int sect) {
	switch (sect) {
		case kSectHills:
		case kSectMountain: return kSectField;
			break;
		case kSectHillsRain: return kSectFieldRain;
			break;
		case kSectHillsSnow:
		case kSectMountainSnow: return kSectFieldSnow;
			break;
	}
	return sect;
}

int calculate_move_cost(CharData *ch, int dir) {
	// move points needed is avg. move loss for src and destination sect type
	auto ch_inroom = real_sector(ch->in_room);
	auto ch_toroom = real_sector(EXIT(ch, dir)->to_room());

	if (can_use_feat(ch, FOREST_PATHS_FEAT)) {
		ch_inroom = real_forest_paths_sect(ch_inroom);
		ch_toroom = real_forest_paths_sect(ch_toroom);
	}

	if (can_use_feat(ch, MOUNTAIN_PATHS_FEAT)) {
		ch_inroom = real_mountains_paths_sect(ch_inroom);
		ch_toroom = real_mountains_paths_sect(ch_toroom);
	}

	int need_movement = (IS_FLY(ch) || ch->ahorse()) ? 1 :
						(movement_loss[ch_inroom] + movement_loss[ch_toroom]) / 2;

	if (IS_IMMORTAL(ch))
		need_movement = 0;
	else if (affected_by_spell(ch, kSpellCamouflage))
		need_movement += CAMOUFLAGE_MOVES;
	else if (affected_by_spell(ch, kSpellSneak))
		need_movement += SNEAK_MOVES;

	return need_movement;
}

int legal_dir(CharData *ch, int dir, int need_specials_check, int show_msg) {
	buf2[0] = '\0';
	if (need_specials_check && special(ch, dir + 1, buf2, 1))
		return (false);

	// если нпц идет по маршруту - пропускаем проверку дверей
	const bool npc_roamer = IS_NPC(ch) && (GET_DEST(ch) != kNowhere) && (EXIT(ch, dir) && EXIT(ch, dir)->to_room() != kNowhere);
	if (!npc_roamer) {
		if (!CAN_GO(ch, dir)) {
			return (false);
		}
	}

	// не пускать в ванрумы после пк, если его там прибьет сразу
	if (DeathTrap::check_tunnel_death(ch, EXIT(ch, dir)->to_room())) {
		if (show_msg) {
			send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
		}
		return (false);
	}

	// charmed
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& ch->has_master()
		&& ch->in_room == ch->get_master()->in_room) {
		if (show_msg) {
			send_to_char("Вы не можете покинуть свой идеал.\r\n", ch);
			act("$N попытал$U покинуть вас.", false, ch->get_master(), nullptr, ch, kToChar);
		}
		return (false);
	}

	// check NPC's
	if (IS_NPC(ch)) {
		if (GET_DEST(ch) != kNowhere) {
			return (true);
		}

		//  if this room or the one we're going to needs a boat, check for one */
		if (!MOB_FLAGGED(ch, MOB_SWIMMING)
			&& !MOB_FLAGGED(ch, MOB_FLYING)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_FLY)
			&& (real_sector(ch->in_room) == kSectWaterNoswim
				|| real_sector(EXIT(ch, dir)->to_room()) == kSectWaterNoswim)) {
			if (!has_boat(ch)) {
				return (false);
			}
		}

		// Добавляем проверку на то что моб может вскрыть дверь
		if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) &&
			!MOB_FLAGGED(ch, MOB_OPENDOOR))
			return (false);

		if (!MOB_FLAGGED(ch, MOB_FLYING) &&
			!AFF_FLAGGED(ch, EAffectFlag::AFF_FLY) && SECT(EXIT(ch, dir)->to_room()) == kSectOnlyFlying)
			return (false);

		if (MOB_FLAGGED(ch, MOB_ONLYSWIMMING) &&
			!(real_sector(EXIT(ch, dir)->to_room()) == kSectWaterSwim ||
				real_sector(EXIT(ch, dir)->to_room()) == kSectWaterNoswim ||
				real_sector(EXIT(ch, dir)->to_room()) == kSectUnderwater))
			return (false);

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_NOMOB) &&
			!IS_HORSE(ch) &&
			!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && !(MOB_FLAGGED(ch, MOB_ANGEL) || MOB_FLAGGED(ch, MOB_GHOST))
			&& !MOB_FLAGGED(ch, MOB_IGNORNOMOB))
			return (false);

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_DEATH) && !IS_HORSE(ch))
			return (false);

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_GODROOM))
			return (false);

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_NOHORSE) && IS_HORSE(ch))
			return (false);
	} else {
		//Вход в замок
		if (ROOM_FLAGGED(ch->in_room, ROOM_ATRIUM)) {
			if (!Clan::MayEnter(ch, EXIT(ch, dir)->to_room(), HCE_ATRIUM)) {
				if (show_msg)
					send_to_char("Частная собственность! Вход воспрещен!\r\n", ch);
				return (false);
			}
		}

		if (real_sector(ch->in_room) == kSectWaterNoswim ||
			real_sector(EXIT(ch, dir)->to_room()) == kSectWaterNoswim) {
			if (!has_boat(ch)) {
				if (show_msg)
					send_to_char("Вам нужна лодка, чтобы попасть туда.\r\n", ch);
				return (false);
			}
		}
		if (real_sector(EXIT(ch, dir)->to_room()) == kSectOnlyFlying
			&& !IS_GOD(ch)
			&& !AFF_FLAGGED(ch, EAffectFlag::AFF_FLY)) {
			if (show_msg) {
				send_to_char("Туда можно только влететь.\r\n", ch);
			}
			return (false);
		}

		// если там ДТ и чар верхом на пони
		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_DEATH) && ch->ahorse()) {
			if (show_msg) {
				// я весьма костоязычен, исправьте кто-нибудь на нормальную
				// мессагу, антуражненькую
				send_to_char("Ваш скакун не хочет идти туда.\r\n", ch);
			}
			return (false);
		}

		const auto need_movement = calculate_move_cost(ch, dir);
		if (GET_MOVE(ch) < need_movement) {
			if (need_specials_check
				&& ch->has_master()) {
				if (show_msg) {
					send_to_char("Вы слишком устали, чтобы следовать туда.\r\n", ch);
				}
			} else {
				if (show_msg) {
					send_to_char("Вы слишком устали.\r\n", ch);
				}
			}
			return (false);
		}
		//Вход в замок
		if (ROOM_FLAGGED(ch->in_room, ROOM_ATRIUM)) {
			if (!Clan::MayEnter(ch, EXIT(ch, dir)->to_room(), HCE_ATRIUM)) {
				if (show_msg)
					send_to_char("Частная собственность! Вход воспрещен!\r\n", ch);
				return (false);
			}
		}

		//чтобы конь не лез в комнату с флагом !лошадь
		if (ch->ahorse() && !legal_dir(ch->get_horse(), dir, need_specials_check, false)) {
			if (show_msg) {
				act("$Z $N отказывается туда идти, и вам пришлось соскочить.",
					false, ch, nullptr, ch->get_horse(), kToChar);
				ch->dismount();
			}
		}
		//проверка на ванрум: скидываем игрока с коня, если там незанято
		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_TUNNEL) &&
			(num_pc_in_room((world[EXIT(ch, dir)->to_room()])) > 0)) {
			if (show_msg)
				send_to_char("Слишком мало места.\r\n", ch);
			return (false);
		}

		if (ch->ahorse() && GET_HORSESTATE(ch->get_horse()) <= 0) {
			if (show_msg)
				act("$Z $N загнан$G настолько, что не может нести вас на себе.",
					false, ch, nullptr, ch->get_horse(), kToChar);
			return (false);
		}

		if (ch->ahorse()
			&& (AFF_FLAGGED(ch->get_horse(), EAffectFlag::AFF_HOLD)
				|| AFF_FLAGGED(ch->get_horse(), EAffectFlag::AFF_SLEEP))) {
			if (show_msg)
				act("$Z $N не в состоянии нести вас на себе.\r\n", false, ch, nullptr, ch->get_horse(), kToChar);
			return (false);
		}

		if (ch->ahorse()
			&& (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_TUNNEL)
				|| ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_NOHORSE))) {
			if (show_msg)
				act("$Z $N не в состоянии пройти туда.\r\n", false, ch, nullptr, ch->get_horse(), kToChar);
			return false;
		}

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ROOM_GODROOM) && !IS_GRGOD(ch)) {
			if (show_msg)
				send_to_char("Вы не столь Божественны, как вам кажется!\r\n", ch);
			return (false);
		}

		for (const auto tch : world[ch->in_room]->people) {
			if (!IS_NPC(tch))
				continue;
			if (NPC_FLAGGED(tch, 1 << dir)
				&& AWAKE(tch)
				&& GET_POS(tch) > EPosition::kSleep
				&& CAN_SEE(tch, ch)
				&& !AFF_FLAGGED(tch, EAffectFlag::AFF_CHARM)
				&& !AFF_FLAGGED(tch, EAffectFlag::AFF_HOLD)
				&& !IS_GRGOD(ch)) {
				if (show_msg) {
					act("$N преградил$G вам путь.", false, ch, nullptr, tch, kToChar);
				}

				return false;
			}
		}
	}

	return true;
}

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)
#define MAX_DRUNK_SONG 6
#define MAX_DRUNK_VOICE 5

void performDunkSong(CharData *ch) {
	const char *drunk_songs[MAX_DRUNK_SONG] = {"\"Шумел камыш, и-к-к..., деревья гнулися\"",
											   "\"Куда ты, тропинка, меня завела\"",
											   "\"Пабабам, пара пабабам\"",
											   "\"А мне любое море по колено\"",
											   "\"Не жди меня мама, хорошего сына\"",
											   "\"Бываааали дниии, весеееелыя\"",
	};
	const char *drunk_voice[MAX_DRUNK_VOICE] = {" - затянул$g $n",
												" - запел$g $n.",
												" - прохрипел$g $n.",
												" - зычно заревел$g $n.",
												" - разухабисто протянул$g $n.",
	};
	// орем песни
	if (!ch->get_fighting() && number(10, 24) < GET_COND(ch, DRUNK)) {
		sprintf(buf, "%s", drunk_songs[number(0, MAX_DRUNK_SONG - 1)]);
		send_to_char(buf, ch);
		send_to_char("\r\n", ch);
		strcat(buf, drunk_voice[number(0, MAX_DRUNK_VOICE - 1)]);
		act(buf, false, ch, nullptr, nullptr, kToRoom | kToNotDeaf);
		affect_from_char(ch, kSpellSneak);
		affect_from_char(ch, kSpellHide);
		affect_from_char(ch, kSpellCamouflage);
	}
}

int calcDrunkDirection(CharData *ch, int direction, bool need_specials_check) {

	int drunk_move = direction;
	//пересчет направления, в зависимости от степени опьянения
	if (!IS_NPC(ch)
		&& GET_COND(ch, DRUNK) >= CHAR_MORTALLY_DRUNKED
		&& !ch->ahorse()
		&& GET_COND(ch, DRUNK) >= number(CHAR_DRUNKED, 50)) {
		int possibleDirs[kDirMaxNumber];
		int correct_dirs = 0;

		for (auto i = 0; i < kDirMaxNumber; ++i) {
			if (legal_dir(ch, i, need_specials_check, true)) {
				possibleDirs[correct_dirs] = i;
				++correct_dirs;
			}
		}

		if (correct_dirs > 0
			&& !bernoulli_trial(std::pow((1.0 - static_cast<double>(correct_dirs) / kDirMaxNumber), kDirMaxNumber))) {
			drunk_move = possibleDirs[number(0, correct_dirs - 1)];
		}
	}

	if (direction != drunk_move) {
		sprintf(buf, "Ваши ноги не хотят слушаться вас...\r\n");
		send_to_char(buf, ch);
	}
	return drunk_move;
}

int do_simple_move(CharData *ch, int dir, int need_specials_check, CharData *leader, bool is_flee) {
	struct TrackData *track;
	RoomRnum was_in, go_to;
	int i, invis = 0, use_horse = 0, is_horse = 0, direction = 0;
	int mob_rnum = -1;
	CharData *horse = nullptr;

	if (ch->purged()) {
		return false;
	}
	// если не моб и не ведут за ручку - проверка на пьяные выкрутасы
	if (!IS_NPC(ch) && !leader) {
		dir = calcDrunkDirection(ch, dir, need_specials_check);
	}
	performDunkSong(ch);

	if (!legal_dir(ch, dir, need_specials_check, true))
		return (false);

	if (!entry_mtrigger(ch))
		return (false);

	if (!enter_wtrigger(world[EXIT(ch, dir)->to_room()], ch, dir))
		return (false);

	// Now we know we're allow to go into the room.
	if (!IS_IMMORTAL(ch) && !IS_NPC(ch))
		GET_MOVE(ch) -= calculate_move_cost(ch, dir);

	i = MUD::Skills()[ESkill::kSneak].difficulty;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SNEAK) && !is_flee) {
		if (IS_NPC(ch))
			invis = 1;
		else if (awake_sneak(ch)) {
			affect_from_char(ch, kSpellSneak);
		} else if (!affected_by_spell(ch, kSpellSneak) || CalcCurrentSkill(ch, ESkill::kSneak, nullptr) >= number(1, i))
			invis = 1;
	}

	i = MUD::Skills()[ESkill::kDisguise].difficulty;
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE) && !is_flee) {
		if (IS_NPC(ch))
			invis = 1;
		else if (awake_camouflage(ch)) {
			affect_from_char(ch, kSpellCamouflage);
		} else if (!affected_by_spell(ch, kSpellCamouflage) ||
			CalcCurrentSkill(ch, ESkill::kDisguise, nullptr) >= number(1, i))
			invis = 1;
	}

	if (!is_flee) {
		sprintf(buf, "Вы поплелись %s%s.", leader ? "следом за $N4 " : "", DirsTo[dir]);
		act(buf, false, ch, nullptr, leader, kToChar);
	}
	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SENTINEL) && !IS_CHARMICE(ch) && ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
		return false;
	was_in = ch->in_room;
	go_to = world[was_in]->dir_option[dir]->to_room();
	direction = dir + 1;
	use_horse = ch->ahorse() && ch->has_horse(false)
		&& (IN_ROOM(ch->get_horse()) == was_in || IN_ROOM(ch->get_horse()) == go_to);
	is_horse = IS_HORSE(ch)
		&& ch->has_master()
		&& !AFF_FLAGGED(ch->get_master(), EAffectFlag::AFF_INVISIBLE)
		&& (IN_ROOM(ch->get_master()) == was_in
			|| IN_ROOM(ch->get_master()) == go_to);

	if (!invis && !is_horse) {
		if (is_flee)
			strcpy(smallBuf, "сбежал$g");
		else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVERUN))
			strcpy(smallBuf, "убежал$g");
		else if ((!use_horse && AFF_FLAGGED(ch, EAffectFlag::AFF_FLY))
			|| (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEFLY))) {
			strcpy(smallBuf, "улетел$g");
		} else if (IS_NPC(ch)
			&& NPC_FLAGGED(ch, NPC_MOVESWIM)
			&& (real_sector(was_in) == kSectWaterSwim
				|| real_sector(was_in) == kSectWaterNoswim
				|| real_sector(was_in) == kSectUnderwater)) {
			strcpy(smallBuf, "уплыл$g");
		} else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEJUMP))
			strcpy(smallBuf, "ускакал$g");
		else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVECREEP))
			strcpy(smallBuf, "уполз$q");
		else if (real_sector(was_in) == kSectWaterSwim
			|| real_sector(was_in) == kSectWaterNoswim
			|| real_sector(was_in) == kSectUnderwater) {
			strcpy(smallBuf, "уплыл$g");
		} else if (use_horse) {
			horse = ch->get_horse();
			if (horse && AFF_FLAGGED(horse, EAffectFlag::AFF_FLY))
				strcpy(smallBuf, "улетел$g");
			else
				strcpy(smallBuf, "уехал$g");
		} else
			strcpy(smallBuf, "уш$y");

		if (is_flee && !IS_NPC(ch) && can_use_feat(ch, WRIGGLER_FEAT))
			sprintf(buf2, "$n %s.", smallBuf);
		else
			sprintf(buf2, "$n %s %s.", smallBuf, DirsTo[dir]);
		act(buf2, true, ch, nullptr, nullptr, kToRoom);
	}

	if (invis && !is_horse) {
		act("Кто-то тихо удалился отсюда.", true, ch, nullptr, nullptr, kToRoomSensors);
	}

	if (ch->ahorse())
		horse = ch->get_horse();

	// Если сбежали, и по противнику никто не бьет, то убираем с него аттаку
	if (is_flee) {
		stop_fighting(ch, true);
	}

	if (!IS_NPC(ch) && IS_BITS(ch->track_dirs, dir)) {
		send_to_char("Вы двинулись по следу.\r\n", ch);
		ImproveSkill(ch, ESkill::kTrack, true, nullptr);
	}

	char_from_room(ch);
	//затычка для бегства. чтоьы не отрабатывал MSDP протокол
	if (is_flee && !IS_NPC(ch) && !can_use_feat(ch, CALMNESS_FEAT))
		char_flee_to_room(ch, go_to);
	else
		char_to_room(ch, go_to);
	if (horse) {
		GET_HORSESTATE(horse) -= 1;
		char_from_room(horse);
		char_to_room(horse, go_to);
	}

	if (PRF_FLAGGED(ch, PRF_BLIND)) {
		for (int i = 0; i < kDirMaxNumber; i++) {
			if (CAN_GO(ch, i)
				|| (EXIT(ch, i)
					&& EXIT(ch, i)->to_room() != kNowhere)) {
				const auto &rdata = EXIT(ch, i);
				if (ROOM_FLAGGED(rdata->to_room(), ROOM_DEATH)) {
					send_to_char("\007 Внимание, рядом гиблое место!\r\n", ch);
				}
			}
		}
	}

	if (!invis && !is_horse) {
		if (is_flee
			|| (IS_NPC(ch)
				&& NPC_FLAGGED(ch, NPC_MOVERUN))) {
			strcpy(smallBuf, "прибежал$g");
		} else if ((!use_horse && AFF_FLAGGED(ch, EAffectFlag::AFF_FLY))
			|| (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEFLY))) {
			strcpy(smallBuf, "прилетел$g");
		} else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVESWIM)
			&& (real_sector(go_to) == kSectWaterSwim
				|| real_sector(go_to) == kSectWaterNoswim
				|| real_sector(go_to) == kSectUnderwater)) {
			strcpy(smallBuf, "приплыл$g");
		} else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVEJUMP))
			strcpy(smallBuf, "прискакал$g");
		else if (IS_NPC(ch) && NPC_FLAGGED(ch, NPC_MOVECREEP))
			strcpy(smallBuf, "приполз$q");
		else if (real_sector(go_to) == kSectWaterSwim
			|| real_sector(go_to) == kSectWaterNoswim
			|| real_sector(go_to) == kSectUnderwater) {
			strcpy(smallBuf, "приплыл$g");
		} else if (use_horse) {
			horse = ch->get_horse();
			if (horse && AFF_FLAGGED(horse, EAffectFlag::AFF_FLY)) {
				strcpy(smallBuf, "прилетел$g");
			} else {
				strcpy(smallBuf, "приехал$g");
			}

		} else
			strcpy(smallBuf, "приш$y");

		//log("%s-%d",GET_NAME(ch),ch->in_room);
		sprintf(buf2, "$n %s %s.", smallBuf, DirsFrom[dir]);
		//log(buf2);
		act(buf2, true, ch, nullptr, nullptr, kToRoom);
		//log("ACT OK !");
	};

	if (invis && !is_horse) {
		act("Кто-то тихо подкрался сюда.", true, ch, nullptr, nullptr, kToRoomSensors);
	}

	if (ch->desc != nullptr)
		look_at_room(ch, 0);

	if (!IS_NPC(ch))
		room_affect_process_on_entry(ch, ch->in_room);

	if (DeathTrap::check_death_trap(ch)) {
		if (horse) {
			extract_char(horse, false);
		}
		return (false);
	}

	if (check_death_ice(go_to, ch)) {
		return (false);
	}

	if (DeathTrap::tunnel_damage(ch)) {
		return (false);
	}

	greet_mtrigger(ch, dir);
	greet_otrigger(ch, dir);

	// add track info
	if (!AFF_FLAGGED(ch, EAffectFlag::AFF_NOTRACK)
		&& (!IS_NPC(ch)
			|| (mob_rnum = GET_MOB_RNUM(ch)) >= 0)) {
		for (track = world[go_to]->track; track; track = track->next) {
			if ((IS_NPC(ch) && IS_SET(track->track_info, TRACK_NPC) && track->who == mob_rnum)
				|| (!IS_NPC(ch) && !IS_SET(track->track_info, TRACK_NPC) && track->who == GET_IDNUM(ch))) {
				break;
			}
		}

		if (!track && !ROOM_FLAGGED(go_to, ROOM_NOTRACK)) {
			CREATE(track, 1);
			track->track_info = IS_NPC(ch) ? TRACK_NPC : 0;
			track->who = IS_NPC(ch) ? mob_rnum : GET_IDNUM(ch);
			track->next = world[go_to]->track;
			world[go_to]->track = track;
		}

		if (track) {
			SET_BIT(track->time_income[Reverse[dir]], 1);
			if (affected_by_spell(ch, kSpellLightWalk) && !ch->ahorse())
				if (AFF_FLAGGED(ch, EAffectFlag::AFF_LIGHT_WALK))
					track->time_income[Reverse[dir]] <<= number(15, 30);
			REMOVE_BIT(track->track_info, TRACK_HIDE);
		}

		for (track = world[was_in]->track; track; track = track->next)
			if ((IS_NPC(ch) && IS_SET(track->track_info, TRACK_NPC)
				&& track->who == mob_rnum) || (!IS_NPC(ch)
				&& !IS_SET(track->track_info, TRACK_NPC)
				&& track->who == GET_IDNUM(ch)))
				break;

		if (!track && !ROOM_FLAGGED(was_in, ROOM_NOTRACK)) {
			CREATE(track, 1);
			track->track_info = IS_NPC(ch) ? TRACK_NPC : 0;
			track->who = IS_NPC(ch) ? mob_rnum : GET_IDNUM(ch);
			track->next = world[was_in]->track;
			world[was_in]->track = track;
		}
		if (track) {
			SET_BIT(track->time_outgone[dir], 1);
			if (affected_by_spell(ch, kSpellLightWalk) && !ch->ahorse())
				if (AFF_FLAGGED(ch, EAffectFlag::AFF_LIGHT_WALK))
					track->time_outgone[dir] <<= number(15, 30);
			REMOVE_BIT(track->track_info, TRACK_HIDE);
		}
	}

	// hide improovment
	if (IS_NPC(ch)) {
		for (const auto vict : world[ch->in_room]->people) {
			if (!IS_NPC(vict)) {
				skip_hiding(vict, ch);
			}
		}
	}

	income_mtrigger(ch, direction - 1);

	if (ch->purged())
		return false;

	// char income, go mobs action
	if (!IS_NPC(ch)) {
		for (const auto vict : world[ch->in_room]->people) {
			if (!IS_NPC(vict)) {
				continue;
			}

			if (!CAN_SEE(vict, ch)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_SNEAK)
				|| AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE)
				|| vict->get_fighting()
				|| GET_POS(vict) < EPosition::kRest) {
				continue;
			}

			// AWARE mobs
			if (MOB_FLAGGED(vict, MOB_AWARE)
				&& GET_POS(vict) < EPosition::kFight
				&& !AFF_FLAGGED(vict, EAffectFlag::AFF_HOLD)
				&& GET_POS(vict) > EPosition::kSleep) {
				act("$n поднял$u.", false, vict, nullptr, nullptr, kToRoom | kToArenaListen);
				GET_POS(vict) = EPosition::kStand;
			}
		}
	}

	// If flee - go agressive mobs
	if (!IS_NPC(ch)
		&& is_flee) {
		do_aggressive_room(ch, false);
	}

	return direction;
}

int perform_move(CharData *ch, int dir, int need_specials_check, int checkmob, CharData *master) {
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BANDAGE)) {
		send_to_char("Перевязка была прервана!\r\n", ch);
		affect_from_char(ch, kSpellBandage);
	}
	ch->set_motion(true);

	RoomRnum was_in;
	struct Follower *k, *next;

	if (ch == nullptr || dir < 0 || dir >= kDirMaxNumber || ch->get_fighting())
		return false;
	else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room() == kNowhere)
		send_to_char("Вы не сможете туда пройти...\r\n", ch);
	else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) {
		if (EXIT(ch, dir)->keyword) {
			sprintf(buf2, "Закрыто (%s).\r\n", EXIT(ch, dir)->keyword);
			send_to_char(buf2, ch);
		} else
			send_to_char("Закрыто.\r\n", ch);
	} else {
		if (!ch->followers) {
			if (!do_simple_move(ch, dir, need_specials_check, master, false))
				return (false);
		} else {
			was_in = ch->in_room;
			// When leader mortally drunked - he change direction
			// So returned value set to false or DIR + 1
			if (!(dir = do_simple_move(ch, dir, need_specials_check, master, false)))
				return (false);

			--dir;
			for (k = ch->followers; k && k->ch->get_master(); k = next) {
				next = k->next;
				if (k->ch->in_room == was_in
					&& !k->ch->get_fighting()
					&& HERE(k->ch)
					&& !GET_MOB_HOLD(k->ch)
					&& AWAKE(k->ch)
					&& (IS_NPC(k->ch)
						|| (!PLR_FLAGGED(k->ch, PLR_MAILING)
							&& !PLR_FLAGGED(k->ch, PLR_WRITING)))
					&& (!IS_HORSE(k->ch)
						|| !AFF_FLAGGED(k->ch, EAffectFlag::AFF_TETHERED))) {
					if (GET_POS(k->ch) < EPosition::kStand) {
						if (IS_NPC(k->ch)
							&& IS_NPC(k->ch->get_master())
							&& GET_POS(k->ch) > EPosition::kSleep
							&& !GET_WAIT(k->ch)) {
							act("$n поднял$u.", false, k->ch, nullptr, nullptr, kToRoom | kToArenaListen);
							GET_POS(k->ch) = EPosition::kStand;
						} else {
							continue;
						}
					}
//                   act("Вы поплелись следом за $N4.",false,k->ch,0,ch,TO_CHAR);
					perform_move(k->ch, dir, 1, false, ch);
				}
			}
		}
		if (checkmob) {
			do_aggressive_room(ch, true);
		}
		return (true);
	}
	return (false);
}

void do_move(CharData *ch, char * /*argument*/, int/* cmd*/, int subcmd) {
	/*
	 * This is basically a mapping of cmd numbers to perform_move indices.
	 * It cannot be done in perform_move because perform_move is called
	 * by other functions which do not require the remapping.
	 */
	perform_move(ch, subcmd - 1, 0, true, nullptr);
}

void do_hidemove(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int dir = 0, sneaking = affected_by_spell(ch, kSpellSneak);

	skip_spaces(&argument);
	if (!ch->get_skill(ESkill::kSneak)) {
		send_to_char("Вы не умеете этого.\r\n", ch);
		return;
	}

	if (!*argument) {
		send_to_char("И куда это вы направляетесь?\r\n", ch);
		return;
	}

	if ((dir = search_block(argument, dirs, false)) < 0 && (dir = search_block(argument, DirIs, false)) < 0) {
		send_to_char("Неизвестное направление.\r\n", ch);
		return;
	}
	if (ch->ahorse()) {
		act("Вам мешает $N.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	if (!sneaking) {
		Affect<EApplyLocation> af;
		af.type = kSpellSneak;
		af.location = EApplyLocation::APPLY_NONE;
		af.modifier = 0;
		af.duration = 1;
		const int calculated_skill = CalcCurrentSkill(ch, ESkill::kSneak, nullptr);
		const int chance = number(1, MUD::Skills()[ESkill::kSneak].difficulty);
		af.bitvector = (chance < calculated_skill) ? to_underlying(EAffectFlag::AFF_SNEAK) : 0;
		af.battleflag = 0;
		affect_join(ch, af, false, false, false, false);
	}
	perform_move(ch, dir, 0, true, nullptr);
	if (!sneaking || affected_by_spell(ch, kSpellGlitterDust)) {
		affect_from_char(ch, kSpellSneak);
	}
}

#define DOOR_IS_OPENABLE(ch, obj, door)    ((obj) ? \
            ((GET_OBJ_TYPE(obj) == ObjData::ITEM_CONTAINER) && \
            OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
            (EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS(ch, door)    ((EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))

#define DOOR_IS_OPEN(ch, obj, door)    ((obj) ? \
            (!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :\
            (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_BROKEN(ch, obj, door)    ((obj) ? \
    (OBJVAL_FLAGGED(obj, CONT_BROKEN)) :\
    (EXIT_FLAGGED(EXIT(ch, door), EX_BROKEN)))
#define DOOR_IS_UNLOCKED(ch, obj, door)    ((obj) ? \
            (!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :\
            (!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
    (OBJVAL_FLAGGED(obj, CONT_PICKPROOF) || OBJVAL_FLAGGED(obj, CONT_BROKEN)) : \
    (EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF) || EXIT_FLAGGED(EXIT(ch, door), EX_BROKEN)))

#define DOOR_IS_CLOSED(ch, obj, door)    (!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door)    (!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)        ((obj) ? (GET_OBJ_VAL(obj, 2)) : \
                    (EXIT(ch, door)->key))
#define DOOR_LOCK(ch, obj, door)    ((obj) ? (GET_OBJ_VAL(obj, 1)) : \
                    (EXIT(ch, door)->exit_info))
#define DOOR_LOCK_COMPLEX(ch, obj, door) ((obj) ? \
            (GET_OBJ_VAL(obj,3)) :\
            (EXIT(ch, door)->lock_complexity))

int find_door(CharData *ch, const char *type, char *dir, DOOR_SCMD scmd) {
	int door;
	bool found = false;

	if (*dir)  //Указано направление (второй аргумент)
	{
		//Проверяем соответствует ли аргумент английским или русским направлениям
		if ((door = search_block(dir, dirs, false)) == -1
			&& (door = search_block(dir, DirIs, false)) == -1)    // Partial Match
		{
			//strcpy(doorbuf,"Уточните направление.\r\n");
			return (FD_WRONG_DIR); //НЕВЕРНОЕ НАПРАВЛЕНИЕ
		}
		if (EXIT(ch, door)) { //Проверяем есть ли такая дверь в указанном направлении
			if (EXIT(ch, door)->keyword && EXIT(ch, door)->vkeyword) {//Дверь как-то по-особенному называется?
				if (isname(type, EXIT(ch, door)->keyword) || isname(type, EXIT(ch, door)->vkeyword))
					//Первый аргумент соответствует именительному или винительному алиасу двери
					return (door);
				else
					return (FD_WRONG_DOOR_NAME); //НЕ ПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ В ЭТОМ НАПРАВЛЕНИИ
			} else if (utils::IsAbbrev(type, "дверь") || utils::IsAbbrev(type, "door")) {
				//Аргумент соответствует "дверь" или "door" и есть в указанном направлении
				return (door);
			} else
				//Дверь с названием "дверь" есть, но аргумент не соответствует
				return (FD_DOORNAME_WRONG);
		} else {
			return (FD_NO_DOOR_GIVEN_DIR); //В ЭТОМ НАПРАВЛЕНИИ НЕТ ДВЕРЕЙ
		}
	} else //Направления не указано, ищем дверь по названию
	{
		if (!*type) { //Названия не указано
			return (FD_DOORNAME_EMPTY); //НЕ УКАЗАНО АРГУМЕНТОВ
		}
		for (door = 0; door < kDirMaxNumber; door++) {//Проверяем все направления, не найдется ли двери?
			found = false;
			if (EXIT(ch, door)) {//Есть выход в этом направлении
				if (EXIT(ch, door)->keyword && EXIT(ch, door)->vkeyword) { //Дверь как-то по-особенному называется?
					if (isname(type, EXIT(ch, door)->keyword) || isname(type, EXIT(ch, door)->vkeyword))
						//Аргумент соответствует имени этой двери
						found = true;
				} else if (DOOR_IS(ch, door) && (utils::IsAbbrev(type, "дверь") || utils::IsAbbrev(type, "door")))
					//Дверь не имеет особых алиасов, аргумент соответствует двери
					found = true;
			}
			// дверь найдена проверяем ейный статус.
			// скипуем попытку закрыть закрытую и открыть открытую.
			// остальные статусы двери автоматом не проверяются.
			if (found) {
				if ((scmd == SCMD_OPEN && DOOR_IS_OPEN(ch, nullptr, door)) ||
					(scmd == SCMD_CLOSE && DOOR_IS_CLOSED(ch, nullptr, door)))
					continue;
				else
					return door;
			}
		}
		return (FD_DOORNAME_WRONG); //НЕПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ БЕЗ УКАЗАНИЯ НАПРАВЛЕНИЯ
	}

}

int has_key(CharData *ch, ObjVnum key) {
	for (auto o = ch->carrying; o; o = o->get_next_content()) {
		if (GET_OBJ_VNUM(o) == key && key != -1) {
			return (true);
		}
	}

	if (GET_EQ(ch, WEAR_HOLD)) {
		if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key && key != -1) {
			return (true);
		}
	}

	return (false);
}

#define NEED_OPEN    (1 << 0)
#define NEED_CLOSED    (1 << 1)
#define NEED_UNLOCKED    (1 << 2)
#define NEED_LOCKED    (1 << 3)

const char *cmd_door[] =
	{
		"открыл$g",
		"закрыл$g",
		"отпер$q",
		"запер$q",
		"взломал$g"
	};

const char *a_cmd_door[] =
	{
		"открыть",
		"закрыть",
		"отпереть",
		"запереть",
		"взломать"
	};

const int flags_door[] =
	{
		NEED_CLOSED | NEED_UNLOCKED,
		NEED_OPEN,
		NEED_CLOSED | NEED_LOCKED,
		NEED_CLOSED | NEED_UNLOCKED,
		NEED_CLOSED | NEED_LOCKED
	};

#define EXITN(room, door)        (world[room]->dir_option[door])

inline void OPEN_DOOR(const RoomRnum room, ObjData *obj, const int door) {
	if (obj) {
		auto v = obj->get_val(1);
		TOGGLE_BIT(v, CONT_CLOSED);
		obj->set_val(1, v);
	} else {
		TOGGLE_BIT(EXITN(room, door)->exit_info, EX_CLOSED);
	}
}

inline void LOCK_DOOR(const RoomRnum room, ObjData *obj, const int door) {
	if (obj) {
		auto v = obj->get_val(1);
		TOGGLE_BIT(v, CONT_LOCKED);
		obj->set_val(1, v);
	} else {
		TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED);
	}
}

// для кейсов
extern std::vector<TreasureCase> cases;;
void do_doorcmd(CharData *ch, ObjData *obj, int door, DOOR_SCMD scmd) {
	bool deaf = false;
	int other_room = 0;
	int r_num, vnum;
	int rev_dir[] = {kDirSouth, kDirWest, kDirNorth, kDirEast, kDirDown, kDirUp};
	char local_buf[kMaxStringLength]; // строка, в которую накапливается совершенное действо
	// пишем начало строки - кто чё сделал
	sprintf(local_buf, "$n %s ", cmd_door[scmd]);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DEAFNESS))
		deaf = true;
	// ищем парную дверь в другой клетке
	RoomData::exit_data_ptr back;
	if (!obj && EXIT(ch, door) && ((other_room = EXIT(ch, door)->to_room()) != kNowhere)) {
		back = world[other_room]->dir_option[rev_dir[door]];
		if (back) {
			if ((back->to_room() != ch->in_room)
				|| ((EXITDATA(ch->in_room, door)->exit_info
					^ EXITDATA(other_room, rev_dir[door])->exit_info)
					& (EX_ISDOOR | EX_CLOSED | EX_LOCKED))) {
				back.reset();
			}
		}
	}

	switch (scmd) {
		case SCMD_OPEN:
		case SCMD_CLOSE:
			if (scmd == SCMD_OPEN && obj && !open_otrigger(obj, ch, false))
				return;
			if (scmd == SCMD_OPEN && !obj && !open_wtrigger(world[ch->in_room], ch, door, false))
				return;
			if (scmd == SCMD_OPEN && !obj && back && !open_wtrigger(world[other_room], ch, rev_dir[door], false))
				return;
			if (scmd == SCMD_CLOSE && obj && !close_otrigger(obj, ch, false))
				return;
			if (scmd == SCMD_CLOSE && !obj && !close_wtrigger(world[ch->in_room], ch, door, false))
				return;
			if (scmd == SCMD_CLOSE && !obj && back && !close_wtrigger(world[other_room], ch, rev_dir[door], false))
				return;
			OPEN_DOOR(ch->in_room, obj, door);
			if (back) {
				OPEN_DOOR(other_room, obj, rev_dir[door]);
			}
			// вываливание и пурж кошелька
			if (obj && system_obj::is_purse(obj)) {
				sprintf(buf,
						"<%s> {%d} открыл трупный кошелек %s.",
						ch->get_name().c_str(),
						GET_ROOM_VNUM(ch->in_room),
						get_name_by_unique(GET_OBJ_VAL(obj, 3)));
				mudlog(buf, NRM, kLevelGreatGod, MONEY_LOG, true);
				system_obj::process_open_purse(ch, obj);
				return;
			} else {
				send_to_char(OK, ch);
			}
			break;

		case SCMD_UNLOCK:
		case SCMD_LOCK:
			if (scmd == SCMD_UNLOCK && obj && !open_otrigger(obj, ch, true))
				return;
			if (scmd == SCMD_LOCK && obj && !close_otrigger(obj, ch, true))
				return;
			if (scmd == SCMD_UNLOCK && !obj && !open_wtrigger(world[ch->in_room], ch, door, true))
				return;
			if (scmd == SCMD_LOCK && !obj && !close_wtrigger(world[ch->in_room], ch, door, true))
				return;
			if (scmd == SCMD_UNLOCK && !obj && back && !open_wtrigger(world[other_room], ch, rev_dir[door], true))
				return;
			if (scmd == SCMD_LOCK && !obj && back && !close_wtrigger(world[other_room], ch, rev_dir[door], true))
				return;
			LOCK_DOOR(ch->in_room, obj, door);
			if (back)
				LOCK_DOOR(other_room, obj, rev_dir[door]);
			if (!deaf)
				send_to_char("*Щелк*\r\n", ch);
			if (obj) {
				for (unsigned long i = 0; i < cases.size(); i++) {
					if (GET_OBJ_VNUM(obj) == cases[i].vnum) {
						if (!deaf)
							send_to_char("&GГде-то далеко наверху раздалась звонкая музыка.&n\r\n", ch);
						// drop_chance = cases[i].drop_chance;
						// drop_chance пока что не учитывается, просто падает одна рандомная стафина из всего этого
						const int maximal_chance = static_cast<int>(cases[i].vnum_objs.size() - 1);
						const int random_number = number(0, maximal_chance);
						vnum = cases[i].vnum_objs[random_number];
						if ((r_num = real_object(vnum)) < 0) {
							act("$o исчез$Y с яркой вспышкой. Случилось неладное, поняли вы..",
								false,
								ch,
								obj,
								nullptr,
								kToRoom);
							sprintf(local_buf,
									"[ERROR] do_doorcmd: ошибка при открытии контейнера %d, неизвестное содержимое!",
									obj->get_vnum());
							mudlog(local_buf, LogMode::CMP, kLevelGreatGod, MONEY_LOG, true);
							return;
						}
						// сначала удалим ключ из инвентаря
						int vnum_key = GET_OBJ_VAL(obj, 2);
						// первый предмет в инвентаре
						ObjData *obj_inv = ch->carrying;
						ObjData *i;
						for (i = obj_inv; i; i = i->get_next_content()) {
							if (GET_OBJ_VNUM(i) == vnum_key) {
								extract_obj(i);
								break;
							}
						}
						extract_obj(obj);
						obj = world_objects.create_from_prototype_by_rnum(r_num).get();
						obj->set_crafter_uid(GET_UNIQUE(ch));
						obj_to_char(obj, ch);
						act("$n завизжал$g от радости.", false, ch, nullptr, nullptr, kToRoom);
						load_otrigger(obj);
						obj_decay(obj);
						olc_log("%s load obj %s #%d", GET_NAME(ch), obj->get_short_description().c_str(), vnum);
						return;
					}
				}
			}
			break;

		case SCMD_PICK:
			if (obj && !pick_otrigger(obj, ch))
				return;
			if (!obj && !pick_wtrigger(world[ch->in_room], ch, door))
				return;
			if (!obj && back && !pick_wtrigger(world[other_room], ch, rev_dir[door]))
				return;
			LOCK_DOOR(ch->in_room, obj, door);
			if (back)
				LOCK_DOOR(other_room, obj, rev_dir[door]);
			send_to_char("Замок очень скоро поддался под вашим натиском.\r\n", ch);
			strcpy(local_buf, "$n умело взломал$g ");
			break;
	}

	// Notify the room
	sprintf(local_buf + strlen(local_buf), "%s.", (obj) ? "$o3" : (EXIT(ch, door)->vkeyword ? "$F" : "дверь"));
	if (!obj || (obj->get_in_room() != kNowhere)) {
		act(local_buf, false, ch, obj, obj ? nullptr : EXIT(ch, door)->vkeyword, kToRoom);
	}

	// Notify the other room
	if ((scmd == SCMD_OPEN || scmd == SCMD_CLOSE) && back) {
		const auto &people = world[EXIT(ch, door)->to_room()]->people;
		if (!people.empty()) {
			sprintf(local_buf + strlen(local_buf) - 1, " с той стороны.");
			int allowed_items_remained = 1000;
			for (const auto to : people) {
				if (0 == allowed_items_remained) {
					break;
				}
				perform_act(local_buf, ch, obj, obj ? nullptr : EXIT(ch, door)->vkeyword, to);
				--allowed_items_remained;
			}
		}
	}
}

bool ok_pick(CharData *ch, ObjVnum /*keynum*/, ObjData *obj, int door, int scmd) {
	const bool pickproof = DOOR_IS_PICKPROOF(ch, obj, door);

	if (scmd != SCMD_PICK) {
		return true;
	}

	if (pickproof) {
		send_to_char("Вы никогда не сможете взломать ЭТО.\r\n", ch);
		return false;
	}

	if (!check_moves(ch, PICKLOCK_MOVES)) {
		return false;
	}

	const PickProbabilityInformation &pbi = get_pick_probability(ch, DOOR_LOCK_COMPLEX(ch, obj, door));

	const bool pick_success = pbi.unlock_probability >= number(1, 100);

	if (pbi.skill_train_allowed) {
		TrainSkill(ch, ESkill::kPickLock, pick_success, nullptr);
	}

	if (!pick_success) {
		// неудачная попытка взлома - есть шанс сломать замок
		const int brake_probe = number(1, 100);
		if (pbi.brake_lock_probability >= brake_probe) {
			send_to_char("Вы все-таки сломали этот замок...\r\n", ch);
			if (obj) {
				auto v = obj->get_val(1);
				SET_BIT(v, CONT_BROKEN);
				obj->set_val(1, v);
			} else {
				SET_BIT(EXIT(ch, door)->exit_info, EX_BROKEN);
			}
		} else {
			if (pbi.unlock_probability == 0) {
				send_to_char("С таким сложным замком даже и пытаться не следует...\r\n", ch);
				return false;
			} else {
				send_to_char("Взломщик из вас пока еще никудышний.\r\n", ch);
			}
		}
	}

	return pick_success;
}

void do_gen_door(CharData *ch, char *argument, int, int subcmd) {
	int door = -1;
	ObjVnum keynum;
	char type[kMaxInputLength], dir[kMaxInputLength];
	ObjData *obj = nullptr;
	CharData *victim = nullptr;
	int where_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP;

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND)) {
		send_to_char("Очнитесь, вы же слепы!\r\n", ch);
		return;
	}

	if (subcmd == SCMD_PICK && !ch->get_skill(ESkill::kPickLock)) {
		send_to_char("Это умение вам недоступно.\r\n", ch);
		return;
	}
	skip_spaces(&argument);
	if (!*argument) {
		sprintf(buf, "%s что?\r\n", a_cmd_door[subcmd]);
		send_to_char(CAP(buf), ch);
		return;
	}
	two_arguments(argument, type, dir);

	if (isname(dir, "земля комната room ground"))
		where_bits = FIND_OBJ_ROOM;
	else if (isname(dir, "инвентарь inventory"))
		where_bits = FIND_OBJ_INV;
	else if (isname(dir, "экипировка equipment"))
		where_bits = FIND_OBJ_EQUIP;

	//Сначала ищем дверь, считая второй аргумент указанием на сторону света
	door = find_door(ch, type, dir, (DOOR_SCMD) subcmd);
	//Если двери не нашлось, проверяем объекты в экипировке, инвентаре, на земле
	if (door < 0)
		if (!generic_find(type, where_bits, ch, &victim, &obj)) {
			//Если и объектов не нашлось, выдаем одно из сообщений об ошибке
			switch (door) {
				case -1: //НЕВЕРНОЕ НАПРАВЛЕНИЕ
					send_to_char("Уточните направление.\r\n", ch);
					break;
				case -2: //НЕ ПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ В ЭТОМ НАПРАВЛЕНИИ
					sprintf(buf, "Вы не видите '%s' в этой комнате.\r\n", type);
					send_to_char(buf, ch);
					break;
				case -3: //В ЭТОМ НАПРАВЛЕНИИ НЕТ ДВЕРЕЙ
					sprintf(buf, "Вы не можете это '%s'.\r\n", a_cmd_door[subcmd]);
					send_to_char(buf, ch);
					break;
				case -4: //НЕ УКАЗАНО АРГУМЕНТОВ
					sprintf(buf, "Что вы хотите '%s'?\r\n", a_cmd_door[subcmd]);
					send_to_char(buf, ch);
					break;
				case -5: //НЕПРАВИЛЬНО НАЗВАЛИ ДВЕРЬ БЕЗ УКАЗАНИЯ НАПРАВЛЕНИЯ
					sprintf(buf, "Вы не видите здесь '%s'.\r\n", type);
					send_to_char(buf, ch);
					break;
			}
			return;
		}
	if ((obj) || (door >= 0)) {
		if ((obj) && !IS_IMMORTAL(ch) && (OBJ_FLAGGED(obj, EExtraFlag::ITEM_NAMED))
			&& NamedStuff::check_named(ch, obj, true))//Именной предмет открывать(закрывать) может только владелец
		{
			if (!NamedStuff::wear_msg(ch, obj))
				send_to_char("Просьба не трогать! Частная собственность!\r\n", ch);
			return;
		}
		keynum = DOOR_KEY(ch, obj, door);
		if ((subcmd == SCMD_CLOSE || subcmd == SCMD_LOCK) && !IS_NPC(ch) && NORENTABLE(ch))
			send_to_char("Ведите себя достойно во время боевых действий!\r\n", ch);
		else if (!(DOOR_IS_OPENABLE(ch, obj, door)))
			act("Вы никогда не сможете $F это!", false, ch, nullptr, a_cmd_door[subcmd], kToChar);
		else if (!DOOR_IS_OPEN(ch, obj, door)
			&& IS_SET(flags_door[subcmd], NEED_OPEN))
			send_to_char("Вообще-то здесь закрыто!\r\n", ch);
		else if (!DOOR_IS_CLOSED(ch, obj, door) && IS_SET(flags_door[subcmd], NEED_CLOSED))
			send_to_char("Уже открыто!\r\n", ch);
		else if (!(DOOR_IS_LOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_LOCKED))
			send_to_char("Да отперли уже все...\r\n", ch);
		else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED))
			send_to_char("Угу, заперто.\r\n", ch);
		else if (!has_key(ch, keynum) && !Privilege::check_flag(ch, Privilege::USE_SKILLS)
			&& ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
			send_to_char("У вас нет ключа.\r\n", ch);
		else if (DOOR_IS_BROKEN(ch, obj, door) && !Privilege::check_flag(ch, Privilege::USE_SKILLS)
			&& ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
			send_to_char("Замок сломан.\r\n", ch);
		else if (ok_pick(ch, keynum, obj, door, subcmd))
			do_doorcmd(ch, obj, door, (DOOR_SCMD) subcmd);
	}
}

void do_enter(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	int door, from_room;
	const char *p_str = "пентаграмма";
	struct Follower *k, *k_next;

	one_argument(argument, smallBuf);

	if (*smallBuf)
//     {if (!str_cmp("пентаграмма",smallBuf))
	{
		if (isname(smallBuf, p_str)) {
			if (!world[ch->in_room]->portal_time)
				send_to_char("Вы не видите здесь пентаграмму.\r\n", ch);
			else {
				from_room = ch->in_room;
				door = world[ch->in_room]->portal_room;
				// не пускать игрока на холженном коне
				if (ch->ahorse() && GET_MOB_HOLD(ch->get_horse())) {
					act("$Z $N не в состоянии нести вас на себе.\r\n",
						false, ch, nullptr, ch->get_horse(), kToChar);
					return;
				}
				// не пускать в ванрумы после пк, если его там прибьет сразу
				if (DeathTrap::check_tunnel_death(ch, door)) {
					send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
					return;
				}
				// Если чар под местью, и портал односторонний, то не пускать
				if (NORENTABLE(ch) && !IS_NPC(ch) && !world[door]->portal_time) {
					send_to_char("Грехи мешают вам воспользоваться вратами.\r\n", ch);
					return;
				}
				//проверка на флаг нельзя_верхом
				if (ROOM_FLAGGED(door, ROOM_NOHORSE) && ch->ahorse()) {
					act("$Z $N отказывается туда идти, и вам пришлось соскочить.",
						false, ch, nullptr, ch->get_horse(), kToChar);
					ch->dismount();
				}
				//проверка на ванрум и лошадь
				if (ROOM_FLAGGED(door, ROOM_TUNNEL) &&
					(num_pc_in_room(world[door]) > 0 || ch->ahorse())) {
					if (num_pc_in_room(world[door]) > 0) {
						send_to_char("Слишком мало места.\r\n", ch);
						return;
					} else {
						act("$Z $N заупрямил$U, и вам пришлось соскочить.",
							false, ch, nullptr, ch->get_horse(), kToChar);
						ch->dismount();
					}
				}
				// Обработка флагов NOTELEPORTIN и NOTELEPORTOUT здесь же
				if (!IS_IMMORTAL(ch)
					&& ((!IS_NPC(ch)
						&& (!Clan::MayEnter(ch, door, HCE_PORTAL) || (GET_REAL_LEVEL(ch) <= 10 && world[door]->portal_time && GET_REAL_REMORT(ch) < 9)))
						|| (ROOM_FLAGGED(from_room, ROOM_NOTELEPORTOUT) || ROOM_FLAGGED(door, ROOM_NOTELEPORTIN))
						|| AFF_FLAGGED(ch, EAffectFlag::AFF_NOTELEPORT)
						|| (world[door]->pkPenterUnique
							&& (ROOM_FLAGGED(door, ROOM_ARENA) || ROOM_FLAGGED(door, ROOM_HOUSE))))) {
					sprintf(smallBuf, "%sПентаграмма ослепительно вспыхнула!%s\r\n",
							CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
					act(smallBuf, true, ch, nullptr, nullptr, kToChar);
					act(smallBuf, true, ch, nullptr, nullptr, kToRoom);

					send_to_char("Мощным ударом вас отшвырнуло от пентаграммы.\r\n", ch);
					act("$n с визгом отлетел$g от пентаграммы.\r\n", true, ch,
						nullptr, nullptr, kToRoom | kToNotDeaf);
					act("$n отлетел$g от пентаграммы.\r\n", true, ch, nullptr, nullptr, kToRoom | kToDeaf);
					WAIT_STATE(ch, kPulseViolence);
					return;
				}
				if (!enter_wtrigger(world[door], ch, -1))
					return;
				act("$n исчез$q в пентаграмме.", true, ch, nullptr, nullptr, kToRoom);
				if (world[from_room]->pkPenterUnique && world[from_room]->pkPenterUnique != GET_UNIQUE(ch)
					&& !IS_IMMORTAL(ch)) {
					send_to_char(ch, "%sВаш поступок был расценен как потенциально агрессивный.%s\r\n",
								 CCIRED(ch, C_NRM), CCINRM(ch, C_NRM));
					pkPortal(ch);
				}
				char_from_room(ch);
				char_to_room(ch, door);
				greet_mtrigger(ch, -1);
				greet_otrigger(ch, -1);
				SetWait(ch, 3, false);
				act("$n появил$u из пентаграммы.", true, ch, nullptr, nullptr, kToRoom);
				// ищем ангела и лошадь
				for (k = ch->followers; k; k = k_next) {
					k_next = k->next;
					if (IS_HORSE(k->ch) &&
						!k->ch->get_fighting() &&
						!GET_MOB_HOLD(k->ch) &&
						IN_ROOM(k->ch) == from_room && AWAKE(k->ch)) {
						if (!ROOM_FLAGGED(door, ROOM_NOHORSE)) {
							char_from_room(k->ch);
							char_to_room(k->ch, door);
						}
					}
					if (AFF_FLAGGED(k->ch, EAffectFlag::AFF_HELPER)
						&& !GET_MOB_HOLD(k->ch)
						&& (MOB_FLAGGED(k->ch, MOB_ANGEL) || MOB_FLAGGED(k->ch, MOB_GHOST))
						&& !k->ch->get_fighting()
						&& IN_ROOM(k->ch) == from_room
						&& AWAKE(k->ch)) {
						act("$n исчез$q в пентаграмме.", true,
							k->ch, nullptr, nullptr, kToRoom);
						char_from_room(k->ch);
						char_to_room(k->ch, door);
						SetWait(k->ch, 3, false);
						act("$n появил$u из пентаграммы.", true,
							k->ch, nullptr, nullptr, kToRoom);
					}
					if (IS_CHARMICE(k->ch) &&
						!GET_MOB_HOLD(k->ch) &&
						GET_POS(k->ch) == EPosition::kStand &&
						IN_ROOM(k->ch) == from_room) {
						snprintf(buf2, kMaxStringLength, "войти пентаграмма");
						command_interpreter(k->ch, buf2);
					}
				}
				if (ch->desc != nullptr)
					look_at_room(ch, 0);
			}
		} else {    // an argument was supplied, search for door keyword
			for (door = 0; door < kDirMaxNumber; door++) {
				if (EXIT(ch, door)
					&& (isname(smallBuf, EXIT(ch, door)->keyword)
						|| isname(smallBuf, EXIT(ch, door)->vkeyword))) {
					perform_move(ch, door, 1, true, nullptr);
					return;
				}
			}
			sprintf(buf2, "Вы не нашли здесь '%s'.\r\n", smallBuf);
			send_to_char(buf2, ch);
		}
	} else if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
		send_to_char("Вы уже внутри.\r\n", ch);
	else            // try to locate an entrance
	{
		for (door = 0; door < kDirMaxNumber; door++)
			if (EXIT(ch, door))
				if (EXIT(ch, door)->to_room() != kNowhere)
					if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
						ROOM_FLAGGED(EXIT(ch, door)->to_room(), ROOM_INDOORS)) {
						perform_move(ch, door, 1, true, nullptr);
						return;
					}
		send_to_char("Вы не можете найти вход.\r\n", ch);
	}
}

void do_stand(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (GET_POS(ch) > EPosition::kSleep && AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP)) {
		send_to_char("Вы сладко зевнули и решили еще немного подремать.\r\n", ch);
		act("$n сладко зевнул$a и решил$a еще немного подремать.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		GET_POS(ch) = EPosition::kSleep;
	}

	if (ch->ahorse()) {
		act("Прежде всего, вам стоит слезть с $N1.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	switch (GET_POS(ch)) {
		case EPosition::kStand: send_to_char("А вы уже стоите.\r\n", ch);
			break;
		case EPosition::kSit: send_to_char("Вы встали.\r\n", ch);
			act("$n поднял$u.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			// Will be sitting after a successful bash and may still be fighting.
			GET_POS(ch) = ch->get_fighting() ? EPosition::kFight : EPosition::kStand;
			break;
		case EPosition::kRest: send_to_char("Вы прекратили отдыхать и встали.\r\n", ch);
			act("$n прекратил$g отдых и поднял$u.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			GET_POS(ch) = ch->get_fighting() ? EPosition::kFight : EPosition::kStand;
			break;
		case EPosition::kSleep: send_to_char("Пожалуй, сначала стоит проснуться!\r\n", ch);
			break;
		case EPosition::kFight: send_to_char("Вы дрались лежа? Это что-то новенькое.\r\n", ch);
			break;
		default: send_to_char("Вы прекратили летать и опустились на грешную землю.\r\n", ch);
			act("$n опустил$u на землю.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			GET_POS(ch) = EPosition::kStand;
			break;
	}
}

void do_sit(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->ahorse()) {
		act("Прежде всего, вам стоит слезть с $N1.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	switch (GET_POS(ch)) {
		case EPosition::kStand: send_to_char("Вы сели.\r\n", ch);
			act("$n сел$g.", false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			GET_POS(ch) = EPosition::kSit;
			break;
		case EPosition::kSit: send_to_char("А вы и так сидите.\r\n", ch);
			break;
		case EPosition::kRest: send_to_char("Вы прекратили отдыхать и сели.\r\n", ch);
			act("$n прервал$g отдых и сел$g.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			GET_POS(ch) = EPosition::kSit;
			break;
		case EPosition::kSleep: send_to_char("Вам стоит проснуться.\r\n", ch);
			break;
		case EPosition::kFight: send_to_char("Сесть? Во время боя? Вы явно не в себе.\r\n", ch);
			break;
		default: send_to_char("Вы прекратили свой полет и сели.\r\n", ch);
			act("$n прекратил$g свой полет и сел$g.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			GET_POS(ch) = EPosition::kSit;
			break;
	}
}

void do_rest(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (ch->ahorse()) {
		act("Прежде всего, вам стоит слезть с $N1.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	switch (GET_POS(ch)) {
		case EPosition::kStand: send_to_char("Вы присели отдохнуть.\r\n", ch);
			act("$n присел$g отдохнуть.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			GET_POS(ch) = EPosition::kRest;
			break;
		case EPosition::kSit: send_to_char("Вы пристроились поудобнее для отдыха.\r\n", ch);
			act("$n пристроил$u поудобнее для отдыха.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			GET_POS(ch) = EPosition::kRest;
			break;
		case EPosition::kRest: send_to_char("Вы и так отдыхаете.\r\n", ch);
			break;
		case EPosition::kSleep: send_to_char("Вам лучше сначала проснуться.\r\n", ch);
			break;
		case EPosition::kFight: send_to_char("Отдыха в бою вам не будет!\r\n", ch);
			break;
		default: send_to_char("Вы прекратили полет и присели отдохнуть.\r\n", ch);
			act("$n прекратил$g полет и пристроил$u поудобнее для отдыха.", false, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			GET_POS(ch) = EPosition::kSit;
			break;
	}
}

void do_sleep(CharData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	if (GET_REAL_LEVEL(ch) >= kLevelImmortal) {
		send_to_char("Не время вам спать, родина в опасности!\r\n", ch);
		return;
	}
	if (ch->ahorse()) {
		act("Прежде всего, вам стоит слезть с $N1.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	switch (GET_POS(ch)) {
		case EPosition::kStand:
		case EPosition::kSit:
		case EPosition::kRest: send_to_char("Вы заснули.\r\n", ch);
			act("$n сладко зевнул$g и задал$g храпака.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			GET_POS(ch) = EPosition::kSleep;
			break;
		case EPosition::kSleep: send_to_char("А вы и так спите.\r\n", ch);
			break;
		case EPosition::kFight: send_to_char("Вам нужно сражаться! Отоспитесь после смерти.\r\n", ch);
			break;
		default: send_to_char("Вы прекратили свой полет и отошли ко сну.\r\n", ch);
			act("$n прекратил$g летать и нагло заснул$g.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
			GET_POS(ch) = EPosition::kSleep;
			break;
	}
}

void do_wake(CharData *ch, char *argument, int/* cmd*/, int subcmd) {
	CharData *vict;
	int self = 0;

	one_argument(argument, arg);

	if (subcmd == SCMD_WAKEUP) {
		if (!(*arg)) {
			send_to_char("Кого будить то будем???\r\n", ch);
			return;
		}
	} else {
		*arg = 0;
	}

	if (*arg) {
		if (GET_POS(ch) == EPosition::kSleep)
			send_to_char("Может быть вам лучше проснуться?\r\n", ch);
		else if ((vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)) == nullptr)
			send_to_char(NOPERSON, ch);
		else if (vict == ch)
			self = 1;
		else if (AWAKE(vict))
			act("$E и не спал$G.", false, ch, nullptr, vict, kToChar);
		else if (GET_POS(vict) < EPosition::kSleep)
			act("$M так плохо! Оставьте $S в покое!", false, ch, nullptr, vict, kToChar);
		else {
			act("Вы $S разбудили.", false, ch, nullptr, vict, kToChar);
			act("$n растолкал$g вас.", false, ch, nullptr, vict, kToVict | kToSleep);
			GET_POS(vict) = EPosition::kSit;
		}
		if (!self)
			return;
	}
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP))
		send_to_char("Вы не можете проснуться!\r\n", ch);
	else if (GET_POS(ch) > EPosition::kSleep)
		send_to_char("А вы и не спали...\r\n", ch);
	else {
		send_to_char("Вы проснулись и сели.\r\n", ch);
		act("$n проснул$u.", true, ch, nullptr, nullptr, kToRoom | kToArenaListen);
		GET_POS(ch) = EPosition::kSit;
	}
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
