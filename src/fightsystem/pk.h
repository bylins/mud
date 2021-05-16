/* ************************************************************************
*   File: pk.h                                          Part of Bylins    *
*  Usage: header file: constants and fn prototypes for ПК система         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _PK_H_
#define _PK_H_

#include "chars/character.h"

#include <string>

class OBJ_DATA;	// forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.

//*************************************************************************
// Основные функци и разрешения конфликтных ситуаций между игроками

// Использование функций:
//   1. Вызов функции may_kill_here()
//      Имеем ли право атаковать жертву (проверяются ПК флаги, мирки и т.д.)
//   2. Вызов функции check_pkill()
//      Определяем потенциальную возможность агрессии и заставляем
//      вводить имя жертвы полностью
//   3. Вызов функции pk_agro_action() при каждой агрессии
//   4. Вызов функции pk_thiefs_action() при воровстве
//   5. Вызов функции pk_revenge_action() в случае убийства

// Структуры для сохранения ПК информаци
struct PK_Memory_type
{
	long unique;		// unique игрока
	long kill_num;		// количество флагов носителя структуры на unique
	long kill_at;		// время получения последнего флага (для spamm)
	long revenge_num;	// количетсво попыток реализации мести со стороны unique
	long battle_exp;	// время истечения поединка
	long thief_exp;		// время истечения флага воровства
	long clan_exp;		// время истечения клан-флага
	struct PK_Memory_type *next;
};

const short MAX_REVENGE = 2;	// Максимальное количество попыток реализации мести

#define		PK_ACTION_NO		1	// никаких конфликтов
#define		PK_ACTION_FIGHT     2	// действия в процессе поединка
#define		PK_ACTION_REVENGE	4	// попытка реализовать месть
#define		PK_ACTION_KILL		8	// нападение

// agressor действует против victim
// Результат - тип действий (см. выше)
int pk_action_type(CHAR_DATA * agressor, CHAR_DATA * victim);

// Проверка может ли ch начать аргессивные действия против victim
// TRUE - может
// FALSE - не может
int may_kill_here(CHAR_DATA * ch, CHAR_DATA * victim, char * argument);

// проверка на агробд для сетов
bool check_agrobd(CHAR_DATA *ch);

// Определение необходимости вводить имя жертвы полностью
bool need_full_alias(CHAR_DATA * ch, CHAR_DATA * opponent);

//Определение, является ли строка arg полным именем ch
int name_cmp(CHAR_DATA * ch, const char *arg);

// Определение возможности агродействий
int check_pkill(CHAR_DATA * ch, CHAR_DATA * opponent, const char *arg);
int check_pkill(CHAR_DATA * ch, CHAR_DATA * opponent, const std::string &arg);

// agressor проводит действия против victim
bool pk_agro_action(CHAR_DATA * agressor, CHAR_DATA * victim);

// thief проводит действия против victim, приводящие к скрытому флагу
void pk_thiefs_action(CHAR_DATA * thief, CHAR_DATA * victim);

// killer убивает victim
void pk_revenge_action(CHAR_DATA * killer, CHAR_DATA * victim);

// удалить список ПК
void pk_free_list(CHAR_DATA * ch);

// посчитать количество флагов мести
int pk_count(CHAR_DATA * ch);

//Количество местей на игроков (уникальное мыло)
int pk_player_count(CHAR_DATA * ch);

//*************************************************************************
// Информационные функции отображения статуса ПК

void aura(CHAR_DATA * ch, int lvl, CHAR_DATA * victim, char *s);
const char *CCPK(CHAR_DATA * ch, int lvl, CHAR_DATA * victim);
inline const char *CCPK(CHAR_DATA* ch, int lvl, const CHAR_DATA::shared_ptr& victim) { return CCPK(ch, lvl, victim.get()); }
void pk_list_sprintf(CHAR_DATA * ch, char *buff);
void do_revenge(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/);

//*************************************************************************
// Системные функции сохранения/загрузки ПК флагов
void save_pkills(CHAR_DATA * ch, FILE * saved);

//*************************************************************************
bool has_clan_members_in_group(CHAR_DATA * ch);

//проверяем не чармис ли это наш или группы
//bool check_charmise(CHAR_DATA * ch, CHAR_DATA * victim, char * argument);

//Polud
void pkPortal(CHAR_DATA* ch);

//Кровавый стаф
namespace bloody
{
	//обновляет флаг кровавости на стафе
	void update();
	//убирает флаг
	void remove_obj(const OBJ_DATA* obj);

	//обработка передачи предмета obj от ch к victim
	//ch может быть NULL (лут)
	//victim может быть null (в случае с бросить, аук, продать..)
	//возвращает true, если передача может состояться, и false в противном случае
	bool handle_transfer(CHAR_DATA* ch, CHAR_DATA* victim, OBJ_DATA* obj, OBJ_DATA* container=NULL);
	//Помечает стаф в трупе как кровавый
	void handle_corpse(OBJ_DATA* corpse, CHAR_DATA* ch, CHAR_DATA* killer);
	bool is_bloody(const OBJ_DATA* obj);
}

//Структура для хранения информации о кровавом стафе
//Чтобы не добавлять новых полей в OBJ_DATA, объект просто помечается экстрафлагом ITEM_BLOODY и добавляется запись в bloody_map
struct BloodyInfo
{
	long owner_unique; //С кого сняли шмотку
	long kill_at; //Когда произошло убийство
	OBJ_DATA * object; //сама шмотка
	//какой-либо ID вместо указателя хранить не получается, потому что тогда при апдейте таймера кровавости для каждой записи придется искать шмотку по id по списку шмоток
	//А это O(n)
	//По-этому в деструкторе OBJ_DATA нужно удалять запись о шмотке из bloody_map. Такой вот костыль в отсутствие shared_ptr'ов и иже с ними
	BloodyInfo(const long _owner_unique=0, const long _kill_at=0, OBJ_DATA* _object=0):
		owner_unique(_owner_unique), kill_at(_kill_at), object(_object) { }
};

typedef std::map<const OBJ_DATA*, BloodyInfo> BloodyInfoMap;

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
