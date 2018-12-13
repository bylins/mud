/* ************************************************************************
*   File: spells.cpp                                    Part of Bylins    *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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

#include "spells.h"

#include "coredump.hpp"
#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "char_obj_utils.inl"
#include "obj.hpp"
#include "comm.h"
#include "skills.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "screen.h"
#include "house.h"
#include "pk.h"
#include "features.hpp"
#include "im.h"
#include "deathtrap.hpp"
#include "privilege.hpp"
#include "char.hpp"
#include "depot.hpp"
#include "parcel.hpp"
#include "liquid.hpp"
#include "modify.h"
#include "room.hpp"
#include "birth_places.hpp"
#include "obj_sets.hpp"
#include "logger.hpp"
#include "magic.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"
#include "world.characters.hpp"
#include "zone.table.hpp"

#include <vector>

extern room_rnum r_mortal_start_room;

extern DESCRIPTOR_DATA *descriptor_list;
extern const char *material_name[];
extern const char *weapon_affects[];
extern TIME_INFO_DATA time_info;
extern int cmd_tell;
extern char cast_argument[MAX_INPUT_LENGTH];
extern int slot_for_char(CHAR_DATA * ch, int slot_num);
// added by WorM  опознание магических ингров 2011.05.21
extern im_type *imtypes;
extern int top_imtypes;
//end by WorM

ESkill get_magic_skill_number_by_spell(int spellnum);
bool can_get_spell(CHAR_DATA *ch, int spellnum);
bool can_get_spell_with_req(CHAR_DATA *ch, int spellnum, int req_lvl);
void weight_change_object(OBJ_DATA * obj, int weight);
int compute_armor_class(CHAR_DATA * ch);
char *diag_weapon_to_char(const CObjectPrototype* obj, int show_wear);
void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
int calc_anti_savings(CHAR_DATA * ch);
void go_flee(CHAR_DATA * ch);

void do_tell(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

void perform_remove(CHAR_DATA * ch, int pos);
int get_zone_rooms(int, int *, int *);

int pk_action_type_summon(CHAR_DATA * agressor, CHAR_DATA * victim);
void pk_increment_revenge(CHAR_DATA * agressor, CHAR_DATA * victim);

int what_sky = SKY_CLOUDLESS;
// * Special spells appear below.

ESkill get_magic_skill_number_by_spell(int spellnum)
{
	switch (spell_info[spellnum].spell_class)
	{
	case STYPE_AIR:
		return SKILL_AIR_MAGIC;
		break;

	case STYPE_FIRE:
		return SKILL_FIRE_MAGIC;
		break;

	case STYPE_WATER:
		return SKILL_WATER_MAGIC;
		break;

	case STYPE_EARTH:
		return SKILL_EARTH_MAGIC;
		break;

	case STYPE_LIGHT:
		return SKILL_LIGHT_MAGIC;
		break;

	case STYPE_DARK:
		return SKILL_DARK_MAGIC;
		break;

	case STYPE_MIND:
		return SKILL_MIND_MAGIC;
		break;

	case STYPE_LIFE:
		return SKILL_LIFE_MAGIC;
		break;

	case STYPE_NEUTRAL:
	default:
		return SKILL_INVALID;
	}
}

//Определим мин уровень для изучения спелла из книги
//req_lvl - требуемый уровень из книги
int min_spell_lvl_with_req(CHAR_DATA *ch, int spellnum, int req_lvl)
{
	int min_lvl = MAX(req_lvl, BASE_CAST_LEV(spell_info[spellnum], ch)) - (MAX(GET_REMORT(ch) - MIN_CAST_REM(spell_info[spellnum], ch), 0) / 3);

	return MAX(1, min_lvl);
}

bool can_get_spell_with_req(CHAR_DATA *ch, int spellnum, int req_lvl)
{
	if (min_spell_lvl_with_req(ch, spellnum, req_lvl) > GET_LEVEL(ch)
		|| MIN_CAST_REM(spell_info[spellnum], ch) > GET_REMORT(ch))
		return FALSE;

	return TRUE;
};

// Функция определяет возможность изучения спелла из книги или в гильдии
bool can_get_spell(CHAR_DATA *ch, int spellnum)
{
	if (MIN_CAST_LEV(spell_info[spellnum], ch) > GET_LEVEL(ch) || MIN_CAST_REM(spell_info[spellnum], ch) > GET_REMORT(ch))
		return FALSE;

	return TRUE;
};

typedef std::map<EIngredientFlag, std::string> EIngredientFlag_name_by_value_t;
typedef std::map<const std::string, EIngredientFlag> EIngredientFlag_value_by_name_t;
EIngredientFlag_name_by_value_t EIngredientFlag_name_by_value;
EIngredientFlag_value_by_name_t EIngredientFlag_value_by_name;

void init_EIngredientFlag_ITEM_NAMES()
{
	EIngredientFlag_name_by_value.clear();
	EIngredientFlag_value_by_name.clear();

	EIngredientFlag_name_by_value[EIngredientFlag::ITEM_RUNES] = "ITEM_RUNES";
	EIngredientFlag_name_by_value[EIngredientFlag::ITEM_CHECK_USES] = "ITEM_CHECK_USES";
	EIngredientFlag_name_by_value[EIngredientFlag::ITEM_CHECK_LAG] = "ITEM_CHECK_LAG";
	EIngredientFlag_name_by_value[EIngredientFlag::ITEM_CHECK_LEVEL] = "ITEM_CHECK_LEVEL";
	EIngredientFlag_name_by_value[EIngredientFlag::ITEM_DECAY_EMPTY] = "ITEM_DECAY_EMPTY";

	for (const auto& i : EIngredientFlag_name_by_value)
	{
		EIngredientFlag_value_by_name[i.second] = i.first;
	}
}

template <>
EIngredientFlag ITEM_BY_NAME(const std::string& name)
{
	if (EIngredientFlag_name_by_value.empty())
	{
		init_EIngredientFlag_ITEM_NAMES();
	}
	return EIngredientFlag_value_by_name.at(name);
}

template <>
const std::string& NAME_BY_ITEM<EIngredientFlag>(const EIngredientFlag item)
{
	if (EIngredientFlag_name_by_value.empty())
	{
		init_EIngredientFlag_ITEM_NAMES();
	}
	return EIngredientFlag_name_by_value.at(item);
}

void spell_create_water(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA* obj)
{
	int water;
	if (ch == NULL || (obj == NULL && victim == NULL))
		return;
	// level = MAX(MIN(level, LVL_IMPL), 1);       - not used

	if (obj
		&& GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON)
	{
		if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0))
		{
			send_to_char("Прекратите, ради бога, химичить.\r\n", ch);
			return;
		}
		else
		{
			water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
			if (water > 0)
			{
				if (GET_OBJ_VAL(obj, 1) >= 0)
				{
					name_from_drinkcon(obj);
				}
				obj->set_val(2, LIQ_WATER);
				obj->add_val(1, water);
				act("Вы наполнили $o3 водой.", FALSE, ch, obj, 0, TO_CHAR);
				name_to_drinkcon(obj, LIQ_WATER);
				weight_change_object(obj, water);
			}
		}
	}
	if (victim && !IS_NPC(victim) && !IS_IMMORTAL(victim))
	{
		GET_COND(victim, THIRST) = 0;
		send_to_char("Вы полностью утолили жажду.\r\n", victim);
		if (victim != ch)
		{
			act("Вы напоили $N3.", FALSE, ch, 0, victim, TO_CHAR);
		}
	}
}

#define SUMMON_FAIL "Попытка перемещения не удалась.\r\n"
#define SUMMON_FAIL2 "Ваша жертва устойчива к этому.\r\n"
#define SUMMON_FAIL3 "Магический кокон, окружающий вас, помешал заклинанию сработать правильно.\r\n"
#define SUMMON_FAIL4 "Ваша жертва в бою, подождите немного.\r\n"
#define MIN_NEWBIE_ZONE  20
#define MAX_NEWBIE_ZONE  79
#define MAX_SUMMON_TRIES 2000

// Поиск комнаты для перемещающего заклинания
int get_teleport_target_room(CHAR_DATA * ch,	// ch - кого перемещают
							 int rnum_start,	// rnum_start - первая комната диапазона
							 int rnum_stop	// rnum_stop - последняя комната диапазона
							)
{
	int *r_array;
	int n, i, j;
	int fnd_room = NOWHERE;

	n = rnum_stop - rnum_start + 1;

	if (n <= 0)
		return NOWHERE;

	r_array = (int *) malloc(n * sizeof(int));
	for (i = 0; i < n; ++i)
		r_array[i] = rnum_start + i;

	for (; n; --n)
	{
		j = number(0, n - 1);
		fnd_room = r_array[j];
		r_array[j] = r_array[n - 1];

		if (SECT(fnd_room) != SECT_SECRET &&
				!ROOM_FLAGGED(fnd_room, ROOM_DEATH) &&
				!ROOM_FLAGGED(fnd_room, ROOM_TUNNEL) &&
				!ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORTIN) &&
				!ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH) &&
				!ROOM_FLAGGED(fnd_room, ROOM_ICEDEATH) &&
				(!ROOM_FLAGGED(fnd_room, ROOM_GODROOM) || IS_IMMORTAL(ch)) &&
				Clan::MayEnter(ch, fnd_room, HCE_PORTAL))
			break;
	}

	free(r_array);

	return n ? fnd_room : NOWHERE;
}

void spell_recall(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA* /* obj*/)
{
	room_rnum to_room = NOWHERE, fnd_room = NOWHERE;
	room_rnum rnum_start, rnum_stop;

	if (!victim || IS_NPC(victim) || ch->in_room != IN_ROOM(victim) || GET_LEVEL(victim) >= LVL_IMMORT)
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	if (!IS_GOD(ch) && (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORTOUT) || AFF_FLAGGED(victim, EAffectFlag::AFF_NOTELEPORT)))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	if (victim != ch)
	{
		if (same_group(ch, victim))
		{
			if (number(1, 100) <= 5 )
			{
				send_to_char(SUMMON_FAIL, ch);
				return;
			}
		}
		else if (!IS_NPC(ch) || (ch->has_master() && !IS_NPC(ch->get_master()))) // игроки не в группе и  чармисы по приказу не могут реколить свитком
		{
				send_to_char(SUMMON_FAIL, ch);
				return;
		}

		if ((IS_NPC(ch) && general_savingthrow(ch, victim, SAVING_WILL, GET_REAL_INT(ch))) || IS_GOD(victim))
		{
			return;
		}
	}

	if ((to_room = real_room(GET_LOADROOM(victim))) == NOWHERE)
		to_room = real_room(calc_loadroom(victim));

	if (to_room == NOWHERE)
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	(void) get_zone_rooms(world[to_room]->zone, &rnum_start, &rnum_stop);
	fnd_room = get_teleport_target_room(victim, rnum_start, rnum_stop);
	if (fnd_room == NOWHERE)
	{
		to_room = Clan::CloseRent(to_room);
		(void) get_zone_rooms(world[to_room]->zone, &rnum_start, &rnum_stop);
		fnd_room = get_teleport_target_room(victim, rnum_start, rnum_stop);
	}

	if (fnd_room == NOWHERE)
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	if (victim->get_fighting() && (victim != ch))
	{
		if (!pk_agro_action(ch, victim->get_fighting()))
			return;
	}

	act("$n исчез$q.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	char_from_room(victim);
	char_to_room(victim, fnd_room);
	check_horse(victim);
	act("$n появил$u в центре комнаты.", TRUE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, 0);
	greet_mtrigger(victim, -1);
	greet_otrigger(victim, -1);
}

// ПРЫЖОК в рамках зоны
void spell_teleport(int/* level*/, CHAR_DATA *ch, CHAR_DATA* /*victim*/, OBJ_DATA* /* obj*/)
{
	room_rnum in_room = ch->in_room, fnd_room = NOWHERE;
	room_rnum rnum_start, rnum_stop;

	if (!IS_GOD(ch) && (ROOM_FLAGGED(in_room, ROOM_NOTELEPORTOUT) || AFF_FLAGGED(ch, EAffectFlag::AFF_NOTELEPORT)))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	get_zone_rooms(world[in_room]->zone, &rnum_start, &rnum_stop);
	fnd_room = get_teleport_target_room(ch, rnum_start, rnum_stop);
	if (fnd_room == NOWHERE)
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	act("$n медленно исчез$q из виду.", FALSE, ch, 0, 0, TO_ROOM);
	char_from_room(ch);
	char_to_room(ch, fnd_room);
	check_horse(ch);
	act("$n медленно появил$u откуда-то.", FALSE, ch, 0, 0, TO_ROOM);
	look_at_room(ch, 0);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
}

// ПЕРЕМЕСТИТЬСЯ
void spell_relocate(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA* /* obj*/)
{
	room_rnum to_room, fnd_room;

	if (victim == NULL)
		return;

	// Если левел жертвы больше чем перемещяющегося - фейл
	if (IS_NPC(victim) || (GET_LEVEL(victim) > GET_LEVEL(ch)) || IS_IMMORTAL(victim))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	// Для иммов обязательные для перемещения условия не существенны
	if (!IS_GOD(ch))
	{
		// Нельзя перемещаться из клетки ROOM_NOTELEPORTOUT
		if (ROOM_FLAGGED(ch->in_room, ROOM_NOTELEPORTOUT))
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
		}

		// Нельзя перемещаться после того, как попал под заклинание "приковать противника".
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_NOTELEPORT))
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
		}
	}

	to_room = IN_ROOM(victim);

	if (to_room == NOWHERE)
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}
	// в случае, если жертва не может зайти в замок (по любой причине)
	// прыжок в зону ближайшей ренты
	if (!Clan::MayEnter(ch, to_room, HCE_PORTAL))
		fnd_room = Clan::CloseRent(to_room);
	else
		fnd_room = to_room;

	if (fnd_room != to_room && !IS_GOD(ch))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	if (!IS_GOD(ch) &&
			(SECT(fnd_room) == SECT_SECRET ||
			 ROOM_FLAGGED(fnd_room, ROOM_DEATH) ||
			 ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH) ||
			 ROOM_FLAGGED(fnd_room, ROOM_TUNNEL) ||
			 ROOM_FLAGGED(fnd_room, ROOM_NORELOCATEIN) ||
			 ROOM_FLAGGED(fnd_room, ROOM_ICEDEATH) || (ROOM_FLAGGED(fnd_room, ROOM_GODROOM) && !IS_IMMORTAL(ch))))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	act("$n медленно исчез$q из виду.", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("Лазурные сполохи пронеслись перед вашими глазами.\r\n", ch);
	char_from_room(ch);
	char_to_room(ch, fnd_room);
	check_horse(ch);
	act("$n медленно появил$u откуда-то.", TRUE, ch, 0, 0, TO_ROOM);
	if (!(PRF_FLAGGED(victim, PRF_SUMMONABLE) || same_group(ch, victim) || IS_IMMORTAL(ch)))
	{
		send_to_char(ch, "%sВаш поступок был расценен как потенциально агрессивный.%s\r\n",
			CCIRED(ch, C_NRM), CCINRM(ch, C_NRM));
		pkPortal(ch);
	}
	look_at_room(ch, 0);
	// Прыжок на чара в БД удваивает лаг
	if (RENTABLE(victim))
	{
		WAIT_STATE(ch, 4 * PULSE_VIOLENCE);
	}
	else
	{
		WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
	}
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
}

inline void decay_portal(const int room_num)
{
	act("Пентаграмма медленно растаяла.", FALSE, world[room_num]->first_character(), 0, 0, TO_ROOM);
	act("Пентаграмма медленно растаяла.", FALSE, world[room_num]->first_character(), 0, 0, TO_CHAR);
	world[room_num]->portal_time = 0;
	world[room_num]->portal_room = 0;
}

void check_auto_nosummon(CHAR_DATA *ch)
{
	if (PRF_FLAGGED(ch, PRF_AUTO_NOSUMMON) && PRF_FLAGGED(ch, PRF_SUMMONABLE))
	{
		PRF_FLAGS(ch).unset(PRF_SUMMONABLE);
		send_to_char("Режим автопризыв: вы защищены от призыва.\r\n", ch);
	}
}

void spell_portal(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA* /* obj*/)
{
	room_rnum to_room, fnd_room;

	if (victim == NULL)
		return;
	if (GET_LEVEL(victim) > GET_LEVEL(ch) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) && !same_group(ch, victim))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}
	// пентить чаров <=10 уровня, нельзя так-же нельзя пентать иммов
	if (!IS_GOD(ch))
	{
		if ((!IS_NPC(victim) && GET_LEVEL(victim) <= 10) || IS_IMMORTAL(victim) || AFF_FLAGGED(victim, EAffectFlag::AFF_NOTELEPORT))
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
		}
	}
	if (IS_NPC(victim))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	fnd_room = IN_ROOM(victim);
	if (fnd_room == NOWHERE)
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}
	// обработка NOTELEPORTIN и NOTELEPORTOUT теперь происходит при входе в портал
	if (!IS_GOD(ch) && ( //ROOM_FLAGGED(ch->in_room, ROOM_NOTELEPORTOUT)||
				//ROOM_FLAGGED(ch->in_room, ROOM_NOTELEPORTIN)||
				SECT(fnd_room) == SECT_SECRET || ROOM_FLAGGED(fnd_room, ROOM_DEATH) || ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH) || ROOM_FLAGGED(fnd_room, ROOM_ICEDEATH) || ROOM_FLAGGED(fnd_room, ROOM_TUNNEL) || ROOM_FLAGGED(fnd_room, ROOM_GODROOM)	//||
				//ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORTOUT) ||
				//ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORTIN)
			))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	//Юзабилити фикс: не ставим пенту в одну клетку с кастующим
	if (ch->in_room == fnd_room)
	{
		send_to_char("Может вам лучше просто потоптаться на месте?\r\n", ch);
		return;
	}

	if (world[fnd_room]->portal_time)
	{
		if (world[world[fnd_room]->portal_room]->portal_room == fnd_room && world[world[fnd_room]->portal_room]->portal_time)
			decay_portal(world[fnd_room]->portal_room);
		decay_portal(fnd_room);
	}
	if (world[ch->in_room]->portal_time)
	{
		if (world[world[ch->in_room]->portal_room]->portal_room == ch->in_room && world[world[ch->in_room]->portal_room]->portal_time)
			decay_portal(world[ch->in_room]->portal_room);
		decay_portal(ch->in_room);
	}
	bool pkPortal = pk_action_type_summon(ch, victim) == PK_ACTION_REVENGE ||
			pk_action_type_summon(ch, victim) == PK_ACTION_FIGHT;

	if (IS_IMMORTAL(ch) || GET_GOD_FLAG(victim, GF_GODSCURSE)
			// раньше было <= PK_ACTION_REVENGE, что вызывало абьюз при пенте на чара на арене,
			// или пенте кидаемой с арены т.к. в данном случае использовалось PK_ACTION_NO которое меньше PK_ACTION_REVENGE
			   || pkPortal || ((!IS_NPC(victim) || IS_CHARMICE(ch)) && PRF_FLAGGED(victim, PRF_SUMMONABLE))
			|| same_group(ch, victim))
	{
		// Если пента по мести - то считаем постановку пенты попыткой ее реализовать
		// после 3ех попыток реализаци (3ех пент) -- месть исчезает
		if (pkPortal) pk_increment_revenge(ch, victim);

		to_room = ch->in_room;
		world[fnd_room]->portal_room = to_room;
		world[fnd_room]->portal_time = 1;
		if (pkPortal) world[fnd_room]->pkPenterUnique = GET_UNIQUE(ch);

		if (pkPortal)
		{
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.", FALSE, world[fnd_room]->first_character(), 0, 0, TO_CHAR);
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.", FALSE, world[fnd_room]->first_character(), 0, 0, TO_ROOM);
		}else
		{
			act("Лазурная пентаграмма возникла в воздухе.", FALSE, world[fnd_room]->first_character(), 0, 0, TO_CHAR);
			act("Лазурная пентаграмма возникла в воздухе.", FALSE, world[fnd_room]->first_character(), 0, 0, TO_ROOM);
		}
		check_auto_nosummon(victim);

		// если пенту ставит имм с привилегией arena (и находясь на арене), то пента получается односторонняя
		if (Privilege::check_flag(ch, Privilege::ARENA_MASTER) && ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
			return;

		world[to_room]->portal_room = fnd_room;
		world[to_room]->portal_time = 1;
		if (pkPortal) world[to_room]->pkPenterUnique = GET_UNIQUE(ch);

		if (pkPortal)
		{
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.", FALSE, world[to_room]->first_character(), 0, 0, TO_CHAR);
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.", FALSE, world[to_room]->first_character(), 0, 0, TO_ROOM);
		}else
		{
			act("Лазурная пентаграмма возникла в воздухе.", FALSE, world[to_room]->first_character(), 0, 0, TO_CHAR);
			act("Лазурная пентаграмма возникла в воздухе.", FALSE, world[to_room]->first_character(), 0, 0, TO_ROOM);
		}
	}
}

void spell_summon(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA* /* obj*/)
{
	room_rnum ch_room, vic_room;
	struct follow_type *k, *k_next;
	// Если переменные не определены или каст на себя - завершаем.
	if (ch == NULL || victim == NULL || ch == victim)
		return;

	ch_room = ch->in_room;
	vic_room = IN_ROOM(victim);

	// Нельзя суммонить находясь в NOWHERE или если цель в NOWHERE.
	if (ch_room == NOWHERE || vic_room == NOWHERE)
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	// Игрок не может присуммонить моба.
	if (!IS_NPC(ch) && IS_NPC(victim))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	// Моб не может призвать моба
	// Насчет этого я не особо уверен что такое может случиться (разве что суммон запоминающим мобом чармиса),
	// но в старом коде суммона данная строчка присутствовала в одном из условий.
	if (IS_NPC(ch) && IS_NPC(victim))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	// Богов лучше не суммонить - им это может не понравится.
	if (IS_IMMORTAL(victim))
	{
		if (IS_NPC(ch) || (!IS_NPC(ch) && GET_LEVEL(ch) < GET_LEVEL(victim)))
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
		}
	}

	// Ограничения для смертных (богов ниже следующее не касается)
	if (!IS_IMMORTAL(ch))
	{
		// Если игрок не моб или чармис, то:
		if (!IS_NPC(ch) || IS_CHARMICE(ch))
		{
			// Нельзя производить суммон под ЗБ
			if (AFF_FLAGGED(ch, EAffectFlag::AFF_SHIELD))
			{
				send_to_char(SUMMON_FAIL3, ch);	// Про маг. кокон вокруг суммонера
				return;
			}
			// Нельзя призвать жертву без режима (если не в группе)
			if (!PRF_FLAGGED(victim, PRF_SUMMONABLE) && !same_group(ch, victim))
			{
				send_to_char(SUMMON_FAIL2, ch);	// Устойчива
				return;
			}
			// Нельзя призвать жертву в БД
			if (RENTABLE(victim))
			{
				send_to_char(SUMMON_FAIL, ch);
				return;
			}
			// Нельзя призвать жертву в бою
			if (victim->get_fighting())
			{
				send_to_char(SUMMON_FAIL4, ch);	// Цель в бою
				return;
			}
		}
		// И независимо от того моб это или нет:
		// Жертву в лаге нельзя суммонить.
		if (GET_WAIT(victim) > 0)
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
		}
		// и чаров 10 и ниже левела тоже
		if (!IS_NPC(ch) && GET_LEVEL(victim) <= 10)
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
		}

		// Проверка мест где нельзя суммонить и откуда нельзя суммонить.
		// Где нельзя суммонить:
		if (ROOM_FLAGGED(ch_room, ROOM_NOSUMMON) ||	// суммонер в комнате с флагом !призвать
				ROOM_FLAGGED(ch_room, ROOM_DEATH) ||	// суммонер в ДТ
				ROOM_FLAGGED(ch_room, ROOM_SLOWDEATH) ||	// суммонер в медленном ДТ
				ROOM_FLAGGED(ch_room, ROOM_TUNNEL) ||	// суммонер в ван-руме
				ROOM_FLAGGED(ch_room, ROOM_PEACEFUL) ||	// суммонер в мирной комнате
				ROOM_FLAGGED(ch_room, ROOM_NOBATTLE) ||	// суммонер в комнате с запретом на драки
				ROOM_FLAGGED(ch_room, ROOM_GODROOM) ||	// суммонер в комнате для бессмертных
				ROOM_FLAGGED(ch_room, ROOM_ARENA) ||	// суммонер на арене
				!Clan::MayEnter(victim, ch_room, HCE_PORTAL) ||	// суммонер стоит во внутренней части клан-замка
				SECT(ch->in_room) == SECT_SECRET)	// суммонер стоит в клетке с типом "секретый"
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
		}
		// Жертву нельзя призвать если она в:
		// разные варианты моб и игрок
		if (!IS_NPC(ch))
		{
			// для чаров
			if (ROOM_FLAGGED(vic_room, ROOM_NOSUMMON) ||	// жертва в комнате с флагом !призвать
				ROOM_FLAGGED(vic_room, ROOM_TUNNEL)	||	// жертва стоит в ван-руме
				ROOM_FLAGGED(vic_room, ROOM_GODROOM)||	// жертва в комнате для бессмертных
				ROOM_FLAGGED(vic_room, ROOM_ARENA)	||	// жертва на арене
				!Clan::MayEnter(ch, vic_room, HCE_PORTAL)||// жертва во внутренних покоях клан-замка
				AFF_FLAGGED(victim, EAffectFlag::AFF_NOTELEPORT))	// жертва под действием заклинания "приковать противника"
			{
			send_to_char(SUMMON_FAIL, ch);
				return;
			}
		}
		else
		{
			// для мобов возможно только 2 ошибки 
			if (ROOM_FLAGGED(vic_room, ROOM_NOSUMMON)	||	// жертва в комнате с флагом !призвать
			AFF_FLAGGED(victim, EAffectFlag::AFF_NOTELEPORT))	// жертва под действием заклинания "приковать противника"
			{
				send_to_char(SUMMON_FAIL, ch);
				return;
			}
		}

		// Фейл заклинания суммон
		if (number(1, 100) < 30)
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
		}
	}

	// Ничто не помешало нашему суммону - и он все-таки произошел
	act("$n растворил$u на ваших глазах.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	char_from_room(victim);
	char_to_room(victim, ch_room);
	check_horse(victim);
	act("$n прибыл$g по вызову.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	act("$n призвал$g вас!", FALSE, ch, 0, victim, TO_VICT);
	check_auto_nosummon(victim);
	look_at_room(victim, 0);
	// призываем чармисов
	for (k = victim->followers; k; k = k_next)
	{
		k_next = k->next;
		// проверяем, находится ли чармис в той же комнате, где был сам чар
		if (IN_ROOM(k->follower) == vic_room)
		{
			// проверяем, чармис ли это вообще
			if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM))
			{
				// чармис не должен быть в бою
				if (!k->follower->get_fighting())
				{
					// призываем
					act("$n растворил$u на ваших глазах.", TRUE, k->follower, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
					char_from_room(k->follower);
					char_to_room(k->follower, ch_room);
					act("$n прибыл$g за хозяином.", TRUE, k->follower, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
					act("$n призвал$g вас!", FALSE, ch, 0, k->follower, TO_VICT);
				}
			}
		}
	}

	greet_mtrigger(victim, -1);	// УЖАС!!! Не стоит в эту функцию передавать -1 :)
	greet_otrigger(victim, -1);	// УЖАС!!! Не стоит в эту функцию передавать -1 :)
	return;
}

void spell_townportal(int/* level*/, CHAR_DATA *ch, CHAR_DATA* /*victim*/, OBJ_DATA* /* obj*/)
{
	int gcount = 0, cn = 0, ispr = 0;
	bool has_label_portal = false;
	struct timed_type timed;
	char *nm;
	struct char_portal_type *tmp;
	struct portals_list_type *port;
	struct portals_list_type label_port;
	ROOM_DATA * label_room;

	port = get_portal(-1, cast_argument);

	//если портала нет, проверяем, возможно игрок ставит врата на свою метку
	if (!port && name_cmp(ch, cast_argument))
	{
		//Таки да, персонаж пытается поставить врата на себя, то бишь на свою метку. Ищем комнату с меткой.
		label_room = RoomSpells::find_affected_roomt(GET_ID(ch), SPELL_RUNE_LABEL);

		//Если такая комната есть - заполняем структуру портала
		//Тупо конечно, но какого блин туча проверок по всем функциям рассована? Их все обойти - убиться проще.
		if (label_room)
		{
			label_port.vnum = label_room->number;
			label_port.level = 1;
			port = &label_port;
			has_label_portal = true;
		}
	}
	if (port && (has_char_portal(ch, port->vnum) || has_label_portal))
	{
		// Проверяем скилл тут, чтобы можно было смотреть список и удалять без -!-
		if (timed_by_skill(ch, SKILL_TOWNPORTAL))
		{
			send_to_char("У вас недостаточно сил для постановки врат.\r\n", ch);
			return;
		}

		// Если мы открываем врата из комнаты с камнем, то они не работают //
		if (find_portal_by_vnum(GET_ROOM_VNUM(ch->in_room)))
		{
			send_to_char("Камень рядом с вами мешает вашей магии.\r\n", ch);
			return;
		}

		// Если в комнате есть метка-"камень" то врата ставить нельзя //
		const auto& room = world[ch->in_room];
		const auto room_affect_i = find_room_affect(room, SPELL_RUNE_LABEL);
		if (room_affect_i != room->affected.end())
		{
			send_to_char("Начертанные на земле магические руны подавляют вашу магию!\r\n", ch);
			return;
		}

		// Чтоб не кастили в NOMAGIC
		if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) && !IS_GRGOD(ch))
		{
			send_to_char("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", ch);
			act("Магия $n1 потерпела неудачу и развеялась по воздуху.", FALSE, ch, 0, 0, TO_ROOM);
			return;
		}
		//удаляем переходы
		if (world[ch->in_room]->portal_time)
		{
			if (world[world[ch->in_room]->portal_room]->portal_room == ch->in_room && world[world[ch->in_room]->portal_room]->portal_time)
			{
				decay_portal(world[ch->in_room]->portal_room);
			}
			decay_portal(ch->in_room);
		}


		// Открываем пентаграмму в комнату rnum //
		improove_skill(ch, SKILL_TOWNPORTAL, 1, NULL);
		ROOM_DATA* from_room = world[ch->in_room];
		from_room->portal_room = real_room(port->vnum);
		from_room->portal_time = 1;
		from_room->pkPenterUnique = 0;
		OneWayPortal::add(world[from_room->portal_room], from_room);
		act("Лазурная пентаграмма возникла в воздухе.", FALSE, ch, 0, 0, TO_CHAR);
		act("$n сложил$g руки в молитвенном жесте, испрашивая у Богов врата...", FALSE, ch, 0, 0, TO_ROOM);
		act("Лазурная пентаграмма возникла в воздухе.", FALSE, ch, 0, 0, TO_ROOM);
		if (!IS_IMMORTAL(ch))
		{
			timed.skill = SKILL_TOWNPORTAL;
			// timed.time - это unsigned char, поэтому при уходе в минус будет вынос на 255 и ниже
			int modif = ch->get_skill(SKILL_TOWNPORTAL) / 7 + number(1, 5);
			timed.time = MAX(1, 25 - modif);
			timed_to_char(ch, &timed);
		}
		return;
	}

	// выдаем список мест //
	gcount = sprintf(buf2 + gcount, "Вам доступны следующие врата:\r\n");
	for (tmp = GET_PORTALS(ch); tmp; tmp = tmp->next)
	{
		nm = find_portal_by_vnum(tmp->vnum);
		if (nm)
		{
			gcount += sprintf(buf2 + gcount, "%11s", nm);
			cn++;
			ispr++;
			if (cn == 3)
			{
				gcount += sprintf(buf2 + gcount, "\r\n");
				cn = 0;
			}
		}
	}
	if (cn)
		gcount += sprintf(buf2 + gcount, "\r\n");
	if (!ispr)
	{
		gcount += sprintf(buf2 + gcount, "     В настоящий момент вам неизвестны порталы.\r\n");
	}
	else
	{
		gcount += sprintf(buf2 + gcount, "     Сейчас в памяти - %d.\r\n", ispr);
	}
	gcount += sprintf(buf2 + gcount, "     Максимально     - %d.\r\n", MAX_PORTALS(ch));

	page_string(ch->desc, buf2, 1);
}

void spell_locate_object(int level, CHAR_DATA *ch, CHAR_DATA* /*victim*/, OBJ_DATA* obj)
{
	/*
	 * FIXME: This is broken.  The spell parser routines took the argument
	 * the player gave to the spell and located an object with that keyword.
	 * Since we're passed the object and not the keyword we can only guess
	 * at what the player originally meant to search for. -gg
	 */
	if (!obj)
	{
		return;
	}

	char name[MAX_INPUT_LENGTH];
	bool bloody_corpse = false;
	strcpy(name, cast_argument);

	int tmp_lvl = (IS_GOD(ch)) ? 300 : level;
	unsigned count = tmp_lvl;
	const auto result = world_objects.find_if_and_dec_number([&](const OBJ_DATA::shared_ptr& i)
	{
		const auto obj_ptr = world_objects.get_by_raw_ptr(i.get());
		if (!obj_ptr)
		{
			sprintf(buf, "SYSERR: Illegal object iterator while locate");
			mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);

			return false;
		}

		bloody_corpse = false;
		if (!IS_GOD(ch))
		{
			if (number(1, 100) > (40 + MAX((GET_REAL_INT(ch) - 25) * 2, 0)))
			{
				return false;
			}

			if (IS_CORPSE(i))
			{
				bloody_corpse = catch_bloody_corpse(i.get());
				if (!bloody_corpse)
				{
					return false;
				}
			}
		}

		if (i->get_extra_flag(EExtraFlag::ITEM_NOLOCATE)
			&& i->get_carried_by() != ch) //!локейт стаф может локейтить только имм или тот кто его держит
		{
			return false;
		}

		if (SECT(i->get_in_room()) == SECT_SECRET)
		{
			return false;
		}

		if (i->get_carried_by())
		{
			const auto carried_by = i->get_carried_by();
			const auto carried_by_ptr = character_list.get_character_by_address(carried_by);

			if (!carried_by_ptr)
			{
//				debug::coredump();
				sprintf(buf, "SYSERR: Illegal carried_by ptr. Создана кора для исследований");
				mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
				return false;
			}

			if (!VALID_RNUM(IN_ROOM(carried_by)))
			{
//				debug::coredump();
				sprintf(buf, "SYSERR: Illegal room %d, char %s. Создана кора для исследований", IN_ROOM(carried_by), carried_by->get_name().c_str());
				mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
				return false;
			}

			if (SECT(IN_ROOM(carried_by)) == SECT_SECRET
				|| IS_IMMORTAL(carried_by))
			{
				return false;
			}
		}

		if (!isname(name, i->get_aliases()))
		{
			return false;
		}

		if (i->get_carried_by())
		{
			const auto carried_by = i->get_carried_by();
			if (world[IN_ROOM(carried_by)]->zone == world[ch->in_room]->zone
				|| !IS_NPC(carried_by)
				|| IS_GOD(ch))
			{
				sprintf(buf, "%s наход%sся у %s в инвентаре.\r\n",
					i->get_short_description().c_str(),
					GET_OBJ_POLY_1(ch, i), PERS(carried_by, ch, 1));
			}
			else
			{
				return false;
			}
		}
		else if (i->get_in_room() != NOWHERE
			&& i->get_in_room())
		{
			const auto room = i->get_in_room();
			if ((world[room]->zone == world[ch->in_room]->zone
					&& !i->get_extra_flag(EExtraFlag::ITEM_NOLOCATE))
				|| IS_GOD(ch)
				|| bloody_corpse)
			{
				if (bloody_corpse)
				{
					sprintf(buf, "%s наход%sся в %s (%s).\r\n",
						i->get_short_description().c_str(),
						GET_OBJ_POLY_1(ch, i),
						world[room]->name,
						zone_table[world[room]->zone].name);
				}
				else
				{
					sprintf(buf, "%s наход%sся в %s.\r\n",
						i->get_short_description().c_str(),
						GET_OBJ_POLY_1(ch, i),
						world[room]->name);
				}
			}
			else
			{
				return false;
			}
		}
		else if (i->get_in_obj())
		{
			if (Clan::is_clan_chest(i->get_in_obj()))
			{
				return false; // шоб не забивало локейт на мобах/плеерах - по кланам проходим ниже отдельно
			}
			else
			{
				if (!IS_GOD(ch))
				{
					if (i->get_in_obj()->get_carried_by())
					{
						const auto carried_by = i->get_in_obj()->get_carried_by();
						if (IS_NPC(i->get_in_obj()->get_carried_by())
							&& (i->get_extra_flag(EExtraFlag::ITEM_NOLOCATE)
								|| world[IN_ROOM(carried_by)]->zone != world[ch->in_room]->zone))
						{
							return false;
						}
					}
					if (i->get_in_obj()->get_in_room() != NOWHERE
						&& i->get_in_obj()->get_in_room())
					{
						if ((world[i->get_in_obj()->get_in_room()]->zone != world[IN_ROOM(ch)]->zone
								|| i->get_extra_flag(EExtraFlag::ITEM_NOLOCATE))
							&& !bloody_corpse)
						{
							return false;
						}
					}
					if (i->get_in_obj()->get_worn_by())
					{
						const auto worn_by = i->get_in_obj()->get_worn_by();
						if ((IS_NPC(worn_by)
							&& (i->get_extra_flag(EExtraFlag::ITEM_NOLOCATE)
								|| world[worn_by->in_room]->zone != world[IN_ROOM(ch)]->zone))
							&& !bloody_corpse)
						{
							return false;
						}
					}
				}

				sprintf(buf, "%s наход%sся в %s.\r\n",
					i->get_short_description().c_str(),
					GET_OBJ_POLY_1(ch, i),
					i->get_in_obj()->get_PName(5).c_str());
			}
		}
		else if (i->get_worn_by())
		{
			const auto worn_by = i->get_worn_by();
			if ((IS_NPC(worn_by)
					&& !i->get_extra_flag(EExtraFlag::ITEM_NOLOCATE)
					&& world[IN_ROOM(worn_by)]->zone == world[ch->in_room]->zone)
				|| (!IS_NPC(worn_by)
					&& GET_LEVEL(worn_by) < LVL_IMMORT)
				|| IS_GOD(ch)
				|| bloody_corpse)
			{
				sprintf(buf, "%s надет%s на %s.\r\n",
					i->get_short_description().c_str(),
					GET_OBJ_SUF_6(i),
					PERS(worn_by, ch, 3));
			}
			else
			{
				return false;
			}
		}
		else
		{
			sprintf(buf, "Местоположение %s неопределимо.\r\n", OBJN(i.get(), ch, 1));
		}

//		CAP(buf); issue #59
		send_to_char(buf, ch);

		return true;
	}, count);

	int j = count;
	if (j > 0)
	{
		j = Clan::print_spell_locate_object(ch, j, std::string(name));
	}

	if (j > 0)
	{
		j = Depot::print_spell_locate_object(ch, j, std::string(name));
	}

	if (j > 0)
	{
		j = Parcel::print_spell_locate_object(ch, j, std::string(name));
	}

	if (j == tmp_lvl)
	{
		send_to_char("Вы ничего не чувствуете.\r\n", ch);
	}
}

bool catch_bloody_corpse(OBJ_DATA * l)
{
	bool temp_bloody = false;
	OBJ_DATA* next_element;

	if (!l->get_contains())
	{
		return false;
	}

	if (bloody::is_bloody(l->get_contains()))
	{
		return true;
	}

	if (!l->get_contains()->get_next_content())
	{
		return false;
	}

	next_element = l->get_contains()->get_next_content();
	while (next_element)
	{
		if (next_element->get_contains())
		{
			temp_bloody = catch_bloody_corpse(next_element->get_contains());
			if (temp_bloody)
			{
				return true;
			}
		}

		if (bloody::is_bloody(next_element))
		{
			return true;
		}

		next_element = next_element->get_contains();
	}

	return false;
}

void spell_create_weapon(int/* level*/, CHAR_DATA* /*ch*/, CHAR_DATA* /*victim*/, OBJ_DATA* /* obj*/)
{				//go_create_weapon(ch,NULL,what_sky);
// отключено, так как не реализовано
}


int check_charmee(CHAR_DATA * ch, CHAR_DATA * victim, int spellnum)
{
	struct follow_type *k;
	int cha_summ = 0, reformed_hp_summ = 0;
	bool undead_in_group = FALSE, living_in_group = FALSE;

	for (k = ch->followers; k; k = k->next)
	{
		if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM)
			&& k->follower->get_master() == ch)
		{
			cha_summ++;
			//hp_summ += GET_REAL_MAX_HIT(k->follower);
			reformed_hp_summ += get_reformed_charmice_hp(ch, k->follower, spellnum);
// Проверка на тип последователей -- некрасиво, зато эффективно
			if (MOB_FLAGGED(k->follower, MOB_CORPSE))
			{
				undead_in_group = TRUE;
			}
			else
			{
				living_in_group = TRUE;
			}
		}
	}

	if (undead_in_group && living_in_group)
	{
		mudlog("SYSERR: Undead and living in group simultaniously", NRM, LVL_GOD, ERRLOG, TRUE);
		return (FALSE);
	}

	if (spellnum == SPELL_CHARM && undead_in_group)
	{
		send_to_char("Ваша жертва боится ваших последователей.\r\n", ch);
		return (FALSE);
	}

	if (spellnum != SPELL_CHARM && living_in_group)
	{
		send_to_char("Ваш последователь мешает вам произнести это заклинание.\r\n", ch);
		return (FALSE);
	}

	if (spellnum == SPELL_CLONE && cha_summ >= MAX(1, (GET_LEVEL(ch) + 4) / 5 - 2))
	{
		send_to_char("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (FALSE);
	}

	if (spellnum != SPELL_CLONE && cha_summ >= (GET_LEVEL(ch) + 9) / 10)
	{
		send_to_char("Вы не сможете управлять столькими последователями.\r\n", ch);
		return (FALSE);
	}

	if (spellnum != SPELL_CLONE &&
			reformed_hp_summ + get_reformed_charmice_hp(ch, victim, spellnum) >= get_player_charms(ch, spellnum))
	{
		send_to_char("Вам не под силу управлять такой боевой мощью.\r\n", ch);
		return (FALSE);
	}
	return (TRUE);
}

void spell_charm(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA* /* obj*/)
{
	if (victim == NULL || ch == NULL)
		return;

	if (victim == ch)
		send_to_char("Вы просто очарованы своим внешним видом!\r\n", ch);
	else if (!IS_NPC(victim))
	{
		send_to_char("Вы не можете очаровать реального игрока!\r\n", ch);
		if (!pk_agro_action(ch, victim))
			return;
	}
	else if (!IS_IMMORTAL(ch) && (AFF_FLAGGED(victim, EAffectFlag::AFF_SANCTUARY) || MOB_FLAGGED(victim, MOB_PROTECT)))
		send_to_char("Ваша жертва освящена Богами!\r\n", ch);
// shapirus: нельзя почармить моба под ЗБ
	else if (!IS_IMMORTAL(ch) && (AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD) || MOB_FLAGGED(victim, MOB_PROTECT)))
		send_to_char("Ваша жертва защищена Богами!\r\n", ch);
	else if (!IS_IMMORTAL(ch) && MOB_FLAGGED(victim, MOB_NOCHARM))
		send_to_char("Ваша жертва устойчива к этому!\r\n", ch);
	else if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		send_to_char("Вы сами очарованы кем-то и не можете иметь последователей.\r\n", ch);
	else if (AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM)
			 || MOB_FLAGGED(victim, MOB_AGGRESSIVE)
			 || MOB_FLAGGED(victim, MOB_AGGRMONO)
			 || MOB_FLAGGED(victim, MOB_AGGRPOLY)
			 || MOB_FLAGGED(victim, MOB_AGGR_DAY)
			 || MOB_FLAGGED(victim, MOB_AGGR_NIGHT)
			 || MOB_FLAGGED(victim, MOB_AGGR_FULLMOON)
			 || MOB_FLAGGED(victim, MOB_AGGR_WINTER)
			 || MOB_FLAGGED(victim, MOB_AGGR_SPRING)
			 || MOB_FLAGGED(victim, MOB_AGGR_SUMMER)
			 || MOB_FLAGGED(victim, MOB_AGGR_AUTUMN))
		send_to_char("Ваша магия потерпела неудачу.\r\n", ch);
	else if (IS_HORSE(victim))
		send_to_char("Это боевой скакун, а не хухры-мухры.\r\n", ch);
	else if (victim->get_fighting() || GET_POS(victim) < POS_RESTING)
		act("$M сейчас, похоже, не до вас.", FALSE, ch, 0, victim, TO_CHAR);
	else if (circle_follow(victim, ch))
		send_to_char("Следование по кругу запрещено.\r\n", ch);
	else if (!IS_IMMORTAL(ch) && general_savingthrow(ch, victim, SAVING_WILL, (GET_REAL_CHA(ch) - 10) * 4 + GET_REMORT(ch) * 3))
		send_to_char("Ваша магия потерпела неудачу.\r\n", ch);
	else
	{
		if (!check_charmee(ch, victim, SPELL_CHARM))
		{
			return;
		}

		// Левая проверка
		if (victim->has_master())
		{
			if (stop_follower(victim, SF_MASTERDIE))
			{
				return;
			}
		}

		affect_from_char(victim, SPELL_CHARM);
		ch->add_follower(victim);
		AFFECT_DATA<EApplyLocation> af;
		af.type = SPELL_CHARM;

		if (GET_REAL_INT(victim) > GET_REAL_INT(ch))
		{
			af.duration = pc_duration(victim, GET_REAL_CHA(ch), 0, 0, 0, 0);
		}
		else
		{
			af.duration = pc_duration(victim, GET_REAL_CHA(ch) + number(1, 10) + GET_REMORT(ch) * 2, 0, 0, 0, 0);
		}

		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = to_underlying(EAffectFlag::AFF_CHARM);
		af.battleflag = 0;
		affect_to_char(victim, af);

		if (GET_HELPER(victim))
		{
			GET_HELPER(victim) = NULL;
		}

		act("$n покорил$g ваше сердце настолько, что вы готовы на все ради н$s.", FALSE, ch, 0, victim, TO_VICT);
		if (IS_NPC(victim))
		{
//Eli. Раздеваемся.
			for (int i = 0; i < NUM_WEARS; i++)
			{
				if (GET_EQ(victim, i))
				{
					if (!remove_otrigger(GET_EQ(victim, i), victim))
					{
						continue;
					}

					act("Вы прекратили использовать $o3.", FALSE, victim, GET_EQ(victim, i), 0, TO_CHAR);
					act("$n прекратил$g использовать $o3.", TRUE, victim, GET_EQ(victim, i), 0, TO_ROOM);
					obj_to_char(unequip_char(victim, i | 0x40), victim);
				}
			}

//Eli закончили раздеваться.
			MOB_FLAGS(victim).unset(MOB_AGGRESSIVE);
			MOB_FLAGS(victim).unset(MOB_SPEC);
			PRF_FLAGS(victim).unset(PRF_PUNCTUAL);
// shapirus: !train для чармисов
			MOB_FLAGS(victim).set(MOB_NOTRAIN);
			victim->set_skill(SKILL_PUNCTUAL, 0);
			// по идее при речарме и последующем креше можно оказаться с сейвом без шмота на чармисе -- Krodo
			Crash_crashsave(ch);
			ch->save_char();
		}
	}
}

void do_findhelpee(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	CHAR_DATA *helpee;
	struct follow_type *k;
	int cost=0, times, i;
	char isbank[MAX_INPUT_LENGTH];

	if (IS_NPC(ch) || (!WAITLESS(ch) && !can_use_feat(ch, EMPLOYER_FEAT)))
	{
		send_to_char("Вам недоступно это!\r\n", ch);
		return;
	}

	if (subcmd == SCMD_FREEHELPEE)
	{
		for (k = ch->followers; k; k = k->next)
		{
			if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER)
				&& AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM))
			{
				break;
			}
		}

		if (k)
		{
			if (ch->in_room != IN_ROOM(k->follower))
				act("Вам следует встретиться с $N4 для этого.", FALSE, ch, 0, k->follower, TO_CHAR);
			else if (GET_POS(k->follower) < POS_STANDING)
				act("$N2 сейчас, похоже, не до вас.", FALSE, ch, 0, k->follower, TO_CHAR);
			else
			{
				// added by WorM (Видолюб) 2010.06.04 Cохраняем цену найма моба
				if (!IS_IMMORTAL(ch))
				{
					for (const auto& aff : k->follower->affected)
					{
						if (aff->type == SPELL_CHARM)
						{
							cost = MAX(0, (int)((aff->duration - 1) / 2)*(int)abs(k->follower->mob_specials.hire_price));
							if (cost > 0)
							{
								if (k->follower->mob_specials.hire_price < 0)
								{
									ch->add_bank(cost);
								}
								else
								{
									ch->add_gold(cost);
								}
							}

							break;
						}
					}
				}

				act("Вы рассчитали $N3.", FALSE, ch, 0, k->follower, TO_CHAR);
				// end by WorM
				affect_from_char(k->follower, SPELL_CHARM);
				stop_follower(k->follower, SF_CHARMLOST);
			}
		}
		else
		{
			act("У вас нет наемников!", FALSE, ch, 0, 0, TO_CHAR);
		}
		return;
	}

	argument = one_argument(argument, arg);

	if (!*arg)
	{
		for (k = ch->followers; k; k = k->next)
		{
			if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER)
				&& AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM))
			{
				break;
			}
		}

		if (k)
		{
			act("Вашим наемником является $N.", FALSE, ch, 0, k->follower, TO_CHAR);
		}
		else
		{
			act("У вас нет наемников!", FALSE, ch, 0, 0, TO_CHAR);
		}
		return;
	}

	if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
	{
		send_to_char("Вы не видите никого похожего.\r\n", ch);
		return;
	}

	for (k = ch->followers; k; k = k->next)
	{
		if (AFF_FLAGGED(k->follower, EAffectFlag::AFF_HELPER)
			&& AFF_FLAGGED(k->follower, EAffectFlag::AFF_CHARM))
		{
			break;
		}
	}

	if (helpee == ch)
		send_to_char("И как вы это представляете - нанять самого себя?\r\n", ch);
	else if (!IS_NPC(helpee))
		send_to_char("Вы не можете нанять реального игрока!\r\n", ch);
	else if (!NPC_FLAGGED(helpee, NPC_HELPED))
		act("$N не нанимается!", FALSE, ch, 0, helpee, TO_CHAR);
	else if (AFF_FLAGGED(helpee, EAffectFlag::AFF_CHARM) && (!k  || (k && helpee != k->follower)))
		act("$N под чьим-то контролем.", FALSE, ch, 0, helpee, TO_CHAR);
	else if (AFF_FLAGGED(helpee, EAffectFlag::AFF_DEAFNESS))
		act("$N не слышит вас.", FALSE, ch, 0, helpee, TO_CHAR);
	else if (IS_HORSE(helpee))
		send_to_char("Это боевой скакун, а не хухры-мухры.\r\n", ch);
	else if (helpee->get_fighting() || GET_POS(helpee) < POS_RESTING)
		act("$M сейчас, похоже, не до вас.", FALSE, ch, 0, helpee, TO_CHAR);
	else if (circle_follow(helpee, ch))
		send_to_char("Следование по кругу запрещено.\r\n", ch);
	else
	{
		two_arguments(argument, arg, isbank);
		if (!*arg)
			times = 0;
		else if ((times = atoi(arg)) < 0)
		{
			act("Уточните время, на которое вы хотите нанять $N3.", FALSE, ch, 0, helpee, TO_CHAR);
			return;
		}
		if (!times)  	//cost = GET_LEVEL(helpee) * TIME_KOEFF;
		{
			cost = calc_hire_price(ch, helpee);
			sprintf(buf,
					"$n сказал$g вам : \"Один час моих услуг стоит %d %s\".\r\n",
					cost, desc_count(cost, WHAT_MONEYu));
			act(buf, FALSE, helpee, 0, ch, TO_VICT | CHECK_DEAF);
			return;
		}

		if (k && helpee != k->follower)
		{
			act("Вы уже наняли $N3.", FALSE, ch, 0, k->follower, TO_CHAR);
			return;
		}

		i = calc_hire_price(ch, helpee);
		cost = (WAITLESS(ch) ? 0 : 1) * i * times;
// проверка на overflow - не совсем корректно, но в принципе годится
		sprintf(buf1, "%d", i);
		if (cost < 0 || (strlen(buf1) + strlen(arg)) > 9)
		{
			cost = 100000000;
			sprintf(buf, "$n сказал$g вам : \" Хорошо, не буду жадничать, скину тебе немного с цены.\"");
			act(buf, FALSE, helpee, 0, ch, TO_VICT | CHECK_DEAF);
		}
		if ((!isname(isbank, "банк bank") && cost > ch->get_gold()) ||
				(isname(isbank, "банк bank") && cost > ch->get_bank()))
		{
			sprintf(buf,
					"$n сказал$g вам : \" Мои услуги за %d %s стоят %d %s - это тебе не по карману.\"",
					times, desc_count(times, WHAT_HOUR), cost, desc_count(cost, WHAT_MONEYu));
			act(buf, FALSE, helpee, 0, ch, TO_VICT | CHECK_DEAF);
			return;
		}

		if (helpee->has_master()
			&& helpee->get_master() != ch)
		{
			if (stop_follower(helpee, SF_MASTERDIE))
			{
				return;
			}
		}

		AFFECT_DATA<EApplyLocation> af;
		if (!(k && k->follower == helpee))
		{
			ch->add_follower(helpee);
			af.duration = pc_duration(helpee, times * TIME_KOEFF, 0, 0, 0, 0);
		}
		else
		{
			auto aff = k->follower->affected.begin();
			for (; aff != k->follower->affected.end(); ++aff)
			{
				if ((*aff)->type == SPELL_CHARM)
				{
					break;
				}
			}

			if (aff != k->follower->affected.end())
			{
				af.duration = (*aff)->duration + pc_duration(helpee, times * TIME_KOEFF, 0, 0, 0, 0);
			}
		}

		affect_from_char(helpee, SPELL_CHARM);

		if (isname(isbank, "банк bank"))
		{
			ch->remove_bank(cost);
			helpee->mob_specials.hire_price = -i;// added by WorM (Видолюб) 2010.06.04 Сохраняем цену по которой наняли моба через банк
		}
		else
		{
			ch->remove_gold(cost);
			helpee->mob_specials.hire_price = i;// added by WorM (Видолюб) 2010.06.04 Сохраняем цену по которой наняли моба
		}

		af.type = SPELL_CHARM;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = to_underlying(EAffectFlag::AFF_CHARM);
		af.battleflag = 0;
		affect_to_char(helpee, af);
//		AFF_FLAGS(helpee).set(EAffectFlag::AFF_HELPER); 
		af.type = SPELL_CHARM;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = to_underlying(EAffectFlag::AFF_HELPER);
		af.battleflag = 0;
		affect_to_char(helpee, af);
		sprintf(buf, "$n сказал$g вам : \"Приказывай, %s!\"", IS_FEMALE(ch) ? "хозяйка" : "хозяин");
		act(buf, FALSE, helpee, 0, ch, TO_VICT | CHECK_DEAF);
		if (IS_NPC(helpee))
		{
			for (i = 0; i < NUM_WEARS; i++)
				if (GET_EQ(helpee, i))
				{
					if (!remove_otrigger(GET_EQ(helpee, i), helpee))
						continue;
					act("Вы прекратили использовать $o3.", FALSE, helpee, GET_EQ(helpee, i), 0, TO_CHAR);
					act("$n прекратил$g использовать $o3.", TRUE, helpee, GET_EQ(helpee, i), 0, TO_ROOM);
					obj_to_char(unequip_char(helpee, i | 0x40), helpee);
				}
			MOB_FLAGS(helpee).unset(MOB_AGGRESSIVE);
			MOB_FLAGS(helpee).unset(MOB_SPEC);
			PRF_FLAGS(helpee).unset(PRF_PUNCTUAL);
			// shapirus: !train для чармисов
			MOB_FLAGS(helpee).set(MOB_NOTRAIN);
			helpee->set_skill(SKILL_PUNCTUAL, 0);
			// по идее при речарме и последующем креше можно оказаться с сейвом без шмота на чармисе -- Krodo
			Crash_crashsave(ch);
			ch->save_char();
		}
	}
}

void show_weapon(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON)
	{
		*buf = '\0';
		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD))
		{
			sprintf(buf, "Можно взять %s в правую руку.\r\n", OBJN(obj, ch, 3));
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD))
		{
			sprintf(buf + strlen(buf), "Можно взять %s в левую руку.\r\n", OBJN(obj, ch, 3));
		}

		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS))
		{
			sprintf(buf + strlen(buf), "Можно взять %s в обе руки.\r\n", OBJN(obj, ch, 3));
		}

		if (*buf)
		{
			send_to_char(buf, ch);
		}
	}
}

void print_book_uprgd_skill(CHAR_DATA *ch, const OBJ_DATA *obj)
{
	const int skill_num = GET_OBJ_VAL(obj, 1);
	if (skill_num < 1 || skill_num >= MAX_SKILL_NUM)
	{
		log("SYSERR: invalid skill_num: %d, ch_name=%s, obj_vnum=%d (%s %s %d)",
			skill_num,
			ch->get_name().c_str(),
			GET_OBJ_VNUM(obj),
			__FILE__,
			__func__,
			__LINE__);
		return;
	}
	if (GET_OBJ_VAL(obj, 3) > 0)
	{
		send_to_char(ch, "повышает умение \"%s\" (максимум %d)\r\n",
			skill_info[skill_num].name, GET_OBJ_VAL(obj, 3));
	}
	else
	{
		send_to_char(ch, "повышает умение \"%s\" (не больше максимума текущего перевоплощения)\r\n",
			skill_info[skill_num].name);
	}
}

void mort_show_obj_values(const OBJ_DATA * obj, CHAR_DATA * ch, int fullness)
{
	int i, found, drndice = 0, drsdice = 0, j;
	long int li;

	send_to_char("Вы узнали следующее:\r\n", ch);
	sprintf(buf, "Предмет \"%s\", тип : ", obj->get_short_description().c_str());
	sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
	strcat(buf, buf2);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	strcpy(buf, diag_weapon_to_char(obj, 2));
	if (*buf)
		send_to_char(buf, ch);

	if (fullness < 20)
		return;

	//show_weapon(ch, obj);

	sprintf(buf, "Вес: %d, Цена: %d, Рента: %d(%d)\r\n",
			GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_RENTEQ(obj));
	send_to_char(buf, ch);

	if (fullness < 30)
		return;

	send_to_char("Материал : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprinttype(obj->get_material(), material_name, buf);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (fullness < 40)
		return;

	send_to_char("Неудобен : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	obj->get_no_flags().sprintbits(no_bits, buf, ",",IS_IMMORTAL(ch)?4:0);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (fullness < 50)
		return;

	send_to_char("Недоступен : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	obj->get_anti_flags().sprintbits(anti_bits, buf, ",",IS_IMMORTAL(ch)?4:0);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (obj->get_auto_mort_req() > 0)
	{
		send_to_char(ch, "Требует перевоплощений : %s%d%s\r\n",
			CCCYN(ch, C_NRM), obj->get_auto_mort_req(), CCNRM(ch, C_NRM));
	}
	else if (obj->get_auto_mort_req() < -1)
	{
		send_to_char(ch, "Максимальное количество перевоплощение : %s%d%s\r\n",
			CCCYN(ch, C_NRM), abs(obj->get_minimum_remorts()), CCNRM(ch, C_NRM));
	}

	if (fullness < 60)
		return;

	send_to_char("Имеет экстрафлаги: ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	GET_OBJ_EXTRA(obj).sprintbits(extra_bits, buf, ",",IS_IMMORTAL(ch)?4:0);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (fullness < 75)
		return;

	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_SCROLL:
	case OBJ_DATA::ITEM_POTION:
		sprintf(buf, "Содержит заклинания: ");
		if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
			sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 1)));
		if (GET_OBJ_VAL(obj, 2) >= 1 && GET_OBJ_VAL(obj, 2) < MAX_SPELLS)
			sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 2)));
		if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
			sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 3)));
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		break;

	case OBJ_DATA::ITEM_WAND:
	case OBJ_DATA::ITEM_STAFF:
		sprintf(buf, "Вызывает заклинания: ");
		if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
			sprintf(buf + strlen(buf), " %s\r\n", spell_name(GET_OBJ_VAL(obj, 3)));
		sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
		send_to_char(buf, ch);
		break;

	case OBJ_DATA::ITEM_WEAPON:
		drndice = GET_OBJ_VAL(obj, 1);
		drsdice = GET_OBJ_VAL(obj, 2);
		sprintf(buf, "Наносимые повреждения '%dD%d'", drndice, drsdice);
		sprintf(buf + strlen(buf), " среднее %.1f.\r\n", ((drsdice + 1) * drndice / 2.0));
		send_to_char(buf, ch);
		break;

	case OBJ_DATA::ITEM_ARMOR:
	case OBJ_DATA::ITEM_ARMOR_LIGHT:
	case OBJ_DATA::ITEM_ARMOR_MEDIAN:
	case OBJ_DATA::ITEM_ARMOR_HEAVY:
		drndice = GET_OBJ_VAL(obj, 0);
		drsdice = GET_OBJ_VAL(obj, 1);
		sprintf(buf, "защита (AC) : %d\r\n", drndice);
		send_to_char(buf, ch);
		sprintf(buf, "броня       : %d\r\n", drsdice);
		send_to_char(buf, ch);
		break;

	case OBJ_DATA::ITEM_BOOK:
		switch (GET_OBJ_VAL(obj, 0))
		{
		case BOOK_SPELL:
			if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
			{
				drndice = GET_OBJ_VAL(obj, 1);
				if (MIN_CAST_REM(spell_info[GET_OBJ_VAL(obj, 1)], ch) > GET_REMORT(ch))
					drsdice = 34;
				else
					drsdice = min_spell_lvl_with_req(ch, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
				sprintf(buf, "содержит заклинание        : \"%s\"\r\n", spell_info[drndice].name);
				send_to_char(buf, ch);
				sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
				send_to_char(buf, ch);
			}
			break;

		case BOOK_SKILL:
			if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SKILL_NUM)
			{
				drndice = GET_OBJ_VAL(obj, 1);
				if (skill_info[drndice].classknow[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] == KNOW_SKILL)
				{
					drsdice = min_skill_level_with_req(ch, drndice, GET_OBJ_VAL(obj, 2));
				}
				else
				{
					drsdice = LVL_IMPL;
				}
				sprintf(buf, "содержит секрет умения     : \"%s\"\r\n", skill_info[drndice].name);
				send_to_char(buf, ch);
				sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
				send_to_char(buf, ch);
			}
			break;

		case BOOK_UPGRD:
			print_book_uprgd_skill(ch, obj);
			break;

		case BOOK_RECPT:
			drndice = im_get_recipe(GET_OBJ_VAL(obj, 1));
			if (drndice >= 0)
			{
				drsdice = MAX(GET_OBJ_VAL(obj, 2), imrecipes[drndice].level);
				int count = imrecipes[drndice].remort;
				if (imrecipes[drndice].classknow[(int) GET_CLASS(ch)] != KNOW_RECIPE)
					drsdice = LVL_IMPL;
				sprintf(buf, "содержит рецепт отвара     : \"%s\"\r\n", imrecipes[drndice].name);
				send_to_char(buf, ch);
				if (drsdice == -1 || count == -1)
				{
					send_to_char(CCIRED(ch, C_NRM), ch);
					send_to_char("Некорректная запись рецепта для вашего класса - сообщите Богам.\r\n", ch);
					send_to_char(CCNRM(ch, C_NRM), ch);
				}
				else if (drsdice == LVL_IMPL)
				{
					sprintf(buf, "уровень изучения (количество ремортов) : %d (--)\r\n", drsdice);
					send_to_char(buf, ch);
				}
				else
				{
					sprintf(buf, "уровень изучения (количество ремортов) : %d (%d)\r\n", drsdice, count);
					send_to_char(buf, ch);
				}
			}
			break;

		case BOOK_FEAT:
			if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_FEATS)
			{
				drndice = GET_OBJ_VAL(obj, 1);
				if (can_get_feat(ch, drndice))
				{
					drsdice = feat_info[drndice].slot[(int) GET_CLASS(ch)][(int) GET_KIN(ch)];
				}
				else
				{
					drsdice = LVL_IMPL;
				}
				sprintf(buf, "содержит секрет способности : \"%s\"\r\n", feat_info[drndice].name);
				send_to_char(buf, ch);
				sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
				send_to_char(buf, ch);
			}
			break;

		default:
			send_to_char(CCIRED(ch, C_NRM), ch);
			send_to_char("НЕВЕРНО УКАЗАН ТИП КНИГИ - сообщите Богам\r\n", ch);
			send_to_char(CCNRM(ch, C_NRM), ch);
			break;
		}
		break;

	case OBJ_DATA::ITEM_INGREDIENT:
		sprintbit(GET_OBJ_SKILL(obj), ingradient_bits, buf2);
		sprintf(buf, "%s\r\n", buf2);
		send_to_char(buf, ch);

		if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_USES))
		{
			sprintf(buf, "можно применить %d раз\r\n", GET_OBJ_VAL(obj, 2));
			send_to_char(buf, ch);
		}

		if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_LAG))
		{
			sprintf(buf, "можно применить 1 раз в %d сек", (i = GET_OBJ_VAL(obj, 0) & 0xFF));
			if (GET_OBJ_VAL(obj, 3) == 0 || GET_OBJ_VAL(obj, 3) + i < time(NULL))
				strcat(buf, "(можно применять).\r\n");
			else
			{
				li = GET_OBJ_VAL(obj, 3) + i - time(NULL);
				sprintf(buf + strlen(buf), "(осталось %ld сек).\r\n", li);
			}
			send_to_char(buf, ch);
		}

		if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_LEVEL))
		{
			sprintf(buf, "можно применить с %d уровня.\r\n", (GET_OBJ_VAL(obj, 0) >> 8) & 0x1F);
			send_to_char(buf, ch);
		}

		if ((i = real_object(GET_OBJ_VAL(obj, 1))) >= 0)
		{
			sprintf(buf, "прототип %s%s%s.\r\n",
				CCICYN(ch, C_NRM), obj_proto[i]->get_PName(0).c_str(), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		break;

	case OBJ_DATA::ITEM_MING:
		for (j = 0; imtypes[j].id != GET_OBJ_VAL(obj, IM_TYPE_SLOT) && j <= top_imtypes;)
		{
			j++;
		}
		sprintf(buf, "Это ингредиент вида '%s%s%s'\r\n", CCCYN(ch, C_NRM), imtypes[j].name, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		i = GET_OBJ_VAL(obj, IM_POWER_SLOT);
		if (i > 30)
		{
			send_to_char("Вы не в состоянии определить качество этого ингредиента.\r\n", ch);
		}
		else
		{
			sprintf(buf, "Качество ингредиента ");
			if (i > 40)
				strcat(buf, "божественное.\r\n");
			else if (i > 35)
				strcat(buf, "идеальное.\r\n");
			else if (i > 30)
				strcat(buf, "наилучшее.\r\n");
			else if (i > 25)
				strcat(buf, "превосходное.\r\n");
			else if (i > 20)
				strcat(buf, "отличное.\r\n");
			else if (i > 15)
				strcat(buf, "очень хорошее.\r\n");
			else if (i > 10)
				strcat(buf, "выше среднего.\r\n");
			else if (i > 5)
				strcat(buf, "весьма посредственное.\r\n");
			else
				strcat(buf, "хуже не бывает.\r\n");
			send_to_char(buf, ch);
		}
		break;

	//Информация о контейнерах (Купала)
	case OBJ_DATA::ITEM_CONTAINER:
		sprintf(buf, "Максимально вместимый вес: %d.\r\n", GET_OBJ_VAL(obj, 0));
		send_to_char(buf, ch);
		break;
	
	//Информация о емкостях (Купала)
	case OBJ_DATA::ITEM_DRINKCON:
		drinkcon::identify(ch, obj);
		break;

	case OBJ_DATA::ITEM_MAGIC_ARROW:
	case OBJ_DATA::ITEM_MAGIC_CONTAINER:
		sprintf(buf, "Может вместить стрел: %d.\r\n", GET_OBJ_VAL(obj, 1));
		sprintf(buf, "Осталось стрел: %s%d&n.\r\n",
			GET_OBJ_VAL(obj, 2) > 3 ? "&G" : "&R", GET_OBJ_VAL(obj, 2));
		send_to_char(buf, ch);
		break;

	default:
		break;
	} // switch

	if (fullness < 90)
	{
		return;
	}

	send_to_char("Накладывает на вас аффекты: ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	obj->get_affect_flags().sprintbits(weapon_affects, buf, ",",IS_IMMORTAL(ch)?4:0);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (fullness < 100)
	{
		return;
	}

	found = FALSE;
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->get_affected(i).location != APPLY_NONE
			&& obj->get_affected(i).modifier != 0)
		{
			if (!found)
			{
				send_to_char("Дополнительные свойства :\r\n", ch);
				found = TRUE;
			}
			print_obj_affects(ch, obj->get_affected(i));
		}
	}

	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ENCHANT
		&& GET_OBJ_VAL(obj, 0) != 0)
	{
		if (!found)
		{
			send_to_char("Дополнительные свойства :\r\n", ch);
			found = TRUE;
		}
		send_to_char(ch, "%s   %s вес предмета на %d%s\r\n", CCCYN(ch, C_NRM),
			GET_OBJ_VAL(obj, 0) > 0 ? "увеличивает" : "уменьшает",
			abs(GET_OBJ_VAL(obj, 0)), CCNRM(ch, C_NRM));
	}

	if (obj->has_skills())
	{
		send_to_char("Меняет умения :\r\n", ch);
		CObjectPrototype::skills_t skills;
		obj->get_skills(skills);
		int skill_num;
		int percent;
		for (const auto& it : skills)
		{
			skill_num = it.first;
			percent = it.second;

			if (percent == 0) // TODO: такого не должно быть?
				continue;

			sprintf(buf, "   %s%s%s%s%s%d%%%s\r\n",
				CCCYN(ch, C_NRM), skill_info[skill_num].name, CCNRM(ch, C_NRM),
				CCCYN(ch, C_NRM),
				percent < 0 ? " ухудшает на " : " улучшает на ", abs(percent), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
	}

	id_to_set_info_map::iterator it = OBJ_DATA::set_table.begin();
	if (obj->get_extra_flag(EExtraFlag::ITEM_SETSTUFF))
	{
		for (; it != OBJ_DATA::set_table.end(); it++)
		{
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end())
			{
				sprintf(buf, "Часть набора предметов: %s%s%s\r\n", CCNRM(ch, C_NRM), it->second.get_name().c_str(), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				for (set_info::iterator vnum = it->second.begin(), iend = it->second.end(); vnum != iend; ++vnum)
				{
					const int r_num = real_object(vnum->first);
					if (r_num < 0)
					{
						send_to_char("Неизвестный объект!!!\r\n", ch);
						continue;
					}
					sprintf(buf, "   %s\r\n", obj_proto[r_num]->get_short_description().c_str());
					send_to_char(buf, ch);
				}
				break;
			}
		}
	}

	if (!obj->get_enchants().empty())
	{
		obj->get_enchants().print(ch);
	}
	obj_sets::print_identify(ch, obj);
}

void imm_show_obj_values(OBJ_DATA * obj, CHAR_DATA * ch)
{
	int i, found, drndice = 0, drsdice = 0;
	long int li;

	send_to_char("Вы узнали следующее:\r\n", ch);
	sprintf(buf, "UID: %u, Предмет \"%s\", тип : ", obj->get_uid(), obj->get_short_description().c_str());
	sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
	strcat(buf, buf2);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	strcpy(buf, diag_weapon_to_char(obj, 2));
	if (*buf)
	{
		send_to_char(buf, ch);
	}

	//show_weapon(ch, obj);

	send_to_char("Материал : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprinttype(obj->get_material(), material_name, buf);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	sprintf(buf, "Таймер : %d\r\n", obj->get_timer());
	send_to_char(buf, ch);
	sprintf(buf, "Прочность : %d\\%d\r\n", obj->get_current_durability(), obj->get_maximum_durability());
	send_to_char(buf, ch);

	send_to_char("Накладывает на вас аффекты: ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	obj->get_affect_flags().sprintbits(weapon_affects, buf, ",",IS_IMMORTAL(ch)?4:0);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	send_to_char("Имеет экстрафлаги: ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	GET_OBJ_EXTRA(obj).sprintbits(extra_bits, buf, ",",IS_IMMORTAL(ch)?4:0);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	send_to_char("Недоступен : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	obj->get_anti_flags().sprintbits(anti_bits, buf, ",",IS_IMMORTAL(ch)?4:0);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	send_to_char("Неудобен : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	obj->get_no_flags().sprintbits(no_bits, buf, ",",IS_IMMORTAL(ch)?4:0);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (obj->get_auto_mort_req() > 0)
	{
		send_to_char(ch, "Требует перевоплощений : %s%d%s\r\n",
			CCCYN(ch, C_NRM), obj->get_auto_mort_req(), CCNRM(ch, C_NRM));
	}
	else if (obj->get_auto_mort_req() < -1)
	{
		send_to_char(ch, "Максимальное перевоплощений : %s%d%s\r\n",
			CCCYN(ch, C_NRM), obj->get_auto_mort_req(), CCNRM(ch, C_NRM));
	}

	sprintf(buf, "Вес: %d, Цена: %d, Рента: %d(%d)\r\n",
		GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_RENTEQ(obj));
	send_to_char(buf, ch);

	switch (GET_OBJ_TYPE(obj))
	{
	case OBJ_DATA::ITEM_SCROLL:
	case OBJ_DATA::ITEM_POTION:
		sprintf(buf, "Содержит заклинания: ");
		if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
			sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 1)));
		if (GET_OBJ_VAL(obj, 2) >= 1 && GET_OBJ_VAL(obj, 2) < MAX_SPELLS)
			sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 2)));
		if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
			sprintf(buf + strlen(buf), " %s", spell_name(GET_OBJ_VAL(obj, 3)));
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		break;

	case OBJ_DATA::ITEM_WAND:
	case OBJ_DATA::ITEM_STAFF:
		sprintf(buf, "Вызывает заклинания: ");
		if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
			sprintf(buf + strlen(buf), " %s\r\n", spell_name(GET_OBJ_VAL(obj, 3)));
		sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
		send_to_char(buf, ch);
		break;

	case OBJ_DATA::ITEM_WEAPON:
		sprintf(buf, "Наносимые повреждения '%dD%d'", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
		sprintf(buf + strlen(buf), " среднее %.1f.\r\n",
				(((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
		send_to_char(buf, ch);
		break;

	case OBJ_DATA::ITEM_ARMOR:
	case OBJ_DATA::ITEM_ARMOR_LIGHT:
	case OBJ_DATA::ITEM_ARMOR_MEDIAN:
	case OBJ_DATA::ITEM_ARMOR_HEAVY:
		sprintf(buf, "защита (AC) : %d\r\n", GET_OBJ_VAL(obj, 0));
		send_to_char(buf, ch);
		sprintf(buf, "броня       : %d\r\n", GET_OBJ_VAL(obj, 1));
		send_to_char(buf, ch);
		break;

	case OBJ_DATA::ITEM_BOOK:
		switch (GET_OBJ_VAL(obj, 0))
		{
		case BOOK_SPELL:
			if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
			{
				drndice = GET_OBJ_VAL(obj, 1);
				if (MIN_CAST_REM(spell_info[GET_OBJ_VAL(obj, 1)], ch) > GET_REMORT(ch))
					drsdice = 34;
				else
					drsdice = min_spell_lvl_with_req(ch, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
				sprintf(buf, "содержит заклинание        : \"%s\"\r\n", spell_info[drndice].name);
				send_to_char(buf, ch);
				sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
				send_to_char(buf, ch);
			}
			break;

		case BOOK_SKILL:
			if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SKILL_NUM)
			{
				drndice = GET_OBJ_VAL(obj, 1);
				if (skill_info[drndice].classknow[(int) GET_CLASS(ch)][(int) GET_KIN(ch)] == KNOW_SKILL)
				{
					drsdice = min_skill_level_with_req(ch, drndice, GET_OBJ_VAL(obj, 2));
				}
				else
				{
					drsdice = LVL_IMPL;
				}
				sprintf(buf, "содержит секрет умения     : \"%s\"\r\n", skill_info[drndice].name);
				send_to_char(buf, ch);
				sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
				send_to_char(buf, ch);
			}
			break;

		case BOOK_UPGRD:
			print_book_uprgd_skill(ch, obj);
			break;

		case BOOK_RECPT:
			drndice = im_get_recipe(GET_OBJ_VAL(obj, 1));
			if (drndice >= 0)
			{
				drsdice = MAX(GET_OBJ_VAL(obj, 2), imrecipes[drndice].level);
				i = imrecipes[drndice].remort;
				if (imrecipes[drndice].classknow[(int) GET_CLASS(ch)] != KNOW_RECIPE)
					drsdice = LVL_IMPL;
				sprintf(buf, "содержит рецепт отвара     : \"%s\"\r\n", imrecipes[drndice].name);
				send_to_char(buf, ch);
				if (drsdice == -1 || i == -1)
				{
					send_to_char(CCIRED(ch, C_NRM), ch);
					send_to_char("Некорректная запись рецепта для вашего класса - сообщите Богам.\r\n", ch);
					send_to_char(CCNRM(ch, C_NRM), ch);
				}
				else if (drsdice == LVL_IMPL)
				{
					sprintf(buf, "уровень изучения (количество ремортов) : %d (--)\r\n", drsdice);
					send_to_char(buf, ch);
				}
				else
				{
					sprintf(buf, "уровень изучения (количество ремортов) : %d (%d)\r\n", drsdice, i);
					send_to_char(buf, ch);
				}
			}
			break;

		case BOOK_FEAT:
			if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_FEATS)
			{
				drndice = GET_OBJ_VAL(obj, 1);
				if (can_get_feat(ch, drndice))
				{
					drsdice = feat_info[drndice].slot[(int) GET_CLASS(ch)][(int) GET_KIN(ch)];
				}
				else
				{
					drsdice = LVL_IMPL;
				}
				sprintf(buf, "содержит секрет способности : \"%s\"\r\n", feat_info[drndice].name);
				send_to_char(buf, ch);
				sprintf(buf, "уровень изучения (для вас) : %d\r\n", drsdice);
				send_to_char(buf, ch);
			}
			break;

		default:
			send_to_char(CCIRED(ch, C_NRM), ch);
			send_to_char("НЕВЕРНО УКАЗАН ТИП КНИГИ - сообщите Богам\r\n", ch);
			send_to_char(CCNRM(ch, C_NRM), ch);
			break;
		}
		break;

	case OBJ_DATA::ITEM_INGREDIENT:
		sprintbit(GET_OBJ_SKILL(obj), ingradient_bits, buf2);
		sprintf(buf, "%s\r\n", buf2);
		send_to_char(buf, ch);

		if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_USES))
		{
			sprintf(buf, "можно применить %d раз\r\n", GET_OBJ_VAL(obj, 2));
			send_to_char(buf, ch);
		}

		if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_LAG))
		{
			sprintf(buf, "можно применить 1 раз в %d сек", (i = GET_OBJ_VAL(obj, 0) & 0xFF));
			if (GET_OBJ_VAL(obj, 3) == 0 || GET_OBJ_VAL(obj, 3) + i < time(NULL))
				strcat(buf, "(можно применять).\r\n");
			else
			{
				li = GET_OBJ_VAL(obj, 3) + i - time(NULL);
				sprintf(buf + strlen(buf), "(осталось %ld сек).\r\n", li);
			}
			send_to_char(buf, ch);
		}

		if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_LEVEL))
		{
			sprintf(buf, "можно применить с %d уровня.\r\n", (GET_OBJ_VAL(obj, 0) >> 8) & 0x1F);
			send_to_char(buf, ch);
		}

		if ((i = real_object(GET_OBJ_VAL(obj, 1))) >= 0)
		{
			sprintf(buf, "прототип %s%s%s.\r\n",
				CCICYN(ch, C_NRM), obj_proto[i]->get_PName(0).c_str(), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		break;

	case OBJ_DATA::ITEM_MING:
		sprintf(buf, "Сила ингредиента = %d\r\n", GET_OBJ_VAL(obj, IM_POWER_SLOT));
		send_to_char(buf, ch);
		break;

//Информация о емкостях и контейнерах (Купала)
	case OBJ_DATA::ITEM_CONTAINER:
		sprintf(buf, "Максимально вместимый вес: %d.\r\n", GET_OBJ_VAL(obj, 0));
		send_to_char(buf, ch);
		break;
	case OBJ_DATA::ITEM_DRINKCON:
		drinkcon::identify(ch, obj);
		break;
//Конец инфы о емкостях и контейнерах (Купала)

	default:
		break;
	} // switch

	found = FALSE;
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->get_affected(i).location != APPLY_NONE && obj->get_affected(i).modifier != 0)
		{
			if (!found)
			{
				send_to_char("Дополнительные свойства :\r\n", ch);
				found = TRUE;
			}
			print_obj_affects(ch, obj->get_affected(i));
		}
	}

	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ENCHANT
		&& GET_OBJ_VAL(obj, 0) != 0)
	{
		if (!found)
		{
			send_to_char("Дополнительные свойства :\r\n", ch);
			found = TRUE;
		}
		send_to_char(ch, "%s   %s вес предмета на %d%s\r\n", CCCYN(ch, C_NRM),
				GET_OBJ_VAL(obj, 0) > 0 ? "увеличивает" : "уменьшает",
				abs(GET_OBJ_VAL(obj, 0)), CCNRM(ch, C_NRM));
	}

	if (obj->has_skills())
	{
		send_to_char("Меняет умения :\r\n", ch);
		CObjectPrototype::skills_t skills;
		obj->get_skills(skills);
		int skill_num;
		int percent;
		for (const auto& it : skills)
		{
			skill_num = it.first;
			percent = it.second;

			if (percent == 0) // TODO: такого не должно быть?
				continue;

			sprintf(buf, "   %s%s%s%s%s%d%%%s\r\n",
					CCCYN(ch, C_NRM), skill_info[skill_num].name, CCNRM(ch, C_NRM),
					CCCYN(ch, C_NRM),
					percent < 0 ? " ухудшает на " : " улучшает на ", abs(percent), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
	}

	//added by WorM 2010.09.07 доп ифна о сете
	id_to_set_info_map::iterator it = OBJ_DATA::set_table.begin();
	if (obj->get_extra_flag(EExtraFlag::ITEM_SETSTUFF))
	{
		for (; it != OBJ_DATA::set_table.end(); it++)
		{
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end())
			{
				sprintf(buf, "Часть набора предметов: %s%s%s\r\n", CCNRM(ch, C_NRM), it->second.get_name().c_str(), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				for (set_info::iterator vnum = it->second.begin(), iend = it->second.end(); vnum != iend; ++vnum)
				{
					int r_num;
					if ((r_num = real_object(vnum->first)) < 0)
					{
						send_to_char("Неизвестный объект!!!\r\n", ch);
						continue;
					}
					sprintf(buf, "   %s\r\n", obj_proto[r_num]->get_short_description().c_str());
					send_to_char(buf, ch);
				}
				break;
			}
		}
	}
	//end by WorM

	if (!obj->get_enchants().empty())
	{
		obj->get_enchants().print(ch);
	}
	obj_sets::print_identify(ch, obj);
}

#define IDENT_SELF_LEVEL 6

void mort_show_char_values(CHAR_DATA * victim, CHAR_DATA * ch, int fullness)
{
	int val0, val1, val2;

	sprintf(buf, "Имя: %s\r\n", GET_NAME(victim));
	send_to_char(buf, ch);
	if (!IS_NPC(victim) && victim == ch)
	{
		sprintf(buf, "Написание : %s/%s/%s/%s/%s/%s\r\n",
				GET_PAD(victim, 0), GET_PAD(victim, 1), GET_PAD(victim, 2),
				GET_PAD(victim, 3), GET_PAD(victim, 4), GET_PAD(victim, 5));
		send_to_char(buf, ch);
	}

	if (!IS_NPC(victim) && victim == ch)
	{
		sprintf(buf,
				"Возраст %s  : %d лет, %d месяцев, %d дней и %d часов.\r\n",
				GET_PAD(victim, 1), age(victim)->year, age(victim)->month,
				age(victim)->day, age(victim)->hours);
		send_to_char(buf, ch);
	}
	if (fullness < 20 && ch != victim)
		return;

	val0 = GET_HEIGHT(victim);
	val1 = GET_WEIGHT(victim);
	val2 = GET_SIZE(victim);
	sprintf(buf, /*"Рост %d , */ " Вес %d, Размер %d\r\n", /*val0, */ val1,
			val2);
	send_to_char(buf, ch);
	if (fullness < 60 && ch != victim)
		return;

	val0 = GET_LEVEL(victim);
	val1 = GET_HIT(victim);
	val2 = GET_REAL_MAX_HIT(victim);
	sprintf(buf, "Уровень : %d, может выдержать повреждений : %d(%d)\r\n", val0, val1, val2);
	send_to_char(buf, ch);

	val0 = MIN(GET_AR(victim), 100);
	val1 = MIN(GET_MR(victim), 100);
	val2 = MIN(GET_PR(victim), 100);
	sprintf(buf, "Защита от чар : %d, Защита от магических повреждений : %d, Защита от физических повреждений : %d\r\n", val0, val1, val2);
	send_to_char(buf, ch);
	if (fullness < 90 && ch != victim)
		return;

	send_to_char(ch, "Атака : %d, Повреждения : %d\r\n",
		GET_HR(victim), GET_DR(victim));
	send_to_char(ch, "Защита : %d, Броня : %d, Поглощение : %d\r\n",
		compute_armor_class(victim), GET_ARMOUR(victim), GET_ABSORBE(victim));

	if (fullness < 100 || (ch != victim && !IS_NPC(victim)))
		return;

	val0 = victim->get_str();
	val1 = victim->get_int();
	val2 = victim->get_wis();
	sprintf(buf, "Сила: %d, Ум: %d, Муд: %d, ", val0, val1, val2);
	val0 = victim->get_dex();
	val1 = victim->get_con();
	val2 = victim->get_cha();
	sprintf(buf + strlen(buf), "Ловк: %d, Тел: %d, Обаян: %d\r\n", val0, val1, val2);
	send_to_char(buf, ch);

	if (fullness < 120 || (ch != victim && !IS_NPC(victim)))
		return;

	int found = FALSE;
	for (const auto& aff : victim->affected)
	{
		if (aff->location != APPLY_NONE && aff->modifier != 0)
		{
			if (!found)
			{
				send_to_char("Дополнительные свойства :\r\n", ch);
				found = TRUE;
				send_to_char(CCIRED(ch, C_NRM), ch);
			}
			sprinttype(aff->location, apply_types, buf2);
			sprintf(buf, "   %s изменяет на %s%d\r\n", buf2, aff->modifier > 0 ? "+" : "", aff->modifier);
			send_to_char(buf, ch);
		}
	}
	send_to_char(CCNRM(ch, C_NRM), ch);

	send_to_char("Аффекты :\r\n", ch);
	send_to_char(CCICYN(ch, C_NRM), ch);
	victim->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, "\r\n",IS_IMMORTAL(ch)?4:0);
	sprintf(buf, "%s\r\n", buf2);
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);
}

void imm_show_char_values(CHAR_DATA * victim, CHAR_DATA * ch)
{
	sprintf(buf, "Имя: %s\r\n", GET_NAME(victim));
	send_to_char(buf, ch);
	sprintf(buf, "Написание : %s/%s/%s/%s/%s/%s\r\n",
			GET_PAD(victim, 0), GET_PAD(victim, 1), GET_PAD(victim, 2),
			GET_PAD(victim, 3), GET_PAD(victim, 4), GET_PAD(victim, 5));
	send_to_char(buf, ch);

	if (!IS_NPC(victim))
	{
		sprintf(buf,
				"Возраст %s  : %d лет, %d месяцев, %d дней и %d часов.\r\n",
				GET_PAD(victim, 1), age(victim)->year, age(victim)->month,
				age(victim)->day, age(victim)->hours);
		send_to_char(buf, ch);
	}

	sprintf(buf, "Рост %d(%s%d%s), Вес %d(%s%d%s), Размер %d(%s%d%s)\r\n",
			GET_HEIGHT(victim),
			CCIRED(ch, C_NRM), GET_REAL_HEIGHT(victim), CCNRM(ch, C_NRM),
			GET_WEIGHT(victim),
			CCIRED(ch, C_NRM), GET_REAL_WEIGHT(victim), CCNRM(ch, C_NRM),
			GET_SIZE(victim), CCIRED(ch, C_NRM), GET_REAL_SIZE(victim), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	sprintf(buf,
			"Уровень : %d, может выдержать повреждений : %d(%d,%s%d%s)\r\n",
			GET_LEVEL(victim), GET_HIT(victim), GET_MAX_HIT(victim),
			CCIRED(ch, C_NRM), GET_REAL_MAX_HIT(victim), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	sprintf(buf,
			"Защита от чар : %d, Защита от магических повреждений : %d, Защита от физических повреждений : %d\r\n",
			MIN(GET_AR(victim), 100), MIN(GET_MR(victim), 100), MIN(GET_PR(victim), 100));
	send_to_char(buf, ch);

	sprintf(buf,
			"Защита : %d(%s%d%s), Атака : %d(%s%d%s), Повреждения : %d(%s%d%s)\r\n",
			GET_AC(victim), CCIRED(ch, C_NRM), compute_armor_class(victim),
			CCNRM(ch, C_NRM), GET_HR(victim), CCIRED(ch, C_NRM),
			GET_REAL_HR(victim), CCNRM(ch, C_NRM), GET_DR(victim),
			CCIRED(ch, C_NRM), GET_REAL_DR(victim), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	sprintf(buf, "Сила: %d, Ум: %d, Муд: %d, Ловк: %d, Тел: %d, Обаян: %d\r\n",
			victim->get_inborn_str(), victim->get_inborn_int(), victim->get_inborn_wis(), victim->get_inborn_dex(),
			victim->get_inborn_con(), victim->get_inborn_cha());
	send_to_char(buf, ch);
	sprintf(buf,
			"Сила: %s%d%s, Ум: %s%d%s, Муд: %s%d%s, Ловк: %s%d%s, Тел: %s%d%s, Обаян: %s%d%s\r\n",
			CCIRED(ch, C_NRM), GET_REAL_STR(victim), CCNRM(ch, C_NRM),
			CCIRED(ch, C_NRM), GET_REAL_INT(victim), CCNRM(ch, C_NRM),
			CCIRED(ch, C_NRM), GET_REAL_WIS(victim), CCNRM(ch, C_NRM),
			CCIRED(ch, C_NRM), GET_REAL_DEX(victim), CCNRM(ch, C_NRM),
			CCIRED(ch, C_NRM), GET_REAL_CON(victim), CCNRM(ch, C_NRM),
			CCIRED(ch, C_NRM), GET_REAL_CHA(victim), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);

	int found = FALSE;
	for (const auto& aff : victim->affected)
	{
		if (aff->location != APPLY_NONE && aff->modifier != 0)
		{
			if (!found)
			{
				send_to_char("Дополнительные свойства :\r\n", ch);
				found = TRUE;
				send_to_char(CCIRED(ch, C_NRM), ch);
			}
			sprinttype(aff->location, apply_types, buf2);
			sprintf(buf, "   %s изменяет на %s%d\r\n", buf2, aff->modifier > 0 ? "+" : "", aff->modifier);
			send_to_char(buf, ch);
		}
	}
	send_to_char(CCNRM(ch, C_NRM), ch);

	send_to_char("Аффекты :\r\n", ch);
	send_to_char(CCIBLU(ch, C_NRM), ch);
	victim->char_specials.saved.affected_by.sprintbits(affected_bits, buf2, "\r\n",IS_IMMORTAL(ch)?4:0);
	sprintf(buf, "%s\r\n", buf2);

	if (victim->followers)
	{
		sprintf(buf + strlen(buf), "Имеет последователей.\r\n");
	}
	else if (victim->has_master())
	{
		sprintf(buf + strlen(buf), "Следует за %s.\r\n", GET_PAD(victim->get_master(), 4));
	}

	sprintf(buf + strlen(buf),
		"Уровень повреждений %d, уровень заклинаний %d.\r\n", GET_DAMAGE(victim), GET_CASTER(victim));
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);
}

void skill_identify(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)
{
	if (obj)
	{
		if (IS_IMMORTAL(ch))
		{
			imm_show_obj_values(obj, ch);
		}
		else
		{
			mort_show_obj_values(obj, ch, train_skill(ch, SKILL_IDENTIFY, skill_info[SKILL_IDENTIFY].max_percent, 0));
		}
	}
	else if (victim)
	{
		if (IS_IMMORTAL(ch))
			imm_show_char_values(victim, ch);
		else if (GET_LEVEL(victim) < 3)
		{
			send_to_char("Вы можете опознать только персонажа, достигнувшего третьего уровня.\r\n", ch);
			return;
		}
		mort_show_char_values(victim, ch,
							  train_skill(ch, SKILL_IDENTIFY, skill_info[SKILL_IDENTIFY].max_percent, victim));
	}
}

void spell_identify(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)
{
	if (obj)
		mort_show_obj_values(obj, ch, 100);
	else if (victim)
	{
		if (victim != ch)
		{
			send_to_char("С помощью магии нельзя опознать другое существо.\r\n", ch);
			return;
		}
		if (GET_LEVEL(victim) < 3)
		{
			send_to_char("Вы можете опознать себя только достигнув третьего уровня.\r\n", ch);
			return;
		}
		mort_show_char_values(victim, ch, 100);
	}
}

void spell_control_weather(int/* level*/, CHAR_DATA *ch, CHAR_DATA* /*victim*/, OBJ_DATA* /*obj*/)
{
	const char *sky_info = 0;
	int i, duration, zone, sky_type = 0;

	if (what_sky > SKY_LIGHTNING)
		what_sky = SKY_LIGHTNING;

	switch (what_sky)
	{
	case SKY_CLOUDLESS:
		sky_info = "Небо покрылось облаками.";
		break;
	case SKY_CLOUDY:
		sky_info = "Небо покрылось тяжелыми тучами.";
		break;
	case SKY_RAINING:
		if (time_info.month >= MONTH_MAY && time_info.month <= MONTH_OCTOBER)
		{
			sky_info = "Начался проливной дождь.";
			create_rainsnow(&sky_type, WEATHER_LIGHTRAIN, 0, 50, 50);
		}
		else if (time_info.month >= MONTH_DECEMBER || time_info.month <= MONTH_FEBRUARY)
		{
			sky_info = "Повалил снег.";
			create_rainsnow(&sky_type, WEATHER_LIGHTSNOW, 0, 50, 50);
		}
		else if (time_info.month == MONTH_MART || time_info.month == MONTH_NOVEMBER)
		{
			if (weather_info.temperature > 2)
			{
				sky_info = "Начался проливной дождь.";
				create_rainsnow(&sky_type, WEATHER_LIGHTRAIN, 0, 50, 50);
			}
			else
			{
				sky_info = "Повалил снег.";
				create_rainsnow(&sky_type, WEATHER_LIGHTSNOW, 0, 50, 50);
			}
		}
		break;
	case SKY_LIGHTNING:
		sky_info = "На небе не осталось ни единого облачка.";
		break;
	default:
		break;
	}

	if (sky_info)
	{
		duration = MAX(GET_LEVEL(ch) / 8, 2);
		zone = world[ch->in_room]->zone;
		for (i = FIRST_ROOM; i <= top_of_world; i++)
			if (world[i]->zone == zone && SECT(i) != SECT_INSIDE && SECT(i) != SECT_CITY)
			{
				world[i]->weather.sky = what_sky;
				world[i]->weather.weather_type = sky_type;
				world[i]->weather.duration = duration;
				if (world[i]->first_character())
				{
					act(sky_info, FALSE, world[i]->first_character(), 0, 0, TO_ROOM | TO_ARENA_LISTEN);
					act(sky_info, FALSE, world[i]->first_character(), 0, 0, TO_CHAR);
				}
			}
	}
}

void spell_fear(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA* /*obj*/)
{
	int modi = 0;
	if (ch != victim)
	{
		modi = calc_anti_savings(ch);
		if (!pk_agro_action(ch, victim))
			return;
	}
	if (!IS_NPC(ch) && (GET_LEVEL(ch) > 10))
		modi += (GET_LEVEL(ch) - 10);
	if (PRF_FLAGGED(ch, PRF_AWAKE))
		modi = modi - 50;
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_BLESS))
		modi -= 25;

	if (!MOB_FLAGGED(victim, MOB_NOFEAR) && !general_savingthrow(ch, victim, SAVING_WILL, modi))
		go_flee(victim);
}

void spell_energydrain(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA* /*obj*/)
{
	// истощить энергию - круг 28 уровень 9 (1)
	// для всех
	int modi = 0;
	if (ch != victim)
	{
		modi = calc_anti_savings(ch);
		if (!pk_agro_action(ch, victim))
			return;
	}
	if (!IS_NPC(ch) && (GET_LEVEL(ch) > 10))
		modi += (GET_LEVEL(ch) - 10);
	if (PRF_FLAGGED(ch, PRF_AWAKE))
		modi = modi - 50;

	if (ch == victim || !general_savingthrow(ch, victim, SAVING_WILL, CALC_SUCCESS(modi, 33)))
	{
		int i;
		for (i = 0; i <= MAX_SPELLS; GET_SPELL_MEM(victim, i++) = 0);
		GET_CASTER(victim) = 0;
		send_to_char("Внезапно вы осознали, что у вас напрочь отшибло память.\r\n", victim);
	}
	else
		send_to_char(NOEFFECT, ch);
}

// накачка хитов
void do_sacrifice(CHAR_DATA * ch, int dam)
{
//MZ.overflow_fix
	GET_HIT(ch) = MAX(GET_HIT(ch), MIN(GET_HIT(ch) + MAX(1, dam), GET_REAL_MAX_HIT(ch)
									   + GET_REAL_MAX_HIT(ch) * GET_LEVEL(ch) / 10));
//-MZ.overflow_fix
	update_pos(ch);
}

void spell_sacrifice(int/* level*/, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA* /*obj*/)
{
	int dam, d0 = GET_HIT(victim);
	struct follow_type *f;

	// Высосать жизнь - некроманы - уровень 18 круг 6й (5)
	// *** мин 54 макс 66 (330)

	if (WAITLESS(victim) || victim == ch || IS_CHARMICE(victim))
	{
		send_to_char(NOEFFECT, ch);
		return;
	}

	dam = mag_damage(GET_LEVEL(ch), ch, victim, SPELL_SACRIFICE, SAVING_STABILITY);
	// victim может быть спуржен

	if (dam < 0)
		dam = d0;
	if (dam > d0)
		dam = d0;
	if (dam <= 0)
		return;

	do_sacrifice(ch, dam);
	if (!IS_NPC(ch))
	{
		for (f = ch->followers; f; f = f->next)
		{
			if (IS_NPC(f->follower)
				&& AFF_FLAGGED(f->follower, EAffectFlag::AFF_CHARM)
				&& MOB_FLAGGED(f->follower, MOB_CORPSE)
				&& ch->in_room == IN_ROOM(f->follower))
			{
				do_sacrifice(f->follower, dam);
			}
		}
	}
}

void spell_eviless(int/* level*/, CHAR_DATA *ch, CHAR_DATA* /*victim*/, OBJ_DATA* /*obj*/)
{
	for (const auto tch : world[ch->in_room]->people)
	{
		if (IS_NPC(tch)
			&& tch->get_master() == ch
			&& MOB_FLAGGED(tch, MOB_CORPSE))
		{
			if (mag_affects(GET_LEVEL(ch), ch, tch, SPELL_EVILESS, SAVING_STABILITY))
			{
				GET_HIT(tch) = MAX(GET_HIT(tch), GET_REAL_MAX_HIT(tch));
			}
		}
	}
}

void spell_holystrike(int/* level*/, CHAR_DATA *ch, CHAR_DATA* /*victim*/, OBJ_DATA* /*obj*/)
{
	const char *msg1 = "Земля под вами засветилась и всех поглотил плотный туман.";
	const char *msg2 = "Вдруг туман стал уходить обратно в землю, забирая с собой тела поверженных.";

	act(msg1, FALSE, ch, 0, 0, TO_CHAR);
	act(msg1, FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

	const auto people_copy = world[ch->in_room]->people;
	for (const auto tch : people_copy)
	{
		if (IS_NPC(tch))
		{
			if (!MOB_FLAGGED(tch, MOB_CORPSE)
				&& GET_RACE(tch) != NPC_RACE_ZOMBIE
				&& GET_RACE(tch) != NPC_RACE_EVIL_SPIRIT)
			{
				continue;
			}
		}
		else
		{
			//Чуток нелогично, но раз зомби гоняет -- сам немного мертвяк. :)
			//Тут сам спелл бредовый... Но пока на скорую руку.
			if (!can_use_feat(tch, ZOMBIE_DROVER_FEAT))
			{
				continue;
			}
		}

		mag_affects(GET_LEVEL(ch), ch, tch, SPELL_HOLYSTRIKE, SAVING_STABILITY);
		mag_damage(GET_LEVEL(ch), ch, tch, SPELL_HOLYSTRIKE, SAVING_STABILITY);
	}

	act(msg2, FALSE, ch, 0, 0, TO_CHAR);
	act(msg2, FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

	OBJ_DATA *o = nullptr;
	do
	{
		for (o = world[ch->in_room]->contents; o; o = o->get_next_content())
		{
			if (!IS_CORPSE(o))
			{
				continue;
			}

			extract_obj(o);

			break;
		}
	} while (o);
}

void spell_angel(int/* level*/, CHAR_DATA *ch, CHAR_DATA* /*victim*/, OBJ_DATA* /*obj*/)
{
	mob_vnum mob_num = 108;
	int modifier = 0;
	CHAR_DATA *mob = NULL;
	struct follow_type *k, *k_next;

	auto eff_cha = get_effective_cha(ch);

	for (k = ch->followers; k; k = k_next)
	{
		k_next = k->next;
		if (MOB_FLAGGED(k->follower, MOB_ANGEL))  	//send_to_char("Боги не обратили на вас никакого внимания!\r\n", ch);
		{
			//return;
			//пуржим старого ангела
			stop_follower(k->follower, SF_CHARMLOST);
		}
	}
	if (eff_cha < 16 && !IS_IMMORTAL(ch))
	{
		send_to_char("Боги не обратили на вас никакого внимания!\r\n", ch);
		return;
	};
	if (number(1, 1001) < 500 - 30 * GET_REMORT(ch) && !IS_IMMORTAL(ch))
	{
		send_to_char("Боги только посмеялись над вами!\r\n", ch);
		return;
	};
	if (!(mob = read_mobile(-mob_num, VIRTUAL)))
	{
		send_to_char("Вы точно не помните, как создать данного монстра.\r\n", ch);
		return;
	}
	//reset_char(mob);
	clear_char_skills(mob);
	AFFECT_DATA<EApplyLocation> af;
	af.type = SPELL_CHARM;
	af.duration = pc_duration(mob, 5 + (int) VPOSI<float>((eff_cha - 16.0) / 2, 0, 50), 0, 0, 0, 0);
	af.modifier = 0;
	af.location = EApplyLocation::APPLY_NONE;
	af.battleflag = 0;
	af.bitvector = to_underlying(EAffectFlag::AFF_HELPER);
	affect_to_char(mob, af);
	//AFF_FLAGS(mob).set(EAffectFlag::AFF_FLY);
	af.bitvector = to_underlying(EAffectFlag::AFF_FLY);
	affect_to_char(mob, af);
	//AFF_FLAGS(mob).set(EAffectFlag::AFF_INFRAVISION);
	af.bitvector = to_underlying(EAffectFlag::AFF_INFRAVISION);
	affect_to_char(mob, af);
	if (eff_cha >= 22)
	{
		af.bitvector = to_underlying(EAffectFlag::AFF_SANCTUARY);
		affect_to_char(mob, af);
	}
	if (eff_cha >= 30)
	{
		af.bitvector = to_underlying(EAffectFlag::AFF_AIRSHIELD);
		affect_to_char(mob, af);
	}

	if (IS_FEMALE(ch))
	{
		GET_SEX(mob) = ESex::SEX_MALE;
		mob->set_pc_name("Небесный защитник");
		mob->player_data.PNames[0] = "Небесный защитник";
		mob->player_data.PNames[1] = "Небесного защитника";
		mob->player_data.PNames[2] = "Небесному защитнику";
		mob->player_data.PNames[3] = "Небесного защитника";
		mob->player_data.PNames[4] = "Небесным защитником";
		mob->player_data.PNames[5] = "Небесном защитнике";
		mob->set_npc_name("Небесный защитник");
		mob->player_data.long_descr = str_dup("Небесный защитник летает тут.\r\n");
		mob->player_data.description = str_dup("Сияющая призрачная фигура о двух крылах.\r\n");
	}
	else
	{
		GET_SEX(mob) = ESex::SEX_FEMALE;
		mob->set_pc_name("Небесная защитница");
		mob->player_data.PNames[0] = "Небесная защитница";
		mob->player_data.PNames[1] = "Небесной защитницы";
		mob->player_data.PNames[2] = "Небесной защитнице";
		mob->player_data.PNames[3] = "Небесную защитницу";
		mob->player_data.PNames[4] = "Небесной защитницей";
		mob->player_data.PNames[5] = "Небесной защитнице";
		mob->set_npc_name("Небесная защитница");
		mob->player_data.long_descr = str_dup("Небесная защитница летает тут.\r\n");
		mob->player_data.description = str_dup("Сияющая призрачная фигура о двух крылах.\r\n");
	}

	mob->set_str(11);
	mob->set_dex(16);
	mob->set_con(17);
	mob->set_int(25);
	mob->set_wis(25);
	mob->set_cha(22);

	GET_WEIGHT(mob) = 150;
	GET_HEIGHT(mob) = 200;
	GET_SIZE(mob) = 65;

	GET_HR(mob) = 25;
	GET_AC(mob) = 100;
	GET_DR(mob) = 0;

	mob->mob_specials.damnodice = 1;
	mob->mob_specials.damsizedice = 1;
	mob->mob_specials.ExtraAttack = 1;

	mob->set_exp(0);

	GET_MAX_HIT(mob) = 600;
	GET_HIT(mob) = 600;
	mob->set_gold(0);
	GET_GOLD_NoDs(mob) = 0;
	GET_GOLD_SiDs(mob) = 0;

	GET_POS(mob) = POS_STANDING;
	GET_DEFAULT_POS(mob) = POS_STANDING;

//----------------------------------------------------------------------
	mob->set_skill(SKILL_RESCUE, 65);
	mob->set_skill(SKILL_AWAKE, 50);
	mob->set_skill(SKILL_PUNCH, 50);
	mob->set_skill(SKILL_BLOCK, 50);

	SET_SPELL(mob, SPELL_CURE_BLIND, 1);
	SET_SPELL(mob, SPELL_CURE_CRITIC, 3);
	SET_SPELL(mob, SPELL_REMOVE_HOLD, 1);
	SET_SPELL(mob, SPELL_REMOVE_POISON, 1);

//----------------------------------------------------------------------
	if (mob->get_skill(SKILL_AWAKE))
	{
		PRF_FLAGS(mob).set(PRF_AWAKE);
	}

	GET_LIKES(mob) = 100;
	IS_CARRYING_W(mob) = 0;
	IS_CARRYING_N(mob) = 0;

	MOB_FLAGS(mob).set(MOB_CORPSE);
	MOB_FLAGS(mob).set(MOB_ANGEL);
	MOB_FLAGS(mob).set(MOB_LIGHTBREATH);

	mob->set_level(ch->get_level());
//----------------------------------------------------------------------
// добавляем зависимости от уровня и от обаяния
// level

	modifier = (int)(5 * VPOSI(GET_LEVEL(ch) - 26, 0, 50)
					 + 5 * VPOSI<float>(eff_cha - 16, 0, 50));

	mob->set_skill(SKILL_RESCUE, mob->get_skill(SKILL_RESCUE) + modifier);
	mob->set_skill(SKILL_AWAKE, mob->get_skill(SKILL_AWAKE) + modifier);
	mob->set_skill(SKILL_PUNCH, mob->get_skill(SKILL_PUNCH) + modifier);
	mob->set_skill(SKILL_BLOCK, mob->get_skill(SKILL_BLOCK) + modifier);

	modifier = (int)(2 * VPOSI(GET_LEVEL(ch) - 26, 0, 50)
					 + 1 * VPOSI<float>(eff_cha - 16, 0, 50));
	GET_HR(mob) += modifier;

	modifier = VPOSI(GET_LEVEL(ch) - 26, 0, 50);
	mob->inc_con(modifier);

	modifier = (int)(20 * VPOSI<float>(eff_cha - 16, 0, 50));
	GET_MAX_HIT(mob) += modifier;
	GET_HIT(mob) += modifier;

	modifier = (int)(3 * VPOSI<float>(eff_cha - 16, 0, 50));
	GET_AC(mob) -= modifier;

	modifier = 1 * VPOSI((int)((eff_cha - 16) / 2), 0, 50);
	mob->inc_str(modifier);
	mob->inc_dex(modifier);

	modifier = VPOSI((int)((eff_cha - 22) / 4), 0, 50);
	SET_SPELL(mob, SPELL_HEAL, GET_SPELL_MEM(mob, SPELL_HEAL) + modifier);

	if (eff_cha >= 26)
		mob->mob_specials.ExtraAttack += 1;

	if (eff_cha >= 24)
	{
		mob->mob_specials.damnodice += 1;
		mob->mob_specials.damsizedice += 1;
	}

	char_to_room(mob, ch->in_room);

	if (IS_FEMALE(mob))
	{
		act("Небесная защитница появилась в яркой вспышке света!", TRUE, mob, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	}
	else
	{
		act("Небесный защитник появился в яркой вспышке света!", TRUE, mob, 0, 0, TO_ROOM);
	}
	ch->add_follower(mob);
	return;
}

void spell_vampire(int/* level*/, CHAR_DATA* /*ch*/, CHAR_DATA* /*victim*/, OBJ_DATA* /*obj*/)
{
}

void spell_mental_shadow(int/* level*/, CHAR_DATA* ch, CHAR_DATA* /*victim*/, OBJ_DATA* /*obj*/)
{
	// подготовка контейнера для создания заклинания ментальная тень
	// все предложения пишем мад почтой

	mob_vnum mob_num = MOB_MENTAL_SHADOW;

	CHAR_DATA *mob = NULL;
	struct follow_type *k, *k_next;
	for (k = ch->followers; k; k = k_next)
	{
		k_next = k->next;
		if (MOB_FLAGGED(k->follower, MOB_GHOST))
		{
			stop_follower(k->follower, FALSE);
		}
	}
	if (get_effective_int(ch) < 26 && !IS_IMMORTAL(ch))
	{
		send_to_char("Головные боли мешают работать!\r\n", ch);
		return;
	};

	if (!(mob = read_mobile(-mob_num, VIRTUAL)))
	{
		send_to_char("Вы точно не помните, как создать данного монстра.\r\n", ch);
		return;
	}
	AFFECT_DATA<EApplyLocation> af;
	af.type = SPELL_CHARM;
	af.duration =
		pc_duration(mob, 5 + (int) VPOSI<float>((get_effective_int(ch) - 16.0) / 2, 0, 50), 0, 0, 0, 0);
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = to_underlying(EAffectFlag::AFF_HELPER);
	af.battleflag = 0;
	affect_to_char(mob, af);

	char_to_room(mob, IN_ROOM(ch));
	mob->set_protecting(ch);
	MOB_FLAGS(mob).set(MOB_CORPSE);
	MOB_FLAGS(mob).set(MOB_GHOST);

	act("Мимолётное наваждение воплотилось в призрачную тень.", TRUE, mob, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

	ch->add_follower(mob);
	return;
}

const spell_wear_off_msg_t spell_wear_off_msg =
{
	"RESERVED DB.C",	// 0
	"Вы почувствовали себя менее защищенно.",	// 1
	"!Teleport!",
	"Вы почувствовали себя менее доблестно.",
	"Вы вновь можете видеть.",
	"!Burning Hands!",	// 5
	"!Call Lightning",
	"Вы подчиняетесь теперь только себе.",
	"Вы отметили, что силы вернулись к вам.",
	"!Clone!",
	"!Color Spray!",	// 10
	"!Control Weather!",
	"!Create Food!",
	"!Create Water!",
	"!Cure Blind!",
	"!Cure Critic!",	// 15
	"!Cure Light!",
	"Вы почувствовали себя более уверенно.",
	"Вы более не можете определять наклонности.",
	"Вы не в состоянии больше видеть невидимых.",
	"Вы не в состоянии более определять магию.",	// 20
	"Вы не в состоянии более определять яды.",
	"!Dispel Evil!",
	"!Earthquake!",
	"!Enchant Weapon!",
	"!Energy Drain!",	// 25
	"!Fireball!",
	"!Harm!",
	"!Heal!",
	"Вы вновь видимы.",
	"!Lightning Bolt!",	// 30
	"!Locate object!",
	"!Magic Missile!",
	"В вашей крови не осталось ни капельки яда.",
	"Вы вновь ощущаете страх перед тьмой.",
	"!Remove Curse!",	// 35
	"Белая аура вокруг вашего тела угасла.",
	"!Shocking Grasp!",
	"Вы не чувствуете сонливости.",
	"Вы чувствуете себя немного слабее.",
	"!Summon!",		// 40
	"Вы утратили покровительство высших сил.",
	"!Word of Recall!",
	"!Remove Poison!",
	"Вы больше не можете чувствовать жизнь.",
	"!Animate Dead!",	// 45
	"!Dispel Good!",
	"!Group Armor!",
	"!Group Heal!",
	"!Group Recall!",
	"Вы больше не можете видеть ночью.",	// 50
	"Вы больше не можете ходить по воде.",
	"!SPELL CURE SERIOUS!",
	"!SPELL GROUP STRENGTH!",
	"К вам вернулась способность двигаться.",
	"!SPELL POWER HOLD!",	// 55
	"!SPELL MASS HOLD!",
	"Вы приземлились на землю.",
	"Вы вновь стали уязвимы для оцепенения.",
	"Вы опять можете сбежать с поля боя.",
	"!SPELL CREATE LIGHT!",	// 60
	"Облако тьмы, окружающее вас, спало.",
	"Ваша кожа вновь стала мягкой и бархатистой.",
	"Ваши очертания приобрели отчетливость.",
	"Теперь вы можете болтать, все что думаете.",
	"Ваше тело перестало светиться.",	// 65
	"!SPELL CHAIN LIGHTNING!",
	"!SPELL FIREBLAST!",
	"!SPELL IMPLOSION!",
	"Силы вернулись к вам.",
	"!SPELL GROUP INVISIBLE!",	// 70
	"Ваша теневая мантия замерцала и растаяла.",
	"!SPELL ACID!",
	"!SPELL REPAIR!",
	"Ваши размеры стали прежними.",
	"!SPELL FEAR!",		// 75
	"!SPELL SACRIFICE!",
	"Магическая сеть, покрывавшая вас, исчезла.",
	"Вы перестали мигать.",
	"!SPELL REMOVE HOLD!",
	"Вы стали вновь похожи сами на себя.",	// 80
	"!SPELL POWER BLINDNESS!",
	"!SPELL MASS BLINDNESS!",
	"!SPELL POWER SIELENCE!",
	"!SPELL EXTRA HITS!",
	"!SPELL RESSURECTION!",	// 85
	"Ваш волшебный щит рассеялся.",
	"Магия, запечатывающая входы, пропала.",
	"!SPELL MASS SIELENCE!",
	"!SPELL REMOVE SIELENCE!",
	"!SPELL DAMAGE LIGHT!",	// 90
	"!SPELL DAMAGE SERIOUS!",
	"!SPELL DAMAGE CRITIC!",
	"!SPELL MASS CURSE!",
	"!SPELL ARMAGEDDON!",
	"!SPELL GROUP FLY!",	// 95
	"!SPELL GROUP BLESS!",
	"!SPELL REFRESH!",
	"!SPELL STUNNING!",
	"Вы стали заметны окружающим.",
	"Ваши передвижения стали заметны.",	// 100
	"Кураж прошел. Мама, лучше бы я умер$q вчера.",
	"А головка ваша уже не болит.",
	"Вам снова захотелось жаренького.",
	"Вы согрелись и подвижность вернулась к вам.",
	"К вам вернулась способность нормально сражаться.",	// 105
	"Ваши кровоточащие раны затянулись.",
	"Вы успокоились.",
	"Вы более не способны дышать водой.",
	"Медлительность исчезла.",
	"Вы стали более медлительны.",	// 110
	"!SPELL MASS SLOW!",
	"!SPELL MASS HASTE!",
	"Голубой кокон вокруг вашего тела угас.",
	"Лихорадка прекратилась.",
	"!SPELL CURE PLAQUE!",	// 115
	"Вы стали менее внимательны.",
	"Вы утратили расположение Богов.",
	"Ваш воздушный щит исчез.",
	"!PORTAL!",
	"!DISPELL MAGIC!",	// 120
	"!SUMMON KEEPER!",
	"Живительная сила покинула вас.",
	"!CREATE WEAPON!",
	"Огненный щит вокруг вашего тела исчез.",
	"!RELOCATE!",		// 125
	"!SUMMON FIREKEEPER!",
	"Ледяной щит вокруг вашего тела исчез.",
	"Ваши мышцы оттаяли и вы снова можете двигаться.",
	"Ваши размеры вновь стали прежними.",
	"!SHINE LIGHT!",	// 130
	"Безумие боя отпустило вас.",
	"!GROUP MAGICGLASS!",
	"Облако стрел вокруг вас рассеялось.",
	"!VACUUM!",
	"Последний громовой камень грянул в землю и все стихло.",	// 135 SPELL_METEORSTORM
	"Ваши руки вернулись к прежнему состоянию.",
	"Ваш разум просветлел.",
	"Призматическая аура вокруг вашего тела угасла.",
	"Силы зла оставили вас.",
	"Воздушная аура вокруг вас исчезла.",	// 140
	"Огненная аура вокруг вас исчезла.",
	"Ледяная аура вокруг вас исчезла.",
	"!SHOCK!",
	"Вы вновь чувствительны к магическим поражениям.",
	"!SPELL GROUP SANCTUARY!",	// 145
	"!SPELL GROUP PRISMATICAURA!",
	"Вы вновь можете слышать.",
	"!SPELL_POWER_DEAFNESS!",
	"!SPELL_REMOVE_DEAFNESS!",
	"!SPELL_MASS_DEAFNESS!",	// 150
	"!SPELL_DUSTSTORM!",
	"!SPELL_EARTHFALL!",
	"!SPELL_SONICWAVE!",
	"!SPELL_HOLYSTRIKE!",
	"!SPELL_SPELL_ANGEL!",                 // 155
	"!SPELL_SPELL_MASS_FEAR!",
	"Ваша красота куда-то пропала.",
	"Ваша душа успокоилась.",
	"!SPELL_OBLIVION!",
	"!SPELL_BURDEN_OF_TIME!",        // 160
	"!SPELL_GROUP_REFRESH!",
	"Смирение в вашей душе вдруг куда-то исчезло.",
	"К вам вернулась способность нормально сражаться.",
	"Неистовство оставило вас.",
	"!stone bones!",               // 165
	"Колдовской свет угас.",          // SPELL_ROOM_LIGHT - строка при снятии аффекта
	"Порыв ветра развеял ядовитый туман.",   // SPELL_POISONED_FOG - строка при снятии аффекта
	"Ветер прогнал грозовые тучи.",		 // SPELL_THUNDERSTORM - строка при снятии аффекта
	"Ваши следы вновь стали заметны.",
	"Удача вновь вернулась к вам.",		// 170
	"Магические чары ослабели со временем и покинули вас.",
	"Покрывавшая вас блестящая пыль осыпалась и растаяла в воздухе.", // SPELL_GLITTERDUST
	"Леденящий душу испуг отпустил вас.",
	"Ваши движения утратили прежнюю колдовскую ловкость.",
	"Ваше телосложение вновь стало обычным.",
	"Вы утратили навеянную магией мудрость.",
	"Навеянная магией хитрость покинула вас.",
	"!SPELL_WC_OF_CHALLENGE!",
	"",		// SPELL_WC_OF_MENACE
	"!SPELL_WC_OF_RAGE!",
	"",		// SPELL_WC_OF_MADNESS
	"!SPELL_WC_OF_THUNDER!",
	"Действие клича 'призыв к обороне' закончилось.", //SPELL_WC_OF_DEFENSE
	"Действие клича битвы закончилось.",		// SPELL_WC_OF_BATTLE
	"Действие клича мощи закончилось.",		// SPELL_WC_OF_POWER
	"Действие клича доблести закончилось.",		// SPELL_WC_OF_BLESS
	"Действие клича отваги закончилось.",		// SPELL_WC_OF_COURAGE
	"Магические письмена на земле угасли.",         // SPELL_RUNE_LABEL
	"В вашей крови не осталось ни капельки яда.", // SPELL_ACONITUM_POISON
	"В вашей крови не осталось ни капельки яда.", // SPELL_SCOPOLIA_POISON
	"В вашей крови не осталось ни капельки яда.", // SPELL_BELENA_POISON
	"В вашей крови не осталось ни капельки яда.",  // SPELL_DATURA_POISON
	"SPELL_TIMER_REPAIR",
	"!SPELL_LACKY!",
	"Вы аккуратно перевязали свои раны.",
	"Вы снова можете перевязывать свои раны.",
	"!SPELL_CAPABLE!",
	"Удушье отпустило вас, и вы вздохнули полной грудью.", //SPELL_STRANGLE
	"Вам стало не на чем концентрироваться.", //SPELL_RECALL_SPELLS
	"Плывший в воздухе огненный узор потускнел и растаял струйками дыма.", //SPELL_HYPNOTIC_PATTERN
	"Одна из наград прекратила действовать.", //SPELL_SOLOBONUS
	"!SPELL_VAMPIRE!",
	"!SPELLS_RESTORATION!",
	"Силы нави покинули вас.",
	"!SPELL_RECOVERY!",
	"!SPELL_MASS_RECOVERY!",
	"Аура зла больше не помогает вам.",
	"!SPELL_MENTAL_SHADOW!",                 // 208
	"Жуткие черные руки побледнели и расплылись зловонной дымкой.", //209 SPELL_EVARDS_BLACK_TENTACLES
	"!SPELL_WHIRLWIND!", //210
	"Каменные зубы исчезли, возвратив способность двигаться.",
	"!SPELL_MELFS_ACID_ARROW!",
	"!SPELL_THUNDERSTONE!",
	"!SPELL_CLOD!",
	"Эффект боевого приема завершился.",
	"!SPELL SIGHT OF DARKNESS!",
	"!SPELL GENERAL SINCERITY!",
	"!SPELL MAGICAL GAZE!",
	"!SPELL ALL SEEING EYE!",
	"!SPELL EYE OF GODS!", //220
	"!SPELL BREATHING AT DEPTH!",
	"!SPELL GENERAL RECOVERY!",
	"!SPELL COMMON MEAL!",
	"!SPELL STONE WALL!",
	"!SPELL SNAKE EYES!",
	"Матушка земля забыла про Вас.",
	"Вы вновь ощущаете страх перед тьмой.",
	"!NONE",
	"!NONE",
	"!NONE", //230
	"!NONE",
	"!NONE",
	"*Боевое воодушевление угасло, а с ним и вся жажда подвигов!"
// * в начале строки значит не выводить текст окончания заклинания см void show_spell_off
};

/**
** Only for debug purposes: to trace values of the spell_wear_off_msg array.
*/
void output_spell_wear_off_msg()
{
	unsigned n = 0;
	for (const auto i : spell_wear_off_msg)
	{
		const size_t BUFFER_LENGTH = 4096;
		char buffer[BUFFER_LENGTH];
		const size_t length = std::min(BUFFER_LENGTH, strlen(i));
		strncpy(buffer, i, length + 1);
		koi_to_alt(buffer, static_cast<int>(length));
		printf("%d. %s\n", n++, buffer);
	}
}

const cast_phrases_t cast_phrase =
{
	cast_phrase_t{ "\nRESERVED DB.C", "\n" },	// 0
	cast_phrase_t{ "буде во прибежище", "... Он - помощь наша и защита наша." },
	cast_phrase_t{ "несите ветры", "... дух поднял меня и перенес меня." },
	cast_phrase_t{ "истягну умь крепостию", "... даст блага просящим у Него." },
	cast_phrase_t{ "Чтоб твои зенки вылезли!", "... поразит тебя Господь слепотою." },
	cast_phrase_t{ "узри огонь!", "... простер руку свою к огню." },	// 5
	cast_phrase_t{ "Разрази тебя Перун!", "... и путь для громоносной молнии." },
	cast_phrase_t{ "умь полонить", "... слушай пастыря сваего, и уразумей." },
	cast_phrase_t{ "хладну персты воскладаше", "... которые черны от льда." },
	cast_phrase_t{ "пусть будет много меня", "... и плодились, и весьма умножились." },
	cast_phrase_t{ "хлад и мраз исторгнути", "... и из воды делается лед." },	// 10
	cast_phrase_t{ "стихия подкоряшися", "... власть затворить небо, чтобы не шел дождь." },
	cast_phrase_t{ "будовати снедь", "... это хлеб, который Господь дал вам в пищу." },
	cast_phrase_t{ "напоиши влагой", "... и потекло много воды." },
	cast_phrase_t{ "зряще узрите", "... и прозрят из тьмы и мрака глаза слепых." },
	cast_phrase_t{ "гой еси", "... да зарубцуются гноища твои." },	// 15
	cast_phrase_t{ "малейше целити раны", "... да затянутся раны твои." },
	cast_phrase_t{ "порча", "... проклят ты пред всеми скотами." },
	cast_phrase_t{ "узряще норов", "... и отделит одних от других, как пастырь отделяет овец от козлов." },
	cast_phrase_t{ "взор мечетный", "... ибо нет ничего тайного, что не сделалось бы явным." },
	cast_phrase_t{ "зряще ворожбу", "... покажись, ересь богопротивная." },	// 20
	cast_phrase_t{ "зряще трутизну", "... по плодам их узнаете их." },
	cast_phrase_t{ "долой нощи", "... грешников преследует зло, а праведникам воздается добром." },
	cast_phrase_t{ "земля тутнет", "... в тот же час произошло великое землетрясение." },
	cast_phrase_t{ "ницовати стружие", "... укрепи сталь Божьим перстом." },
	cast_phrase_t{ "преторгоста", "... да иссякнут соки, питающие тело."},	// 25
	cast_phrase_t{ "огненну солнце", "... да ниспадет огонь с неба, и пожрет их." },
	cast_phrase_t{ "згола скверна", "... и жестокою болью во всех костях твоих." },
	cast_phrase_t{ "згола гой еси", "... тебе говорю, встань." },
	cast_phrase_t{ "низовати мечетно", "... ибо видимое временно, а невидимое вечно." },
	cast_phrase_t{ "грянет гром", "... и были громы и молнии." },	// 30
	cast_phrase_t{ "рища, летая умом под облакы", "... ибо всякий просящий получает, и ищущий находит." },
	cast_phrase_t{ "ворожья стрела", "... остры стрелы Твои." },
	cast_phrase_t{ "трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле." },
	cast_phrase_t{ "супостат нощи", "... свет, который в тебе, да убоится тьма." },
	cast_phrase_t{ "изыде порча", "... да простятся тебе прегрешения твои." },	// 35
	cast_phrase_t{ "иже во святых", "... буде святым, аки Господь наш свят." },
	cast_phrase_t{ "воскладше огненну персты", "... и дано буде жечь врагов огнем." },
	cast_phrase_t{ "иже дремлет", "... на веки твои тяжесть покладет." },
	cast_phrase_t{ "будет силен", "... и человек разумный укрепляет силу свою." },
	cast_phrase_t{ "кличу-велю", "... и послали за ним и призвали его." },	// 40
	cast_phrase_t{ "ибо будет угоден Богам", "... ибо спасет людей Своих от грехов их." },
	cast_phrase_t{ "с глаз долой исчезни", "... ступай с миром." },
	cast_phrase_t{ "изыде трутизна", "... именем Божьим, изгнати сгниенье из тела." },
	cast_phrase_t{ "зряще живота", "... ибо нет ничего сокровенного, что не обнаружилось бы." },
	cast_phrase_t{ "живот изо праха створисте", "... и земля извергнет мертвецов." },	// 45
	cast_phrase_t{ "свет сгинь", "... и тьма свет накроет." },
	cast_phrase_t{ "прибежище други", "... ибо кто Бог, кроме Господа, и кто защита, кроме Бога нашего?" },
	cast_phrase_t{ "други, гой еси", "... вам говорю, встаньте." },
	cast_phrase_t{ "исчезните с глаз моих", "... вам говорю, ступайте с миром." },
	cast_phrase_t{ "в нощи зряще", "...ибо ни днем, ни ночью сна не знают очи." },	// 50 - INFRAVISION
	cast_phrase_t{ "по воде аки по суху", "... поднимись и ввергнись в море." },
	cast_phrase_t{ "целите раны", "... да уменьшатся раны твои." },
	cast_phrase_t{ "други сильны", "... и даст нам Господь силу." },
	cast_phrase_t{ "аки околел", "... замри." },
	cast_phrase_t{ "згола аки околел", "... замри надолго." },	// 55
	cast_phrase_t{ "их окалеть", "... замрите." },
	cast_phrase_t{ "летать зегзицею", "... и полетел, и понесся на крыльях ветра." },
	cast_phrase_t{ "вериги и цепи железные изорву", "... и цепи упали с рук его." },
	cast_phrase_t{ "опуташа в путины железны", "... да поищешь уйти, да не возможешь." },
	cast_phrase_t{ "будовати светоч", "... да будет свет." },	// 60
	cast_phrase_t{ "тьмою прикрыты", "... тьма покроет землю." },
	cast_phrase_t{ "буде тверд аки камень", "... твердость ли камней твердость твоя?" },
	cast_phrase_t{ "мгла покрыла", "... будут как утренний туман." },
	cast_phrase_t{ "типун тебе на язык!", "... да замкнутся уста твои." },
	cast_phrase_t{ "буде аки светоч", "... и да воссияет над ним свет!" },	// 65
	cast_phrase_t{ "глаголят небеса", "... понесутся меткие стрелы молний из облаков." },
	cast_phrase_t{ "створисте огненну струя", "... и ввергне их в озеро огненное." },
	cast_phrase_t{ "гнев божиа не минути", "... и воспламенится гнев Господа, и Он скоро истребит тебя." },
	cast_phrase_t{ "буде чахнуть", "... и силу могучих ослабляет." },
	cast_phrase_t{ "други, низовати мечетны", "... возвещай всем великую силу Бога. И, сказав сие, они стали невидимы." },	// 70
	cast_phrase_t{ "будут тени и туман, и мрак ночной", "... распростираются вечерние тени." },
	cast_phrase_t{ "жги аки смола горячая", "... подобно мучению от скорпиона." },
	cast_phrase_t{ "будь целым, аки прежде", "... заделаю трещины в ней и разрушенное восстановлю." },
	cast_phrase_t{ "возросши к небу", "... и плоть выросла." },
	cast_phrase_t{ "падоша в тернии", "... убойся того, кто по убиении ввергнет в геенну." },	// 75
	cast_phrase_t{ "да коснется тебя Чернобог", "... плоть твоя и тело твое будут истощены." },
	cast_phrase_t{ "сети ловчи", "... терны и сети на пути коварного." },
	cast_phrase_t{ "от стрел укрытие и от меча оборона", "...да защитит он себя." },
	cast_phrase_t{ "буде быстр аки прежде", "... встань, и ходи." },
	cast_phrase_t{ "\n!Маскировка!", "\n" },	// 80
	cast_phrase_t{ "згола застить очеса", "... поразит тебя Господь слепотою навечно." },
	cast_phrase_t{ "их очеса непотребны", "... и Он поразил их слепотою." },
	cast_phrase_t{ "згола не прерчет", "... исходящее из уст твоих, да не осквернит слуха." },
	cast_phrase_t{ "буде полон здоровья", "... крепкое тело лучше несметного богатства." },
	cast_phrase_t{ "воскресе из мертвых", "... оживут мертвецы Твои, восстанут мертвые тела!" },	// 85
	cast_phrase_t{ "и ворога оберегись", "... руками своими да защитит он себя" },
	cast_phrase_t{ "вороги не войдут", "... ибо положена печать, и никто не возвращается." },
	cast_phrase_t{ "их уста непотребны", "... да замкнутся уста ваши." },
	cast_phrase_t{ "глаголите", "... слова из уст мудрого - благодать." },
	cast_phrase_t{ "падош", "... будет чувствовать боль." },	// 90
	cast_phrase_t{ "скверна", "... постигнут тебя муки." },
	cast_phrase_t{ "сильна скверна", "... боль и муки схватили." },
	cast_phrase_t{ "порча их", "... прокляты вы пред всеми скотами." },
	cast_phrase_t{ "суд божиа не минути", "... какою мерою мерите, такою отмерено будет и вам." },
	cast_phrase_t{ "крыла им створисте", "... и все летающие по роду их." },	// 95
	cast_phrase_t{ "други, наполнися ратнаго духа", "... блажены те, слышащие слово Божие." },
	cast_phrase_t{ "буде свеж", "... не будет у него ни усталого, ни изнемогающего." },
	cast_phrase_t{ "да обратит тебя Чернобог в мертвый камень!", "... и проклял его именем Господним." },
	cast_phrase_t{ "\n!Спрятался!", "\n" },
	cast_phrase_t{ "\n!Крадется!", "\n" },	// 100
	cast_phrase_t{ "\n!Опьянение!", "\n" },
	cast_phrase_t{ "\n!Абстиненция!", "\n" },
	cast_phrase_t{ "брюхо полно", "... душа больше пищи, и тело - одежды." },
	cast_phrase_t{ "веют ветры", "... подует северный холодный ветер." },
	cast_phrase_t{ "\n!Получил в бою!", "\n" },	// 105
	cast_phrase_t{ "\n!Кровотечение!", "\n" },
	cast_phrase_t{ "\n!Ярость!", "\n" },
	cast_phrase_t{ "не затвори темне березе", "... дух дышит, где хочет." },
	cast_phrase_t{ "немочь", "...и помедлил еще семь дней других." },
	cast_phrase_t{ "скор аки ястреб", "... поднимет его ветер и понесет, и он быстро побежит от него." },	// 110
	cast_phrase_t{ "тернии им", "... загорожу путь их тернами." },
	cast_phrase_t{ "быстры аки ястребов стая", "... и они быстры как серны на горах." },
	cast_phrase_t{ "Живый в помощи Вышняго", "... благословен буде Грядый во имя Господне." },
	cast_phrase_t{ "нутро снеде", "... и сделаются жестокие и кровавые язвы." },
	cast_phrase_t{ "Навь, очисти тело", "... хочу, очистись." },	// 115
	cast_phrase_t{ "око недреманно", "... не дам сна очам моим и веждам моим - дремания." },
	cast_phrase_t{ "\n!молитва или жертва!", "\n" },
	cast_phrase_t{ "Стрибог, даруй прибежище", "... защита от ветра и покров от непогоды." },
	cast_phrase_t{ "буде путь короток", "... входите во врата Его." },
	cast_phrase_t{ "изыде ворожба", "... выйди, дух нечистый." },	// 120
	cast_phrase_t{ "Сварог, даруй защитника", "... и благословен защитник мой!" },
	cast_phrase_t{ "заживет, аки на собаке", "... нет богатства лучше телесного здоровья." },
	cast_phrase_t{ "будовати стружие", "...вооружите из себя людей на войну" },
	cast_phrase_t{ "Хорс, даруй прибежище", "... душа горячая, как пылающий огонь." },
	cast_phrase_t{ "Стрибог, укажи путь...", "... указывай им путь, по которому они должны идти." },	// 125
	cast_phrase_t{ "Дажьбог, даруй защитника", "... Ангел Мой с вами, и он защитник душ ваших." },
	cast_phrase_t{ "Морена, даруй прибежище", "... а снег и лед выдерживали огонь и не таяли." },
	cast_phrase_t{ "торже, яко вихор", "... и град, величиною в талант, падет с неба." },
	cast_phrase_t{ "буде мал аки мышь", "... плоть на нем пропадает." },
	cast_phrase_t{ "засти очи им", "... свет пламени из средины огня." }, // 130
	cast_phrase_t{ "згола яростен", "... и ярость его загорелась в нем." },
	cast_phrase_t{ "гладь воды отразит", "... воздай им по делам их, по злым поступкам их." },
	cast_phrase_t{ "и будут стрелы молний, и зарницы в высях", "... соберу на них бедствия и истощу на них стрелы Мои." },
	cast_phrase_t{ "Умри!", "... и услышав слова сии - пал бездыханен." },
	cast_phrase_t{ "идти дождю стрелами", "... и камни, величиною в талант, падут с неба." }, // 135
	cast_phrase_t{ "сильны велетов руки", "... рука Моя пребудет с ним, и мышца Моя укрепит его." },
	cast_phrase_t{ "разум аки мутный омут", "... и безумие его с ним." },
	cast_phrase_t{ "окружен радугой", "... явится радуга в облаке." },
	cast_phrase_t{ "зло творяще", "... и ты воздашь им злом." },
	cast_phrase_t{ "Мать-земля, даруй защиту.", "... поклон тебе матушка земля." },	// 140
	cast_phrase_t{ "Сварог, даруй защиту.", "... и огонь низводит с неба." },
	cast_phrase_t{ "Морена, даруй защиту.", "... текущие холодные воды." },
	cast_phrase_t{ "будет слеп и глух, аки мертвец", "... кто делает или глухим, или слепым." },
	cast_phrase_t{ "Аз воздам!", "... и воздам каждому из вас." },
	cast_phrase_t{ "иже во святых, други", "... будьте святы, аки Господь наш свят." },
	cast_phrase_t{ "други, буде окружены радугой", "... взгляни на радугу, и прославь Сотворившего ее." },
	cast_phrase_t{ "оглохни", "... и глухота поразит тебя." },	//SPELL_DEAFNESS
	cast_phrase_t{ "да застит уши твои", "... и будь глухим надолго." },	//SPELL_POWER_DEAFNESS
	cast_phrase_t{ "слушай глас мой", "... услышь слово Его." },	//SPELL_REMOVE_DEAFNESS
	cast_phrase_t{ "будьте глухи", "... и не будут слышать уши ваши." },	//SPELL_MASS_DEAFNESS
	cast_phrase_t{ "пыль поднимется столбами", "... и пыль поглотит вас." },	//SPELL_DUSTSTORM
	cast_phrase_t{ "пусть каменья падут", "... и обрушатся камни с небес." },	//SPELL_EARTHFALL
	cast_phrase_t{ "да невзлюбит тебя воздух", "... и даже воздух покарает тебя." },	//SPELL_SONICWAVE
	cast_phrase_t{ "Велес, упокой мертвых", "... и предоставь мертвым погребать своих мертвецов." },	//SPELL_HOLYSTRIKE
	cast_phrase_t{ "Боги, даруйте защитника", "... дабы уберег он меня от зла." },	//SPELL_ANGEL
	cast_phrase_t{ "Поврещу сташивые души их в скарядие!", "... и затмил ужас разум их." },	//SPELL_MASS_FEAR
	cast_phrase_t{ "Да пребудет с тобой вся краса мира!", "... и омолодил он, и украсил их." },
	cast_phrase_t{ "Будут слезы твои, аки камень на сердце", "... и постигнет твой дух угнетение вечное." },	//SPELL_CRYING
	cast_phrase_t{ "будь живот аки буява с шерстнями.", /* Добавлено. Далим. */ "... опадет на тебя чернь страшная." },	// SPELL_OBLIVION
	cast_phrase_t{ "Яко небытие нещадно к вам, али время вернулось вспять.",	/* Добавлено. Далим. */ "... и время не властно над ними." },	// SPELL_BURDEN_OF_TIME
	cast_phrase_t{ "Исполняше други силою!", "...да не останется ни обделенного ни обессиленого." },	//SPELL_GROUP_REFRESH
	cast_phrase_t{ "Избавь речь свою от недобрых слов, а ум - от крамольных мыслей.", "... любите врагов ваших и благотворите ненавидящим вас." },
	cast_phrase_t{ "\n!получил в бою!", "\n" },
	cast_phrase_t{ "\n!Предсмертная ярость!", "\n" },
	cast_phrase_t{ "Обращу кости их в твердый камень.", "...и тот, кто упадет на камень сей, разобьется." }, // SPELL_STONE_BONE
	cast_phrase_t{ "Да буде СВЕТ !!!", "...ибо сказал МОНТЕР !!!" }, // SPELL_ROOM_LIGHT
	cast_phrase_t{ "Порчу воздух !!!", "...и зловонное дыхание его." }, // SPELL_POISONED_FOG
	cast_phrase_t{ "Абие велий вихрь деяти!", "...творит молнии при дожде, изводит ветер из хранилищ Своих." }, // SPELL_THUNDERSTORM
	cast_phrase_t{ "\n!легкая поступь!", "\n" },
	cast_phrase_t{ "аще доля зла и удача немилостива", ".. и несчастен, и жалок, и нищ." },
	cast_phrase_t{ "\n!кланаффект!", "\n" },
	cast_phrase_t{ "зрети супостат охабиша", "...и бросали пыль на воздух." }, //SPELL_GLITTERDUST
	cast_phrase_t{ "язвень голки уведати", "...но в полночь раздался крик." }, //SPELL_SCREAM
	cast_phrase_t{ "ристати споро", "...и не уязвит враг того, кто скор." }, //SPELL_CATS_GRACE
	cast_phrase_t{ "руци яре ворога супротив", "...и мощь звериная жила в теле его." },
	cast_phrase_t{ "веси и зрети стези отай", "...и даровал мудрость ему." },
	cast_phrase_t{ "клюка вящего улучити", "...ибо кто познал ум Господень?" },
	cast_phrase_t{ "Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!", "Эй, псы шелудивые, керасти плешивые, ослопы беспорточные, мшицы задочные!" },	// клич вызова
	cast_phrase_t{ "Покрошу-изувечу, душу выну и в блины закатаю!", "Покрошу-изувечу, душу выну и в блины закатаю!" }, // клич угрозы
	cast_phrase_t{ "Не отступим, други, они наше сало сперли!", "Не отступим, други, они наше сало сперли!" }, // клич ярости
	cast_phrase_t{ "Всех убью, а сам$g останусь!", "Всех убью, а сам$g останусь!" },	// клич устрашения
	cast_phrase_t{ "Шоб вас приподняло, да шлепнуло!!!", "Шоб вас приподняло да шлепнуло!!!" },
	cast_phrase_t{ "В строй други, защитим животами Русь нашу!", "В строй други, защитим животами Русь нашу!" }, // клич призыв к обороне
	cast_phrase_t{ "Дер-ржать строй, волчьи хвосты!", "Дер-ржать строй, волчьи хвосты!" },	// клич битвы
	cast_phrase_t{ "Сарынь на кичку!", "Сарынь на кичку!" },
	cast_phrase_t{ "Стоять крепко! За нами Киев, Подол и трактир с пивом!!!", "Стоять крепко! За нами Киев, Подол и трактир с пивом!!!" },
	cast_phrase_t{ "Орлы! Будем биться как львы!", "Орлы! Будем биться как львы!" }, //SPELL_WC_OF_COURAGE
	cast_phrase_t{ "...пьсати черты и резы.", "...и Сам отошел от них на вержение камня." }, // SPELL_MAGIC_LABEL
	cast_phrase_t{ "трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле." }, // SPELL_ACONITUM_POISON
	cast_phrase_t{ "трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле." }, // SPELL_SCOPOLIA_POISON
	cast_phrase_t{ "трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле." }, // SPELL_BELENA_POISON
	cast_phrase_t{ "трутизна", "... и пошлю на них зубы зверей и яд ползающих по земле." }, // SPELL_DATURA_POISON
	cast_phrase_t{ "\n", "\n" }, // SPELL_TIMER_REPAIR
	cast_phrase_t{ "\n", "\n" }, // SPELL_LACKY
	cast_phrase_t{ "\n", "\n" }, // SPELL_BANDAGE
	cast_phrase_t{ "\n", "\n" }, // SPELL_NO_BANDAGE
	cast_phrase_t{ "\n", "\n" }, // SPELL_CAPABLE
	cast_phrase_t{ "\n", "\n" }, // SPELL_STRANGLE
	cast_phrase_t{ "\n", "\n" }, // SPELL_RECALL_SPELLS
	cast_phrase_t{ "ажбо супостаты блазнити да клюковати", "...и утроба его приготовляет обман." }, //SPELL_HYPNOTIC_PATTERN
	cast_phrase_t{ "\n", "\n" }, // SPELL_SOLOBONUS
	cast_phrase_t{ "\n", "\n" }, // SPELL_VAMPIRE
	cast_phrase_t{ "Да прими вид прежний, якой был.", ".. Воззри на предмет сей Отче и верни ему силу прежнюю." }, // SPELLS_RESTORATION
	cast_phrase_t{ "Надели силою своею Навь, дабы собрать урожай тебе.", "...налякай ворогов наших и покарай их немощью." }, // SPELL_AURA_DEATH
	cast_phrase_t{ "Обрасти плотью сызнова.", "... прости Господи грехи, верни плоть созданию." }, // SPELL_RECOVERY
	cast_phrase_t{ "Обрастите плотью сызнова.", "... прости Господи грехи, верни плоть созданиям." }, // SPELL_MASS_RECOVERY
	cast_phrase_t{ "Возьми личину зла для жатвы славной.", "Надели силой злою во благо." }, // SPELL_AURA_EVIL
	cast_phrase_t{ "Силою мысли защиту будую себе.", "Даруй Отче защиту, силой разума воздвигнутую." }, // SPELL_MENTAL_SHADOW
	cast_phrase_t{ "Ато егоже руци попасти.", "И он не знает, что мертвецы там и что в глубине..." }, // SPELL_EVARDS_BLACK_TENTACLES
	cast_phrase_t{ "Вждати бурю обло створити.", "И поднялась великая буря..." }, // SPELL_WHIRLWIND
	cast_phrase_t{ "Идеже индрика зубы супостаты изъмати.", "Есть род, у которого зубы - мечи и челюсти - ножи..." }, // SPELL_WHIRLWIND
	cast_phrase_t{ "Варно сожжет струя!", "...и на коже его сделаются как бы язвы проказы" }, // SPELL_MELFS_ACID_ARROW
	cast_phrase_t{ "Небесе тутнет!", "...и взял оттуда камень, и бросил из пращи." }, // SPELL_THUNDERSTONE
	cast_phrase_t{ "Онома утес низринется!", "...доколе камень не оторвался от горы без содействия рук." }, // SPELL_CLODd
	cast_phrase_t{ "!Применил боевой прием!", "!use battle expedient!" }, // SPELL_EXPEDIENT (set by program)
	cast_phrase_t{ "Что свет, что тьма - глазу однаково.", "Станьте зрячи в тьме кромешной!" }, // SPELL_SIGHT_OF_DARKNESS
	cast_phrase_t{ "...да не скроются намерения.", "И узрим братья намерения окружающих." }, // SPELL_GENERAL_SINCERITY
	cast_phrase_t{ "Узрим же все, что с магией навкруги нас.", "Покажи, Спаситель, магические силы братии." }, // SPELL_MAGICAL_GAZE
	cast_phrase_t{ "Все тайное станет явным.", "Не спрячется, не скроется, ни заяц, ни блоха." }, // SPELL_ALL_SEEING_EYE
	cast_phrase_t{ "Осязаемое откройся взору!", "Да не скроется от взора вашего, ни одна живая душа." }, // SPELL_EYE_OF_GODS
	cast_phrase_t{ "Аки стайка рыбок, плывите вместе.", "Что в воде, что на земле, воздух свежим будет." }, // SPELL_BREATHING_AT_DEPTH
	cast_phrase_t{ "...дабы пройти вместе не одну сотню верст", "Сохрани Отче от усталости детей своих!" }, // SPELL_GENERAL_RECOVERY
	cast_phrase_t{ "Благодарите богов за хлеб и соль!", "...дабы не осталось голодающих на свете белом" }, // SPELL_COMMON_MEAL
	cast_phrase_t{ "Станем други крепки як николы!", "Укрепим тела наши перед битвой!" }, // SPELL_STONE_WALL
	cast_phrase_t{ "Что яд, а что мед. Не обманемся!", "...и самый сильный яд станет вам виден." }, // SPELL_SNAKE_EYES
	cast_phrase_t{ "Велес, даруй защиту.", "... земля благословенна твоя." }, // SPELL_EARTH_AURA
	cast_phrase_t{ "други, супостат нощи", "други, свет который в тебе, да убоится тьма." }, // SPELL_GROUP_PROT_FROM_EVIL
	cast_phrase_t{ "!магический выстрел!", "!use battle expedient!" }, // SPELL_ARROWS_FIRE (set by program)
	cast_phrase_t{ "!магический выстрел!", "!use battle expedient!" }, // SPELL_ARROWS_ (set by program)
	cast_phrase_t{ "!магический выстрел!", "!use battle expedient!" }, // SPELL_ARROWS_ (set by program)
	cast_phrase_t{ "!магический выстрел!", "!use battle expedient!" }, // SPELL_ARROWS_ (set by program)
	cast_phrase_t{ "!магический выстрел!", "!use battle expedient!" }, // SPELL_ARROWS_DEATH (set by program)
	cast_phrase_t{ "Воодушевление", "!set by programm!" }, // воодушевление
};

typedef std::map<ESpell, std::string> ESpell_name_by_value_t;
typedef std::map<const std::string, ESpell> ESpell_value_by_name_t;
ESpell_name_by_value_t ESpell_name_by_value;
ESpell_value_by_name_t ESpell_value_by_name;
void init_ESpell_ITEM_NAMES()
{
	ESpell_value_by_name.clear();
	ESpell_name_by_value.clear();

	ESpell_name_by_value[ESpell::SPELL_NO_SPELL] = "SPELL_NO_SPELL";
	ESpell_name_by_value[ESpell::SPELL_ARMOR] = "SPELL_ARMOR";
	ESpell_name_by_value[ESpell::SPELL_TELEPORT] = "SPELL_TELEPORT";
	ESpell_name_by_value[ESpell::SPELL_BLESS] = "SPELL_BLESS";
	ESpell_name_by_value[ESpell::SPELL_BLINDNESS] = "SPELL_BLINDNESS";
	ESpell_name_by_value[ESpell::SPELL_BURNING_HANDS] = "SPELL_BURNING_HANDS";
	ESpell_name_by_value[ESpell::SPELL_CALL_LIGHTNING] = "SPELL_CALL_LIGHTNING";
	ESpell_name_by_value[ESpell::SPELL_CHARM] = "SPELL_CHARM";
	ESpell_name_by_value[ESpell::SPELL_CHILL_TOUCH] = "SPELL_CHILL_TOUCH";
	ESpell_name_by_value[ESpell::SPELL_CLONE] = "SPELL_CLONE";
	ESpell_name_by_value[ESpell::SPELL_COLOR_SPRAY] = "SPELL_COLOR_SPRAY";
	ESpell_name_by_value[ESpell::SPELL_CONTROL_WEATHER] = "SPELL_CONTROL_WEATHER";
	ESpell_name_by_value[ESpell::SPELL_CREATE_FOOD] = "SPELL_CREATE_FOOD";
	ESpell_name_by_value[ESpell::SPELL_CREATE_WATER] = "SPELL_CREATE_WATER";
	ESpell_name_by_value[ESpell::SPELL_CURE_BLIND] = "SPELL_CURE_BLIND";
	ESpell_name_by_value[ESpell::SPELL_CURE_CRITIC] = "SPELL_CURE_CRITIC";
	ESpell_name_by_value[ESpell::SPELL_CURE_LIGHT] = "SPELL_CURE_LIGHT";
	ESpell_name_by_value[ESpell::SPELL_CURSE] = "SPELL_CURSE";
	ESpell_name_by_value[ESpell::SPELL_DETECT_ALIGN] = "SPELL_DETECT_ALIGN";
	ESpell_name_by_value[ESpell::SPELL_DETECT_INVIS] = "SPELL_DETECT_INVIS";
	ESpell_name_by_value[ESpell::SPELL_DETECT_MAGIC] = "SPELL_DETECT_MAGIC";
	ESpell_name_by_value[ESpell::SPELL_DETECT_POISON] = "SPELL_DETECT_POISON";
	ESpell_name_by_value[ESpell::SPELL_DISPEL_EVIL] = "SPELL_DISPEL_EVIL";
	ESpell_name_by_value[ESpell::SPELL_EARTHQUAKE] = "SPELL_EARTHQUAKE";
	ESpell_name_by_value[ESpell::SPELL_ENCHANT_WEAPON] = "SPELL_ENCHANT_WEAPON";
	ESpell_name_by_value[ESpell::SPELL_ENERGY_DRAIN] = "SPELL_ENERGY_DRAIN";
	ESpell_name_by_value[ESpell::SPELL_FIREBALL] = "SPELL_FIREBALL";
	ESpell_name_by_value[ESpell::SPELL_HARM] = "SPELL_HARM";
	ESpell_name_by_value[ESpell::SPELL_HEAL] = "SPELL_HEAL";
	ESpell_name_by_value[ESpell::SPELL_INVISIBLE] = "SPELL_INVISIBLE";
	ESpell_name_by_value[ESpell::SPELL_LIGHTNING_BOLT] = "SPELL_LIGHTNING_BOLT";
	ESpell_name_by_value[ESpell::SPELL_LOCATE_OBJECT] = "SPELL_LOCATE_OBJECT";
	ESpell_name_by_value[ESpell::SPELL_MAGIC_MISSILE] = "SPELL_MAGIC_MISSILE";
	ESpell_name_by_value[ESpell::SPELL_POISON] = "SPELL_POISON";
	ESpell_name_by_value[ESpell::SPELL_PROT_FROM_EVIL] = "SPELL_PROT_FROM_EVIL";
	ESpell_name_by_value[ESpell::SPELL_REMOVE_CURSE] = "SPELL_REMOVE_CURSE";
	ESpell_name_by_value[ESpell::SPELL_SANCTUARY] = "SPELL_SANCTUARY";
	ESpell_name_by_value[ESpell::SPELL_SHOCKING_GRASP] = "SPELL_SHOCKING_GRASP";
	ESpell_name_by_value[ESpell::SPELL_SLEEP] = "SPELL_SLEEP";
	ESpell_name_by_value[ESpell::SPELL_STRENGTH] = "SPELL_STRENGTH";
	ESpell_name_by_value[ESpell::SPELL_SUMMON] = "SPELL_SUMMON";
	ESpell_name_by_value[ESpell::SPELL_PATRONAGE] = "SPELL_PATRONAGE";
	ESpell_name_by_value[ESpell::SPELL_WORD_OF_RECALL] = "SPELL_WORD_OF_RECALL";
	ESpell_name_by_value[ESpell::SPELL_REMOVE_POISON] = "SPELL_REMOVE_POISON";
	ESpell_name_by_value[ESpell::SPELL_SENSE_LIFE] = "SPELL_SENSE_LIFE";
	ESpell_name_by_value[ESpell::SPELL_ANIMATE_DEAD] = "SPELL_ANIMATE_DEAD";
	ESpell_name_by_value[ESpell::SPELL_DISPEL_GOOD] = "SPELL_DISPEL_GOOD";
	ESpell_name_by_value[ESpell::SPELL_GROUP_ARMOR] = "SPELL_GROUP_ARMOR";
	ESpell_name_by_value[ESpell::SPELL_GROUP_HEAL] = "SPELL_GROUP_HEAL";
	ESpell_name_by_value[ESpell::SPELL_GROUP_RECALL] = "SPELL_GROUP_RECALL";
	ESpell_name_by_value[ESpell::SPELL_INFRAVISION] = "SPELL_INFRAVISION";
	ESpell_name_by_value[ESpell::SPELL_WATERWALK] = "SPELL_WATERWALK";
	ESpell_name_by_value[ESpell::SPELL_CURE_SERIOUS] = "SPELL_CURE_SERIOUS";
	ESpell_name_by_value[ESpell::SPELL_GROUP_STRENGTH] = "SPELL_GROUP_STRENGTH";
	ESpell_name_by_value[ESpell::SPELL_HOLD] = "SPELL_HOLD";
	ESpell_name_by_value[ESpell::SPELL_POWER_HOLD] = "SPELL_POWER_HOLD";
	ESpell_name_by_value[ESpell::SPELL_MASS_HOLD] = "SPELL_MASS_HOLD";
	ESpell_name_by_value[ESpell::SPELL_FLY] = "SPELL_FLY";
	ESpell_name_by_value[ESpell::SPELL_BROKEN_CHAINS] = "SPELL_BROKEN_CHAINS";
	ESpell_name_by_value[ESpell::SPELL_NOFLEE] = "SPELL_NOFLEE";
	ESpell_name_by_value[ESpell::SPELL_CREATE_LIGHT] = "SPELL_CREATE_LIGHT";
	ESpell_name_by_value[ESpell::SPELL_DARKNESS] = "SPELL_DARKNESS";
	ESpell_name_by_value[ESpell::SPELL_STONESKIN] = "SPELL_STONESKIN";
	ESpell_name_by_value[ESpell::SPELL_CLOUDLY] = "SPELL_CLOUDLY";
	ESpell_name_by_value[ESpell::SPELL_SILENCE] = "SPELL_SILENCE";
	ESpell_name_by_value[ESpell::SPELL_LIGHT] = "SPELL_LIGHT";
	ESpell_name_by_value[ESpell::SPELL_CHAIN_LIGHTNING] = "SPELL_CHAIN_LIGHTNING";
	ESpell_name_by_value[ESpell::SPELL_FIREBLAST] = "SPELL_FIREBLAST";
	ESpell_name_by_value[ESpell::SPELL_IMPLOSION] = "SPELL_IMPLOSION";
	ESpell_name_by_value[ESpell::SPELL_WEAKNESS] = "SPELL_WEAKNESS";
	ESpell_name_by_value[ESpell::SPELL_GROUP_INVISIBLE] = "SPELL_GROUP_INVISIBLE";
	ESpell_name_by_value[ESpell::SPELL_SHADOW_CLOAK] = "SPELL_SHADOW_CLOAK";
	ESpell_name_by_value[ESpell::SPELL_ACID] = "SPELL_ACID";
	ESpell_name_by_value[ESpell::SPELL_REPAIR] = "SPELL_REPAIR";
	ESpell_name_by_value[ESpell::SPELL_ENLARGE] = "SPELL_ENLARGE";
	ESpell_name_by_value[ESpell::SPELL_FEAR] = "SPELL_FEAR";
	ESpell_name_by_value[ESpell::SPELL_SACRIFICE] = "SPELL_SACRIFICE";
	ESpell_name_by_value[ESpell::SPELL_WEB] = "SPELL_WEB";
	ESpell_name_by_value[ESpell::SPELL_BLINK] = "SPELL_BLINK";
	ESpell_name_by_value[ESpell::SPELL_REMOVE_HOLD] = "SPELL_REMOVE_HOLD";
	ESpell_name_by_value[ESpell::SPELL_CAMOUFLAGE] = "SPELL_CAMOUFLAGE";
	ESpell_name_by_value[ESpell::SPELL_POWER_BLINDNESS] = "SPELL_POWER_BLINDNESS";
	ESpell_name_by_value[ESpell::SPELL_MASS_BLINDNESS] = "SPELL_MASS_BLINDNESS";
	ESpell_name_by_value[ESpell::SPELL_POWER_SILENCE] = "SPELL_POWER_SILENCE";
	ESpell_name_by_value[ESpell::SPELL_EXTRA_HITS] = "SPELL_EXTRA_HITS";
	ESpell_name_by_value[ESpell::SPELL_RESSURECTION] = "SPELL_RESSURECTION";
	ESpell_name_by_value[ESpell::SPELL_MAGICSHIELD] = "SPELL_MAGICSHIELD";
	ESpell_name_by_value[ESpell::SPELL_FORBIDDEN] = "SPELL_FORBIDDEN";
	ESpell_name_by_value[ESpell::SPELL_MASS_SILENCE] = "SPELL_MASS_SILENCE";
	ESpell_name_by_value[ESpell::SPELL_REMOVE_SILENCE] = "SPELL_REMOVE_SILENCE";
	ESpell_name_by_value[ESpell::SPELL_DAMAGE_LIGHT] = "SPELL_DAMAGE_LIGHT";
	ESpell_name_by_value[ESpell::SPELL_DAMAGE_SERIOUS] = "SPELL_DAMAGE_SERIOUS";
	ESpell_name_by_value[ESpell::SPELL_DAMAGE_CRITIC] = "SPELL_DAMAGE_CRITIC";
	ESpell_name_by_value[ESpell::SPELL_MASS_CURSE] = "SPELL_MASS_CURSE";
	ESpell_name_by_value[ESpell::SPELL_ARMAGEDDON] = "SPELL_ARMAGEDDON";
	ESpell_name_by_value[ESpell::SPELL_GROUP_FLY] = "SPELL_GROUP_FLY";
	ESpell_name_by_value[ESpell::SPELL_GROUP_BLESS] = "SPELL_GROUP_BLESS";
	ESpell_name_by_value[ESpell::SPELL_REFRESH] = "SPELL_REFRESH";
	ESpell_name_by_value[ESpell::SPELL_STUNNING] = "SPELL_STUNNING";
	ESpell_name_by_value[ESpell::SPELL_HIDE] = "SPELL_HIDE";
	ESpell_name_by_value[ESpell::SPELL_SNEAK] = "SPELL_SNEAK";
	ESpell_name_by_value[ESpell::SPELL_DRUNKED] = "SPELL_DRUNKED";
	ESpell_name_by_value[ESpell::SPELL_ABSTINENT] = "SPELL_ABSTINENT";
	ESpell_name_by_value[ESpell::SPELL_FULL] = "SPELL_FULL";
	ESpell_name_by_value[ESpell::SPELL_CONE_OF_COLD] = "SPELL_CONE_OF_COLD";
	ESpell_name_by_value[ESpell::SPELL_BATTLE] = "SPELL_BATTLE";
	ESpell_name_by_value[ESpell::SPELL_HAEMORRAGIA] = "SPELL_HAEMORRAGIA";
	ESpell_name_by_value[ESpell::SPELL_COURAGE] = "SPELL_COURAGE";
	ESpell_name_by_value[ESpell::SPELL_WATERBREATH] = "SPELL_WATERBREATH";
	ESpell_name_by_value[ESpell::SPELL_SLOW] = "SPELL_SLOW";
	ESpell_name_by_value[ESpell::SPELL_HASTE] = "SPELL_HASTE";
	ESpell_name_by_value[ESpell::SPELL_MASS_SLOW] = "SPELL_MASS_SLOW";
	ESpell_name_by_value[ESpell::SPELL_GROUP_HASTE] = "SPELL_GROUP_HASTE";
	ESpell_name_by_value[ESpell::SPELL_SHIELD] = "SPELL_SHIELD";
	ESpell_name_by_value[ESpell::SPELL_PLAQUE] = "SPELL_PLAQUE";
	ESpell_name_by_value[ESpell::SPELL_CURE_PLAQUE] = "SPELL_CURE_PLAQUE";
	ESpell_name_by_value[ESpell::SPELL_AWARNESS] = "SPELL_AWARNESS";
	ESpell_name_by_value[ESpell::SPELL_RELIGION] = "SPELL_RELIGION";
	ESpell_name_by_value[ESpell::SPELL_AIR_SHIELD] = "SPELL_AIR_SHIELD";
	ESpell_name_by_value[ESpell::SPELL_PORTAL] = "SPELL_PORTAL";
	ESpell_name_by_value[ESpell::SPELL_DISPELL_MAGIC] = "SPELL_DISPELL_MAGIC";
	ESpell_name_by_value[ESpell::SPELL_SUMMON_KEEPER] = "SPELL_SUMMON_KEEPER";
	ESpell_name_by_value[ESpell::SPELL_FAST_REGENERATION] = "SPELL_FAST_REGENERATION";
	ESpell_name_by_value[ESpell::SPELL_CREATE_WEAPON] = "SPELL_CREATE_WEAPON";
	ESpell_name_by_value[ESpell::SPELL_FIRE_SHIELD] = "SPELL_FIRE_SHIELD";
	ESpell_name_by_value[ESpell::SPELL_RELOCATE] = "SPELL_RELOCATE";
	ESpell_name_by_value[ESpell::SPELL_SUMMON_FIREKEEPER] = "SPELL_SUMMON_FIREKEEPER";
	ESpell_name_by_value[ESpell::SPELL_ICE_SHIELD] = "SPELL_ICE_SHIELD";
	ESpell_name_by_value[ESpell::SPELL_ICESTORM] = "SPELL_ICESTORM";
	ESpell_name_by_value[ESpell::SPELL_ENLESS] = "SPELL_ENLESS";
	ESpell_name_by_value[ESpell::SPELL_SHINEFLASH] = "SPELL_SHINEFLASH";
	ESpell_name_by_value[ESpell::SPELL_MADNESS] = "SPELL_MADNESS";
	ESpell_name_by_value[ESpell::SPELL_GROUP_MAGICGLASS] = "SPELL_GROUP_MAGICGLASS";
	ESpell_name_by_value[ESpell::SPELL_CLOUD_OF_ARROWS] = "SPELL_CLOUD_OF_ARROWS";
	ESpell_name_by_value[ESpell::SPELL_VACUUM] = "SPELL_VACUUM";
	ESpell_name_by_value[ESpell::SPELL_METEORSTORM] = "SPELL_METEORSTORM";
	ESpell_name_by_value[ESpell::SPELL_STONEHAND] = "SPELL_STONEHAND";
	ESpell_name_by_value[ESpell::SPELL_MINDLESS] = "SPELL_MINDLESS";
	ESpell_name_by_value[ESpell::SPELL_PRISMATICAURA] = "SPELL_PRISMATICAURA";
	ESpell_name_by_value[ESpell::SPELL_EVILESS] = "SPELL_EVILESS";
	ESpell_name_by_value[ESpell::SPELL_AIR_AURA] = "SPELL_AIR_AURA";
	ESpell_name_by_value[ESpell::SPELL_FIRE_AURA] = "SPELL_FIRE_AURA";
	ESpell_name_by_value[ESpell::SPELL_ICE_AURA] = "SPELL_ICE_AURA";
	ESpell_name_by_value[ESpell::SPELL_SHOCK] = "SPELL_SHOCK";
	ESpell_name_by_value[ESpell::SPELL_MAGICGLASS] = "SPELL_MAGICGLASS";
	ESpell_name_by_value[ESpell::SPELL_GROUP_SANCTUARY] = "SPELL_GROUP_SANCTUARY";
	ESpell_name_by_value[ESpell::SPELL_GROUP_PRISMATICAURA] = "SPELL_GROUP_PRISMATICAURA";
	ESpell_name_by_value[ESpell::SPELL_DEAFNESS] = "SPELL_DEAFNESS";
	ESpell_name_by_value[ESpell::SPELL_POWER_DEAFNESS] = "SPELL_POWER_DEAFNESS";
	ESpell_name_by_value[ESpell::SPELL_REMOVE_DEAFNESS] = "SPELL_REMOVE_DEAFNESS";
	ESpell_name_by_value[ESpell::SPELL_MASS_DEAFNESS] = "SPELL_MASS_DEAFNESS";
	ESpell_name_by_value[ESpell::SPELL_DUSTSTORM] = "SPELL_DUSTSTORM";
	ESpell_name_by_value[ESpell::SPELL_EARTHFALL] = "SPELL_EARTHFALL";
	ESpell_name_by_value[ESpell::SPELL_SONICWAVE] = "SPELL_SONICWAVE";
	ESpell_name_by_value[ESpell::SPELL_HOLYSTRIKE] = "SPELL_HOLYSTRIKE";
	ESpell_name_by_value[ESpell::SPELL_ANGEL] = "SPELL_ANGEL";
	ESpell_name_by_value[ESpell::SPELL_MASS_FEAR] = "SPELL_MASS_FEAR";
	ESpell_name_by_value[ESpell::SPELL_FASCINATION] = "SPELL_FASCINATION";
	ESpell_name_by_value[ESpell::SPELL_CRYING] = "SPELL_CRYING";
	ESpell_name_by_value[ESpell::SPELL_OBLIVION] = "SPELL_OBLIVION";
	ESpell_name_by_value[ESpell::SPELL_BURDEN_OF_TIME] = "SPELL_BURDEN_OF_TIME";
	ESpell_name_by_value[ESpell::SPELL_GROUP_REFRESH] = "SPELL_GROUP_REFRESH";
	ESpell_name_by_value[ESpell::SPELL_PEACEFUL] = "SPELL_PEACEFUL";
	ESpell_name_by_value[ESpell::SPELL_MAGICBATTLE] = "SPELL_MAGICBATTLE";
	ESpell_name_by_value[ESpell::SPELL_BERSERK] = "SPELL_BERSERK";
	ESpell_name_by_value[ESpell::SPELL_STONEBONES] = "SPELL_STONEBONES";
	ESpell_name_by_value[ESpell::SPELL_ROOM_LIGHT] = "SPELL_ROOM_LIGHT";
	ESpell_name_by_value[ESpell::SPELL_POISONED_FOG] = "SPELL_POISONED_FOG";
	ESpell_name_by_value[ESpell::SPELL_THUNDERSTORM] = "SPELL_THUNDERSTORM";
	ESpell_name_by_value[ESpell::SPELL_LIGHT_WALK] = "SPELL_LIGHT_WALK";
	ESpell_name_by_value[ESpell::SPELL_FAILURE] = "SPELL_FAILURE";
	ESpell_name_by_value[ESpell::SPELL_CLANPRAY] = "SPELL_CLANPRAY";
	ESpell_name_by_value[ESpell::SPELL_GLITTERDUST] = "SPELL_GLITTERDUST";
	ESpell_name_by_value[ESpell::SPELL_SCREAM] = "SPELL_SCREAM";
	ESpell_name_by_value[ESpell::SPELL_CATS_GRACE] = "SPELL_CATS_GRACE";
	ESpell_name_by_value[ESpell::SPELL_BULL_BODY] = "SPELL_BULL_BODY";
	ESpell_name_by_value[ESpell::SPELL_SNAKE_WISDOM] = "SPELL_SNAKE_WISDOM";
	ESpell_name_by_value[ESpell::SPELL_GIMMICKRY] = "SPELL_GIMMICKRY";
	ESpell_name_by_value[ESpell::SPELL_WC_OF_CHALLENGE] = "SPELL_WC_OF_CHALLENGE";
	ESpell_name_by_value[ESpell::SPELL_WC_OF_MENACE] = "SPELL_WC_OF_MENACE";
	ESpell_name_by_value[ESpell::SPELL_WC_OF_RAGE] = "SPELL_WC_OF_RAGE";
	ESpell_name_by_value[ESpell::SPELL_WC_OF_MADNESS] = "SPELL_WC_OF_MADNESS";
	ESpell_name_by_value[ESpell::SPELL_WC_OF_THUNDER] = "SPELL_WC_OF_THUNDER";
	ESpell_name_by_value[ESpell::SPELL_WC_OF_DEFENSE] = "SPELL_WC_OF_DEFENSE";
	ESpell_name_by_value[ESpell::SPELL_WC_OF_BATTLE] = "SPELL_WC_OF_BATTLE";
	ESpell_name_by_value[ESpell::SPELL_WC_OF_POWER] = "SPELL_WC_OF_POWER";
	ESpell_name_by_value[ESpell::SPELL_WC_OF_BLESS] = "SPELL_WC_OF_BLESS";
	ESpell_name_by_value[ESpell::SPELL_WC_OF_COURAGE] = "SPELL_WC_OF_COURAGE";
	ESpell_name_by_value[ESpell::SPELL_RUNE_LABEL] = "SPELL_RUNE_LABEL";
	ESpell_name_by_value[ESpell::SPELL_ACONITUM_POISON] = "SPELL_ACONITUM_POISON";
	ESpell_name_by_value[ESpell::SPELL_SCOPOLIA_POISON] = "SPELL_SCOPOLIA_POISON";
	ESpell_name_by_value[ESpell::SPELL_BELENA_POISON] = "SPELL_BELENA_POISON";
	ESpell_name_by_value[ESpell::SPELL_DATURA_POISON] = "SPELL_DATURA_POISON";
	ESpell_name_by_value[ESpell::SPELL_TIMER_REPAIR] = "SPELL_TIMER_REPAIR";
	ESpell_name_by_value[ESpell::SPELL_LACKY] = "SPELL_LACKY";
	ESpell_name_by_value[ESpell::SPELL_BANDAGE] = "SPELL_BANDAGE";
	ESpell_name_by_value[ESpell::SPELL_NO_BANDAGE] = "SPELL_NO_BANDAGE";
	ESpell_name_by_value[ESpell::SPELL_CAPABLE] = "SPELL_CAPABLE";
	ESpell_name_by_value[ESpell::SPELL_STRANGLE] = "SPELL_STRANGLE";
	ESpell_name_by_value[ESpell::SPELL_RECALL_SPELLS] = "SPELL_RECALL_SPELLS";
	ESpell_name_by_value[ESpell::SPELL_HYPNOTIC_PATTERN] = "SPELL_HYPNOTIC_PATTERN";
	ESpell_name_by_value[ESpell::SPELL_SOLOBONUS] = "SPELL_SOLOBONUS";
	ESpell_name_by_value[ESpell::SPELL_VAMPIRE] = "SPELL_VAMPIRE";
	ESpell_name_by_value[ESpell::SPELLS_RESTORATION] = "SPELLS_RESTORATION";
	ESpell_name_by_value[ESpell::SPELL_AURA_DEATH] = "SPELL_AURA_DEATH";
	ESpell_name_by_value[ESpell::SPELL_RECOVERY] = "SPELL_RECOVERY";
	ESpell_name_by_value[ESpell::SPELL_MASS_RECOVERY] = "SPELL_MASS_RECOVERY";
	ESpell_name_by_value[ESpell::SPELL_AURA_EVIL] = "SPELL_AURA_EVIL";
	ESpell_name_by_value[ESpell::SPELL_MENTAL_SHADOW] = "SPELL_MENTAL_SHADOW";
	ESpell_name_by_value[ESpell::SPELL_EVARDS_BLACK_TENTACLES] = "SPELL_EVARDS_BLACK_TENTACLES";
	ESpell_name_by_value[ESpell::SPELL_WHIRLWIND] = "SPELL_WHIRLWIND";
	ESpell_name_by_value[ESpell::SPELL_INDRIKS_TEETH] = "SPELL_INDRIKS_TEETH";
	ESpell_name_by_value[ESpell::SPELL_MELFS_ACID_ARROW] = "SPELL_MELFS_ACID_ARROW";
	ESpell_name_by_value[ESpell::SPELL_THUNDERSTONE] = "SPELL_THUNDERSTONE";
	ESpell_name_by_value[ESpell::SPELL_CLOD] = "SPELL_CLOD";
	ESpell_name_by_value[ESpell::SPELL_EXPEDIENT] = "SPELL_EXPEDIENT";
	ESpell_name_by_value[ESpell::SPELL_SIGHT_OF_DARKNESS] = "SPELL_SIGHT_OF_DARKNESS";
	ESpell_name_by_value[ESpell::SPELL_GENERAL_SINCERITY] = "SPELL_GENERAL_SINCERITY";
	ESpell_name_by_value[ESpell::SPELL_MAGICAL_GAZE] = "SPELL_MAGICAL_GAZE";
	ESpell_name_by_value[ESpell::SPELL_ALL_SEEING_EYE] = "SPELL_ALL_SEEING_EYE";
	ESpell_name_by_value[ESpell::SPELL_EYE_OF_GODS] = "SPELL_EYE_OF_GODS";
	ESpell_name_by_value[ESpell::SPELL_BREATHING_AT_DEPTH] = "SPELL_BREATHING_AT_DEPTH";
	ESpell_name_by_value[ESpell::SPELL_GENERAL_RECOVERY] = "SPELL_GENERAL_RECOVERY";
	ESpell_name_by_value[ESpell::SPELL_COMMON_MEAL] = "SPELL_COMMON_MEAL";
	ESpell_name_by_value[ESpell::SPELL_STONE_WALL] = "SPELL_STONE_WALL";
	ESpell_name_by_value[ESpell::SPELL_SNAKE_EYES] = "SPELL_SNAKE_EYES";
	ESpell_name_by_value[ESpell::SPELL_EARTH_AURA] = "SPELL_EARTH_AURA";
	ESpell_name_by_value[ESpell::SPELL_GROUP_PROT_FROM_EVIL] = "SPELL_GROUP_PROT_FROM_EVIL";
	ESpell_name_by_value[ESpell::SPELL_ARROWS_FIRE] = "SPELL_ARROWS_FIRE";
	ESpell_name_by_value[ESpell::SPELL_ARROWS_WATER] = "SPELL_ARROWS_WATER";
	ESpell_name_by_value[ESpell::SPELL_ARROWS_EARTH] = "SPELL_ARROWS_EARTH";
	ESpell_name_by_value[ESpell::SPELL_ARROWS_AIR] = "SPELL_ARROWS_AIR";
	ESpell_name_by_value[ESpell::SPELL_ARROWS_DEATH] = "SPELL_ARROWS_DEATH";

	for (const auto& i : ESpell_name_by_value)
	{
		ESpell_value_by_name[i.second] = i.first;
	}
}

template <>
const std::string& NAME_BY_ITEM<ESpell>(const ESpell item)
{
	if (ESpell_name_by_value.empty())
	{
		init_ESpell_ITEM_NAMES();
	}
	return ESpell_name_by_value.at(item);
}

template <>
ESpell ITEM_BY_NAME(const std::string& name)
{
	if (ESpell_name_by_value.empty())
	{
		init_ESpell_ITEM_NAMES();
	}
	return ESpell_value_by_name.at(name);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
