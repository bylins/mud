// deathtrap.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "deathtrap.hpp"

#include "constants.h"
#include "db.h"
#include "spells.h"
#include "handler.h"
#include "house.h"
#include "char.hpp"
#include "corpse.hpp"
#include "room.hpp"
#include "fight.h"
#include "fight_stuff.hpp"
#include "logger.hpp"
#include "utils.h"
#include "conf.h"

#include <list>
#include <algorithm>

extern int has_boat(CHAR_DATA * ch);
extern void death_cry(CHAR_DATA * ch, CHAR_DATA * killer);
extern void reset_affects(CHAR_DATA *ch);

namespace DeathTrap
{

// список текущих слоу-дт в маде
std::list<ROOM_DATA*> room_list;

void log_death_trap(CHAR_DATA * ch);
void remove_items(CHAR_DATA *ch);

} // namespace DeathTrap

// * Инициализация списка при загрузке мада или редактирования комнат в олц
void DeathTrap::load()
{
	// на случай релоада, свапать смысла нету
	room_list.clear();

	for (int i = FIRST_ROOM; i <= top_of_world; ++i)
		if (ROOM_FLAGGED(i, ROOM_SLOWDEATH) || ROOM_FLAGGED(i, ROOM_ICEDEATH))
			room_list.push_back(world[i]);
}

/**
* Добавление новой комнаты с проверкой на присутствие
* \param room - комната, кот. добавляем
*/
void DeathTrap::add(ROOM_DATA* room)
{
	std::list<ROOM_DATA*>::const_iterator it = std::find(room_list.begin(), room_list.end(), room);
	if (it == room_list.end())
		room_list.push_back(room);
}

/**
* Удаление комнаты из списка слоу-дт
* \param room - комната, кот. удаляем
*/
void DeathTrap::remove(ROOM_DATA* room)
{
	room_list.remove(room);
}

/// Проверка активности дт, дергается каждые 2 секунды в хеарбите.
/// Доп список строится для случаев, когда в списке комнаты сначала идет чар,
/// а следом его чармисы и последовательный проход по ch->next_in_room
/// с пуржем чара натыкается далее на обнуленные структуры чармисов.
void DeathTrap::activity()
{
	for (auto it = room_list.cbegin(); it != room_list.cend(); ++it)
	{
		const auto people = (*it)->people; // make copy of people in the room
		for (const auto i : people)
		{
			if (i->purged() || IS_NPC(i))
			{
				continue;
			}
			std::string name = i->get_name_str();

			Damage dmg(SimpleDmg(TYPE_ROOMDEATH), MAX(1, GET_REAL_MAX_HIT(i) >> 2), FightSystem::UNDEF_DMG);
			dmg.flags.set(FightSystem::NO_FLEE);

			if (dmg.process(i, i) < 0)
			{
				char buf_[MAX_INPUT_LENGTH];
				snprintf(buf_, sizeof(buf_),
					"Player %s died in slow DT (room %d)",
					name.c_str(), (*it)->number);
				mudlog(buf_, LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
		}
	}
}

namespace OneWayPortal
{

// список односторонних порталов <куда указывает, откуда поставлен>
std::unordered_map<room_vnum /*to*/, ROOM_DATA* /*from*/> portal_list;

/**
* Добавление портала в список
* \param to_room - куда ставится пента
* \param from_room - откуда ставится
*/
void add(ROOM_DATA* to_room, ROOM_DATA* from_room)
{
	portal_list.emplace(to_room->number, from_room);
}

/**
* Удаление портала из списка
* \param to_room - куда указывает пента
*/
void remove(ROOM_DATA* to_room)
{
	const auto it = portal_list.find(to_room->number);
	if (it != portal_list.end())
		portal_list.erase(it);
}

/**
* Проверка на наличие комнаты в списке
* \param to_room - куда указывает пента
* \return указатель на источник пенты
*/
ROOM_DATA* get_from_room(ROOM_DATA* to_room)
{
	
	const auto it = portal_list.find(to_room->number);
	if (it != portal_list.end())
		return it->second;

	return nullptr;
}

} // namespace OneWayPortal

// * Логирование в отдельный файл уходов в дт для интересу и мб статистики.
void DeathTrap::log_death_trap(CHAR_DATA * ch)
{
	const char *filename = "../log/death_trap.log";
	static FILE *file = 0;
	if (!file)
	{
		file = fopen(filename, "a");
		if (!file)
		{
			log("SYSERR: can't open %s!", filename);
			return;
		}
		opened_files.push_back(file);
	}
	write_time(file);
	fprintf(file, "%s hit death trap #%d (%s)\n", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), world[ch->in_room]->name);
	fflush(file);
}

// * Попадание в обычное дт.
int DeathTrap::check_death_trap(CHAR_DATA * ch)
{
	if (ch->in_room != NOWHERE && !PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		if ((ROOM_FLAGGED(ch->in_room, ROOM_DEATH)
			&& !IS_IMMORTAL(ch))
			|| (real_sector(ch->in_room) == SECT_FLYING && !IS_NPC(ch)
				&& !IS_GOD(ch)
				&& !AFF_FLAGGED(ch, EAffectFlag::AFF_FLY))
			|| (real_sector(ch->in_room) == SECT_WATER_NOSWIM && !IS_NPC(ch)
				&& !IS_GOD(ch)
				&& !has_boat(ch)))
		{
			OBJ_DATA *corpse;
			DeathTrap::log_death_trap(ch);

			if (check_tester_death(ch, nullptr))
			{
				sprintf(buf1, "Player %s died in DT (room %d) but zone is under construction.", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room));
				mudlog(buf1, LGH, LVL_IMMORT, SYSLOG, TRUE);
				return FALSE;
			}

			sprintf(buf1, "Player %s died in DT (room %d)", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room));
			mudlog(buf1, LGH, LVL_IMMORT, SYSLOG, TRUE);
			death_cry(ch, NULL);
			//29.11.09 Для счета количество рипов (с) Василиса
			GET_RIP_DT(ch) = GET_RIP_DT(ch) + 1;
			GET_RIP_DTTHIS(ch) = GET_RIP_DTTHIS(ch) + 1;
			//конец правки (с) Василиса
			corpse = make_corpse(ch);
			if (corpse != NULL)
			{
				obj_from_room(corpse);	// для того, чтобы удалилость все содержимое
				extract_obj(corpse);
			}
			GET_HIT(ch) = GET_MOVE(ch) = 0;
			if (RENTABLE(ch))
			{
				die(ch, NULL);
			}
			else
				extract_char(ch, TRUE);
			return TRUE;
		}
	}
	return FALSE;
}

bool DeathTrap::is_slow_dt(int rnum)
{
	if (ROOM_FLAGGED(rnum, ROOM_SLOWDEATH) || ROOM_FLAGGED(rnum, ROOM_ICEDEATH))
		return true;
	return false;
}

/// Проверка чара на дамаг в ванруме, если он попадет в комнату room_rnum
/// \return если > 0, то величину дамага,
/// иначе - чара в tunnel_damage() не дамагнет
int calc_tunnel_dmg(CHAR_DATA *ch, int room_rnum)
{
	if (!IS_NPC(ch)
		&& !IS_IMMORTAL(ch)
		&& RENTABLE(ch)
		&& ROOM_FLAGGED(room_rnum, ROOM_TUNNEL))
	{
		return std::max(20, GET_REAL_MAX_HIT(ch) >> 3);
	}
	return 0;
}

/// \return true - чара может убить сразу при входе в ванрум
/// предполагается не пускать чара на верную смерть
bool DeathTrap::check_tunnel_death(CHAR_DATA *ch, int room_rnum)
{
	const int dam = calc_tunnel_dmg(ch, room_rnum);
	if (dam > 0 && GET_HIT(ch) <= dam * 2)
	{
		return true;
	}
	return false;
}

/// дамаг чаров с бд в ван-румах раз в 2 секунды (SECS_PER_PLAYER_AFFECT)
/// и просто по факту входа (char_to_room), чтобы не так резво скакали
bool DeathTrap::tunnel_damage(CHAR_DATA *ch)
{
	const int dam = calc_tunnel_dmg(ch, ch->in_room);
	if (dam > 0)
	{
		const int room_rnum = ch->in_room;
		const std::string name = ch->get_name_str();
		Damage dmg(SimpleDmg(TYPE_TUNNERLDEATH), dam, FightSystem::UNDEF_DMG);
		dmg.flags.set(FightSystem::NO_FLEE);

		if (dmg.process(ch, ch) < 0)
		{
			char buf_[MAX_INPUT_LENGTH];
			snprintf(buf_, sizeof(buf_),
				"Player %s died in tunnel room (room %d)",
				name.c_str(), GET_ROOM_VNUM(room_rnum));
			mudlog(buf_, NRM, LVL_IMMORT, SYSLOG, TRUE);
			return true;
		}
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
