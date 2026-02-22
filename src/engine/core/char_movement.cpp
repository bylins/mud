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
#include "char_movement.h"

#include "gameplay/mechanics/deathtrap.h"
#include "gameplay/mechanics/hide.h"
#include "engine/entities/entities_constants.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/pk.h"
#include "gameplay/ai/mobact.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "gameplay/mechanics/named_stuff.h"
#include "engine/db/obj_prototypes.h"
#include "gameplay/skills/track.h"
#include "utils/random.h"
#include "engine/db/global_objects.h"
#include "gameplay/mechanics/liquid.h"
#include "engine/core/utils_char_obj.inl"
#include "gameplay/mechanics/sight.h"
#include "gameplay/mechanics/awake.h"
#include "gameplay/mechanics/boat.h"
#include "gameplay/magic/magic_rooms.h"
#include "engine/ui/color.h"
#include "gameplay/mechanics/cities.h"
#include "utils/backtrace.h"

void FleeToRoom(CharData *ch, RoomRnum room);

// external functs
void SetWait(CharData *ch, int waittime, int victim_in_room);
int find_eq_pos(CharData *ch, ObjData *obj, char *local_arg);
const int Reverse[EDirection::kMaxDirNum] = {2, 3, 0, 1, 5, 4};

inline int GetForestPathsSect(int sect) {
	switch (sect) {
		case ESector::kForest:		return ESector::kField;
		case ESector::kForestSnow:	return ESector::kFieldSnow;
		case ESector::kForestRain:	return ESector::kFieldRain;
		default:					return sect;
	}
}

inline int GetMountainsPathsSect(int sect) {
	switch (sect) {
		case ESector::kHills:			[[fallthrough]];
		case ESector::kMountain:		return ESector::kField;
		case ESector::kHillsRain:		return ESector::kFieldRain;
		case ESector::kHillsSnow:		[[fallthrough]];
		case ESector::kMountainSnow:	return ESector::kFieldSnow;
		default: return sect;
	}
}

int CalcMoveCost(CharData *ch, int dir) {
	// move points needed is avg. move loss for src and destination sect type
	auto ch_inroom = real_sector(ch->in_room);
	auto ch_toroom = real_sector(EXIT(ch, dir)->to_room());

	if (CanUseFeat(ch, EFeat::kForestPath)) {
		ch_inroom = GetForestPathsSect(ch_inroom);
		ch_toroom = GetForestPathsSect(ch_toroom);
	}

	if (CanUseFeat(ch, EFeat::kMountainPath)) {
		ch_inroom = GetMountainsPathsSect(ch_inroom);
		ch_toroom = GetMountainsPathsSect(ch_toroom);
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

		if (IsCharNeedBoatToMove(ch, dir)) {
			return false;
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
				return false;
			}
		}

		if (IsCharNeedBoatToMove(ch, dir)) {
			if (show_msg) {
				SendMsgToChar("Вам нужна лодка, чтобы попасть туда.\r\n", ch);
			}
			return false;
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

		const auto need_movement = CalcMoveCost(ch, dir);
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

	if (directions.empty() || number(1, 100 + 25 * static_cast<int>(directions.size())) < fail_chance) {
		return EDirection::kUndefinedDir;
	}
	return directions[number(0, static_cast<int>(directions.size()) - 1)];
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

bool PerformSimpleMove(CharData *ch, int dir, int following, CharData *leader, EMoveType move_type) {
	struct TrackData *track;
	RoomRnum was_in, go_to;
	int i, invis = 0, use_horse = 0, is_horse = 0;
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
		ch->set_move(ch->get_move() - CalcMoveCost(ch, dir));

	i = MUD::Skill(ESkill::kSneak).difficulty;
	if (AFF_FLAGGED(ch, EAffect::kSneak) && move_type != EMoveType::kFlee) {
		if (ch->IsNpc())
			invis = 1;
		else if (awake_sneak(ch)) {
			RemoveAffectFromChar(ch, ESpell::kSneak);
			AFF_FLAGS(ch).unset(EAffect::kSneak);
		} else if (!IsAffectedBySpell(ch, ESpell::kSneak) || CalcCurrentSkill(ch, ESkill::kSneak, nullptr) >= number(1, i))
			invis = 1;
	}

	i = MUD::Skill(ESkill::kDisguise).difficulty;
	if (AFF_FLAGGED(ch, EAffect::kDisguise) && move_type != EMoveType::kFlee) {
		if (ch->IsNpc())
			invis = 1;
		else if (awake_camouflage(ch)) {
			RemoveAffectFromChar(ch, ESpell::kCamouflage);
			AFF_FLAGS(ch).unset(EAffect::kDisguise);
		} else if (!IsAffectedBySpell(ch, ESpell::kCamouflage) ||
		CalcCurrentSkill(ch, ESkill::kDisguise, nullptr) >= number(1, i))
			invis = 1;
	}

	if (move_type == EMoveType::kDefault) {
		sprintf(buf, "Вы поплелись %s%s.", leader ? "следом за $N4 " : "", DirsTo[dir]);
		act(buf, false, ch, nullptr, leader, kToChar);
	}
	if (move_type == EMoveType::kThrowOut) {
		sprintf(buf, "Вы со свистом улетели %s.", DirsTo[dir]);
		act(buf, false, ch, nullptr, leader, kToChar);
	}
	if (ch->IsNpc() && ch->IsFlagged(EMobFlag::kSentinel) &&
	!IS_CHARMICE(ch) && ROOM_FLAGGED(ch->in_room, ERoomFlag::kArena))
		return false;
	was_in = ch->in_room;
	go_to = world[was_in]->dir_option[dir]->to_room();
	use_horse = ch->IsOnHorse() && ch->has_horse(false)
		&& (ch->get_horse()->in_room == was_in || ch->get_horse()->in_room == go_to);
	is_horse = IS_HORSE(ch)
		&& ch->has_master()
		&& !AFF_FLAGGED(ch->get_master(), EAffect::kInvisible)
		&& (ch->get_master()->in_room == was_in
			|| ch->get_master()->in_room == go_to);

	if (!invis && !is_horse) {
		if (move_type == EMoveType::kFlee)
			strcpy(smallBuf, "сбежал$g");
		else if (move_type == EMoveType::kThrowOut)
			strcpy(smallBuf, "со свистом полетел$g");
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

		if (move_type == EMoveType::kFlee && !ch->IsNpc() && CanUseFeat(ch, EFeat::kWriggler))
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
	if (move_type == EMoveType::kFlee) {
		stop_fighting(ch, true);
	}

	if (!ch->IsNpc() && IS_SET(ch->track_dirs, 1 << dir)) {
		SendMsgToChar("Вы двинулись по следу.\r\n", ch);
		ImproveSkill(ch, ESkill::kTrack, true, nullptr);
	}

	RemoveCharFromRoom(ch);
	//затычка для бегства. чтоьы не отрабатывал MSDP протокол
	if (move_type == EMoveType::kFlee && !ch->IsNpc() && !CanUseFeat(ch, EFeat::kCalmness))
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
		if (move_type == EMoveType::kFlee
			|| (ch->IsNpc()
				&& NPC_FLAGGED(ch, ENpcFlag::kMoveRun))) {
			strcpy(smallBuf, "прибежал$g");
		} else if (move_type == EMoveType::kThrowOut)
			strcpy(smallBuf, "со свистом прилетел$g");
		else if ((!use_horse && AFF_FLAGGED(ch, EAffect::kFly))
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
		look_at_room(ch, 0, move_type != EMoveType::kFlee);

	if (!ch->IsNpc())
		room_spells::ProcessRoomAffectsOnEntry(ch, ch->in_room);

	if (deathtrap::check_death_trap(ch)) {
		if (horse) {
			ExtractCharFromWorld(horse, false);
		}
		return false;
	}

	if (deathtrap::CheckIceDeathTrap(go_to, ch)) {
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
				|| (!ch->IsNpc() && !IS_SET(track->track_info, TRACK_NPC) && track->who == ch->get_uid())) {
				break;
			}
		}

		if (!track && !ROOM_FLAGGED(go_to, ERoomFlag::kNoTrack)) {
			CREATE(track, 1);
			track->track_info = ch->IsNpc() ? TRACK_NPC : 0;
			track->who = ch->IsNpc() ? mob_rnum : ch->get_uid();
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
				&& track->who == ch->get_uid()))
				break;

		if (!track && !ROOM_FLAGGED(was_in, ERoomFlag::kNoTrack)) {
			CREATE(track, 1);
			track->track_info = ch->IsNpc() ? TRACK_NPC : 0;
			track->who = ch->IsNpc() ? mob_rnum : ch->get_uid();
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
				SkipHiding(vict, ch);
			}
		}
	}

	income_mtrigger(ch, dir);

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

	if (!ch->IsNpc() && move_type == EMoveType::kFlee) {
		mob_ai::do_aggressive_room(ch, false);
	}

	return true;
}

bool PerformMove(CharData *ch, int dir, int need_specials_check, int checkmob, CharData *master) {
	if (AFF_FLAGGED(ch, EAffect::kBandage)) {
		SendMsgToChar("Перевязка была прервана!\r\n", ch);
		RemoveAffectFromChar(ch, ESpell::kBandage);
		AFF_FLAGS(ch).unset(EAffect::kBandage);
	}
	ch->set_motion(true);

	RoomRnum was_in;
	struct FollowerType *k, *next;
/*
	if (!ch->IsNpc() || IS_CHARMICE(ch)) {
		std::ostringstream out;
		out << "Двигаемся по направлению: " << dir << "\r\n";
		mudlog(out.str());
	}
*/
	if (ch == nullptr || dir < EDirection::kFirstDir || dir > EDirection::kLastDir || ch->GetEnemy())
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
			if (!PerformSimpleMove(ch, dir, need_specials_check, master, EMoveType::kDefault))
				return false;
		} else {
			was_in = ch->in_room;
			// When leader mortally drunked - he changes direction
			// So returned value set to false or DIR + 1
			if (!PerformSimpleMove(ch, dir, need_specials_check, master, EMoveType::kDefault)) {
				return false;
			}
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
					PerformMove(k->follower, dir, 1, false, ch);
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

void FleeToRoom(CharData *ch, RoomRnum room) {
	if (ch == nullptr || room < kNowhere + 1 || room > top_of_world) {
		debug::backtrace(runtime_config.logs(ERRLOG).handle());
		log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p", room, top_of_world, ch);
		return;
	}

	if (!ch->IsNpc() && !Clan::MayEnter(ch, room, kHousePortal)) {
		room = ch->get_from_room();
	}

	if (!ch->IsNpc() && NORENTABLE(ch) && ROOM_FLAGGED(room, ERoomFlag::kArena) && !IS_IMMORTAL(ch)) {
		SendMsgToChar("Вы не можете попасть на арену в состоянии боевых действий!\r\n", ch);
		room = ch->get_from_room();
	}
	world[room]->people.push_front(ch);

	ch->in_room = room;
	CheckLight(ch, kLightNo, kLightNo, kLightNo, kLightNo, 1);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILHIDE);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILSNEAK);
	EXTRA_FLAGS(ch).unset(EXTRA_FAILCAMOUFLAGE);
	if (ch->IsFlagged(EPrf::kCoderinfo)) {
		sprintf(buf,
				"%sКомната=%s%d %sСвет=%s%d %sОсвещ=%s%d %sКостер=%s%d %sЛед=%s%d "
				"%sТьма=%s%d %sСолнце=%s%d %sНебо=%s%d %sЛуна=%s%d%s.\r\n",
				kColorNrm, kColorBoldBlk, room,
				kColorRed, kColorBoldRed, world[room]->light,
				kColorGrn, kColorBoldGrn, world[room]->glight,
				kColorYel, kColorBoldYel, world[room]->fires,
				kColorYel, kColorBoldYel, world[room]->ices,
				kColorBlu, kColorBoldBlu, world[room]->gdark,
				kColorMag, kColorBoldCyn, weather_info.sky,
				kColorWht, kColorBoldBlk, weather_info.sunlight,
				kColorYel, kColorBoldYel, weather_info.moon_day, kColorNrm);
		SendMsgToChar(buf, ch);
	}
	// Stop fighting now, if we left.
	if (ch->GetEnemy() && ch->in_room != ch->GetEnemy()->in_room) {
		stop_fighting(ch->GetEnemy(), false);
		stop_fighting(ch, true);
	}

	if (!ch->IsNpc()) {
		zone_table[world[room]->zone_rn].used = true;
		zone_table[world[room]->zone_rn].activity++;
	} else {
		//sventovit: здесь обрабатываются только неписи, чтобы игрок успел увидеть комнату
		room_spells::ProcessRoomAffectsOnEntry(ch, ch->in_room);
	}

	cities::CheckCityVisit(ch, room);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
