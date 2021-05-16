/**************************************************************************
*   File: exchange.cpp                                 Part of Bylins     *
*  Usage: Exchange functions used by the MUD                              *
*                                                                         *
*                                                                         *
*  $Author$                                                         *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "exchange.h"

#include "object.prototypes.hpp"
#include "chars/world.characters.hpp"
#include "obj.hpp"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "im.h"
#include "constants.h"
#include "skills.h"
#include "chars/character.h"
#include "chars/char_player.hpp"
#include "named_stuff.hpp"
#include "modify.h"
#include "room.hpp"
#include "mail.h"
#include "objsave.h"
#include "fightsystem/pk.h"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <stdexcept>
#include <sstream>

//Используемые внешние ф-ии.
extern int get_buf_line(char **source, char *target);
extern int get_buf_lines(char **source, char *target);

extern int invalid_anti_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_unique(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_no_class(CHAR_DATA * ch, const OBJ_DATA * obj);
extern int invalid_align(CHAR_DATA * ch, const OBJ_DATA * obj);
extern char *diag_weapon_to_char(const CObjectPrototype* obj, int show_wear);
extern const char *diag_obj_timer(const OBJ_DATA* obj);
extern char *diag_timer_to_char(const OBJ_DATA* obj);
extern void mort_show_obj_values(const OBJ_DATA * obj, CHAR_DATA * ch, int fullness);
extern void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
extern bool is_dig_stone(OBJ_DATA *obj);

//свои ф-ии
int exchange_exhibit(CHAR_DATA * ch, char *arg);
int exchange_change_cost(CHAR_DATA * ch, char *arg);
int exchange_withdraw(CHAR_DATA * ch, char *arg);
int exchange_information(CHAR_DATA * ch, char *arg);
int exchange_identify(CHAR_DATA * ch, char *arg);
int exchange_purchase(CHAR_DATA * ch, char *arg);
int exchange_offers(CHAR_DATA * ch, char *arg);
int exchange_setfilter(CHAR_DATA * ch, char *arg);


int exchange_database_load();
int exchange_database_reload(bool loadbackup);
void check_exchange(OBJ_DATA * obj);
void extract_exchange_item(EXCHANGE_ITEM_DATA * item);
int get_unique_lot(void);
void message_exchange(char *message, CHAR_DATA * ch, EXCHANGE_ITEM_DATA * j);
void show_lots(char *filter, short int show_type, CHAR_DATA * ch);
int parse_exch_filter(ParseFilter &filter, char *buf, bool parse_affects);
void clear_exchange_lot(EXCHANGE_ITEM_DATA * lot);
extern void obj_info(CHAR_DATA * ch, OBJ_DATA *obj, char buf[MAX_STRING_LENGTH]);

//Polud
void do_exchange(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

EXCHANGE_ITEM_DATA *create_exchange_item(void);

EXCHANGE_ITEM_DATA *exchange_item_list = NULL;
std::vector<bool> lot_usage;

char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

char info_message[] = ("базар выставить <предмет> <цена>        - выставить предмет на продажу\r\n"
					   "базар цена <#лот> <цена>                - изменить цену на свой лот\r\n"
					   "базар снять <#лот>                      - снять с продажи свой лот\r\n"
					   "базар информация <#лот>                 - информация о лоте\r\n"
					   "базар характеристики <#лот>             - характеристики лота (цена услуги 110 кун)\r\n"
					   "базар купить <#лот>                     - купить лот\r\n"
					   "базар предложения все <предмет>         - все предложения\r\n"
					   "базар предложения последние             - последние\r\n"
					   "базар предложения мои <предмет>         - мои предложения\r\n"
					   "базар предложения руны <предмет>        - предложения рун\r\n"
					   "базар предложения броня <предмет>       - предложения одежды и брони\r\n"
//					   "базар предложения легкие <предмет>      - предложения легких доспехов\r\n"
//					   "базар предложения средние <предмет>     - предложения средних доспехов\r\n"
//					   "базар предложения тяжелые <предмет>     - предложения тяжелых доспехов\r\n"
					   "базар предложения оружие <предмет>      - предложения оружия\r\n"
					   "базар предложения книги <предмет>       - предложения книг\r\n"
					   "базар предложения ингредиенты <предмет> - предложения ингредиентов\r\n"
					   "базар предложения прочие <предмет>      - прочие предложения\r\n"
					   "базар предложения аффект имя.аффекта    - поиск по аффекту (цена услуги 55 кун)\r\n"
					   "базар фильтрация <фильтр>               - фильтрация товара на базаре\r\n");

int exchange(CHAR_DATA *ch, void* /*me*/, int cmd, char* argument)
{
	if (CMD_IS("exchange") || CMD_IS("базар"))
	{
		if (IS_NPC(ch))
			return 0;
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_SILENCE) || AFF_FLAGGED(ch, EAffectFlag::AFF_STRANGLED))
		{
			send_to_char("Вы немы, как рыба об лед.\r\n", ch);
			return 1;
		}
		if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB))
		{
			send_to_char("Вам запрещено общаться с торговцами!\r\n", ch);
			return 1;
		}
		/*
				if (PLR_FLAGGED(ch, PLR_MUTE)) {
					send_to_char("Вам не к лицу торговаться.\r\n", ch);
					return 1;
				}
		*/
		if (GET_LEVEL(ch) < EXCHANGE_MIN_CHAR_LEV && !GET_REMORT(ch))
		{
			sprintf(buf1,
					"Вам стоит достичь хотя бы %d уровня, чтобы пользоваться базаром.\r\n",
					EXCHANGE_MIN_CHAR_LEV);
			send_to_char(buf1, ch);
			return 1;
		}
		if (RENTABLE(ch))
		{
			send_to_char("Завершите сначала боевые действия.\r\n", ch);
			return 1;
		}

		argument = one_argument(argument, arg1);

		if (is_abbrev(arg1, "выставить") || is_abbrev(arg1, "exhibit"))
			exchange_exhibit(ch, argument);
		else if (is_abbrev(arg1, "цена") || is_abbrev(arg1, "cost"))
			exchange_change_cost(ch, argument);
		else if (is_abbrev(arg1, "снять") || is_abbrev(arg1, "withdraw"))
			exchange_withdraw(ch, argument);
		else if (is_abbrev(arg1, "информация") || is_abbrev(arg1, "information"))
			exchange_information(ch, argument);
		else if (is_abbrev(arg1, "характеристики") || is_abbrev(arg1, "identify"))
			exchange_identify(ch, argument);
		else if (is_abbrev(arg1, "купить") || is_abbrev(arg1, "purchase"))
			exchange_purchase(ch, argument);
		else if (is_abbrev(arg1, "предложения") || is_abbrev(arg1, "offers"))
			exchange_offers(ch, argument);
		else if (is_abbrev(arg1, "фильтрация") || is_abbrev(arg1, "filter"))
			exchange_setfilter(ch, argument);
		else if (is_abbrev(arg1, "save") && (GET_LEVEL(ch) >= LVL_IMPL))
			exchange_database_save();
		else if (is_abbrev(arg1, "savebackup") && (GET_LEVEL(ch) >= LVL_IMPL))
			exchange_database_save(true);
		else if (is_abbrev(arg1, "reload") && (GET_LEVEL(ch) >= LVL_IMPL))
			exchange_database_reload(false);
		else if (is_abbrev(arg1, "reloadbackup") && (GET_LEVEL(ch) >= LVL_IMPL))
			exchange_database_reload(true);
		else
			send_to_char(info_message, ch);

		return 1;
	}
	else
		return 0;

}

int exchange_exhibit(CHAR_DATA * ch, char *arg)
{

	char obj_name[MAX_INPUT_LENGTH];
	int item_cost = 0, lot;
	OBJ_DATA *obj;
	char tmpbuf[MAX_INPUT_LENGTH];
	EXCHANGE_ITEM_DATA *item = NULL;
	EXCHANGE_ITEM_DATA *j, *next_thing = NULL;
	int counter;
	int counter_ming; //количиство ингридиентов
	int tax;	//налог

	if (!*arg)
	{
		send_to_char(info_message, ch);
		return false;
	}
	if (GET_LEVEL(ch) >= LVL_IMMORT && GET_LEVEL(ch) < LVL_IMPL)
	{
		send_to_char("Боже, не лезьте в экономику смертных, вам это не к чему.\r\n", ch);
		return false;
	}


	arg = one_argument(arg, obj_name);
	arg = one_argument(arg, arg2);
	if (!*obj_name)
	{
		send_to_char("Формат: базар выставить предмет цена комментарий\r\n", ch);
		return false;
	}
	if (!sscanf(arg2, "%d", &item_cost))
		item_cost = 0;
	if (!*obj_name)
	{
		send_to_char("Не указан предмет.\r\n", ch);
		return false;
	}
	if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying)))
	{
		send_to_char("У вас этого нет.\r\n", ch);
		return false;
	}
	if (!bloody::handle_transfer(ch, NULL, obj))
		return false;
	if (item_cost > 400000 && OBJ_FLAGGED(obj, EExtraFlag::ITEM_SETSTUFF))
	{
		send_to_char("Никто не купит ЭТО за такую сумму.\r\n", ch);
		return false;
	}
	if (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_BOOK)
	{
		if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_NORENT)
			|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_NOSELL)
			|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_ZONEDECAY)
			|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_REPOP_DECAY)
			|| GET_OBJ_RNUM(obj) < 0)
		{
			send_to_char("Этот предмет не предназначен для базара.\r\n", ch);
			return false;
		}
	}
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_DECAY)
		|| OBJ_FLAGGED(obj, EExtraFlag::ITEM_NODROP)
		|| GET_OBJ_COST(obj) <= 0
		|| obj->get_owner() > 0)
	{
		send_to_char("Этот предмет не предназначен для базара.\r\n", ch);
		return false;
	}
	if (obj->get_contains())
	{
		sprintf(tmpbuf, "Опустошите %s перед продажей.\r\n", obj->get_PName(3).c_str());
		send_to_char(tmpbuf, ch);
		return false;
	}
	else if (SetSystem::is_big_set(obj, true))
	{
		snprintf(buf, MAX_STRING_LENGTH, "%s является частью большого набора предметов.\r\n",
			obj->get_PName(0).c_str());
		send_to_char(CAP(buf), ch);
		return false;
	}

	if (item_cost <= 0)
	{
		item_cost = MAX(1, GET_OBJ_COST(obj));
	}

	tax = (GET_OBJ_TYPE(obj) != OBJ_DATA::ITEM_MING)
		? EXCHANGE_EXHIBIT_PAY + (int)(item_cost * EXCHANGE_EXHIBIT_PAY_COEFF)
		: (int)(item_cost * EXCHANGE_EXHIBIT_PAY_COEFF / 2);
	if ((ch->get_total_gold() < tax)
		&& (GET_LEVEL(ch) < LVL_IMPL))
	{
		send_to_char("У вас не хватит денег на налоги!\r\n", ch);
		return false;
	}
	for (j = exchange_item_list, counter = 0, counter_ming = 0;
			j && (counter + (counter_ming / 20)  <= EXCHANGE_MAX_EXHIBIT_PER_CHAR + (GET_REMORT(ch) * 2));
			j = next_thing)
	{
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_SELLERID(j) == GET_IDNUM(ch))
		{
			if (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_MING
				&& GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_INGREDIENT)
			{
				counter++;
			}
			else
			{
				counter_ming++;
			}
		}
	}

	if (counter + (counter_ming / 20)  >= EXCHANGE_MAX_EXHIBIT_PER_CHAR + (GET_REMORT(ch) * 2))
	{
		send_to_char("Вы уже выставили на базар максимальное количество предметов!\r\n", ch);
		return false;
	}

	if ((lot = get_unique_lot()) <= 0)
	{
		send_to_char("Базар переполнен!\r\n", ch);
		return false;
	}
	item = create_exchange_item();
	GET_EXCHANGE_ITEM_LOT(item) = lot;
	GET_EXCHANGE_ITEM_SELLERID(item) = GET_IDNUM(ch);
	GET_EXCHANGE_ITEM_COST(item) = item_cost;
	GET_EXCHANGE_ITEM_TIME(item) = time(0);
	skip_spaces(&arg);
	if (*arg)
		GET_EXCHANGE_ITEM_COMMENT(item) = str_dup(arg);
	else
		GET_EXCHANGE_ITEM_COMMENT(item) = NULL;

	if  (check_unlimited_timer(obj)) // если нерушима таймер 1 неделя
		obj->set_timer(10080);
	GET_EXCHANGE_ITEM(item) = obj;
	obj_from_char(obj);

	sprintf(tmpbuf, "Вы выставили на базар $O3 (лот %d) за %d %s.\r\n",
			GET_EXCHANGE_ITEM_LOT(item), item_cost, desc_count(item_cost, WHAT_MONEYu));
	act(tmpbuf, FALSE, ch, 0, obj, TO_CHAR);
	sprintf(tmpbuf, "Базар : новый лот (%d) - %s - цена %d %s. \r\n",
		GET_EXCHANGE_ITEM_LOT(item), obj->get_PName(0).c_str(), item_cost, desc_count(item_cost, WHAT_MONEYa));
	message_exchange(tmpbuf, ch, item);

	ch->remove_both_gold(tax);

	if (EXCHANGE_SAVEONEVERYOPERATION)
	{
		exchange_database_save();
		Crash_crashsave(ch);
		ch->save_char();
	}
	return (true);
}


int exchange_change_cost(CHAR_DATA * ch, char *arg)
{
	EXCHANGE_ITEM_DATA *item = NULL, *j, *next_thing = NULL;
	int lot, newcost, pay;
	char tmpbuf[MAX_INPUT_LENGTH];

	if (!*arg)
	{
		send_to_char(info_message, ch);
		return false;
	}
	if (GET_LEVEL(ch) >= LVL_IMMORT && GET_LEVEL(ch) < LVL_IMPL)
	{
		send_to_char("Боже, не лезьте в экономику смертных, вам это не к чему.\r\n", ch);
		return false;
	}
	if (sscanf(arg, "%d %d", &lot, &newcost) != 2)
	{
		send_to_char("Формат команды: базар цена <лот> <новая цена>.\r\n", ch);
		return false;
	}
	for (j = exchange_item_list; j && (!item); j = next_thing)
	{
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_LOT(j) == lot)
			item = j;
	}
	if ((lot < 0) || (!item))
	{
		send_to_char("Неверный номер лота.\r\n", ch);
		return false;
	}
	if ((GET_EXCHANGE_ITEM_SELLERID(item) != GET_IDNUM(ch)) && (GET_LEVEL(ch) < LVL_IMPL))
	{
		send_to_char("Это не ваш лот.\r\n", ch);
		return false;
	}
	if (newcost == GET_EXCHANGE_ITEM_COST(item))
	{
		send_to_char("Ваша новая цена совпадает с текущей.\r\n", ch);
		return false;
	}
	if (newcost <= 0)
	{
		send_to_char("Вы указали неправильную цену.\r\n", ch);
		return false;
	}
	pay = newcost - GET_EXCHANGE_ITEM_COST(item);
	if (pay > 0)
		if ((ch->get_total_gold() < (pay * EXCHANGE_EXHIBIT_PAY_COEFF)) && (GET_LEVEL(ch) < LVL_IMPL))
		{
			send_to_char("У вас не хватит денег на налоги!\r\n", ch);
			return false;
		}


	GET_EXCHANGE_ITEM_COST(item) = newcost;
	if (pay > 0)
	{
		ch->remove_both_gold(static_cast<long>(pay * EXCHANGE_EXHIBIT_PAY_COEFF));
	}

	sprintf(tmpbuf, "Вы назначили цену %d %s за %s (лот %d).\r\n",
		newcost, desc_count(newcost, WHAT_MONEYu), GET_EXCHANGE_ITEM(item)->get_PName(3).c_str(), GET_EXCHANGE_ITEM_LOT(item));
	send_to_char(tmpbuf, ch);
	sprintf(tmpbuf, "Базар : лот (%d) - %s - выставлен за новую цену %d %s.\r\n",
		GET_EXCHANGE_ITEM_LOT(item), GET_EXCHANGE_ITEM(item)->get_PName(0).c_str(), newcost, desc_count(newcost, WHAT_MONEYa));
	message_exchange(tmpbuf, ch, item);
	set_wait(ch, 2, FALSE);

	return true;
}

int exchange_withdraw(CHAR_DATA * ch, char *arg)
{
	EXCHANGE_ITEM_DATA *item = NULL, *j, *next_thing = NULL;
	int lot;
	char tmpbuf[MAX_INPUT_LENGTH];


	if (!*arg)
	{
		send_to_char(info_message, ch);
		return false;
	}
	if (GET_LEVEL(ch) >= LVL_IMMORT && GET_LEVEL(ch) < LVL_IMPL)
	{
		send_to_char("Боже, не лезьте в экономику смертных, вам это не к чему.\r\n", ch);
		return false;
	}

	if (!sscanf(arg, "%d", &lot))
	{
		send_to_char("Не указан номер лота.\r\n", ch);
		return false;
	}
	for (j = exchange_item_list; j && (!item); j = next_thing)
	{
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_LOT(j) == lot)
			item = j;
	}
	if ((lot < 0) || (!item))
	{
		send_to_char("Неверный номер лота.\r\n", ch);
		return false;
	}
	if ((GET_EXCHANGE_ITEM_SELLERID(item) != GET_IDNUM(ch)) && (GET_LEVEL(ch) < LVL_IMPL))
	{
		send_to_char("Это не ваш лот.\r\n", ch);
		return false;
	}
	act("Вы сняли $O3 с базара.\r\n", FALSE, ch, 0, GET_EXCHANGE_ITEM(item), TO_CHAR);
	if (GET_EXCHANGE_ITEM_SELLERID(item) != GET_IDNUM(ch))
	{
		sprintf(tmpbuf, "Базар : лот %d(%s) снят%s с базара Богами.\r\n", lot,
			GET_EXCHANGE_ITEM(item)->get_PName(0).c_str(), GET_OBJ_SUF_6(GET_EXCHANGE_ITEM(item)));
	}
	else
	{
		sprintf(tmpbuf, "Базар : лот %d(%s) снят%s с базара владельцем.\r\n", lot,
			GET_EXCHANGE_ITEM(item)->get_PName(0).c_str(), GET_OBJ_SUF_6(GET_EXCHANGE_ITEM(item)));
	}
	message_exchange(tmpbuf, ch, item);
	if  (check_unlimited_timer(GET_EXCHANGE_ITEM(item))) // если нерушима фрешим таймер из прототипа
		GET_EXCHANGE_ITEM(item)->set_timer(obj_proto.at(GET_OBJ_RNUM(GET_EXCHANGE_ITEM(item)))->get_timer());
	obj_to_char(GET_EXCHANGE_ITEM(item), ch);
	clear_exchange_lot(item);

	if (EXCHANGE_SAVEONEVERYOPERATION)
	{
		exchange_database_save();
		Crash_crashsave(ch);
		ch->save_char();
	}

	return true;
}


int exchange_information(CHAR_DATA * ch, char *arg)
{
	EXCHANGE_ITEM_DATA *item = NULL, *j, *next_thing = NULL;
	int lot;
	char buf[MAX_STRING_LENGTH], buf2[MAX_INPUT_LENGTH];


	if (!*arg)
	{
		send_to_char(info_message, ch);
		return false;
	}
	if (!sscanf(arg, "%d", &lot))
	{
		send_to_char("Не указан номер лота.\r\n", ch);
		return false;
	}
	for (j = exchange_item_list; j && (!item); j = next_thing)
	{
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_LOT(j) == lot)
			item = j;
	}
	if ((lot < 0) || (!item))
	{
		send_to_char("Неверный номер лота.\r\n", ch);
		return false;
	}

	sprintf(buf, "Лот %d. Цена %d\r\n", GET_EXCHANGE_ITEM_LOT(item), GET_EXCHANGE_ITEM_COST(item));

	sprintf(buf + strlen(buf), "Предмет \"%s\", ", GET_EXCHANGE_ITEM(item)->get_short_description().c_str());
	if (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(item)) == OBJ_DATA::ITEM_WAND
		|| GET_OBJ_TYPE(GET_EXCHANGE_ITEM(item)) == OBJ_DATA::ITEM_STAFF)
	{
		if (GET_OBJ_VAL(GET_EXCHANGE_ITEM(item), 2) < GET_OBJ_VAL(GET_EXCHANGE_ITEM(item), 1))
		{
			strcat(buf, "(б/у), ");
		}
	}
	strcat(buf, " тип ");
	sprinttype(GET_OBJ_TYPE(GET_EXCHANGE_ITEM(item)), item_types, buf2);
	if (*buf2)
	{
		strcat(buf, buf2);
		strcat(buf, "\n");
	};
	strcat(buf, diag_weapon_to_char(GET_EXCHANGE_ITEM(item), TRUE));
	strcat(buf, diag_timer_to_char(GET_EXCHANGE_ITEM(item)));
	obj_info(ch, GET_EXCHANGE_ITEM(item), buf);
	strcat(buf, "\n");
	if (invalid_anti_class(ch, GET_EXCHANGE_ITEM(item)) || invalid_unique(ch, GET_EXCHANGE_ITEM(item)) || NamedStuff::check_named(ch, GET_EXCHANGE_ITEM(item), 0))
	{
		sprintf(buf2, "Эта вещь вам недоступна!");
		strcat(buf, buf2);
		strcat(buf, "\n");
	}
	if (invalid_align(ch, GET_EXCHANGE_ITEM(item)) || invalid_no_class(ch, GET_EXCHANGE_ITEM(item)))
	{
		sprintf(buf2, "Вы не сможете пользоваться этой вещью.");
		strcat(buf, buf2);
		strcat(buf, "\n");
	}
	sprintf(buf2, "%s",
			get_name_by_id(GET_EXCHANGE_ITEM_SELLERID(item)) ?
			get_name_by_id(GET_EXCHANGE_ITEM_SELLERID(item)) : "(null)");
	*buf2 = UPPER(*buf2);
	strcat(buf, "Продавец ");
	strcat(buf, buf2);
	strcat(buf, "\n");

	if (GET_EXCHANGE_ITEM_COMMENT(item))
	{
		strcat(buf, "Берестовая наклейка на лоте гласит: ");
		sprintf(buf2, "'%s'.", GET_EXCHANGE_ITEM_COMMENT(item));
		strcat(buf, buf2);
		strcat(buf, "\n");
	}
	send_to_char(buf, ch);
	return true;
}

int exchange_identify(CHAR_DATA * ch, char *arg)
{
	EXCHANGE_ITEM_DATA *item = NULL, *j, *next_thing = NULL;
	int lot;
	if (!*arg)
	{
		send_to_char(info_message, ch);
		return false;
	}
	if (!sscanf(arg, "%d", &lot))
	{
		send_to_char("Не указан номер лота.\r\n", ch);
		return false;
	}
	for (j = exchange_item_list; j && (!item); j = next_thing)
	{
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_LOT(j) == lot)
			item = j;
	}
	if ((lot < 0) || (!item))
	{
		send_to_char("Неверный номер лота.\r\n", ch);
		return false;
	}

	if (GET_LEVEL(ch) >= LVL_IMMORT && GET_LEVEL(ch) < LVL_IMPL)
	{
		send_to_char("Господи, а ведь смертные за это деньги платят.\r\n", ch);
		return false;
	}
	if ((ch->get_total_gold() < (EXCHANGE_IDENT_PAY)) && (GET_LEVEL(ch) < LVL_IMPL))
	{
		send_to_char("У вас не хватит на это денег!\r\n", ch);
		return false;
	}
	mort_show_obj_values(GET_EXCHANGE_ITEM(item), ch, 200);	//200 - полное опознание
	ch->remove_both_gold(EXCHANGE_IDENT_PAY);
	send_to_char(ch, "\r\n%sЗа информацию о предмете с вашего банковского счета сняли %d %s%s\r\n",
		CCIGRN(ch, C_NRM), EXCHANGE_IDENT_PAY, desc_count(EXCHANGE_IDENT_PAY, WHAT_MONEYu), CCNRM(ch, C_NRM));

	return true;
}

CHAR_DATA *get_char_by_id(int id)
{
	for (const auto i : character_list)
	{
		if (!IS_NPC(i) && GET_IDNUM(i) == id)
		{
			return i.get();
		}
	}
	return 0;
}

int exchange_purchase(CHAR_DATA * ch, char *arg)
{
	EXCHANGE_ITEM_DATA *item = NULL, *j, *next_thing = NULL;
	int lot;
	char tmpbuf[MAX_INPUT_LENGTH];
	CHAR_DATA *seller;

	if (!*arg)
	{
		send_to_char(info_message, ch);
		return false;
	}
	if (GET_LEVEL(ch) >= LVL_IMMORT && GET_LEVEL(ch) < LVL_IMPL)
	{
		send_to_char("Боже, не лезьте в экономику смертных, вам это не к чему.\r\n", ch);
		return false;
	}

	if (!sscanf(arg, "%d", &lot))
	{
		send_to_char("Не указан номер лота.\r\n", ch);
		return false;
	}
	for (j = exchange_item_list; j && (!item); j = next_thing)
	{
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM_LOT(j) == lot)
			item = j;
	}
	if ((lot < 0) || (!item))
	{
		send_to_char("Неверный номер лота.\r\n", ch);
		return false;
	}
	if (GET_EXCHANGE_ITEM_SELLERID(item) == GET_IDNUM(ch))
	{
		send_to_char("Это же ваш лот. Воспользуйтесь командой 'базар снять <лот>'\r\n", ch);
		return false;
	}
	if ((ch->get_total_gold() < (GET_EXCHANGE_ITEM_COST(item))) && (GET_LEVEL(ch) < LVL_IMPL))
	{
		send_to_char("У вас в банке не хватает денег на этот лот!\r\n", ch);
		return false;
	}



	seller = get_char_by_id(GET_EXCHANGE_ITEM_SELLERID(item));

	if (seller == NULL)
	{
		const char *seller_name = get_name_by_id(GET_EXCHANGE_ITEM_SELLERID(item));

		auto seller_ptr = std::make_unique<Player>();
		seller = seller_ptr.get(); // TODO: переделать на стек
		if (seller_name == NULL
			|| load_char(seller_name, seller) < 0)
		{
			ch->remove_both_gold(GET_EXCHANGE_ITEM_COST(item));

			//edited by WorM 2011.05.21
			act("Вы купили $O3 на базаре.\r\n", FALSE, ch, 0, GET_EXCHANGE_ITEM(item), TO_CHAR);
			sprintf(tmpbuf, "Базар : лот %d(%s) продан%s за %d %s.\r\n", lot,
				GET_EXCHANGE_ITEM(item)->get_PName(0).c_str(), GET_OBJ_SUF_6(GET_EXCHANGE_ITEM(item)),
				GET_EXCHANGE_ITEM_COST(item), desc_count(GET_EXCHANGE_ITEM_COST(item), WHAT_MONEYu));
			//end by WorM

			message_exchange(tmpbuf, ch, item);
			if  (check_unlimited_timer(GET_EXCHANGE_ITEM(item))) // если нерушима фрешим таймер из прототипа
				GET_EXCHANGE_ITEM(item)->set_timer(obj_proto.at(GET_OBJ_RNUM(GET_EXCHANGE_ITEM(item)))->get_timer());
			obj_to_char(GET_EXCHANGE_ITEM(item), ch);
			clear_exchange_lot(item);
			if (EXCHANGE_SAVEONEVERYOPERATION)
			{
				exchange_database_save();
				Crash_crashsave(ch);
				ch->save_char();
			}

			return true;
		}

		seller->add_bank(GET_EXCHANGE_ITEM_COST(item));
		ch->remove_both_gold(GET_EXCHANGE_ITEM_COST(item));
//Polud напишем письмецо чару, раз уж его нету онлайн
		if (NOTIFY_EXCH_PRICE(seller) && GET_EXCHANGE_ITEM_COST(item) >= NOTIFY_EXCH_PRICE(seller))
		{
			sprintf(tmpbuf, "Базар : лот %d(%s) продан%s. %d %s переведено на ваш счет.\r\n", lot,
				GET_EXCHANGE_ITEM(item)->get_PName(0).c_str(), GET_OBJ_SUF_6(GET_EXCHANGE_ITEM(item)),
				GET_EXCHANGE_ITEM_COST(item), desc_count(GET_EXCHANGE_ITEM_COST(item), WHAT_MONEYa));
			mail::add_by_id(GET_EXCHANGE_ITEM_SELLERID(item), -1, tmpbuf);
		}
//-Polud
		seller->save_char();

		act("Вы купили $O3 на базаре.\r\n", FALSE, ch, 0, GET_EXCHANGE_ITEM(item), TO_CHAR);
		sprintf(tmpbuf, "Базар : лот %d(%s) продан%s за %d %s.\r\n", lot,
			GET_EXCHANGE_ITEM(item)->get_PName(0).c_str(), GET_OBJ_SUF_6(GET_EXCHANGE_ITEM(item)),
			GET_EXCHANGE_ITEM_COST(item), desc_count(GET_EXCHANGE_ITEM_COST(item), WHAT_MONEYu));
		message_exchange(tmpbuf, ch, item);
		if  (check_unlimited_timer(GET_EXCHANGE_ITEM(item))) // если нерушима фрешим таймер из прототипа
			GET_EXCHANGE_ITEM(item)->set_timer(obj_proto.at(GET_OBJ_RNUM(GET_EXCHANGE_ITEM(item)))->get_timer());
		obj_to_char(GET_EXCHANGE_ITEM(item), ch);
		clear_exchange_lot(item);
		if (EXCHANGE_SAVEONEVERYOPERATION)
		{
			exchange_database_save();
			Crash_crashsave(ch);
			ch->save_char();
		}

		return true;
	}
	else
	{
		seller->add_bank(GET_EXCHANGE_ITEM_COST(item));
		ch->remove_both_gold(GET_EXCHANGE_ITEM_COST(item));

		act("Вы купили $O3 на базаре.\r\n", FALSE, ch, 0, GET_EXCHANGE_ITEM(item), TO_CHAR);
		sprintf(tmpbuf, "Базар : лот %d(%s) продан%s за %d %s.\r\n", lot,
			GET_EXCHANGE_ITEM(item)->get_PName(0).c_str(), GET_OBJ_SUF_6(GET_EXCHANGE_ITEM(item)),
			GET_EXCHANGE_ITEM_COST(item), desc_count(GET_EXCHANGE_ITEM_COST(item), WHAT_MONEYu));
		message_exchange(tmpbuf, seller, item);
		sprintf(tmpbuf, "Базар : лот %d(%s) продан%s. %d %s переведено на ваш счет.\r\n", lot,
			GET_EXCHANGE_ITEM(item)->get_PName(0).c_str(), GET_OBJ_SUF_6(GET_EXCHANGE_ITEM(item)),
			GET_EXCHANGE_ITEM_COST(item), desc_count(GET_EXCHANGE_ITEM_COST(item), WHAT_MONEYa));
		act(tmpbuf, FALSE, seller, 0, NULL, TO_CHAR);

		obj_to_char(GET_EXCHANGE_ITEM(item), ch);
		clear_exchange_lot(item);
		if (EXCHANGE_SAVEONEVERYOPERATION)
		{
			exchange_database_save();
			Crash_crashsave(ch);
			ch->save_char();
			Crash_crashsave(seller);
			seller->save_char();
		}

		return true;
	}
}

/**
* Проверка длины фильтра.
* \return false - перебор
*         true - корректный
*/
bool correct_filter_length(CHAR_DATA *ch, const char *str)
{
	if (strlen(str) >= FILTER_LENGTH)
	{
		send_to_char(ch, "Слишком длинный фильтр, максимальная длина: %d символа.\r\n", FILTER_LENGTH - 1);
		return false;
	}
	return true;
}

int exchange_offers(CHAR_DATA * ch, char *arg)
{
//влом байты считать. Если кто хочет оптимизировать, посчитайте точно.
	char filter[MAX_STRING_LENGTH];
	char multifilter[MAX_STRING_LENGTH];
	short int show_type;
	bool ignore_filter;
	char arg3[MAX_INPUT_LENGTH], arg4[MAX_INPUT_LENGTH];

	/*
	show_type
	0 - все
	1 - мои
	2 - руны
	3 - одежда
	4 - оружие
	5 - книги
	6 - ингры
	7 - прочее
	8 - последние
	*/

	memset(filter, 0, FILTER_LENGTH);
	memset(multifilter, 0, FILTER_LENGTH);

	(arg = one_argument(arg, arg1));
	(arg = one_argument(arg, arg2));
	(arg = one_argument(arg, arg3));
	(arg = one_argument(arg, arg4));

	ignore_filter = ((*arg2 == '*') || (*arg3 == '*') || (*arg4 == '*'));

	if (*arg2 == '!')
	{
		strcpy(multifilter, arg2 + 1);
	}
	if (*arg3 == '!')
	{
		strcpy(multifilter, arg3 + 1);
	}
	if (*arg4 == '!')
	{
		strcpy(multifilter, arg4 + 1);
	}

	if (is_abbrev(arg1, "все") || is_abbrev(arg1, "all"))
	{
		show_type = 0;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			sprintf(filter, "И%s", arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " О");
			strcat(filter, multifilter);
		}
	}
	else if (is_abbrev(arg1, "мои") || is_abbrev(arg1, "mine"))
	{
		show_type = 1;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			snprintf(filter, MAX_STRING_LENGTH, "%s И%s", filter, arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " О");
			strcat(filter, multifilter);
		}
	}
	else if (is_abbrev(arg1, "руны") || is_abbrev(arg1, "runes"))
	{
		show_type = 2;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			snprintf(filter, MAX_STRING_LENGTH, "%s И%s", filter, arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " С");
			strcat(filter, multifilter);
		}
	}
	else if (is_abbrev(arg1, "броня") || is_abbrev(arg1, "armor"))
	{
		show_type = 3;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " О");
			strcat(filter, multifilter);
		}
	}
	else if (is_abbrev(arg1, "оружие") || is_abbrev(arg1, "weapons"))
	{
		show_type = 4;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " К");
			strcat(filter, multifilter);
		}
	}
	else if (is_abbrev(arg1, "книги") || is_abbrev(arg1, "books"))
	{
		show_type = 5;
		if ((*arg2) && (*arg2 != '*'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
	}
	else if (is_abbrev(arg1, "ингредиенты") || is_abbrev(arg1, "ingradients"))
	{
		show_type = 6;
		if ((*arg2) && (*arg2 != '*'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
	}
	else if (is_abbrev(arg1, "прочие") || is_abbrev(arg1, "other"))
	{
		show_type = 7;
		if ((*arg2) && (*arg2 != '*'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
	}
	else if (is_abbrev(arg1, "последние") || is_abbrev(arg1, "last"))
	{
		show_type = 8;
		// я х3 как тут писать сравнение времен
	}
/*	else if (is_abbrev(arg1, "средние") || is_abbrev(arg1, "средняя"))
	{
		show_type = 9;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " О");
			strcat(filter, multifilter);
		}
	}
	else if (is_abbrev(arg1, "тяжелые") || is_abbrev(arg1, "тяжелая"))
	{
		show_type = 10;
		if ((*arg2) && (*arg2 != '*') && (*arg2 != '!'))
		{
			sprintf(filter, "%s И%s", filter, arg2);
		}
		if (*multifilter)
		{
			strcat(filter, " О");
			strcat(filter, multifilter);
		}
	}
*/
	else if (is_abbrev(arg1, "аффект") || is_abbrev(arg1, "affect"))
	{
		if (ch->get_total_gold() < EXCHANGE_IDENT_PAY / 2
			&& GET_LEVEL(ch) < LVL_IMPL)
		{
			send_to_char("У вас не хватит на это денег!\r\n", ch);
			return 0;
		}
		if (*arg2 == '\0')
		{
			send_to_char("Пустое имя аффекта!\r\n", ch);
			return 0;
		}
		show_type = 11;
		sprintf(filter, "%s А%s", filter, arg2);
	}
	else
	{
		send_to_char(info_message, ch);
		return 0;
	}

	if (!ignore_filter && EXCHANGE_FILTER(ch))
		snprintf(multifilter, MAX_STRING_LENGTH, "%s %s", EXCHANGE_FILTER(ch), filter);
	else
		strcpy(multifilter, filter);

	if (!correct_filter_length(ch, multifilter))
	{
		return 0;
	}

	show_lots(multifilter, show_type, ch);
	return 1;
}

int exchange_setfilter(CHAR_DATA * ch, char *arg)
{
	if (!*arg)
	{
		if (!EXCHANGE_FILTER(ch))
		{
			send_to_char("Ваш фильтр базара пуст.\r\n", ch);
			return true;
		}
		ParseFilter params(ParseFilter::EXCHANGE);
		if (!parse_exch_filter(params, EXCHANGE_FILTER(ch), false))
		{
			free(EXCHANGE_FILTER(ch));
			EXCHANGE_FILTER(ch) = NULL;
			send_to_char("Ваш фильтр базара пуст\r\n", ch);
		}
		else
		{
			send_to_char(ch, "Ваш текущий фильтр базара: %s.\r\n",
				EXCHANGE_FILTER(ch));
		}
		return true;
	}

	char tmpbuf[MAX_INPUT_LENGTH];
	char filter[MAX_INPUT_LENGTH];

	skip_spaces(&arg);
	strcpy(filter, arg);
	if (!correct_filter_length(ch, filter))
	{
		return false;
	}
	if (!strncmp(filter, "нет", 3))
	{
		if (EXCHANGE_FILTER(ch))
		{
			snprintf(tmpbuf, sizeof(tmpbuf),
				"Ваш старый фильтр: %s. Новый фильтр пуст.\r\n",
				EXCHANGE_FILTER(ch));
			free(EXCHANGE_FILTER(ch));
			EXCHANGE_FILTER(ch) = NULL;
		}
		else
		{
			sprintf(tmpbuf, "Новый фильтр пуст.\r\n");
		}
		send_to_char(tmpbuf, ch);
		return true;
	}

	ParseFilter params(ParseFilter::EXCHANGE);
	if (!parse_exch_filter(params, filter, false))
	{
		send_to_char("Неверный формат фильтра. Прочтите справку.\r\n", ch);
		free(EXCHANGE_FILTER(ch));
		EXCHANGE_FILTER(ch) = NULL;
		return false;
	}

	if (EXCHANGE_FILTER(ch))
	{
		snprintf(tmpbuf, MAX_INPUT_LENGTH, "Ваш старый фильтр: %s. Новый фильтр: %s.\r\n",
			EXCHANGE_FILTER(ch), filter);
	}
	else
	{
		snprintf(tmpbuf, MAX_INPUT_LENGTH,"Ваш новый фильтр: %s.\r\n", filter);
	}
	send_to_char(tmpbuf, ch);

	if (EXCHANGE_FILTER(ch))
		free(EXCHANGE_FILTER(ch));
	EXCHANGE_FILTER(ch) = str_dup(filter);

	return true;
}

EXCHANGE_ITEM_DATA *create_exchange_item(void)
{
	EXCHANGE_ITEM_DATA *item;
	CREATE(item, 1);
	GET_EXCHANGE_ITEM_LOT(item) = -1;
	GET_EXCHANGE_ITEM_SELLERID(item) = -1;
	GET_EXCHANGE_ITEM_COST(item) = 0;
	GET_EXCHANGE_ITEM_TIME(item) = time(NULL);
	GET_EXCHANGE_ITEM_COMMENT(item) = NULL;
	GET_EXCHANGE_ITEM(item) = NULL;
	item->next = exchange_item_list;
	exchange_item_list = item;
	return (item);
}

void extract_exchange_item(EXCHANGE_ITEM_DATA * item)
{
	REMOVE_FROM_LIST(item, exchange_item_list);
	if (GET_EXCHANGE_ITEM_LOT(item) > 0 && GET_EXCHANGE_ITEM_LOT(item) <= (int) lot_usage.size())
		lot_usage[GET_EXCHANGE_ITEM_LOT(item) - 1] = false;
	if (item->comment)
		free(item->comment);
	if (item->obj)
		extract_obj(item->obj);
	free(item);

}

void check_exchange(OBJ_DATA * obj)
{
	if (!obj)
	{
		return;
	}
	EXCHANGE_ITEM_DATA *j, *next_thing;

	for (j = exchange_item_list; j; j = next_thing)
	{
		next_thing = j->next;
		if (GET_EXCHANGE_ITEM(j) == obj)
		{
			REMOVE_FROM_LIST(j, exchange_item_list);
			lot_usage[GET_EXCHANGE_ITEM_LOT(j) - 1] = false;
			if (j->comment)
			{
				free(j->comment);
			}
			free(j);
			break;
		}
	}
}

EXCHANGE_ITEM_DATA *exchange_read_one_object_new(char **data, int *error)
{
	char buffer[MAX_STRING_LENGTH];
	EXCHANGE_ITEM_DATA *item = NULL;
	//char *d;

	*error = 1;
	// Станем на начало предмета
	for (; **data != EX_NEW_ITEM_CHAR; (*data)++)
		if (!**data || **data == EX_END_CHAR)
			return (item);

	*error = 2;

	// Пропустим #
	(*data)++;

	// Считаем lot предмета
	if (!get_buf_line(data, buffer))
		return (item);

	*error = 3;
	item = create_exchange_item();
	if ((GET_EXCHANGE_ITEM_LOT(item) = atoi(buffer)) <= 0)
		return (item);

	*error = 4;
	// Считаем seler_id предмета
	if (!get_buf_line(data, buffer))
		return (item);
	*error = 5;
	if (!(GET_EXCHANGE_ITEM_SELLERID(item) = atoi(buffer)))
		return (item);
	*error = 6;
	// Считаем цену предмета
	if (!get_buf_line(data, buffer))
		return (item);

	*error = 7;
	if (!(GET_EXCHANGE_ITEM_COST(item) = atoi(buffer)))
		return (item);

	if (!get_buf_line(data, buffer))
		return (item);
	*error = 8;
	if (!(GET_EXCHANGE_ITEM_TIME(item) = atoi(buffer)))
		return (item);
	*error = 9;
	// Считаем comment предмета
	char *str_last_symb = strchr(*data, '\n');
	strncpy(buffer, *data, str_last_symb - *data);
	buffer[str_last_symb - *data] = '\0';
	*data = str_last_symb;

	if (strcmp(buffer, "EMPTY"))
		if (!(GET_EXCHANGE_ITEM_COMMENT(item) = str_dup(buffer)))
			return (item);

	*error = 0;

	GET_EXCHANGE_ITEM(item) = read_one_object_new(data, error).get();
	if (*error)
	{
		*error = 9;
	}

	return (item);
}

void exchange_write_one_object_new(std::stringstream &out, EXCHANGE_ITEM_DATA * item)
{
	out << "#" << GET_EXCHANGE_ITEM_LOT(item) << "\n"
		<< GET_EXCHANGE_ITEM_SELLERID(item) << "\n"
		<< GET_EXCHANGE_ITEM_COST(item) << "\n"
		<< GET_EXCHANGE_ITEM_TIME(item) << "\n"
		<< (GET_EXCHANGE_ITEM_COMMENT(item) ? GET_EXCHANGE_ITEM_COMMENT(item) : "EMPTY\n")
		<< "\n";
	write_one_object(out, GET_EXCHANGE_ITEM(item), 0);
	out << "\n";
}

int exchange_database_load()
{
	FILE *fl;
	char *data, *readdata;
	EXCHANGE_ITEM_DATA *item;
	int fsize, error, max_lot = 0;
	char buffer[MAX_STRING_LENGTH];

	log("Exchange: loading database... (exchange.cpp)");

	if (!(fl = fopen(EXCHANGE_DATABASE_FILE, "r")))
	{
		log("SYSERR: Error opening exchange database. (exchange.cpp)");
		return (0);
	}

	fseek(fl, 0L, SEEK_END);
	fsize = ftell(fl);

	CREATE(readdata, fsize + 1);
	fseek(fl, 0L, SEEK_SET);
	auto actual_size = fread(readdata, 1, fsize, fl);
	if (!actual_size || ferror(fl))
	{
		fclose(fl);
		log("SYSERR: Memory error or cann't read exchange database file. (exchange.cpp)");
		free(readdata);
		return (0);
	};
	fclose(fl);

	data = readdata;
	*(data + actual_size) = '\0';

	// Новая база или старая?
	get_buf_line(&data, buffer);
	if (strstr(buffer, "!NEW!") == NULL)
	{
		log("SYSERR: Old format exchange database file.");
		return 0;
	}

	for (fsize = 0; *data && *data != EX_END_CHAR; fsize++)
	{
		item = exchange_read_one_object_new(&data, &error);

		if (item == NULL)
		{
			log("SYSERR: Error #%d reading exchange database file. (exchange.cpp)", error);
			return (0);
		}

		if (error)
		{
			log("SYSERR: Error #%d reading item from exchange database.", error);
			extract_exchange_item(item);
			continue;
		}

		// Предмет разваливается от старости
		if (GET_EXCHANGE_ITEM(item)->get_timer() <= 0)
		{
			std::string cap = GET_EXCHANGE_ITEM(item)->get_PName(0);
			cap[0] = UPPER(cap[0]);
			log("Exchange: - %s рассыпал%s от длительного использования.\r\n",
				cap.c_str(), GET_OBJ_SUF_2(GET_EXCHANGE_ITEM(item)));
			extract_exchange_item(item);
			continue;
		}
		max_lot = MAX(max_lot, GET_EXCHANGE_ITEM_LOT(item));
	}

	free(readdata);

	lot_usage.resize(max_lot, false);
	for (item = exchange_item_list; item; item = item->next)
		lot_usage[GET_EXCHANGE_ITEM_LOT(item) - 1] = true;

	log("Exchange: done loading database.");

	return (1);
}


int exchange_database_reload(bool loadbackup)
{
	FILE *fl;
	char *data, *readdata;
	EXCHANGE_ITEM_DATA *item, *j, *next_thing;
	int fsize, error, max_lot = 0;
	char buffer[MAX_STRING_LENGTH];

	for (j = exchange_item_list; j; j = next_thing)
	{
		next_thing = j->next;
		extract_exchange_item(j);
	}
	if (loadbackup)
	{
		log("Exchange: reloading backup of database... (exchange.cpp)");
		if (!(fl = fopen(EXCHANGE_DATABASE_BACKUPFILE, "r")))
		{
			log("SYSERR: Error opening exchange database backup. (exchange.cpp)");
			return (0);
		}
	}
	else
	{
		log("Exchange: reloading database... (exchange.cpp)");
		if (!(fl = fopen(EXCHANGE_DATABASE_FILE, "r")))
		{
			log("SYSERR: Error opening exchange database. (exchange.cpp)");
			return (0);
		}
	}

	fseek(fl, 0L, SEEK_END);
	fsize = ftell(fl);

	CREATE(readdata, fsize + 1);
	fseek(fl, 0L, SEEK_SET);
	auto actual_size = fread(readdata, 1, fsize, fl);
	if (!actual_size || ferror(fl))
	{
		fclose(fl);
		if (loadbackup)
			log("SYSERR: Memory error or cann't read exchange database backup file. (exchange.cpp)");
		else
			log("SYSERR: Memory error or cann't read exchange database file. (exchange.cpp)");
		free(readdata);
		return (0);
	};
	fclose(fl);

	data = readdata;
	*(data + actual_size) = '\0';

	// Новая база или старая?
	get_buf_line(&data, buffer);
	if (strstr(buffer, "!NEW!") == NULL)
	{
		log("SYSERR: Old format exchange database file.");
		return 0;
	}

	for (fsize = 0; *data && *data != EX_END_CHAR; fsize++)
	{
		item = exchange_read_one_object_new(&data, &error);

		if (item == NULL)
		{
			if (loadbackup)
				log("SYSERR: Error #%d reading exchange database backup file. (exchange.cpp)", error);
			else
				log("SYSERR: Error #%d reading exchange database file. (exchange.cpp)", error);
			return (0);
		}

		if (error)
		{
			if (loadbackup)
				log("SYSERR: Error #%d reading item from exchange database backup.", error);
			else
				log("SYSERR: Error #%d reading item from exchange database.", error);
			extract_exchange_item(item);
			continue;
		}

		// Предмет разваливается от старости
		if (GET_EXCHANGE_ITEM(item)->get_timer() <= 0)
		{
			std::string cap = GET_EXCHANGE_ITEM(item)->get_PName(0);
			cap[0] = UPPER(cap[0]);
			log("Exchange: - %s рассыпал%s от длительного использования.\r\n",
				cap.c_str(), GET_OBJ_SUF_2(GET_EXCHANGE_ITEM(item)));
			extract_exchange_item(item);
			continue;
		}
		max_lot = MAX(max_lot, GET_EXCHANGE_ITEM_LOT(item));
	}

	free(readdata);

	lot_usage.resize(max_lot, false);
	for (item = exchange_item_list; item; item = item->next)
		lot_usage[GET_EXCHANGE_ITEM_LOT(item) - 1] = true;

	if (loadbackup)
		log("Exchange: done reloading database backup.");
	else
		log("Exchange: done reloading database.");

	return (1);
}

/**
* \param backup - false (дефолт) обычное сохранение
*               - true сохранение в бэкап, вместо основного файла
*/
void exchange_database_save(bool backup)
{
	log("Exchange: Saving exchange database...");

	std::stringstream out;
	out << "!NEW!\n";
	EXCHANGE_ITEM_DATA *j, *next_thing;
	for (j = exchange_item_list; j; j = next_thing)
	{
		next_thing = j->next;
		exchange_write_one_object_new(out, j);
	}
	out << "$\n$\n";

	const char *filename = backup ? EXCHANGE_DATABASE_BACKUPFILE : EXCHANGE_DATABASE_FILE;
	std::ofstream file(filename);
	if (!file.is_open())
	{
		sprintf(buf, "[SYSERROR] open exchange db ('%s')", filename);
		mudlog(buf, BRF, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
	file << out.rdbuf();
	file.close();

	log("Exchange: done saving database.");
	return;
}

int get_unique_lot(void)
{
	int i;
	for (i = 0; i < (int) lot_usage.size(); i++)
		if (!lot_usage[i])
		{
			lot_usage[i] = true;
			return i + 1;
		}

	if (lot_usage.size() < INT_MAX)
	{
		lot_usage.push_back(true);
		return i + 1;
	}
	else
		return -1;
}


void message_exchange(char *message, CHAR_DATA * ch, EXCHANGE_ITEM_DATA * j)
{
	for (DESCRIPTOR_DATA *i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) == CON_PLAYING
			&& (!ch || i != ch->desc)
			&& i->character
			&& !PRF_FLAGGED(i->character, PRF_NOEXCHANGE)
			&& !PLR_FLAGGED(i->character, PLR_WRITING)
			&& !ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF)
			&& GET_POS(i->character) > POS_SLEEPING)
		{
			if (!PRF_FLAGGED(i->character, PRF_NOINGR_MODE)
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) == OBJ_DATA::ITEM_INGREDIENT
					|| GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) == OBJ_DATA::ITEM_MING))
			{
				continue;
			}
			ParseFilter params(ParseFilter::EXCHANGE);
			if (!EXCHANGE_FILTER(i->character)
				|| (parse_exch_filter(params, EXCHANGE_FILTER(i->character), false)
					&& params.check(j)))
			{
				if (COLOR_LEV(i->character) >= C_NRM)
				{
					send_to_char("&Y&q", i->character.get());
				}

				act(message, FALSE, i->character.get(), 0, 0, TO_CHAR | TO_SLEEP);

				if (COLOR_LEV(i->character) >= C_NRM)
				{
					send_to_char("&Q&n", i->character.get());
				}
			}
		}
	}
}

void show_lots(char *filter, short int show_type, CHAR_DATA * ch)
{
	/*
	show_type
	0 - все
	1 - мои
	2 - руны
	3 - одежда
	4 - оружие
	5 - книги
	6 - ингры
	7 - прочие
	8 - легкие доспехи
	9 - средние доспехи
	10 - тяжелые доспехи
	11 - аффект
	*/
	char tmpbuf[MAX_INPUT_LENGTH];
	bool any_item = 0;

	ParseFilter params(ParseFilter::EXCHANGE);
	if (!parse_exch_filter(params, filter, true))
	{
		send_to_char("Неверная строка фильтрации!\r\n", ch);
		log("Exchange: Player uses wrong filter '%s'", filter);
		return;
	}

	std::string buffer;
	if (show_type == 11)
	{
		buffer += params.print();
	}
	buffer +=
		" Лот     Предмет                                                     Цена  Состояние\r\n"
		"--------------------------------------------------------------------------------------------\r\n";

	for (EXCHANGE_ITEM_DATA* j = exchange_item_list; j; j = j->next)
	{
		if (!params.check(j)
			|| ((show_type == 1)
				&& (!isname(GET_NAME(ch), get_name_by_id(GET_EXCHANGE_ITEM_SELLERID(j)))))
			|| ((show_type == 2)
				&& ((GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_INGREDIENT)
					|| (GET_OBJ_VNUM(GET_EXCHANGE_ITEM(j)) < 200)
					|| (GET_OBJ_VNUM(GET_EXCHANGE_ITEM(j)) > 299)))
			|| ((show_type == 3)
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_ARMOR)
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_ARMOR_LIGHT)
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_ARMOR_MEDIAN)
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_ARMOR_HEAVY))
			|| ((show_type == 4)
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_WEAPON))
			|| ((show_type == 5)
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_BOOK))
			|| (show_type == 6
				&& GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_MING
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_INGREDIENT
					|| (GET_OBJ_VNUM(GET_EXCHANGE_ITEM(j)) >= 200
						&& GET_OBJ_VNUM(GET_EXCHANGE_ITEM(j)) <= 299)))
			|| ((show_type == 7)
				&& ((GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) == OBJ_DATA::ITEM_INGREDIENT)
					|| ObjSystem::is_armor_type(GET_EXCHANGE_ITEM(j))
					|| (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) == OBJ_DATA::ITEM_WEAPON)
					|| (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) == OBJ_DATA::ITEM_BOOK)
					|| (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) == OBJ_DATA::ITEM_MING)))
			|| ((show_type == 8)
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_ARMOR_LIGHT))
			|| ((show_type == 9)
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_ARMOR_MEDIAN))
			|| ((show_type == 10)
				&& (GET_OBJ_TYPE(GET_EXCHANGE_ITEM(j)) != OBJ_DATA::ITEM_ARMOR_HEAVY)))
		{
			continue;
		}
		// ну идиотизм сидеть статить 5-10 страниц резных
		if (is_abbrev("резное запястье", GET_EXCHANGE_ITEM(j)->get_PName(0).c_str())
			|| is_abbrev("широкое серебряное обручье", GET_EXCHANGE_ITEM(j)->get_PName(0).c_str())
			|| is_abbrev("медное запястье", GET_EXCHANGE_ITEM(j)->get_PName(0).c_str()))
		{
			GET_EXCHANGE_ITEM(j)->get_affect_flags().sprintbits(weapon_affects, buf, ",");
			// небольшое дублирование кода, чтобы зря не гонять по аффектам всех шмоток
			if (!strcmp(buf, "ничего"))  // added by WorM (Видолюб) отображение не только аффектов, но и доп.свойств запястий
			{
				bool found = false;
				int drndice = 0, drsdice = 0;

				for (int n = 0; n < MAX_OBJ_AFFECT; n++)
				{
					drndice = GET_EXCHANGE_ITEM(j)->get_affected(n).location;
					drsdice = GET_EXCHANGE_ITEM(j)->get_affected(n).modifier;
					if ((drndice != APPLY_NONE) && (drsdice != 0))
					{
						sprinttype(drndice, apply_types, buf2);
						bool negative = false;
						for (int k = 0; *apply_negative[k] != '\n'; k++)
						{
							if (!str_cmp(buf2, apply_negative[k]))
							{
								negative = true;
								break;
							}
						}
						if (drsdice < 0)
							negative = !negative;
						snprintf(buf, MAX_STRING_LENGTH, "%s %s%d", buf2, negative ? "-" : "+", abs(drsdice));
						found = true;
						break;
					}
				}

				if (!found)
				{
					sprintf(tmpbuf, "[%4d]   %s", GET_EXCHANGE_ITEM_LOT(j), GET_OBJ_PNAME(GET_EXCHANGE_ITEM(j), 0).c_str());
				}
				else
				{
					snprintf(tmpbuf, MAX_INPUT_LENGTH, "[%4d]   %s (%s)", GET_EXCHANGE_ITEM_LOT(j), GET_OBJ_PNAME(GET_EXCHANGE_ITEM(j), 0).c_str(), buf);
				}
			}
			else  // end by WorM
			{
				snprintf(tmpbuf, MAX_INPUT_LENGTH, "[%4d]   %s (%s)", GET_EXCHANGE_ITEM_LOT(j), 
					GET_OBJ_PNAME(GET_EXCHANGE_ITEM(j), 0).c_str(), buf);
			}
		}
		else if (is_dig_stone(GET_EXCHANGE_ITEM(j))
			&& GET_OBJ_MATER(GET_EXCHANGE_ITEM(j)) == OBJ_DATA::MAT_GLASS)
		{
			sprintf(tmpbuf, "[%4d]   %s (стекло)", GET_EXCHANGE_ITEM_LOT(j), GET_OBJ_PNAME(GET_EXCHANGE_ITEM(j), 0).c_str());
		}
		else
		{
			sprintf(tmpbuf, "[%4d]   %s%s", GET_EXCHANGE_ITEM_LOT(j), GET_OBJ_PNAME(GET_EXCHANGE_ITEM(j), 0).c_str(), params.show_obj_aff(GET_EXCHANGE_ITEM(j)).c_str());
		}
		char *tmstr;
		tmstr = (char *) asctime(localtime(&GET_EXCHANGE_ITEM_TIME(j)));
		if (IS_GOD(ch)) //asctime добавляет перевод строки лишний
			sprintf(tmpbuf, "%s %9d  %-s %s", colored_name(tmpbuf, 63, true), GET_EXCHANGE_ITEM_COST(j), diag_obj_timer(GET_EXCHANGE_ITEM(j)), tmstr);
		else
			sprintf(tmpbuf, "%s %9d  %-s\r\n", colored_name(tmpbuf, 63, true), GET_EXCHANGE_ITEM_COST(j), diag_obj_timer(GET_EXCHANGE_ITEM(j)));
		// Такое вот кино, на выделения для каждой строчки тут уходило до 0.6 секунды при выводе всего базара. стринги рулят -- Krodo
		buffer += tmpbuf;
		any_item = 1;
	}

	if (!any_item)
	{
		buffer = "Базар пуст!\r\n";
	}
	else if (show_type == 11)
	{
		const int price = EXCHANGE_IDENT_PAY / 2;
		ch->remove_both_gold(price);
		send_to_char(ch, "\r\n%sЗа информацию об аффектах с вашего банковского счета сняли %d %s%s\r\n",
			CCIGRN(ch, C_NRM), price, desc_count(price, WHAT_MONEYu), CCNRM(ch, C_NRM));
	}
	page_string(ch->desc, buffer);
}

int parse_exch_filter(ParseFilter &filter, char *buf, bool parse_affects)
{
	char tmpbuf[FILTER_LENGTH];

	while (*buf && (*buf != '\r') && (*buf != '\n'))
	{
		switch (*buf)
		{
		case ' ':
			buf++;
			break;
		case 'И':
			buf = one_argument(++buf, tmpbuf);
			filter.name = tmpbuf;
			break;
		case 'В':
			buf = one_argument(++buf, tmpbuf);
			filter.owner = tmpbuf;
			//filter.owner_id = get_id_by_name(tmpbuf);
			break;
		case 'Т':
			buf = one_argument(++buf, tmpbuf);
			if (!filter.init_type(tmpbuf))
				return 0;
			break;
		case 'Ц':
			buf = one_argument(++buf, tmpbuf);
			if (!filter.init_cost(tmpbuf))
				return 0;
			break;
		case 'С':
			buf = one_argument(++buf, tmpbuf);
			if (!filter.init_state(tmpbuf))
				return 0;
			break;
		case 'О':
			buf = one_argument(++buf, tmpbuf);
			if (!filter.init_wear(tmpbuf))
				return 0;
			break;
		case 'К':
			buf = one_argument(++buf, tmpbuf);
			if (!filter.init_weap_class(tmpbuf))
				return 0;
			break;
		case 'А':
			if (!parse_affects)
				return 0;
			buf = one_argument(++buf, tmpbuf);
			if (filter.affects_cnt() >= 1)
				break;
			else if (!filter.init_affect(tmpbuf, strlen(tmpbuf)))
				return 0;
			break;
		case 'Р':
			buf = one_argument(++buf, tmpbuf);
			if (!filter.init_realtime(tmpbuf))
				return 0;
			break;
		default:
			return 0;
		}
	}

	return 1;

}

void clear_exchange_lot(EXCHANGE_ITEM_DATA * lot)
{
	if (lot == NULL)
	{
		return;
	}

	REMOVE_FROM_LIST(lot, exchange_item_list);
	lot_usage[GET_EXCHANGE_ITEM_LOT(lot) - 1] = false;
	if (lot->comment)
	{
		free(lot->comment);
	}
	free(lot);
}

//Polud дублируем кучку кода, чтобы можно было часть команд базара выполнять в любой комнате
void do_exchange(CHAR_DATA *ch, char *argument, int cmd, int/* subcmd*/)
{
		char* arg = str_dup(argument);
		argument = one_argument(argument, arg1);

		if (IS_NPC(ch)) {
			send_to_char("Торговать?! Да вы же не человек!\r\n", ch);
		} else if ((is_abbrev(arg1, "выставить") || is_abbrev(arg1, "exhibit")
		|| is_abbrev(arg1, "цена") || is_abbrev(arg1, "cost")
		|| is_abbrev(arg1, "снять") || is_abbrev(arg1, "withdraw")
		|| is_abbrev(arg1, "купить") || is_abbrev(arg1, "purchase")) && !IS_IMPL(ch))
		{
			send_to_char("Вам необходимо находиться возле базарного торговца, чтобы воспользоваться этой командой.\r\n", ch);
		} else
			exchange(ch,NULL,cmd,arg);
		free(arg);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
