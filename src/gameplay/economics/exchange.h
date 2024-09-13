/**************************************************************************
*   File: exchange.h                                 Part of Bylins       *
*  Usage: Exhange headers functions used by the MUD                       *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef EXCHANGE_HPP_
#define EXCHANGE_HPP_

#include <vector>
#include <ctime>

class CharData;    // to avoid inclusion of "char.hpp"
class ObjData;        // to avoid inclusion of "obj.hpp"

int exchange(CharData *ch, void *me, int cmd, char *argument);

struct ExchangeItem {
	int lot_id;        //Номер лота
	int seller_id;        //Номер продавца
	int obj_cost;        //цена лота
	time_t time; // время
	char *comment;        //коментарий
	ObjData *obj;        //собственно предмет
	ExchangeItem *next;    //для списка объектов базара
};

extern ExchangeItem *exchange_item_list;
extern std::vector<bool> lot_usage;

#define EXCHANGE_AUTOSAVETIME 300    //Кол-во секунд между автосохранениями Базара (0 для отключения)
#define EXCHANGE_AUTOSAVEBACKUPTIME 750    //Кол-во секунд между автосохранениями Базара (0 для отключения)
#define EXCHANGE_SAVEONEVERYOPERATION false    //Сохранять базар после каждой операции
#define EXCHANGE_DATABASE_FILE LIB_PLRSTUFF"exchange.db"
#define EXCHANGE_DATABASE_BACKUPFILE LIB_PLRSTUFF"exchange.backup"
#define EX_NEW_ITEM_CHAR '#'
#define EX_END_CHAR '$'
#define FILTER_LENGTH 250
#define EXCHANGE_EXHIBIT_PAY 100    // Плата за выставление на базар
#define EXCHANGE_EXHIBIT_PAY_COEFF 0.05    // Коэффициент оплаты в зависимости от цены товара
#define EXCHANGE_IDENT_PAY 110    //куны за опознание
#define EXCHANGE_MAX_EXHIBIT_PER_CHAR 20    //максимальное кол-во выставляемых объектов одним чаром
//минимальный уровень для доступа к базару
const int EXCHANGE_MIN_CHAR_LEV = 8;

#define GET_EXCHANGE_ITEM_LOT(item)  ((item)->lot_id)
#define GET_EXCHANGE_ITEM_SELLERID(item)  ((item)->seller_id)
#define GET_EXCHANGE_ITEM_COST(item)  ((item)->obj_cost)
#define GET_EXCHANGE_ITEM_COMMENT(item)  ((item)->comment)
#define GET_EXCHANGE_ITEM(item)  ((item)->obj)

void extract_exchange_item(ExchangeItem *item);
void check_exchange(ObjData *obj);

void exchange_database_save(bool backup = false);

#endif // EXCHANGE_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
