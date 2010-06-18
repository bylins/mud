/* ************************************************************************
*   File: shop.h                                        Part of Bylins    *
*  Usage: shop file definitions, structures, constants                    *
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

#ifndef _SHOP_H_
#define _SHOP_H_

#include <boost/array.hpp>

struct shop_buy_data
{
	int type;
	char *keywords;
};

#define BUY_TYPE(i)		((i).type)
#define BUY_WORD(i)		((i).keywords)


struct shop_data
{
	room_vnum vnum;		/* Virtual number of this shop         */
	obj_vnum *producing;	/* Which item to produce (virtual)      */
	float profit_buy;	/* Factor to multiply cost with         */
	float profit_sell;	/* Factor to multiply cost with         */
	float profit_change;	/* Factor to multiple cost with     */
	struct shop_buy_data *type;	/* Which items to trade                        */
	struct shop_buy_data *change;	/* Which items to change           */
	char *no_such_item1;	/* Message if keeper hasn't got an item */
	char *no_such_item2;	/* Message if player hasn't got an item */
	char *missing_cash1;	/* Message if keeper hasn't got cash    */
	char *missing_cash2;	/* Message if player hasn't got cash    */
	char *do_not_buy;	/* If keeper dosn't buy such things     */
	char *message_buy;	/* Message when player buys item        */
	char *message_sell;	/* Message when player sells item       */
	int temper1;		/* How does keeper react if no money */
	bitvector_t bitvector;	/* Can attack? Use bank? Cast here?     */
	mob_rnum keeper;	/* The mobile who owns the shop (rnum) */
	int with_who;		/* Who does the shop trade with? */
	room_rnum *in_room;	/* Where is the shop?                     */
	int open1, open2;	/* When does the shop open?               */
	int close1, close2;	/* When does the shop close?      */
	int bankAccount;	/* Store all gold over 15000 (disabled) */
	int lastsort;		/* How many items are sorted in inven?      */
	SPECIAL(*func);	/* Secondary spec_proc for shopkeeper       */
};

#define MODE_TRADE        0
#define MODE_CHANGE       1
#define MODE_REPAIR       2

#define OPER_CHANGE_VALUE 0
#define OPER_CHANGE_DONE  1

#define MAX_TRADE	    5	/* List maximums for compatibility      */
#define MAX_PROD	    5	/*      with shops before v3.0          */
#define VERSION3_TAG	"v3.0"	/* The file has v3.0 shops in it!       */
#define MAX_SHOP_OBJ	100	/* "Soft" maximum for list maximums     */


/* Pretty general macros that could be used elsewhere */
#define GET_OBJ_NUM(obj)	(obj->item_number)
#define END_OF(buffer)		((buffer) + strlen((buffer)))


/* Possible states for objects trying to be sold */
#define OBJECT_DEAD		0
#define OBJECT_NOTOK	1
#define OBJECT_OK		2
#define OBJECT_NOVAL	3
#define OBJECT_NOTEMPTY 4


/* Types of lists to read */
#define LIST_PRODUCE		0
#define LIST_TRADE		1
#define LIST_ROOM		2


/* Whom will we not trade with (bitvector for SHOP_TRADE_WITH()) */
#define TRADE_NOMONO		(1 << 0)
#define TRADE_NOPOLY		(1 << 1)
#define TRADE_NONEUTRAL		(1 << 2)
#define TRADE_NOMAGIC_USER	(1 << 3)
#define TRADE_NOCLERIC		(1 << 4)
#define TRADE_NOTHIEF		(1 << 5)
#define TRADE_NOWARRIOR		(1 << 6)
#define TRADE_NOASSASINE	(1 << 7)
#define TRADE_NOGUARD		(1 << 8)
#define TRADE_NOPALADINE	(1 << 9)
#define TRADE_NORANGER		(1 << 10)
#define TRADE_NOSMITH		(1 << 11)
#define TRADE_NOMERCHANT	(1 << 12)
#define TRADE_NODRUID		(1 << 13)


struct stack_data
{
	boost::array<int, 100> data;
	int len;
};

#define S_DATA(stack, index)	((stack)->data[(index)])
#define S_LEN(stack)		((stack)->len)


/* Which expression type we are now parsing */
#define OPER_OPEN_PAREN		0
#define OPER_CLOSE_PAREN	1
#define OPER_OR			2
#define OPER_AND		3
#define OPER_NOT		4
#define MAX_OPER		4


#define SHOP_NUM(i)		(shop_index[(i)].vnum)
#define SHOP_KEEPER(i)		(shop_index[(i)].keeper)
#define SHOP_OPEN1(i)		(shop_index[(i)].open1)
#define SHOP_CLOSE1(i)		(shop_index[(i)].close1)
#define SHOP_OPEN2(i)		(shop_index[(i)].open2)
#define SHOP_CLOSE2(i)		(shop_index[(i)].close2)
#define SHOP_ROOM(i, num)	(shop_index[(i)].in_room[(num)])
#define SHOP_BUYTYPE(i, num)	(BUY_TYPE(shop_index[(i)].type[(num)]))
#define SHOP_BUYWORD(i, num)	(BUY_WORD(shop_index[(i)].type[(num)]))
#define SHOP_CHANGETYPE(i, num)	(BUY_TYPE(shop_index[(i)].change[(num)]))
#define SHOP_CHANGEWORD(i, num)	(BUY_WORD(shop_index[(i)].change[(num)]))
#define SHOP_PRODUCT(i, num)	(shop_index[(i)].producing[(num)])
#define SHOP_BANK(i)		(shop_index[(i)].bankAccount)
#define SHOP_BROKE_TEMPER(i)	(shop_index[(i)].temper1)
#define SHOP_BITVECTOR(i)	(shop_index[(i)].bitvector)
#define SHOP_TRADE_WITH(i)	(shop_index[(i)].with_who)
#define SHOP_SORT(i)		(shop_index[(i)].lastsort)
#define SHOP_BUYPROFIT(i)	(shop_index[(i)].profit_buy)
#define SHOP_SELLPROFIT(i)	(shop_index[(i)].profit_sell)
#define SHOP_CHANGEPROFIT(i)(shop_index[(i)].profit_change)
#define SHOP_FUNC(i)		(shop_index[(i)].func)

#define NOTRADE_MONO(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOMONO))
#define NOTRADE_POLY(i)		(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOPOLY))
#define NOTRADE_NEUTRAL(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NONEUTRAL))
#define NOTRADE_MAGIC_USER(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOMAGIC_USER))
#define NOTRADE_CLERIC(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOCLERIC))
#define NOTRADE_THIEF(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOTHIEF))
#define NOTRADE_WARRIOR(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOWARRIOR))
#define NOTRADE_ASSASINE(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOASSASINE))
#define NOTRADE_GUARD(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOGUARD))
#define NOTRADE_PALADINE(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOPALADINE))
#define NOTRADE_RANGER(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NORANGER))
#define NOTRADE_SMITH(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOSMITH))
#define NOTRADE_MERCHANT(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NOMERCHANT))
#define NOTRADE_DRUID(i)	(IS_SET(SHOP_TRADE_WITH((i)), TRADE_NODRUID))



#define	WILL_START_FIGHT	1
#define WILL_BANK_MONEY		2

#define SHOP_KILL_CHARS(i)	(IS_SET(SHOP_BITVECTOR(i), WILL_START_FIGHT))
#define SHOP_USES_BANK(i)	(IS_SET(SHOP_BITVECTOR(i), WILL_BANK_MONEY))


#define MIN_OUTSIDE_BANK	5000
#define MAX_OUTSIDE_BANK	15000

#define MSG_NOT_OPEN_YET	"Приходите позже!"
#define MSG_NOT_REOPEN_YET	"Извините, но мы еще не открылись, приходите позже."
#define MSG_CLOSED_FOR_DAY	"Извините, приходите завтра."
#define MSG_NO_STEAL_HERE	"$n, грязн$w воришка, чеши отседова!"
#define MSG_NO_SEE_CHAR		"Изыди отсюда, голос нечистый!"
#define MSG_NO_SELL_ALIGN	"Уходи лучше, пока я не вызвал стражу!"
#define MSG_NO_SELL_CLASS	"Я не торгую с такими особами!"
#define MSG_NO_USED_WANDSTAFF	"Я не покупаю использованные вещи!"
#define MSG_CANT_KILL_KEEPER	"Ступай отсюда, пока не пришли стражи!"

int transaction_amt(char *arg);

#endif
