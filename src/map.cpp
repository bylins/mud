// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "map.hpp"

#include "obj.hpp"
#include "screen.h"
#include "room.hpp"
#include "db.h"
#include "char_player.hpp"
#include "shop_ext.hpp"
#include "noob.hpp"
#include "char_obj_utils.inl"
#include "zone.table.hpp"
#include "logger.hpp"
#include "conf.h"

#include <boost/algorithm/string.hpp>
#include "boost/multi_array.hpp"
#include <boost/format.hpp>

#include <map>
#include <sstream>
#include <iomanip>
#include <vector>

int shop_ext(CHAR_DATA *ch, void *me, int cmd, char* argument);
int receptionist(CHAR_DATA *ch, void *me, int cmd, char* argument);
int postmaster(CHAR_DATA *ch, void *me, int cmd, char* argument);
int bank(CHAR_DATA *ch, void *me, int cmd, char* argument);
int exchange(CHAR_DATA *ch, void *me, int cmd, char* argument);
int horse_keeper(CHAR_DATA *ch, void *me, int cmd, char* argument);
int guild_mono(CHAR_DATA *ch, void *me, int cmd, char* argument);
int guild_poly(CHAR_DATA *ch, void *me, int cmd, char* argument);
int torc(CHAR_DATA *ch, void *me, int cmd, char* argument);
extern std::vector<City> cities;

namespace Noob
{
int outfit(CHAR_DATA *ch, void *me, int cmd, char* argument);
}
extern int has_boat(CHAR_DATA *ch);



namespace MapSystem
{


// размер поля для отрисовка
size_t MAX_LINES = 25;
size_t MAX_LENGTH = 50;
// глубина рекурсии по комнатам
int MAX_DEPTH_ROOMS = 5;

/*
#define MAX_LINES ch->map_check_option(MAP_MODE_BIG) ? 50 : 25
#define MAX_LENGHT ch->map_check_option(MAP_MODE_BIG) ? 100 : 50
#define MAX_DEPTH_ROOMS ch->map_check_option(MAP_MODE_BIG) ? 10 : 5
	*/

const int MAX_DEPTH_ROOM_STANDART = 5;
const size_t MAX_LINES_STANDART = MAX_DEPTH_ROOM_STANDART * 4 + 1;
const size_t MAX_LENGTH_STANDART = MAX_DEPTH_ROOM_STANDART * 8 + 1;

// Все тоже самое, но для увеличенной карты
const int MAX_DEPTH_ROOM_BIG = 10; 
const size_t MAX_LINES_BIG = MAX_DEPTH_ROOM_BIG * 4 + 1;
const size_t MAX_LENGTH_BIG = MAX_DEPTH_ROOM_BIG * 8 + 1;


// поле для отрисовки
//int screen[MAX_LINES][MAX_LENGHT];
boost::multi_array<int, 2> screen(boost::extents[MAX_LINES_BIG][MAX_LENGTH_BIG]);
// копия поля для хранения глубины текущей отрисовки по нужным координатам
// используется для случаев наезжания комнат друг на друга, в этом случае
// ближняя затирает более дальнюю и все остальные после нее
//int depths[MAX_LINES][MAX_LENGHT];
boost::multi_array<int, 2> depths(boost::extents[MAX_LINES_BIG][MAX_LENGTH_BIG]);

enum
{
	// свободный проход
	SCREEN_Y_OPEN,
	// закрытая дверь
	SCREEN_Y_DOOR,
	// скрытый проход (иммы)
	SCREEN_Y_HIDE,
	// нет прохода
	SCREEN_Y_WALL,
	SCREEN_X_OPEN,
	SCREEN_X_DOOR,
	SCREEN_X_HIDE,
	SCREEN_X_WALL,
	SCREEN_UP_OPEN,
	SCREEN_UP_DOOR,
	SCREEN_UP_HIDE,
	SCREEN_UP_WALL,
	SCREEN_DOWN_OPEN,
	SCREEN_DOWN_DOOR,
	SCREEN_DOWN_HIDE,
	SCREEN_DOWN_WALL,
	SCREEN_Y_UP_OPEN,
	SCREEN_Y_UP_DOOR,
	SCREEN_Y_UP_HIDE,
	SCREEN_Y_UP_WALL,
	SCREEN_Y_DOWN_OPEN,
	SCREEN_Y_DOWN_DOOR,
	SCREEN_Y_DOWN_HIDE,
	SCREEN_Y_DOWN_WALL,
	// текущая клетка персонажа
	SCREEN_CHAR,
	// переход в другую зону
	SCREEN_NEW_ZONE,
	// мирная комната
	SCREEN_PEACE,
	// дт (иммы + 0 морт)
	SCREEN_DEATH_TRAP,
	// чтобы пробелы не втыкать
	SCREEN_EMPTY,
	// в комнате темно, существ не пишем
	SCREEN_MOB_UNDEF,
	// кол-во существ в комнате
	SCREEN_MOB_1,
	SCREEN_MOB_2,
	SCREEN_MOB_3,
	SCREEN_MOB_4,
	SCREEN_MOB_5,
	SCREEN_MOB_6,
	SCREEN_MOB_7,
	SCREEN_MOB_8,
	SCREEN_MOB_9,
	// в комнате больше 9 существ
	SCREEN_MOB_OVERFLOW,
	// тоже с предметами
	SCREEN_OBJ_UNDEF,
	SCREEN_OBJ_1,
	SCREEN_OBJ_2,
	SCREEN_OBJ_3,
	SCREEN_OBJ_4,
	SCREEN_OBJ_5,
	SCREEN_OBJ_6,
	SCREEN_OBJ_7,
	SCREEN_OBJ_8,
	SCREEN_OBJ_9,
	SCREEN_OBJ_OVERFLOW,
	// мобы со спешиалами
	SCREEN_MOB_SPEC_SHOP,
	SCREEN_MOB_SPEC_RENT,
	SCREEN_MOB_SPEC_MAIL,
	SCREEN_MOB_SPEC_BANK,
	SCREEN_MOB_SPEC_HORSE,
	SCREEN_MOB_SPEC_TEACH,
	SCREEN_MOB_SPEC_EXCH,
	SCREEN_MOB_SPEC_TORC,
	// клетка в которой можно утонуть
	SCREEN_WATER,
	// можно умереть без полета
	SCREEN_FLYING,
	// кладовщики для показа нубам
	SCREEN_MOB_SPEC_OUTFIT,
	// SCREEN_WATER красным, если персонаж без дышки
	SCREEN_WATER_RED,
	// SCREEN_FLYING красным, если персонаж без полета
	SCREEN_FLYING_RED,
	// всегда в конце
	SCREEN_TOTAL
};

const char *signs[] =
{
	// SCREEN_Y
	"&K - &n",
	"&C-=-&n",
	"&R---&n",
	"&G---&n",
	// SCREEN_X
	"&K:&n",
	"&C/&n",
	"&R|&n",
	"&G|&n",
	// SCREEN_UP
	"&K^&n",
	"&C^&n",
	"&R^&n",
	"",
	// SCREEN_DOWN
	"&Kv&n",
	"&Cv&n",
	"&Rv&n",
	"",
	// SCREEN_Y_UP
	"&K -&n",
	"&C-=&n",
	"&R--&n",
	"&G--&n",
	// SCREEN_Y_DOWN
	"&K- &n",
	"&C=-&n",
	"&R--&n",
	"&G--&n",
	// OTHERS
	"&c@&n",
	"&C>&n",
	"&K~&n",
	"&RЖ&n",
	"",
	"&K?&n",
	"&r1&n",
	"&r2&n",
	"&r3&n",
	"&r4&n",
	"&r5&n",
	"&r6&n",
	"&r7&n",
	"&r8&n",
	"&r9&n",
	"&R!&n",
	"&K?&n",
	"&y1&n",
	"&y2&n",
	"&y3&n",
	"&y4&n",
	"&y5&n",
	"&y6&n",
	"&y7&n",
	"&y8&n",
	"&y9&n",
	"&Y!&n",
	"&W$&n",
	"&WR&n",
	"&WM&n",
	"&WB&n",
	"&WH&n",
	"&WT&n",
	"&WE&n",
	"&WG&n",
	"&C,&n",
	"&C`&n",
	"&WO&n",
	"&R,&n",
	"&R`&n"
};

std::map<int /* room vnum */, int /* min depth */> check_dupe;

// отрисовка символа на поле по координатам
void put_on_screen(unsigned y, unsigned x, int num, int depth)
{
	if (y >= MAX_LINES || x >= MAX_LENGTH)
	{
		log("SYSERROR: %d;%d (%s %s %d)", y, x, __FILE__, __func__, __LINE__);
		return;
	}
	if (depths[y][x] == -1)
	{
		// поле было чистое
		screen[y][x] = num;
		depths[y][x] = depth;
	}
	else if (depths[y][x] > depth)
	{
		// уже что-то было отрисовано
		if (screen[y][x] == num)
		{
			// если тот же самый символ,
			// то надо обновить глубину на случай последующих затираний
			depths[y][x] = depth;
		}
		else
		{
			// другой символ и меньшая глубина
			// затираем все символы этой и далее глубины
			// и поверх рисуем текущий символ
			const int hide_num = depths[y][x];
			for (unsigned i = 0; i < MAX_LINES; ++i)
			{
				for (unsigned k = 0; k < MAX_LENGTH; ++k)
				{
					if (depths[i][k] >= hide_num)
					{
						screen[i][k] = -1;
						depths[i][k] = -1;
					}
				}
			}
			screen[y][x] = num;
			depths[y][x] = depth;
		}
	}
	else if ((screen[y][x] >= SCREEN_UP_OPEN && screen[y][x] <= SCREEN_UP_WALL)
			|| (screen[y][x] >= SCREEN_DOWN_OPEN && screen[y][x] <= SCREEN_DOWN_WALL))
	{
		// выходы ^ и v затираются, если есть чем
		screen[y][x] = num;
		depths[y][x] = depth;
	}
}

// важные знаки в центре клеток, которые надо рисовать для выходов вверх/вниз
// затирают символы выходов ^ и v, но не затирают друг друга, т.е. что
// первое отрисовалось, то и остается, поэтому идут по важности
void check_position_and_put_on_screen(int next_y, int next_x, int sign_num, int depth, int exit_num)
{
	if (exit_num == UP)
	{
		switch(sign_num)
		{
		case SCREEN_DEATH_TRAP:
		case SCREEN_WATER:
		case SCREEN_FLYING:
		case SCREEN_WATER_RED:
		case SCREEN_FLYING_RED:
			put_on_screen(next_y - 1, next_x + 1, sign_num, depth);
			return;
		}
	}
	else if (exit_num == DOWN)
	{
		switch(sign_num)
		{
		case SCREEN_DEATH_TRAP:
		case SCREEN_WATER:
		case SCREEN_FLYING:
		case SCREEN_WATER_RED:
		case SCREEN_FLYING_RED:
			put_on_screen(next_y + 1, next_x - 1, sign_num, depth);
			return;
		}
	}
	else
	{
		put_on_screen(next_y, next_x, sign_num, depth);
	}
}

void draw_mobs(const CHAR_DATA *ch, int room_rnum, int next_y, int next_x)
{
	if (IS_DARK(room_rnum) && !IS_IMMORTAL(ch))
	{
		put_on_screen(next_y, next_x - 1, SCREEN_MOB_UNDEF, 1);
	}
	else
	{
		int cnt = 0;
		for (const auto tch : world[room_rnum]->people)
		{
			if (tch == ch)
			{
				continue;
			}
			if (IS_NPC(tch) && !ch->map_check_option(MAP_MODE_MOBS))
			{
				continue;
			}
			if (!IS_NPC(tch) && !ch->map_check_option(MAP_MODE_PLAYERS))
			{
				continue;
			}
			if (HERE(tch)
				&& (CAN_SEE(ch, tch)
					|| awaking(tch, AW_HIDE | AW_INVIS | AW_CAMOUFLAGE)))
			{
				++cnt;
			}
		}

		if (cnt > 0 && cnt <= 9)
		{
			put_on_screen(next_y, next_x - 1, SCREEN_MOB_UNDEF + cnt, 1);
		}
		else if (cnt > 9)
		{
			put_on_screen(next_y, next_x - 1, SCREEN_MOB_OVERFLOW, 1);
		}
	}
}

void draw_objs(const CHAR_DATA *ch, int room_rnum, int next_y, int next_x)
{
	if (IS_DARK(room_rnum) && !IS_IMMORTAL(ch))
	{
		put_on_screen(next_y, next_x + 1, SCREEN_OBJ_UNDEF, 1);
	}
	else
	{
		int cnt = 0;

		for (OBJ_DATA *obj = world[room_rnum]->contents; obj; obj = obj->get_next_content())
		{
			if (IS_CORPSE(obj) && GET_OBJ_VAL(obj, 2) >= 0
				&& !ch->map_check_option(MAP_MODE_MOBS_CORPSES))
			{
				continue;
			}
			if (IS_CORPSE(obj) && GET_OBJ_VAL(obj, 2) < 0
				&& !ch->map_check_option(MAP_MODE_PLAYER_CORPSES))
			{
				continue;
			}
			if (!ch->map_check_option(MAP_MODE_INGREDIENTS)
				&& (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_INGREDIENT
					|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_MING))
			{
				continue;
			}
			if (!IS_CORPSE(obj)
				&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_INGREDIENT
				&& GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_MING
				&& !ch->map_check_option(MAP_MODE_OTHER_OBJECTS))
			{
				continue;
			}
			if (CAN_SEE_OBJ(ch, obj))
			{
				++cnt;
			}
		}
		if (cnt > 0 && cnt <= 9)
		{
			put_on_screen(next_y, next_x + 1, SCREEN_OBJ_UNDEF + cnt, 1);
		}
		else if (cnt > 9)
		{
			put_on_screen(next_y, next_x + 1, SCREEN_OBJ_OVERFLOW, 1);
		}
	}
}

void draw_spec_mobs(const CHAR_DATA *ch, int room_rnum, int next_y, int next_x, int cur_depth)
{
	bool all = ch->map_check_option(MAP_MODE_MOB_SPEC_ALL) ? true : false;

	for (const auto tch : world[room_rnum]->people)
	{
		auto func = GET_MOB_SPEC(tch);
		if (func)
		{
			if (func == shop_ext
				&& (all || ch->map_check_option(MAP_MODE_MOB_SPEC_SHOP)))
			{
				put_on_screen(next_y, next_x, SCREEN_MOB_SPEC_SHOP, cur_depth);
			}
			else if (func == receptionist
				&& (all || ch->map_check_option(MAP_MODE_MOB_SPEC_RENT)))
			{
				put_on_screen(next_y, next_x, SCREEN_MOB_SPEC_RENT, cur_depth);
			}
			else if (func == postmaster
				&& (all || ch->map_check_option(MAP_MODE_MOB_SPEC_MAIL)))
			{
				put_on_screen(next_y, next_x, SCREEN_MOB_SPEC_MAIL, cur_depth);
			}
			else if (func == bank
				&& (all || ch->map_check_option(MAP_MODE_MOB_SPEC_BANK)))
			{
				put_on_screen(next_y, next_x, SCREEN_MOB_SPEC_BANK, cur_depth);
			}
			else if (func == exchange
				&& (all || ch->map_check_option(MAP_MODE_MOB_SPEC_EXCH)))
			{
				put_on_screen(next_y, next_x, SCREEN_MOB_SPEC_EXCH, cur_depth);
			}
			else if (func == horse_keeper
				&& (all || ch->map_check_option(MAP_MODE_MOB_SPEC_HORSE)))
			{
				put_on_screen(next_y, next_x, SCREEN_MOB_SPEC_HORSE, cur_depth);
			}
			else if ((func == guild_mono || func == guild_poly)
				&& (all || ch->map_check_option(MAP_MODE_MOB_SPEC_TEACH)))
			{
				put_on_screen(next_y, next_x, SCREEN_MOB_SPEC_TEACH, cur_depth);
			}
			else if (func == torc
				&& (all || ch->map_check_option(MAP_MODE_MOB_SPEC_TORC)))
			{
				put_on_screen(next_y, next_x, SCREEN_MOB_SPEC_TORC, cur_depth);
			}
			else if (func == Noob::outfit && (Noob::is_noob(ch)))
			{
				put_on_screen(next_y, next_x, SCREEN_MOB_SPEC_OUTFIT, cur_depth);
			}
		}
	}
}

bool mode_allow(const CHAR_DATA *ch, int cur_depth)
{
	if (ch->map_check_option(MAP_MODE_1_DEPTH)
		&& !ch->map_check_option(MAP_MODE_2_DEPTH)
		&& cur_depth > 1)
	{
		return false;
	}

	if (ch->map_check_option(MAP_MODE_2_DEPTH) && cur_depth > 2)
	{
		return false;
	}

	return true;
}

void draw_room(CHAR_DATA *ch, const ROOM_DATA *room, int cur_depth, int y, int x)
{
	// чтобы не ходить по комнатам вторично, но с проверкой на глубину
	std::map<int, int>::iterator i = check_dupe.find(room->number);
	if (i != check_dupe.end())
	{
		if (i->second <= cur_depth)
		{
			return;
		}
		else
		{
			i->second = cur_depth;
		}
	}
	else
	{
		check_dupe.insert(std::make_pair(room->number, cur_depth));
	}

	if (world[ch->in_room] == room)
	{
		put_on_screen(y, x, SCREEN_CHAR, cur_depth);
		if (ch->map_check_option(MAP_MODE_MOBS_CURR_ROOM))
		{
			draw_mobs(ch, ch->in_room, y, x);
		}
		if (ch->map_check_option(MAP_MODE_OBJS_CURR_ROOM))
		{
			draw_objs(ch, ch->in_room, y, x);
		}
	}
	else if (room->get_flag(ROOM_PEACEFUL))
	{
		put_on_screen(y, x, SCREEN_PEACE, cur_depth);
	}

	for (int i = 0; i < NUM_OF_DIRS; ++i)
	{
		int cur_y = y, cur_x = x, cur_sign = -1, next_y = y, next_x = x;
		switch(i)
		{
		case NORTH:
			cur_y -= 1;
			next_y -= 2;
			cur_sign = SCREEN_Y_OPEN;
			break;
		case EAST:
			cur_x += 2;
			next_x += 4;
			cur_sign = SCREEN_X_OPEN;
			break;
		case SOUTH:
			cur_y += 1;
			next_y += 2;
			cur_sign = SCREEN_Y_OPEN;
			break;
		case WEST:
			cur_x -= 2;
			next_x -= 4;
			cur_sign = SCREEN_X_OPEN;
			break;
		case UP:
			cur_y -= 1;
			cur_x += 1;
			cur_sign = SCREEN_UP_OPEN;
			break;
		case DOWN:
			cur_y += 1;
			cur_x -= 1;
			cur_sign = SCREEN_DOWN_OPEN;
			break;
		default:
			log("SYSERROR: i=%d (%s %s %d)", i, __FILE__, __func__, __LINE__);
			return;
		}

		if (room->dir_option[i]
			&& room->dir_option[i]->to_room() != NOWHERE
			&& (!EXIT_FLAGGED(room->dir_option[i], EX_HIDDEN) || IS_IMMORTAL(ch)))
		{
			// отрисовка выхода
			if (EXIT_FLAGGED(room->dir_option[i], EX_CLOSED))
			{
				put_on_screen(cur_y, cur_x, cur_sign + 1, cur_depth);
			}
			else if (EXIT_FLAGGED(room->dir_option[i], EX_HIDDEN))
			{
				put_on_screen(cur_y, cur_x, cur_sign + 2, cur_depth);
			}
			else
			{
				put_on_screen(cur_y, cur_x, cur_sign, cur_depth);
			}
			// за двери закрытые смотрят только иммы
			if (EXIT_FLAGGED(room->dir_option[i], EX_CLOSED) && !IS_IMMORTAL(ch))
			{
				continue;
			}
			// здесь важна очередность, что первое отрисовалось - то и будет
			const ROOM_DATA *next_room = world[room->dir_option[i]->to_room()];
			bool view_dt = false;
			for (const auto aff : ch->affected)
			{
				if (aff->location == APPLY_VIEW_DT) // скушал свиток с эксп бонусом
				{
					view_dt = true;
				}
			}
			// дт иммам и нубам с 0 мортов
			if (next_room->get_flag(ROOM_DEATH)
				&& (GET_REMORT(ch) <= 5
					|| view_dt || IS_IMMORTAL(ch)))
			{
				check_position_and_put_on_screen(next_y, next_x, SCREEN_DEATH_TRAP, cur_depth, i);
			}
			// можно утонуть
			if (next_room->sector_type == SECT_WATER_NOSWIM)
			{
				if (!has_boat(ch))
				{
					check_position_and_put_on_screen(next_y, next_x, SCREEN_WATER_RED, cur_depth, i);
				}
				else
				{
					check_position_and_put_on_screen(next_y, next_x, SCREEN_WATER, cur_depth, i);
				}
			}
			// можно задохнуться
			if (next_room->sector_type == SECT_UNDERWATER)
			{
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_WATERBREATH))
				{
					check_position_and_put_on_screen(next_y, next_x, SCREEN_WATER_RED, cur_depth, i);
				}
				else
				{
					check_position_and_put_on_screen(next_y, next_x, SCREEN_WATER, cur_depth, i);
				}
			}
			// Флай-дт
			if (next_room->sector_type == SECT_FLYING)
			{
				if (!AFF_FLAGGED(ch, EAffectFlag::AFF_FLY))
				{
					check_position_and_put_on_screen(next_y, next_x, SCREEN_FLYING_RED, cur_depth, i);
				}
				else
				{
					check_position_and_put_on_screen(next_y, next_x, SCREEN_FLYING, cur_depth, i);
				}
			}
			// знаки в центре клетки, не рисующиеся для выходов вверх/вниз
			if (i != UP && i != DOWN)
			{
				// переход в другую зону
				if (next_room->zone != world[ch->in_room]->zone)
				{
					put_on_screen(next_y, next_x, SCREEN_NEW_ZONE, cur_depth);
				}
				// моб со спешиалом
				draw_spec_mobs(ch, room->dir_option[i]->to_room(), next_y, next_x, cur_depth);
			}
			// существа
			if (cur_depth == 1
				&& (!EXIT_FLAGGED(room->dir_option[i], EX_CLOSED) || IS_IMMORTAL(ch))
				&& (ch->map_check_option(MAP_MODE_MOBS) || ch->map_check_option(MAP_MODE_PLAYERS)))
			{
				// в случае вверх/вниз next_y/x = y/x, рисуется относительно
				// координат чара, со смещением, чтобы писать около полей v и ^
				// внутри draw_mobs происходит еще одно смещение на x-1
				if (cur_sign == SCREEN_UP_OPEN)
				{
					draw_mobs(ch, room->dir_option[i]->to_room(), next_y - 1, next_x + 3);
				}
				else if (cur_sign == SCREEN_DOWN_OPEN)
				{
					draw_mobs(ch, room->dir_option[i]->to_room(), next_y + 1, next_x - 1);
				}
				else
				{
					// остальные выходы вокруг пишутся как обычно, со смещением
					// относительно центра следующей за выходом клетки
					draw_mobs(ch, room->dir_option[i]->to_room(), next_y, next_x);
				}
			}
			// предметы
			if (cur_depth == 1
				&& (!EXIT_FLAGGED(room->dir_option[i], EX_CLOSED) || IS_IMMORTAL(ch))
				&& (ch->map_check_option(MAP_MODE_MOBS_CORPSES)
					|| ch->map_check_option(MAP_MODE_PLAYER_CORPSES)
					|| ch->map_check_option(MAP_MODE_INGREDIENTS)
					|| ch->map_check_option(MAP_MODE_OTHER_OBJECTS)))
			{
				if (cur_sign == SCREEN_UP_OPEN)
				{
					draw_objs(ch, room->dir_option[i]->to_room(), next_y - 1, next_x);
				}
				else if (cur_sign == SCREEN_DOWN_OPEN)
				{
					draw_objs(ch, room->dir_option[i]->to_room(), next_y + 1, next_x - 2);
				}
				else
				{
					draw_objs(ch, room->dir_option[i]->to_room(), next_y, next_x);
				}
			}
			// проход по следующей в глубину комнате
			if (i != UP && i != DOWN
				&& cur_depth < MAX_DEPTH_ROOMS
				&& (!EXIT_FLAGGED(room->dir_option[i], EX_CLOSED) || IS_IMMORTAL(ch))
				&& next_room->zone == world[ch->in_room]->zone
				&& mode_allow(ch, cur_depth))
			{
				draw_room(ch, next_room, cur_depth + 1, next_y, next_x);
			}
		}
		else
		{
			put_on_screen(cur_y, cur_x, cur_sign + 3, cur_depth);
		}
	}
}

// imm по дефолту = 0, если нет, то распечатанная карта засылается ему
void print_map(CHAR_DATA *ch, CHAR_DATA *imm)
{
	MAX_LINES = MAX_LINES_STANDART;
	MAX_LENGTH = MAX_LENGTH_STANDART;
	MAX_DEPTH_ROOMS = MAX_DEPTH_ROOM_STANDART;
	if (ch->map_check_option(MAP_MODE_BIG))
	{
		for (unsigned int i = 0; i < cities.size(); i++)
		{
			if (zone_table[world[ch->in_room]->zone].number == cities[i].rent_vnum / 100)
			{
				MAX_LINES = MAX_LINES_BIG;
				MAX_LENGTH = MAX_LENGTH_BIG;
				MAX_DEPTH_ROOMS = MAX_DEPTH_ROOM_BIG;
				break;
			}
		}
	}
	for (unsigned i = 0; i < MAX_LINES; ++i)
	{
		for (unsigned k = 0; k < MAX_LENGTH; ++k)
		{
			screen[i][k] = -1;
			depths[i][k] = -1;
		}
	}
	check_dupe.clear();

	draw_room(ch, world[ch->in_room], 1, static_cast<int>(MAX_LINES/2), static_cast<int>(MAX_LENGTH/2));

	int start_line = -1, end_line = static_cast<int>(MAX_LINES), char_line = -1;
	// для облегчения кода - делаем проход по экрану
	// для расставления Y символов в зависимости от наличия
	// или отсутствия выходов вверх/вниз
	// заодно убираем пустые строки экрана, чтобы не делать
	// потом аллокации на каждую печатаемую строку
	for (unsigned i = 0; i < MAX_LINES; ++i)
	{
		bool found = false;

		for (unsigned k = 0; k < MAX_LENGTH; ++k)
		{
			if (screen[i][k] > -1 && screen[i][k] < SCREEN_TOTAL)
			{
				found = true;

				if (screen[i][k] == SCREEN_CHAR)
				{
					char_line = i;
				}

				if (screen[i][k] >= SCREEN_Y_OPEN
					&& screen[i][k] <= SCREEN_Y_WALL
					&& k + 1 < MAX_LENGTH && k >= 1)
				{
					if (screen[i][k + 1] > -1
						&& screen[i][k + 1] != SCREEN_UP_WALL)
					{
						screen[i][k - 1] = screen[i][k] + SCREEN_Y_UP_OPEN;
						screen[i][k] = SCREEN_EMPTY;
					}
					else if (screen[i][k - 1] > -1
						&& screen[i][k - 1] != SCREEN_DOWN_WALL)
					{
						screen[i][k] += SCREEN_Y_DOWN_OPEN;
						screen[i][k + 1] = SCREEN_EMPTY;
					}
					else
					{
						screen[i][k - 1] = screen[i][k];
						screen[i][k] = SCREEN_EMPTY;
						screen[i][k + 1] = SCREEN_EMPTY;
					}
				}
				else if (screen[i][k] >= SCREEN_Y_OPEN
					&& screen[i][k] <= SCREEN_Y_WALL)
				{
					screen[i][k] = SCREEN_EMPTY;
					send_to_char("Ошибка при генерации карты (1), сообщите богам!\r\n", ch);
				}
			}
		}

		if (found && start_line < 0)
		{
			start_line = i;
		}
		else if (!found && start_line > 0)
		{
			end_line = i;
			break;
		}
	}

	if (start_line == -1 || char_line == -1)
	{
		log("assert print_map start_line=%d, char_line=%d", start_line, char_line);
		return;
	}

	std::string out;
	out += "\r\n";

	bool fixed_1 = false;
	bool fixed_2 = false;

	if (ch->map_check_option(MAP_MODE_DEPTH_FIXED))
	{
		if (ch->map_check_option(MAP_MODE_1_DEPTH))
		{
			fixed_1 = true;
		}
		if (ch->map_check_option(MAP_MODE_2_DEPTH))
		{
			fixed_1 = false;
			fixed_2 = true;
		}
	}

	if (fixed_1 || fixed_2)
	{
		const int need_top = fixed_2 ? 5 : 3;
		int top_lines = char_line - start_line;
		if (top_lines < need_top)
		{
			for (int i = 1; i <= need_top - top_lines; ++i)
			{
				out += ": \r\n";
			}
		}
	}

	for (int i = start_line; i < end_line; ++i)
	{
		out += ": ";
		//if (ch->map_check_option(MAP_MODE_BIG))
		//	k = 10;
		for (unsigned k = 0; k < MAX_LENGTH; ++k)
		{
			if (screen[i][k] <= -1)
			{
				out += " ";
			}
			else if (screen[i][k] < SCREEN_TOTAL
				&& screen[i][k] != SCREEN_EMPTY)
			{
				out += signs[screen[i][k]];
			}
		}
		out += "\r\n";
	}

	if (fixed_1 || fixed_2)
	{
		const int need_bot = fixed_2 ? 6 : 4;
		int bot_lines = end_line - char_line;
		if (bot_lines < need_bot)
		{
			for (int i = 1; i <= need_bot - bot_lines; ++i)
			{
				out += ": \r\n";
			}
		}
	}

	out += "\r\n";

	if (imm)
	{
		send_to_char(out, imm);
	}
	else
	{
		send_to_char(out, ch);
	}
}

void Options::olc_menu(CHAR_DATA *ch)
{
	std::stringstream out;
	out << "Доступно для отображения на карте:\r\n";
	boost::format menu1("%s%2d%s) %s %-30s");
	boost::format menu2("   %s%2d%s) %s %-30s\r\n");

	int cnt = 0;
	for (int i = 0; i < TOTAL_MAP_OPTIONS; ++i)
	{
		switch (i)
		{
		case MAP_MODE_MOBS:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOBS] ? "[x]" : "[ ]")
				% "существа кроме игроков (мобы)";
			break;
		case MAP_MODE_PLAYERS:
			out << menu2 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_PLAYERS] ? "[x]" : "[ ]")
				% "другие игроки";
			break;
		case MAP_MODE_MOBS_CORPSES:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOBS_CORPSES] ? "[x]" : "[ ]")
				% "трупы существ (кроме игроков)";
			break;
		case MAP_MODE_PLAYER_CORPSES:
			out << menu2 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_PLAYER_CORPSES] ? "[x]" : "[ ]")
				% "трупы игроков";
			break;
		case MAP_MODE_INGREDIENTS:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_INGREDIENTS] ? "[x]" : "[ ]")
				% "ингредиенты";
			break;
		case MAP_MODE_OTHER_OBJECTS:
			out << menu2 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_OTHER_OBJECTS] ? "[x]" : "[ ]")
				% "другие предметы\r\n";
			break;
		case MAP_MODE_1_DEPTH:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_1_DEPTH] ? "[x]" : "[ ]")
				% "только прилегающие комнаты";
			break;
		case MAP_MODE_2_DEPTH:
			out << menu2 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_2_DEPTH] ? "[x]" : "[ ]")
				% "прилегающие комнаты + 1";
			break;
		case MAP_MODE_DEPTH_FIXED:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_DEPTH_FIXED] ? "[x]" : "[ ]")
				% "фиксировать высоту карты прилегающих комнат\r\n\r\n";
			break;
		case MAP_MODE_MOB_SPEC_SHOP:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOB_SPEC_SHOP] ? "[x]" : "[ ]")
				% "продавцы (магазины, $)";
			break;
		case MAP_MODE_MOB_SPEC_RENT:
			out << menu2 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOB_SPEC_RENT] ? "[x]" : "[ ]")
				% "рентеры (постой, R)";
			break;
		case MAP_MODE_MOB_SPEC_MAIL:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOB_SPEC_MAIL] ? "[x]" : "[ ]")
				% "ямщики (почта, M)";
			break;
		case MAP_MODE_MOB_SPEC_BANK:
			out << menu2 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOB_SPEC_BANK] ? "[x]" : "[ ]")
				% "банкиры (лежня, B)";
			break;
		case MAP_MODE_MOB_SPEC_EXCH:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOB_SPEC_EXCH] ? "[x]" : "[ ]")
				% "зазывалы (базар, E)";
			break;
		case MAP_MODE_MOB_SPEC_HORSE:
			out << menu2 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOB_SPEC_HORSE] ? "[x]" : "[ ]")
				% "конюхи (конюшня, H)";
			break;
		case MAP_MODE_MOB_SPEC_TEACH:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOB_SPEC_TEACH] ? "[x]" : "[ ]")
				% "учителя (любые, T)";
			break;
		case MAP_MODE_MOB_SPEC_TORC:
			out << menu2 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOB_SPEC_TORC] ? "[x]" : "[ ]")
				% "глашатаи (гривны, G)";
			break;
		case MAP_MODE_MOB_SPEC_ALL:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOB_SPEC_ALL] ? "[x]" : "[ ]")
				% "все мобы со спец. функциями\r\n\r\n";
			break;
		case MAP_MODE_MOBS_CURR_ROOM:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_MOBS_CURR_ROOM] ? "[x]" : "[ ]")
				% "существа (п. 1-2) в комнате с персонажем\r\n";
			break;
		case MAP_MODE_OBJS_CURR_ROOM:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_OBJS_CURR_ROOM] ? "[x]" : "[ ]")
				% "объекты (п. 3-6) в комнате с персонажем\r\n";
			break;
		case MAP_MODE_BIG:
			out << menu1 % CCGRN(ch, C_NRM) % ++cnt % CCNRM(ch, C_NRM)
				% (bit_list_[MAP_MODE_BIG] ? "[x]" : "[ ]")
				% "увеличенный размер карты\r\n\r\n";
			break;
		}
	}

	out << CCGRN(ch, C_NRM) << std::setw(2) << ++cnt << CCNRM(ch, C_NRM)
		<< ") включить все\r\n";
	out << CCGRN(ch, C_NRM) << std::setw(2) << ++cnt << CCNRM(ch, C_NRM)
		<< ") выключить все\r\n\r\n";

	out << CCGRN(ch, C_NRM) << std::setw(2) << ++cnt << CCNRM(ch, C_NRM)
		<< ") Выйти без сохранения\r\n";
	out << CCGRN(ch, C_NRM) << std::setw(2) << ++cnt << CCNRM(ch, C_NRM)
		<< ") Сохранить и выйти\r\n"
		<< "Ваш выбор:";

	send_to_char(out.str(), ch);
}

void Options::parse_menu(CHAR_DATA *ch, const char *arg)
{
	if (!*arg)
	{
		send_to_char("Неверный выбор!\r\n", ch);
		olc_menu(ch);
		return;
	}

	int num = atoi(arg);
	--num;

	if (num >= 0 && num < TOTAL_MAP_OPTIONS)
	{
		bit_list_.flip(num);
		olc_menu(ch);
	}
	else if (num == TOTAL_MAP_OPTIONS)
	{
		bit_list_.reset();
		bit_list_.flip();
		bit_list_.reset(MAP_MODE_1_DEPTH);
		bit_list_.reset(MAP_MODE_2_DEPTH);
		bit_list_.reset(MAP_MODE_DEPTH_FIXED);
		olc_menu(ch);
	}
	else if (num == TOTAL_MAP_OPTIONS + 1)
	{
		bit_list_.reset();
		olc_menu(ch);
	}
	else if (num == TOTAL_MAP_OPTIONS + 2)
	{
		ch->desc->map_options.reset();
		STATE(ch->desc) = CON_PLAYING;
		send_to_char("Редактирование отменено.\r\n", ch);
		return;
	}
	else if (num == TOTAL_MAP_OPTIONS + 3)
	{
		ch->map_olc_save();
		ch->desc->map_options.reset();
		STATE(ch->desc) = CON_PLAYING;
		send_to_char("Изменения сохранены.\r\n", ch);
		return;
	}
	else
	{
		send_to_char("Неверный выбор!\r\n", ch);
		olc_menu(ch);
		return;
	}
}

const char *message =
"Доступные опции:\r\n"
"   &Wкарта&n - показ карты с текущими настройками\r\n"
"   &Wкарта редактировать|опции|меню&n - управление отображаемой на карте информацией (интерактивное меню)\r\n"
"   &Wкарта включить|выключить&n\r\n"
"      все, мобы, игроки, трупы мобов, трупы игроков, ингредиенты, другие предметы, комнаты 1, комнаты 2\r\n"
"      фиксировать высоту, магазин, рента, почта, банк, базар, лошадь, учитель, глашатай, все специальные\r\n"
"      мобы в комнате, предметы в комнате\r\n"
"      - управляет опциями карты при показе по команде или при активном режиме\r\n"
"   &Wкарта показать (все теже опции из предыдущего пункта) &n\r\n"
"      - однократно показывает карту с выбранными опциями, не изменяя настроек\r\n"
"   &Wкарта справка|помощь&n - вывод данной справки\r\n";

bool parse_text_olc(CHAR_DATA *ch, const std::string &str, std::bitset<TOTAL_MAP_OPTIONS> &bits, bool flag)
{
	std::vector<std::string> str_list;
	boost::split(str_list, str, boost::is_any_of(","), boost::token_compress_on);
	bool error = false;

	for (std::vector<std::string>::const_iterator k = str_list.begin(),
		kend = str_list.end(); k != kend; ++k)
	{
		if (isname(*k, "все"))
		{
			bits.reset();
			if (flag)
			{
				bits.flip();
				bits.reset(MAP_MODE_1_DEPTH);
				bits.reset(MAP_MODE_2_DEPTH);
				bits.reset(MAP_MODE_DEPTH_FIXED);
			}
			return error;
		}
		if (isname(*k, "мобы"))
		{
			bits[MAP_MODE_MOBS] = flag;
		}
		else if (isname(*k, "игроки"))
		{
			bits[MAP_MODE_PLAYERS] = flag;
		}
		else if (isname(*k, "трупы мобов"))
		{
			bits[MAP_MODE_MOBS_CORPSES] = flag;
		}
		else if (isname(*k, "трупы игроков"))
		{
			bits[MAP_MODE_PLAYER_CORPSES] = flag;
		}
		else if (isname(*k, "ингредиенты"))
		{
			bits[MAP_MODE_INGREDIENTS] = flag;
		}
		else if (isname(*k, "другие предметы"))
		{
			bits[MAP_MODE_OTHER_OBJECTS] = flag;
		}
		else if (isname(*k, "комнаты 1"))
		{
			bits[MAP_MODE_1_DEPTH] = flag;
		}
		else if (isname(*k, "комнаты 2"))
		{
			bits[MAP_MODE_2_DEPTH] = flag;
		}
		else if (isname(*k, "фиксировать высоту"))
		{
			bits[MAP_MODE_DEPTH_FIXED] = flag;
		}
		else if (isname(*k, "магазин"))
		{
			bits[MAP_MODE_MOB_SPEC_SHOP] = flag;
		}
		else if (isname(*k, "рента"))
		{
			bits[MAP_MODE_MOB_SPEC_RENT] = flag;
		}
		else if (isname(*k, "почта"))
		{
			bits[MAP_MODE_MOB_SPEC_MAIL] = flag;
		}
		else if (isname(*k, "банк"))
		{
			bits[MAP_MODE_MOB_SPEC_BANK] = flag;
		}
		else if (isname(*k, "базар"))
		{
			bits[MAP_MODE_MOB_SPEC_EXCH] = flag;
		}
		else if (isname(*k, "лошадь"))
		{
			bits[MAP_MODE_MOB_SPEC_HORSE] = flag;
		}
		else if (isname(*k, "учитель"))
		{
			bits[MAP_MODE_MOB_SPEC_TEACH] = flag;
		}
		else if (isname(*k, "глашатай"))
		{
			bits[MAP_MODE_MOB_SPEC_TORC] = flag;
		}
		else if (isname(*k, "все специальные"))
		{
			bits[MAP_MODE_MOB_SPEC_ALL] = flag;
		}
		else if (isname(*k, "мобы в комнате"))
		{
			bits[MAP_MODE_MOBS_CURR_ROOM] = flag;
		}
		else if (isname(*k, "предметы в комнате"))
		{
			bits[MAP_MODE_OBJS_CURR_ROOM] = flag;
		}
		else
		{
			error = true;
			send_to_char(ch, "Неверный параметр: %s\r\n", k->c_str());
		}
	}

	return error;
}

void Options::text_olc(CHAR_DATA *ch, const char *arg)
{
	std::string str(arg), first_arg;
	GetOneParam(str, first_arg);
	boost::trim(str);

	if (isname(first_arg, "редактировать") || isname(first_arg, "опции")|| isname(first_arg, "меню")
		|| isname(first_arg, "edit") || isname(first_arg, "options") || isname(first_arg, "menu"))
	{
		ch->map_olc();
	}
	else if (isname(first_arg, "справка") || isname(first_arg, "помощь"))
	{
		send_to_char(message, ch);
	}
	else if (isname(first_arg, "включить") || isname(first_arg, "activate")
		|| isname(first_arg, "выключить") || isname(first_arg, "deactivate"))
	{
		if (str.empty())
		{
			send_to_char(message, ch);
			send_to_char("\r\nУкажите нужные опции.\r\n", ch);
			return;
		}

		const bool flag = (isname(first_arg, "включить") || isname(first_arg, "activate")) ? true : false;
		const bool error = parse_text_olc(ch, str, bit_list_, flag);
		if (!error)
		{
			send_to_char("Сделано.\r\n", ch);
		}
	}
	else if (isname(first_arg, "показать") || isname(first_arg, "show"))
	{
		if (str.empty())
		{
			send_to_char(message, ch);
			send_to_char("\r\nУкажите нужные опции.\r\n", ch);
			return;
		}

		std::bitset<TOTAL_MAP_OPTIONS> tmp_bits;
		parse_text_olc(ch, str, tmp_bits, true);

		std::bitset<TOTAL_MAP_OPTIONS> saved_ch_bits = bit_list_;
		bit_list_ = tmp_bits;
		print_map(ch);
		bit_list_ = saved_ch_bits;
	}
	else
	{
		send_to_char(message, ch);
	}
}

} // namespace MapSystem

void do_map(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	if (IS_NPC(ch))
	{
		return;
	}
	if (PRF_FLAGGED(ch, PRF_BLIND))
	{	send_to_char("В режиме слепого игрока карта недоступна.\r\n", ch);
		return;
	}
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
	{
		send_to_char("Слепому карта не поможет!\r\n", ch);
		return;
	}
	else if (is_dark(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !can_use_feat(ch, DARK_READING_FEAT))
	{
		send_to_char("Идем на ощупь и на запах!\r\n", ch);
		return;
	}
	
	skip_spaces(&argument);

	if (!argument || !*argument)
	{
		MapSystem::print_map(ch);
	}
	else
	{
		ch->map_text_olc(argument);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
