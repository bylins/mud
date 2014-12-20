/* ************************************************************************
*   File: act.other.cpp                                 Part of Bylins    *
*  Usage: Miscellaneous player-level commands                             *
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

#define __ACT_OTHER_C__

#include "conf.h"
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <iterator>
#include <sys/stat.h>

#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "skills.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "pk.h"
#include "fight.h"
#include "magic.h"
#include "features.hpp"
#include "depot.hpp"
#include "privilege.hpp"
#include "random.hpp"
#include "char.hpp"
#include "char_player.hpp"
#include "remember.hpp"
#include "room.hpp"
#include "objsave.h"
#include "shop_ext.hpp"
#include "noob.hpp"

using std::ifstream;
using std::fstream;
using std::map;
using std::iterator;

extern class insert_wanted_gem iwg;

// extern variables
extern DESCRIPTOR_DATA *descriptor_list;
extern INDEX_DATA *mob_index;
extern char const *class_abbrevs[];
extern int free_rent;
extern int max_filesize;
extern int nameserver_is_slow;
extern int auto_save;
extern struct skillvariables_dig dig_vars;
extern struct skillvariables_insgem insgem_vars;

// extern procedures
void list_feats(CHAR_DATA * ch, CHAR_DATA * vict, bool all_feats);
void list_skills(CHAR_DATA * ch, CHAR_DATA * vict);
void list_spells(CHAR_DATA * ch, CHAR_DATA * vict, int all_spells);
void appear(CHAR_DATA * ch);
void write_aliases(CHAR_DATA * ch);
void perform_immort_vis(CHAR_DATA * ch);
int have_mind(CHAR_DATA * ch);
ACMD(do_gen_comm);
int Crash_delete_file(char *name, int mask);
int HaveMind(CHAR_DATA * ch);
extern char *color_value(CHAR_DATA * ch, int real, int max);
int posi_value(int real, int max);
int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern void split_or_clan_tax(CHAR_DATA *ch, long amount);
extern bool is_wear_light(CHAR_DATA *ch);
extern bool check_unlimited_timer(OBJ_DATA *obj);
// local functions
ACMD(do_antigods);
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_sneak);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_spells);
ACMD(do_features);
ACMD(do_skills);
ACMD(do_visible);
int perform_group(CHAR_DATA * ch, CHAR_DATA * vict);
void print_group(CHAR_DATA * ch);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_wimpy);
ACMD(do_display);
ACMD(do_gen_tog);
ACMD(do_courage);
ACMD(do_toggle);
ACMD(do_color);
ACMD(do_recall);
ACMD(do_dig);
ACMD(do_summon);
bool is_dark(room_rnum room);

ACMD(do_antigods)
{
	if (IS_IMMORTAL(ch))
	{
		send_to_char("Оно вам надо?\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, AFF_SHIELD))
	{
		if (affected_by_spell(ch, SPELL_SHIELD))
			affect_from_char(ch, SPELL_SHIELD);
		REMOVE_BIT(AFF_FLAGS(ch, AFF_SHIELD), AFF_SHIELD);
		send_to_char("Голубой кокон вокруг вашего тела угас.\r\n", ch);
		act("&W$n отринул$g защиту, дарованную богами.&n", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	}
	else
		send_to_char("Вы и так не под защитой богов.\r\n", ch);
}

ACMD(do_quit)
{
	DESCRIPTOR_DATA *d, *next_d;

	if (IS_NPC(ch) || !ch->desc)
		return;

	if (subcmd != SCMD_QUIT)
		send_to_char("Вам стоит набрать эту команду полностью во избежание недоразумений!\r\n", ch);
	else if (GET_POS(ch) == POS_FIGHTING)
		send_to_char("Угу! Щаз-з-з! Вы, батенька, деретесь!\r\n", ch);
	else if (GET_POS(ch) < POS_STUNNED)
	{
		send_to_char("Вас пригласила к себе владелица косы...\r\n", ch);
		die(ch, NULL);
	}
	else if (AFF_FLAGGED(ch, AFF_SLEEP))
	{
		return;
	}
	else if (*argument)
	{
		send_to_char("Если вы хотите выйти из игры с потерей всех вещей, то просто наберите 'конец'.\r\n", ch);
	}
	else
	{
//		int loadroom = ch->in_room;
		if (RENTABLE(ch))
		{
			send_to_char("В связи с боевыми действиями эвакуация временно прекращена.\r\n", ch);
			return;
		}
		if (!GET_INVIS_LEV(ch))
			act("$n покинул$g игру.", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		sprintf(buf, "%s quit the game.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
		send_to_char("До свидания, странник... Мы ждем тебя снова!\r\n", ch);

		int depot_cost = Depot::get_total_cost_per_day(ch);
		if (depot_cost)
		{
			send_to_char(ch, "За вещи в хранилище придется заплатить %ld %s в день.\r\n", depot_cost, desc_count(depot_cost, WHAT_MONEYu));
			int deadline = ((ch->get_gold() + ch->get_bank()) / depot_cost);
			send_to_char(ch, "Твоих денег хватит на %ld %s.\r\n", deadline, desc_count(deadline, WHAT_DAY));
		}
		Depot::exit_char(ch);
		Clan::clan_invoice(ch, false);

		/*
		 * kill off all sockets connected to the same player as the one who is
		 * trying to quit.  Helps to maintain sanity as well as prevent duping.
		 */
		for (d = descriptor_list; d; d = next_d)
		{
			next_d = d->next;
			if (d == ch->desc)
				continue;
			if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
				STATE(d) = CON_DISCONNECT;
		}

		if (free_rent || IS_GOD(ch))
			Crash_rentsave(ch, 0);
		extract_char(ch, FALSE);

	}
}

ACMD(do_summon)
{
	// для начала проверяем, есть ли вообще лошадь у чара
	CHAR_DATA *horse = NULL;
	horse = get_horse(ch);
	if (!horse)
		send_to_char("У вас нет скакуна!\r\n", ch);
		return;
	if (IN_ROOM(ch) == IN_ROOM(horse))
		send_to_char("Но ваш какун рядом с вами!\r\n", ch);
		return;
	
	send_to_char("Ваш скакун появился перед вами.\r\n", ch);
	act("$n исчез$q в голубом пламени.", TRUE, horse, 0, 0, TO_ROOM);
	char_from_room(horse);
	char_to_room(horse, IN_ROOM(ch));
	look_at_room(horse, 0);
	act("$n появил$u из голубого пламени!", TRUE, horse, 0, 0, TO_ROOM);
}


ACMD(do_save)
{
	if (IS_NPC(ch) || !ch->desc)
		return;

	// Only tell the char we're saving if they actually typed "save"
	if (cmd)
	{		/*
				 * This prevents item duplication by two PC's using coordinated saves
				 * (or one PC with a house) and system crashes. Note that houses are
				 * still automatically saved without this enabled. This code assumes
				 * that guest immortals aren't trustworthy. If you've disabled guest
				 * immortal advances from mortality, you may want < instead of <=.
				 */
		if (auto_save && GET_LEVEL(ch) <= LVL_IMMORT)
		{
			send_to_char("Записываю синонимы.\r\n", ch);
			write_aliases(ch);
			return;
		}
		sprintf(buf, "Записываю %s и алиасы.\r\n", GET_NAME(ch));
		send_to_char(buf, ch);
	}

	write_aliases(ch);
	ch->save_char();
	Crash_crashsave(ch);
}


// generic function for commands which are normally overridden by
// special procedures - i.e., shop commands, mail commands, etc.
ACMD(do_not_here)
{
	send_to_char("Эта команда недоступна в этом месте!\r\n", ch);
}

int check_awake(CHAR_DATA * ch, int what)
{
	int i, retval = 0, wgt = 0;

	if (!IS_GOD(ch))
	{
		if (IS_SET(what, ACHECK_AFFECTS) && (AFF_FLAGGED(ch, AFF_STAIRS) || AFF_FLAGGED(ch, AFF_SANCTUARY)))
			SET_BIT(retval, ACHECK_AFFECTS);

		if (IS_SET(what, ACHECK_LIGHT) &&
				IS_DEFAULTDARK(IN_ROOM(ch)) && (AFF_FLAGGED(ch, AFF_SINGLELIGHT) || AFF_FLAGGED(ch, AFF_HOLYLIGHT)))
			SET_BIT(retval, ACHECK_LIGHT);

		for (i = 0; i < NUM_WEARS; i++)
		{
			if (!GET_EQ(ch, i))
				continue;

			if (IS_SET(what, ACHECK_HUMMING) && OBJ_FLAGGED(GET_EQ(ch, i), ITEM_HUM))
				SET_BIT(retval, ACHECK_HUMMING);

			if (IS_SET(what, ACHECK_GLOWING) && OBJ_FLAGGED(GET_EQ(ch, i), ITEM_GLOW))
				SET_BIT(retval, ACHECK_GLOWING);

			if (IS_SET(what, ACHECK_LIGHT) &&
					IS_DEFAULTDARK(IN_ROOM(ch)) &&
					GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT && GET_OBJ_VAL(GET_EQ(ch, i), 2))
				SET_BIT(retval, ACHECK_LIGHT);

			if (ObjSystem::is_armor_type(GET_EQ(ch, i)) && GET_OBJ_MATER(GET_EQ(ch, i)) <= MAT_COLOR)
				wgt += GET_OBJ_WEIGHT(GET_EQ(ch, i));
		}

		if (IS_SET(what, ACHECK_WEIGHT) && wgt > GET_REAL_STR(ch) * 2)
			SET_BIT(retval, ACHECK_WEIGHT);
	}
	return (retval);
}

int awake_hide(CHAR_DATA * ch)
{
	if (IS_GOD(ch))
		return (FALSE);
	return check_awake(ch, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING
		| ACHECK_GLOWING | ACHECK_WEIGHT);
}

int awake_sneak(CHAR_DATA * ch)
{
	return awake_hide(ch);
}

int awake_invis(CHAR_DATA * ch)
{
	if (IS_GOD(ch))
		return (FALSE);
	return check_awake(ch, ACHECK_AFFECTS | ACHECK_LIGHT | ACHECK_HUMMING
		| ACHECK_GLOWING);
}

int awake_camouflage(CHAR_DATA * ch)
{
	return awake_invis(ch);
}

int awaking(CHAR_DATA * ch, int mode)
{
	if (IS_GOD(ch))
		return (FALSE);
	if (IS_SET(mode, AW_HIDE) && awake_hide(ch))
		return (TRUE);
	if (IS_SET(mode, AW_INVIS) && awake_invis(ch))
		return (TRUE);
	if (IS_SET(mode, AW_CAMOUFLAGE) && awake_camouflage(ch))
		return (TRUE);
	if (IS_SET(mode, AW_SNEAK) && awake_sneak(ch))
		return (TRUE);
	return (FALSE);
}

int char_humming(CHAR_DATA * ch)
{
	int i;

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
		return (FALSE);

	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i) && OBJ_FLAGGED(GET_EQ(ch, i), ITEM_HUM))
			return (TRUE);
	}
	return (FALSE);
}

/*
int char_glowing(CHAR_DATA * ch)
{
	int i;

	if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM))
		return (FALSE);

	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i) && OBJ_FLAGGED(GET_EQ(ch, i), ITEM_GLOW))
			return (TRUE);
	}
	return (FALSE);
}
*/

ACMD(do_sneak)
{
	AFFECT_DATA af;
	ubyte prob, percent;

	if (IS_NPC(ch) || !ch->get_skill(SKILL_SNEAK))
	{
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (on_horse(ch))
	{
		act("Вам стоит подумать о мягкой обуви для $N1", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		return;
	}

	if (affected_by_spell(ch, SPELL_GLITTERDUST))
	{
		send_to_char("Вы бесшумно крадетесь, отбрасывая тысячи солнечных зайчиков...\r\n", ch);
		return;
	}

	affect_from_char(ch, SPELL_SNEAK);

	if (affected_by_spell(ch, SPELL_SNEAK))
	{
		send_to_char("Вы уже пытаетесь красться.\r\n", ch);
		return;
	}

	send_to_char("Хорошо, вы попытаетесь двигаться бесшумно.\r\n", ch);
	REMOVE_BIT(EXTRA_FLAGS(ch, EXTRA_FAILSNEAK), EXTRA_FAILSNEAK);
	percent = number(1, skill_info[SKILL_SNEAK].max_percent);
	prob = calculate_skill(ch, SKILL_SNEAK, skill_info[SKILL_SNEAK].max_percent, 0);

	af.type = SPELL_SNEAK;
	af.duration = pc_duration(ch, 0, GET_LEVEL(ch), 8, 0, 1);
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.battleflag = 0;
	if (percent > prob)
		af.bitvector = 0;
	else
		af.bitvector = AFF_SNEAK;
	affect_to_char(ch, &af);
}

ACMD(do_camouflage)
{
	AFFECT_DATA af;
	struct timed_type timed;
	ubyte prob, percent;

	if (IS_NPC(ch) || !ch->get_skill(SKILL_CAMOUFLAGE))
	{
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (on_horse(ch))
	{
		send_to_char("Вы замаскировались под статую Юрия Долгорукого.\r\n", ch);
		return;
	}

	if (affected_by_spell(ch, SPELL_GLITTERDUST))
	{
		send_to_char("Вы замаскировались под золотую рыбку.\r\n", ch);
		return;
	}


	if (timed_by_skill(ch, SKILL_CAMOUFLAGE))
	{
		send_to_char("У вас пока не хватает фантазии. Побудьте немного самим собой.\r\n", ch);
		return;
	}

	if (IS_IMMORTAL(ch))
		affect_from_char(ch, SPELL_CAMOUFLAGE);

	if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
	{
		send_to_char("Вы уже маскируетесь.\r\n", ch);
		return;
	}

	send_to_char("Вы начали усиленно маскироваться.\r\n", ch);
	REMOVE_BIT(EXTRA_FLAGS(ch, EXTRA_FAILCAMOUFLAGE), EXTRA_FAILCAMOUFLAGE);
	percent = number(1, skill_info[SKILL_CAMOUFLAGE].max_percent);
	prob = calculate_skill(ch, SKILL_CAMOUFLAGE, skill_info[SKILL_CAMOUFLAGE].max_percent, 0);

	af.type = SPELL_CAMOUFLAGE;
	af.duration = pc_duration(ch, 0, GET_LEVEL(ch), 6, 0, 2);
	af.modifier = world[IN_ROOM(ch)]->zone;
	af.location = APPLY_NONE;
	af.battleflag = 0;
	if (percent > prob)
		af.bitvector = 0;
	else
		af.bitvector = AFF_CAMOUFLAGE;
	affect_to_char(ch, &af);
	if (!IS_IMMORTAL(ch))
	{
		timed.skill = SKILL_CAMOUFLAGE;
		timed.time = 2;
		timed_to_char(ch, &timed);
	}
}

ACMD(do_hide)
{
	AFFECT_DATA af;
	ubyte prob, percent;

	if (IS_NPC(ch) || !ch->get_skill(SKILL_HIDE))
	{
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (on_horse(ch))
	{
		act("А куда вы хотите спрятать $N3?", FALSE, ch, 0, get_horse(ch), TO_CHAR);
		return;
	}

	affect_from_char(ch, SPELL_HIDE);

	if (affected_by_spell(ch, SPELL_HIDE))
	{
		send_to_char("Вы уже пытаетесь спрятаться.\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, AFF_BLIND))
	{
		send_to_char("Вы слепы и не видите куда прятаться.\r\n", ch);
		return;
	}

	if (affected_by_spell(ch, SPELL_GLITTERDUST))
	{
		send_to_char("Спрятаться?! Да вы сверкаете как корчма во время гулянки!.\r\n", ch);
		return;
	}

	send_to_char("Хорошо, вы попытаетесь спрятаться.\r\n", ch);
	REMOVE_BIT(EXTRA_FLAGS(ch, EXTRA_FAILHIDE), EXTRA_FAILHIDE);
	percent = number(1, skill_info[SKILL_HIDE].max_percent);
	prob = calculate_skill(ch, SKILL_HIDE, skill_info[SKILL_HIDE].max_percent, 0);

	af.type = SPELL_HIDE;
	af.duration = pc_duration(ch, 0, GET_LEVEL(ch), 8, 0, 1);
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.battleflag = 0;
	if (percent > prob)
		af.bitvector = 0;
	else
		af.bitvector = AFF_HIDE;
	affect_to_char(ch, &af);
}

void go_steal(CHAR_DATA * ch, CHAR_DATA * vict, char *obj_name)
{
	int percent, gold, eq_pos, ohoh = 0, success = 0, prob;
	OBJ_DATA *obj;

	if (!vict)
		return;

	if (!WAITLESS(ch) && vict->get_fighting())
	{
		act("$N слишком быстро перемещается.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	if (!WAITLESS(ch) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARENA))
	{
		send_to_char("Воровство при поединке недопустимо.\r\n", ch);
		return;
	}

	// 101% is a complete failure
	percent = number(1, skill_info[SKILL_STEAL].max_percent);

	if (WAITLESS(ch) || (GET_POS(vict) <= POS_SLEEPING && !AFF_FLAGGED(vict, AFF_SLEEP)))
		success = 1;	// ALWAYS SUCCESS, unless heavy object.

	if (!AWAKE(vict))	// Easier to steal from sleeping people.
		percent = MAX(percent - 50, 0);

	// NO NO With Imp's and Shopkeepers, and if player thieving is not allowed
	if ((IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, GF_GODSLIKE) || GET_MOB_SPEC(vict) == shop_ext)
		&& !IS_IMPL(ch))
	{
		send_to_char("Вы постеснялись красть у такого хорошего человека.\r\n", ch);
		return;
	}

	if (str_cmp(obj_name, "coins") &&
			str_cmp(obj_name, "gold") && str_cmp(obj_name, "кун") && str_cmp(obj_name, "деньги"))
	{
		if (!(obj = get_obj_in_list_vis(ch, obj_name, vict->carrying)))
		{
			for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
				if (GET_EQ(vict, eq_pos) &&
						(isname(obj_name, GET_EQ(vict, eq_pos)->aliases)) &&
						CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos)))
				{
					obj = GET_EQ(vict, eq_pos);
					break;
				}
			if (!obj)
			{
				act("А у н$S этого и нет - ха-ха-ха (2 раза)...", FALSE, ch, 0, vict, TO_CHAR);
				return;
			}
			else  	// It is equipment
			{
				if (!success)
				{
					send_to_char("Украсть? Из экипировки? Щаз-з-з!\r\n", ch);
					return;
				}
				else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
				{
					send_to_char("Вы не сможете унести столько предметов.\r\n", ch);
					return;
				}
				else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
				{
					send_to_char("Вы не сможете унести такой вес.\r\n", ch);
					return;
				}
				else if (IS_OBJ_STAT(obj, ITEM_BLOODY))
				{
					send_to_char("\"Мокрухой пахнет!\" - пронеслось у вас в голове, и вы вовремя успели отдернуть руку, не испачкавшись в крови.\r\n", ch);
					return;
				}
				else
				{
					act("Вы раздели $N3 и взяли $o3.", FALSE, ch, obj, vict, TO_CHAR);
					act("$n украл$g $o3 у $N1.", FALSE, ch, obj, vict, TO_NOTVICT | TO_ARENA_LISTEN);
					obj_to_char(unequip_char(vict, eq_pos), ch);
				}
			}
		}
		else  	// obj found in inventory
		{
			if (IS_OBJ_STAT(obj, ITEM_BLOODY))
			{
				send_to_char("\"Мокрухой пахнет!\" - пронеслось у вас в голове, и вы вовремя успели отдернуть руку, не испачкавшись в крови.\r\n", ch);
				return;
			}
			percent += GET_OBJ_WEIGHT(obj);	// Make heavy harder
			prob = calculate_skill(ch, SKILL_STEAL, percent, vict);

			if (AFF_FLAGGED(ch, AFF_HIDE))
				prob += 5;	// Add by Alez - Improove in hide steal probability
			if (!WAITLESS(ch) && AFF_FLAGGED(vict, AFF_SLEEP))
				prob = 0;
			if (percent > prob && !success)
			{
				ohoh = TRUE;
				if (AFF_FLAGGED(ch, AFF_HIDE))
				{
					affect_from_char(ch, SPELL_HIDE);
					send_to_char("Вы прекратили прятаться.\r\n", ch);
					act("$n прекратил$g прятаться.", FALSE, ch, 0, 0, TO_ROOM);
				};
				send_to_char("Атас.. Дружина на конях!\r\n", ch);
				act("$n пытал$u обокрасть вас!", FALSE, ch, 0, vict, TO_VICT);
				act("$n пытал$u украсть нечто у $N1.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
			}
			else  	// Steal the item
			{
				if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))
				{
					if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch))
					{
						obj_from_char(obj);
						obj_to_char(obj, ch);
						act("Вы украли $o3 у $N1!", FALSE, ch, obj, vict, TO_CHAR);
					}
				}
				else
				{
					send_to_char("Вы не можете столько нести.\r\n", ch);
					return;
				}
			}
			if (CAN_SEE(vict, ch) && AWAKE(vict))
				improove_skill(ch, SKILL_STEAL, 0, vict);
		}
	}
	else  		// Steal some coins
	{
		prob = calculate_skill(ch, SKILL_STEAL, percent, vict);
		if (AFF_FLAGGED(ch, AFF_HIDE))
			prob += 5;	// Add by Alez - Improove in hide steal probability
		if (!WAITLESS(ch) && AFF_FLAGGED(vict, AFF_SLEEP))
			prob = 0;
		if (percent > prob && !success)
		{
			ohoh = TRUE;
			if (AFF_FLAGGED(ch, AFF_HIDE))
			{
				affect_from_char(ch, SPELL_HIDE);
				send_to_char("Вы прекратили прятаться.\r\n", ch);
				act("$n прекратил$g прятаться.", FALSE, ch, 0, 0, TO_ROOM);
			};
			send_to_char("Вы влипли... Вас посодют... А вы не воруйте..\r\n", ch);
			act("Вы обнаружили руку $n1 в своем кармане.", FALSE, ch, 0, vict, TO_VICT);
			act("$n пытал$u спионерить деньги у $N1.", TRUE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
		}
		else  	// Steal some gold coins
		{
			if (!vict->get_gold())
			{
				act("$E богат$A, как амбарная мышь :)", FALSE, ch, 0, vict, TO_CHAR);
				return;
			}
			else
			{
				// Считаем вероятность крит-воровства (воровства всех денег)
				if ((number(1, 100) - ch->get_skill(SKILL_STEAL) -
						ch->get_dex() + vict->get_wis() + vict->get_gold() / 500) < 0)
				{
					act("Тугой кошелек $N1 перекочевал к вам.", TRUE, ch, 0, vict, TO_CHAR);
					gold = vict->get_gold();
				}
				else
					gold = (int)((vict->get_gold() * number(1, 75)) / 100);

				if (gold > 0)
				{
					if (gold > 1)
					{
						sprintf(buf, "УР-Р-Р-А! Вы таки сперли %d %s.\r\n",
								gold, desc_count(gold, WHAT_MONEYu));
						send_to_char(buf, ch);
					}
					else
					{
						send_to_char("УРА-А-А ! Вы сперли :) 1 (одну) куну :(.\r\n", ch);
					}
					ch->add_gold(gold);
					split_or_clan_tax(ch, gold);
					vict->remove_gold(gold);
				}
				else
					send_to_char("Вы ничего не сумели украсть...\r\n", ch);
			}
		}
		if (CAN_SEE(vict, ch) && AWAKE(vict))
			improove_skill(ch, SKILL_STEAL, 0, vict);
	}
	if (!WAITLESS(ch) && ohoh)
		WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
	pk_thiefs_action(ch, vict);
	if (ohoh && IS_NPC(vict) && AWAKE(vict) && CAN_SEE(vict, ch) && MAY_ATTACK(vict))
		hit(vict, ch, TYPE_UNDEFINED, 1);
}


ACMD(do_steal)
{
	CHAR_DATA *vict;
	char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];

	if (IS_NPC(ch) || !ch->get_skill(SKILL_STEAL))
	{
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (!WAITLESS(ch) && on_horse(ch))
	{
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	two_arguments(argument, obj_name, vict_name);

	if (!(vict = get_char_vis(ch, vict_name, FIND_CHAR_ROOM)))
	{
		send_to_char("Украсть у кого?\r\n", ch);
		return;
	}
	else if (vict == ch)
	{
		send_to_char("Попробуйте набрать \"бросить <n> кун\".\r\n", ch);
		return;
	}

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
	{
		send_to_char("Здесь слишком мирно. Вам не хочется нарушать сию благодать...\r\n", ch);
		return;
	}

	if (IS_NPC(vict)
			&& (MOB_FLAGGED(vict, MOB_NOFIGHT) || AFF_FLAGGED(vict, AFF_SHIELD) || MOB_FLAGGED(vict, MOB_PROTECT))
			&& !(IS_IMMORTAL(ch) || GET_GOD_FLAG(ch, GF_GODSLIKE)))
	{
		send_to_char("А ежели поймают? Посодют ведь!\r\nПодумав так, вы отказались от сего намеренья.\r\n", ch);
		return;
	}

	go_steal(ch, vict, obj_name);
}

ACMD(do_features)
{
	if (IS_NPC(ch))
		return;
	skip_spaces(&argument);
	if (is_abbrev(argument, "все") || is_abbrev(argument, "all"))
		list_feats(ch, ch, TRUE);
	else
		list_feats(ch, ch, FALSE);
}

ACMD(do_skills)
{
	if (IS_NPC(ch))
		return;
	list_skills(ch, ch);
}

ACMD(do_spells)
{
	if (IS_NPC(ch))
		return;
	skip_spaces(&argument);
	if (is_abbrev(argument, "все") || is_abbrev(argument, "all"))
		list_spells(ch, ch, TRUE);
	else
		list_spells(ch, ch, FALSE);
}


ACMD(do_visible)
{
	if (IS_IMMORTAL(ch))
	{
		perform_immort_vis(ch);
		return;
	}

	if (AFF_FLAGGED(ch, AFF_INVISIBLE)
		|| AFF_FLAGGED(ch, AFF_CAMOUFLAGE)
		|| AFF_FLAGGED(ch, AFF_HIDE)
		|| AFF_FLAGGED(ch, AFF_SNEAK))
	{
		appear(ch);
		send_to_char("Вы перестали быть невидимым.\r\n", ch);
	}
	else
		send_to_char("Вы и так видимы.\r\n", ch);
}

ACMD(do_courage)
{
	OBJ_DATA *obj;
	AFFECT_DATA af[4];
	int prob;
	struct timed_type timed;
	int i;
	if (IS_NPC(ch))		// Cannot use GET_COND() on mobs.
		return;

	if (!ch->get_skill(SKILL_COURAGE))
	{
		send_to_char("Вам это не по силам.\r\n", ch);
		return;
	}

	if (timed_by_skill(ch, SKILL_COURAGE))
	{
		send_to_char("Вы не можете слишком часто впадать в ярость.\r\n", ch);
		return;
	}

	timed.skill = SKILL_COURAGE;
	timed.time = 6;
	timed_to_char(ch, &timed);
	/******************* Remove to hit()
	percent = number(1,skill_info[SKILL_COURAGE].max_percent+GET_REAL_MAX_HIT(ch)-GET_HIT(ch));
	prob    = train_skill(ch,SKILL_COURAGE,skill_info[SKILL_COURAGE].max_percent,0);
	if (percent > prob)
	   {af[0].type      = SPELL_COURAGE;
	    af[0].duration  = pc_duration(ch,3,0,0,0,0);
	    af[0].modifier  = 0;
	    af[0].location  = APPLY_DAMROLL;
	    af[0].bitvector = AFF_NOFLEE;
	    af[0].battleflag= 0;
	    af[1].type      = SPELL_COURAGE;
	    af[1].duration  = pc_duration(ch,3,0,0,0,0);
	    af[1].modifier  = 0;
	    af[1].location  = APPLY_HITROLL;
	    af[1].bitvector = AFF_NOFLEE;
	    af[1].battleflag= 0;
	    af[2].type      = SPELL_COURAGE;
	    af[2].duration  = pc_duration(ch,3,0,0,0,0);
	    af[2].modifier  = 20;
	    af[2].location  = APPLY_AC;
	    af[2].bitvector = AFF_NOFLEE;
	    af[2].battleflag= 0;
	   }
	else
	   {af[0].type      = SPELL_COURAGE;
	    af[0].duration  = pc_duration(ch,3,0,0,0,0);
	    af[0].modifier  = MAX(1, (prob+19) / 20);
	    af[0].location  = APPLY_DAMROLL;
	    af[0].bitvector = AFF_NOFLEE;
	    af[0].battleflag= 0;
	    af[1].type      = SPELL_COURAGE;
	    af[1].duration  = pc_duration(ch,3,0,0,0,0);
	    af[1].modifier  = MAX(1, (prob+9) / 10);
	    af[1].location  = APPLY_HITROLL;
	    af[1].bitvector = AFF_NOFLEE;
	    af[1].battleflag= 0;
	    af[2].type      = SPELL_COURAGE;
	    af[2].duration  = pc_duration(ch,3,0,0,0,0);
	    af[2].modifier  = 20;
	    af[2].location  = APPLY_AC;
	    af[2].bitvector = AFF_NOFLEE;
	    af[2].battleflag= 0;
	   }
	 for (prob = 0; prob < 3; prob++)
	     affect_join(ch,&af[prob],TRUE,FALSE,TRUE,FALSE);
	 ************************************/
	prob = calculate_skill(ch, SKILL_COURAGE, skill_info[SKILL_COURAGE].max_percent, 0) / 20;
	af[0].type = SPELL_COURAGE;
	af[0].duration = pc_duration(ch, 3, 0, 0, 0, 0);
	af[0].modifier = 40;
	af[0].location = APPLY_AC;
	af[0].bitvector = AFF_NOFLEE;
	af[0].battleflag = 0;
	af[1].type = SPELL_COURAGE;
	af[1].duration = pc_duration(ch, 3, 0, 0, 0, 0);
	af[1].modifier = MAX(1, prob);
	af[1].location = APPLY_DAMROLL;
	af[1].bitvector = AFF_NOFLEE;
	af[1].battleflag = 0;
	af[2].type = SPELL_COURAGE;
	af[2].duration = pc_duration(ch, 3, 0, 0, 0, 0);
	af[2].modifier = MAX(1, prob * 7);
	af[2].location = APPLY_ABSORBE;
	af[2].bitvector = AFF_NOFLEE;
	af[2].battleflag = 0;
	af[3].type = SPELL_COURAGE;
	af[3].duration = pc_duration(ch, 3, 0, 0, 0, 0);
	af[3].modifier = 50;
	af[3].location = APPLY_HITREG;
	af[3].bitvector = AFF_NOFLEE;
	af[3].battleflag = 0;

	for (i = 0; i < 4; i++)
		affect_join(ch, &af[i], TRUE, FALSE, TRUE, FALSE);

	send_to_char("Вы пришли в ярость.\r\n", ch);
	if ((obj = GET_EQ(ch, WEAR_WIELD)) || (obj = GET_EQ(ch, WEAR_BOTHS)))
		strcpy(buf, "Глаза $n1 налились кровью и $e яростно сжал$g в руках $o3.");
	else
		strcpy(buf, "Глаза $n1 налились кровью.");
	act(buf, FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
}

int max_group_size(CHAR_DATA *ch)
{
	return MAX_GROUPED_FOLLOWERS + (int) VPOSI((ch->get_skill(SKILL_LEADERSHIP) - 80) / 5, 0, 4);
}

bool is_group_member(CHAR_DATA *ch, CHAR_DATA *vict)
{
	if (IS_NPC(vict) || !AFF_FLAGGED(vict, AFF_GROUP) || vict->master != ch)
		return false;
	else
		return true;
}

/**
* Смена лидера группы на персонажа с макс лидеркой.
* Сам лидер при этом остается в группе, если он живой.
* \param vict - новый лидер, если смена происходит по команде 'гр лидер имя',
* старый лидер соответственно группится обратно, если нулевой, то считаем, что
* произошла смерть старого лидера и новый выбирается по наибольшей лидерке.
*/
void change_leader(CHAR_DATA *ch, CHAR_DATA *vict)
{
	if (IS_NPC(ch) || ch->master || !ch->followers)
		return;

	CHAR_DATA *leader = vict;
	if (!leader)
	{
		// лидер умер, ищем согрупника с максимальным скиллом лидерки
		for (struct follow_type *l = ch->followers; l; l = l->next)
		{
			if (!is_group_member(ch, l->follower))
				continue;
			if (!leader)
				leader = l->follower;
			else if (l->follower->get_skill(SKILL_LEADERSHIP) > leader->get_skill(SKILL_LEADERSHIP))
				leader = l->follower;
		}
	}
	if (!leader) return;

	// для реследования используем стандартные функции
	std::vector<CHAR_DATA *> temp_list;
	for (struct follow_type *n = 0, *l = ch->followers; l; l = n)
	{
		n = l->next;
		if (!is_group_member(ch, l->follower))
			continue;
		else
		{
			CHAR_DATA *temp_vict = l->follower;
			if (temp_vict->master && stop_follower(temp_vict, SF_SILENCE))
				continue;
			if (temp_vict != leader)
				temp_list.push_back(temp_vict);
		}
	}

	// вся эта фигня только для того, чтобы при реследовании пройтись по списку в обратном
	// направлении и сохранить относительный порядок следования в группе
	if (!temp_list.empty())
		for (std::vector<CHAR_DATA *>::reverse_iterator it = temp_list.rbegin(); it != temp_list.rend(); ++it)
			add_follower(*it, leader, true);

	// бывшего лидера последним закидываем обратно в группу, если он живой
	if (vict)
	{
		// флаг группы надо снять, иначе при регрупе не будет писаться о старом лидере
		REMOVE_BIT(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP);
		add_follower(ch, leader, true);
	}

	if (!leader->followers)
	{
		return;
	}

	ch->dps_copy(leader);
	perform_group(leader, leader);
	int followers = 0;
	for (struct follow_type *f = leader->followers; f; f = f->next)
	{
		if (followers < max_group_size(leader))
		{
			if (perform_group(leader, f->follower))
				++followers;
		}
		else
		{
			send_to_char("Вы больше никого не можете принять в группу.\r\n", ch);
			return;
		}
	}
}

int perform_group(CHAR_DATA * ch, CHAR_DATA * vict)
{
	if (AFF_FLAGGED(vict, AFF_GROUP) ||
			AFF_FLAGGED(vict, AFF_CHARM) || MOB_FLAGGED(vict, MOB_ANGEL) || IS_HORSE(vict))
		return (FALSE);

	SET_BIT(AFF_FLAGS(vict, AFF_GROUP), AFF_GROUP);
	if (ch != vict)
	{
		act("$N принят$A в члены вашего кружка (тьфу-ты, группы :).", FALSE, ch, 0, vict, TO_CHAR);
		act("Вы приняты в группу $n1.", FALSE, ch, 0, vict, TO_VICT);
		act("$N принят$A в группу $n1.", FALSE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
	}
	return (TRUE);
}

int low_charm(CHAR_DATA * ch)
{
	AFFECT_DATA *aff;
	for (aff = ch->affected; aff; aff = aff->next)
		if (aff->type == SPELL_CHARM && aff->duration <= 1)
			return (TRUE);
	return (FALSE);
}

void print_one_line(CHAR_DATA * ch, CHAR_DATA * k, int leader, int header)
{
	int ok, ok2, div;
	const char *WORD_STATE[] = { "При смерти",
								 "Оч.тяж.ран",
								 "Оч.тяж.ран",
								 " Тяж.ранен",
								 " Тяж.ранен",
								 "  Ранен   ",
								 "  Ранен   ",
								 "  Ранен   ",
								 "Лег.ранен ",
								 "Лег.ранен ",
								 "Слег.ранен",
								 " Невредим "
							   };
	const char *MOVE_STATE[] = { "Истощен",
								 "Истощен",
								 "О.устал",
								 " Устал ",
								 " Устал ",
								 "Л.устал",
								 "Л.устал",
								 "Хорошо ",
								 "Хорошо ",
								 "Хорошо ",
								 "Отдохн.",
								 " Полон "
							   };
	const char *POS_STATE[] = { "Умер",
								"Истекает кровью",
								"При смерти",
								"Без сознания",
								"Спит",
								"Отдыхает",
								"Сидит",
								"Сражается",
								"Стоит"
							  };

	if (IS_NPC(k))
	{
		if (!header)
//       send_to_char("Персонаж       | Здоровье |Рядом| Доп | Положение     | Лояльн.\r\n",ch);
			send_to_char("Персонаж            | Здоровье |Рядом| Аффект | Положение\r\n", ch);
		std::string name = GET_NAME(k);
		name[0] = UPPER(name[0]);
		sprintf(buf, "%s%-20s%s|", CCIBLU(ch, C_NRM),
				name.substr(0, 20).c_str(), CCNRM(ch, C_NRM));
		sprintf(buf + strlen(buf), "%s%10s%s|",
				color_value(ch, GET_HIT(k), GET_REAL_MAX_HIT(k)),
				WORD_STATE[posi_value(GET_HIT(k), GET_REAL_MAX_HIT(k)) + 1], CCNRM(ch, C_NRM));

		ok = IN_ROOM(ch) == IN_ROOM(k);
		sprintf(buf + strlen(buf), "%s%5s%s|",
				ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM), ok ? " Да  " : " Нет ", CCNRM(ch, C_NRM));

		sprintf(buf + strlen(buf), " %s%s%s%s%s%s%s%s%s%s%s%s%s |",
				CCIRED(ch, C_NRM), AFF_FLAGGED(k, AFF_SANCTUARY) ? "О" : (AFF_FLAGGED(k, AFF_PRISMATICAURA)
						? "П" : " "), CCGRN(ch,
											 C_NRM),
				AFF_FLAGGED(k, AFF_WATERBREATH) ? "Д" : " ", CCICYN(ch,
						C_NRM),
				AFF_FLAGGED(k, AFF_INVISIBLE) ? "Н" : " ", CCIYEL(ch, C_NRM), (AFF_FLAGGED(k, AFF_SINGLELIGHT)
						|| AFF_FLAGGED(k, AFF_HOLYLIGHT)
						|| (GET_EQ(k, WEAR_LIGHT)
							&&
							GET_OBJ_VAL(GET_EQ
										(k, WEAR_LIGHT),
										2))) ? "С" : " ",
				CCIBLU(ch, C_NRM), AFF_FLAGGED(k, AFF_FLY) ? "Л" : " ", CCYEL(ch, C_NRM),
				low_charm(k) ? "Т" : " ", CCNRM(ch, C_NRM));

//      sprintf(buf+strlen(buf),"%-15s| %d",POS_STATE[(int) GET_POS(k)],
//                                        on_charm_points(k));
		sprintf(buf + strlen(buf), "%-15s", POS_STATE[(int) GET_POS(k)]);

		act(buf, FALSE, ch, 0, k, TO_CHAR);
	}
	else
	{
		if (!header)
			send_to_char
			("Персонаж            | Здоровье |Энергия|Рядом|Учить| Аффект | Кто | Положение\r\n", ch);

		std::string name = GET_NAME(k);
		name[0] = UPPER(name[0]);
		sprintf(buf, "%s%-20s%s|", CCIBLU(ch, C_NRM), name.c_str(), CCNRM(ch, C_NRM));
		sprintf(buf + strlen(buf), "%s%10s%s|",
				color_value(ch, GET_HIT(k), GET_REAL_MAX_HIT(k)),
				WORD_STATE[posi_value(GET_HIT(k), GET_REAL_MAX_HIT(k)) + 1], CCNRM(ch, C_NRM));

		sprintf(buf + strlen(buf), "%s%7s%s|",
				color_value(ch, GET_MOVE(k), GET_REAL_MAX_MOVE(k)),
				MOVE_STATE[posi_value(GET_MOVE(k), GET_REAL_MAX_MOVE(k)) + 1], CCNRM(ch, C_NRM));

		ok = IN_ROOM(ch) == IN_ROOM(k);
		sprintf(buf + strlen(buf), "%s%5s%s|",
				ok ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM), ok ? " Да  " : " Нет ", CCNRM(ch, C_NRM));

		if ((!IS_MANA_CASTER(k) && !MEMQUEUE_EMPTY(k)) ||
				(IS_MANA_CASTER(k) && GET_MANA_STORED(k) < GET_MAX_MANA(k)))
		{
			div = mana_gain(k);
			if (div > 0)
			{
				if (!IS_MANA_CASTER(k))
				{
					ok2 = MAX(0, 1 + GET_MEM_TOTAL(k) - GET_MEM_COMPLETED(k));
					ok2 = ok2 * 60 / div;	// время мема в сек
				}
				else
				{
					ok2 = MAX(0, 1 + GET_MAX_MANA(k) - GET_MANA_STORED(k));
					ok2 = ok2 / div;	// время восстановления в секундах
				}
				ok = ok2 / 60;
				ok2 %= 60;
				if (ok > 99)
					sprintf(buf + strlen(buf), "&g%5d&n|", ok);
				else
					sprintf(buf + strlen(buf), "&g%2d:%02d&n|", ok, ok2);
			}
			else
			{
				sprintf(buf + strlen(buf), "&r    -&n|");
			}
		}
		else
			sprintf(buf + strlen(buf), "     |");

		sprintf(buf + strlen(buf), " %s%s%s%s%s%s%s%s%s%s%s%s%s |",
				CCIRED(ch, C_NRM), AFF_FLAGGED(k, AFF_SANCTUARY) ? "О" : (AFF_FLAGGED(k, AFF_PRISMATICAURA)
						? "П" : " "), CCGRN(ch,
											 C_NRM),
				AFF_FLAGGED(k, AFF_WATERBREATH) ? "Д" : " ", CCICYN(ch,
						C_NRM),
				AFF_FLAGGED(k, AFF_INVISIBLE) ? "Н" : " ", CCIYEL(ch, C_NRM), (AFF_FLAGGED(k, AFF_SINGLELIGHT)
						|| AFF_FLAGGED(k, AFF_HOLYLIGHT)
						|| (GET_EQ(k, WEAR_LIGHT)
							&&
							GET_OBJ_VAL(GET_EQ
										(k, WEAR_LIGHT),
										2))) ? "С" : " ",
				CCIBLU(ch, C_NRM), AFF_FLAGGED(k, AFF_FLY) ? "Л" : " ", CCYEL(ch, C_NRM),
				on_horse(k) ? "В" : " ", CCNRM(ch, C_NRM));

		sprintf(buf + strlen(buf), "%5s|", leader ? "Лидер" : "");
		sprintf(buf + strlen(buf), "%s", POS_STATE[(int) GET_POS(k)]);
		act(buf, FALSE, ch, 0, k, TO_CHAR);
	}
}


void print_group(CHAR_DATA * ch)
{
	int gfound = 0, cfound = 0;
	CHAR_DATA *k;
	struct follow_type *f, *g;

	k = (ch->master ? ch->master : ch);
	if (AFF_FLAGGED(ch, AFF_GROUP))
	{
		send_to_char("Ваша группа состоит из:\r\n", ch);
		if (AFF_FLAGGED(k, AFF_GROUP))
			print_one_line(ch, k, TRUE, gfound++);
		for (f = k->followers; f; f = f->next)
		{
			if (!AFF_FLAGGED(f->follower, AFF_GROUP))
				continue;
			print_one_line(ch, f->follower, FALSE, gfound++);
		}
	}
	for (f = ch->followers; f; f = f->next)
	{
		if (!(AFF_FLAGGED(f->follower, AFF_CHARM)
				|| MOB_FLAGGED(f->follower, MOB_ANGEL)))
			continue;
		if (!cfound)
			send_to_char("Ваши последователи:\r\n", ch);
		print_one_line(ch, f->follower, FALSE, cfound++);
	}
	if (!gfound && !cfound)
	{
		send_to_char("Но вы же не член (в лучшем смысле этого слова) группы!\r\n", ch);
		return;
	}
	if (PRF_FLAGGED(ch, PRF_SHOWGROUP))
		for (g = k->followers, cfound = 0; g; g = g->next)
		{
			for (f = g->follower->followers; f; f = f->next)
			{
				if (!(AFF_FLAGGED(f->follower, AFF_CHARM)
						|| MOB_FLAGGED(f->follower, MOB_ANGEL))
						|| !AFF_FLAGGED(ch, AFF_GROUP))
					continue;
				if (f->follower->master == ch || !AFF_FLAGGED(f->follower->master, AFF_GROUP))
					continue;
// shapirus: при включенном режиме не показываем клонов и хранителей
				if (PRF_FLAGGED(ch, PRF_NOCLONES) &&
						IS_NPC(f->follower) &&
						(MOB_FLAGGED(f->follower, MOB_CLONE) || GET_MOB_VNUM(f->follower) == MOB_KEEPER))
					continue;
				if (!cfound)
					send_to_char("Последователи членов вашей группы:\r\n", ch);
				print_one_line(ch, f->follower, FALSE, cfound++);
			}
			if (ch->master)
			{
				if (!(AFF_FLAGGED(g->follower, AFF_CHARM)
						|| MOB_FLAGGED(g->follower, MOB_ANGEL))
						|| !AFF_FLAGGED(ch, AFF_GROUP))
					continue;
// shapirus: при включенном режиме не показываем клонов и хранителей
				if (PRF_FLAGGED(ch, PRF_NOCLONES) &&
						IS_NPC(g->follower) &&
						(MOB_FLAGGED(g->follower, MOB_CLONE) || GET_MOB_VNUM(g->follower) == MOB_KEEPER))
					continue;
				if (!cfound)
					send_to_char("Последователи членов вашей группы:\r\n", ch);
				print_one_line(ch, g->follower, FALSE, cfound++);
			}
		}
}

ACMD(do_group)
{
	CHAR_DATA *vict;
	struct follow_type *f;
	int found, f_number;

	argument = one_argument(argument, buf);

	if (!*buf)
	{
		print_group(ch);
		return;
	}

	if (GET_POS(ch) < POS_RESTING)
	{
		send_to_char("Трудно управлять группой в таком состоянии.\r\n", ch);
		return;
	}

	if (ch->master)
	{
		act("Вы не можете управлять группой. Вы еще не ведущий.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!ch->followers)
	{
		send_to_char("За вами никто не следует.\r\n", ch);
		return;
	}
// вычисляем количество последователей
	for (f_number = 0, f = ch->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP))
			f_number++;

	if (!str_cmp(buf, "all") || !str_cmp(buf, "все"))
	{
		perform_group(ch, ch);
		for (found = 0, f = ch->followers; f; f = f->next)
		{
			if ((f_number + found) >= max_group_size(ch))
			{
				send_to_char("Вы больше никого не можете принять в группу.\r\n", ch);
				return;
			}
			found += perform_group(ch, f->follower);
		}
		if (!found)
			send_to_char("Все, кто за вами следуют, уже включены в вашу группу.\r\n", ch);
		return;
	}
	else if (!str_cmp(buf, "leader") || !str_cmp(buf, "лидер"))
	{
		vict = get_player_vis(ch, argument, FIND_CHAR_WORLD);
		// added by WorM (Видолюб) Если найден клон и его хозяин персонаж
		// а то чото как-то глючно Двойник %1 не является членом вашей группы.
		if (vict && IS_NPC(vict) && MOB_FLAGGED(vict, MOB_CLONE) && AFF_FLAGGED(vict, AFF_CHARM) && vict->master && !IS_NPC(vict->master))
		{
			if (CAN_SEE(ch, vict->master))
			{
				vict = vict->master;
			}
			else
			{
				vict = NULL;
			}
		}
		// end by WorM
		if (!vict)
		{
			send_to_char("Нет такого персонажа.\r\n", ch);
			return;
		}
		else if (vict == ch)
		{
			send_to_char("Вы и так лидер группы...\r\n", ch);
			return;
		}
		else if (!AFF_FLAGGED(vict, AFF_GROUP) || vict->master != ch)
		{
			send_to_char(ch, "%s не является членом вашей группы.\r\n", GET_NAME(vict));
			return;
		}
		change_leader(ch, vict);
		return;
	}

	if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		send_to_char(NOPERSON, ch);
	else if ((vict->master != ch) && (vict != ch))
		act("$N2 нужно следовать за вами, чтобы стать членом вашей группы.", FALSE, ch, 0, vict, TO_CHAR);
	else
	{
		if (!AFF_FLAGGED(vict, AFF_GROUP))
		{
			if (AFF_FLAGGED(vict, AFF_CHARM) || MOB_FLAGGED(vict, MOB_ANGEL) || IS_HORSE(vict))
			{
				send_to_char("Только равноправные персонажи могут быть включены в группу.\r\n", ch);
				send_to_char("Только равноправные персонажи могут быть включены в группу.\r\n", vict);
			};
			if (f_number >= max_group_size(ch))
			{
				send_to_char("Вы больше никого не можете принять в группу.\r\n", ch);
				return;
			}
			perform_group(ch, ch);
			perform_group(ch, vict);
		}
		else if (ch != vict)
		{
			act("$N исключен$A из состава вашей группы.", FALSE, ch, 0, vict, TO_CHAR);
			act("Вы исключены из группы $n1!", FALSE, ch, 0, vict, TO_VICT);
			act("$N был$G исключен$A из группы $n1!", FALSE, ch, 0, vict, TO_NOTVICT | TO_ARENA_LISTEN);
			REMOVE_BIT(AFF_FLAGS(vict, AFF_GROUP), AFF_GROUP);
		}
	}
}

ACMD(do_ungroup)
{
	struct follow_type *f, *next_fol;
	CHAR_DATA *tch;

	one_argument(argument, buf);

	if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP)))
	{
		send_to_char("Вы же не лидер группы!\r\n", ch);
		return;
	}

	if (!*buf)
	{
		sprintf(buf2, "Вы исключены из группы %s.\r\n", GET_PAD(ch, 1));
		for (f = ch->followers; f; f = next_fol)
		{
			next_fol = f->next;
			if (AFF_FLAGGED(f->follower, AFF_GROUP))
			{
				REMOVE_BIT(AFF_FLAGS(f->follower, AFF_GROUP), AFF_GROUP);
				send_to_char(buf2, f->follower);
				if (!AFF_FLAGGED(f->follower, AFF_CHARM) && !(IS_NPC(f->follower)
						&& AFF_FLAGGED(f->follower, AFF_HORSE)))
					stop_follower(f->follower, SF_EMPTY);
			}
		}
		REMOVE_BIT(AFF_FLAGS(ch, AFF_GROUP), AFF_GROUP);
		send_to_char("Вы распустили группу.\r\n", ch);
		return;
	}
	for (f = ch->followers; f; f = next_fol)
	{
		next_fol = f->next;
		tch = f->follower;
		if (isname(buf, tch->get_pc_name()) && !AFF_FLAGGED(tch, AFF_CHARM) && !IS_HORSE(tch))
		{
			REMOVE_BIT(AFF_FLAGS(tch, AFF_GROUP), AFF_GROUP);
			act("$N более не член вашей группы.", FALSE, ch, 0, tch, TO_CHAR);
			act("Вы исключены из группы $n1!", FALSE, ch, 0, tch, TO_VICT);
			act("$N был$G изгнан$A из группы $n1!", FALSE, ch, 0, tch, TO_NOTVICT | TO_ARENA_LISTEN);
			stop_follower(tch, SF_EMPTY);
			return;
		}
	}
	send_to_char("Этот игрок не входит в состав вашей группы.\r\n", ch);
	return;
}

ACMD(do_report)
{
	CHAR_DATA *k;
	struct follow_type *f;

	if (!AFF_FLAGGED(ch, AFF_GROUP) && !AFF_FLAGGED(ch, AFF_CHARM))
	{
		send_to_char("И перед кем вы отчитываетесь?\r\n", ch);
		return;
	}
	if (IS_MANA_CASTER(ch))
	{
		sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V, %d(%d)M\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch),
				GET_HIT(ch), GET_REAL_MAX_HIT(ch),
				GET_MOVE(ch), GET_REAL_MAX_MOVE(ch),
				GET_MANA_STORED(ch), GET_MAX_MANA(ch));
	}
	else if (AFF_FLAGGED(ch, AFF_CHARM))
	{
		AFFECT_DATA *aff;
		int loyalty = 0;
		for (aff = ch->affected; aff; aff = aff->next)
			if (aff->type == SPELL_CHARM)
			{
				loyalty = aff->duration / 2;
				break;
			}
		sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V, %dL\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch),
				GET_HIT(ch), GET_REAL_MAX_HIT(ch),
				GET_MOVE(ch), GET_REAL_MAX_MOVE(ch),
				loyalty);
	}
	else
	{
		sprintf(buf, "%s доложил%s : %d(%d)H, %d(%d)V\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch),
				GET_HIT(ch), GET_REAL_MAX_HIT(ch),
				GET_MOVE(ch), GET_REAL_MAX_MOVE(ch));
	}
	CAP(buf);
	k = (ch->master ? ch->master : ch);
	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch && !AFF_FLAGGED(f->follower, AFF_DEAFNESS))
			send_to_char(buf, f->follower);
	if (k != ch && !AFF_FLAGGED(k, AFF_DEAFNESS))
		send_to_char(buf, k);
	send_to_char("Вы доложили о состоянии всем членам вашей группы.\r\n", ch);
}



ACMD(do_split)
{
	int amount, num, share, rest;
	CHAR_DATA *k;
	struct follow_type *f;

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (is_number(buf))
	{
		amount = atoi(buf);
		if (amount <= 0)
		{
			send_to_char("И как вы это планируете сделать?\r\n", ch);
			return;
		}
		if (amount > ch->get_gold())
		{
			send_to_char("И где бы взять вам столько денег?.\r\n", ch);
			return;
		}

		k = (ch->master ? ch->master : ch);

		if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
			num = 1;
		else
			num = 0;

		for (f = k->followers; f; f = f->next)
			if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
					!IS_NPC(f->follower) && IN_ROOM(f->follower) == IN_ROOM(ch))
				num++;

		if (num && AFF_FLAGGED(ch, AFF_GROUP))
		{
			share = amount / num;
			rest = amount % num;
		}
		else
		{
			send_to_char("С кем вы хотите разделить это добро?\r\n", ch);
			return;
		}

		ch->remove_gold(share * (num - 1));

		sprintf(buf, "%s разделил%s %d %s; вам досталось %d.\r\n",
				GET_NAME(ch), GET_CH_SUF_1(ch), amount, desc_count(amount, WHAT_MONEYu), share);
		if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch) && !IS_NPC(k) && k != ch)
		{
			send_to_char(buf, k);
			k->add_gold(share, true, true);
		}
		for (f = k->followers; f; f = f->next)
		{
			if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
					!IS_NPC(f->follower) && IN_ROOM(f->follower) == IN_ROOM(ch) && f->follower != ch)
			{
				send_to_char(buf, f->follower);
				f->follower->add_gold(share, true, true);
			}
		}
		sprintf(buf, "Вы разделили %d %s на %d  -  по %d каждому.\r\n",
				amount, desc_count(amount, WHAT_MONEYu), num, share);
		if (rest)
		{
			sprintf(buf + strlen(buf),
				"Как истинный еврей вы оставили %d %s (которые не смогли разделить нацело) себе.\r\n",
				rest, desc_count(rest, WHAT_MONEYu));
		}
		send_to_char(buf, ch);
		// клан-налог лутера с той части, которая пошла каждому в группе
		const long clan_tax = ClanSystem::do_gold_tax(ch, share);
		ch->remove_gold(clan_tax);
	}
	else
	{
		send_to_char("Сколько и чего вы хотите разделить?\r\n", ch);
		return;
	}
}

OBJ_DATA * get_obj_equip_or_carry(CHAR_DATA *ch, const std::string &text)
{
	int eq_num = 0;
	OBJ_DATA *obj = get_object_in_equip_vis(ch, text, ch->equipment, &eq_num);
	if (!obj)
	{
		obj = get_obj_in_list_vis(ch, text, ch->carrying);
	}
	return obj;
}

void apply_enchant(CHAR_DATA *ch, OBJ_DATA *obj, std::string text)
{
	std::string tmp_buf;
	GetOneParam(text, tmp_buf);
	if (tmp_buf.empty())
	{
		send_to_char("Укажите цель применения.\r\n", ch);
		return;
	}

	OBJ_DATA *target = get_obj_equip_or_carry(ch, tmp_buf);
	if (!target)
	{
		send_to_char(ch, "Окститесь, нет у вас %s.\r\n", arg);
		return;
	}

	if (OBJ_FLAGGED(target, ITEM_SETSTUFF))
	{
		std::string name = GET_OBJ_PNAME(target, 0) ? GET_OBJ_PNAME(target, 0) : "<null>";
		name[0] = UPPER(name[0]);
		send_to_char(ch, "%s не %s быть зачарован%s, т.к. %s частью набора предметов.\r\n",
				name.c_str(),
				GET_OBJ_SEX(target) == SEX_POLY ? "могут" : "может",
				GET_OBJ_VIS_SUF_6(target, ch),
				GET_OBJ_SEX(target) == SEX_POLY ? "являются" : "является");
		return;
	}
	if (GET_OBJ_TYPE(target) == ITEM_ENCHANT)
	{
		send_to_char(ch, "Этот предмет уже магический и не может быть зачарован.\r\n");
		return;
	}
	if (target->enchants.check(obj::ENCHANT_FROM_OBJ))
	{
		send_to_char(ch, "На %s уже наложено зачарование.\r\n",
			GET_OBJ_PNAME(target, 3));
		return;
	}

	int check_slots = GET_OBJ_WEAR(obj) & GET_OBJ_WEAR(target);
	if (check_slots > 0 && check_slots != ITEM_WEAR_TAKE)
	{
		send_to_char(ch, "Вы успешно зачаровали %s.\r\n", GET_OBJ_PNAME(target, 0));
		obj::enchant ench(obj);
		ench.apply_to_obj(target);
		extract_obj(obj);
	}
	else
	{
		int slots = obj->obj_flags.wear_flags;
		REMOVE_BIT(slots, ITEM_WEAR_TAKE);
		if (sprintbit(slots, wear_bits, buf2))
		{
			send_to_char(ch,
				"Это зачарование применяется к предметам со слотами надевания: %s\r\n", buf2);
		}
		else
		{
			send_to_char(ch,
				"Некорретное зачарование, не проставлены слоты надевания.\r\n", buf2);
		}
	}
}

ACMD(do_use)
{
	OBJ_DATA *mag_item;
	int do_hold = 0;	
	two_arguments(argument, arg, buf);
	char *buf_temp = str_dup(buf);
	if (!*arg)
	{
		sprintf(buf2, "Что вы хотите %s?\r\n", CMD_NAME);
		send_to_char(buf2, ch);
		return;
	}

	if (IS_SET(PRF_FLAGS(ch, PRF_IRON_WIND), PRF_IRON_WIND))
	{
		send_to_char("Вы в бою, и вам сейчас не до этих магических выкрутасов!\r\n", ch);
		return;
	}

	mag_item = GET_EQ(ch, WEAR_HOLD);
	if (!mag_item || !isname(arg, mag_item->aliases))
	{
		switch (subcmd)
		{
		case SCMD_RECITE:
		case SCMD_QUAFF:
			if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying)))
			{
				sprintf(buf2, "Окститесь, нет у вас %s.\r\n", arg);
				send_to_char(buf2, ch);
				return;
			}
			break;
		case SCMD_USE:
			mag_item = get_obj_in_list_vis(ch, arg, ch->carrying);
			if (!mag_item || GET_OBJ_TYPE(mag_item) != ITEM_ENCHANT)
			{
				sprintf(buf2, "Возьмите в руку '%s' перед применением!\r\n", arg);
				send_to_char(buf2, ch);
				return;
			}
			break;
		default:
			log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
			return;
		}
	}
	switch (subcmd)
	{
	case SCMD_QUAFF:
		if (IS_SET(PRF_FLAGS(ch, PRF_IRON_WIND), PRF_IRON_WIND))
		{
			send_to_char("Не стоит отвлекаться в бою!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(mag_item) != ITEM_POTION)
		{
			send_to_char("Осушить вы можете только напиток (ну, Богам еще пЫво по вкусу ;)\r\n", ch);
			return;
		}
		do_hold = 1;
		break;
	case SCMD_RECITE:
		if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL)
		{
			send_to_char("Пригодны для зачитывания только свитки.\r\n", ch);
			return;
		}
		do_hold = 1;
		break;
	case SCMD_USE:
		if (GET_OBJ_TYPE(mag_item) == ITEM_ENCHANT)
		{
			apply_enchant(ch, mag_item, buf);
			return;
		}
		if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) && (GET_OBJ_TYPE(mag_item) != ITEM_STAFF))
		{
			send_to_char("Применять можно только магические предметы!\r\n", ch);
			return;
		}
		// палочки с чармами/оживлялками юзают только кастеры и дружи до 25 левева
		if (GET_OBJ_VAL(mag_item, 3) == SPELL_CHARM
				|| GET_OBJ_VAL(mag_item, 3) == SPELL_ANIMATE_DEAD
				|| GET_OBJ_VAL(mag_item, 3) == SPELL_RESSURECTION)
		{
			if (!can_use_feat(ch, MAGIC_USER_FEAT))
			{
				send_to_char("Да, штука явно магическая! Но совершенно непонятно как ей пользоваться. :(\r\n", ch);
				return;
			}
		}
		break;
	}
	if (do_hold && GET_EQ(ch, WEAR_HOLD) != mag_item)
	{
		if (GET_EQ(ch, WEAR_BOTHS))
			do_hold = WEAR_BOTHS;
		else if (GET_EQ(ch, WEAR_SHIELD))
			do_hold = WEAR_SHIELD;
		else
			do_hold = WEAR_HOLD;

		if (GET_EQ(ch, do_hold))
		{
			act("Вы прекратили использовать $o3.", FALSE, ch, GET_EQ(ch, do_hold), 0, TO_CHAR);
			act("$n прекратил$g использовать $o3.", FALSE, ch, GET_EQ(ch, do_hold), 0, TO_ROOM | TO_ARENA_LISTEN);
			obj_to_char(unequip_char(ch, do_hold), ch);
		}
		if (GET_EQ(ch, WEAR_HOLD))
			obj_to_char(unequip_char(ch, WEAR_HOLD), ch);
		//obj_from_char(mag_item);
		equip_char(ch, mag_item, WEAR_HOLD);
	}
	if ((do_hold && GET_EQ(ch, WEAR_HOLD) == mag_item) || (!do_hold))
		mag_objectmagic(ch, mag_item, buf_temp);
	free (buf_temp);
		
}



ACMD(do_wimpy)
{
	int wimp_lev;

	// 'wimp_level' is a player_special. -gg 2/25/98
	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (!*arg)
	{
		if (GET_WIMP_LEV(ch))
		{
			sprintf(buf, "Вы попытаетесь бежать при %d ХП.\r\n", GET_WIMP_LEV(ch));
			send_to_char(buf, ch);
			return;
		}
		else
		{
			send_to_char("Вы будете драться, драться и драться (пока не помрете, ессно...)\r\n", ch);
			return;
		}
	}
	if (isdigit(*arg))
	{
		if ((wimp_lev = atoi(arg)) != 0)
		{
			if (wimp_lev < 0)
				send_to_char("Да, перегрев похоже. С такими хитами вы и так помрете :)\r\n", ch);
			else if (wimp_lev > GET_REAL_MAX_HIT(ch))
				send_to_char("Осталось только дожить до такого количества ХП.\r\n", ch);
			else if (wimp_lev > (GET_REAL_MAX_HIT(ch) / 2))
				send_to_char
				("Размечтались. Сбечь то можно, но не более половины максимальных ХП.\r\n", ch);
			else
			{
				sprintf(buf, "Ладушки. Вы сбегите (или сбежите) по достижению %d ХП.\r\n", wimp_lev);
				send_to_char(buf, ch);
				GET_WIMP_LEV(ch) = wimp_lev;
			}
		}
		else
		{
			send_to_char("Вы будете сражаться до конца (скорее всего своего ;).\r\n", ch);
			GET_WIMP_LEV(ch) = 0;
		}
	}
	else
		send_to_char
		("Уточните, при достижении какого количества ХП вы планируете сбежать (0 - драться до смерти)\r\n",
		 ch);
}

void set_display_bits(CHAR_DATA *ch, bool flag)
{
	if (flag)
	{
		SET_BIT(PRF_FLAGS(ch, PRF_DISPHP), PRF_DISPHP);
		SET_BIT(PRF_FLAGS(ch, PRF_DISPMANA), PRF_DISPMANA);
		SET_BIT(PRF_FLAGS(ch, PRF_DISPMOVE), PRF_DISPMOVE);
		SET_BIT(PRF_FLAGS(ch, PRF_DISPEXITS), PRF_DISPEXITS);
		SET_BIT(PRF_FLAGS(ch, PRF_DISPGOLD), PRF_DISPGOLD);
		SET_BIT(PRF_FLAGS(ch, PRF_DISPLEVEL), PRF_DISPLEVEL);
		SET_BIT(PRF_FLAGS(ch, PRF_DISPEXP), PRF_DISPEXP);
		SET_BIT(PRF_FLAGS(ch, PRF_DISPFIGHT), PRF_DISPFIGHT);
		if (!IS_IMMORTAL(ch))
		{
			SET_BIT(PRF_FLAGS(ch, PRF_DISP_TIMED), PRF_DISP_TIMED);
		}
	}
	else
	{
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPHP), PRF_DISPHP);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPMANA), PRF_DISPMANA);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPMOVE), PRF_DISPMOVE);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPEXITS), PRF_DISPEXITS);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPGOLD), PRF_DISPGOLD);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPLEVEL), PRF_DISPLEVEL);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPEXP), PRF_DISPEXP);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DISPFIGHT), PRF_DISPFIGHT);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_DISP_TIMED), PRF_DISP_TIMED);
	}
}

const char *DISPLAY_HELP =
	"Формат: статус { { Ж | Э | З | В | Д | У | О | Б | П } | все | нет }\r\n";

ACMD(do_display)
{
	if (IS_NPC(ch))
	{
		send_to_char("И зачем это монстру? Не юродствуйте.\r\n", ch);
		return;
	}
	skip_spaces(&argument);

	if (!*argument)
	{
		send_to_char(DISPLAY_HELP, ch);
		return;
	}

	if (!str_cmp(argument, "on") || !str_cmp(argument, "all") ||
			!str_cmp(argument, "вкл") || !str_cmp(argument, "все"))
	{
		set_display_bits(ch, true);
	}
	else if (!str_cmp(argument, "off")
		|| !str_cmp(argument, "none")
		|| !str_cmp(argument, "выкл")
		|| !str_cmp(argument, "нет"))
	{
		set_display_bits(ch, false);
	}
	else
	{
		set_display_bits(ch, false);

		const size_t len = strlen(argument);
		for (size_t i = 0; i < len; i++)
		{
			switch (LOWER(argument[i]))
			{
			case 'h':
			case 'ж':
				SET_BIT(PRF_FLAGS(ch, PRF_DISPHP), PRF_DISPHP);
				break;
			case 'w':
			case 'з':
				SET_BIT(PRF_FLAGS(ch, PRF_DISPMANA), PRF_DISPMANA);
				break;
			case 'm':
			case 'э':
				SET_BIT(PRF_FLAGS(ch, PRF_DISPMOVE), PRF_DISPMOVE);
				break;
			case 'e':
			case 'в':
				SET_BIT(PRF_FLAGS(ch, PRF_DISPEXITS), PRF_DISPEXITS);
				break;
			case 'g':
			case 'д':
				SET_BIT(PRF_FLAGS(ch, PRF_DISPGOLD), PRF_DISPGOLD);
				break;
			case 'l':
			case 'у':
				SET_BIT(PRF_FLAGS(ch, PRF_DISPLEVEL), PRF_DISPLEVEL);
				break;
			case 'x':
			case 'о':
				SET_BIT(PRF_FLAGS(ch, PRF_DISPEXP), PRF_DISPEXP);
				break;
			case 'б':
			case 'f':
				SET_BIT(PRF_FLAGS(ch, PRF_DISPFIGHT), PRF_DISPFIGHT);
				break;
			case 'п':
			case 't':
				SET_BIT(PRF_FLAGS(ch, PRF_DISP_TIMED), PRF_DISP_TIMED);
				break;
			case ' ':
				break;
			default:
				send_to_char(DISPLAY_HELP, ch);
				return;
			}
		}
	}

	send_to_char(OK, ch);
}

#define TOG_OFF 0
#define TOG_ON  1
const char *gen_tog_type[] = { "автовыходы", "autoexits",
							   "краткий", "brief",
							   "сжатый", "compact",
							   "цвет", "color",
							   "повтор", "norepeat",
							   "обращения", "notell",
							   "кто-то", "noinvistell",
							   "болтать", "nogossip",
							   "кричать", "noshout",
							   "орать", "noholler",
							   "поздравления", "nogratz",
							   "аукцион", "noauction",
							   "базар", "exchange",
							   "задание", "quest",
							   "автозаучивание", "automem",
							   "нет агров", "nohassle",
							   "призыв", "nosummon",
							   "частный", "nowiz",
							   "флаги комнат", "roomflags",
							   "замедление", "slowns",
							   "выслеживание", "trackthru",
							   "супервидение", "holylight",
							   "кодер", "coder",
							   "автозавершение", "goahead",
							   "группа", "showgroup",
							   "без двойников", "noclones",
							   "автопомощь", "autoassist",
							   "автограбеж", "autoloot",
							   "автодележ", "autosplit",
							   "брать куны", "automoney",
							   "арена", "arena",
							   "ширина", "length",
							   "высота", "width",
							   "экран", "screen",
							   "новости", "news",
							   "доски", "boards",
							   "хранилище", "chest",
							   "пклист", "pklist",
							   "политика", "politics",
							   "пкформат", "pkformat",
							   "соклановцы", "workmate",
							   "оффтоп", "offtop",
							   "потеря связи", "disconnect",
							   "ингредиенты", "ingredient",
							   "вспомнить", "remember",
							   "уведомления", "notify",
							   "карта", "map",
							   "вход в зону", "enter zone",
							   "опечатки", "misprint",
							   "магщиты", "mageshields",
							   "автопризыв", "autonosummon",
							   "сдемигодам", "sdemigod",
							   "\n"
							 };




struct gen_tog_param_type
{
	int level;
	int subcmd;
} gen_tog_param[] =
{
	{
		0, SCMD_AUTOEXIT}, {
		0, SCMD_BRIEF}, {
		0, SCMD_COMPACT}, {
		0, SCMD_COLOR}, {
		0, SCMD_NOREPEAT}, {
		0, SCMD_NOTELL}, {
		0, SCMD_NOINVISTELL}, {
		0, SCMD_NOGOSSIP}, {
		0, SCMD_NOSHOUT}, {
		0, SCMD_NOHOLLER}, {
		0, SCMD_NOGRATZ}, {
		0, SCMD_NOAUCTION}, {
		0, SCMD_NOEXCHANGE}, {
		0, SCMD_QUEST}, {
		0, SCMD_AUTOMEM}, {
		LVL_GRGOD, SCMD_NOHASSLE}, {
		0, SCMD_NOSUMMON}, {
		LVL_GOD, SCMD_NOWIZ}, {
		LVL_GRGOD, SCMD_ROOMFLAGS}, {
		LVL_IMPL, SCMD_SLOWNS}, {
		LVL_GOD, SCMD_TRACK}, {
		LVL_GOD, SCMD_HOLYLIGHT}, {
		LVL_IMPL, SCMD_CODERINFO}, {
		0, SCMD_GOAHEAD}, {
		0, SCMD_SHOWGROUP}, {
		0, SCMD_NOCLONES}, {
		0, SCMD_AUTOASSIST}, {
		0, SCMD_AUTOLOOT}, {
		0, SCMD_AUTOSPLIT}, {
		0, SCMD_AUTOMONEY}, {
		0, SCMD_NOARENA}, {
		0, SCMD_LENGTH}, {
		0, SCMD_WIDTH}, {
		0, SCMD_SCREEN}, {
		0, SCMD_NEWS_MODE}, {
		0, SCMD_BOARD_MODE}, {
		0, SCMD_CHEST_MODE}, {
		0, SCMD_PKL_MODE}, {
		0, SCMD_POLIT_MODE} , {
		0, SCMD_PKFORMAT_MODE}, {
		0, SCMD_WORKMATE_MODE}, {
		0, SCMD_OFFTOP_MODE}, {
		0, SCMD_ANTIDC_MODE}, {
		0, SCMD_NOINGR_MODE}, {
		0, SCMD_REMEMBER}, {
		0, SCMD_NOTIFY_EXCH}, {
		0, SCMD_DRAW_MAP}, {
		0, SCMD_ENTER_ZONE}, {
		LVL_GOD, SCMD_MISPRINT}, {
		0, SCMD_BRIEF_SHIELDS}, {
		0, SCMD_AUTO_NOSUMMON}, {
		LVL_IMPL, SCMD_SDEMIGOD}
};

ACMD(do_mode)
{
	int i, showhelp = FALSE;
	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, arg);
	if (!*arg)
	{
		do_toggle(ch, argument, 0, 0);
		return;
	}
	else if (*arg == '?')
		showhelp = TRUE;
	else if ((i = search_block(arg, gen_tog_type, FALSE)) < 0)
		showhelp = TRUE;
	else if (GET_LEVEL(ch) < gen_tog_param[i >> 1].level && !Privilege::check_flag(ch, Privilege::KRODER))
	{
		send_to_char("Эта команда вам недоступна.\r\n", ch);
		showhelp = TRUE;
	}
	else
		do_gen_tog(ch, argument, 0, gen_tog_param[i >> 1].subcmd);

	if (showhelp)
	{
		strcpy(buf, "Вы можете установить следующее.\r\n");
		for (i = 0; *gen_tog_type[i << 1] != '\n'; i++)
			if (GET_LEVEL(ch) >= gen_tog_param[i].level || Privilege::check_flag(ch, Privilege::KRODER))
				sprintf(buf + strlen(buf), "%-20s(%s)\r\n", gen_tog_type[i << 1], gen_tog_type[(i << 1) + 1]);
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}
}


// установки экрана flag: 0 - ширина, 1 - высота
void SetScreen(CHAR_DATA * ch, char *argument, int flag)
{
	if (IS_NPC(ch))
		return;
	skip_spaces(&argument);
	int size = atoi(argument);

	if (!flag && (size < 30 || size > 300))
		send_to_char("Ширина экрана должна быть в пределах 30 - 300 символов.\r\n", ch);
	else if (flag == 1 && (size < 10 || size > 100))
		send_to_char("Высота экрана должна быть в пределах 10 - 100 строк.\r\n", ch);
	else if (!flag)
	{
		STRING_LENGTH(ch) = size;
		send_to_char("Ладушки.\r\n", ch);
		ch->save_char();
	}
	else if (flag == 1)
	{
		STRING_WIDTH(ch) = size;
		send_to_char("Ладушки.\r\n", ch);
		ch->save_char();
	}
	else
	{
		std::ostringstream buffer;
		for (int i = 50; i > 0; --i)
			buffer << i << "\r\n";
		send_to_char(buffer.str(), ch);
	}
}

// * 'режим автограбеж все|ингредиенты', по дефолту - все
void set_autoloot_mode(CHAR_DATA *ch, char *argument)
{
	static const char *message_on = "Автоматический грабеж трупов включен.\r\n";
	static const char *message_no_ingr = "Автоматический грабеж трупов, исключая ингредиенты и магические компоненты, включен.\r\n";
	static const char *message_off = "Автоматический грабеж трупов выключен.\r\n";

	skip_spaces(&argument);
	if (!*argument)
	{
		if (PRF_TOG_CHK(ch, PRF_AUTOLOOT))
		{
			send_to_char(PRF_FLAGGED(ch, PRF_NOINGR_LOOT) ? message_no_ingr : message_on, ch);
		}
		else
		{
			send_to_char(message_off, ch);
		}
	}
	else if (is_abbrev(argument, "все"))
	{
		SET_BIT(PRF_FLAGS(ch, PRF_AUTOLOOT), PRF_AUTOLOOT);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_NOINGR_LOOT), PRF_NOINGR_LOOT);
		send_to_char(message_on, ch);
	}
	else if (is_abbrev(argument, "ингредиенты"))
	{
		SET_BIT(PRF_FLAGS(ch, PRF_AUTOLOOT), PRF_AUTOLOOT);
		SET_BIT(PRF_FLAGS(ch, PRF_NOINGR_LOOT), PRF_NOINGR_LOOT);
		send_to_char(message_no_ingr, ch);
	}
	else if (is_abbrev(argument, "нет"))
	{
		REMOVE_BIT(PRF_FLAGS(ch, PRF_AUTOLOOT), PRF_AUTOLOOT);
		REMOVE_BIT(PRF_FLAGS(ch, PRF_NOINGR_LOOT), PRF_NOINGR_LOOT);
		send_to_char(message_off, ch);
	}
	else
	{
		send_to_char("Формат команды: режим автограбеж <без-параметров|все|ингредиенты|нет>\r\n", ch);
	}
}

//Polud установка оффлайн-уведомлений базара
void setNotifyEchange(CHAR_DATA* ch, char *argument)
{
	skip_spaces(&argument);
	if (!*argument)
	{
		send_to_char(ch, "Формат команды: режим уведомления <минимальная цена, число от 0 до %ld>.\r\n", 0x7fffffff);
		return;
	}
	long size = atol(argument);
	if (size>=100)
	{
		send_to_char(ch, "Вам будут приходить уведомления о продаже с базара ваших лотов стоимостью не менее чем %ld %s.\r\n",
			size, desc_count(size, WHAT_MONEYa));
		NOTIFY_EXCH_PRICE(ch) = size;
		ch->save_char();
	}
	else if (size>=0 && size<100)
	{
		send_to_char(ch, "Вам не будут приходить уведомления о продаже с базара ваших лотов, так как указана цена меньше 100 кун.\r\n");
		NOTIFY_EXCH_PRICE(ch) = 0;
		ch->save_char();
	}
	else
	{
		send_to_char(ch, "Укажите стоимость лота от 0 до %ld\r\n", 0x7fffffff);
	}

}
//-Polud
ACMD(do_gen_tog)
{
	long result = 0;

	const char *tog_messages[][2] =
	{
		{"Вы защищены от призыва.\r\n",
			"Вы можете быть призваны.\r\n"},
		{"Nohassle disabled.\r\n",
		 "Nohassle enabled.\r\n"},
		{"Краткий режим выключен.\r\n",
		 "Краткий режим включен.\r\n"},
		{"Сжатый режим выключен.\r\n",
		 "Сжатый режим включен.\r\n"},
		{"К вам можно обратиться.\r\n",
		 "Вы глухи к обращениям.\r\n"},
		{"Вам будут выводиться сообщения аукциона.\r\n",
		 "Вы отключены от участия в аукционе.\r\n"},
		{"Вы слышите то, что орут.\r\n",
		 "Вы глухи к тому, что орут.\r\n"},
		{"Вы слышите всю болтовню.\r\n",
		 "Вы глухи к болтовне.\r\n"},
		{"Вы слышите все поздравления.\r\n",
		 "Вы глухи к поздравлениям.\r\n"},
		{"You can now hear the Wiz-channel.\r\n",
		 "You are now deaf to the Wiz-channel.\r\n"},
		{"Вы больше не выполняете задания.\r\n",
		 "Вы выполняете задание!\r\n"},
		{"Вы больше не будете видеть флаги комнат.\r\n",
		 "Вы способны различать флаги комнат.\r\n"},
		{"Ваши сообщения будут дублироваться.\r\n",
		 "Ваши сообщения не будут дублироваться вам.\r\n"},
		{"HolyLight mode off.\r\n",
		 "HolyLight mode on.\r\n"},
		{"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
		 "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
		{"Показ выходов автоматически выключен.\r\n",
		 "Показ выходов автоматически включен.\r\n"},
		{"Вы не можете более выследить сквозь двери.\r\n",
		 "Вы способны выслеживать сквозь двери.\r\n"},
		{"\r\n",
		 "\r\n"},
		{"Режим показа дополнительной информации выключен.\r\n",
		 "Режим показа дополнительной информации включен.\r\n"},
		{"Автозаучивание заклинаний выключено.\r\n",
		 "Автозаучивание заклинаний включено.\r\n"},
		{"Сжатие запрещено.\r\n",
		 "Сжатие разрешено.\r\n"},
		{"\r\n",
		 "\r\n"},
		{"Вы слышите все крики.\r\n",
		 "Вы глухи к крикам.\r\n"},
		{"Режим автозавершения (IAC GA) выключен.\r\n",
		 "Режим автозавершения (IAC GA) включен.\r\n"},
		{"При просмотре состава группы будут отображаться только персонажи и ваши последователи.\r\n",
		 "При просмотре состава группы будут отображаться все персонажи и последователи.\r\n"},
		{"Вы не будете автоматически помогать согрупникам.\r\n",
		 "Вы будете автоматически помогать согрупникам.\r\n"},
		{"", ""}, // SCMD_AUTOLOOT
		{"Вы не будете делить деньги, поднятые с трупов.\r\n",
		 "Вы будете автоматически делить деньги, поднятые с трупов.\r\n"},
		{"Вы не будете брать куны, оставшиеся в трупах.\r\n",
		 "Вы будете автоматически брать куны, оставшиеся в трупах.\r\n"},
		{"Вы будете слышать сообщения с арены.\r\n",
		 "Вы не будете слышать сообщения с арены.\r\n"},
		{"Вам будут выводиться сообщения базара.\r\n",
		 "Вы отключены от участия в базаре.\r\n"},
		{"При просмотре состава группы будут отображаться все последователи.\r\n",
		 "При просмотре состава группы не будут отображаться чужие двойники и хранители.\r\n"},
		{"К вам сможет обратиться кто угодно.\r\n",
		 "К вам смогут обратиться только те, кого вы видите.\r\n"},
		{"", ""}, // SCMD_LENGTH
		{"", ""}, // SCMD_WIDTH
		{"", ""}, // SCMD_SCREEN
		{"Вариант чтения новостей былин и дружины: лента.\r\n",
		 "Вариант чтения новостей былин и дружины: доска.\r\n"},
		{"Вы не видите уведомлений о новых сообщениях на досках.\r\n",
		 "Вы получаете уведомления о новых сообщениях на досках.\r\n"},
		{"", ""}, // SCMD_CHEST_MODE
		{"Вы игнорируете уведомления о добавлении или очистке вас из листов дружин.\r\n",
		 "Вы получаете уведомления о добавлении или очистке вас из листов дружин.\r\n"},
		{"Вы игнорируете уведомления об изменениях политики вашей и к вашей дружине.\r\n",
		 "Вы получаете уведомления об изменениях политики вашей и к вашей дружине.\r\n"},
		{"Формат показа пкл/дрл установлен как 'полный'.\r\n",
		 "Формат показа пкл/дрл установлен как 'краткий'.\r\n"},
		{"Вам не будут показываться входы и выходы из игры ваших соклановцев.\r\n",
		 "Вы видите входы и выходы из игры ваших соклановцев.\r\n"},
		{"Вы отключены от канала оффтоп.\r\n",
		 "Вы будете слышать болтовню в канале оффтоп.\r\n"},
		{"Вы отключили защиту от потери связь во время боя.\r\n",
		 "Защита от потери связи во время боя включена.\r\n"},
		{"Показ покупок и продаж ингредиентов в режиме базара отключен.\r\n",
		 "Показ покупок и продаж ингредиентов в режиме базара включен.\r\n"},
		{"", ""}, 		// SCMD_REMEMBER
		{"", ""},		//SCMD_NOTIFY_EXCH
		{"Показ карты окрестностей при перемещении отключен.\r\n",
		 "Вы будете видеть карту окрестностей при перемещении.\r\n"},
		{"Показ информации при входе в новую зону отключен.\r\n",
		 "Вы будете видеть информацию при входе в новую зону.\r\n"},
		{"Показ уведомлений доски опечаток отключен.\r\n",
		 "Вы будете видеть уведомления доски опечаток.\r\n"},
		{"Показ сообщений при срабатывании магических щитов: полный.\r\n",
		 "Показ сообщений при срабатывании магических щитов: краткий.\r\n"},
		{"Автоматический режим защиты от призыва выключен.\r\n",
		 "Вы будете автоматически включать режим защиты от призыва после его использования.\r\n"},
	        {"Канал для демигодов выключен.\r\n",
	         "Канал для демигодов включен.\r\n"}
	};

	if (IS_NPC(ch))
		return;

	switch (subcmd)
	{
	case SCMD_NOSUMMON:
		result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
		break;
	case SCMD_NOHASSLE:
		result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
		break;
	case SCMD_BRIEF:
		result = PRF_TOG_CHK(ch, PRF_BRIEF);
		break;
	case SCMD_COMPACT:
		result = PRF_TOG_CHK(ch, PRF_COMPACT);
		break;
	case SCMD_NOTELL:
		result = PRF_TOG_CHK(ch, PRF_NOTELL);
		break;
	case SCMD_NOAUCTION:
		result = PRF_TOG_CHK(ch, PRF_NOAUCT);
		break;
	case SCMD_NOHOLLER:
		result = PRF_TOG_CHK(ch, PRF_NOHOLLER);
		break;
	case SCMD_NOGOSSIP:
		result = PRF_TOG_CHK(ch, PRF_NOGOSS);
		break;
	case SCMD_NOSHOUT:
		result = PRF_TOG_CHK(ch, PRF_NOSHOUT);
		break;
	case SCMD_NOGRATZ:
		result = PRF_TOG_CHK(ch, PRF_NOGOSS);
		break;
	case SCMD_NOWIZ:
		result = PRF_TOG_CHK(ch, PRF_NOWIZ);
		break;
	case SCMD_QUEST:
		result = PRF_TOG_CHK(ch, PRF_QUEST);
		break;
	case SCMD_ROOMFLAGS:
		result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
		break;
	case SCMD_NOREPEAT:
		result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
		break;
	case SCMD_HOLYLIGHT:
		result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
		break;
	case SCMD_SLOWNS:
		result = (nameserver_is_slow = !nameserver_is_slow);
		break;
	case SCMD_AUTOEXIT:
		result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
		break;
	case SCMD_CODERINFO:
		result = PRF_TOG_CHK(ch, PRF_CODERINFO);
		break;
	case SCMD_AUTOMEM:
		result = PRF_TOG_CHK(ch, PRF_AUTOMEM);
		break;
	case SCMD_COLOR:
		do_color(ch, argument, 0, 0);
		return;
		break;
	case SCMD_SDEMIGOD:
		result = PRF_TOG_CHK(ch, PRF_SDEMIGOD);
		break;
#if defined(HAVE_ZLIB)
	case SCMD_COMPRESS:
		result = toggle_compression(ch->desc);
		break;
#else
	case SCMD_COMPRESS:
		send_to_char("Compression not supported.\r\n", ch);
		return;
#endif
	case SCMD_GOAHEAD:
		result = PRF_TOG_CHK(ch, PRF_GOAHEAD);
		break;
	case SCMD_SHOWGROUP:
		result = PRF_TOG_CHK(ch, PRF_SHOWGROUP);
		break;
	case SCMD_AUTOASSIST:
		result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
		break;
	case SCMD_AUTOLOOT:
		set_autoloot_mode(ch, argument);
		return;
	case SCMD_AUTOSPLIT:
		result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
		break;
	case SCMD_AUTOMONEY:
		result = PRF_TOG_CHK(ch, PRF_AUTOMONEY);
		break;
	case SCMD_NOARENA:
		result = PRF_TOG_CHK(ch, PRF_NOARENA);
		break;
	case SCMD_NOEXCHANGE:
		result = PRF_TOG_CHK(ch, PRF_NOEXCHANGE);
		break;
	case SCMD_NOCLONES:
		result = PRF_TOG_CHK(ch, PRF_NOCLONES);
		break;
	case SCMD_NOINVISTELL:
		result = PRF_TOG_CHK(ch, PRF_NOINVISTELL);
		break;
	case SCMD_LENGTH:
		SetScreen(ch, argument, 0);
		return;
		break;
	case SCMD_WIDTH:
		SetScreen(ch, argument, 1);
		return;
		break;
	case SCMD_SCREEN:
		SetScreen(ch, argument, 2);
		return;
		break;
	case SCMD_NEWS_MODE:
		result = PRF_TOG_CHK(ch, PRF_NEWS_MODE);
		break;
	case SCMD_BOARD_MODE:
		result = PRF_TOG_CHK(ch, PRF_BOARD_MODE);
		break;
	case SCMD_CHEST_MODE:
	{
		std::string buffer = argument;
		SetChestMode(ch, buffer);
		break;
	}
	case SCMD_PKL_MODE:
		result = PRF_TOG_CHK(ch, PRF_PKL_MODE);
		break;
	case SCMD_POLIT_MODE:
		result = PRF_TOG_CHK(ch, PRF_POLIT_MODE);
		break;
	case SCMD_PKFORMAT_MODE:
		result = PRF_TOG_CHK(ch, PRF_PKFORMAT_MODE);
		break;
	case SCMD_WORKMATE_MODE:
		result = PRF_TOG_CHK(ch, PRF_WORKMATE_MODE);
		break;
	case SCMD_OFFTOP_MODE:
		result = PRF_TOG_CHK(ch, PRF_OFFTOP_MODE);
		break;
	case SCMD_ANTIDC_MODE:
		result = PRF_TOG_CHK(ch, PRF_ANTIDC_MODE);
		break;
	case SCMD_NOINGR_MODE:
		result = PRF_TOG_CHK(ch, PRF_NOINGR_MODE);
		break;
	case SCMD_REMEMBER:
	{
		skip_spaces(&argument);
		if (!*argument)
		{
			send_to_char("Формат команды: режим вспомнить <число строк от 1 до 100>.\r\n", ch);
			return;
		}
		unsigned int size = atoi(argument);
		if (ch->remember_set_num(size))
		{
			send_to_char(ch, "Количество выводимых строк по команде 'вспомнить' установлено в %d.\r\n", size);
			ch->save_char();
		}
		else
		{
			send_to_char(ch, "Количество строк для вывода может быть в пределах от 1 до %d.\r\n", Remember::MAX_REMEMBER_NUM);
		}
		return;
	}
	case SCMD_NOTIFY_EXCH:
	{
		setNotifyEchange(ch, argument);
		return;
	}
	case SCMD_DRAW_MAP:
		result = PRF_TOG_CHK(ch, PRF_DRAW_MAP);
		break;
	case SCMD_ENTER_ZONE:
		result = PRF_TOG_CHK(ch, PRF_ENTER_ZONE);
		break;
	case SCMD_MISPRINT:
		result = PRF_TOG_CHK(ch, PRF_MISPRINT);
		break;
	case SCMD_BRIEF_SHIELDS:
		result = PRF_TOG_CHK(ch, PRF_BRIEF_SHIELDS);
		break;
	case SCMD_AUTO_NOSUMMON:
		result = PRF_TOG_CHK(ch, PRF_AUTO_NOSUMMON);
		break;
	default:
		log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
		return;
	}

	if (result)
		send_to_char(tog_messages[subcmd][TOG_ON], ch);
	else
		send_to_char(tog_messages[subcmd][TOG_OFF], ch);

	return;
}

ACMD(do_pray)
{
	int metter = -1, i;
	OBJ_DATA *obj = NULL;
	AFFECT_DATA af;
	struct timed_type timed;


	if (IS_NPC(ch))
		return;

	if (!IS_IMMORTAL(ch) &&
			((subcmd == SCMD_DONATE && GET_RELIGION(ch) != RELIGION_POLY) ||
			 (subcmd == SCMD_PRAY && GET_RELIGION(ch) != RELIGION_MONO)))
	{
		send_to_char("Не кощунствуйте!\r\n", ch);
		return;
	}

	if (subcmd == SCMD_DONATE && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_POLY))
	{
		send_to_char("Найдите подходящее место для вашей жертвы.\r\n", ch);
		return;
	}
	if (subcmd == SCMD_PRAY && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_MONO))
	{
		send_to_char("Это место не обладает необходимой святостью.\r\n", ch);
		return;
	}

	half_chop(argument, arg, buf);

	if (!*arg || (metter = search_block(arg, pray_whom, FALSE)) < 0)
	{
		if (subcmd == SCMD_DONATE)
		{
			send_to_char("Вы можете принести жертву :\r\n", ch);
			for (metter = 0; *(pray_metter[metter]) != '\n'; metter++)
				if (*(pray_metter[metter]) == '-')
				{
					send_to_char(pray_metter[metter], ch);
					send_to_char("\r\n", ch);
				}
			send_to_char("Укажите, кому и что вы хотите жертвовать.\r\n", ch);
		}
		else if (subcmd == SCMD_PRAY)
		{
			send_to_char("Вы можете вознести молитву :\r\n", ch);
			for (metter = 0; *(pray_metter[metter]) != '\n'; metter++)
				if (*(pray_metter[metter]) == '*')
				{
					send_to_char(pray_metter[metter], ch);
					send_to_char("\r\n", ch);
				}
			send_to_char("Укажите, кому вы хотите вознести молитву.\r\n", ch);
		}
		return;
	}

	if (subcmd == SCMD_DONATE && *(pray_metter[metter]) != '-')
	{
		send_to_char("Приносите жертвы только своим Богам.\r\n", ch);
		return;
	}

	if (subcmd == SCMD_PRAY && *(pray_metter[metter]) != '*')
	{
		send_to_char("Возносите молитвы только своим Богам.\r\n", ch);
		return;
	}

	if (subcmd == SCMD_DONATE)
	{
		if (!*buf || !(obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
		{
			send_to_char("Вы должны пожертвовать что-то стоящее.\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(obj) != ITEM_FOOD && GET_OBJ_TYPE(obj) != ITEM_TREASURE)
		{
			send_to_char("Богам неугодна эта жертва.\r\n", ch);
			return;
		}
	}
	else if (subcmd == SCMD_PRAY)
	{
		if (ch->get_gold() < 10)
		{
			send_to_char("У вас не хватит денег на свечку.\r\n", ch);
			return;
		}
	}
	else
		return;

	if (!IS_IMMORTAL(ch) && (timed_by_skill(ch, SKILL_RELIGION)
							 || affected_by_spell(ch, SPELL_RELIGION)))
	{
		send_to_char("Вы не можете так часто взывать к Богам.\r\n", ch);
		return;
	}

	timed.skill = SKILL_RELIGION;
	timed.time = 12;
	timed_to_char(ch, &timed);

	for (i = 0; pray_affect[i].metter >= 0; i++)
		if (pray_affect[i].metter == metter)
		{
			af.type = SPELL_RELIGION;
			af.duration = pc_duration(ch, 12, 0, 0, 0, 0);
			af.modifier = pray_affect[i].modifier;
			af.location = pray_affect[i].location;
			af.bitvector = pray_affect[i].bitvector;
			af.battleflag = pray_affect[i].battleflag;
			affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
		}

	if (subcmd == SCMD_PRAY)
	{
		sprintf(buf, "$n затеплил$g свечку и вознес$q молитву %s.", pray_whom[metter]);
		act(buf, FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		sprintf(buf, "Вы затеплили свечку и вознесли молитву %s.", pray_whom[metter]);
		act(buf, FALSE, ch, 0, 0, TO_CHAR);
		ch->remove_gold(10);
	}
	else if (subcmd == SCMD_DONATE && obj)
	{
		sprintf(buf, "$n принес$q $o3 в жертву %s.", pray_whom[metter]);
		act(buf, FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
		sprintf(buf, "Вы принесли $o3 в жертву %s.", pray_whom[metter]);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		obj_from_char(obj);
		extract_obj(obj);
	}
}

ACMD(do_recall)
{
	if (IS_NPC(ch))
	{
		send_to_char("Монстрам некуда возвращаться!\r\n", ch);
		return;
	}

	const int rent_room = real_room(GET_LOADROOM(ch));
	if (rent_room == NOWHERE || ch->in_room == NOWHERE)
	{
		send_to_char("Вам некуда возвращаться!\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch)
		&& (SECT(ch->in_room) == SECT_SECRET
			|| ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)
			|| ROOM_FLAGGED(ch->in_room, ROOM_DEATH)
			|| ROOM_FLAGGED(ch->in_room, ROOM_SLOWDEATH)
			|| ROOM_FLAGGED(ch->in_room, ROOM_TUNNEL)
			|| ROOM_FLAGGED(ch->in_room, ROOM_NORELOCATEIN)
			|| ROOM_FLAGGED(ch->in_room, ROOM_NOTELEPORTIN)
			|| ROOM_FLAGGED(ch->in_room, ROOM_ICEDEATH)
			|| ROOM_FLAGGED(ch->in_room, ROOM_GODROOM)
			|| !Clan::MayEnter(ch, ch->in_room, HCE_PORTAL)
			|| !Clan::MayEnter(ch, rent_room, HCE_PORTAL)))
	{
		send_to_char("У вас не получилось вернуться!\r\n", ch);
		return;
	}

	send_to_char("Вам очень захотелось оказаться подальше от этого места!\r\n", ch);
	if (IS_GOD(ch) || Noob::is_noob(ch))
	{
		if (ch->in_room != rent_room)
		{
			send_to_char("Вы почувствовали, как чья-то огромная рука подхватила вас и куда-то унесла!\r\n", ch);
			act("$n поднял$a глаза к небу и внезапно исчез$q!", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			char_from_room(ch);
			char_to_room(ch, rent_room);
			look_at_room(ch, 0);
			act("$n внезапно появил$u в центре комнаты!", TRUE, ch, 0, 0, TO_ROOM);
		}
		else
		{
			send_to_char("Но тут и так достаточно мирно...\r\n", ch);
		}
	}
	else
	{
		send_to_char("Ну и хотите себе на здоровье...\r\n", ch);
	}
}

void perform_beep(CHAR_DATA *ch, CHAR_DATA *vict)
{
	send_to_char(CCRED(vict, C_NRM), vict);
	sprintf(buf, "\007\007 $n вызывает вас!");
	act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
	send_to_char(CCNRM(vict, C_NRM), vict);

	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
	else
	{
		send_to_char(CCRED(ch, C_CMP), ch);
		sprintf(buf, "Вы вызвали $N3.");
		act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
		send_to_char(CCNRM(ch, C_CMP), ch);
	}
}

ACMD(do_beep)
{
	CHAR_DATA *vict = NULL;

	one_argument(argument, buf);

	if (!*buf)
		send_to_char("Кого вызывать?\r\n", ch);
	else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)) || IS_NPC(vict))
		send_to_char(NOPERSON, ch);
	else if (ch == vict)
		send_to_char("\007\007Вы вызвали себя!\r\n", ch);
	else if (PRF_FLAGGED(ch, PRF_NOTELL))
		send_to_char("Вы не можете пищать в режиме обращения.\r\n", ch);
	else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
		send_to_char("Стены заглушили ваш писк.\r\n", ch);
	else if (!IS_NPC(vict) && !vict->desc)	// linkless
		act("$N потерял связь.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (PLR_FLAGGED(vict, PLR_WRITING))
		act("$N пишет сейчас; Попищите позже.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else
		perform_beep(ch, vict);
}


//Polos.insert_wanted_gem

void insert_wanted_gem::show(CHAR_DATA *ch, int gem_vnum)
{
	map< int, alias_type >::iterator it;
	alias_type::iterator alias_it;
	char buf[MAX_INPUT_LENGTH];

	it = content.find(gem_vnum);
	if (it == content.end()) return;

	send_to_char("Будучи искусным ювелиром, вы можете выбрать, какого эффекта вы желаете добиться: \r\n", ch);
	for (alias_it = it->second.begin();alias_it != it->second.end();++alias_it)
	{
		sprintf(buf, " %s\r\n", alias_it->first.c_str());
		send_to_char(buf, ch);
	}
}

void insert_wanted_gem::init()
{
	ifstream file;
	char dummy;
	char buf[MAX_INPUT_LENGTH];
	string str;
	int val, val2, curr_val = 0;
	map< int, alias_type >::iterator it;
	alias_type temp;
	alias_type::iterator alias_it;
	struct int3 arr;

	content.clear();
	temp.clear();

	file.open(LIB_MISC "insert_wanted.lst", fstream::in);
	if (!file.is_open()) return log("failed to open insert_wanted.lst.");

	file.width(MAX_INPUT_LENGTH);

	while (1)
	{
		if (!(file >> dummy)) break;

		if (dummy == '*')
		{
			if (!file.getline(buf, MAX_INPUT_LENGTH)) break;
			continue;
		}

		if (dummy == '#')
		{
			if (!(file >> val)) break;

			if (!temp.empty() && (curr_val != 0))
			{
				content.insert(std::make_pair(curr_val, temp));
				temp.clear();
			}
			curr_val = val;

			continue;
		}

		if (dummy == '$')
		{
			if (curr_val == 0) break;
			if (!(file >> str))  break;
			if (str.size() > MAX_ALIAS_LENGTH - 1) break;
			if (!(file >> val))  break;
			if (curr_val == 0) break;

			switch (val)
			{
			case 1:
				if (!(file >> val >> val2)) break;

				arr.type = 1;
				arr.bit = val;
				arr.qty = val2;
				temp.insert(std::make_pair(str, arr));

				break;

			case 2:
			case 3:
				if (!(file >> val2))  break;

				arr.type = val;
				arr.bit = val2;
				arr.qty = 0;
				temp.insert(std::make_pair(str, arr));

				break;
			default:
			{
				log("something goes wrong\r\nclosed insert_wanted.lst.");
				file.close();
				return;
			}
			};

		}

	}

	file.close();
	log("closed insert_wanted.lst.");

	if (!temp.empty())
	{
		content.insert(std::make_pair(curr_val, temp));
	}

	return;
}

int insert_wanted_gem::get_type(int gem_vnum, string str)
{
	return content[gem_vnum][str].type;
}

int insert_wanted_gem::get_bit(int gem_vnum, string str)
{
	return content[gem_vnum][str].bit;
}

int insert_wanted_gem::get_qty(int gem_vnum, string str)
{
	return content[gem_vnum][str].qty;
}

int insert_wanted_gem::exist(int gem_vnum, string str)
{
	map< int, alias_type >::iterator it;
	alias_type::iterator alias_it;

	it = content.find(gem_vnum);
	if (it == content.end()) return 0;
	alias_it = content[gem_vnum].find(str);
	if (alias_it == content[gem_vnum].end()) return 0;

	return 1;
}

//-Polos.insert_wanted_gem


int make_hole(CHAR_DATA *ch)
{
	if (roundup(world[ch->in_room]->holes / HOLES_TIME) >= dig_vars.hole_max_deep)
	{
		send_to_char("Тут и так все перекопано.\r\n", ch);
		return 0;
	}


	return 1;
}

void break_inst(CHAR_DATA *ch)
{
	int i;
	char buf[300];

	for (i = WEAR_WIELD; i <= WEAR_BOTHS; i++)
	{
		if (GET_EQ(ch, i) && (strstr(GET_EQ(ch, i)->aliases, "лопата") || strstr(GET_EQ(ch, i)->aliases, "кирка")))
		{
			if (GET_OBJ_CUR(GET_EQ(ch, i)) > 1)
			{
				if (number(1, dig_vars.instr_crash_chance) == 1)
				{
					GET_OBJ_CUR(GET_EQ(ch, i)) -= 1;
				}
			}
			else
			{
				GET_EQ(ch, i)->set_timer(0);
			}
			if (GET_OBJ_CUR(GET_EQ(ch, i)) <= 1 && number(1, 3) == 1)
			{
				sprintf(buf, "Ваша %s трескается!\r\n", GET_EQ(ch, i)->short_description);
				send_to_char(buf, ch);
			}
		}
	}

}

int check_for_dig(CHAR_DATA *ch)
{
	int i;

	for (i = WEAR_WIELD; i <= WEAR_BOTHS; i++)
	{
		if (GET_EQ(ch, i) && (strstr(GET_EQ(ch, i)->aliases, "лопата") || strstr(GET_EQ(ch, i)->aliases, "кирка")))
		{
			return 1;
		}
	}
	return 0;
}

void dig_obj(CHAR_DATA *ch, struct obj_data *obj)
{
	char textbuf[300];

	if (GET_OBJ_MIW(obj) >=
			obj_index[GET_OBJ_RNUM(obj)].stored + obj_index[GET_OBJ_RNUM(obj)].number || GET_OBJ_MIW(obj) == -1)
	{
		sprintf(textbuf, "Вы нашли %s!\r\n", obj->PNames[3]);
		send_to_char(textbuf, ch);
		sprintf(textbuf, "$n выкопал$g %s!\r\n", obj->PNames[3]);
		act(textbuf, FALSE, ch, 0, 0, TO_ROOM);
		if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		{
			send_to_char("Вы не смогли унести столько предметов.\r\n", ch);
			obj_to_room(obj, ch->in_room);
		}
		else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
		{
			send_to_char("Вы не смогли унести такой веc.\r\n", ch);
			obj_to_room(obj, ch->in_room);
		}
		else
			obj_to_char(obj, ch);
	}
}

ACMD(do_dig)
{
	CHAR_DATA *mob;
	struct obj_data *obj;
	char textbuf[300];
	int percent, prob;
	int stone_num, random_stone;
	int vnum;
	int old_wis, old_int;

	if (IS_NPC(ch) || !ch->get_skill(SKILL_DIG))
	{
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (!check_for_dig(ch) && !IS_IMMORTAL(ch))
	{
		send_to_char("Вам бы лопату взять в руки... Или кирку...\r\n", ch);
		return;
	}


	if (world[IN_ROOM(ch)]->sector_type != SECT_MOUNTAIN &&
			world[IN_ROOM(ch)]->sector_type != SECT_HILLS && !IS_IMMORTAL(ch))
	{
		send_to_char("Полезные минералы водятся только в гористой местности!\r\n", ch);
		return;
	}

	if (!WAITLESS(ch) && on_horse(ch))
	{
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, AFF_BLIND) && !IS_IMMORTAL(ch))
	{
		send_to_char("Вы слепы и не видите где копать.\r\n", ch);
		return;
	}

	if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) && !IS_IMMORTAL(ch))
	{
		send_to_char("Куда копать? Чего копать? Ничего не видно...\r\n", ch);
		return;
	}

	if (!make_hole(ch) && !IS_IMMORTAL(ch))
		return;

	if (!check_moves(ch, dig_vars.need_moves))
		return;

	world[ch->in_room]->holes += HOLES_TIME;

	send_to_char("Вы стали усердно ковырять каменистую почву...\r\n", ch);
	act("$n стал$g усердно ковырять каменистую почву...", FALSE, ch, 0, 0, TO_ROOM);

	break_inst(ch);

	// случайные события

	if (number(1, dig_vars.treasure_chance) == 1)	// копнули клад
	{
		int gold = number(40000, 60000);
		send_to_char("Вы нашли клад!\r\n", ch);
		act("$n выкопал$g клад!", FALSE, ch, 0, 0, TO_ROOM);
		sprintf(textbuf, "Вы насчитали %i монет.\r\n", gold);
		send_to_char(textbuf, ch);
		ch->add_gold(gold);
		split_or_clan_tax(ch, gold);
		return;
	}

	if (number(1, dig_vars.mob_chance) == 1)	// копнули мертвяка
	{
		vnum = number(dig_vars.mob_vnum_start, dig_vars.mob_vnum_end);
		mob = read_mobile(real_mobile(vnum), REAL);
		if (mob)
		{
			if (GET_LEVEL(mob) <= GET_LEVEL(ch))
			{
				SET_BIT(MOB_FLAGS(mob, MOB_AGGRESSIVE), MOB_AGGRESSIVE);
				sprintf(textbuf, "Вы выкопали %s!\r\n", mob->player_data.PNames[3]);
				send_to_char(textbuf, ch);
				sprintf(textbuf, "$n выкопал$g %s!\r\n", mob->player_data.PNames[3]);
				act(textbuf, FALSE, ch, 0, 0, TO_ROOM);
				char_to_room(mob, ch->in_room);
				return;
			}
		}
		else
			send_to_char("Не найден прототип обжекта!", ch);
	}
	if (number(1, dig_vars.pandora_chance) == 1)	// копнули шкатулку пандоры
	{
		vnum = dig_vars.pandora_vnum;
		obj = read_object(real_object(vnum), REAL);
		if (obj)
			dig_obj(ch, obj);
		else
			send_to_char("Не найден прототип обжекта!", ch);
		return;
	}
	if (number(1, dig_vars.trash_chance) == 1)	// копнули мусор
	{
		vnum = number(dig_vars.trash_vnum_start, dig_vars.trash_vnum_end);
		obj = read_object(real_object(vnum), REAL);
		if (obj)
			dig_obj(ch, obj);
		else
			send_to_char("Не найден прототип обжекта!", ch);
		return;
	}

	percent = number(1, skill_info[SKILL_DIG].max_percent);
	prob = ch->get_skill(SKILL_DIG);
	old_int = ch->get_int();
	old_wis = ch->get_wis();
	ch->set_int(ch->get_int() + 14 - MAX(14, GET_REAL_INT(ch)));
	ch->set_wis(ch->get_wis() + 14 - MAX(14, GET_REAL_WIS(ch)));
	improove_skill(ch, SKILL_DIG, 0, 0);
	ch->set_int(old_int);
	ch->set_wis(old_wis);


	WAIT_STATE(ch, dig_vars.lag * PULSE_VIOLENCE);



	if (percent > prob / dig_vars.prob_divide)
	{
		send_to_char("Вы только зря расковыряли землю и раскидали камни.\r\n", ch);
		act("$n отрыл$g смешную ямку.", FALSE, ch, 0, 0, TO_ROOM);
		return;
	}


	// возможность копать мощные камни зависит от навыка

	random_stone = number(1, MIN(prob, 100));
	if (random_stone >= dig_vars.stone9_skill)
		stone_num = 9;
	else if (random_stone >= dig_vars.stone8_skill)
		stone_num = 8;
	else if (random_stone >= dig_vars.stone7_skill)
		stone_num = 7;
	else if (random_stone >= dig_vars.stone6_skill)
		stone_num = 6;
	else if (random_stone >= dig_vars.stone5_skill)
		stone_num = 5;
	else if (random_stone >= dig_vars.stone4_skill)
		stone_num = 4;
	else if (random_stone >= dig_vars.stone3_skill)
		stone_num = 3;
	else if (random_stone >= dig_vars.stone2_skill)
		stone_num = 2;
	else if (random_stone >= dig_vars.stone1_skill)
		stone_num = 1;
	else
		stone_num = 0;

	if (stone_num == 0)
	{
		send_to_char("Вы долго копали, но так и не нашли ничего полезного.\r\n", ch);
		act("$n долго копал$g землю, но все без толку.", FALSE, ch, 0, 0, TO_ROOM);
		return;
	}

	vnum = dig_vars.stone1_vnum - 1 + stone_num;
	obj = read_object(real_object(vnum), REAL);
	if (obj)
	{
		if (number(1, dig_vars.glass_chance) != 1)
			GET_OBJ_MATER(obj) = 11;
		else
			GET_OBJ_MATER(obj) = 18;

		dig_obj(ch, obj);
	}
	else
		send_to_char("Не найден прототип обжекта!", ch);
}

void set_obj_aff(struct obj_data *itemobj, int bitv)
{
	int i;

	for (i = 0; weapon_affect[i].aff_bitvector != -1; i++)
	{
		if (weapon_affect[i].aff_bitvector == bitv)
		{
			SET_BIT(GET_OBJ_AFF(itemobj, weapon_affect[i].aff_pos), weapon_affect[i].aff_pos);
		}
	}
}

void set_obj_eff(struct obj_data *itemobj, int type, int mod)
{
	int i;

	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (itemobj->affected[i].location == type)
		{
			(itemobj->affected[i].modifier) += mod;
			break;
		}
		else if (itemobj->affected[i].location == APPLY_NONE)
		{
			itemobj->affected[i].location = type;
			itemobj->affected[i].modifier = mod;
			break;
		}

}

extern struct index_data *obj_index;

bool is_dig_stone(OBJ_DATA *obj)
{
	if ((GET_OBJ_VNUM(obj) >= dig_vars.stone1_vnum
			&& GET_OBJ_VNUM(obj) <= dig_vars.stone1_vnum + 17)
		|| GET_OBJ_VNUM(obj) == DIG_GLASS_VNUM)
	{
		return true;
	}

	return false;
}

ACMD(do_insertgem)
{
	int percent, prob, i;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char buf[300];
	char *gem, *item;
	struct obj_data *gemobj, *itemobj;

	argument = two_arguments(argument, arg1, arg2);

	if (IS_NPC(ch) || !ch->get_skill(SKILL_INSERTGEM))
	{
		send_to_char("Но вы не знаете как.\r\n", ch);
		return;
	}

	if (!IS_IMMORTAL(ch))
	{
		if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_SMITH))
		{
			send_to_char("Вам нужно попасть в кузницу для этого.\r\n", ch);
			return;
		}
	}

	if (!*arg1)
	{
		send_to_char("Вплавить что?\r\n", ch);
		return;
	}
	else
		gem = arg1;

	if (!(gemobj = get_obj_in_list_vis(ch, gem, ch->carrying)))
	{
		sprintf(buf, "У вас нет '%s'.\r\n", gem);
		send_to_char(buf, ch);
		return;
	}

	if (!is_dig_stone(gemobj))
	{
		sprintf(buf, "Вы не умеете вплавлять %s.\r\n", gemobj->PNames[3]);
		send_to_char(buf, ch);
		return;
	}

	if (!*arg2)
	{
		send_to_char("Вплавить куда?\r\n", ch);
		return;
	}
	else
		item = arg2;

	if (!(itemobj = get_obj_in_list_vis(ch, item, ch->carrying)))
	{
		sprintf(buf, "У вас нет '%s'.\r\n", item);
		send_to_char(buf, ch);
		return;
	}
	if (GET_OBJ_MATER(itemobj) < 1 || (GET_OBJ_MATER(itemobj) > 6 && GET_OBJ_MATER(itemobj) != 13))
	{
		sprintf(buf, "%s состоит из неподходящего материала.\r\n", itemobj->PNames[0]);
		send_to_char(buf, ch);
		return;
	}
	/*(!(CAN_WEAR(itemobj, ITEM_WEAR_BODY)) &&
	   !(CAN_WEAR(itemobj, ITEM_WEAR_HEAD)) &&
	   !(CAN_WEAR(itemobj, ITEM_WEAR_LEGS)) &&
	   !(CAN_WEAR(itemobj, ITEM_WEAR_ARMS)) &&
	   !(CAN_WEAR(itemobj, ITEM_WEAR_SHIELD)) &&
	   !(CAN_WEAR(itemobj, ITEM_WEAR_WIELD)) &&
	   !(CAN_WEAR(itemobj, ITEM_WEAR_HOLD)) &&
	   !(CAN_WEAR(itemobj, ITEM_WEAR_BOTHS)))
	   {
	   send_to_char("Нет смысла вплавлять камень в предмет этого типа.\r\n", ch);
	   return;
	   } */

//    if (GET_OBJ_OWNER (itemobj) == GET_UNIQUE (ch))
//    {
//    if ((!(CAN_WEAR(itemobj, ITEM_WEAR_BODY)) && OBJ_FLAGGED(itemobj, ITEM_WITH2SLOTS)) ||
//      (OBJ_FLAGGED(itemobj, ITEM_WITH3SLOTS)))
//       {
//      send_to_char("В этот предмет вплавлено максимальное количество камней.\r\n", ch);
//        return;
//       }
//    }
//    else
//    {
//    if (((CAN_WEAR(itemobj, ITEM_WEAR_BODY) ||
//        CAN_WEAR(itemobj, ITEM_WEAR_SHIELD)) && OBJ_FLAGGED(itemobj, ITEM_WITH2SLOTS)))
//       {
//      send_to_char("В этот предмет вплавлено максимальное количество камней.\r\n", ch);
//        return;
//       }
	if (!OBJ_FLAGGED(itemobj, ITEM_WITH1SLOT) && !OBJ_FLAGGED(itemobj, ITEM_WITH2SLOTS)
			&& !OBJ_FLAGGED(itemobj, ITEM_WITH3SLOTS))
	{
		send_to_char("Вы не видите куда здесь можно вплавить камень.\r\n", ch);
		return;
	}
//    }

	if (!WAITLESS(ch) && on_horse(ch))
	{
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, AFF_BLIND))
	{
		send_to_char("Вы слепы!\r\n", ch);
		return;
	}

	if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) && !IS_IMMORTAL(ch))
	{
		send_to_char("Да тут темно хоть глаза выколи...\r\n", ch);
		return;
	}

	percent = number(1, skill_info[SKILL_INSERTGEM].max_percent);
	prob = ch->get_skill(SKILL_INSERTGEM);

	WAIT_STATE(ch, PULSE_VIOLENCE);

	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (itemobj->affected[i].location == APPLY_NONE)
		{
			prob -= i * insgem_vars.minus_for_affect;
			break;
		}

	for (i = 0; weapon_affect[i].aff_bitvector != -1; i++)
	{
		if (IS_SET(GET_OBJ_AFF(itemobj, weapon_affect[i].aff_pos), weapon_affect[i].aff_pos))
		{
			prob -= insgem_vars.minus_for_affect;
		}
	}

//Polos.insert_wanted_gem

	argument = one_argument(argument, arg3);

	if (!*arg3)
	{
//-Polos.insert_wanted_gem
		improove_skill(ch, SKILL_INSERTGEM, 0, 0);

		if (percent > prob / insgem_vars.prob_divide)
		{
			sprintf(buf, "Вы неудачно попытались вплавить %s в %s, испортив камень...\r\n", gemobj->short_description,
					itemobj->PNames[3]);
			send_to_char(buf, ch);
			sprintf(buf, "$n испортил$g %s, вплавляя его в %s!\r\n", gemobj->PNames[3], itemobj->PNames[3]);
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
			extract_obj(gemobj);
			if (number(1, 100) <= insgem_vars.dikey_percent)
			{
				sprintf(buf, "...и испортив хорошую вещь!\r\n");
				send_to_char(buf, ch);
				sprintf(buf, "$n испортил$g %s!\r\n", itemobj->PNames[3]);
				act(buf, FALSE, ch, 0, 0, TO_ROOM);
				extract_obj(itemobj);
			}
			return;
		}
//Polos.insert_wanted_gem
	}
	else
	{
		if (ch->get_skill(SKILL_INSERTGEM) < 80)
		{
			sprintf(buf, "Вы должны достигнуть мастерства в умении ювелир, чтобы вплавлять желаемые аффекты!\r\n");
			send_to_char(buf, ch);
			return;

		}
		if (GET_OBJ_OWNER(itemobj) != GET_UNIQUE(ch))
		{
			sprintf(buf, "Вы можете вплавлять желаемые аффекты только в перековку!\r\n");
			send_to_char(buf, ch);
			return;
		}

		string str(arg3);
		if (!iwg.exist(GET_OBJ_VNUM(gemobj), str))
		{
			iwg.show(ch, GET_OBJ_VNUM(gemobj));
			return;
		}

		improove_skill(ch, SKILL_INSERTGEM, 0, 0);

		//успех или фэйл? при 80% скила успех 30% при 100% скила 50% при 200% скила успех 75%
		if (number(1, ch->get_skill(SKILL_INSERTGEM)) > (ch->get_skill(SKILL_INSERTGEM) - 50))
		{
			sprintf(buf, "Вы неудачно попытались вплавить %s в %s, испортив камень...\r\n", gemobj->short_description,
					itemobj->PNames[3]);
			send_to_char(buf, ch);
			sprintf(buf, "$n испортил$g %s, вплавляя его в %s!\r\n", gemobj->PNames[3], itemobj->PNames[3]);
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
			extract_obj(gemobj);
			return;
		}

	}
//-Polos.insert_wanted_gem

	sprintf(buf, "Вы вплавили %s в %s!\r\n", gemobj->PNames[3], itemobj->PNames[3]);
	send_to_char(buf, ch);
	sprintf(buf, "$n вплавил$g %s в %s.\r\n", gemobj->PNames[3], itemobj->PNames[3]);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);

	if (GET_OBJ_OWNER(itemobj) == GET_UNIQUE(ch))
	{
		int timer = itemobj->get_timer() + itemobj->get_timer() / 100 * insgem_vars.timer_plus_percent;
		itemobj->set_timer(timer);
	}
	else
	{
		int timer = itemobj->get_timer() - itemobj->get_timer() / 100 * insgem_vars.timer_minus_percent;
		itemobj->set_timer(timer);
	}
	if (check_unlimited_timer(itemobj))
	{
		itemobj->set_timer(UTIMER);
	}
	if (GET_OBJ_MATER(gemobj) == 18)
	{
//Polos.insert_wanted_gem
		if (!*arg3)
		{
//-Polos.insert_wanted_gem
			if (GET_OBJ_VNUM(gemobj) == dig_vars.stone1_vnum)
				switch (number(1, 6))
				{
				case 1:
					set_obj_aff(itemobj, AFF_DETECT_INVIS);
					break;
				case 2:
					set_obj_aff(itemobj, AFF_DETECT_MAGIC);
					break;
				case 3:
					set_obj_aff(itemobj, AFF_DETECT_ALIGN);
					break;
				case 4:
					set_obj_aff(itemobj, AFF_BLESS);
					break;
				case 5:
					set_obj_aff(itemobj, AFF_HASTE);
					break;

				case 6:
					set_obj_eff(itemobj, APPLY_HITROLL, -1);
					break;

				}
			if (GET_OBJ_VNUM(gemobj) == dig_vars.stone1_vnum + 1)
				switch (number(1, 6))
				{
				case 1:
					set_obj_eff(itemobj, APPLY_HITROLL, 1);
					break;
				case 2:
					set_obj_eff(itemobj, APPLY_AC, -20);
					set_obj_eff(itemobj, APPLY_SAVING_REFLEX, -5);
					set_obj_eff(itemobj, APPLY_SAVING_STABILITY, -5);
					break;
				case 3:
					set_obj_aff(itemobj, AFF_WATERWALK);
					break;
				case 4:
					set_obj_aff(itemobj, AFF_SINGLELIGHT);
					break;
				case 5:
					set_obj_aff(itemobj, AFF_INFRAVISION);
					break;
				case 6:
					set_obj_aff(itemobj, AFF_CURSE);
					break;

				}
			if (GET_OBJ_VNUM(gemobj) == dig_vars.stone1_vnum + 2)
				switch (number(1, 6))
				{
				case 1:
					set_obj_eff(itemobj, APPLY_HIT, 20);
					break;
				case 2:
					set_obj_eff(itemobj, APPLY_MOVE, 20);
					break;
				case 3:
					set_obj_aff(itemobj, AFF_PROTECT_EVIL);
					break;
				case 4:
					set_obj_aff(itemobj, AFF_PROTECT_GOOD);
					break;
				case 5:
					set_obj_aff(itemobj, AFF_AWARNESS);
					break;
				case 6:
					set_obj_eff(itemobj, APPLY_MOVEREG, -5);
					break;

				}
			if (GET_OBJ_VNUM(gemobj) == dig_vars.stone1_vnum + 3)
				switch (number(1, 6))
				{
				case 1:
					set_obj_eff(itemobj, APPLY_SAVING_REFLEX, -10);
					break;
				case 2:
					set_obj_eff(itemobj, APPLY_HITREG, 10);
					break;
				case 3:
					set_obj_aff(itemobj, AFF_HOLYDARK);
					break;
				case 4:
					if (!CAN_WEAR(itemobj, ITEM_WEAR_WIELD) &&
							!CAN_WEAR(itemobj, ITEM_WEAR_HOLD) && !CAN_WEAR(itemobj, ITEM_WEAR_BOTHS))
						set_obj_eff(itemobj, APPLY_SIZE, 7);
					else
						set_obj_eff(itemobj, APPLY_MORALE, 3);
					break;

				case 5:
					set_obj_eff(itemobj, APPLY_MORALE, 3);
					break;

				case 6:
					set_obj_eff(itemobj, APPLY_HITREG, -5);
					break;
				}
			if (GET_OBJ_VNUM(gemobj) == dig_vars.stone1_vnum + 4)
				switch (number(1, 6))
				{
				case 1:
					set_obj_eff(itemobj, APPLY_SAVING_STABILITY, -10);
					break;
				case 2:
					set_obj_eff(itemobj, APPLY_SAVING_CRITICAL, -10);
					break;
				case 3:
					set_obj_eff(itemobj, APPLY_RESIST_AIR, 15);
					break;
				case 4:
					set_obj_eff(itemobj, APPLY_RESIST_WATER, 15);
					break;
				case 5:
					set_obj_eff(itemobj, APPLY_RESIST_FIRE, 15);
					break;
				case 6:
					set_obj_eff(itemobj, APPLY_SAVING_CRITICAL, 10);
					break;
				}
			if (GET_OBJ_VNUM(gemobj) == dig_vars.stone1_vnum + 5)
				switch (number(1, 6))
				{
				case 1:
					set_obj_aff(itemobj, AFF_SANCTUARY);
					break;
				case 2:
					set_obj_aff(itemobj, AFF_BLINK);
					break;
				case 3:
					if (!CAN_WEAR(itemobj, ITEM_WEAR_WIELD) &&
							!CAN_WEAR(itemobj, ITEM_WEAR_HOLD) && !CAN_WEAR(itemobj, ITEM_WEAR_BOTHS))
						set_obj_eff(itemobj, APPLY_ABSORBE, 5);
					else
						set_obj_eff(itemobj, APPLY_HITREG, 25);
					break;
				case 4:
					set_obj_aff(itemobj, AFF_FLY);
					break;
				case 5:
					set_obj_eff(itemobj, APPLY_MANAREG, 10);
					break;
				case 6:
					set_obj_eff(itemobj, APPLY_SAVING_STABILITY, -10);
					break;
				}
			if (GET_OBJ_VNUM(gemobj) == dig_vars.stone1_vnum + 6)
				switch (number(1, 6))
				{
				case 1:
					set_obj_eff(itemobj, APPLY_DAMROLL, 3);
					break;
				case 2:
					set_obj_eff(itemobj, APPLY_HITROLL, 3);
					break;
				case 3:
					if ((CAN_WEAR(itemobj, ITEM_WEAR_WIELD) ||
							CAN_WEAR(itemobj, ITEM_WEAR_HOLD) || CAN_WEAR(itemobj, ITEM_WEAR_BOTHS))
							&& !OBJ_FLAGGED(itemobj, ITEM_NODISARM))
						SET_BIT(GET_OBJ_EXTRA(itemobj, ITEM_NODISARM), ITEM_NODISARM);
					else
						set_obj_eff(itemobj, APPLY_HITROLL, 3);
					break;
				case 4:
					set_obj_eff(itemobj, APPLY_SAVING_WILL, -10);
					break;
				case 5:
					set_obj_aff(itemobj, AFF_INVISIBLE);
					break;
				case 6:
					set_obj_eff(itemobj, APPLY_SAVING_WILL, 10);
					break;
				}
			if (GET_OBJ_VNUM(gemobj) == dig_vars.stone1_vnum + 7)
				switch (number(1, 6))
				{
				case 1:
					set_obj_eff(itemobj, APPLY_INT, 1);
					break;
				case 2:
					set_obj_eff(itemobj, APPLY_WIS, 1);
					break;
				case 3:
					set_obj_eff(itemobj, APPLY_DEX, 1);
					break;
				case 4:
					set_obj_eff(itemobj, APPLY_STR, 1);
					break;
				case 5:
					set_obj_eff(itemobj, APPLY_CON, 1);
					break;
				case 6:
					set_obj_eff(itemobj, APPLY_CHA, 1);
					break;
				}
			if (GET_OBJ_VNUM(gemobj) == dig_vars.stone1_vnum + 8)
				switch (number(1, 6))
				{
				case 1:
					set_obj_eff(itemobj, APPLY_INT, 2);
					break;
				case 2:
					set_obj_eff(itemobj, APPLY_WIS, 2);
					break;
				case 3:
					set_obj_eff(itemobj, APPLY_DEX, 2);
					break;
				case 4:
					set_obj_eff(itemobj, APPLY_STR, 2);
					break;
				case 5:
					set_obj_eff(itemobj, APPLY_CON, 2);
					break;
				case 6:
					set_obj_eff(itemobj, APPLY_CHA, 2);
					break;
				}
//Polos.insert_wanted_gem
		}
		else
		{
			int tmp_type, tmp_bit, tmp_qty;
			string str(arg3);

			tmp_type = iwg.get_type(GET_OBJ_VNUM(gemobj), str);

			tmp_bit = iwg.get_bit(GET_OBJ_VNUM(gemobj), str);
			tmp_qty = iwg.get_qty(GET_OBJ_VNUM(gemobj), str);
			switch (tmp_type)
			{
			case 1:
				set_obj_eff(itemobj, tmp_bit, tmp_qty);
				break;

			case 2:
				set_obj_aff(itemobj, tmp_bit);
				break;

			case 3:
				SET_BIT(GET_OBJ_EXTRA(itemobj, tmp_bit), tmp_bit);
				break;

			default:
				break;

			};
		}
//-Polos.insert_wanted_gem

		// флаги, определяющие, сколько остается свободных слотов
		if (OBJ_FLAGGED(itemobj, ITEM_WITH3SLOTS))
		{
			REMOVE_BIT(GET_OBJ_EXTRA(itemobj, ITEM_WITH3SLOTS), ITEM_WITH3SLOTS);
			SET_BIT(GET_OBJ_EXTRA(itemobj, ITEM_WITH2SLOTS), ITEM_WITH2SLOTS);
		}
		else if (OBJ_FLAGGED(itemobj, ITEM_WITH2SLOTS))
		{
			REMOVE_BIT(GET_OBJ_EXTRA(itemobj, ITEM_WITH2SLOTS), ITEM_WITH2SLOTS);
			SET_BIT(GET_OBJ_EXTRA(itemobj, ITEM_WITH1SLOT), ITEM_WITH1SLOT);
		}
		else if (OBJ_FLAGGED(itemobj, ITEM_WITH1SLOT))
		{
			REMOVE_BIT(GET_OBJ_EXTRA(itemobj, ITEM_WITH1SLOT), ITEM_WITH1SLOT);
		}
	}
	extract_obj(gemobj);
}

ACMD(do_bandage)
{
	if (IS_NPC(ch))
	{
		return;
	}
	if (GET_HIT(ch) == GET_REAL_MAX_HIT(ch))
	{
		send_to_char("Вы не нуждаетесь в перевязке!\r\n", ch);
		return;
	}
	if (ch->get_fighting())
	{
		send_to_char("Вы не можете перевязывать раны во время боя!\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, AFF_BANDAGE))
	{
		send_to_char("Вы и так уже занимаетесь перевязкой!\r\n", ch);
		return;
	}
	if (AFF_FLAGGED(ch, AFF_NO_BANDAGE))
	{
		send_to_char("Вы не можете перевязывать свои раны чаще раза в минуту!\r\n", ch);
		return;
	}

	OBJ_DATA *bandage = 0;
	for (OBJ_DATA *i = ch->carrying; i ; i = i->next_content)
	{
		if (GET_OBJ_TYPE(i) == ITEM_BANDAGE)
		{
			bandage = i;
			break;
		}
	}
	if (!bandage || GET_OBJ_WEIGHT(bandage) <= 0)
	{
		send_to_char("В вашем инвентаре нет подходящих для перевязки бинтов!\r\n", ch);
		return;
	}

	send_to_char("Вы достали бинты и начали оказывать себе первую помощь...\r\n", ch);
	act("$n начал$g перевязывать свои раны.&n", TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

	AFFECT_DATA af;
	af.type = SPELL_BANDAGE;
	af.location = APPLY_NONE;
	af.modifier = GET_OBJ_VAL(bandage, 0);
	af.duration = pc_duration(ch, 10, 0, 0, 0, 0);
	af.bitvector = AFF_BANDAGE;
	af.battleflag = AF_PULSEDEC;
	affect_join(ch, &af, 0, 0, 0, 0);

	af.type = SPELL_NO_BANDAGE;
	af.location = APPLY_NONE;
	af.duration = pc_duration(ch, 60, 0, 0, 0, 0);
	af.bitvector = AFF_NO_BANDAGE;
	af.battleflag = AF_PULSEDEC;
	affect_join(ch, &af, 0, 0, 0, 0);

	GET_OBJ_WEIGHT(bandage) -= 1;
	IS_CARRYING_W(ch) -= 1;
	if (GET_OBJ_WEIGHT(bandage) <= 0)
	{
		send_to_char("Очередная пачка бинтов подошла к концу.\r\n", ch);
		extract_obj(bandage);
	}
}

bool is_dark(room_rnum room)
{
	double coef = 0.0;
	CHAR_DATA *tmp_ch;
	// если на комнате висит флаг всегда светло, то добавляем
	// +2 к коэф
	if (ROOM_AFFECTED(room, AFF_ROOM_LIGHT))
		coef += 2.0;
	// если светит луна и комната !помещение и !город
	if ((SECT(room) != SECT_INSIDE) && (SECT(room) != SECT_CITY) && (IS_MOONLIGHT(room)))
		coef += 1.0;
		
	// если ночь и мы не внутри и не в городе
	if ((SECT(room) != SECT_INSIDE) && (SECT(room) != SECT_CITY) && ((weather_info.sunlight == SUN_SET) || (weather_info.sunlight == SUN_DARK)))
		coef -= 1.0;
	// если на комнате флаг темно
	if (ROOM_FLAGGED(room, ROOM_DARK))
		coef -= 1.0;
	
	// проверка на костер
	if (world[room]->fires)
		coef += 1.0;	
	
	// проходим по всем чарам и смотрим на них аффекты тьма/свет/освещение

	for (tmp_ch = world[room]->people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
	{
		// если на чаре есть освещение, например, шарик или лампа
		if (is_wear_light(tmp_ch))
			coef += 1.0;
		// если на чаре аффект свет
		if (AFF_FLAGGED(tmp_ch, AFF_SINGLELIGHT))
			coef += 3.0;
		// освещение ?
		if (AFF_FLAGGED(tmp_ch, AFF_HOLYLIGHT))
			coef += 1.0;
		// Санка. Логично, что если чар светится ярким сиянием, то это сияние распространяется на комнату
		if (AFF_FLAGGED(tmp_ch, AFF_SANCTUARY))
			coef += 1.0;
		// Тьма. Сразу фигачим коэф 6. 
		if (AFF_FLAGGED(tmp_ch, AFF_HOLYDARK))
			coef -= 6.0;
	}
	
	if (coef < 0)
		return true;
	return false;
		
}

