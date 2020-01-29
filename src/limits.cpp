/* ************************************************************************
*   File: limits.cpp                                      Part of Bylins    *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
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

#include "world.objects.hpp"
#include "world.characters.hpp"
#include "obj.hpp"
#include "spells.h"
#include "skills.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "screen.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "house.h"
#include "constants.h"
#include "exchange.h"
#include "top.h"
#include "deathtrap.hpp"
#include "ban.hpp"
#include "depot.hpp"
#include "glory.hpp"
#include "features.hpp"
#include "char.hpp"
#include "char_player.hpp"
#include "room.hpp"
#include "birth_places.hpp"
#include "objsave.h"
#include "fight.h"
#include "ext_money.hpp"
#include "mob_stat.hpp"
#include "spell_parser.hpp"
#include "zone.table.hpp"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <boost/format.hpp>

extern int check_dupes_host(DESCRIPTOR_DATA * d, bool autocheck = 0);

extern room_rnum r_unreg_start_room;

extern CHAR_DATA *mob_proto;

extern DESCRIPTOR_DATA *descriptor_list;
extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
extern int free_rent;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_helled_start_room;
extern room_rnum r_named_start_room;
extern struct spell_create_type spell_create[];
extern const unsigned RECALL_SPELLS_INTERVAL;
extern int CheckProxy(DESCRIPTOR_DATA * ch);
extern int check_death_ice(int room, CHAR_DATA * ch);

void decrease_level(CHAR_DATA * ch);
int max_exp_gain_pc(CHAR_DATA * ch);
int max_exp_loss_pc(CHAR_DATA * ch);
int average_day_temp(void);

// local functions
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
int level_exp(CHAR_DATA * ch, int level);
void update_char_objects(CHAR_DATA * ch);	// handler.cpp
// Delete this, if you delete overflow fix in beat_points_update below.
void die(CHAR_DATA * ch, CHAR_DATA * killer);
// When age < 20 return the value p0 //
// When age in 20..29 calculate the line between p1 & p2 //
// When age in 30..34 calculate the line between p2 & p3 //
// When age in 35..49 calculate the line between p3 & p4 //
// When age in 50..74 calculate the line between p4 & p5 //
// When age >= 75 return the value p6 //
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

	if (age < 20)
		return (p0);	// < 20   //
	else if (age <= 29)
		return (int)(p1 + (((age - 20) * (p2 - p1)) / 10));	// 20..29 //
	else if (age <= 34)
		return (int)(p2 + (((age - 30) * (p3 - p2)) / 5));	// 30..34 //
	else if (age <= 49)
		return (int)(p3 + (((age - 35) * (p4 - p3)) / 15));	// 35..59 //
	else if (age <= 74)
		return (int)(p4 + (((age - 50) * (p5 - p4)) / 25));	// 50..74 //
	else
		return (p6);	// >= 80 //
}

void handle_recall_spells(CHAR_DATA* ch)
{
	AFFECT_DATA<EApplyLocation>::shared_ptr aff;
	for (const auto& af : ch->affected)
	{
		if (af->type == SPELL_RECALL_SPELLS)
		{
			aff = af;
			break;
		}
	}

	if (!aff)
	{
		return;
	}

	//максимальный доступный чару круг
	unsigned max_slot = max_slots.get(ch);
	//обрабатываем только каждые RECALL_SPELLS_INTERVAL секунд
	int secs_left = (SECS_PER_PLAYER_AFFECT*aff->duration) / SECS_PER_MUD_HOUR - SECS_PER_PLAYER_AFFECT;
	if (secs_left / RECALL_SPELLS_INTERVAL < max_slot - aff->modifier || secs_left <= 2)
	{
		int slot_to_restore = aff->modifier++;

		bool found_spells = false;
		struct spell_mem_queue_item *next = NULL, *prev = NULL, *i = ch->MemQueue.queue;
		while (i)
		{
			next = i->link;
			if (spell_info[i->spellnum].slot_forc[(int)GET_CLASS(ch)][(int)GET_KIN(ch)] == slot_to_restore)
			{
				if (!found_spells)
				{
					send_to_char("Ваша голова прояснилась, в памяти всплыло несколько новых заклинаний.\r\n", ch);
					found_spells = true;
				}
				if (prev) prev->link = next;
				if (i == ch->MemQueue.queue)
				{
					ch->MemQueue.queue = next;
					GET_MEM_COMPLETED(ch) = 0;
				}
				GET_MEM_TOTAL(ch) = MAX(0, GET_MEM_TOTAL(ch) - mag_manacost(ch, i->spellnum));
				sprintf(buf, "Вы вспомнили заклинание \"%s%s%s\".\r\n",
					CCICYN(ch, C_NRM), spell_info[i->spellnum].name, CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				GET_SPELL_MEM(ch, i->spellnum)++;
				free(i);
			}
			else prev = i;
			i = next;
		}
	}
}
void handle_recall_spells(CHAR_DATA::shared_ptr& ch) { handle_recall_spells(ch.get()); }

/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

 // manapoint gain pr. game hour
int mana_gain(const CHAR_DATA * ch)
{
	int gain = 0, restore = int_app[GET_REAL_INT(ch)].mana_per_tic, percent = 100;
	int stopmem = FALSE;

	if (IS_NPC(ch))
	{
		gain = GET_LEVEL(ch);
	}
	else
	{
		if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
			return (0);

		if (!IS_MANA_CASTER(ch))
		{
			gain = graf(age(ch)->year, restore - 8, restore - 4, restore,
				restore + 5, restore, restore - 4, restore - 8);
		}
		else
		{
			gain = mana_gain_cs[GET_REAL_INT(ch)];
		}

		// Room specification
		if (LIKE_ROOM(ch))
			percent += 25;
		// Weather specification
		if (average_day_temp() < -20)
			percent -= 10;
		else if (average_day_temp() < -10)
			percent -= 5;
	}

	if (world[ch->in_room]->fires)
		percent += MAX(100, 10 + (world[ch->in_room]->fires * 5) * 2);

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_DEAFNESS))
		percent += 15;

	// Skill/Spell calculations


	// Position calculations
	if (ch->get_fighting())
	{
		if (IS_MANA_CASTER(ch))
			percent -= 50;
		else
			percent -= 90;
	}
	else
		switch (GET_POS(ch))
		{
		case POS_SLEEPING:
			if (IS_MANA_CASTER(ch))
			{
				percent += 80;
			}
			else
			{
				stopmem = TRUE;
				percent = 0;
			}
			break;
		case POS_RESTING:
			percent += 45;
			break;
		case POS_SITTING:
			percent += 30;
			break;
		case POS_STANDING:
			break;
		default:
			stopmem = TRUE;
			percent = 0;
			break;
		}

	if (!IS_MANA_CASTER(ch) &&
		(AFF_FLAGGED(ch, EAffectFlag::AFF_HOLD) ||
			AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND) ||
			AFF_FLAGGED(ch, EAffectFlag::AFF_SLEEP) ||
			((ch->in_room != NOWHERE) && IS_DARK(ch->in_room) && !can_use_feat(ch, DARK_READING_FEAT))))
	{
		stopmem = TRUE;
		percent = 0;
	}

	if (!IS_MANA_CASTER(ch))
		percent += GET_MANAREG(ch);
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_POISON) && percent > 0)
		percent /= 4;
	if (!IS_NPC(ch))
		percent *= ch->get_cond_penalty(P_MEM_GAIN);
	percent = MAX(0, MIN(250, percent));
	gain = gain * percent / 100;
	return (stopmem ? 0 : gain);
}
int mana_gain(const CHAR_DATA::shared_ptr& ch) { return mana_gain(ch.get()); }

// Hitpoint gain pr. game hour
int hit_gain(CHAR_DATA* ch)
{
	int gain = 0, restore = MAX(10, GET_REAL_CON(ch) * 3 / 2), percent = 100;

	if (IS_NPC(ch))
		gain = GET_LEVEL(ch) + GET_REAL_CON(ch);
	else
	{
		if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
			return (0);

		if (!AFF_FLAGGED(ch, EAffectFlag::AFF_NOOB_REGEN))
		{
			gain = graf(age(ch)->year, restore - 3, restore, restore, restore - 2,
				restore - 3, restore - 5, restore - 7);
		}
		else
		{
			const double base_hp = std::max(1, PlayerSystem::con_total_hp(ch));
			const double rest_time = 80 + 10 * GET_LEVEL(ch);
			gain = base_hp / rest_time * 60;
		}

		// Room specification    //
		if (LIKE_ROOM(ch))
			percent += 25;
		// Weather specification //
		if (average_day_temp() < -20)
			percent -= 15;
		else if (average_day_temp() < -10)
			percent -= 10;
	}

	if (world[ch->in_room]->fires)
		percent += MAX(100, 10 + (world[ch->in_room]->fires * 5) * 2);

	// Skill/Spell calculations //

	// Position calculations    //
	switch (GET_POS(ch))
	{
	case POS_SLEEPING:
		percent += 25;
		break;
	case POS_RESTING:
		percent += 15;
		break;
	case POS_SITTING:
		percent += 10;
		break;
	}

	percent += GET_HITREG(ch);

	// TODO: перевоткнуть на apply_аффект
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_POISON) && percent > 0)
		percent /= 4;

	if (!IS_NPC(ch))
		percent *= ch->get_cond_penalty(P_HIT_GAIN);
	percent = MAX(0, MIN(250, percent));
	gain = gain * percent / 100;
	if (!IS_NPC(ch))
	{
		if (GET_POS(ch) == POS_INCAP || GET_POS(ch) == POS_MORTALLYW)
			gain = 0;
	}
	return (gain);
}
int hit_gain(const CHAR_DATA::shared_ptr& ch) { return hit_gain(ch.get()); }

// move gain pr. game hour //
int move_gain(CHAR_DATA* ch)
{
	int gain = 0, restore = GET_REAL_CON(ch) / 2, percent = 100;

	if (IS_NPC(ch))
		gain = GET_LEVEL(ch);
	else
	{
		if (!ch->desc || STATE(ch->desc) != CON_PLAYING)
			return (0);
		gain =
			graf(age(ch)->year, 15 + restore, 20 + restore, 25 + restore,
				20 + restore, 16 + restore, 12 + restore, 8 + restore);
		// Room specification    //
		if (LIKE_ROOM(ch))
			percent += 25;
		// Weather specification //
		if (average_day_temp() < -20)
			percent -= 10;
		else if (average_day_temp() < -10)
			percent -= 5;
	}

	if (world[ch->in_room]->fires)
		percent += MAX(50, 10 + world[ch->in_room]->fires * 5);

	// Class/Level calculations //

	// Skill/Spell calculations //


	// Position calculations    //
	switch (GET_POS(ch))
	{
	case POS_SLEEPING:
		percent += 25;
		break;
	case POS_RESTING:
		percent += 15;
		break;
	case POS_SITTING:
		percent += 10;
		break;
	}



	percent += GET_MOVEREG(ch);
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_POISON) && percent > 0)
		percent /= 4;

	if (!IS_NPC(ch))
		percent *= ch->get_cond_penalty(P_HIT_GAIN);
	percent = MAX(0, MIN(250, percent));
	gain = gain * percent / 100;
	return (gain);
}
int move_gain(const CHAR_DATA::shared_ptr& ch) { return move_gain(ch.get()); }

#define MINUTE            60
#define UPDATE_PC_ON_BEAT TRUE

int interpolate(int min_value, int pulse)
{
	int sign = 1, int_p, frac_p, i, carry, x, y;

	if (min_value < 0)
	{
		sign = -1;
		min_value = -min_value;
	}
	int_p = min_value / MINUTE;
	frac_p = min_value % MINUTE;
	if (!frac_p)
		return (sign * int_p);
	pulse = time(NULL) % MINUTE + 1;
	x = MINUTE;
	y = 0;
	for (i = 0, carry = 0; i <= pulse; i++)
	{
		y += frac_p;
		if (y >= x)
		{
			x += MINUTE;
			carry = 1;
		}
		else
			carry = 0;
	}
	return (sign * (int_p + carry));
}
void beat_punish(const CHAR_DATA::shared_ptr& i)
{
	int restore;
	// Проверяем на выпуск чара из кутузки
	if (PLR_FLAGGED(i, PLR_HELLED) && HELL_DURATION(i) && HELL_DURATION(i) <= time(NULL))
	{
		restore = PLR_TOG_CHK(i, PLR_HELLED);
		if (HELL_REASON(i))
			free(HELL_REASON(i));
		HELL_REASON(i) = 0;
		GET_HELL_LEV(i) = 0;
		HELL_GODID(i) = 0;
		HELL_DURATION(i) = 0;
		send_to_char("Вас выпустили из темницы.\r\n", i.get());
		if ((restore = GET_LOADROOM(i)) == NOWHERE)
			restore = calc_loadroom(i.get());
		restore = real_room(restore);
		if (restore == NOWHERE)
		{
			if (GET_LEVEL(i) >= LVL_IMMORT)
				restore = r_immort_start_room;
			else
				restore = r_mortal_start_room;
		}
		char_from_room(i);
		char_to_room(i, restore);
		look_at_room(i.get(), restore);
		act("Насвистывая \"От звонка до звонка...\", $n появил$u в центре комнаты.", FALSE, i.get(), 0, 0, TO_ROOM);
	}

	if (PLR_FLAGGED(i, PLR_NAMED)
		&& NAME_DURATION(i)
		&& NAME_DURATION(i) <= time(NULL))
	{
		restore = PLR_TOG_CHK(i, PLR_NAMED);
		if (NAME_REASON(i))
		{
			free(NAME_REASON(i));
		}
		NAME_REASON(i) = 0;
		GET_NAME_LEV(i) = 0;
		NAME_GODID(i) = 0;
		NAME_DURATION(i) = 0;
		send_to_char("Вас выпустили из КОМНАТЫ ИМЕНИ.\r\n", i.get());

		if ((restore = GET_LOADROOM(i)) == NOWHERE)
		{
			restore = calc_loadroom(i.get());
		}

		restore = real_room(restore);

		if (restore == NOWHERE)
		{
			if (GET_LEVEL(i) >= LVL_IMMORT)
			{
				restore = r_immort_start_room;
			}
			else
			{
				restore = r_mortal_start_room;
			}
		}

		char_from_room(i);
		char_to_room(i, restore);
		look_at_room(i.get(), restore);
		act("С ревом \"Имья, сестра, имья...\", $n появил$u в центре комнаты.", FALSE, i.get(), 0, 0, TO_ROOM);
	}

	if (PLR_FLAGGED(i, PLR_MUTE)
		&& MUTE_DURATION(i) != 0
		&& MUTE_DURATION(i) <= time(NULL))
	{
		restore = PLR_TOG_CHK(i, PLR_MUTE);
		if (MUTE_REASON(i))
			free(MUTE_REASON(i));
		MUTE_REASON(i) = 0;
		GET_MUTE_LEV(i) = 0;
		MUTE_GODID(i) = 0;
		MUTE_DURATION(i) = 0;
		send_to_char("Вы можете орать.\r\n", i.get());
	}

	if (PLR_FLAGGED(i, PLR_DUMB)
		&& DUMB_DURATION(i) != 0
		&& DUMB_DURATION(i) <= time(NULL))
	{
		restore = PLR_TOG_CHK(i, PLR_DUMB);
		if (DUMB_REASON(i))
			free(DUMB_REASON(i));
		DUMB_REASON(i) = 0;
		GET_DUMB_LEV(i) = 0;
		DUMB_GODID(i) = 0;
		DUMB_DURATION(i) = 0;
		send_to_char("Вы можете говорить.\r\n", i.get());
	}


	if (!PLR_FLAGGED(i, PLR_REGISTERED)
		&& UNREG_DURATION(i) != 0
		&& UNREG_DURATION(i) <= time(NULL))
	{
		restore = PLR_TOG_CHK(i, PLR_REGISTERED);
		if (UNREG_REASON(i))
			free(UNREG_REASON(i));
		UNREG_REASON(i) = 0;
		GET_UNREG_LEV(i) = 0;
		UNREG_GODID(i) = 0;
		UNREG_DURATION(i) = 0;
		send_to_char("Ваша регистрация восстановлена.\r\n", i.get());

		if (IN_ROOM(i) == r_unreg_start_room)
		{
			if ((restore = GET_LOADROOM(i)) == NOWHERE)
			{
				restore = calc_loadroom(i.get());
			}

			restore = real_room(restore);

			if (restore == NOWHERE)
			{
				if (GET_LEVEL(i) >= LVL_IMMORT)
				{
					restore = r_immort_start_room;
				}
				else
				{
					restore = r_mortal_start_room;
				}
			}

			char_from_room(i);
			char_to_room(i, restore);
			look_at_room(i.get(), restore);

			act("$n появил$u в центре комнаты, с гордостью показывая всем штампик регистрации!",
				FALSE, i.get(), 0, 0, TO_ROOM);
		};

	}

	if (GET_GOD_FLAG(i, GF_GODSLIKE)
		&& GCURSE_DURATION(i) != 0
		&& GCURSE_DURATION(i) <= time(NULL))
	{
		CLR_GOD_FLAG(i, GF_GODSLIKE);
		send_to_char("Вы более не под защитой Богов.\r\n", i.get());
	}

	if (GET_GOD_FLAG(i, GF_GODSCURSE)
		&& GCURSE_DURATION(i) != 0
		&& GCURSE_DURATION(i) <= time(NULL))
	{
		CLR_GOD_FLAG(i, GF_GODSCURSE);
		send_to_char("Боги более не в обиде на вас.\r\n", i.get());
	}

	if (PLR_FLAGGED(i, PLR_FROZEN)
		&& FREEZE_DURATION(i) != 0
		&& FREEZE_DURATION(i) <= time(NULL))
	{
		restore = PLR_TOG_CHK(i, PLR_FROZEN);
		if (FREEZE_REASON(i))
		{
			free(FREEZE_REASON(i));
		}
		FREEZE_REASON(i) = 0;
		GET_FREEZE_LEV(i) = 0;
		FREEZE_GODID(i) = 0;
		FREEZE_DURATION(i) = 0;
		send_to_char("Вы оттаяли.\r\n", i.get());
		Glory::remove_freeze(GET_UNIQUE(i));
		if ((restore = GET_LOADROOM(i)) == NOWHERE)
		{
			restore = calc_loadroom(i.get());
		}
		restore = real_room(restore);
		if (restore == NOWHERE)
		{
			if (GET_LEVEL(i) >= LVL_IMMORT)
				restore = r_immort_start_room;
			else
				restore = r_mortal_start_room;
		}
		char_from_room(i);
		char_to_room(i, restore);
		look_at_room(i.get(), restore);
		act("Насвистывая \"От звонка до звонка...\", $n появил$u в центре комнаты.",
			FALSE, i.get(), 0, 0, TO_ROOM);
	}

	// Проверяем а там ли мы где должны быть по флагам.
	if (IN_ROOM(i) == STRANGE_ROOM)
	{
		restore = i->get_was_in_room();
	}
	else
	{
		restore = IN_ROOM(i);
	}

	if (PLR_FLAGGED(i, PLR_HELLED))
	{
		if (restore != r_helled_start_room)
		{
			if (IN_ROOM(i) == STRANGE_ROOM)
			{
				i->set_was_in_room(r_helled_start_room);
			}
			else
			{
				send_to_char("Чья-то злая воля вернула вас в темницу.\r\n", i.get());
				act("$n возвращен$a в темницу.", FALSE, i.get(), 0, 0, TO_ROOM);

				char_from_room(i);
				char_to_room(i, r_helled_start_room);
				look_at_room(i.get(), r_helled_start_room);

				i->set_was_in_room(NOWHERE);
			}
		}
	}
	else if (PLR_FLAGGED(i, PLR_NAMED))
	{
		if (restore != r_named_start_room)
		{
			if (IN_ROOM(i) == STRANGE_ROOM)
			{
				i->set_was_in_room(r_named_start_room);
			}
			else
			{
				send_to_char("Чья-то злая воля вернула вас в комнату имени.\r\n", i.get());
				act("$n возвращен$a в комнату имени.", FALSE, i.get(), 0, 0, TO_ROOM);
				char_from_room(i);
				char_to_room(i, r_named_start_room);
				look_at_room(i.get(), r_named_start_room);

				i->set_was_in_room(NOWHERE);
			};
		};
	}
	else if (!RegisterSystem::is_registered(i.get()) && i->desc && STATE(i->desc) == CON_PLAYING)
	{
		if (restore != r_unreg_start_room
			&& !RENTABLE(i)
			&& !DeathTrap::is_slow_dt(IN_ROOM(i))
			&& !check_dupes_host(i->desc, 1))
		{
			if (IN_ROOM(i) == STRANGE_ROOM)
			{
				i->set_was_in_room(r_unreg_start_room);
			}
			else
			{
				act("$n водворен$a в комнату для незарегистрированных игроков, играющих через прокси.\r\n",
					FALSE, i.get(), 0, 0, TO_ROOM);

				char_from_room(i);
				char_to_room(i, r_unreg_start_room);
				look_at_room(i.get(), r_unreg_start_room);

				i->set_was_in_room(NOWHERE);
			};
		}
		else if (restore == r_unreg_start_room && check_dupes_host(i->desc, 1) && !IS_IMMORTAL(i))
		{
			send_to_char("Неведомая вытолкнула вас из комнаты для незарегистрированных игроков.\r\n", i.get());
			act("$n появил$u в центре комнаты, правда без штампика регистрации...\r\n",
				FALSE, i.get(), 0, 0, TO_ROOM);
			restore = i->get_was_in_room();
			if (restore == NOWHERE
				|| restore == r_unreg_start_room)
			{
				restore = GET_LOADROOM(i);
				if (restore == NOWHERE)
				{
					restore = calc_loadroom(i.get());
				}
				restore = real_room(restore);
			}

			char_from_room(i);
			char_to_room(i, restore);
			look_at_room(i.get(), restore);

			i->set_was_in_room(NOWHERE);
		}
	}
}

void beat_points_update(int pulse)
{
	int restore;

	if (!UPDATE_PC_ON_BEAT)
		return;

	// only for PC's
	character_list.foreach_on_copy([&](const auto& i)
	{
		if (IS_NPC(i))
			return;

		if (IN_ROOM(i) == NOWHERE)
		{
			log("SYSERR: Pulse character in NOWHERE.");
			return;
		}

		if (RENTABLE(i) <= time(NULL))
		{
			RENTABLE(i) = 0;
			AGRESSOR(i) = 0;
			AGRO(i) = 0;
			i->agrobd = false;
		}

		if (AGRO(i) < time(NULL))
		{
			AGRO(i) = 0;
		}
		beat_punish(i);

		// This line is used only to control all situations when someone is
		// dead (POS_DEAD). You can comment it to minimize heartbeat function
		// working time, if you're sure, that you control these situations
		// everywhere. To the time of this code revision I've fix some of them
		// and haven't seen any other.
		//             if (GET_POS(i) == POS_DEAD)
		//                     die(i, NULL);

		if (GET_POS(i) < POS_STUNNED)
		{
			return;
		}

		// Restore hitpoints
		restore = hit_gain(i);
		restore = interpolate(restore, pulse);

		if (AFF_FLAGGED(i, EAffectFlag::AFF_BANDAGE))
		{
			for (const auto& aff : i->affected)
			{
				if (aff->type == SPELL_BANDAGE)
				{
					restore += MIN(GET_REAL_MAX_HIT(i) / 10, aff->modifier);
					break;
				}
			}
		}

		if (GET_HIT(i) < GET_REAL_MAX_HIT(i))
		{
			GET_HIT(i) = MIN(GET_HIT(i) + restore, GET_REAL_MAX_HIT(i));
		}

		// Проверка аффекта !исступление!. Поместил именно здесь,
		// но если кто найдет более подходящее место переносите =)
		//Gorrah: перенес в handler::affect_total
		//check_berserk(i);

		// Restore PC caster mem
		if (!IS_MANA_CASTER(i) && !MEMQUEUE_EMPTY(i))
		{
			restore = mana_gain(i);
			restore = interpolate(restore, pulse);
			GET_MEM_COMPLETED(i) += restore;

			if (AFF_FLAGGED(i, EAffectFlag::AFF_RECALL_SPELLS))
			{
				handle_recall_spells(i.get());
			}

			while (GET_MEM_COMPLETED(i) > GET_MEM_CURRENT(i.get())
				&& !MEMQUEUE_EMPTY(i))
			{
				int spellnum;
				spellnum = MemQ_learn(i);
				GET_SPELL_MEM(i, spellnum)++;
				GET_CASTER(i) += spell_info[spellnum].danger;
			}

			if (MEMQUEUE_EMPTY(i))
			{
				if (GET_RELIGION(i) == RELIGION_MONO)
				{
					send_to_char("Наконец ваши занятия окончены. Вы с улыбкой захлопнули свой часослов.\r\n", i.get());
					act("Окончив занятия, $n с улыбкой захлопнул$g часослов.", FALSE, i.get(), 0, 0, TO_ROOM);
				}
				else
				{
					send_to_char("Наконец ваши занятия окончены. Вы с улыбкой убрали свои резы.\r\n", i.get());
					act("Окончив занятия, $n с улыбкой убрал$g резы.", FALSE, i.get(), 0, 0, TO_ROOM);
				}
			}
		}

		if (!IS_MANA_CASTER(i) && MEMQUEUE_EMPTY(i))
		{
			GET_MEM_TOTAL(i) = 0;
			GET_MEM_COMPLETED(i) = 0;
		}

		// Гейн маны у волхвов
		if (IS_MANA_CASTER(i) && GET_MANA_STORED(i) < GET_MAX_MANA(i.get())) {
			GET_MANA_STORED(i) += mana_gain(i);
			if (GET_MANA_STORED(i) >= GET_MAX_MANA(i.get())) {
				GET_MANA_STORED(i) = GET_MAX_MANA(i.get());
				send_to_char("Ваша магическая энергия полностью восстановилась\r\n", i.get());
			}
		}

		if (IS_MANA_CASTER(i) && GET_MANA_STORED(i) > GET_MAX_MANA(i.get())) {
			GET_MANA_STORED(i) = GET_MAX_MANA(i.get());
		}

		// Restore moves
		restore = move_gain(i);
		restore = interpolate(restore, pulse);

		//MZ.overflow_fix
		if (GET_MOVE(i) < GET_REAL_MAX_MOVE(i))
		{
			GET_MOVE(i) = MIN(GET_MOVE(i) + restore, GET_REAL_MAX_MOVE(i));
		}
		//-MZ.overflow_fix
	});
}

void update_clan_exp(CHAR_DATA *ch, int gain)
{
	if (CLAN(ch) && gain != 0)
	{
		// экспа для уровня клана (+ только на праве, - любой, но /5)
		if (gain < 0 || GET_GOD_FLAG(ch, GF_REMORT))
		{
			int tmp = gain > 0 ? gain : gain / 5;
			CLAN(ch)->SetClanExp(ch, tmp);
		}
		// экспа для топа кланов за месяц (учитываются все + и -)
		CLAN(ch)->last_exp.add_temp(gain);
		// экспа для топа кланов за все время (учитываются все + и -)
		CLAN(ch)->AddTopExp(ch, gain);
		// экспа для авто-очистки кланов (учитываются только +)
		if (gain > 0)
		{
			CLAN(ch)->exp_history.add_exp(gain);
		}
	}
}

void gain_exp(CHAR_DATA * ch, int gain)
{
	int is_altered = FALSE;
	int num_levels = 0;
	char buf[128];

	if (IS_NPC(ch))
	{
		ch->set_exp(ch->get_exp() + gain);
		return;
	}
	else
	{
		ch->dps_add_exp(gain);
		ZoneExpStat::add(zone_table[world[ch->in_room]->zone].number, gain);
	}

	if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
		return;

	if (gain > 0 && GET_LEVEL(ch) < LVL_IMMORT)
	{
		gain = MIN(max_exp_gain_pc(ch), gain);	// put a cap on the max gain per kill
		ch->set_exp(ch->get_exp() + gain);
		if (GET_EXP(ch) >= level_exp(ch, LVL_IMMORT))
		{
			if (!GET_GOD_FLAG(ch, GF_REMORT) && GET_REMORT(ch) < MAX_REMORT)
			{
				if (Remort::can_remort_now(ch))
				{
					send_to_char(ch, "%sПоздравляем, вы получили право на перевоплощение!%s\r\n",
						CCIGRN(ch, C_NRM), CCNRM(ch, C_NRM));
				}
				else
				{
					send_to_char(ch,
						"%sПоздравляем, вы набрали максимальное количество опыта!\r\n"
						"%s%s\r\n", CCIGRN(ch, C_NRM), Remort::WHERE_TO_REMORT_STR.c_str(), CCNRM(ch, C_NRM));
				}
				SET_GOD_FLAG(ch, GF_REMORT);
			}
		}
		ch->set_exp(MIN(GET_EXP(ch), level_exp(ch, LVL_IMMORT) - 1));
		while (GET_LEVEL(ch) < LVL_IMMORT && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
		{
			ch->set_level(ch->get_level() + 1);
			num_levels++;
			sprintf(buf, "%sВы достигли следующего уровня!%s\r\n", CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
			advance_level(ch);
			is_altered = TRUE;
		}

		if (is_altered)
		{
			sprintf(buf, "%s advanced %d level%s to level %d.",
				GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
			mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
		}
	}
	else if (gain < 0 && GET_LEVEL(ch) < LVL_IMMORT)
	{
		gain = MAX(-max_exp_loss_pc(ch), gain);	// Cap max exp lost per death
		ch->set_exp(ch->get_exp() + gain);
		while (GET_LEVEL(ch) > 1 && GET_EXP(ch) < level_exp(ch, GET_LEVEL(ch)))
		{
			ch->set_level(ch->get_level() - 1);
			num_levels++;
			sprintf(buf,
				"%sВы потеряли уровень. Вам должно быть стыдно!%s\r\n",
				CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
			decrease_level(ch);
			is_altered = TRUE;
		}
		if (is_altered)
		{
			sprintf(buf, "%s decreases %d level%s to level %d.",
				GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
			mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
		}
	}
	if ((GET_EXP(ch) < level_exp(ch, LVL_IMMORT) - 1)
		&& GET_GOD_FLAG(ch, GF_REMORT)
		&& gain
		&& (GET_LEVEL(ch) < LVL_IMMORT))
	{
		if (Remort::can_remort_now(ch))
		{
			send_to_char(ch, "%sВы потеряли право на перевоплощение!%s\r\n",
				CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
		}
		CLR_GOD_FLAG(ch, GF_REMORT);
	}

	char_stat::add_class_exp(GET_CLASS(ch), gain);
	update_clan_exp(ch, gain);
}

// юзается исключительно в act.wizards.cpp в имм командах "advance" и "set exp".
void gain_exp_regardless(CHAR_DATA * ch, int gain)
{
	int is_altered = FALSE;
	int num_levels = 0;

	ch->set_exp(ch->get_exp() + gain);
	if (!IS_NPC(ch))
	{
		if (gain > 0)
		{
			while (GET_LEVEL(ch) < LVL_IMPL && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
			{
				ch->set_level(ch->get_level() + 1);
				num_levels++;
				sprintf(buf, "%sВы достигли следующего уровня!%s\r\n",
					CCWHT(ch, C_NRM), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);

				advance_level(ch);
				is_altered = TRUE;
			}

			if (is_altered)
			{
				sprintf(buf, "%s advanced %d level%s to level %d.",
					GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
				mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
			}
		}
		else if (gain < 0)
		{
			// Pereplut: глупый участок кода.
			//			gain = MAX(-max_exp_loss_pc(ch), gain);	// Cap max exp lost per death
			//			GET_EXP(ch) += gain;
			//			if (GET_EXP(ch) < 0)
			//				GET_EXP(ch) = 0;
			while (GET_LEVEL(ch) > 1 && GET_EXP(ch) < level_exp(ch, GET_LEVEL(ch)))
			{
				ch->set_level(ch->get_level() - 1);
				num_levels++;
				sprintf(buf,
					"%sВы потеряли уровень!%s\r\n",
					CCIRED(ch, C_NRM), CCNRM(ch, C_NRM));
				send_to_char(buf, ch);
				decrease_level(ch);
				is_altered = TRUE;
			}
			if (is_altered)
			{
				sprintf(buf, "%s decreases %d level%s to level %d.",
					GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
				mudlog(buf, BRF, LVL_IMPL, SYSLOG, TRUE);
			}
		}

	}
}

void gain_condition(CHAR_DATA * ch, unsigned condition, int value)
{
	int cond_state = GET_COND(ch, condition);

	if (condition >= ch->player_specials->saved.conditions.size())
	{
		log("SYSERROR : condition=%d (%s:%d)", condition, __FILE__, __LINE__);
		return;
	}

	if (IS_NPC(ch) || GET_COND(ch, condition) == -1)
	{
		return;
	}

	if (IS_GOD(ch) && condition != DRUNK)
	{
		GET_COND(ch, condition) = -1;
		return;
	}

	GET_COND(ch, condition) += value;
	GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));

	// обработка после увеличения
	switch (condition)
	{
	case DRUNK:
		GET_COND(ch, condition) = MIN(CHAR_MORTALLY_DRUNKED + 1, GET_COND(ch, condition));
		break;

	default:
		GET_COND(ch, condition) = MIN(MAX_COND_VALUE, GET_COND(ch, condition));
		break;
	}

	if (cond_state >= CHAR_DRUNKED && GET_COND(ch, DRUNK) < CHAR_DRUNKED)
	{
		GET_DRUNK_STATE(ch) = 0;
	}

	if (PLR_FLAGGED(ch, PLR_WRITING))
		return;

	int cond_value = GET_COND(ch, condition);
	switch (condition)
	{
	case FULL:
		if (!GET_COND_M(ch, condition))
		{
			return;
		}

		if (cond_value < 30)
		{
			send_to_char("Вы голодны.\r\n", ch);
		}
		else if (cond_value < 40)
		{
			send_to_char("Вы очень голодны.\r\n", ch);
		}
		else
		{
			send_to_char("Вы готовы сожрать быка.\r\n", ch);
			//сюда оповещение можно вставить что бы люди видели что чар страдает
		}
		return;

	case THIRST:
		if (!GET_COND_M(ch, condition))
		{
			return;
		}

		if (cond_value < 30)
		{
			send_to_char("Вас мучает жажда.\r\n", ch);
		}
		else if (cond_value < 40)
		{
			send_to_char("Вас сильно мучает жажда.\r\n", ch);
		}
		else
		{
			send_to_char("Вам хочется выпить озеро.\r\n", ch);
			//сюда оповещение можно вставить что бы люди видели что чар страдает
		}
		return;

	case DRUNK:
		//Если чара прекратило штормить, шлем сообщение
		if (cond_state >= CHAR_MORTALLY_DRUNKED && GET_COND(ch, DRUNK) < CHAR_MORTALLY_DRUNKED)
		{
			send_to_char("Наконец-то вы протрезвели.\r\n", ch);
		}
		return;

	default:
		break;
	}
}

void underwater_check()
{
	DESCRIPTOR_DATA *d;
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->character
			&& SECT(d->character->in_room) == SECT_UNDERWATER
			&& !IS_GOD(d->character)
			&& !AFF_FLAGGED(d->character, EAffectFlag::AFF_WATERBREATH))
		{
			sprintf(buf, "Player %s died under water (room %d)",
				GET_NAME(d->character), GET_ROOM_VNUM(d->character->in_room));

			Damage dmg(SimpleDmg(TYPE_WATERDEATH), MAX(1, GET_REAL_MAX_HIT(d->character) >> 2), FightSystem::UNDEF_DMG);
			dmg.flags.set(FightSystem::NO_FLEE_DMG);

			if (dmg.process(d->character.get(), d->character.get()) < 0)
			{
				log("%s", buf);
			}
		}
	}
}

void check_idling(CHAR_DATA * ch)
{
	if (!RENTABLE(ch))
	{
		if (++(ch->char_specials.timer) > idle_void)
		{
			ch->set_motion(false);

			if (ch->get_was_in_room() == NOWHERE && ch->in_room != NOWHERE)
			{
				if (ch->desc)
				{
					ch->save_char();
				}
				ch->set_was_in_room(ch->in_room);
				if (ch->get_fighting())
				{
					stop_fighting(ch->get_fighting(), FALSE);
					stop_fighting(ch, TRUE);
				}
				act("$n растворил$u в пустоте.", TRUE, ch, 0, 0, TO_ROOM);
				send_to_char("Вы пропали в пустоте этого мира.\r\n", ch);

				Crash_crashsave(ch);
				char_from_room(ch);
				char_to_room(ch, STRANGE_ROOM);
				remove_rune_label(ch);
			}
			else if (ch->char_specials.timer > idle_rent_time)
			{
				if (ch->in_room != NOWHERE)
					char_from_room(ch);
				char_to_room(ch, STRANGE_ROOM);
				if (free_rent)
					Crash_rentsave(ch, 0);
				else
					Crash_idlesave(ch);
				Depot::exit_char(ch);
				Clan::clan_invoice(ch, false);
				sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
				mudlog(buf, NRM, LVL_GOD, SYSLOG, TRUE);
				extract_char(ch, FALSE);

				// чара в лд уже посейвило при обрыве коннекта
				if (ch->desc)
				{
					STATE(ch->desc) = CON_DISCONNECT;
					/*
					* For the 'if (d->character)' test in close_socket().
					* -gg 3/1/98 (Happy anniversary.)
					*/
					ch->desc->character = NULL;
					ch->desc = NULL;
				}

			}
		}
	}
}

// Update PCs, NPCs, and objects
inline bool NO_DESTROY(const OBJ_DATA* obj)
{
	return (obj->get_carried_by()
		|| obj->get_worn_by()
		|| obj->get_in_obj()
		|| (obj->get_script()->has_triggers())
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_FOUNTAIN
		|| obj->get_in_room() == NOWHERE
		|| (obj->get_extra_flag(EExtraFlag::ITEM_NODECAY)
			&& !ROOM_FLAGGED(obj->get_in_room(), ROOM_DEATH)));
}

inline bool NO_TIMER(const OBJ_DATA* obj)
{
	return GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_FOUNTAIN;
}

int up_obj_where(OBJ_DATA * obj)
{
	if (obj->get_in_obj())
	{
		return up_obj_where(obj->get_in_obj());
	}
	else
	{
		return OBJ_WHERE(obj);
	}
}

void hour_update(void)
{
	DESCRIPTOR_DATA *i;

	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) != CON_PLAYING || i->character == NULL || PLR_FLAGGED(i->character, PLR_WRITING))
			continue;
		sprintf(buf, "%sМинул час.%s\r\n", CCIRED(i->character, C_NRM), CCNRM(i->character, C_NRM));
		SEND_TO_Q(buf, i);
	}
}

void room_point_update()
{
	int mana;
	for (int count = FIRST_ROOM; count <= top_of_world; count++)
	{
		if (world[count]->fires)
		{
			switch (get_room_sky(count))
			{
			case SKY_CLOUDY:
			case SKY_CLOUDLESS:
				mana = number(1, 2);
				break;
			case SKY_RAINING:
				mana = 2;
				break;
			default:
				mana = 1;
			}
			world[count]->fires -= MIN(mana, world[count]->fires);
			if (world[count]->fires <= 0)
			{
				act("Костер затух.", FALSE, world[count]->first_character(), 0, 0, TO_ROOM);
				act("Костер затух.", FALSE, world[count]->first_character(), 0, 0, TO_CHAR);

				world[count]->fires = 0;
			}
		}

		if (world[count]->portal_time)
		{
			world[count]->portal_time--;
			if (!world[count]->portal_time)
			{
				OneWayPortal::remove(world[count]);
				world[count]->pkPenterUnique = 0;
				act("Пентаграмма медленно растаяла.", FALSE, world[count]->first_character(), 0, 0, TO_ROOM);
				act("Пентаграмма медленно растаяла.", FALSE, world[count]->first_character(), 0, 0, TO_CHAR);
			}
		}
		if (world[count]->holes)
		{
			world[count]->holes--;
			if (!world[count]->holes || roundup(world[count]->holes) == world[count]->holes)
			{
				act("Ямку присыпало землей...", FALSE, world[count]->first_character(), 0, 0, TO_ROOM);
				act("Ямку присыпало землей...", FALSE, world[count]->first_character(), 0, 0, TO_CHAR);
			}
		}
		if (world[count]->ices)
			if (!--world[count]->ices)
			{
				GET_ROOM(count)->unset_flag(ROOM_ICEDEATH);
				DeathTrap::remove(world[count]);
			}

		world[count]->glight = MAX(0, world[count]->glight);
		world[count]->gdark = MAX(0, world[count]->gdark);

		struct track_data *track, *next_track, *temp;
		int spellnum;
		for (track = world[count]->track, temp = NULL; track; track = next_track)
		{
			next_track = track->next;
			switch (real_sector(count))
			{
			case SECT_FLYING:
			case SECT_UNDERWATER:
			case SECT_SECRET:
			case SECT_WATER_SWIM:
			case SECT_WATER_NOSWIM:
				spellnum = 31;
				break;
			case SECT_THICK_ICE:
			case SECT_NORMAL_ICE:
			case SECT_THIN_ICE:
				spellnum = 16;
				break;
			case SECT_CITY:
				spellnum = 4;
				break;
			case SECT_FIELD:
			case SECT_FIELD_RAIN:
				spellnum = 2;
				break;
			case SECT_FIELD_SNOW:
				spellnum = 1;
				break;
			case SECT_FOREST:
			case SECT_FOREST_RAIN:
				spellnum = 2;
				break;
			case SECT_FOREST_SNOW:
				spellnum = 1;
				break;
			case SECT_HILLS:
			case SECT_HILLS_RAIN:
				spellnum = 3;
				break;
			case SECT_HILLS_SNOW:
				spellnum = 1;
				break;
			case SECT_MOUNTAIN:
				spellnum = 4;
				break;
			case SECT_MOUNTAIN_SNOW:
				spellnum = 1;
				break;
			default:
				spellnum = 2;
			}

			int restore;
			for (mana = 0, restore = FALSE; mana < NUM_OF_DIRS; mana++)
			{
				if ((track->time_income[mana] <<= spellnum))
				{
					restore = TRUE;
				}
				if ((track->time_outgone[mana] <<= spellnum))
				{
					restore = TRUE;
				}
			}
			if (!restore)
			{
				if (temp)
				{
					temp->next = next_track;
				}
				else
				{
					world[count]->track = next_track;
				}
				free(track);
			}
			else
			{
				temp = track;
			}
		}

		check_death_ice(count, NULL);
	}
}

void exchange_point_update()
{
	EXCHANGE_ITEM_DATA *exch_item, *next_exch_item;
	for (exch_item = exchange_item_list; exch_item; exch_item = next_exch_item)
	{
		next_exch_item = exch_item->next;
		if (GET_EXCHANGE_ITEM(exch_item)->get_timer() > 0)
		{
			GET_EXCHANGE_ITEM(exch_item)->dec_timer(1, true, true);
		}

		if (GET_EXCHANGE_ITEM(exch_item)->get_timer() <= 0)
		{
			std::string cap = GET_EXCHANGE_ITEM(exch_item)->get_PName(0);
			cap[0] = UPPER(cap[0]);
			sprintf(buf, "Exchange: - %s рассыпал%s от длительного использования.\r\n",
				cap.c_str(), GET_OBJ_SUF_2(GET_EXCHANGE_ITEM(exch_item)));
			log("%s", buf);
			extract_exchange_item(exch_item);
		}
	}

}

// * Оповещение о дикее шмотки из храна в клан-канал.
void clan_chest_invoice(OBJ_DATA *j)
{
	const int room = GET_ROOM_VNUM(j->get_in_obj()->get_in_room());

	if (room <= 0)
	{
		snprintf(buf, sizeof(buf), "clan_chest_invoice: room=%d, obj_vnum=%d",
			room, GET_OBJ_VNUM(j));
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
	{
		if (d->character
			&& STATE(d) == CON_PLAYING
			&& !AFF_FLAGGED(d->character, EAffectFlag::AFF_DEAFNESS)
			&& PRF_FLAGGED(d->character, PRF_DECAY_MODE)
			&& CLAN(d->character)
			&& CLAN(d->character)->GetRent() == room)
		{
			send_to_char(d->character.get(), "[Хранилище]: %s'%s%s рассыпал%s в прах'%s\r\n",
				CCIRED(d->character, C_NRM),
				j->get_short_description().c_str(),
				clan_get_custom_label(j, CLAN(d->character)).c_str(),
				GET_OBJ_SUF_2(j),
				CCNRM(d->character, C_NRM));
		}
	}

	for (const auto& i : Clan::ClanList)
	{
		if (i->GetRent() == room)
		{
			std::string log_text = boost::str(boost::format("%s%s рассыпал%s в прах\r\n")
				% j->get_short_description()
				% clan_get_custom_label(j, i)
				% GET_OBJ_SUF_2(j));
			i->chest_log.add(log_text);
			return;
		}
	}
}

// * Дикей шмоток в клан-хране.
void clan_chest_point_update(OBJ_DATA *j)
{
	// если это шмотка из большого набора предметов и она такая одна в хране, то таймер тикает в 2 раза быстрее
	if (SetSystem::is_big_set(j, true))
	{
		SetSystem::init_vnum_list(GET_OBJ_VNUM(j));
		if (!SetSystem::find_set_item(j->get_in_obj()) && j->get_timer() > 0)
			j->dec_timer();
	}

	if (j->get_timer() > 0)
	{
		j->dec_timer();
	}

	if (j->get_timer() <= 0
		|| (j->get_extra_flag(EExtraFlag::ITEM_ZONEDECAY)
			&& GET_OBJ_ZONE(j) != NOWHERE
			&& up_obj_where(j->get_in_obj()) != NOWHERE
			&& GET_OBJ_ZONE(j) != world[up_obj_where(j->get_in_obj())]->zone))
	{
		clan_chest_invoice(j);
		obj_from_obj(j);
		extract_obj(j);
	}
}

// симуляция телла чармиса при дикее стафа по таймеру
/* параметр where определяет положение вещи:
   0: руки или инвентарь
   1: экипа
   2: контейнер
*/
void charmee_obj_decay_tell(CHAR_DATA *charmee, OBJ_DATA *obj, int where)
{
	char buf[MAX_STRING_LENGTH];
	char buf1[128]; // ну не лепить же сюда malloc

	if (!charmee->has_master())
	{
		return;
	}

	if (where == 0)
	{
		sprintf(buf1, "в ваших руках");
	}
	else if (where == 1)
	{
		sprintf(buf1, "прямо на вас");
	}
	else if (where == 2 && obj->get_in_obj())
	{
		snprintf(buf1, 128, "в %s", obj->get_in_obj()->get_PName(5).c_str());
	}
	else
	{
		sprintf(buf1, "непонятно где"); // для дебага -- сюда выполнение доходить не должно
	}

	/*
	   реализация телла не самая красивая, но ничего более подходящего придумать не удается:
	   через do_tell чармисы телять не могут, а если это будет выглядеть не как телл,
	   то в игре будет выглядеть неестественно.
	   короче, рефакторинг приветствуется, если кто-нибудь придумает лучше.
	*/
	std::string cap = obj->get_PName(0);
	cap[0] = UPPER(cap[0]);
	snprintf(buf, MAX_STRING_LENGTH, "%s сказал%s вам : '%s рассыпал%s %s...'",
		GET_NAME(charmee),
		GET_CH_SUF_1(charmee),
		cap.c_str(),
		GET_OBJ_SUF_2(obj),
		buf1);
	send_to_char(charmee->get_master(), "%s%s%s\r\n", CCICYN(charmee->get_master(), C_NRM), CAP(buf), CCNRM(charmee->get_master(), C_NRM));
}

void obj_point_update()
{
	int count;

	world_objects.foreach_on_copy([&](const OBJ_DATA::shared_ptr& j)
	{
		// смотрим клан-сундуки
		if (j->get_in_obj() && Clan::is_clan_chest(j->get_in_obj()))
		{
			clan_chest_point_update(j.get());
			return;
		}

		// контейнеры на земле с флагом !дикей, но не загружаемые в этой комнате, а хз кем брошенные
		// извращение конечно перебирать на каждый объект команды резета зоны, но в голову ниче интересного
		// не лезет, да и не так уж и много на самом деле таких предметов будет, условий порядочно
		// а так привет любителям оставлять книги в клановых сумках или лоадить в замке столы
		if (j->get_in_obj()
			&& !j->get_in_obj()->get_carried_by()
			&& !j->get_in_obj()->get_worn_by()
			&& j->get_in_obj()->get_extra_flag(EExtraFlag::ITEM_NODECAY)
			&& GET_ROOM_VNUM(j->get_in_obj()->get_in_room()) % 100 != 99)
		{
			int zone = world[j->get_in_obj()->get_in_room()]->zone;
			bool find = 0;
			const auto clan = Clan::GetClanByRoom(j->get_in_obj()->get_in_room());
			if (!clan)   // внутри замков даже и смотреть не будем
			{
				for (int cmd_no = 0; zone_table[zone].cmd[cmd_no].command != 'S'; ++cmd_no)
				{
					if (zone_table[zone].cmd[cmd_no].command == 'O'
						&& zone_table[zone].cmd[cmd_no].arg1 == GET_OBJ_RNUM(j->get_in_obj())
						&& zone_table[zone].cmd[cmd_no].arg3 == j->get_in_obj()->get_in_room())
					{
						find = 1;
						break;
					}
				}
			}

			if (!find && j->get_timer() > 0)
			{
				j->dec_timer();
			}
		}

		// If this is a corpse
		if (IS_CORPSE(j))  	// timer count down
		{
			if (j->get_timer() > 0)
			{
				j->dec_timer();
			}

			if (j->get_timer() <= 0)
			{
				OBJ_DATA *jj, *next_thing2;
				for (jj = j->get_contains(); jj; jj = next_thing2)
				{
					next_thing2 = jj->get_next_content();	// Next in inventory
					obj_from_obj(jj);
					if (j->get_in_obj())
					{
						obj_to_obj(jj, j->get_in_obj());
					}
					else if (j->get_carried_by())
					{
						obj_to_char(jj, j->get_carried_by());
					}
					else if (j->get_in_room() != NOWHERE)
					{
						obj_to_room(jj, j->get_in_room());
					}
					else
					{
						log("SYSERR: extract %s from %s to NOTHING !!!",
							jj->get_PName(0).c_str(), j->get_PName(0).c_str());
						// core_dump();
						extract_obj(jj);
					}
				}

				// Конец Ладник
				if (j->get_carried_by())
				{
					act("$p рассыпал$U в ваших руках.", FALSE, j->get_carried_by(), j.get(), 0, TO_CHAR);
					obj_from_char(j.get());
				}
				else if (j->get_in_room() != NOWHERE)
				{
					if (!world[j->get_in_room()]->people.empty())
					{
						act("Черви полностью сожрали $o3.", TRUE, world[j->get_in_room()]->first_character(), j.get(), 0, TO_ROOM);
						act("Черви не оставили от $o1 и следа.", TRUE, world[j->get_in_room()]->first_character(), j.get(), 0, TO_CHAR);
					}

					obj_from_room(j.get());
				}
				else if (j->get_in_obj())
				{
					obj_from_obj(j.get());
				}
				extract_obj(j.get());
			}
		}
		else
		{
			if (SCRIPT_CHECK(j.get(), OTRIG_TIMER))
			{
				if (j->get_timer() > 0 && j->get_extra_flag(EExtraFlag::ITEM_TICKTIMER))
				{
					j->dec_timer();
				}

				if (!j->get_timer())
				{
					timer_otrigger(j.get());
					return;
				}
			}
			else if (GET_OBJ_DESTROY(j) > 0
				&& !NO_DESTROY(j.get()))
			{
				j->dec_destroyer();
			}

			if (j
				&& (j->get_in_room() != NOWHERE)
				&& j->get_timer() > 0
				&& !NO_DESTROY(j.get()))
			{
				j->dec_timer();
			}

			if (j
				&& ((j->get_extra_flag(EExtraFlag::ITEM_ZONEDECAY)
					&& GET_OBJ_ZONE(j) != NOWHERE
					&& up_obj_where(j.get()) != NOWHERE
					&& GET_OBJ_ZONE(j) != world[up_obj_where(j.get())]->zone)
					|| (j->get_timer() <= 0
						&& !NO_TIMER(j.get()))
					|| (GET_OBJ_DESTROY(j) == 0
						&& !NO_DESTROY(j.get()))))
			{
				// *** рассыпание объекта
				OBJ_DATA *jj, *next_thing2;
				for (jj = j->get_contains(); jj; jj = next_thing2)
				{
					next_thing2 = jj->get_next_content();
					obj_from_obj(jj);
					if (j->get_in_obj())
					{
						obj_to_obj(jj, j->get_in_obj());
					}
					else if (j->get_worn_by())
					{
						obj_to_char(jj, j->get_worn_by());
					}
					else if (j->get_carried_by())
					{
						obj_to_char(jj, j->get_carried_by());
					}
					else if (j->get_in_room() != NOWHERE)
					{
						obj_to_room(jj, j->get_in_room());
					}
					else
					{
						log("SYSERR: extract %s from %s to NOTHING !!!",
							jj->get_PName(0).c_str(),
							j->get_PName(0).c_str());
						// core_dump();
						extract_obj(jj);
					}
				}

				// Конец Ладник
				if (j->get_worn_by())
				{
					switch (j->get_worn_on())
					{
					case WEAR_LIGHT:
					case WEAR_SHIELD:
					case WEAR_WIELD:
					case WEAR_HOLD:
					case WEAR_BOTHS:
						if (IS_CHARMICE(j->get_worn_by()))
						{
							charmee_obj_decay_tell(j->get_worn_by(), j.get(), 0);
						}
						else
						{
							act("$o рассыпал$U в ваших руках...", FALSE, j->get_worn_by(), j.get(), 0, TO_CHAR);
						}
						break;

					default:
						if (IS_CHARMICE(j->get_worn_by()))
						{
							charmee_obj_decay_tell(j->get_worn_by(), j.get(), 1);
						}
						else
						{
							act("$o рассыпал$U прямо на вас...", FALSE, j->get_worn_by(), j.get(), 0, TO_CHAR);
						}
						break;
					}
					unequip_char(j->get_worn_by(), j->get_worn_on());
				}
				else if (j->get_carried_by())
				{
					if (IS_CHARMICE(j->get_carried_by()))
					{
						charmee_obj_decay_tell(j->get_carried_by(), j.get(), 0);
					}
					else
					{
						act("$o рассыпал$U в ваших руках...", FALSE, j->get_carried_by(), j.get(), 0, TO_CHAR);
					}
					obj_from_char(j.get());
				}
				else if (j->get_in_room() != NOWHERE)
				{
					if (!world[j->get_in_room()]->people.empty())
					{
						act("$o рассыпал$U в прах, который был развеян ветром...",
							FALSE, world[j->get_in_room()]->first_character(), j.get(), 0, TO_CHAR);
						act("$o рассыпал$U в прах, который был развеян ветром...",
							FALSE, world[j->get_in_room()]->first_character(), j.get(), 0, TO_ROOM);
					}

					obj_from_room(j.get());
				}
				else if (j->get_in_obj())
				{
					// если сыпется в находящемся у чара или чармиса контейнере, то об этом тоже сообщаем
					CHAR_DATA *cont_owner = NULL;
					if (j->get_in_obj()->get_carried_by())
					{
						cont_owner = j->get_in_obj()->get_carried_by();
					}
					else if (j->get_in_obj()->get_worn_by())
					{
						cont_owner = j->get_in_obj()->get_worn_by();
					}

					if (cont_owner)
					{
						if (IS_CHARMICE(cont_owner))
						{
							charmee_obj_decay_tell(cont_owner, j.get(), 2);
						}
						else
						{
							char buf[MAX_STRING_LENGTH];
							snprintf(buf, MAX_STRING_LENGTH, "$o рассыпал$U в %s...", j->get_in_obj()->get_PName(5).c_str());
							act(buf, FALSE, cont_owner, j.get(), 0, TO_CHAR);
						}
					}
					obj_from_obj(j.get());
				}
				extract_obj(j.get());
			}
			else
			{
				if (!j)
				{
					return;
				}

				// decay poison && other affects
				for (count = 0; count < MAX_OBJ_AFFECT; count++)
				{
					if (j->get_affected(count).location == APPLY_POISON)
					{
						j->dec_affected_value(count);
						if (j->get_affected(count).modifier <= 0)
						{
							j->set_affected(count, APPLY_NONE, 0);
						}
					}
				}
			}
		}
	});

	// Тонущие, падающие, и сыпящиеся обьекты.
	world_objects.foreach_on_copy([&](const OBJ_DATA::shared_ptr& j)
	{
		obj_decay(j.get());
	});
}

void point_update(void)
{
	memory_rec *mem, *nmem, *pmem;

	std::vector<int> real_spell(MAX_SPELLS + 1);
	for (int count = 0; count <= MAX_SPELLS; count++)
	{
		real_spell[count] = count;
	}
	std::random_shuffle(real_spell.begin(), real_spell.end());

	for (const auto& character : character_list)
	{
		const auto i = character.get();

		if (IS_NPC(i))
		{
			i->inc_restore_timer(SECS_PER_MUD_HOUR);
		}

		/* Если чар или моб попытался проснуться а на нем аффект сон,
		то он снова должен валиться в сон */
		if (AFF_FLAGGED(i, EAffectFlag::AFF_SLEEP)
			&& GET_POS(i) > POS_SLEEPING)
		{
			GET_POS(i) = POS_SLEEPING;
			send_to_char("Вы попытались очнуться, но снова заснули и упали наземь.\r\n", i);
			act("$n попытал$u очнуться, но снова заснул$a и упал$a наземь.", TRUE, i, 0, 0, TO_ROOM);
		}

		if (!IS_NPC(i))
		{
			gain_condition(i, DRUNK, -1);

			if (average_day_temp() < -20)
			{
				gain_condition(i, FULL, +2);
			}
			else if (average_day_temp() < -5)
			{
				gain_condition(i, FULL, number(+2, +1));
			}
			else
			{
				gain_condition(i, FULL, +1);
			}

			if (average_day_temp() > 25)
			{
				gain_condition(i, THIRST, +2);
			}
			else if (average_day_temp() > 20)
			{
				gain_condition(i, THIRST, number(+2, +1));
			}
			else
			{
				gain_condition(i, THIRST, +1);
			}
		}
	}

	character_list.foreach_on_copy([&](const auto& character)
	{
		const auto i = character.get();

		if (GET_POS(i) >= POS_STUNNED)  	// Restore hit points
		{
			if (IS_NPC(i)
				|| !UPDATE_PC_ON_BEAT)
			{
				const int count = hit_gain(i);
				if (GET_HIT(i) < GET_REAL_MAX_HIT(i))
				{
					GET_HIT(i) = MIN(GET_HIT(i) + count, GET_REAL_MAX_HIT(i));
				}
			}

			// Restore mobs
			if (IS_NPC(i)
				&& !i->get_fighting())  	// Restore horse
			{
				if (IS_HORSE(i))
				{
					int mana = 0;

					switch (real_sector(IN_ROOM(i)))
					{
					case SECT_FLYING:
					case SECT_UNDERWATER:
					case SECT_SECRET:
					case SECT_WATER_SWIM:
					case SECT_WATER_NOSWIM:
					case SECT_THICK_ICE:
					case SECT_NORMAL_ICE:
					case SECT_THIN_ICE:
						mana = 0;
						break;
					case SECT_CITY:
						mana = 20;
						break;
					case SECT_FIELD:
					case SECT_FIELD_RAIN:
						mana = 100;
						break;
					case SECT_FIELD_SNOW:
						mana = 40;
						break;
					case SECT_FOREST:
					case SECT_FOREST_RAIN:
						mana = 80;
						break;
					case SECT_FOREST_SNOW:
						mana = 30;
						break;
					case SECT_HILLS:
					case SECT_HILLS_RAIN:
						mana = 70;
						break;
					case SECT_HILLS_SNOW:
						mana = 25;
						break;
					case SECT_MOUNTAIN:
						mana = 25;
						break;
					case SECT_MOUNTAIN_SNOW:
						mana = 10;
						break;
					default:
						mana = 10;
					}

					if (on_horse(i->get_master()))
					{
						mana /= 2;
					}

					GET_HORSESTATE(i) = MIN(800, GET_HORSESTATE(i) + mana);
				}

				// Forget PC's
				for (mem = MEMORY(i), pmem = NULL; mem; mem = nmem)
				{
					nmem = mem->next;
					if (mem->time <= 0
						&& i->get_fighting())
					{
						pmem = mem;
						continue;
					}

					if (mem->time < time(NULL)
						|| mem->time <= 0)
					{
						if (pmem)
						{
							pmem->next = nmem;
						}
						else
						{
							MEMORY(i) = nmem;
						}
						free(mem);
					}
					else
					{
						pmem = mem;
					}
				}

				// Remember some spells
				const auto mob_num = GET_MOB_RNUM(i);
				if (mob_num >= 0)
				{
					int mana = 0;
					int count = 0;
					const auto max_mana = GET_REAL_INT(i) * 10;
					while (count < MAX_SPELLS && mana < max_mana)
					{
						const auto spellnum = real_spell[count];
						if (GET_SPELL_MEM(mob_proto + mob_num, spellnum) > GET_SPELL_MEM(i, spellnum))
						{
							GET_SPELL_MEM(i, spellnum)++;
							mana += ((spell_info[spellnum].mana_max + spell_info[spellnum].mana_min) / 2);
							GET_CASTER(i) += (IS_SET(spell_info[spellnum].routines, NPC_CALCULATE) ? 1 : 0);
						}

						++count;
					}
				}
			}

			// Restore moves
			if (IS_NPC(i)
				|| !UPDATE_PC_ON_BEAT)
			{
				//MZ.overflow_fix
				if (GET_MOVE(i) < GET_REAL_MAX_MOVE(i))
				{
					GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_REAL_MAX_MOVE(i));
				}
				//-MZ.overflow_fix
			}

			// Update PC/NPC position
			if (GET_POS(i) <= POS_STUNNED)
			{
				update_pos(i);
			}
		}
		else if (GET_POS(i) == POS_INCAP)
		{
			Damage dmg(SimpleDmg(TYPE_SUFFERING), 1, FightSystem::UNDEF_DMG);
			dmg.flags.set(FightSystem::NO_FLEE_DMG);

			if (dmg.process(i, i) == -1)
			{
				return;
			}
		}
		else if (GET_POS(i) == POS_MORTALLYW)
		{
			Damage dmg(SimpleDmg(TYPE_SUFFERING), 2, FightSystem::UNDEF_DMG);
			dmg.flags.set(FightSystem::NO_FLEE_DMG);

			if (dmg.process(i, i) == -1)
			{
				return;
			}
		}

		update_char_objects(i);

		if (!IS_NPC(i)
			&& GET_LEVEL(i) < idle_max_level
			&& !PRF_FLAGGED(i, PRF_CODERINFO))
		{
			check_idling(i);
		}
	});
}

void repop_decay(zone_rnum zone)
{
	const zone_vnum zone_num = zone_table[zone].number;

	world_objects.foreach_on_copy([&](const OBJ_DATA::shared_ptr& j)
	{
		const zone_vnum obj_zone_num = j->get_vnum() / 100;

		if (obj_zone_num == zone_num
			&& j->get_extra_flag(EExtraFlag::ITEM_REPOP_DECAY))
		{
			if (j->get_worn_by())
			{
				act("$o рассыпал$U, вспыхнув ярким светом...", FALSE, j->get_worn_by(), j.get(), 0, TO_CHAR);
			}
			else if (j->get_carried_by())
			{
				act("$o рассыпал$U в ваших руках, вспыхнув ярким светом...",
					FALSE, j->get_carried_by(), j.get(), 0, TO_CHAR);
			}
			else if (j->get_in_room() != NOWHERE)
			{
				if (!world[j->get_in_room()]->people.empty())
				{
					act("$o рассыпал$U, вспыхнув ярким светом...",
						FALSE, world[j->get_in_room()]->first_character(), j.get(), 0, TO_CHAR);
					act("$o рассыпал$U, вспыхнув ярким светом...",
						FALSE, world[j->get_in_room()]->first_character(), j.get(), 0, TO_ROOM);
				}
			}
			else if(j->get_in_obj())
			{
				CHAR_DATA* owner = nullptr;

				if (j->get_in_obj()->get_carried_by())
				{
					owner = j->get_in_obj()->get_carried_by();
				}
				else if (j->get_in_obj()->get_worn_by())
				{
					owner = j->get_in_obj()->get_worn_by();
				}

				if (owner)
				{
					char buf[MAX_STRING_LENGTH];
					snprintf(buf, MAX_STRING_LENGTH, "$o рассыпал$U в %s...", j->get_in_obj()->get_PName(5).c_str());
					act(buf, FALSE, owner, j.get(), 0, TO_CHAR);
				}
			}

			extract_obj(j.get());
		}
	});
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
