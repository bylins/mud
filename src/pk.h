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

// ничего не отнимающая смерть в пк (пк-счетчики и статистика тоже не трогаются)
const bool FREE_PK_MODE = false;

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
int may_kill_here(CHAR_DATA * ch, CHAR_DATA * victim);

// Определение необходимости вводить имя жертвы полностью
bool need_full_alias(CHAR_DATA * ch, CHAR_DATA * opponent);

//Определение, является ли строка arg полным именем ch
int name_cmp(CHAR_DATA * ch, const char *arg);

// Определение возможности агродействий
int check_pkill(CHAR_DATA * ch, CHAR_DATA * opponent, const char *arg);
int check_pkill(CHAR_DATA * ch, CHAR_DATA * opponent, const std::string &arg);

// agressor проводит действия против victim
void pk_agro_action(CHAR_DATA * agressor, CHAR_DATA * victim);

// thief проводит действия против victim, приводящие к скрытому флагу
void pk_thiefs_action(CHAR_DATA * thief, CHAR_DATA * victim);

// killer убивает victim
void pk_revenge_action(CHAR_DATA * killer, CHAR_DATA * victim);

// удалить список ПК
void pk_free_list(CHAR_DATA * ch);

// посчитать количество флагов мести
int pk_count(CHAR_DATA * ch);

//*************************************************************************
// Информационные функции отображения статуса ПК

void aura(CHAR_DATA * ch, int lvl, CHAR_DATA * victim, char *s);
const char *CCPK(CHAR_DATA * ch, int lvl, CHAR_DATA * victim);
void pk_list_sprintf(CHAR_DATA * ch, char *buff);

//*************************************************************************
// Системные функции сохранения/загрузки ПК флагов
void save_pkills(CHAR_DATA * ch, FILE * saved);

//*************************************************************************
bool has_clan_members_in_group(CHAR_DATA * ch);
//Polud
void pkPortal(CHAR_DATA* ch);

// проверка возможности атаковать/кастить на моба, который сражается с каким-то игроком
bool check_group_assist(CHAR_DATA *ch, CHAR_DATA *victim);

#endif
