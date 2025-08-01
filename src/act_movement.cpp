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

#include "gameplay/mechanics/deathtrap.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/pk.h"
#include "gameplay/ai/mobact.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/named_stuff.h"
#include "engine/db/obj_prototypes.h"
#include "administration/privilege.h"
#include "gameplay/skills/pick.h"
#include "gameplay/skills/track.h"
#include "utils/random.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/awake.h"

// external functs
void SetWait(CharData *ch, int waittime, int victim_in_room);
int find_eq_pos(CharData *ch, ObjData *obj, char *local_arg);
// local functions
void check_ice(int room);

const int Reverse[EDirection::kMaxDirNum] = {2, 3, 0, 1, 5, 4};

// check ice in room
int check_death_ice(int room, CharData * /*ch*/) {
	int sector, mass = 0, result = false;

	if (room == kNowhere)
		return false;
	sector = SECT(room);
	if (sector != ESector::kWaterSwim && sector != ESector::kWaterNoswim)
		return false;
	if ((sector = real_sector(room)) != ESector::kThinIce && sector != ESector::kNormalIce)
		return false;

	for (const auto vict : world[room]->people) {
		if (!vict->IsNpc()
			&& !AFF_FLAGGED(vict, EAffect::kFly)) {
			mass += GET_WEIGHT(vict) + vict->GetCarryingWeight();
		}
	}

	if (!mass) {
		return false;
	}

	if ((sector == ESector::kThinIce && mass > 500) || (sector == ESector::kNormalIce && mass > 1500)) {
		const auto first_in_room = world[room]->first_character();

		act("Лед проломился под вашей тяжестью.", false, first_in_room, nullptr, nullptr, kToRoom);
		act("Лед проломился под вашей тяжестью.", false, first_in_room, nullptr, nullptr, kToChar);

		world[room]->weather.icelevel = 0;
		world[room]->ices = 2;
		world[room]->set_flag(ERoomFlag::kIceTrap);
		deathtrap::add(world[room]);
	} else {
		return false;
	}

	return (result);
}

/**
 * Return true if char can walk on water
 */
bool HasBoat(CharData *ch) {
	if (IS_IMMORTAL(ch)) {
		return true;
	}

	if (AFF_FLAGGED(ch, EAffect::kWaterWalk) || AFF_FLAGGED(ch, EAffect::kFly)) {
		return true;
	}

	for (auto obj = ch->carrying; obj; obj = obj->get_next_content()) {
		if (GET_OBJ_TYPE(obj) == EObjType::kBoat && (find_eq_pos(ch, obj, nullptr) < 0)) {
			return true;
		}
	}

	for (auto i = 0; i < EEquipPos::kNumEquipPos; i++) {
		if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == EObjType::kBoat) {
			return true;
		}
	}

	return false;
}

void make_visible(CharData *ch, const EAffect affect) {
	char to_room[kMaxStringLength], to_char[kMaxStringLength];

	*to_room = *to_char = 0;

	switch (affect) {
		case EAffect::kHide: strcpy(to_char, "Вы прекратили прятаться.\r\n");
			strcpy(to_room, "$n прекратил$g прятаться.");
			break;

		case EAffect::kDisguise: strcpy(to_char, "Вы прекратили маскироваться.\r\n");
			strcpy(to_room, "$n прекратил$g маскироваться.");
			break;

		default: break;
	}
	AFF_FLAGS(ch).unset(affect);
	ch->check_aggressive = true;
	if (*to_char)
		SendMsgToChar(to_char, ch);
	if (*to_room)
		act(to_room, false, ch, nullptr, nullptr, kToRoom);
}

int skip_hiding(CharData *ch, CharData *vict) {
	int percent, prob;

	if (MAY_SEE(ch, vict, ch) && IsAffectedBySpell(ch, ESpell::kHide)) {
		if (awake_hide(ch)) {
			SendMsgToChar("Вы попытались спрятаться, но ваша экипировка выдала вас.\r\n", ch);
			RemoveAffectFromChar(ch, ESpell::kHide);
			make_visible(ch, EAffect::kHide);
			EXTRA_FLAGS(ch).set(EXTRA_FAILHIDE);
		} else if (IsAffectedBySpell(ch, ESpell::kHide)) {
			if (AFF_FLAGGED(vict, EAffect::kDetectLife)) {
				act("$N почувствовал$G ваше присутствие.", false, ch, nullptr, vict, kToChar);
				return false;
			}
			percent = number(1, 82 + GetRealInt(vict));
			prob = CalcCurrentSkill(ch, ESkill::kHide, vict);
			if (percent > prob) {
				RemoveAffectFromChar(ch, ESpell::kHide);
				AFF_FLAGS(ch).unset(EAffect::kHide);
				act("Вы не сумели остаться незаметным.", false, ch, nullptr, vict, kToChar);
			} else {
				ImproveSkill(ch, ESkill::kHide, true, vict);
				act("Вам удалось остаться незаметным.", false, ch, nullptr, vict, kToChar);
				return true;
			}
		}
	}
	return false;
}

int skip_camouflage(CharData *ch, CharData *vict) {
	int percent, prob;

	if (MAY_SEE(ch, vict, ch) && IsAffectedBySpell(ch, ESpell::kCamouflage)) {
		if (awake_camouflage(ch)) {
			SendMsgToChar("Вы попытались замаскироваться, но ваша экипировка выдала вас.\r\n", ch);
			RemoveAffectFromChar(ch, ESpell::kCamouflage);
			make_visible(ch, EAffect::kDisguise);
			EXTRA_FLAGS(ch).set(EXTRA_FAILCAMOUFLAGE);
		} else if (IsAffectedBySpell(ch, ESpell::kCamouflage)) {
			if (AFF_FLAGGED(vict, EAffect::kDetectLife)) {
				act("$N почувствовал$G ваше присутствие.", false, ch, nullptr, vict, kToChar);
				return false;
			}
			percent = number(1, 82 + GetRealInt(vict));
			prob = CalcCurrentSkill(ch, ESkill::kDisguise, vict);
			if (percent > prob) {
				RemoveAffectFromChar(ch, ESpell::kCamouflage);
				AFF_FLAGS(ch).unset(EAffect::kDisguise);
				ImproveSkill(ch, ESkill::kDisguise, false, vict);
				act("Вы не сумели правильно замаскироваться.", false, ch, nullptr, vict, kToChar);
			} else {
				ImproveSkill(ch, ESkill::kDisguise, true, vict);
				act("Ваша маскировка оказалась на высоте.", false, ch, nullptr, vict, kToChar);
				return true;
			}
		}
	}
	return false;
}

int skip_sneaking(CharData *ch, CharData *vict) {
	int percent, prob, absolute_fail;
	bool try_fail;

	if (MAY_SEE(ch, vict, ch) && IsAffectedBySpell(ch, ESpell::kSneak)) {
		if (awake_sneak(ch))    //if (affected_by_spell(ch,SPELL_SNEAK))
		{
			SendMsgToChar("Вы попытались подкрасться, но ваша экипировка выдала вас.\r\n", ch);
			RemoveAffectFromChar(ch, ESpell::kSneak);
			make_visible(ch, EAffect::kSneak);
			RemoveAffectFromChar(ch, ESpell::kHide);
			AFF_FLAGS(ch).unset(EAffect::kHide);
			AFF_FLAGS(ch).unset(EAffect::kSneak);
		} else if (IsAffectedBySpell(ch, ESpell::kSneak)) {
			percent = number(1, 112 + (GetRealInt(vict) * (vict->get_role(MOB_ROLE_BOSS) ? 3 : 1)) +
				(GetRealLevel(vict) > 30 ? GetRealLevel(vict) : 0));
			prob = CalcCurrentSkill(ch, ESkill::kSneak, vict);

			int catch_level = (GetRealLevel(vict) - GetRealLevel(ch));
			if (catch_level > 5) {
				//5% шанс фэйла при prob==200 всегда, при prob = 100 - 10%, если босс, шанс множим на 5
				absolute_fail = ((200 - prob) / 20 + 5) * (vict->get_role(MOB_ROLE_BOSS) ? 5 : 1);
				try_fail = number(1, 100) < absolute_fail;
			} else
				try_fail = false;

			if ((percent > prob) || try_fail) {
				RemoveAffectFromChar(ch, ESpell::kSneak);
				RemoveAffectFromChar(ch, ESpell::kHide);
				AFF_FLAGS(ch).unset(EAffect::kHide);
				AFF_FLAGS(ch).unset(EAffect::kSneak);
				ImproveSkill(ch, ESkill::kSneak, false, vict);
				act("Вы не сумели пробраться незаметно.", false, ch, nullptr, vict, kToChar);
			} else {
				ImproveSkill(ch, ESkill::kSneak, true, vict);
				act("Вам удалось прокрасться незаметно.", false, ch, nullptr, vict, kToChar);
				return true;
			}
		}
	}
	return false;
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
		case ESector::kForest:		return ESector::kField;
		case ESector::kForestSnow:	return ESector::kFieldSnow;
		case ESector::kForestRain:	return ESector::kFieldRain;
		default:					return sect;
	}
}

int real_mountains_paths_sect(int sect) {
	switch (sect) {
		case ESector::kHills:			[[fallthrough]];
		case ESector::kMountain:		return ESector::kField;
		case ESector::kHillsRain:		return ESector::kFieldRain;
		case ESector::kHillsSnow:		[[fallthrough]];
		case ESector::kMountainSnow:	return ESector::kFieldSnow;
		default: return sect;
	}
}

int calculate_move_cost(CharData *ch, int dir) {
	// move points needed is avg. move loss for src and destination sect type
	auto ch_inroom = real_sector(ch->in_room);
	auto ch_toroom = real_sector(EXIT(ch, dir)->to_room());

	if (CanUseFeat(ch, EFeat::kForestPath)) {
		ch_inroom = real_forest_paths_sect(ch_inroom);
		ch_toroom = real_forest_paths_sect(ch_toroom);
	}

	if (CanUseFeat(ch, EFeat::kMountainPath)) {
		ch_inroom = real_mountains_paths_sect(ch_inroom);
		ch_toroom = real_mountains_paths_sect(ch_toroom);
	}

	int need_movement = (IS_FLY(ch) || ch->IsOnHorse()) ? 1 :
						(movement_loss[ch_inroom] + movement_loss[ch_toroom]) / 2;

	if (IS_IMMORTAL(ch))
		need_movement = 0;
	else if (IsAffectedBySpell(ch, ESpell::kCamouflage))
		need_movement += kCamouflageMoves;
	else if (IsAffectedBySpell(ch, ESpell::kSneak))
		need_movement += kSneakMoves;

	return need_movement;
}

bool IsCorrectDirection(CharData *ch, int dir, bool check_specials, bool show_msg) {
	buf2[0] = '\0';
	if (dir == EDirection::kUndefinedDir) {
		return false;
	}
	if (check_specials && special(ch, dir + 1, buf2, 1)) {
		return false;
	}

	// если нпц идет по маршруту - пропускаем проверку дверей
	const bool npc_roamer = ch->IsNpc() && (GET_DEST(ch) != kNowhere) && (EXIT(ch, dir) && EXIT(ch, dir)->to_room() != kNowhere);
	if (!npc_roamer) {
		if (!CAN_GO(ch, dir)) {
			return false;
		}
	}

	if (deathtrap::check_tunnel_death(ch, EXIT(ch, dir)->to_room())) {
		if (show_msg) {
			SendMsgToChar("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
		}
		return false;
	}

	// charmed
	if (IS_CHARMICE(ch) && ch->has_master() && ch->in_room == ch->get_master()->in_room) {
		if (show_msg) {
			SendMsgToChar("Вы не можете покинуть свой идеал.\r\n", ch);
			act("$N попытал$U покинуть вас.", false, ch->get_master(), nullptr, ch, kToChar);
		}
		return false;
	}

	// check NPC's
	if (ch->IsNpc()) {
		if (GET_DEST(ch) != kNowhere) {
			return true;
		}

		//  if this room or the one we're going to needs a boat, check for one */
		if (!ch->IsFlagged(EMobFlag::kSwimming)
			&& !ch->IsFlagged(EMobFlag::kFlying)
			&& !AFF_FLAGGED(ch, EAffect::kFly)
			&& (real_sector(ch->in_room) == ESector::kWaterNoswim
				|| real_sector(EXIT(ch, dir)->to_room()) == ESector::kWaterNoswim)) {
			if (!HasBoat(ch)) {
				return false;
			}
		}

		// Добавляем проверку на то что моб может вскрыть дверь
		if (EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kClosed) &&
			!ch->IsFlagged(EMobFlag::kOpensDoor))
			return false;

		if (!ch->IsFlagged(EMobFlag::kFlying) &&
			!AFF_FLAGGED(ch, EAffect::kFly) && SECT(EXIT(ch, dir)->to_room()) == ESector::kOnlyFlying)
			return false;

		if (ch->IsFlagged(EMobFlag::kOnlySwimming) &&
			!(real_sector(EXIT(ch, dir)->to_room()) == ESector::kWaterSwim ||
				real_sector(EXIT(ch, dir)->to_room()) == ESector::kWaterNoswim ||
				real_sector(EXIT(ch, dir)->to_room()) == ESector::kUnderwater))
			return false;

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ERoomFlag::kNoEntryMob) &&
			!IS_HORSE(ch) &&
			!AFF_FLAGGED(ch, EAffect::kCharmed) && !(ch->IsFlagged(EMobFlag::kTutelar) || ch->IsFlagged(EMobFlag::kMentalShadow))
			&& !ch->IsFlagged(EMobFlag::kIgnoresNoMob))
			return false;

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ERoomFlag::kDeathTrap) && !IS_HORSE(ch))
			return false;

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ERoomFlag::kGodsRoom))
			return false;

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ERoomFlag::kNohorse) && IS_HORSE(ch))
			return false;
	} else {
		//Вход в замок
		if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kHouseEntry)) {
			if (!Clan::MayEnter(ch, EXIT(ch, dir)->to_room(), kHouseAtrium)) {
				if (show_msg)
					SendMsgToChar("Частная собственность! Вход воспрещен!\r\n", ch);
				return (false);
			}
		}

		if (real_sector(ch->in_room) == ESector::kWaterNoswim ||
			real_sector(EXIT(ch, dir)->to_room()) == ESector::kWaterNoswim) {
			if (!HasBoat(ch)) {
				if (show_msg)
					SendMsgToChar("Вам нужна лодка, чтобы попасть туда.\r\n", ch);
				return (false);
			}
		}
		if (real_sector(EXIT(ch, dir)->to_room()) == ESector::kOnlyFlying
			&& !IS_GOD(ch)
			&& !AFF_FLAGGED(ch, EAffect::kFly)) {
			if (show_msg) {
				SendMsgToChar("Туда можно только влететь.\r\n", ch);
			}
			return false;
		}

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ERoomFlag::kDeathTrap) && ch->IsOnHorse()) {
			if (show_msg) {
				SendMsgToChar("Ваш скакун артачится и боится идти туда.\r\n", ch);
			}
			return false;
		}

		const auto need_movement = calculate_move_cost(ch, dir);
		if (ch->get_move() < need_movement) {
			if (check_specials
				&& ch->has_master()) {
				if (show_msg) {
					SendMsgToChar("Вы слишком устали, чтобы следовать туда.\r\n", ch);
				}
			} else {
				if (show_msg) {
					SendMsgToChar("Вы слишком устали.\r\n", ch);
				}
			}
			return false;
		}
		//Вход в замок
		if (ROOM_FLAGGED(ch->in_room, ERoomFlag::kHouseEntry)) {
			if (!Clan::MayEnter(ch, EXIT(ch, dir)->to_room(), kHouseAtrium)) {
				if (show_msg)
					SendMsgToChar("Частная собственность! Вход воспрещен!\r\n", ch);
				return (false);
			}
		}

		//чтобы конь не лез в комнату с флагом !лошадь
		if (ch->IsOnHorse() && !IsCorrectDirection(ch->get_horse(), dir, check_specials, false)) {
			if (show_msg) {
				act("$Z $N отказывается туда идти, и вам пришлось соскочить.",
					false, ch, nullptr, ch->get_horse(), kToChar);
				ch->dismount();
			}
		}
		//проверка на ванрум: скидываем игрока с коня, если там незанято
		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ERoomFlag::kTunnel) &&
			(num_pc_in_room((world[EXIT(ch, dir)->to_room()])) > 0)) {
			if (show_msg)
				SendMsgToChar("Слишком мало места.\r\n", ch);
			return (false);
		}

		if (ch->IsOnHorse() && GET_HORSESTATE(ch->get_horse()) <= 0) {
			if (show_msg)
				act("$Z $N загнан$G настолько, что не может нести вас на себе.",
					false, ch, nullptr, ch->get_horse(), kToChar);
			return false;
		}

		if (ch->IsOnHorse()
			&& (AFF_FLAGGED(ch->get_horse(), EAffect::kHold)
				|| AFF_FLAGGED(ch->get_horse(), EAffect::kSleep))) {
			if (show_msg)
				act("$Z $N не в состоянии нести вас на себе.\r\n", false, ch, nullptr, ch->get_horse(), kToChar);
			return false;
		}

		if (ch->IsOnHorse()
			&& (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ERoomFlag::kTunnel)
				|| ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ERoomFlag::kNohorse))) {
			if (show_msg)
				act("$Z $N не в состоянии пройти туда.\r\n", false, ch, nullptr, ch->get_horse(), kToChar);
			return false;
		}

		if (ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ERoomFlag::kGodsRoom) && !IS_GRGOD(ch)) {
			if (show_msg)
				SendMsgToChar("Вы не столь Божественны, как вам кажется!\r\n", ch);
			return false;
		}

		for (const auto tch : world[ch->in_room]->people) {
			if (!tch->IsNpc())
				continue;
			if (NPC_FLAGGED(tch, 1 << dir)
				&& AWAKE(tch)
				&& tch->GetPosition() > EPosition::kSleep
				&& CAN_SEE(tch, ch)
				&& !AFF_FLAGGED(tch, EAffect::kCharmed)
				&& !AFF_FLAGGED(tch, EAffect::kHold)) {
				if (show_msg) {
					act("$N преградил$G вам путь.", false, ch, nullptr, tch, kToChar);
				}
				if (IS_GRGOD(ch) || InTestZone(ch)) {
					act("Но уважительно пропустил$G дальше.", false, ch, nullptr, tch, kToChar);
					return true;
				}
				return false;
			}
		}
	}

	return true;
}

const int kMaxDrunkSong{6};
const int kMaxDrunkVoice{5};
void PerformDunkSong(CharData *ch) {
	const char *drunk_songs[kMaxDrunkSong] = {"\"Шумел камыш, и-к-к..., деревья гнулися\"",
											  "\"Куда ты, тропинка, меня завела\"",
											  "\"Пабабам, пара пабабам\"",
											  "\"А мне любое море по колено\"",
											  "\"Не жди меня мама, хорошего сына\"",
											  "\"Бываааали дниии, весеееелыя\"",
	};
	const char *drunk_voice[kMaxDrunkVoice] = {" - затянул$g $n",
											   " - запел$g $n.",
											   " - прохрипел$g $n.",
											   " - зычно заревел$g $n.",
											   " - разухабисто протянул$g $n.",
	};
	// орем песни
	if (!ch->GetEnemy() && number(10, 24) < GET_COND(ch, DRUNK)) {
		sprintf(buf, "%s", drunk_songs[number(0, kMaxDrunkSong - 1)]);
		SendMsgToChar(buf, ch);
		SendMsgToChar("\r\n", ch);
		strcat(buf, drunk_voice[number(0, kMaxDrunkVoice - 1)]);
		act(buf, false, ch, nullptr, nullptr, kToRoom | kToNotDeaf);
		RemoveAffectFromChar(ch, ESpell::kHide);
		RemoveAffectFromChar(ch, ESpell::kSneak);
		RemoveAffectFromChar(ch, ESpell::kCamouflage);
		AFF_FLAGS(ch).unset(EAffect::kHide);
		AFF_FLAGS(ch).unset(EAffect::kSneak);
		AFF_FLAGS(ch).unset(EAffect::kDisguise);
	}
}

/**
 * Выбор случайной комнаты из списка выходов комнаты, в которой находится персонаж.
 * @param ch - персонаж, для которого производится выбор.
 * @param fail_chance - шанс не найти выход. Сравнивается со случайным числом от 1 до 100 + 25*число выходов.
 * @return Выбранное направление. Может быть kUndefinedDir в случае неудачи.
 */
EDirection SelectRndDirection(CharData *ch, int fail_chance) {
	std::vector<EDirection> directions;
	for (auto dir = EDirection::kFirstDir; dir <= EDirection::kLastDir; ++dir) {
		if (IsCorrectDirection(ch, dir, true, false) &&
			!ROOM_FLAGGED(EXIT(ch, dir)->to_room(), ERoomFlag::kDeathTrap)) {
			directions.push_back(dir);
		}
	}

	if (directions.empty() || number(1, 100 + 25 * directions.size()) < fail_chance) {
		return EDirection::kUndefinedDir;
	}
	return directions[number(0, directions.size() - 1)];
}

int SelectDrunkDirection(CharData *ch, int direction) {
	auto drunk_dir{direction};
	if (!ch->IsNpc() && GET_COND(ch, DRUNK) >= kMortallyDrunked &&
		!ch->IsOnHorse() && GET_COND(ch, DRUNK) >= number(kDrunked, 50)) {
		drunk_dir = SelectRndDirection(ch, 60);
	}

	if (direction != drunk_dir) {
		SendMsgToChar("Ваши ноги не хотят слушаться вас...\r\n", ch);
	}
	return drunk_dir;
}

int DoSimpleMove(CharData *ch, int dir, int following, CharData *leader, bool is_flee) {
	struct TrackData *track;
	RoomRnum was_in, go_to;
	int i, invis = 0, use_horse = 0, is_horse = 0, direction = 0;
	int mob_rnum = -1;
	CharData *horse = nullptr;

	if (ch->purged()) {
		return false;
	}
	// если не моб и не ведут за ручку - проверка на пьяные выкрутасы
	if (!ch->IsNpc() && !leader) {
		dir = SelectDrunkDirection(ch, dir);
	}
	PerformDunkSong(ch);

	if (!IsCorrectDirection(ch, dir, following, true)) {
		return false;
	}
	if (!entry_mtrigger(ch)) {
		return false;
	}
	if (!enter_wtrigger(world[EXIT(ch, dir)->to_room()], ch, dir)) {
		return false;
	}

	// Now we know we're allowed to go into the room.
	if (!IS_IMMORTAL(ch) && !ch->IsNpc())
		ch->set_move(ch->get_move() - calculate_move_cost(ch, dir));

	i = MUD::Skill(ESkill::kSneak).difficulty;
	if (AFF_FLAGGED(ch, EAffect::kSneak) && !is_flee) {
		if (ch->IsNpc())
			invis = 1;
		else if (awake_sneak(ch)) {
			RemoveAffectFromChar(ch, ESpell::kSneak);
			AFF_FLAGS(ch).unset(EAffect::kSneak);
		} else if (!IsAffectedBySpell(ch, ESpell::kSneak) || CalcCurrentSkill(ch, ESkill::kSneak, nullptr) >= number(1, i))
			invis = 1;
	}

	i = MUD::Skill(ESkill::kDisguise).difficulty;
	if (AFF_FLAGGED(ch, EAffect::kDisguise) && !is_flee) {
		if (ch->IsNpc())
			invis = 1;
		else if (awake_camouflage(ch)) {
			RemoveAffectFromChar(ch, ESpell::kCamouflage);
			AFF_FLAGS(ch).unset(EAffect::kDisguise);
		} else if (!IsAffectedBySpell(ch, ESpell::kCamouflage) || CalcCurrentSkill(ch, ESkill::kDisguise, nullptr) >= number(1, i))
			invis = 1;
	}

	if (!is_flee) {
		sprintf(buf, "Вы поплелись %s%s.", leader ? "следом за $N4 " : "", DirsTo[dir]);
		act(buf, false, ch, nullptr, leader, kToChar);
	}
	if (ch->IsNpc() && ch->IsFlagged(EMobFlag::kSentinel) && !IS_CHARMICE(ch) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena))
		return false;
	was_in = ch->in_room;
	go_to = world[was_in]->dir_option[dir]->to_room();
	direction = dir + 1;
	use_horse = ch->IsOnHorse() && ch->has_horse(false)
		&& (ch->get_horse()->in_room == was_in || ch->get_horse()->in_room == go_to);
	is_horse = IS_HORSE(ch)
		&& ch->has_master()
		&& !AFF_FLAGGED(ch->get_master(), EAffect::kInvisible)
		&& (ch->get_master()->in_room == was_in
			|| ch->get_master()->in_room == go_to);

	if (!invis && !is_horse) {
		if (is_flee)
			strcpy(smallBuf, "сбежал$g");
		else if (ch->IsNpc() && NPC_FLAGGED(ch, ENpcFlag::kMoveRun))
			strcpy(smallBuf, "убежал$g");
		else if ((!use_horse && AFF_FLAGGED(ch, EAffect::kFly))
			|| (ch->IsNpc() && NPC_FLAGGED(ch, ENpcFlag::kMoveFly))) {
			strcpy(smallBuf, "улетел$g");
		} else if (ch->IsNpc()
			&& NPC_FLAGGED(ch, ENpcFlag::kMoveSwim)
			&& (real_sector(was_in) == ESector::kWaterSwim
				|| real_sector(was_in) == ESector::kWaterNoswim
				|| real_sector(was_in) == ESector::kUnderwater)) {
			strcpy(smallBuf, "уплыл$g");
		} else if (ch->IsNpc() && NPC_FLAGGED(ch, ENpcFlag::kMoveJump))
			strcpy(smallBuf, "ускакал$g");
		else if (ch->IsNpc() && NPC_FLAGGED(ch, ENpcFlag::kMoveCreep))
			strcpy(smallBuf, "уполз$q");
		else if (real_sector(was_in) == ESector::kWaterSwim
			|| real_sector(was_in) == ESector::kWaterNoswim
			|| real_sector(was_in) == ESector::kUnderwater) {
			strcpy(smallBuf, "уплыл$g");
		} else if (use_horse) {
			horse = ch->get_horse();
			if (horse && AFF_FLAGGED(horse, EAffect::kFly))
				strcpy(smallBuf, "улетел$g");
			else
				strcpy(smallBuf, "уехал$g");
		} else
			strcpy(smallBuf, "уш$y");

		if (is_flee && !ch->IsNpc() && CanUseFeat(ch, EFeat::kWriggler))
			sprintf(buf2, "$n %s.", smallBuf);
		else
			sprintf(buf2, "$n %s %s.", smallBuf, DirsTo[dir]);
		act(buf2, true, ch, nullptr, nullptr, kToRoom);
	}

	if (invis && !is_horse) {
		act("Кто-то тихо удалился отсюда.", true, ch, nullptr, nullptr, kToRoomSensors);
	}

	if (ch->IsOnHorse())
		horse = ch->get_horse();

	// Если сбежали, и по противнику никто не бьет, то убираем с него аттаку
	if (is_flee) {
		stop_fighting(ch, true);
	}

	if (!ch->IsNpc() && IS_SET(ch->track_dirs, 1 << dir)) {
		SendMsgToChar("Вы двинулись по следу.\r\n", ch);
		ImproveSkill(ch, ESkill::kTrack, true, nullptr);
	}

	RemoveCharFromRoom(ch);
	//затычка для бегства. чтоьы не отрабатывал MSDP протокол
	if (is_flee && !ch->IsNpc() && !CanUseFeat(ch, EFeat::kCalmness))
		FleeToRoom(ch, go_to);
	else
		PlaceCharToRoom(ch, go_to);
	if (horse) {
		GET_HORSESTATE(horse) -= 1;
		RemoveCharFromRoom(horse);
		PlaceCharToRoom(horse, go_to);
	}

	if (ch->IsFlagged(EPrf::kBlindMode)) {
		for (int i = 0; i < EDirection::kMaxDirNum; i++) {
			if (CAN_GO(ch, i)
				|| (EXIT(ch, i)
					&& EXIT(ch, i)->to_room() != kNowhere)) {
				const auto &rdata = EXIT(ch, i);
				if (ROOM_FLAGGED(rdata->to_room(), ERoomFlag::kDeathTrap)) {
					SendMsgToChar("\007 Внимание, рядом гиблое место!\r\n", ch);
				}
			}
		}
	}

	if (!invis && !is_horse) {
		if (is_flee
			|| (ch->IsNpc()
				&& NPC_FLAGGED(ch, ENpcFlag::kMoveRun))) {
			strcpy(smallBuf, "прибежал$g");
		} else if ((!use_horse && AFF_FLAGGED(ch, EAffect::kFly))
			|| (ch->IsNpc() && NPC_FLAGGED(ch, ENpcFlag::kMoveFly))) {
			strcpy(smallBuf, "прилетел$g");
		} else if (ch->IsNpc() && NPC_FLAGGED(ch, ENpcFlag::kMoveSwim)
			&& (real_sector(go_to) == ESector::kWaterSwim
				|| real_sector(go_to) == ESector::kWaterNoswim
				|| real_sector(go_to) == ESector::kUnderwater)) {
			strcpy(smallBuf, "приплыл$g");
		} else if (ch->IsNpc() && NPC_FLAGGED(ch, ENpcFlag::kMoveJump))
			strcpy(smallBuf, "прискакал$g");
		else if (ch->IsNpc() && NPC_FLAGGED(ch, ENpcFlag::kMoveCreep))
			strcpy(smallBuf, "приполз$q");
		else if (real_sector(go_to) == ESector::kWaterSwim
			|| real_sector(go_to) == ESector::kWaterNoswim
			|| real_sector(go_to) == ESector::kUnderwater) {
			strcpy(smallBuf, "приплыл$g");
		} else if (use_horse) {
			horse = ch->get_horse();
			if (horse && AFF_FLAGGED(horse, EAffect::kFly)) {
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
		look_at_room(ch, 0, !is_flee);

	if (!ch->IsNpc())
		ProcessRoomAffectsOnEntry(ch, ch->in_room);

	if (deathtrap::check_death_trap(ch)) {
		if (horse) {
			ExtractCharFromWorld(horse, false);
		}
		return false;
	}

	if (check_death_ice(go_to, ch)) {
		return false;
	}

	if (deathtrap::tunnel_damage(ch)) {
		return false;
	}

	greet_mtrigger(ch, dir);
	greet_otrigger(ch, dir);

	// add track info
	if (!AFF_FLAGGED(ch, EAffect::kNoTrack)
		&& (!ch->IsNpc()
			|| (mob_rnum = GET_MOB_RNUM(ch)) >= 0)) {
		for (track = world[go_to]->track; track; track = track->next) {
			if ((ch->IsNpc() && IS_SET(track->track_info, TRACK_NPC) && track->who == mob_rnum)
				|| (!ch->IsNpc() && !IS_SET(track->track_info, TRACK_NPC) && track->who == GET_UID(ch))) {
				break;
			}
		}

		if (!track && !ROOM_FLAGGED(go_to, ERoomFlag::kNoTrack)) {
			CREATE(track, 1);
			track->track_info = ch->IsNpc() ? TRACK_NPC : 0;
			track->who = ch->IsNpc() ? mob_rnum : GET_UID(ch);
			track->next = world[go_to]->track;
			world[go_to]->track = track;
		}

		if (track) {
			SET_BIT(track->time_income[Reverse[dir]], 1);
			if (IsAffectedBySpell(ch, ESpell::kLightWalk) && !ch->IsOnHorse())
				if (AFF_FLAGGED(ch, EAffect::kLightWalk))
					track->time_income[Reverse[dir]] <<= number(15, 30);
			REMOVE_BIT(track->track_info, TRACK_HIDE);
		}

		for (track = world[was_in]->track; track; track = track->next)
			if ((ch->IsNpc() && IS_SET(track->track_info, TRACK_NPC)
				&& track->who == mob_rnum) || (!ch->IsNpc()
				&& !IS_SET(track->track_info, TRACK_NPC)
				&& track->who == GET_UID(ch)))
				break;

		if (!track && !ROOM_FLAGGED(was_in, ERoomFlag::kNoTrack)) {
			CREATE(track, 1);
			track->track_info = ch->IsNpc() ? TRACK_NPC : 0;
			track->who = ch->IsNpc() ? mob_rnum : GET_UID(ch);
			track->next = world[was_in]->track;
			world[was_in]->track = track;
		}
		if (track) {
			SET_BIT(track->time_outgone[dir], 1);
			if (IsAffectedBySpell(ch, ESpell::kLightWalk) && !ch->IsOnHorse())
				if (AFF_FLAGGED(ch, EAffect::kLightWalk))
					track->time_outgone[dir] <<= number(15, 30);
			REMOVE_BIT(track->track_info, TRACK_HIDE);
		}
	}

	// hide improovment
	if (ch->IsNpc()) {
		for (const auto vict : world[ch->in_room]->people) {
			if (!vict->IsNpc()) {
				skip_hiding(vict, ch);
			}
		}
	}

	income_mtrigger(ch, direction - 1);

	if (ch->purged())
		return false;

	// char income, go mobs action
	if (!ch->IsNpc()) {
		for (const auto vict : world[ch->in_room]->people) {
			if (!vict->IsNpc()) {
				continue;
			}

			if (!CAN_SEE(vict, ch)
				|| AFF_FLAGGED(ch, EAffect::kSneak)
				|| AFF_FLAGGED(ch, EAffect::kDisguise)
				|| vict->GetEnemy()
				|| vict->GetPosition() < EPosition::kRest) {
				continue;
			}

			// AWARE mobs
			if (vict->IsFlagged(EMobFlag::kAware)
				&& vict->GetPosition() < EPosition::kFight
				&& !AFF_FLAGGED(vict, EAffect::kHold)
				&& vict->GetPosition() > EPosition::kSleep) {
				act("$n поднял$u.", false, vict, nullptr, nullptr, kToRoom | kToArenaListen);
				vict->SetPosition(EPosition::kStand);
			}
		}
	}

	// If flee - go agressive mobs
	if (!ch->IsNpc()
		&& is_flee) {
		mob_ai::do_aggressive_room(ch, false);
	}

	return direction;
}

int perform_move(CharData *ch, int dir, int need_specials_check, int checkmob, CharData *master) {
	if (AFF_FLAGGED(ch, EAffect::kBandage)) {
		SendMsgToChar("Перевязка была прервана!\r\n", ch);
		RemoveAffectFromChar(ch, ESpell::kBandage);
		AFF_FLAGS(ch).unset(EAffect::kBandage);
	}
	ch->set_motion(true);

	RoomRnum was_in;
	struct FollowerType *k, *next;

	if (ch == nullptr || dir < 0 || dir >= EDirection::kMaxDirNum || ch->GetEnemy())
		return false;
	else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room() == kNowhere)
		SendMsgToChar("Вы не сможете туда пройти...\r\n", ch);
	else if (EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kClosed)) {
		if (EXIT(ch, dir)->keyword) {
			sprintf(buf2, "Закрыто (%s).\r\n", EXIT(ch, dir)->keyword);
			SendMsgToChar(buf2, ch);
		} else
			SendMsgToChar("Закрыто.\r\n", ch);
	} else {
		if (!ch->followers) {
			if (!DoSimpleMove(ch, dir, need_specials_check, master, false))
				return false;
		} else {
			was_in = ch->in_room;
			// When leader mortally drunked - he changes direction
			// So returned value set to false or DIR + 1
			if (!(dir = DoSimpleMove(ch, dir, need_specials_check, master, false)))
				return false;

			--dir;
			for (k = ch->followers; k && k->follower->get_master(); k = next) {
				next = k->next;
				if (k->follower->in_room == was_in
					&& !k->follower->GetEnemy()
					&& HERE(k->follower)
					&& !(AFF_FLAGGED(k->follower, EAffect::kHold)
							|| AFF_FLAGGED(k->follower, EAffect::kStopFight)
							|| AFF_FLAGGED(k->follower, EAffect::kMagicStopFight))
					&& AWAKE(k->follower)
					&& (k->follower->IsNpc()
						|| (!k->follower->IsFlagged(EPlrFlag::kMailing)
							&& !k->follower->IsFlagged(EPlrFlag::kWriting)))
					&& (!IS_HORSE(k->follower)
						|| !AFF_FLAGGED(k->follower, EAffect::kTethered))) {
					if (k->follower->GetPosition() < EPosition::kStand) {
						if (k->follower->IsNpc()
							&& k->follower->get_master()->IsNpc()
							&& k->follower->GetPosition() > EPosition::kSleep
							&& !k->follower->get_wait()) {
							act("$n поднял$u.", false, k->follower, nullptr, nullptr, kToRoom | kToArenaListen);
							k->follower->SetPosition(EPosition::kStand);
						} else {
							continue;
						}
					}
//                   act("Вы поплелись следом за $N4.",false,k->ch,0,ch,TO_CHAR);
					perform_move(k->follower, dir, 1, false, ch);
				}
			}
		}
		if (checkmob) {
			mob_ai::do_aggressive_room(ch, true);
		}
		return true;
	}
	return false;
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
	int dir = 0, sneaking = IsAffectedBySpell(ch, ESpell::kSneak);

	skip_spaces(&argument);
	if (!ch->GetSkill(ESkill::kSneak)) {
		SendMsgToChar("Вы не умеете этого.\r\n", ch);
		return;
	}

	if (!*argument) {
		SendMsgToChar("И куда это вы направляетесь?\r\n", ch);
		return;
	}

	if ((dir = search_block(argument, dirs, false)) < 0 && (dir = search_block(argument, dirs_rus, false)) < 0) {
		SendMsgToChar("Неизвестное направление.\r\n", ch);
		return;
	}
	if (ch->IsOnHorse()) {
		act("Вам мешает $N.", false, ch, nullptr, ch->get_horse(), kToChar);
		return;
	}
	if (!sneaking) {
		Affect<EApply> af;
		af.type = ESpell::kSneak;
		af.location = EApply::kNone;
		af.modifier = 0;
		af.duration = 1;
		const int calculated_skill = CalcCurrentSkill(ch, ESkill::kSneak, nullptr);
		const int chance = number(1, MUD::Skill(ESkill::kSneak).difficulty);
		af.bitvector = (chance < calculated_skill) ? to_underlying(EAffect::kSneak) : 0;
		af.battleflag = 0;
		ImposeAffect(ch, af, false, false, false, false);
	}
	perform_move(ch, dir, 0, true, nullptr);
	if (!sneaking || IsAffectedBySpell(ch, ESpell::kGlitterDust)) {
		RemoveAffectFromChar(ch, ESpell::kSneak);
		AFF_FLAGS(ch).unset(EAffect::kSneak);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
