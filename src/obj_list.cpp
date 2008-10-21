// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include <list>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include "obj_list.hpp"
#include "utils.h"
#include "char.hpp"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "dg_scripts.h"
#include "house.h"
#include "screen.h"
#include "depot.hpp"
#include "olc.h"

extern void print_object_location(int num, OBJ_DATA *obj, CHAR_DATA *ch, int recur);
extern struct zone_data *zone_table;
extern void oedit_object_free(OBJ_DATA * obj);

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{

typedef std::list<OBJ_DATA *> ObjListType;
ObjListType obj_list;

/**
* Оповещение о дикее шмотки из храна в клан-канал.
*/
void clan_chest_invoice(OBJ_DATA *j)
{
	int room = GET_ROOM_VNUM(j->in_obj->in_room);
	for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
	{
		if (d->character
			&& STATE(d) == CON_PLAYING
			&& !AFF_FLAGGED(d->character, AFF_DEAFNESS)
			&& CLAN(d->character)
			&& PRF_FLAGGED(d->character, PRF_DECAY_MODE)
			&& world[real_room(CLAN(d->character)->GetRent())]->zone == world[real_room(room)]->zone)
		{
			send_to_char(d->character, "[Хранилище]: %s'%s рассыпал%s в прах'%s\r\n",
				CCIRED(d->character, C_NRM), j->short_description, GET_OBJ_SUF_2(j), CCNRM(d->character, C_NRM));
		}
	}
}

/**
* Дикей шмоток в клан-хране.
*/
void clan_chest_point_update(OBJ_DATA *j)
{
	if (GET_OBJ_TIMER(j) > 0)
		GET_OBJ_TIMER(j)--;

	if ((OBJ_FLAGGED(j, ITEM_ZONEDECAY)
			&& GET_OBJ_ZONE(j) != NOWHERE
			&& up_obj_where(j->in_obj) != NOWHERE
			&& GET_OBJ_ZONE(j) != world[up_obj_where(j->in_obj)]->zone)
		|| GET_OBJ_TIMER(j) <= 0)
	{
		clan_chest_invoice(j);
		obj_from_obj(j);
		extract_obj(j);
	}
}

/* Update PCs, NPCs, and objects */
#define NO_DESTROY(obj) ((obj)->carried_by   || \
                         (obj)->worn_by      || \
                         (obj)->in_obj       || \
                         (obj)->script       || \
                         GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN || \
	                 obj->in_room == NOWHERE || \
			 (OBJ_FLAGGED(obj, ITEM_NODECAY) && !ROOM_FLAGGED(obj->in_room, ROOM_DEATH)))
#define NO_TIMER(obj)   (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN)
/* || OBJ_FLAGGED(obj, ITEM_NODECAY)) */

void check_decay()
{
	ObjListType::iterator next_it;
	OBJ_DATA *obj;

	for (ObjListType::iterator it = obj_list.begin(); it != obj_list.end(); it = next_it)
	{
		obj = *it;
		next_it = ++it;

		bool cont = obj->contains ? true : false;
		if (obj_decay(obj) && cont)
			next_it = obj_list.begin();
	}
}

} // namespace
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ObjList
{

void add(OBJ_DATA *obj)
{
	obj_list.push_front(obj);
	obj->it_ptr.reset(new std::list<OBJ_DATA *>::iterator);
	*(obj->it_ptr) = obj_list.begin();
}

// здесь могут быть шмотки вне списка, при экстракте из перс-хранилищ например
void remove(OBJ_DATA *obj)
{
	if (obj->it_ptr)
	{
		obj_list.erase(*(obj->it_ptr));
		obj->it_ptr.reset();
	}
}

void print_god_where(CHAR_DATA *ch, char const *arg)
{
	int num = 0;
	bool found = false;
	for (ObjListType::const_iterator it = obj_list.begin(); it != obj_list.end(); ++it)
	{
		if (CAN_SEE_OBJ(ch, (*it)) && isname(arg, (*it)->name))
		{
			found = true;
			print_object_location(++num, *it, ch, true);
		}
	}
	if (!found)
		send_to_char("Нет ничего похожего.\r\n", ch);
}

void paste()
{
	ObjListType::iterator next_it;
	OBJ_DATA *obj;

	for (ObjListType::iterator it = obj_list.begin(); it != obj_list.end(); it = next_it)
	{
		obj = *it;
		next_it = ++it;

		paste_obj(obj, IN_ROOM(obj));
	}
}

size_t size()
{
	return obj_list.size();
}

OBJ_DATA * obj_by_id(int id)
{
	ObjListType::const_iterator it = std::find_if(obj_list.begin(), obj_list.end(),
			boost::bind(std::equal_to<int>(),
			boost::bind(&OBJ_DATA::id, _1), id));

	return it != obj_list.end() ? *it : 0;
}

OBJ_DATA * obj_by_rnum(obj_rnum nr)
{
	ObjListType::const_iterator it = std::find_if(obj_list.begin(), obj_list.end(),
			boost::bind(std::equal_to<int>(),
			boost::bind(&OBJ_DATA::item_number, _1), nr));

	return it != obj_list.end() ? *it : 0;
}

OBJ_DATA * obj_by_name(char const *name)
{
	for (ObjListType::const_iterator it = obj_list.begin(); it != obj_list.end(); ++it)
	{
		if (isname(name, (*it)->name))
			return *it;
	}
	return 0;
}

int id_by_vnum(int vnum)
{
	for (ObjListType::const_iterator it = obj_list.begin(); it != obj_list.end(); ++it)
	{
		if (GET_OBJ_VNUM(*it) == vnum)
			return GET_ID(*it);
	}
	return -1;
}

void check_script()
{
	for (ObjListType::const_iterator it = obj_list.begin(); it != obj_list.end(); ++it)
	{
		if (SCRIPT((*it)))
		{
			SCRIPT_DATA *sc = SCRIPT((*it));
			if (IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM))
				random_otrigger((*it));
		}
	}
}

int count_dupes(OBJ_DATA *obj)
{
	int count = 0;
	for (ObjListType::const_iterator it = obj_list.begin(); it != obj_list.end(); ++it)
	{
		if (GET_OBJ_UID((*it)) == GET_OBJ_UID(obj)
			&& GET_OBJ_TIMER((*it)) > 0
			&& GET_OBJ_VNUM((*it)) == GET_OBJ_VNUM(obj))
		{
			++count;
		}
	}
	return count;
}

OBJ_DATA * obj_by_nname(CHAR_DATA *ch, char const *name, int number)
{
	int i = 0;
	OBJ_DATA *obj = 0;
	for (ObjListType::const_iterator it = obj_list.begin(); it != obj_list.end() && i <= number; ++it)
	{
		if (isname(name, (*it)->name) && CAN_SEE_OBJ(ch, (*it)) && ++i == number)
			obj = *it;
	}
	return obj;
}

void point_update()
{
	OBJ_DATA *jj, *next_thing2;
	int count;

	ObjListType::iterator next_it;
	OBJ_DATA *j;

	for (ObjListType::iterator it = obj_list.begin(); it != obj_list.end(); it = next_it)
	{
		j = *it;
		next_it = ++it;

		// смотрим клан-сундуки
		if (j->in_obj && Clan::is_clan_chest(j->in_obj))
		{
			clan_chest_point_update(j);
			continue;
		}

		// контейнеры на земле с флагом !дикей, но не загружаемые в этой комнате, а хз кем брошенные
		// извращение конечно перебирать на каждый объект команды резета зоны, но в голову ниче интересного
		// не лезет, да и не так уж и много на самом деле таких предметов будет, условий порядочно
		// а так привет любителям оставлять книги в клановых сумках или лоадить в замке столы
		if (j->in_obj
				&& !j->in_obj->carried_by
				&& !j->in_obj->worn_by
				&& OBJ_FLAGGED(j->in_obj, ITEM_NODECAY)
				&& GET_ROOM_VNUM(IN_ROOM(j->in_obj)) % 100 != 99)
		{
			int zone = world[j->in_obj->in_room]->zone;
			bool find = 0;
			ClanListType::const_iterator clan = Clan::IsClanRoom(j->in_obj->in_room);
			if (clan == Clan::ClanList.end())   // внутри замков даже и смотреть не будем
			{
				for (int cmd_no = 0; zone_table[zone].cmd[cmd_no].command != 'S'; ++cmd_no)
				{
					if (zone_table[zone].cmd[cmd_no].command == 'O'
							&& zone_table[zone].cmd[cmd_no].arg1 == GET_OBJ_RNUM(j->in_obj)
							&& zone_table[zone].cmd[cmd_no].arg3 == IN_ROOM(j->in_obj))
					{
						find = 1;
						break;
					}
				}
			}

			if (!find && GET_OBJ_TIMER(j) > 0)
				GET_OBJ_TIMER(j)--;
		}

		/* If this is a corpse */
		if (IS_CORPSE(j))  	/* timer count down */
		{
			if (GET_OBJ_TIMER(j) > 0)
				GET_OBJ_TIMER(j)--;
			if (GET_OBJ_TIMER(j) <= 0)
			{
				for (jj = j->contains; jj; jj = next_thing2)
				{
					next_thing2 = jj->next_content;	/* Next in inventory */
					obj_from_obj(jj);
					if (j->in_obj)
						obj_to_obj(jj, j->in_obj);
					else if (j->carried_by)
						obj_to_char(jj, j->carried_by);
					else if (j->in_room != NOWHERE)
						obj_to_room(jj, j->in_room);
					else
					{
						log("SYSERR: extract %s from %s to NOTHING !!!",
							jj->PNames[0], j->PNames[0]);
						// core_dump();
						extract_obj(jj);
					}
				}
				// Добавлено Ладником
//              next_thing = j->next; /* пурж по obj_to_room я убрал, но пускай на всякий случай */
				// Конец Ладник
				if (j->carried_by)
				{
					act("$p рассыпал$U в Ваших руках.", FALSE, j->carried_by, j, 0, TO_CHAR);
					obj_from_char(j);
				}
				else if (j->in_room != NOWHERE)
				{
					if (world[j->in_room]->people)
					{
						act("Черви полностью сожрали $o3.",
							TRUE, world[j->in_room]->people, j, 0, TO_ROOM);
						act("Черви не оставили от $o1 и следа.",
							TRUE, world[j->in_room]->people, j, 0, TO_CHAR);
					}
					obj_from_room(j);
				}
				else if (j->in_obj)
					obj_from_obj(j);
				extract_obj(j);
			}
		}
		/* If the timer is set, count it down and at 0, try the trigger */
		/* note to .rej hand-patchers: make this last in your point-update() */
		else
		{
			if (SCRIPT_CHECK(j, OTRIG_TIMER))
			{
				if (GET_OBJ_TIMER(j) > 0 && OBJ_FLAGGED(j, ITEM_TICKTIMER))
					GET_OBJ_TIMER(j)--;
				if (!GET_OBJ_TIMER(j))
				{
					timer_otrigger(j);
					j = NULL;
				}
			}
			else if (GET_OBJ_DESTROY(j) > 0 && !NO_DESTROY(j))
				GET_OBJ_DESTROY(j)--;

			if (j && (j->in_room != NOWHERE) && GET_OBJ_TIMER(j) > 0 && !NO_DESTROY(j))
				GET_OBJ_TIMER(j)--;

			if (j && ((OBJ_FLAGGED(j, ITEM_ZONEDECAY) && GET_OBJ_ZONE(j) != NOWHERE && up_obj_where(j) != NOWHERE && GET_OBJ_ZONE(j) != world[up_obj_where(j)]->zone) || (GET_OBJ_TIMER(j) <= 0 && !NO_TIMER(j)) || (GET_OBJ_DESTROY(j) == 0 && !NO_DESTROY(j))))
			{
				/**** рассыпание обьекта */
				for (jj = j->contains; jj; jj = next_thing2)
				{
					next_thing2 = jj->next_content;
					obj_from_obj(jj);
					if (j->in_obj)
						obj_to_obj(jj, j->in_obj);
					else if (j->worn_by)
						obj_to_char(jj, j->worn_by);
					else if (j->carried_by)
						obj_to_char(jj, j->carried_by);
					else if (j->in_room != NOWHERE)
						obj_to_room(jj, j->in_room);
					else
					{
						log("SYSERR: extract %s from %s to NOTHING !!!",
							jj->PNames[0], j->PNames[0]);
						// core_dump();
						extract_obj(jj);
					}
				}
				// Добавлено Ладником
//              next_thing = j->next; /* пурж по obj_to_room я убрал, но пускай на всякий случай */
				// Конец Ладник
				if (j->worn_by)
				{
					switch (j->worn_on)
					{
					case WEAR_LIGHT:
					case WEAR_SHIELD:
					case WEAR_WIELD:
					case WEAR_HOLD:
					case WEAR_BOTHS:
						act("$o рассыпал$U в Ваших руках...", FALSE, j->worn_by, j, 0, TO_CHAR);
						break;
					default:
						act("$o рассыпал$U прямо на Вас...", FALSE, j->worn_by, j, 0, TO_CHAR);
						break;
					}
					unequip_char(j->worn_by, j->worn_on);
				}
				else if (j->carried_by)
				{
					act("$o рассыпал$U в Ваших руках...", FALSE, j->carried_by, j, 0, TO_CHAR);
					obj_from_char(j);
				}
				else if (j->in_room != NOWHERE)
				{
					if (world[j->in_room]->people)
					{
						act("$o рассыпал$U в прах, который был развеян ветром...",
							FALSE, world[j->in_room]->people, j, 0, TO_CHAR);
						act("$o рассыпал$U в прах, который был развеян ветром...",
							FALSE, world[j->in_room]->people, j, 0, TO_ROOM);
					}
					obj_from_room(j);
				}
				else if (j->in_obj)
					obj_from_obj(j);
				extract_obj(j);
			}
			else
			{
				if (!j)
					continue;

				/* decay poision && other affects */
				for (count = 0; count < MAX_OBJ_AFFECT; count++)
					if (j->affected[count].location == APPLY_POISON)
					{
						j->affected[count].modifier--;
						if (j->affected[count].modifier <= 0)
						{
							j->affected[count].location = APPLY_NONE;
							j->affected[count].modifier = 0;
						}
					}
			}
		}
	}

	/* Тонущие, падающие, и сыпящиеся обьекты. */
	check_decay();
}

void repop_decay(zone_rnum zone)
{
	/* рассыпание обьектов ITEM_REPOP_DECAY */
	int cont = FALSE;
	zone_vnum obj_zone_num, zone_num;

	zone_num = zone_table[zone].number;

	ObjListType::iterator next_it;
	OBJ_DATA *j;

	for (ObjListType::iterator it = obj_list.begin(); it != obj_list.end(); it = next_it)
	{
		j = *it;
		next_it = ++it;

		if (j->contains)
		{
			cont = TRUE;
		}
		else
		{
			cont = FALSE;
		}
		obj_zone_num = GET_OBJ_VNUM(j) / 100;
		if (((obj_zone_num == zone_num) && IS_OBJ_STAT(j, ITEM_REPOP_DECAY)))
		{
			/* F@N
			 * Если мне кто-нибудь объяснит глубинный смысл последующей строчки,
			 * буду очень признателен
			*/
//                 || (GET_OBJ_TYPE(j) == ITEM_INGRADIENT && GET_OBJ_SKILL(j) > 19)
			if (j->worn_by)
				act("$o рассыпал$U, вспыхнув ярким светом...", FALSE, j->worn_by, j, 0, TO_CHAR);
			else if (j->carried_by)
				act("$o рассыпал$U в Ваших руках, вспыхнув ярким светом...",
					FALSE, j->carried_by, j, 0, TO_CHAR);
			else if (j->in_room != NOWHERE)
			{
				if (world[j->in_room]->people)
				{
					act("$o рассыпал$U, вспыхнув ярким светом...",
						FALSE, world[j->in_room]->people, j, 0, TO_CHAR);
					act("$o рассыпал$U, вспыхнув ярким светом...",
						FALSE, world[j->in_room]->people, j, 0, TO_ROOM);
				}
			}
			extract_obj(j);
			if (cont)
				next_it = obj_list.begin();
		}
	}
}


/**
* Пересчет рнумов объектов (больше нигде не меняются).
*/
void recalculate_rnum(int rnum)
{
	OBJ_DATA *obj;
	for (ObjListType::const_iterator it = obj_list.begin(); it != obj_list.end(); ++it)
	{
		obj = *it;
		if (GET_OBJ_RNUM(obj) >= rnum)
		{
			// пересчет персональных хранилищ
			if (GET_OBJ_RNUM(obj) == Depot::PERS_CHEST_RNUM)
				Depot::PERS_CHEST_RNUM++;
			GET_OBJ_RNUM(obj)++;
		}
	}
	Depot::renumber_obj_rnum(rnum);
	Clan::init_chest_rnum();
}

void olc_update(obj_rnum nr, DESCRIPTOR_DATA *d)
{
	OBJ_DATA *obj;
	for (ObjListType::const_iterator it = obj_list.begin(); it != obj_list.end(); ++it)
	{
		obj = *it;

		if (obj->item_number == nr)
		{
			// Итак, нашел объект
			// Внимание! Таймер объекта, его состояние и т.д. обновятся!

			// Сохраняю текущую игровую информацию
			OBJ_DATA tmp(*obj);

			// Удаляю его строки и т.д.
			// прототип скрипта не удалится, т.к. его у экземпляра нету
			// скрипт не удалится, т.к. его не удаляю
			oedit_object_free(obj);

			// Нужно скопировать все новое, сохранив определенную информацию
			*obj = *OLC_OBJ(d);
			obj->proto_script = NULL;
			// Восстанавливаю игровую информацию
			obj->in_room = tmp.in_room;
			obj->item_number = nr;
			obj->carried_by = tmp.carried_by;
			obj->worn_by = tmp.worn_by;
			obj->worn_on = tmp.worn_on;
			obj->in_obj = tmp.in_obj;
			obj->contains = tmp.contains;
			obj->next_content = tmp.next_content;
			SCRIPT(obj) = SCRIPT(&tmp);
		}
	}
}

int print_spell_locate_object(CHAR_DATA *ch, int count, char const *name)
{
	OBJ_DATA *i;

	for (ObjListType::const_iterator it = obj_list.begin(); it != obj_list.end() && count > 0; ++it)
	{
		i = *it;

		if (number(1, 100) > (40 + MAX((GET_REAL_INT(ch) - 25) * 2, 0)))
			continue;

		if (IS_CORPSE(i))
			continue;

		if (!isname(name, i->name))
			continue;

		if (SECT(IN_ROOM(i)) == SECT_SECRET)
			continue;

		if (i->carried_by)
			if (SECT(IN_ROOM(i->carried_by)) == SECT_SECRET ||
					(OBJ_FLAGGED(i, ITEM_NOLOCATE) && IS_NPC(i->carried_by)) ||
					IS_IMMORTAL(i->carried_by))
				continue;

		if (i->carried_by)
		{
			if (world[IN_ROOM(i->carried_by)]->zone == world[IN_ROOM(ch)]->zone || !IS_NPC(i->carried_by))
				sprintf(buf, "%s находится у %s в инвентаре.\r\n",
						i->short_description, PERS(i->carried_by, ch, 1));
			else
				continue;
		}
		else if (IN_ROOM(i) != NOWHERE && IN_ROOM(i))
		{
			if (world[IN_ROOM(i)]->zone == world[IN_ROOM(ch)]->zone && !OBJ_FLAGGED(i, ITEM_NOLOCATE))
				sprintf(buf, "%s находится в %s.\r\n", i->short_description, world[IN_ROOM(i)]->name);
			else
				continue;
		}
		else if (i->in_obj)
		{
			if (Clan::is_clan_chest(i->in_obj))
			{
				ClanListType::const_iterator clan = Clan::IsClanRoom(i->in_obj->in_room);
				if (clan != Clan::ClanList.end())
					sprintf(buf, "%s находится в хранилище дружины '%s'.\r\n", i->short_description, (*clan)->GetAbbrev());
				else
					continue;
			}
			else
			{
				if (i->in_obj->carried_by)
					if (IS_NPC(i->in_obj->carried_by) && (OBJ_FLAGGED(i, ITEM_NOLOCATE) || world[IN_ROOM(i->in_obj->carried_by)]->zone != world[IN_ROOM(ch)]->zone))
						continue;
				if (IN_ROOM(i->in_obj) != NOWHERE && IN_ROOM(i->in_obj))
					if (world[IN_ROOM(i->in_obj)]->zone != world[IN_ROOM(ch)]->zone || OBJ_FLAGGED(i, ITEM_NOLOCATE))
						continue;
				if (i->in_obj->worn_by)
					if (IS_NPC(i->in_obj->worn_by)
							&& (OBJ_FLAGGED(i, ITEM_NOLOCATE)
								|| world[IN_ROOM(i->in_obj->worn_by)]->zone != world[IN_ROOM(ch)]->zone))
						continue;
				sprintf(buf, "%s находится в %s.\r\n", i->short_description, i->in_obj->PNames[5]);
			}
		}
		else if (i->worn_by)
		{
			if ((IS_NPC(i->worn_by) && !OBJ_FLAGGED(i, ITEM_NOLOCATE)
					&& world[IN_ROOM(i->worn_by)]->zone == world[IN_ROOM(ch)]->zone)
					|| (!IS_NPC(i->worn_by) && GET_LEVEL(i->worn_by) < LVL_IMMORT))
				sprintf(buf, "%s одет%s на %s.\r\n", i->short_description,
						GET_OBJ_SUF_6(i), PERS(i->worn_by, ch, 3));
			else
				continue;
		}
		else
			sprintf(buf, "Местоположение %s неопределимо.\r\n", OBJN(i, ch, 1));

		CAP(buf);
		send_to_char(buf, ch);
		--count;
	}
	return count;
}

} // namespace ObjList
