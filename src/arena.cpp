/*************************************************************************
*   File: item.creation.cpp                            Part of Bylins    *
*   Arena and toto code                                                  *
*                                                                        *
*  $Author$                                                       *
*  $Date$                                          *
*  $Revision$                                                     *
************************************************************************ */
// Возможности:
//1. Регистрация участников арены (добавить,удалить) - IMMORTALS
//2. Сделать ставку на участника арены - MORTALS
//3. Канал арена - трансляция сообщений от иммов в канал.
//4. Открытие арены 
//5. Флаг слышать арену или нет. (реж арена)

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "screen.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "arena.hpp"

Arena tour_arena;

const char *arstate[] = {
	"ожидание",
	"победил",
	"проиграл"
};

const char *arenamng_cmd[] = {
	"добавить", "add",
	"удалить", "del",
	"открыть", "open",
	"закрыть", "close",
	"выйграл", "win",
	"проиграл", "lose",
	"блокировать", "lock",
	"разблокировать", "unlock",
	"\n"
};

/* Отображает состояние арены и ставки */
ACMD(do_arena)
{
	// Выдает справку по состоянию арены и игрока 
	// Количество поставленных денег
	// Текущий и итоговый выйгрыш.
	// Общий призовой фонд. 
}

/* Вызывается управляющим иммом */
ACMD(do_mngarena)
{
	// Подкоманды : 
	// добавить - добавляем участника арены (если соревнуются группы то лидера)
	// удалить - удаляем участника арены
	// выйграл - присуждаем победу тому или иному участнику
	// проиграл - присуждаем проигрыш все деньги проигравших списываются. ???

	// закрыть - закрываем арену - ставки запрещены (деньги возвращаются)
	// открыть - открываем арену - ставки разрешаются
	ARENA_DATA *tad;

	int mode, num;


	if (!*argument) {
		tour_arena.show(ch);
		return;
	}

	argument = one_argument(argument, arg);

	if ((mode = search_block(arg, arenamng_cmd, FALSE)) < 0) {
		send_to_char(ch, "Команды управления ареной : \r\n"
			     "добавить, удалить, открыть, выйграл, проиграл,\r\n"
			     "закрыть, блокировать, разблокировать.\r\n");
		return;
	}

	mode >>= 1;
	switch (mode) {
	case ARENA_CMD_ADD:
		// Добавляем участника ... формат имя участника 
		if (!tour_arena.is_open())
			send_to_char(ch, "Необходима сначало открыть арену.\r\n");
		else if (!tour_arena.is_locked())
			send_to_char(ch, "Заблокируйте арену от приема ставок.\r\n");
		else {
			skip_spaces(&argument);
			if (!*argument) {
				send_to_char("Формат: mngarena добавить имя.участника\r\n", ch);
				return;
			}
			send_to_char(ch, "Добавляем нового участника.\r\n");
			// Создаем запись
			CREATE(tad, ARENA_DATA, 1);
			tad->name = str_dup(argument);
			tad->bet = 0;
			tad->state = AS_WAIT;
			tour_arena.add(ch, tad);
		}
		break;

	case ARENA_CMD_DEL:
		if (!tour_arena.is_open())
			send_to_char(ch, "Необходима сначало открыть арену.\r\n");
		else if (!tour_arena.is_locked())
			send_to_char(ch, "Заблокируйте арену от приема ставок.\r\n");
		else {
			if (!sscanf(argument, "%d", &num))
				send_to_char(ch, "Формат: mngarena удалить номер.участника\r\n");
			else if ((tad = tour_arena.get_slot(num)) == NULL)
				send_to_char(ch, "Участника под таким номером не существует.\r\n");
			else {
				send_to_char(ch, "Удаляем участника под номером %d.\r\n", num);
				tour_arena.del(ch, tad);
			}
		}
		break;

	case ARENA_CMD_OPEN:
		if (tour_arena.is_open())
			send_to_char(ch, "Арена уже открыта.\r\n");
		else {
			send_to_char(ch, "Открываем арену.\r\n");
			tour_arena.open(ch);
		}
		break;

	case ARENA_CMD_CLOSE:
		if (!tour_arena.is_open())
			send_to_char(ch, "Арена уже закрыта.\r\n");
		else {
			send_to_char(ch, "Закрываем арену.\r\n");
			tour_arena.close(ch);
		}
		break;

	case ARENA_CMD_WIN:
		if (!tour_arena.is_open())
			send_to_char(ch, "Необходимо сначало открыть арену.\r\n");
		else if (!tour_arena.is_locked())
			send_to_char(ch, "Заблокируйте прием ставок.\r\n");
		else {
			if (!sscanf(argument, "%d", &num))
				send_to_char(ch, "Формат: mngarena удалить номер.участника\r\n");
			else if ((tad = tour_arena.get_slot(num)) == NULL)
				send_to_char(ch, "Участника под таким номером не существует.\r\n");
			else {
				send_to_char(ch, "Участнику (%s) присуждена победа.\r\n", tad->name);
				tour_arena.win(ch, tad);
			}
		}
		break;

	case ARENA_CMD_LOSE:
		if (!tour_arena.is_open())
			send_to_char(ch, "Необходимо сначало открыть арену.\r\n");
		else if (!tour_arena.is_locked())
			send_to_char(ch, "Заблокируйте прием ставок.\r\n");
		else {
			if (!sscanf(argument, "%d", &num))
				send_to_char(ch, "Формат: mngarena удалить номер.участника\r\n");
			else if ((tad = tour_arena.get_slot(num)) == NULL)
				send_to_char(ch, "Участника под таким номером не существует.\r\n");
			else {
				send_to_char(ch, "Участнику (%s) присуждено поражение.\r\n", tad->name);
				tour_arena.lose(ch, tad);
			}
		}
		break;
	case ARENA_CMD_LOCK:
		if (!tour_arena.is_open())
			send_to_char(ch, "Необходимо сначало открыть арену.\r\n");
		else if (tour_arena.is_locked())
			send_to_char(ch, "Арена уже заблокирована.\r\n");
		else {
			send_to_char(ch, "Прием ставок заблокирован.\r\n");
			tour_arena.lock(ch);
		}
		break;
	case ARENA_CMD_UNLOCK:
		if (!tour_arena.is_open())
			send_to_char(ch, "Необходимо сначало открыть арену.\r\n");
		else if (!tour_arena.is_locked())
			send_to_char(ch, "Арена уже разблокирована.\r\n");
		else {
			send_to_char(ch, "Прием ставок разрешен.\r\n");
			tour_arena.unlock(ch);
		}

		break;
	}
	return;
}

/* Сообщение в канал арены */
void message_arena(char *message, CHAR_DATA * ch)
{
	// Вывод сообщения в канал арены
	DESCRIPTOR_DATA *i;

	/* now send all the strings out */
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) == CON_PLAYING &&
		    (!ch || i != ch->desc) &&
		    i->character &&
		    !PRF_FLAGGED(i->character, PRF_NOARENA) &&
		    !PLR_FLAGGED(i->character, PLR_WRITING) &&
		    !ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && GET_POS(i->character) > POS_SLEEPING) {
			if (COLOR_LEV(i->character) >= C_NRM)
				send_to_char(CCIGRN(i->character, C_NRM), i->character);
			act(message, FALSE, i->character, 0, 0, TO_CHAR | TO_SLEEP);

			if (COLOR_LEV(i->character) >= C_NRM)
				send_to_char(CCNRM(i->character, C_NRM), i->character);
		}
	}
}

Arena::Arena()
{
	opened = false;
	locked = true;
}

Arena::~Arena()
{
	clear();
}

// Очистка арены
void
 Arena::clear()
{
	// Очищаем список
	ArenaList::iterator p;

	p = arena_slots.begin();

	while (p != arena_slots.end()) {
		if ((*p)->name != NULL)
			free((*p)->name);
		delete(*p);
		p++;
	}
	arena_slots.erase(arena_slots.begin(), arena_slots.end());

	return;
}

// Отображение арены
int Arena::show(CHAR_DATA * ch)
{
	//
	ArenaList::iterator p;
	int i = 0;

	if (!opened)
		send_to_char(ch, "Арена закрыта.\r\n");
	else {
		send_to_char(ch, "Арена открыта.\r\n");
		if (locked)
			send_to_char(ch, "Прием ставок запрещен.\r\n");
		else
			send_to_char(ch, "Прием ставок разрешен.\r\n");

		if (arena_slots.size() == 0)
			send_to_char(ch, "Нет участников турнира.\r\n");
		else {
			send_to_char(ch, "###  Участник              Cтавка   Состояние\r\n");
			send_to_char(ch, "---------------------------------------------\r\n");
			p = arena_slots.begin();
			while (p != arena_slots.end()) {
				i++;
				send_to_char(ch, "%2d.  %-20s %6d   %-10s\r\n", i, (*p)->name,
					     (*p)->bet, arstate[(*p)->state]);
				p++;
			}
		}

	}
	return (TRUE);
}

// Добавление в список         
int Arena::add(CHAR_DATA * ch, ARENA_DATA * new_arr)
{
	arena_slots.push_back(new_arr);
	sprintf(buf, "Арена : Добавлен новый участник турнира - %s.", new_arr->name);
	message_arena(buf, ch);
	return (TRUE);
}

// Удаление с арены         
int Arena::del(CHAR_DATA * ch, ARENA_DATA * rem_arr)
{
	if (rem_arr == NULL)
		return (FALSE);

	sprintf(buf, "Арена : Удален участник турнира - %s.", rem_arr->name);
	message_arena(buf, ch);

	arena_slots.remove(rem_arr);

	free(rem_arr->name);
	free(rem_arr);

	return (TRUE);
}

int Arena::win(CHAR_DATA * ch, ARENA_DATA * arenaman)
{
	// Персонаж выйграл ... ??? 
	sprintf(buf, "Арена : Участник (%s) выйграл.", arenaman->name);
	message_arena(buf, ch);


	return (TRUE);
}

int Arena::lose(CHAR_DATA * ch, ARENA_DATA * arenaman)
{
	// Персонаж проиграл ...
	// Ставки на него распределяются между не-проигравшими игроками.
	sprintf(buf, "Арена : Участник (%s) проиграл.", arenaman->name);
	message_arena(buf, ch);

	// Пробегаемся по всем игрокам в игре и лд.


	return (TRUE);
}

int Arena::open(CHAR_DATA * ch)
{
	opened = true;
	locked = true;

	// Cообщаем о открытии арены.
	return (TRUE);
}

ARENA_DATA *Arena::get_slot(int slot_num)
{
	ArenaList::iterator p;

	int i = 0;

	if (slot_num <= 0)
		return (NULL);

	p = arena_slots.begin();
	while (p != arena_slots.end()) {
		if (slot_num - 1 == i) {
			return (*p);
		}
		i++;
		p++;
	}
	return (NULL);
}

int Arena::lock(CHAR_DATA * ch)
{
	locked = true;
	// Сообщаем о блокировании арены.
	sprintf(buf, "Арена : Прием ставок закончен.");
	message_arena(buf, ch);

	return (TRUE);
}

int Arena::unlock(CHAR_DATA * ch)
{
	locked = false;
	// Сообщаем о блокировании арены.
	sprintf(buf, "Арена : Начат прием ставок.");
	message_arena(buf, ch);

	return (TRUE);
}

int Arena::close(CHAR_DATA * ch)
{
	opened = false;
	locked = true;

	// Сообщаем о закрытие арены.
	return (TRUE);
}

int Arena::is_open()
{
	if (opened)
		return (TRUE);
	else
		return (FALSE);
}

int Arena::is_locked()
{
	if (locked)
		return (TRUE);
	else
		return (FALSE);
}

int Arena::make_bet(CHAR_DATA * ch, int slot, int bet)
{
	// Делаем ставку на указанного игрока... 
	ARENA_DATA *tad;
	// Удалять и создавать после 1-ой ставки участников низя.

	if ((tad = tour_arena.get_slot(slot)) == NULL)
		return (FALSE);

	// Снимаем деньги с банки.
	GET_BANK_GOLD(ch) -= bet;

	if ((GET_BANK_GOLD(ch) -= bet) < 0) {
		GET_GOLD(ch) += GET_BANK_GOLD(ch);
		GET_BANK_GOLD(ch) = 0;
	}
	// Устанавливаем параметры ставки
	GET_BET(ch) = bet;
	GET_BET_SLOT(ch) = slot;

	tad->bet += bet;

	return (TRUE);
}

SPECIAL(betmaker)
{
	int slot, value;

	if (CMD_IS("ставка")) {
		// Делаем ставку на указанного участника.
		if (tour_arena.is_open() && !tour_arena.is_locked()) {
			if (sscanf(argument, "%d %d", &slot, &value) != 2) {
				send_to_char("Формат: арена ставка номер.участника величина.ставки\r\n", ch);
				return (TRUE);
			}
			if (!tour_arena.get_slot(slot)) {
				send_to_char("Участник не зарегистрирован.\r\n", ch);
				return (TRUE);
			}

			if (GET_BET(ch) || GET_BET_SLOT(ch)) {
				send_to_char(ch, "Но Вы же уже сделали ставку.\r\n");
				return (TRUE);
			}

			if (value > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
				send_to_char("У Вас нет такой суммы.\r\n", ch);
				return (TRUE);
			}

			if (tour_arena.make_bet(ch, slot, value)) {
				send_to_char(ch, "Ставка принята.\r\n");
				return (TRUE);
			} else {
				send_to_char(ch, "Ставка не принята.\r\n");
				return (TRUE);
			}
		} else {
			// Отсылаем арена закрыта 
			send_to_char(ch, "Ставки сейчас не принимаются.\r\n");
		}
		return (TRUE);
	} else if (CMD_IS("получить")) {
		// Получаем выйгрыш

		return (TRUE);
	} else
		return (FALSE);
}
