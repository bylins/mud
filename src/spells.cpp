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

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
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

extern room_rnum r_mortal_start_room;

extern OBJ_DATA *object_list;
extern vector < OBJ_DATA * >obj_proto;
extern CHAR_DATA *character_list;
extern INDEX_DATA *obj_index;
extern DESCRIPTOR_DATA *descriptor_list;
extern struct zone_data *zone_table;
extern const char *material_name[];
extern const char *weapon_affects[];
extern TIME_INFO_DATA time_info;
extern int mini_mud, cmd_tell;
extern char cast_argument[MAX_INPUT_LENGTH];
extern int slot_for_char(CHAR_DATA * ch, int slot_num);
// added by WorM  опознание магических ингров 2011.05.21
extern im_type *imtypes;
extern int top_imtypes;
//end by WorM

bool can_get_spell(CHAR_DATA *ch, int spellnum);
void clearMemory(CHAR_DATA * ch);
void weight_change_object(OBJ_DATA * obj, int weight);
int compute_armor_class(CHAR_DATA * ch);
char *diag_weapon_to_char(const OBJ_DATA * obj, int show_wear);
void create_rainsnow(int *wtype, int startvalue, int chance1, int chance2, int chance3);
int calc_loadroom(CHAR_DATA * ch, int bplace_mode = BIRTH_PLACE_UNDEFINED);
int calc_anti_savings(CHAR_DATA * ch);
void go_flee(CHAR_DATA * ch);

ACMD(do_tell);

void perform_remove(CHAR_DATA * ch, int pos);
int get_zone_rooms(int, int *, int *);

int pk_action_type_summon(CHAR_DATA * agressor, CHAR_DATA * victim);
void pk_increment_revenge(CHAR_DATA * agressor, CHAR_DATA * victim);

int what_sky = SKY_CLOUDLESS;
// * Special spells appear below.

// Функция определяет возможность изучения спелла из книги или в гильдии
bool can_get_spell(CHAR_DATA *ch, int spellnum)
{
	if ((MIN_CAST_LEV(spell_info[spellnum], ch) > GET_LEVEL(ch) || MIN_CAST_REM(spell_info[spellnum], ch) > GET_REMORT(ch) ||
			 		 slot_for_char(ch, spell_info[spellnum].slot_forc[(int) GET_CLASS(ch)][(int) GET_KIN(ch)]) <= 0))
			 		 return FALSE;
	return TRUE;
};

ASPELL(spell_create_water)
{
	int water;
	if (ch == NULL || (obj == NULL && victim == NULL))
		return;
	// level = MAX(MIN(level, LVL_IMPL), 1);       - not used

	if (obj && GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
	{
		if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0))
		{
			send_to_char("Прекратите, ради бога, химичить.\r\n", ch);
			return;
			name_from_drinkcon(obj);
			GET_OBJ_VAL(obj, 2) = LIQ_BLOOD;
			name_to_drinkcon(obj, LIQ_BLOOD);
		}
		else
		{
			water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
			if (water > 0)
			{
				if (GET_OBJ_VAL(obj, 1) >= 0)
					name_from_drinkcon(obj);
				GET_OBJ_VAL(obj, 2) = LIQ_WATER;
				GET_OBJ_VAL(obj, 1) += water;
				act("Вы наполнили $o3 водой.", FALSE, ch, obj, 0, TO_CHAR);
				name_to_drinkcon(obj, LIQ_WATER);
				weight_change_object(obj, water);
			}
		}
	}
	if (victim && !IS_NPC(victim) && !IS_IMMORTAL(victim))
	{
		gain_condition(victim, THIRST, 25);
		send_to_char("Вы полностью утолили жажду.\r\n", victim);
		if (victim != ch)
			act("Вы напоили $N3.", FALSE, ch, 0, victim, TO_CHAR);
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


ASPELL(spell_recall)
{
	room_rnum to_room = NOWHERE, fnd_room = NOWHERE;
	room_rnum rnum_start, rnum_stop;
	int modi = 0;

	if (!victim || IS_NPC(victim) || IN_ROOM(ch) != IN_ROOM(victim) || GET_LEVEL(victim) >= LVL_IMMORT)
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	if (!IS_GOD(ch) && (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORTOUT) || AFF_FLAGGED(victim, AFF_NOTELEPORT)))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	if (victim != ch)
	{
		if (WAITLESS(ch) && !WAITLESS(victim))
			modi += 100;	// always success
		else if (same_group(ch, victim))
			modi += 75;	// 75% chance to success
		else if (!IS_NPC(ch) || (ch->master && !IS_NPC(ch->master)))
			modi = -100;	// always fail

		if (modi == -100 || general_savingthrow(ch, victim, SAVING_WILL, modi))
		{
			send_to_char(SUMMON_FAIL, ch);
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
		pk_agro_action(ch, victim->get_fighting());
	}

	act("$n исчез$q.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	char_from_room(victim);
	char_to_room(victim, fnd_room);
	check_horse(victim);
	act("$n появил$u в центре комнаты.", TRUE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, 0);
	entry_memory_mtrigger(victim);
	greet_mtrigger(victim, -1);
	greet_otrigger(victim, -1);
	greet_memory_mtrigger(victim);
}


// ПРЫЖОК в рамках зоны
ASPELL(spell_teleport)
{
	room_rnum in_room = IN_ROOM(ch), fnd_room = NOWHERE;
	room_rnum rnum_start, rnum_stop;

	if (!IS_GOD(ch) && (ROOM_FLAGGED(in_room, ROOM_NOTELEPORTOUT) || AFF_FLAGGED(ch, AFF_NOTELEPORT)))
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
	entry_memory_mtrigger(ch);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
	greet_memory_mtrigger(ch);
}

// ПЕРЕМЕСТИТЬСЯ
ASPELL(spell_relocate)
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
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTELEPORTOUT))
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
		}
		//Polud убираем все это, поскольку теперь за релокейт без режима и не в группе вешается агроБД
		// Перемещаться можно только с отключенным "режимом призыв"
		//if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		//{
		//	send_to_char("При применении заклинания требуется отключить \"режим призыв\"!\r\n", ch);
		//	send_to_char(SUMMON_FAIL, ch);
		//	return;
		//}
		// Перемещаться нельзя состоя в группе (т.к. пента на согрупника и суммон идут без режима)
		//if (AFF_FLAGGED(ch, AFF_GROUP))
		//{
		//	send_to_char("При применении заклинания нельзя состоять в группе!\r\n", ch);
		//	send_to_char(SUMMON_FAIL, ch);
		//	return;
		//}
		// Во избежания абьюза с заследованием а потом погрупливанием после перемещения и юзанием переместившегося как маяка
		//if (ch->master)
		//{
		//	send_to_char(SUMMON_FAIL, ch);
		//	return;
		//}
		// Нельзя перемещаться после того, как попал под заклинание "приковать противника".
		if (AFF_FLAGGED(ch, AFF_NOTELEPORT))
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
	entry_memory_mtrigger(ch);
	greet_mtrigger(ch, -1);
	greet_otrigger(ch, -1);
	greet_memory_mtrigger(ch);
}

inline void decay_portal(const int room_num)
{
	act("Пентаграмма медленно растаяла.", FALSE, world[room_num]->people, 0, 0, TO_ROOM);
	act("Пентаграмма медленно растаяла.", FALSE, world[room_num]->people, 0, 0, TO_CHAR);
	world[room_num]->portal_time = 0;
	world[room_num]->portal_room = 0;
}

void check_auto_nosummon(CHAR_DATA *ch)
{
	if (PRF_FLAGGED(ch, PRF_AUTO_NOSUMMON) && PRF_FLAGGED(ch, PRF_SUMMONABLE))
	{
		REMOVE_BIT(PRF_FLAGS(ch, PRF_SUMMONABLE), PRF_SUMMONABLE);
		send_to_char("Режим автопризыв: вы защищены от призыва.\r\n", ch);
	}
}

ASPELL(spell_portal)
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
		if ((!IS_NPC(victim) && GET_LEVEL(victim) <= 10) || IS_IMMORTAL(victim) || AFF_FLAGGED(victim, AFF_NOTELEPORT))
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
	if (!IS_GOD(ch) && ( //ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTELEPORTOUT)||
				//ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTELEPORTIN)||
				SECT(fnd_room) == SECT_SECRET || ROOM_FLAGGED(fnd_room, ROOM_DEATH) || ROOM_FLAGGED(fnd_room, ROOM_SLOWDEATH) || ROOM_FLAGGED(fnd_room, ROOM_ICEDEATH) || ROOM_FLAGGED(fnd_room, ROOM_TUNNEL) || ROOM_FLAGGED(fnd_room, ROOM_GODROOM)	//||
				//ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORTOUT) ||
				//ROOM_FLAGGED(fnd_room, ROOM_NOTELEPORTIN)
			))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}

	//Юзабилити фикс: не ставим пенту в одну клетку с кастующим
	if (IN_ROOM(ch) == fnd_room)
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
	if (world[IN_ROOM(ch)]->portal_time)
	{
		if (world[world[IN_ROOM(ch)]->portal_room]->portal_room == IN_ROOM(ch) && world[world[IN_ROOM(ch)]->portal_room]->portal_time)
			decay_portal(world[IN_ROOM(ch)]->portal_room);
		decay_portal(IN_ROOM(ch));
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

		to_room = IN_ROOM(ch);
		world[fnd_room]->portal_room = to_room;
		world[fnd_room]->portal_time = 1;
		if (pkPortal) world[fnd_room]->pkPenterUnique = GET_UNIQUE(ch);

		if (pkPortal)
		{
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.", FALSE, world[fnd_room]->people, 0, 0, TO_CHAR);
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.", FALSE, world[fnd_room]->people, 0, 0, TO_ROOM);
		}else
		{
			act("Лазурная пентаграмма возникла в воздухе.", FALSE, world[fnd_room]->people, 0, 0, TO_CHAR);
			act("Лазурная пентаграмма возникла в воздухе.", FALSE, world[fnd_room]->people, 0, 0, TO_ROOM);
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
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.", FALSE, world[to_room]->people, 0, 0, TO_CHAR);
			act("Лазурная пентаграмма с кровавым отблеском возникла в воздухе.", FALSE, world[to_room]->people, 0, 0, TO_ROOM);
		}else
		{
			act("Лазурная пентаграмма возникла в воздухе.", FALSE, world[to_room]->people, 0, 0, TO_CHAR);
			act("Лазурная пентаграмма возникла в воздухе.", FALSE, world[to_room]->people, 0, 0, TO_ROOM);
		}
	}
}

ASPELL(spell_summon)
{
	room_rnum ch_room, vic_room;

	// Если переменные не определены или каст на себя - завершаем.
	if (ch == NULL || victim == NULL || ch == victim)
		return;

	ch_room = IN_ROOM(ch);
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
			if (AFF_FLAGGED(ch, AFF_SHIELD))
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
				SECT(IN_ROOM(ch)) == SECT_SECRET)	// суммонер стоит в клетке с типом "секретый"
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
		}
		// Жертву нельзя призвать если она в:
		if (ROOM_FLAGGED(vic_room, ROOM_NOSUMMON)	||	// жертва в комнате с флагом !призвать
				ROOM_FLAGGED(vic_room, ROOM_TUNNEL)	||	// жертва стоит в ван-руме
				ROOM_FLAGGED(vic_room, ROOM_GODROOM)||	// жертва в комнате для бессмертных
				ROOM_FLAGGED(vic_room, ROOM_ARENA)	||	// жертва на арене
				!Clan::MayEnter(ch, vic_room, HCE_PORTAL)||// жертва во внутренних покоях клан-замка
				AFF_FLAGGED(victim, AFF_NOTELEPORT))	// жертва под действием заклинания "приковать противника"
		{
			send_to_char(SUMMON_FAIL, ch);
			return;
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

	entry_memory_mtrigger(victim);
	greet_mtrigger(victim, -1);	// УЖАС!!! Не стоит в эту функцию передавать -1 :)
	greet_otrigger(victim, -1);	// УЖАС!!! Не стоит в эту функцию передавать -1 :)
	greet_memory_mtrigger(victim);
	return;
}

ASPELL(spell_townportal)
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
		if (find_portal_by_vnum(GET_ROOM_VNUM(IN_ROOM(ch))))
		{
			send_to_char("Камень рядом с вами мешает вашей магии.\r\n", ch);
			return;
		}
		// Если в комнате есть метка-"камень" то врата ставить нельзя //
		if (room_affected_by_spell(world[IN_ROOM(ch)], SPELL_RUNE_LABEL))
		{
			send_to_char("Начертанные на земле магические руны подавляют вашу магию!\r\n", ch);
			return;
		}
		// Чтоб не кастили в NOMAGIC
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC) && !IS_GRGOD(ch))
		{
			send_to_char("Ваша магия потерпела неудачу и развеялась по воздуху.\r\n", ch);
			act("Магия $n1 потерпела неудачу и развеялась по воздуху.", FALSE, ch, 0, 0, TO_ROOM);
			return;
		}
		// Открываем пентаграмму в комнату rnum //
		improove_skill(ch, SKILL_TOWNPORTAL, 1, NULL);
		ROOM_DATA* from_room = world[IN_ROOM(ch)];
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


ASPELL(spell_locate_object)
{
	OBJ_DATA *i;
	char name[MAX_INPUT_LENGTH];
	int j;

	/*
	 * FIXME: This is broken.  The spell parser routines took the argument
	 * the player gave to the spell and located an object with that keyword.
	 * Since we're passed the object and not the keyword we can only guess
	 * at what the player originally meant to search for. -gg
	 */
	if (!obj)
		return;

	strcpy(name, cast_argument);
	j = level;

	for (i = object_list; i && (j > 0); i = i->next)
	{
		if (number(1, 100) > (40 + MAX((GET_REAL_INT(ch) - 25) * 2, 0)))
			continue;

		if (IS_CORPSE(i))
			continue;

		if (!isname(name, i->aliases))
			continue;

		if (SECT(IN_ROOM(i)) == SECT_SECRET)
			continue;

		if (i->carried_by)
			if (SECT(IN_ROOM(i->carried_by)) == SECT_SECRET ||
					(OBJ_FLAGGED(i, ITEM_NOLOCATE) && IS_NPC(i->carried_by)) ||
					IS_IMMORTAL(i->carried_by))
				continue;

		if (i->carried_by)
		{
			if (world[IN_ROOM(i->carried_by)]->zone == world[IN_ROOM(ch)]->zone || !IS_NPC(i->carried_by))
				sprintf(buf, "%s наход%sся у %s в инвентаре.\r\n",
						i->short_description, GET_OBJ_POLY_1(ch, i), PERS(i->carried_by, ch, 1));
			else
				continue;
		}
		else if (IN_ROOM(i) != NOWHERE && IN_ROOM(i))
		{
			if (world[IN_ROOM(i)]->zone == world[IN_ROOM(ch)]->zone && !OBJ_FLAGGED(i, ITEM_NOLOCATE))
				sprintf(buf, "%s наход%sся в %s.\r\n", i->short_description, GET_OBJ_POLY_1(ch, i), world[IN_ROOM(i)]->name);
			else
				continue;
		}
		else if (i->in_obj)
		{
			if (Clan::is_clan_chest(i->in_obj))
			{
				continue; // шоб не забивало локейт на мобах/плеерах - по кланам проходим ниже отдельно
			}
			else
			{
				if (i->in_obj->carried_by)
					if (IS_NPC(i->in_obj->carried_by) && (OBJ_FLAGGED(i, ITEM_NOLOCATE) || world[IN_ROOM(i->in_obj->carried_by)]->zone != world[IN_ROOM(ch)]->zone))
						continue;
				if (IN_ROOM(i->in_obj) != NOWHERE && IN_ROOM(i->in_obj))
					if (world[IN_ROOM(i->in_obj)]->zone != world[IN_ROOM(ch)]->zone || OBJ_FLAGGED(i, ITEM_NOLOCATE))
						continue;
				if (i->in_obj->worn_by)
					if (IS_NPC(i->in_obj->worn_by)
							&& (OBJ_FLAGGED(i, ITEM_NOLOCATE)
							|| world[IN_ROOM(i->in_obj->worn_by)]->zone != world[IN_ROOM(ch)]->zone))
						continue;
				sprintf(buf, "%s наход%sся в %s.\r\n", i->short_description, GET_OBJ_POLY_1(ch, i), i->in_obj->PNames[5]);
			}
		}
		else if (i->worn_by)
		{
			if ((IS_NPC(i->worn_by) && !OBJ_FLAGGED(i, ITEM_NOLOCATE)
					&& world[IN_ROOM(i->worn_by)]->zone == world[IN_ROOM(ch)]->zone)
					|| (!IS_NPC(i->worn_by) && GET_LEVEL(i->worn_by) < LVL_IMMORT))
				sprintf(buf, "%s надет%s на %s.\r\n", i->short_description,
						GET_OBJ_SUF_6(i), PERS(i->worn_by, ch, 3));
			else
				continue;
		}
		else
			sprintf(buf, "Местоположение %s неопределимо.\r\n", OBJN(i, ch, 1));

		CAP(buf);
		send_to_char(buf, ch);
		j--;
	}

	if (j > 0)
		j = Clan::print_spell_locate_object(ch, j, std::string(name));
	if (j > 0)
		j = Depot::print_spell_locate_object(ch, j, std::string(name));
	if (j > 0)
		j = Parcel::print_spell_locate_object(ch, j, std::string(name));

	if (j == level)
		send_to_char("Вы ничего не чувствуете.\r\n", ch);
}

ASPELL(spell_create_weapon)
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
		if (AFF_FLAGGED(k->follower, AFF_CHARM) && k->follower->master == ch)
		{
			cha_summ++;
			//hp_summ += GET_REAL_MAX_HIT(k->follower);
			reformed_hp_summ += get_reformed_charmice_hp(ch, k->follower, spellnum);
// Проверка на тип последователей -- некрасиво, зато эффективно
			if (MOB_FLAGGED(k->follower, MOB_CORPSE))
				undead_in_group = TRUE;
			else
				living_in_group = TRUE;
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
//    !WAITLESS(ch) &&
//    hp_summ + GET_REAL_MAX_HIT(victim) >= cha_app[GET_REAL_CHA(ch)].charms )
			reformed_hp_summ + get_reformed_charmice_hp(ch, victim, spellnum) >= get_player_charms(ch, spellnum))
	{
		send_to_char("Вам не под силу управлять такой боевой мощью.\r\n", ch);
		return (FALSE);
	}
	return (TRUE);
}

ASPELL(spell_charm)
{
	AFFECT_DATA af;
	int i;

	if (victim == NULL || ch == NULL)
		return;

	if (victim == ch)
		send_to_char("Вы просто очарованы своим внешним видом!\r\n", ch);
	else if (!IS_NPC(victim))
	{
		send_to_char("Вы не можете очаровать реального игрока!\r\n", ch);
		pk_agro_action(ch, victim);
	}
	else if (!IS_IMMORTAL(ch) && (AFF_FLAGGED(victim, AFF_SANCTUARY) || MOB_FLAGGED(victim, MOB_PROTECT)))
		send_to_char("Ваша жертва освящена Богами!\r\n", ch);
// shapirus: нельзя почармить моба под ЗБ
	else if (!IS_IMMORTAL(ch) && (AFF_FLAGGED(victim, AFF_SHIELD) || MOB_FLAGGED(victim, MOB_PROTECT)))
		send_to_char("Ваша жертва защищена Богами!\r\n", ch);
	else if (!IS_IMMORTAL(ch) && MOB_FLAGGED(victim, MOB_NOCHARM))
		send_to_char("Ваша жертва устойчива к этому!\r\n", ch);
	else if (AFF_FLAGGED(ch, AFF_CHARM))
		send_to_char("Вы сами очарованы кем-то и не можете иметь последователей.\r\n", ch);
	else if (AFF_FLAGGED(victim, AFF_CHARM)
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
	else if (!IS_IMMORTAL(ch) && general_savingthrow(ch, victim, SAVING_WILL, (GET_REAL_CHA(ch) - 10) * 3))
		send_to_char("Ваша магия потерпела неудачу.\r\n", ch);
	else
	{
//    Проверяем - можем ли мы зачармить моба с уровнем victim
//    if (charm_points(ch) < used_charm_points(ch)
//                            + on_charm_points(victim)) {
//       send_to_char("Вам не под силу управлять такой боевой мощью.\r\n", ch);
		if (!check_charmee(ch, victim, SPELL_CHARM))
			return;
//    }
		// Левая проверка
		if (victim->master)
		{
			if (stop_follower(victim, SF_MASTERDIE))
				return;
		}
//    Загружаем моба CHARM_MOB_VNUM+уровень victim
//    if (!(victim = charm_mob(victim))) {
//      send_to_char("ОШИБКА! Запомните какого моба вы хотели зачармить и сообщите богам.\r\n",ch);
//      return;
//    }
//    // --------

		affect_from_char(victim, SPELL_CHARM);
		add_follower(victim, ch);
		af.type = SPELL_CHARM;
		if (GET_REAL_INT(victim) > GET_REAL_INT(ch))
			af.duration = pc_duration(victim, GET_REAL_CHA(ch), 0, 0, 0, 0);
		else
			af.duration = pc_duration(victim, GET_REAL_CHA(ch) + number(1, 10), 0, 0, 0, 0);
		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		af.battleflag = 0;
		affect_to_char(victim, &af);
		if (GET_HELPER(victim))
			GET_HELPER(victim) = NULL;
		act("$n покорил$g ваше сердце настолько, что вы готовы на все ради н$s.",
			FALSE, ch, 0, victim, TO_VICT);
		if (IS_NPC(victim))
		{
//Eli. Раздеваемся.
			for (i = 0; i < NUM_WEARS; i++)
				if (GET_EQ(victim, i))
				{
					if (!remove_otrigger(GET_EQ(victim, i), victim))
						continue;
					act("Вы прекратили использовать $o3.", FALSE, victim, GET_EQ(victim, i), 0, TO_CHAR);
					act("$n прекратил$g использовать $o3.", TRUE, victim, GET_EQ(victim, i), 0, TO_ROOM);
					obj_to_char(unequip_char(victim, i | 0x40), victim);
				}
//Eli закончили раздеваться.
			REMOVE_BIT(MOB_FLAGS(victim, MOB_AGGRESSIVE), MOB_AGGRESSIVE);
			REMOVE_BIT(MOB_FLAGS(victim, MOB_SPEC), MOB_SPEC);
			REMOVE_BIT(PRF_FLAGS(victim, PRF_PUNCTUAL), PRF_PUNCTUAL);
// shapirus: !train для чармисов
			SET_BIT(MOB_FLAGS(victim, MOB_NOTRAIN), MOB_NOTRAIN);
			victim->set_skill(SKILL_PUNCTUAL, 0);
			// по идее при речарме и последующем креше можно оказаться с сейвом без шмота на чармисе -- Krodo
			Crash_crashsave(ch);
			ch->save_char();
		}
	}
}

ACMD(do_findhelpee)
{
	CHAR_DATA *helpee;
	struct follow_type *k;
	AFFECT_DATA af, *aff;
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
			if (AFF_FLAGGED(k->follower, AFF_HELPER) && AFF_FLAGGED(k->follower, AFF_CHARM))
				break;
		if (k)
		{
			if (IN_ROOM(ch) != IN_ROOM(k->follower))
				act("Вам следует встретиться с $N4 для этого.", FALSE, ch, 0, k->follower, TO_CHAR);
			else if (GET_POS(k->follower) < POS_STANDING)
				act("$N2 сейчас, похоже, не до вас.", FALSE, ch, 0, k->follower, TO_CHAR);
			else
			{
				// added by WorM (Видолюб) 2010.06.04 Cохраняем цену найма моба
				if (!IS_IMMORTAL(ch))
				{
					for (aff = k->follower->affected; aff; aff = aff->next)
					 if (aff->type==SPELL_CHARM)
					{
						cost = MAX(0,(int)((aff->duration-1)/2)*(int)abs(k->follower->mob_specials.hire_price));
						if(cost>0)
						{
							if(k->follower->mob_specials.hire_price < 0)
								ch->add_bank(cost);
							else
								ch->add_gold(cost);
						}
					        break;
					}
				}
				act("Вы рассчитали $N3.", FALSE, ch, 0, k->follower, TO_CHAR);
				// end by WorM
				affect_from_char(k->follower, SPELL_CHARM);
				stop_follower(k->follower, SF_CHARMLOST);
			}
		}
		else
			act("У вас нет наемников!", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	argument = one_argument(argument, arg);

	if (!*arg)
	{
		for (k = ch->followers; k; k = k->next)
			if (AFF_FLAGGED(k->follower, AFF_HELPER) && AFF_FLAGGED(k->follower, AFF_CHARM))
				break;
		if (k)
			act("Вашим наемником является $N.", FALSE, ch, 0, k->follower, TO_CHAR);
		else
			act("У вас нет наемников!", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
	{
		send_to_char("Вы не видите никого похожего.\r\n", ch);
		return;
	}
	for (k = ch->followers; k; k = k->next)
		if (AFF_FLAGGED(k->follower, AFF_HELPER) && AFF_FLAGGED(k->follower, AFF_CHARM))
			break;

	if (helpee == ch)
		send_to_char("И как вы это представляете - нанять самого себя?\r\n", ch);
	else if (!IS_NPC(helpee))
		send_to_char("Вы не можете нанять реального игрока!\r\n", ch);
	else if (!NPC_FLAGGED(helpee, NPC_HELPED))
		act("$N не нанимается!", FALSE, ch, 0, helpee, TO_CHAR);
	else if (AFF_FLAGGED(helpee, AFF_CHARM) && (!k  || (k && helpee != k->follower)))
		act("$N под чьим-то контролем.", FALSE, ch, 0, helpee, TO_CHAR);
	else if (AFF_FLAGGED(helpee, AFF_DEAFNESS))
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
		/*    if (GET_LEVEL(ch) < GET_LEVEL(helpee))
				 {sprintf(buf,"$n сказал$g вам : \" Вы слишком малы для того, чтоб я служил вам.\"");
				  act(buf,FALSE,helpee,0,ch,TO_VICT|CHECK_DEAF);
				  return;
				 }	 */
		if (helpee->master && helpee->master != ch)
		{
			if (stop_follower(helpee, SF_MASTERDIE))
				return;
		}

		if (!(k && k->follower == helpee))
		{
			add_follower(helpee, ch);
			af.duration = pc_duration(helpee, times * TIME_KOEFF, 0, 0, 0, 0);
		}
		else
		{
			for (aff = k->follower->affected; aff; aff = aff->next)
			if (aff->type==SPELL_CHARM)
				break;
			if (aff)
				af.duration = aff->duration + pc_duration(helpee, times * TIME_KOEFF, 0, 0, 0, 0);
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
		af.location = 0;
		af.bitvector = AFF_CHARM;
		af.battleflag = 0;
		affect_to_char(helpee, &af);
		SET_BIT(AFF_FLAGS(helpee, AFF_HELPER), AFF_HELPER);
		sprintf(buf, "$n сказал$g вам : \"Приказывай, %s!\"",
				GET_SEX(ch) == IS_FEMALE(ch) ? "хозяйка" : "хозяин");
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
			REMOVE_BIT(MOB_FLAGS(helpee, MOB_AGGRESSIVE), MOB_AGGRESSIVE);
			REMOVE_BIT(MOB_FLAGS(helpee, MOB_SPEC), MOB_SPEC);
			REMOVE_BIT(PRF_FLAGS(helpee, PRF_PUNCTUAL), PRF_PUNCTUAL);
			// shapirus: !train для чармисов
			SET_BIT(MOB_FLAGS(helpee, MOB_NOTRAIN), MOB_NOTRAIN);
			helpee->set_skill(SKILL_PUNCTUAL, 0);
			// по идее при речарме и последующем креше можно оказаться с сейвом без шмота на чармисе -- Krodo
			Crash_crashsave(ch);
			ch->save_char();
		}
	}
}

void show_weapon(CHAR_DATA * ch, OBJ_DATA * obj)
{
	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
	{
		*buf = '\0';
		if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
			sprintf(buf, "Можно взять %s в правую руку.\r\n", OBJN(obj, ch, 3));
		if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
			sprintf(buf + strlen(buf), "Можно взять %s в левую руку.\r\n", OBJN(obj, ch, 3));
		if (CAN_WEAR(obj, ITEM_WEAR_BOTHS))
			sprintf(buf + strlen(buf), "Можно взять %s в обе руки.\r\n", OBJN(obj, ch, 3));
		if (*buf)
			send_to_char(buf, ch);
	}
}

void print_book_uprgd_skill(CHAR_DATA *ch, const OBJ_DATA *obj)
{
	const int skill_num = GET_OBJ_VAL(obj, 1);
	if (skill_num < 1 || skill_num >= MAX_SKILL_NUM)
	{
		log("SYSERR: invalid skill_num: %d, ch_name=%s, obj_vnum=%d (%s %s %d)",
				skill_num, ch->get_name(), GET_OBJ_VNUM(obj),
				__FILE__, __func__, __LINE__);
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

void print_obj_affects(CHAR_DATA *ch, const obj_affected_type &affect)
{
	sprinttype(affect.location, apply_types, buf2);
	bool negative = false;
	for (int j = 0; *apply_negative[j] != '\n'; j++)
	{
		if (!str_cmp(buf2, apply_negative[j]))
		{
			negative = true;
			break;
		}
	}
	if (!negative && affect.modifier < 0)
	{
		negative = true;
	}
	else if (negative && affect.modifier < 0)
	{
		negative = false;
	}
	sprintf(buf, "   %s%s%s%s%s%d%s\r\n",
			CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM),
			CCCYN(ch, C_NRM),
			negative ? " ухудшает на " : " улучшает на ", abs(affect.modifier), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
}

std::string print_obj_affects(const obj_affected_type &affect)
{
	sprinttype(affect.location, apply_types, buf2);
	bool negative = false;
	for (int j = 0; *apply_negative[j] != '\n'; j++)
	{
		if (!str_cmp(buf2, apply_negative[j]))
		{
			negative = true;
			break;
		}
	}
	if (!negative && affect.modifier < 0)
	{
		negative = true;
	}
	else if (negative && affect.modifier < 0)
	{
		negative = false;
	}

	sprintf(buf, "%s%s%s%s%s%d%s\r\n",
		KCYN, buf2, KNRM,
		KCYN, (negative ? " ухудшает на " : " улучшает на "),
		abs(affect.modifier), KNRM);

	return std::string(buf);
}

void mort_show_obj_values(const OBJ_DATA * obj, CHAR_DATA * ch, int fullness)
{
	int i, found, drndice = 0, drsdice = 0, j;
	long int li;

	send_to_char("Вы узнали следующее:\r\n", ch);
	sprintf(buf, "Предмет \"%s\", тип : ", obj->short_description);
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
	sprinttype(obj->obj_flags.Obj_mater, material_name, buf);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (fullness < 40)
		return;

	send_to_char("Неудобен : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprintbits(obj->obj_flags.no_flag, no_bits, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (fullness < 50)
		return;

	send_to_char("Недоступен : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprintbits(obj->obj_flags.anti_flag, anti_bits, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (obj->get_mort_req() > 0)
	{
		send_to_char(ch, "Требует перевоплощений : %s%d%s\r\n",
				CCCYN(ch, C_NRM), obj->get_mort_req(), CCNRM(ch, C_NRM));
	}

	if (fullness < 60)
		return;

	send_to_char("Имеет экстрафлаги: ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprintbits(obj->obj_flags.extra_flags, extra_bits, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (fullness < 75)
		return;

	switch (GET_OBJ_TYPE(obj))
	{
	case ITEM_SCROLL:
	case ITEM_POTION:
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
	case ITEM_WAND:
	case ITEM_STAFF:
		sprintf(buf, "Вызывает заклинания: ");
		if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
			sprintf(buf + strlen(buf), " %s\r\n", spell_name(GET_OBJ_VAL(obj, 3)));
		sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
		send_to_char(buf, ch);
		break;
	case ITEM_WEAPON:
		drndice = GET_OBJ_VAL(obj, 1);
		drsdice = GET_OBJ_VAL(obj, 2);
		sprintf(buf, "Наносимые повреждения '%dD%d'", drndice, drsdice);
		sprintf(buf + strlen(buf), " среднее %.1f.\r\n", ((drsdice + 1) * drndice / 2.0));
		send_to_char(buf, ch);
		break;
	case ITEM_ARMOR:
	case ITEM_ARMOR_LIGHT:
	case ITEM_ARMOR_MEDIAN:
	case ITEM_ARMOR_HEAVY:
		drndice = GET_OBJ_VAL(obj, 0);
		drsdice = GET_OBJ_VAL(obj, 1);
		sprintf(buf, "защита (AC) : %d\r\n", drndice);
		send_to_char(buf, ch);
		sprintf(buf, "броня       : %d\r\n", drsdice);
		send_to_char(buf, ch);
		break;
	case ITEM_BOOK:
// +newbook.patch (Alisher)
		switch (GET_OBJ_VAL(obj, 0))
		{
		case BOOK_SPELL:
			if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
			{
				drndice = GET_OBJ_VAL(obj, 1);
				if (MIN_CAST_REM(spell_info[GET_OBJ_VAL(obj, 1)], ch) > GET_REMORT(ch))
					drsdice = 34;
				else
					drsdice = MIN_CAST_LEV(spell_info[GET_OBJ_VAL(obj, 1)], ch);
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
					drsdice = min_skill_level(ch, drndice);
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
				drsdice = imrecipes[drndice].level;
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
// -newbook.patch (Alisher)
	case ITEM_INGRADIENT:

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
					CCICYN(ch, C_NRM), obj_proto[i]->PNames[0], CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		break;
// added by WorM  опознание магических ингров 2011.05.21
	case ITEM_MING:
		for (j = 0; imtypes[j].id != GET_OBJ_VAL(obj, IM_TYPE_SLOT)  && j <= top_imtypes;)
			j++;
		sprintf(buf, "Это ингредиент вида '%s%s%s'\r\n", CCCYN(ch, C_NRM), imtypes[j].name, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
		i = GET_OBJ_VAL(obj, IM_POWER_SLOT);
		if (i > 30)
			send_to_char("Вы не в состоянии определить качество этого ингредиента.\r\n", ch);
		else {
			sprintf(buf, "Качество ингредиента ");
			if (i > 25)
				strcat(buf, "наилучшее.\r\n");
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
//end by WorM
//Информация о емкостях и контейнерах (Купала)
	case ITEM_CONTAINER:
		sprintf(buf, "Максимально вместимый вес: %d.\r\n", GET_OBJ_VAL(obj, 0));
		send_to_char(buf, ch);
		break;
	case ITEM_DRINKCON:
		drinkcon::identify(ch, obj);
		break;
//Конец инфы о емкостях и контейнерах (Купала)
	} // switch


	if (fullness < 90)
		return;

	send_to_char("Накладывает на вас аффекты: ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprintbits(obj->obj_flags.affects, weapon_affects, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (fullness < 100)
		return;

	found = FALSE;
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != APPLY_NONE && obj->affected[i].modifier != 0)
		{
			if (!found)
			{
				send_to_char("Дополнительные свойства :\r\n", ch);
				found = TRUE;
			}
			print_obj_affects(ch, obj->affected[i]);
		}
	}
	if (GET_OBJ_TYPE(obj) == ITEM_ENCHANT && GET_OBJ_VAL(obj, 0) != 0)
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
		std::map<int, int> skills;
		obj->get_skills(skills);
		int skill_num;
		int percent;
		for (std::map<int, int>::iterator it = skills.begin(); it != skills.end(); ++it)
		{
			skill_num = it->first;
			percent = it->second;

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
	id_to_set_info_map::iterator it = obj_data::set_table.begin();
	if (OBJ_FLAGGED(obj, ITEM_SETSTUFF))
		for (; it != obj_data::set_table.end(); it++)
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end())
			{
				sprintf(buf, "Часть набора предметов: %s%s%s\r\n",CCNRM(ch, C_NRM), it->second.get_name().c_str(), CCNRM(ch, C_NRM));
				send_to_char(buf, ch );
	      for (set_info::iterator vnum = it->second.begin(), iend = it->second.end(); vnum != iend; ++vnum)
	      {
					int r_num;
	      	if ((r_num = real_object(vnum->first)) < 0)
	      	{
	      		send_to_char("Неизвестный объект!!!\r\n",ch);
	      		continue;
	      	}
					sprintf(buf, "   %s\r\n", obj_proto[r_num]->short_description);
	        send_to_char(buf, ch);
	      }
				break;
			}
	//end by WorM
	if (!obj->enchants.empty())
	{
		obj->enchants.print(ch);
	}
	obj_sets::print_identify(ch, obj);
}

void imm_show_obj_values(OBJ_DATA * obj, CHAR_DATA * ch)
{
	int i, found, drndice = 0, drsdice = 0;
	long int li;

	send_to_char("Вы узнали следующее:\r\n", ch);
	sprintf(buf, "UID: %u, Предмет \"%s\", тип : ",
		obj->uid, obj->short_description);
	sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
	strcat(buf, buf2);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	strcpy(buf, diag_weapon_to_char(obj, 2));
	if (*buf)
		send_to_char(buf, ch);

	//show_weapon(ch, obj);

	send_to_char("Материал : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprinttype(obj->obj_flags.Obj_mater, material_name, buf);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	sprintf(buf, "Таймер : %d\r\n", obj->get_timer());
	send_to_char(buf, ch);

	send_to_char("Накладывает на вас аффекты: ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprintbits(obj->obj_flags.affects, weapon_affects, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	send_to_char("Имеет экстрафлаги: ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprintbits(obj->obj_flags.extra_flags, extra_bits, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	send_to_char("Недоступен : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprintbits(obj->obj_flags.anti_flag, anti_bits, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	send_to_char("Неудобен : ", ch);
	send_to_char(CCCYN(ch, C_NRM), ch);
	sprintbits(obj->obj_flags.no_flag, no_bits, buf, ",");
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);

	if (obj->get_mort_req() > 0)
	{
		send_to_char(ch, "Требует перевоплощений : %s%d%s\r\n",
				CCCYN(ch, C_NRM), obj->get_mort_req(), CCNRM(ch, C_NRM));
	}

	sprintf(buf, "Вес: %d, Цена: %d, Рента: %d(%d)\r\n",
			GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_RENTEQ(obj));
	send_to_char(buf, ch);

	switch (GET_OBJ_TYPE(obj))
	{
	case ITEM_SCROLL:
	case ITEM_POTION:
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
	case ITEM_WAND:
	case ITEM_STAFF:
		sprintf(buf, "Вызывает заклинания: ");
		if (GET_OBJ_VAL(obj, 3) >= 1 && GET_OBJ_VAL(obj, 3) < MAX_SPELLS)
			sprintf(buf + strlen(buf), " %s\r\n", spell_name(GET_OBJ_VAL(obj, 3)));
		sprintf(buf + strlen(buf), "Зарядов %d (осталось %d).\r\n", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
		send_to_char(buf, ch);
		break;
	case ITEM_WEAPON:
		sprintf(buf, "Наносимые повреждения '%dD%d'", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
		sprintf(buf + strlen(buf), " среднее %.1f.\r\n",
				(((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
		send_to_char(buf, ch);
		break;
	case ITEM_ARMOR:
	case ITEM_ARMOR_LIGHT:
	case ITEM_ARMOR_MEDIAN:
	case ITEM_ARMOR_HEAVY:
		sprintf(buf, "защита (AC) : %d\r\n", GET_OBJ_VAL(obj, 0));
		send_to_char(buf, ch);
		sprintf(buf, "броня       : %d\r\n", GET_OBJ_VAL(obj, 1));
		send_to_char(buf, ch);
		break;
	case ITEM_BOOK:
// +newbook.patch (Alisher)
		switch (GET_OBJ_VAL(obj, 0))
		{
		case BOOK_SPELL:
			if (GET_OBJ_VAL(obj, 1) >= 1 && GET_OBJ_VAL(obj, 1) < MAX_SPELLS)
			{
				drndice = GET_OBJ_VAL(obj, 1);
				if (MIN_CAST_REM(spell_info[GET_OBJ_VAL(obj, 1)], ch) > GET_REMORT(ch))
					drsdice = 34;
				else
					drsdice = MIN_CAST_LEV(spell_info[GET_OBJ_VAL(obj, 1)], ch);
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
					drsdice = GET_OBJ_VAL(obj, 2);
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
				drsdice = imrecipes[drndice].level;
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
// -newbook.patch (Alisher)
	case ITEM_INGRADIENT:

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
					CCICYN(ch, C_NRM), obj_proto[i]->PNames[0], CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		break;
	case ITEM_MING:
		sprintf(buf, "Сила ингредиента = %d\r\n", GET_OBJ_VAL(obj, IM_POWER_SLOT));
		send_to_char(buf, ch);
		break;
//Информация о емкостях и контейнерах (Купала)
	case ITEM_CONTAINER:
		sprintf(buf, "Максимально вместимый вес: %d.\r\n", GET_OBJ_VAL(obj, 0));
		send_to_char(buf, ch);
		break;
	case ITEM_DRINKCON:
		drinkcon::identify(ch, obj);
		break;
//Конец инфы о емкостях и контейнерах (Купала)
	} // switch

	found = FALSE;
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != APPLY_NONE && obj->affected[i].modifier != 0)
		{
			if (!found)
			{
				send_to_char("Дополнительные свойства :\r\n", ch);
				found = TRUE;
			}
			print_obj_affects(ch, obj->affected[i]);
		}
	}
	if (GET_OBJ_TYPE(obj) == ITEM_ENCHANT && GET_OBJ_VAL(obj, 0) != 0)
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
		std::map<int, int> skills;
		obj->get_skills(skills);
		int skill_num;
		int percent;
		for (std::map<int, int>::iterator it = skills.begin(); it != skills.end(); ++it)
		{
			skill_num = it->first;
			percent = it->second;

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
	id_to_set_info_map::iterator it = obj_data::set_table.begin();
	if (OBJ_FLAGGED(obj, ITEM_SETSTUFF))
		for (; it != obj_data::set_table.end(); it++)
			if (it->second.find(GET_OBJ_VNUM(obj)) != it->second.end())
			{
				sprintf(buf, "Часть набора предметов: %s%s%s\r\n",CCNRM(ch, C_NRM), it->second.get_name().c_str(), CCNRM(ch, C_NRM));
				send_to_char(buf, ch );
	      for (set_info::iterator vnum = it->second.begin(), iend = it->second.end(); vnum != iend; ++vnum)
	      {
					int r_num;
	      	if ((r_num = real_object(vnum->first)) < 0)
	      	{
	      		send_to_char("Неизвестный объект!!!\r\n",ch);
	      		continue;
	      	}
					sprintf(buf, "   %s\r\n", obj_proto[r_num]->short_description);
	        send_to_char(buf, ch);
	      }
				break;
			}
	//end by WorM
	if (!obj->enchants.empty())
	{
		obj->enchants.print(ch);
	}
	obj_sets::print_identify(ch, obj);
}

#define IDENT_SELF_LEVEL 6

void mort_show_char_values(CHAR_DATA * victim, CHAR_DATA * ch, int fullness)
{
	AFFECT_DATA *aff;
	int found, val0, val1, val2;

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

	for (aff = victim->affected, found = FALSE; aff; aff = aff->next)
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
	sprintbits(victim->char_specials.saved.affected_by, affected_bits, buf2, "\r\n");
	sprintf(buf, "%s\r\n", buf2);
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);
}

void imm_show_char_values(CHAR_DATA * victim, CHAR_DATA * ch)
{
	AFFECT_DATA *aff;
	int found;

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

	for (aff = victim->affected, found = FALSE; aff; aff = aff->next)
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
	sprintbits(victim->char_specials.saved.affected_by, affected_bits, buf2, "\r\n");
	sprintf(buf, "%s\r\n", buf2);
	if (victim->followers)
		sprintf(buf + strlen(buf), "Имеет последователей.\r\n");
	else if (victim->master)
		sprintf(buf + strlen(buf), "Следует за %s.\r\n", GET_PAD(victim->master, 4));
	sprintf(buf + strlen(buf),
			"Уровень повреждений %d, уровень заклинаний %d.\r\n", GET_DAMAGE(victim), GET_CASTER(victim));
	send_to_char(buf, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);
}

ASPELL(skill_identify)
{
	if (obj)
		if (IS_IMMORTAL(ch))
			imm_show_obj_values(obj, ch);
		else
			mort_show_obj_values(obj, ch,
								 train_skill(ch, SKILL_IDENTIFY,
											 skill_info[SKILL_IDENTIFY].max_percent, 0));
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

ASPELL(spell_identify)
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



/*
 * Cannot use this spell on an equipped object or it will mess up the
 * wielding character's hit/dam totals.
 */

/*
ASPELL(spell_detect_poison)
{
  if (victim)
     {if (victim == ch)
         {if (AFF_FLAGGED(victim, AFF_POISON))
             send_to_char("Вы чувствуете яд в своей крови.\r\n", ch);
          else
             send_to_char("Вы чувствуете себя здоровым.\r\n", ch);
         }
      else
         {if (AFF_FLAGGED(victim, AFF_POISON))
             act("Аура $N1 пропитана ядом.", FALSE, ch, 0, victim, TO_CHAR);
          else
             act("Аура $N1 имеет ровную окраску.", FALSE, ch, 0, victim, TO_CHAR);
         }
     }

  if (obj)
     {switch (GET_OBJ_TYPE(obj))
      {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
	act("Аура $o1 пропитана ядом.",FALSE,ch,obj,0,TO_CHAR);
      else
	act("Аура $o1 имеет ровную окраску.", FALSE, ch, obj, 0,TO_CHAR);
      break;
    default:
      send_to_char("Этот предмет не может быть отравлен.\r\n", ch);
      }
     }
}
*/

ASPELL(spell_control_weather)
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
		zone = world[IN_ROOM(ch)]->zone;
		for (i = FIRST_ROOM; i <= top_of_world; i++)
			if (world[i]->zone == zone && SECT(i) != SECT_INSIDE && SECT(i) != SECT_CITY)
			{
				world[i]->weather.sky = what_sky;
				world[i]->weather.weather_type = sky_type;
				world[i]->weather.duration = duration;
				if (world[i]->people)
				{
					act(sky_info, FALSE, world[i]->people, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
					act(sky_info, FALSE, world[i]->people, 0, 0, TO_CHAR);
				}
			}
	}
}

ASPELL(spell_fear)
{
	int modi = 0;
	if (ch != victim)
	{
		modi = calc_anti_savings(ch);
		pk_agro_action(ch, victim);
	}
	if (!IS_NPC(ch) && (GET_LEVEL(ch) > 10))
		modi += (GET_LEVEL(ch) - 10);
	if (PRF_FLAGGED(ch, PRF_AWAKE))
		modi = modi - 50;
	if (AFF_FLAGGED(victim, AFF_BLESS))
		modi -= 25;

	if (!MOB_FLAGGED(victim, MOB_NOFEAR) && !general_savingthrow(ch, victim, SAVING_WILL, modi))
		go_flee(victim);
}

ASPELL(spell_wc_of_fear)
{
	int modi = GET_REAL_CON(ch) * 3 / 2;
	if (ch != victim)
		pk_agro_action(ch, victim);

	if (!MOB_FLAGGED(victim, MOB_NOFEAR) && !general_savingthrow(ch, victim, SAVING_WILL, modi))
		go_flee(victim);
}

ASPELL(spell_energydrain)
{
	// истощить энергию - круг 28 уровень 9 (1)
	// для всех
	int modi = 0;
	if (ch != victim)
	{
		modi = calc_anti_savings(ch);
		pk_agro_action(ch, victim);
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

ASPELL(spell_sacrifice)
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
					&& AFF_FLAGGED(f->follower, AFF_CHARM)
					&& MOB_FLAGGED(f->follower, MOB_CORPSE) && IN_ROOM(ch) == IN_ROOM(f->follower))
			{
				do_sacrifice(f->follower, dam);
			}
		}
	}
}

ASPELL(spell_eviless)
{
	CHAR_DATA *tch;

	for (tch = world[IN_ROOM(ch)]->people; tch; tch = tch->next_in_room)
		if (IS_NPC(tch) && tch->master == ch && MOB_FLAGGED(tch, MOB_CORPSE))
		{
			if (mag_affects(GET_LEVEL(ch), ch, tch, SPELL_EVILESS, SAVING_STABILITY))
			{
				GET_HIT(tch) = MAX(GET_HIT(tch), GET_REAL_MAX_HIT(tch));
			}
		}
	return;
}

ASPELL(spell_holystrike)
{
	const char *msg1 = "Земля под вами засветилась и всех поглотил плотный туман.";
	const char *msg2 = "Вдруг туман стал уходить обратно в землю, забирая с собой тела поверженных.";
	CHAR_DATA *tch, *nxt;
	OBJ_DATA *o;

	act(msg1, FALSE, ch, 0, 0, TO_CHAR);
	act(msg1, FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

	for (tch = world[IN_ROOM(ch)]->people; tch; tch = nxt)
	{
		nxt = tch->next_in_room;
//    if ( SAME_GROUP( ch, tch ) ) continue;
		if (IS_NPC(tch))
		{
			if (!MOB_FLAGGED(tch, MOB_CORPSE)
				&& GET_RACE(tch) != NPC_RACE_ZOMBIE && GET_RACE(tch) != NPC_RACE_EVIL_SPIRIT)
				continue;
		}
		else
		{
			//Чуток нелогично, но раз зомби гоняет -- сам немного мертвяк. :)
			//Тут сам спелл бредовый... Но пока на скорую руку.
			if (!can_use_feat(tch, ZOMBIE_DROVER_FEAT))
				continue;
		}
		mag_affects(GET_LEVEL(ch), ch, tch, SPELL_HOLYSTRIKE, SAVING_STABILITY);
		mag_damage(GET_LEVEL(ch), ch, tch, SPELL_HOLYSTRIKE, SAVING_STABILITY);
	}

	act(msg2, FALSE, ch, 0, 0, TO_CHAR);
	act(msg2, FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

	do
	{
		for (o = world[IN_ROOM(ch)]->contents; o; o = o->next_content)
		{
			if (!IS_CORPSE(o))
				continue;
			extract_obj(o);
			break;
		}
	}
	while (o);

}

ASPELL(spell_angel)
{
	mob_vnum mob_num = 108;
	int modifier = 0;

	CHAR_DATA *mob = NULL;
	AFFECT_DATA af;
	struct follow_type *k, *k_next;
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
	if (get_effective_cha(ch, SPELL_ANGEL) < 16 && !IS_IMMORTAL(ch))
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
	af.type = SPELL_CHARM;
	af.duration =
		pc_duration(mob, 5 + (int) VPOSI((get_effective_cha(ch, SPELL_ANGEL) - 16.0) / 2, 0, 50), 0, 0, 0, 0);
	af.modifier = 0;
	af.location = 0;
	af.bitvector = AFF_HELPER;
	af.battleflag = 0;
	affect_to_char(mob, &af);

	if (IS_FEMALE(ch))
	{
		GET_SEX(mob) = SEX_MALE;
		mob->set_pc_name("Небесный защитник");
		GET_PAD(mob, 0) = str_dup("Небесный защитник");
		GET_PAD(mob, 1) = str_dup("Небесного защитника");
		GET_PAD(mob, 2) = str_dup("Небесному защитнику");
		GET_PAD(mob, 3) = str_dup("Небесного защитника");
		GET_PAD(mob, 4) = str_dup("Небесным защитником");
		GET_PAD(mob, 5) = str_dup("Небесном защитнике");
		mob->set_npc_name("Небесный защитник");
		mob->player_data.long_descr = str_dup("Небесный защитник летает тут.\r\n");
		mob->player_data.description = str_dup("Сияющая призрачная фигура о двух крылах.\r\n");
	}
	else
	{
		GET_SEX(mob) = SEX_FEMALE;
		mob->set_pc_name("Небесная защитница");
		GET_PAD(mob, 0) = str_dup("Небесная защитница");
		GET_PAD(mob, 1) = str_dup("Небесной защитницы");
		GET_PAD(mob, 2) = str_dup("Небесной защитнице");
		GET_PAD(mob, 3) = str_dup("Небесную защитницу");
		GET_PAD(mob, 4) = str_dup("Небесной защитницей");
		GET_PAD(mob, 5) = str_dup("Небесной защитнице");
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
		SET_BIT(PRF_FLAGS(mob, PRF_AWAKE), PRF_AWAKE);

	GET_LIKES(mob) = 100;
	IS_CARRYING_W(mob) = 0;
	IS_CARRYING_N(mob) = 0;

	SET_BIT(MOB_FLAGS(mob, MOB_CORPSE), MOB_CORPSE);
	SET_BIT(MOB_FLAGS(mob, MOB_ANGEL), MOB_ANGEL);
	SET_BIT(MOB_FLAGS(mob, MOB_LIGHTBREATH), MOB_LIGHTBREATH);

	SET_BIT(AFF_FLAGS(mob, AFF_FLY), AFF_FLY);
	SET_BIT(AFF_FLAGS(mob, AFF_INFRAVISION), AFF_INFRAVISION);
	mob->set_level(ch->get_level());
//----------------------------------------------------------------------
// добавляем зависимости от уровня и от обаяния
// level

	modifier = (int)(5 * VPOSI(GET_LEVEL(ch) - 26, 0, 50)
					 + 5 * VPOSI(get_effective_cha(ch, SPELL_ANGEL) - 16, 0, 50));

	mob->set_skill(SKILL_RESCUE, mob->get_skill(SKILL_RESCUE) + modifier);
	mob->set_skill(SKILL_AWAKE, mob->get_skill(SKILL_AWAKE) + modifier);
	mob->set_skill(SKILL_PUNCH, mob->get_skill(SKILL_PUNCH) + modifier);
	mob->set_skill(SKILL_BLOCK, mob->get_skill(SKILL_BLOCK) + modifier);

	modifier = (int)(2 * VPOSI(GET_LEVEL(ch) - 26, 0, 50)
					 + 1 * VPOSI(get_effective_cha(ch, SPELL_ANGEL) - 16, 0, 50));
	GET_HR(mob) += modifier;

	modifier = VPOSI(GET_LEVEL(ch) - 26, 0, 50);
	mob->inc_con(modifier);

	modifier = (int)(20 * VPOSI(get_effective_cha(ch, SPELL_ANGEL) - 16, 0, 50));
	GET_MAX_HIT(mob) += modifier;
	GET_HIT(mob) += modifier;

	modifier = (int)(3 * VPOSI(get_effective_cha(ch, SPELL_ANGEL) - 16, 0, 50));
	GET_AC(mob) -= modifier;

	modifier = 1 * VPOSI((int)((get_effective_cha(ch, SPELL_ANGEL) - 16) / 2), 0, 50);
	mob->inc_str(modifier);
	mob->inc_dex(modifier);

	modifier = VPOSI((int)((get_effective_cha(ch, SPELL_ANGEL) - 22) / 4), 0, 50);
	SET_SPELL(mob, SPELL_HEAL, GET_SPELL_MEM(mob, SPELL_HEAL) + modifier);

	if (get_effective_cha(ch, SPELL_ANGEL) >= 26)
		mob->mob_specials.ExtraAttack += 1;

	if (get_effective_cha(ch, SPELL_ANGEL) >= 24)
	{
		mob->mob_specials.damnodice += 1;
		mob->mob_specials.damsizedice += 1;
	}

	if (get_effective_cha(ch, SPELL_ANGEL) >= 22)
		SET_BIT(AFF_FLAGS(mob, AFF_SANCTUARY), AFF_SANCTUARY);

	if (get_effective_cha(ch, SPELL_ANGEL) >= 30)
		SET_BIT(AFF_FLAGS(mob, AFF_AIRSHIELD), AFF_AIRSHIELD);



//sprintf(buf,"RESCUE= %d",get_skill(mob,SKILL_RESCUE));
//send_to_char(buf, ch);

//    GET_CLASS(mob)       = GET_CLASS(ch);

	char_to_room(mob, IN_ROOM(ch));

	if (IS_FEMALE(mob))
	{
//   act("Небесная защитница шагнула к вам из сгустка света!", FALSE, ch, 0, 0, TO_CHAR);
		act("Небесная защитница появилась в яркой вспышке света!", TRUE, mob, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	}
	else
	{
//   act("Небесный защитник шагнул к вам из сгустка света!", FALSE, ch, 0, 0, TO_CHAR);
		act("Небесный защитник появился в яркой вспышке света!", TRUE, mob, 0, 0, TO_ROOM);
	}
	add_follower(mob, ch);
	return;
}
