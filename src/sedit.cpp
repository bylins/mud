/************************************************************************
 * OasisOLC - sedit.c						v1.5	*
 * Copyright 1996 Harvey Gilpin.					*
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "olc.h"
#include "char.hpp"

/*-------------------------------------------------------------------*/

/*
 * External variable declarations.
 */
extern SHOP_DATA *shop_index;
extern int top_shop;
extern CHAR_DATA *mob_proto;
extern vector < OBJ_DATA * >obj_proto;

extern struct zone_data *zone_table;
extern INDEX_DATA *mob_index;
extern INDEX_DATA *obj_index;
extern const char *trade_letters[];
extern const char *shop_bits[];
extern const char *item_types[];

/*-------------------------------------------------------------------*/

/*
 * Handy macros.
 */
#define S_NUM(i)		((i)->vnum)
#define S_KEEPER(i)		((i)->keeper)
#define S_OPEN1(i)		((i)->open1)
#define S_CLOSE1(i)		((i)->close1)
#define S_OPEN2(i)		((i)->open2)
#define S_CLOSE2(i)		((i)->close2)
#define S_BANK(i)		((i)->bankAccount)
#define S_BROKE_TEMPER(i)	((i)->temper1)
#define S_BITVECTOR(i)		((i)->bitvector)
#define S_NOTRADE(i)		((i)->with_who)
#define S_SORT(i)		((i)->lastsort)
#define S_BUYPROFIT(i)		((i)->profit_buy)
#define S_SELLPROFIT(i)		((i)->profit_sell)
#define S_CHANGEPROFIT(i)		((i)->profit_change)
#define S_FUNC(i)		((i)->func)

#define S_ROOMS(i)		((i)->in_room)
#define S_PRODUCTS(i)		((i)->producing)
#define S_NAMELISTS(i)		((i)->type)
#define S_CHANGELISTS(i)     ((i)->change)
#define S_ROOM(i, num)		((i)->in_room[(num)])
#define S_PRODUCT(i, num)	((i)->producing[(num)])
#define S_BUYTYPE(i, num)	(BUY_TYPE((i)->type[(num)]))
#define S_BUYWORD(i, num)	(BUY_WORD((i)->type[(num)]))
#define S_CHANGETYPE(i, num)	(BUY_TYPE((i)->change[(num)]))
#define S_CHANGEWORD(i, num)	(BUY_WORD((i)->change[(num)]))


#define S_NOITEM1(i)		((i)->no_such_item1)
#define S_NOITEM2(i)		((i)->no_such_item2)
#define S_NOCASH1(i)		((i)->missing_cash1)
#define S_NOCASH2(i)		((i)->missing_cash2)
#define S_NOBUY(i)		((i)->do_not_buy)
#define S_BUY(i)		((i)->message_buy)
#define S_SELL(i)		((i)->message_sell)

/*-------------------------------------------------------------------*/

/*
 * Function prototypes.
 */
int real_shop(int vshop_num);
void sedit_setup_new(DESCRIPTOR_DATA * d);
void sedit_setup_existing(DESCRIPTOR_DATA * d, int rmob_num);
void sedit_parse(DESCRIPTOR_DATA * d, char *arg);
void sedit_disp_menu(DESCRIPTOR_DATA * d);
void sedit_namelist_menu(DESCRIPTOR_DATA * d);
void sedit_types_menu(DESCRIPTOR_DATA * d, int mode);
void sedit_products_menu(DESCRIPTOR_DATA * d);
void sedit_rooms_menu(DESCRIPTOR_DATA * d);
void sedit_compact_rooms_menu(DESCRIPTOR_DATA * d);
void sedit_shop_flags_menu(DESCRIPTOR_DATA * d);
void sedit_no_trade_menu(DESCRIPTOR_DATA * d);
void sedit_save_internally(DESCRIPTOR_DATA * d);
void sedit_save_to_disk(int zone);
void copy_shop(SHOP_DATA * tshop, SHOP_DATA * fshop);
void copy_list(int **tlist, int *flist);
void copy_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist);
void sedit_add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *lnew);
void sedit_remove_from_type_list(struct shop_buy_data **list, int num);
void free_shop_strings(SHOP_DATA * shop);
void free_type_list(struct shop_buy_data **list);
void free_shop(SHOP_DATA * shop);
void sedit_modify_string(char **str, char *lnew);

/*
 * External functions.
 */
SPECIAL(shop_keeper);

/*-------------------------------------------------------------------*\
  utility functions
\*-------------------------------------------------------------------*/

void sedit_setup_new(DESCRIPTOR_DATA * d)
{
	SHOP_DATA *shop;
	/*
	 * Allocate a scratch shop structure.
	 */
	CREATE(shop, SHOP_DATA, 1);

	/*
	 * Fill in some default values.
	 */
	S_KEEPER(shop) = -1;
	S_CLOSE1(shop) = 28;
	S_BUYPROFIT(shop) = 1.0;
	S_SELLPROFIT(shop) = 1.0;
	S_CHANGEPROFIT(shop) = 1.0;
	/*
	 * Add a spice of default strings.
	 */
	S_NOITEM1(shop) = str_dup("%s Извини, у меня этого нет.");
	S_NOITEM2(shop) = str_dup("%s У тебя ведь нет этого.");
	S_NOCASH1(shop) = str_dup("%s Я не желаю иметь дела с этим!");
	S_NOCASH2(shop) = str_dup("%s У тебя нет столько денег!");
	S_NOBUY(shop) = str_dup("%s Я не торгую подобными вещами.");
	S_BUY(shop) = str_dup("%s Вот тебе %d.");
	S_SELL(shop) = str_dup("%s Пожалуй, я заплачу за это %d.");
	/*
	 * Stir the lists lightly.
	 */
	CREATE(S_PRODUCTS(shop), obj_vnum, 1);
	S_PRODUCT(shop, 0) = -1;

	CREATE(S_ROOMS(shop), room_vnum, 1);
	S_ROOM(shop, 0) = -1;

	CREATE(S_NAMELISTS(shop), struct shop_buy_data, 1);
	S_BUYTYPE(shop, 0) = -1;

	CREATE(S_CHANGELISTS(shop), struct shop_buy_data, 1);
	S_CHANGETYPE(shop, 0) = -1;
	/*
	 * Presto! A shop.
	 */
	OLC_SHOP(d) = shop;
	sedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void sedit_setup_existing(DESCRIPTOR_DATA * d, int rshop_num)
{
	/*
	 * Create a scratch shop structure.
	 */
	CREATE(OLC_SHOP(d), SHOP_DATA, 1);

	copy_shop(OLC_SHOP(d), shop_index + rshop_num);
	sedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void copy_shop(SHOP_DATA * tshop, SHOP_DATA * fshop)
{
	/*
	 * Copy basic information over.
	 */
	S_NUM(tshop) = S_NUM(fshop);
	S_KEEPER(tshop) = S_KEEPER(fshop);
	S_OPEN1(tshop) = S_OPEN1(fshop);
	S_CLOSE1(tshop) = S_CLOSE1(fshop);
	S_OPEN2(tshop) = S_OPEN2(fshop);
	S_CLOSE2(tshop) = S_CLOSE2(fshop);
	S_BANK(tshop) = S_BANK(fshop);
	S_BROKE_TEMPER(tshop) = S_BROKE_TEMPER(fshop);
	S_BITVECTOR(tshop) = S_BITVECTOR(fshop);
	S_NOTRADE(tshop) = S_NOTRADE(fshop);
	S_SORT(tshop) = S_SORT(fshop);
	S_BUYPROFIT(tshop) = S_BUYPROFIT(fshop);
	S_SELLPROFIT(tshop) = S_SELLPROFIT(fshop);
	S_CHANGEPROFIT(tshop) = S_CHANGEPROFIT(fshop);
	S_FUNC(tshop) = S_FUNC(fshop);

	/*
	 * Copy lists over.
	 */
	copy_list((int **) &(S_ROOMS(tshop)), (int *) S_ROOMS(fshop));
	copy_list((int **) &(S_PRODUCTS(tshop)), (int *) S_PRODUCTS(fshop));
	copy_type_list(&(tshop->type), fshop->type);
	copy_type_list(&(tshop->change), fshop->change);

	/*
	 * Copy notification strings over.
	 */
	free_shop_strings(tshop);
	S_NOITEM1(tshop) = str_dup(S_NOITEM1(fshop));
	S_NOITEM2(tshop) = str_dup(S_NOITEM2(fshop));
	S_NOCASH1(tshop) = str_dup(S_NOCASH1(fshop));
	S_NOCASH2(tshop) = str_dup(S_NOCASH2(fshop));
	S_NOBUY(tshop) = str_dup(S_NOBUY(fshop));
	S_BUY(tshop) = str_dup(S_BUY(fshop));
	S_SELL(tshop) = str_dup(S_SELL(fshop));

}

/*-------------------------------------------------------------------*/

/*
 * Copy a -1 terminated integer array list.
 */
void copy_list(int **tlist, int *flist)
{
	int num_items, i;

	if (*tlist)
		free(*tlist);

	/*
	 * Count number of entries.
	 */
	for (i = 0; flist[i] != -1; i++);
	num_items = i + 1;

	/*
	 * Make space for entries.
	 */
	CREATE(*tlist, int, num_items);

	/*
	 * Copy entries over.
	 */
	i = 0;
	do
	{
		(*tlist)[i] = flist[i];
	}
	while (++i < num_items);
}

/*-------------------------------------------------------------------*/

/*
 * Copy a -1 terminated (in the type field) shop_buy_data
 * array list.
 */
void copy_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist)
{
	int num_items, i;

	if (*tlist)
		free_type_list(tlist);

	/*
	 * Count number of entries.
	 */
	for (i = 0; BUY_TYPE(flist[i]) != -1; i++)
		log("%d", flist[i].type);

	num_items = i + 1;

	/*
	 * Make space for entries.
	 */
	CREATE(*tlist, struct shop_buy_data, num_items);

	/*
	 * Copy entries over.
	 */

	i = 0;
	do
	{
		(*tlist)[i].type = flist[i].type;

		if (BUY_WORD(flist[i]))
			BUY_WORD((*tlist)[i]) = str_dup(BUY_WORD(flist[i]));
		else
			BUY_WORD((*tlist)[i]) = NULL;
	}
	while (++i < num_items);

}

/*-------------------------------------------------------------------*/

void sedit_remove_from_type_list(struct shop_buy_data **list, int num)
{
	int i, num_items;
	struct shop_buy_data *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i].type != -1; i++);

	if (num >= i || num < 0)
		return;
	num_items = i;

	CREATE(nlist, struct shop_buy_data, num_items);

	for (i = 0; i < num_items; i++)
		nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

	free(BUY_WORD((*list)[num]));
	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

void sedit_add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *lnew)
{
	int i, num_items;
	struct shop_buy_data *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i].type != -1; i++);
	num_items = i;

	/*
	 * Make a new list and slot in the new entry.
	 */
	CREATE(nlist, struct shop_buy_data, num_items + 2);

	for (i = 0; i < num_items; i++)
		nlist[i] = (*list)[i];
	nlist[num_items] = *lnew;
	nlist[num_items + 1].type = -1;

	/*
	 * Out with the old, in with the new.
	 */
	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

void sedit_add_to_int_list(int **list, int lnew)
{
	int i, num_items, *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i] != -1; i++);
	num_items = i;

	/*
	 * Make a new list and slot in the new entry.
	 */
	CREATE(nlist, int, num_items + 2);

	for (i = 0; i < num_items; i++)
		nlist[i] = (*list)[i];
	nlist[num_items] = lnew;
	nlist[num_items + 1] = -1;

	/*
	 * Out with the old, in with the new.
	 */
	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

void sedit_remove_from_int_list(int **list, int num)
{
	int i, num_items, *nlist;

	/*
	 * Count number of entries.
	 */
	for (i = 0; (*list)[i] != -1; i++);

	if (num >= i || num < 0)
		return;
	num_items = i;

	CREATE(nlist, int, num_items);

	for (i = 0; i < num_items; i++)
		nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

	free(*list);
	*list = nlist;
}

/*-------------------------------------------------------------------*/

/*
 * Free all the notice character strings in a shop structure.
 */
void free_shop_strings(SHOP_DATA * shop)
{
	if (S_NOITEM1(shop))
	{
		free(S_NOITEM1(shop));
		S_NOITEM1(shop) = NULL;
	}
	if (S_NOITEM2(shop))
	{
		free(S_NOITEM2(shop));
		S_NOITEM2(shop) = NULL;
	}
	if (S_NOCASH1(shop))
	{
		free(S_NOCASH1(shop));
		S_NOCASH1(shop) = NULL;
	}
	if (S_NOCASH2(shop))
	{
		free(S_NOCASH2(shop));
		S_NOCASH2(shop) = NULL;
	}
	if (S_NOBUY(shop))
	{
		free(S_NOBUY(shop));
		S_NOBUY(shop) = NULL;
	}
	if (S_BUY(shop))
	{
		free(S_BUY(shop));
		S_BUY(shop) = NULL;
	}
	if (S_SELL(shop))
	{
		free(S_SELL(shop));
		S_SELL(shop) = NULL;
	}
}

/*-------------------------------------------------------------------*/

/*
 * Free a type list and all the strings it contains.
 */
void free_type_list(struct shop_buy_data **list)
{
	int i;

	for (i = 0; (*list)[i].type != -1; i++)
		if (BUY_WORD((*list)[i]))
			free(BUY_WORD((*list)[i]));
	free(*list);
	*list = NULL;
}

/*-------------------------------------------------------------------*/

/*
 * Free up the whole shop structure and it's content.
 */
void free_shop(SHOP_DATA * shop)
{
	free_shop_strings(shop);
	free_type_list(&(S_NAMELISTS(shop)));
	free_type_list(&(S_CHANGELISTS(shop)));
	free(S_ROOMS(shop));
	free(S_PRODUCTS(shop));
	free(shop);
}

/*-------------------------------------------------------------------*/

int real_shop(int vshop_num)
{
	int rshop_num;

	for (rshop_num = 0; rshop_num < top_shop; rshop_num++)
		if (SHOP_NUM(rshop_num) == vshop_num)
			return rshop_num;

	return -1;
}

/*-------------------------------------------------------------------*/

/*
 * Generic string modifyer for shop keeper messages.
 */
void sedit_modify_string(char **str, char *lnew)
{
	char *pointer;

	/*
	 * Check the '%s' is present, if not, add it.
	 */
	if (*lnew != '%')
	{
		strcpy(buf, "%s ");
		strcat(buf, lnew);
		pointer = buf;
	}
	else
		pointer = lnew;

	if (*str)
		free(*str);
	*str = str_dup(pointer);
}

/*-------------------------------------------------------------------*/

void sedit_save_internally(DESCRIPTOR_DATA * d)
{
	int rshop, found = 0;
	SHOP_DATA *shop;
	SHOP_DATA *new_index;

	rshop = real_shop(OLC_NUM(d));
	shop = OLC_SHOP(d);
	S_NUM(shop) = OLC_NUM(d);

	if (rshop > -1)  	/* The shop already exists, just update it. */
	{
		copy_shop((shop_index + rshop), shop);
	}
	else  		/* Doesn't exist - have to insert it. */
	{
		CREATE(new_index, SHOP_DATA, top_shop + 1);

		for (rshop = 0; rshop < top_shop; rshop++)
		{
			if (!found)  	/* Is this the place? */
			{
				if (SHOP_NUM(rshop) > OLC_NUM(d))  	/* Yep, stick it in here. */
				{
					found = 1;
					copy_shop(&(new_index[rshop]), shop);
					/*
					 * Move the entry that used to go here up a place.
					 */
					new_index[rshop + 1] = shop_index[rshop];
				}
				else
					/* This isn't the place, copy over info. */
					new_index[rshop] = shop_index[rshop];
			}
			else  	/* Shop's already inserted, copy rest over. */
			{
				new_index[rshop + 1] = shop_index[rshop];
			}
		}
		if (!found)
			copy_shop(&(new_index[rshop]), shop);

		/*
		 * Switch the new index in.
		 */
		free(shop_index);
		shop_index = new_index;
		top_shop++;
	}
	olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_SHOP);
}

/*-------------------------------------------------------------------*/

void sedit_save_to_disk(int zone_num)
{
	int i, j, rshop, zone, top;
	FILE *shop_file;
	char fname[64];
	SHOP_DATA *shop;

	zone = zone_table[zone_num].number;
	top = zone_table[zone_num].top;

	sprintf(fname, "%s/%d.new", SHP_PREFIX, zone);
	if (!(shop_file = fopen(fname, "w")))
	{
		mudlog("SYSERR: OLC: Cannot open shop file!", BRF, LVL_BUILDER, SYSLOG, TRUE);
		return;
	}
	else if (fprintf(shop_file, "CircleMUD v3.0 Shop File~\n") < 0)
	{
		mudlog("SYSERR: OLC: Cannot write to shop file!", BRF, LVL_BUILDER, SYSLOG, TRUE);
		fclose(shop_file);
		return;
	}
	/*
	 * Search database for shops in this zone.
	 */
	for (i = zone * 100; i <= top; i++)
	{
		if ((rshop = real_shop(i)) != -1)
		{
			shop = shop_index + rshop;
			fprintf(shop_file, "#%d%s~\n", i, (S_CHANGELISTS(shop)
											   && S_CHANGETYPE(shop, 0) != -1) ? " E" : "");
			/*
			 * Save the products.
			 */
			for (j = 0; S_PRODUCT(shop, j) != -1; j++)
				fprintf(shop_file, "%d\n", obj_index[S_PRODUCT(shop, j)].vnum);

			/*
			 * Save the rates.
			 */
			sprintf(buf, "OLC: <%f-%f> (shop)", S_BUYPROFIT(shop), S_SELLPROFIT(shop));
			olc_log("OLC: <%f-%f> (shop)", S_BUYPROFIT(shop), S_SELLPROFIT(shop));
			mudlog(buf, NRM, LVL_BUILDER, SYSLOG, TRUE);
			fprintf(shop_file, "-1\n%1.2f\n%1.2f\n", S_BUYPROFIT(shop), S_SELLPROFIT(shop));
			if (S_CHANGELISTS(shop) && S_CHANGETYPE(shop, 0) != -1)
				fprintf(shop_file, "%1.2f\n", S_CHANGEPROFIT(shop));


			/*
			 * Save the buy types and namelists.
			 */
			j = -1;
			do
			{
				j++;
				fprintf(shop_file, "%d%s\n", S_BUYTYPE(shop, j),
						S_BUYWORD(shop, j) ? S_BUYWORD(shop, j) : "");
			}
			while (S_BUYTYPE(shop, j) != -1);

			if (S_CHANGELISTS(shop) && S_CHANGETYPE(shop, 0) != -1)
			{
				j = -1;
				do
				{
					j++;
					fprintf(shop_file, "%d%s\n", S_CHANGETYPE(shop, j),
							S_CHANGEWORD(shop, j) ? S_CHANGEWORD(shop, j) : "");
				}
				while (S_CHANGETYPE(shop, j) != -1);
			}
			/*
			 * Save messages'n'stuff.
			 * Added some small'n'silly defaults as sanity checks.
			 */
			fprintf(shop_file,
					"%s~\n%s~\n%s~\n%s~\n%s~\n%s~\n%s~\n"
					"%d\n%ld\n%d\n%d\n",
					S_NOITEM1(shop) ? S_NOITEM1(shop) : "%s Кхм ?!",
					S_NOITEM2(shop) ? S_NOITEM2(shop) : "%s Кхм ?!",
					S_NOBUY(shop) ? S_NOBUY(shop) : "%s Кхм?!",
					S_NOCASH1(shop) ? S_NOCASH1(shop) : "%s Кхм?!",
					S_NOCASH2(shop) ? S_NOCASH2(shop) : "%s Кхм?!",
					S_BUY(shop) ? S_BUY(shop) : "%s Кхм?! %d ?",
					S_SELL(shop) ? S_SELL(shop) : "%s Кхм?! %d?",
					S_BROKE_TEMPER(shop),
					S_BITVECTOR(shop), mob_index[S_KEEPER(shop)].vnum, S_NOTRADE(shop));

			/*
			 * Save the rooms.
			 */
			j = -1;
			do
			{
				j++;
				fprintf(shop_file, "%d\n", S_ROOM(shop, j));
			}
			while (S_ROOM(shop, j) != -1);

			/*
			 * Save open/closing times
			 */
			fprintf(shop_file, "%d\n%d\n%d\n%d\n", S_OPEN1(shop),
					S_CLOSE1(shop), S_OPEN2(shop), S_CLOSE2(shop));
		}
	}
	fprintf(shop_file, "$\n$~\n");
	fclose(shop_file);
	sprintf(buf2, "%s/%d.shp", SHP_PREFIX, zone);
	/*
	 * We're fubar'd if we crash between the two lines below.
	 */
	remove(buf2);
	rename(fname, buf2);

	olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_SHOP);
}

/**************************************************************************
 Menu functions
 **************************************************************************/

void sedit_products_menu(DESCRIPTOR_DATA * d)
{
	SHOP_DATA *shop;
	int i;

	shop = OLC_SHOP(d);
	get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	send_to_char("##     VNUM     Производит\r\n", d->character);
	for (i = 0; S_PRODUCT(shop, i) != -1; i++)
	{
		sprintf(buf, "%2d - [%s%5d%s] - %s%s%s\r\n", i,
				cyn, obj_index[S_PRODUCT(shop, i)].vnum, nrm,
				yel, obj_proto[S_PRODUCT(shop, i)]->short_description, nrm);
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n"
			"%sA%s) Добавить предмет.\r\n"
			"%sD%s) Убрать предмет.\r\n" "%sQ%s) Выход\r\n" "Ваш выбор : ", grn, nrm, grn, nrm, grn, nrm);
	send_to_char(buf, d->character);

	OLC_MODE(d) = SEDIT_PRODUCTS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_compact_rooms_menu(DESCRIPTOR_DATA * d)
{
	SHOP_DATA *shop;
	int i, count = 0;

	shop = OLC_SHOP(d);
	get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (i = 0; S_ROOM(shop, i) != -1; i++)
	{
		sprintf(buf, "%2d - [%s%5d%s]  | %s", i, cyn, S_ROOM(shop, i), nrm, !(++count % 5) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n"
			"%sA%s) Добавить комнату.\r\n"
			"%sD%s) Удалить комнату.\r\n"
			"%sL%s) Описание комнаты.\r\n"
			"%sQ%s) Выход\r\n" "Ваш выбор : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm);
	send_to_char(buf, d->character);

	OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_rooms_menu(DESCRIPTOR_DATA * d)
{
	SHOP_DATA *shop;
	int i;

	shop = OLC_SHOP(d);
	get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	send_to_char("##     VNUM     Комната\r\n\r\n", d->character);
	for (i = 0; S_ROOM(shop, i) != -1; i++)
	{
		sprintf(buf, "%2d - [%s%5d%s] - %s%s%s\r\n", i, cyn, S_ROOM(shop, i),
				nrm, yel, world[real_room(S_ROOM(shop, i))]->name, nrm);
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n"
			"%sA%s) Добавить новую комнату.\r\n"
			"%sD%s) Удалить комнату.\r\n"
			"%sC%s) Компактное описание.\r\n"
			"%sQ%s) Выход\r\n" "Ваш выбор : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm);
	send_to_char(buf, d->character);

	OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_namelist_menu(DESCRIPTOR_DATA * d)
{
	SHOP_DATA *shop;
	int i;

	shop = OLC_SHOP(d);
	get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	send_to_char("## (ПОКУПКА)     Тип   Параметры\r\n\r\n", d->character);
	for (i = 0; S_BUYTYPE(shop, i) != -1; i++)
	{
		sprintf(buf, "%2d - %s%15s%s - %s%s%s\r\n", i, cyn,
				item_types[S_BUYTYPE(shop, i)], nrm, yel,
				S_BUYWORD(shop, i) ? S_BUYWORD(shop, i) : "<None>", nrm);
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n"
			"%sA%s) Новый товар.\r\n"
			"%sD%s) Удалить товар.\r\n" "%sQ%s) Выход\r\n" "Ваш выбор : ", grn, nrm, grn, nrm, grn, nrm);
	send_to_char(buf, d->character);
	OLC_MODE(d) = SEDIT_NAMELIST_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_changelist_menu(DESCRIPTOR_DATA * d)
{
	SHOP_DATA *shop;
	int i;

	shop = OLC_SHOP(d);
	get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	send_to_char("## (ОБМЕН)     Тип   Параметры\r\n\r\n", d->character);
	for (i = 0; S_CHANGETYPE(shop, i) != -1; i++)
	{
		sprintf(buf, "%2d - %s%15s%s - %s%s%s\r\n", i, cyn,
				item_types[S_CHANGETYPE(shop, i)], nrm, yel,
				S_CHANGEWORD(shop, i) ? S_CHANGEWORD(shop, i) : "<None>", nrm);
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n"
			"%sA%s) Новый товар.\r\n"
			"%sD%s) Удалить товар.\r\n" "%sQ%s) Выход\r\n" "Ваш выбор : ", grn, nrm, grn, nrm, grn, nrm);
	send_to_char(buf, d->character);
	OLC_MODE(d) = SEDIT_CHANGELIST_MENU;
}


/*-------------------------------------------------------------------*/

void sedit_shop_flags_menu(DESCRIPTOR_DATA * d)
{
	int i, count = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (i = 0; i < NUM_SHOP_FLAGS; i++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s   %s", grn, i + 1, nrm, shop_bits[i], !(++count % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbit(S_BITVECTOR(OLC_SHOP(d)), shop_bits, buf1);
	sprintf(buf, "\r\nТекущие флаги магазина : %s%s%s\r\nВаш выбор : ", cyn, buf1, nrm);
	send_to_char(buf, d->character);
	OLC_MODE(d) = SEDIT_SHOP_FLAGS;
}

/*-------------------------------------------------------------------*/

void sedit_no_trade_menu(DESCRIPTOR_DATA * d)
{
	int i, count = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (i = 0; i < NUM_TRADERS; i++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s   %s", grn, i + 1, nrm, trade_letters[i], !(++count % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbit(S_NOTRADE(OLC_SHOP(d)), trade_letters, buf1);
	sprintf(buf, "\r\nВ настоящее время не торгует с : %s%s%s\r\n" "Ваш выбор : ", cyn, buf1, nrm);
	send_to_char(buf, d->character);
	OLC_MODE(d) = SEDIT_NOTRADE;
}

/*-------------------------------------------------------------------*/

void sedit_types_menu(DESCRIPTOR_DATA * d, int mode)
{
	SHOP_DATA *shop;
	int i, count = 0;

	shop = OLC_SHOP(d);
	get_char_cols(d->character);

#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (i = 0; i < NUM_ITEM_TYPES; i++)
	{
		sprintf(buf, "%s%2d%s) %s%-20s%s  %s", grn, i, nrm, cyn, item_types[i],
				nrm, !(++count % 3) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n%sВаш выбор : ", nrm);
	send_to_char(buf, d->character);
	OLC_MODE(d) = mode;
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void sedit_disp_menu(DESCRIPTOR_DATA * d)
{
	SHOP_DATA *shop;

	shop = OLC_SHOP(d);
	get_char_cols(d->character);

	sprintbit(S_NOTRADE(shop), trade_letters, buf1);
	sprintbit(S_BITVECTOR(shop), shop_bits, buf2);
	sprintf(buf,
#if defined(CLEAR_SCREEN)
			"[H[J"
#endif
			"-- Магазин : [%s%d%s]\r\n"
			"%s0%s) Продавец : [%s%d%s] %s%s\r\n"
			"%s1%s) Открывается 1 : %s%4d%s          %s2%s) Закрывается 1     : %s%4d\r\n"
			"%s3%s) Открывается 2 : %s%4d%s          %s4%s) Закрывается 2     : %s%4d\r\n"
			"%s5%s) Коэф. покупки : %s%1.2f%s        %s6%s) Коэф.продажи      : %s%1.2f\r\n"
			"%s7%s) Нет у продавца  : %s%s\r\n"
			"%s8%s) Нет у покупателя: %s%s\r\n"
			"%s9%s) Нет денег у продавца: %s%s\r\n"
			"%sA%s) Нет денег у покупателя: %s%s\r\n"
			"%sB%s) Не покупает    : %s%s\r\n"
			"%sC%s) Купил          : %s%s\r\n"
			"%sD%s) Продал         : %s%s\r\n"
			"%sE%s) Не торгует     : %s%s\r\n"
			"%sF%s) Флаги магазина : %s%s\r\n"
			"%sR%s) Меню комнат\r\n"
			"%sP%s) Меню продукции\r\n"
			"%sT%s) Меню типов для покупки\r\n"
			"%sX%s) Меню типов для обмена\r\n"
			"%sY%s) Коэф.обмена    : %s%1.2f\r\n"
			"%sQ%s) Выход\r\n"
			"Ваш выбор : ",
			cyn, OLC_NUM(d), nrm,
			grn, nrm, cyn, S_KEEPER(shop) == -1 ?
			-1 : mob_index[S_KEEPER(shop)].vnum, nrm,
			yel, S_KEEPER(shop) == -1 ?
			"None" : mob_proto[S_KEEPER(shop)].player.short_descr,
			grn, nrm, cyn, S_OPEN1(shop), nrm,
			grn, nrm, cyn, S_CLOSE1(shop),
			grn, nrm, cyn, S_OPEN2(shop), nrm,
			grn, nrm, cyn, S_CLOSE2(shop),
			grn, nrm, cyn, S_BUYPROFIT(shop), nrm,
			grn, nrm, cyn, S_SELLPROFIT(shop),
			grn, nrm, yel, S_NOITEM1(shop),
			grn, nrm, yel, S_NOITEM2(shop),
			grn, nrm, yel, S_NOCASH1(shop),
			grn, nrm, yel, S_NOCASH2(shop),
			grn, nrm, yel, S_NOBUY(shop),
			grn, nrm, yel, S_BUY(shop),
			grn, nrm, yel, S_SELL(shop),
			grn, nrm, cyn, buf1,
			grn, nrm, cyn, buf2,
			grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, S_CHANGEPROFIT(shop), grn, nrm);
	send_to_char(buf, d->character);

	OLC_MODE(d) = SEDIT_MAIN_MENU;
}

/**************************************************************************
  The GARGANTUAN event handler
 **************************************************************************/

void sedit_parse(DESCRIPTOR_DATA * d, char *arg)
{
	int i;

	if (OLC_MODE(d) > SEDIT_NUMERICAL_RESPONSE)
	{
		if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1]))))
		{
			send_to_char("Field must be numerical, try again : ", d->character);
			return;
		}
	}
	switch (OLC_MODE(d))
	{
		/*-------------------------------------------------------------------*/
	case SEDIT_CONFIRM_SAVESTRING:
		switch (*arg)
		{
		case 'y':
		case 'Y':
		case 'д':
		case 'Д':
			send_to_char("Saving shop to memory.\r\n", d->character);
			sedit_save_internally(d);
			sprintf(buf, "OLC: %s edits shop %d", GET_NAME(d->character), OLC_NUM(d));
			olc_log("%s edit shop %d", GET_NAME(d->character), OLC_NUM(d));
			mudlog(buf, NRM, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
			cleanup_olc(d, CLEANUP_STRUCTS);
			return;
		case 'n':
		case 'N':
		case 'н':
		case 'Н':
			cleanup_olc(d, CLEANUP_ALL);
			return;
		default:
			send_to_char("Неверный выбор!\r\nВы желаете сохранить магазин ? : ", d->character);
			return;
		}
		break;

		/*-------------------------------------------------------------------*/
	case SEDIT_MAIN_MENU:
		i = 0;
		switch (*arg)
		{
		case 'q':
		case 'Q':
			if (OLC_VAL(d))  	/* Anything been changed? */
			{
				send_to_char("Вы желаете сохранить изменения магазина ? (y/n) : ", d->character);
				OLC_MODE(d) = SEDIT_CONFIRM_SAVESTRING;
			}
			else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case '0':
			OLC_MODE(d) = SEDIT_KEEPER;
			send_to_char("Введите виртуальный номер продавца : ", d->character);
			return;
		case '1':
			OLC_MODE(d) = SEDIT_OPEN1;
			i++;
			break;
		case '2':
			OLC_MODE(d) = SEDIT_CLOSE1;
			i++;
			break;
		case '3':
			OLC_MODE(d) = SEDIT_OPEN2;
			i++;
			break;
		case '4':
			OLC_MODE(d) = SEDIT_CLOSE2;
			i++;
			break;
		case '5':
			OLC_MODE(d) = SEDIT_BUY_PROFIT;
			i++;
			break;
		case '6':
			OLC_MODE(d) = SEDIT_SELL_PROFIT;
			i++;
			break;
		case '7':
			OLC_MODE(d) = SEDIT_NOITEM1;
			i--;
			break;
		case '8':
			OLC_MODE(d) = SEDIT_NOITEM2;
			i--;
			break;
		case '9':
			OLC_MODE(d) = SEDIT_NOCASH1;
			i--;
			break;
		case 'a':
		case 'A':
			OLC_MODE(d) = SEDIT_NOCASH2;
			i--;
			break;
		case 'b':
		case 'B':
			OLC_MODE(d) = SEDIT_NOBUY;
			i--;
			break;
		case 'c':
		case 'C':
			OLC_MODE(d) = SEDIT_BUY;
			i--;
			break;
		case 'd':
		case 'D':
			OLC_MODE(d) = SEDIT_SELL;
			i--;
			break;
		case 'e':
		case 'E':
			sedit_no_trade_menu(d);
			return;
		case 'f':
		case 'F':
			sedit_shop_flags_menu(d);
			return;
		case 'r':
		case 'R':
			sedit_rooms_menu(d);
			return;
		case 'p':
		case 'P':
			sedit_products_menu(d);
			return;
		case 't':
		case 'T':
			sedit_namelist_menu(d);
			return;
		case 'x':
		case 'X':
			sedit_changelist_menu(d);
			return;
		case 'y':
		case 'Y':
			OLC_MODE(d) = SEDIT_CHANGE_PROFIT;
			i++;
			break;
		default:
			sedit_disp_menu(d);
			return;
		}

		if (i != 0)
		{
			send_to_char(i == 1 ? "\r\nВведите новое значение : " :
						 (i == -1 ? "\r\nВведите новый текст :\r\n] " : "Опаньки...\r\n"), d->character);
			return;
		}
		break;
		/*-------------------------------------------------------------------*/
	case SEDIT_NAMELIST_MENU:
		switch (*arg)
		{
		case 'a':
		case 'A':
			sedit_types_menu(d, SEDIT_BUYTYPE_MENU);
			return;
		case 'd':
		case 'D':
			send_to_char("\r\nУдалить какой тип для покупки ? : ", d->character);
			OLC_MODE(d) = SEDIT_DELETE_BUYTYPE;
			return;
		case 'q':
		case 'Q':
			break;
		}
		break;
		/*-------------------------------------------------------------------*/
	case SEDIT_CHANGELIST_MENU:
		switch (*arg)
		{
		case 'a':
		case 'A':
			sedit_types_menu(d, SEDIT_CHANGETYPE_MENU);
			return;
		case 'd':
		case 'D':
			send_to_char("\r\nУдалить какой тип для обмена ? : ", d->character);
			OLC_MODE(d) = SEDIT_DELETE_CHANGETYPE;
			return;
		case 'q':
		case 'Q':
			break;
		}
		break;
		/*-------------------------------------------------------------------*/
	case SEDIT_PRODUCTS_MENU:
		switch (*arg)
		{
		case 'a':
		case 'A':
			send_to_char("\r\nВведите виртуальный номер предмета : ", d->character);
			OLC_MODE(d) = SEDIT_NEW_PRODUCT;
			return;
		case 'd':
		case 'D':
			send_to_char("\r\nУдалить какой предмет ? : ", d->character);
			OLC_MODE(d) = SEDIT_DELETE_PRODUCT;
			return;
		case 'q':
		case 'Q':
			break;
		}
		break;
		/*-------------------------------------------------------------------*/
	case SEDIT_ROOMS_MENU:
		switch (*arg)
		{
		case 'a':
		case 'A':
			send_to_char("\r\nВведите новый виртуальный номер комнаты : ", d->character);
			OLC_MODE(d) = SEDIT_NEW_ROOM;
			return;
		case 'c':
		case 'C':
			sedit_compact_rooms_menu(d);
			return;
		case 'l':
		case 'L':
			sedit_rooms_menu(d);
			return;
		case 'd':
		case 'D':
			send_to_char("\r\nУдалить какую комнату ? : ", d->character);
			OLC_MODE(d) = SEDIT_DELETE_ROOM;
			return;
		case 'q':
		case 'Q':
			break;
		}
		break;
		/*-------------------------------------------------------------------*/
		/*
		 * String edits.
		 */
	case SEDIT_NOITEM1:
		sedit_modify_string(&S_NOITEM1(OLC_SHOP(d)), arg);
		break;
	case SEDIT_NOITEM2:
		sedit_modify_string(&S_NOITEM2(OLC_SHOP(d)), arg);
		break;
	case SEDIT_NOCASH1:
		sedit_modify_string(&S_NOCASH1(OLC_SHOP(d)), arg);
		break;
	case SEDIT_NOCASH2:
		sedit_modify_string(&S_NOCASH2(OLC_SHOP(d)), arg);
		break;
	case SEDIT_NOBUY:
		sedit_modify_string(&S_NOBUY(OLC_SHOP(d)), arg);
		break;
	case SEDIT_BUY:
		sedit_modify_string(&S_BUY(OLC_SHOP(d)), arg);
		break;
	case SEDIT_SELL:
		sedit_modify_string(&S_SELL(OLC_SHOP(d)), arg);
		break;
	case SEDIT_NAMELIST:
	{
		struct shop_buy_data new_entry;

		BUY_TYPE(new_entry) = OLC_VAL(d);
		BUY_WORD(new_entry) = (arg && *arg) ? str_dup(arg) : NULL;
		sedit_add_to_type_list(&(S_NAMELISTS(OLC_SHOP(d))), &new_entry);
	}
	sedit_namelist_menu(d);
	return;
	case SEDIT_CHANGELIST:
	{
		struct shop_buy_data new_entry;

		BUY_TYPE(new_entry) = OLC_VAL(d);
		BUY_WORD(new_entry) = (arg && *arg) ? str_dup(arg) : NULL;
		sedit_add_to_type_list(&(S_CHANGELISTS(OLC_SHOP(d))), &new_entry);
	}
	sedit_changelist_menu(d);
	return;

	/*-------------------------------------------------------------------*/
	/*
	 * Numerical responses.
	 */
	case SEDIT_KEEPER:
		i = atoi(arg);
		if ((i = atoi(arg)) != -1)
		{
			if ((i = real_mobile(i)) < 0)
			{
				send_to_char("Нет такого моба. Повторите ввод : ", d->character);
				return;
			}
		}
		S_KEEPER(OLC_SHOP(d)) = i;
		if (i == -1)
			break;
		/*
		 * Fiddle with special procs.
		 */
		S_FUNC(OLC_SHOP(d)) = mob_index[i].func;
		mob_index[i].func = shop_keeper;
		break;
	case SEDIT_OPEN1:
		S_OPEN1(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
		break;
	case SEDIT_OPEN2:
		S_OPEN2(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
		break;
	case SEDIT_CLOSE1:
		S_CLOSE1(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
		break;
	case SEDIT_CLOSE2:
		S_CLOSE2(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
		break;
	case SEDIT_BUY_PROFIT:
		sscanf(arg, "%f", &S_BUYPROFIT(OLC_SHOP(d)));
		break;
	case SEDIT_SELL_PROFIT:
		sscanf(arg, "%f", &S_SELLPROFIT(OLC_SHOP(d)));
		break;
	case SEDIT_CHANGE_PROFIT:
		sscanf(arg, "%f", &S_CHANGEPROFIT(OLC_SHOP(d)));
		break;
	case SEDIT_BUYTYPE_MENU:
		OLC_VAL(d) = MAX(0, MIN(NUM_ITEM_TYPES - 1, atoi(arg)));
		send_to_char("Введите параметры (предмет для покупки) (return - ничего) :-\r\n] ", d->character);
		OLC_MODE(d) = SEDIT_NAMELIST;
		return;
	case SEDIT_CHANGETYPE_MENU:
		OLC_VAL(d) = MAX(0, MIN(NUM_ITEM_TYPES - 1, atoi(arg)));
		send_to_char("Введите параметры (предмет для обмена) (return - ничего) :-\r\n] ", d->character);
		OLC_MODE(d) = SEDIT_CHANGELIST;
		return;

	case SEDIT_DELETE_BUYTYPE:
		sedit_remove_from_type_list(&(S_NAMELISTS(OLC_SHOP(d))), atoi(arg));
		sedit_namelist_menu(d);
		return;
	case SEDIT_DELETE_CHANGETYPE:
		sedit_remove_from_type_list(&(S_CHANGELISTS(OLC_SHOP(d))), atoi(arg));
		sedit_changelist_menu(d);
		return;

	case SEDIT_NEW_PRODUCT:
		if ((i = atoi(arg)) != -1)
		{
			if ((i = real_object(i)) == -1)
			{
				send_to_char("Этот предмет не существует. Повторите ввод : ", d->character);
				return;
			}
		}
		if (i > 0)
			sedit_add_to_int_list((int **) &(S_PRODUCTS(OLC_SHOP(d))), i);
		sedit_products_menu(d);
		return;
	case SEDIT_DELETE_PRODUCT:
		sedit_remove_from_int_list((int **) &(S_PRODUCTS(OLC_SHOP(d))), atoi(arg));
		sedit_products_menu(d);
		return;
	case SEDIT_NEW_ROOM:
		if ((i = atoi(arg)) != -1)
		{
			if ((i = real_room(i)) == NOWHERE)
			{
				send_to_char("Эта комната не существует. Повторите ввод : ", d->character);
				return;
			}
		}
		if (i >= 0)
			sedit_add_to_int_list((int **) &(S_ROOMS(OLC_SHOP(d))), atoi(arg));
		sedit_rooms_menu(d);
		return;
	case SEDIT_DELETE_ROOM:
		sedit_remove_from_int_list((int **) &(S_ROOMS(OLC_SHOP(d))), atoi(arg));
		sedit_rooms_menu(d);
		return;
	case SEDIT_SHOP_FLAGS:
		if ((i = MAX(0, MIN(NUM_SHOP_FLAGS, atoi(arg)))) > 0)
		{
			TOGGLE_BIT(S_BITVECTOR(OLC_SHOP(d)), 1 << (i - 1));
			sedit_shop_flags_menu(d);
			return;
		}
		break;
	case SEDIT_NOTRADE:
		if ((i = MAX(0, MIN(NUM_TRADERS, atoi(arg)))) > 0)
		{
			TOGGLE_BIT(S_NOTRADE(OLC_SHOP(d)), 1 << (i - 1));
			sedit_no_trade_menu(d);
			return;
		}
		break;

		/*-------------------------------------------------------------------*/
	default:
		/*
		 * We should never get here.
		 */
		cleanup_olc(d, CLEANUP_ALL);
		mudlog("SYSERR: OLC: sedit_parse(): Reached default case!", BRF, LVL_BUILDER, SYSLOG, TRUE);
		send_to_char("Опаньки...\r\n", d->character);
		break;
	}

	/*-------------------------------------------------------------------*/

	/*
	 * END OF CASE
	 * If we get here, we have probably changed something, and now want to
	 * return to main menu.  Use OLC_VAL as a 'has changed' flag.
	 */
	OLC_VAL(d) = 1;
	sedit_disp_menu(d);
}
