// deathtrap.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "deathtrap.h"

#include "gameplay/core/constants.h"
#include "engine/entities/entities_constants.h"
#include "engine/core/handler.h"
#include "gameplay/clans/house.h"
#include "corpse.h"
#include "gameplay/fight/fight.h"
#include "gameplay/fight/fight_stuff.h"
#include "gameplay/mechanics/damage.h"
#include "boat.h"
#include "engine/db/global_objects.h"

extern void death_cry(CharData *ch, CharData *killer);

namespace deathtrap {

// список текущих слоу-дт в маде
std::list<RoomData *> room_list;

void log_death_trap(CharData *ch);
void remove_items(CharData *ch);

} // namespace DeathTrap

// * Инициализация списка при загрузке мада или редактирования комнат в олц
void deathtrap::load() {
	// на случай релоада, свапать смысла нету
	room_list.clear();

	for (int i = kFirstRoom; i <= top_of_world; ++i)
		if (ROOM_FLAGGED(i, ERoomFlag::kSlowDeathTrap) || ROOM_FLAGGED(i, ERoomFlag::kIceTrap))
			room_list.push_back(world[i]);
}

/**
* Добавление новой комнаты с проверкой на присутствие
* \param room - комната, кот. добавляем
*/
void deathtrap::add(RoomData *room) {
	std::list<RoomData *>::const_iterator it = std::find(room_list.begin(), room_list.end(), room);
	if (it == room_list.end())
		room_list.push_back(room);
}

/**
* Удаление комнаты из списка слоу-дт
* \param room - комната, кот. удаляем
*/
void deathtrap::remove(RoomData *room) {
	room_list.remove(room);
}

/// Проверка активности дт, дергается каждые 2 секунды в хеарбите.
/// Доп список строится для случаев, когда в списке комнаты сначала идет чар,
/// а следом его чармисы и последовательный проход по ch->next_in_room
/// с пуржем чара натыкается далее на обнуленные структуры чармисов.
void deathtrap::activity() {
	for (auto it = room_list.cbegin(); it != room_list.cend(); ++it) {
		const auto people = (*it)->people; // make copy of people in the room
		for (const auto i : people) {
			if (i->purged() || (i->IsNpc() && !IS_CHARMICE(i))) {
				continue;
			}
			std::string name = i->get_name_str();

			Damage dmg(SimpleDmg(kTypeRoomdeath), std::max(1, i->get_real_max_hit() >> 2), fight::kUndefDmg);
			dmg.flags.set(fight::kNoFleeDmg);

			if (dmg.Process(i, i) < 0) {
				char buf_[kMaxInputLength];
				snprintf(buf_, sizeof(buf_), "Player %s died in slow DT (room %d)", name.c_str(), (*it)->vnum);
				mudlog(buf_, CMP, kLvlImmortal, SYSLOG, true);
			}
		}
	}
}

// * Логирование в отдельный файл уходов в дт для интересу и мб статистики.
void deathtrap::log_death_trap(CharData *ch) {
	const auto filename = runtime_config.log_dir() + "/death_trap.log";
	static FILE *file = 0;
	if (!file) {
		file = fopen(filename.c_str(), "a");
		if (!file) {
			log("SYSERR: can't open %s!", filename.c_str());
			return;
		}
		opened_files.push_back(file);
	}
	write_time(file);
	fprintf(file, "%s hit death trap #%d (%s)\n", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), world[ch->in_room]->name);
	fflush(file);
}

// * Попадание в обычное дт.
int deathtrap::check_death_trap(CharData *ch) {
	if (ch->in_room != kNowhere && !ch->IsFlagged(EPrf::kCoderinfo)) {
		if ((ROOM_FLAGGED(ch->in_room, ERoomFlag::kDeathTrap) && !IS_IMMORTAL(ch))
			|| (real_sector(ch->in_room) == ESector::kOnlyFlying && !ch->IsNpc() &&
			!IS_GOD(ch) && !AFF_FLAGGED(ch, EAffect::kFly))
			|| IsCharCanDrownThere(ch, ch->in_room)) {
			ObjData *corpse;
			deathtrap::log_death_trap(ch);

			if (check_tester_death(ch, nullptr)) {
				sprintf(buf1,
						"Player %s died in DT (room %d) but zone is under construction.",
						GET_NAME(ch),
						GET_ROOM_VNUM(ch->in_room));
				mudlog(buf1, LGH, kLvlImmortal, SYSLOG, true);
				return false;
			}

			sprintf(buf1, "Player %s died in DT (room %d)", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room));
			mudlog(buf1, LGH, kLvlImmortal, SYSLOG, true);
			death_cry(ch, nullptr);
			corpse = make_corpse(ch);
			if (corpse != nullptr) {
				RemoveObjFromRoom(corpse);    // для того, чтобы удалилость все содержимое
				ExtractObjFromWorld(corpse);
			}
			ch->set_hit(0);
			ch->set_move(0);
			if (NORENTABLE(ch)) {
				die(ch, nullptr);
			} else {
				CharStat::UpdateOnKill(ch, nullptr, 0);
				ExtractCharFromWorld(ch, true);
			}
			return true;
		}
	}
	return false;
}

bool deathtrap::IsSlowDeathtrap(int rnum) {
	if (ROOM_FLAGGED(rnum, ERoomFlag::kSlowDeathTrap) || ROOM_FLAGGED(rnum, ERoomFlag::kIceTrap))
		return true;
	return false;
}

/// Проверка чара на дамаг в ванруме, если он попадет в комнату RoomRnum
/// \return если > 0, то величину дамага,
/// иначе - чара в tunnel_damage() не дамагнет
int calc_tunnel_dmg(CharData *ch, int room_rnum) {
	if (!ch->IsNpc()
		&& !IS_IMMORTAL(ch)
		&& NORENTABLE(ch)
		&& ROOM_FLAGGED(room_rnum, ERoomFlag::kTunnel)) {
		return std::max(20, ch->get_real_max_hit() >> 3);
	}
	return 0;
}

/// \return true - чара может убить сразу при входе в ванрум
/// предполагается не пускать чара на верную смерть
bool deathtrap::check_tunnel_death(CharData *ch, int room_rnum) {
	const int dam = calc_tunnel_dmg(ch, room_rnum);
	if (dam > 0 && ch->get_hit() <= dam * 2) {
		return true;
	}
	return false;
}

/// дамаг чаров с бд в ван-румах раз в 2 секунды (SECS_PER_PLAYER_AFFECT)
/// и просто по факту входа (char_to_room), чтобы не так резво скакали
bool deathtrap::tunnel_damage(CharData *ch) {
	const int dam = calc_tunnel_dmg(ch, ch->in_room);
	if (dam > 0) {
		const int room_rnum = ch->in_room;
		const std::string name = ch->get_name_str();
		Damage dmg(SimpleDmg(kTypeTunnerldeath), dam, fight::kUndefDmg);
		dmg.flags.set(fight::kNoFleeDmg);

		if (dmg.Process(ch, ch) < 0) {
			char buf_[kMaxInputLength];
			snprintf(buf_, sizeof(buf_),
					 "Player %s died in tunnel room (room %d)",
					 name.c_str(), GET_ROOM_VNUM(room_rnum));
			mudlog(buf_, NRM, kLvlImmortal, SYSLOG, true);
			return true;
		}
	}
	return false;
}

bool deathtrap::CheckIceDeathTrap(RoomRnum room_rnum, CharData * /*ch*/) {
	if (room_rnum == kNowhere) {
		return false;
	}
	auto sector = world[room_rnum]->sector_type;
	if (sector != ESector::kWaterSwim && sector != ESector::kWaterNoswim) {
		return false;
	}
	if ((sector = real_sector(room_rnum)) != ESector::kThinIce && sector != ESector::kNormalIce) {
		return false;
	}

	int mass{0};
	for (const auto vict : world[room_rnum]->people) {
		if (!vict->IsNpc()
			&& !AFF_FLAGGED(vict, EAffect::kFly)) {
			mass += GET_WEIGHT(vict) + vict->GetCarryingWeight();
		}
	}

	if ((sector == ESector::kThinIce && mass > 500) || (sector == ESector::kNormalIce && mass > 1500)) {
		const auto first_in_room = world[room_rnum]->first_character();
		act("Лед проломился под вашей тяжестью.", false, first_in_room, nullptr, nullptr, kToRoom);
		act("Лед проломился под вашей тяжестью.", false, first_in_room, nullptr, nullptr, kToChar);
		auto room = world[room_rnum];
		room->weather.icelevel = 0;
		room->ices = 2;
		room->set_flag(ERoomFlag::kIceTrap);
		deathtrap::add(room);
		return true;
	}

	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
