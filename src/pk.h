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

//*************************************************************************
// Основные функци и разрешения конфликтных ситуаций между игроками

// Использование функций:
//   1. Вызов функции may_kill_here()
//      Имеем ли право атаковать жертву (проверяются ПК флаги, мирки и т.д.)
//   2. Вызов функции check_pkill()
//      Определяем потенциальную возможность ПК акта и заставляем
//      вводить имя жертвы полностью
//   3. Вызов функции pk_agro_action() при каждой агрессии
//   4. Вызов функции pk_thiefs_action() при воровстве
//   5. Вызов функции pk_revenge_action() в случае убийства

// Структуры для сохранения ПК информаци
struct PK_Memory_type {
	long unique;		// unique игрока
	long kill_num;		// количество флагов носителя структуры на unique
	long kill_at;		// время получения последнего флага (для spamm)
	long revenge_num;	// количетсво попыток реализации мести со стороны unique
	long battle_exp;	// время истечения поединка
	long thief_exp;		// время истечения флага воровства
	long clan_exp;		// время истечения клан-флага
	long pentagram_exp;
	struct PK_Memory_type *next;
};

#define		PK_ACTION_NO		1	// никаких конфликтов
#define		PK_ACTION_FIGHT     2	// действия в процессе поединка
#define		PK_ACTION_REVENGE	4	// попытка реализовать месть
#define		PK_ACTION_KILL		8	// нападение
#define		PK_ACTION_PENTAGRAM_REVENGE		16	// Pent restiction override

// agressor действует против victim 
// Результат - тип действий (см. выше)
int pk_action_type(CHAR_DATA * agressor, CHAR_DATA * victim);

// Проверка может ли ch начать аргессивные действия против victim
// TRUE - может
// FALSE - не может
int may_kill_here(CHAR_DATA * ch, CHAR_DATA * victim);

// Определение возможных ПК действий
int check_pkill(CHAR_DATA * ch, CHAR_DATA * opponent, char *arg);

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

//Добавить флаг нарушителя пентаграммы
void set_pentagram_pk(CHAR_DATA * ch, bool isPortalEnter, int isGates);
void remove_pent_pk(CHAR_DATA * agressor, CHAR_DATA * victim);
//*************************************************************************
// Информационные функции отображения статуса ПК

void aura(CHAR_DATA * ch, int lvl, CHAR_DATA * victim, char *s);
const char *CCPK(CHAR_DATA * ch, int lvl, CHAR_DATA * victim);
void pk_list_sprintf(CHAR_DATA * ch, char *buff);

//*************************************************************************
// Системные функции сохранения/загрузки ПК флагов

void load_pkills(CHAR_DATA * ch);
void save_pkills(CHAR_DATA * ch);
void save_pkills(CHAR_DATA * ch);

//*************************************************************************
bool has_clan_members_in_group(CHAR_DATA * ch);
