/* ************************************************************************
*   File: magic.cpp                                     Part of Bylins    *
*  Usage: low-level functions for magic; spell template code              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "skills.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "pk.h"
#include "features.hpp"
#include "magic.h"
#include "fight.h"
#include "deathtrap.hpp"
#include "random.hpp"
#include "char.hpp"
#include "poison.hpp"
#include "modify.h"
#include "room.hpp"
#include "AffectHandler.hpp"

extern int what_sky;
extern DESCRIPTOR_DATA *descriptor_list;
extern struct zone_data *zone_table;
extern struct spell_create_type spell_create[];
extern int mini_mud;
extern const char *spell_wear_off_msg[];

extern const char *cast_phrase[LAST_USED_SPELL + 1][2];
extern int interpolate(int min_value, int pulse);

byte saving_throws(int class_num, int type, int level);	// class.cpp
byte extend_saving_throws(int class_num, int type, int level);
void clearMemory(CHAR_DATA * ch);
void weight_change_object(OBJ_DATA * obj, int weight);
int attack_best(CHAR_DATA * ch, CHAR_DATA * victim);
void alterate_object(OBJ_DATA * obj, int dam, int chance);
void death_cry(CHAR_DATA * ch);
int has_boat(CHAR_DATA * ch);
int check_death_trap(CHAR_DATA * ch);
int check_charmee(CHAR_DATA * ch, CHAR_DATA * victim, int spellnum);
int slot_for_char(CHAR_DATA * ch, int slotnum);
void cast_reaction(CHAR_DATA * victim, CHAR_DATA * caster, int spellnum);

// Extern functions
CHAR_DATA *try_protect(CHAR_DATA * victim, CHAR_DATA * ch);
// local functions
int mag_materials(CHAR_DATA * ch, int item0, int item1, int item2, int extract, int verbose);
void perform_mag_groups(int level, CHAR_DATA * ch, CHAR_DATA * tch, int spellnum, int savetype);
void affect_update(void);
void battle_affect_update(CHAR_DATA * ch);
void pulse_affect_update(CHAR_DATA * ch);


CHAR_DATA * find_char_in_room(long char_id, ROOM_DATA *room)
{
	CHAR_DATA * tch, * next_ch;
	assert(room);

	for (tch = room->people ; tch ; tch = next_ch)
	{
		next_ch = tch->next_in_room;
		if (GET_ID(tch) == char_id)
			return (tch);
	}
	return NULL;
}

CHAR_DATA * random_char_in_room(ROOM_DATA *room)
{
	CHAR_DATA * c , * result = NULL;
	int count = 0, index = 0;
	// посчитаем число перцев  в комнате
	for (c = room->people; c; c = c->next_in_room)
		if (!PRF_FLAGGED(c, PRF_NOHASSLE)
				&& !GET_INVIS_LEV(c))
		{
			count++;
		}
	count = number(0, count - 1);
	// А теперь узнаем его указатель.
	for (c = room->people; c; c = c->next_in_room)
		if (!PRF_FLAGGED(c, PRF_NOHASSLE)
				&& !GET_INVIS_LEV(c))
		{
			if (index == count)
			{
				result = c;
				break;
			}

			index++;
		}
	return (result);

}

bool is_room_forbidden(ROOM_DATA * room)
{
	AFFECT_DATA* af;
	for (af = room->affected ; af ; af = af->next)
		if (af->type == SPELL_FORBIDDEN && (number(1, 100) <= af->modifier))
			return true;
	return false;
}

// * Структуры и функции для работы с заклинаниями, обкастовывающими комнаты

namespace RoomSpells {

// список всех обкстованных комнат //
std::list<ROOM_DATA*> aff_room_list;

 // Поиск первой комнаты с аффектом от spellnum и кастером с идом Id //
ROOM_DATA * find_affected_roomt(long id, int spellnum);
// Показываем комнаты под аффектами //
void ShowRooms(CHAR_DATA *ch);
// Поиск и удаление первого аффекта от спелла spellnum и кастером с идом id //
void find_and_remove_room_affect(long id, int spellnum);
 // Обработка самих аффектов т.е. их влияния на персонажей в комнате раз в 2 секунды //
void pulse_room_affect_handler(room_rnum room, CHAR_DATA * ch, AFFECT_DATA * aff);
// Сообщение при снятии аффекта //
void show_room_spell_off(int aff, room_rnum room);
// Добавление новой комнаты в список //
void AddRoom(ROOM_DATA* room);
// Применение заклинания к комнате //
int mag_room(int level, CHAR_DATA * ch , ROOM_DATA * room, int spellnum);

// =============================================================== //

// Показываем комнаты под аффектами //
void ShowRooms(CHAR_DATA *ch)
{
	buf[0] = '\0';
	AFFECT_DATA *af;
    strcpy(buf, "Список комнат под аффектами:\r\n" "-------------------\r\n");
    for (std::list<ROOM_DATA*>::iterator it = aff_room_list.begin();it != aff_room_list.end();++it)
	{
		buf1[0] = '\0';
		for (af = (*it)->affected ; af ; af = af->next)
			sprintf(buf1 + strlen(buf1),  " !%s! (%s) ", spell_info[af->type].name, get_name_by_id(af->caster_id));
        sprintf(buf + strlen(buf),  "   [%d] %s\r\n", (*it)->number, buf1);
	}
    page_string(ch->desc, buf, TRUE);
}

// =============================================================== //

// Поиск первой комнаты с аффектом от spellnum и кастером с идом Id //
ROOM_DATA * find_affected_roomt(long id, int spellnum)
{
    AFFECT_DATA *af;

    for (std::list<ROOM_DATA*>::iterator it = aff_room_list.begin();it != aff_room_list.end();++it)
    {
        for (af = (*it)->affected ; af ; af = af->next)
        {
			if ((af->type == spellnum) && (af->caster_id == id))
                return *it;
        }
    }
    return NULL;
}

// =============================================================== //

// Поиск и удаление первого аффекта от спелла spellnum и кастером с идом id //
void find_and_remove_room_affect(long id, int spellnum)
{
    AFFECT_DATA *af;

    for (std::list<ROOM_DATA*>::iterator it = aff_room_list.begin();it != aff_room_list.end();++it)
    {
        for (af = (*it)->affected ; af ; af = af->next)
        {
			if ((af->type == spellnum) && (af->caster_id == id))
			{
                if (af->type > 0 && af->type <= LAST_USED_SPELL && *spell_wear_off_msg[af->type])
							show_room_spell_off(af->type, real_room((*it)->number));
                affect_room_remove(*it, af);
                return;
			}
        }
    }
}

// =============================================================== //

// Сообщение при снятии аффекта //
void show_room_spell_off(int aff, room_rnum room)
{
	send_to_room(spell_wear_off_msg[aff], room, 0);
}

// =============================================================== //

// Добавление новой комнаты в список //
void AddRoom(ROOM_DATA* room)
{
	std::list<ROOM_DATA*>::const_iterator it = std::find(aff_room_list.begin(), aff_room_list.end(), room);
	if (it == aff_room_list.end())
		aff_room_list.push_back(room);
}

// =============================================================== //

// Раз в 2 секунды идет вызов обработчиков аффектов//
void pulse_room_affect_handler(ROOM_DATA * room, CHAR_DATA * ch, AFFECT_DATA * aff)
{
	// Аффект в комнате.
	// Проверяем на то что нам передали бяку в параметрах.
	assert(aff);
	assert(room);

	// Тут надо понимать что если закл наложит не один аффект а несколько
	// то обработчик будет вызываться за пульс именно столько раз.
	int spellnum = aff->type;
	CHAR_DATA * tch, *tch_next;
	switch (spellnum)
	{
	case SPELL_FORBIDDEN:
		break;
	case SPELL_ROOM_LIGHT:
//			sprintf(buf2 , "Ярко светит колдовской свет. (%d)\r\n", aff->apply_time);
//			send_to_room(buf2,room, 0);
		break;

	case SPELL_POISONED_FOG:
		// Обработчик закла
		// По сути это каст яда на всех без разбора.
		//	sprintf(buf2 , "Травим всех тут. (%d : %d)\r\n", aff->apply_time, aff->duration);
		//send_to_room(buf2, room, 0);
		if (ch) // Кастер нашелся и он тут ...
		{
			for (tch = room->people; tch ; tch = tch_next)
			{
				tch_next = tch->next_in_room;

				if (!call_magic(ch, tch, NULL, NULL, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL))
				{
					aff->duration = 0;
					break;
				}
			}
		}

		break;
	case SPELL_THUNDERSTORM:
		/* Что можно учудить тут :
			1-2 - Первые два раунда чары только предупреждаются что тут будет гоооорячо.
			3 - начинает идти дождь
			4-6 - цепи молний
			7-8 - 1-но испепеление - убивает нафиг рэндомно одного чара ...
			9-10 - шаравухи
			11 - дождь кончается.
		*/
		switch (aff->duration)
		{
		case 1:
			what_sky = SKY_CLOUDY;
			if (!call_magic(ch, NULL, NULL, NULL, SPELL_CONTROL_WEATHER, GET_LEVEL(ch), CAST_SPELL))
			{
				// Если закл сфейлился то прекращаем новый год )
				aff->duration = 0;
				break;
			}
			break;

		case 2:
			what_sky = SKY_RAINING;
			if (!call_magic(ch, NULL, NULL, NULL, SPELL_CONTROL_WEATHER, GET_LEVEL(ch), CAST_SPELL))
			{
				// Если закл сфейлился то прекращаем новый год )
				aff->duration = 0;
				break;
			}
			break;
		case 3:
			// Хреначим  рэндомно молниями.
			tch = random_char_in_room(room);
			if (tch)
			{
				call_magic(ch, tch, NULL, NULL, SPELL_CHAIN_LIGHTNING, GET_LEVEL(ch), CAST_SPELL);
			}
			break;
		case 5:
			// Делаем мега молнию - которая мочит чара Ж)
			tch = random_char_in_room(room);
			if (tch)
			{
				send_to_char("Удар молнии просто испепелил вас", tch);
				act("Удар молнии призванной испепелил $n!", FALSE, tch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
				pk_agro_action(ch, tch);
				raw_kill(tch, ch);
			}
			break;
		case 6:
			// Хреначим  рэндомно молниями.
			tch = random_char_in_room(room);
			if (tch)
			{
				call_magic(ch, tch, NULL, NULL, SPELL_CHAIN_LIGHTNING, GET_LEVEL(ch), CAST_SPELL);
			}
			break;

		case 7:
			// Успокаиваем погоду.

			what_sky = SKY_CLOUDLESS;
			if (!call_magic(ch, NULL, NULL, NULL, SPELL_CONTROL_WEATHER, GET_LEVEL(ch), CAST_SPELL))
			{
				// Если закл сфейлился то прекращаем новый год )
				aff->duration = 0;
				break;
			}
			break;
		}

		break;
	default:
		log("Try handle room affect for spell without handler");
	}
}

// =============================================================== //

// Апдейт аффектов для комнат - надеюсь это мир не прикончит//
// Gorrah: Вынес в отдельный лист обкастованные комнаты - теперь думаю не прикончит //
void room_affect_update(void)
{
	AFFECT_DATA *af, *next;
	CHAR_DATA *ch;
	int spellnum;
	//std::list<ROOM_DATA*>::iterator i = aff_room_list.begin();

	for (std::list<ROOM_DATA*>::iterator it = aff_room_list.begin();it != aff_room_list.end();)
	{
		assert(*it);
		for (af = (*it)->affected ; af ; af = next)
		{
			next = af->next;
			spellnum = af->type;
			ch = NULL;

			if (IS_SET(SpINFO.routines, MAG_CASTER_INROOM) || IS_SET(SpINFO.routines, MAG_CASTER_INWORLD))
			{
				ch = find_char_in_room(af->caster_id, *it);
                // Кастер слинял ... или помер - зря.
				if (!ch)
                    af->duration = 0;
			} else if (IS_SET(SpINFO.routines, MAG_CASTER_INWORLD_DELAY))
			{
			    //Если спелл с задержкой таймера - то обнулять не надо, даже если чара нет, просто тикаем таймером как обычно
                ch = find_char_in_room(af->caster_id, *it);
			}

			// Чую долгое это будет дело ... но деваться некуда
			if ((!ch) && IS_SET(SpINFO.routines, MAG_CASTER_INWORLD))
			{
				// Ищем чара по миру
				ch = find_char(af->caster_id);
				if (!ch)
					af->duration = 0;
			} else if (IS_SET(SpINFO.routines, MAG_CASTER_INWORLD_DELAY))
			{
				ch = find_char(af->caster_id);
			}

			if (ch && IS_SET(SpINFO.routines, MAG_CASTER_INWORLD_DELAY))
			{
			//Если персонаж найден, то таймер тикать не должен - восстанавливаем время.
                switch (spellnum)
                {
                    case SPELL_RUNE_LABEL:
                        af->duration = TIME_SPELL_RUNE_LABEL;
                        break;
                }
			}

			if (af->duration >= 1)
				af->duration--;
			else if (af->duration == -1)
				af->duration = -1;
			else
			{
				if ((af->type > 0) && (af->type <= MAX_SPELLS))
				{
					if (!af->next || (af->next->type != af->type)
							|| (af->next->duration > 0))
					{
						if (af->type > 0 &&
								af->type <= LAST_USED_SPELL && *spell_wear_off_msg[af->type])
						{
							show_room_spell_off(af->type, real_room((*it)->number));
						}
					}
				}
				affect_room_remove(*it, af);
//sprintf(buf2 , "\r\nАффект снят с комнаты. Всего в списке осталось %d комнат.\r\n", aff_room_list.size());
//send_to_gods(buf2);
				continue;  // Чтоб не вызвался обработчик
			}

			// Учитываем что время выдается в пульсах а не в секундах  т.е. надо умножать на 2
			af->apply_time++;
			if (af->must_handled)
                pulse_room_affect_handler(*it, ch, af);
		}
        //если больше аффектов нет, удаляем комнату из списка обкастованных
        if ((*it)->affected == NULL)
            it = aff_room_list.erase(it);
        //Инкремент итератора. Здесь, чтобы можно было удалять элементы списка.
        if (it != aff_room_list.end())
            ++it;
	}
}

// =============================================================== //

// Применение заклинания к комнате //
int mag_room(int level, CHAR_DATA * ch , ROOM_DATA * room, int spellnum)
{
	// Специфика заклов на комнаты заключается в том, что
	// практически все взаимодействие с персонажами осуществляется
	// не в прямую а косвенно , через аффекты которые на ней присутствуют
	// поэтому любой каст на комнату это прежде всего вешание
	// на нее какого-то определенного аффекта.

	AFFECT_DATA af[MAX_SPELL_AFFECTS];
	bool accum_affect = FALSE, accum_duration = FALSE, success = TRUE;
	//Данный флажок говорит можно ли обновлять закл
	//или  оно должно висеть пока не спадет
	bool update_spell = FALSE;
	// Должен ли данный спелл быть только 1 в мире от этого кастера?
	bool only_one = FALSE;
	const char *to_char = NULL;
	const char *to_room = NULL;
	int i = 0, lag = 0;
	// Sanity check
	if (room == NULL || IN_ROOM(ch) == NOWHERE || ch == NULL)
		return 0;

	// Нулим все аффекты в массиве.
	for (i = 0; i < MAX_SPELL_AFFECTS; i++)
	{
		af[i].type = spellnum;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].location = APPLY_NONE;
		af[i].caster_id = 0;
		af[i].must_handled = false;
		af[i].apply_time = 0;
	}
	// Проверки ПК - флагов осуществлятся тогда когда аффекты реально работают

	switch (spellnum)
	{
	case SPELL_FORBIDDEN:
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_NONE;
		af[0].duration = (1 + (GET_LEVEL(ch) + 14) / 15)*30;
		af[0].caster_id = GET_ID(ch);
		af[0].bitvector = AFF_ROOM_FORBIDDEN;
		// ROOM_AFF_FLAGS(room, AFF_ROOM_FORBIDDEN); смысл этой строки?
		af[0].must_handled = false;
		accum_duration = FALSE;
		update_spell = TRUE;
		af[0].modifier = MIN(100, GET_REAL_INT(ch) + MAX((GET_REAL_INT(ch) - 30) * 4, 0));
		if (af[0].modifier > 99)
		{
			to_char = "Вы запечатали магией все входы.";
			to_room = "$n запечатал$g магией все входы.";
		}
		else if (af[0].modifier > 79)
		{
			to_char = "Вы почти полностью запечатали магией все входы.";
			to_room = "$n почти полностью запечатал$g магией все входы.";
		}
		else
		{
			to_char = "Вы очень плохо запечатали магией все входы.";
			to_room = "$n очень плохо запечатал$g магией все входы.";
		}
		break;
	case SPELL_ROOM_LIGHT:
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_NONE;
		af[0].modifier = 0;
		// Расчет взят  только ориентировочный
		af[0].duration = pc_duration(ch, 0, GET_LEVEL(ch) + 5, 6, 0, 0);
		af[0].caster_id = GET_ID(ch);
		af[0].bitvector = AFF_ROOM_LIGHT;
		// Сохраняем ID кастера т.к. возможно что в живых его к моменту
		// срабатывания аффекта уже не будет, а ПК-флаги на него вешать
		// придется
		af[0].must_handled = false;
		accum_duration = TRUE;
		update_spell = TRUE;
		to_char = "Вы облили комнату бензином и бросили окурок.";
		to_room = "Пространство вокруг начало светиться.";
		break;

	case SPELL_POISONED_FOG:
		// Идея закла - комната заражается и охватывается туманом
		// не знаю там какие плюшки с туманом. Но яд травит Ж)
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_POISON; // Изменяет уровень зараженности территории
		af[0].modifier = 50;
		// Расчет взят  только ориентировочный
		af[0].duration = pc_duration(ch, 0, GET_LEVEL(ch) + 5, 6, 0, 0);
		af[0].bitvector = AFF_ROOM_FOG; //Добаляет бит туман
		af[0].caster_id = GET_ID(ch);
		af[0].must_handled = true;
		// Не имеет смысла разделять на разные аффекты
		//если описание будет одно и время работы одно
		update_spell = FALSE;
		to_room = "$n испортил$g воздух и плюнул$g в суп.";
		break;

	case SPELL_THUNDERSTORM:
		// Иллюстрация закла со стадиями
		af[0].type = spellnum;
		af[0].duration = 10; //закл длится 20 секунд
		af[0].must_handled = true; //требует вызов обработчика
		af[0].caster_id = GET_ID(ch);
		update_spell = FALSE;
		to_room = "Вы услышали раскат далекой грозы.";
		break;

	case SPELL_RUNE_LABEL:
		af[0].type = spellnum;
		af[0].location = APPLY_ROOM_NONE;
		af[0].modifier = 0;
		af[0].duration = TIME_SPELL_RUNE_LABEL;
		af[0].caster_id = GET_ID(ch);
		af[0].bitvector = AFF_ROOM_RUNE_LABEL;
		af[0].must_handled = false;
		accum_duration = TRUE;
		update_spell = FALSE; //ибо нефик
		only_one = TRUE;
		to_char = "Вы начертали свое имя рунами на земле и произнесли заклинание.";
		to_room = "$n начертил$g на земле несколько рун и произнес$q заклинание.";
		lag = 2; //Чет много тут всяких циклов-искалок. Повесим-ка лаг на чара. И вобще может пригодиться.
		break;
	}

	// Проверяем а не висит ли уже аффектов от аналогичных заклов на комнате.
	if ((world[IN_ROOM(ch)] == room) && room_affected_by_spell(room, spellnum) && success && (!update_spell))
	{
		send_to_char(NOEFFECT, ch);
		success = FALSE;
	} else if (only_one) //Вообще по хорошему счетчик надо, типа не более N экземпляров, но это потом
	{
	    find_and_remove_room_affect(GET_ID(ch), SPELL_RUNE_LABEL);
	}

	// Перебираем заклы чтобы понять не производиться ли рефрешь закла
	for (i = 0; success && i < MAX_SPELL_AFFECTS; i++)
	{
		af[i].type = spellnum;
		if (af[i].bitvector || af[i].location != APPLY_ROOM_NONE
				|| af[i].must_handled)
		{
			af[i].duration = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_EFFECT, af[i].duration);
			if (update_spell)
				affect_room_join_fspell(room, af + i);
			else
				affect_room_join(room, af + i, accum_duration, FALSE, accum_affect, FALSE);
			//Вставляем указатель на комнату в список обкастованных, с проверкой на наличие
			//Здесь - потому что все равно надо проверять, может это не первый спелл такого типа на руме
            AddRoom(room);
		}
	}

	if (success)
	{
		if (to_room != NULL)
			act(to_room, TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		if (to_char != NULL)
			act(to_char, TRUE, ch, 0, 0, TO_CHAR);
		return 1;
	}
    if (!WAITLESS(ch))
            WAIT_STATE(ch, lag * PULSE_VIOLENCE);

	return 0;

}

// ===============================================================

} // namespace RoomSpells

// * Saving throws are now in class.cpp as of bpl13.

/*
 * Negative apply_saving_throw[] values make saving throws better!
 * Then, so do negative modifiers.  Though people may be used to
 * the reverse of that. It's due to the code modifying the target
 * saving throw instead of the random number of the character as
 * in some other systems.
 */

int calc_anti_savings(CHAR_DATA * ch)
{
	int modi = 0;

	if (WAITLESS(ch))
		modi = 150;
	else if (GET_GOD_FLAG(ch, GF_GODSLIKE))
		modi = 50;
	else if (GET_GOD_FLAG(ch, GF_GODSCURSE))
		modi = -50;
	else
		modi = GET_CAST_SUCCESS(ch);
	modi += MAX(0, MIN(20, (int)((GET_REAL_WIS(ch) - 23) * 3 / 2)));
//  log("[EXT_APPLY] Name==%s modi==%d",GET_NAME(ch), modi);
	return modi;
}


int general_savingthrow(CHAR_DATA *killer, CHAR_DATA *victim, int type, int ext_apply)
{
	if (- GET_SAVE(victim, type) / 10 > number(1, 100))
	{
		return 1;
	}

	// NPCs use warrior tables according to some book
	int save;
	int class_sav = GET_CLASS(victim);

	if (IS_NPC(victim))
	{
		class_sav = CLASS_MOB;	// неизвестный класс моба
	}
	else
	{
		if (class_sav < 0 || class_sav >= NUM_CLASSES)
			class_sav = CLASS_WARRIOR;	// неизвестный класс игрока
	}

	// Базовые спасброски профессии/уровня
	save = extend_saving_throws(class_sav, type, GET_LEVEL(victim));

	switch (type)
	{
	case SAVING_REFLEX:
		if ((save > 0) && can_use_feat(victim, DODGER_FEAT))
			save >>= 1;
		save -= dex_bonus(GET_REAL_DEX(victim));
		break;
	case SAVING_STABILITY:
		save += -GET_REAL_CON(victim);
		break;
	case SAVING_WILL:
		save += -GET_REAL_WIS(victim);
		break;
	case SAVING_CRITICAL:
		save += -GET_REAL_CON(victim);
		break;
	}

	// Ослабление магических атак
	if (type != SAVING_REFLEX)
	{
		if ((save > 0) &&
				(AFF_FLAGGED(victim, AFF_AIRAURA) || AFF_FLAGGED(victim, AFF_FIREAURA) || AFF_FLAGGED(victim, AFF_ICEAURA)))
			save >>= 1;
	}
	// Учет осторожного стиля
	if (PRF_FLAGGED(victim, PRF_AWAKE))
	{
		if (can_use_feat(victim, IMPREGNABLE_FEAT))
			save -= MAX(0, victim->get_skill(SKILL_AWAKE) - 80)  /  2;
		save -= calculate_awake_mod(killer, victim);
	}

	save += GET_SAVE(victim, type);	// одежда
	save += ext_apply;	// внешний модификатор

	if (IS_GOD(victim))
		save = -150;
	else if (GET_GOD_FLAG(victim, GF_GODSLIKE))
		save -= 50;
	else if (GET_GOD_FLAG(victim, GF_GODSCURSE))
		save += 50;
//  if (!IS_NPC(victim))
//     log("SAVING: Name==%s type==%d save==%d ext_apply==%d",GET_NAME(victim),type,save, ext_apply);
	// Throwing a 0 is always a failure.
	if (MAX(5, save) <= number(1, 100))
		return (TRUE);

	// Oops, failed. Sorry.
	return (FALSE);
}


int multi_cast_say(CHAR_DATA * ch)
{
	if (!IS_NPC(ch))
        return 1;
	switch (GET_RACE(ch))
	{
	case NPC_RACE_EVIL_SPIRIT:
	case NPC_RACE_GHOST:
	case NPC_RACE_HUMAN:
	case NPC_RACE_ZOMBIE:
	case NPC_RACE_SPIRIT:
		return 1;
	}
	return 0;
}


// affect_update: called from comm.cpp (causes spells to wear off)

void show_spell_off(int aff, CHAR_DATA * ch)
{
	if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_WRITING))
		return;

	act(spell_wear_off_msg[aff], FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
	send_to_char("\r\n", ch);

}

void mobile_affect_update(void)
{
	AFFECT_DATA *af, *next;
	struct timed_type *timed, *timed_next;
	CHAR_DATA *i, *i_next;
	int was_charmed = 0, charmed_msg = FALSE;

	for (i = character_list; i; i = i_next)
	{
		i_next = i->next;
		charmed_msg = FALSE;
		was_charmed = FALSE;

		bool was_purged = false;

		for (af = i->affected; IS_NPC(i) && af; af = next)
		{
			next = af->next;
			if (af->duration >= 1)
			{
				if (IS_SET(af->battleflag, AF_SAME_TIME) && (!i->get_fighting() || af->location == APPLY_POISON))
				{
					// здесь плеера могут спуржить
					if (same_time_update(i, af) == -1)
					{
						was_purged = true;
						break;
					}
				}

				af->duration--;
				if (af->type == SPELL_CHARM && !charmed_msg && af->duration <= 1)
				{
					act("$n начал$g растерянно оглядываться по сторонам.", FALSE, i, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
					charmed_msg = TRUE;
				}
			}
			else if (af->duration == -1)
				af->duration = -1;	// GODS - unlimited
			else
			{
				if ((af->type > 0) && (af->type <= MAX_SPELLS))
				{
					if (!af->next || (af->next->type != af->type)
							|| (af->next->duration > 0))
					{
						if (af->type > 0 &&
								af->type <= LAST_USED_SPELL && *spell_wear_off_msg[af->type])
						{
							show_spell_off(af->type, i);
							if (af->type == SPELL_CHARM || af->bitvector == AFF_CHARM)
								was_charmed = TRUE;
						}
					}
				}
				affect_remove(i, af);
			}
		}

		if (!was_purged)
		{
			//log("[MOBILE_AFFECT_UPDATE->AFFECT_TOTAL] (%s) Start",GET_NAME(i));
			affect_total(i);
			//log("[MOBILE_AFFECT_UPDATE->AFFECT_TOTAL] Stop");
			for (timed = i->timed; timed; timed = timed_next)
			{
				timed_next = timed->next;
				if (timed->time >= 1)
					timed->time--;
				else
					timed_from_char(i, timed);
			}
			for (timed = i->timed_feat; timed; timed = timed_next)
			{
				timed_next = timed->next;
				if (timed->time >= 1)
					timed->time--;
				else
					timed_feat_from_char(i, timed);
			}

			if (DeathTrap::check_death_trap(i))
				continue;
			if (was_charmed)
			{
				stop_follower(i, SF_CHARMLOST);
			}
		}
	}
}

void player_affect_update(void)
{
	AFFECT_DATA *af, *next;
	CHAR_DATA *i, *i_next;

	for (i = character_list; i; i = i_next)
	{
		// на всякий случай проверка на пурж, в целом пурж чармисов внутри
		// такого цикла сейчас выглядит безопасным, чармисы если и есть, то они
		// добавлялись в чар-лист в начало списка и идут до самого чара
		if (i->purged())
			break;
		i_next = i->next;

		if (IS_NPC(i))
			continue;
		if (DeathTrap::tunnel_damage(i))
			continue;

		pulse_affect_update(i);

		bool was_purged = false;

		for (af = i->affected; af; af = next)
		{
			next = af->next;
			if (af->duration >= 1)
			{
				if (IS_SET(af->battleflag, AF_SAME_TIME) && !i->get_fighting())
				{
					// здесь плеера могут спуржить
					if (same_time_update(i, af) == -1)
					{
						was_purged = true;
						break;
					}
				}
				af->duration--;
			}
			else if (af->duration != -1)
			{
				if ((af->type > 0) && (af->type <= MAX_SPELLS))
				{
					if (!af->next || (af->next->type != af->type)
							|| (af->next->duration > 0))
					{
						if (af->type > 0 &&
								af->type <= LAST_USED_SPELL && *spell_wear_off_msg[af->type])
						{
							//чтобы не выдавалось, "что теперь вы можете сражаться",
							//хотя на самом деле не можете :)
							if (!(af->type == SPELL_MAGICBATTLE &&
									AFF_FLAGGED(i, AFF_STOPFIGHT)))
								if (!(af->type == SPELL_BATTLE &&
										AFF_FLAGGED(i, AFF_MAGICSTOPFIGHT)))
									show_spell_off(af->type, i);
						}
					}
				}
				affect_remove(i, af);
			}
		}

		if (!was_purged)
		{
			(void) MemQ_slots(i);	// сколько каких слотов занято (с коррекцией)


			//log("[PLAYER_AFFECT_UPDATE->AFFECT_TOTAL] Start");
			affect_total(i);
			//log("[PLAYER_AFFECT_UPDATE->AFFECT_TOTAL] Stop");
		}
	}
}


// This file update battle affects only
void battle_affect_update(CHAR_DATA * ch)
{
	AFFECT_DATA *af, *next;


	for (af = ch->affected; af; af = next)
	{
		next = af->next;
		if (!IS_SET(af->battleflag, AF_BATTLEDEC) && !IS_SET(af->battleflag, AF_SAME_TIME))
			continue;
		if (IS_NPC(ch) && af->location == APPLY_POISON)
			continue;
		if (af->duration >= 1)
		{
			if (IS_SET(af->battleflag, AF_SAME_TIME))
			{
				// здесь плеера могут спуржить
				if (same_time_update(ch, af) == -1)
					return;
				af->duration--;
			}
			else
			{
				if (IS_NPC(ch))
					af->duration--;
				else
					af->duration -= MIN(af->duration, SECS_PER_MUD_HOUR / SECS_PER_PLAYER_AFFECT);
			}
		}
		else if (af->duration != -1)
		{
			if ((af->type > 0) && (af->type <= MAX_SPELLS))
			{
				if (!af->next || (af->next->type != af->type)
						|| (af->next->duration > 0))
				{
					if (af->type > 0 &&
							af->type <= LAST_USED_SPELL && *spell_wear_off_msg[af->type])
					{
						show_spell_off(af->type, ch);
					}
				}
			}
			affect_remove(ch, af);
		}
	}

	//log("[BATTLE_AFFECT_UPDATE->AFFECT_TOTAL] Start");
	affect_total(ch);
	//log("[BATTLE_AFFECT_UPDATE->AFFECT_TOTAL] Stop");
}


// This file update pulse affects only
void pulse_affect_update(CHAR_DATA * ch)
{
	AFFECT_DATA *af, *next;
	bool pulse_aff = FALSE;

	if (ch->get_fighting())
		return;


	for (af = ch->affected; af; af = next)
	{
		next = af->next;
		if (!IS_SET(af->battleflag, AF_PULSEDEC))
			continue;
		pulse_aff = TRUE;
		if (af->duration >= 1)
		{
			if (IS_NPC(ch))
				af->duration--;
			else
				af->duration -= MIN(af->duration, SECS_PER_PLAYER_AFFECT * PASSES_PER_SEC);
		}
		else if (af->duration == -1)	// No action //
			af->duration = -1;	// GODs only! unlimited //
		else
		{
			if ((af->type > 0) && (af->type <= MAX_SPELLS))
			{
				if (!af->next || (af->next->type != af->type)
						|| (af->next->duration > 0))
				{
					if (af->type > 0 &&
							af->type <= LAST_USED_SPELL && *spell_wear_off_msg[af->type])
					{
						show_spell_off(af->type, ch);
					}
				}
			}
			affect_remove(ch, af);
		}
	}
	if (pulse_aff)
		affect_total(ch);
}


/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
bool mag_item_ok(CHAR_DATA * ch, OBJ_DATA * obj, int spelltype)
{
	int num = 0;

	if (spelltype == SPELL_RUNES && GET_OBJ_TYPE(obj) != ITEM_INGRADIENT)
	{
		return false;
	}
	if (GET_OBJ_TYPE(obj) == ITEM_INGRADIENT)
	{
		if ((!IS_SET(GET_OBJ_SKILL(obj), ITEM_RUNES) && spelltype == SPELL_RUNES)
			|| (IS_SET(GET_OBJ_SKILL(obj), ITEM_RUNES) && spelltype != SPELL_RUNES))
		{
			return false;
		}
	}

	if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_USES) && GET_OBJ_VAL(obj, 2) <= 0)
		return false;

	if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_LAG))
	{
		num = 0;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LAG1s))
			num += 1;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LAG2s))
			num += 2;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LAG4s))
			num += 4;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LAG8s))
			num += 8;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LAG16s))
			num += 16;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LAG32s))
			num += 32;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LAG64s))
			num += 64;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LAG128s))
			num += 128;
		if (GET_OBJ_VAL(obj, 3) + num - 5 * GET_REMORT(ch) >= time(NULL))
			return false;
	}

	if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_LEVEL))
	{
		num = 0;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LEVEL1))
			num += 1;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LEVEL2))
			num += 2;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LEVEL4))
			num += 4;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LEVEL8))
			num += 8;
		if (IS_SET(GET_OBJ_VAL(obj, 0), MI_LEVEL16))
			num += 16;
		if (GET_LEVEL(ch) + GET_REMORT(ch) < num)
			return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////
#include <boost/format.hpp>

std::map<int /* vnum */, int /* count */> rune_list;

void add_rune_stats(CHAR_DATA *ch, int vnum, int spelltype)
{
	if (IS_NPC(ch) || SPELL_RUNES != spelltype)
	{
		return;
	}
	std::map<int, int>::iterator i = rune_list.find(vnum);
	if (rune_list.end() != i)
	{
		i->second += 1;
	}
	else
	{
		rune_list[vnum] = 1;
	}
}

void print_rune_stats(CHAR_DATA *ch)
{
	if (!PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		send_to_char(ch, "Пока в разработке.\r\n");
		return;
	}

	std::multimap<int, int> tmp_list;
	for (std::map<int, int>::const_iterator i = rune_list.begin(),
		iend = rune_list.end(); i != iend; ++i)
	{
		tmp_list.insert(std::make_pair(i->second, i->first));
	}
	std::string out(
		"Rune stats:\r\n"
		"vnum -> count\r\n"
		"--------------\r\n");
	for (std::multimap<int, int>::const_reverse_iterator i = tmp_list.rbegin(),
		iend = tmp_list.rend(); i != iend; ++i)
	{
		out += boost::str(boost::format("%1% -> %2%\r\n") % i->second % i->first);
	}
	send_to_char(out, ch);
}

void print_rune_log()
{
	for (std::map<int, int>::const_iterator i = rune_list.begin(),
		iend = rune_list.end(); i != iend; ++i)
	{
		log("RuneUsed: %d %d", i->first, i->second);
	}
}

////////////////////////////////////////////////////////////////////////////////

void extract_item(CHAR_DATA * ch, OBJ_DATA * obj, int spelltype)
{
	int extract = FALSE;
	if (!obj)
		return;

	GET_OBJ_VAL(obj, 3) = time(NULL);

	if (IS_SET(GET_OBJ_SKILL(obj), ITEM_CHECK_USES))
	{
		GET_OBJ_VAL(obj, 2)--;
		if (GET_OBJ_VAL(obj, 2) <= 0 && IS_SET(GET_OBJ_SKILL(obj), ITEM_DECAY_EMPTY))
			extract = TRUE;
	}
	else if (spelltype != SPELL_RUNES)
		extract = TRUE;

	if (extract)
	{
		if (spelltype == SPELL_RUNES)
			act("$o рассыпал$U у вас в руках.", FALSE, ch, obj, 0, TO_CHAR);
		obj_from_char(obj);
		extract_obj(obj);
	}
}

int check_recipe_items(CHAR_DATA * ch, int spellnum, int spelltype, int extract, const CHAR_DATA * targ)
{
	OBJ_DATA *obj;
	OBJ_DATA *obj0 = NULL, *obj1 = NULL, *obj2 = NULL, *obj3 = NULL, *objo = NULL;
	int item0 = -1, item1 = -1, item2 = -1, item3 = -1;
	int create = 0, obj_num = -1, skillnum = -1, percent = 0, num = 0;
	struct spell_create_item *items;

	if (spellnum <= 0 || spellnum > MAX_SPELLS)
		return (FALSE);
	if (spelltype == SPELL_ITEMS)
	{
		items = &spell_create[spellnum].items;
	}
	else if (spelltype == SPELL_POTION)
	{
		items = &spell_create[spellnum].potion;
		skillnum = SKILL_CREATE_POTION;
		create = 1;
	}
	else if (spelltype == SPELL_WAND)
	{
		items = &spell_create[spellnum].wand;
		skillnum = SKILL_CREATE_WAND;
		create = 1;
	}
	else if (spelltype == SPELL_SCROLL)
	{
		items = &spell_create[spellnum].scroll;
		skillnum = SKILL_CREATE_SCROLL;
		create = 1;
	}
	else if (spelltype == SPELL_RUNES)
	{
		items = &spell_create[spellnum].runes;
	}
	else
		return (FALSE);

	if (((spelltype == SPELL_RUNES || spelltype == SPELL_ITEMS) &&
			(item3 = items->rnumber) +
			(item0 = items->items[0]) +
			(item1 = items->items[1]) +
			(item2 = items->items[2]) < -3)
		|| ((spelltype == SPELL_SCROLL
			|| spelltype == SPELL_WAND
			|| spelltype == SPELL_POTION)
				&& ((obj_num = items->rnumber) < 0
					|| (item0 = items->items[0]) + (item1 = items->items[1])
						+ (item2 = items->items[2]) < -2)))
	{
		return (FALSE);
	}

	const int item0_rnum = item0 >= 0 ? real_object(item0) : -1;
	const int item1_rnum = item1 >= 0 ? real_object(item1) : -1;
	const int item2_rnum = item2 >= 0 ? real_object(item2) : -1;
	const int item3_rnum = item3 >= 0 ? real_object(item3) : -1;

	for (obj = ch->carrying; obj; obj = obj->next_content)
	{
		if (item0 >= 0 && item0_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item0_rnum], 1)
			&& mag_item_ok(ch, obj, spelltype))
		{
			obj0 = obj;
			item0 = -2;
			objo = obj0;
			num++;
		}
		else if (item1 >= 0 && item1_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item1_rnum], 1)
			&& mag_item_ok(ch, obj, spelltype))
		{
			obj1 = obj;
			item1 = -2;
			objo = obj1;
			num++;
		}
		else if (item2 >= 0 && item2_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item2_rnum], 1)
			&& mag_item_ok(ch, obj, spelltype))
		{
			obj2 = obj;
			item2 = -2;
			objo = obj2;
			num++;
		}
		else if (item3 >= 0 && item3_rnum >= 0
			&& GET_OBJ_VAL(obj, 1) == GET_OBJ_VAL(obj_proto[item3_rnum], 1)
			&& mag_item_ok(ch, obj, spelltype))
		{
			obj3 = obj;
			item3 = -2;
			objo = obj3;
			num++;
		}
	}

//  log("%d %d %d %d",items->items[0],items->items[1],items->items[2],items->rnumber);
//  log("%d %d %d %d",item0,item1,item2,item3);
	if (!objo ||
			(items->items[0] >= 0 && item0 >= 0) ||
			(items->items[1] >= 0 && item1 >= 0) ||
			(items->items[2] >= 0 && item2 >= 0) || (items->rnumber >= 0 && item3 >= 0))
		return (FALSE);
//  log("OK!");
	if (extract)
	{
		if (spelltype == SPELL_RUNES)
			strcpy(buf, "Вы сложили ");
		else
			strcpy(buf, "Вы взяли ");
		if (create)
		{
			if (!(obj = read_object(obj_num, VIRTUAL)))
				return (FALSE);
			else
			{
				percent = number(1, 100);
				if (skillnum > 0 && percent > train_skill(ch, skillnum, percent, 0))
					percent = -1;
			}
		}
		if (item0 == -2)
		{
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj0->PNames[3]);
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj0, 1), spelltype);

		}
		if (item1 == -2)
		{
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj1->PNames[3]);
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj1, 1), spelltype);
		}
		if (item2 == -2)
		{
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj2->PNames[3]);
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj2, 1), spelltype);
		}
		if (item3 == -2)
		{
			strcat(buf, CCWHT(ch, C_NRM));
			strcat(buf, obj3->PNames[3]);
			strcat(buf, ", ");
			add_rune_stats(ch, GET_OBJ_VAL(obj3, 1), spelltype);
		}
		strcat(buf, CCNRM(ch, C_NRM));
		if (create)
		{
			if (percent >= 0)
			{
				strcat(buf, " и создали $o3.");
				act(buf, FALSE, ch, obj, 0, TO_CHAR);
				act("$n создал$g $o3.", FALSE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);
				obj_to_char(obj, ch);
			}
			else
			{
				strcat(buf, " и попытались создать $o3.\r\n" "Ничего не вышло.");
				act(buf, FALSE, ch, obj, 0, TO_CHAR);
				extract_obj(obj);
			}
		}
		else
		{
			if (spelltype == SPELL_ITEMS)
			{
				strcat(buf, "и создали магическую смесь.\r\n");
				act(buf, FALSE, ch, 0, 0, TO_CHAR);
				act("$n смешал$g что-то в своей ноше.\r\n"
					"Вы почувствовали резкий запах.", TRUE, ch, NULL, NULL, TO_ROOM | TO_ARENA_LISTEN);
			}
			else if (spelltype == SPELL_RUNES)
			{
				sprintf(buf + strlen(buf),
						"котор%s вспыхнул%s ярким светом.%s",
						num > 1 ? "ые" : GET_OBJ_SUF_3(objo), num > 1 ? "и" : GET_OBJ_SUF_1(objo),
						PRF_FLAGGED(ch, PRF_COMPACT) ? "" : "\r\n");
				act(buf, FALSE, ch, 0, 0, TO_CHAR);
				act("$n сложил$g руны, которые вспыхнули ярким пламенем.",
					TRUE, ch, NULL, NULL, TO_ROOM);
				sprintf(buf, "$n сложил$g руны в заклинание '%s'%s%s.", spell_name(spellnum), (targ && targ != ch ? " на " : ""), (targ && targ != ch ? GET_PAD(targ, 1) : ""));
				act(buf, TRUE, ch, NULL, NULL, TO_ARENA_LISTEN);
			}
		}
		extract_item(ch, obj0, spelltype);
		extract_item(ch, obj1, spelltype);
		extract_item(ch, obj2, spelltype);
		extract_item(ch, obj3, spelltype);
	}
	return (TRUE);
}


int check_recipe_values(CHAR_DATA * ch, int spellnum, int spelltype, int showrecipe)
{
	int item0 = -1, item1 = -1, item2 = -1, obj_num = -1;
	struct spell_create_item *items;

	if (spellnum <= 0 || spellnum > MAX_SPELLS)
		return (FALSE);
	if (spelltype == SPELL_ITEMS)
	{
		items = &spell_create[spellnum].items;
	}
	else if (spelltype == SPELL_POTION)
	{
		items = &spell_create[spellnum].potion;
	}
	else if (spelltype == SPELL_WAND)
	{
		items = &spell_create[spellnum].wand;
	}
	else if (spelltype == SPELL_SCROLL)
	{
		items = &spell_create[spellnum].scroll;
	}
	else if (spelltype == SPELL_RUNES)
	{
		items = &spell_create[spellnum].runes;
	}
	else
		return (FALSE);

	if (((obj_num = real_object(items->rnumber)) < 0 &&
			spelltype != SPELL_ITEMS && spelltype != SPELL_RUNES) ||
			((item0 = real_object(items->items[0])) +
			 (item1 = real_object(items->items[1])) + (item2 = real_object(items->items[2])) < -2))
	{
		if (showrecipe)
			send_to_char("Боги хранят в секрете этот рецепт.\n\r", ch);
		return (FALSE);
	}

	if (!showrecipe)
		return (TRUE);
	else
	{
		strcpy(buf, "Вам потребуется :\r\n");
		if (item0 >= 0)
		{
			strcat(buf, CCIRED(ch, C_NRM));
			strcat(buf, obj_proto[item0]->PNames[0]);
			strcat(buf, "\r\n");
		}
		if (item1 >= 0)
		{
			strcat(buf, CCIYEL(ch, C_NRM));
			strcat(buf, obj_proto[item1]->PNames[0]);
			strcat(buf, "\r\n");
		}
		if (item2 >= 0)
		{
			strcat(buf, CCIGRN(ch, C_NRM));
			strcat(buf, obj_proto[item2]->PNames[0]);
			strcat(buf, "\r\n");
		}
		if (obj_num >= 0 && (spelltype == SPELL_ITEMS || spelltype == SPELL_RUNES))
		{
			strcat(buf, CCIBLU(ch, C_NRM));
			strcat(buf, obj_proto[obj_num]->PNames[0]);
			strcat(buf, "\r\n");
		}

		strcat(buf, CCNRM(ch, C_NRM));
		if (spelltype == SPELL_ITEMS || spelltype == SPELL_RUNES)
		{
			strcat(buf, "для создания магии '");
			strcat(buf, spell_name(spellnum));
			strcat(buf, "'.");
		}
		else
		{
			strcat(buf, "для создания ");
			strcat(buf, obj_proto[obj_num]->PNames[1]);
		}
		act(buf, FALSE, ch, 0, 0, TO_CHAR);
	}

	return (TRUE);
}

//******************************************************************************
//******************************************************************************
//******************************************************************************

/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */

int mag_damage(int level, CHAR_DATA * ch, CHAR_DATA * victim, int spellnum, int savetype)
{
	int dam = 0, rand = 0, count = 1, modi = 0, ndice = 0, sdice = 0, adice = 0, no_savings = FALSE;
	OBJ_DATA *obj = NULL;

	if (victim == NULL || IN_ROOM(victim) == NOWHERE || ch == NULL)
		return (0);

	pk_agro_action(ch, victim);

//  log("[MAG DAMAGE] %s damage %s (%d)",GET_NAME(ch),GET_NAME(victim),spellnum);
	// Magic glass
	if (!IS_SET(SpINFO.routines, MAG_WARCRY))
	{
		if (ch != victim && spellnum < MAX_SPELLS &&
			((AFF_FLAGGED(victim, AFF_MAGICGLASS) && number(1, 100) < (GET_LEVEL(victim) / 3))))
		{
			act("Магическое зеркало $N1 отразило вашу магию!", FALSE, ch, 0, victim, TO_CHAR);
			act("Магическое зеркало $N1 отразило магию $n1!", FALSE, ch, 0, victim, TO_NOTVICT);
			act("Ваше магическое зеркало отразило поражение $n1!", FALSE, ch, 0, victim, TO_VICT);
			return (mag_damage(level, ch, ch, spellnum, savetype));
		}
	}
	else
	{
		if (ch != victim && spellnum < MAX_SPELLS && IS_GOD(victim) && (IS_NPC(ch) || GET_LEVEL(victim) > GET_LEVEL(ch)))
		{
			act("Звуковой барьер $N1 отразил ваш крик!", FALSE, ch, 0, victim, TO_CHAR);
			act("Звуковой барьер $N1 отразил крик $n1!", FALSE, ch, 0, victim, TO_NOTVICT);
			act("Ваш звуковой барьер отразил крик $n1!", FALSE, ch, 0, victim, TO_VICT);
			return (mag_damage(level, ch, ch, spellnum, savetype));
		}
	}

	if (!IS_SET(SpINFO.routines, MAG_WARCRY) && AFF_FLAGGED(victim, AFF_SHADOW_CLOAK) && spellnum < MAX_SPELLS && number(1, 100) < 21)
	{
		act("Густая тень вокруг $N1 жадно поглотила вашу магию.", FALSE, ch, 0, victim, TO_CHAR);
		act("Густая тень вокруг $N1 жадно поглотила магию $n1.", FALSE, ch, 0, victim, TO_NOTVICT);
		act("Густая тень вокруг вас поглотила магию $n1.", FALSE, ch, 0, victim, TO_VICT);
		return (0);
	}

	// позиции до начала атаки для расчета модификаторов в damage()
	// в принципе могут меняться какими-то заклами, но пока по дефолту нет
	int ch_start_pos = GET_POS(ch);
	int victim_start_pos = GET_POS(victim);

	if (ch != victim)
	{
		modi = calc_anti_savings(ch);
	}

	if (!IS_NPC(ch) && (GET_LEVEL(ch) > 10))
		modi += (GET_LEVEL(ch) - 10);
//  if (!IS_NPC(ch) && !IS_NPC(victim))
//     modi = 0;
	if (PRF_FLAGGED(ch, PRF_AWAKE))
		modi = modi - 50;

	switch (spellnum)
	{
		// ******** ДЛЯ ВСЕХ МАГОВ ********
		// магическая стрела - для всех с 1го левела 1го круга(8 слотов)
		// *** мин 15 макс 45 (360)
		// нейтрал
	case SPELL_MAGIC_MISSILE:
		modi += 300;//hotelos by postavit "no_saving = THRUE" no ono po idiotski propisano
		ndice = 2;
		sdice = 4;
		adice = 10;
		count = (level + 9) / 10;
		break;
		// ледяное прикосновение - для всех с 7го левела 3го круга(7 слотов)
		// *** мин 29.5 макс 55.5  (390)
		// нейтрал
	case SPELL_CHILL_TOUCH:
		savetype = SAVING_REFLEX;
		ndice = 15;
		sdice = 2;
		adice = level;
		break;
		// кислота - для всех с 18го левела 5го круга (6 слотов)
		// *** мин 48 макс 70 (420)
		// нейтрал
	case SPELL_ACID:
		savetype = SAVING_REFLEX;
		obj = NULL;
		if (IS_NPC(victim))
		{
			rand = number(1, 50);
			if (rand <= WEAR_BOTHS)
				obj = GET_EQ(victim, rand);
			else
				for (rand -= WEAR_BOTHS, obj = victim->carrying; rand && obj;
						rand--, obj = obj->next_content);
		}
		if (obj)
		{
			ndice = 6;
			sdice = 10;
			adice = level;
			act("Кислота покрыла $o3.", FALSE, victim, obj, 0, TO_CHAR);
			alterate_object(obj, number(level * 2, level * 4), 100);
		}
		else
		{
			ndice = 6;
			sdice = 15;
			adice = (level - 18) * 2;
		}
		break;

		// землетрясение чернокнижники 22 уровень 7 круг (4)
		// *** мин 48 макс 60 (240)
		// нейтрал
	case SPELL_EARTHQUAKE:
		savetype = SAVING_REFLEX;
		ndice = 6;
		sdice = 15;
		adice = (level - 22) * 2;
		if (GET_POS(victim) > POS_SITTING &&
				!WAITLESS(victim) && (number(1, 999)  > GET_AR(victim) * 10) &&
				(GET_MOB_HOLD(victim) || !general_savingthrow(ch, victim, SAVING_REFLEX, CALC_SUCCESS(modi, 30))))
		{
			act("$n3 повалило на землю.", FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			act("Вас повалило на землю.", FALSE, victim, 0, 0, TO_CHAR);
			GET_POS(victim) = POS_SITTING;
			update_pos(victim);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
		}
		break;

	case SPELL_SONICWAVE:
		savetype = SAVING_STABILITY;
		ndice = 6;
		sdice = 8;
		adice = (level - 25) * 3;
		if (GET_POS(victim) > POS_SITTING &&
				!WAITLESS(victim) && (number(1, 999)  > GET_AR(victim) * 10) &&
				(GET_MOB_HOLD(victim) || !general_savingthrow(ch, victim, SAVING_STABILITY, CALC_SUCCESS(modi, 60))))
		{
			act("$n3 повалило на землю.", FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			act("Вас повалило на землю.", FALSE, victim, 0, 0, TO_CHAR);
			GET_POS(victim) = POS_SITTING;
			update_pos(victim);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
		}
		break;

		// ********** ДЛЯ ФРАГЕРОВ **********
		// горящие руки - с 1го левела 1го круга (8 слотов)
		// *** мин 21 мах 30 (240)
		// ОГОНЬ
	case SPELL_BURNING_HANDS:
		savetype = SAVING_REFLEX;
		ndice = 10;
		sdice = 3;
		adice = (level + 2) / 3;
		break;

		// обжигающая хватка - с 4го левела 2го круга (8 слотов)
		// *** мин 36 макс 45 (360)
		// ОГОНЬ
	case SPELL_SHOCKING_GRASP:
		savetype = SAVING_REFLEX;
		ndice = 10;
		sdice = 6;
		adice = (level + 2) / 3;
		break;

		// молния - с 7го левела 3го круга (7 слотов)
		// *** мин 18 - макс 45 (315)
		// ВОЗДУХ
	case SPELL_LIGHTNING_BOLT:
		savetype = SAVING_REFLEX;
		ndice = 3;
		sdice = 5;
		count = (level + 5) / 6;
		break;

		// яркий блик - с 7го 3го круга (7 слотов)
		// *** мин 33 - макс 40 (280)
		// ОГОНЬ
	case SPELL_SHINEFLASH:
		ndice = 10;
		sdice = 5;
		adice = (level + 2) / 3;
		break;

		// шаровая молния - с 10го левела 4го круга (6 слотов)
		// *** мин 35 макс 55 (330)
		// ВОЗДУХ
	case SPELL_CALL_LIGHTNING:
		savetype = SAVING_REFLEX;
		ndice = 7;
		sdice = 6;
		adice = level;
		break;

		// огненная стрела - уровень 14 круг 5 (6 слотов)
		// *** мин 44 макс 60 (360)
		// ОГОНЬ
	case SPELL_COLOR_SPRAY:
		savetype = SAVING_STABILITY;
		ndice = 10;
		sdice = 5;
		adice = level;
		break;

		// ледяной ветер - уровень 14 круг 5 (6 слотов)
		// *** мин 44 макс 60 (360)
		// ВОДА
	case SPELL_CONE_OF_COLD:
		savetype = SAVING_STABILITY;
		ndice = 10;
		sdice = 5;
		adice = level;
		break;

		// Огненный шар - уровень 25 круг 7 (4 слотов)
		// *** мин 66 макс 80 (400)
		// ОГОНЬ
	case SPELL_FIREBALL:
		savetype = SAVING_REFLEX;
		ndice = 10;
		sdice = 21;
		adice = (level - 25) * 5;
		break;

		// Огненный поток - уровень 18 круг 6 (5 слотов)
		// ***  мин 38 макс 50 (250)
		// ОГОНЬ, ареа
	case SPELL_FIREBLAST:
		savetype = SAVING_STABILITY;
		ndice = 10;
		sdice = 3;
		adice = level;
		break;

		// метеоритный шторм - уровень 22 круг 7 (4 слота)
		// *** мин 66 макс 80  (240)
		// нейтрал, ареа
	case SPELL_METEORSTORM:
		savetype = SAVING_REFLEX;
		ndice = 11;
		sdice = 11;
		adice = (level - 22) * 3;
		break;

		// цепь молний - уровень 22 круг 7 (4 слота)
		// *** мин 76 макс 100 (400)
		// ВОЗДУХ, ареа
	case SPELL_CHAIN_LIGHTNING:
		savetype = SAVING_STABILITY;
		ndice = 2;
		sdice = 4;
		adice = level * 2;
		break;

		// гнев богов - уровень 26 круг 8 (2 слота)
		// *** мин 226 макс 250 (500)
		// ВОДА
	case SPELL_IMPLOSION:
		savetype = SAVING_WILL;
		ndice = 10;
		sdice = 13;
		adice = level * 6;
		break;

		// ледяной шторм - 26 левела 8й круг (2)
		// *** мин 55 макс 75 (150)
		// ВОДА, ареа
	case SPELL_ICESTORM:
		savetype = SAVING_STABILITY;
		ndice = 5;
		sdice = 10;
		adice = (level - 26) * 5;
		break;

		// суд богов - уровень 28 круг 9 (1 слот)
		// *** мин 188 макс 200 (200)
		// ВОЗДУХ, ареа
	case SPELL_ARMAGEDDON:
		savetype = SAVING_WILL;
		ndice = 10;
		sdice = 3;
		adice = level * 6;
		break;

		// ******* ХАЙЛЕВЕЛ СУПЕРДАМАДЖ МАГИЯ ******
		// каменное проклятие - круг 28 уровень 9 (1)
		// для всех
	case SPELL_STUNNING:
		if (ch == victim ||
				((number(1, 999)  > GET_AR(victim) * 10) &&
				 !general_savingthrow(ch, victim, SAVING_CRITICAL, CALC_SUCCESS(modi, GET_REAL_WIS(ch)))))
		{
			savetype = SAVING_STABILITY;
			ndice = GET_REAL_WIS(ch) / 5;
			sdice = GET_REAL_WIS(ch);
			adice = 5 + (GET_REAL_WIS(ch) - 20) / 6;
			if (number(1, 999) > 750)
			{
				act("Ваше каменное проклятье отшибло сознание у $N1.", FALSE, ch, 0, victim, TO_CHAR);
				act("Каменное проклятье $n1 отшибло сознание у $N1.", FALSE, ch, 0, victim, TO_NOTVICT);
				act("У вас отшибло сознание, вам очень плохо...", FALSE, ch, 0, victim, TO_VICT);
				GET_POS(victim) = POS_STUNNED;
				WAIT_STATE(victim, adice * PULSE_VIOLENCE);
			}
		}
		else
		{
			ndice = GET_REAL_WIS(ch) / 7;
			sdice = GET_REAL_WIS(ch);
			adice = level;
		}
		break;

		// круг пустоты - круг 28 уровень 9 (1)
		// для всех
	case SPELL_VACUUM:
		savetype = SAVING_STABILITY;
		ndice = MAX(1, (GET_REAL_WIS(ch) - 10) / 2);
		sdice = MAX(1, GET_REAL_WIS(ch) - 10);
		//	    adice = MAX(1, 2 + 30 - GET_LEVEL(ch) + (GET_REAL_WIS(ch) - 29)) / 7;
		//	    Ну явно кривота была. Отбалансил на свой вкус. В 50 мудры на 25м леве лаг на 3 на 30 лаг на 4 а не наоборот
		adice = MAX(1, GET_LEVEL(ch) + 1 + (GET_REAL_WIS(ch) - 29)) / 7;
		if (ch == victim ||
				(!general_savingthrow(ch, victim, SAVING_CRITICAL, CALC_SUCCESS(modi, GET_REAL_WIS(ch))) &&
				 (number(1, 999)  > GET_AR(victim) * 10) &&
				 number(0, 1000) <= 500))
		{
			GET_POS(victim) = POS_INCAP;
			WAIT_STATE(victim, adice * PULSE_VIOLENCE);
		}
		break;

		// ********* СПЕЦИФИЧНАЯ ДЛЯ КЛЕРИКОВ МАГИЯ **********
	case SPELL_DAMAGE_LIGHT:
		savetype = SAVING_CRITICAL;
		ndice = 4;
		sdice = 3;
		adice = (level + 2) / 3;
		break;
	case SPELL_DAMAGE_SERIOUS:
		savetype = SAVING_CRITICAL;
		ndice = 10;
		sdice = 3;
		adice = (level + 1) / 2;
		break;
	case SPELL_DAMAGE_CRITIC:
		savetype = SAVING_CRITICAL;
		ndice = 15;
		sdice = 4;
		adice = (level + 1) / 2;
		break;
	case SPELL_DISPEL_EVIL:
		ndice = 4;
		sdice = 4;
		adice = level;
		if (ch != victim && IS_EVIL(ch) && !WAITLESS(ch) && GET_HIT(ch) > 1)
		{
			send_to_char("Ваша магия обратилась против вас.", ch);
			GET_HIT(ch) = 1;
		}
		if (!IS_EVIL(victim))
		{
			if (victim != ch)
				act("Боги защитили $N3 от вашей магии.", FALSE, ch, 0, victim, TO_CHAR);
			return (0);
		};
		break;
	case SPELL_DISPEL_GOOD:
		ndice = 4;
		sdice = 4;
		adice = level;
		if (ch != victim && IS_GOOD(ch) && !WAITLESS(ch) && GET_HIT(ch) > 1)
		{
			send_to_char("Ваша магия обратилась против вас.", ch);
			GET_HIT(ch) = 1;
		}
		if (!IS_GOOD(victim))
		{
			if (victim != ch)
				act("Боги защитили $N3 от вашей магии.", FALSE, ch, 0, victim, TO_CHAR);
			return (0);
		};
		break;
	case SPELL_HARM:
		savetype = SAVING_CRITICAL;
		ndice = 7;
		sdice = level;
		adice = level * GET_REMORT(ch) / 4;
		//adice = (level + 4) / 5;
		break;

	case SPELL_FIRE_BREATH:
	case SPELL_FROST_BREATH:
	case SPELL_ACID_BREATH:
	case SPELL_LIGHTNING_BREATH:
//		savetype = SAVING_REFLEX;
	case SPELL_GAS_BREATH:
		savetype = SAVING_STABILITY;
		if (!IS_NPC(ch))
			return (0);
		ndice = ch->mob_specials.damnodice;
		sdice = ch->mob_specials.damsizedice;
		adice = GET_REAL_DR(ch) + str_bonus(GET_REAL_STR(ch), STR_TO_DAM);
		break;

	case SPELL_SACRIFICE:
		if (WAITLESS(victim))
			break;
		ndice = 8;
		sdice = 8;
		adice = level;
		break;

	case SPELL_DUSTSTORM:
		savetype = SAVING_STABILITY;
		ndice = 5;
		sdice = 6;
		adice = level;
		if (GET_POS(victim) > POS_SITTING &&
				!WAITLESS(victim) && (number(1, 999)  > GET_AR(victim) * 10) &&
				(!general_savingthrow(ch, victim, SAVING_REFLEX, CALC_SUCCESS(modi, 30))))
		{
			act("$n3 повалило на землю.", FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			act("Вас повалило на землю.", FALSE, victim, 0, 0, TO_CHAR);
			GET_POS(victim) = POS_SITTING;
			update_pos(victim);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
		}
		break;

	case SPELL_EARTHFALL:
		savetype = SAVING_REFLEX;
		ndice = 8;
		sdice = 8;
		adice = level * 2;
		break;

	case SPELL_SHOCK:
		savetype = SAVING_REFLEX;
		ndice = 6;
		sdice = level / 2;
		adice = (level + GET_REMORT(ch)) * 2;
		break;

	case SPELL_SCREAM:
		savetype = SAVING_STABILITY;
		ndice = 10;
		sdice = (level + GET_REMORT(ch)) / 5;
		adice = level + GET_REMORT(ch) * 2;
		break;


	case SPELL_HOLYSTRIKE:
		if (AFF_FLAGGED(victim, AFF_EVILESS))
		{
			dam = -1;
			no_savings = TRUE;
			// смерть или диспелл :)
			if (general_savingthrow(ch, victim, SAVING_WILL, modi))
			{
				act("Черное облако вокруг вас нейтрализовало действие тумана, растворившись в нем.",
					FALSE, victim, 0, 0, TO_CHAR);
				act("Черное облако вокруг $n1 нейтрализовало действие тумана.",
					FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
				affect_from_char(victim, SPELL_EVILESS);
			}
			else
			{
				dam = MAX(1, GET_HIT(victim) + 1);
				if (IS_NPC(victim))
					dam += 99;	// чтобы насмерть
			}
		}
		else
		{
			ndice = 10;
			sdice = 8;
			adice = level * 5;
		}
		break;

	case SPELL_WC_OF_RAGE:
		ndice = (level + 3) / 4;
		sdice = 6;
		adice = dice(GET_REMORT(ch) / 2, 8);
		break;

	case SPELL_WC_OF_THUNDER:
		ndice = GET_REMORT(ch) + (level + 2) / 3;
		sdice = 5;
		if (GET_POS(victim) > POS_SITTING &&
				!WAITLESS(victim) &&
				(GET_MOB_HOLD(victim) || !general_savingthrow(ch, victim, SAVING_STABILITY, GET_REAL_CON(ch))))
		{
			act("$n3 повалило на землю.", FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
			act("Вас повалило на землю.", FALSE, victim, 0, 0, TO_CHAR);
			GET_POS(victim) = POS_SITTING;
			update_pos(victim);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
		}
		break;

	}			// switch(spellnum)

	if (!dam && !no_savings)
	{
		double koeff = 1;
		if (IS_NPC(victim))
		{
			if (NPC_FLAGGED(victim, NPC_FIRECREATURE))
			{
				if (IS_SET(SpINFO.spell_class, STYPE_FIRE))
					koeff /= 2;
				if (IS_SET(SpINFO.spell_class, STYPE_WATER))
					koeff *= 2;
			}
			if (NPC_FLAGGED(victim, NPC_AIRCREATURE))
			{
				if (IS_SET(SpINFO.spell_class, STYPE_EARTH))
					koeff *= 2;
				if (IS_SET(SpINFO.spell_class, STYPE_AIR))
					koeff /= 2;
			}
			if (NPC_FLAGGED(victim, NPC_WATERCREATURE))
			{
				if (IS_SET(SpINFO.spell_class, STYPE_FIRE))
					koeff *= 2;
				if (IS_SET(SpINFO.spell_class, STYPE_WATER))
					koeff /= 2;
			}
			if (NPC_FLAGGED(victim, NPC_EARTHCREATURE))
			{
				if (IS_SET(SpINFO.spell_class, STYPE_EARTH))
					koeff /= 2;
				if (IS_SET(SpINFO.spell_class, STYPE_AIR))
					koeff *= 2;
			}
		}
		dam = dice(ndice, sdice) + adice;
		dam = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_EFFECT, dam);

		// колдуны в 2 раза сильнее фрагают по мобам
		if (can_use_feat(ch, POWER_MAGIC_FEAT) && IS_NPC(victim))
		{
			dam += dam;
		}

		// по +5% дамага за каждую мудрость от 23 и выше
		if (GET_REAL_WIS(ch) >= 23)
		{
			dam += dam * ((GET_REAL_WIS(ch) - 22) * 5) / 100;
		}

		if (AFF_FLAGGED(ch, AFF_DATURA_POISON))
			dam -= dam * GET_POISON(ch) / 100;

		if (!IS_SET(SpINFO.routines, MAG_WARCRY))
		{
			if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
				koeff /= 2;
		}

		if (dam > 0)
		{
			koeff *= 1000;
			dam = (int)MMAX(1.0, (dam * MMAX(300.0, MMIN(koeff, 2500.0)) / 1000.0));
		}
	}
	dam = MAX(0, calculate_resistance_coeff(victim, get_resist_type(spellnum), dam));

	if (!IS_SET(SpINFO.routines, MAG_WARCRY) && number(1, 999) <= GET_MR(victim) * 10)
		dam = 0;

	for (; count > 0 && rand >= 0; count--)
	{
		if (IN_ROOM(ch) != NOWHERE
			&& IN_ROOM(victim) != NOWHERE
			&& GET_POS(ch) > POS_STUNNED
			&& GET_POS(victim) > POS_DEAD)
		{
			// инит полей для дамага
			Damage dmg(SpellDmg(spellnum), dam, FightSystem::MAGE_DMG);
			dmg.ch_start_pos = ch_start_pos;
			dmg.victim_start_pos = victim_start_pos;
			// колдуны игнорят поглощение у мобов
			if (can_use_feat(ch, POWER_MAGIC_FEAT) && IS_NPC(victim))
			{
				dmg.flags.set(FightSystem::IGNORE_ABSORBE);
			}
			// отражение магии в кастующего
			if (ch == victim)
			{
				dmg.flags.set(FightSystem::MAGIC_REFLECT);
			}
			if (count <= 1)
			{
				dmg.flags.reset(FightSystem::NO_FLEE);
			}
			else
			{
				dmg.flags.set(FightSystem::NO_FLEE);
			}
			rand = dmg.process(ch, victim);
		}
	}
	return rand;
}

int pc_duration(CHAR_DATA * ch, int cnst, int level, int level_divisor, int min, int max)
{
	int result = 0;
	if (IS_NPC(ch))
	{
		result = cnst;
		if (level > 0 && level_divisor > 0)
			level = level / level_divisor;
		else
			level = 0;
		if (min > 0)
			level = MIN(level, min);
		if (max > 0)
			level = MAX(level, max);
		return (level + result);
	}
	result = cnst * SECS_PER_MUD_HOUR;
	if (level > 0 && level_divisor > 0)
		level = level * SECS_PER_MUD_HOUR / level_divisor;
	else
		level = 0;
	if (min > 0)
		level = MIN(level, min * SECS_PER_MUD_HOUR);
	if (max > 0)
		level = MAX(level, max * SECS_PER_MUD_HOUR);
	result = (level + result) / SECS_PER_PLAYER_AFFECT;
	return (result);
}

/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
*/

int mag_affects(int level, CHAR_DATA * ch, CHAR_DATA * victim, int spellnum, int savetype)
{
	AFFECT_DATA af[MAX_SPELL_AFFECTS];
	bool accum_affect = FALSE, accum_duration = FALSE, success = TRUE;
	bool update_spell = FALSE;
	const char *to_vict = NULL, *to_room = NULL;
	int i, modi = 0;
	int rnd = 0;
	OBJ_DATA *tobj;

	if (victim == NULL || IN_ROOM(victim) == NOWHERE || ch == NULL)
		return 0;

	// Calculate PKILL's affects
	//   1) "NPC affect PC spellflag"  for victim
	//   2) "NPC affect NPC spellflag" if victim cann't pkill FICHTING(victim)
	if (ch != victim)  	//send_to_char("Start\r\n",ch);
	{
		//send_to_char("Start\r\n",victim);
		if (IS_SET(SpINFO.routines, NPC_AFFECT_PC))  	//send_to_char("1\r\n",ch);
		{
			//send_to_char("1\r\n",victim);
			pk_agro_action(ch, victim);
		}
		else if (IS_SET(SpINFO.routines, NPC_AFFECT_NPC) && victim->get_fighting())  	//send_to_char("2\r\n",ch);
		{
			//send_to_char("2\r\n",victim);
			pk_agro_action(ch, victim->get_fighting());
		}
		//send_to_char("Stop\r\n",ch);
		//send_to_char("Stop\r\n",victim);
	}
	// Magic glass
	if (!IS_SET(SpINFO.routines, MAG_WARCRY))
	{
		if (ch != victim && SpINFO.violent
				&& ((AFF_FLAGGED(victim, AFF_MAGICGLASS)
					 && (IN_ROOM(ch) == IN_ROOM(victim)) //зеркало сработает только если оба в одной комнате
					 && number(1, 100) < (GET_LEVEL(victim) / 3)) || (IS_GOD(victim)
							 && (IS_NPC(ch) || GET_LEVEL(victim) > GET_LEVEL(ch)))))
		{
			act("Магическое зеркало $N1 отразило вашу магию!", FALSE, ch, 0, victim, TO_CHAR);
			act("Магическое зеркало $N1 отразило магию $n1!", FALSE, ch, 0, victim, TO_NOTVICT);
			act("Ваше магическое зеркало отразило поражение $n1!", FALSE, ch, 0, victim, TO_VICT);
			mag_affects(level, ch, ch, spellnum, savetype);
			return 0;
		}
	}
	else
	{
		if (ch != victim && SpINFO.violent && IS_GOD(victim) && (IS_NPC(ch) || GET_LEVEL(victim) > GET_LEVEL(ch)))
		{
			act("Звуковой барьер $N1 отразил ваш крик!", FALSE, ch, 0, victim, TO_CHAR);
			act("Звуковой барьер $N1 отразил крик $n1!", FALSE, ch, 0, victim, TO_NOTVICT);
			act("Ваш звуковой барьер отразил крик $n1!", FALSE, ch, 0, victim, TO_VICT);
			mag_affects(level, ch, ch, spellnum, savetype);
			return 0;
		}
	}

	if (!IS_SET(SpINFO.routines, MAG_WARCRY) && ch != victim && SpINFO.violent && number(1, 999) <= GET_AR(victim) * 10)
	{
		send_to_char(NOEFFECT, ch);
		return 0;
	}


	for (i = 0; i < MAX_SPELL_AFFECTS; i++)
	{
		af[i].type = spellnum;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].location = APPLY_NONE;
	}

	// decrease modi for failing, increese fo success
	if (ch != victim)
	{
		modi = calc_anti_savings(ch);
	}

	if (PRF_FLAGGED(ch, PRF_AWAKE))
		modi = modi - 50;

//  log("[MAG Affect] Modifier value for %s (caster %s) = %d(spell %d)",
//      GET_NAME(victim), GET_NAME(ch), modi, spellnum);

	switch (spellnum)
	{
	case SPELL_CHILL_TOUCH:
		savetype = SAVING_STABILITY;
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		af[0].location = APPLY_STR;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 2, level, 4, 6, 0));
		af[0].modifier = -1;
		af[0].battleflag = AF_BATTLEDEC;
		accum_duration = TRUE;
		to_room = "Боевый пыл $n1 несколько остыл.";
		to_vict = "Вы почувствовали себя слабее!";
		break;

	case SPELL_ENERGY_DRAIN:
	case SPELL_WEAKNESS:
		savetype = SAVING_WILL;
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		if (affected_by_spell(victim, SPELL_STRENGTH))
		{
			affect_from_char(victim, SPELL_STRENGTH);
			success = FALSE;
			break;
		}
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 4, level, 5, 4, 0));
		af[0].location = APPLY_STR;
		if (spellnum == SPELL_WEAKNESS)
			af[0].modifier = -1 * ((level / 6 + GET_REMORT(ch) / 2));
		else
			af[0].modifier = -2 * ((level / 6 + GET_REMORT(ch) / 2));
		if (IS_NPC(ch) && level >= (LVL_IMMORT))
			af[0].modifier += (LVL_IMMORT - level - 1);	//1 str per mob level above 30
		af[0].battleflag = AF_BATTLEDEC;
		accum_duration = TRUE;
		to_room = "$n стал$g немного слабее.";
		to_vict = "Вы почувствовали себя слабее!";
		spellnum = SPELL_WEAKNESS;
		break;
	case SPELL_STONESKIN:
		af[0].location = APPLY_ABSORBE;
		af[0].modifier = (level * 2 + 1) / 3;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "Кожа $n1 покрылась каменными пластинами.";
		to_vict = "Вы стали менее чувствительны к ударам.";
		break;

	case SPELL_FAST_REGENERATION:
		af[0].location = APPLY_HITREG;
		af[0].modifier = 50;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[1].location = APPLY_MOVEREG;
		af[1].modifier = 50;
		af[1].duration = af[0].duration;
		accum_duration = TRUE;
		to_room = "$n расцвел$g на ваших глазах.";
		to_vict = "Вас наполнила живительная сила.";
		break;

	case SPELL_AIR_SHIELD:
		if (affected_by_spell(victim, SPELL_ICE_SHIELD))
			affect_from_char(victim, SPELL_ICE_SHIELD);
		if (affected_by_spell(victim, SPELL_FIRE_SHIELD))
			affect_from_char(victim, SPELL_FIRE_SHIELD);
		af[0].bitvector = AFF_AIRSHIELD;
		af[0].battleflag = TRUE;
		if (IS_NPC(victim) || victim == ch)
			af[0].duration = pc_duration(victim, 10, 0, 0, 0, 0);
		else
			af[0].duration = pc_duration(victim, 4, 0, 0, 0, 0);
		to_room = "$n3 окутал воздушный щит.";
		to_vict = "Вас окутал воздушный щит.";
		break;

	case SPELL_FIRE_SHIELD:
		if (affected_by_spell(victim, SPELL_ICE_SHIELD))
			affect_from_char(victim, SPELL_ICE_SHIELD);
		if (affected_by_spell(victim, SPELL_AIR_SHIELD))
			affect_from_char(victim, SPELL_AIR_SHIELD);
		af[0].bitvector = AFF_FIRESHIELD;
		af[0].battleflag = TRUE;
		if (IS_NPC(victim) || victim == ch)
			af[0].duration = pc_duration(victim, 10, 0, 0, 0, 0);
		else
			af[0].duration = pc_duration(victim, 4, 0, 0, 0, 0);
		to_room = "$n3 окутал огненный щит.";
		to_vict = "Вас окутал огненный щит.";
		break;

	case SPELL_ICE_SHIELD:
		if (affected_by_spell(victim, SPELL_FIRE_SHIELD))
			affect_from_char(victim, SPELL_FIRE_SHIELD);
		if (affected_by_spell(victim, SPELL_AIR_SHIELD))
			affect_from_char(victim, SPELL_AIR_SHIELD);
		af[0].bitvector = AFF_ICESHIELD;
		af[0].battleflag = TRUE;
		if (IS_NPC(victim) || victim == ch)
			af[0].duration = pc_duration(victim, 10, 0, 0, 0, 0);
		else
			af[0].duration = pc_duration(victim, 4, 0, 0, 0, 0);
		to_room = "$n3 окутал ледяной щит.";
		to_vict = "Вас окутал ледяной щит.";
		break;

	case SPELL_AIR_AURA:
		af[0].location = APPLY_RESIST_AIR;
		af[0].modifier = level;
		af[0].bitvector = AFF_AIRAURA;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "$n3 окружила воздушная аура.";
		to_vict = "Вас окружила воздушная аура.";
		break;

	case SPELL_FIRE_AURA:
		af[0].location = APPLY_RESIST_WATER;
		af[0].modifier = level;
		af[0].bitvector = AFF_FIREAURA;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "$n3 окружила огненная аура.";
		to_vict = "Вас окружила огненная аура.";
		break;

	case SPELL_ICE_AURA:
		af[0].location = APPLY_RESIST_FIRE;
		af[0].modifier = level;
		af[0].bitvector = AFF_ICEAURA;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "$n3 окружила ледяная аура.";
		to_vict = "Вас окружила ледяная аура.";
		break;


	case SPELL_CLOUDLY:
		af[0].location = APPLY_AC;
		af[0].modifier = -20;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "Очертания $n1 расплылись и стали менее отчетливыми.";
		to_vict = "Ваше тело стало прозрачным, как туман.";
		break;

	case SPELL_GROUP_ARMOR:
	case SPELL_ARMOR:
		af[0].location = APPLY_AC;
		af[0].modifier = -20;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[1].location = APPLY_SAVING_REFLEX;
		af[1].modifier = -5;
		af[1].duration = af[0].duration;
		af[2].location = APPLY_SAVING_STABILITY;
		af[2].modifier = -5;
		af[2].duration = af[0].duration;
		accum_duration = TRUE;
		to_room = "Вокруг $n1 вспыхнул белый щит и тут же погас.";
		to_vict = "Вы почувствовали вокруг себя невидимую защиту.";
		spellnum = SPELL_ARMOR;
		break;

	case SPELL_FASCINATION:
		if (affected_by_spell(ch, SPELL_FASCINATION))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		tobj = get_obj_in_list_vnum(3000, ch->carrying);
		if (!tobj)	//не нашли один инрг, ищем другой
		{
			tobj = get_obj_in_list_vnum(3001, ch->carrying);
		}
		if (!tobj)	//не нашли ни одного ингридиента
		{
			act("Батюшки светы! А помаду-то я дома забыл$g.\r\n", FALSE, victim, 0, ch, TO_CHAR);
			//send_to_char("Батюшки светы! А помаду то я дома забыл.\r\n",ch);
			success = FALSE;
			break;
		}

		af[0].location = APPLY_CHA;
		rnd = number(1, 10);
		if (rnd < 4)
		{
			rnd = number(0, 4);
			af[0].modifier = -rnd;	//(  10 - int(GET_REMORT(victim)/2));
		}
		else
		{
			rnd = number(0, 15);
			if (rnd < 1)
				af[0].modifier = 4;	//(  10 - int(GET_REMORT(victim)/2));
			else if (rnd < 4)
				af[0].modifier = 3;
			else if (rnd < 8)
				af[0].modifier = 2;
			else if (rnd < 15)
				af[0].modifier = 1;
			else
				af[0].modifier = 0;
		}
		if (af[0].modifier > 0)
		{
			af[0].modifier = af[0].modifier / int ((GET_REMORT(ch) + 3) / 3);	//модификатор мортов
		}
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_vict =
			"Вы попытались вспомнить уроки старой цыганки, что учила вас людям головы морочить.\r\nХотя вы ее не очень то слушали.\r\n";
		to_room =
			"$n0 достал$g из маленькой сумочки какие-то вонючие порошки и отвернул$u, бормоча под нос \r\n\"..так это на ресницы надо, кажется... Эх, только бы не перепутать...\" \r\n";

		rnd = number(1, 100);
		if (rnd < 20)
		{
			//send_to_char("$o рассыпался в ваших руках от неловкого движения",ch);
			act("$o рассыпался в ваших руках от неловкого движения.\r\n", FALSE, ch, tobj, 0, TO_CHAR);
			obj_from_char(tobj);	//удаляем ингр
			extract_obj(tobj);
		}
		break;

	case SPELL_GROUP_BLESS:
	case SPELL_BLESS:
		af[0].location = APPLY_SAVING_STABILITY;
		af[0].modifier = -5;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_BLESS;
		af[1].location = APPLY_SAVING_WILL;
		af[1].modifier = -5;
		af[1].duration = af[0].duration;
		af[1].bitvector = AFF_BLESS;
		to_room = "$n осветил$u на миг неземным светом.";
		to_vict = "Боги одарили вас своей улыбкой.";
		spellnum = SPELL_BLESS;
		break;

	case SPELL_CONE_OF_COLD:
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
		}
		af[0].location = APPLY_DEX;
		af[0].modifier = -dice(int (MAX(1, ((level - 14) / 7))), 3);
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 9, 0, 0, 0, 0));
		to_vict = "Вы покрылись серебристым инеем.";
		to_room = "$n покрыл$u красивым серебристым инеем.";
		break;

	case SPELL_AWARNESS:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_AWARNESS;
		to_room = "$n начал$g внимательно осматриваться по сторонам.";
		to_vict = "Вы стали более внимательны к окружающему.";
		break;

	case SPELL_SHIELD:
		af[0].duration = pc_duration(victim, 4, 0, 0, 0, 0);
		af[0].bitvector = AFF_SHIELD;
		af[0].location = APPLY_SAVING_STABILITY;
		af[0].modifier = -10;
		af[0].battleflag = AF_BATTLEDEC;
		af[1].duration = af[0].duration;
		af[1].bitvector = AFF_SHIELD;
		af[1].location = APPLY_SAVING_WILL;
		af[1].modifier = -10;
		af[1].battleflag = AF_BATTLEDEC;
		af[2].duration = af[0].duration;
		af[2].bitvector = AFF_SHIELD;
		af[2].location = APPLY_SAVING_REFLEX;
		af[2].modifier = -10;
		af[2].battleflag = AF_BATTLEDEC;

		to_room = "$n покрыл$u сверкающим коконом.";
		to_vict = "Вас покрыл голубой кокон.";
		break;

	case SPELL_GROUP_HASTE:
	case SPELL_HASTE:
		if (affected_by_spell(victim, SPELL_SLOW))
		{
			affect_from_char(victim, SPELL_SLOW);
			success = FALSE;
			break;
		}
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_HASTE;
		to_vict = "Вы начали двигаться быстрее.";
		to_room = "$n начал$g двигаться заметно быстрее.";
		spellnum = SPELL_HASTE;
		break;

	case SPELL_SHADOW_CLOAK:
		af[0].bitvector = AFF_SHADOW_CLOAK;
		af[0].location = APPLY_SAVING_STABILITY;
		af[0].modifier = - GET_LEVEL(ch) / 3;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "$n скрыл$u в густой тени.";
		to_vict = "Густые тени окутали вас.";
		break;

	case SPELL_ENLARGE:
		if (affected_by_spell(victim, SPELL_ENLESS))
		{
			affect_from_char(victim, SPELL_ENLESS);
			success = FALSE;
			break;
		}
		af[0].location = APPLY_SIZE;
		af[0].modifier = 5 + level / 3;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "$n начал$g расти, как на дрожжах.";
		to_vict = "Вы стали крупнее.";
		break;

	case SPELL_ENLESS:
		if (affected_by_spell(victim, SPELL_ENLARGE))
		{
			affect_from_char(victim, SPELL_ENLARGE);
			success = FALSE;
			break;
		}
		af[0].location = APPLY_SIZE;
		af[0].modifier = -(5 + level / 3);
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "$n скукожил$u.";
		to_vict = "Вы стали мельче.";
		break;

	case SPELL_GROUP_MAGICGLASS:
	case SPELL_MAGICGLASS:
		af[0].bitvector = AFF_MAGICGLASS;
		af[0].duration = pc_duration(victim, 10, GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "$n3 покрыла зеркальная пелена.";
		to_vict = "Вас покрыло зеркало магии.";
		spellnum = SPELL_MAGICGLASS;
		break;

	case SPELL_CLOUD_OF_ARROWS:
		af[0].duration = pc_duration(victim, 10, GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_CLOUD_OF_ARROWS;
		af[0].location = APPLY_HITROLL;
		af[0].modifier = level / 6;
		accum_duration = TRUE;
		to_room = "$n3 окружило облако летающих огненных стрел.";
		to_vict = "Вас окружило облако летающих огненных стрел.";
		break;

	case SPELL_STONEHAND:
		af[0].bitvector = AFF_STONEHAND;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "Руки $n1 задубели.";
		to_vict = "Ваши руки задубели.";
		break;

	case SPELL_GROUP_PRISMATICAURA:
	case SPELL_PRISMATICAURA:
		if (!IS_NPC(ch) && !same_group(ch, victim))
		{
			send_to_char("Только на себя или одногруппника!\r\n", ch);
			return 0;
		}
		if (affected_by_spell(victim, SPELL_SANCTUARY))
		{
			affect_from_char(victim, SPELL_SANCTUARY);
			success = FALSE;
			break;
		}
		if (AFF_FLAGGED(victim, AFF_SANCTUARY))
		{
			success = FALSE;
			break;
		}
		af[0].bitvector = AFF_PRISMATICAURA;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		accum_duration = TRUE;
		to_room = "$n3 покрыла призматическая аура.";
		to_vict = "Вас покрыла призматическая аура.";
		spellnum = SPELL_PRISMATICAURA;
		break;

	case SPELL_MINDLESS:
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		af[0].location = APPLY_MANAREG;
		af[0].modifier = -50;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 0, GET_REAL_WIS(ch) + GET_REAL_INT(ch), 10, 0, 0));
		af[1].location = APPLY_CAST_SUCCESS;
		af[1].modifier = -50;
		af[1].duration = af[0].duration;
		af[2].location = APPLY_HITROLL;
		af[2].modifier = -5;
		af[2].duration = af[0].duration;

		to_room = "$n0 стал$g слаб$g на голову!";
		to_vict = "Ваш разум помутился!";
		break;

	case SPELL_DUSTSTORM:
	case SPELL_SHINEFLASH:
	case SPELL_MASS_BLINDNESS:
	case SPELL_POWER_BLINDNESS:
	case SPELL_BLINDNESS:
			savetype = SAVING_STABILITY;
		if (MOB_FLAGGED(victim, MOB_NOBLIND) ||
				WAITLESS(victim) ||
				((ch != victim) &&
				 !GET_GOD_FLAG(victim, GF_GODSCURSE) && general_savingthrow(ch, victim, savetype, modi)))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		switch (spellnum)
		{
		case SPELL_DUSTSTORM:
			af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 3, level, 6, 0, 0));
			break;
		case SPELL_SHINEFLASH:
			af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 2, level + 7, 8, 0, 0));
			break;
		case SPELL_MASS_BLINDNESS:
		case SPELL_BLINDNESS:
			af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 2, level, 8, 0, 0));
			break;
		case SPELL_POWER_BLINDNESS:
			af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 3, level, 6, 0, 0));
			break;
		}
		af[0].bitvector = AFF_BLIND;
		af[0].battleflag = AF_BATTLEDEC;
		to_room = "$n0 ослеп$q!";
		to_vict = "Вы ослепли!";
		spellnum = SPELL_BLINDNESS;
		break;

	case SPELL_MADNESS:
		savetype = SAVING_WILL;
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 3, 0, 0, 0, 0));
		af[0].bitvector = AFF_NOFLEE;
		to_room = "Теперь $n не сможет сбежать из боя!";
		to_vict = "Вас обуяло безумие боя!";
		break;

	case SPELL_WEB:
		savetype = SAVING_REFLEX;
		if (AFF_FLAGGED(victim, AFF_BROKEN_CHAINS)
				|| (ch != victim && general_savingthrow(ch, victim, savetype, modi)))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		af[0].location = APPLY_HITROLL;
		af[0].modifier = -2;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 3, level, 6, 0, 0));
		af[0].battleflag = AF_BATTLEDEC;
		af[0].bitvector = AFF_NOFLEE;
		af[1].location = APPLY_AC;
		af[1].modifier = 20;
		af[1].duration = af[0].duration;
		af[1].battleflag = AF_BATTLEDEC;
		af[1].bitvector = AFF_NOFLEE;
		to_room = "$n3 покрыла невидимая паутина, сковывая $s движения!";
		to_vict = "Вас покрыла невидимая паутина!";
		break;


	case SPELL_MASS_CURSE:
	case SPELL_CURSE:
		savetype = SAVING_WILL;
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		af[0].location = APPLY_INITIATIVE;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 1, level, 2, 0, 0));
		af[0].modifier = -5;
		af[0].bitvector = AFF_CURSE;
		if (level >= 20)
		{
			af[1].location = APPLY_CAST_SUCCESS;
			af[1].duration = af[0].duration;
			af[1].modifier = -1 * (level / 3 + GET_REMORT(ch));
			if (IS_NPC(ch) && level >= (LVL_IMMORT))
				af[1].modifier += (LVL_IMMORT - level - 1);	//1 cast per mob level above 30
			af[1].bitvector = AFF_CURSE;
		}
		accum_duration = TRUE;
		accum_affect = TRUE;
		to_room = "Красное сияние вспыхнуло над $n4 и тут же погасло!";
		to_vict = "Боги сурово поглядели на вас.";
		spellnum = SPELL_CURSE;
		break;

	case SPELL_MASS_SLOW:
	case SPELL_SLOW:
		savetype = SAVING_STABILITY;
		if (AFF_FLAGGED(victim, AFF_BROKEN_CHAINS)
				|| (ch != victim && general_savingthrow(ch, victim, savetype, modi)))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		if (affected_by_spell(victim, SPELL_HASTE))
		{
			affect_from_char(victim, SPELL_HASTE);
			success = FALSE;
			break;
		}

		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 9, 0, 0, 0, 0));
		af[0].bitvector = AFF_SLOW;
		to_room = "Движения $n1 заметно замедлились.";
		to_vict = "Ваши движения заметно замедлились.";
		spellnum = SPELL_SLOW;
		break;

	case SPELL_DETECT_ALIGN:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_DETECT_ALIGN;
		accum_duration = TRUE;
		to_vict = "Ваши глаза приобрели зеленый оттенок.";
		to_room = "Глаза $n1 приобрели зеленый оттенок.";
		break;

	case SPELL_DETECT_INVIS:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_DETECT_INVIS;
		accum_duration = TRUE;
		to_vict = "Ваши глаза приобрели золотистый оттенок.";
		to_room = "Глаза $n1 приобрели золотистый оттенок.";
		break;

	case SPELL_DETECT_MAGIC:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_DETECT_MAGIC;
		accum_duration = TRUE;
		to_vict = "Ваши глаза приобрели желтый оттенок.";
		to_room = "Глаза $n1 приобрели желтый оттенок.";
		break;

	case SPELL_INFRAVISION:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_INFRAVISION;
		accum_duration = TRUE;
		to_vict = "Ваши глаза приобрели красный оттенок.";
		to_room = "Глаза $n1 приобрели красный оттенок.";
		break;

	case SPELL_DETECT_POISON:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_DETECT_POISON;
		accum_duration = TRUE;
		to_vict = "Ваши глаза приобрели карий оттенок.";
		to_room = "Глаза $n1 приобрели карий оттенок.";
		break;

	case SPELL_GROUP_INVISIBLE:
	case SPELL_INVISIBLE:
		if (!victim)
			victim = ch;
		if (affected_by_spell(victim, SPELL_GLITTERDUST))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].modifier = -40;
		af[0].location = APPLY_AC;
		af[0].bitvector = AFF_INVISIBLE;
		accum_duration = TRUE;
		to_vict = "Вы стали невидимы для окружающих.";
		to_room = "$n медленно растворил$u в пустоте.";
		spellnum = SPELL_INVISIBLE;
		break;

	case SPELL_PLAQUE:
		savetype = SAVING_STABILITY;
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		af[0].location = APPLY_HITREG;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 0, level, 2, 0, 0));
		af[0].modifier = -95;
		af[1].location = APPLY_MANAREG;
		af[1].duration = af[0].duration;
		af[1].modifier = -95;
		af[2].location = APPLY_MOVEREG;
		af[2].duration = af[0].duration;
		af[2].modifier = -95;
		to_vict = "Вас скрутило в жестокой лихорадке.";
		to_room = "$n3 скрутило в жестокой лихорадке.";
		break;


	case SPELL_POISON:
		savetype = SAVING_CRITICAL;
		if (ch != victim && (AFF_FLAGGED(victim, AFF_SHIELD) ||
							 general_savingthrow(ch, victim, savetype, modi - GET_REAL_CON(victim) / 2)))
		{
			if (IN_ROOM(ch) == IN_ROOM(victim)) // Добавлено чтобы яд нанесенный SPELL_POISONED_FOG не спамил чару постоянно
				send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		af[0].location = APPLY_STR;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
				pc_duration(victim, 0, level, 1, 0, 0));
		af[0].modifier = -2;
		af[0].bitvector = AFF_POISON;
		af[1].battleflag = AF_SAME_TIME;

		af[1].location = APPLY_POISON;
		af[1].duration = af[0].duration;
		af[1].modifier = level;
		af[1].bitvector = AFF_POISON;
		af[1].battleflag = AF_SAME_TIME;

		to_vict = "Вы почувствовали себя отравленным.";
		to_room = "$n позеленел$g от действия яда.";
		break;

	case SPELL_PROT_FROM_EVIL:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_PROTECT_EVIL;
		accum_duration = TRUE;
		to_vict = "Вы подавили в себе страх к тьме.";
		break;

	case SPELL_GROUP_SANCTUARY:
	case SPELL_SANCTUARY:
		if (!IS_NPC(ch) && !same_group(ch, victim))
		{
			send_to_char("Только на себя или одногруппника!\r\n", ch);
			return 0;
		}
		if (affected_by_spell(victim, SPELL_PRISMATICAURA))
		{
			affect_from_char(victim, SPELL_PRISMATICAURA);
			success = FALSE;
			break;
		}
		if (AFF_FLAGGED(victim, AFF_PRISMATICAURA))
		{
			success = FALSE;
			break;
		}

		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_SANCTUARY;
		to_vict = "Белая аура мгновенно окружила вас.";
		to_room = "Белая аура покрыла $n3 с головы до пят.";
		spellnum = SPELL_SANCTUARY;
		break;

	case SPELL_SLEEP:
		savetype = SAVING_WILL;
		if (AFF_FLAGGED(victim, AFF_HOLD) || MOB_FLAGGED(victim, MOB_NOSLEEP)
				|| (ch != victim && general_savingthrow(ch, victim, SAVING_WILL, modi)))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		};

		if (victim->get_fighting())
			stop_fighting(victim, FALSE);
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 1, level, 6, 1, 6));
		af[0].bitvector = AFF_SLEEP;
		af[0].battleflag = AF_BATTLEDEC;
		if (GET_POS(victim) > POS_SLEEPING && success)
		{
			// add by Pereplut
			if (on_horse(victim))
			{
				sprintf(buf, "%s свалил%s со своего скакуна.", GET_PAD(victim, 0),
						GET_CH_SUF_2(victim));
				act(buf, FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
				REMOVE_BIT(AFF_FLAGS(victim, AFF_HORSE), AFF_HORSE);
			}

			send_to_char("Вы слишком устали... Спать... Спа...\r\n", victim);
			act("$n прилег$q подремать.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

			GET_POS(victim) = POS_SLEEPING;
		}
		break;

	case SPELL_GROUP_STRENGTH:
	case SPELL_STRENGTH:
		if (affected_by_spell(victim, SPELL_WEAKNESS))
		{
			affect_from_char(victim, SPELL_WEAKNESS);
			success = FALSE;
			break;
		}
		af[0].location = APPLY_STR;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		if (ch == victim)
			af[0].modifier = (level + 9) / 10;
		else
			af[0].modifier = (level + 14) / 15;
		accum_duration = TRUE;
		accum_affect = TRUE;
		to_vict = "Вы почувствовали себя сильнее.";
		to_room = "Мышцы $n1 налились силой.";
		spellnum = SPELL_STRENGTH;
		break;

	case SPELL_PATRONAGE:
		af[0].location = APPLY_HIT;
		af[0].duration = pc_duration(victim, 3, level, 10, 0, 0);
		af[0].modifier = GET_LEVEL(ch) * 2 + GET_REMORT(ch);
		if (GET_ALIGNMENT(victim) >= 0)
		{
			to_vict = "Исходящий с небес свет на мгновение озарил вас.";
			to_room = "Исходящий с небес свет на мгновение озарил $n3.";
		}
		else
		{
			to_vict = "Вас окутало клубящееся облако Тьмы.";
			to_room = "Клубящееся темное облако на мгновение окутало $n3.";
		}
		break;

	case SPELL_SENSE_LIFE:
		to_vict = "Вы способны разглядеть даже микроба.";
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_SENSE_LIFE;
		accum_duration = TRUE;
		break;

	case SPELL_WATERWALK:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_WATERWALK;
		accum_duration = TRUE;
		to_vict = "На рыбалку вы можете отправляться без лодки.";
		break;

	case SPELL_WATERBREATH:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_WATERBREATH;
		accum_duration = TRUE;
		to_vict = "У вас выросли жабры.";
		to_room = "У $n1 выросли жабры.";
		break;

	case SPELL_HOLYSTRIKE:
		if (AFF_FLAGGED(victim, AFF_EVILESS))
		{
			// все решится в дамадже части спелла
			success = FALSE;
			break;
		}
		// тут break не нужен
	case SPELL_MASS_HOLD:
	case SPELL_POWER_HOLD:
	case SPELL_HOLD:
		if (AFF_FLAGGED(victim, AFF_SLEEP)
				|| MOB_FLAGGED(victim, MOB_NOHOLD) || AFF_FLAGGED(victim, AFF_BROKEN_CHAINS)
				|| (ch != victim && general_savingthrow(ch, victim, SAVING_WILL, modi)))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 spellnum == SPELL_POWER_HOLD ? pc_duration(victim, 2, level + 7, 8, 2, 5)
						 : pc_duration(victim, 1, level + 9, 10, 1, 3));

		af[0].bitvector = AFF_HOLD;
		af[0].battleflag = AF_BATTLEDEC;
		to_room = "$n0 замер$q на месте!";
		to_vict = "Вы замерли на месте, не в силах пошевельнуться.";
		spellnum = SPELL_HOLD;
		break;

	case SPELL_WC_OF_RAGE:
	case SPELL_SONICWAVE:
	case SPELL_MASS_DEAFNESS:
	case SPELL_POWER_DEAFNESS:
	case SPELL_DEAFNESS:
		switch (spellnum)
		{
		case SPELL_WC_OF_RAGE:
			savetype = SAVING_WILL;
			modi = GET_REAL_CON(ch);
			break;
		case SPELL_SONICWAVE:
		case SPELL_MASS_DEAFNESS:
		case SPELL_POWER_DEAFNESS:
		case SPELL_DEAFNESS:
			savetype = SAVING_STABILITY;
			break;
		}
		if (  //MOB_FLAGGED(victim, MOB_NODEAFNESS) ||
			(ch != victim && general_savingthrow(ch, victim, savetype, modi)))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		switch (spellnum)
		{
		case SPELL_WC_OF_RAGE:
		case SPELL_POWER_DEAFNESS:
		case SPELL_SONICWAVE:
			af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 2, level + 3, 4, 6, 0));
			break;
		case SPELL_MASS_DEAFNESS:
		case SPELL_DEAFNESS:
			af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 2, level + 7, 8, 3, 0));
			break;
		}
		af[0].bitvector = AFF_DEAFNESS;
		af[0].battleflag = AF_BATTLEDEC;
		to_room = "$n0 оглох$q!";
		to_vict = "Вы оглохли.";
		spellnum = SPELL_DEAFNESS;
		break;

	case SPELL_MASS_SIELENCE:
	case SPELL_POWER_SIELENCE:
	case SPELL_SIELENCE:
		savetype = SAVING_WILL;
		if (MOB_FLAGGED(victim, MOB_NOSIELENCE) ||
				(ch != victim && general_savingthrow(ch, victim, savetype, modi)))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 spellnum == SPELL_POWER_SIELENCE ? pc_duration(victim, 2, level + 3, 4, 6, 0)
						 : pc_duration(victim, 2, level + 7, 8, 3, 0));
		af[0].bitvector = AFF_SIELENCE;
		af[0].battleflag = AF_BATTLEDEC;
		to_room = "$n0 прикусил$g язык!";
		to_vict = "Вы не в состоянии вымолвить ни слова.";
		spellnum = SPELL_SIELENCE;
		break;

	case SPELL_GROUP_FLY:
	case SPELL_FLY:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_FLY;
		to_room = "$n0 медленно поднял$u в воздух.";
		to_vict = "Вы медленно поднялись в воздух.";
		spellnum = SPELL_FLY;
		break;

	case SPELL_BROKEN_CHAINS:
		af[0].duration = pc_duration(victim, 10, GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_BROKEN_CHAINS;
		af[0].battleflag = AF_BATTLEDEC;
		to_room = "Ярко-синий ореол вспыхнул вокруг $n1 и тут же угас.";
		to_vict = "Волна ярко-синего света омыла вас с головы до ног.";
		break;

	case SPELL_BLINK:
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_BLINK;
		to_room = "$n начал$g мигать.";
		to_vict = "Вы начали мигать.";
		break;

	case SPELL_MAGICSHIELD:
		af[0].location = APPLY_AC;
		af[0].modifier = - GET_LEVEL(ch) * 10 / 6;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[1].location = APPLY_SAVING_REFLEX;
		af[1].modifier = - GET_LEVEL(ch) / 5;
		af[1].duration = af[0].duration;
		af[2].location = APPLY_SAVING_STABILITY;
		af[2].modifier = - GET_LEVEL(ch) / 5;
		af[2].duration = af[0].duration;
		accum_duration = TRUE;
		to_room = "Сверкающий щит вспыхнул вокруг $n1 и угас.";
		to_vict = "Сверкающий щит вспыхнул вокруг вас и угас.";
		break;

	case SPELL_NOFLEE: // "приковать противника"
		savetype = SAVING_WILL;
		if (AFF_FLAGGED(victim, AFF_BROKEN_CHAINS)
				|| (ch != victim && general_savingthrow(ch, victim, savetype, modi)))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 3, level, 4, 4, 0));
		af[0].bitvector = AFF_NOTELEPORT;
		to_room = "$n0 теперь прикован$a к $N2.";
		to_vict = "Вы не сможете покинуть $N3.";
		break;

	case SPELL_LIGHT:
		if (!IS_NPC(ch) && !same_group(ch, victim))
		{
			send_to_char("Только на себя или одногруппника!\r\n", ch);
			return 0;
		}
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_HOLYLIGHT;
		to_room = "$n0 начал$g светиться ярким светом.";
		to_vict = "Вы засветились, освещая комнату.";
		break;

	case SPELL_DARKNESS:
		if (!IS_NPC(ch) && !same_group(ch, victim))
		{
			send_to_char("Только на себя или одногруппника!\r\n", ch);
			return 0;
		}
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].bitvector = AFF_HOLYDARK;
		to_room = "$n0 погрузил$g комнату во мрак.";
		to_vict = "Вы погрузили комнату в непроглядную тьму.";
		break;

	case SPELL_EVILESS:
		af[0].duration = pc_duration(victim, 10, GET_REMORT(ch), 1, 0, 0);
		af[0].location = APPLY_DAMROLL;
		af[0].modifier = 15;
		af[0].bitvector = AFF_EVILESS;
		af[1].duration = af[0].duration;
		af[1].location = APPLY_HITROLL;
		af[1].modifier = 7;
		af[1].bitvector = AFF_EVILESS;
		af[2].duration = af[0].duration;
		af[2].location = APPLY_HIT;
		af[2].modifier = GET_REAL_MAX_HIT(victim);
		af[2].bitvector = AFF_EVILESS;
		to_vict = "Черное облако покрыло вас.";
		to_room = "Черное облако покрыло $n3 с головы до пят.";
		break;

	case SPELL_WC_OF_THUNDER:
	case SPELL_ICESTORM:
	case SPELL_EARTHFALL:
	case SPELL_SHOCK:
		switch (spellnum)
		{
		case SPELL_WC_OF_THUNDER:
			savetype = SAVING_WILL;
			modi = GET_REAL_CON(ch) * 3 / 2;
			break;
		case SPELL_ICESTORM:
			savetype = SAVING_REFLEX;
			modi = CALC_SUCCESS(modi, 30);
			break;
		case SPELL_EARTHFALL:
			savetype = SAVING_REFLEX;
			modi = CALC_SUCCESS(modi, 95);
			break;
        case SPELL_SHOCK:
			savetype = SAVING_REFLEX;
			if (GET_CLASS(ch) == CLASS_CLERIC) {
                modi = CALC_SUCCESS(modi, 75);
			} else {
			    modi = CALC_SUCCESS(modi, 25);
			}
			break;
		}
		if (WAITLESS(victim) || (!WAITLESS(ch) && general_savingthrow(ch, victim, savetype, modi)))
		{
			success = FALSE;
			break;
		}
		switch (spellnum)
		{
		case SPELL_WC_OF_THUNDER:
			af[0].type = SPELL_DEAFNESS;
			af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 2, level + 3, 4, 6, 0));
			af[0].duration = complex_spell_modifier(ch, SPELL_DEAFNESS, GAPPLY_SPELL_EFFECT, af[0].duration);
			af[0].bitvector = AFF_DEAFNESS;
			af[0].battleflag = AF_BATTLEDEC;
			to_room = "$n0 оглох$q!";
			to_vict = "Вы оглохли.";

			if ((IS_NPC(victim) && AFF_FLAGGED(victim, af[0].bitvector)) ||
					(ch != victim && affected_by_spell(victim, SPELL_DEAFNESS)))
			{
				if (IN_ROOM(ch) == IN_ROOM(victim))
					send_to_char(NOEFFECT, ch);
			}
			else
			{
				affect_join(victim, af, accum_duration, FALSE, accum_affect, FALSE);
				act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
				act(to_room, TRUE, victim, 0, ch, TO_ROOM | TO_ARENA_LISTEN);
			}
		case SPELL_ICESTORM:
		case SPELL_EARTHFALL:
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 2, 0, 0, 0, 0));
			af[0].bitvector = AFF_MAGICSTOPFIGHT;
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			to_room = "$n3 оглушило.";
			to_vict = "Вас оглушило.";
			spellnum = SPELL_MAGICBATTLE;
			break;
        case SPELL_SHOCK:
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 2, 0, 0, 0, 0));
			af[0].bitvector = AFF_MAGICSTOPFIGHT;
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			to_room = "$n3 оглушило.";
			to_vict = "Вас оглушило.";
			spellnum = SPELL_MAGICBATTLE;
			mag_affects(level, ch, victim, SPELL_BLINDNESS, SAVING_STABILITY);
            break;
		}
		break;

//Заклинание плач. Далим.
	case SPELL_CRYING:
		if (AFF_FLAGGED(victim, AFF_CRYING) || (ch != victim && general_savingthrow(ch, victim, savetype, modi)))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		af[0].location = APPLY_HIT;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 4, 0, 0, 0, 0));
		af[0].modifier =
			-1 * MAX(1,
					 (MIN(29, GET_LEVEL(ch)) - MIN(24, GET_LEVEL(victim)) +
					  GET_REMORT(ch) / 3) * GET_MAX_HIT(victim) / 100);
		af[0].bitvector = AFF_CRYING;
		if (IS_NPC(victim))
		{
			af[1].location = APPLY_LIKES;
			af[1].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 5, 0, 0, 0, 0));
			af[1].modifier = -1 * MAX(1, ((level + 9) / 2 + 9 - GET_LEVEL(victim) / 2));
			af[1].bitvector = AFF_CRYING;
			af[1].battleflag = AF_BATTLEDEC;
			to_room = "$n0 издал$g протяжный стон.";
			break;
		}
		af[1].location = APPLY_CAST_SUCCESS;
		af[1].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 5, 0, 0, 0, 0));
		af[1].modifier = -1 * MAX(1, (level / 3 + GET_REMORT(ch) / 3 - GET_LEVEL(victim) / 10));
		af[1].bitvector = AFF_CRYING;
		af[1].battleflag = AF_BATTLEDEC;
		af[2].location = APPLY_MORALE;
		af[2].duration = af[1].duration;
		af[2].modifier = -1 * MAX(1, (level / 3 + GET_REMORT(ch) / 5 - GET_LEVEL(victim) / 5));
		af[2].bitvector = AFF_CRYING;
		af[2].battleflag = AF_BATTLEDEC;
		to_room = "$n0 издал$g протяжный стон.";
		to_vict = "Вы впали в уныние.";
		break;
//Заклинания Забвение, Бремя времени. Далим.
	case SPELL_OBLIVION:
	case SPELL_BURDEN_OF_TIME:
		if (WAITLESS(victim)
				|| general_savingthrow(ch, victim, SAVING_REFLEX,
									   CALC_SUCCESS(modi, (spellnum == SPELL_OBLIVION ? 40 : 90))))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		WAIT_STATE(victim, (level / 10 + 1) * PULSE_VIOLENCE);
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 3, 0, 0, 0, 0));
		af[0].bitvector = AFF_SLOW;
		af[0].battleflag = AF_BATTLEDEC;
		to_room = "Облако забвения окружило $n3.";
		to_vict = "Ваш разум помутился.";
		spellnum = SPELL_OBLIVION;
		break;

	case SPELL_PEACEFUL:
		if (AFF_FLAGGED(victim, AFF_PEACEFUL) || (IS_NPC(victim) && !AFF_FLAGGED(victim, AFF_CHARM)) ||
				(ch != victim && general_savingthrow(ch, victim, savetype, modi)))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		if (victim->get_fighting())
		{
			stop_fighting(victim, TRUE);
			change_fighting(victim, TRUE);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
		}
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 2, 0, 0, 0, 0));
		af[0].bitvector = AFF_PEACEFUL;
		to_room = "Взгляд $n1 потускнел, а сам он успокоился.";
		to_vict = "Ваша душа очистилась от зла и странно успокоилась.";
		break;

	case SPELL_STONEBONES:
		if (GET_MOB_VNUM(victim) < MOB_SKELETON || GET_MOB_VNUM(victim) > LAST_NECR_MOB)
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
		}
		af[0].location = APPLY_ARMOUR;
		af[0].duration = pc_duration(victim, 100, level, 1, 0, 0);
		af[0].modifier = level + 10;
		accum_duration = TRUE;
		to_vict = " ";
		to_room = "Кости $n1 обрели твердость кремня.";
		break;

	case SPELL_FAILURE:
		savetype = SAVING_WILL;
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		af[0].location = APPLY_MORALE;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 2, level, 2, 0, 0));
		af[0].modifier = -5 - (GET_LEVEL(ch) + GET_REMORT(ch)) / 2;
		af[1].location = number(1, 6);
		af[1].duration = af[0].duration;
		af[1].modifier = - (GET_LEVEL(ch) + GET_REMORT(ch) * 3) / 15;
		to_room = "Тяжелое бурое облако сгустилось над $n4.";
		to_vict = "Тяжелые тучи сгустились над вами, и вы почувствовали, что удача покинула вас.";
		break;

	case SPELL_GLITTERDUST:
		savetype = SAVING_REFLEX;
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi + 50))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}

		if (affected_by_spell(victim, SPELL_INVISIBLE))
		{
			affect_from_char(victim, SPELL_INVISIBLE);
		}
		if (affected_by_spell(victim, SPELL_CAMOUFLAGE))
		{
			affect_from_char(victim, SPELL_CAMOUFLAGE);
		}
		if (affected_by_spell(victim, SPELL_HIDE))
		{
			affect_from_char(victim, SPELL_HIDE);
		}
		af[0].location = APPLY_SAVING_REFLEX;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 4, 0, 0, 0, 0));
		af[0].modifier = (GET_LEVEL(ch) + GET_REMORT(ch)) / 3;
		af[0].bitvector = AFF_GLITTERDUST;
		accum_duration = TRUE;
		accum_affect = TRUE;
		to_room = "Облако ярко блестящей пыли накрыло $n3.";
		to_vict = "Липкая блестящая пыль покрыла вас с головы до пят.";
		break;

	case SPELL_SCREAM:
		savetype = SAVING_STABILITY;
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		af[0].bitvector = AFF_AFFRIGHT;
		af[0].location = APPLY_SAVING_WILL;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 2, level, 2, 0, 0));
		af[0].modifier = (2 * GET_LEVEL(ch) + GET_REMORT(ch)) / 4;

		af[1].bitvector = AFF_AFFRIGHT;
		af[1].location = APPLY_MORALE;
		af[1].duration = af[0].duration;
		af[1].modifier = -(GET_LEVEL(ch) + GET_REMORT(ch)) / 6;

		to_room = "$n0 побледнел$g и задрожал$g от страха.";
		to_vict = "Страх сжал ваше сердце ледяными когтями.";
		break;

	case SPELL_CATS_GRACE:
		af[0].location = APPLY_DEX;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		if (ch == victim)
			af[0].modifier = (level + 5) / 10;
		else
			af[0].modifier = (level + 10) / 15;
		accum_duration = TRUE;
		accum_affect = TRUE;
		to_vict = "Ваши движения обрели невиданную ловкость.";
		to_room = "Движения $n1 обрели невиданную ловкость.";
		break;

	case SPELL_BULL_BODY:
		af[0].location = APPLY_CON;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		if (ch == victim)
			af[0].modifier = (level + 5) / 10;
		else
			af[0].modifier = (level + 10) / 15;
		accum_duration = TRUE;
		accum_affect = TRUE;
		to_vict = "Ваше тело налилось звериной мощью.";
		to_room = "Плечи $n1 раздались вширь, а тело налилось звериной мощью.";
		break;

	case SPELL_SNAKE_WISDOM:
		af[0].location = APPLY_WIS;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].modifier = (level + 6) / 15;
		accum_duration = TRUE;
		accum_affect = TRUE;
		to_vict = "Шелест змеиной чешуи коснулся вашего сознания, и вы стали мудрее.";
		to_room = "$n спокойно и мудро посмотрел$g вокруг.";
		break;

	case SPELL_GIMMICKRY:
		af[0].location = APPLY_INT;
		af[0].duration = pc_duration(victim, 20, SECS_PER_PLAYER_AFFECT * GET_REMORT(ch), 1, 0, 0);
		af[0].modifier = (level + 6) / 15;
		accum_duration = TRUE;
		accum_affect = TRUE;
		to_vict = "Вы почувствовали, что для вашего ума более нет преград.";
		to_room = "$n хитро прищурил$u и поглядел$g по сторонам.";
		break;

	case SPELL_WC_OF_MENACE:
		savetype = SAVING_WILL;
		modi = GET_REAL_CON(ch);
		if (ch != victim && general_savingthrow(ch, victim, savetype, modi))
		{
			send_to_char(NOEFFECT, ch);
			success = FALSE;
			break;
		}
		af[0].location = APPLY_MORALE;
		af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
						 pc_duration(victim, 2, level + 3, 4, 6, 0));
		af[0].modifier = -dice((7 + level) / 8, 3);
		to_vict = "Похоже, сегодня не ваш день.";
		to_room = "Удача покинула $n3.";
		break;

	case SPELL_WC_OF_MADNESS:
		savetype = SAVING_STABILITY;
		modi = GET_REAL_CON(ch) * 3 / 2;
		if (ch == victim || !general_savingthrow(ch, victim, savetype, modi))
		{
			af[0].location = APPLY_INT;
			af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
							 pc_duration(victim, 2, level + 3, 4, 6, 0));
			af[0].modifier = -dice((7 + level) / 8, 2);
			to_vict = "Вы потеряли рассудок.";
			to_room = "$n0 потерял$g рассудок.";

			savetype = SAVING_STABILITY;
			modi = GET_REAL_CON(ch) * 2;
			if (ch == victim || !general_savingthrow(ch, victim, savetype, modi))
			{
				af[1].location = APPLY_CAST_SUCCESS;
				af[1].duration = af[0].duration;
				af[1].modifier = -(dice((2 + level) / 3, 4) + dice(GET_REMORT(ch) / 2, 5));

				af[2].location = APPLY_MANAREG;
				af[2].duration = af[1].duration;
				af[2].modifier = af[1].modifier;
				to_vict = "Вы обезумели.";
				to_room = "$n0 обезумел$g.";
			}
		}
		else
		{
			savetype = SAVING_STABILITY;
			modi = GET_REAL_CON(ch) * 2;
			if (!general_savingthrow(ch, victim, savetype, modi))
			{
				af[0].location = APPLY_CAST_SUCCESS;
				af[0].duration = calculate_resistance_coeff(victim, get_resist_type(spellnum),
								 pc_duration(victim, 2, level + 3, 4, 6, 0));
				af[0].modifier = -(dice((2 + level) / 3, 4) + dice(GET_REMORT(ch) / 2, 5));

				af[1].location = APPLY_MANAREG;
				af[1].duration = af[0].duration;
				af[1].modifier = af[0].modifier;
				to_vict = "Вас охватила паника.";
				to_room = "$n0 начал$g сеять панику.";
			}
			else
			{
				send_to_char(NOEFFECT, ch);
				success = FALSE;
			}
		}
		update_spell = TRUE;
		break;

	case SPELL_WC_OF_BATTLE:
		af[0].location = APPLY_AC;
		af[0].modifier = - (10 + MIN(20, 2 * GET_REMORT(ch)));
		af[0].duration = pc_duration(victim, 1, ch->get_skill(SKILL_WARCRY), 28, 7, 0);
		to_room = NULL;
		break;

	case SPELL_WC_OF_POWER:
		af[0].location = APPLY_HIT;
		af[0].modifier = MIN(SCHAR_MAX, (4 * ch->get_con() + ch->get_skill(SKILL_WARCRY)) / 2);
		af[0].duration = pc_duration(victim, 1, ch->get_skill(SKILL_WARCRY), 28, 7, 0);
		to_vict = NULL;
		to_room = NULL;
		break;

	case SPELL_WC_OF_BLESS:
		af[0].location = APPLY_SAVING_STABILITY;
		af[0].modifier = -(4 * ch->get_con() + ch->get_skill(SKILL_WARCRY)) / 24;
		af[0].duration = pc_duration(victim, 1, ch->get_skill(SKILL_WARCRY), 28, 7, 0);
		af[1].location = APPLY_SAVING_WILL;
		af[1].modifier = af[0].modifier;
		af[1].duration = af[0].duration;
		to_vict = NULL;
		to_room = NULL;
		break;

	case SPELL_WC_OF_COURAGE:
		af[0].location = APPLY_HITROLL;
		af[0].modifier = (44 + ch->get_skill(SKILL_WARCRY)) / 45;
		af[0].duration = pc_duration(victim, 1, ch->get_skill(SKILL_WARCRY), 28, 7, 0);
		af[1].location = APPLY_DAMROLL;
		af[1].modifier = (29 + ch->get_skill(SKILL_WARCRY)) / 30;
		af[1].duration = af[0].duration;
		to_vict = NULL;
		to_room = NULL;
		break;

	case SPELL_ACONITUM_POISON:
		af[0].location = APPLY_ACONITUM_POISON;
		af[0].duration = 7;
		af[0].modifier = level;
		af[0].bitvector = AFF_POISON;
		af[0].battleflag = AF_SAME_TIME;
		to_vict = "Вы почувствовали себя отравленным.";
		to_room = "$n позеленел$g от действия яда.";
		break;

	case SPELL_SCOPOLIA_POISON:
		af[0].location = APPLY_SCOPOLIA_POISON;
		af[0].duration = 7;
		af[0].modifier = 5;
		af[0].bitvector = AFF_POISON | AFF_SCOPOLIA_POISON;
		af[0].battleflag = AF_SAME_TIME;
		to_vict = "Вы почувствовали себя отравленным.";
		to_room = "$n позеленел$g от действия яда.";
		break;

	case SPELL_BELENA_POISON:
		af[0].location = APPLY_BELENA_POISON;
		af[0].duration = 7;
		af[0].modifier = 5;
		af[0].bitvector = AFF_POISON;
		af[0].battleflag = AF_SAME_TIME;
		to_vict = "Вы почувствовали себя отравленным.";
		to_room = "$n позеленел$g от действия яда.";
		break;

	case SPELL_DATURA_POISON:
		af[0].location = APPLY_DATURA_POISON;
		af[0].duration = 7;
		af[0].modifier = 5;
		af[0].bitvector = AFF_POISON;
		af[0].battleflag = AF_SAME_TIME;
		to_vict = "Вы почувствовали себя отравленным.";
		to_room = "$n позеленел$g от действия яда.";
		break;
	case SPELL_LACKY:
		af[0].duration = pc_duration(victim, 6, 0, 0, 0, 0);
		af[0].bitvector = AFF_LACKY;
//Polud пробный обработчик аффектов
		af[0].handler = boost::shared_ptr<LackyAffectHandler>(new LackyAffectHandler());
		af[0].type = SPELL_LACKY;
		af[0].location = APPLY_HITROLL;
		af[0].modifier = 0;
		to_room = "$n вдохновенно выпятил$g грудь.";
		to_vict = "Вы почувствовали вдохновение.";
		break;
	}


	/*
	 * If this is a mob that has this affect set in its mob file, do not
	 * perform the affect.  This prevents people from un-sancting mobs
	 * by sancting them and waiting for it to fade, for example.
	 */
	if (IS_NPC(victim) && success)
		for (i = 0; i < MAX_SPELL_AFFECTS && success; i++)
			if (AFF_FLAGGED(victim, af[i].bitvector))
			{
				if (IN_ROOM(ch) == IN_ROOM(victim))
					send_to_char(NOEFFECT, ch);
				success = FALSE;
			}

	// * If the victim is already affected by this spell

	// Если прошло, и спелл дружествееный - то снять его нафиг чтобы обновить
	if (!SpINFO.violent && affected_by_spell(victim, spellnum) && success)
	{
//    affect_from_char(victim,spellnum);
		update_spell = TRUE;
	}

	if ((ch != victim) && affected_by_spell(victim, spellnum) && success && (!update_spell))
	{

		if (IN_ROOM(ch) == IN_ROOM(victim))
			send_to_char(NOEFFECT, ch);
		success = FALSE;
	}


	for (i = 0; success && i < MAX_SPELL_AFFECTS; i++)
	{
		af[i].type = spellnum;
		if (af[i].bitvector || af[i].location != APPLY_NONE)
		{
			af[i].duration = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_EFFECT, af[i].duration);
			if (update_spell)
				affect_join_fspell(victim, af + i);
			else
				affect_join(victim, af + i, accum_duration, FALSE, accum_affect, FALSE);
		}
	}

	if (success)
	{
		if (spellnum == SPELL_POISON)
			victim->Poisoner = GET_ID(ch);
		if (spellnum == SPELL_EVILESS && (GET_HIT(victim) < GET_MAX_HIT(victim)))
		{
			GET_HIT(victim) = GET_MAX_HIT(victim); //Без этой строки update_pos еще не видит восстановленных ХП
			update_pos(victim);
		}
		if (to_vict != NULL)
			act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
		if (to_room != NULL)
			act(to_room, TRUE, victim, 0, ch, TO_ROOM | TO_ARENA_LISTEN);
		return 1;
	}
	return 0;
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in Circle 3.0; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

// * These use act(), don't put the \r\n.
const char *mag_summon_msgs[] =
{
	"\r\n",
	"$n сделал$g несколько изящних пассов - вы почувствовали странное дуновение!",
	"$n поднял$g труп!",
	"$N появил$G из клубов голубого дыма!",
	"$N появил$G из клубов зеленого дыма!",
	"$N появил$G из клубов красного дыма!",
	"$n сделал$g несколько изящных пассов - вас обдало порывом холодного ветра.",
	"$n сделал$g несколько изящных пассов, от чего ваши волосы встали дыбом.",
	"$n сделал$g несколько изящных пассов, обдав вас нестерпимым жаром.",
	"$n сделал$g несколько изящных пассов, вызвав у вас приступ тошноты.",
	"$n раздвоил$u!",
	"$n оживил$g труп!",
	"$n призвал$g защитника!",
	"Огненный хранитель появился из вихря пламени!"
};

// * Keep the \r\n because these use send_to_char.
const char *mag_summon_fail_msgs[] =
{
	"\r\n",
	"Нет такого существа в мире.\r\n",
	"Жаль, сорвалось...\r\n",
	"Ничего.\r\n",
	"Черт! Ничего не вышло.\r\n",
	"Вы не смогли сделать этого!\r\n",
	"Ваша магия провалилась.\r\n",
	"У вас нет подходящего трупа!\r\n"
};

int mag_summons(int level, CHAR_DATA * ch, OBJ_DATA * obj, int spellnum, int savetype)
{
	CHAR_DATA *tmp_mob, *mob = NULL;
	OBJ_DATA *tobj, *next_obj;
	AFFECT_DATA af;
	struct follow_type *k;
	int pfail = 0, msg = 0, fmsg = 0, handle_corpse = FALSE, keeper = FALSE, cha_num = 0, modifier = 0;
	mob_vnum mob_num;

	if (ch == NULL)
		return 0;

	switch (spellnum)
	{
	case SPELL_CLONE:
		msg = 10;
		fmsg = number(3, 5);	// Random fail message.
		mob_num = MOB_DOUBLE;
		pfail = 50 - GET_CAST_SUCCESS(ch) - GET_REMORT(ch) * 5;	// 50% failure, should be based on something later.
		keeper = TRUE;
		break;

	case SPELL_SUMMON_KEEPER:
		msg = 12;
		fmsg = number(2, 6);
		mob_num = MOB_KEEPER;
		if (ch->get_fighting())
			pfail = 50 - GET_CAST_SUCCESS(ch) - GET_REMORT(ch);
		else
			pfail = 0;
		keeper = TRUE;
		break;

	case SPELL_SUMMON_FIREKEEPER:
		msg = 13;
		fmsg = number(2, 6);
		mob_num = MOB_FIREKEEPER;
		if (ch->get_fighting())
			pfail = 50 - GET_CAST_SUCCESS(ch) - GET_REMORT(ch);
		else
			pfail = 0;
		keeper = TRUE;
		break;

	case SPELL_ANIMATE_DEAD:
		if (obj == NULL || !IS_CORPSE(obj))
		{
			act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
			return 0;
		}
		mob_num = GET_OBJ_VAL(obj, 2);
		if (mob_num <= 0)
			mob_num = MOB_SKELETON;
		else
		{
			const int real_mob_num = real_mobile(mob_num);
			tmp_mob = (mob_proto + real_mob_num);
			tmp_mob->set_normal_morph();
			pfail = 10 + tmp_mob->get_con() * 2
				- number(1, GET_LEVEL(ch)) - GET_CAST_SUCCESS(ch) - GET_REMORT(ch) * 5;


			if (GET_LEVEL(mob_proto + real_mob_num) <= 5)
			{
				mob_num = MOB_SKELETON;
			}
			else if (GET_LEVEL(mob_proto + real_mob_num) <= 10)
			{
				mob_num = MOB_ZOMBIE;
			}
			else if (GET_LEVEL(mob_proto + real_mob_num) <= 20)
			{
				mob_num = MOB_BONEDOG;
			}
			else if (GET_LEVEL(mob_proto + real_mob_num) <= 27)
			{
				mob_num = MOB_BONEDRAGON;
			}
			else
			{
				mob_num = MOB_BONESPIRIT;
			}

			if (GET_LEVEL(ch) + GET_REMORT(ch) + 4 < 15 && mob_num > MOB_ZOMBIE)
			{
				mob_num = MOB_ZOMBIE;
			}
			else if (GET_LEVEL(ch) + GET_REMORT(ch) + 4 < 25 && mob_num > MOB_BONEDOG)
			{
				mob_num = MOB_BONEDOG;
			}
			else if (GET_LEVEL(ch) + GET_REMORT(ch) + 4 < 32 && mob_num > MOB_BONEDRAGON)
			{
				mob_num = MOB_BONEDRAGON;
			}
		}
		handle_corpse = TRUE;
		msg = number(1, 9);
		fmsg = number(2, 6);
		break;

	case SPELL_RESSURECTION:
		if (obj == NULL || !IS_CORPSE(obj))
		{
			act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
			return 0;
		}
		if ((mob_num = GET_OBJ_VAL(obj, 2)) <= 0)
		{
			send_to_char("Вы не можете поднять труп этого монстра!\r\n", ch);
			return 0;
		}

		handle_corpse = TRUE;
		msg = 11;
		fmsg = number(2, 6);

		tmp_mob = mob_proto + real_mobile(mob_num);
		tmp_mob->set_normal_morph();

		pfail = 10 + tmp_mob->get_con() * 2
			- number(1, GET_LEVEL(ch)) - GET_CAST_SUCCESS(ch) - GET_REMORT(ch) * 5;
		break;

	default:
		return 0;
	}

	if (AFF_FLAGGED(ch, AFF_CHARM))
	{
		send_to_char("Вы слишком зависимы, чтобы искать себе последователей!\r\n", ch);
		return 0;
	}

	if (!IS_IMMORTAL(ch) && number(0, 101) < pfail && savetype)
	{
		send_to_char(mag_summon_fail_msgs[fmsg], ch);
		if (handle_corpse)
			extract_obj(obj);
		return 0;
	}

	if (!(mob = read_mobile(-mob_num, VIRTUAL)))
	{
		send_to_char("Вы точно не помните, как создать данного монстра.\r\n", ch);
		return 0;
	}
	// очищаем умения у оживляемого моба
	if (spellnum == SPELL_RESSURECTION)
	{
		clear_char_skills(mob);
		// Меняем именование.
		sprintf(buf2, "умертвие %s %s", GET_PAD(mob, 1), GET_NAME(mob));
		mob->set_pc_name(buf2);
		sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
		mob->set_npc_name(buf2);
		mob->player_data.long_descr = NULL;
		sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
		GET_PAD(mob, 0) = str_dup(buf2);
		sprintf(buf2, "умертвию %s", GET_PAD(mob, 1));
		GET_PAD(mob, 2) = str_dup(buf2);
		sprintf(buf2, "умертвие %s", GET_PAD(mob, 1));
		GET_PAD(mob, 3) = str_dup(buf2);
		sprintf(buf2, "умертвием %s", GET_PAD(mob, 1));
		GET_PAD(mob, 4) = str_dup(buf2);
		sprintf(buf2, "умертвии %s", GET_PAD(mob, 1));
		GET_PAD(mob, 5) = str_dup(buf2);
		sprintf(buf2, "умертвия %s", GET_PAD(mob, 1));
		GET_PAD(mob, 1) = str_dup(buf2);
		GET_SEX(mob) = SEX_NEUTRAL;
		SET_BIT(MOB_FLAGS(mob, MOB_RESURRECTED), MOB_RESURRECTED);	// added by Pereplut
	}
	char_to_room(mob, ch->in_room);
	if (!IS_IMMORTAL(ch) && (AFF_FLAGGED(mob, AFF_SANCTUARY) || MOB_FLAGGED(mob, MOB_PROTECT)))
	{
		send_to_char("Оживляемый был освящен Богами и противится этому!\r\n", ch);
		extract_char(mob, FALSE);
		return 0;
	}
	if (!IS_IMMORTAL(ch) && (GET_MOB_SPEC(mob) || MOB_FLAGGED(mob, MOB_NORESURRECTION) || MOB_FLAGGED(mob, MOB_AREA_ATTACK)))
	{
		send_to_char("Вы не можете обрести власть над этим созданием!\r\n", ch);
		extract_char(mob, FALSE);
		return 0;
	}
// shapirus: нельзя оживить моба под ЗБ
	if (!IS_IMMORTAL(ch) && AFF_FLAGGED(mob, AFF_SHIELD))
	{
		send_to_char("Боги защищают это существо даже после смерти.\r\n", ch);
		extract_char(mob, FALSE);
		return 0;
	}
	if (IS_HORSE(mob))
	{
		send_to_char("Это был боевой скакун, а не хухры-мухры.\r\n", ch);
		extract_char(mob, FALSE);
		return 0;
	}

	if (!check_charmee(ch, mob, spellnum))
	{
		extract_char(mob, FALSE);
		if (handle_corpse)
			extract_obj(obj);
		return 0;
	}

	mob->set_exp(0);
	IS_CARRYING_W(mob) = 0;
	IS_CARRYING_N(mob) = 0;
//Polud при оживлении и поднятии трупа лоадились куны из прототипа
	mob->set_gold(0);
	GET_GOLD_NoDs(mob) = 0;
	GET_GOLD_SiDs(mob) = 0;
//-Polud
	af.type = SPELL_CHARM;
	if (weather_info.moon_day < 14)
		af.duration = pc_duration(mob, GET_REAL_WIS(ch) + number(0, weather_info.moon_day % 14), 0, 0, 0, 0);
	else
		af.duration =
			pc_duration(mob, GET_REAL_WIS(ch) + number(0, 14 - weather_info.moon_day % 14), 0, 0, 0, 0);
	af.modifier = 0;
	af.location = 0;
	af.bitvector = AFF_CHARM;
	af.battleflag = 0;
	affect_to_char(mob, &af);
	if (keeper)
	{
		af.bitvector = AFF_HELPER;
		affect_to_char(mob, &af);
		mob->set_skill(SKILL_RESCUE, 100);
// shapirus: проставим флаг клона тут в явном виде, чтобы
// режим отсева клонов при показе группы работал гарантированно
// (это была идиотская идея)
//      SET_BIT (MOB_FLAGS (mob, MOB_CLONE), MOB_CLONE);
	}

	SET_BIT(MOB_FLAGS(mob, MOB_CORPSE), MOB_CORPSE);
	if (spellnum == SPELL_CLONE)  	// Don't mess up the proto with strcpy.
	{
		sprintf(buf2, "двойник %s %s", GET_PAD(ch, 1), GET_NAME(ch));
		mob->set_pc_name(buf2);
		sprintf(buf2, "двойник %s", GET_PAD(ch, 1));
		mob->set_npc_name(buf2);
		mob->player_data.long_descr = NULL;
		sprintf(buf2, "двойник %s", GET_PAD(ch, 1));
		GET_PAD(mob, 0) = str_dup(buf2);
		sprintf(buf2, "двойника %s", GET_PAD(ch, 1));
		GET_PAD(mob, 1) = str_dup(buf2);
		sprintf(buf2, "двойнику %s", GET_PAD(ch, 1));
		GET_PAD(mob, 2) = str_dup(buf2);
		sprintf(buf2, "двойника %s", GET_PAD(ch, 1));
		GET_PAD(mob, 3) = str_dup(buf2);
		sprintf(buf2, "двойником %s", GET_PAD(ch, 1));
		GET_PAD(mob, 4) = str_dup(buf2);
		sprintf(buf2, "двойнике %s", GET_PAD(ch, 1));
		GET_PAD(mob, 5) = str_dup(buf2);

		mob->set_str(ch->get_str());
		mob->set_dex(ch->get_dex());
		mob->set_con(ch->get_con());
		mob->set_wis(ch->get_wis());
		mob->set_int(ch->get_int());
		mob->set_cha(ch->get_cha());

		mob->set_level(ch->get_level());
//      GET_HR (mob) = GET_HR (ch);
// shapirus: нефиг клонам дамагать. сделаем хитролл достаточно плохой.
		GET_HR(mob) = -20;
		GET_AC(mob) = GET_AC(ch);
		GET_DR(mob) = GET_DR(ch);

		GET_MAX_HIT(mob) = GET_MAX_HIT(ch);
		GET_HIT(mob) = GET_MAX_HIT(ch);
		mob->mob_specials.damnodice = 0;
		mob->mob_specials.damsizedice = 0;
		mob->set_gold(0);
		GET_GOLD_NoDs(mob) = 0;
		GET_GOLD_SiDs(mob) = 0;
		mob->set_exp(0);

		GET_POS(mob) = POS_STANDING;
		GET_DEFAULT_POS(mob) = POS_STANDING;
		GET_SEX(mob) = SEX_MALE;

		mob->set_class(ch->get_class());
		GET_WEIGHT(mob) = GET_WEIGHT(ch);
		GET_HEIGHT(mob) = GET_HEIGHT(ch);
		GET_SIZE(mob) = GET_SIZE(ch);
		SET_BIT(MOB_FLAGS(mob, MOB_CLONE), MOB_CLONE);
		REMOVE_BIT(MOB_FLAGS(mob, MOB_MOUNTING), MOB_MOUNTING);
	}
	act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM | TO_ARENA_LISTEN);
//   load_mtrigger(mob);
	add_follower(mob, ch);
	if (spellnum == SPELL_CLONE)
	{
		// клоны теперь кастятся все вместе // ужасно некрасиво сделано
		for (k = ch->followers; k; k = k->next)
			if (AFF_FLAGGED(k->follower, AFF_CHARM) && k->follower->master == ch)
				cha_num++;
		cha_num = MAX(1, (GET_LEVEL(ch) + 4) / 5 - 2) - cha_num;
		if (cha_num < 1)
			return 0;
		mag_summons(level, ch, obj, spellnum, 0);
	}
	if (spellnum == SPELL_ANIMATE_DEAD)
	{
		SET_BIT(MOB_FLAGS(mob, MOB_RESURRECTED), MOB_RESURRECTED);	// added by Pereplut
	}
//added by Adept
	if (spellnum == SPELL_SUMMON_FIREKEEPER)
	{
		if (get_effective_cha(ch, SPELL_SUMMON_FIREKEEPER) >= 30)
		{
			SET_BIT(AFF_FLAGS(mob, AFF_FIRESHIELD), AFF_FIRESHIELD);
		}
		else
		{
			SET_BIT(AFF_FLAGS(mob, AFF_FIREAURA), AFF_FIREAURA);
		}

		modifier = VPOSI((int)get_effective_cha(ch, SPELL_SUMMON_FIREKEEPER) - 20, 0, 30);

		GET_DR(mob) = 10 + modifier * 3 / 2;
		GET_NDD(mob) = 1;
		GET_SDD(mob) = modifier / 5 + 1;
		mob->mob_specials.ExtraAttack = 0;

		GET_MAX_HIT(mob) = GET_HIT(mob) = 300 + number(modifier * 12, modifier * 16);
		mob->set_skill(SKILL_AWAKE, 50 + modifier * 2);
		SET_BIT(PRF_FLAGS(mob, PRF_AWAKE), PRF_AWAKE);
	}
// shapirus: !train для мобов, созданных магией, тоже сделаем
	SET_BIT(MOB_FLAGS(mob, MOB_NOTRAIN), MOB_NOTRAIN);

	// А надо ли это вообще делать???
	if (handle_corpse)
	{
		for (tobj = obj->contains; tobj;)
		{
			next_obj = tobj->next_content;
			obj_from_obj(tobj);
			obj_to_room(tobj, IN_ROOM(ch));
			if (!obj_decay(tobj) && tobj->in_room != NOWHERE)
				act("На земле остал$U лежать $o.", FALSE, ch, tobj, 0, TO_ROOM | TO_ARENA_LISTEN);
			tobj = next_obj;
		}
		extract_obj(obj);
	}


	return 1;
}


int mag_points(int level, CHAR_DATA * ch, CHAR_DATA * victim, int spellnum, int savetype)
{
	int hit = 0, move = 0;

	if (victim == NULL)
		return 0;

	switch (spellnum)
	{
	case SPELL_CURE_LIGHT:
		hit = dice(6, 3) + (level + 2) / 3;
		send_to_char("Вы почувствовали себя немножко лучше.\r\n", victim);
		break;
	case SPELL_CURE_SERIOUS:
		hit = dice(25, 2) + (level + 2) / 3;
		send_to_char("Вы почувствовали себя намного лучше.\r\n", victim);
		break;
	case SPELL_CURE_CRITIC:
		hit = dice(45, 2) + level;
		send_to_char("Вы почувствовали себя значительно лучше.\r\n", victim);
		break;
	case SPELL_HEAL:
	case SPELL_GROUP_HEAL:
//MZ.overflow_fix
		hit = GET_REAL_MAX_HIT(victim) - GET_HIT(victim);
//-MZ.overflow_fix
		send_to_char("Вы почувствовали себя здоровым.\r\n", victim);
		break;
	case SPELL_PATRONAGE:
		hit = (GET_LEVEL(victim) + GET_REMORT(victim)) * 2;
		break;
	case SPELL_REFRESH:
	case SPELL_GROUP_REFRESH:
//MZ.overflow_fix
		move = GET_REAL_MAX_MOVE(victim) - GET_MOVE(victim);
//-MZ.overflow_fix
		send_to_char("Вы почувствовали себя полным сил.\r\n", victim);
		break;
	case SPELL_EXTRA_HITS:
		hit = dice(10, level / 3) + level;
		send_to_char("По вашему телу начала струиться живительная сила.\r\n", victim);
		break;
	case SPELL_FULL:
		if (!IS_NPC(victim) && !IS_IMMORTAL(victim))
		{
			GET_COND(victim, THIRST) = 24;
			GET_COND(victim, FULL) = 24;
			send_to_char("Вы полностью насытились.\r\n", victim);
		}
		break;
	case SPELL_WC_OF_POWER:
		hit = (4 * ch->get_con() + ch->get_skill(SKILL_WARCRY)) / 2;
		send_to_char("По вашему телу начала струиться живительная сила.\r\n", victim);
		break;
	}

	hit = complex_spell_modifier(ch, spellnum, GAPPLY_SPELL_EFFECT, hit);

	if (hit && victim->get_fighting() && ch != victim)
	{
		pk_agro_action(ch, victim->get_fighting());
	}

	if ((spellnum == SPELL_EXTRA_HITS || spellnum == SPELL_PATRONAGE)
			&& (GET_HIT(victim) < MAX_HITS))
//MZ.extrahits_fix
		if (GET_REAL_MAX_HIT(victim) <= 0)
			GET_HIT(victim) = MAX(GET_HIT(victim), MIN(GET_HIT(victim) + hit, 1));
		else
			GET_HIT(victim) = MAX(GET_HIT(victim),
								  MIN(GET_HIT(victim) + hit,
									  GET_REAL_MAX_HIT(victim) + GET_REAL_MAX_HIT(victim) * 33 / 100));
//-MZ.extrahits_fix
	else if (GET_HIT(victim) < GET_REAL_MAX_HIT(victim))
		GET_HIT(victim) = MIN(GET_HIT(victim) + hit, GET_REAL_MAX_HIT(victim));
//MZ.overflow_fix
	if (GET_MOVE(victim) < GET_REAL_MAX_MOVE(victim))
		GET_MOVE(victim) = MIN(GET_MOVE(victim) + move, GET_REAL_MAX_MOVE(victim));
//-MZ.overflow_fix
	update_pos(victim);
	return 1;
}

#define NODISPELL(hjp) (!hjp || !spell_info[hjp->type].name    || \
                        *spell_info[hjp->type].name == '!' || \
			hjp->bitvector == AFF_CHARM  || \
			hjp->type == SPELL_CHARM || \
			hjp->type == SPELL_QUEST || \
			hjp->type == SPELL_FASCINATION || \
			hjp->type == SPELL_PATRONAGE)

int mag_unaffects(int level, CHAR_DATA * ch, CHAR_DATA * victim, int spellnum, int type)
{
	AFFECT_DATA *hjp;
	int spell = 0, remove = 0, rspell = 0;
	const char *to_vict = NULL, *to_room = NULL;

	if (victim == NULL)
		return 0;

	switch (spellnum)
	{
	case SPELL_CURE_BLIND:
		spell = SPELL_BLINDNESS;
		to_vict = "К вам вернулась способность видеть.";
		to_room = "$n прозрел$g.";
		break;
	case SPELL_REMOVE_POISON:
		spell = SPELL_POISON;
		to_vict = "Тепло заполнило ваше тело.";
		to_room = "$n выглядит лучше.";
		break;
	case SPELL_CURE_PLAQUE:
		spell = SPELL_PLAQUE;
		to_vict = "Лихорадка прекратилась.";
		break;
	case SPELL_REMOVE_CURSE:
		spell = SPELL_CURSE;
		to_vict = "Боги вернули вам свое покровительство.";
		break;
	case SPELL_REMOVE_HOLD:
		spell = SPELL_HOLD;
		to_vict = "К вам вернулась способность двигаться.";
		break;
	case SPELL_REMOVE_SIELENCE:
		spell = SPELL_SIELENCE;
		to_vict = "К вам вернулась способность разговаривать.";
		break;
	case SPELL_REMOVE_DEAFNESS:
		spell = SPELL_DEAFNESS;
		to_vict = "К вам вернулась способность слышать.";
		break;
	case SPELL_DISPELL_MAGIC:
		if (!IS_NPC(ch) && !same_group(ch, victim))
		{
			send_to_char("Только на себя или одногруппника!\r\n", ch);
			return 0;
		}

		for (spell = 0, hjp = victim->affected; hjp; hjp = hjp->next, spell++);

		if (!spell)
		{
			send_to_char(NOEFFECT, ch);
			return 0;
		}

		rspell = number(1, spell);

		for (hjp = victim->affected; spell > 1 && NODISPELL(hjp); hjp = hjp->next, spell--);


		for (spell = 0, hjp = victim->affected; spell != rspell - 1; hjp = hjp->next, spell++);

		if (NODISPELL(hjp))
		{
			send_to_char(NOEFFECT, ch);
			return 0;
		}

		spell = hjp->type;
		remove = TRUE;
		break;
	default:
		log("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
		return 0;
	}

  if(spellnum == SPELL_REMOVE_POISON && !affected_by_spell(victim, spell))
  {
  	if(affected_by_spell(victim, SPELL_ACONITUM_POISON))
  	 spell = SPELL_ACONITUM_POISON;
		else if(affected_by_spell(victim, SPELL_SCOPOLIA_POISON))
  	 spell = SPELL_SCOPOLIA_POISON;
		else if(affected_by_spell(victim, SPELL_BELENA_POISON))
  	 spell = SPELL_BELENA_POISON;
		else if(affected_by_spell(victim, SPELL_DATURA_POISON))
  	 spell = SPELL_DATURA_POISON;
  }
	if (!affected_by_spell(victim, spell))
	{
		if (spellnum != SPELL_HEAL)	// 'cure blindness' message.
			send_to_char(NOEFFECT, ch);
		return 0;
	}
	spellnum = spell;
	if (ch != victim && !remove)
	{
		if (IS_SET(SpINFO.routines, NPC_AFFECT_NPC))
		{
			pk_agro_action(ch, victim);
		}
		else if (IS_SET(SpINFO.routines, NPC_AFFECT_PC) && victim->get_fighting())
		{
			pk_agro_action(ch, victim->get_fighting());
		}
	}
//Polud затычка для закла !удалить яд!. По хорошему нужно его переделать с параметром - тип удаляемого яда
	if (spell == SPELL_POISON)
	{
		affect_from_char(victim, SPELL_ACONITUM_POISON);
		affect_from_char(victim, SPELL_DATURA_POISON);
		affect_from_char(victim, SPELL_SCOPOLIA_POISON);
		affect_from_char(victim, SPELL_BELENA_POISON);
	}
	affect_from_char(victim, spell);
	if (to_vict != NULL)
		act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
	if (to_room != NULL)
		act(to_room, TRUE, victim, 0, ch, TO_ROOM | TO_ARENA_LISTEN);

	return 1;
}


int mag_alter_objs(int level, CHAR_DATA * ch, OBJ_DATA * obj, int spellnum, int savetype)
{
	const char *to_char = NULL, *to_room = NULL;

	if (obj == NULL)
		return 0;

	if (IS_OBJ_STAT(obj, ITEM_NOALTER))
	{
		act("$o устойчив$A к вашей магии.", TRUE, ch, obj, 0, TO_CHAR);
		return 0;
	}

	switch (spellnum)
	{
	case SPELL_BLESS:
		if (!IS_OBJ_STAT(obj, ITEM_BLESS) && (GET_OBJ_WEIGHT(obj) <= 5 * GET_LEVEL(ch)))
		{
			SET_BIT(GET_OBJ_EXTRA(obj, ITEM_BLESS), ITEM_BLESS);
			if (IS_OBJ_STAT(obj, ITEM_NODROP))
			{
				REMOVE_BIT(GET_OBJ_EXTRA(obj, ITEM_NODROP), ITEM_NODROP);
				if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
					GET_OBJ_VAL(obj, 2)++;
			}
			GET_OBJ_MAX(obj) += MAX(GET_OBJ_MAX(obj) >> 2, 1);
			GET_OBJ_CUR(obj) = GET_OBJ_MAX(obj);
			to_char = "$o вспыхнул$G голубым светом и тут же погас$Q.";
			obj->timed_spell.add(obj, SPELL_BLESS, -1);
		}
		break;
	case SPELL_CURSE:
		if (!IS_OBJ_STAT(obj, ITEM_NODROP))
		{
			SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NODROP), ITEM_NODROP);
			if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
			{
				if (GET_OBJ_VAL(obj, 2) > 0)
					GET_OBJ_VAL(obj, 2)--;
			}
			else if (ObjSystem::is_armor_type(obj))
			{
				if (GET_OBJ_VAL(obj, 0) > 0)
					GET_OBJ_VAL(obj, 0)--;
				if (GET_OBJ_VAL(obj, 1) > 0)
					GET_OBJ_VAL(obj, 1)--;
			}
			to_char = "$o вспыхнул$G красным светом и тут же погас$Q.";
		}
		break;
	case SPELL_INVISIBLE:
		if (!IS_OBJ_STAT(obj, ITEM_NOINVIS)
				&& !IS_OBJ_STAT(obj, ITEM_INVISIBLE))
		{
			SET_BIT(GET_FLAG(obj->obj_flags.extra_flags, ITEM_INVISIBLE), ITEM_INVISIBLE);
			to_char = "$o растворил$U в пустоте.";
		}
		break;
	case SPELL_POISON:
		if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
				(GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
				(GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3))
		{
			GET_OBJ_VAL(obj, 3) = 1;
			to_char = "$o отравлен$G.";
		}
		break;
	case SPELL_REMOVE_CURSE:
		if (IS_OBJ_STAT(obj, ITEM_NODROP))
		{
			REMOVE_BIT(GET_OBJ_EXTRA(obj, ITEM_NODROP), ITEM_NODROP);
			if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
				GET_OBJ_VAL(obj, 2)++;
			to_char = "$o вспыхнул$G розовым светом и тут же погас$Q.";
		}
		break;
	case SPELL_ENCHANT_WEAPON:
		if (ch == NULL || obj == NULL)
			return 0;
		// Either already enchanted or not a weapon.
		if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || OBJ_FLAGGED(obj, ITEM_MAGIC))
			break;

		SET_BIT(GET_OBJ_EXTRA(obj, ITEM_MAGIC), ITEM_MAGIC);
		obj->affected[0].location = APPLY_HITROLL;
		obj->affected[0].modifier = 1 + (WAITLESS(ch) ? 3 : (level >= 18));
		obj->affected[1].location = APPLY_DAMROLL;
		obj->affected[1].modifier = 1 + (WAITLESS(ch) ? 3 : (level >= 20));
		if (!WAITLESS(ch))
		{
			obj->set_timer(MAX(obj->get_timer(), 4 * 24 * 60));
		}
		if (GET_RELIGION(ch) == RELIGION_MONO)
			to_char = "$o вспыхнул$G на миг голубым светом и тут же потух$Q.";
		else if (GET_RELIGION(ch) == RELIGION_POLY)
			to_char = "$o вспыхнул$G на миг красным светом и тут же потух$Q.";
		else
			to_char = "$o вспыхнул$G на миг желтым светом и тут же потух$Q.";
		break;

	case SPELL_REMOVE_POISON:
		if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
				(GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) || (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3))
		{
			GET_OBJ_VAL(obj, 3) = 0;
			to_char = "$o стал$G вполне пригодным к применению.";
		}
		break;
	case SPELL_FLY:
//		obj->timed_spell.add(obj, SPELL_FLY, 60 * 24 * 3);
		obj->timed_spell.add(obj, SPELL_FLY, -1);
		SET_BIT(GET_OBJ_EXTRA(obj, ITEM_FLYING), ITEM_FLYING);
		//В связи с тем, что летающие вещи более не тонут, флаг плавает тут неуместен
		//SET_BIT(GET_OBJ_EXTRA(obj, ITEM_SWIMMING), ITEM_SWIMMING);
		to_char = "$o вспыхнул$G зеленоватым светом и тут же погас$Q.";
		break;
	case SPELL_ACID:
		alterate_object(obj, number(GET_LEVEL(ch) * 2, GET_LEVEL(ch) * 4), 100);
		break;
	case SPELL_REPAIR:
		GET_OBJ_CUR(obj) = GET_OBJ_MAX(obj);
		to_char = "Вы полностью восстановили $o3.";
		break;
	case SPELL_TIMER_REPAIR:
		if (GET_OBJ_RNUM(obj) != NOTHING)
		{
			GET_OBJ_CUR(obj) = GET_OBJ_MAX(obj);
			obj->set_timer(obj_proto.at(GET_OBJ_RNUM(obj))->get_timer());
			to_char = "Вы полностью восстановили $o3.";
			log("%s used magic repair", GET_NAME(ch));
		}
		else
		{
			to_char = "Этот предмет нельзя восстановить.";
			return 0;
		}
		break;
	case SPELL_LIGHT:
		obj->timed_spell.add(obj, SPELL_LIGHT, -1);
		SET_BIT(GET_OBJ_EXTRA(obj, ITEM_GLOW), ITEM_GLOW);
		to_char = "$o засветил$U ровным зеленоватым светом.";
		break;
	case SPELL_DARKNESS:
		if (obj->timed_spell.check_spell(SPELL_LIGHT))
		{
			obj->timed_spell.del(obj, SPELL_LIGHT, true);
			return 1;
		}
		break;
	} // switch

	if (to_char == NULL)
		send_to_char(NOEFFECT, ch);
	else
		act(to_char, TRUE, ch, obj, 0, TO_CHAR);

	if (to_room != NULL)
		act(to_room, TRUE, ch, obj, 0, TO_ROOM | TO_ARENA_LISTEN);

	return 1;
}


int mag_creations(int level, CHAR_DATA * ch, int spellnum)
{
	OBJ_DATA *tobj;
	obj_vnum z;

	if (ch == NULL)
		return 0;
	// level = MAX(MIN(level, LVL_IMPL), 1); - Hm, not used.

	switch (spellnum)
	{
	case SPELL_CREATE_FOOD:
		z = START_BREAD;
		break;
	case SPELL_CREATE_LIGHT:
		z = CREATE_LIGHT;
		break;
	default:
		send_to_char("Spell unimplemented, it would seem.\r\n", ch);
		return 0;
	}

	if (!(tobj = read_object(z, VIRTUAL)))
	{
		send_to_char("Что-то не видно образа для создания.\r\n", ch);
		log("SYSERR: spell_creations, spell %d, obj %d: obj not found", spellnum, z);
		return 0;
	}
	act("$n создал$g $o3.", FALSE, ch, tobj, 0, TO_ROOM | TO_ARENA_LISTEN);
	act("Вы создали $o3.", FALSE, ch, tobj, 0, TO_CHAR);
	load_otrigger(tobj);

	if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
	{
		send_to_char("Вы не сможете унести столько предметов.\r\n", ch);
		obj_to_room(tobj, IN_ROOM(ch));
		obj_decay(tobj);
	}
	else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(tobj) > CAN_CARRY_W(ch))
	{
		send_to_char("Вы не сможете унести такой вес.\r\n", ch);
		obj_to_room(tobj, IN_ROOM(ch));
		obj_decay(tobj);
	}
	else
		obj_to_char(tobj, ch);

	return 1;
}

int mag_manual(int level, CHAR_DATA * caster, CHAR_DATA * cvict, OBJ_DATA * ovict, int spellnum, int savetype)
{
	switch (spellnum)
	{
	case SPELL_GROUP_RECALL:
	case SPELL_WORD_OF_RECALL:
		MANUAL_SPELL(spell_recall);
		break;
	case SPELL_TELEPORT:
		MANUAL_SPELL(spell_teleport);
		break;
	case SPELL_EVILESS:
		MANUAL_SPELL(spell_eviless);
		break;
	case SPELL_CONTROL_WEATHER:
		MANUAL_SPELL(spell_control_weather);
		break;
	case SPELL_CREATE_WATER:
		MANUAL_SPELL(spell_create_water);
		break;
	case SPELL_LOCATE_OBJECT:
		MANUAL_SPELL(spell_locate_object);
		break;
	case SPELL_SUMMON:
		MANUAL_SPELL(spell_summon);
		break;
	case SPELL_PORTAL:
		MANUAL_SPELL(spell_portal);
		break;
	case SPELL_CREATE_WEAPON:
		MANUAL_SPELL(spell_create_weapon);
		break;
	case SPELL_RELOCATE:
		MANUAL_SPELL(spell_relocate);
		break;
//    case SPELL_TOWNPORTAL:      MANUAL_SPELL(spell_townportal);      break;
	case SPELL_CHARM:
		MANUAL_SPELL(spell_charm);
		break;
	case SPELL_ENERGY_DRAIN:
		MANUAL_SPELL(spell_energydrain);
		break;
	case SPELL_MASS_FEAR:	//Added by Niker
	case SPELL_FEAR:
		MANUAL_SPELL(spell_fear);
		break;
	case SPELL_WC_OF_FEAR:
		MANUAL_SPELL(spell_wc_of_fear);
		break;
	case SPELL_SACRIFICE:
		MANUAL_SPELL(spell_sacrifice);
		break;
	case SPELL_IDENTIFY:
		MANUAL_SPELL(spell_identify);
		break;
	case SPELL_HOLYSTRIKE:
		MANUAL_SPELL(spell_holystrike);
		break;
	case SPELL_ANGEL:
		MANUAL_SPELL(spell_angel);
		break;
	default:
		return 0;
	}
	return 1;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************

// Применение заклинания к одной цели
//---------------------------------------------------------
int mag_single_target(int level, CHAR_DATA * caster, CHAR_DATA * cvict, OBJ_DATA * ovict, int spellnum, int savetype)
{
	//туповато конечно, но подобные проверки тут как счупалцьа перепутаны
	//и чтоб сделать по человечески надо треть пары модулей перелопатить
	if (cvict && IS_GOD(cvict) && (IS_NPC(caster) || GET_LEVEL(cvict) > GET_LEVEL(caster)))
	{
		send_to_char(NOEFFECT, caster);
		return (-1);
	}

	if (IS_SET(SpINFO.routines, MAG_WARCRY) && cvict && IS_UNDEAD(cvict))
		return 1;

	if (IS_SET(SpINFO.routines, MAG_DAMAGE))
		if (mag_damage(level, caster, cvict, spellnum, savetype) == -1)
			return (-1);	// Successful and target died, don't cast again.

	if (IS_SET(SpINFO.routines, MAG_AFFECTS))
		mag_affects(level, caster, cvict, spellnum, savetype);

	if (IS_SET(SpINFO.routines, MAG_UNAFFECTS))
		mag_unaffects(level, caster, cvict, spellnum, savetype);

	if (IS_SET(SpINFO.routines, MAG_POINTS))
		mag_points(level, caster, cvict, spellnum, savetype);

	if (IS_SET(SpINFO.routines, MAG_ALTER_OBJS))
		mag_alter_objs(level, caster, ovict, spellnum, savetype);

	if (IS_SET(SpINFO.routines, MAG_SUMMONS))
		mag_summons(level, caster, ovict, spellnum, 1);	// savetype =1 -- ВРЕМЕННО, показатель что фэйлить надо

	if (IS_SET(SpINFO.routines, MAG_CREATIONS))
		mag_creations(level, caster, spellnum);

	if (IS_SET(SpINFO.routines, MAG_MANUAL))
		mag_manual(level, caster, cvict, ovict, spellnum, savetype);

	cast_reaction(cvict, caster, spellnum);
	return 1;
}

typedef struct
{
	int spell;
	const char *to_char;
	const char *to_room;
	const char *to_vict;
	int decay;
} spl_message;


const spl_message masses_messages[] =
{
	{SPELL_MASS_BLINDNESS,
	 "У вас над головой возникла яркая вспышка, которая ослепила все живое.",
	 "Вдруг над головой $n1 возникла яркая вспышка.",
	 "Вы невольно взглянули на вспышку света, вызванную $n4, и ваши глаза заслезились.",
	 0},
	{SPELL_MASS_HOLD,
	 "Вы сжали зубы от боли, когда из вашего тела вырвалось множество невидимых каменных лучей.",
	 NULL,
	 "В вас попал каменный луч, исходящий от $n1.",
	 0},
	{SPELL_MASS_CURSE,
	 "Медленно оглянувшись, вы прошептали древние слова.",
	 NULL,
	 "$n злобно посмотрел$g на вас и начал$g шептать древние слова.",
	 0},
	{SPELL_MASS_SIELENCE,
	 "Поведя вокруг грозным взглядом, вы заставили всех замолчать.",
	 NULL,
	 "Вы встретились взглядом с $n4, и у вас появилось ощущение, что горлу чего-то не хватает.",
	 0},
	{SPELL_MASS_DEAFNESS,
	 "Вы нахмурились, склонив голову, и громкий хлопок сотряс воздух.",
	 "Как только $n0 склонил$g голову, раздался оглушающий хлопок.",
	 NULL,
	 0},
	{SPELL_MASS_SLOW,
	 "Положив ладони на землю, вы вызвали цепкие корни,\r\nопутавшие существ, стоящих рядом с вами.",
	 NULL,
	 "$n вызвал$g цепкие корни, опутавшие ваши ноги.",
	 0},
	{SPELL_ARMAGEDDON,
	 "Вы сплели руки в замысловатом жесте, и все потускнело!",
	 "$n сплел$g руки в замысловатом жесте, и все потускнело!",
	 NULL,
	 0},
	{SPELL_EARTHQUAKE,
	 "Вы опустили руки, и земля начала дрожать вокруг вас!",
	 "$n опустил$g руки, и земля задрожала!",
	 NULL,
	 0},
	{SPELL_METEORSTORM,
	 "Вы воздели руки к небу, и огромные глыбы посыпались с небес!",
	 "$n воздел$g руки к небу, и огромные глыбы посыпались с небес!",
	 NULL,
	 0},
	{SPELL_FIREBLAST,
	 "Вы потерли ладони, и на них вспыхнуло яркое пламя!",
	 "На ладонях $n1 вспыхнуло яркое пламя!",
	 NULL,
	 0},
	{SPELL_ICESTORM,
	 "Вы воздели руки к небу, и тысячи мелких льдинок хлынули вниз!",
	 "$n воздел$g руки к небу, и тысячи мелких льдинок хлынули вниз!",
	 NULL,
	 0},
	{SPELL_DUSTSTORM,
	 "Вы взмахнули руками и вызвали огромное пылевое облако,\r\nскрывшее все вокруг.",
	 "Вас поглотила пылевая буря, вызванная $n4.",
	 NULL,
	 0},
	{SPELL_MASS_FEAR,
	 "Вы оглядели комнату устрашающим взглядом, заставив всех содрогнуться.",
	 "$n0 оглядел$g комнату устрашающим взглядом.",  //Added by Niker
	 NULL,
	 0},
	{SPELL_GLITTERDUST,
	 "Вы слегка прищелкнули пальцами, и вокруг сгустилось облако блестящей пыли.",
	 "$n0 сотворил$g облако блестящей пыли, медленно осевшее на землю.",
	 NULL,
	 0},
	{ -1, 0, 0, 0, 0}
};

// наколенный список чаров для масс-заклов, бьющих по комнате
// в необходимость самого списка не вникал, но данная конструкция над ним
// нужна потому, что в случае смерти чара при проходе по уже сформированному
// списку - за ним могут спуржиться и клоны например, которые тоже в этот
// список попали, после чего имеем креш, т.к. бьем по невалидным указателям
typedef std::vector<CHAR_DATA *>  AreaCharListType;
AreaCharListType tmp_char_list;

void add_to_tmp_char_list(CHAR_DATA *ch)
{
	std::vector<CHAR_DATA *>::iterator it = std::find(tmp_char_list.begin(), tmp_char_list.end(), ch);
	if (it == tmp_char_list.end())
		tmp_char_list.push_back(ch);
}

void delete_from_tmp_char_list(CHAR_DATA *ch)
{
	if (tmp_char_list.empty()) return;

	std::vector<CHAR_DATA *>::iterator it = std::find(tmp_char_list.begin(), tmp_char_list.end(), ch);
	if (it != tmp_char_list.end())
		*it = 0;
}

// Применение заклинания к всем существам в комнате
//---------------------------------------------------------
int mag_masses(int level, CHAR_DATA * ch, ROOM_DATA * room, int spellnum, int savetype)
{
	int i, k;
	CHAR_DATA *ch_vict;
	const char *msg;

	if (ch == NULL)
		return 0;

	for (i = 0; masses_messages[i].spell != -1; ++i)
		if (masses_messages[i].spell == spellnum)
			break;

	if (masses_messages[i].spell == -1)
		return 0;

	if (world[IN_ROOM(ch)] == room)	 // Давим вывод если чар не в той же комнате
	{
		if (multi_cast_say(ch))
		{
			if ((msg = masses_messages[i].to_char) != NULL)
				act(msg, FALSE, ch, 0, 0, TO_CHAR);
			if ((msg = masses_messages[i].to_room) != NULL)
				act(msg, FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
		}
	};

	k = i;

	tmp_char_list.clear();
	for (i = 0, ch_vict = room->people; ch_vict; ch_vict = ch_vict->next_in_room)
	{
		if (IS_IMMORTAL(ch_vict))
			continue;
		if (!HERE(ch_vict))
			continue;
		if (SpINFO.violent && same_group(ch, ch_vict))
			continue;
		add_to_tmp_char_list(ch_vict);
	}

	// наколенная (в прямом смысле этого слова, даже стола нет)
	// версия снижения каста при масс-кастах на чаров, по 9% за каждого игрока
	const int atacker_cast = GET_CAST_SUCCESS(ch);
	int targets_count = 0;
	for (AreaCharListType::const_iterator it = tmp_char_list.begin(); it != tmp_char_list.end(); ++it)
	{
		ch_vict = *it;
		if (!ch_vict || IN_ROOM(ch) == NOWHERE || IN_ROOM(ch_vict) == NOWHERE)
		{
			continue;
		}
		if ((msg = masses_messages[k].to_vict) != NULL && ch_vict->desc)
		{
			act(msg, FALSE, ch, 0, ch_vict, TO_VICT);
		}
		if (!IS_NPC(ch) && !IS_NPC(ch_vict))
		{
			++targets_count;
		}
		mag_single_target(level, ch, ch_vict, NULL, spellnum, savetype);
		if (ch->purged())
		{
			return 1;
		}
		GET_CAST_SUCCESS(ch) = atacker_cast - atacker_cast * targets_count * 9 / 100;
	}
	GET_CAST_SUCCESS(ch) = atacker_cast;

	return 1;
}

const spl_message areas_messages[] =
{
	{SPELL_CHAIN_LIGHTNING,
	 "Вы подняли руки к небу и оно осветилось яркими вспышками!",
	 "$n поднял$g руки к небу и оно осветилось яркими вспышками!",
	 NULL,
	 5},
	{SPELL_EARTHFALL,
	 "Вы высоко подбросили комок земли и он, увеличиваясь на глазах, обрушился вниз.",
	 "$n высоко подбросил$g комок земли, который, увеличиваясь на глазах, стал падать вниз.",
	 NULL,
	 8},
	{SPELL_SONICWAVE,
	 "Вы слегка хлопнули в ладоши и во все стороны побежала воздушная волна,\r\nсокрушающая все на своем пути.",
	 "Негромкий хлопок $n1 породил воздушную волну, сокрушающую все на своем пути.",
	 NULL,
	 3},
	{SPELL_SHOCK,
	 "Яркая вспышка слетела с кончиков ваших пальцев и с оглушительным грохотом взорвалась в воздухе.",
	 "Выпущенная $n1 яркая вспышка с оглушительным грохотом взорвалась в воздухе.",
	 NULL,
	 8},
	{SPELL_BURDEN_OF_TIME,
	 "Вы скрестили руки на груди, вызвав яркую вспышку синего света.",
	 "$n0 скрестил$g руки на груди, вызвав яркую вспышку синего света.",
	 NULL,
	 8},
	{SPELL_FAILURE,
	 "Вы простерли руки над головой, вызвав череду раскатов грома.",
	 "$n0 вызвал$g череду раскатов грома, заставивших все вокруг содрогнуться.",
	 NULL,
	 7},
	{SPELL_SCREAM,
	 "Вы испустили кошмарный вопль, едва не разорвавший вам горло.",
	 "$n0 испустил$g кошмарный вопль, отдавшийся в вашей душе замогильным холодом.",
	 NULL,
	 5},
	{SPELL_WC_OF_CHALLENGE,
	 NULL,
	 "Вы не стерпели насмешки, и бросились на $n1!",
	 NULL,
	 0},
	{SPELL_WC_OF_MENACE,
	 NULL,
	 NULL,
	 NULL,
	 0},
	{SPELL_WC_OF_RAGE,
	 NULL,
	 NULL,
	 NULL,
	 0},
	{SPELL_WC_OF_MADNESS,
	 NULL,
	 NULL,
	 NULL,
	 0},
	{SPELL_WC_OF_THUNDER,
	 NULL,
	 NULL,
	 NULL,
	 0},
	{SPELL_WC_OF_FEAR,
	 NULL,
	 NULL,
	 NULL,
	 0},
	{ -1, 0, 0, 0, 0}
};

// Применение заклинания к части существ в комнате
//---------------------------------------------------------
int mag_areas(int level, CHAR_DATA * ch, CHAR_DATA * victim, int spellnum, int savetype)
{
	int i, decay;
	CHAR_DATA *ch_vict;
	const char *msg;

	if (!ch || !victim)
		return 0;

	for (i = 0; areas_messages[i].spell != -1; ++i)
		if (areas_messages[i].spell == spellnum)
			break;

	if (areas_messages[i].spell == -1)
		return 0;

	if (IN_ROOM(ch) == IN_ROOM(victim)) // Подавляем вывод если кастер не в комнате
	{
		if (multi_cast_say(ch))
		{
			if ((msg = areas_messages[i].to_char) != NULL)
				act(msg, FALSE, ch, 0, victim, TO_CHAR);
			if ((msg = areas_messages[i].to_room) != NULL)
				act(msg, FALSE, ch, 0, victim, TO_ROOM | TO_ARENA_LISTEN);
		}
	}
	decay = areas_messages[i].decay;

	// список генерится до дамага по виктиму, т.к. на нем могут висеть death тригеры
	// с появлением новых мобов, по которым тот же шок бьет уже после смерти основной цели
	tmp_char_list.clear();
	for (i = 0, ch_vict = world[ch->in_room]->people; ch_vict; ch_vict = ch_vict->next_in_room)
	{
		if (IS_IMMORTAL(ch_vict))
			continue;
		if (!HERE(ch_vict))
			continue;
		if (ch_vict == victim)
			continue;
		if (SpINFO.violent && same_group(ch, ch_vict))
			continue;
		add_to_tmp_char_list(ch_vict);
	}

	mag_single_target(level, ch, victim, NULL, spellnum, savetype);
	if (ch->purged())
	{
		return 1;
	}

	level -= decay;

	// у шока после первой цели - рандом на остальные две цели
	int max_targets = 0;
	if (spellnum == SPELL_SHOCK)
	{
		max_targets = number(0, 2);
		if (max_targets == 0)
		{
			return 1;
		}
	}

	int size = tmp_char_list.size();
	int count = 0;
	while (level > 0 && level >= decay && size != 0)
	{
		if (max_targets > 0 && count >= max_targets)
		{
			break;
		}
		i = number(0, size - 1);
		ch_vict = tmp_char_list[i];
		tmp_char_list[i] = tmp_char_list[--size];
		if (!ch_vict || IN_ROOM(ch) == NOWHERE || IN_ROOM(ch_vict) == NOWHERE)
		{
			continue;
		}
		mag_single_target(level, ch, ch_vict, NULL, spellnum, savetype);
		if (ch->purged())
		{
			break;
		}
		level -= decay;
		++count;
	}

	return 1;
}

const spl_message groups_messages[] =
{
	{SPELL_GROUP_HEAL,
	 "Вы подняли голову вверх и ощутили яркий свет, ласково бегущий по вашему телу.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_ARMOR,
	 "Вы создали защитную сферу, которая окутала вас и пространство рядом с вами.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_RECALL,
	 "Вы выкрикнули заклинание и хлопнули в ладоши.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_STRENGTH,
	 "Вы призвали мощь Вселенной.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_BLESS,
	 "Прикрыв глаза, вы прошептали таинственную молитву.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_HASTE,
	 "Разведя руки в стороны, вы ощутили всю мощь стихии ветра.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_FLY,
	 "Ваше заклинание вызвало белое облако, которое разделилось, подхватывая вас и товарищей.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_INVISIBLE,
	 "Вы вызвали прозрачный туман, поглотивший все дружественное вам.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_MAGICGLASS,
	 "Вы произнесли несколько резких слов, и все вокруг засеребрилось.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_SANCTUARY,
	 "Вы подняли руки к небу и произнесли священную молитву.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_PRISMATICAURA,
	 "Силы духа, призванные вами, окутали вас и окружающих голубоватым сиянием.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_FIRE_AURA,
	 "Силы огня пришли к вам на помощь и защитили вас.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_AIR_AURA,
	 "Силы воздуха пришли к вам на помощь и защитили вас.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_ICE_AURA,
	 "Силы холода пришли к вам на помощь и защитили вас.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_GROUP_REFRESH,
	 "Ваша магия наполнила воздух зеленоватым сиянием.\r\n",
	 NULL,
	 NULL,
	 0},
	{SPELL_WC_OF_BATTLE,
	 NULL,
	 NULL,
	 NULL,
	 0},
	{SPELL_WC_OF_POWER,
	 NULL,
	 NULL,
	 NULL,
	 0},
	{SPELL_WC_OF_BLESS,
	 NULL,
	 NULL,
	 NULL,
	 0},
	{SPELL_WC_OF_COURAGE,
	 NULL,
	 NULL,
	 NULL,
	 0},
	{ -1, 0, 0, 0, 0 }
};

// Применение заклинания к группе в комнате
//---------------------------------------------------------
int mag_groups(int level, CHAR_DATA * ch, int spellnum, int savetype)
{
	int i;
	CHAR_DATA *ch_vict;
	const char *msg;

	if (ch == NULL)
		return 0;

	for (i = 0; groups_messages[i].spell != -1; ++i)
		if (groups_messages[i].spell == spellnum)
			break;

	if (groups_messages[i].spell == -1)
		return 0;

	if (multi_cast_say(ch))
	{
		if ((msg = groups_messages[i].to_char) != NULL)
			act(msg, FALSE, ch, 0, 0, TO_CHAR);
		if ((msg = groups_messages[i].to_room) != NULL)
			act(msg, FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	}

	tmp_char_list.clear();
	for (i = 0, ch_vict = world[ch->in_room]->people; ch_vict; ch_vict = ch_vict->next_in_room)
	{
		if (!HERE(ch_vict))
			continue;
		if (!same_group(ch, ch_vict))
			continue;
		add_to_tmp_char_list(ch_vict);
	}

	for (AreaCharListType::const_iterator it = tmp_char_list.begin(); it != tmp_char_list.end(); ++it)
	{
		ch_vict = *it;
		if (!ch_vict || IN_ROOM(ch) == NOWHERE || IN_ROOM(ch_vict) == NOWHERE)
		{
			continue;
		}
		mag_single_target(level, ch, ch_vict, NULL, spellnum, savetype);
		if (ch->purged())
		{
			return 1;
		}
	}

	return 1;
}

//Функция определяет какой резист для какого типа спелла следует брать.
//Работает только если каждый спелл имеет 1 тип

int get_resist_type(int spellnum)
{
	if (SpINFO.spell_class == STYPE_FIRE)
	{
		return FIRE_RESISTANCE;
	}
	if (SpINFO.spell_class == STYPE_AIR)
	{
		return AIR_RESISTANCE;
	}
	if (SpINFO.spell_class == STYPE_WATER)
	{
		return WATER_RESISTANCE;
	}
	if (SpINFO.spell_class == STYPE_EARTH)
	{
		return EARTH_RESISTANCE;
	}
	if (SpINFO.spell_class == STYPE_LIGHT)
	{
		return VITALITY_RESISTANCE;
	}
	if (SpINFO.spell_class == STYPE_DARK)
	{
		return VITALITY_RESISTANCE;
	}
	if (SpINFO.spell_class == STYPE_MIND)
	{
		return MIND_RESISTANCE;
	}
	if (SpINFO.spell_class == STYPE_LIFE)
	{
		return IMMUNITY_RESISTANCE;
	}
	if (SpINFO.spell_class == STYPE_NEUTRAL)
	{
		return VITALITY_RESISTANCE;
	}
	log("SYSERR: Unknown spell type in %s", SpINFO.name);
	return 0;
}
