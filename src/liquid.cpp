/*************************************************************************
*   File: liquid.cpp                                   Part of Bylins    *
*   Все по жидкостям                                                     *
*                                                                        *
*  $Author$                                                      *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */

#include "liquid.hpp"
#include "char.hpp"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "skills.h"
#include "features.hpp"

extern void weight_change_object(OBJ_DATA * obj, int weight);

// виды жидскостей, наливаемых в контейнеры
const int LIQ_WATER = 0;
const int LIQ_BEER = 1;
const int LIQ_WINE = 2;
const int LIQ_ALE = 3;
const int LIQ_QUAS = 4;
const int LIQ_BRANDY = 5;
const int LIQ_MORSE = 6;
const int LIQ_VODKA = 7;
const int LIQ_BRAGA = 8;
const int LIQ_MED = 9;
const int LIQ_MILK = 10;
const int LIQ_TEA = 11;
const int LIQ_COFFE = 12;
const int LIQ_BLOOD = 13;
const int LIQ_SALTWATER = 14;
const int LIQ_CLEARWATER = 15;
const int LIQ_POTION = 16;
const int LIQ_POTION_RED = 17;
const int LIQ_POTION_BLUE = 18;
const int LIQ_POTION_WHITE = 19;
const int LIQ_POTION_GOLD = 20;
const int LIQ_POTION_BLACK = 21;
const int LIQ_POTION_GREY = 22;
const int LIQ_POTION_FUCHSIA = 23;
const int LIQ_POISON_ACONITUM = 24;
// терминатор
const int NUM_LIQ_TYPES = 25;

/* LIQ_x */
const char *drinks[] = { "воды",
						 "пива",
						 "вина",
						 "медовухи",
						 "кваса",
						 "самогона",
						 "морсу",
						 "водки",
						 "браги",
						 "меду",
						 "молока",
						 "чаю",
						 "кофею",
						 "КРОВИ",
						 "соленой воды",
						 "родниковой воды",
						 "колдовского зелья",
						 "красного колдовского зелья",
						 "синего колдовского зелья",
						 "белого колдовского зелья",
						 "золотистого колдовского зелья",
						 "черного колдовского зелья",
						 "серого колдовского зелья",
						 "фиолетового колдовского зелья",
						 "настойки аконита",
						 "\n"
					   };

/* one-word alias for each drink */
const char *drinknames[] = { "водой",
							 "пивом",
							 "вином",
							 "медовухой",
							 "квасом",
							 "самогоном",
							 "морсом",
							 "водкой",
							 "брагой",
							 "медом",
							 "молоком",
							 "чаем",
							 "кофе",
							 "КРОВЬЮ",
							 "соленой водой",
							 "родниковой водой",
							 "колдовским зельем",
							 "красным колдовским зельем",
							 "синим колдовским зельем",
							 "белым колдовским зельем",
							 "золотистым колдовским зельем",
							 "черным колдовским зельем",
							 "серым колдовским зельем",
							 "фиолетовым колдовским зельем",
							 "настойкой аконита",
							 "\n"
						   };

/* effect of drinks on DRUNK, FULL, THIRST -- see values.doc */
const int drink_aff[][3] = { {0, 1, 10},	// вода
	{2, 2, 5},			// пиво
	{5, 2, 5},			// вино
	{2, 2, 5},			// медовуха
	{1, 2, 5},			// квас
	{6, 1, 4},			// самогон
	{0, 1, 8},			// морс
	{10, 0, 0},			// водка
	{3, 3, 3},			// брага
	{0, 4, -8},			// мед
	{0, 3, 6},			// молоко
	{0, 1, 6},			// чай
	{0, 1, 6},			// кофе
	{0, 2, -1},			// кровь
	{0, 1, -2},			// соленая вода
	{0, 0, 13},			// родниковая вода
	{0, 1, -1},			// магическое зелье
	{0, 1, -1},			// красное магическое зелье
	{0, 1, -1},			// синее магическое зелье
	{0, 1, -1},			// белое магическое зелье
	{0, 1, -1},			// золотистоемагическое зелье
	{0, 1, -1},			// черноемагическое зелье
	{0, 1, -1},			// серое магическое зелье
	{0, 1, -1},			// фиолетовое магическое зелье
	{0, 0, 0}			// настойка аконита
};

/* color of the various drinks */
const char *color_liquid[] = { "прозрачной",
							   "коричневой",
							   "бордовой",
							   "золотистой",
							   "бурой",
							   "прозрачной",
							   "рубиновой",
							   "зеленой",
							   "чистой",
							   "светло зеленой",
							   "белой",
							   "коричневой",
							   "темно-коричневой",
							   "красной",
							   "прозрачной",
							   "кристально чистой",
							   "искрящейся",
							   "бордовой",
							   "лазурной",
							   "серебристой",
							   "золотистой",
							   "черной вязкой",
							   "бесцветной",
							   "фиолетовой",
							   "ядовитой",
							   "\n"
							 };

/**
* Зелья, перелитые в контейнеры, можно пить во время боя.
* На случай, когда придется добавлять еще пошенов, которые
* уже будут идти не подряд по номерам.
*/
bool is_potion(OBJ_DATA *obj)
{
	switch(GET_OBJ_VAL(obj, 2))
	{
	case LIQ_POTION:
	case LIQ_POTION_RED:
	case LIQ_POTION_BLUE:
	case LIQ_POTION_WHITE:
	case LIQ_POTION_GOLD:
	case LIQ_POTION_BLACK:
	case LIQ_POTION_GREY:
	case LIQ_POTION_FUCHSIA:
		return true;
		break;
	}
	return false;
}

ACMD(do_drink)
{
	OBJ_DATA *temp;
	AFFECT_DATA af;
	OBJ_DATA *obj_potion;
	int amount, weight, duration, i, level;
	int on_ground = 0;

	one_argument(argument, arg);

	if (IS_NPC(ch))		/* Cannot use GET_COND() on mobs. */
		return;

	if (!*arg)
	{
		send_to_char("Пить из чего ?\r\n", ch);
		return;
	}

	if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		if (!(temp = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents)))
		{
			send_to_char("Вы не смогли это найти!\r\n", ch);
			return;
		}
		else
			on_ground = 1;
	}
	if (IS_SET(PRF_FLAGS(ch, PRF_IRON_WIND), PRF_IRON_WIND))
	{
		send_to_char("Не стоит отвлекаться в бою!\r\n", ch);
		return;
	}
	if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) && (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN))
	{
		send_to_char("Не стоит. Козлят и так много !\r\n", ch);
		return;
	}
	if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON))
	{
		send_to_char("Прежде это стоит поднять.\r\n", ch);
		return;
	}

	if (!GET_OBJ_VAL(temp, 1))
	{
		send_to_char("Пусто.\r\n", ch);
		return;
	}
	// Added by Adept - обкаст если в фонтане или емкости зелье
	if (is_potion(temp))
	{
		act("$n выпил$g зелья из $o1.", TRUE, ch, temp, 0, TO_ROOM);
		sprintf(buf, "Вы выпили зелья из %s.\r\n", OBJN(temp, ch, 1));
		send_to_char(buf, ch);
		obj_potion = read_object(GET_OBJ_SKILL(temp), VIRTUAL);
		if (obj_potion == NULL)
		{
			sprintf(buf,
					"ERROR: Попытка зачитывания заклинания из несуществующего зелья в предмете (VNUM:%d).",
					GET_OBJ_VNUM(temp));
			mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
			return;
		}
		if (GET_OBJ_TYPE(obj_potion) != ITEM_POTION)
		{
			sprintf(buf, "ERROR: Неверный vnum зелья в объекте (VNUM:%d).", GET_OBJ_VNUM(temp));
			mudlog(buf, NRM, LVL_BUILDER, ERRLOG, TRUE);
		}
		else
		{
			//Если внум верный и такое зелье есть - кастим из него спеллы и уничтожаем
			WAIT_STATE(ch, PULSE_VIOLENCE);
			level = GET_OBJ_VAL(obj_potion, 0);
			for (i = 1; i <= 3; i++)
				if (call_magic(ch, ch, NULL, world[IN_ROOM(ch)], GET_OBJ_VAL(obj_potion, i), level, CAST_POTION) <= 0)
					break;
			//Если все зелье выпито - обнуляем внум зелья-прототипа
			if (--GET_OBJ_VAL(temp, 1) <= 0 && GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)
			{
				name_from_drinkcon(temp);
				GET_OBJ_SKILL(temp) = 0;
			}
			GET_OBJ_WEIGHT(temp)--;
		}
		extract_obj(obj_potion);
		return;
	}
	else if (FIGHTING(ch))
	{
		send_to_char("Не стоит отвлекаться в бою.\r\n", ch);
		return;
	}
	//Конец изменений Adept'ом

	if ((GET_COND(ch, DRUNK) > CHAR_DRUNKED) && (GET_COND(ch, THIRST) > 0))  	// The pig is drunk
	{
		send_to_char("Вы не смогли сделать и глотка.\r\n", ch);
		act("$n попытал$u выпить еще, но не смог$q сделать и глотка.", TRUE, ch, 0, 0, TO_ROOM);
		return;
	}

	if (subcmd == SCMD_DRINK)
	{
		if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
			amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
		else
			amount = number(3, 10);
	}
	else
	{
		amount = 1;
	}

	amount = MIN(amount, GET_OBJ_VAL(temp, 1));
	amount = MIN(amount, 24 - GET_COND(ch, THIRST));

	if (amount <= 0)
	{
		send_to_char("В Вас больше не лезет.\r\n", ch);
		return;
	}
	else if (subcmd == SCMD_DRINK)
	{
		sprintf(buf, "$n выпил$g %s из $o1.", drinks[GET_OBJ_VAL(temp, 2)]);
		act(buf, TRUE, ch, temp, 0, TO_ROOM);
		sprintf(buf, "Вы выпили %s из %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)], OBJN(temp, ch, 1));
		send_to_char(buf, ch);
	}
	else
	{
		act("$n отхлебнул$g из $o1.", TRUE, ch, temp, 0, TO_ROOM);
		sprintf(buf, "Вы узнали вкус %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
		send_to_char(buf, ch);
	}

	/* You can't subtract more than the object weighs */
	weight = MIN(amount, GET_OBJ_WEIGHT(temp));

	if (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)
		weight_change_object(temp, -weight);	/* Subtract amount */
	gain_condition(ch, DRUNK, (int)((int) drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount) / 4);

	gain_condition(ch, FULL, (int)((int) drink_aff[GET_OBJ_VAL(temp, 2)][FULL] * amount) / 4);

	gain_condition(ch, THIRST, (int)((int) drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount) / 4);

	if (GET_COND(ch, THIRST) > 20)
		send_to_char("Вы не чувствуете жажды.\r\n", ch);

	if (GET_COND(ch, FULL) > 20)
		send_to_char("Вы чувствуете приятную тяжесть в желудке.\r\n", ch);

	if (GET_COND(ch, DRUNK) >= CHAR_DRUNKED)
	{
		if (GET_COND(ch, DRUNK) >= CHAR_MORTALLY_DRUNKED)
		{
			send_to_char("Напилися Вы пьяны, не дойти Вам до дому....\r\n", ch);
			duration = 2;
		}
		else
		{
			send_to_char("Приятное тепло разлилось по Вашему телу.\r\n", ch);
			duration = 2 + MAX(0, GET_COND(ch, DRUNK) - CHAR_DRUNKED);
		}
		GET_DRUNK_STATE(ch) = MAX(GET_DRUNK_STATE(ch), GET_COND(ch, DRUNK));
		if (!AFF_FLAGGED(ch, AFF_DRUNKED) && !AFF_FLAGGED(ch, AFF_ABSTINENT))
		{
			send_to_char("Винные пары ударили Вам в голову.\r\n", ch);
			/***** Decrease AC ******/
			af.type = SPELL_DRUNKED;
			af.duration = pc_duration(ch, duration, 0, 0, 0, 0);
			af.modifier = -20;
			af.location = APPLY_AC;
			af.bitvector = AFF_DRUNKED;
			af.battleflag = 0;
			affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
			/***** Decrease HR ******/
			af.type = SPELL_DRUNKED;
			af.duration = pc_duration(ch, duration, 0, 0, 0, 0);
			af.modifier = -2;
			af.location = APPLY_HITROLL;
			af.bitvector = AFF_DRUNKED;
			af.battleflag = 0;
			affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
			/***** Increase DR ******/
			af.type = SPELL_DRUNKED;
			af.duration = pc_duration(ch, duration, 0, 0, 0, 0);
			af.modifier = (GET_LEVEL(ch) + 4) / 5;
			af.location = APPLY_DAMROLL;
			af.bitvector = AFF_DRUNKED;
			af.battleflag = 0;
			affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
		}
	}

	if (GET_OBJ_VAL(temp, 3) && !IS_GOD(ch))  	/* The shit was poisoned ! */
	{
		send_to_char("Что-то вкус какой-то странный !\r\n", ch);
		act("$n поперхнул$u и закашлял$g.", TRUE, ch, 0, 0, TO_ROOM);

		af.type = SPELL_POISON;
		af.duration = pc_duration(ch, amount == 1 ? amount : amount * 3, 0, 0, 0, 0);
		af.modifier = -2;
		af.location = APPLY_STR;
		af.bitvector = AFF_POISON;
		af.battleflag = AF_SAME_TIME;
		affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
		af.type = SPELL_POISON;
		af.modifier = amount * 3;
		af.location = APPLY_POISON;
		af.bitvector = AFF_POISON;
		af.battleflag = AF_SAME_TIME;
		affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
		ch->Poisoner = 0;
	}

	/* empty the container, and no longer poison. 999 - whole fountain */
	if (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN || GET_OBJ_VAL(temp, 1) != 999)
		GET_OBJ_VAL(temp, 1) -= amount;
	if (!GET_OBJ_VAL(temp, 1))  	/* The last bit */
	{
		GET_OBJ_VAL(temp, 2) = 0;
		GET_OBJ_VAL(temp, 3) = 0;
		name_from_drinkcon(temp);
	}

	return;
}

ACMD(do_drunkoff)
{
	OBJ_DATA *obj;
	AFFECT_DATA af[3];
	struct timed_type timed;
	int amount, weight, prob, percent, duration;
	int on_ground = 0;

	if (IS_NPC(ch))		/* Cannot use GET_COND() on mobs. */
		return;

	if (FIGHTING(ch))
	{
		send_to_char("Не стоит отвлекаться в бою.\r\n", ch);
		return;
	}

	if (AFF_FLAGGED(ch, AFF_DRUNKED))
	{
		send_to_char("Вы хотите испортить себе весь кураж ?\r\n" "Это не есть по русски !\r\n", ch);
		return;
	}

	if (!AFF_FLAGGED(ch, AFF_ABSTINENT) && GET_COND(ch, DRUNK) < CHAR_DRUNKED)
	{
		send_to_char("Не стоит делать этого на трезвую голову.\r\n", ch);
		return;
	}

	if (timed_by_skill(ch, SKILL_DRUNKOFF))
	{
		send_to_char("Вы не в состоянии так часто похмеляться.\r\n"
					 "Попросите Богов закодировать Вас.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if (!*arg)
	{
		for (obj = ch->carrying; obj; obj = obj->next_content)
			if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
				break;
		if (!obj)
		{
			send_to_char("У Вас нет подходящего напитка для похмелья.\r\n", ch);
			return;
		}
	}
	else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
	{
		if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room]->contents)))
		{
			send_to_char("Вы не смогли это найти!\r\n", ch);
			return;
		}
		else
			on_ground = 1;
	}

	if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) && (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
	{
		send_to_char("Этим Вы вряд-ли сможете похмелиться.\r\n", ch);
		return;
	}

	if (on_ground && (GET_OBJ_TYPE(obj) == ITEM_DRINKCON))
	{
		send_to_char("Прежде это стоит поднять.\r\n", ch);
		return;
	}

	if (!GET_OBJ_VAL(obj, 1))
	{
		send_to_char("Пусто.\r\n", ch);
		return;
	}

	switch (GET_OBJ_VAL(obj, 2))
	{
	case LIQ_BEER:
	case LIQ_WINE:
	case LIQ_ALE:
	case LIQ_QUAS:
	case LIQ_BRANDY:
	case LIQ_VODKA:
	case LIQ_BRAGA:
		break;
	default:
		send_to_char("Вспомните народную мудрость :\r\n" "\"Клин вышибают клином...\"\r\n", ch);
		return;
	}

	timed.skill = SKILL_DRUNKOFF;
	timed.time = can_use_feat(ch, DRUNKARD_FEAT) ? feature_mod(DRUNKARD_FEAT, FEAT_TIMER) : 12;
	timed_to_char(ch, &timed);

	amount = MAX(1, GET_WEIGHT(ch) / 50);
	percent = number(1, skill_info[SKILL_DRUNKOFF].max_percent);
	if (amount > GET_OBJ_VAL(obj, 1))
		percent += 50;
	prob = train_skill(ch, SKILL_DRUNKOFF, skill_info[SKILL_DRUNKOFF].max_percent, 0);
	amount = MIN(amount, GET_OBJ_VAL(obj, 1));
	weight = MIN(amount, GET_OBJ_WEIGHT(obj));
	weight_change_object(obj, -weight);	/* Subtract amount */
	GET_OBJ_VAL(obj, 1) -= amount;
	if (!GET_OBJ_VAL(obj, 1))  	/* The last bit */
	{
		GET_OBJ_VAL(obj, 2) = 0;
		GET_OBJ_VAL(obj, 3) = 0;
		name_from_drinkcon(obj);
	}

	if (percent > prob)
	{
		sprintf(buf,
				"Вы отхлебнули %s из $o1, но Ваша голова стала еще тяжелее...", drinks[GET_OBJ_VAL(obj, 2)]);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		act("$n попробовал$g похмелиться, но это не пошло $m на пользу.", FALSE, ch, 0, 0, TO_ROOM);
		duration = MAX(1, amount / 3);
		af[0].type = SPELL_ABSTINENT;
		af[0].duration = pc_duration(ch, duration, 0, 0, 0, 0);
		af[0].modifier = 0;
		af[0].location = APPLY_DAMROLL;
		af[0].bitvector = AFF_ABSTINENT;
		af[0].battleflag = 0;
		af[1].type = SPELL_ABSTINENT;
		af[1].duration = pc_duration(ch, duration, 0, 0, 0, 0);
		af[1].modifier = 0;
		af[1].location = APPLY_HITROLL;
		af[1].bitvector = AFF_ABSTINENT;
		af[1].battleflag = 0;
		af[2].type = SPELL_ABSTINENT;
		af[2].duration = pc_duration(ch, duration, 0, 0, 0, 0);
		af[2].modifier = 0;
		af[2].location = APPLY_AC;
		af[2].bitvector = AFF_ABSTINENT;
		af[2].battleflag = 0;
		switch (number(0, ch->get_skill(SKILL_DRUNKOFF) / 20))
		{
		case 0:
		case 1:
			af[0].modifier = -2;
		case 2:
		case 3:
			af[1].modifier = -2;
		default:
			af[2].modifier = 10;
		}
		for (prob = 0; prob < 3; prob++)
			affect_join(ch, &af[prob], TRUE, FALSE, TRUE, FALSE);
		gain_condition(ch, DRUNK, amount);
	}
	else
	{
		sprintf(buf,
				"Вы отхлебнули %s из $o1 и почувствовали приятную легкость во всем теле...",
				drinks[GET_OBJ_VAL(obj, 2)]);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		act("$n похмелил$u и расцвел$g прям на глазах.", FALSE, ch, 0, 0, TO_ROOM);
		affect_from_char(ch, SPELL_ABSTINENT);
	}

	return;
}

ACMD(do_pour)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	OBJ_DATA *from_obj = NULL, *to_obj = NULL;
	int amount;

	two_arguments(argument, arg1, arg2);

	if (subcmd == SCMD_POUR)
	{
		if (!*arg1)  	/* No arguments */
		{
			send_to_char("Откуда переливаем ?\r\n", ch);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
		{
			send_to_char("У Вас нет этого !\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON && GET_OBJ_TYPE(from_obj) != ITEM_POTION)
		{
			send_to_char("Вы не можете из этого переливать !\r\n", ch);
			return;
		}
	}
	if (subcmd == SCMD_FILL)
	{
		if (!*arg1)  	/* no arguments */
		{
			send_to_char("Что и из чего Вы хотели бы наполнить ?\r\n", ch);
			return;
		}
		if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
		{
			send_to_char("У Вас этого нет !\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON)
		{
			act("Вы не можете наполнить $o3!", FALSE, ch, to_obj, 0, TO_CHAR);
			return;
		}
		if (!*arg2)  	/* no 2nd argument */
		{
			act("Из чего Вы планируете наполнить $o3?", FALSE, ch, to_obj, 0, TO_CHAR);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis(ch, arg2, world[ch->in_room]->contents)))
		{
			sprintf(buf, "Вы не видите здесь '%s'.\r\n", arg2);
			send_to_char(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN)
		{
			act("Вы не сможете ничего наполнить из $o1.", FALSE, ch, from_obj, 0, TO_CHAR);
			return;
		}
	}
	if (GET_OBJ_VAL(from_obj, 1) == 0)
	{
		act("Пусто.", FALSE, ch, from_obj, 0, TO_CHAR);
		return;
	}
	if (subcmd == SCMD_POUR)  	/* pour */
	{
		if (!*arg2)
		{
			send_to_char("Куда Вы хотите лить ?  На землю или во что-то ?\r\n", ch);
			return;
		}
		if (!str_cmp(arg2, "out") || !str_cmp(arg2, "земля"))
		{
			act("$n опустошил$g $o3.", TRUE, ch, from_obj, 0, TO_ROOM);
			act("Вы опустошили $o3.", FALSE, ch, from_obj, 0, TO_CHAR);

			weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1));	/* Empty */

			GET_OBJ_VAL(from_obj, 1) = 0;
			GET_OBJ_VAL(from_obj, 2) = 0;
			GET_OBJ_VAL(from_obj, 3) = 0;
			GET_OBJ_SKILL(from_obj) = 0;
			name_from_drinkcon(from_obj);

			return;
		}
		if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying)))
		{
			send_to_char("Вы не можете этого найти !\r\n", ch);
			return;
		}
		if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) && (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN))
		{
			send_to_char("Вы не сможете в это налить.\r\n", ch);
			return;
		}
	}
	if (to_obj == from_obj)
	{
		send_to_char("Более тупого действа Вы придумать, конечно, не могли.\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(to_obj, 1) != 0 &&
			GET_OBJ_TYPE(from_obj) != ITEM_POTION && GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2))
	{
		send_to_char("Вы станете неплохим Химиком, но не в нашей игре.\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(to_obj, 1) >= GET_OBJ_VAL(to_obj, 0))
	{
		send_to_char("Там нет места.\r\n", ch);
		return;
	}
//Added by Adept - переливание зелья из бутылки или емкости в емкость

	//Переливает из бутылки с зельем в емкость
	if (GET_OBJ_TYPE(from_obj) == ITEM_POTION)
	{
		if (GET_OBJ_VNUM(from_obj) == GET_OBJ_SKILL(to_obj) || GET_OBJ_VAL(to_obj, 1) == 0)
		{
			sprintf(buf, "Вы занялись переливанием зелья в %s.\r\n", OBJN(to_obj, ch, 3));
			send_to_char(buf, ch);
			if (GET_OBJ_VAL(to_obj, 1) == 0)
			{
				/* определение названия зелья по содержащемуся заклинанию */
				switch (GET_OBJ_VAL(from_obj, 1))
				{
					/* восстановление (красное) */
				case SPELL_REFRESH:
				case SPELL_GROUP_REFRESH:
					GET_OBJ_VAL(to_obj, 2) = LIQ_POTION_RED;
					name_to_drinkcon(to_obj, LIQ_POTION_RED);
					break;
					/* насыщение (синее) */
				case SPELL_FULL:
					GET_OBJ_VAL(to_obj, 2) = LIQ_POTION_BLUE;
					name_to_drinkcon(to_obj, LIQ_POTION_BLUE);
					break;
					/* детекты (белое) */
				case SPELL_DETECT_INVIS:
				case SPELL_DETECT_MAGIC:
				case SPELL_DETECT_POISON:
				case SPELL_DETECT_ALIGN:
				case SPELL_SENSE_LIFE:
				case SPELL_INFRAVISION:
					GET_OBJ_VAL(to_obj, 2) = LIQ_POTION_WHITE;
					name_to_drinkcon(to_obj, LIQ_POTION_WHITE);
					break;
					/* защитные (золотистое) */
				case SPELL_ARMOR:
				case SPELL_GROUP_ARMOR:
				case SPELL_CLOUDLY:
					GET_OBJ_VAL(to_obj, 2) = LIQ_POTION_GOLD;
					name_to_drinkcon(to_obj, LIQ_POTION_GOLD);
					break;
					/* восстанавливающие здоровье (черное) */
				case SPELL_CURE_CRITIC:
				case SPELL_CURE_LIGHT:
				case SPELL_HEAL:
				case SPELL_GROUP_HEAL:
				case SPELL_CURE_SERIOUS:
					GET_OBJ_VAL(to_obj, 2) = LIQ_POTION_BLACK;
					name_to_drinkcon(to_obj, LIQ_POTION_BLACK);
					break;
					/* снимающее вредные аффекты (серое) */
				case SPELL_CURE_BLIND:
				case SPELL_REMOVE_CURSE:
				case SPELL_REMOVE_HOLD:
				case SPELL_REMOVE_SIELENCE:
				case SPELL_CURE_PLAQUE:
				case SPELL_REMOVE_DEAFNESS:
				case SPELL_REMOVE_POISON:
					GET_OBJ_VAL(to_obj, 2) = LIQ_POTION_GREY;
					name_to_drinkcon(to_obj, LIQ_POTION_GREY);
					break;
					/* прочие полезности (фиолетовое) */
				case SPELL_INVISIBLE:
				case SPELL_GROUP_INVISIBLE:
				case SPELL_STRENGTH:
				case SPELL_GROUP_STRENGTH:
				case SPELL_FLY:
				case SPELL_GROUP_FLY:
				case SPELL_BLESS:
				case SPELL_GROUP_BLESS:
				case SPELL_HASTE:
				case SPELL_GROUP_HASTE:
				case SPELL_STONESKIN:
				case SPELL_BLINK:
				case SPELL_EXTRA_HITS:
				case SPELL_WATERBREATH:
					GET_OBJ_VAL(to_obj, 2) = LIQ_POTION_FUCHSIA;
					name_to_drinkcon(to_obj, LIQ_POTION_FUCHSIA);
					break;
				default:
					GET_OBJ_VAL(to_obj, 2) = LIQ_POTION;
					name_to_drinkcon(to_obj, LIQ_POTION);	/* добавляем новый синоним */
				}
			}
			GET_OBJ_SKILL(to_obj) = GET_OBJ_VNUM(from_obj);	/* помещаем vnum зелья в поле skill */
			weight_change_object(to_obj, 1);
			GET_OBJ_VAL(to_obj, 1)++;
			extract_obj(from_obj);
			return;
		}
		else
		{
			send_to_char("Смешивать разные зелья?! Да вы, батенька, гурман!\r\n", ch);
			return;
		}
	}
	//Переливает из емкости или колодца с зельем куда-то
	if ((GET_OBJ_TYPE(from_obj) == ITEM_DRINKCON ||
			GET_OBJ_TYPE(from_obj) == ITEM_FOUNTAIN) &&
			GET_OBJ_VAL(from_obj, 2) >= LIQ_POTION && GET_OBJ_VAL(from_obj, 2) <= LIQ_POTION_FUCHSIA)
	{
		if ((GET_OBJ_SKILL(from_obj) == GET_OBJ_SKILL(to_obj)) || GET_OBJ_VAL(to_obj, 1) == 0)
			GET_OBJ_SKILL(to_obj) = GET_OBJ_SKILL(from_obj);
		else
		{
			send_to_char("Смешивать разные зелья?! Да вы, батенька, гурман!\r\n", ch);
			return;
		}
	}
//Конец изменений Adept'ом

	if (subcmd == SCMD_POUR)
	{
		sprintf(buf, "Вы занялись переливанием %s в %s.",
				drinks[GET_OBJ_VAL(from_obj, 2)], OBJN(to_obj, ch, 3));
		send_to_char(buf, ch);
	}
	if (subcmd == SCMD_FILL)
	{
		act("Вы наполнили $o3 из $O1.", FALSE, ch, to_obj, from_obj, TO_CHAR);
		act("$n наполнил$g $o3 из $O1.", TRUE, ch, to_obj, from_obj, TO_ROOM);
	}

	/* копируем тип жидкости */
	GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

	/* New alias */
	if (GET_OBJ_VAL(to_obj, 1) == 0)
		name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

	/* Then how much to pour */
	if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN || GET_OBJ_VAL(from_obj, 1) != 999)
		GET_OBJ_VAL(from_obj, 1) -= (amount = (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));
	else
		amount = GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1);

	GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

	/* Then the poison boogie */
	GET_OBJ_VAL(to_obj, 3) = (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));

	if (GET_OBJ_VAL(from_obj, 1) <= 0)  	/* There was too little */
	{
		GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
		amount += GET_OBJ_VAL(from_obj, 1);
		GET_OBJ_VAL(from_obj, 1) = 0;
		GET_OBJ_VAL(from_obj, 2) = 0;
		GET_OBJ_VAL(from_obj, 3) = 0;
		name_from_drinkcon(from_obj);
	}

	/* And the weight boogie */
	if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN)
		weight_change_object(from_obj, -amount);
	weight_change_object(to_obj, amount);	/* Add weight */
}

void name_from_drinkcon(OBJ_DATA * obj)
{
	int i, c, j = 0;
	char new_name[MAX_STRING_LENGTH];

	for (i = 0; *(obj->name + i) && a_isspace(*(obj->name + i)); i++);
	for (j = 0; *(obj->name + i) && !(a_isspace(*(obj->name + i))); new_name[j] = *(obj->name + i), i++, j++);
	new_name[j] = '\0';
	if (*new_name)
	{
		if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)]->name)
			free(obj->name);
		obj->name = str_dup(new_name);
	}

	for (i = 0; *(obj->short_description + i)
			&& a_isspace(*(obj->short_description + i)); i++);
	for (j = 0; *(obj->short_description + i)
			&& !(a_isspace(*(obj->short_description + i))); new_name[j] = *(obj->short_description + i), i++, j++);
	new_name[j] = '\0';
	if (*new_name)
	{
		if (GET_OBJ_RNUM(obj) < 0 || obj->short_description != obj_proto[GET_OBJ_RNUM(obj)]->short_description)
			free(obj->short_description);
		obj->short_description = str_dup(new_name);
	}


	for (c = 0; c < NUM_PADS; c++)
	{
		for (i = 0; a_isspace(*(obj->PNames[c] + i)); i++);
		for (j = 0; !a_isspace(*(obj->PNames[c] + i)); new_name[j] = *(obj->PNames[c] + i), i++, j++);
		new_name[j] = '\0';
		if (*new_name)
		{
			if (GET_OBJ_RNUM(obj) < 0 || obj->PNames[c] != obj_proto[GET_OBJ_RNUM(obj)]->PNames[c])
				free(obj->PNames[c]);
			obj->PNames[c] = str_dup(new_name);
		}
	}
}

void name_to_drinkcon(OBJ_DATA * obj, int type)
{
	int c;
	char new_name[MAX_INPUT_LENGTH];

	sprintf(new_name, "%s %s", obj->name, drinknames[type]);
	if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)]->name)
		free(obj->name);
	obj->name = str_dup(new_name);

	sprintf(new_name, "%s c %s", obj->short_description, drinknames[type]);
	if (GET_OBJ_RNUM(obj) < 0 || obj->short_description != obj_proto[GET_OBJ_RNUM(obj)]->short_description)
		free(obj->short_description);
	obj->short_description = str_dup(new_name);


	for (c = 0; c < NUM_PADS; c++)
	{
		sprintf(new_name, "%s с %s", obj->PNames[c], drinknames[type]);
		if (GET_OBJ_RNUM(obj) < 0 || obj->PNames[c] != obj_proto[GET_OBJ_RNUM(obj)]->PNames[c])
			free(obj->PNames[c]);
		obj->PNames[c] = str_dup(new_name);
	}
}
