/*************************************************************************
*   File: arena.hpp                                    Part of Bylins    *
*   Arena and toto code header                                           *
*                                                                        *
*  $Author$                                                       * 
*  $Date$                                          *
*  $Revision$                                                      *
**************************************************************************/

#ifndef __ARENA_HPP__
#define __ARENA_HPP__

typedef struct arena_data ARENA_DATA;
void message_arena(CHAR_DATA * ch, char *message);

#define MAX_ARENA_SLOTS 16

#define ARENA_CMD_ADD 		0
#define ARENA_CMD_DEL 		1
#define ARENA_CMD_OPEN 		2
#define ARENA_CMD_CLOSE	 	3
#define ARENA_CMD_WIN 		4
#define ARENA_CMD_LOSE		5
#define ARENA_CMD_LOCK		6
#define ARENA_CMD_UNLOCK	7


#define AS_WAIT		0
#define AS_LOSE		1
#define AS_WIN		2

typedef list < ARENA_DATA * >ArenaList;

ACMD(do_mngarena);
ACMD(do_arena);

struct arena_data {
	char *name;		// имя группы задается иммом
	int bet;		// общая сумма ставки
	int state;		// состояние (выйграл,проиграл,ожидание)
};


class Arena {
	ArenaList arena_slots;

	// Состояние арены.
	bool opened;
	bool locked;

      public:

	// Создание деструктор
	 Arena();

	~Arena();

	// Вывод списка на арене    
	int show(CHAR_DATA * ch);
	// Добаление в список         
	// Удаление с арены         
	int add(CHAR_DATA * ch, ARENA_DATA * new_arr);
	int del(CHAR_DATA * ch, ARENA_DATA * rem_arr);

	int win(CHAR_DATA * ch, ARENA_DATA * arenaman);
	int lose(CHAR_DATA * ch, ARENA_DATA * arenaman);

	int open(CHAR_DATA * ch);
	int close(CHAR_DATA * ch);

	int lock(CHAR_DATA * ch);
	int unlock(CHAR_DATA * ch);

	int is_open();
	int is_locked();

	void clear();

	ARENA_DATA *get_slot(int slot_num);

	int make_bet(CHAR_DATA * ch, int slot, int bet);

};

// Объявляем одну турнирную арену 
extern Arena tour_arena;

#endif
