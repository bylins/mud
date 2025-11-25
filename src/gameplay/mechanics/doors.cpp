/**
\file doors.cpp - a part of the Bylins engine.
\authors Created by Sventovit.
\date 24.09.2024.
\brief Маханика работы с дверями и хзакрывающимися контенйерами.
\detail Что тут нужно сделать - разделить функции на работающие отдельно с контейнерами и с дверями.
 В команда проверять, что мы собствено делаем, дальше вызывать конкретные функции - открыть дверь, открыть конейнер,
 взломать дверь, взломать контейнер и так далее. А не проверять в каждой функции, работаем мы с объектом или дверью.
 Тогда и scmd тут не понадобится и его можно будет либо убрать в do_gen_door, либо вообще удалить.
*/

#include "engine/entities/obj_data.h"
#include "engine/entities/room_data.h"
#include "engine/entities/char_data.h"
#include "gameplay/mechanics/doors.h"
#include "engine/core/comm.h"
#include "utils/utils_string.h"
#include "engine/core/handler.h"
#include "treasure_cases.h"
#include "gameplay/skills/pick.h"
#include "administration/privilege.h"
#include "named_stuff.h"
#include "engine/db/player_index.h"

enum EDoorError : int {
  kWrongDir = -1,
  kWrongDirDoorName = -2,
  kNoDoorGivenDir = -3,
  kDoorNameIsEmpty = -4,
  kWrongDoorName = -5,
  kNoError = -6
};

struct FindDoorDirResult {
  explicit FindDoorDirResult(EDoorError error_, EDirection dir_ = EDirection::kUndefinedDir)
	  : error(error_),
		dir(dir_) {};

  EDoorError error{EDoorError::kNoError};
  EDirection dir{};
};

const char *a_cmd_door[] =
	{
		"открыть",
		"закрыть",
		"отпереть",
		"запереть",
		"взломать"
	};

const Bitvector kNeedOpen = 1 << 0;
const Bitvector kNeedClosed = 1 << 1;
const Bitvector kNeedUnlocked = 1 << 2;
const Bitvector kNeedLocked = 1 << 3;

const int flags_door[] =
	{
		kNeedClosed | kNeedUnlocked,
		kNeedOpen,
		kNeedClosed | kNeedLocked,
		kNeedClosed | kNeedUnlocked,
		kNeedClosed | kNeedLocked
	};

FindDoorDirResult FindDoorDir(CharData *ch, const char *type, char *dir, EDoorScmd scmd);

ObjVnum GetDoorKeyVnum(CharData *ch, ObjData *obj, EDirection dir) {
	if (obj) {
		return ObjVnum(GET_OBJ_VAL(obj, 2));
	} else {
		return EXIT(ch, dir)->key;
	}
}

bool IsdoorOpenable(CharData *ch, ObjData *obj, EDirection dir) {
	if (obj) {
		return (obj->get_type() == EObjType::kContainer) && OBJVAL_FLAGGED(obj, EContainerFlag::kCloseable);
	} else {
		return EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kHasDoor);
	}
}

bool IsDoorBroken(CharData *ch, ObjData *obj, EDirection dir) {
	if (obj) {
		return OBJVAL_FLAGGED(obj, EContainerFlag::kLockIsBroken);
	} else {
		return EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kBrokenLock);
	}
}

bool IdDoorExist(CharData *ch, EDirection dir) {
	return EXIT_FLAGGED(EXIT(ch,dir), EExitFlag::kHasDoor);
}

bool IsDoorPickroof(CharData *ch, ObjData *obj, EDirection dir) {
	if (obj) {
		return OBJVAL_FLAGGED(obj, EContainerFlag::kUncrackable) || OBJVAL_FLAGGED(obj, EContainerFlag::kLockIsBroken);
	} else {
		return EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kPickroof)
			|| EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kBrokenLock);
	}
}

ubyte GetLockComplexity(CharData *ch, ObjData *obj, EDirection dir) {
	if (obj) {
		return GET_OBJ_VAL(obj, 3);
	} else {
		return EXIT(ch, dir)->lock_complexity;
	}
}

bool IsDoorOpen(CharData *ch, ObjData *obj, EDirection dir) {
	if (obj) {
		return !OBJVAL_FLAGGED(obj, EContainerFlag::kShutted);
	} else {
            return !EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kClosed);
	}
}

bool IsDoorUnlocked(CharData *ch, ObjData *obj, EDirection dir) {
	if (obj) {
		return !OBJVAL_FLAGGED(obj, EContainerFlag::kLockedUp);
	} else {
		return !EXIT_FLAGGED(EXIT(ch, dir), EExitFlag::kLocked);
	}
}

bool IsDoorLocked(CharData *ch, ObjData *obj, EDirection dir) { return !IsDoorUnlocked(ch, obj, dir); }

bool IsDoorClosed(CharData *ch, ObjData *obj, EDirection dir) {
	return !IsDoorOpen(ch, obj, dir);
}

const char *cmd_door[] =
	{
		"открыл$g",
		"закрыл$g",
		"отпер$q",
		"запер$q",
		"взломал$g"
	};

FindDoorDirResult FindDoorDir(CharData *ch, const char *type, char *dir, EDoorScmd scmd) {
	EDirection door;
	if (*dir) {
		if ((door = static_cast<EDirection>(search_block(dir, dirs, false))) == EDirection::kUndefinedDir
			&& (door = static_cast<EDirection>(search_block(dir, dirs_rus, false))) == EDirection::kUndefinedDir) {
			return FindDoorDirResult(EDoorError::kWrongDir);
		}
		if (EXIT(ch, door)) {
			if (EXIT(ch, door)->keyword && EXIT(ch, door)->vkeyword) {
				if (isname(type, EXIT(ch, door)->keyword) || isname(type, EXIT(ch, door)->vkeyword)) {
					return FindDoorDirResult(EDoorError::kNoError, door);
				} else {
					return FindDoorDirResult(EDoorError::kWrongDoorName);
				}
			} else if (utils::IsAbbr(type, "дверь") || utils::IsAbbr(type, "door")) {
				return FindDoorDirResult(EDoorError::kNoError, door);
			} else {
				return FindDoorDirResult(EDoorError::kWrongDoorName);
			}
		} else {
			return FindDoorDirResult(EDoorError::kNoDoorGivenDir);
		}
	} else {
		if (!*type) {
			return FindDoorDirResult(EDoorError::kDoorNameIsEmpty);
		}
		for (door = EDirection::kFirstDir; door <= EDirection::kLastDir; ++door) {
			auto found = false;
			if (EXIT(ch, door)) {
				if (EXIT(ch, door)->keyword && EXIT(ch, door)->vkeyword) {
					if (isname(type, EXIT(ch, door)->keyword) || isname(type, EXIT(ch, door)->vkeyword)) {
						found = true;
					}
				} else if (IdDoorExist(ch, door) && (utils::IsAbbr(type, "дверь") || utils::IsAbbr(type, "door"))) {
					found = true;
				}
			}
			if (found) {
				if ((scmd == kScmdOpen && IsDoorOpen(ch, nullptr, door)) ||
					(scmd == kScmdClose && IsDoorClosed(ch, nullptr, door))) {
					continue;
				} else {
					return FindDoorDirResult(EDoorError::kNoError, door);
				}
			}
		}
		return FindDoorDirResult(EDoorError::kWrongDoorName);
	}

}

bool IsPickLockSucessdul(CharData *ch, ObjVnum /*keynum*/, ObjData *obj, EDirection door, EDoorScmd scmd) {
	if (scmd != kScmdPick) {
		return true;
	}

	if (IsDoorPickroof(ch, obj, door)) {
		SendMsgToChar("Вы никогда не сможете взломать ЭТО.\r\n", ch);
		return false;
	}

	if (!check_moves(ch, kPicklockMoves)) {
		return false;
	}

	const auto &pbi = get_pick_probability(ch, GetLockComplexity(ch, obj, door));

	const bool pick_success = pbi.unlock_probability >= number(1, 100);

	if (pbi.skill_train_allowed) {
		TrainSkill(ch, ESkill::kPickLock, pick_success, nullptr);
	}

	if (!pick_success) {
		// неудачная попытка взлома - есть шанс сломать замок
		const int brake_probe = number(1, 100);
		if (pbi.brake_lock_probability >= brake_probe) {
			SendMsgToChar("Вы все-таки сломали этот замок...\r\n", ch);
			if (obj) {
				auto v = obj->get_val(1);
				SET_BIT(v, EContainerFlag::kLockIsBroken);
				obj->set_val(1, v);
			} else {
				SET_BIT(EXIT(ch, door)->exit_info, EExitFlag::kBrokenLock);
			}
		} else {
			if (pbi.unlock_probability == 0) {
				SendMsgToChar("С таким сложным замком даже и пытаться не следует...\r\n", ch);
				return false;
			} else {
				SendMsgToChar("Взломщик из вас пока еще никудышний.\r\n", ch);
			}
		}
	}

	return pick_success;
}

void OPEN_DOOR(const RoomRnum room, ObjData *obj, const int door) {
	if (obj) {
		auto v = obj->get_val(1);
		TOGGLE_BIT(v, EContainerFlag::kShutted);
		obj->set_val(1, v);
	} else {
		TOGGLE_BIT(world[room]->dir_option[door]->exit_info, EExitFlag::kClosed);
	}
}

void LOCK_DOOR(const RoomRnum room, ObjData *obj, const int door) {
	if (obj) {
		auto v = obj->get_val(1);
		TOGGLE_BIT(v, EContainerFlag::kLockedUp);
		obj->set_val(1, v);
	} else {
		TOGGLE_BIT(world[room]->dir_option[door]->exit_info, EExitFlag::kLocked);
	}
}

void do_doorcmd(CharData *ch, ObjData *obj, int door, EDoorScmd scmd) {
	bool deaf = false;
	int other_room = 0;
	int rev_dir[] = {EDirection::kSouth, EDirection::kWest, EDirection::kNorth, EDirection::kEast, EDirection::kDown,
					 EDirection::kUp};
	char local_buf[kMaxStringLength]; // строка, в которую накапливается совершенное действо
	// пишем начало строки - кто чё сделал
	sprintf(local_buf, "$n %s ", cmd_door[scmd]);

	if (AFF_FLAGGED(ch, EAffect::kDeafness))
		deaf = true;
	// ищем парную дверь в другой клетке
	RoomData::exit_data_ptr back;
	if (!obj && EXIT(ch, door) && ((other_room = EXIT(ch, door)->to_room()) != kNowhere)) {
		back = world[other_room]->dir_option[rev_dir[door]];
		if (back) {
			if ((back->to_room() != ch->in_room)
				|| ((EXITDATA(ch->in_room, door)->exit_info
					^ EXITDATA(other_room, rev_dir[door])->exit_info)
					& (EExitFlag::kHasDoor | EExitFlag::kClosed | EExitFlag::kLocked))) {
				back.reset();
			}
		}
	}

	switch (scmd) {
		case kScmdOpen:
		case kScmdClose:
			if (scmd == kScmdOpen && obj && !open_otrigger(obj, ch, false))
				return;
			if (scmd == kScmdOpen && !obj && !open_wtrigger(world[ch->in_room], ch, door, false))
				return;
			if (scmd == kScmdOpen && !obj && back && !open_wtrigger(world[other_room], ch, rev_dir[door], false))
				return;
			if (scmd == kScmdClose && obj && !close_otrigger(obj, ch, false))
				return;
			if (scmd == kScmdClose && !obj && !close_wtrigger(world[ch->in_room], ch, door, false))
				return;
			if (scmd == kScmdClose && !obj && back && !close_wtrigger(world[other_room], ch, rev_dir[door], false))
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
						GetPlayerNameByUnique(GET_OBJ_VAL(obj, 3)).c_str());
				mudlog(buf, NRM, kLvlGreatGod, MONEY_LOG, true);
				system_obj::process_open_purse(ch, obj);
				return;
			} else {
				SendMsgToChar(OK, ch);
			}
			break;

		case kScmdUnlock:
		case kScmdLock:
			if (scmd == kScmdUnlock && obj && !open_otrigger(obj, ch, true))
				return;
			if (scmd == kScmdLock && obj && !close_otrigger(obj, ch, true))
				return;
			if (scmd == kScmdUnlock && !obj && !open_wtrigger(world[ch->in_room], ch, door, true))
				return;
			if (scmd == kScmdLock && !obj && !close_wtrigger(world[ch->in_room], ch, door, true))
				return;
			if (scmd == kScmdUnlock && !obj && back && !open_wtrigger(world[other_room], ch, rev_dir[door], true))
				return;
			if (scmd == kScmdLock && !obj && back && !close_wtrigger(world[other_room], ch, rev_dir[door], true))
				return;
			LOCK_DOOR(ch->in_room, obj, door);
			if (back)
				LOCK_DOOR(other_room, obj, rev_dir[door]);
			if (!deaf)
				SendMsgToChar("*Щелк*\r\n", ch);
			if (obj) {
				treasure_cases::UnlockTreasureCase(ch, obj);
			}
			break;

		case kScmdPick:
			if (obj && !pick_otrigger(obj, ch))
				return;
			if (!obj && !pick_wtrigger(world[ch->in_room], ch, door))
				return;
			if (!obj && back && !pick_wtrigger(world[other_room], ch, rev_dir[door]))
				return;
			LOCK_DOOR(ch->in_room, obj, door);
			if (back)
				LOCK_DOOR(other_room, obj, rev_dir[door]);
			SendMsgToChar("Замок очень скоро поддался под вашим натиском.\r\n", ch);
			strcpy(local_buf, "$n умело взломал$g ");
			break;
	}

	// Notify the room
	sprintf(local_buf + strlen(local_buf), "%s.", (obj) ? "$o3" : (EXIT(ch, door)->vkeyword ? "$F" : "дверь"));
	if (!obj || (obj->get_in_room() != kNowhere)) {
		act(local_buf, false, ch, obj, obj ? nullptr : EXIT(ch, door)->vkeyword, kToRoom);
	}

	// Notify the other room
	if ((scmd == kScmdOpen || scmd == kScmdClose) && back) {
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

int HasKey(CharData *ch, ObjVnum key) {
	for (auto o = ch->carrying; o; o = o->get_next_content()) {
		if (GET_OBJ_VNUM(o) == key && key != -1) {
			return true;
		}
	}

	if (GET_EQ(ch, kHold)) {
		if (GET_OBJ_VNUM(GET_EQ(ch, kHold)) == key && key != -1) {
			return true;
		}
	}

	return false;
}

void go_gen_door(CharData *ch, char *type, char *dir, int where_bits, int subcmd) {
	auto door = FindDoorDir(ch, type, dir, (EDoorScmd) subcmd);
	ObjData *obj{nullptr};
	CharData *victim{nullptr};
	if (door.error != EDoorError::kNoError) {
		if (!generic_find(type, where_bits, ch, &victim, &obj)) {
			switch (door.error) {
				case EDoorError::kWrongDir: SendMsgToChar("Уточните направление.\r\n", ch);
					break;
				case EDoorError::kWrongDirDoorName: sprintf(buf, "Вы не видите '%s' в этой комнате.\r\n", type);
					SendMsgToChar(buf, ch);
					break;
				case EDoorError::kNoDoorGivenDir: sprintf(buf, "Вы не можете это '%s'.\r\n", a_cmd_door[subcmd]);
					SendMsgToChar(buf, ch);
					break;
				case EDoorError::kDoorNameIsEmpty: sprintf(buf, "Что вы хотите '%s'?\r\n", a_cmd_door[subcmd]);
					SendMsgToChar(buf, ch);
					break;
				case EDoorError::kWrongDoorName: sprintf(buf, "Вы не видите здесь '%s'.\r\n", type);
					SendMsgToChar(buf, ch);
					break;
				default: SendMsgToChar("Что-то с дверью произошло непнятное, сообщите богам.", ch);
					break;
			}
			return;
		}
	}

	if ((obj) || (door.dir != EDirection::kUndefinedDir)) {
		if ((obj) && !IS_IMMORTAL(ch) && (obj->has_flag(EObjFlag::kNamed))
			&& NamedStuff::check_named(ch, obj, true))//Именной предмет открывать(закрывать) может только владелец
		{
			if (!NamedStuff::wear_msg(ch, obj))
				SendMsgToChar("Просьба не трогать! Частная собственность!\r\n", ch);
			return;
		}
		auto keynum = GetDoorKeyVnum(ch, obj, door.dir);
		if ((subcmd == kScmdClose || subcmd == kScmdLock) && !ch->IsNpc() && NORENTABLE(ch))
			SendMsgToChar("Ведите себя достойно во время боевых действий!\r\n", ch);
		else if (!(IsdoorOpenable(ch, obj, door.dir)))
			act("Вы никогда не сможете $F это!", false, ch, nullptr, a_cmd_door[subcmd], kToChar);
		else if (!IsDoorOpen(ch, obj, door.dir) && IS_SET(flags_door[subcmd], kNeedOpen))
			SendMsgToChar("Вообще-то здесь закрыто!\r\n", ch);
		else if (!IsDoorClosed(ch, obj, door.dir) && IS_SET(flags_door[subcmd], kNeedClosed))
			SendMsgToChar("Уже открыто!\r\n", ch);
		else if (!(IsDoorLocked(ch, obj, door.dir)) && IS_SET(flags_door[subcmd], kNeedLocked))
			SendMsgToChar("Да отперли уже все...\r\n", ch);
		else if (!(IsDoorUnlocked(ch, obj, door.dir)) && IS_SET(flags_door[subcmd], kNeedUnlocked))
			SendMsgToChar("Угу, заперто.\r\n", ch);
		else if (!HasKey(ch, keynum) && !privilege::CheckFlag(ch, privilege::kUseSkills)
			&& ((subcmd == kScmdLock) || (subcmd == kScmdUnlock)))
			SendMsgToChar("У вас нет ключа.\r\n", ch);
		else if (IsDoorBroken(ch, obj, door.dir) && !privilege::CheckFlag(ch, privilege::kUseSkills)
			&& ((subcmd == kScmdLock) || (subcmd == kScmdUnlock)))
			SendMsgToChar("Замок сломан.\r\n", ch);
		else if (IsPickLockSucessdul(ch, keynum, obj, door.dir, static_cast<EDoorScmd>(subcmd)))
			do_doorcmd(ch, obj, door.dir, (EDoorScmd) subcmd);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
