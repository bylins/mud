/**************************************************************************
*   File: exchange.h                                 Part of Bylins       *
*  Usage: Exhange headers functions used by the MUD                       *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _EXCHANGE_HPP_
#define _EXCHANGE_HPP_

#include <vector>
#include <ctime>

class CHAR_DATA;	// to avoid inclusion of "char.hpp"
class OBJ_DATA;		// to avoid inclusion of "obj.hpp"

int exchange(CHAR_DATA *ch, void *me, int cmd, char* argument);

typedef struct exchange_item_data EXCHANGE_ITEM_DATA;

extern EXCHANGE_ITEM_DATA *exchange_item_list;
extern std::vector<bool> lot_usage;

#define EXCHANGE_AUTOSAVETIME 300	//Кол-во секунд между автосохранениями Базара (0 для отключения)
#define EXCHANGE_AUTOSAVEBACKUPTIME 750	//Кол-во секунд между автосохранениями Базара (0 для отключения)
#define EXCHANGE_SAVEONEVERYOPERATION FALSE	//Сохранять базар после каждой операции
#define EXCHANGE_DATABASE_FILE LIB_PLRSTUFF"exchange.db"
#define EXCHANGE_DATABASE_BACKUPFILE LIB_PLRSTUFF"exchange.backup"
#define EX_NEW_ITEM_CHAR '#'
#define EX_END_CHAR '$'
#define FILTER_LENGTH 25
#define EXCHANGE_EXHIBIT_PAY 100	// Плата за выставление на базар
#define EXCHANGE_EXHIBIT_PAY_COEFF 0.05	// Коэффициент оплаты в зависимости от цены товара
#define EXCHANGE_IDENT_PAY 110	//куны за опознание
#define EXCHANGE_MAX_EXHIBIT_PER_CHAR 20	//максимальное кол-во выставляемых объектов одним чаром
//минимальный уровень для доступа к базару
const int EXCHANGE_MIN_CHAR_LEV = 8;

#define GET_EXCHANGE_ITEM_LOT(item)  ((item)->lot_id)
#define GET_EXCHANGE_ITEM_SELLERID(item)  ((item)->seller_id)
#define GET_EXCHANGE_ITEM_COST(item)  ((item)->obj_cost)
#define GET_EXCHANGE_ITEM_COMMENT(item)  ((item)->comment)
#define GET_EXCHANGE_ITEM(item)  ((item)->obj)
#define GET_EXCHANGE_ITEM_TIME(item)  ((item)->time)


void extract_exchange_item(EXCHANGE_ITEM_DATA * item);
void check_exchange(OBJ_DATA * obj);

struct exchange_item_data
{
	int lot_id;		//Номер лота
	int seller_id;		//Номер продавца
	int obj_cost;		//цена лота
	time_t time; // время
	char *comment;		//коментарий
	OBJ_DATA *obj;		//собственно предмет
	EXCHANGE_ITEM_DATA *next;	//для списка объектов базара
};

void exchange_database_save(bool backup = false);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
