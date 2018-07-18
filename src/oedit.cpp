/************************************************************************
 * OasisOLC - oedit.c						v1.5	*
 * Copyright 1996 Harvey Gilpin.					*
 * Original author: Levork						*
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
 ************************************************************************/

#include "world.objects.hpp"
#include "object.prototypes.hpp"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "spells.h"
#include "logger.hpp"
#include "utils.h"
#include "db.h"
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
#include "obj.hpp"

#include <array>
#include <vector>
#include <stack>

// * External variable declarations.
extern struct zone_data *zone_table;
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
extern const char *magic_container_bits[];
extern struct spell_info_type spell_info[];
extern DESCRIPTOR_DATA *descriptor_list;
extern int top_imrecipes;
extern void extract_obj(OBJ_DATA * obj);
int real_zone(int number);

//------------------------------------------------------------------------

// * Handy macros.
#define S_PRODUCT(s, i) ((s)->producing[(i)])

//------------------------------------------------------------------------
void oedit_setup(DESCRIPTOR_DATA * d, int real_num);

void oedit_object_copy(OBJ_DATA * dst, CObjectPrototype* src);

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

// ------------------------------------------------------------------------
//  Utility and exported functions
// ------------------------------------------------------------------------
//***********************************************************************

void oedit_setup(DESCRIPTOR_DATA * d, int real_num)
/*++
   Подготовка данных для редактирования объекта.
      d        - OLC дескриптор
      real_num - RNUM исходного объекта, новый -1
--*/
{
	OBJ_DATA *obj;
	const auto vnum = OLC_NUM(d);

	NEWCREATE(obj, vnum);

	if (real_num == -1)
	{
		obj->set_aliases("новый предмет");
		obj->set_description("что-то новое лежит здесь");
		obj->set_short_description("новый предмет");
		obj->set_PName(0, "это что");
		obj->set_PName(1, "нету чего");
		obj->set_PName(2, "привязать к чему");
		obj->set_PName(3, "взять что");
		obj->set_PName(4, "вооружиться чем");
		obj->set_PName(5, "говорить о чем");
		obj->set_wear_flags(to_underlying(EWearFlag::ITEM_WEAR_TAKE));
	}
	else
	{
		obj->clone_olc_object_from_prototype(vnum);
		obj->set_rnum(real_num);
	}

	OLC_OBJ(d) = obj;
	OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
	oedit_disp_menu(d);
	OLC_VAL(d) = 0;
}

// * Обновление данных у конкретной шмотки (для update_online_objects).
void olc_update_object(int robj_num, OBJ_DATA *obj, OBJ_DATA *olc_proto)
{
	// Итак, нашел объект
	// Внимание! Таймер объекта, его состояние и т.д. обновятся!

	// Удаляю его строки и т.д.
	// прототип скрипта не удалится, т.к. его у экземпляра нету
	// скрипт не удалится, т.к. его не удаляю

	bool fullUpdate = true; //флажок если дальше делать выборочные шаги
	/*if (obj->get_crafter_uid()) //Если шмотка крафченная
		fullUpdate = false;*/

	//если объект не зависит от прототипа
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_NOT_DEPEND_RPOTO)) 
		fullUpdate = false;
	//если объект изменен кодом
	if (OBJ_FLAGGED(obj, EExtraFlag::ITEM_TRANSFORMED))
		fullUpdate = false;
	
	if (!fullUpdate) {
		//тут можно вставить изменение объекта ограниченное
		//в obj лежит объект, в olc_proto лежит прототип
		return;
	}

	
	// Сохраняю текущую игровую информацию	
	OBJ_DATA tmp(*obj);
	
	// Копируем информацию из прототипа
	*obj = *olc_proto;
	
	//Восстанавливаем падежи если объект поренеймлен
	if (tmp.get_is_rename()) {
		obj->copy_name_from(&tmp);
		obj->set_is_rename(true);
	}

	obj->clear_proto_script();
	// Восстанавливаю игровую информацию
	obj->set_uid(tmp.get_uid());
	obj->set_id(tmp.get_id()); // аук работает не по рнум а по id объекта, поэтому вернем и его
	obj->set_in_room(tmp.get_in_room());
	obj->set_rnum(robj_num);
	obj->set_owner(tmp.get_owner());
	obj->set_crafter_uid(tmp.get_crafter_uid());
	obj->set_parent(tmp.get_parent());
	obj->set_carried_by(tmp.get_carried_by());
	obj->set_worn_by(tmp.get_worn_by());
	obj->set_worn_on(tmp.get_worn_on());
	obj->set_in_obj(tmp.get_in_obj());
	obj->set_contains(tmp.get_contains());
	obj->set_next_content(tmp.get_next_content());
	obj->set_next(tmp.get_next());
	obj->set_script(tmp.get_script());
	// для name_list
	obj->set_serial_num(tmp.get_serial_num());
	obj->set_current_durability(GET_OBJ_CUR(&tmp));
//	если таймер шмота в мире меньше  чем установленный, восстанавливаем его.
	if (obj->get_timer() > tmp.get_timer())
	{
		obj->set_timer(tmp.get_timer());
	}
	// емкостям сохраняем жидкость и кол-во глотков, во избежание жалоб
	if (GET_OBJ_TYPE(&tmp) == OBJ_DATA::ITEM_DRINKCON
		&& GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON)
	{
		obj->set_val(1, GET_OBJ_VAL(&tmp, 1)); //кол-во глотков
		if (is_potion(&tmp))
		{
			obj->set_val(2, GET_OBJ_VAL(&tmp, 2)); //описание жидкости
		}
		// сохранение в случае перелитых заклов
		// пока там ничего кроме заклов и нет - копируем весь values
		if (tmp.get_value(ObjVal::EValueKey::POTION_PROTO_VNUM) > 0)
		{
			obj->set_values(tmp.get_all_values());
		}
	}
	if (tmp.get_extra_flag(EExtraFlag::ITEM_TICKTIMER))//если у старого объекта запущен таймер
	{
		obj->set_extra_flag(EExtraFlag::ITEM_TICKTIMER);//ставим флаг таймер запущен
	}
	if (tmp.get_extra_flag(EExtraFlag::ITEM_NAMED))//если у старого объекта стоит флаг именной предмет
	{
		obj->set_extra_flag(EExtraFlag::ITEM_NAMED);//ставим флаг именной предмет
	}
}

// * Обновление полей объектов при изменении их прототипа через олц.
void olc_update_objects(int robj_num, OBJ_DATA *olc_proto)
{
	world_objects.foreach_with_rnum(robj_num, [&](const OBJ_DATA::shared_ptr& object)
	{
		olc_update_object(robj_num, object.get(), olc_proto);
	});
	Depot::olc_update_from_proto(robj_num, olc_proto);
	Parcel::olc_update_from_proto(robj_num, olc_proto);
}

//------------------------------------------------------------------------

#define ZCMD zone_table[zone].cmd[cmd_no]

void oedit_save_internally(DESCRIPTOR_DATA * d)
{
	int robj_num;

	robj_num = GET_OBJ_RNUM(OLC_OBJ(d));
	ObjSystem::init_ilvl(OLC_OBJ(d));
	// * Write object to internal tables.
	if (robj_num >= 0)
	{	/*
		 * We need to run through each and every object currently in the
		 * game to see which ones are pointing to this prototype.
		 * if object is pointing to this prototype, then we need to replace it
		 * with the new one.
		 */
		log("[OEdit] Save object to mem %d", GET_OBJ_VNUM(OLC_OBJ(d)));
		olc_update_objects(robj_num, OLC_OBJ(d));

		// Все существующие в мире объекты обновлены согласно новому прототипу
		// Строки в этих объектах как ссылки на данные прототипа

		// Копирую новый прототип в массив
		// Использовать функцию oedit_object_copy() нельзя,
		// т.к. будут изменены указатели на данные прототипа
		obj_proto.set(robj_num, OLC_OBJ(d));	// old prototype will be deleted automatically
		// OLC_OBJ(d) удалять не нужно, т.к. он перенесен в массив
		// прототипов
	}
	else
	{
		// It's a new object, we must build new tables to contain it.
		log("[OEdit] Save mem new %d(%zd/%zd)", OLC_NUM(d), 1 + obj_proto.size(), sizeof(OBJ_DATA));

		const size_t index = obj_proto.add(OLC_OBJ(d), OLC_NUM(d));
		obj_proto.zone(index, real_zone(OLC_NUM(d)));
	}

	olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_OBJ);
}

//------------------------------------------------------------------------

void oedit_save_to_disk(int zone_num)
{
	int counter, counter2, realcounter;
	FILE *fp;

	sprintf(buf, "%s/%d.new", OBJ_PREFIX, zone_table[zone_num].number);
	if (!(fp = fopen(buf, "w+")))
	{
		mudlog("SYSERR: OLC: Cannot open objects file!", BRF, LVL_BUILDER, SYSLOG, TRUE);
		return;
	}
	// * Start running through all objects in this zone.
	for (counter = zone_table[zone_num].number * 100; counter <= zone_table[zone_num].top; counter++)
	{
		if ((realcounter = real_object(counter)) >= 0)
		{
			const auto& obj = obj_proto[realcounter];
			if (!obj->get_action_description().empty())
			{
				strcpy(buf1, obj->get_action_description().c_str());
				strip_string(buf1);
			}
			else
			{
				*buf1 = '\0';
			}
			*buf2 = '\0';
			GET_OBJ_AFFECTS(obj).tascii(4, buf2);
			GET_OBJ_ANTI(obj).tascii(4, buf2);
			GET_OBJ_NO(obj).tascii(4, buf2);
			sprintf(buf2 + strlen(buf2), "\n%d ", GET_OBJ_TYPE(obj));
			GET_OBJ_EXTRA(obj).tascii(4, buf2);
			const auto wear_flags = GET_OBJ_WEAR(obj);
			tascii(&wear_flags, 1, buf2);
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
				obj->get_vnum(),
				!obj->get_aliases().empty() ? obj->get_aliases().c_str() : "undefined",
				!obj->get_PName(0).empty() ? obj->get_PName(0).c_str() : "что-то",
				!obj->get_PName(1).empty() ? obj->get_PName(1).c_str() : "чего-то",
				!obj->get_PName(2).empty() ? obj->get_PName(2).c_str() : "чему-то",
				!obj->get_PName(3).empty() ? obj->get_PName(3).c_str() : "что-то",
				!obj->get_PName(4).empty() ? obj->get_PName(4).c_str() : "чем-то",
				!obj->get_PName(5).empty() ? obj->get_PName(5).c_str() : "о чем-то",
				!obj->get_description().empty() ? obj->get_description().c_str() : "undefined",
				buf1,
				GET_OBJ_SKILL(obj), GET_OBJ_MAX(obj), GET_OBJ_CUR(obj),
				GET_OBJ_MATER(obj), static_cast<int>(GET_OBJ_SEX(obj)),
				obj->get_timer(), GET_OBJ_SPELL(obj),
				GET_OBJ_LEVEL(obj), buf2, GET_OBJ_VAL(obj, 0),
				GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
				GET_OBJ_VAL(obj, 3), GET_OBJ_WEIGHT(obj),
				GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_RENTEQ(obj));

			script_save_to_disk(fp, obj.get(), OBJ_TRIGGER);

			if (GET_OBJ_MIW(obj))
			{
				fprintf(fp, "M %d\n", GET_OBJ_MIW(obj));
			}

			if (obj->get_minimum_remorts() != 0)
			{
				fprintf(fp, "R %d\n", obj->get_minimum_remorts());
			}

			// * Do we have extra descriptions?
			if (obj->get_ex_description())  	// Yes, save them too.
			{
				for (auto ex_desc = obj->get_ex_description(); ex_desc; ex_desc = ex_desc->next)
				{
					// * Sanity check to prevent nasty protection faults.
					if (!ex_desc->keyword
						|| !ex_desc->description)
					{
						mudlog("SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!",
							BRF, LVL_BUILDER, SYSLOG, TRUE);
						continue;
					}
					strcpy(buf1, ex_desc->description);
					strip_string(buf1);
					fprintf(fp, "E\n" "%s~\n" "%s~\n", ex_desc->keyword, buf1);
				}
			}
			// * Do we have affects?
			for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
			{
				if (obj->get_affected(counter2).location
					&& obj->get_affected(counter2).modifier)
				{
					fprintf(fp, "A\n%d %d\n",
						obj->get_affected(counter2).location,
						obj->get_affected(counter2).modifier);
				}
			}

			if (obj->has_skills())
			{
				CObjectPrototype::skills_t skills;
				obj->get_skills(skills);
				for (const auto& it : skills)
				{
					fprintf(fp, "S\n%d %d\n", it.first, it.second);
				}
			}

			// ObjVal
			const auto values = obj->get_all_values().print_to_zone();
			if (!values.empty())
			{
				fprintf(fp, "%s", values.c_str());
			}
		}
	}

	// * Write the final line, close the file.
	fprintf(fp, "$\n$\n");
	fclose(fp);
	sprintf(buf2, "%s/%d.obj", OBJ_PREFIX, zone_table[zone_num].number);
	// * We're fubar'd if we crash between the two lines below.
	remove(buf2);
	rename(buf, buf2);

	olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_OBJ);
}

// **************************************************************************
// * Menu functions                                                         *
// **************************************************************************

// * For container flags.
void oedit_disp_container_flags_menu(DESCRIPTOR_DATA * d)
{
	get_char_cols(d->character.get());
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
	send_to_char(buf, d->character.get());
}

// * For extra descriptions.
void oedit_disp_extradesc_menu(DESCRIPTOR_DATA * d)
{
	auto extra_desc = OLC_DESC(d);

	strcpy(buf1, !extra_desc->next ? "<Not set>\r\n" : "Set.");

	get_char_cols(d->character.get());
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
	send_to_char(buf, d->character.get());
	OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

// * Ask for *which* apply to edit.
void oedit_disp_prompt_apply_menu(DESCRIPTOR_DATA * d)
{
	int counter;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < MAX_OBJ_AFFECT; counter++)
	{
		if (OLC_OBJ(d)->get_affected(counter).modifier)
		{
			sprinttype(OLC_OBJ(d)->get_affected(counter).location, apply_types, buf2);
			sprintf(buf, " %s%d%s) %+d to %s\r\n", grn, counter + 1, nrm,
				OLC_OBJ(d)->get_affected(counter).modifier, buf2);
			send_to_char(buf, d->character.get());
		}
		else
		{
			sprintf(buf, " %s%d%s) Ничего.\r\n", grn, counter + 1, nrm);
			send_to_char(buf, d->character.get());
		}
	}
	send_to_char("\r\nВыберите изменяемый аффект (0 - выход) : ", d->character.get());
	OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

// * Ask for liquid type.
void oedit_liquid_type(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_LIQ_TYPES; counter++)
	{
		sprintf(buf, " %s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
			drinks[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	sprintf(buf, "\r\n%sВыберите тип жидкости : ", nrm);
	send_to_char(buf, d->character.get());
	OLC_MODE(d) = OEDIT_VALUE_3;
}

void show_apply_olc(DESCRIPTOR_DATA *d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_APPLIES; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
				apply_types[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	send_to_char("\r\nЧто добавляем (0 - выход) : ", d->character.get());
}

// * The actual apply to set.
void oedit_disp_apply_menu(DESCRIPTOR_DATA * d)
{
	show_apply_olc(d);
	OLC_MODE(d) = OEDIT_APPLY;
}

// * Weapon type.
void oedit_disp_weapon_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ATTACK_TYPES; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
				attack_hit_text[counter].singular, !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	send_to_char("\r\nВыберите тип удара (0 - выход): ", d->character.get());
}

// * Spell type.
void oedit_disp_spells_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < MAX_SPELLS; counter++)
	{
		if (!spell_info[counter].name || *spell_info[counter].name == '!')
			continue;
		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	sprintf(buf, "\r\n%sВыберите магию (0 - выход) : ", nrm);
	send_to_char(buf, d->character.get());
}

void oedit_disp_skills2_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < MAX_SKILL_NUM; counter++)
	{
		if (!skill_info[counter].name || *skill_info[counter].name == '!')
		{
			continue;
		}

		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				skill_info[counter].name, !(++columns % 3) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	sprintf(buf, "\r\n%sВыберите умение (0 - выход) : ", nrm);
	send_to_char(buf, d->character.get());
}

void oedit_disp_receipts_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter <= top_imrecipes; counter++)
	{
		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				imrecipes[counter].name, !(++columns % 3) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	sprintf(buf, "\r\n%sВыберите рецепт : ", nrm);
	send_to_char(buf, d->character.get());
}

void oedit_disp_feats_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 1; counter < MAX_FEATS; counter++)
	{
		if (!feat_info[counter].name || *feat_info[counter].name == '!')
		{
			continue;
		}

		sprintf(buf, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
				feat_info[counter].name, !(++columns % 3) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	sprintf(buf, "\r\n%sВыберите способность (0 - выход) : ", nrm);
	send_to_char(buf, d->character.get());
}

void oedit_disp_skills_mod_menu(DESCRIPTOR_DATA* d)
{
	int columns = 0, counter;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	int percent;
	for (counter = 1; counter <= MAX_SKILL_NUM; ++counter)
	{
		if (!skill_info[counter].name || *skill_info[counter].name == '!')
		{
			continue;
		}

		percent = OLC_OBJ(d)->get_skill(counter);
		if (percent != 0)
		{
			sprintf(buf1, "%s[%3d]%s", cyn, percent, nrm);
		}
		else
		{
			strcpy(buf1, "     ");
		}
		sprintf(buf, "%s%3d%s) %25s%s%s", grn, counter, nrm,
				skill_info[counter].name, buf1, !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	send_to_char("\r\nУкажите номер и уровень владения умением (0 - конец) : ", d->character.get());
}

// * Object value #1
void oedit_disp_val1_menu(DESCRIPTOR_DATA * d)
{
	OLC_MODE(d) = OEDIT_VALUE_1;
	switch (GET_OBJ_TYPE(OLC_OBJ(d)))
	{
	case OBJ_DATA::ITEM_LIGHT:
		// * values 0 and 1 are unused.. jump to 2
		oedit_disp_val3_menu(d);
		break;

	case OBJ_DATA::ITEM_SCROLL:
	case OBJ_DATA::ITEM_WAND:
	case OBJ_DATA::ITEM_STAFF:
	case OBJ_DATA::ITEM_POTION:
		send_to_char("Уровень заклинания : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_WEAPON:
		// * This doesn't seem to be used if I remember right.
		send_to_char("Модификатор попадания : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_ARMOR:
	case OBJ_DATA::ITEM_ARMOR_LIGHT:
	case OBJ_DATA::ITEM_ARMOR_MEDIAN:
	case OBJ_DATA::ITEM_ARMOR_HEAVY:
		send_to_char("Изменяет АС на : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_CONTAINER:
		send_to_char("Максимально вместимый вес : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_DRINKCON:
	case OBJ_DATA::ITEM_FOUNTAIN:
		send_to_char("Количество глотков : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_FOOD:
		send_to_char("На сколько часов насыщает : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_MONEY:
		send_to_char("Сумма : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_NOTE:
		// * This is supposed to be language, but it's unused.
		break;

	case OBJ_DATA::ITEM_BOOK:
		sprintf(buf,
				"%s0%s) %sКнига заклинаний\r\n"
				"%s1%s) %sКнига умений\r\n"
				"%s2%s) %sУлучшение умения\r\n"
				"%s3%s) %sКнига рецептов\r\n"
				"%s4%s) %sКнига способностей\r\n"
				"%sВыберите тип книги : ", grn, nrm, yel, grn, nrm, yel, grn, nrm, yel, grn, nrm, yel, grn, nrm, yel, nrm);
		send_to_char(buf, d->character.get());
		break;

	case OBJ_DATA::ITEM_INGREDIENT:
		send_to_char("Первый байт - лаг после применения в сек, 5 бит - уровень : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_MING:
		oedit_disp_val4_menu(d);
		break;

	case OBJ_DATA::ITEM_MATERIAL:
		send_to_char("Уровень игрока для использования + морт * 2: ", d->character.get());
		break;

	case OBJ_DATA::ITEM_BANDAGE:
		send_to_char("Хитов в секунду: ", d->character.get());
		break;

	case OBJ_DATA::ITEM_ENCHANT:
		send_to_char("Изменяет вес: ", d->character.get());
		break;
	case OBJ_DATA::ITEM_MAGIC_CONTAINER:
	case OBJ_DATA::ITEM_MAGIC_ARROW:
		oedit_disp_spells_menu(d);
		break;

	default:
		oedit_disp_menu(d);
	}
}

// * Object value #2
void oedit_disp_val2_menu(DESCRIPTOR_DATA * d)
{
	OLC_MODE(d) = OEDIT_VALUE_2;
	switch (GET_OBJ_TYPE(OLC_OBJ(d)))
	{
	case OBJ_DATA::ITEM_SCROLL:
	case OBJ_DATA::ITEM_POTION:
		oedit_disp_spells_menu(d);
		break;

	case OBJ_DATA::ITEM_WAND:
	case OBJ_DATA::ITEM_STAFF:
		send_to_char("Количество зарядов : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_WEAPON:
		send_to_char("Количество бросков кубика : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_ARMOR:
	case OBJ_DATA::ITEM_ARMOR_LIGHT:
	case OBJ_DATA::ITEM_ARMOR_MEDIAN:
	case OBJ_DATA::ITEM_ARMOR_HEAVY:
		send_to_char("Изменяет броню на : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_FOOD:
		// * Values 2 and 3 are unused, jump to 4...Odd.
		oedit_disp_val4_menu(d);
		break;
	
	case OBJ_DATA::ITEM_MONEY:
		sprintf(buf,
				"%s0%s) %sКуны\r\n"
				"%s1%s) %sСлава\r\n"
				"%s2%s) %sГривны\r\n"
				"%s3%s) %sСнежинки\r\n"
				"%sВыберите тип валюты : ", 
					grn, nrm, yel,
					grn, nrm, yel,
					grn, nrm, yel,
					grn, nrm, yel,
					nrm);
		send_to_char(buf, d->character.get());
		break;

	case OBJ_DATA::ITEM_CONTAINER:
		// * These are flags, needs a bit of special handling.
		oedit_disp_container_flags_menu(d);
		break;

	case OBJ_DATA::ITEM_DRINKCON:
	case OBJ_DATA::ITEM_FOUNTAIN:
		send_to_char("Начальное количество глотков : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_BOOK:
		switch (GET_OBJ_VAL(OLC_OBJ(d), 0))
		{
		case BOOK_SPELL:
			oedit_disp_spells_menu(d);
			break;

		case BOOK_SKILL:
		case BOOK_UPGRD:
			oedit_disp_skills2_menu(d);
			break;

		case BOOK_RECPT:
			oedit_disp_receipts_menu(d);
			break;

		case BOOK_FEAT:
			oedit_disp_feats_menu(d);
			break;

		default:
			oedit_disp_val4_menu(d);
		}
		break;

	case OBJ_DATA::ITEM_INGREDIENT:
		send_to_char("Виртуальный номер прототипа  : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_MATERIAL:
		send_to_char("Введите VNUM прототипа: ", d->character.get());
		break;
	
	case OBJ_DATA::ITEM_MAGIC_CONTAINER:
		send_to_char("Объем колчана: ", d->character.get());
		break;

	case OBJ_DATA::ITEM_MAGIC_ARROW:
		send_to_char("Размер пучка: ", d->character.get());
		break;

	default:
		oedit_disp_menu(d);
	}
}

// * Object value #3
void oedit_disp_val3_menu(DESCRIPTOR_DATA * d)
{
	OLC_MODE(d) = OEDIT_VALUE_3;
	switch (GET_OBJ_TYPE(OLC_OBJ(d)))
	{
	case OBJ_DATA::ITEM_LIGHT:
		send_to_char("Длительность горения (0 = погасла, -1 - вечный свет) : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_SCROLL:
	case OBJ_DATA::ITEM_POTION:
		oedit_disp_spells_menu(d);
		break;

	case OBJ_DATA::ITEM_WAND:
	case OBJ_DATA::ITEM_STAFF:
		send_to_char("Осталось зарядов : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_WEAPON:
		send_to_char("Количество граней кубика : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_CONTAINER:
		send_to_char("Vnum ключа для контейнера (-1 - нет ключа) : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_DRINKCON:
	case OBJ_DATA::ITEM_FOUNTAIN:
		oedit_liquid_type(d);
		break;

	case OBJ_DATA::ITEM_BOOK:
//		send_to_char("Уровень изучения (+ к умению если тип = 2 ) : ", d->character);
		switch (GET_OBJ_VAL(OLC_OBJ(d), 0))
		{
		case BOOK_SKILL:
			send_to_char("Введите уровень изучения : ", d->character.get());
			break;
		case BOOK_UPGRD:
			send_to_char("На сколько увеличится умение : ", d->character.get());
			break;
		default:
			oedit_disp_val4_menu(d);
		}
		break;

	case OBJ_DATA::ITEM_INGREDIENT:
		send_to_char("Сколько раз можно использовать : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_MATERIAL:
		send_to_char("Введите силу ингридиента: ", d->character.get());
		break;

	case OBJ_DATA::ITEM_MAGIC_CONTAINER:
        case OBJ_DATA::ITEM_MAGIC_ARROW:
		send_to_char("Количество стрел: ", d->character.get());
		break;

	default:
		oedit_disp_menu(d);
	}
}

// * Object value #4
void oedit_disp_val4_menu(DESCRIPTOR_DATA * d)
{
	OLC_MODE(d) = OEDIT_VALUE_4;
	switch (GET_OBJ_TYPE(OLC_OBJ(d)))
	{
	case OBJ_DATA::ITEM_SCROLL:
	case OBJ_DATA::ITEM_POTION:
	case OBJ_DATA::ITEM_WAND:
	case OBJ_DATA::ITEM_STAFF:
		oedit_disp_spells_menu(d);
		break;

	case OBJ_DATA::ITEM_WEAPON:
		oedit_disp_weapon_menu(d);
		break;

	case OBJ_DATA::ITEM_DRINKCON:
	case OBJ_DATA::ITEM_FOUNTAIN:
	case OBJ_DATA::ITEM_FOOD:
		send_to_char("Отравлено (0 - не отравлено, 1 - отравлено, >1 - таймер) : ", d->character.get());
		break;

	case OBJ_DATA::ITEM_BOOK:
		switch (GET_OBJ_VAL(OLC_OBJ(d), 0))
		{
		case BOOK_UPGRD:
			send_to_char("Максимальный % умения :\r\n"
					"Если <= 0, то учитывается только макс. возможный навык игрока на данном реморте.\r\n"
					"Если > 0, то учитывается только данное значение без учета макс. навыка игрока.\r\n"
					, d->character.get());
			break;

		default:
			OLC_VAL(d) = 1;
			oedit_disp_menu(d);
		}
		break;

	case OBJ_DATA::ITEM_MING:
		send_to_char("Класс ингредиента (0-РОСЛЬ,1-ЖИВЬ,2-ТВЕРДЬ): ", d->character.get());
		break;

	case OBJ_DATA::ITEM_MATERIAL:
		send_to_char("Введите условный уровень: ", d->character.get());
		break;

	case OBJ_DATA::ITEM_CONTAINER:
		send_to_char("Введите сложность замка (0-255): ", d->character.get());
		break;

	default:
		oedit_disp_menu(d);
	}
}

// * Object type.
void oedit_disp_type_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ITEM_TYPES; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
				item_types[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	send_to_char("\r\nВыберите тип предмета : ", d->character.get());
}

// * Object extra flags.
void oedit_disp_extra_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0, plane = 0;
	char c;

	get_char_cols(d->character.get());
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
		send_to_char(buf, d->character.get());
	}

	GET_OBJ_EXTRA(OLC_OBJ(d)).sprintbits(extra_bits, buf1, ",", 5);
	sprintf(buf, "\r\nЭкстрафлаги: %s%s%s\r\n" "Выберите экстрафлаг (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character.get());
}

void oedit_disp_anti_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0, plane = 0;
	char c;

	get_char_cols(d->character.get());
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
		send_to_char(buf, d->character.get());
	}
	OLC_OBJ(d)->get_anti_flags().sprintbits(anti_bits, buf1, ",", 5);
	sprintf(buf, "\r\nПредмет запрещен для : %s%s%s\r\n" "Выберите флаг запрета (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character.get());
}

void oedit_disp_no_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0, plane = 0;
	char c;

	get_char_cols(d->character.get());
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
		send_to_char(buf, d->character.get());
	}
	OLC_OBJ(d)->get_no_flags().sprintbits(no_bits, buf1, ",", 5);
	sprintf(buf, "\r\nПредмет неудобен для : %s%s%s\r\n" "Выберите флаг неудобств (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character.get());
}

void show_weapon_affects_olc(DESCRIPTOR_DATA *d, const FLAG_DATA &flags)
{
	int counter, columns = 0, plane = 0;
	char c;

	get_char_cols(d->character.get());
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
		send_to_char(buf, d->character.get());
	}
	flags.sprintbits(weapon_affects, buf1, ",", 5);
	sprintf(buf,
		"\r\nНакладываемые аффекты : %s%s%s\r\n"
		"Выберите аффект (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character.get());
}

void oedit_disp_affects_menu(DESCRIPTOR_DATA * d)
{
	show_weapon_affects_olc(d, OLC_OBJ(d)->get_affect_flags());
}

// * Object wear flags.
void oedit_disp_wear_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ITEM_WEARS; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				wear_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	sprintbit(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, buf1);
	sprintf(buf, "\r\nМожет быть одет : %s%s%s\r\n" "Выберите позицию (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character.get());
}

void oedit_disp_mater_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < 32 && *material_name[counter] != '\n'; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				material_name[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	sprintf(buf, "\r\nСделан из : %s%s%s\r\n"
			"Выберите материал (0 - выход) : ", cyn, material_name[GET_OBJ_MATER(OLC_OBJ(d))], nrm);
	send_to_char(buf, d->character.get());
}

void oedit_disp_ingradient_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < 32 && *ingradient_bits[counter] != '\n'; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				ingradient_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	sprintbit(GET_OBJ_SKILL(OLC_OBJ(d)), ingradient_bits, buf1);
	sprintf(buf, "\r\nТип ингредиента : %s%s%s\r\n" "Дополните тип (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character.get());
}

void oedit_disp_magic_container_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

	get_char_cols(d->character.get());

	for (counter = 0; counter < 32 && *magic_container_bits[counter] != '\n'; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
				magic_container_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	sprintbit(GET_OBJ_SKILL(OLC_OBJ(d)), magic_container_bits, buf1);
	sprintf(buf, "\r\nТип контейнера : %s%s%s\r\n" "Дополните тип (0 - выход) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character.get());
}

std::string print_spell_value(OBJ_DATA *obj, const ObjVal::EValueKey key1, const ObjVal::EValueKey key2)
{
	if (obj->get_value(key1) < 0)
	{
		return "нет";
	}
	char buf_[MAX_INPUT_LENGTH];
	snprintf(buf_, sizeof(buf_), "%s:%d", spell_name(obj->get_value(key1)), obj->get_value(key2));
	return buf_;
}

void drinkcon_values_menu(DESCRIPTOR_DATA *d)
{
	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif

	char buf_[1024];
	snprintf(buf_, sizeof(buf_),
		"%s1%s) Заклинание1 : %s%s\r\n"
		"%s2%s) Заклинание2 : %s%s\r\n"
		"%s3%s) Заклинание3 : %s%s\r\n"
		"%s",
		grn, nrm, cyn,
		print_spell_value(OLC_OBJ(d),
			ObjVal::EValueKey::POTION_SPELL1_NUM,
			ObjVal::EValueKey::POTION_SPELL1_LVL).c_str(),
		grn, nrm, cyn,
		print_spell_value(OLC_OBJ(d),
			ObjVal::EValueKey::POTION_SPELL2_NUM,
			ObjVal::EValueKey::POTION_SPELL2_LVL).c_str(),
		grn, nrm, cyn,
		print_spell_value(OLC_OBJ(d),
			ObjVal::EValueKey::POTION_SPELL3_NUM,
			ObjVal::EValueKey::POTION_SPELL3_LVL).c_str(),
		nrm);

	send_to_char(buf_, d->character.get());
	send_to_char("Ваш выбор (0 - выход) :", d->character.get());
	return;
}

std::array<const char *, 9> wskill_bits =
{{
	"палицы и дубины(141)",
	"секиры(142)",
	"длинные лезвия(143)",
	"короткие лезвия(144)",
	"иное(145)",
	"двуручники(146)",
	"проникающее(147)",
	"копья и рогатины(148)",
	"луки(154)"
}};

void oedit_disp_skills_menu(DESCRIPTOR_DATA * d)
{
	if (GET_OBJ_TYPE(OLC_OBJ(d)) == OBJ_DATA::ITEM_INGREDIENT)
	{
		oedit_disp_ingradient_menu(d);
		return;
	}
	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	int columns = 0;
	for (size_t counter = 0; counter < wskill_bits.size(); counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s",
				grn,
				static_cast<int>(counter + 1),
				nrm,
				wskill_bits[counter],
				!(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	sprintf(buf,
		"%sТренируемое умение : %s%d%s\r\n"
		"Выберите умение (0 - выход) : ",
		(columns%2 == 1?"\r\n":""), cyn, GET_OBJ_SKILL(OLC_OBJ(d)), nrm);
	send_to_char(buf, d->character.get());
}

std::string print_values2_menu(OBJ_DATA *obj)
{
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_DRINKCON
		|| GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_FOUNTAIN)
	{
		return "Спец.параметры";
	}

	char buf_[MAX_INPUT_LENGTH];
	snprintf(buf_, sizeof(buf_), "Skill       : %d", GET_OBJ_SKILL(obj));
	return buf_;
}

// * Display main menu.
void oedit_disp_menu(DESCRIPTOR_DATA * d)
{
	OBJ_DATA *obj;

	obj = OLC_OBJ(d);
	get_char_cols(d->character.get());

	sprinttype(GET_OBJ_TYPE(obj), item_types, buf1);
	GET_OBJ_EXTRA(obj).sprintbits(extra_bits, buf2, ",",4);

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
		grn, nrm, yel, not_empty(obj->get_aliases()),
		grn, not_empty(obj->get_PName(0)),
		grn, not_empty(obj->get_PName(1)),
		grn, not_empty(obj->get_PName(2)),
		grn, not_empty(obj->get_PName(3)),
		grn, not_empty(obj->get_PName(4)),
		grn, not_empty(obj->get_PName(5)),
		grn, not_empty(obj->get_description()),
		grn, yel, not_empty(obj->get_action_description(), "<not set>\r\n"),
		grn, nrm, cyn, buf1, grn, nrm, cyn, buf2);
	// * Send first half.
	send_to_char(buf, d->character.get());

	sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf1);
	obj->get_no_flags().sprintbits(no_bits, buf2, ",");
	sprintf(buf,
		"%sC%s) Одевается  : %s%s\r\n"
		"%sD%s) Неудобен    : %s%s\r\n", grn, nrm, cyn, buf1, grn, nrm, cyn, buf2);
	send_to_char(buf, d->character.get());

	obj->get_anti_flags().sprintbits(anti_bits, buf1, ",",4);
	obj->get_affect_flags().sprintbits(weapon_affects, buf2, ",",4);
	const size_t gender = static_cast<size_t>(to_underlying(GET_OBJ_SEX(obj)));
	sprintf(buf,
		"%sE%s) Запрещен    : %s%s\r\n"
		"%sF%s) Вес         : %s%8d   %sG%s) Цена        : %s%d\r\n"
		"%sH%s) Рента(снято): %s%8d   %sI%s) Рента(одето): %s%d\r\n"
		"%sJ%s) Мах.проч.   : %s%8d   %sK%s) Тек.проч    : %s%d\r\n"
		"%sL%s) Материал    : %s%s\r\n"
		"%sM%s) Таймер      : %s%8d   %sN%s) %s\r\n"
		"%sO%s) Values      : %s%d %d %d %d\r\n"
		"%sP%s) Аффекты     : %s%s\r\n"
		"%sR%s) Меню наводимых аффектов\r\n"
		"%sT%s) Меню экстраописаний\r\n"
		"%sS%s) Скрипт      : %s%s\r\n"
		"%sU%s) Пол         : %s%s\r\n"
		"%sV%s) Макс.в мире : %s%d\r\n"
		"%sW%s) Меню умений\r\n"
		"%sX%s) Требует перевоплощений: %s%d\r\n"
		"%sZ%s) Клонирование\r\n"
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
		grn, nrm, print_values2_menu(obj).c_str(),
		grn, nrm, cyn,
		GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
		GET_OBJ_VAL(obj, 3), grn, nrm, grn, buf2, grn, nrm, grn, nrm, grn,
		nrm, cyn, !obj->get_proto_script().empty() ? "Set." : "Not Set.",
		grn, nrm, cyn, genders[gender],
		grn, nrm, cyn, GET_OBJ_MIW(obj),
		grn, nrm,
		grn, nrm, cyn, obj->get_minimum_remorts(),
		grn, nrm,
		grn, nrm);
	send_to_char(buf, d->character.get());
	OLC_MODE(d) = OEDIT_MAIN_MENU;
}

// ***************************************************************************
// * main loop (of sorts).. basically interpreter throws all input to here   *
// ***************************************************************************
int planebit(const char *str, int *plane, int *bit)
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

void check_potion_proto(OBJ_DATA *obj)
{
	if (obj->get_value(ObjVal::EValueKey::POTION_SPELL1_NUM) > 0
		|| obj->get_value(ObjVal::EValueKey::POTION_SPELL2_NUM) > 0
		|| obj->get_value(ObjVal::EValueKey::POTION_SPELL3_NUM) > 0 )
	{
		obj->set_value(ObjVal::EValueKey::POTION_PROTO_VNUM, 0);
	}
	else
	{
		obj->set_value(ObjVal::EValueKey::POTION_PROTO_VNUM, -1);
	}
}

bool parse_val_spell_num(DESCRIPTOR_DATA *d, const ObjVal::EValueKey key, int val)
{
	if (val < 1
		|| val > SPELLS_COUNT)
	{
		if (val != 0)
		{
			send_to_char("Неверный выбор.\r\n", d->character.get());
		}
		OLC_OBJ(d)->set_value(key, -1);
		check_potion_proto(OLC_OBJ(d));
		OLC_MODE(d) = OEDIT_DRINKCON_VALUES;
		drinkcon_values_menu(d);
		return false;
	}
	OLC_OBJ(d)->set_value(key, val);
	send_to_char(d->character.get(), "Выбранное заклинание: %s\r\n"
		"Ведите уровень заклинания от 1 до 50 (0 - выход) :",
		spell_name(val));
	return true;
}

void parse_val_spell_lvl(DESCRIPTOR_DATA *d, const ObjVal::EValueKey key, int val)
{
	if (val <= 0 || val > 50)
	{
		if (val != 0)
		{
			send_to_char("Некорректный уровень заклинания.\r\n", d->character.get());
		}

		switch (key)
		{
		case ObjVal::EValueKey::POTION_SPELL1_LVL:
			OLC_OBJ(d)->set_value(ObjVal::EValueKey::POTION_SPELL1_NUM, -1);
			break;

		case ObjVal::EValueKey::POTION_SPELL2_LVL:
			OLC_OBJ(d)->set_value(ObjVal::EValueKey::POTION_SPELL2_NUM, -1);
			break;

		case ObjVal::EValueKey::POTION_SPELL3_LVL:
			OLC_OBJ(d)->set_value(ObjVal::EValueKey::POTION_SPELL3_NUM, -1);
			break;

		default:
			break;
		}

		check_potion_proto(OLC_OBJ(d));
		OLC_MODE(d) = OEDIT_DRINKCON_VALUES;
		drinkcon_values_menu(d);

		return;
	}
	OLC_OBJ(d)->set_value(key, val);
	check_potion_proto(OLC_OBJ(d));
	OLC_MODE(d) = OEDIT_DRINKCON_VALUES;
	drinkcon_values_menu(d);
}

void oedit_disp_clone_menu(DESCRIPTOR_DATA* d)
{
	get_char_cols(d->character.get());

	sprintf(buf,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
		"%s1%s) Заменить триггеры\r\n"
		"%s2%s) Не заменять триггеры\r\n"
		"%s3%s) Quit\r\n"
		"Ваш выбор : ",
		grn, nrm,
		grn, nrm,
		grn, nrm);

	send_to_char(buf, d->character.get());
}

void oedit_parse(DESCRIPTOR_DATA * d, char *arg)
{
	int number = 0;
	int max_val, min_val, plane, bit;

	switch (OLC_MODE(d))
	{
	case OEDIT_CONFIRM_SAVESTRING:
		switch (*arg)
		{
		case 'y':
		case 'Y':
		case 'д':
		case 'Д':
			send_to_char("Saving object to memory.\r\n", d->character.get());
			OLC_OBJ(d)->remove_incorrect_values_keys(GET_OBJ_TYPE(OLC_OBJ(d)));
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
			send_to_char("Неверный выбор!\r\n", d->character.get());
			send_to_char("Вы хотите сохранить этот предмет?\r\n", d->character.get());
			break;
		}
		return;

	case OEDIT_MAIN_MENU:
		// * Throw us out to whichever edit mode based on user input.
		switch (*arg)
		{
		case 'q':
		case 'Q':
			if (OLC_VAL(d))  	// Something has been modified.
			{
				send_to_char("Вы хотите сохранить этот предмет? : ", d->character.get());
				OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
			}
			else
			{
				cleanup_olc(d, CLEANUP_ALL);
			}
			return;

		case '1':
			send_to_char("Введите синонимы : ", d->character.get());
			OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
			break;

		case '2':
			send_to_char(d->character.get(), "&S%s&s\r\nИменительный падеж [это ЧТО] : ", OLC_OBJ(d)->get_PName(0).c_str());
			OLC_MODE(d) = OEDIT_PAD0;
			break;

		case '3':
			send_to_char(d->character.get(), "&S%s&s\r\nРодительный падеж [нет ЧЕГО] : ", OLC_OBJ(d)->get_PName(1).c_str());
			OLC_MODE(d) = OEDIT_PAD1;
			break;

		case '4':
			send_to_char(d->character.get(), "&S%s&s\r\nДательный падеж [прикрепить к ЧЕМУ] : ", OLC_OBJ(d)->get_PName(2).c_str());
			OLC_MODE(d) = OEDIT_PAD2;
			break;

		case '5':
			send_to_char(d->character.get(), "&S%s&s\r\nВинительный падеж [держать ЧТО] : ", OLC_OBJ(d)->get_PName(3).c_str());
			OLC_MODE(d) = OEDIT_PAD3;
			break;

		case '6':
			send_to_char(d->character.get(), "&S%s&s\r\nТворительный падеж [вооружиться ЧЕМ] : ", OLC_OBJ(d)->get_PName(4).c_str());
			OLC_MODE(d) = OEDIT_PAD4;
			break;
		case '7':
			send_to_char(d->character.get(), "&S%s&s\r\nПредложный падеж [писать на ЧЕМ] : ", OLC_OBJ(d)->get_PName(5).c_str());
			OLC_MODE(d) = OEDIT_PAD5;
			break;

		case '8':
			send_to_char(d->character.get(), "&S%s&s\r\nВведите длинное описание :-\r\n| ", OLC_OBJ(d)->get_description().c_str());
			OLC_MODE(d) = OEDIT_LONGDESC;
			break;

		case '9':
			OLC_MODE(d) = OEDIT_ACTDESC;
			SEND_TO_Q("Введите описание при применении: (/s сохранить /h помощь)\r\n\r\n", d);
			d->backstr = NULL;
			if (!OLC_OBJ(d)->get_action_description().empty())
			{
				SEND_TO_Q(OLC_OBJ(d)->get_action_description().c_str(), d);
				d->backstr = str_dup(OLC_OBJ(d)->get_action_description().c_str());
			}
			d->writer.reset(new CActionDescriptionWriter(*OLC_OBJ(d)));
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
			send_to_char("Вес предмета : ", d->character.get());
			OLC_MODE(d) = OEDIT_WEIGHT;
			break;

		case 'g':
		case 'G':
			send_to_char("Цена предмета : ", d->character.get());
			OLC_MODE(d) = OEDIT_COST;
			break;

		case 'h':
		case 'H':
			send_to_char("Рента предмета (в инвентаре) : ", d->character.get());
			OLC_MODE(d) = OEDIT_COSTPERDAY;
			break;

		case 'i':
		case 'I':
			send_to_char("Рента предмета (в экипировке) : ", d->character.get());
			OLC_MODE(d) = OEDIT_COSTPERDAYEQ;
			break;

		case 'j':
		case 'J':
			send_to_char("Максимальная прочность : ", d->character.get());
			OLC_MODE(d) = OEDIT_MAXVALUE;
			break;

		case 'k':
		case 'K':
			send_to_char("Текущая прочность : ", d->character.get());
			OLC_MODE(d) = OEDIT_CURVALUE;
			break;

		case 'l':
		case 'L':
			oedit_disp_mater_menu(d);
			OLC_MODE(d) = OEDIT_MATER;
			break;

		case 'm':
		case 'M':
			send_to_char("Таймер (в минутах РЛ) : ", d->character.get());
			OLC_MODE(d) = OEDIT_TIMER;
			break;

		case 'n':
		case 'N':
			if (GET_OBJ_TYPE(OLC_OBJ(d)) == OBJ_DATA::ITEM_WEAPON
				|| GET_OBJ_TYPE(OLC_OBJ(d)) == OBJ_DATA::ITEM_INGREDIENT)
			{
				oedit_disp_skills_menu(d);
				OLC_MODE(d) = OEDIT_SKILL;
			}
			else if (GET_OBJ_TYPE(OLC_OBJ(d)) == OBJ_DATA::ITEM_DRINKCON
				|| GET_OBJ_TYPE(OLC_OBJ(d)) == OBJ_DATA::ITEM_FOUNTAIN)
			{
				drinkcon_values_menu(d);
				OLC_MODE(d) = OEDIT_DRINKCON_VALUES;
			}
			else
			{
				oedit_disp_menu(d);
			}
			break;

		case 'o':
		case 'O':
			// * Clear any old values
			OLC_OBJ(d)->set_val(0, 0);
			OLC_OBJ(d)->set_val(1, 0);
			OLC_OBJ(d)->set_val(2, 0);
			OLC_OBJ(d)->set_val(3, 0);
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
			// * If extra descriptions don't exist.
			if (!OLC_OBJ(d)->get_ex_description())
			{
				OLC_OBJ(d)->set_ex_description(new EXTRA_DESCR_DATA());
			}
			OLC_DESC(d) = OLC_OBJ(d)->get_ex_description();
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
			send_to_char("Пол : ", d->character.get());
			OLC_MODE(d) = OEDIT_SEXVALUE;
			break;

		case 'v':
		case 'V':
			send_to_char("Максимальное число в мире : ", d->character.get());
			OLC_MODE(d) = OEDIT_MIWVALUE;
			break;

		case 'w':
		case 'W':
			oedit_disp_skills_mod_menu(d);
			OLC_MODE(d) = OEDIT_SKILLS;
			break;

		case 'x':
		case 'X':
			send_to_char("Требует перевоплощений (-1 выключено, 0 автопростановка, в - до какого морта, в + после какого): ", d->character.get());
			OLC_MODE(d) = OEDIT_MORT_REQ;
			break;

		case 'z':
		case 'Z':
			OLC_MODE(d) = OEDIT_CLONE;
			oedit_disp_clone_menu(d);
			break;

		default:
			oedit_disp_menu(d);
			break;
		}
		olc_log("%s command %c", GET_NAME(d->character), *arg);
		return;
		// * end of OEDIT_MAIN_MENU

	case OLC_SCRIPT_EDIT:
		if (dg_script_edit_parse(d, arg))
		{
			return;
		}
		break;

	case OEDIT_EDIT_NAMELIST:
		OLC_OBJ(d)->set_aliases(not_null(arg, NULL));
		break;

	case OEDIT_PAD0:
		OLC_OBJ(d)->set_short_description(not_null(arg, "что-то"));
		OLC_OBJ(d)->set_PName(0, not_null(arg, "что-то"));
		break;

	case OEDIT_PAD1:
		OLC_OBJ(d)->set_PName(1, not_null(arg, "-чего-то"));
		break;

	case OEDIT_PAD2:
		OLC_OBJ(d)->set_PName(2, not_null(arg, "-чему-то"));
		break;

	case OEDIT_PAD3:
		OLC_OBJ(d)->set_PName(3, not_null(arg, "-что-то"));
		break;

	case OEDIT_PAD4:
		OLC_OBJ(d)->set_PName(4, not_null(arg, "-чем-то"));
		break;

	case OEDIT_PAD5:
		OLC_OBJ(d)->set_PName(5, not_null(arg, "-чем-то"));
		break;

	case OEDIT_LONGDESC:
		OLC_OBJ(d)->set_description(not_null(arg, "неопределено"));
		break;

	case OEDIT_TYPE:
		number = atoi(arg);
		if ((number < 1) || (number >= NUM_ITEM_TYPES))
		{
			send_to_char("Invalid choice, try again : ", d->character.get());
			return;
		}
		else
		{
			OLC_OBJ(d)->set_type(static_cast<OBJ_DATA::EObjectType>(number));
			sprintf(buf, "%s  меняет тип предмета для %d!!!", GET_NAME(d->character), OLC_NUM(d));
			mudlog(buf, BRF, LVL_GOD, SYSLOG, TRUE);
			if (number != OBJ_DATA::ITEM_WEAPON
				&& number != OBJ_DATA::ITEM_INGREDIENT)
			{
				OLC_OBJ(d)->set_skill(SKILL_INVALID);
			}
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
		{
			break;
		}
		else
		{
			OLC_OBJ(d)->toggle_extra_flag(plane, 1 << bit);
			oedit_disp_extra_menu(d);
			return;
		}

	case OEDIT_WEAR:
		number = atoi(arg);
		if ((number < 0) || (number > NUM_ITEM_WEARS))
		{
			send_to_char("Неверный выбор!\r\n", d->character.get());
			oedit_disp_wear_menu(d);
			return;
		}
		else if (number == 0)	// Quit.
		{
			break;
		}
		else
		{
			OLC_OBJ(d)->toggle_wear_flag(1 << (number - 1));
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
		{
			break;
		}
		else
		{
			OLC_OBJ(d)->toggle_no_flag(plane, 1 << bit);
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
		{
			break;
		}
		else
		{
			OLC_OBJ(d)->toggle_anti_flag(plane, 1 << bit);
			oedit_disp_anti_menu(d);
			return;
		}


	case OEDIT_WEIGHT:
		OLC_OBJ(d)->set_weight(atoi(arg));
		break;

	case OEDIT_COST:
		OLC_OBJ(d)->set_cost(atoi(arg));
		break;

	case OEDIT_COSTPERDAY:
		OLC_OBJ(d)->set_rent_off(atoi(arg));
		break;

	case OEDIT_MAXVALUE:
		OLC_OBJ(d)->set_maximum_durability(atoi(arg));
		break;

	case OEDIT_CURVALUE:
		OLC_OBJ(d)->set_current_durability(MIN(GET_OBJ_MAX(OLC_OBJ(d)), atoi(arg)));
		break;

	case OEDIT_SEXVALUE:
		if ((number = atoi(arg)) >= 0
			&& number < NUM_SEXES)
		{
			OLC_OBJ(d)->set_sex(static_cast<ESex>(number));
		}
		else
		{
			send_to_char("Пол (0-3) : ", d->character.get());
			return;
		}
		break;

	case OEDIT_MIWVALUE:
		if ((number = atoi(arg)) >= -1 && number <= 10000)
		{
			OLC_OBJ(d)->set_max_in_world(number);
		}
		else
		{
			send_to_char("Максимальное число предметов в мире (0-10000 или -1) : ", d->character.get());
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
		{
			OLC_OBJ(d)->set_material(static_cast<OBJ_DATA::EObjectMaterial>(number - 1));
		}
		break;

	case OEDIT_COSTPERDAYEQ:
		OLC_OBJ(d)->set_rent_on(atoi(arg));
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
		{
			break;
		}
		if (GET_OBJ_TYPE(OLC_OBJ(d)) == OBJ_DATA::ITEM_INGREDIENT)
		{
			OLC_OBJ(d)->toggle_skill(1 << (number - 1));
			oedit_disp_skills_menu(d);
			return;
		}
		if (GET_OBJ_TYPE(OLC_OBJ(d)) == OBJ_DATA::ITEM_WEAPON)
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
		OLC_OBJ(d)->set_skill(number);
		oedit_disp_skills_menu(d);
		return;
		break;

	case OEDIT_VALUE_1:
		// * Lucky, I don't need to check any of these for out of range values.
		// * Hmm, I'm not so sure - Rv
		number = atoi(arg);

		if (GET_OBJ_TYPE(OLC_OBJ(d)) == OBJ_DATA::ITEM_BOOK
			&& (number < 0
				|| number > 4))
		{
			send_to_char("Неправильный тип книги, повторите.\r\n", d->character.get());
			oedit_disp_val1_menu(d);
			return;
		}
		// * proceed to menu 2
		OLC_OBJ(d)->set_val(0, number);
		OLC_VAL(d) = 1;
		oedit_disp_val2_menu(d);
		return;

	case OEDIT_VALUE_2:
		// * Here, I do need to check for out of range values.
		number = atoi(arg);
		switch (GET_OBJ_TYPE(OLC_OBJ(d)))
		{
		case OBJ_DATA::ITEM_SCROLL:
		case OBJ_DATA::ITEM_POTION:
			if (number < 1
				|| number > SPELLS_COUNT)
			{
				oedit_disp_val2_menu(d);
			}
			else
			{
				OLC_OBJ(d)->set_val(1, number);
				oedit_disp_val3_menu(d);
			}
			return;

		case OBJ_DATA::ITEM_CONTAINER:
			// Needs some special handling since we are dealing with flag values
			// here.
			if (number < 0
				|| number > 4)
			{
				oedit_disp_container_flags_menu(d);
			}
			else if (number != 0)
			{
				OLC_OBJ(d)->toggle_val_bit(1, 1 << (number - 1));
				OLC_VAL(d) = 1;
				oedit_disp_val2_menu(d);
			}
			else
			{
				oedit_disp_val3_menu(d);
			}
			return;

		case OBJ_DATA::ITEM_BOOK:
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
					send_to_char("Неизвестное заклинание, повторите.\r\n", d->character.get());
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
				if (number > MAX_SKILL_NUM
					|| !skill_info[number].name
					|| *skill_info[number].name == '!')
				{
					send_to_char("Неизвестное умение, повторите.\r\n", d->character.get());
					oedit_disp_val2_menu(d);
					return;
				}
				break;

			case BOOK_RECPT:
				if (number > top_imrecipes || number < 0  || !imrecipes[number].name)
				{
					send_to_char("Неизвестный рецепт, повторите.\r\n", d->character.get());
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
				if (number <= 0 
					|| number >= MAX_FEATS 
					|| !feat_info[number].name 
					|| *feat_info[number].name == '!')
				{
					send_to_char("Неизвестная способность, повторите.\r\n", d->character.get());
					oedit_disp_val2_menu(d);
					return;
				}
				break;
			}
		default:
			break;
		}
		OLC_OBJ(d)->set_val(1, number);
		OLC_VAL(d) = 1;
		oedit_disp_val3_menu(d);
		return;

	case OEDIT_VALUE_3:
		number = atoi(arg);
		// * Quick'n'easy error checking.
		switch (GET_OBJ_TYPE(OLC_OBJ(d)))
		{
		case OBJ_DATA::ITEM_SCROLL:
		case OBJ_DATA::ITEM_POTION:
			min_val = 0;
			max_val = SPELLS_COUNT;
			break;

		case OBJ_DATA::ITEM_WEAPON:
			min_val = 1;
			max_val = 50;
			break;

		case OBJ_DATA::ITEM_WAND:
		case OBJ_DATA::ITEM_STAFF:
			min_val = 0;
			max_val = 20;
			break;

		case OBJ_DATA::ITEM_DRINKCON:
		case OBJ_DATA::ITEM_FOUNTAIN:
			min_val = 0;
			max_val = NUM_LIQ_TYPES - 1;
			break;

		case OBJ_DATA::ITEM_MATERIAL:
			min_val = 0;
			max_val = 1000;
			break;

		default:
			min_val = -999999;
			max_val = 999999;
		}
		OLC_OBJ(d)->set_val(2, MAX(min_val, MIN(number, max_val)));
		OLC_VAL(d) = 1;
		oedit_disp_val4_menu(d);
		return;

	case OEDIT_VALUE_4:
		number = atoi(arg);
		switch (GET_OBJ_TYPE(OLC_OBJ(d)))
		{
		case OBJ_DATA::ITEM_SCROLL:
		case OBJ_DATA::ITEM_POTION:
			min_val = 0;
			max_val = SPELLS_COUNT;
			break;

		case OBJ_DATA::ITEM_WAND:
		case OBJ_DATA::ITEM_STAFF:
			min_val = 1;
			max_val = SPELLS_COUNT;
			break;

		case OBJ_DATA::ITEM_WEAPON:
			min_val = 0;
			max_val = NUM_ATTACK_TYPES - 1;
			break;
		case OBJ_DATA::ITEM_MING:
			min_val = 0;
			max_val = 2;
			break;

		case OBJ_DATA::ITEM_MATERIAL:
			min_val = 0;
			max_val = 100;
			break;

		default:
			min_val = -999999;
			max_val = 999999;
			break;
		}
		OLC_OBJ(d)->set_val(3, MAX(min_val, MIN(number, max_val)));
		break;

	case OEDIT_AFFECTS:
		number = planebit(arg, &plane, &bit);
		if (number < 0)
		{
			oedit_disp_affects_menu(d);
			return;
		}
		else if (number == 0)
		{
			break;
		}
		else
		{
			OLC_OBJ(d)->toggle_affect_flag(plane, 1 << bit);
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
			OLC_OBJ(d)->set_affected(OLC_VAL(d), EApplyLocation::APPLY_NONE, 0);
			oedit_disp_prompt_apply_menu(d);
		}
		else if (number < 0 || number >= NUM_APPLIES)
		{
			oedit_disp_apply_menu(d);
		}
		else
		{
			OLC_OBJ(d)->set_affected_location(OLC_VAL(d), static_cast<EApplyLocation>(number));
			send_to_char("Modifier : ", d->character.get());
			OLC_MODE(d) = OEDIT_APPLYMOD;
		}
		return;

	case OEDIT_APPLYMOD:
		OLC_OBJ(d)->set_affected_modifier(OLC_VAL(d), atoi(arg));
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
				OLC_DESC(d).reset();
				OLC_OBJ(d)->set_ex_description(nullptr);
			}
			break;

		case 1:
			OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
			send_to_char("Enter keywords, separated by spaces :-\r\n| ", d->character.get());
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
			d->writer.reset(new DelegatedStringWriter(OLC_DESC(d)->description));
			d->max_str = 4096;
			d->mail_to = 0;
			OLC_VAL(d) = 1;
			return;

		case 3:
			// * Only go to the next description if this one is finished.
			if (OLC_DESC(d)->keyword && OLC_DESC(d)->description)
			{
				if (OLC_DESC(d)->next)
				{
					OLC_DESC(d) = OLC_DESC(d)->next;
				}
				else  	// Make new extra description and attach at end.
				{
					EXTRA_DESCR_DATA::shared_ptr new_extra(new EXTRA_DESCR_DATA());
					OLC_DESC(d)->next = new_extra;
					OLC_DESC(d) = OLC_DESC(d)->next;
				}
			}
			// * No break - drop into default case.
		default:
			oedit_disp_extradesc_menu(d);
			return;
		}
		break;

	case OEDIT_SKILLS:
		number = atoi(arg);
		if (number == 0)
		{
			break;
		}
		if (number > MAX_SKILL_NUM
			|| number < 0
			|| !skill_info[number].name
			|| *skill_info[number].name == '!')
		{
			send_to_char("Неизвестное умение.\r\n", d->character.get());
		}
		else if (OLC_OBJ(d)->get_skill(number) != 0)
		{
			OLC_OBJ(d)->set_skill(number, 0);
		}
		else if (sscanf(arg, "%d %d", &plane, &bit) < 2)
		{
			send_to_char("Не указан уровень владения умением.\r\n", d->character.get());
		}
		else
		{
			OLC_OBJ(d)->set_skill(number, (MIN(200, MAX(-200, bit))));
		}
		oedit_disp_skills_mod_menu(d);
		return;

	case OEDIT_MORT_REQ:
		number = atoi(arg);
		OLC_OBJ(d)->set_minimum_remorts(number);
		break;

	case OEDIT_DRINKCON_VALUES:
		switch(number = atoi(arg))
		{
		case 0:
			break;
		case 1:
			OLC_MODE(d) = OEDIT_POTION_SPELL1_NUM;
			oedit_disp_spells_menu(d);
			return;
		case 2:
			OLC_MODE(d) = OEDIT_POTION_SPELL2_NUM;
			oedit_disp_spells_menu(d);
			return;
		case 3:
			OLC_MODE(d) = OEDIT_POTION_SPELL3_NUM;
			oedit_disp_spells_menu(d);
			return;
		default:
			send_to_char("Неверный выбор.\r\n", d->character.get());
			drinkcon_values_menu(d);
			return;
		}
		break;
	case OEDIT_POTION_SPELL1_NUM:
		number = atoi(arg);
		if (parse_val_spell_num(d, ObjVal::EValueKey::POTION_SPELL1_NUM, number))
		{
			OLC_MODE(d) = OEDIT_POTION_SPELL1_LVL;
		}
		return;
	case OEDIT_POTION_SPELL2_NUM:
		number = atoi(arg);
		if (parse_val_spell_num(d, ObjVal::EValueKey::POTION_SPELL2_NUM, number))
		{
			OLC_MODE(d) = OEDIT_POTION_SPELL2_LVL;
		}
		return;
	case OEDIT_POTION_SPELL3_NUM:
		number = atoi(arg);
		if (parse_val_spell_num(d, ObjVal::EValueKey::POTION_SPELL3_NUM, number))
		{
			OLC_MODE(d) = OEDIT_POTION_SPELL3_LVL;
		}
		return;
	case OEDIT_POTION_SPELL1_LVL:
		number = atoi(arg);
		parse_val_spell_lvl(d, ObjVal::EValueKey::POTION_SPELL1_LVL, number);
		return;
	case OEDIT_POTION_SPELL2_LVL:
		number = atoi(arg);
		parse_val_spell_lvl(d, ObjVal::EValueKey::POTION_SPELL2_LVL, number);
		return;
	case OEDIT_POTION_SPELL3_LVL:
		number = atoi(arg);
		parse_val_spell_lvl(d, ObjVal::EValueKey::POTION_SPELL3_LVL, number);
		return;
	case OEDIT_CLONE:
		switch (*arg)
		{
		case '1':
			OLC_MODE(d) = OEDIT_CLONE_WITH_TRIGGERS;
			send_to_char("Введите VNUM объекта для клонирования:", d->character.get());
			return;
		case '2':
			OLC_MODE(d) = OEDIT_CLONE_WITHOUT_TRIGGERS;
			send_to_char("Введите VNUM объекта для клонирования:", d->character.get());
			return;
		case '3':
			break;	//to main menu
		default:
			oedit_disp_clone_menu(d);
			return;
		}
		break;
	case OEDIT_CLONE_WITH_TRIGGERS:
	{
		number = atoi(arg);
		const int rnum_object = real_object((OLC_OBJ(d)->get_vnum()));
		if (!OLC_OBJ(d)->clone_olc_object_from_prototype(number))
		{
			send_to_char("Нет объекта с таким внумом. Повторите ввод : ", d->character.get());
			return;
		}
		if (rnum_object >= 0)
			OLC_OBJ(d)->set_rnum(rnum_object);
		break;
	}
	case OEDIT_CLONE_WITHOUT_TRIGGERS:
	{
		number = atoi(arg);

		auto proto_script_old = OLC_OBJ(d)->get_proto_script();
		const int rnum_object = real_object((OLC_OBJ(d)->get_vnum()));

		if (!OLC_OBJ(d)->clone_olc_object_from_prototype(number))
		{
			send_to_char("Нет объекта с таким внумом. Повторите ввод: :", d->character.get());
			return;
		}
		if (rnum_object >= 0)
			OLC_OBJ(d)->set_rnum(rnum_object);
		OLC_OBJ(d)->set_proto_script(proto_script_old);
		break;
	}
	default:
		mudlog("SYSERR: OLC: Reached default case in oedit_parse()!", BRF, LVL_BUILDER, SYSLOG, TRUE);
		send_to_char("Oops...\r\n", d->character.get());
		break;
	}

	// * If we get here, we have changed something.
	OLC_VAL(d) = 1;
	oedit_disp_menu(d);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
