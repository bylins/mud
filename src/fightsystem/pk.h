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

#ifndef _PVP_H_
#define _PVP_H_

#include "entities/char_data.h"

#include <string>

class ObjData;    // forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.

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
struct PK_Memory_type {
	long unique;        // unique игрока
	long kill_num;        // количество флагов носителя структуры на unique
	long kill_at;        // время получения последнего флага (для spamm)
	long revenge_num;    // количетсво попыток реализации мести со стороны unique
	long battle_exp;    // время истечения поединка
	long thief_exp;        // время истечения флага воровства
	long clan_exp;        // время истечения клан-флага
	struct PK_Memory_type *next;
};

const short MAX_REVENGE = 2;    // Максимальное количество попыток реализации мести

#define        PK_ACTION_NO        1    // никаких конфликтов
#define        PK_ACTION_FIGHT     2    // действия в процессе поединка
#define        PK_ACTION_REVENGE    4    // попытка реализовать месть
#define        PK_ACTION_KILL        8    // нападение

// agressor действует против victim
// Результат - тип действий (см. выше)
int pk_action_type(CharData *agressor, CharData *victim);

// Проверка может ли ch начать аргессивные действия против victim
// true - может
// false - не может
int may_kill_here(CharData *ch, CharData *victim, char *argument);

// проверка на агробд для сетов
bool check_agrobd(CharData *ch);

// Определение необходимости вводить имя жертвы полностью
bool need_full_alias(CharData *ch, CharData *opponent);

//Определение, является ли строка arg полным именем ch
int name_cmp(CharData *ch, const char *arg);

// Определение возможности агродействий
int check_pkill(CharData *ch, CharData *opponent, const char *arg);
int check_pkill(CharData *ch, CharData *opponent, const std::string &arg);

// agressor проводит действия против victim
bool pk_agro_action(CharData *agressor, CharData *victim);

// thief проводит действия против victim, приводящие к скрытому флагу
void pk_thiefs_action(CharData *thief, CharData *victim);

// killer убивает victim
void pk_revenge_action(CharData *killer, CharData *victim);

// удалить список ПК
void pk_free_list(CharData *ch);

// посчитать количество флагов мести
int pk_count(CharData *ch);

//Количество местей на игроков (уникальное мыло)
int pk_player_count(CharData *ch);

//*************************************************************************
// Информационные функции отображения статуса ПК

void aura(CharData *ch, int lvl, CharData *victim, char *s);
const char *CCPK(CharData *ch, int lvl, CharData *victim);
inline const char *CCPK(CharData *ch, int lvl, const CharData::shared_ptr &victim) {
	return CCPK(ch,
				lvl,
				victim.get());
}
void pk_list_sprintf(CharData *ch, char *buff);
void do_revenge(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/);

//*************************************************************************
// Системные функции сохранения/загрузки ПК флагов
void save_pkills(CharData *ch, FILE *saved);

//*************************************************************************
bool has_clan_members_in_group(CharData *ch);

//проверяем не чармис ли это наш или группы
//bool check_charmise(CharacterData * ch, CharacterData * victim, char * argument);

//Polud
void pkPortal(CharData *ch);

//Кровавый стаф
namespace bloody {
//обновляет флаг кровавости на стафе
void update();
//убирает флаг
void remove_obj(const ObjData *obj);

//обработка передачи предмета obj от ch к victim
//ch может быть NULL (лут)
//victim может быть null (в случае с бросить, аук, продать..)
//возвращает true, если передача может состояться, и false в противном случае
bool handle_transfer(CharData *ch, CharData *victim, ObjData *obj, ObjData *container = nullptr);
//Помечает стаф в трупе как кровавый
void handle_corpse(ObjData *corpse, CharData *ch, CharData *killer);
bool is_bloody(const ObjData *obj);
}

//Структура для хранения информации о кровавом стафе
//Чтобы не добавлять новых полей в ObjectData, объект просто помечается экстрафлагом ITEM_BLOODY и добавляется запись в bloody_map
struct BloodyInfo {
	long owner_unique; //С кого сняли шмотку
	long kill_at; //Когда произошло убийство
	ObjData *object; //сама шмотка
	//какой-либо ID вместо указателя хранить не получается, потому что тогда при апдейте таймера кровавости для каждой записи придется искать шмотку по id по списку шмоток
	//А это O(n)
	//По-этому в деструкторе ObjectData нужно удалять запись о шмотке из bloody_map. Такой вот костыль в отсутствие shared_ptr'ов и иже с ними
	BloodyInfo(const long _owner_unique = 0, const long _kill_at = 0, ObjData *_object = 0) :
		owner_unique(_owner_unique), kill_at(_kill_at), object(_object) {}
};

typedef std::map<const ObjData *, BloodyInfo> BloodyInfoMap;

#endif // _PVP_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
