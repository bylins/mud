/************************************************************************
 * OasisOLC - oedit.c						v1.5	*
 * Copyright 1996 Harvey Gilpin.					*
 * Original author: Levork						*
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "olc.h"
#include "dg_olc.h"
#include "im.h"
#include "features.hpp"
#include "depot.hpp"
#include "char.hpp"
#include "house.h"
#include "skills.h"
#include "parcel.hpp"
#include "liquid.hpp"
#include "name_list.hpp"
#include "corpse.hpp"
#include "shop_ext.hpp"
#include "constants.h"
#include "sets_drop.hpp"
#include "fight.h"

/*------------------------------------------------------------------------*/

/*
 * External variable declarations.
 */

extern vector < OBJ_DATA * >obj_proto;
extern INDEX_DATA *obj_index;
extern OBJ_DATA *object_list;
extern obj_rnum top_of_objt;
extern struct zone_data *zone_table;
extern SHOP_DATA *shop_index;
extern int top_shop;
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *drinks[];
extern const char *apply_types[];
extern const char *container_bits[];
extern const char *anti_bits[];
extern const char *no_bits[];
extern const char *weapon_affects[];
extern const char *material_name[];
extern const char *ingradient_bits[];
extern struct spell_info_type spell_info[];
extern DESCRIPTOR_DATA *descriptor_list;
extern int top_imrecipes;

int real_zone(int number);

/*------------------------------------------------------------------------*/

/*
 * Handy macros.
 */
#define S_PRODUCT(s, i) ((s)->producing[(i)])

/*------------------------------------------------------------------------*/
void oedit_setup(DESCRIPTOR_DATA * d, int real_num);

void oedit_object_copy(OBJ_DATA * dst, OBJ_DATA * src);
void oedit_object_free(OBJ_DATA * obj);

void oedit_save_internally(DESCRIPTOR_DATA * d);
void oedit_save_to_disk(int zone);

void oedit_parse(DESCRIPTOR_DATA * d, char *arg);
void oedit_disp_spells_menu(DESCRIPTOR_DATA * d);
void oedit_liquid_type(DESCRIPTOR_DATA * d);
void oedit_disp_container_flags_menu(DESCRIPTOR_DATA * d);
void oedit_disp_extradesc_menu(DESCRIPTOR_DATA * d);
void oedit_disp_weapon_menu(DESCRIPTOR_DATA * d);
void oedit_disp_val1_menu(DESCRIPTOR_DATA * d);
void oedit_disp_val2_menu(DESCRIPTOR_DATA * d);
void oedit_disp_val3_menu(DESCRIPTOR_DATA * d);
void oedit_disp_val4_menu(DESCRIPTOR_DATA * d);
void oedit_disp_type_menu(DESCRIPTOR_DATA * d);
void oedit_disp_extra_menu(DESCRIPTOR_DATA * d);
void oedit_disp_wear_menu(DESCRIPTOR_DATA * d);
void oedit_disp_menu(DESCRIPTOR_DATA * d);
void oedit_disp_skills_menu(DESCRIPTOR_DATA * d);
void oedit_disp_receipts_menu(DESCRIPTOR_DATA * d);
void oedit_disp_feats_menu(DESCRIPTOR_DATA * d);
void oedit_disp_skills_mod_menu(DESCRIPTOR_DATA* d);

/*------------------------------------------------------------------------*\
  Utility and exported functions
\*------------------------------------------------------------------------*/

void oedit_object_copy(OBJ_DATA * dst, OBJ_DATA * src)
/*++
   Функция делает создает копию объекта.
   После вызова этой функции создается полностью независимая копия объекта src.
   Все поля имеют те же значения, но занимают свои области памяти.
      dst - "чистый" указатель на структуру OBJ_DATA.
      src - исходный объект
   Примечание: Неочищенный указатель dst приведет к утечке памяти.
               Используйте oedit_object_free() для очистки содержимого объекта
--*/
{
	int i;
	EXTRA_DESCR_DATA **pddd, *sdd;

	// Копирую все поверх
	*dst = *src;

	// Теперь нужно выделить собственные области памяти

	// Имена и названия
	dst->name = str_dup(src->name ? src->name : "нет");
	dst->short_description = str_dup(src->short_description ? src->short_description : "неопределено");
	dst->description = str_dup(src->description ? src->description : "неопределено");
	dst->action_description = (src->action_description ? str_dup(src->action_description) : NULL);
	for (i = 0; i < NUM_PADS; i++)
		GET_OBJ_PNAME(dst, i) = str_dup(GET_OBJ_PNAME(src, i) ? GET_OBJ_PNAME(src, i) : "неопределен");

	// Дополнительные описания, если есть
	pddd = &dst->ex_description;
	sdd = src->ex_description;

	while (sdd)
	{
		CREATE(pddd[0], EXTRA_DESCR_DATA, 1);
		pddd[0]->keyword = sdd->keyword ? str_dup(sdd->keyword) : NULL;
		pddd[0]->description = sdd->description ? str_dup(sdd->description) : NULL;
		pddd = &(pddd[0]->next);
		sdd = sdd->next;
	}

	// Копирую скрипт и прототипы
	SCRIPT(dst) = NULL;
	dst->proto_script = NULL;
	proto_script_copy(&dst->proto_script, src->proto_script);
}

void oedit_object_free(OBJ_DATA * obj)
/*++
   Функция полностью освобождает память, занимаемую данными объекта.
   ВНИМАНИЕ. Память самой структуры OBJ_DATA не освобождается.
             Необходимо дополнительно использовать free()
--*/
{
	int i, j;
	EXTRA_DESCR_DATA *lthis, *next;

	if (!obj)
		return;

	i = GET_OBJ_RNUM(obj);

	if (i == -1 || obj == obj_proto[i])
	{
		// Нет прототипа или сам прототип, удалять все подряд
		if (obj->name)
			free(obj->name);
		if (obj->description)
			free(obj->description);
		if (obj->short_description)
			free(obj->short_description);
		if (obj->action_description)
			free(obj->action_description);
		for (j = 0; j < NUM_PADS; j++)
			if (GET_OBJ_PNAME(obj, j))
				free(GET_OBJ_PNAME(obj, j));
		for (lthis = obj->ex_description; lthis; lthis = next)
		{
			next = lthis->next;
			if (lthis->keyword)
				free(lthis->keyword);
			if (lthis->description)
				free(lthis->description);
			free(lthis);
		}
	}
	else
	{
		// Есть прототип, удалять несовпадающее
		if (obj->name && obj->name != obj_proto[i]->name)
			free(obj->name);
		if (obj->description && obj->description != obj_proto[i]->description)
			free(obj->description);
		if (obj->short_description && obj->short_description != obj_proto[i]->short_description)
			free(obj->short_description);
		if (obj->action_description && obj->action_description != obj_proto[i]->action_description)
			free(obj->action_description);
		for (j = 0; j < NUM_PADS; j++)
			if (GET_OBJ_PNAME(obj, j)
					&& GET_OBJ_PNAME(obj, j) != GET_OBJ_PNAME(obj_proto[i], j))
				free(GET_OBJ_PNAME(obj, j));
		if (obj->ex_description && obj->ex_description != obj_proto[i]->ex_description)
			for (lthis = obj->ex_description; lthis; lthis = next)
			{
				next = lthis->next;
				if (lthis->keyword)
					free(lthis->keyword);
				if (lthis->description)
					free(lthis->description);
				free(lthis);
			}
	}

	// Прототип
	proto_script_free(obj->proto_script);
	// Скрипт уже NULL
}


//***********************************************************************

void oedit_setup(DESCRIPTOR_DATA * d, int real_num)
/*++
   Подготовка данных для редактирования объекта.
      d        - OLC дескриптор
      real_num - RNUM исходного объекта, новый -1
--*/
{
	OBJ_DATA *obj;

	NEWCREATE(obj, OBJ_DATA);

	if (real_num == -1)
	{
		obj->name = str_dup("новый предмет");
		obj->description = str_dup("что-то новое лежит здесь");
		obj->short_description = str_dup("новый предмет");
		obj->PNames[0] = str_dup("это что");
		obj->PNames[1] = str_dup("нету чего");
		obj->PNames[2] = str_dup("привязать к чему");
		obj->PNames[3] = str_dup("взять что");
		obj->PNames[4] = str_dup("вооружиться чем");
		obj->PNames[5] = str_dup("говорить о чем");
		GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
	}
	else
	{
		oedit_object_copy(obj, obj_proto[real_num]);
	}

	OLC_OBJ(d) = obj;
	OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
	oedit_disp_menu(d);
	OLC_VAL(d) = 0;
}

/**
* Пересчет рнумов объектов (больше нигде не меняются).
*/
void renumber_obj_rnum(int rnum)
{
	for (OBJ_DATA *obj = object_list; obj; obj = obj->next)
	{
		if (GET_OBJ_RNUM(obj) >= rnum)
		{
			// пересчет персональных хранилищ
			if (GET_OBJ_RNUM(obj) == Depot::PERS_CHEST_RNUM)
				Depot::PERS_CHEST_RNUM++;
			GET_OBJ_RNUM(obj)++;
		}
	}
	Clan::init_chest_rnum();
	Depot::renumber_obj_rnum(rnum);
	Parcel::renumber_obj_rnum(rnum);
	GlobalDrop::renumber_obj_rnum(rnum);
	ShopExt::renumber_obj_rnum(rnum);
	SetsDrop::renumber_obj_rnum(rnum);
}

/**
* Обновление данных у конкретной шмотки (для update_online_objects).
*/
extern bool is_potion(OBJ_DATA *obj);
void olc_update_object(int robj_num, OBJ_DATA *obj, OBJ_DATA *olc_proto)
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
	*obj = *olc_proto;
	obj->proto_script = NULL;
	// Восстанавливаю игровую информацию
	obj->uid = tmp.uid;
	obj->in_room = tmp.in_room;
	obj->item_number = robj_num;
	obj->carried_by = tmp.carried_by;
	obj->worn_by = tmp.worn_by;
	obj->worn_on = tmp.worn_on;
	obj->in_obj = tmp.in_obj;
	obj->contains = tmp.contains;
	obj->next_content = tmp.next_content;
	obj->next = tmp.next;
	SCRIPT(obj) = SCRIPT(&tmp);
	// для name_list
	obj->set_serial_num(tmp.get_serial_num());
	GET_OBJ_CUR(obj) = GET_OBJ_CUR(&tmp);
	obj->set_timer(tmp.get_timer());
	// емкостям сохраняем жидкость и кол-во глотков, во избежание жалоб
	if ((GET_OBJ_TYPE(&tmp) == ITEM_DRINKCON) && (GET_OBJ_TYPE(obj) == ITEM_DRINKCON))
	{
		GET_OBJ_VAL(obj, 1) = GET_OBJ_VAL(&tmp, 1); //кол-во глотков
		if (is_potion(&tmp))
		{
			GET_OBJ_VAL(obj, 2) = GET_OBJ_VAL(&tmp, 2); //описание жидкости
			GET_OBJ_SKILL(obj) = GET_OBJ_SKILL(&tmp);   //vnum зелья
		}
	}
	if (OBJ_FLAGGED(&tmp, ITEM_TICKTIMER))//если у старого объекта запущен таймер
		SET_BIT(GET_OBJ_EXTRA(obj, ITEM_TICKTIMER), ITEM_TICKTIMER);//ставим флаг таймер запущен
	if (OBJ_FLAGGED(&tmp, ITEM_NAMED))//если у старого объекта стоит флаг именной предмет
		SET_BIT(GET_OBJ_EXTRA(obj, ITEM_NAMED), ITEM_NAMED);//ставим флаг именной предмет
//	ObjectAlias::remove(obj);
//	ObjectAlias::add(obj);
}

/**
* Обновление полей объектов при изменении их прототипа через олц.
*/
void olc_update_objects(int robj_num, OBJ_DATA *olc_proto)
{
	for (OBJ_DATA *obj = object_list; obj; obj = obj->next)
	{
		if (obj->item_number == robj_num)
			olc_update_object(robj_num, obj, olc_proto);
	}
	Depot::olc_update_from_proto(robj_num, olc_proto);
	Parcel::olc_update_from_proto(robj_num, olc_proto);
}

/*------------------------------------------------------------------------*/

#define ZCMD zone_table[zone].cmd[cmd_no]

void oedit_save_internally(DESCRIPTOR_DATA * d)
{
	int i, shop, robj_num, found = FALSE, zone, cmd_no;
	INDEX_DATA *new_obj_index;
	DESCRIPTOR_DATA *dsc;

//  robj_num = real_object(OLC_NUM(d));
	robj_num = GET_OBJ_RNUM(OLC_OBJ(d));


	/*
	 * Write object to internal tables.
	 */
	if (robj_num >= 0)
	{	/*
				 * We need to run through each and every object currently in the
				 * game to see which ones are pointing to this prototype.
				 * if object is pointing to this prototype, then we need to replace it
				 * with the new one.
				 */
		log("[OEdit] Save object to mem %d", robj_num);
		olc_update_objects(robj_num, OLC_OBJ(d));

		// Все существующие в мире объекты обновлены согласно новому прототипу
		// Строки в этих объектах как ссылки на данные прототипа

		// Теперь возьмусь за прототип.
		// Уничтожаю старый прототип
		oedit_object_free(obj_proto[robj_num]);
		delete obj_proto[robj_num];
		// Копирую новый прототип в массив
		// Использовать функцию oedit_object_copy() нельзя,
		// т.к. будут изменены указатели на данные прототипа
		obj_proto[robj_num] = OLC_OBJ(d);
		// OLC_OBJ(d) удалять не нужно, т.к. он перенесен в массив
		// прототипов
	}
	else
	{		/*
				 * It's a new object, we must build new tables to contain it.
				 */
		log("[OEdit] Save mem new %d(%d/%d)", OLC_NUM(d), (top_of_objt + 2), sizeof(OBJ_DATA));

		CREATE(new_obj_index, INDEX_DATA, top_of_objt + 2);

		/*
		 * Start counting through both tables.
		 */
		for (i = 0; i <= top_of_objt; i++)  	/* If we haven't found it. */
		{
			if (!found)  	/* Check if current virtual is bigger than our virtual number. */
			{
				if (obj_index[i].vnum > OLC_NUM(d))
				{
					found = TRUE;
#if defined(DEBUG)
					fprintf(stderr, "Inserted: robj_num: %d\n", i);
#endif
					new_obj_index[i].vnum = OLC_NUM(d);
					new_obj_index[i].number = 0;
					new_obj_index[i].stored = 0;
					new_obj_index[i].func = NULL;
					robj_num = i;
					GET_OBJ_RNUM(OLC_OBJ(d)) = robj_num;
					obj_proto.insert(obj_proto.begin() + i, OLC_OBJ(d));
					new_obj_index[i].zone = real_zone(OLC_NUM(d));
					--i;
					continue;
				}
				else		/*
						 * Just copy from old to new, no number change.
						 */
					new_obj_index[i] = obj_index[i];
			}
			else
			{	/*
					 * We HAVE already found it, therefore copy to object + 1
					 */
				new_obj_index[i + 1] = obj_index[i];
				obj_proto[i + 1]->item_number = i + 1;
			}
		}
		if (!found)
		{
			new_obj_index[i].vnum = OLC_NUM(d);
			new_obj_index[i].number = 0;
			new_obj_index[i].stored = 0;
			new_obj_index[i].func = NULL;
			robj_num = i;
			GET_OBJ_RNUM(OLC_OBJ(d)) = robj_num;
			obj_proto.push_back(OLC_OBJ(d));
			new_obj_index[i].zone = real_zone(OLC_NUM(d));
		}

		/*
		 * Free and replace old table.
		 */
		free(obj_index);
		obj_index = new_obj_index;
		top_of_objt++;



// ПЕРЕИНДЕКСАЦИЯ по всей программе

		/* Renumber live objects */
		renumber_obj_rnum(robj_num);

		/* Renumber zone table. */
		for (zone = 0; zone <= top_of_zone_table; zone++)
			for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
				switch (ZCMD.command)
				{
				case 'P':
					if (ZCMD.arg3 >= robj_num)
						ZCMD.arg3++;
					// break; - no break here
				case 'O':
				case 'G':
				case 'E':
					if (ZCMD.arg1 >= robj_num)
						ZCMD.arg1++;
					break;
				case 'R':
					if (ZCMD.arg2 >= robj_num)
						ZCMD.arg2++;
					break;
				}

		/* Renumber shop produce. */
		for (shop = 0; shop < top_shop; shop++)
			for (i = 0; SHOP_PRODUCT(shop, i) != -1; i++)
				if (SHOP_PRODUCT(shop, i) >= robj_num)
					SHOP_PRODUCT(shop, i)++;

		/* Renumber produce in shops being edited. */
		for (dsc = descriptor_list; dsc; dsc = dsc->next)
			if (dsc->connected == CON_SEDIT)
				for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != -1; i++)
					if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
						S_PRODUCT(OLC_SHOP(dsc), i)++;

	}

	olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_OBJ);
}

/*------------------------------------------------------------------------*/
void tascii(int *pointer, int num_planes, char *ascii)
{
	int i, c, found;

	for (i = 0, found = FALSE; i < num_planes; i++)
	{
		for (c = 0; c < 30; c++)
			if (*(pointer + i) & (1 << c))
			{
				found = TRUE;
				sprintf(ascii + strlen(ascii), "%c%d", c < 26 ? c + 'a' : c - 26 + 'A', i);
			}
	}
	if (!found)
		strcat(ascii, "0 ");
	else
		strcat(ascii, " ");
}

void oedit_save_to_disk(int zone_num)
{
	int counter, counter2, realcounter;
	FILE *fp;
	OBJ_DATA *obj;
	EXTRA_DESCR_DATA *ex_desc;

	sprintf(buf, "%s/%d.new", OBJ_PREFIX, zone_table[zone_num].number);
	if (!(fp = fopen(buf, "w+")))
	{
		mudlog("SYSERR: OLC: Cannot open objects file!", BRF, LVL_BUILDER, SYSLOG, TRUE);
		return;
	}
	/*
	 * Start running through all objects in this zone.
	 */
	for (counter = zone_table[zone_num].number * 100; counter <= zone_table[zone_num].top; counter++)
	{
		if ((realcounter = real_object(counter)) >= 0)
		{
			if ((obj = obj_proto[realcounter])->action_description)
			{
				strcpy(buf1, obj->action_description);
				strip_string(buf1);
			}
			else
				*buf1 = '\0';
			*buf2 = '\0';
			tascii((int *) &GET_OBJ_AFFECTS(obj), 4, buf2);
			tascii((int *) &GET_OBJ_ANTI(obj), 4, buf2);
			tascii((int *) &GET_OBJ_NO(obj), 4, buf2);
			sprintf(buf2 + strlen(buf2), "\n%d ", GET_OBJ_TYPE(obj));
			tascii(&GET_OBJ_EXTRA(obj, 0), 4, buf2);
			tascii(&GET_OBJ_WEAR(obj), 1, buf2);
			strcat(buf2, "\n");

			fprintf(fp, "#%d\n"
					"%s~\n"
					"%s~\n"
					"%s~\n"
					"%s~\n"
					"%s~\n"
					"%s~\n"
					"%s~\n"
					"%s~\n"
					"%s~\n"
					"%d %d %d %d\n"
					"%d %d %d %d\n"
					"%s"
					"%d %d %d %d\n"
					"%d %d %d %d\n",
					GET_OBJ_VNUM(obj),
					not_null(obj->name, NULL),
					obj->PNames[0] ? obj->PNames[0] : "что-то",
					obj->PNames[1] ? obj->PNames[1] : "чего-то",
					obj->PNames[2] ? obj->PNames[2] : "чему-то",
					obj->PNames[3] ? obj->PNames[3] : "что-то",
					obj->PNames[4] ? obj->PNames[4] : "чем-то",
					obj->PNames[5] ? obj->PNames[5] : "о чем-то",
					not_null(obj->description, NULL),
					buf1,
					GET_OBJ_SKILL(obj), GET_OBJ_MAX(obj), GET_OBJ_CUR(obj),
					GET_OBJ_MATER(obj), GET_OBJ_SEX(obj),
					obj->get_timer(), GET_OBJ_SPELL(obj),
					GET_OBJ_LEVEL(obj), buf2, GET_OBJ_VAL(obj, 0),
					GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
					GET_OBJ_VAL(obj, 3), GET_OBJ_WEIGHT(obj),
					GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_RENTEQ(obj));

			script_save_to_disk(fp, obj, OBJ_TRIGGER);

			if (GET_OBJ_MIW(obj))
			{
				fprintf(fp, "M %d\n", GET_OBJ_MIW(obj));
			}

			if (obj->get_mort_req() > 0)
			{
				fprintf(fp, "R %d\n", obj->get_mort_req());
			}

			/*
			 * Do we have extra descriptions?
			 */
			if (obj->ex_description)  	/* Yes, save them too. */
			{
				for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next)
				{	/*
													 * Sanity check to prevent nasty protection faults.
													 */
					if (!*ex_desc->keyword || !*ex_desc->description)
					{
						mudlog
						("SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!",
						 BRF, LVL_BUILDER, SYSLOG, TRUE);
						continue;
					}
					strcpy(buf1, ex_desc->description);
					strip_string(buf1);
					fprintf(fp, "E\n" "%s~\n" "%s~\n", ex_desc->keyword, buf1);
				}
			}
			/*
			 * Do we have affects?
			 */
			for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
				if (obj->affected[counter2].location && obj->affected[counter2].modifier)
					fprintf(fp, "A\n"
							"%d %d\n", obj->affected[counter2].location,
							obj->affected[counter2].modifier);
			if (obj->has_skills())
			{
				std::map<int, int> skills;
				obj->get_skills(skills);
				for (std::map<int, int>::iterator it = skills.begin(); it != skills.end(); ++it)
					fprintf(fp, "S\n%d %d\n", it->first, it->second);
			}
		}
	}

	/*
	 * Write the final line, close the file.
	 */
	fprintf(fp, "$\n$\n");
	fclose(fp);
	sprintf(buf2, "%s/%d.obj", OBJ_PREFIX, zone_table[zone_num].number);
	/*
	 * We're fubar'd if we crash between the two lines below.
	 */
	remove(buf2);
	rename(buf, buf2);

	olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_OBJ);
}

/**************************************************************************
 Menu functions
 **************************************************************************/

/*
 * For container flags.
 */
void oedit_disp_container_flags_menu(DESCRIPTOR_DATA * d)
{
	get_char_cols(d->character);
	sprintbit(GET_OBJ_VAL(OLC_OBJ(d), 1), container_bits, buf1);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	sprintf(buf,
			"%s1%s) Закрываем\r\n"
			"%s2%s) Нельзя взломать\r\n"
			"%s3%s) Закрыт\r\n"
			"%s4%s) Заперт\r\n"
			"Флаги контейнера: %s%s%s\r\n"
			"Выберите флаг, 0 - выход : ", grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, buf1, nrm);
	send_to_char(buf, d->character);
}

/*
 * For extra descriptions.
 */
void oedit_disp_extradesc_menu(DESCRIPTOR_DATA * d)
{
	EXTRA_DESCR_DATA *extra_desc = OLC_DESC(d);

	strcpy(buf1, !extra_desc->next ? "<Not set>\r\n" : "Set.");

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	sprintf(buf,
			"Меню экстрадескрипторов\r\n"
			"%s1%s) Ключ: %s%s\r\n"
			"%s2%s) Описание:\r\n%s%s\r\n"
			"%s3%s) Следующий дескриптор: %s\r\n"
			"%s0%s) Выход\r\n"
			"Ваш выбор : ",
			grn, nrm, yel, not_null(extra_desc->keyword, "<NONE>"),
			grn, nrm, yel, not_null(extra_desc->description, "<NONE>"),
			grn, nrm, buf1, grn, nrm);
	send_to_char(buf, d->character);
	OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/*
 * Ask for *which* apply to edit.
 */
void oedit_disp_prompt_apply_menu(DESCRIPTOR_DATA * d)
{
	int counter;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < MAX_OBJ_AFFECT; counter++)
	{
		if (OLC_OBJ(d)->affected[counter].modifier)
		{
			sprinttype(OLC_OBJ(d)->affected[counter].location, apply_types, buf2);
			sprintf(buf, " %s%d%s) %+d to %s\r\n", grn, counter + 1, nrm,
					OLC_OBJ(d)->affected[counter].modifier, buf2);
			send_to_char(buf, d->character);
		}
		else
		{
			sprintf(buf, " %s%d%s) Ничего.\r\n", grn, counter + 1, nrm);
			send_to_char(buf, d->character);
		}
	}
	send_to_char("\r\nВыберите изменяемый аффект (0 - выход) : ", d->character);
	OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

/*
 * Ask for liquid type.
 */
void oedit_liquid_type(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_LIQ_TYPES; counter++)
	{
		sprintf(buf, " %s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				drinks[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n%sВыберите тип жидкости : ", nrm);
	send_to_char(buf, d->character);
	OLC_MODE(d) = OEDIT_VALUE_3;
}

/*
 * The actual apply to set.
 */
void oedit_disp_apply_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_APPLIES; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
				apply_types[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	send_to_char("\r\nЧто добавляем (0 - выход) : ", d->character);
	OLC_MODE(d) = OEDIT_APPLY;
}

/*
 * Weapon type.
 */
void oedit_disp_weapon_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ATTACK_TYPES; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
				attack_hit_text[counter].singular, !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	send_to_char("\r\nВыберите тип удара (0 - выход): ", d->character);
}

/*
 * Spell type.
 */
void oedit_disp_spells_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < MAX_SPELLS; counter++)
	{
		if (!spell_info[counter].name || *spell_info[counter].name == '!')
			continue;
		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n%sВыберите магию (0 - выход) : ", nrm);
	send_to_char(buf, d->character);
}

void oedit_disp_skills2_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < MAX_SKILL_NUM; counter++)
	{
		if (!skill_info[counter].name || *skill_info[counter].name == '!')
			continue;
		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				skill_info[counter].name, !(++columns % 3) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n%sВыберите умение (0 - выход) : ", nrm);
	send_to_char(buf, d->character);
}

void oedit_disp_receipts_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter <= top_imrecipes; counter++)
	{
		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				imrecipes[counter].name, !(++columns % 3) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n%sВыберите рецепт : ", nrm);
	send_to_char(buf, d->character);
}

void oedit_disp_feats_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < MAX_FEATS; counter++)
	{
		if (!feat_info[counter].name || *feat_info[counter].name == '!')
			continue;
		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				feat_info[counter].name, !(++columns % 3) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\n%sВыберите способность (0 - выход) : ", nrm);
	send_to_char(buf, d->character);
}

void oedit_disp_skills_mod_menu(DESCRIPTOR_DATA* d)
{
	int columns = 0, counter;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	int percent;
	for (counter = 1; counter <= MAX_SKILL_NUM; ++counter)
	{
		if (!skill_info[counter].name || *skill_info[counter].name == '!')
			continue;
		percent = OLC_OBJ(d)->get_skill(counter);
		if (percent != 0)
			sprintf(buf1, "%s[%3d]%s", cyn, percent, nrm);
		else
			strcpy(buf1, "     ");
		sprintf(buf, "%s%3d%s) %25s%s%s", grn, counter, nrm,
				skill_info[counter].name, buf1, !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	send_to_char("\r\nУкажите номер и уровень владения умением (0 - конец) : ", d->character);
}

/*
 * Object value #1
 */
void oedit_disp_val1_menu(DESCRIPTOR_DATA * d)
{
	OLC_MODE(d) = OEDIT_VALUE_1;
	switch (GET_OBJ_TYPE(OLC_OBJ(d)))
	{
	case ITEM_LIGHT:
		/*
		 * values 0 and 1 are unused.. jump to 2
		 */
		oedit_disp_val3_menu(d);
		break;
	case ITEM_SCROLL:
	case ITEM_WAND:
	case ITEM_STAFF:
	case ITEM_POTION:
		send_to_char("Уровень заклинания : ", d->character);
		break;
	case ITEM_WEAPON:
		/*
		 * This doesn't seem to be used if I remember right.
		 */
		send_to_char("Модификатор попадания : ", d->character);
		break;
	case ITEM_ARMOR:
	case ITEM_ARMOR_LIGHT:
	case ITEM_ARMOR_MEDIAN:
	case ITEM_ARMOR_HEAVY:
		send_to_char("Изменяет АС на : ", d->character);
		break;
	case ITEM_CONTAINER:
		send_to_char("Максимально вместимый вес : ", d->character);
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
		send_to_char("Количество глотков : ", d->character);
		break;
	case ITEM_FOOD:
		send_to_char("На сколько часов насыщает : ", d->character);
		break;
	case ITEM_MONEY:
		send_to_char("Сколько кун содержит : ", d->character);
		break;
	case ITEM_NOTE:
		/*
		 * This is supposed to be language, but it's unused.
		 */
		break;
	case ITEM_BOOK:
//		send_to_char("Тип книги (0-закл, 1-ум, 2-ум+, 3-рецепт, 4-фит): ", d->character);
		sprintf(buf,
				"%s0%s) %sКнига заклинаний\r\n"
				"%s1%s) %sКнига умений\r\n"
				"%s2%s) %sУлучшение умения\r\n"
				"%s3%s) %sКнига рецептов\r\n"
				"%s4%s) %sКнига способностей\r\n"
				"%sВыберите тип книги : ", grn, nrm, yel, grn, nrm, yel, grn, nrm, yel, grn, nrm, yel, grn, nrm, yel, nrm);
		send_to_char(buf, d->character);
		break;
	case ITEM_INGRADIENT:
		send_to_char("Первый байт - лаг после применения в сек, 5 бит - уровень : ", d->character);
		break;
	case ITEM_MING:
		oedit_disp_val4_menu(d);
		break;
	case ITEM_MATERIAL:
		send_to_char("Первый параметр: ", d->character);
		break;
	case ITEM_BANDAGE:
		send_to_char("Хитов в секунду: ", d->character);
		break;
	case ITEM_ENCHANT:
		send_to_char("Изменяет вес: ", d->character);
		break;

	default:
		oedit_disp_menu(d);
	}
}

/*
 * Object value #2
 */
void oedit_disp_val2_menu(DESCRIPTOR_DATA * d)
{
	OLC_MODE(d) = OEDIT_VALUE_2;
	switch (GET_OBJ_TYPE(OLC_OBJ(d)))
	{
	case ITEM_SCROLL:
	case ITEM_POTION:
		oedit_disp_spells_menu(d);
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		send_to_char("Количество зарядов : ", d->character);
		break;
	case ITEM_WEAPON:
		send_to_char("Количество бросков кубика : ", d->character);
		break;
	case ITEM_ARMOR:
	case ITEM_ARMOR_LIGHT:
	case ITEM_ARMOR_MEDIAN:
	case ITEM_ARMOR_HEAVY:
		send_to_char("Изменяет броню на : ", d->character);
		break;
	case ITEM_FOOD:
		/*
		 * Values 2 and 3 are unused, jump to 4...Odd.
		 */
		oedit_disp_val4_menu(d);
		break;
	case ITEM_CONTAINER:
		/*
		 * These are flags, needs a bit of special handling.
		 */
		oedit_disp_container_flags_menu(d);
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
		send_to_char("Начальное количество глотков : ", d->character);
		break;
	case ITEM_BOOK:
		switch (GET_OBJ_VAL(OLC_OBJ(d), 0))
		{
		case BOOK_SPELL:
//		  send_to_char("Введите номер заклинания : ", d->character);
			oedit_disp_spells_menu(d);
			break;
		case BOOK_SKILL:
		case BOOK_UPGRD:
//		  send_to_char("Введите номер умения : ", d->character);
			oedit_disp_skills2_menu(d);
			break;
		case BOOK_RECPT:
//		  send_to_char("Введите номер рецепта : ", d->character);
			oedit_disp_receipts_menu(d);
			break;
		case BOOK_FEAT:
//		  send_to_char("Введите номер способности : ", d->character);
			oedit_disp_feats_menu(d);
			break;
		default:
			oedit_disp_val4_menu(d);
		}
		break;
	case ITEM_INGRADIENT:
		send_to_char("Виртуальный номер прототипа  : ", d->character);
		break;
	case ITEM_MATERIAL:
		send_to_char("Второй параметр: ", d->character);
		break;
	default:
		oedit_disp_menu(d);
	}
}

/*
 * Object value #3
 */
void oedit_disp_val3_menu(DESCRIPTOR_DATA * d)
{
	OLC_MODE(d) = OEDIT_VALUE_3;
	switch (GET_OBJ_TYPE(OLC_OBJ(d)))
	{
	case ITEM_LIGHT:
		send_to_char("Длительность горения (0 = погасла, -1 - вечный свет) : ", d->character);
		break;
	case ITEM_SCROLL:
	case ITEM_POTION:
		oedit_disp_spells_menu(d);
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		send_to_char("Осталось зарядов : ", d->character);
		break;
	case ITEM_WEAPON:
		send_to_char("Количество граней кубика : ", d->character);
		break;
	case ITEM_CONTAINER:
		send_to_char("Vnum ключа для контейнера (-1 - нет ключа) : ", d->character);
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
		oedit_liquid_type(d);
		break;
	case ITEM_BOOK:
//		send_to_char("Уровень изучения (+ к умению если тип = 2 ) : ", d->character);
		switch (GET_OBJ_VAL(OLC_OBJ(d), 0))
		{
		case BOOK_SKILL:
			send_to_char("Введите уровень изучения : ", d->character);
			break;
		case BOOK_UPGRD:
			send_to_char("На сколько увеличится умение : ", d->character);
			break;
		default:
			oedit_disp_val4_menu(d);
		}
		break;
	case ITEM_INGRADIENT:
		send_to_char("Сколько раз можно использовать : ", d->character);
		break;
	case ITEM_MATERIAL:
		send_to_char("Введите количество (размер): ", d->character);
		break;
	default:
		oedit_disp_menu(d);
	}
}

/*
 * Object value #4
 */
void oedit_disp_val4_menu(DESCRIPTOR_DATA * d)
{
	OLC_MODE(d) = OEDIT_VALUE_4;
	switch (GET_OBJ_TYPE(OLC_OBJ(d)))
	{
	case ITEM_SCROLL:
	case ITEM_POTION:
	case ITEM_WAND:
	case ITEM_STAFF:
		oedit_disp_spells_menu(d);
		break;
	case ITEM_WEAPON:
		oedit_disp_weapon_menu(d);
		break;
	case ITEM_DRINKCON:
	case ITEM_FOUNTAIN:
	case ITEM_FOOD:
		send_to_char("Отравлено (0 = не отравлено) : ", d->character);
		break;
	case ITEM_BOOK:
//		send_to_char("Макс. уровень умения или тип способности (0-врожденная, 1) : ", d->character);
		switch (GET_OBJ_VAL(OLC_OBJ(d), 0))
		{
		case BOOK_UPGRD:
			send_to_char("Максимальный % умения :\r\n"
					"Если <= 0, то учитывается только макс. возможный навык игрока на данном реморте.\r\n"
					"Если > 0, то учитывается только данное значение без учета макс. навыка игрока.\r\n"
					, d->character);
			break;
		default:
			OLC_VAL(d) = 1;
			oedit_disp_menu(d);
		}
		break;
	case ITEM_MING:
		send_to_char("Класс ингредиента (0-РОСЛЬ,1-ЖИВЬ,2-ТВЕРДЬ): ", d->character);
		break;
	case ITEM_MATERIAL:
		send_to_char("Введите материал: ", d->character);
		break;
	case ITEM_CONTAINER:
		send_to_char("Введите сложность замка (0-255): ", d->character);
		break;
	default:
		oedit_disp_menu(d);
	}
}

/*
 * Object type.
 */
void oedit_disp_type_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ITEM_TYPES; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
				item_types[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	send_to_char("\r\nВыберите тип предмета : ", d->character);
}

/*
 * Object extra flags.
 */
void oedit_disp_extra_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0, plane = 0;
	char c;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0, c = 'a' - 1; plane < NUM_PLANES; counter++)
	{
		if (*extra_bits[counter] == '\n')
		{
			plane++;
			c = 'a' - 1;
			continue;
		}
		else if (c == 'z')
			c = 'A';
		else
			c++;

		sprintf(buf, "%s%c%d%s) %-20.20s %s", grn, c, plane, nrm,
				extra_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbits(OLC_OBJ(d)->obj_flags.extra_flags, extra_bits, buf1, ",", true);
	sprintf(buf, "\r\nЭкстрафлаги: %s%s%s\r\n" "Выберите экстрафлаг (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character);
}

void oedit_disp_anti_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0, plane = 0;
	char c;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0, c = 'a' - 1; plane < NUM_PLANES; counter++)
	{
		if (*anti_bits[counter] == '\n')
		{
			plane++;
			c = 'a' - 1;
			continue;
		}
		else if (c == 'z')
			c = 'A';
		else
			c++;

		sprintf(buf, "%s%c%d%s) %-20.20s %s", grn, c, plane, nrm,
				anti_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbits(OLC_OBJ(d)->obj_flags.anti_flag, anti_bits, buf1, ",", true);
	sprintf(buf, "\r\nПредмет запрещен для : %s%s%s\r\n" "Выберите флаг запрета (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character);
}

void oedit_disp_no_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0, plane = 0;
	char c;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0, c = 'a' - 1; plane < NUM_PLANES; counter++)
	{
		if (*no_bits[counter] == '\n')
		{
			plane++;
			c = 'a' - 1;
			continue;
		}
		else if (c == 'z')
			c = 'A';
		else
			c++;

		sprintf(buf, "%s%c%d%s) %-20.20s %s", grn, c, plane, nrm,
				no_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbits(OLC_OBJ(d)->obj_flags.no_flag, no_bits, buf1, ",", true);
	sprintf(buf, "\r\nПредмет неудобен для : %s%s%s\r\n" "Выберите флаг неудобств (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character);
}

void oedit_disp_affects_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0, plane = 0;
	char c;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0, c = 'a' - 1; plane < NUM_PLANES; counter++)
	{
		if (*weapon_affects[counter] == '\n')
		{
			plane++;
			c = 'a' - 1;
			continue;
		}
		else if (c == 'z')
			c = 'A';
		else
			c++;

		sprintf(buf, "%s%c%d%s) %-20.20s %s", grn, c, plane, nrm,
				weapon_affects[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbits(OLC_OBJ(d)->obj_flags.affects, weapon_affects, buf1, ",", true);
	sprintf(buf, "\r\nНакладываемые аффекты : %s%s%s\r\n" "Выберите аффект (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character);
}

/*
 * Object wear flags.
 */
void oedit_disp_wear_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ITEM_WEARS; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				wear_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbit(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, buf1);
	sprintf(buf, "\r\nМожет быть одет : %s%s%s\r\n" "Выберите позицию (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character);
}

void oedit_disp_mater_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < 32 && *material_name[counter] != '\n'; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				material_name[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintf(buf, "\r\nСделан из : %s%s%s\r\n"
			"Выберите материал (0 - выход) : ", cyn, material_name[GET_OBJ_MATER(OLC_OBJ(d))], nrm);
	send_to_char(buf, d->character);
}

void oedit_disp_ingradient_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < 32 && *ingradient_bits[counter] != '\n'; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				ingradient_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintbit(GET_OBJ_SKILL(OLC_OBJ(d)), ingradient_bits, buf1);
	sprintf(buf, "\r\nТип ингредиента : %s%s%s\r\n" "Дополните тип (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character);
}

const char *wskill_bits[] = { "палицы и дубины(141)",
							  "секиры(142)",
							  "длинные лезвия(143)",
							  "короткие лезвия(144)",
							  "иное(145)",
							  "двуручники(146)",
							  "проникающее(147)",
							  "копья и рогатины(148)",
							  "луки(154)",
							  "\n"
							};


void oedit_disp_skills_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_INGRADIENT)
	{
		oedit_disp_ingradient_menu(d);
		return;
	}
	if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_DRINKCON || GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_FOUNTAIN)
	{
		sprintf(buf, "Текущий номер зелья : %s%d%s\r\n", cyn, GET_OBJ_SKILL(OLC_OBJ(d)), nrm);
		send_to_char(buf, d->character);
		send_to_char("Введите номер зелья (0 для выхода): ", d->character);
		return;
	}
	get_char_cols(d->character);
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < 32 && *wskill_bits[counter] != '\n'; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				wskill_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character);
	}
	sprintf(buf, "%sТренируемое умение : %s%d%s\r\n"
			"Выберите умение (0 - выход) : ", (columns%2 == 1?"\r\n":""), cyn, GET_OBJ_SKILL(OLC_OBJ(d)), nrm);
	send_to_char(buf, d->character);
}


/*
 * Display main menu.
 */
void oedit_disp_menu(DESCRIPTOR_DATA * d)
{
	OBJ_DATA *obj;

	obj = OLC_OBJ(d);
	get_char_cols(d->character);

	sprinttype(GET_OBJ_TYPE(obj), item_types, buf1);
	sprintbits(obj->obj_flags.extra_flags, extra_bits, buf2, ",");

	sprintf(buf,
#if defined(CLEAR_SCREEN)
			"[H[J"
#endif
			"-- Предмет : [%s%d%s]\r\n"
			"%s1%s) Синонимы : %s&S%s&s\r\n"
			"%s2&n) Именительный (это ЧТО)             : %s&e\r\n"
			"%s3&n) Родительный  (нету ЧЕГО)           : %s&e\r\n"
			"%s4&n) Дательный    (прикрепить к ЧЕМУ)   : %s&e\r\n"
			"%s5&n) Винительный  (держать ЧТО)         : %s&e\r\n"
			"%s6&n) Творительный (вооружиться ЧЕМ)     : %s&e\r\n"
			"%s7&n) Предложный   (писать на ЧЕМ)       : %s&e\r\n"
			"%s8&n) Описание          :-\r\n&Y&q%s&e&Q\r\n"
			"%s9&n) Опис.при действии :-\r\n%s%s\r\n"
			"%sA%s) Тип предмета      :-\r\n%s%s\r\n"
			"%sB%s) Экстрафлаги       :-\r\n%s%s\r\n",
			cyn, OLC_NUM(d), nrm,
			grn, nrm, yel, not_null(obj->name, NULL),
			grn, not_null(obj->PNames[0], NULL),
			grn, not_null(obj->PNames[1], NULL),
			grn, not_null(obj->PNames[2], NULL),
			grn, not_null(obj->PNames[3], NULL),
			grn, not_null(obj->PNames[4], NULL),
			grn, not_null(obj->PNames[5], NULL),
			grn, not_null(obj->description, NULL),
			grn, yel, not_null(obj->action_description, "<not set>\r\n"),
			grn, nrm, cyn, buf1, grn, nrm, cyn, buf2);
	/*
	 * Send first half.
	 */
	send_to_char(buf, d->character);

	sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf1);
	sprintbits(obj->obj_flags.no_flag, no_bits, buf2, ",");
	sprintf(buf,
			"%sC%s) Одевается  : %s%s\r\n"
			"%sD%s) Неудобен    : %s%s\r\n", grn, nrm, cyn, buf1, grn, nrm, cyn, buf2);
	send_to_char(buf, d->character);

	sprintbits(obj->obj_flags.anti_flag, anti_bits, buf1, ",");
	sprintbits(obj->obj_flags.affects, weapon_affects, buf2, ",");
	sprintf(buf,
			"%sE%s) Запрещен    : %s%s\r\n"
			"%sF%s) Вес         : %s%8d   %sG%s) Цена        : %s%d\r\n"
			"%sH%s) Рента(снято): %s%8d   %sI%s) Рента(одето): %s%d\r\n"
			"%sJ%s) Мах.проч.   : %s%8d   %sK%s) Тек.проч    : %s%d\r\n"
			"%sL%s) Материал    : %s%s\r\n"
			"%sM%s) Таймер      : %s%8d   %sN%s) Skill       : %s%d\r\n"
			"%sO%s) Values      : %s%d %d %d %d\r\n"
			"%sP%s) Аффекты     : %s%s\r\n"
			"%sR%s) Меню наводимых аффектов\r\n"
			"%sT%s) Меню экстраописаний\r\n"
			"%sS%s) Скрипт      : %s%s\r\n"
			"%sU%s) Пол         : %s%s\r\n"
			"%sV%s) Макс.в мире : %s%d\r\n"
			"%sW%s) Меню умений\r\n"
			"%sX%s) Требует перевоплощений : %s%d\r\n"
			"%sQ%s) Quit\r\n"
			"Ваш выбор : ",
			grn, nrm, cyn, buf1,
			grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
			grn, nrm, cyn, GET_OBJ_COST(obj),
			grn, nrm, cyn, GET_OBJ_RENT(obj),
			grn, nrm, cyn, GET_OBJ_RENTEQ(obj),
			grn, nrm, cyn, GET_OBJ_MAX(obj),
			grn, nrm, cyn, GET_OBJ_CUR(obj),
			grn, nrm, cyn, material_name[GET_OBJ_MATER(obj)],
			grn, nrm, cyn, obj->get_timer(),
			grn, nrm, cyn, GET_OBJ_SKILL(obj),
			grn, nrm, cyn,
			GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
			GET_OBJ_VAL(obj, 3), grn, nrm, grn, buf2, grn, nrm, grn, nrm, grn,
			nrm, cyn, obj->proto_script ? "Set." : "Not Set.",
			grn, nrm, cyn, genders[GET_OBJ_SEX(obj)],
			grn, nrm, cyn, GET_OBJ_MIW(obj),
			grn, nrm,
			grn, nrm, cyn, obj->get_mort_req(),
			grn, nrm);
	send_to_char(buf, d->character);
	OLC_MODE(d) = OEDIT_MAIN_MENU;
}

/***************************************************************************
 main loop (of sorts).. basically interpreter throws all input to here
 ***************************************************************************/
int planebit(char *str, int *plane, int *bit)
{
	if (!str || !*str)
		return (-1);
	if (*str == '0')
		return (0);
	if (*str >= 'a' && *str <= 'z')
		*bit = (*(str) - 'a');
	else if (*str >= 'A' && *str <= 'D')
		*bit = (*(str) - 'A' + 26);
	else
		return (-1);

	if (*(str + 1) >= '0' && *(str + 1) <= '3')
		*plane = (*(str + 1) - '0');
	else
		return (-1);
	return (1);
}

void oedit_parse(DESCRIPTOR_DATA * d, char *arg)
{
	int number, max_val, min_val, plane, bit;

	switch (OLC_MODE(d))
	{
	case OEDIT_CONFIRM_SAVESTRING:
		switch (*arg)
		{
		case 'y':
		case 'Y':
		case 'д':
		case 'Д':
			send_to_char("Saving object to memory.\r\n", d->character);
			oedit_save_internally(d);
			sprintf(buf, "OLC: %s edits obj %d", GET_NAME(d->character), OLC_NUM(d));
			olc_log("%s edit obj %d", GET_NAME(d->character), OLC_NUM(d));
			mudlog(buf, NRM, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
			cleanup_olc(d, CLEANUP_STRUCTS);
			break;
		case 'n':
		case 'N':
		case 'н':
		case 'Н':
			cleanup_olc(d, CLEANUP_ALL);
			break;
		default:
			send_to_char("Неверный выбор !\r\n", d->character);
			send_to_char("Вы хотите сохранить этот предмет ?\r\n", d->character);
			break;
		}
		return;

	case OEDIT_MAIN_MENU:
		/*
		 * Throw us out to whichever edit mode based on user input.
		 */
		switch (*arg)
		{
		case 'q':
		case 'Q':
			if (OLC_VAL(d))  	/* Something has been modified. */
			{
				send_to_char("Вы хотите сохранить этот предмет ? : ", d->character);
				OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
			}
			else
				cleanup_olc(d, CLEANUP_ALL);
			return;
		case 'x':
		case 'X':
			send_to_char("Требует перевоплощений : ", d->character);
			OLC_MODE(d) = OEDIT_MORT_REQ;
			break;
		case '1':
			send_to_char("Введите синонимы : ", d->character);
			OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
			break;
		case '2':
			send_to_char(d->character, "&S%s&s\r\nИменительный падеж [это ЧТО] : ", OLC_OBJ(d)->PNames[0]);
			OLC_MODE(d) = OEDIT_PAD0;
			break;
		case '3':
			send_to_char(d->character, "&S%s&s\r\nРодительный падеж [нет ЧЕГО] : ", OLC_OBJ(d)->PNames[1]);
			OLC_MODE(d) = OEDIT_PAD1;
			break;
		case '4':
			send_to_char(d->character, "&S%s&s\r\nДательный падеж [прикрепить к ЧЕМУ] : ", OLC_OBJ(d)->PNames[2]);
			OLC_MODE(d) = OEDIT_PAD2;
			break;
		case '5':
			send_to_char(d->character, "&S%s&s\r\nВинительный падеж [держать ЧТО] : ", OLC_OBJ(d)->PNames[3]);
			OLC_MODE(d) = OEDIT_PAD3;
			break;
		case '6':
			send_to_char(d->character, "&S%s&s\r\nТворительный падеж [вооружиться ЧЕМ] : ", OLC_OBJ(d)->PNames[4]);
			OLC_MODE(d) = OEDIT_PAD4;
			break;
		case '7':
			send_to_char(d->character, "&S%s&s\r\nПредложный падеж [писать на ЧЕМ] : ", OLC_OBJ(d)->PNames[5]);
			OLC_MODE(d) = OEDIT_PAD5;
			break;
		case '8':
			send_to_char(d->character, "&S%s&s\r\nВведите длинное описание :-\r\n| ", OLC_OBJ(d)->description);
			OLC_MODE(d) = OEDIT_LONGDESC;
			break;
		case '9':
			OLC_MODE(d) = OEDIT_ACTDESC;
			SEND_TO_Q("Введите описание при применении: (/s сохранить /h помощь)\r\n\r\n", d);
			d->backstr = NULL;
			if (OLC_OBJ(d)->action_description)
			{
				SEND_TO_Q(OLC_OBJ(d)->action_description, d);
				d->backstr = str_dup(OLC_OBJ(d)->action_description);
			}
			d->str = &OLC_OBJ(d)->action_description;
			d->max_str = 4096;
			d->mail_to = 0;
			OLC_VAL(d) = 1;
			break;
		case 'a':
		case 'A':
			oedit_disp_type_menu(d);
			OLC_MODE(d) = OEDIT_TYPE;
			break;
		case 'b':
		case 'B':
			oedit_disp_extra_menu(d);
			OLC_MODE(d) = OEDIT_EXTRAS;
			break;
		case 'c':
		case 'C':
			oedit_disp_wear_menu(d);
			OLC_MODE(d) = OEDIT_WEAR;
			break;
		case 'd':
		case 'D':
			oedit_disp_no_menu(d);
			OLC_MODE(d) = OEDIT_NO;
			break;
		case 'e':
		case 'E':
			oedit_disp_anti_menu(d);
			OLC_MODE(d) = OEDIT_ANTI;
			break;
		case 'f':
		case 'F':
			send_to_char("Вес предмета : ", d->character);
			OLC_MODE(d) = OEDIT_WEIGHT;
			break;
		case 'g':
		case 'G':
			send_to_char("Цена предмета : ", d->character);
			OLC_MODE(d) = OEDIT_COST;
			break;
		case 'h':
		case 'H':
			send_to_char("Рента предмета (в инвентаре) : ", d->character);
			OLC_MODE(d) = OEDIT_COSTPERDAY;
			break;
		case 'i':
		case 'I':
			send_to_char("Рента предмета (в экипировке) : ", d->character);
			OLC_MODE(d) = OEDIT_COSTPERDAYEQ;
			break;
		case 'j':
		case 'J':
			send_to_char("Максимальная прочность : ", d->character);
			OLC_MODE(d) = OEDIT_MAXVALUE;
			break;
		case 'k':
		case 'K':
			send_to_char("Текущая прочность : ", d->character);
			OLC_MODE(d) = OEDIT_CURVALUE;
			break;
		case 'l':
		case 'L':
			oedit_disp_mater_menu(d);
			OLC_MODE(d) = OEDIT_MATER;
			break;
		case 'm':
		case 'M':
			send_to_char("Таймер (в тиках) : ", d->character);
			OLC_MODE(d) = OEDIT_TIMER;
			break;
		case 'n':
		case 'N':
			if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_WEAPON ||
					GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_INGRADIENT ||
					GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_DRINKCON || GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_FOUNTAIN)
			{
				oedit_disp_skills_menu(d);
				OLC_MODE(d) = OEDIT_SKILL;
			}
			else
				oedit_disp_menu(d);
			break;
		case 'o':
		case 'O':
			/*
			 * Clear any old values
			 */
			GET_OBJ_VAL(OLC_OBJ(d), 0) = 0;
			GET_OBJ_VAL(OLC_OBJ(d), 1) = 0;
			GET_OBJ_VAL(OLC_OBJ(d), 2) = 0;
			GET_OBJ_VAL(OLC_OBJ(d), 3) = 0;
			oedit_disp_val1_menu(d);
			break;
		case 'p':
		case 'P':
			oedit_disp_affects_menu(d);
			OLC_MODE(d) = OEDIT_AFFECTS;
			break;
		case 'r':
		case 'R':
			oedit_disp_prompt_apply_menu(d);
			break;
		case 't':
		case 'T':
			/*
			 * If extra descriptions don't exist.
			 */
			if (!OLC_OBJ(d)->ex_description)
			{
				CREATE(OLC_OBJ(d)->ex_description, EXTRA_DESCR_DATA, 1);
				OLC_OBJ(d)->ex_description->next = NULL;
			}
			OLC_DESC(d) = OLC_OBJ(d)->ex_description;
			oedit_disp_extradesc_menu(d);
			break;
		case 's':
		case 'S':
			dg_olc_script_copy(d);
			OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
			dg_script_menu(d);
			return;
		case 'u':
		case 'U':
			send_to_char("Пол : ", d->character);
			OLC_MODE(d) = OEDIT_SEXVALUE;
			break;
		case 'v':
		case 'V':
			send_to_char("Максимальное число в мире : ", d->character);
			OLC_MODE(d) = OEDIT_MIWVALUE;
			break;
		case 'w':
		case 'W':
			oedit_disp_skills_mod_menu(d);
			OLC_MODE(d) = OEDIT_SKILLS;
			break;
		default:
			oedit_disp_menu(d);
			break;
		}
		return;
		/*
		 * end of OEDIT_MAIN_MENU
		 */

	case OLC_SCRIPT_EDIT:
		if (dg_script_edit_parse(d, arg))
			return;
		break;

	case OEDIT_EDIT_NAMELIST:
		if (OLC_OBJ(d)->name)
			free(OLC_OBJ(d)->name);
		OLC_OBJ(d)->name = str_dup(not_null(arg, NULL));
		break;

	case OEDIT_PAD0:
		if (OLC_OBJ(d)->PNames[0])
			free(OLC_OBJ(d)->PNames[0]);
		if (OLC_OBJ(d)->short_description)
			free(OLC_OBJ(d)->short_description);
		OLC_OBJ(d)->short_description = str_dup(not_null(arg, "что-то"));
		OLC_OBJ(d)->PNames[0] = str_dup(not_null(arg, "что-то"));
		break;

	case OEDIT_PAD1:
		if (OLC_OBJ(d)->PNames[1])
			free(OLC_OBJ(d)->PNames[1]);
		OLC_OBJ(d)->PNames[1] = str_dup(not_null(arg, "-чего-то"));
		break;

	case OEDIT_PAD2:
		if (OLC_OBJ(d)->PNames[2])
			free(OLC_OBJ(d)->PNames[2]);
		OLC_OBJ(d)->PNames[2] = str_dup(not_null(arg, "-чему-то"));
		break;

	case OEDIT_PAD3:
		if (OLC_OBJ(d)->PNames[3])
			free(OLC_OBJ(d)->PNames[3]);
		OLC_OBJ(d)->PNames[3] = str_dup(not_null(arg, "-что-то"));
		break;

	case OEDIT_PAD4:
		if (OLC_OBJ(d)->PNames[4])
			free(OLC_OBJ(d)->PNames[4]);
		OLC_OBJ(d)->PNames[4] = str_dup(not_null(arg, "-чем-то"));
		break;

	case OEDIT_PAD5:
		if (OLC_OBJ(d)->PNames[5])
			free(OLC_OBJ(d)->PNames[5]);
		OLC_OBJ(d)->PNames[5] = str_dup(not_null(arg, "-чем-то"));
		break;

	case OEDIT_LONGDESC:
		if (OLC_OBJ(d)->description)
			free(OLC_OBJ(d)->description);
		OLC_OBJ(d)->description = str_dup(not_null(arg, "неопределено"));
		break;

	case OEDIT_TYPE:
		number = atoi(arg);
		if ((number < 1) || (number >= NUM_ITEM_TYPES))
		{
			send_to_char("Invalid choice, try again : ", d->character);
			return;
		}
		else
		{
			GET_OBJ_TYPE(OLC_OBJ(d)) = number;
			if (number != ITEM_WEAPON && number != ITEM_INGRADIENT &&
					number != ITEM_DRINKCON && number != ITEM_FOUNTAIN)
				GET_OBJ_SKILL(OLC_OBJ(d)) = 0;
		}
		break;

	case OEDIT_EXTRAS:
		number = planebit(arg, &plane, &bit);
		if (number < 0)
		{
			oedit_disp_extra_menu(d);
			return;
		}
		else if (number == 0)
			break;
		else
		{
			TOGGLE_BIT(OLC_OBJ(d)->obj_flags.extra_flags.flags[plane], 1 << (bit));
			oedit_disp_extra_menu(d);
			return;
		}

	case OEDIT_WEAR:
		number = atoi(arg);
		if ((number < 0) || (number > NUM_ITEM_WEARS))
		{
			send_to_char("Неверный выбор !\r\n", d->character);
			oedit_disp_wear_menu(d);
			return;
		}
		else if (number == 0)	/* Quit. */
			break;
		else
		{
			TOGGLE_BIT(GET_OBJ_WEAR(OLC_OBJ(d)), 1 << (number - 1));
			oedit_disp_wear_menu(d);
			return;
		}

	case OEDIT_NO:
		number = planebit(arg, &plane, &bit);
		if (number < 0)
		{
			oedit_disp_no_menu(d);
			return;
		}
		else if (number == 0)
			break;
		else
		{
			TOGGLE_BIT(OLC_OBJ(d)->obj_flags.no_flag.flags[plane], 1 << (bit));
			oedit_disp_no_menu(d);
			return;
		}

	case OEDIT_ANTI:
		number = planebit(arg, &plane, &bit);
		if (number < 0)
		{
			oedit_disp_anti_menu(d);
			return;
		}
		else if (number == 0)
			break;
		else
		{
			TOGGLE_BIT(OLC_OBJ(d)->obj_flags.anti_flag.flags[plane], 1 << (bit));
			oedit_disp_anti_menu(d);
			return;
		}


	case OEDIT_WEIGHT:
		GET_OBJ_WEIGHT(OLC_OBJ(d)) = atoi(arg);
		break;

	case OEDIT_COST:
		GET_OBJ_COST(OLC_OBJ(d)) = atoi(arg);
		break;

	case OEDIT_COSTPERDAY:
		GET_OBJ_RENT(OLC_OBJ(d)) = atoi(arg);
		break;

	case OEDIT_MAXVALUE:
		GET_OBJ_MAX(OLC_OBJ(d)) = atoi(arg);
		break;

	case OEDIT_CURVALUE:
		GET_OBJ_CUR(OLC_OBJ(d)) = MIN(GET_OBJ_MAX(OLC_OBJ(d)), atoi(arg));
		break;

	case OEDIT_SEXVALUE:
		if ((number = atoi(arg)) >= 0 && number < 4)
			GET_OBJ_SEX(OLC_OBJ(d)) = number;
		else
		{
			send_to_char("Пол (0-3) : ", d->character);
			return;
		}
		break;

	case OEDIT_MIWVALUE:
		if ((number = atoi(arg)) >= -1 && number <= 10000)
			GET_OBJ_MIW(OLC_OBJ(d)) = number;
		else
		{
			send_to_char("Максимальное число предметов в мире (0-10000 или -1) : ", d->character);
			return;
		}
		break;


	case OEDIT_MATER:
		number = atoi(arg);
		if (number < 0 || number > NUM_MATERIALS)
		{
			oedit_disp_mater_menu(d);
			return;
		}
		else if (number > 0)
			GET_OBJ_MATER(OLC_OBJ(d)) = number - 1;
		break;

	case OEDIT_COSTPERDAYEQ:
		GET_OBJ_RENTEQ(OLC_OBJ(d)) = atoi(arg);
		break;

	case OEDIT_TIMER:
		OLC_OBJ(d)->set_timer(atoi(arg));
		break;

	case OEDIT_SKILL:
		number = atoi(arg);
		if (number < 0)
		{
			oedit_disp_skills_menu(d);
			return;
		}
		if (number == 0)
			break;
		if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_INGRADIENT)
		{
			TOGGLE_BIT(GET_OBJ_SKILL(OLC_OBJ(d)), 1 << (number - 1));
			oedit_disp_skills_menu(d);
			return;
		}
		if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_WEAPON)
			switch (number)
			{
			case 1:
				number = 141;
				break;
			case 2:
				number = 142;
				break;
			case 3:
				number = 143;
				break;
			case 4:
				number = 144;
				break;
			case 5:
				number = 145;
				break;
			case 6:
				number = 146;
				break;
			case 7:
				number = 147;
				break;
			case 8:
				number = 148;
				break;
			case 9:
				number = 154;
				break;
			default:
				oedit_disp_skills_menu(d);
				return;
			}
		if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_DRINKCON)
			if (real_object(number) < 0)
			{
				send_to_char("Неверный номер объекта, повторите :\r\n ", d->character);
				oedit_disp_skills_menu(d);
				return;
			}
		GET_OBJ_SKILL(OLC_OBJ(d)) = number;
		oedit_disp_skills_menu(d);
		return;
		break;

	case OEDIT_VALUE_1:
		/*
		 * Lucky, I don't need to check any of these for out of range values.
		 * Hmm, I'm not so sure - Rv
		 */
		number = atoi(arg);

		if (GET_OBJ_TYPE(OLC_OBJ(d)) == ITEM_BOOK && (number < 0 || number > 4))
		{
			send_to_char("Неправильный тип книги, повторите.\r\n", d->character);
			oedit_disp_val1_menu(d);
			return;
		}
		/*
		 * proceed to menu 2
		 */
		GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
		OLC_VAL(d) = 1;
		oedit_disp_val2_menu(d);
		return;

	case OEDIT_VALUE_2:
		/*
		 * Here, I do need to check for out of range values.
		 */
		number = atoi(arg);
		switch (GET_OBJ_TYPE(OLC_OBJ(d)))
		{
		case ITEM_SCROLL:
		case ITEM_POTION:
			if (number < 0 || number >= LAST_USED_SPELL)
				oedit_disp_val2_menu(d);
			else
			{
				GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
				oedit_disp_val3_menu(d);
			}
			return;
		case ITEM_CONTAINER:
			/*
			 * Needs some special handling since we are dealing with flag values
			 * here.
			 */
			if (number < 0 || number > 4)
				oedit_disp_container_flags_menu(d);
			else if (number != 0)
			{
				TOGGLE_BIT(GET_OBJ_VAL(OLC_OBJ(d), 1), 1 << (number - 1));
				OLC_VAL(d) = 1;
				oedit_disp_val2_menu(d);
			}
			else
				oedit_disp_val3_menu(d);
			return;
		case ITEM_BOOK:
			switch (GET_OBJ_VAL(OLC_OBJ(d), 0))
			{
			case BOOK_SPELL:
				if (number == 0)
				{
					OLC_VAL(d) = 0;
					oedit_disp_menu(d);
					return;
				}
				if (number < 0 || (number > MAX_SPELLS || !spell_info[number].name || *spell_info[number].name == '!'))
				{
					send_to_char("Неизвестное заклинание, повторите.\r\n", d->character);
					oedit_disp_val2_menu(d);
					return;
				}
				break;
			case BOOK_SKILL:
			case BOOK_UPGRD:
				if (number == 0)
				{
					OLC_VAL(d) = 0;
					oedit_disp_menu(d);
					return;
				}
				if (number > MAX_SKILL_NUM|| !skill_info[number].name || *skill_info[number].name == '!')
				{
					send_to_char("Неизвестное умение, повторите.\r\n", d->character);
					oedit_disp_val2_menu(d);
					return;
				}
				break;
			case BOOK_RECPT:
				if (number > top_imrecipes || number < 0  || !imrecipes[number].name)
				{
					send_to_char("Неизвестный рецепт, повторите.\r\n", d->character);
					oedit_disp_val2_menu(d);
					return;
				}
				break;
			case BOOK_FEAT:
				if (number == 0)
				{
					OLC_VAL(d) = 0;
					oedit_disp_menu(d);
					return;
				}
				if (number > MAX_FEATS || !feat_info[number].name || *feat_info[number].name == '!')
				{
					send_to_char("Неизвестная способность, повторите.\r\n", d->character);
					oedit_disp_val2_menu(d);
					return;
				}
				break;
			}
		}
		GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
		OLC_VAL(d) = 1;
		oedit_disp_val3_menu(d);
		return;

	case OEDIT_VALUE_3:
		number = atoi(arg);
		/*
		 * Quick'n'easy error checking.
		 */
		switch (GET_OBJ_TYPE(OLC_OBJ(d)))
		{
		case ITEM_SCROLL:
		case ITEM_POTION:
			min_val = 0;
			max_val = LAST_USED_SPELL - 1;
			break;
		case ITEM_WEAPON:
			min_val = 1;
			max_val = 50;
		case ITEM_WAND:
		case ITEM_STAFF:
			min_val = 0;
			max_val = 20;
			break;
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
			min_val = 0;
			max_val = NUM_LIQ_TYPES - 1;
			break;
		case ITEM_MATERIAL:
			min_val = 0;
			max_val = 1000;
			break;
		default:
			min_val = -999999;
			max_val = 999999;
		}
		GET_OBJ_VAL(OLC_OBJ(d), 2) = MAX(min_val, MIN(number, max_val));
		OLC_VAL(d) = 1;
		oedit_disp_val4_menu(d);
		return;

	case OEDIT_VALUE_4:
		number = atoi(arg);
		switch (GET_OBJ_TYPE(OLC_OBJ(d)))
		{
		case ITEM_SCROLL:
		case ITEM_POTION:
			min_val = 0;
			max_val = LAST_USED_SPELL - 1;
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			min_val = 1;
			max_val = LAST_USED_SPELL - 1;
			break;
		case ITEM_WEAPON:
			min_val = 0;
			max_val = NUM_ATTACK_TYPES - 1;
			break;
		case ITEM_MING:
			min_val = 0;
			max_val = 2;
			break;
		case ITEM_MATERIAL:
			min_val = 0;
			max_val = 100;
			break;
		default:
			min_val = -999999;
			max_val = 999999;
			break;
		}
		GET_OBJ_VAL(OLC_OBJ(d), 3) = MAX(min_val, MIN(number, max_val));
		break;

	case OEDIT_AFFECTS:
		number = planebit(arg, &plane, &bit);
		if (number < 0)
		{
			oedit_disp_affects_menu(d);
			return;
		}
		else if (number == 0)
			break;
		else
		{
			TOGGLE_BIT(OLC_OBJ(d)->obj_flags.affects.flags[plane], 1 << (bit));
			oedit_disp_affects_menu(d);
			return;
		}

	case OEDIT_PROMPT_APPLY:
		if ((number = atoi(arg)) == 0)
			break;
		else if (number < 0 || number > MAX_OBJ_AFFECT)
		{
			oedit_disp_prompt_apply_menu(d);
			return;
		}
		OLC_VAL(d) = number - 1;
		OLC_MODE(d) = OEDIT_APPLY;
		oedit_disp_apply_menu(d);
		return;

	case OEDIT_APPLY:
		if ((number = atoi(arg)) == 0)
		{
			OLC_OBJ(d)->affected[OLC_VAL(d)].location = 0;
			OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = 0;
			oedit_disp_prompt_apply_menu(d);
		}
		else if (number < 0 || number >= NUM_APPLIES)
			oedit_disp_apply_menu(d);
		else
		{
			OLC_OBJ(d)->affected[OLC_VAL(d)].location = number;
			send_to_char("Modifier : ", d->character);
			OLC_MODE(d) = OEDIT_APPLYMOD;
		}
		return;

	case OEDIT_APPLYMOD:
		OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = atoi(arg);
		oedit_disp_prompt_apply_menu(d);
		return;

	case OEDIT_EXTRADESC_KEY:
		if (OLC_DESC(d)->keyword)
			free(OLC_DESC(d)->keyword);
		OLC_DESC(d)->keyword = str_dup(not_null(arg, NULL));
		oedit_disp_extradesc_menu(d);
		return;

	case OEDIT_EXTRADESC_MENU:
		switch ((number = atoi(arg)))
		{
		case 0:
			if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description)
			{
				EXTRA_DESCR_DATA **tmp_desc;

				if (OLC_DESC(d)->keyword)
					free(OLC_DESC(d)->keyword);
				if (OLC_DESC(d)->description)
					free(OLC_DESC(d)->description);

				/*
				 * Clean up pointers
				 */
				for (tmp_desc = &(OLC_OBJ(d)->ex_description); *tmp_desc;
						tmp_desc = &((*tmp_desc)->next))
				{
					if (*tmp_desc == OLC_DESC(d))
					{
						*tmp_desc = NULL;
						break;
					}
				}
				free(OLC_DESC(d));
			}
			break;

		case 1:
			OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
			send_to_char("Enter keywords, separated by spaces :-\r\n| ", d->character);
			return;

		case 2:
			OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
			SEND_TO_Q("Enter the extra description: (/s saves /h for help)\r\n\r\n", d);
			d->backstr = NULL;
			if (OLC_DESC(d)->description)
			{
				SEND_TO_Q(OLC_DESC(d)->description, d);
				d->backstr = str_dup(OLC_DESC(d)->description);
			}
			d->str = &OLC_DESC(d)->description;
			d->max_str = 4096;
			d->mail_to = 0;
			OLC_VAL(d) = 1;
			return;

		case 3:
			/*
			 * Only go to the next description if this one is finished.
			 */
			if (OLC_DESC(d)->keyword && OLC_DESC(d)->description)
			{
				EXTRA_DESCR_DATA *new_extra;

				if (OLC_DESC(d)->next)
					OLC_DESC(d) = OLC_DESC(d)->next;
				else  	/* Make new extra description and attach at end. */
				{
					CREATE(new_extra, EXTRA_DESCR_DATA, 1);
					OLC_DESC(d)->next = new_extra;
					OLC_DESC(d) = OLC_DESC(d)->next;
				}
			}
			/*
			 * No break - drop into default case.
			 */
		default:
			oedit_disp_extradesc_menu(d);
			return;
		}
		break;
	case OEDIT_SKILLS:
		number = atoi(arg);
		if (number == 0)
			break;
		if (number > MAX_SKILL_NUM || !skill_info[number].name || *skill_info[number].name == '!')
			send_to_char("Неизвестное умение.\r\n", d->character);
		else if (OLC_OBJ(d)->get_skill(number) != 0)
			OLC_OBJ(d)->set_skill(number, 0);
		else if (sscanf(arg, "%d %d", &plane, &bit) < 2)
			send_to_char("Не указан уровень владения умением.\r\n", d->character);
		else
			OLC_OBJ(d)->set_skill(number, bit);
		oedit_disp_skills_mod_menu(d);
		return;
	case OEDIT_MORT_REQ:
		number = atoi(arg);
		if (number < 0)
			send_to_char("Число не должно быть меньше нуля.\r\n", d->character);
		else
			OLC_OBJ(d)->set_mort_req(number);
		break;

	default:
		mudlog("SYSERR: OLC: Reached default case in oedit_parse()!", BRF, LVL_BUILDER, SYSLOG, TRUE);
		send_to_char("Oops...\r\n", d->character);
		break;
	}

	/*
	 * If we get here, we have changed something.
	 */
	OLC_VAL(d) = 1;
	oedit_disp_menu(d);
}
