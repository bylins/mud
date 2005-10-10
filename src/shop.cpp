/* ************************************************************************
*   File: shop.cpp                                      Part of Bylins    *
*  Usage: shopkeepers: loading config files, spec procs.                  *
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

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/
#define __SHOP_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "shop.h"
#include "dg_scripts.h"
#include "constants.h"

extern TIME_INFO_DATA time_info;

/* Forward/External function declarations */
ACMD(do_tell);
ACMD(do_echo);
ACMD(do_say);
void sort_keeper_objs(CHAR_DATA * keeper, int shop_nr);
int do_social(CHAR_DATA * ch, char *argument);
char *diag_weapon_to_char(OBJ_DATA * obj, int show_wear);
char *diag_obj_to_char(CHAR_DATA * i, OBJ_DATA * obj, int mode);
char *diag_timer_to_char(OBJ_DATA * obj);
int invalid_anti_class(CHAR_DATA * ch, OBJ_DATA * obj);
int invalid_unique(CHAR_DATA * ch, OBJ_DATA * obj);

/* Local variables */
SHOP_DATA *shop_index;
int top_shop = -1;
int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke;

/* local functions */
int read_type_list(FILE * shop_f, struct shop_buy_data *list, int new_format, int max);
int read_list(FILE * shop_f, struct shop_buy_data *list, int new_format, int max, int type);
void shopping_list(char *arg, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr);
void shopping_value(char *arg, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr);
void shopping_sell(char *arg, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr);
OBJ_DATA *get_selling_obj(CHAR_DATA * ch, char *name, CHAR_DATA * keeper, int shop_nr, int msg);
OBJ_DATA *slide_obj(OBJ_DATA * obj, CHAR_DATA * keeper, int shop_nr);
void shopping_buy(char *arg, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr);
OBJ_DATA *get_purchase_obj(CHAR_DATA * ch, char *arg, CHAR_DATA * keeper, int shop_nr, int msg);
OBJ_DATA *get_hash_obj_vis(CHAR_DATA * ch, char *name, OBJ_DATA * list);
OBJ_DATA *get_slide_obj_vis(CHAR_DATA * ch, char *name, OBJ_DATA * list);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
void assign_the_shopkeepers(void);
char *customer_string(int shop_nr, int detailed);
void list_all_shops(CHAR_DATA * ch);
void handle_detailed_list(char *buf, char *buf1, CHAR_DATA * ch);
void list_detailed_shop(CHAR_DATA * ch, int shop_nr);
void show_shops(CHAR_DATA * ch, char *arg);
int is_ok_char(CHAR_DATA * keeper, CHAR_DATA * ch, int shop_nr);
int is_open(CHAR_DATA * keeper, int shop_nr, int msg);
int is_ok(CHAR_DATA * keeper, CHAR_DATA * ch, int shop_nr);
void push(struct stack_data *stack, int pushval);
int top(struct stack_data *stack);
int pop(struct stack_data *stack);
void evaluate_operation(struct stack_data *ops, struct stack_data *vals);
int find_oper_num(char token);
int evaluate_expression(OBJ_DATA * obj, char *expr);
int trade_with(OBJ_DATA * item, int shop_nr, int mode);
int same_obj(OBJ_DATA * obj1, OBJ_DATA * obj2);
int shop_producing(OBJ_DATA * item, int shop_nr);
int transaction_amt(char *arg);
char *times_message(OBJ_DATA * obj, char *name, int num, int padis);
int buy_price(OBJ_DATA * obj, CHAR_DATA * ch, int shop_nr);
int sell_price(OBJ_DATA * obj, CHAR_DATA * ch, int shop_nr);
char *list_object(OBJ_DATA * obj, int cnt, int index, int shop_nr);
int ok_shop_room(int shop_nr, int room);
SPECIAL(shop_keeper);
int ok_damage_shopkeeper(CHAR_DATA * ch, CHAR_DATA * victim);
int add_to_list(struct shop_buy_data *list, int type, int *len, int *val);
int end_read_list(struct shop_buy_data *list, int len, int error);
void read_line(FILE * shop_f, const char *string, void *data);

#define SCMD_SELL   0
#define SCMD_REPAIR 1
#define SCMD_VALUE  2

/* config arrays */
const char *operator_str[] = { "[({",
	"])}",
	"|+",
	"&*",
	"^'"
};

/* Constant list for printing out who we sell to */
const char *trade_letters[] = { "Христиане",	/* First, the alignment based ones */
	"Язычники",
	"Neutral",
	"Маги",			/* Then the class based ones */
	"Клерики",
	"Воры",
	"Воины",
	"Наемники",
	"Дружинники",
	"Витязи",
	"Охотники",
	"Кузнецы",
	"Купцы",
	"Волхвы",
	"\n"
};


const char *shop_bits[] = { "Можно напасть",
	"Хранит деньги в банке",
	"\n"
};

void write_buf(char *buf, char *msg, char *sparam, int dparam, int ending)
{
	char *pos;

	if ((pos = strstr(msg, "%d"))) {
		*pos = '\0';
		sprintf(buf2, "%s%d %s%s", msg, dparam, desc_count(dparam, ending), pos + 2);
		*pos = '%';
	} else
		strcpy(buf2, msg);

	sprintf(buf, buf2, sparam, dparam);
}

int is_ok_char(CHAR_DATA * keeper, CHAR_DATA * ch, int shop_nr)
{
	char buf[200];

	if (!(CAN_SEE(keeper, ch))) {
		do_say(keeper, MSG_NO_SEE_CHAR, cmd_say, 0);
		return (FALSE);
	}
	if (IS_GOD(ch))
		return (TRUE);

	if (IS_NPC(ch))
		return (FALSE);

	if ((GET_RELIGION(ch) == RELIGION_MONO && NOTRADE_MONO(shop_nr)) ||
	    (GET_RELIGION(ch) == RELIGION_POLY && NOTRADE_POLY(shop_nr)) ||
	    (IS_NEUTRAL(ch) && NOTRADE_NEUTRAL(shop_nr))) {
		sprintf(buf, "%s! %s", GET_NAME(ch), MSG_NO_SELL_ALIGN);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}
	if (IS_NPC(ch))
		return (TRUE);

	if ((IS_MAGE(ch) && NOTRADE_MAGIC_USER(shop_nr)) ||
	    (IS_CLERIC(ch) && NOTRADE_CLERIC(shop_nr)) ||
	    (IS_THIEF(ch) && NOTRADE_THIEF(shop_nr)) ||
	    (IS_WARRIOR(ch) && NOTRADE_WARRIOR(shop_nr)) ||
	    (IS_ASSASINE(ch) && NOTRADE_ASSASINE(shop_nr)) ||
	    (IS_GUARD(ch) && NOTRADE_GUARD(shop_nr)) ||
	    (IS_PALADINE(ch) && NOTRADE_PALADINE(shop_nr)) ||
	    (IS_RANGER(ch) && NOTRADE_RANGER(shop_nr)) ||
	    (IS_SMITH(ch) && NOTRADE_SMITH(shop_nr)) ||
	    (IS_MERCHANT(ch) && NOTRADE_MERCHANT(shop_nr)) || (IS_DRUID(ch) && NOTRADE_DRUID(shop_nr))) {
		sprintf(buf, "%s! %s", GET_NAME(ch), MSG_NO_SELL_CLASS);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}
	return (TRUE);
}


int is_open(CHAR_DATA * keeper, int shop_nr, int msg)
{
	char buf[200];

	*buf = 0;
	if (SHOP_OPEN1(shop_nr) > time_info.hours)
		strcpy(buf, MSG_NOT_OPEN_YET);
	else if (SHOP_CLOSE1(shop_nr) < time_info.hours) {
		if (SHOP_OPEN2(shop_nr) > time_info.hours)
			strcpy(buf, MSG_NOT_REOPEN_YET);
		else if (SHOP_CLOSE2(shop_nr) < time_info.hours)
			strcpy(buf, MSG_CLOSED_FOR_DAY);
	}
	if (!(*buf))
		return (TRUE);
	if (msg)
		do_say(keeper, buf, cmd_tell, 0);
	return (FALSE);
}


int is_ok(CHAR_DATA * keeper, CHAR_DATA * ch, int shop_nr)
{
	if (is_open(keeper, shop_nr, TRUE))
		return (is_ok_char(keeper, ch, shop_nr));
	else
		return (FALSE);
}


void push(struct stack_data *stack, int pushval)
{
	S_DATA(stack, S_LEN(stack)++) = pushval;
}


int top(struct stack_data *stack)
{
	if (S_LEN(stack) > 0)
		return (S_DATA(stack, S_LEN(stack) - 1));
	else
		return (NOTHING);
}


int pop(struct stack_data *stack)
{
	if (S_LEN(stack) > 0)
		return (S_DATA(stack, --S_LEN(stack)));
	else {
		log("SYSERR: Illegal expression %d in shop keyword list.", S_LEN(stack));
		return (0);
	}
}


void evaluate_operation(struct stack_data *ops, struct stack_data *vals)
{
	int oper;

	if ((oper = pop(ops)) == OPER_NOT)
		push(vals, !pop(vals));
	else {
		int val1 = pop(vals), val2 = pop(vals);

		/* Compiler would previously short-circuit these. */
		if (oper == OPER_AND)
			push(vals, val1 && val2);
		else if (oper == OPER_OR)
			push(vals, val1 || val2);
	}
}


int find_oper_num(char token)
{
	int index;

	for (index = 0; index <= MAX_OPER; index++)
		if (strchr(operator_str[index], token))
			return (index);
	return (NOTHING);
}


int evaluate_expression(OBJ_DATA * obj, char *expr)
{
	struct stack_data ops, vals;
	char *ptr, *end, name[200];
	int temp, index, value = 0, bank = 0, bit = 0;

	if (!expr || !*expr)	/* Allows opening ( first. */
		return (TRUE);

	ops.len = vals.len = 0;
	ptr = expr;
	while (*ptr) {
		if (a_isspace(*ptr))
			ptr++;
		else {
			if ((temp = find_oper_num(*ptr)) == NOTHING) {
				end = ptr;
				while (*ptr && !a_isspace(*ptr)
				       && (find_oper_num(*ptr) == NOTHING))
					ptr++;
				strncpy(name, end, ptr - end);
				name[ptr - end] = 0;

				for (index = bank = value = 0, bit = 1; bank <= 3; index++)
					if (*extra_bits[index] == '\n') {
						switch (bank) {
						case 0:
							value = INT_ONE;
							break;
						case 1:
							value = INT_TWO;
							break;
						case 2:
							value = INT_THREE;
							break;
						default:
							value = 0;
						}
						if (++bank > 3)
							break;
						bit = 1;
					} else {
						if (!str_cmp(name, extra_bits[index])) {
							push(&vals, IS_SET(GET_OBJ_EXTRA(obj, value), (value | bit)));
							break;
						};
						bit <<= 1;
					}
				if (*extra_bits[index] == '\n')
					push(&vals, isname(name, obj->name));
			} else {
				if (temp != OPER_OPEN_PAREN)
					while (top(&ops) > temp)
						evaluate_operation(&ops, &vals);

				if (temp == OPER_CLOSE_PAREN) {
					if ((temp = pop(&ops)) != OPER_OPEN_PAREN) {
						log("SYSERR: Illegal parenthesis in shop keyword expression.");
						return (FALSE);
					}
				} else
					push(&ops, temp);
				ptr++;
			}
		}
	}
	while (top(&ops) != NOTHING)
		evaluate_operation(&ops, &vals);
	temp = pop(&vals);
	if (top(&vals) != NOTHING) {
		log("SYSERR: Extra operands left on shop keyword expression stack.");
		return (FALSE);
	}
	return (temp);
}

int value_for_mode(int shop_nr, int counter, int mode)
{
	switch (mode) {
	case MODE_CHANGE:
		return SHOP_CHANGETYPE(shop_nr, counter);
	case MODE_REPAIR:
	case MODE_TRADE:
		return SHOP_BUYTYPE(shop_nr, counter);
	}
	return NOTHING;
}

char *word_for_mode(int shop_nr, int counter, int mode)
{
	static char nothing[2] = "\0";
	switch (mode) {
	case MODE_CHANGE:
		return SHOP_CHANGEWORD(shop_nr, counter);
	case MODE_REPAIR:
	case MODE_TRADE:
		return SHOP_BUYWORD(shop_nr, counter);
	}
	return nothing;
}


int trade_with(OBJ_DATA * item, int shop_nr, int mode)
{
	int counter;

	if (GET_OBJ_COST(item) < 1)
		return (OBJECT_NOVAL);

	if (IS_OBJ_STAT(item, ITEM_NOSELL))
		return (OBJECT_NOTOK);

	for (counter = 0; value_for_mode(shop_nr, counter, mode) != NOTHING; counter++)
		if (value_for_mode(shop_nr, counter, mode) == GET_OBJ_TYPE(item)) {	// Used wand or stave
			if (GET_OBJ_VAL(item, 2) == 0 &&
			    (GET_OBJ_TYPE(item) == ITEM_WAND || GET_OBJ_TYPE(item) == ITEM_STAFF))
				return (OBJECT_DEAD);
			else
				// Containers
			if (GET_OBJ_TYPE(item) == ITEM_CONTAINER && item->contains)
				return (OBJECT_NOTEMPTY);
			else
				// Drink containers
				if (GET_OBJ_TYPE(item) == ITEM_DRINKCON &&
				    (GET_OBJ_VAL(item, 3) != 0 ||
				     GET_OBJ_RNUM(item) < 0 ||
				     GET_OBJ_VAL(item, 1) != GET_OBJ_VAL(obj_proto + GET_OBJ_RNUM(item), 1)
				     || GET_OBJ_VAL(item, 2) != GET_OBJ_VAL(obj_proto + GET_OBJ_RNUM(item), 2)))
				return (OBJECT_NOTOK);
			else
				// Ingradients
			if (GET_OBJ_TYPE(item) == ITEM_INGRADIENT &&
				    (GET_OBJ_RNUM(item) < 0 ||
					     GET_OBJ_VAL(item, 2) < GET_OBJ_VAL(obj_proto + GET_OBJ_RNUM(item), 2)))
				return (OBJECT_NOTOK);
			else
				// Special bits
			if (OBJ_FLAGGED(item, ITEM_ARMORED) ||
				    OBJ_FLAGGED(item, ITEM_SHARPEN) ||
				    OBJ_FLAGGED(item, ITEM_NODONATE) ||
				    OBJ_FLAGGED(item, ITEM_NODROP) ||
				    OBJ_FLAGGED(item, ITEM_NOSELL))
				return (OBJECT_NOTOK);
			else if (evaluate_expression(item, word_for_mode(shop_nr, counter, mode))) {
				return (OBJECT_OK);
			}
		}
	return (OBJECT_NOTOK);
}


int same_obj(OBJ_DATA * obj1, OBJ_DATA * obj2)
{
	int index;

	if (!obj1 || !obj2)
		return (FALSE /* obj1 == obj2 */ );

	if (GET_OBJ_RNUM(obj1) != GET_OBJ_RNUM(obj2))
		return (FALSE);

	if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
		return (FALSE);

	if (GET_OBJ_EXTRA(obj1, 0) != GET_OBJ_EXTRA(obj2, 0) ||
	    GET_OBJ_EXTRA(obj1, INT_ONE) != GET_OBJ_EXTRA(obj2, INT_ONE) ||
	    GET_OBJ_EXTRA(obj1, INT_TWO) != GET_OBJ_EXTRA(obj2, INT_TWO) ||
	    GET_OBJ_EXTRA(obj1, INT_THREE) != GET_OBJ_EXTRA(obj2, INT_THREE))
		return (FALSE);

	for (index = 0; index < MAX_OBJ_AFFECT; index++)
		if ((obj1->affected[index].location != obj2->affected[index].location) ||
		    (obj1->affected[index].modifier != obj2->affected[index].modifier))
			return (FALSE);

	return (TRUE);
}

/**
* Dee, 30.08.2004: Old code checked whether tested object are the same exacly as produced.
* Need to be test whether tested object has the same object type (object type id from database)
*/
int shop_producing(OBJ_DATA * item, int shop_nr)
{
	int counter;
	obj_data *prod;
	int obj_id = GET_OBJ_VNUM(item);
	if (GET_OBJ_RNUM(item) < 0)
		return (FALSE);

	for (counter = 0; SHOP_PRODUCT(shop_nr, counter) != NOTHING; counter++) {
		prod = &obj_proto[SHOP_PRODUCT(shop_nr, counter)];
		if (obj_id == GET_OBJ_VNUM(prod)) {
			return (TRUE);
		}
	}
	if (GET_OBJ_OWNER(item) != 0) {
		return (TRUE);
	}
	return (FALSE);
}


int transaction_amt(char *arg)
{
	int num;
	char *buywhat;

	/*
	 * If we have two arguments, it means 'buy 5 3', or buy 5 of #3.
	 * We don't do that if we only have one argument, like 'buy 5', buy #5.
	 * Code from Andrey Fidrya <andrey@ALEX-UA.COM>
	 */
	buywhat = one_argument(arg, buf);
	if (*buywhat && *buf && is_number(buf)) {
		num = atoi(buf);
		strcpy(arg, arg + strlen(buf) + 1);
		return (num);
	}
	return (1);
}


char *times_message(OBJ_DATA * obj, char *name, int num, int padis)
{
	static char buf[256];
	char *ptr;

	if (obj)
		strcpy(buf, obj->PNames[padis]);
	else {
		if ((ptr = strchr(name, '.')) == NULL)
			ptr = name;
		else
			ptr++;
		sprintf(buf, "%s", ptr);
	}

	if (num > 1)
		sprintf(END_OF(buf), " (x %d %s)", num, desc_count(num, WHAT_THINGa));
	return (buf);
}


OBJ_DATA *get_slide_obj_vis(CHAR_DATA * ch, char *name, OBJ_DATA * list)
{
	OBJ_DATA *i, *last_match = 0;
	int j, number;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp;

	strcpy(tmpname, name);
	tmp = tmpname;
	if (!(number = get_number(&tmp)))
		return (0);

	for (i = list, j = 1; i && (j <= number); i = i->next_content)
		if (isname(tmp, i->name))
			if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i)) {
				if (j == number)
					return (i);
				last_match = i;
				j++;
			}
	return (0);
}


OBJ_DATA *get_hash_obj_vis(CHAR_DATA * ch, char *name, OBJ_DATA * list)
{
	OBJ_DATA *loop, *last_obj = 0;
	int index;

	if (is_number(name))
		index = atoi(name);
	else if (is_number(name + 1))
		index = atoi(name + 1);
	else
		return (0);

	for (loop = list; loop; loop = loop->next_content)
		if (CAN_SEE_OBJ(ch, loop) && (loop->obj_flags.cost > 0))
			if (!same_obj(last_obj, loop)) {
				if (--index == 0)
					return (loop);
				last_obj = loop;
			}
	return (0);
}


OBJ_DATA *get_purchase_obj(CHAR_DATA * ch, char *arg, CHAR_DATA * keeper, int shop_nr, int msg)
{
	char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;

	one_argument(arg, name);
	do {
		if (*name == '#' || is_number(name))
			obj = get_hash_obj_vis(ch, name, keeper->carrying);
		else
			obj = get_slide_obj_vis(ch, name, keeper->carrying);
		if (!obj) {
			if (msg) {
				sprintf(buf, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
				do_tell(keeper, buf, cmd_tell, 0);
			}
			return (0);
		}
		if (GET_OBJ_COST(obj) <= 0) {
			extract_obj(obj);
			obj = 0;
		}
	}
	while (!obj);
	return (obj);
}


int buy_price(OBJ_DATA * obj, CHAR_DATA * ch, int shop_nr)
{
	int profit;
	if (ch && !IS_NPC(ch) && GET_CLASS(ch) == CLASS_MERCHANT) {
		profit = (int) (SHOP_BUYPROFIT(shop_nr) * 100);
		if (profit < 100) {
			profit += ((100 - profit) * GET_LEVEL(ch) / (LVL_IMMORT - 1));
			profit = MIN(100, profit);
		}
	} else
		profit = (int) (SHOP_BUYPROFIT(shop_nr) * 100);
	return (GET_OBJ_COST(obj) * profit / 100);
}


void shopping_buy(char *arg, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr)
{
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj, *last_obj = NULL;
	int goldamt = 0, buynum, bought = 0, decay;

	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);

	if ((buynum = transaction_amt(arg)) < 0) {
		sprintf(buf, "%s! Протри глаза, у меня нет такой вещи.\r\n", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	if (!(*arg) || !(buynum)) {
		sprintf(buf, "%s! ЧТО ты хочешь купить ?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
		return;

	if ((buy_price(obj, ch, shop_nr) > GET_GOLD(ch)) && !IS_GOD(ch)) {
		sprintf(buf, shop_index[shop_nr].missing_cash2, GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		switch (SHOP_BROKE_TEMPER(shop_nr)) {
		case 0:
			sprintf(buf, "ругать %s!", GET_NAME(ch));
			do_social(keeper, buf);
			return;
		case 1:
			sprintf(buf, "отхлебнул$g немелкий глоток %s", IS_MALE(keeper) ? "водки" : "медовухи");
			do_echo(keeper, buf, cmd_emote, SCMD_EMOTE);
			return;
		default:
			return;
		}
	}
	if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))) {
		sprintf(buf, "%s, я понимаю, своя ноша карман не тянет,\r\n"
			"но %s Вам явно некуда положить.\r\n", GET_NAME(ch), OBJN(obj, ch, 3));
		send_to_char(buf, ch);
		return;
	}
	if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
		sprintf(buf, "%s, я понимаю, своя ноша карман не тянет,\r\n"
			"но %s Вам явно не поднять.\r\n", GET_NAME(ch), OBJN(obj, ch, 3));
		send_to_char(buf, ch);
		return;
	}
	while ((obj) &&
	       (GET_GOLD(ch) >= buy_price(obj, ch, shop_nr) || IS_GOD(ch)) &&
	       (IS_CARRYING_N(ch) < CAN_CARRY_N(ch)) &&
	       (bought < buynum) && (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch))) {
		bought++;

		/* Test if producing shop ! */
		if (shop_producing(obj, shop_nr)) {
			obj = read_object(GET_OBJ_RNUM(obj), REAL);
			load_otrigger(obj);
		} else {
			obj_from_char(obj);
			SHOP_SORT(shop_nr)--;
		}

		// Repair object
		decay = (GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj)) / 5;
		decay = MAX(1, MIN(decay, GET_OBJ_MAX(obj) / 10));
		if (GET_OBJ_MAX(obj) > decay)
			GET_OBJ_MAX(obj) -= decay;
		else
			GET_OBJ_MAX(obj) = 1;
		GET_OBJ_CUR(obj) = GET_OBJ_MAX(obj);

		goldamt += buy_price(obj, ch, shop_nr);
		if (!IS_GOD(ch))
			GET_GOLD(ch) -= buy_price(obj, ch, shop_nr);

		last_obj = obj;
		obj = get_purchase_obj(ch, arg, keeper, shop_nr, FALSE);
		if (!same_obj(obj, last_obj)) {
			obj_to_char(last_obj, ch);
			break;
		}
		obj_to_char(last_obj, ch);
	}

	if (!(obj = last_obj))
		return;

	if (bought < buynum) {
		if (GET_GOLD(ch) < buy_price(obj, ch, shop_nr))
			sprintf(buf, "%s! Мошенни%s, ты можешь оплатить только %d.",
				GET_NAME(ch), IS_MALE(ch) ? "к" : "ца", bought);
		else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
			sprintf(buf, "%s! Ты сможешь унести только %d.", GET_NAME(ch), bought);
		else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
			sprintf(buf, "%s! Ты сможешь поднять только %d.", GET_NAME(ch), bought);
		else if (!obj || !same_obj(last_obj, obj))
			sprintf(buf, "%s! У меня их осталось всего %d.", GET_NAME(ch), bought);
		else
			sprintf(buf, "%s! Я продам тебе только %d.", GET_NAME(ch), bought);
		do_tell(keeper, buf, cmd_tell, 0);
	}

	/* if (!IS_GOD(ch)) */
	GET_GOLD(keeper) += goldamt;

	sprintf(buf, "$n купил$g %s.", times_message(obj /* ch->carrying */ , 0, bought, 3));
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

	/* sprintf(buf, shop_index[shop_nr].message_buy, GET_NAME(ch), goldamt); */
	write_buf(buf, shop_index[shop_nr].message_buy, GET_NAME(ch), goldamt, WHAT_MONEYu);
	do_tell(keeper, buf, cmd_tell, 0);

	sprintf(buf, "Теперь Вы стали %s %s.\r\n",
		IS_MALE(ch) ? "счастливым обладателем" :
		"счастливой обладательницей", times_message(obj /* ch->carrying */ , 0, bought, 1));
	send_to_char(buf, ch);

	if (SHOP_USES_BANK(shop_nr))
		if (GET_GOLD(keeper) > MAX_OUTSIDE_BANK) {
			SHOP_BANK(shop_nr) += (GET_GOLD(keeper) - MAX_OUTSIDE_BANK);
			GET_GOLD(keeper) = MAX_OUTSIDE_BANK;
		}
}

OBJ_DATA *get_selling_item(CHAR_DATA * ch, OBJ_DATA * obj, CHAR_DATA * keeper, int shop_nr, int msg, int mode)
{
	char buf[MAX_STRING_LENGTH];
	int result;

	if (!obj)
		return (0);

	if ((result = trade_with(obj, shop_nr, mode)) == OBJECT_OK)
		return (obj);

	switch (result) {
	case OBJECT_NOVAL:
		sprintf(buf, "%s! Не впаривай мне мозги, этот хлам можешь оставить себе !", GET_NAME(ch));
		break;
	case OBJECT_NOTOK:
		if (mode == MODE_REPAIR)
			sprintf(buf, "%s! Я не собираюсь чинить %s.", GET_NAME(ch), obj ? obj->PNames[3] : "эту вещь");
		else if (mode == MODE_CHANGE)
			sprintf(buf, "%s! Я не меняю подобные вещи.", GET_NAME(ch));
		else
			sprintf(buf, shop_index[shop_nr].do_not_buy, GET_NAME(ch));
		break;
	case OBJECT_DEAD:
		sprintf(buf, "%s! %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
		break;
	case OBJECT_NOTEMPTY:
		sprintf(buf, "%s! В %s что-то лежит", GET_NAME(ch), obj->PNames[5]);
		break;
	default:
		log("SYSERR: Illegal return value of %d from trade_with() (%s)", result, __FILE__);	/* Someone might rename it... */
		sprintf(buf, "%s! An error has occurred.", GET_NAME(ch));
		break;
	}
	if (msg)
		do_tell(keeper, buf, cmd_tell, 0);
	return (0);
}



OBJ_DATA *get_selling_obj(CHAR_DATA * ch, char *name, CHAR_DATA * keeper, int shop_nr, int msg)
{
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	int result;

	if (!(obj = get_obj_in_list_vis(ch, name, ch->carrying))) {
		if (msg) {
			sprintf(buf, shop_index[shop_nr].no_such_item2, GET_NAME(ch));
			do_tell(keeper, buf, cmd_tell, 0);
		}
		return (0);
	}
	if ((result = trade_with(obj, shop_nr, MODE_TRADE)) == OBJECT_OK)
		return (obj);

	switch (result) {
	case OBJECT_NOVAL:
		sprintf(buf, "%s! Не впаривай мне мозги, этот хлам можешь оставить себе !", GET_NAME(ch));
		break;
	case OBJECT_NOTOK:
		sprintf(buf, shop_index[shop_nr].do_not_buy, GET_NAME(ch));
		break;
	case OBJECT_DEAD:
		sprintf(buf, "%s! %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
		break;
	case OBJECT_NOTEMPTY:
		sprintf(buf, "%s! В %s что-то лежит", GET_NAME(ch), obj->PNames[5]);
		break;
	default:
		log("SYSERR: Illegal return value of %d from trade_with() (%s)", result, __FILE__);	/* Someone might rename it... */
		sprintf(buf, "%s! An error has occurred.", GET_NAME(ch));
		break;
	}
	if (msg)
		do_tell(keeper, buf, cmd_tell, 0);
	return (0);
}


int sell_price(OBJ_DATA * obj, CHAR_DATA * ch, int shop_nr)
{
	double profit;
	if (ch && !IS_NPC(ch) && GET_CLASS(ch) == CLASS_MERCHANT)
		profit = MMAX(SHOP_SELLPROFIT(shop_nr), 1.0);
	else
		profit = SHOP_SELLPROFIT(shop_nr);

	if (GET_OBJ_TIMER(obj) < GET_OBJ_TIMER(obj_proto + GET_OBJ_RNUM(obj)))	// Если у шмотки все-таки таймер меньше текущего
		profit = (profit * (GET_OBJ_TIMER(obj)) / GET_OBJ_TIMER(obj_proto + GET_OBJ_RNUM(obj)));

	if (GET_OBJ_CUR(obj) < MMAX(1, GET_OBJ_MAX(obj)))
		return (MMAX(1, (int) (GET_OBJ_COST(obj) * profit * GET_OBJ_CUR(obj) / MMAX(1, GET_OBJ_MAX(obj)))));
	else
		return (MMAX(1, (int) (GET_OBJ_COST(obj) * profit)));
}


OBJ_DATA *slide_obj(OBJ_DATA * obj, CHAR_DATA * keeper, int shop_nr)
/*
   This function is a slight hack!  To make sure that duplicate items are
   only listed once on the "list", this function groups "identical"
   objects together on the shopkeeper's inventory list.  The hack involves
   knowing how the list is put together, and manipulating the order of
   the objects on the list.  (But since most of DIKU is not encapsulated,
   and information hiding is almost never used, it isn't that big a deal) -JF
*/
{
	OBJ_DATA *loop;
	int temp;

	if (!obj)
		return (NULL);

	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);

	/* Extract the object if it is identical to one produced */
	if (shop_producing(obj, shop_nr) || trade_with(obj, shop_nr, MODE_TRADE) != OBJECT_OK) {
		temp = GET_OBJ_RNUM(obj);
		extract_obj(obj);
		return (&obj_proto[temp]);
	}
	SHOP_SORT(shop_nr)++;
	loop = keeper->carrying;
	obj_to_char(obj, keeper);
	keeper->carrying = loop;
	while (loop) {
		if (same_obj(obj, loop)) {
			obj->next_content = loop->next_content;
			loop->next_content = obj;
			return (obj);
		}
		loop = loop->next_content;
	}
	keeper->carrying = obj;
	return (obj);
}


void sort_keeper_objs(CHAR_DATA * keeper, int shop_nr)
{
	OBJ_DATA *list = 0, *temp;

	while (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper)) {
		temp = keeper->carrying;
		// log(">>%s[%s]",temp->carried_by ? GET_NAME(temp->carried_by) : "EMPTY",
		// temp->PNames[0]);
		obj_from_char(temp);
		temp->next_content = list;
		list = temp;
	}

	while (list) {
		temp = list;
		list = list->next_content;
		if ((shop_producing(temp, shop_nr)) && !(get_obj_in_list_num(GET_OBJ_RNUM(temp), keeper->carrying))) {
			obj_to_char(temp, keeper);
			SHOP_SORT(shop_nr)++;
		} else
			(void) slide_obj(temp, keeper, shop_nr);
	}
}

void shopping_sell_item(OBJ_DATA * obj, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr)
{
	char buf[MAX_STRING_LENGTH];
	int sold = 0, goldamt = 0;

	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if (!obj) {
		sprintf(buf, "%s! ЧТО ты хочешь мне втюрить ?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	if (!(obj = get_selling_item(ch, obj, keeper, shop_nr, TRUE, MODE_TRADE)))
		return;

	if (GET_OBJ_MAX(obj) > GET_OBJ_CUR(obj) * 2) {
		sprintf(buf, "%s! Я не куплю %s в таком ужасном состоянии.", GET_NAME(ch), obj->PNames[3]);
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	goldamt += sell_price(obj, ch, shop_nr);	// стоимость продаваемой шмотки
	if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) < goldamt) {
		sprintf(buf, shop_index[shop_nr].missing_cash1, GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	GET_GOLD(keeper) -= goldamt;
	obj_from_char(obj);
	obj = slide_obj(obj, keeper, shop_nr);

	GET_GOLD(ch) += goldamt;
	sprintf(buf, "$n продал$g %s.", times_message(obj, 0, sold, 3));
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

/*  sprintf(buf, shop_index[shop_nr].message_sell, GET_NAME(ch), goldamt); */
	write_buf(buf, shop_index[shop_nr].message_sell, GET_NAME(ch), goldamt, WHAT_MONEYa);
	do_tell(keeper, buf, cmd_tell, 0);

	sprintf(buf, "Теперь у %s есть %s.\r\n", keeper->player.PNames[1], times_message(obj, 0, sold, 0));
	send_to_char(buf, ch);

	if (GET_GOLD(keeper) < MIN_OUTSIDE_BANK) {
		goldamt = MIN(MAX_OUTSIDE_BANK - GET_GOLD(keeper), SHOP_BANK(shop_nr));
		SHOP_BANK(shop_nr) -= goldamt;
		GET_GOLD(keeper) += goldamt;
	}
}



void shopping_sell(char *arg, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr)
{
	char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int sellnum, sold = 0, goldamt = 0;

	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if ((sellnum = transaction_amt(arg)) < 0) {
		sprintf(buf, "%s! Этой вещи у тебя быть не может.", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	if (!(*arg) || !(sellnum)) {
		sprintf(buf, "%s! ЧТО ты хочешь мне втюрить ?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	one_argument(arg, name);
	if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
		return;

	if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) < sell_price(obj, ch, shop_nr)) {
		sprintf(buf, shop_index[shop_nr].missing_cash1, GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	while ((obj) && (GET_GOLD(keeper) + SHOP_BANK(shop_nr) >= sell_price(obj, ch, shop_nr)) && (sold < sellnum)) {
		sold++;

		goldamt += sell_price(obj, ch, shop_nr);
		GET_GOLD(keeper) -= sell_price(obj, ch, shop_nr);

		obj_from_char(obj);
		slide_obj(obj, keeper, shop_nr);	/* Seems we don't use return value. */
		obj = get_selling_obj(ch, name, keeper, shop_nr, FALSE);
	}

	if (sold < sellnum) {
		if (!obj)
			sprintf(buf, "%s! У Вас их ведь только %d.", GET_NAME(ch), sold);
		else if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) < sell_price(obj, ch, shop_nr))
			sprintf(buf, "%s! Я смогу оплатить только %d %s.",
				GET_NAME(ch), sold, desc_count(sold, WHAT_THINGu));
		else
			sprintf(buf, "%s! Но я могу купить только %d %s.",
				GET_NAME(ch), sold, desc_count(sold, WHAT_THINGu));

		do_tell(keeper, buf, cmd_tell, 0);
	}
	GET_GOLD(ch) += goldamt;
	sprintf(buf, "$n продал$g %s.", times_message(0, name, sold, 3));
	act(buf, FALSE, ch, obj, 0, TO_ROOM);

/*  sprintf(buf, shop_index[shop_nr].message_sell, GET_NAME(ch), goldamt); */
	write_buf(buf, shop_index[shop_nr].message_sell, GET_NAME(ch), goldamt, WHAT_MONEYa);
	do_tell(keeper, buf, cmd_tell, 0);

	sprintf(buf, "Теперь у %s есть %s.\r\n", keeper->player.PNames[1], times_message(0, name, sold, 0));
	send_to_char(buf, ch);

	if (GET_GOLD(keeper) < MIN_OUTSIDE_BANK) {
		goldamt = MIN(MAX_OUTSIDE_BANK - GET_GOLD(keeper), SHOP_BANK(shop_nr));
		SHOP_BANK(shop_nr) -= goldamt;
		GET_GOLD(keeper) += goldamt;
	}
}

#define MAX_LIST_LENGTH 200

void restore_objects(CHAR_DATA * ch, OBJ_DATA * objs[], int count)
{
	int i;
	for (i = 0; i < count; i++)
		if (objs[i])
			obj_to_char(objs[i], ch);
}

void shopping_change(char *argument, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr, int operation)
{
	OBJ_DATA *source_list[MAX_LIST_LENGTH], *dest_list[MAX_LIST_LENGTH], *obj, *next_obj;
	char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
	int srcs = 0, dests = 0, buynum = 0, bought = 0, sellprice = 0, buyprice =
	    0, items_inc = 0, weight_inc = 0, items_dec = 0, weight_dec = 0, i, count, multi, dotmode;
	double profit;


	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);

	if (!*argument) {
		sprintf(buf, "%s! ЧТО ты хочешь менять ?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	while (argument && *argument) {
		argument = one_argument(argument, arg);

		if (!str_cmp(arg, "на"))
			break;
		else if (srcs >= MAX_LIST_LENGTH)
			continue;

		if (is_number(arg)) {
			multi = atoi(arg);
			argument = one_argument(argument, arg);
			if (!arg || !*arg)
				continue;
			if (multi <= 0)
				continue;
			else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				sprintf(buf, "У Вас нет ничего похожего на %s.\r\n", arg);
				send_to_char(buf, ch);
			} else {
				do {
					next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
					obj_from_char(obj);
					source_list[srcs++] = obj;
					obj = next_obj;
				}
				while (obj && --multi && srcs < MAX_LIST_LENGTH);
			}
		} else {
			dotmode = find_all_dots(arg);

			if (dotmode == FIND_ALL) {
				if (!ch->carrying)
					continue;
				else
					for (obj = ch->carrying; obj && srcs < MAX_LIST_LENGTH; obj = next_obj) {
						next_obj = obj->next_content;
						obj_from_char(obj);
						source_list[srcs++] = obj;
						obj = next_obj;
					}
			} else if (dotmode == FIND_ALLDOT) {
				if (!*arg)
					continue;
				if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
					continue;
				while (obj && srcs < MAX_LIST_LENGTH) {
					next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
					obj_from_char(obj);
					source_list[srcs++] = obj;
					obj = next_obj;
				}
			} else if ((obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				obj_from_char(obj);
				source_list[srcs++] = obj;
			}
		}
	}

	if (!srcs) {
		sprintf(buf, "%s! Пустые слова !", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	if ((buynum = transaction_amt(argument)) < 0) {
		sprintf(buf, "%s! Протри глаза, у меня нет такой вещи.\r\n", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		restore_objects(ch, source_list, srcs);
		return;
	}

	if (!*argument || !buynum) {
		sprintf(buf, "%s! НА ЧТО ты хочешь это обменять ?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		restore_objects(ch, source_list, srcs);
		return;
	}

	if (!(obj = get_purchase_obj(ch, argument, keeper, shop_nr, TRUE))) {
		restore_objects(ch, source_list, srcs);
		return;
	}
	// Calc incoming value
	for (i = 0; i < srcs; i++) {
		if (get_selling_item(ch, source_list[i], keeper, shop_nr, TRUE, MODE_CHANGE)) {
			if (GET_OBJ_MAX(source_list[i]) > GET_OBJ_CUR(source_list[i]) * 2) {
				sprintf(buf, "%s! Я не возьму %s в таком ужасном состоянии.",
					GET_NAME(ch), source_list[i]->PNames[4]);
				do_tell(keeper, buf, cmd_tell, 0);
				obj_to_char(source_list[i], ch);
				source_list[i] = NULL;
			} else {
				sellprice +=
				    MAX(1,
					GET_OBJ_COST(source_list[i]) *
					GET_OBJ_CUR(source_list[i]) / MAX(1, GET_OBJ_MAX(source_list[i])));
				weight_dec += GET_OBJ_WEIGHT(source_list[i]);
				items_dec += 1;
			}
		} else {
			obj_to_char(source_list[i], ch);
			source_list[i] = NULL;
		}
	}

	if (!sellprice) {
		sprintf(buf, "%s! Ты не предлагаешь мне ничего, что мне требуется.", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		restore_objects(ch, source_list, srcs);
		return;
	}
	// Calc outgouing value
	buyprice = GET_OBJ_COST(obj) * buynum;
	if (!IS_NPC(ch) && GET_CLASS(ch) == CLASS_MERCHANT)
		profit = MMAX(SHOP_CHANGEPROFIT(shop_nr), 1.0);
	else
		profit = SHOP_CHANGEPROFIT(shop_nr);
	count = (int) (sellprice * profit / MMAX(1, buyprice));
	if (!count)
		sprintf(buf, "%s! Меня не устраивает такой обмен.", GET_NAME(ch));
	else if (buynum == 1 && operation == OPER_CHANGE_VALUE)
		sprintf(buf, "%s! Я дам тебе за это %d %s (%s).", GET_NAME(ch),
			count, desc_count(count, WHAT_THINGa), obj->PNames[0]);
	else
		sprintf(buf, "%s! Меня устраивает такой обмен.", GET_NAME(ch));

	if (!count || operation == OPER_CHANGE_VALUE) {
		do_tell(keeper, buf, cmd_tell, 0);
		restore_objects(ch, source_list, srcs);
		return;
	}

	while (obj && bought < buynum && dests < MAX_LIST_LENGTH) {
		bought++;
		// Test if producing shop !
		if (shop_producing(obj, shop_nr)) {
			obj = read_object(GET_OBJ_RNUM(obj), REAL);
			load_otrigger(obj);
			dest_list[dests++] = obj;
		} else {
			obj_from_char(obj);
			SHOP_SORT(shop_nr)--;
			dest_list[dests++] = obj;
		}
		GET_OBJ_CUR(obj) = GET_OBJ_MAX(obj);
		items_inc += 1;
		weight_inc += GET_OBJ_WEIGHT(obj);
		next_obj = obj;
		obj = get_purchase_obj(ch, argument, keeper, shop_nr, FALSE);
		if (!same_obj(obj, next_obj))
			break;
	}

	if (bought < buynum)
		sprintf(buf, "%s У меня осталось всего %d %s.", GET_NAME(ch), bought, desc_count(bought, WHAT_THINGa));
	else if (IS_CARRYING_N(ch) + items_inc - items_dec >= CAN_CARRY_N(ch))
		sprintf(buf, "%s! Боюсь, ты столько просто не удержишь :)", GET_NAME(ch));
	else if (IS_CARRYING_W(ch) + weight_inc - weight_dec >= CAN_CARRY_W(ch))
		sprintf(buf, "%s! Боюсь, у тебя развяжеться пупок :)", GET_NAME(ch));
	else
		*buf = '\0';

	if (*buf) {
		restore_objects(ch, source_list, srcs);
		for (i = 0; i < dests; i++)
			slide_obj(dest_list[i], keeper, shop_nr);
	} else {
		restore_objects(ch, dest_list, dests);
		for (i = 0; i < srcs; i++)
			if (source_list[i])
				slide_obj(source_list[i], keeper, shop_nr);
		sprintf(buf, "%s! По рукам !", GET_NAME(ch));
	}
	do_tell(keeper, buf, cmd_tell, 0);
}


void show_values(char *arg, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr, int operation)
{
	OBJ_DATA *obj;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);

	if (transaction_amt(arg) < 0) {
		sprintf(buf, "%s! Протри глаза, у меня нет такой вещи.\r\n", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	if (!*arg) {
		sprintf(buf, "%s! ЧТО ты хочешь рассмотреть ?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
		return;

	sprintf(buf, "%s Предмет \"%s\", тип ", GET_NAME(ch), obj->short_description);
	sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
	strcat(buf, buf2);
	strcat(buf, "\n");
	strcpy(buf2, diag_weapon_to_char(obj, TRUE));
	if (*buf2) {
		strcat(buf, buf2);
	};
	strcat(buf, diag_timer_to_char(obj));
	do_tell(keeper, buf, cmd_tell, 0);
	if (invalid_anti_class(ch, obj) || invalid_unique(ch, obj)) {
		sprintf(buf, "%s Но лучше бы тебе не заглядываться на нее, не унесешь все равно.", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
	}
}

void shopping_value_item(OBJ_DATA * obj, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr)
{
	char buf[MAX_STRING_LENGTH];
	char name[MAX_INPUT_LENGTH];
	int price = 0;

	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if (!obj) {
		sprintf(buf, "%s! Что я тебе должен оценить ?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	one_argument(arg, name);

	if (!(obj = get_selling_item(ch, obj, keeper, shop_nr, TRUE, MODE_TRADE)))
		return;

	if (GET_OBJ_MAX(obj) > GET_OBJ_CUR(obj) * 2) {
		sprintf(buf, "%s! Я не куплю %s в таком ужасном состоянии.", GET_NAME(ch), obj->PNames[3]);
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}


	sprintf(buf, "%s! Я, пожалуй, дам тебе за %s %d %s !",
		GET_NAME(ch), OBJN(obj, ch, 3), (price = sell_price(obj, ch, shop_nr)), desc_count(price, WHAT_MONEYu));
	do_tell(keeper, buf, cmd_tell, 0);

	return;
}

void shopping_value(char *arg, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr)
{
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	char name[MAX_INPUT_LENGTH];
	int price = 0;

	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if (!(*arg)) {
		sprintf(buf, "%s! Что я Вам должен оценить ?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	one_argument(arg, name);
	if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
		return;

	if (GET_OBJ_MAX(obj) > GET_OBJ_CUR(obj) * 2) {
		sprintf(buf, "%s! Я не куплю %s в таком ужасном состоянии.", GET_NAME(ch), obj->PNames[3]);
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	sprintf(buf, "%s! Я пожалуй дам тебе за %s %d %s !",
		GET_NAME(ch), OBJN(obj, ch, 3), (price = sell_price(obj, ch, shop_nr)), desc_count(price, WHAT_MONEYu));
	do_tell(keeper, buf, cmd_tell, 0);

	return;
}

void shopping_repair_item(OBJ_DATA * obj, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr)
{
	char buf[MAX_STRING_LENGTH];
	char name[MAX_INPUT_LENGTH];
	int price = 0, repair = 0;

	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if (!obj) {
		sprintf(buf, "%s! Что я Вам должен починить ?", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	one_argument(arg, name);

	if (!(obj = get_selling_item(ch, obj, keeper, shop_nr, TRUE, MODE_REPAIR)))
		return;

	price = GET_OBJ_COST(obj);
	repair = GET_OBJ_MAX(obj) - GET_OBJ_CUR(obj);
	price = MAX(1, price * MAX(0, repair) / MAX(1, GET_OBJ_MAX(obj)));
	if (repair <= 0) {
		std::string obj_name = OBJN(obj, ch, 0);
		obj_name[0] = UPPER(obj_name[0]);
		sprintf(buf, "%s! %s не нужда%s в починке.", GET_NAME(ch),
			obj_name.c_str(), GET_OBJ_SEX(obj) == SEX_POLY ? "ются" : "ется");
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	switch (obj->obj_flags.Obj_mater) {
	case MAT_BULAT:
	case MAT_CRYSTALL:
	case MAT_DIAMOND:
	case MAT_SWORDSSTEEL:
		price *= 2;
		break;
	case MAT_SUPERWOOD:
	case MAT_COLOR:
	case MAT_GLASS:
	case MAT_BRONZE:
	case MAT_FARFOR:
	case MAT_BONE:
	case MAT_ORGANIC:
		price += MAX(1, price / 2);
		break;
	case MAT_IRON:
	case MAT_STEEL:
	case MAT_SKIN:
	case MAT_MATERIA:
		price = price;
		break;
	default:
		price = price;
	}

	if (price <= 0 ||
	    OBJ_FLAGGED(obj, ITEM_DECAY) ||
	    OBJ_FLAGGED(obj, ITEM_NOSELL) || OBJ_FLAGGED(obj, ITEM_NODROP)) {
		sprintf(buf, "%s! Я не буду тратить свое драгоценное время на %s.", GET_NAME(ch), OBJN(obj, ch, 3));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}

	sprintf(buf, "%s! Починка %s обойдется в %d %s",
		GET_NAME(ch), OBJN(obj, ch, 1), price, desc_count(price, WHAT_MONEYu));
	do_tell(keeper, buf, cmd_tell, 0);

	if (!IS_GOD(ch) && price > GET_GOLD(ch)) {
		act("А вот их у тебя как-раз то и нет.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	GET_GOLD(keeper) += price;
	if (!IS_GOD(ch))
		GET_GOLD(ch) -= price;
	act("$n сноровисто починил$g $o3.", FALSE, keeper, obj, 0, TO_ROOM);
	GET_OBJ_CUR(obj) = GET_OBJ_MAX(obj);

	if (SHOP_USES_BANK(shop_nr))
		if (GET_GOLD(keeper) > MAX_OUTSIDE_BANK) {
			SHOP_BANK(shop_nr) += (GET_GOLD(keeper) - MAX_OUTSIDE_BANK);
			GET_GOLD(keeper) = MAX_OUTSIDE_BANK;
		}

	return;
}


const char *shop_op[3][2] = { {"продать", "продать"},
{"починить", "починить"},
{"оценить", "оценить"}
};

void do_multi(int subcmd, char *argument, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr)
{
	OBJ_DATA *obj, *next_obj;
	int dotmode, multi;
	int sname;
	char arg[MAX_STRING_LENGTH];

	switch (subcmd) {
	case SCMD_SELL:
		sname = 0;
		break;
	case SCMD_VALUE:
		sname = 2;
	case SCMD_REPAIR:
		sname = 1;
		break;
	default:
		return;
	}

	argument = one_argument(argument, arg);

	if (!*arg) {
		sprintf(buf, "Что Вы хотите %s?\r\n", shop_op[sname][0]);
		send_to_char(buf, ch);
		return;
	} else if (is_number(arg)) {
		multi = atoi(arg);
		one_argument(argument, arg);
		if (multi <= 0)
			send_to_char("Не имеет смысла.\r\n", ch);
		else if (!*arg) {
			sprintf(buf, "%s %d чего ?\r\n", shop_op[sname][0], multi);
			send_to_char(buf, ch);
		} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
			sprintf(buf, "У Вас нет ничего похожего на %s.\r\n", arg);
			send_to_char(buf, ch);
		} else {
			do {
				next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
				switch (subcmd) {
				case SCMD_SELL:
					shopping_sell_item(obj, ch, keeper, shop_nr);
					break;
				case SCMD_REPAIR:
					shopping_repair_item(obj, ch, keeper, shop_nr);
					break;
				default:
					shopping_value_item(obj, ch, keeper, shop_nr);
				}
				obj = next_obj;
			}
			while (obj && --multi);
		}
	} else {
		dotmode = find_all_dots(arg);
		if (dotmode == FIND_ALL) {
			if (!ch->carrying)
				send_to_char("А у Вас ничего и нет.\r\n", ch);
			else
				for (obj = ch->carrying; obj; obj = next_obj) {
					next_obj = obj->next_content;
					switch (subcmd) {
					case SCMD_SELL:
						shopping_sell_item(obj, ch, keeper, shop_nr);
						break;
					case SCMD_REPAIR:
						shopping_repair_item(obj, ch, keeper, shop_nr);
						break;
					default:
						shopping_value_item(obj, ch, keeper, shop_nr);
					}
				}
		} else if (dotmode == FIND_ALLDOT) {
			if (!*arg) {
				sprintf(buf, "%s \"все\" какого типа предметов ?\r\n", shop_op[sname][0]);
				send_to_char(buf, ch);
				return;
			}
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				sprintf(buf, "У Вас нет ничего похожего на '%s'.\r\n", arg);
				send_to_char(buf, ch);
			}
			while (obj) {
				next_obj = get_obj_in_list_vis(ch, arg, obj->next_content);
				switch (subcmd) {
				case SCMD_SELL:
					shopping_sell_item(obj, ch, keeper, shop_nr);
					break;
				case SCMD_REPAIR:
					shopping_repair_item(obj, ch, keeper, shop_nr);
					break;
				default:
					shopping_value_item(obj, ch, keeper, shop_nr);
				}
				obj = next_obj;
			}
		} else {
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
				sprintf(buf, "У Вас нет '%s'.\r\n", arg);
				send_to_char(buf, ch);
			} else {
				switch (subcmd) {
				case SCMD_SELL:
					shopping_sell_item(obj, ch, keeper, shop_nr);
					break;
				case SCMD_REPAIR:
					shopping_repair_item(obj, ch, keeper, shop_nr);
					break;
				default:
					shopping_value_item(obj, ch, keeper, shop_nr);
				}
			}
		}
	}
}



char *list_object(OBJ_DATA * obj, int cnt, int index, int shop_nr)
{
	static char buf[256];
	char buf2[300], buf3[200];

	if (shop_producing(obj, shop_nr))
		strcpy(buf2, "Навалом     ");
	else
		sprintf(buf2, "%5d       ", cnt);
	sprintf(buf, " %2d)  %s", index, buf2);

	/* Compile object name and information */
	strcpy(buf3, obj->short_description);
	/*
	   if ((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) && (GET_OBJ_VAL(obj, 1)))
	   sprintf(END_OF(buf3), "  %s", drinks[GET_OBJ_VAL(obj, 2)]);
	 */
	/* FUTURE: */
	/* Add glow/hum/etc */

	if ((GET_OBJ_TYPE(obj) == ITEM_WAND) || (GET_OBJ_TYPE(obj) == ITEM_STAFF))
		if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1))
			strcat(buf3, " (б/у)");

	sprintf(buf2, "%-48s %6d\r\n", buf3, buy_price(obj, 0, shop_nr));
	strcat(buf, CAP(buf2));
	return (buf);
}

void shopping_list(char *arg, CHAR_DATA * ch, CHAR_DATA * keeper, int shop_nr)
{
	char *buf;		// Add by Alez
	char name[MAX_INPUT_LENGTH];	// Add by Alez
	OBJ_DATA *obj, *last_obj = NULL;
	int cnt = 0, index = 0;

	if (!(is_ok(keeper, ch, shop_nr)))
		return;

	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);

	one_argument(arg, name);
	buf = NULL;
	buf = str_add(buf, " ##   Доступно    Предмет                                           Цена \r\n");
	buf = str_add(buf, "-------------------------------------------------------------------------\r\n");
	if (keeper->carrying)
		for (obj = keeper->carrying; obj; obj = obj->next_content)
			if (CAN_SEE_OBJ(ch, obj) && (obj->obj_flags.cost > 0)) {
				if (!last_obj) {
					last_obj = obj;
					cnt = 1;
				} else if (same_obj(last_obj, obj))
					cnt++;
				else {
					index++;
					if (!(*name) || isname(name, last_obj->name)) {
						buf = str_add(buf, list_object(last_obj, cnt, index, shop_nr));
					}
					cnt = 1;
					last_obj = obj;
				}
			}
	index++;
	if (!last_obj) {
		free(buf);
		buf = NULL;
		if (*name)
			buf = str_add(buf, "Ничего похожего для продажи нет.\r\n");
		else
			buf = str_add(buf, "Увы, товара пока нет.\r\n");
	} else if (!(*name) || isname(name, last_obj->name))
		buf = str_add(buf, list_object(last_obj, cnt, index, shop_nr));

	page_string(ch->desc, buf, 1);
	free(buf);
}


int ok_shop_room(int shop_nr, int room)
{
	int index;

	for (index = 0; SHOP_ROOM(shop_nr, index) != NOWHERE; index++)
		if (SHOP_ROOM(shop_nr, index) == room)
			return (TRUE);
	return (FALSE);
}


SPECIAL(shop_keeper)
{
	char argm[MAX_INPUT_LENGTH];
	CHAR_DATA *keeper = (CHAR_DATA *) me;
	int shop_nr;

	for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
		if (SHOP_KEEPER(shop_nr) == keeper->nr)
			break;

	if (shop_nr > top_shop)
		return (FALSE);

	if (SHOP_FUNC(shop_nr))	/* Check secondary function */
		if ((SHOP_FUNC(shop_nr)) (ch, me, cmd, arg))
			return (TRUE);

	if (keeper == ch) {
		if (cmd)
			SHOP_SORT(shop_nr) = 0;	/* Safety in case "drop all" */
		return (FALSE);
	}
	if (!ok_shop_room(shop_nr, GET_ROOM_VNUM(IN_ROOM(ch))))
		return (0);

	if (!AWAKE(keeper))
		return (FALSE);

	if (CMD_IS("steal") || CMD_IS("украсть")) {
		sprintf(argm, "$N вскричал$g '%s'", MSG_NO_STEAL_HERE);
		sprintf(buf, "ругать %s", GET_NAME(ch));
		do_social(keeper, buf);
		act(argm, FALSE, ch, 0, keeper, TO_CHAR);
		return (TRUE);
	}

	if (CMD_IS("buy") || CMD_IS("купить")) {
		shopping_buy(argument, ch, keeper, shop_nr);
		return (TRUE);
	} else if (CMD_IS("sell") || CMD_IS("продать")) {	/* shopping_sell(argument, ch, keeper, shop_nr); */
		do_multi(SCMD_SELL, argument, ch, keeper, shop_nr);
		return (TRUE);
	} else if (CMD_IS("value") || CMD_IS("оценить")) {	/* shopping_value(argument, ch, keeper, shop_nr); */
		do_multi(SCMD_VALUE, argument, ch, keeper, shop_nr);
		return (TRUE);
	} else if (CMD_IS("list") || CMD_IS("список")) {
		shopping_list(argument, ch, keeper, shop_nr);
		return (TRUE);
	} else if (CMD_IS("repair") || CMD_IS("чинить")) {
		do_multi(SCMD_REPAIR, argument, ch, keeper, shop_nr);
		return (TRUE);
	} else if (CMD_IS("менять")) {
		shopping_change(argument, ch, keeper, shop_nr, OPER_CHANGE_VALUE);
		return (TRUE);
	} else if (CMD_IS("обмен")) {
		shopping_change(argument, ch, keeper, shop_nr, OPER_CHANGE_DONE);
		return (TRUE);
	} else if (CMD_IS("свойства")) {
		show_values(argument, ch, keeper, shop_nr, TRUE);
		return (TRUE);
	} else if (CMD_IS("рассмотреть")) {
		show_values(argument, ch, keeper, shop_nr, FALSE);
		return (TRUE);
	}
	return (FALSE);
}


int ok_damage_shopkeeper(CHAR_DATA * ch, CHAR_DATA * victim)
{
	char buf[200];
	int index;

	if (IS_MOB(victim)
	    && (mob_index[GET_MOB_RNUM(victim)].func == shop_keeper))
		for (index = 0; index <= top_shop; index++)
			if ((GET_MOB_RNUM(victim) == SHOP_KEEPER(index))
			    && !SHOP_KILL_CHARS(index)) {
				sprintf(buf, "ругать %s!", GET_NAME(ch));
				do_social(victim, buf);
				sprintf(buf, "%s! %s", GET_NAME(ch), MSG_CANT_KILL_KEEPER);
				do_tell(victim, buf, cmd_tell, 0);
				return (FALSE);
			}
	return (TRUE);
}


int add_to_list(struct shop_buy_data *list, int type, int *len, int *val)
{
	if (*val >= 0) {
		if (*len < MAX_SHOP_OBJ) {
			if (type == LIST_PRODUCE)
				*val = real_object(*val);
			if (*val >= 0) {
				BUY_TYPE(list[*len]) = *val;
				BUY_WORD(list[(*len)++]) = 0;
			} else
				*val = 0;
			return (FALSE);
		} else
			return (TRUE);
	}
	return (FALSE);
}


int end_read_list(struct shop_buy_data *list, int len, int error)
{
	if (error)
		log("SYSERR: Raise MAX_SHOP_OBJ constant in shop.h to %d", len + error);
	BUY_WORD(list[len]) = 0;
	BUY_TYPE(list[len++]) = NOTHING;
	return (len);
}


void read_line(FILE * shop_f, const char *string, void *data)
{
	if (!get_line(shop_f, buf) || !sscanf(buf, string, data)) {
		log("SYSERR: Error in shop #%d\n", SHOP_NUM(top_shop));
		exit(1);
	}
}


int read_list(FILE * shop_f, struct shop_buy_data *list, int new_format, int max, int type)
{
	int count, temp, len = 0, error = 0;

	if (new_format) {
		do {
			read_line(shop_f, "%d", &temp);
			error += add_to_list(list, type, &len, &temp);
		}
		while (temp >= 0);
	} else
		for (count = 0; count < max; count++) {
			read_line(shop_f, "%d", &temp);
			error += add_to_list(list, type, &len, &temp);
		}
	return (end_read_list(list, len, error));
}


int read_type_list(FILE * shop_f, struct shop_buy_data *list, int new_format, int max)
{
	int index, num, len = 0, error = 0;
	char *ptr;

	if (!new_format)
		return (read_list(shop_f, list, 0, max, LIST_TRADE));
	memset(buf, 0, MAX_STRING_LENGTH - 1);
	//log("0>");
	do {
		fgets(buf, MAX_STRING_LENGTH - 1, shop_f);
		if ((ptr = strchr(buf, ';')) != NULL)
			*ptr = 0;
		else
			*(END_OF(buf) - 1) = 0;
		//log("1>%s",buf);
		for (index = 0, num = NOTHING; *item_types[index] != '\n'; index++)
			if (!strn_cmp(item_types[index], buf, strlen(item_types[index]))) {
				num = index;
				strcpy(buf, buf + strlen(item_types[index]));
				break;
			}
		ptr = buf;
		//log("2>%s",buf);
		if (num == NOTHING) {
			sscanf(buf, "%d", &num);
			while (!isdigit(*ptr))
				ptr++;
			while (isdigit(*ptr))
				ptr++;
		}
		while (*ptr && a_isspace(*ptr))
			ptr++;
		//log("3>%s",ptr);
		while (*ptr && a_isspace(*(END_OF(ptr) - 1)))
			*(END_OF(ptr) - 1) = 0;
		//log("4>%s",ptr);
		error += add_to_list(list, LIST_TRADE, &len, &num);

		if (*ptr) {
			BUY_WORD(list[len - 1]) = str_dup(ptr);
		}
	}
	while (num >= 0);
	//log("5>");
	return (end_read_list(list, len, error));
}


void boot_the_shops(FILE * shop_f, char *filename, int rec_count)
{
	char *buf, buf2[150], use_change;
	int temp, count, new_format = 0;
	struct shop_buy_data list[MAX_SHOP_OBJ + 1];
	int done = 0;

	sprintf(buf2, "beginning of shop file %s", filename);

	while (!done) {
		buf = fread_string(shop_f, buf2);
		if (*buf == '#') {	/* New shop */
			use_change = 0;
			sscanf(buf, "#%d %c\n", &temp, &use_change);
			sprintf(buf2, "shop #%d in shop file %s", temp, filename);
			log("Shop %d %s", temp, use_change ? "(extend)" : "");
			free(buf);	/* Plug memory leak! */
			top_shop++;
			if (!top_shop)
				CREATE(shop_index, SHOP_DATA, rec_count);

			SHOP_NUM(top_shop) = temp;
			log("Read list...");
			temp = read_list(shop_f, list, new_format, MAX_PROD, LIST_PRODUCE);
			CREATE(shop_index[top_shop].producing, obj_vnum, temp);
			for (count = 0; count < temp; count++) {
				SHOP_PRODUCT(top_shop, count) = BUY_TYPE(list[count]);
//                  log("%d", BUY_TYPE(list[count]));
			}

			log("Read buy profit...");
			read_line(shop_f, "%f", &SHOP_BUYPROFIT(top_shop));
			log("Read sell profit...");
			read_line(shop_f, "%f", &SHOP_SELLPROFIT(top_shop));
			if (use_change) {
				log("Read change profit...");
				read_line(shop_f, "%f", &SHOP_CHANGEPROFIT(top_shop));
			}

			log("Read sell type list...");
			temp = read_type_list(shop_f, list, new_format, MAX_TRADE);
			CREATE(shop_index[top_shop].type, struct shop_buy_data, temp);
			for (count = 0; count < temp; count++) {
				SHOP_BUYTYPE(top_shop, count) = BUY_TYPE(list[count]);
				SHOP_BUYWORD(top_shop, count) = BUY_WORD(list[count]);
			}
			if (use_change) {
				log("Read change type list...");
				temp = read_type_list(shop_f, list, new_format, MAX_TRADE);
				CREATE(shop_index[top_shop].change, struct shop_buy_data, temp);
				for (count = 0; count < temp; count++) {
					SHOP_CHANGETYPE(top_shop, count) = BUY_TYPE(list[count]);
					SHOP_CHANGEWORD(top_shop, count) = BUY_WORD(list[count]);
				}
			} else {
				CREATE(shop_index[top_shop].change, struct shop_buy_data, 1);
				SHOP_CHANGETYPE(top_shop, 0) = -1;
				SHOP_CHANGEWORD(top_shop, 0) = NULL;
			}

			log("Read strings...");
			shop_index[top_shop].no_such_item1 = fread_string(shop_f, buf2);
			shop_index[top_shop].no_such_item2 = fread_string(shop_f, buf2);
			shop_index[top_shop].do_not_buy = fread_string(shop_f, buf2);
			shop_index[top_shop].missing_cash1 = fread_string(shop_f, buf2);
			shop_index[top_shop].missing_cash2 = fread_string(shop_f, buf2);
			shop_index[top_shop].message_buy = fread_string(shop_f, buf2);
			shop_index[top_shop].message_sell = fread_string(shop_f, buf2);
			read_line(shop_f, "%d", &SHOP_BROKE_TEMPER(top_shop));
			read_line(shop_f, "%d", &SHOP_BITVECTOR(top_shop));
			read_line(shop_f, "%d", &SHOP_KEEPER(top_shop));
			SHOP_KEEPER(top_shop) = real_mobile(SHOP_KEEPER(top_shop));

			log("Read trade with...");
			read_line(shop_f, "%d", &SHOP_TRADE_WITH(top_shop));

			log("Read shoprooms...");
			temp = read_list(shop_f, list, new_format, 1, LIST_ROOM);
			CREATE(shop_index[top_shop].in_room, room_rnum, temp);
			for (count = 0; count < temp; count++)
				SHOP_ROOM(top_shop, count) = BUY_TYPE(list[count]);

			log("Read work time...");
			read_line(shop_f, "%d", &SHOP_OPEN1(top_shop));
			read_line(shop_f, "%d", &SHOP_CLOSE1(top_shop));
			read_line(shop_f, "%d", &SHOP_OPEN2(top_shop));
			read_line(shop_f, "%d", &SHOP_CLOSE2(top_shop));

			SHOP_BANK(top_shop) = 0;
			SHOP_SORT(top_shop) = 0;
			SHOP_FUNC(top_shop) = 0;
		} else {
			if (*buf == '$')	/* EOF */
				done = TRUE;
			else if (strstr(buf, VERSION3_TAG))	/* New format marker */
				new_format = 1;
			free(buf);	/* Plug memory leak! */
		}
	}
}


void assign_the_shopkeepers(void)
{
	int index;

	cmd_say = find_command("say");
	cmd_tell = find_command("tell");
	cmd_emote = find_command("emote");
	cmd_slap = find_command("slap");
	cmd_puke = find_command("puke");
	for (index = 0; index <= top_shop; index++) {
//     log(">>%d",SHOP_KEEPER(index));
		if (SHOP_KEEPER(index) == NOBODY)
			continue;
		if (mob_index[SHOP_KEEPER(index)].func)
			SHOP_FUNC(index) = mob_index[SHOP_KEEPER(index)].func;
		mob_index[SHOP_KEEPER(index)].func = shop_keeper;
	}
}


char *customer_string(int shop_nr, int detailed)
{
	int index, cnt = 1;
	static char buf[256];

	*buf = 0;
	for (index = 0; *trade_letters[index] != '\n'; index++, cnt *= 2)
		if (!(SHOP_TRADE_WITH(shop_nr) & cnt)) {
			if (detailed) {
				if (*buf)
					strcat(buf, ", ");
				strcat(buf, trade_letters[index]);
			} else
				sprintf(END_OF(buf), "%c", *trade_letters[index]);
		} else if (!detailed)
			strcat(buf, "_");

	return (buf);
}


void list_all_shops(CHAR_DATA * ch)
{
	int shop_nr;
	char *buffer = NULL;

//  strcpy(buf, "\r\n");
	for (shop_nr = 0; shop_nr <= top_shop; shop_nr++) {
		if (!(shop_nr % 20)) {
			//strcat(buf, " ##   ВиртНомер    Где   Продавец    Купля Продажа Обмен Торгует с\r\n");
			//strcat(buf, "------------------------------------------------------------------\r\n");
			buffer =
			    str_add(buffer, " ##   ВиртНомер    Где   Продавец    Купля Продажа Обмен Торгует с\r\n");
			buffer =
			    str_add(buffer, "------------------------------------------------------------------\r\n");
		}
		sprintf(buf2, "%3d   %6d   %6d    ", shop_nr + 1, SHOP_NUM(shop_nr), SHOP_ROOM(shop_nr, 0));
		if (SHOP_KEEPER(shop_nr) < 0)
			strcpy(buf1, " <НЕТ>");
		else
			sprintf(buf1, "%6d", mob_index[SHOP_KEEPER(shop_nr)].vnum);
		sprintf(END_OF(buf2), "%s   %3.2f   %3.2f   %3.2f  ", buf1,
			SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr), SHOP_CHANGEPROFIT(shop_nr));
//       strcat(buf2, customer_string(shop_nr, FALSE));
		sprintf(END_OF(buf2), "%s\r\n", customer_string(shop_nr, FALSE));
//       sprintf(END_OF(buf), "%s\r\n", buf2);
		buffer = str_add(buffer, buf2);
	}
//  page_string(ch->desc, buf, 1);
	page_string(ch->desc, buffer, 1);
	free(buffer);
}


void handle_detailed_list(char *buf, char *buf1, CHAR_DATA * ch)
{
	if ((strlen(buf1) + strlen(buf) < 78) || (strlen(buf) < 20))
		strcat(buf, buf1);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		sprintf(buf, "          %s", buf1);
	}
}


void list_detailed_shop(CHAR_DATA * ch, int shop_nr)
{
	OBJ_DATA *obj;
	CHAR_DATA *k;
	int index, temp;

	sprintf(buf, "Vnum      : [%5d], Rnum: [%5d]\r\n", SHOP_NUM(shop_nr), shop_nr + 1);
	send_to_char(buf, ch);

	strcpy(buf, "Комнаты  :      ");
	for (index = 0; SHOP_ROOM(shop_nr, index) != NOWHERE; index++) {
		if (index)
			strcat(buf, ", ");
		if ((temp = real_room(SHOP_ROOM(shop_nr, index))) != NOWHERE)
			sprintf(buf1, "%s (#%d)", world[temp]->name, GET_ROOM_VNUM(temp));
		else
			sprintf(buf1, "<Неизвестно> (#%d)", SHOP_ROOM(shop_nr, index));
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)
		send_to_char("Комнаты   : Нет!\r\n", ch);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	strcpy(buf, "Продавец  : ");
	if (SHOP_KEEPER(shop_nr) >= 0) {
		sprintf(END_OF(buf), "%s (#%d), Special Function: %s\r\n",
			GET_NAME(&mob_proto[SHOP_KEEPER(shop_nr)]),
			mob_index[SHOP_KEEPER(shop_nr)].vnum, YESNO(SHOP_FUNC(shop_nr)));
		if ((k = get_char_num(SHOP_KEEPER(shop_nr)))) {
			send_to_char(buf, ch);
			sprintf(buf, "Денег     :  [%9d], в банке : [%9d] (всего: %d)\r\n",
				GET_GOLD(k), SHOP_BANK(shop_nr), GET_GOLD(k) + SHOP_BANK(shop_nr));
		}
	} else
		strcat(buf, "<НЕТ>\r\n");
	send_to_char(buf, ch);

	strcpy(buf1, customer_string(shop_nr, TRUE));
	sprintf(buf, "Торгует с:  %s\r\n", (*buf1) ? buf1 : "Нет");
	send_to_char(buf, ch);

	strcpy(buf, "Производит    : ");
	for (index = 0; SHOP_PRODUCT(shop_nr, index) != NOTHING; index++) {
		obj = &obj_proto[SHOP_PRODUCT(shop_nr, index)];
		if (index)
			strcat(buf, ", ");
		sprintf(buf1, "%s (#%d)", obj->short_description, obj_index[SHOP_PRODUCT(shop_nr, index)].vnum);
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)
		send_to_char("Производит: Ничего!\r\n", ch);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	strcpy(buf, "Покупает      : ");
	for (index = 0; SHOP_BUYTYPE(shop_nr, index) != NOTHING; index++) {
		if (index)
			strcat(buf, ", ");
		sprintf(buf1, "%s (#%d) ", item_types[SHOP_BUYTYPE(shop_nr, index)], SHOP_BUYTYPE(shop_nr, index));
		if (SHOP_BUYWORD(shop_nr, index))
			sprintf(END_OF(buf1), "[%s]", SHOP_BUYWORD(shop_nr, index));
		else
			strcat(buf1, "[all]");
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)
		send_to_char("Покупает  : Ничего!\r\n", ch);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	strcpy(buf, "Меняет        : ");
	for (index = 0; SHOP_CHANGETYPE(shop_nr, index) != NOTHING; index++) {
		if (index)
			strcat(buf, ", ");
		sprintf(buf1, "%s (#%d) ",
			item_types[SHOP_CHANGETYPE(shop_nr, index)], SHOP_CHANGETYPE(shop_nr, index));
		if (SHOP_CHANGEWORD(shop_nr, index))
			sprintf(END_OF(buf1), "[%s]", SHOP_CHANGEWORD(shop_nr, index));
		else
			strcat(buf1, "[all]");
		handle_detailed_list(buf, buf1, ch);
	}
	if (!index)
		send_to_char("Меняет    : Нет обмена!\r\n", ch);
	else {
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	sprintf(buf,
		"К.покупки:[%4.2f], К.обмена:[%4.2f], К.продажи:[%4.2f], открыт: [%d-%d, %d-%d]%s",
		SHOP_SELLPROFIT(shop_nr), SHOP_CHANGEPROFIT(shop_nr),
		SHOP_BUYPROFIT(shop_nr), SHOP_OPEN1(shop_nr),
		SHOP_CLOSE1(shop_nr), SHOP_OPEN2(shop_nr), SHOP_CLOSE2(shop_nr), "\r\n");

	send_to_char(buf, ch);

	sprintbit((long) SHOP_BITVECTOR(shop_nr), shop_bits, buf1);
	sprintf(buf, "Биты:       %s\r\n", buf1);
	send_to_char(buf, ch);
}


void show_shops(CHAR_DATA * ch, char *arg)
{
	int shop_nr;

	if (!*arg)
		list_all_shops(ch);
	else {
		if (!str_cmp(arg, ".")) {
			for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
				if (ok_shop_room(shop_nr, GET_ROOM_VNUM(IN_ROOM(ch))))
					break;

			if (shop_nr > top_shop) {
				send_to_char("Это не магазин !\r\n", ch);
				return;
			}
		} else if (is_number(arg))
			shop_nr = atoi(arg) - 1;
		else
			shop_nr = -1;

		if (shop_nr < 0 || shop_nr > top_shop) {
			send_to_char("Неверный номер магазина.\r\n", ch);
			return;
		}
		list_detailed_shop(ch, shop_nr);
	}
}
